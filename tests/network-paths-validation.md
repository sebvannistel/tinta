# Images and links on another computer's share

Validated on Windows on 2026-10-30. Found while checking PR #294 (#293): a
document could make Tinta open a file on any computer's share just by being
shown, printed or exported. Windows signs in to the share's server with the
user's credentials, so a document from anywhere could collect them.

## Reproduction

With the build before the fix (3.9.5 with #292 and #294), from an isolated
portable copy:

- A local document with `![Loop image](//localhost/C$/.../plain.png)`
  printed the image, read through this computer's administrative share
  (`out/check-1030/net-before/`).
- With each guard below removed in turn, the loopback checks show what else
  reached the share: a file link written with mixed slashes
  (`/\server/share/notes.md`) was looked up and badged live, hovering or
  clicking a wiki link `[[//server/share/notes]]` looked it up, and an
  untitled note's HTML and Word exports opened `/localhost/C$/...` (the
  exports put one more separator in front).

## Fix

One rule, in `include/network_path.h`: a document opens files on this
computer, and network paths only on the server its own folder is on.
`pathHost` names a path's server: two leading separators of either kind
(`\\nas`, `//nas`, `/\nas`, `\/nas`), and `\\?\UNC\nas`, `//?/UNC/nas` or
`\\.\UNC\nas`; drive paths (`C:\`, `\\?\C:\`) stay on this computer, and
other device paths (`\\.\pipe\`, `\\?\GLOBALROOT\`) are refused.

It applies to the viewer's images (before the cache, so a file another
document opened does not show here either; Tinta's own rendered PlantUML
picture is exempt), the HTML and Word exports' images, file-link badges,
and wiki links on hover and click. A blocked image keeps its placeholder,
now in the interface's language, with the note "network path not opened";
the note has a translation in all eight built-in languages.

## Security analysis

- Percent-encoded paths are decoded before the check (#293), so
  `%5C%5Cserver` is a network path too. Three leading separators are no UNC
  path to Windows (`\//localhost/C$/...` does not open) and are refused
  anyway.
- `..` cannot leave a server: Windows keeps `\\server\share` as the root, so
  a relative path stays on the document's server.
- Server names compare as written, ignoring case: `\\nas`, `\\nas.local` and
  its IP address count as different servers. WebDAV forms (`\\host@SSL\`)
  are UNC paths and follow the rule. A share mapped to a drive letter opens,
  since the user mapped it.
- Unchanged: remote `http(s)` images download through URLMon, which signs in
  automatically only to intranet sites, as browsers do; raw HTML `<img>` is
  not drawn by Tinta; PlantUML renders with its sandbox security profile;
  clicking an ordinary link to a share opens it, as asked.

## Checks

- `image_cache_and_decode` (`networkPaths`) checks `pathHost` and
  `mayOpenFromDocument` on every spelling above. It lays out
  `fixtures/network-paths/index.md` at 1050 and 650 px: only the local image
  opens; eight placeholders (paragraphs, a table cell, a list item, a quote)
  carry the note; nothing on another computer is cached; both file links to
  the share badge missing while the local one stays live; the table, code
  block and closing paragraph stay. The HTML export embeds only the local
  image and keeps the share's address as written; the Word export embeds
  one image; hovering the share's wiki link previews nothing.
- Through `\\localhost\C$`, when it answers (it did here): a local document
  leaves the share's image unopened, badges its file link missing, and
  neither previews nor opens its wiki link; the same document opened from
  the share shows the image, finds the file, previews and opens the note.
  An untitled note's exports leave `/localhost/C$/...` unopened.
- With each guard removed in turn, its checks fail: viewer images 4 (at
  both widths), HTML export 1, Word export 1, file links 1, wiki hover 1,
  wiki click 1.
- The fixed build printed the loopback document with
  `[image: Loop image — network path not opened]` where the old build drew
  the share's image; the relative image draws in both
  (`out/check-1030/net-fixed/`).
- All 77 fixtures exported (HTML, Word, print pages, Paper and Midnight)
  identical to before: 746 files. The full suite passed, 52 of 52.

## Limitations

- Pandoc exports let Pandoc read the document's images itself.
- A document on a mapped drive does not count as being on that drive's
  server: absolute `\\server\...` images in it stay closed, relative ones
  open.
- The HTML and Word exports still leave out an absolute network image even
  for a document on that server, as before (they resolve it inside the
  document's folder); the viewer shows it.
- The Remote Desktop session was locked, so the window was not captured;
  print pages and exports use the same layout.
