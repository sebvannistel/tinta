# Code block copy without a trailing line break (#269)

Validated on Windows with the Release build on 2026-10-21.

## Problem

A code block's Copy button put the block's text on the clipboard with the
line break that ends its last line, so a one-line block pasted as a line
plus an empty line. md4c ends every code block's text with that break.

## Change

- The Copy button copies the block without the break after its last line
  (`codeCopyText`, `src/utils.cpp`), as GitHub's copy button does. A blank
  line written before the closing fence is content and stays, as do
  trailing spaces and fences nested in a longer fence.
- The block's stored text is unchanged, so its height, search, selection
  and the diagram source are unaffected.
- The "Copied" pill now shows only when something was copied. An empty
  block, or a clipboard another program holds at that moment, no longer
  claims a copy that did not happen.

## Automated checks

`code_block_layout` (updated):

- Every existing layout case states its copied text: LF and CRLF fences, a
  closing fence at end of file, an unclosed fence, raw `.txt` and `.json`
  files, an intentional trailing blank line, empty blocks and an invalid
  Mermaid fence.
- New: `tests/fixtures/code-block-copy-269.md` is laid out in two themes, at
  100 % and 150 % and at 1050 and 650 px. Each of its seven blocks (one
  line, several lines, a kept blank line, a tilde fence, a block in a list,
  one in a quote, and a four-backtick fence holding a fenced block with
  trailing spaces) is clicked through the real mouse handlers in a hidden
  window, and the clipboard is read back.

Reverting `codeCopyText` fails the copied-text check in every layout case.
Reverting only the click handler's use of it fails the click test for the
blocks in all eight configurations.

Programs that follow the clipboard can hold it for a moment after each
change, and a click then copies nothing. The click test repeats such a
click, which would otherwise read as a stale clipboard rather than a wrong
one (see the follow-up below).

## Limits

- Copying a selection with Ctrl+C is unchanged: it copies what is selected.
- Indented code blocks are off in Tinta (`MD_FLAG_NOINDENTEDCODEBLOCKS`),
  so the fixture has none.

## Desktop check

Computer use was not available in this session, so the window was not
operated by hand. The checks above drive the real input handlers, layout
and painting in a hidden window instead. The full suite passed 39 of 40 on
the Release build; the one failure is the known first-run crash of
`editor_context_selection`, which passed on three reruns.

## Follow-up: the click test over Remote Desktop (2026-10-25)

With a Remote Desktop session connected, the click test failed on every
run: 42 of its 56 checks read back the test's sentinel, despite five tries
per block. A probe that polled the clipboard's sequence number, its owner
and the window holding it open showed why. After each change, programs
that follow the clipboard open it to read the new text: WSLg's two
`msrdc.exe` clients 5 to 15 ms later, `rdpclip.exe` at about 20 and 80 ms,
and the clipboard history service at about 220 ms. None of them writes:
the sequence number did not move and the test's window stayed the owner.
The test clicked 5 to 8 ms after writing its sentinel, so Tinta found the
clipboard held and copied nothing, as intended (234 of 247 clicks), and
each try wrote the sentinel again, which started the readers over. The
`chips 1` in the failure message did not mean a copy had landed: a
repeated "Copied" pill replaces the previous one, so the count stays at
one.

The test now writes the sentinel once per block and repeats a click that
leaves it every 50 ms, up to 20 clicks. The check is unchanged: the
clipboard must hold exactly the block's text. The failure message gives
the number of clicks instead of the pill count.

Checked on the Release build with the session connected:

- The test passed 10 runs in a row, in 4.5 to 5.2 s each. In five runs
  that counted clicks, 60 of 280 copies landed on the first click, 219 on
  the second and one on the third.
- Copying the block with its trailing break, as before #269, fails all 56
  click checks, after one or two clicks each.
- A Copy button that copies nothing fails all 56 after 20 clicks; the test
  then takes 68 s instead of 5.
- The full suite passed 45 of 46. `superscript_and_learn` failed its nine
  "Learn copy saves through the normal file writer" checks and took 673 s,
  for a reason outside this change: its Learn copies save to a path that
  does not exist yet, and since Save opens Save As for a missing file
  (#275), each save opened a real Save As dialog.

Not checked: a run with nobody connected, since this session was itself
remote. There the first click lands and the test does what it did before:
one sentinel, one click and one read per block. WSLg's clients were the
most frequent readers here; whether they hold the clipboard the same way
in a local session was not checked. Something that writes to the
clipboard during the run, such as a copy on the client machine, can still
change what is read.
