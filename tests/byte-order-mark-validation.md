# UTF-8 byte order mark

Validated on Windows on 2026-10-31. A Markdown file that starts with a UTF-8
byte order mark (the bytes `EF BB BF`, as Windows PowerShell 5.1's
`Set-Content -Encoding utf8`, Visual Studio and some editors write it)
showed its first block wrong: nothing removed the mark before md4c parsed
the text, so md4c read it as the first line's first character.

## Reproduction

The build before the fix (3.9.5 at d1530cc), from an isolated portable
copy, on `fixtures/byte-order-mark.md` and
`fixtures/byte-order-mark-frontmatter.md`, compared with the same files
without the mark:

- The first line `# Byte order mark` printed as that literal text, in body
  type; the HTML export had `<p>\ufeff# Byte order mark</p>` and the Word
  export a body paragraph with the mark in it.
- Front matter after the mark still became the properties strip, but an
  empty paragraph holding only the mark followed it: `<p>\ufeff</p>` before
  the `<h1>` in HTML, an empty paragraph in Word, and on the printed page
  the heading and everything after it 42 px lower (216 instead of 174).
- The same held for anything else on the first line: a fence, a table, a
  list item, a footnote definition, raw HTML and a bare `www.` link all
  parsed differently behind the mark, and a file holding only the mark
  showed an empty paragraph.
- The editor kept the mark as the first character of its text. Saving an
  untouched buffer wrote it back, but the caret could sit in front of it:
  text typed at the very start went ahead of the mark, so the saved file
  started with the typed text and the old first line, now preceded by the
  mark, still showed as literal text. Select All and typing dropped the
  mark from the file. A buffer parked in another tab, and its crash-recovery
  draft, held the mark where the typing had pushed it.
- Already right before the fix: task boxes flipped the right byte (the
  parser's offsets counted the mark), headings' offsets pointed at their
  text, E opened the editor at the heading being read, the front matter
  strip showed, and a save that stamps `updated` kept the mark.

## Fix

- `parseDocument` blanks the mark in the copy md4c parses, as it blanks
  front matter. Its three bytes become line breaks: the first line is a
  line of its own, and every offset still counts the mark, so task boxes,
  review notes and scroll sync, which work on the raw source, stay on their
  bytes. Spaces would have indented the first line and changed a fence's
  text or a list item's continuation.
- The editor keeps the mark out of its text (`App::editorBom`): the caret
  can no longer sit before it, and `writeEditorFile` writes it back in front
  after the date stamp and the CRLF conversion. A buffer parked in a tab
  (`DocTab::editorBom`) and its draft keep it, so their saves write it too.
- Only a mark at the very start is skipped; the same character further on
  is text, as before. Mermaid and PlantUML documents already skipped it.

## Checks

- `document_types` (`document_tests`): both fixtures and eight first-line
  cases (a fence with indented code, a list item and its continuation, a
  table, a footnote definition, raw HTML, a bare `www.` link, tasks, nothing
  but the mark), each with LF and with CRLF, parse to the same tree as
  without the mark, with every source and task mark offset three bytes on.
  The title is a level 1 heading whose text starts at byte 5, front matter
  becomes properties with the heading right after, and the same character
  inside the text stays. 22 of these checks fail on the build before the
  fix.
- `byte_order_mark` (new, `input_tests --byte-order-mark-tests`) drives the
  real handlers on copies of both fixtures, with LF and with CRLF:
  - Reading view at 1050 and 520 px: the title is a level 1 heading, every
    heading sits at the height it has in the file without the mark, and so
    does the end of the page; each heading's offset points at its text in
    the source. After front matter the strip shows and the heading sits
    where it does without the mark.
  - Task boxes: a click on the first box changes exactly its mark byte on
    disk, keeping the byte order mark and the line breaks; a second click
    restores the file, and the last box flips its own byte.
  - A review note on the title goes on the line after it, line 2, and the
    file keeps its mark.
  - Editor: the text starts with the title; saving an untouched buffer
    writes the same bytes; text typed after Ctrl+Home goes in front of the
    title and the save writes the mark first; Select All and typing keeps
    the mark; the reading view then shows the typed paragraph and the title.
  - Scroll sync: E at each of the five lower headings opens the editor on
    that heading's line, and in the editor each heading's preview anchor
    leads to its own source line.
  - Front matter: the editor's text opens with `---`; a save with `updated`
    maintained stamps the date inside the block and keeps the mark, and the
    reloaded view reads the block.
  - Tabs and drafts: the buffer parked by a tab switch writes a draft that
    starts with the mark; switching back and saving keeps it; the draft
    handed back as at the next launch opens without the mark in its text
    and saves with it.
  - 46 checks fail on the build before the fix, none after. The task box,
    heading offset, E and date stamp checks pass on both builds: they would
    catch a fix that dropped the mark without counting its bytes.
- `render_byte_order_mark_fixtures.ps1` (new) prints and exports both
  fixtures in Paper and Midnight: the HTML opens with the title heading and
  holds no mark, the Word body has its Heading1 and no mark, and the pages
  (2 and 1), the HTML and the Word body are byte for byte those of the same
  files without the mark. The build before the fix fails it at the first
  fixture.
- All 100 other fixtures (subfolders, Mermaid and PlantUML files included),
  none of which starts with a mark, printed and exported to HTML and Word in
  Paper and Midnight by the build before the fix and by the fixed build: the
  600 outputs are identical, 966 files each (pages by hash, HTML, the Word
  body).
- The frame previews of the reading view at 1050 and 520 px show the title
  as a heading, and the properties strip with the heading right below it.
- The full suite passed, 54 of 54.

Evidence is in `out/check-bom/` (ignored): `before/`, `fixed/`, `previews/`,
`sweep/` and the test logs.

## Limitations

- Plain-text documents (`.txt`, `.json`, `.yaml` and the like), shown as one
  code block, keep the mark at the start of that block in the reading view,
  before and after the fix: it draws nothing, but it is in the HTML and Word
  exports and in what the block's Copy button copies. The editor leaves it
  out of their text and writes it back, as for Markdown.
- Only one mark is skipped; a second one right after it stays text, as in
  other editors. UTF-16 files are still not decoded.
- The live window was not driven by hand: the reading view and the editor
  ran through the test harness's handlers, and the reading view was drawn
  into frame previews.
