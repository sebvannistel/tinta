// #280: in the editor beside its preview, the pointer is an arrow over the
// editor's scrollbar and while dragging it, and dragging the preview's
// scrollbar scrolls both panes, as the wheel over the preview does
#include "d2d_init.h"
#include "editor.h"
#include "input.h"
#include "render.h"
#include "settings.h"
#include "sidepanels.h"
#include "utils.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

namespace {
int failures = 0;
void check(bool ok, const std::string& message) {
    if (!ok) { if (failures < 40) std::cerr << "FAIL split scrollbars: " << message << '\n'; ++failures; }
}
bool closeTo(float a, float b) { return std::fabs(a - b) < 0.5f; }

// The pointer's shape after the window handles a move there
HCURSOR cursorAfterMove(App& app, float x, float y) {
    SetCursor(nullptr);
    handleMouseMove(app, app.hwnd, MAKELPARAM((int)x, (int)y));
    return GetCursor();
}

// A frame: the preview lays out, the editor paints (its scrollable height
// comes from that) and the preview follows the editor, as each paint does
void frame(App& app) {
    ensureLayoutComplete(app);
    app.verticalScrollbarVisible = app.contentHeight > app.height;
    app.renderTarget->BeginDraw();
    renderEditor(app, editorPaneWidth(app));
    check(SUCCEEDED(app.renderTarget->EndDraw()), "the editor paints");
    syncPreviewToEditor(app);
}

void scenario(App& app, const char* path, int width, bool wrap) {
    const std::string where = " (" + std::to_string(width) + (wrap ? ", wrap)" : ")");
    const HCURSOR arrow = LoadCursor(nullptr, IDC_ARROW);
    const HCURSOR beam = LoadCursor(nullptr, IDC_IBEAM);
    app.width = width;
    check(openDocumentInViewer(app, toWide(path)), "the fixture opens" + where);
    enterEditMode(app);
    app.editorShowPreview = true;
    app.editorWordWrap = wrap;
    rebuildLineStarts(app);
    ensureLayoutComplete(app);
    frame(app);
    check(app.editorContentHeight > app.height * 2.0f && app.contentHeight > app.height * 2.0f,
          "both panes are several screens long" + where);

    // The editor's scrollbar: its thumb is at the top, its upper end under
    // the tab strip, so it is taken just below the strip
    const float editorWidth = editorPaneWidth(app);
    const float barX = editorWidth - dpi(app, 8.0f);
    const float thumbY = chromeTopHeight(app) + dpi(app, 12.0f);
    check(cursorAfterMove(app, editorWidth * 0.4f, 300.0f) == beam, "a text beam over the editor's text" + where);
    check(cursorAfterMove(app, barX, thumbY) == arrow, "an arrow over the editor's scrollbar" + where);
    handleMouseDown(app, app.hwnd, MK_LBUTTON, MAKELPARAM((int)barX, (int)thumbY));
    check(app.editorScrollbarDragging, "a press on the editor's scrollbar drags it" + where);
    check(cursorAfterMove(app, barX, thumbY + 150.0f) == arrow, "an arrow while the editor's scrollbar is dragged" + where);
    check(cursorAfterMove(app, editorWidth * 0.4f, thumbY + 200.0f) == arrow,
          "still an arrow when the drag strays over the text" + where);
    check(app.editorScrollY > 0.0f, "the drag scrolls the editor" + where);
    handleMouseUp(app, app.hwnd, 0, MAKELPARAM((int)(editorWidth * 0.4f), (int)(thumbY + 200.0f)));
    check(!app.editorScrollbarDragging, "the release ends the drag" + where);

    // The preview's scrollbar, from the top. Its track runs below the tab
    // strip, as drawn (#287)
    app.editorScrollY = 0.0f;
    frame(app);
    const ScrollbarGeometry bar = documentScrollbarV(app);
    const float maxScroll = std::max(0.0f, app.contentHeight - (float)app.height);
    const float thumbH = bar.thumbLength;
    const float track = bar.trackLength - bar.thumbLength;
    const float previewBarX = documentViewportX(app) + documentViewportWidth(app) - dpi(app, 6.0f);
    const float grabY = chromeTopHeight(app) + dpi(app, 12.0f);
    check(closeTo(bar.trackStart, chromeTopHeight(app)) && grabY < bar.thumbStart + thumbH,
          "the preview's thumb starts below the tab strip" + where);
    check(cursorAfterMove(app, previewBarX, grabY) == arrow, "an arrow over the preview's scrollbar" + where);
    handleMouseDown(app, app.hwnd, MK_LBUTTON, MAKELPARAM((int)previewBarX, (int)grabY));
    check(app.scrollbarDragging, "a press on the preview's scrollbar drags it" + where);
    const float dragged = 0.35f * track;
    check(cursorAfterMove(app, previewBarX, grabY + dragged) == arrow, "an arrow while the preview's scrollbar is dragged" + where);
    frame(app);
    const float expected = dragged / track * maxScroll;
    // The panes line up by source line, so the page settles within a few
    // lines of where the thumb was taken
    const float tolerance = std::max(90.0f, 0.04f * maxScroll);
    check(std::abs(app.scrollY - expected) <= tolerance,
          "the preview stays where its thumb was dragged, after the panes sync" + where + ": at " +
              std::to_string((int)app.scrollY) + ", dragged to " + std::to_string((int)expected));
    check(app.editorScrollY > (float)app.height, "the editor follows the preview" + where);
    // To the end and back
    cursorAfterMove(app, previewBarX, (float)app.height + 200.0f);
    frame(app);
    check(app.scrollY >= maxScroll - tolerance, "dragged to the end, the preview shows its end" + where);
    cursorAfterMove(app, previewBarX, -200.0f);
    frame(app);
    check(app.scrollY <= 1.0f && app.editorScrollY <= 1.0f, "dragged back up, both panes are at their top" + where);
    handleMouseUp(app, app.hwnd, 0, MAKELPARAM((int)previewBarX, 0));
    check(!app.scrollbarDragging, "the release ends the preview's drag" + where);

    // A click on the track below the thumb jumps there, the editor with it
    const float clickY = (float)app.height * 0.75f;
    cursorAfterMove(app, previewBarX, clickY);
    handleMouseDown(app, app.hwnd, MK_LBUTTON, MAKELPARAM((int)previewBarX, (int)clickY));
    frame(app);  // the button still held: frames paint before the release
    const float jumped = bar.scrollForThumb(clickY - thumbH * 0.5f);
    check(std::abs(app.scrollY - jumped) <= tolerance && app.editorScrollY > (float)app.height,
          "a click on the preview's track jumps both panes there" + where + ": at " +
              std::to_string((int)app.scrollY) + ", clicked " + std::to_string((int)jumped));
    handleMouseUp(app, app.hwnd, 0, MAKELPARAM((int)previewBarX, (int)clickY));
    exitEditMode(app);
}
}  // namespace

int runSplitScrollbarTests(const char* path) {
    auto state = std::make_unique<App>();
    App& app = *state;
    app.hwnd = CreateWindowExW(0, L"STATIC", L"Split scrollbar tests", WS_POPUP, 0, 0, 1050, 900,
                               nullptr, nullptr, nullptr, nullptr);
    if (!app.hwnd || !initD2D(app) || !createRenderTarget(app)) return 2;
    app.width = 1050;
    app.height = 900;
    updateTextFormats(app);
    Settings settings;
    settings.keyProfile = "windows";
    applyKeymap(app, settings);
    for (int theme : {0, 5}) {
        applyTheme(app, theme);
        for (int width : {1050, 650}) {
            for (bool wrap : {false, true}) scenario(app, path, width, wrap);
        }
    }
    DestroyWindow(app.hwnd);
    app.hwnd = nullptr;
    state.reset();
    CoUninitialize();
    std::cout << "Split scrollbars: " << failures << " failures\n";
    return failures ? 1 : 0;
}
