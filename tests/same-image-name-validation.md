# Same image name in two folders

Validated on Windows on 2026-10-30. Found while working on how Tinta opens
a document's images: the image cache was keyed by the path as written, not
the file it names. Two notes in different folders that both show
`shot.png` drew whichever image loaded first, in both tabs, for as long as
it stayed cached (256 images or 64 MiB; a new window starts empty).

## Fix

A local image is cached as the file it resolves to. A remote image keeps
its address as the key, which its download completes against.

## Checks

- `image_cache_and_decode` (`imagesPerDocument`) lays out
  `fixtures/same-image-name/first/notes.md` and `second/notes.md` in turn,
  twice, at 1050 and 650 px. Each shows its own image four times (a
  paragraph, a table cell, a list item, a quote) at its own width, 120 or
  240 px, with the table, code block and closing paragraph in place.
- Keyed by the written path again, 4 checks fail: the second note shows the
  first one's image.
- All 77 fixtures exported identical to before (746 files); the full suite
  passed, 52 of 52.

## Limitations

- The Remote Desktop session was locked, so switching tabs in a real window
  was not captured; the test lays the notes out through the same code.
