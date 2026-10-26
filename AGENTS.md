# Working in this repository

Start with [the project guide](doc/juxtopposed/README.md) and
[current status](doc/juxtopposed/STATUS.md). For beta implementation, use
[BETA.md](doc/juxtopposed/BETA.md) as the single scope and completion checklist.
Read the build guide or settings audit when relevant.

## Continue and pause

- The user's current request determines the work. Documentation maintenance,
  a status question or a review does not by itself resume paused desktop tests.
- When the user resumes the beta goal, work through the roadmap continuously.
  Completing one repair or work package means selecting the next applicable item;
  it does not complete the beta or end an otherwise unbounded work session.
- A user-requested time limit ends a work session, not the beta objective. Check
  elapsed time, reserve time to save a useful checkpoint, and describe unfinished
  work honestly. Time limits are approximate; never silently leave a child job
  using the desktop after a requested stop.
- On pause, stop launching jobs, close owned tests normally, record any jobs that
  could not finish, replace the short current-status summary and stop. Resume
  from that checkpoint when requested. Do not mark the beta complete at a pause.
- Mark beta readiness only against every applicable gate in BETA.md. Component,
  native, ordinary desktop, visual and final-package evidence have distinct scopes.
- Do not create an app goal or a scheduled automation merely by reading these
  files. Follow the user's explicit goal/scheduling request and available tools.

## Maintain the documentation

- BETA.md owns scope, priorities and completion criteria. STATUS.md owns the
  current checkpoint, selected runtime, next action and open questions. Do not
  prepend the same progress report to several documents.
- Keep STATUS.md short; replace superseded status instead of appending histories.
- `doc/juxtopposed/archive/` and dated reference receipts are historical evidence,
  not current instructions. Guest-machine paths and old resume commands there
  must not override current home-machine guidance. Do not rewrite frozen receipts.
- Current user instructions take precedence over repository guidance. Preserve
  scope decisions and evidence; do not quietly remove an unfinished requirement.

## Preserve the working environment

- Use the authoritative checkout at `C:\Modding\vlc-juxtopposed`. Reuse the
  established home WSL environment, working architecture and authored layouts.
- Keep generated outputs ignored in `build-windows/`. Use fresh run directories,
  freeze helpers before launching, and allow one writer per native build tree.
  Review historical helpers before use and refresh obsolete input inventories.
- Some staged DLLs are hardlinked: unlink the destination before copying a
  replacement. Preserve recoverable tested runtimes and exact binary identities.
- `D:\Media` is authorized for read-only testing. Keep private media and raw
  personal logs out of Git and packages. Cold build archives belong on D:;
  active development remains on C:. Keep C: below the owner's 80% usage target.
- Preserve unrelated work. Do not read, hash, change, stage or delete the four
  unrelated experiments `test/libvlc/native_soxr_reference.py` and
  `test/modules/audio_filter/soxr_{lifecycle_trace,native_reference,pcm_trace}.c`.
- Use the applicable skills for Computer Use and Figma. Respect current desktop
  availability; never change security/privacy choices to remove test friction.
  Ask before installing dependencies beyond the user's authorized setup scope.
- Commit/push, release publication, designer contact and shutdown need current
  user authorization. A previous session's completed push is not standing approval.
