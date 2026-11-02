# Bare text in raw HTML quotes

Validated on Windows on 2026-10-31. A raw HTML `<blockquote>` whose text
has no `<p>` around it (`<blockquote>Bare text</blockquote>`) drew
nothing: the quote's text was missing from the page, the print pages and
a copy, at one level or thirty, in the builds before and after the
nesting cap of 2026-10-30. `<blockquote><p>Text</p></blockquote>` drew
correctly, and GitHub draws both as a quote holding the text. Found while
checking that cap (see `deep-nesting-validation.md`).

## Cause

`parseHtmlIntoElements` puts the bare text, and the inline tags beside
it, straight under the quote. The quote laid out each child as a block,
and block layout draws nothing for text. An `<a id>` straight inside the
quote was skipped the same way, so a link to it had no target. The Word
export took the children one by one too: bare text became an empty
bordered paragraph, and the text of a `<b>` or `<a>` a paragraph of its
own, without its bold or its link. The HTML export already wrote the
text, and a copy is built from what the page draws.

## Fix

- A quote lays out its children in source order, as list items do since
  #292: each run of inline children is a paragraph, and blocks lay out
  as before. The run goes through the helper the raw HTML block already
  used, so it keeps a paragraph's gap, and an anchor alone marks its
  target and takes no room. Markdown quotes always wrap their text in
  paragraphs and are unchanged.
- The Word export writes each run as one quote paragraph, with its bold,
  italics, code, links, scripts and line breaks.

## Checks

- `blockquote_layout` gains raw HTML quote cases, run in Paper and
  Midnight at 1050 and 650 px, at 100% and 150%:
  - seven bare forms lay out exactly as the same text in `<p>`, with the
    same text, runs, bars, links and height: one level, inline tags and
    `<br>`, bare text around a `<p>`, three nested levels, a quote in a
    Markdown quote, one in a list item, and thirty levels;
  - the new fixture `fixtures/html-blockquote-text.md`: its 28 texts
    appear once each, in source order; none of its 18 quote bars is
    empty; the bold, the link, both line breaks, the anchor (no blank
    line) and the nested indents hold; the table, the code block and the
    six headings survive;
  - the HTML and Word exports and a copy of the whole page keep the bare
    text, and Word keeps the inline quote as one paragraph with its bold,
    hyperlink, subscript and two breaks.
- Without the layout fix, 15 of these checks fail, the anchor's among
  them, and 17 of the fixture's 28 texts are missing from the page and
  the copy. Without the Word fix, 2 fail: the 11 bare texts outside the
  list are missing from Word and the inline quote is split up.
- The full suite passed, 53 of 53.

## Fixture runs

- The built app printed and exported the fixture (`--printpages`,
  `--exporthtml`, `--exportdocx`) in Paper and Midnight: three pages
  each, every quote with its text and bar. Before the fix it printed two
  pages, with every bare quote missing.
- A twin of the fixture with each bare run wrapped in `<p>`, printed and
  exported by the build before the fix, gives print pages and Word files
  byte-identical to the fixture's with the fix, in both themes.
- The reading view at 1050 and 650 px in Paper and Midnight, drawn
  through the tests' frame preview helper, shows the same. The HTML
  export, opened in Edge in both themes, shows each bare text inside its
  quote.
- All 78 other fixtures exported in Paper and Midnight before and after
  the fix: 784 files identical. In the new fixture only the print pages
  and Word's document and its link relations changed; its HTML export is
  identical.

Evidence is in `out/blockquote-html/` (ignored): `exp-base/`,
`exp-final/`, `twin/` with its source, `screens/`, `html-shots/`, and the
export and comparison scripts.

## Limitations

- Raw HTML text that runs onto a second source line keeps the line break
  inside its text, and the lines overlap on the page. A `<p>` or `<div>`
  does the same, before and after this fix, so the fixture keeps each
  bare text on one source line.
- Word runs raw HTML in a list item into the item's own paragraph: the
  quote's text stays there without its border, for `<p>` quotes as well.
  Bare text right under a raw HTML block outside a quote
  (`<div>Text</div>`) is still lost in Word, and an anchor alone before
  a quote's `<p>` still writes an empty paragraph there. All three
  predate this fix.
- A stray `<li>` with no list around it draws nothing inside a quote,
  before and after.
- Computer use was not available, so the window itself was not captured;
  the frame preview helper draws the same retained layout. The Word files
  were checked structurally, not opened in Word.
