// A document that starts with a UTF-8 byte order mark, as Windows
// PowerShell 5.1's Set-Content -Encoding utf8, Visual Studio and some
// editors write it. The mark is not text: the first line reads as Markdown,
// task boxes and review notes still write the right bytes, and the editor
// keeps the mark out of its text and writes it back with every save.
#include "annotations.h"
#include "d2d_init.h"
#include "drafts.h"
#include "editor.h"
#include "frame_preview.h"
#include "input.h"
#include "keystroke.h"
#include "render.h"
#include "selection.h"
#include "settings.h"
#include "tabs.h"
#include "utils.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

namespace {
namespace fs = std::filesystem;

int failures = 0;
void check(bool ok, const std::string& message) {
    if (!ok) { if (failures < 60) std::cerr << "FAIL byte order mark: " << message << '\n'; ++failures; }
}

const std::string kBom = "\xEF\xBB\xBF";

std::string readBytes(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

void writeBytes(const fs::path& path, const std::string& bytes) {
    std::ofstream(path, std::ios::binary) << bytes;
}

// The text with every line break written as `eol`
std::string withEol(const std::string& text, const std::string& eol) {
    std::string out;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\r' && i + 1 < text.size() && text[i + 1] == '\n') continue;
        if (text[i] == '\n') out += eol;
        else out += text[i];
    }
    return out;
}

std::string plainText(const qmd::ElementPtr& element) {
    std::string text = element->type == qmd::ElementType::Text ? element->text : std::string();
    for (const auto& child : element->children) text += plainText(child);
    return text;
}

bool isBlock(const qmd::ElementPtr& block, qmd::ElementType type, const std::string& text) {
    return block && block->type == type && plainText(block) == text;
}

void layOut(App& app) {
    ensureLayoutComplete(app);
    ensureLayoutComplete(app);  // the first pass may stop two screens down
}

bool open(App& app, const fs::path& path) {
    app.scrollY = app.targetScrollY = 0.0f;
    const bool opened = openDocumentInViewer(app, path.wstring());
    layOut(app);
    return opened;
}

// The top of the reading view as drawn, saved beside the test for inspection
void savePreview(App& app, const std::string& name, const std::string& where) {
    app.scrollY = app.targetScrollY = 0.0f;
    check(saveFramePreview(app, fs::current_path() / (name + ".png"), [] {}),
          "a preview of the reading view is saved" + where);
}

// A press and release on a document point, as the hand clicks: never the
// second half of a double click
void clickDocument(App& app, float docX, float docY) {
    app.scrollY = app.targetScrollY =
        std::clamp(docY - (float)app.height / 2.0f, 0.0f, std::max(0.0f, app.contentHeight - (float)app.height));
    const LPARAM point = MAKELPARAM((int)std::lround(docX + documentViewportX(app) - app.scrollX),
                                    (int)std::lround(docY - app.scrollY));
    app.lastClickTime = {};
    handleMouseMove(app, app.hwnd, point);
    handleMouseDown(app, app.hwnd, MK_LBUTTON, point);
    handleMouseUp(app, app.hwnd, 0, point);
}

// The write a click makes reloads the view at once, even within the file
// time's resolution of the last write
void clickTask(App& app, size_t index) {
    if (index >= app.taskRects.size()) return;
    const auto rect = app.taskRects[index].rect;
    app.lastFileWriteTime = {};
    clickDocument(app, (rect.left + rect.right) / 2.0f, (rect.top + rect.bottom) / 2.0f);
    layOut(app);
}

// Every heading's text, read at its offset in the source the view holds
bool headingOffsetsPointAtText(const App& app) {
    bool ok = true;
    int headings = 0;
    std::function<void(const qmd::ElementPtr&)> visit = [&](const qmd::ElementPtr& element) {
        if (element->type == qmd::ElementType::Heading) {
            const std::string text = plainText(element);
            ok = ok && element->sourceOffset != SIZE_MAX &&
                 app.sourceText.compare(element->sourceOffset, text.size(), text) == 0;
            ++headings;
        }
        for (const auto& child : element->children) visit(child);
    };
    if (app.root) visit(app.root);
    return ok && headings > 0;
}

// The reading view lays the marked file out as the same file without the
// mark: the first line is the title, and every heading sits where it does
// there
void readingView(App& app, const fs::path& marked, const fs::path& twin, const std::string& where,
                 const std::string& preview) {
    check(open(app, twin), "the file without the mark opens" + where);
    const auto expected = app.headings;
    const float expectedHeight = app.contentHeight;
    check(open(app, marked), "the marked file opens" + where);
    savePreview(app, preview, where);
    const auto& blocks = app.root->children;
    check(!blocks.empty() && isBlock(blocks[0], qmd::ElementType::Heading, "Byte order mark") &&
              blocks[0]->level == 1,
          "the first line is a level 1 heading, not the text \"# Byte order mark\"" + where);
    bool same = app.headings.size() == expected.size();
    for (size_t i = 0; same && i < expected.size(); ++i) {
        same = app.headings[i].text == expected[i].text && app.headings[i].level == expected[i].level &&
               std::abs(app.headings[i].y - expected[i].y) < 0.5f;
    }
    check(same && std::abs(app.contentHeight - expectedHeight) < 0.5f,
          "the headings sit where they do without the mark" + where);
    check(app.sourceText.rfind(kBom, 0) == 0 && headingOffsetsPointAtText(app),
          "source offsets count the mark: each heading's offset points at its text" + where);
}

// A click on a task box flips its mark in the file: the right byte, with
// the byte order mark and the line breaks left as they were
void taskBoxes(App& app, const fs::path& path, const std::string& original, const std::string& where) {
    writeBytes(path, original);
    check(open(app, path), "the task file opens" + where);
    check(app.taskRects.size() == 3, "the three tasks lay out" + where);
    bool marks = !app.taskRects.empty();
    for (const auto& task : app.taskRects) {
        const size_t at = task.markOffset;
        marks = marks && at > 0 && at + 1 < app.sourceText.size() && app.sourceText[at - 1] == '[' &&
                app.sourceText[at + 1] == ']' && app.sourceText[at] == (task.checked ? 'x' : ' ');
    }
    check(marks, "each task's offset points at its mark in the source" + where);

    std::string flipped = original;
    const size_t first = flipped.find("- [ ] Click this box");
    if (first != std::string::npos) flipped[first + 3] = 'x';
    clickTask(app, 0);
    check(readBytes(path) == flipped, "clicking the first box checks it in the file, nothing else changes" + where);
    check(app.taskRects.size() == 3 && app.taskRects[0].checked && !app.root->children.empty() &&
              app.root->children[0]->type == qmd::ElementType::Heading,
          "the view reloads with the box checked and the title still a heading" + where);
    clickTask(app, 0);
    check(readBytes(path) == original, "a second click clears it again" + where);

    std::string last = original;
    const size_t lastTask = last.find("- [ ] A last task");
    if (lastTask != std::string::npos) last[lastTask + 3] = 'x';
    clickTask(app, 2);
    check(readBytes(path) == last, "the last box flips its own mark" + where);
    writeBytes(path, original);
}

// A review note on the title lands on the line after it, and the file
// keeps its byte order mark
void reviewNote(App& app, const fs::path& path, const std::string& original, const std::string& where) {
    writeBytes(path, original);
    check(open(app, path), "the file opens for a review note" + where);
    // The title is the first text laid out
    const std::wstring title = L"Byte order mark";
    const size_t at = app.docText.find(title);
    app.selAnchor = at;
    app.selFocus = at + title.size();
    app.hasSelection = at == 0;
    check(at == 0 && selectionTextForRange(app, app.selAnchor, app.selFocus) == title &&
              annotationBeginCreate(app),
          "the title can be annotated" + where);
    if (!app.annotEditorOpen) return;
    app.annotEditorText = L"Checked";
    app.lastFileWriteTime = {};
    annotationEditorConfirm(app, app.hwnd);
    const std::string saved = readBytes(path);
    const std::string note = "<!-- review \"Byte order mark\": Checked -->";
    check(withEol(saved, "\n").rfind(kBom + "# Byte order mark\n" + note + "\n\nThis file starts", 0) == 0,
          "the note goes on the line after the title, after the mark" + where);
    check(app.annotations.size() == 1 && app.annotations[0].commentLine == 2 &&
              !app.root->children.empty() && app.root->children[0]->type == qmd::ElementType::Heading,
          "the view reloads with the note on line 2 and the title a heading" + where);
    writeBytes(path, original);
}

// The editor holds the text without the mark, and every save puts it back
// in front: the caret never sits before it, so nothing typed lands there
void editor(App& app, const fs::path& path, const std::string& original, const std::string& eol,
            const std::string& where) {
    const std::string body = original.substr(kBom.size());
    writeBytes(path, original);
    check(open(app, path), "the file opens for editing" + where);
    enterEditMode(app);
    check(app.editMode && app.editorText.rfind(L"# Byte order mark\n", 0) == 0,
          "the editor's text starts with the title, not the mark" + where);
    saveEditorFile(app, app.hwnd);
    check(readBytes(path) == original, "saving an untouched buffer writes the same bytes" + where);

    keystroke(app, VK_HOME, {true});
    typeKeys(app, L"Lead\n\n");
    check(app.editorText.rfind(L"Lead\n\n# Byte order mark\n", 0) == 0,
          "text typed at the very start goes in front of the title" + where);
    saveEditorFile(app, app.hwnd);
    check(readBytes(path) == kBom + "Lead" + eol + eol + body,
          "the save writes the mark first, then the typed text" + where);
    exitEditMode(app);
    layOut(app);
    const auto& blocks = app.root->children;
    check(blocks.size() > 1 && isBlock(blocks[0], qmd::ElementType::Paragraph, "Lead") &&
              isBlock(blocks[1], qmd::ElementType::Heading, "Byte order mark"),
          "the view shows the typed paragraph, then the title" + where);

    writeBytes(path, original);
    check(open(app, path), "the file opens for a rewrite" + where);
    enterEditMode(app);
    keystroke(app, 'A', {true});
    typeKeys(app, L"# Replaced\n");
    saveEditorFile(app, app.hwnd);
    check(readBytes(path) == kBom + "# Replaced" + eol, "replacing all the text keeps the mark" + where);
    exitEditMode(app);
    writeBytes(path, original);
}

// The edit-mode preview follows the source: each heading's anchor leads
// the editor to the heading's own line
void scrollSync(App& app, const fs::path& path, const std::string& original, const std::string& where) {
    writeBytes(path, original);
    check(open(app, path), "the file opens for scroll sync" + where);
    std::vector<std::pair<std::wstring, float>> reading;
    for (const auto& heading : app.headings) reading.push_back({heading.text, heading.y});

    // E at a heading opens the editor on that heading's line
    int opened = 0;
    for (const auto& [text, y] : reading) {
        if (y <= documentContentTop(app) + 1.0f) continue;
        app.scrollY = app.targetScrollY = y + 1.0f;
        enterEditMode(app);
        size_t line = 0;
        while (line + 1 < app.editorLineStarts.size() && app.editorLineStarts[line + 1] <= app.editorCursorPos) ++line;
        const size_t start = app.editorLineStarts[line];
        const size_t hashes = app.editorText.find(L' ', start);
        check(hashes != std::wstring::npos && app.editorText.compare(hashes + 1, text.size(), text) == 0,
              "E at \"" + toUtf8(text) + "\" opens the editor on its line" + where);
        ++opened;
        exitEditMode(app);
        layOut(app);
    }
    check(opened >= 5, "the headings below the title were tried" + where);

    // In the editor, each heading's anchor maps back to its line
    app.scrollY = app.targetScrollY = 0.0f;
    enterEditMode(app);
    app.editorShowPreview = true;
    editorReparse(app);
    layOut(app);
    int mapped = 0;
    for (const auto& heading : app.headings) {
        // The heading's block starts above its text, by the heading's margin
        const auto after = std::find_if(app.scrollAnchors.begin(), app.scrollAnchors.end(),
                                        [&](const App::ScrollAnchor& a) { return a.renderedY > heading.y + 0.5f; });
        if (after == app.scrollAnchors.begin()) continue;
        editorScrollToSourceOffset(app, std::prev(after)->sourceOffset);
        const size_t start = app.editorCursorPos;
        const std::wstring marker = std::wstring(heading.level, L'#') + L" " + heading.text;
        check(app.editorText.compare(start, marker.size(), marker) == 0,
              "the preview's \"" + toUtf8(heading.text) + "\" leads to its source line" + where);
        ++mapped;
    }
    check(mapped == (int)app.headings.size() && mapped >= 6, "every heading has an anchor" + where);
    exitEditMode(app);
}

// Front matter right after the mark: the properties strip shows with
// nothing between it and the heading, the editor sees the block, and a
// save that stamps the date keeps the mark
void frontMatter(App& app, const fs::path& marked, const fs::path& twin, const std::string& original,
                 const std::string& eol, const std::string& where, const std::string& preview) {
    writeBytes(marked, original);
    check(open(app, twin), "the front matter file without the mark opens" + where);
    const float headingY = app.headings.empty() ? -1.0f : app.headings.front().y;
    check(open(app, marked), "the front matter file opens" + where);
    savePreview(app, preview, where);
    const auto& blocks = app.root->children;
    bool title = false;
    if (!blocks.empty() && blocks[0]->type == qmd::ElementType::Properties) {
        for (const auto& property : blocks[0]->properties)
            title = title || (property.key == "title" && property.text == "Front matter after a byte order mark");
    }
    check(title, "the block after the mark becomes the properties strip" + where);
    check(blocks.size() > 1 && isBlock(blocks[1], qmd::ElementType::Heading, "Front matter after a byte order mark"),
          "the heading follows the strip with no empty paragraph between" + where);
    check(!app.headings.empty() && std::abs(app.headings.front().y - headingY) < 0.5f,
          "the heading sits where it does without the mark" + where);

    const fm::Settings saved = app.frontmatter;
    for (auto& rule : app.frontmatter.rules) rule.show = rule.key == "title" || rule.key == "updated";
    enterEditMode(app);
    check(app.editorText.rfind(L"---\ntitle: Front matter after a byte order mark\n", 0) == 0,
          "the editor's text opens with the front matter's fence" + where);
    saveEditorFile(app, app.hwnd);
    const std::string bytes = readBytes(marked);
    check(bytes.rfind(kBom + "---" + eol + "title: Front matter after a byte order mark" + eol, 0) == 0 &&
              bytes.find(eol + "updated: \"") != std::string::npos &&
              bytes.find("---" + eol + eol + "# Front matter after a byte order mark" + eol) != std::string::npos,
          "the save stamps the date inside the block and keeps the mark first" + where);
    exitEditMode(app);
    layOut(app);
    check(!app.root->children.empty() && fm::find(fm::parse(app.sourceText), "updated") &&
              app.root->children[0]->type == qmd::ElementType::Properties,
          "the reloaded view reads the stamped block" + where);
    app.frontmatter = saved;
    writeBytes(marked, original);
}

std::vector<fs::path> draftFiles() {
    std::vector<fs::path> drafts;
    const std::wstring prefix = L"draft-" + std::to_wstring(GetCurrentProcessId()) + L"-";
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(fs::path(tintaConfigDir()) / L"drafts", ec))
        if (entry.path().filename().wstring().rfind(prefix, 0) == 0) drafts.push_back(entry.path());
    return drafts;
}

