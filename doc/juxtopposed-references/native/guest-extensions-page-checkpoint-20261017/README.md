# Extensions page checkpoint — 17 October 2026

Work in progress, saved for the requested pause. This is not beta or Extensions acceptance. Production and fixture edits remain uncommitted in the E: checkout; the accepted add-on catalog/storage baseline is unchanged at source d926066e8993ec6ce9ebebbbb544d9d05f95270c, with its documentation at HEAD 32cd62fc4d71b0b59d6a2e79c7ea72f3a6234112.

The actual AddonsModel, MainCtx-owned Extensions preferences controller, shared legacy dialog model, original cached design page and SVG resources have been implemented. The corrected ordinary GUI/Core build and separate private QtTest GUI build both exited zero. All 1,910 ordinary source inputs and four private fixture inputs matched their frozen hashes after the run. These are incremental Windows builds, not a clean package or native Meson build.

The first owned-window run failed at `globalCategoryextensions` before mounting or exercising the page. Four setup assertions passed; the fifth target check failed. This does not distinguish a missing, hidden, disabled or not-yet-ready target. The process did not exit after the fixture requested normal quit; its runner killed that owned process after 100 seconds. No selected QML diagnostic terms were recorded. No subsequent edit/restart modes ran. This is failed evidence, not an accepted UI journey.

The run also observed native startup preference On while the extension engine was not loaded before explicit native menu use. Startup behavior still needs investigation. Installation, removal, persistence, original page fidelity and default-catalog behavior have not been accepted for this frontend.

The runner's `ordinaryGuiUnchanged=false` field remained its initial default because failure skipped that final check. An independent post-run hash check confirmed all four ordinary runtime DLL hashes unchanged; `ordinaryRuntimeHashRecheckedUnchanged` records this separately without editing the failed report.

See [checkpoint.json](checkpoint.json) for exact identities, failure and next actions. Raw build/run artifacts remain under `build-windows/beta-extensions-20261017`; preserve them and use fresh output paths for future runs. Do not overwrite hard-linked binaries. One build or native job at a time, E: only. The five unrelated pre-existing untracked files remain untouched.

Recording retains the user's chosen native VLC preference. Turning it Off does not introduce forced re-encoding. Beta remains the confirmed redesign plus reliability; vanilla parity is V1.0, and new YouTube/Twitch integrations are tentative V1.5 work.
