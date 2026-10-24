# Owned native Windows HTTP checks

These opt-in tests run one child at a time against an already matched ordinary
VLC package. They create fresh portable runtimes, configs, userdata, caches and
loopback services under their output directories. Immutable files use hard links
on the same volume. The ordinary package is preserved. Keep all arguments on E:
on the vacation computer; dot-source its migration `Common.ps1` first.

`native_http_policy.py --package <package> --output-dir <fresh-output>` runs five
actual `vlc.exe` cases: CLI manual/direct precedence, saved Basic credentials,
an unrelated CLI proxy authority, and explicit embedded CLI credentials. It
requires exact payloads, the modern backend, normal termination, credentials
absent from debug logs, and no proxy credentials reaching the origin.

Build the private PAC driver with the configured native test Makefile plus
`test/network-native.mk`, setting `NETWORK_OUT` to a fresh directory and selecting
`network-pac-transport-native`. Run
`native_http_pac.py --package <package> --executable <staged-.libs-driver.exe> --output-dir <fresh-output>`.
The Meson opt-in Windows target is `test_network_proxy_transport`. The executable
supplies settings through its own Core import slot, maps only its synthetic
`media.owned.invalid` hostname to loopback, and observes the real WinHTTP PAC call.
It does not edit the hosts file or read/change the user's proxy settings. Real
media inputs must route two queries on the same origin differently, with exact
payloads and all nine checks passing. `--expected-failure` accepts only the
specific old-consumer failure after five successful setup/open checks, with the
observed old host-only PAC URL and normal exit 1.

Generate two-day, private certificates with
`bash test/libvlc/generate_http_proxy_tls.sh <absolute-fresh-directory>` in the
Linux build environment. Then run
`native_http_proxy.py --package <package> --certificates <Windows-certificate-directory> --output-dir <fresh-output>`.
Its twelve cases cover HTTP forwarding, authenticated CONNECT, TLS proxies and
nested TLS at TLS 1.2/1.3, plus untrusted-proxy and hostname rejection. It disables
system trust and supplies the generated CA explicitly; nothing enters an OS
trust store. Only synthetic Basic credentials are used. The legacy-reader cases
preserve all plugins/settings registrations and deliberately serve a finite 206
without Content-Range: the modern reader rejects it, allowing the legacy reader's
transport to be exercised. This malformed-response adaptation is recorded in
the report; it is not a claim that such a response is standards compliant.

HTTP message/file, HTTP/1, HTTP/2, chunked and tunnel unit executables remain the
upstream native targets under `modules`. Their Windows transport adapter supplies
AF_UNIX stream pairs and drains unread synthetic requests before graceful
shutdown; the original payload, EOF and error assertions remain. The tunnel
fixture uses Winsock send/close and bounded polling for thread cancellation.
Use a short writable TEMP path for the Windows AF_UNIX endpoint names.

Each runner retains a JSON receipt, child output and logs. A timeout/forced
cleanup fails acceptance. Keep generated keys, configs, media and raw logs out
of distribution. These focused checks do not establish all proxy authentication
schemes, PAC failover, other protocols, full UI/DPI behavior or beta acceptance.