// An edit buffer parked in a tab, and its crash-recovery draft, keep the
// mark for the save that follows
void parkedAndDrafted(App& app, const fs::path& path, const fs::path& other, const std::string& original,
                      const std::string& where) {
    const std::string body = original.substr(kBom.size());
    writeBytes(path, original);
    app.tabs.clear();
    check(open(app, path), "the file opens in a tab" + where);
    tabsInit(app);
    enterEditMode(app);
    keystroke(app, VK_HOME, {true});
    typeKeys(app, L"Parked ");
    tabOpenPath(app, app.hwnd, toUtf8(other.wstring()));
    check(app.tabs.size() == 2 && app.activeTab == 1 && app.tabs[0].editMode && app.tabs[0].editorDirty,
          "switching tabs parks the unsaved buffer" + where);

    draftsSweep(app);
    const auto drafts = draftFiles();
    const fs::path kept = path.parent_path() / ("kept-" + path.filename().string());
    std::string draftBody;
    if (drafts.size() == 1) {
        const std::string draft = readBytes(drafts[0]);
        writeBytes(kept, draft);  // the save below deletes the tab's own draft
        const size_t header = draft.find(" -->\n");
        if (header != std::string::npos) draftBody = draft.substr(header + 5);
    }
    check(draftBody == kBom + withEol("Parked " + body, "\n"),
          "the parked buffer's draft starts with the mark, then the text" + where);

    tabActivate(app, app.hwnd, 0);
    check(app.editMode && app.editorDirty && app.editorText.rfind(L"Parked # Byte order mark\n", 0) == 0,
          "switching back restores the buffer without the mark in its text" + where);
    saveEditorFile(app, app.hwnd);
    check(readBytes(path) == kBom + "Parked " + body, "saving the restored buffer keeps the mark" + where);

    // The draft as the next launch hands it back: its save keeps the mark too
    writeBytes(path, original);
    exitEditMode(app);
    app.recoveredDrafts = {kept.wstring()};
    draftsRecoverAll(app, app.hwnd);
    check(app.editMode && app.editorDirty && app.currentFile == toUtf8(path.wstring()) &&
              app.editorText.rfind(L"Parked # Byte order mark\n", 0) == 0,
          "a recovered draft opens without the mark in its text" + where);
    saveEditorFile(app, app.hwnd);
    check(readBytes(path) == kBom + "Parked " + body, "saving a recovered draft writes the mark back" + where);
    exitEditMode(app);
    draftsDeleteAll(app);
    while (app.tabs.size() > 1) tabCloseIndex(app, app.hwnd, (int)app.tabs.size() - 1);
    app.tabs.clear();
    writeBytes(path, original);
}
}  // namespace

