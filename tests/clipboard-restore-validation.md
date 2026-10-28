# Clipboard restore in native tests (0xc000041d)

`editor_context_selection` failed intermittently, usually on the first
`ctest` run after a build, with `Exit code 0xc000041d`
(`STATUS_FATAL_USER_CALLBACK_EXCEPTION`). Rerunning it passed.

## Cause

The clipboard part of `tests/editor_context_tests.cpp` saved the user's
clipboard with `OleGetClipboard`, ran menu Copy/Cut, then restored it with
`OleSetClipboard` on the saved object followed by `OleFlushClipboard`.
`OleGetClipboard` returns a live proxy for whatever is on the clipboard, not a
copy. When a running OLE application owned the clipboard, as it does after
copying from a browser, terminal, editor or `Set-Clipboard`, flushing that
proxy recursed inside ole32 until the stack overflowed (`0xc00000fd`). The
overflow happened inside the clipboard window's callback, so Windows ended the
process with `0xc000041d`.

The failure depended on what was on the clipboard, not on timing:

| Clipboard before the test | Before the fix |
| --- | --- |
| Text from a running OLE application (`Set-Clipboard`) | 3/3 `0xc000041d` |
| Plain Win32 text (`clip.exe`) | 3/3 pass, user's text lost |
| Empty | 2/2 pass |

A developer's clipboard usually comes from an OLE application, so the first
run crashed. The crashed process left a plain or empty clipboard, so later
runs passed. GitHub runners start with an empty clipboard.
`search_input_focus` used the same restore without the flush: it did not
crash, but it emptied the clipboard on every run.

A standalone model of the sequence logged every first-chance exception with
its stack from a vectored handler. No debugger was installed and the shell
was not elevated for WER `LocalDumps`. It reproduced `0xc00000fd` in a single
repeating ole32 frame during `OleFlushClipboard`, then exit `0xc000041d`.
It reproduced the same result with an OLE source in another process or on
another thread of the same process.

## Fix

- Both tests use `ClipboardSnapshot` (`tests/clipboard_snapshot.h`). It
  already copied independent handles with `OleDuplicateData` for the table
  input tests and never hands a proxy back to OLE.
- `ClipboardSnapshot` retries `OpenClipboard` for up to a second. Clipboard
  history and other listeners open the clipboard right after each copy, which
  is exactly when the snapshot restores it.
- `OleClipboardSource` gives the clipboard to a live OLE data object on its own
  STA thread. Editor context tests snapshot that state, run menu Copy/Cut and
  Unicode copies, restore it, and check that both Win32 and OLE readers see the
  original text again. The user's own clipboard is preserved around the whole
  block.
- `search_input_focus` now checks that the clipboard text is unchanged after
  its copy/cut/paste block, instead of only checking `OleSetClipboard`'s result.

The app's own clipboard code was not involved: it uses Win32
`OpenClipboard`/`SetClipboardData` and never `OleGetClipboard`.

## Regression check

With the old `OleGetClipboard`/`OleSetClipboard`/`OleFlushClipboard` restore
temporarily put back inside the new harness, `editor_context_selection` exited
with `0xc000041d` in 5 of 5 runs, starting from an empty clipboard (3 runs) and
from plain text (2 runs). Before, these states passed. The harness now hits the
failing state every run, whatever the developer last copied.

## Validation

```powershell
cmake --build build --config Release --parallel 8
ctest --test-dir build -C Release --output-on-failure
```

Recorded on Windows 11 with MSVC 14.51.36231, 2026-10-16. Each round touched
`src/editor.cpp`, rebuilt every Release target and ran the full suite right
away. The clipboard came from a running OLE application (`Set-Clipboard`) in
rounds 1, 3 and 5, and was plain text (`clip.exe`) in rounds 2 and 4:

- 5 of 5 rounds: zero compiler warnings, **39/39 CTests passed** (33–36 s).
- The clipboard text was unchanged after every full run.
- Targeted runs with the fix: `editor_context_selection` and
  `search_input_focus` passed in all 10 runs across OLE, plain-text and empty
  clipboards, and kept the clipboard each time.
- Logs: ignored `out/clipboard-restore/build-N.log` and `ctest-N.log`.

## Limitations

- The ole32 frames are unsymbolized (no symbol server was used), so the
  recursion is identified by its repeating return address and the exception
  codes, not by function name.
- The native clipboard tests still put test text on the clipboard while they
  run, so it can appear in Windows clipboard history (Win+V), as before.
- Remote CI has not been run for this change.
- No app code or rendering changed, so no fixture rendering or export
  comparison was needed. The existing `fixtures/editor-context-selection.md`
  drives the editor tests.

## Over Remote Desktop (2026-10-27)

Merged onto master after 3.9.3 and run in a Remote Desktop session, where
Remote Desktop's clipboard redirector (`rdpclip`) opens the clipboard to
read each change, the check that OLE applications can paste the restored
clipboard failed in 6 of 6 runs. Logging the read showed why: right after
the restore, `OleGetClipboard` returned `CLIPBRD_E_CANT_OPEN` (0x800401D0),
and the same read 20 ms later returned the restored text. The Win32 read
before it retries `OpenClipboard`; OLE fails at once while another program
has the clipboard open.

`oleClipboardText` now retries for up to a second and prints the HRESULT if
the read still fails, so a clipboard that OLE cannot read at all still fails
the check. The menu Copy and Cut checks also failed in 3 of 15 runs there;
that was the editor giving up on a held clipboard, fixed in the app (see
`tests/editor-cut-busy-clipboard-validation.md`).
