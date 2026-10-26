# Online provider boundary

Checked 26 October 2026. The original online frame shows OpenSubtitles search,
language selection and selectable results inside Add Subtitles. The initial
attachment implementation preserves VLC's existing VLSub search route; that
route is not acceptance of the original in-dialog search/download journey.

OpenSubtitles' [official getting-started guide](https://opensubtitles.tawk.help/article/getting-started)
requires an API consumer key and an application User-Agent for REST requests.
The [official API overview](https://opensubtitles.tawk.help/article/about-the-api)
describes the REST replacement for the earlier XML-RPC interface. The repository's
VLSub 0.11.1 currently uses the older XML-RPC endpoint. Its live behavior therefore
requires an actual service check; opening its window alone is not sufficient.

No provider account/key was created or read, and no credential is embedded in
source or artifacts. A working current provider contract, consent for account
setup if needed, real result/download checks, cancellation and failure/retry
remain open beta work. Other attachment scope and local-file checks continue
independently of this external requirement.
