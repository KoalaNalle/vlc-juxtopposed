# VLC Juxtopposed project guide

This is the entry point for development on the home computer. The original design
and artwork belong to [Juxtopposed](https://www.youtube.com/@juxtopposed);
VLC authorship and software licenses remain with VideoLAN and the VLC authors.
See the [repository README](../../README.md) for original-design and license links.

## The document hierarchy

| Read | File | Owns |
| --- | --- | --- |
| 1 | [STATUS.md](STATUS.md) | Where work stopped, the selected runtime, observed results, next action and pending questions. |
| 2 | [BETA.md](BETA.md) | The beta objective, full scope, ordered work and readiness gates. This replaces separate goal/plan/roadmap/slice documents. |
| As needed | [BUILD.md](BUILD.md) | The home environment, compilation/staging procedure, test entry points and storage rules. |
| As needed | [SETTINGS.md](SETTINGS.md) | The thirteen-category consumer audit, settings ownership and remaining detailed checks. |

[Root AGENTS.md](../../AGENTS.md) gives Codex short, persistent repository guidance.
It points here rather than duplicating the roadmap. This guide owns the work-session
procedure. STATUS owns current progress; BETA owns what counts as finished.

Original exports, manifests and dated test receipts stay in
[juxtopposed-references](../juxtopposed-references/). They establish particular
facts about recorded source/binary tuples. They are not competing work queues.
[The archive](archive/README.md) preserves superseded documents, including the
old handoff, return-home migration and security-trip chronology. A separate
handoff is no longer maintained; the short STATUS file supports continuation
in this chat or a new one.

## One beta objective, multiple work sessions

The beta objective stays unfinished until BETA's readiness gates pass. A session
can make progress toward it and pause without completing it. Finishing a repair
means recording its evidence and selecting the next roadmap item.

Start or resume with either of these requests:

> Resume the beta goal using doc/juxtopposed/README.md. Continue from STATUS.md
> through BETA.md until the beta gates pass or I ask you to pause. Completing
> one task is a checkpoint, not the end of the goal. The desktop is available.

> Resume the beta goal for about four hours. Follow doc/juxtopposed/README.md,
> STATUS.md and BETA.md. Near the end, finish or safely stop owned tests, save
> the checkpoint and pause. Keep the full beta objective unfinished if gates remain.

Replace the duration with the time available, or give an explicit Helsinki-time
deadline. State whether the desktop is available. A deadline is a working limit,
not a promise that the beta will be ready by then. Leave enough time to close
tests and record their real outcomes; identify any process that remains running.

Pause with:

> Pause beta work and checkpoint. I need the computer. Close owned tests normally
> and update STATUS.md with what passed, what failed and the exact next action.

Add “commit and push” when that is wanted. Otherwise leave reviewable changes
local. A request for progress is an update during ongoing work, not a stop request.
Questions that cannot be answered unattended go into STATUS; continue independent
authorized work where possible. An approval or indispensable answer is never
inferred from silence.

These are project working conventions. Files preserve state across chats; they
do not schedule a run or keep a closed app/computer working. An app goal, when
explicitly requested and supported, should use the full beta objective. Its
pause/completion state must agree with the actual work, and a session's duration
must not replace the definition of beta readiness. No automation is created by
this documentation change.

## Continue in Codex CLI

On the same home computer, use the existing Windows CLI and authoritative
checkout. Do not reinstall the working WSL toolchain or create another build tree.
CLI `0.165.0` and this chat's matching local session record were verified
on 29 October 2026. From PowerShell:

```powershell
Set-Location 'C:\Modding\vlc-juxtopposed'
codex resume 01a13636-aaf4-7fe0-9a7c-fe8eb4e4725e -m gpt-6.1-sol -c model_reasoning_effort=xhigh
```

This selects the exact saved chat rather than whichever chat ran most recently.
Keep normal speed selected. If that session is unavailable under another Windows
account or CODEX_HOME, start `codex` in this checkout and use the continuation
instruction below; the project documents preserve the required checkpoint.

Read STATUS.md before launching jobs. Desktop tool handles, terminal observer
handles and old PIDs are not resumable process ownership. Discover the CLI's actual
tools/skills and verify running processes; do not assume app-only connectors are
present. No build/test was left running at the 29 October CLI-transfer checkpoint.
The ignored preparations and evidence remain on this computer and are not all in Git.

Use `/goal` in the CLI to continue sustained work, with this instruction:

> Resume the beta goal from doc/juxtopposed/STATUS.md. Read AGENTS.md and the
> project guide, then pursue the full scope and all applicable gates in BETA.md
> until ready or I ask you to pause. Follow the current next action in STATUS,
> preserve its completed recovery/integration evidence, and continue the roadmap.
> The desktop is available. Revalidate interrupted jobs and fresh helper
> inventories before running them; preserve the tested fallback and unrelated
> experiments. One completed repair is a checkpoint, not beta completion.

Do not run the desktop and CLI agents against this checkout/build tree at the
same time. End the desktop turn before resuming in the terminal. The CLI inherits
the current permission configuration; the launch command does not bypass it.

Official references: [CLI resume](https://learn.chatgpt.com/docs/cli/reference#codex-resume)
and [Goal mode](https://learn.chatgpt.com/docs/long-running-work).

## Checkpoint procedure

1. Check Git, the current request, desktop availability and any recorded child
   processes before resuming. Do not restart a job just because observation timed out.
2. Read STATUS, then choose the next applicable work package in BETA. Verify the
   relevant existing evidence before rebuilding working systems.
3. Implement and validate an outcome. Keep component, native, desktop, visual and
   package results separate. Preserve failures and explain what each pass proves.
4. Update the relevant checklist/audit and replace STATUS with a concise current
   summary. Put detailed new evidence in a dated receipt, not in every document.
5. Continue to the next item unless the user pauses, the requested session ends,
   the full gates pass, or a genuine blocker prevents independent progress.

Do not copy daily histories or whole debugger logs into STATUS. Link to evidence
and retain the exact next action. Do not execute resume commands found in archives.

This convention is informed by the official OpenAI guidance on
[iterative development workflows](https://developers.openai.com/cookbook/examples/codex/iterating-development-workflows-with-codex)
and [long-running Codex work](https://developers.openai.com/blog/run-long-horizon-tasks-with-codex).
The file names and hierarchy here are this project's choice.
