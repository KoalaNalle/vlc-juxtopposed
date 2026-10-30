# Ordinary history-action menus: 28 October 2026

The normal Qt application used a copy of the generated-media Home profile.
VLC exited normally with code 0; 603 modules loaded, no checked runtime/QML
diagnostics, and all seven source/runtime/baseline/profile/fixture/producer/human
configuration preservation guards passed. No native writer or test window
remained open after this run. No history entry was deleted through the UI.

## Observed scope

- Home shows Continue Watching first and Watch Again second, each with its
  named options button and Clear all items menu. Each confirmation names the
  selected section, explains hidden search results and keeps media files.
- Escape cancels both section confirmations and retains both generated entries.
  Continue's right-click item menu offers Pin to sidebar and Remove from this
  list. Watch Again's full-list item menu offers Remove from this list.
- Both full-list routes contain their expected generated video. Their header
  More menu offers Clear all items. On Continue's full list, a typed unmatched
  search displays zero matches but still offers Clear all and the whole-list
  confirmation. Cancelling and clearing search restores the video.
- The native keyboard Menu key opens Continue's full-list item action, and
  reopens it after Escape. Synthesized Shift+F10 on Home and Alt+F4 produced no
  visible action; this run did not isolate automation delivery from app handling.
  Those modifier combinations remain unqualified. Normal pointer Close works.

The images/accessibility trees cover only the owned VLC test window and its
menus/dialogs; all media shown is generated. Some accessibility snapshots lagged
route changes and were refreshed. A nested dialog's No element was not available
to the helper's cached parent state; Escape cancelled it instead. A header
element click focused Back rather than Search, so the visible search field was
focused from a fresh screenshot before typing. These are recorded automation
limitations, not proof of app defects. UIA's focus text remained at the title
even when the visible caret and typed query proved search focus.

## Remaining qualification

Actual ordinary Clear/Remove mutation and the QObject worker's completion/error/
close-under-load behavior remain separate. An action-time approval was requested
for clearing only the single generated Continue entry. No answer had arrived
before this bounded UI run ended, so the confirmation was cancelled and VLC was
closed normally. Reopen the same isolated profile only after the pending answer;
never infer approval from elapsed time. The Computer Use confirmation policy
requires action-time approval for deletion through an app interface.

The separate native receipt establishes 16 passing checks, including scoped
bulk/per-item edits and real paused-input Stop/Close preservation. The component
receipt establishes production-page routing with explicit native adapters.
Neither replaces the unperformed ordinary mutation. This run does not establish
design scoring, Windows DPI/Windows 10 or final-package readiness.

The sanitized verification retains all preservation booleans and source/binary
identities, omitting personal configuration hashes and copied-profile inventory.
Raw VLC logs and settings databases are kept out of this receipt. Frozen producers
use original one-use paths; adapt into a fresh ignored directory before another
run. The collector only copies existing evidence; it does not launch an app.
