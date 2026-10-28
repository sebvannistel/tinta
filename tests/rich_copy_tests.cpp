// Copying from the viewer keeps **bold** for rich paste targets such as
// Outlook: the clipboard carries CF_HTML with <b> spans next to the plain
// text, and the plain text is unchanged
#include "clipboard_snapshot.h"
#include "document.h"
#include "d2d_init.h"
#include "keystroke.h"
#include "render.h"
#include "selection.h"
#include "utils.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

namespace {
int failures = 0;
void check(bool value, const std::string& message) {
    if (!value) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
std::string read(const char* path) {
    std::ifstream f(path, std::ios::binary);
    return {(std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>()};
}
bool has(const std::wstring& text, const std::wstring& part) {
    return text.find(part) != std::wstring::npos;
}
std::string narrow(const std::wstring& text) {
    std::string out;
    for (wchar_t c : text) out += c < 128 ? (char)c : '?';
    return out;
}

size_t headerOffset(const std::string& html, const std::string& key) {
    size_t at = html.find(key + ":");
    if (at == std::string::npos) return std::string::npos;
    return std::stoul(html.substr(at + key.size() + 1, 10));
}

// The CF_HTML header's offsets point at the document and fragment bounds
void checkClipboardHtml(const std::string& html, const std::string& want) {
    size_t startHtml = headerOffset(html, "StartHTML");
    size_t endHtml = headerOffset(html, "EndHTML");
    size_t startFragment = headerOffset(html, "StartFragment");
    size_t endFragment = headerOffset(html, "EndFragment");
    if (startHtml == std::string::npos || endHtml == std::string::npos ||
        startFragment == std::string::npos || endFragment == std::string::npos) {
        check(false, "CF_HTML has its offset header");
        return;
    }
    check(endHtml == html.size(), "EndHTML is the payload's end");
    check(html.compare(startHtml, 6, "<html>") == 0, "StartHTML points at <html>");
    check(startFragment >= 20 && html.compare(startFragment - 20, 20, "<!--StartFragment-->") == 0,
          "StartFragment follows its marker");
    check(html.compare(endFragment, 18, "<!--EndFragment-->") == 0, "EndFragment precedes its marker");
    check(startFragment < endFragment &&
              html.substr(startFragment, endFragment - startFragment).find(want) != std::string::npos,
          "the fragment carries the copied HTML");
}

void layoutTests(App& app) {
    const std::wstring& text = app.docText;
    std::wstring all = selectionHtmlForRange(app, 0, text.size());
    check(selectionTextForRange(app, 0, text.size()) == text, "plain text is the document text");

    check(has(all, L"<b>Please review the attached garden plan</b> before"),
          "a bold phrase is one <b> span, the text after it is not bold");
    check(has(all, L"<b>A12</b> and <b>B7+3</b>."), "adjacent short bold words stay separate");
    check(has(all, L"<b>wraps across a soft line break</b> stays"),
          "a bold span across a soft line break stays one span");
    check(has(all, L"<b>a &lt; b &amp; c &gt; d</b> and &quot;quotes&quot;"),
          "markup characters are escaped inside and outside bold");
    check(has(all, L"<div><b>Please let me know in writing,</b></div>"),
          "a bold paragraph closes with its line");
    // With <br> between lines, Word and Outlook pasted the whole copy as one
    // paragraph; a <div> per line gives a paragraph per line, as plain text
    check(has(all, L"<div>Dear team,</div>\r\n<div><br></div>\r\n<div><b>Please review"),
          "each line is a <div>, an empty line <div><br></div>");
    check(!has(all, L"<br>\r\n"), "no line ends in a bare <br>");
    check(has(all, L"<b>north bed</b>") && has(all, L"<b>watering schedule</b>"),
          "bold inside list items");
    check(has(all, L"<b>bold inside</b> the quote"), "bold inside a blockquote");
    check(has(all, L"<div><b>A12</b>&#9;ready</div>"), "bold inside a table cell");
    // Word made two tabs of a tab wrapped in mso-tab-count
    check(has(all, L"<div>Bed&#9;Status</div>") && !has(all, L"mso-tab-count"),
          "a table cell's separator is one plain tab");
    check(has(all, L"<b>bold code</b> and plain code"), "bold inline code is bold, plain code is not");
    check(has(all, L"Code blocks copy as they read, **not bold**."),
          "a code block's asterisks are literal text");
    check(!has(all, L"<b>Dear") && !has(all, L"<b>Best") && !has(all, L"<b>italic") &&
              !has(all, L"<b>a link"),
          "plain, italic and linked text stay unbolded");
    // Italics and web links carry over too; a link inside the document
    // pastes as its text, as it would lead nowhere from a mail
    check(has(all, L"<i>italic words</i> italic") && has(all, L"<i>italic</i> stays italic"),
          "italics are <i> spans");
    check(has(all, L"<a href=\"https://example.com\">a link</a>"),
          "a web link is an <a href> around its text");
    check(has(all, L"a heading link pastes") && !has(all, L"#message"),
          "a link to a heading pastes as its text");
    check(has(all, L"<b><i>bold italic</i></b>"), "bold italic nests <i> in <b>");
    check(has(all, L"<a href=\"https://example.org/?a=1&amp;b=2\"><b>a bold link</b></a>"),
          "a bold link nests <b> in <a>, its address escaped");
    check(!has(all, L"<b>Rich copy") && !has(all, L"<b>Message"),
          "headings copy as their text, as before");

    // A selection that starts or ends inside a bold span opens and closes it
    size_t at = text.find(L"Caf\u00e9");
    check(at != std::wstring::npos, "the fixture's accented phrase is laid out");
    if (at != std::wstring::npos) {
        std::wstring part = selectionHtmlForRange(app, at + 2, at + 11);
        check(part == L"<div><b>f\u00e9 na\u00efve</b>.</div>", "partial bold selection: " + narrow(part));
    }
    std::string payload = clipboardHtml(all);
    checkClipboardHtml(payload, "<b>A12</b> and <b>B7+3</b>.");
    check(payload.find("Caf\xc3\xa9") != std::string::npos, "CF_HTML is UTF-8");

    // A rebuilt layout leaves no stale ranges behind
    size_t ranges = app.docBoldRanges.size();
    size_t italics = app.docItalicRanges.size(), links = app.docLinkRanges.size();
    layoutDocument(app);
    check(app.docBoldRanges.size() == ranges, "relayout records each bold span once");
    check(app.docItalicRanges.size() == italics && italics >= 3 && app.docLinkRanges.size() == links &&
              links == 2,
          "relayout records each italic span and web link once");
    for (size_t i = 0; i < app.docBoldRanges.size(); ++i) {
        const auto& r = app.docBoldRanges[i];
        check(r.first < r.second && r.second <= text.size() &&
                  (i == 0 || app.docBoldRanges[i - 1].second < r.first),
              "bold ranges are ascending, disjoint and inside docText");
    }
}

// Outlook's compose font reaches Word-based editors only: mso- properties,
// which browsers drop, so web mail keeps its own font (#285)
void composeFontTests() {
    const std::string personal =
        "<html>\n<head>\n<style>\n /* Style Definitions */\n span.PersonalComposeStyle\n"
        "\t{mso-style-name:\"Personal Compose Style\";\n\tmso-style-type:personal-compose;\n"
        "\tmso-ansi-font-size:11.0pt;\n\tmso-bidi-font-size:11.0pt;\n"
        "\tfont-family:\"Aptos\",sans-serif;\n\tmso-ascii-font-family:Aptos;\n"
        "\tmso-hansi-font-family:Aptos;\n\tmso-bidi-font-family:\"Times New Roman\";\n"
        "\tcolor:windowtext;}\n</style>\n</head>\n</html>\n";
    std::string css = composeFontCss(personal);
    check(css == "mso-ascii-font-family:Aptos;mso-hansi-font-family:Aptos;mso-ansi-font-size:11.0pt;",
          "Outlook's compose font becomes mso- properties only: " + css);
    css = composeFontCss("{font-family:\"Segoe UI\",sans-serif;\nmso-ansi-font-size:12.0pt;}");
    check(css == "mso-ascii-font-family:\"Segoe UI\";mso-hansi-font-family:\"Segoe UI\";"
                 "mso-ansi-font-size:12.0pt;",
          "without mso- faces, the first face of the plain list: " + css);
    check(composeFontCss("{font-family:Aptos'><b>;}").empty() && composeFontCss("").empty(),
          "markup in a value, or no compose font, gives no style");
}

std::string clipboardFormat(HWND hwnd, UINT format) {
    std::string out;
    if (!ClipboardSnapshot::open(hwnd)) { check(false, "clipboard opens for inspection"); return out; }
    if (HANDLE data = GetClipboardData(format)) {
        if (auto bytes = static_cast<const char*>(GlobalLock(data))) {
            out = bytes;
            GlobalUnlock(data);
        }
    }
    CloseClipboard();
    return out;
}

// Ctrl+A, Ctrl+C through the real key handler, then both formats back
void clipboardTests(App& app) {
    ClipboardSnapshot saved(app.hwnd);
    keystroke(app, 'A', {true});
    keystroke(app, 'C', {true});
    std::string html = clipboardFormat(app.hwnd, RegisterClipboardFormatW(L"HTML Format"));
    checkClipboardHtml(html, "<b>A12</b> and <b>B7+3</b>.");
    // Retried like the HTML read: over Remote Desktop the clipboard sync
    // holds the clipboard for a moment after each change
    std::wstring text;
    if (ClipboardSnapshot::open(app.hwnd)) {
        if (HANDLE data = GetClipboardData(CF_UNICODETEXT)) {
            if (auto chars = static_cast<const wchar_t*>(GlobalLock(data))) {
                text = chars;
                GlobalUnlock(data);
            }
        }
        CloseClipboard();
    }
    check(text == app.docText, "Ctrl+C keeps the plain text");
}
}  // namespace

int main(int argc, char** argv) {
    // The clipboard run overwrites the clipboard (restored afterwards), so
    // it is opt-in like the other clipboard suites
    bool testClipboard = argc == 2 && std::string(argv[1]) == "--clipboard-test";
    auto state = std::make_unique<App>();
    App& app = *state;
    app.hwnd = CreateWindowExW(0, L"STATIC", L"Tinta rich copy test", 0,
        0, 0, 0, 0, HWND_MESSAGE, nullptr, nullptr, nullptr);
    if (!app.hwnd || !initD2D(app)) return 2;
    auto doc = parseDocument(app.parser, read(TINTA_RICH_COPY_FIXTURE), "rich-copy-bold.md");
    check(doc.success, "the rich copy fixture parses");
    if (!doc.success) return 1;
    applyTheme(app, 0);
    updateTextFormats(app);
    for (int width : {1050, 420}) {
        app.width = width;
        app.height = 900;
        app.root = doc.root;
        layoutDocument(app);
        layoutTests(app);
    }
    composeFontTests();
    if (testClipboard) clipboardTests(app);
    // --dump-html PATH: the copy of the whole fixture (at the 420 px layout
    // last laid out) as an HTML file, for opening in Word without touching
    // the clipboard
    if (argc == 3 && std::string(argv[1]) == "--dump-html") {
        std::string payload = clipboardHtml(selectionHtmlForRange(app, 0, app.docText.size()));
        size_t html = payload.find("<html>");
        std::ofstream(argv[2], std::ios::binary) << payload.substr(html == std::string::npos ? 0 : html);
    }
    std::cout << "Rich copy: " << failures << " failures"
              << (testClipboard ? "" : " (clipboard checks require --clipboard-test)") << '\n';
    return failures ? 1 : 0;
}
