# Deeply nested lists, quotes and spans

Validated on Windows on 2026-10-30. Found while checking PR #292: raw HTML
lists nested a thousand deep crashed Tinta. Every walk over the document
tree (layout, print, the exports, copying, freeing the tree) goes one call
deeper per level, so a document nested deep enough overflowed the stack.

## Reproduction

`--printpages` with the build before the fix (3.9.5 with #292 and #294),
from an isolated portable copy, on generated documents:

- 1000 levels crashed (0xC0000005): raw HTML `<ul><li>`, `<ol><li>` and
  `<blockquote>`, Markdown lists, ordered lists, quotes and quoted lists.
- 5000 levels crashed: Markdown `*emphasis*`, `**strong**`, links around
  images, and inline HTML `<sub>` and `<mark>`.
- Quotes printed at 800 levels, every shape at 300. Raw `<div>`, `<span>`,
  `<table>` and `<details>` add no level, and inline `<b>` or `<a href>`
  stay literal text, so they printed at 20000.
- The fixture `fixtures/deep-nesting.md` crashed printing and exporting to
  Word, in Paper and Midnight.

## Fix

The parser stops nesting past a limit, so no walk goes deeper than about
130 calls. Past 100 levels a quote or list adds no level: a quote's content
joins the deepest open block, and a list nested in an item continues that
item's list, so Markdown and HTML lists nest 50 levels (a list and its item
take two) and quotes about 100. Spans and other HTML elements stop 32 levels
deeper, their text joining the enclosing element; their close tags close
nothing. Ordinary documents never come near either limit.

## Checks

- `deep_nesting` (`nesting_tests`, new) parses twelve generated cases, 5000
  deep (20000 for `<mark>` and `<b>` in an HTML block, 1000 for Markdown
  lists, whose indentation grows): the tree stays at most 133 deep, every
  item and the innermost text survive in order, and each lays out at 1050
  and 650 px, exports to HTML and Word, copies as rich text and frees. The
  slowest, an HTML list of 5000 items, takes 1.8 s.
- The same test reads the fixture: ordinary four-level lists, three-level
  quotes and the task item keep every level; the 60-level Markdown and HTML
  lists nest 50 levels and go on in the deepest; the 1000-level Markdown
  quote nests 100 and the HTML quote 99; all text stays in order, on the page
  at both widths in Paper and Midnight, and in the HTML export.
- With the block limit raised to a million, five fixture checks fail and the
  test process crashes; with only the span limit raised, it crashes on the
  first span case.
- The fixed build printed and exported the fixture (17 pages in each theme,
  HTML, Word) and printed 53 generated documents, every shape at 1000, 5000
  and 20000 levels (Markdown lists at 1000, the HTML list with text at 1000
  and 5000), all without a crash; 20000 nested `<ul><li>` print 575 pages in
  13 s.
- All 77 other fixtures exported (HTML, Word, print pages, Paper and
  Midnight): 746 files identical to the build before the fixes of this day.
- The full suite passed, 53 of 53.

Evidence is in `out/check-1030/nest/`, `deep-before/`, `deep-fixed/` and
`exp-fixes/` (ignored).

## Limitations

- Deep lists and quotes still lay out as narrowing columns, as before: past
  about 25 levels at print width each level's text draws a character per
  line at the right edge. Nothing real nests that deep.
- Raw HTML `<blockquote>` around bare text drew nothing at any depth, before
  and after the fix, so the fixture puts its deep quote's text in a `<p>`;
  the fix that followed is in `html-blockquote-text-validation.md`.
- Past the limit, raw HTML that goes on inside an item after its nested
  list can land one level up, and an ordered list's flattened items continue
  its numbering.
- The Remote Desktop session was locked, so the window was not captured;
  print pages and exports use the same layout.
