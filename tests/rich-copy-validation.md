# Rich copy validation: bold survives a paste into Outlook

Validated on Windows on 2026-10-27.

Copying from the viewer (Ctrl+C, the context menu's Copy, and Ctrl+C on the
docked preview in edit mode) now puts two formats on the clipboard: the same
plain text as before, and an `HTML Format` (CF_HTML) fragment in which
**bold** spans are wrapped in `<b>`. Rich paste targets such as Outlook, Word
and web mail read the HTML; plain-text targets are unchanged.

When Outlook has stored a compose font (`ComposeFontComplex` under
`HKCU\Software\Microsoft\Office\16.0\Common\MailSettings`), the fragment
carries that face and size as `mso-` properties, so a paste into Outlook
matches the mail around it instead of falling back to Times New Roman.
Browsers drop those properties, so web mail keeps its own font.

## Native regression checks

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
# Explicit opt-in: overwrites the clipboard (restored afterwards).
& build/rich_copy_tests.exe --clipboard-test
```

`rich_copy` lays out `fixtures/rich-copy-bold.md` at widths 1050 and 420 and
checks bold in paragraphs, across a soft line break, in list items, a
blockquote, a table cell and bold inline code; plain, italic, linked and
heading text stay unbolded; `< & > "` are escaped; a code block's `**` stays
literal; a partial selection inside a bold span opens and closes it; the
CF_HTML header offsets match the payload; non-ASCII text is UTF-8; a relayout
does not duplicate ranges. The clipboard run drives Ctrl+A and Ctrl+C through
`tests/keystroke.h` and reads both formats back. All 48 CTest suites pass.

## Checked in the built app

The built `tinta.exe` opened the fixture; Ctrl+A, Ctrl+C put `UnicodeText` and
`HTML Format` on the clipboard. Pasting into a new classic Outlook (HTML)
draft through its Word editor gave Aptos 11 pt text with exactly the bold
phrases bold, and plain, italic and linked text not bold. The draft was
discarded.

## Limitations

- Bold, italics and web and mail links carry over (italics and links since
  2026-10-28, below); headings and lists paste as the plain text they did
  before.
- Copy with nothing selected (Ctrl+C copies the whole document's text) stays
  plain text.
- New Outlook and web mail were not checked by hand; they read standard
  CF_HTML.

## Follow-up after merging (2026-10-27)

Tinta's copy of the fixture, taken from the real app (Release build,
isolated portable copy, Ctrl+A and Ctrl+C posted to the window), was pasted
into new documents in Word 16.0.20527 (Microsoft 365) through COM, and
compared with a paste of the plain text alone. Two differences came up:

- the lines were joined with `<br>`, so Word made the whole copy one
  paragraph with 32 manual line breaks, where the plain text gives a
  paragraph per line;
- each table cell's tab arrived as two tabs: Word counts the
  `mso-tab-count` span and the `&#9;` inside it.

Each line is now a `<div>`, an empty line `<div><br></div>`, and a tab is a
plain `&#9;`. The same paste then gives the plain text's 34 paragraphs line
for line, plus one more empty one at the end (the copy ends in a blank
line); no manual line breaks, 3 tabs, Aptos 12 pt with the Normal style's
spacing, and exactly the 13 bold phrases bold. Without the change
`rich_copy` fails 12 checks (6 at each width).

The clipboard run read the plain text with a single `OpenClipboard`; over
Remote Desktop, whose clipboard sync holds the clipboard for a moment after
each change, that read failed in 5 runs of 6. It now retries, like the HTML
read, and passed 10 runs of 10. The full suite passed 48 of 48.

Fonts, as observed on that machine:

- In Word, HTML without a font pastes in the document's own font (Aptos
  12 pt, its Normal style), like plain text. With the style the compose font
  produces (simulated as Aptos 11 pt; the registry was not changed), Word
  takes 11 pt instead. Classic Outlook has no profile there, so the Times
  New Roman fallback described above was not reproduced.
- Word also applies a face and size given only as `mso-ascii-font-family`
  and `mso-ansi-font-size`.
- In Chromium's editing pipeline (`insertHTML` into an editable element,
  the path a native paste takes), bold, lines and tabs survive. A
  `font-family` and `font-size` on the wrapper survive too, so they replace
  the editor's own font, while `mso-*` properties are dropped. Web mail
  editors that handle the paste themselves were not checked.

Evidence is in `out/verify-285/` (ignored).

## Compose font for Word-based editors only (2026-10-28)

The compose font used to be written as `font-family` and `font-size` on
the wrapper. Word applies those, but so does a browser's editor, where
they replaced the web mail's own font. `composeFontCss` now writes the face
as `mso-ascii-font-family` and `mso-hansi-font-family` (from the compose
style's own, or the first face of its `font-family` list) and the size as
`mso-ansi-font-size`. Word, and so Outlook, applies these, as the runs
above showed for a face and size given only that way; Chromium's editing
pipeline drops them. Markup in a value still gives no style.

`rich_copy` checks the style built from an Outlook-like `ComposeFontComplex`
(a Personal Compose Style in Aptos 11 pt, with a Times New Roman bidi face
that stays out), the fallback to the plain list's first face, and that a
value with markup, or no compose font, gives none. The exact new style was
not pasted into Word again: the harness only uses a clipboard it can put
back, and this time it held an image.

## Italics and links (2026-10-28)

The copy now carries italics as `<i>` and links as `<a href>`, nested as
link, bold, italic, so the tags always close in order; only the levels
that change close and reopen. Only links that lead outside the document
become links (`http:`, `https:`, `mailto:`); a link to a heading, a wiki
link or a file reference would lead nowhere from a mail, so it pastes as
its text. A link's address is escaped in its attribute.

The fixture gained an italic phrase in its intro, a link to a heading,
bold italic text and a bold link whose address has an `&`. `rich_copy`
checks the italic spans, the web link, the heading link as plain text, bold
italic as `<b><i>`, the bold link as `<a href="...&amp;..."><b>`, and that a
relayout records each italic span and web link once. Not recording
italics fails 6 checks; recording every link fails 4.

Word 16.0.20527 opened the copy of the whole fixture as an HTML file
(written by `rich_copy_tests --dump-html`, through COM, read-only, so the
clipboard, which held an image, was not touched): 2 hyperlinks, to
`https://example.com/` and `https://example.org/?a=1&b=2`, the heading
link not one of them, and italic runs on exactly "italic words", "italic"
and the bold "bold italic". Opening a file takes Word's HTML import as a
paste does, but it is not the paste itself.
