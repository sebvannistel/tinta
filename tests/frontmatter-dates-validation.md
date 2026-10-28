# Front-matter dates and counts in the interface language

Validated on Windows on 2026-10-28. No GitHub issue; the gap showed while
checking the Settings panel in every language: the date format option read
"Jan 3, 2026" in German, French, Japanese and the rest, because the property
strip wrote every date, relative time and count in English.

## Change

- **Calendar dates** come from Windows' own long date for the language
  (`LOCALE_SLONGDATE`), without the weekday and with the month abbreviated:
  `dddd, MMMM d, yyyy` becomes `MMM d, yyyy`, `dddd, d. MMMM yyyy` becomes
  `d. MMM yyyy`, and the Chinese, Japanese and Korean numeric forms keep
  their order. The locale is the user's regional format when it is in the
  interface language (en-GB, de-AT), else the language's own (en-US, de-DE).
- **Relative dates** ("3 days ago", "in 2 hours") and **list counts**
  ("5 items") are translated in every shipped language. A count's text holds
  its plural forms split by `|`, picked by the language's rule: one|other
  for English, German, Italian; French and Portuguese count 0 and 1 as one;
  Russian (and Ukrainian, Belarusian, Polish) one|few|many; Chinese,
  Japanese and Korean have one form. Text that falls back to English uses
  English's rule. The languages.ini template explains the `|` forms.
- English reads as before, except that a list of one is now "1 item" (it
  said "1 items"), and a British region gives "3 Jan 2026".
- The Settings format option shows its sample date the same way, so the
  fixed `fm.absolute` string ("Jan 3, 2026") is gone.
- The start page's recent files used a fixed `MMM d` in the region's
  locale, which read "1 3" for a Japanese or Korean region (their short
  month names are bare numbers). They now use the same month-and-day form
  (`LOCALE_SMONTHDAY`) in the interface language: "Jan 3", "3. Jan",
  "1月3日".
- The YAML values are never changed; HTML and DOCX exports still leave the
  strip out, and print shows it as the window does.
- Formatting for display moved from the model (`fm::formatValue`) to the
  strip (`frontmatterValue` in `frontmatter_ui.cpp`), so the parser and its
  lean test targets stay free of interface text. The model keeps the date
  arithmetic (`calendarDate`, `secondsSince`), moved unchanged.

## Fixtures and automated checks

`fixtures/frontmatter-dates-localized.md` has calendar dates in January and
May, a past and a future relative date, a five-item and a one-item list,
and a table of what each language shows, with a quote, a nested list, a
link, emphasis and a YAML code block around it.

- `i18n`: counts in English, German, French, Russian (1, 2, 5, 11, 21, 22,
  112, 124) and Japanese, and an untranslated language counting by
  English's rule; the date locale for a matching and a different region;
  13 long-date pictures shortened, including quoted letters and a quoted
  quote; exact dates in US and British English, Japanese and Korean, and
  for German, French, Italian, Russian and Chinese the locale's own
  abbreviated month and no weekday. Every count key has its forms in every
  shipped language, each with the English `%d`; the Russian table now has
  368 keys.
- `frontmatter`: the strip's text for relative dates in English (as
  before), German, Russian and Japanese, an RFC 3339 offset, list counts
  ("1 item"), absolute dates and a plain date; and the fixture's strip laid
  out in all eight languages at 420, 650 and 1180 px, every run inside the
  strip.
- Without the change, `i18n` fails 23 checks (12 pictures, the US, British
  and Korean dates, 3 weekdays, 4 Russian plurals) and `frontmatter` 7
  (every localized value, "1 item" and the absolute dates).
- The full suite passes, 50 of 50.

## Checked in the built app

The Release build ran from isolated portable copies with `created`,
`updated`, `reviewed`, `due`, `tags` and `owners` shown, on the fixture, in
Paper; the strip was read from screenshots on 2026-10-28:

| Language | Created | Updated | Due | Tags, owners |
| --- | --- | --- | --- | --- |
| English (en-GB region) | 3 Jan 2026 | 49 days ago | in 123 days | 5 items, 1 item |
| Deutsch | 3. Jan 2026 | vor 49 Tagen | in 123 Tagen | 5 Einträge, 1 Eintrag |
| Français | 3 janv. 2026 | il y a 49 jours | dans 123 jours | 5 éléments, 1 élément |
| Italiano | 3 gen 2026 | 49 giorni fa | tra 123 giorni | 5 elementi, 1 elemento |
| Русский | 3 янв 2026 г. | 49 дней назад | через 123 дня | 5 элементов, 1 элемент |
| 日本語 | 2026年1月3日 | 49 日前 | 123 日後 | 5 件, 1 件 |
| 简体中文 | 2026年1月3日 | 49 天前 | 123 天后 | 5 项, 1 项 |
| 한국어 | 2026년 1월 3일 | 49일 전 | 123일 후 | 5개, 1개 |

- At 560 px (Russian, German, Korean) the right group stacks under the left
  and every value fits.
- Settings → Frontmatter in all eight languages names the date format by
  the same sample (3. Jan 2026, 2026년 1월 3일); the French format menu
  lists 3 janv. 2026, Date relative and Normal, and the preview matches.
- The previous build, in Japanese and German, showed every value in
  English ("Owners: 1 items", "Created: Jan 3, 2026", "Updated: 49 days
  ago") and offered "Jan 3, 2026" as the format.
- The start page with three recent files opened on 21 May, 3 January and
  24 December: 5月21日, 1月3日, 12月24日 in Japanese; 5월 21일 in Korean;
  21. Mai, 3. Jan, 24. Dez in German; 21 May in English with this machine's
  en-GB region. The previous build showed May 21, Jan 3 and Dec 24 in
  Japanese.
- Printing the fixture in Japanese gives the strip as the window shows it,
  and the fixture's bytes were unchanged.

Captures are in `out/dates-1028/` (ignored).

## Limitations

- The start page's "1 3" needs a Japanese or Korean regional format; this
  machine's region was not changed, so it was seen through
  `GetDateFormatEx` alone: `MMM d` in ja-JP and ko-KR gives "1 3".
- Dates follow Windows' locale data, which can change between Windows
  versions (whether a month abbreviation ends in a period, say). The tests
  compare exact text only for the numeric forms and US and British
  English, and other months against Windows' own names.
- Plural rules cover the shipped languages, Polish, Ukrainian and
  Belarusian. A languages.ini language with other rules counts one|other,
  and a count past its last form uses the last one.
- A relative date is computed when the document is laid out, not kept
  live, as before.
