# Windows checkout line endings in the navigation fixture

The preceding checkpoint's two GitHub Windows jobs failed during the new
navigation fixture's startup. The downloaded branch artifact records
`ValueError: substring not found` at its exact multiline Loader lookup. Local
source files used LF, while the clean Windows checkout materialized CRLF.
Neither a missing worker report nor a subprocess startup error is accepted as
one of the intended negative cases.

The parser now normalizes CRLF to LF **only in its decoded parsing view**.
Original QML bytes are still copied unchanged and hashed before parsing.
The launcher also prints preserved subprocess stderr on failure so a future
startup error is visible directly in the CI log. No oracle or production QML
behavior is weakened.

The exact original worker reproduces the same startup error against CRLF
repaired sources. The updated worker then runs the unchanged 27-case oracle
against all four source materializations:

| Source | Line endings | Actual child result |
| --- | --- | --- |
| Original production | LF | All 27 intended failures, exact 23 diagnostics, child exit 1 |
| Original production | CRLF | The same 27 failures and diagnostics, child exit 1 |
| Repaired production | LF | All 27 pass, no diagnostics/stderr, child exit 0 |
| Repaired production | CRLF | All 27 pass, no diagnostics/stderr, child exit 0 |

Each bounded launcher accepts only its strict expected result. All nineteen
current component/source suites also pass in a new local run. The source
receipt freezes the updated helper identities, materialized source hashes,
original GitHub artifact/error identity and original failed job IDs. The
sixty-two original/public pairs use generic checkout paths. The original 132
native-checkpoint pairs were rechecked against the actual committed Git blobs.

Only two Python fixture/launcher files changed after the native build. All
compiled Qt production sources retain their recorded identities, so the
preceding native DLL and application results remain applicable in their
original scope. No fresh native build or full beta acceptance is claimed by
this test-only repair. The stored `ciFollowupAccepted: false` describes the
publication-time state; later CI results must be checked at the exact PR head.
