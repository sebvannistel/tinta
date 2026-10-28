# Even highlight bands across split runs

Validated on Windows with the Release build on 2026-10-16. No GitHub issue.
The seams were a known limit of the nested inline extension fix
(`nested-inline-extensions-validation.md`), which lets a highlight hold
superscripts, subscripts and strikethrough. They already showed around inline
math in a highlight.

## Cause

`layoutInlineContent` in `src/render.cpp` drew a highlight as one background
rect per piece: one per text segment of each run, and one per inline math box,
each padded 2 px on both sides. `themeHighlightBackground` is translucent
(alpha 0.45 in light themes, 0.28 in dark ones), so:

- where two pieces touch (`==marked x^2^==`, `==tight ($x$)==`), the 4 px the
  two paddings share was painted twice, as a darker strip;
- where a space separates two runs (`==a ~~b~~ c==`, `==a $x$ b==`,
  `==a [[Page]] b==`), the paddings cover the space exactly only if it is 4 px
  wide. Wider spaces (high DPI, zoom) left a pale gap, narrower ones (small
  zoom) a thin dark overlap.

## Change

The only change is to highlight drawing in `layoutInlineContent`. Every
highlight piece now joins the previous piece on its line instead of
overlapping it:

- Pieces of equal height merge into one rect. This covers text, scripts,
  struck words, links, and math no taller than the text.
- Beside a taller math box, the text band ends at the box, which keeps its
  full height. That joint is rounded to a whole pixel, so the two
  antialiased edges leave no seam on screen.
- Pieces of the same highlight also bridge the space between their runs. A
  space inside a single run was already covered this way. The pieces' common
  highlight is found through the elements' parent links. Separate highlights
  (`==first== ==second==`) never bridge; they are only trimmed where their
  paddings overlap.
- Inline code, ruby and images draw no highlight, so a band does not bridge
  across them. Inside a highlight, that can only happen with HTML `<mark>`.
- A line wrap starts a new band, so every wrapped line keeps its 2 px at both
  ends.

## Fixtures and automated checks

`fixtures/highlight-seams.md` splits highlights in several ways:

- with superscripts, subscripts and strikethrough;
- with a wiki link;
- with inline math, both with and without spaces;
- with a tall `\dfrac`.

These sit inside a heading, bold, italic, a link, a table, a quote, and
bullet, nested and ordered lists. Around them are neighbouring separate
highlights, inline code, a fence and a display equation. One long highlight
wraps over several lines.

`highlight_layout` (new, `tests/highlight_layout_tests.cpp`) lays out 16
highlights that each run from `alpha` to `omega`. They are the fixture's
cases, plus heading, list, quote and table variants. Each is laid out in
Paper and Midnight, at 100/150/200% DPI, 80/100/150% zoom and 420/650/1050 px.
For every line of each highlight it checks that:

- no two highlight rects overlap;
- the band starts 2 px before the line's first character, ends 2 px after its
  last, and neighbouring pieces meet exactly;
- every piece spans the text band, so a math piece is never cut to text
  height;
- a software raster of the bands alone has one colour along the middle of the
  band, within 1 per channel.

The long highlight must wrap over at least three lines at 420 px. The test
also checks that `==first== ==second==` keeps the space between the two
highlights wherever that space is wider than their padding. It lays out these
fixtures in every configuration and asserts that no highlight rects overlap:
`highlight-seams.md`, `math-mixed-layout.md`, `superscript-subscript.md`,
`footnotes.md`, `markdown-regression-control.md` and `math-inline-stress.md`.

With the old drawing the test fails:

- 8084 times with the nested inline fix applied: 3542 overlaps, 3564 joints
  that do not meet, and 978 uneven raster rows;
- 2460 times with this commit's parser alone, where only math and wiki links
  split a highlight: 456 overlaps, 1362 joints, and 642 uneven rows.

