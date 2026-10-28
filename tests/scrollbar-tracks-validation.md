# Scrollbar tracks (#287)

Validated on Windows with the Release build on 2026-10-28.

## Report

- The document's scrollbar is drawn below the tab strip, but taking its
  thumb to the top of that track did not reach the start of the document:
  the pointer had to go on into the tab strip.
- The Contents panel's scrollbar could not be dragged; it only showed the
  list's position.

## Cause

- The vertical bar is drawn on a track from the foot of the tab strip
  (40 DIP, none in zen mode) to the window's foot, or to the horizontal bar
  when there is one. A drag and a click on the track worked out the thumb
  from the whole window height instead: a longer track, a longer thumb, and
  a thumb position measured from the window's top. Taken at its middle and
  lifted to the top of the drawn track, the thumb left the page 455 px short
  of its start in the fixture below; a click at the top of the track, 141 px.
- The horizontal bar's drag and track click likewise ignored the 14 DIP the
  vertical bar takes from its track, and both used a 30 px minimum thumb
  where the drawing uses 30 DIP.
- The thin bars beside the Contents, the file browser and the search
  results were drawn only. They take a press's band in the same place, so
  a press there acted on a row instead (or did nothing).
- The browser's wheel worked out its range from row sizes the card no
  longer uses; in a folder of fewer than about 40 entries it could stop a
  row or more short of the last one.

## Change

- `ScrollbarGeometry` (`sidepanels.h`) is a bar as drawn: track, thumb and
  scroll range, in window coordinates. `documentScrollbarV/H` give the
  document's two bars; drawing, track clicks and drags all read them, so the
  thumb stays under the pointer and the track's ends are the document's.
- `panelListScrollbar` is the same for the side panels' lists. Their bars
  now drag, take track clicks (the thumb centres on the press), light up and
  widen under the pointer, and show an arrow there. The release ends the
  drag without opening a row or closing the panel; lost capture, focus loss,
  cancel mode and Escape end it too, and the stray release after a cancel is
  swallowed.
- Where the card's edge resizes the panel (the browser; the Contents slot on
  the left), the edge keeps its resize band and the thumb is drawn clear of
  it, as the document's thumb is beside a divider.
- The browser's wheel uses the card's own range (`folderBrowserMaxScroll`).

## Automated checks

`contents_and_panel_resize` (`tests/sidepanel_tests.cpp`):

- the document's vertical bar at 1x, 1.5x and 2x, with and without a
  horizontal bar, in and out of zen mode: the track runs from the tab strip
  to the window's foot or the other bar; the thumb taken at its middle and
  lifted to the top of the track shows the start, taken to the foot shows
  the end, stays under the pointer midway, and a click on the track centres
  it on the press;
- the horizontal bar beside a pinned Contents on the left: its track runs
  along the viewport short of the vertical bar, and its ends are the
  content's ends;
- the Contents list (right and left, pinned and not, 1x and 1.5x) and the
  browser list (pinned and not): an arrow over the bar, which lights up; a
  press takes the thumb where it is; the foot of the track is the list's
  end; dragged up past the panel the list is at its start and the page has
  not moved; the release over a row ends the drag without jumping to a
  heading, opening a file, entering a folder or closing the panel; a press
  on the track centres the thumb; lost capture ends the drag and the release
  after it is swallowed; a resizing edge keeps its band;
- the wheel takes a short folder's list to its last row.

`search_results_panel` (`tests/search_panel_tests.cpp`) drags the results
list's bar to its end, releases over a row without selecting a match, and
clicks its track. `split_scrollbars` now takes its expectations from the
same geometry.

With `src/input.cpp` from before the change (drawing and geometry kept), the
side-panel run fails 116 checks and the search panel run 3. The full suite
passes 48 of 48.

## Checked in the built app

`tests/fixtures/scrollbar-tracks-287.md`: 50 headings at two levels, a
wide table with bold, code and links, a long code line, quotes, nested
lists and Python blocks. The built `tinta.exe` ran from an isolated portable
copy; mouse messages were posted to its window. The Remote Desktop display
was minimized, so neither screen captures nor `PrintWindow` showed the
window: each scenario ran in a fresh instance, closed with `WM_CLOSE`, and
was read back from the reading position it saved for the file.

| Scenario (1050 x 820) | Before | After |
| --- | --- | --- |
| End key (the page's end) | 10023.6 | 10023.6 |
| End, thumb lifted until its top meets the track's top | 455.7 | 0 |
| End, a click at the top of the track | 141.1 | 0 |
| Tab, Contents bar dragged to its foot, last row clicked | 6033.2 | 10023.6 |

The last row is the final heading only when the list has scrolled to its
end; before, the bar did not move and the click opened a middle section.
At 650 x 700 the fixed build gave 0, 0 and the page's end (11231.6) as
well. Scripts and output are in `out/verify-287/` (ignored).

Later the same day, with the display available, screenshots of the built
app (`out/verify-287/shots*/`, ignored) showed the Contents bar at the
panel's right edge, darker and wider under the pointer, at 1050 and 650
px wide, and the file browser's bar in a folder of 47 entries, lit under
the pointer and drawn clear of the panel's resize edge.

## Limitations

- The editor's own scrollbar (edit mode, left pane) runs the full window
  height under the tab strip; its drawing and dragging already agree, and
  it is unchanged.
