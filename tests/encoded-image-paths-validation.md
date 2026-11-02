# Percent-encoded local image paths (#293, PR #294)

Validated on Windows on 2026-10-30. Reported by tferi99: an image whose
relative path is percent-encoded, as `folder%20with%20space/yoda.png`,
showed as its alt text in 3.9.5, where VS Code and Notepad++ show it. The
reporter's PR #294 decodes local image paths once, with the link decoder
already used for file links, in the viewer and the HTML and Word exports.

## Reproduction

The PR's fixture, `fixtures/encoded-images-293/index.md`, printed with the
3.9.5 build (`--printpages`, isolated portable copy): the five images whose
paths need decoding (spaces in a folder, UTF-8 `caf%C3%A9`, `hash%23name`,
`literal%2520`, an encoded extension) showed as "[image: ...]"; the plain,
`plus+name`, `malformed%zz` and `nul%00` ones rendered.

## Security analysis

- `decodeLinkComponent` keeps `%00` encoded, so no decoded path carries a
  NUL that native file APIs would cut short (`secret.txt%00.png` stays a
  `.png` path). Malformed escapes stay literal and `+` stays a plus.
- Decoding can produce `/`, `\`, `:` and `..`, so a decoded path can be
  absolute, a network path or climb out of the document's folder. Each of
  those can already be written literally in a document, so the PR gives a
  document no reach it did not have. Remote `http(s)` sources keep their
  encoding.
- The cache key stays the raw source, and the decoding happens once in
  each of the three places that open the file.
- The fixture's nine PNGs are one 193-byte image (IHDR, sRGB, gAMA, pHYs,
  IDAT, IEND, valid checksums, nothing after IEND).

Found on the way, before this PR: a local image path that is absolute,
network paths included, replaces the document's folder. A document can
make Tinta open `//host/share/x.png` when it is viewed or exported, as
`//localhost/C$/...` did here, and Windows signs in to such shares with
the user's credentials. That was fixed separately: a document now opens
network paths only on its own server (`network-paths-validation.md`).

## Checks

- `image_cache_and_decode` (the PR's `encodedImagePaths`) lays the fixture
  out at 1050 and 650 px: all nine images render, the table, code block and
  closing paragraph stay, nothing falls back to alt text; the HTML export
  embeds nine PNGs and the Word export nine drawings; a remote URL keeps
  its encoding. Without the viewer's decoding, 4 checks fail.
- The merged build printed the fixture with all nine images and exported
  it: the HTML embeds nine PNGs with no alt-text fallback, the Word file
  holds nine media files.
- The full suite passed, 51 of 51.

Evidence is in `out/check-1030/repro293/` and `out/check-1030/unc/`
(ignored).

## Limitations

- The reporter's own test zip was not downloaded; the PR's fixture covers
  its case (`folder%20with%20space`).
- The Remote Desktop session was locked, so the window itself was not
  captured; the print pages are drawn by the same layout.
