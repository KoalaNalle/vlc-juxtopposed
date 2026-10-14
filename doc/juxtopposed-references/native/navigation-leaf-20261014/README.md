# Navigation leaf properties and selection — 14 October 2026

The registered repair passes eleven offscreen component checks and the actual
QCore NavigationHistory test passes all nine ordered results. Two preserved
negative runs establish the issues independently: original PageLoader passes
a saved row property into ancestors that do not own it; the first pending-index
repair misses countChanged-before-loading-false when the view already exists.
Desktop Home/playlist/Back/Forward acceptance is pending.

| Variant | Observed result | Receipt |
| --- | --- | --- |
| Original production baseline | Row7 restoration fails; three PageLoader:99 unknown initialIndex errors | component/baseline-verification.json |
| Earlier pending mechanism | First two gates pass; loading-settled pending assertion fails, exit1 | component/loading-before-verification.json |
| Corrected ignored shadow | All eleven gates pass; zero unexpected QML messages | component/fixed-shadow-verification.json |
| Registered integrated source | Same eleven gates pass with matching source identities | component/integrated-verification.json |
| Actual QCore History | Exact9 passed,0 failed,0 skipped,0 blacklisted; exit0;2ms | native/results.txt |

The component helper is [registered source](../../../../modules/gui/qt/tests/test_page_loader_routes.py),
SHA f3a1aa40cac99ea740708c1e18622d74b1c855cedbf25e685c1fca866a2b2c22.
It executes uninstrumented production PageLoader, StackViewExt and MainViewLoader
bodies, extracts the real wrapper alias/registry declarations into fixtures,
and uses real Qt property maps, an item-selection model and a QAbstractListModel.
History, model data, Navigation, context and style dependencies are adapters.
This does not run actual C++ History and full native media-library wrappers in
the same QML scene, nor establish primary playback, native focus or desktop UI.

Eleven gates cover genuine row7/selection; later row9 retained across grid/list
switch; genuine row8 consumed after count-before-loading-false; default playlist
row0; ID/title before construction; adapter Back/Forward; Home without an index
property/map key; reused parent/group alias; child false on creation and reuse;
unknown leaf properties failing visibly; and conditional no-ML Home routing.
Intentional doesNotExist property errors are retained in expectedFailures and
allMessages. "Zero unexpected messages" does not mean those errors disappeared.

The [native test source](../../../../modules/gui/qt/tests/test_navigation_history.cpp)
uses actual NavigationHistory and a QCore fixture, preserving five original cases
and adding expanded leaf-property normalization and playlist/Home map isolation.
It was built with the configured MinGW x64 static Qt6.8.3 SDK and run on Windows.
MOC/CXX/link logs and the executed build script are preserved under build/.
The build owner reported exit0. Libtool host-path errors followed by Continuing
are retained; successful actual execution establishes the runtime separately.
Source hashes alone do not prove compilation. No native Meson run is claimed.

The original native runner checks names/totals by substring. Its executed source
is retained unchanged as evidence. native/strict-output-audit.json separately
requires the exact nine ordered PASS names, the sole exact9/0/0/0 total, no rejected
status, exit0 and matching source/executable/selected-runtime identities from the
preserved actual results. The proof audit does not rerun or weaken assertions.

root-integration-receipt.json records that all twelve registered files match the
final frozen proposal f8d4ffb18327e3cf94fa29a373afeb928cd7290f5d6f07dd36d0c830782514d1
byte for byte. The seventeenth [CI command](../../../../.github/workflows/qml-ui-checks.yml)
runs the registered helper directly. Original helpers/native test are linked,
not copied; historical source overlays are copied unchanged for reproduction.
The integration owner performed production edits; this proof author only wrote
ignored NEW proposal files.

## Behavior and limits

Explicit nested registry entries forward the complete final property map to
the genuine leaf, including default paths. Parent screens get only pagePrefix.
Three real initialIndex aliases drive existing views. A pending selection is
consumed when the view exists or model loading settles, once; later view switches
retain the user's selection. Explicit child failure propagates through fresh and
reused parents. Unknown leaf properties are never silently filtered. Because Qt
StackView can warn yet return an item, a declared-property post-check returns
false. This is not transactional rollback or complete malformed-value validation.
The existing four exact-route video/playlist writer guards remain in place.

Actual MainInterface uses an ordinary mode-selecting Loader. MainDisplay owns
the top routing PageLoader. Its six intermediary entries are explicitly marked;
no-ML Home and media-library settings stay leaves. Wider preexisting Music/Network
leaf alias and outgoing History callback gaps remain separate beta checks.
The earlier apparent Playlists-to-Home failure and five native warnings are not
attributed to a particular loader or action. No desktop cause or fix acceptance
is asserted here.

## Reproduction

From the checkout root with PySide6.8.3 and fresh output folders:

```powershell
python modules/gui/qt/tests/test_page_loader_routes.py --repo-root . --mode fixed --output-dir build-windows/navigation-FRESH
python modules/gui/qt/tests/test_page_loader_routes.py --repo-root . --overlay doc/juxtopposed-references/native/navigation-leaf-20261014/history/baseline-source --mode baseline --output-dir build-windows/navigation-baseline-FRESH
python modules/gui/qt/tests/test_page_loader_routes.py --repo-root . --overlay doc/juxtopposed-references/native/navigation-leaf-20261014/history/loading-before-source --mode fixed --output-dir build-windows/navigation-loading-before-FRESH
```

The original baseline deliberately exits0 only when the negative reproduction
is established; its passed field is false. The loading-before command should
exit1 at the pending-consumption assertion after the first two gates. The fixed
command requires all eleven checks and should exit0. Historical source overlays
must be used with the exact recorded current helper/source identities.

For actual History, use an already configured matching MinGW Qt build and build
test_qt_navigation_history.exe, including its MOC, then execute it with fresh
results output and the matching staged core DLLs. build/executed-history-build.sh.txt
and build/executed-native-runner.py.txt preserve the owner's actual bounded
sequence with generic path labels; adapt them to your configured build. They are
evidence text, not an automatically executed installation/setup script. Require
the exact nine named PASS results and9/0/0/0 total, not a missing or timed-out log.

## Provenance

evidence-index.json pairs SHA-256 of each original ignored evidence file with its
separately hashed publishable copy. JSON/text use /checkout and /build/vlc path
labels, plus /windows for the standard executable path; source overlays retain
original bytes. Diagnostic assertions, failures,
counts and recorded identities are preserved. Earlier v1–v4 attempts remain
locally preserved; v1/v2 setup issues and the v2 returned-item warning are not
relabeled as passing tests. This public bundle focuses the final matched v5
negatives/fixes and the integrated native run. No executables, DLLs, userdata,
personal media or desktop images are copied. Desktop/model/focus and complete
Music/Network history acceptance remain outstanding.