int runByteOrderMarkTests(const char* file, const char* frontMatterFile) {
    const std::string fixture = readBytes(file), frontMatterFixture = readBytes(frontMatterFile);
    check(fixture.rfind(kBom, 0) == 0 && frontMatterFixture.rfind(kBom, 0) == 0,
          "both fixtures start with a byte order mark");
    if (fixture.rfind(kBom, 0) != 0 || frontMatterFixture.rfind(kBom, 0) != 0) return 1;

    auto state = std::make_unique<App>();
    App& app = *state;
    app.hwnd = CreateWindowExW(0, L"STATIC", L"Byte order mark tests", WS_POPUP, 0, 0, 1050, 900,
                               nullptr, nullptr, nullptr, nullptr);
    if (!app.hwnd || !initD2D(app) || !createRenderTarget(app)) return 2;
    app.width = 1050;
    app.height = 900;
    updateTextFormats(app);
    Settings settings;
    settings.keyProfile = "windows";
    applyKeymap(app, settings);

    // The fixtures with LF and with CRLF line breaks, whichever the checkout
    // produced, each beside a twin without the mark
    const fs::path dir = fs::current_path();
    const fs::path other = dir / "other.md";
    writeBytes(other, "# Other document\n");
    for (const char* eol : {"\n", "\r\n"}) {
        const std::string name = eol[1] ? "crlf" : "lf";
        const std::string where = " (" + std::string(eol[1] ? "CRLF" : "LF") + ")";
        const std::string marked = withEol(fixture, eol), plain = marked.substr(kBom.size());
        const std::string markedFront = withEol(frontMatterFixture, eol);
        const fs::path path = dir / ("bom-" + name + ".md"), twin = dir / ("plain-" + name + ".md");
        const fs::path front = dir / ("bom-front-" + name + ".md"), frontTwin = dir / ("plain-front-" + name + ".md");
        writeBytes(path, marked);
        writeBytes(twin, plain);
        writeBytes(front, markedFront);
        writeBytes(frontTwin, markedFront.substr(kBom.size()));
        for (int width : {1050, 520}) {
            app.width = width;
            const std::string at = where + " at " + std::to_string(width) + " px";
            const std::string preview = "-" + name + "-" + std::to_string(width);
            readingView(app, path, twin, at, "reading" + preview);
            frontMatter(app, front, frontTwin, markedFront, eol, at, "front-matter" + preview);
        }
        app.width = 1050;
        taskBoxes(app, path, marked, where);
        reviewNote(app, path, marked, where);
        editor(app, path, marked, eol, where);
        scrollSync(app, path, marked, where);
        parkedAndDrafted(app, path, other, marked, where);
    }

    DestroyWindow(app.hwnd);
    app.hwnd = nullptr;
    state.reset();
    CoUninitialize();
    std::cout << "Byte order mark: " << failures << " failures\n";
    return failures ? 1 : 0;
}
