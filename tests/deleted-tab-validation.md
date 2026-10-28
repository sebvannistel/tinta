# Deleted file tabs

The General setting "Keep deleted files in memory" is off by default and
warns "Increases RAM usage; until Tinta closes". Its choice persists in
settings.ini as keepDeletedTabs. Enabling snapshots already open files;
disabling immediately releases all per-tab viewer source copies.

When enabled, tabs retain their last readable source in memory only. A missing original
can still be selected and edited. Save (including Ctrl+S) opens Save As when
the original or its parent folder no longer exists. The old filename is
suggested, but a destination must be explicitly confirmed; cancellation
keeps the text, path and editor open. Existing disk files take precedence on tab activation. Empty files
have a valid empty snapshot. Closing a tab or exiting the process releases
its source copy. No persistent viewer cache is written or read. The existing
session restore continues to skip missing files on the next launch.
Existing unsaved editor draft recovery is unchanged.

Fixture: `tests/fixtures/deleted-tab.md`, a mixed document with Cyrillic,
headings, a table, lists, emphasis, a quote, a link and code.

## Verification

- Built `tinta` and `input_tests` with Visual Studio 2026, MSVC x64 Release.
- Seven CTest checks passed: `deleted_tab`, `editor_features`, `input_shortcuts`,
  `file_fragment_navigation`, `tab_drop_position`, `tab_hover_card`,
  `search_results_panel`.
- The native deleted-tab test covers deletion while active, switching,
  editing, discarding edits, parked editor recovery, saving to recreate the
  document through an explicitly confirmed Save As, externally recreated files, deleted empty files, inactive
  session-seeded tabs, failed opens, releasing cached state on close, and
  the absence of a recovery copy after all in-memory tabs are discarded.
- It verifies that no `tab-cache/` directory is created.
- Native settings mouse input verifies enabling, disabling, immediate
  memory release, persistence of both values, and disabled-file behavior.
- Settings previews were rendered through a GDI-compatible Direct2D target
  and visually inspected in light/dark themes at widths 650 and 1050.
  The switch and full RAM warning fit inside the panel. PNGs live at
  `build/deleted-tab-memory-test-Release/settings-THEME-WIDTH.png`.
- Cached content is laid out at widths 500 and 1000 with themes 0 and 5.
  The native print harness produces `build/deleted-tab-memory-test-Release/cached-pages/page-1.png`.

The native Save As dialog is now exercised with a thread-local CBT hook in
the test process: cancel, a new destination, explicit reuse of the old name,
save-and-exit cancellation, and a deleted parent folder. Normal saves of
existing edited files are checked to bypass the picker. A 30-second wrapper
timeout bounds dialog failures. No production dialog hooks were added.
The fixture read normalizes CRLF to match the viewer's text-mode input.
Files already deleted before this version first read them cannot be recovered.

## Integration with upstream 3.9.0

Merged upstream master 9a6b209 on 2026-10-21. The only textual conflict was
in the input_tests source list; both deleted_tab_tests.cpp and the upstream
scroll_sync_tests.cpp are retained.

Built tinta and input_tests in x64 Release. Nine checks passed with the
normal Windows checkout: the seven checks above, ui_font_language and
wrapped_cursor_navigation. The additional upstream edit_preview_scroll_sync
check failed two heading-position assertions with the CRLF fixture (22 lines
off); it passed with that same fixture temporarily normalized to LF. The
original fixture bytes were restored after the check. No upstream test or
fixture was permanently changed to mask this limitation.

## Closing beside unavailable tabs (#275)

A native regression added on 2026-10-22 reproduced the invalid active index
and wrong document identity before the fix (seven failed assertions; no
full-app crash was claimed). It creates real native App instances, opens
copies of the mixed Markdown fixture, removes inactive files and closes the
active tab with retention disabled (the default).

The four cases cover a missing left neighbor, a missing right neighbor,
an available third tab beyond a missing neighbor, and multiple missing
survivors. Closing now tries available neighbors before removing the active
tab; if none can activate, its slot becomes a start page while missing tabs
remain available for explicit closure or later recreation. Dirty-close
confirmation also requires the requested tab to be active.

After the fix, tinta and input_tests built in MSVC x64 Release and all nine
checks listed under integration passed, including deleted_tab. Existing
active/inactive retention, recovery Save As and settings checks still run.

