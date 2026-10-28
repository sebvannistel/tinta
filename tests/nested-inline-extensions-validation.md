# Nested inline extension spans

Validated on Windows with the Release build on 2026-10-16. No GitHub issue;
the bug showed in `fixtures/superscript-subscript.md`, where
`~~struck x^2^~~` rendered as struck text with literal carets.

## Cause

The Obsidian/Typora post-pass in `src/markdown.cpp` (`splitInlineExtensions`)
split Text runs into Highlight, Strikethrough, Superscript and Subscript spans,
but never looked inside a span it had just made. Spans md4c creates (bold,
italic, links) were already searched, so `**Bold x^2^**` worked while
`~~struck x^2^~~`, `==mark H~2~O==` and `x^a~i~^` kept their inner delimiters.

## Change

- Each new span's content is split by the same pass. Inline math in the
  content stays one opaque node at every level; code never reaches the pass.
  A span's content cannot hold its own delimiter, so nesting ends within
  four levels.
- `~~a ~b~~~`: when a strikethrough's closing `~~` is followed by another
  tilde and a subscript inside would close on the first tilde of the run, the
  subscript takes it and the strikethrough takes the last two. `~~a~~~` still
  leaves one literal tilde, and `~~x~~~y~` is still a strikethrough followed by a
  subscript.

## Fixtures and automated checks

`fixtures/nested-inline-extensions.md` nests scripts, marks and strikethrough
in each other inside headings, emphasis, a link, a table, a quote, bullet,
nested and ordered lists. It also has inline math, inline code, a code fence
and a display equation. The existing `fixtures/superscript-subscript.md` line now
renders as intended.

- `document_types` compares the parsed tree shape for 13 sources: each
  nesting pair, scripts in scripts, shared tilde runs, math inside nested
  spans (`~~old ==$x==y$ mark== end~~`), code beside a nested span, and
  nesting inside md4c bold. Without the fix 11 fail; the two guard cases
  (`~~a~~~`, `~~x~~~y~`) pass both ways.
- `inline_styles` checks that a superscript in strikethrough and a subscript in
  a highlight get the small face, keep the outer decoration, consume their
  delimiters, and that later text returns to plain body style (4 failures
  without the fix).
- `superscript_and_learn` lays out `~~base^SUP^~~ ==base~SUB~==` and
  `==base^SUP^ ~~base~SUB~~~==` with real DirectWrite formats in Paper and
  Midnight, at 100/150/200% DPI, 80/100/150% zoom, and 650 and 1050 px widths.
  It checks that both script runs exist, use the small font, and that the
  subscript sits below the superscript. With the old parser it fails 72 times,
  once for each of the two sources in all 36 layouts.
- `tests/render_superscript_fixtures.ps1` now also exports the nested fixture,
  and asserts nested `<del>…<sup>`, `<mark>…<sub>`, `<mark><del>…<sup>` HTML and
  all 7 equations. It also asserts the repaired line in the script fixture.

`cmake --build build --config Release`: no warnings.
`ctest --test-dir build -C Release`: **35/35 passed**.

## Exports compared with the pre-fix build

The old build (HEAD `9b5b11a`) and this build exported print pages, HTML and
DOCX for ten fixtures in Paper and Midnight: `superscript-subscript`, the new
fixture, `markdown-regression-control`, `tab-drop-position`, `footnotes`,
`math-mixed-layout`, `math-inline-stress`, `reading-view-242`,
`word-count-240` and `theme-colours`.

- Eight fixtures without nested spans: every page, HTML and DOCX file is
  byte-identical in both themes.
- `superscript-subscript`: HTML changes only `<del>struck x^2^</del>` to
  `<del>struck x<sup>2</sup></del>`. DOCX splits the run into a struck run and
  a struck superscript run, and only print page 1 changes.
- The new fixture's HTML text has no `^`, `~` or `==` outside code and math,
  except the one tilde that `~~a~~~` deliberately leaves.

## Desktop check

Computer use was not available in this session. Instead, the built
`tinta.exe` ran from an isolated settings directory with the fixtures open,
and each window was captured with `PrintWindow`, paging with PageDown:

- Paper at 1100 px: the whole nested fixture, through the final paragraph.
- Midnight at 560 px: the whole nested fixture. Nested strikethrough and
  highlight continue across wrapped lines in the paragraph, list item and
  table cells.
- Paper at 560 px: `superscript-subscript.md`, including the repaired
  mixed-styles line.

Captures are in `out/nested-inline/app/`, exports in
`out/nested-inline/compare/` (both ignored).

## Limits (unchanged by this fix)

- A highlight is drawn per run, with 2 px padding on each side, in a
  translucent colour. Where a highlight now holds a script, the overlaps paint
  darker strips around the small run, most visible in Midnight. The same
  strips already appear around inline math in a highlight.
- Scripts in headings use the body script size, so they are small next to
  heading text. A narrow window can also break a line between `H` and its
  `~2~`, because lines may break at any run boundary. Both show the same way in
  the unchanged `superscript-subscript.md` heading.
- Spans still do not cross a soft line break or an md4c span boundary:
  `==a **b** c==` and a highlight continued on the next source line stay
  literal.

## Merged onto master (2026-10-28)

The change was written on `9b5b11a` and merged 85 commits later onto
`bf6b4e6`, where `splitInlineExtensions` had gained inline HTML spans as
opaque parts of a run (#263) and range tildes such as `50~100` (#277). The
recursive call now goes through master's `SiblingRun`, so a nested span's
content can hold inline HTML spans and math, both moved whole.

- `document_types` has two more cases: `~~H<sub>2</sub>O and x^2^~~` nests
  the HTML subscript and a superscript in the strikethrough, and
  `==50~100 mm and H~2~O==` keeps the range tilde as text inside the
  highlight while `H~2~O` still becomes a subscript. Without the fix 13
  cases fail; `inline_styles` still fails 4 checks and
  `superscript_and_learn` 72, as before.
- The full suite passes, 49 of 49.
- All 71 fixtures were exported as HTML, DOCX and print pages in Paper and
  Midnight with master and with the fix. Only `superscript-subscript` and
  `nested-inline-extensions` change, in both themes. Three Paper exports
  that differed in the first run (`docked-preview-245` pages,
  `mermaid-c4-labels` DOCX, `wide-table-239` pages) were identical between
  master and the fix when exported again, and between two master runs.
- The built app showed the nested fixture in Paper at 1100 px and in
  Midnight at 560 px, through its final paragraph: every nested script is
  small and raised or lowered inside its strikethrough or highlight, and
  wrapped list items and table cells keep their decoration. Master's build
  showed the same lines with literal `^`, `~` and `==`.

Captures and exports are in `out/merge-1028/` (ignored).
