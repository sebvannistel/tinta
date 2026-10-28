# Theme editor font list opens on the chosen face (#284)

Validated on Windows with the Release build on 2026-10-27.

## Problem

Opening the theme editor showed the font list from its first family, so the
theme's text face (Segoe UI for every built-in theme but Abyss) was far out
of view. Clicking **Text**, **Headings** or **Code** did scroll to the
chosen face; opening the editor did not.

The scroll that centres a face needs the list's height, which is known only
once the editor has been drawn. The role buttons computed it in the click
handler from the last drawn list; opening the editor just reset the scroll
to the top.

## Change

Based on go4399's patch on the issue: opening the editor sets the font
list's scroll to -1, and the next paint, which knows the list's height,
centres the chosen face. Beyond the patch:

- **Text**, **Headings** and **Code** use the same path, so the click
  handler no longer computes the scroll from a list rect it drew earlier.
- Switching the base theme with the arrows also brings the list to the
  base's face, since the switch replaces all three faces.
- The centred scroll is clamped to the list's range in the same paint, so a
  family near the end stops at the bottom instead of leaving empty rows
  under it for a frame.

## Automated checks

`inline_code_theme` (`tests/theme_tests.cpp`, isolated portable run),
`themeEditorFonts`, through the editor's own clicks in a 1180×900 window:

1. opening the editor from Paper shows Segoe UI, whole and centred in the
   list;
2. **Code** shows Consolas, and **Text** brings Segoe UI back;
3. after browsing to the top of the list, the base arrow (Paper → Sakura)
   brings Segoe UI back to the middle;
4. with the system's last family as the text face, **Text** scrolls to the
   end, with that family's row on the list's bottom edge.

Without the change it fails 3 checks: the list opens away from the text
face (not shown, not centred) and the base switch leaves the face out of
view. The other checks pass before and after: the role buttons already
scrolled, and the paint already kept the scroll in range. The full suite
passed 47 of 47.

## Desktop check

The real app, Release build, in an isolated portable copy at 1100×800 with
the Paper theme. Input was posted to the window's message loop: **T**, the
chooser's Edit footer, then the editor's own buttons. Screenshots are in
`out/verify-284/` (ignored).

- Before the change: the list opened at Agency FB, Algerian, Arial…, with
  Segoe UI far below. With **Code** chosen and the base stepped to Arctic,
  the list stayed around Consolas with no row highlighted, since Arctic's
  Cascadia Code was out of view.
- After: the editor opens with Segoe UI highlighted in the middle of the
  list. **Code** centres Consolas; stepping the base to Sakura keeps it and
  to Arctic centres Cascadia Code; **Text** brings Segoe UI back.

Not checked: a heading face of its own in the real app (no built-in theme
but Abyss has one), and display scales other than 100%.