## Audit of failed activation callers (#275)

A follow-up native audit reproduced four further failed-activation errors
before correction: history applied the unavailable tab's scroll to the live
document, a missing tab armed a drag, the transfer receiver acknowledged a
path it did not activate, and draft recovery edited the wrong live tab.
Forward/reverse tab cycling also failed to get past an unavailable neighbor;
new native assertions reproduced both directions before correction.

Corrections and coverage:

- History and draft recovery require the requested tab to have activated.
  A draft for an unavailable tab opens separately without changing a live tab.
- Clicking an unavailable tab cannot arm a drag. Normal and drag-out closure
  share the same guarded neighbor selection.
- A bulk-close sweep stops if a launcher replacement did not shrink the tab
  list, preventing repeated closure when the kept tab is unavailable.
- Path-only transfers reject unreadable/missing paths and acknowledge only
  activation of the requested document. Cached missing documents cannot tear
  off; users must save them first. Deletion between arming and releasing a
  drag cancels the transfer without removing the source tab.
- Ctrl+Tab and reverse cycling skip unavailable neighbors.
- Native tests exercise the actual tear-off launch probe beside two missing
  neighbors, cached missing documents, deletion during a drag, bulk close,
  history, draft recovery, and failed tab clicks.

The expanded search_input_focus check originally blocked in the intentional
Save As dialog because it fabricated a nonexistent normal-save path. Its
fixture setup now creates an existing destination; this keeps its focus
assertions intact. The modal recovery behavior is separately checked by
SavePickerProbe. Its wrapper now has a 30-second timeout.

After the audit changes, MSVC x64 Release builds of tinta and input_tests
passed 15 checks: editor_features, ui_font_language, editor_context_selection,
file_fragment_navigation, wrapped_cursor_navigation, search_input_focus,
input_shortcuts, table_and_modifier_input, docked_preview_and_hints,
wide_table_layout, window_drag_and_title_space, tab_drop_position, deleted_tab,
tab_hover_card and search_results_panel. This was a targeted regression run,
not the complete repository suite. The upstream CRLF scroll-sync limitation
recorded above is unchanged and that check was not rerun in this audit.

## Follow-up after merging (2026-10-22)

On a machine with Windows' default "hide extensions for known file types"
and a slower dialog (a VM over Remote Desktop), the Save As checks failed
though Tinta behaved correctly:

- the dialog shows the suggested name without `.md`, so the checks now
  accept the name with or without its extension;
- the modern dialog ignores a name set with `SetWindowText` and returns its
  default on OK (which recreated the deleted original), so the probe now
  types the name with `WM_CHAR`;
- a name typed 25 ms after activation could be overwritten while the dialog
  was still filling in its default, so the probe now waits for the default
  name, types, and presses OK only once a later tick still finds the typed
  name.

The suite then passed three runs in a row in about 2.5 s, against 30 s of
picker timeouts before.

## Translations (#283)

Added on 2026-10-27 from go4399's patch on #283. The setting and its hint
showed in English in Chinese, Japanese, Korean, German, French and Italian;
only Russian had them. Four changes to the patch:

- the German, French and Italian lines spell their accented letters as
  escapes, since the sources build without `/utf-8` (the `i18n` source scan
  rejects raw text in a wide literal);
- French uses a straight apostrophe, *jusqu'à*, like every other French
  string;
- Italian uses the imperative, *Mantieni i file eliminati in memoria*, like
  the other Italian settings (*Mostra le notifiche*, *Segui il tema di
  Windows*);
- Korean attaches the particle to the name, *Tinta가 종료될 때까지*, like the
  other Korean strings, and its label spells its spaces as plain spaces.

`i18n` checks that both strings differ from English in all seven built-in
languages, and checks the Chinese label and the French hint word for word.
Without the new strings it fails 14 checks: 12 for the six languages and
the 2 exact ones. `tests/check_russian_translations.py` still reports
complete Russian coverage, and the full suite passed 47 of 47.

In the real app (Release build, isolated portable copies with `language=`
set, Ctrl+, posted to the window), Settings → General shows the translated
label and hint in Chinese, Japanese, Korean, German, French and Italian at
1100 px, with every accent intact. At 650 px the German and French label
and hint end in an ellipsis, like the other long German rows; nothing
overlaps the switch. Screenshots are in `out/verify-283/` (ignored).
