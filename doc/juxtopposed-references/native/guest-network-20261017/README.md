# Network implementation — Windows 10 guest, 17 October 2026

The source mounts all nine Network settings and the Advanced Options action from
the [original reference](../../figma/20261017/network/README.md). This is a local
implementation checkpoint toward the full beta. The subsequent [owned native window journeys](../guest-network-window-20261017/README.md)
pass real edits, native failure/retry, restart and Advanced routing. Full page/
media/theme interaction, independent visual/DPI review, Windows11 and final
clean packaging remain open.

The subsequent [HTTP consumer repair](../guest-http-consumers-20261017/README.md)
closes the confirmed HTTP routing/authentication/logging gaps with actual native
transport and persistence checks on Core6823ce/GUI8cf68a. The older tuple below
remains historical evidence; the full beta and subtitle-stall gates remain open.

A later owned ordinary-HTTP transport audit confirms that this older tuple
ignores CLI proxy/direct overrides and drops the path before PAC lookup. The
initial password negative incorrectly required preemptive authentication without
a 407 challenge. A corrected 407 replay authenticates through the legacy backend,
but exposes the synthetic password in its debug log; the whole application does
support that authentication fallback. This corrects the earlier broader password
failure claim. The 45 resolver/static-proxy checks do not cover these consumer
behaviors. Subsequent consumer repairs and native transport checks are in
progress; do not treat this older tuple as acceptance of them. The ordinary
smoke's direct argument alone does not prove system-policy avoidance on this
older Core. The separate subtitle stall remains unexplained.

The controller reads actual saved values without writing on opening. VLC's
current buffering defaults are Custom; selecting Normal writes its native
300/300/1000/300 ms preset. Custom exposes all four bounded millisecond fields.
Checked multi-key writes roll back on real file-lock rejection and permit retry.
Buffer, proxy, trust and cipher changes affect newly opened media/connections;
existing inputs and TLS credentials keep their original policy.

Empty HTTP proxy now uses Windows static, PAC or automatic discovery settings.
`direct://` explicitly bypasses them. Native per-scheme/generic proxy selection,
case-insensitive wildcard/port/IPv6/Unicode bypasses and `<local>` work through
the ordinary Core export. A successful PAC DIRECT result is authoritative;
failed PAC falls back to static policy. Per-URL evaluation avoids a host-level
result cache hiding path-dependent routes. PAC authentication does not send
Windows domain credentials. Explicit proxy/password behavior is preserved.

The proxy field accepts an HTTP/HTTPS endpoint and optional username. Passwords
are separately masked and never exposed through QML values or status. Existing
embedded credentials are redacted; malformed saved endpoints remain opaque
Custom values until replaced. Changing proxy authority clears its saved secret
in the same transaction. Keeping the authority migrates legacy embedded secrets.
Rejected writes retain only the masked transient draft for correction/retry;
closing or leaving password-capable mode clears it.

System trust adds Windows trusted authorities. Turning it OFF continues to
validate against explicitly configured certificate directories; it never means
accepting untrusted certificates. Native cipher choices are queried from the
loaded GNU TLS module; NORMAL is labelled Balanced. Unknown saved values remain
explicit Custom values. Advanced Options closes the shared QML modal before
opening the existing native expert preferences dialog through the C++ signal.

Controls and Network compose one standalone shared dropdown. An intermediate
inline-component extraction made the owned PySide fixture exit with heap error
0xC0000374; the standalone composition passes both Controls and Privacy
regressions. This does not establish a general Qt defect or a Windows crash.
All nineteen Network SVGs preserve the complete original bytes and intrinsic
18/24 px geometry, including the active icon, header, help and chevrons.

