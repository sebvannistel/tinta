# Translations for every interface string

Validated on Windows with the Release build on 2026-10-28.

## Change

Every shipped language now has its own text for every interface key:

- Chinese, Japanese and Korean had 34 keys in English: the frontmatter
  settings (Settings → Frontmatter) and the theme editor's default name.
- German, French and Italian had 130: the frontmatter settings, the start
  page, the editor's context menu, rail and word count, review
  annotations, Pandoc export, file reference and update messages, and a
  few help rows.

Some keys read the same everywhere and stay out of the German, French and
Italian table, falling back to English as before: the window titles
(`title.*`), the key profiles Windows and Vim, and the sample date of the
absolute date format (`Jan 3, 2026`), since the property strip writes
dates in English.

Fixed in strings that already shipped:

- Korean "자동 줄바꿀" (word wrap) in three places, now "자동 줄바꿈".
- Japanese settings footer "変更はぐすに反映", now "変更はすぐに反映"
  (changes apply immediately).
- Japanese editor section "編集エディター" (edit editor), now
  "エディター".
- Korean search counter "%zu개 중 %d번째" printed the current match and the
  total the other way round: the arguments come as (current, total) and
  `swprintf_s` cannot reorder them. It now reads "%d / %zu개", like the
  Japanese.

The frontmatter label toggle (NAME / NOM / ...) is 38 DIP wide instead of
32, so French "Non" no longer cuts to "N...".

## Automated checks

`i18n` (`tests/i18n_tests.cpp`) now checks, for Chinese, Japanese, Korean,
German, French, Italian and Russian, that every key other than the ones
above has the language's own text (`languageTranslatesKey`), and that each
keeps the English printf arguments in their order; it found the Korean
counter. Spot checks cover new strings in each language and the typo
fixes. `ui_font_language` and `russian_interface` pass; the full suite
passes.

## Checked in the built app

Screenshots of the built `tinta.exe` (isolated portable copies, theme 0,
`out/verify-i18n/`, ignored):

- Settings → Frontmatter on `tests/fixtures/frontmatter-settings.md` in
  German, French, Italian, Chinese, Japanese and Korean at 1050 px, and in
  German and French at 650 px (the compact layout): titles, hints, column
  headers, type names, side buttons, label toggles and formats all fit.
  French "Non" was cut before the toggle was widened.
- The start page in German, French and Italian, and the editor's context
  menu in German.
