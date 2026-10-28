# Copies wait for a busy clipboard, and say when they could not copy

Validated on Windows with the Release build on 2026-10-28, in a Remote
Desktop session.

## Problem

Other programs open the clipboard for a moment to read each change: Remote
Desktop's clipboard sync, Windows clipboard history and clipboard managers.
Only the editor's copy and the table cell editor waited for them
(`tests/editor-cut-busy-clipboard-validation.md`); every other copy gave up
at once, and many then claimed a copy that had not happened:

- Ctrl+C with nothing selected (the whole document) showed "Copied!"
  anyway;
- Ctrl+C on a selection said nothing and dropped the selection;
- the table's Copy and the review annotations' "copy for agent" announced
  their copy anyway;
- the code block's and the diagram's Copy, Copy file path, the context
  menu's Copy, the Find field's copy and the editor's Copy said nothing.

## Change

- `openClipboard` (`utils.h`) opens the clipboard trying again for up to
  about 50 ms (10 tries, 5 ms apart). Every clipboard read and write goes
  through it: the copies above, the rich copy (#285), the diagram image
  copies, and the pastes of the editor, the Find field, table cells and
  annotation notes. The editor's and the table cell editor's own retry
  loops are gone.
- When the clipboard stays held, the copy functions report it (`held`),
  and the action shows a warning instead of its "Copied" message:
  "Clipboard busy — nothing copied", in every language. Warnings show with
  notifications off too.
- A selection whose copy the clipboard refused stays selected; Cut in the
  Find field and in table cells removes only what reached the clipboard,
  as the editor's already did.

## Automated checks

`editor_context_selection` (`tests/editor_context_tests.cpp`), with another
thread holding the clipboard (`ClipboardHolder`):

- held for a second: menu Cut, Ctrl+X and Ctrl+C in the editor say the
  clipboard is busy; in the viewer, Ctrl+C keeps the selection and says so,
  Ctrl+C with nothing selected claims no copy, and the code block's Copy,
  the table's Copy and Copy file path say the clipboard is busy without
  their "Copied" messages;
- held for 20 ms: the viewer's Ctrl+C and the table's Copy wait, copy, and
  say "Copied", as the editor's Ctrl+X already did.

Neutering checks, one at a time: without the warning, the 8 "busy" checks
fail; without the retry, the 3 "held for a moment" checks fail, and so does
the menu Cut check, as the Remote Desktop clipboard sync holds the
clipboard just after each copy; dropping the selection on failure fails
its check. `i18n` checks the new message in every language. The full suite
passes.

## Checked in the built app

The built `tinta.exe` (isolated portable copy) opened
`tests/fixtures/rich-copy-bold.md`; Ctrl+A and Ctrl+C were posted to it
while another thread held the clipboard open for 1.5 s, without reading or
changing it. The screenshot (`out/verify-clipboard/held-ctrl-c.png`,
ignored) shows the warning chip "Clipboard busy — nothing copied" and the
whole document still selected. The successful copy was not repeated in the
real app, so the clipboard, which held an image, stayed untouched.
