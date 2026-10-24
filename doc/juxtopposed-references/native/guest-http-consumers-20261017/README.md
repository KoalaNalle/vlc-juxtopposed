# HTTP proxy consumers — Windows 10 guest, 17 October 2026

Source commit `a5b1cf5e8667100ee44da5636a430b0649d57a6c`
repairs actual HTTP consumers: CLI proxy/direct precedence, credentials scoped to
their proxy authority, complete PAC request URLs, Basic proxy authentication and
TLS proxy transport. The commit is local and has not been pushed.
The original public resolver API remains available.
The checked preferences writer clears the old secret before replacing its
authority, including rollback. HTTP requests omit proxy credentials at the origin,
and outgoing debug logs no longer dump credential-bearing requests.

The new CONNECT exercise also found a real application crash: its HTTP logger
received a TLS credentials object. CDB traced the execute access violation through
`vlc_http_dbg` and `vlc_vaLogCallback`. Passing the actual logger fixes the matching
TLS 1.2/1.3 CONNECT and nested-TLS cases. This is separate from the earlier Windows
blue screen and the still-unexplained subtitle-role stall.

The [verification receipt](verification.json) records these focused passes:

| Check | Result |
| --- | --- |
| Ordinary `vlc.exe` CLI policy/authentication | 5 cases, exact payloads, modern backend |
| HTTP/TLS proxies, CONNECT, nested TLS, certificate rejection | 12 cases |
| Actual media/PAC path and query routing | 9 checks; old consumer fails the targeted assertion after setup/open pass |
| Object/CLI/media credential policy | 16 checks against the actual Core |
| Upstream HTTP message/file/HTTP1/HTTP2/chunked/tunnel executables | 6 normal passes |
| Windows resolver/static proxy | 45 checks |
| Actual buffering and TLS policy | 50 checks at TLS 1.2; 50 at TLS 1.3 |
| Native Network / Controls controllers | 17 / 19 cases |
| Actual settings windows | 31 edit/failure/retry checks; 11 restart checks; two normal processes |
| Ordinary playback | 4 generated-media/local-HLS cases plus one bounded Downloads sample |

All accepted positive processes exit normally without forced cleanup. The old
PAC consumer exits 1 at its specific routing assertion; it is deliberate negative
evidence. These counts include fixture/framework checks and are not counts of
independent product features. Twelve TLS cases retain both real backends; three
use a deliberately incomplete 206 response to select the legacy reader. This
adaptation is recorded, rather than presented as a standards-compliant response.
Untrusted CA and wrong-hostname negatives require actual certificate-verification
diagnostics, no HTTP request and no payload. Synthetic credentials appear in none
of the captured streams for the seventeen CLI/TLS cases.

The [corrected baseline](baseline-policy-407.json) matters: the original password
negative required preemptive authentication without a 407. With a proper challenge,
the old application authenticates through its legacy fallback, but logs the
synthetic password. It did not lack password authentication altogether. The old
CLI proxy/direct and missing-PAC-path failures remain real. Original failed runs,
the real CONNECT crash, Windows fixture-porting failures and the extra playback
test's invalid `--hw-dec=any` invocation remain under ignored build output.
The corrected boolean `--hw-dec` run decodes a private Downloads sample with
D3D11VA/GTX 1080 and quits normally in 3.72 seconds. Its media, filename and frame
remain private; only the bounded result is recorded here.

On this guest, a separate [real WinHTTP diagnostic](pac-loopback-diagnostic.json)
returns DIRECT for loopback-IP targets even with an unconditional proxy script.
The maintained PAC test therefore maps one synthetic hostname to loopback inside
its own process. It observes the real WinHTTP URL/result and actual HTTP payloads.
It changes neither Windows proxy settings nor the hosts file, and sends no saved
manual credentials to PAC/system endpoints. The two queries on the same origin
select different routes. [Microsoft documents per-URL proxy evaluation](https://learn.microsoft.com/en-us/windows/win32/api/winhttp/nf-winhttp-winhttpgetproxyforurl).

The ordinary runtime is
`build-windows/beta-network-20261017/ordinary-consumer-acceptance-r1/runtime`:
Core **6823ce2c…3e62**, ordinary GUI **8cf68a54…5226**, modern HTTP
**d456b075…22f**, legacy HTTP **77e7a0d3…9e7**. Full identities are in the receipt.
The separate private GUI is **20840654…74b0**; it adds the maintained native test
TU and QtTest to the ordinary production objects. No private fixture/probe/test
executable ships in the ordinary runtime. The [edit](network-edit-dark.png) and
[restart](network-reopen-dark.png) captures cover Dark at observed 100% scaling,
with no media playing. They do not establish visual scores or a DPI matrix.

All 2,167 selected raw inputs remained unchanged through the final replay; the
2,160 tracked selections in [source-inputs.json](source-inputs.json) match the
source commit's Git blobs exactly. The seven other selections are the five
preserved unrelated untracked files and two unused prototype inputs. Production
sources still match their Core/HTTP/ordinary-GUI build freeze. The later changes
to Python test checks/distribution documentation do not change compiled TUs.
Meson syntax parses; native Meson compilation and a clean release build are not
claimed. Public JSON is normalized to LF; raw source hashes are retained beside
the normalized evidence hashes.

Reproduce with the maintained [native HTTP guide](../../../../test/libvlc/native_http.md),
Network/Controls native Qt recipes and the
[owned settings-window runner](../../../../modules/gui/qt/tests/native_ui/README.md).
Use a matching ordinary package and fresh E: output directories; compile/run one
workload at a time. Generated certificate keys/configs/raw logs are private test
data. Original design/artwork remains Juxtopposed's, CC BY 4.0.

The full beta remains incomplete. Other proxy authentication schemes, PAC failover,
WebDAV/HTTP PUT end-to-end behavior and reuse across different request paths are
not accepted by this checkpoint. Native theme/DPI/monitor/media interaction,
independent visual review, Windows 11, the subtitle stall, remaining settings/
surfaces and a final clean licensed package remain open.
