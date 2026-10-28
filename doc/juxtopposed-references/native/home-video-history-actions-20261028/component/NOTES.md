# Home video-history actions: component evidence

On 2026-10-28, both bounded offscreen suites passed against the identities in
`source-identities.json`. Both exited 0, all 15 source guards and all 3 producer
guards remained unchanged, and neither suite reported a QML warning. No native
VLC process, desktop input, dependency installation or VLC build was used.

The production dashboard and collection suite checks real pointer and keyboard
input: right-click, Shift+F10 and Menu route the selected row and section;
unavailable video files retain their context menu while click/Return cannot
start playback; both video section options disable while busy. The existing
mixed-media, activation, virtualization, layout and Unicode checks also passed.

The separate page suite loads unchanged production `JuxtopposedHomePage.qml`,
`JuxtopposedHomeHistoryPage.qml`, `PageExt.qml`, dashboard, collection and card.
Its explicit adapters supply native ML models, menus, dialogs, MainCtx,
navigation, style providers and header presentation. These adapters record
calls; they do not implement history editing or manufacture a native success.
Checks cover:

- Continue Watching and Watch Again target the corresponding model for Remove
  and Clear. Cancel preserves the history; confirmed Clear invokes that model.
- The confirmation describes hidden search results and preservation of media
  files. The clear call has no search argument; the selected model's existing
  search stays intact. Actual category-wide backend selection requires separate
  native evidence.
- A menu opened before a job starts rechecks busy state when selected. Full
  history suppresses menus while busy, and failure/success signals update the
  visible status. The production `PageExt` handler dispatches its header More
  request to the history override, while Listen Again retains the global menu.
- Pin/Remove indices remain correct when Pin is unavailable. Unavailable rows
  can be removed. Available Continue and Watch Again retain their respective
  resume/from-start options. Audio does not expose video-history actions.

`home-sections-component.png` uses supplied generated artwork with the real
dashboard. The other screenshots show generated missing-file, error and busy
states in the page harness. Their blank header and default page/footer shell
come from the supplied presentation environment; they are not native menu,
dialog, desktop-theme or Figma fidelity evidence. No private media is included.

The terminal run is retained at
`build-windows/beta-home-sections-20261028/history-actions-components-r5/`.
`guarded-verification.json` records the exact two commands and bounded waits.
The three frozen producers are included here for review. To reproduce, copy
those producers into a **fresh ignored run directory** and execute
`run-guarded.py` with the established
`build-windows/homecoming-20261018/tools/VlcUiTests/Scripts/python.exe`.
Do not execute the wrapper in this frozen reference folder: it creates outputs
next to itself and expects the recorded home checkout. The installed Qt resource
utility only packages the actual test font at its existing qrc URL.

Earlier ignored runs remain preserved: r1 stopped during the Python URL/font
adapter load; its owned Python processes were terminated and recorded. R2 had
an adapter QML syntax error; r3 completed the assertions but rejected adapter
font/navigation warnings. R4 was the first clean page-suite pass. These are
harness failures, not production failures. R5 is the final guarded combined
result with frozen helpers and reviewed screenshots. All jobs have ended.

Native mutation/persistence, actual menu and confirmation presentation,
ordinary desktop interaction, active playback preservation and final-package
qualification remain separate requirements. No production defect was found by
this scoped component review.
