# Editor Cut keeps text it could not copy

Validated on Windows with the Release build on 2026-10-27, in a Remote
Desktop session.

## Problem

Other programs open the clipboard for a moment to read each change: Remote
Desktop's clipboard redirector (`rdpclip`), Windows clipboard history and
clipboard managers. The editor's copy gave up at once if it found the
clipboard open, and Cut (Ctrl+X or the context menu, with one selection or
several carets) then deleted the selection anyway. The text was gone from
the document without reaching the clipboard, recoverable only with Undo,
and pasting brought back whatever was copied before.

Found through `editor_context_selection`: in the Remote Desktop session its
menu Copy and Cut checks failed in 3 of 15 runs, each time leaving the
previous copy on the clipboard.

## Change

- The editor's copy retries for up to about 50 ms (10 tries, 5 ms apart),
  as the table cell editor and the Find field already do, and reports
  whether the text reached the clipboard. It uses the shared
  `copyToClipboard` instead of its own copy of it.
- Cut deletes the selection only after the copy succeeded, in all four
  paths: Ctrl+X and the context menu, each with one selection or several
  carets. While the clipboard stays held, Cut leaves the text and the
  selection as they were.
- Copy itself is unchanged apart from the retry.

## Automated checks

`editor_context_selection` (`tests/editor_context_tests.cpp`), after its
menu Copy and Cut checks on `tests/fixtures/editor-context-selection.md`:

- `ClipboardHolder` (`tests/clipboard_snapshot.h`) opens the clipboard on
  another thread and holds it, as those programs do;
- held for a second: menu Cut and Ctrl+X leave the text and selection;
- held for 20 ms: Ctrl+X waits, cuts exactly the selection and puts it on
  the clipboard, and Undo brings the text back.

Without the change it fails those 3 checks. With it, and the retried OLE
read in `tests/clipboard-restore-validation.md`, the test passed 30 of 30
runs in the Remote Desktop session, the copy checks that had failed 3 of 15
runs included. The full suite passed 47 of 47.

## Desktop check

The real app, Release build, in an isolated portable copy of
`tests/fixtures/editor-context-selection.md` at 1100×800. Keys were posted
to the window's message loop: **E**, Ctrl+Home, Shift+End to select the
heading, then Ctrl+X while the script held the clipboard open, and Ctrl+X
again once it was free. The clipboard's earlier text was put back
afterwards. Screenshots are in `out/verify-cut/` (ignored).

- Before the change: Ctrl+X with the clipboard held emptied line 1 and
  marked the tab modified, while the clipboard kept its earlier text. The
  heading was left only in Undo.
- After: the heading stayed, still selected, and the tab unmodified. Once
  the clipboard was free, Ctrl+X cut it and the clipboard held
  `# Editor context menu`.

## Limitations

- A clipboard held for longer than about 50 ms still makes Cut and Copy do
  nothing, now without losing text. There is no message for it.
- Copies outside the editor (the reading view's Ctrl+C, a code block's Copy
  button, table and file path copies) still try once, as before; none of
  them deletes anything.