| Accepted evidence | Result | Scope |
| --- | --- | --- |
| [Proxy](proxy.json) | 45 assertions | Real Core entry, owned current-user-settings boundary, real WinHTTP PAC and real VLC HTTP payload through an owned proxy; no personal registry/config changes |
| [Baseline negative](proxy-baseline-negative.json) | Expected failure at assertion 6 | The old Core cannot use the owned Windows proxy after five successful setup/explicit-policy assertions |
| [Local-bypass negative](proxy-local-negative.json) | Expected failure at assertion 19 | Intermediate Core incorrectly treated an IPv6 literal as `<local>`; final Core passes all 45 |
| [TLS 1.2/buffering](tls12-and-buffering.json), [TLS 1.3/buffering](tls13-and-buffering.json) | 50 assertions each | Actual file/HTTP buffering delays and exact bytes; real GNU TLS, certificate/hostname rejection, custom CA trust, all four priorities, cached versus fresh credentials, exact PING/PONG |
| [Network controller](network-controller.json) | 17 Qt cases | Native defaults, modes, validation, bounded presets, secrets, authority changes, locked multi-key persistence/rollback/retry, restart and Advanced signal |
| [Network QML](network-qml.json) | Seven groups | Actual host/page through an explicitly supplied controller seam: nine rows, search, dispatch, masked draft/retry, modes, assets, narrow/Light/scaled layouts and Advanced modal release |
| [Controls controller](controls-controller.json), [dialogs](controls-dialogs.json), [input](controls-input.json), [HTTP](controls-http.json) | 19 / 14 / 89 / 29 | Unchanged native consumers on the new Core/GUI runtime |
| [Controls QML](controls-qml.json), [Privacy QML](privacy-qml.json) | Eight groups each | Shared dropdown/feedback helper regressions through owned QML seams |
| [Ordinary](ordinary.json) | Four normal processes | Redesigned/traditional video/audio/sidecar, Library ON and local HLS; decoded frames, D3D11/WASAPI, zero QML/write/visual-callback/PCR errors |
| [Library](library.json), [Subtitles](subtitles.json), [player](player.json) | 18/42/9, 37, 4/25/8 | Matching-runtime positive processes terminate normally. The initial subtitle-role stall below remains unresolved; the complete regression replay is not accepted. |

The final Core is
`49d4c62a6e0be9aa586ca82a7448783de191e2024387352c42688f993784bd7e`;
the ordinary GUI is
`e429f1476bf91d336bc263ee06bcce1351076c384dae54998e066f96980a3643`.
Lua, hotkeys and media-library modules retain their accepted Controls identities.
The [verification receipt](verification.json) records all selected source hashes,
runtime identities and individual evidence hashes. The original package and
earlier accepted runtimes are preserved. The new ordinary runtime contains no
private fixture/probe/test executables.

Builds used the retained configured toolchain, one worker and the E:-backed
CPU-only WSL distro. All 2,318 original frozen inputs matched after the GUI
build. A subsequent test-only protocol observer extension is separately recorded;
all 2,318 selected final inputs match after the recorded runs. Meson syntax parsed, with an
existing duplicate `link_args` warning; a native Meson build is not claimed.
The first extended TLS replay passed every native assertion but its observer
read the protocol after a rejected peer had closed. Capturing it immediately
after handshake fixes that observer; both final protocol replays pass. Failed
intermediate diagnostics remain locally retained and are not acceptance.

The initial final-tuple player replay stalled after five subtitle cases and
was terminated by its 90-second deadline. Its runner then failed to parse the
unfinished XML; [the original failure](initial-controller-stall.txt) and
[failed aggregate](final-tuple.json) remain explicitly failed. Thirteen focused
replays of the same executable/Core each pass all eight cases normally. The
[repeat evidence](player.json) does not explain or resolve the original stall.
Complete player regression and full-beta acceptance remain open; no cause is
attributed to the Network change without evidence.

Maintained opt-in recipes are `test/network-native.mk` (`network-proxy-native`
and `network-settings-native`) and
`modules/gui/qt/tests/native_library/network.mk`
(`network-preferences-native`). Add them after the corresponding configured
Makefile and set `NETWORK_OUT` or `PROPOSAL_OUT` to a fresh owned output path.
Use staged `.libs` Windows executables. The Python runners
`native_network_proxy.py`, `native_network_settings.py` and `run_network.py`
require fresh E: outputs and owned portable runtimes. Generate fresh two-day
loopback certificates with `test/libvlc/generate_network_tls.sh`; never import
them into an OS store or distribute their private keys. Run each protocol using
`--tls-version 1.2` or `--tls-version 1.3`. `--native-core` selects the actual
Core proxy import boundary. The QML runner is
`modules/gui/qt/tests/test_juxtopposed_network_settings.py` under PySide6 6.8.3.

Component captures show [actual native-shaped Custom defaults](network-default-dark.png),
the [authored Normal state](network-reference-normal-dark.png) and
[narrow Light](network-light-narrow.png). These use an owned QML controller seam.
They do not establish native full-page interaction, Windows DPI/monitor behavior
or an independent visual score. Native controller and source/TLS consumer checks
are separately real. Physical discs/capture, enterprise DHCP/DNS WPAD deployment,
all proxy protocols/authentication/failover, all GNU TLS protocol combinations,
sustained performance and the final Windows 11/package matrix remain open.