`tests/render_highlight_fixtures.ps1` exports print pages, HTML and DOCX for
the fixtures in each theme. It checks that the highlight fixture keeps its
blocks and all 27 highlights.

## Exports compared with the pre-change build

The old build was `9b5b11a` plus the uncommitted nested inline fix. The new
build was the same plus this change. Both exported print pages, HTML and DOCX
in Paper and Midnight for seven fixtures: `highlight-seams`,
`nested-inline-extensions`, `markdown-regression-control`, `footnotes`,
`math-mixed-layout`, `superscript-subscript` and `math-inline-stress`.

- HTML and DOCX: byte-identical for all 14 pairs.
- Print pages: `footnotes`, `markdown-regression-control`,
  `superscript-subscript` and `math-inline-stress` are pixel-identical, because
  their highlights are single runs.
- In the other three, only thin vertical strips at highlight joints change.
  Each strip is at most 15 px wide. The old pages are darker where pieces
  touch and paler at spaces. `math-mixed-layout` changes only at the two
  spaces beside `$\sqrt{x}$` (58 px).
- Print uses a white page in both themes, so Midnight's band was checked in
  the app instead.

## Desktop check

Computer use was not available in this session. Instead, the built
`tinta.exe` ran from isolated portable settings, and each window was captured
with `PrintWindow`, paging with PageDown for three screens. This covered
`highlight-seams.md`, `nested-inline-extensions.md` and `math-mixed-layout.md`
in Paper and Midnight, at 1100 and 560 px, for both the old and new builds.

- Old build in Midnight: a darker strip on both sides of every script and
  around `($x$)`, and pale gaps beside `√x`, the struck words and the wiki
  link.
- New build: one even band in both themes. The taller `\dfrac` band keeps its
  height, and at 560 px each wrapped line of a highlight is padded at both
  ends.
- A pixel diff of every capture pair finds only thin vertical strips at
  highlight joints. Everything else is pixel-identical, including
  `math-mixed-layout`'s first and third screens.

Captures and exports are in `out/highlight-seams/` (ignored).

## Limits

- The joint beside a taller math box lands on a whole layout pixel, which is
  a device pixel on screen. A fractional shift (centred or right-aligned table
  cells) or print scaling can leave a faint antialiased line there. Pieces of
  equal height have no joint at all.
- Two separate highlights one space apart still each pad 2 px. At some sizes
  they touch or leave a sub-pixel gap; only their overlap is removed.
- A code chip inside an HTML `<mark>` still interrupts the band, as before.
- A line can still break between a run and its script (`10` then `⁵` on the
  next line), as noted for the nested inline fix.

## Merged onto master (2026-10-28)

Merged onto `bf6b4e6` right after the nested inline fix, which the change
was developed on. Only the `input_tests` source list conflicted; the
drawing change applied as it was. Of master's newer inline elements,
inline HTML spans draw their highlight through the same two calls and
`<a id>` anchors draw nothing and take no width, so code, ruby and images
are still the only runs that break a band.

- `highlight_layout` passes; with master's drawing it fails 8082 times
  (3540 overlaps, 3564 joints that do not meet, 978 uneven raster rows).
  The full suite passes, 50 of 50.
- All 71 fixtures were exported as HTML, DOCX and print pages in Paper and
  Midnight with and without the change. HTML and DOCX are identical for
  every fixture. Print pages change only for `highlight-seams`,
  `nested-inline-extensions` and `math-mixed-layout` (page 2: 58 pixels in
  one-pixel columns beside `$\sqrt{x}$`); no changed run of pixels is wider
  than 5 px, and `tests/render_highlight_fixtures.ps1` passes.
- The built app showed `highlight-seams.md` in Midnight at 1100 px before
  and after: the darker strips at every script, at `($x$)` and at `10^5^`
  are gone, and the taller `\dfrac` and `a/b` bands join the text band
  evenly. At 560 px every wrapped line of the long highlight is padded at
  both ends; Paper looks the same.

Captures and exports are in `out/merge-1028/` (ignored).
