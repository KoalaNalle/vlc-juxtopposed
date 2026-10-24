# Library header, compact rows and Music categories checkpoint

Recorded 14 October 2026. This advances milestones 2/3 of the complete 0–8 Goal. Their native journeys and visual acceptance remain open. Original design and artwork: [Juxtopposed](https://www.youtube.com/@juxtopposed).

## Reference and implementation

The saved 11:45/11:55/12:10 walkthrough frames in [reference notes](reference-notes.md) establish back/forward, breadcrumbs, count, sort/view/search, compact thumbnail rows and Music's Add/Songs/Albums/Artists/Genres destinations. The header's 72px height, 44px rows, 32px thumbnails, 12px title gap and 28px content inset are provisional dimensions inferred from the video. The global title strip, full folder hierarchy and drawer/mini-player variants remain separate work.

- `JuxtopposedLibraryHeader` provides back/forward, breadcrumbs, actual item count, sort/view/search, clear-search and the native main menu. `PageExt` connects it to VLC's History, SortCtx and SearchCtx. Model search updates do not trigger another user action. Compact headers expand the search field and return focus to content when dismissed. The traditional header remains available through the existing opt-out.
- NavigationHistory now retains forward entries and their QQmlPropertyMaps when going back. Forward reuses the stored properties; a new navigation branch clears the forward path. Stale network-tree entries are skipped. This preserves properties written by views; it does not add scroll/search persistence to every view.
- VideoAll's redesign grid uses the existing ExpandGridView, native model, selection, drag and context-menu paths with compact media rows. Group/folder activation still passes through the existing adapters. The rows accept the factory's view/model/index/selected properties, select with modifiers, activate by keyboard or left double-click, and keep right double-click as a context action.
- Songs shares the actual MLAudioModel and selection with VLC's table view, exposing compact rows in grid mode. Native sorting/search and table mode remain available. Its drag item follows the active view.
- Music now opens a category page in redesign mode. Add calls the folder picker; Songs, Albums, Artists and Genres open the corresponding native routes. Its count represents the four category destinations. This does not port the complete album/artist/genre content layouts.

The preserved original sort SVG and Inter font are used. Other icons currently use VLC's glyph font and differ from the designer's original icons; individual exports remain needed. Product items come from native models, with no demonstration media added to the app.

## Build and verification

The Windows x64 Qt 6.8.3 incremental build exits 0. Autotools and Meson register all five new QML types. The generated native resource lists include JuxtopposedLibraryHeader, JuxtopposedMediaRow, JuxtopposedCategoryList, JuxtopposedVideoGrid and MusicCategoriesDisplay. The separately staged Qt plugin SHA-256 is `9cd4dd7e42580a8677187509ba145e597a0ea5943c53e2d32e2b205f251ed81a`.

The DLL is in `build-windows/slice-package/plugins/gui/libqt_plugin.dll`. Its preceding DLL is retained as `.before-library-20261014`, and the user's working `build-windows/package` is preserved. Build status/logs are `gui-build-status.txt` and `native-gui-rebuild.log`.

All eight Qt component suites pass locally. The new library suite checks header actions/search synchronization/focus, visible-control bounds, row modifier selection/keyboard/double-click/drag/current state, actual category destination declarations and layouts at 100/150/200% scale. It reads the real VLC glyph declarations. These tests exercise presentation contracts with supplied state; they do not operate native library models.

Ten changed QML files pass the Qt parser. The native MinGW/QtTest NavigationHistory regression reports seven passes (five test cases plus setup/cleanup), zero failures. It verifies empty actions, retained properties and focus reason, loader normalization, branching, stale forward entries and availability notifications. NetworkTreeItem was moved unchanged into its own header so the history test uses the actual gadget without linking an unrelated model.

[Header component capture](header-component.png) and [Music category component capture](music-categories-component.png) are isolated Qt renders, **not Windows VLC application captures**. The category image demonstrates disabled Add and keyboard focus. Additional captures and full-suite previews remain under ignored `build-windows/library-component-checks` and `library-checkpoint-previews-20261014`.

## Open acceptance gates

The existing native GUI still shows a Windows Firewall permission prompt. No security input was automated and no additional GUI instance was launched to bypass it. The newly staged library presentation has not yet passed desktop interaction or whole-app visual review. Native picker → indexing → browsing → search/clear → play → return/resume, category transitions, sort/grouping, drag/drop, grid/list focus and narrow-window checks remain required.

The prior [native backend diagnostic](implementation-checkpoint.md) proves indexing/search/mixed playlist order/restart using isolated fixtures. It does not establish these new QML adapters. Original icon exports, the global title strip, complete folder breadcrumbs and the remaining library layouts are still open. No whole-app fidelity score or milestone completion is assigned. Continue the full [vertical slice plan](../../juxtopposed-vertical-slice-plan.md).
