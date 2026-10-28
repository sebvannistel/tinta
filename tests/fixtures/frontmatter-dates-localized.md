---
title: Localized dates and counts
created: 2026-01-03
updated: '2026-09-08T18:00:00Z' # shown as a relative date
reviewed: 2026-05-21
due: 2027-03-01
tags: [dates, plurals, languages, settings, strip]
owners: [Jane]
draft: false
---

# Dates and counts in the interface language

Show **created**, **updated**, **reviewed**, **due**, **tags** and **owners** in
*Settings → Frontmatter*, set `tags` and `owners` to *Count only* and `updated`
and `due` to *Relative date*, then switch the interface language. Every date,
relative time and count in the strip above should read in that language, and
the YAML itself never changes.

## What each language shows

| Language | Created (3 January 2026) | Reviewed (21 May 2026) | Tags | Owners |
| --- | --- | --- | --- | --- |
| English, United States region | Jan 3, 2026 | May 21, 2026 | 5 items | 1 item |
| English, United Kingdom region | 3 Jan 2026 | 21 May 2026 | 5 items | 1 item |
| Deutsch | 3. Jan 2026 | 21. Mai 2026 | 5 Einträge | 1 Eintrag |
| Français | 3 janv. 2026 | 21 mai 2026 | 5 éléments | 1 élément |
| Italiano | 3 gen 2026 | 21 mag 2026 | 5 elementi | 1 elemento |
| Русский | 3 янв 2026 г. | 21 мая 2026 г. | 5 элементов | 1 элемент |
| 日本語 | 2026年1月3日 | 2026年5月21日 | 5 件 | 1 件 |
| 简体中文 | 2026年1月3日 | 2026年5月21日 | 5 项 | 1 项 |
| 한국어 | 2026년 1월 3일 | 2026년 5월 21일 | 5개 | 1개 |

Relative dates count minutes, hours or days in the same language: *N days ago*,
*vor N Tagen*, *il y a N jours*, *N giorni fa*, *N дней назад*, *N 日前*,
*N 天前* or *N일 전*, and a future date reads *in N days*, *через N дня* and so
on, with the plural the number takes.

> The start page's recent files use the same words: a file opened before
> yesterday shows its month and day, such as *May 21*, *21. Mai* or *5月21日*.

- Month names come from Windows' own data for the language, in your regional
  format when that is in the same language.
- A list with one item is counted in the singular; Russian also has its
  [few and many forms](https://www.unicode.org/cldr/charts/latest/supplemental/language_plural_rules.html).
  1. *1 элемент*, *2 элемента*, *5 элементов*
  2. *21 день*, *22 дня*, *25 дней*

```yaml
# Literal example: dates in code stay as written
created: 2026-01-03
updated: 2026-09-08
```

All surrounding content must remain visible, including this final paragraph.
