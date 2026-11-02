# Tables and other blocks inside list items (PR #292)

Validated on Windows on 2026-10-30. shinfd's PR #292: a list item holding a
table showed the cells run together as plain text, and the item's
paragraphs out of order. The item's children were split into inline and
block groups laid out in two passes, and an item without a paragraph laid
everything out as inline text.

The PR lays a list item's children out in source order: runs of inline
content, then each block through the normal block layout, so tables,
headings, rules, quotes, code and nested lists keep their own layout. A
marker or task checkbox aligns with a heading that opens the item. Raw
HTML `<li>` items with their optional `</li>` left out stay separate. Print
page breaks keep every text run on one page, list markers included.

## Security analysis

- The changes are layout arithmetic, one parser rule and one page-break
  rule over structures the renderer already builds. No file, network,
  registry or process access is added; the new test does none either (it
  runs as an isolated portable copy).
- The `<li>` rule walks up from the open element only to the HTML
  fragment's root or the nearest list, and closes at most one item through
  the existing `closeElement`, which never pops the stack's last entry.
- Index use is bounded: the heading-first marker shift checks
  `headingStart < headings.size()`, moves only the runs, lines, rects and
  task rects recorded between two snapshots of the same item, and heading
  levels come from md4c or `<h1>`..`<h6>`.
- `firstListContent` recurses into HTML wrappers, as the layout already
  does. Crafted documents behaved the same with and without the PR: 4000
  nested `<div>` in a list item and 400 nested Markdown lists printed
  fine; 1000 nested raw HTML `<ul><li>` crashed both builds (access
  violation) and 4000 hung both past 90 s. That crash predates the PR; the
  nesting limit that fixed it is in `deep-nesting-validation.md`.

## Checks

- The PR's `list_block_order` test, the extended `html_span` and
  `print_and_word_export` checks and the full suite passed, 52 of 52, with
  #290's list copy, #289's drag and the rest on top.
- All 77 fixtures were exported (HTML, Word, print pages) in Paper and
  Midnight with and without the PR: 730 files identical. Besides the PR's
  own fixture, only `markdown-regression-control`'s print pages changed:
  the page break used to cut through a "Note" callout, drawing its title at
  the foot of page 1 and again on page 2; now the callout starts page 2
  whole.
- The PR's fixture printed with the merged build: item 7's paragraph,
  table, heading, rule, quote, code and nested list in source order, the
  table with its rows and columns; before, the table and heading text ran
  onto the item's first line ahead of its paragraph and the rule was
  missing.

Evidence is in `out/check-1030/exp-base/`, `out/check-1030/exp-292/` and
the `compare-*.png` captures (ignored).

## Limitations

- The Remote Desktop session was locked, so the window was not captured;
  print pages and exports are drawn by the same layout.
- Raw HTML nested about a thousand lists deep crashed, as before; it was
  fixed separately.
