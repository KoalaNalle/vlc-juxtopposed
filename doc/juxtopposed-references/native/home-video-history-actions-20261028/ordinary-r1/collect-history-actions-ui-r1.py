from pathlib import Path
from hashlib import file_digest
import json
import shutil

root = Path(__file__).resolve().parent
repo = root.parents[1]
source = root / 'history-actions-ui-r1'
target = repo / 'doc/juxtopposed-references/native/home-video-history-actions-20261028/ordinary-r1'
def digest(path):
    with Path(path).open('rb') as stream:
        return file_digest(stream, 'sha256').hexdigest()
record = json.loads((source / 'verification.json').read_text())
assert record['status'] == 'terminal' and record['exitCode'] == 0
assert all(v for k, v in record.items() if k.endswith('Unchanged'))
assert record['moduleCounts'] == ['603'] and not record['diagnostics']
target.mkdir(exist_ok=False)
copies = {}
for path in sorted((source / 'observations').iterdir()):
    assert path.suffix in ('.png', '.json')
    destination = target / 'observations' / path.name
    destination.parent.mkdir(exist_ok=True)
    shutil.copyfile(path, destination)
    assert digest(destination) == digest(path)
    copies[destination.relative_to(target).as_posix()] = dict(source=str(path), sha256=digest(path))
for path in [root / 'run-history-actions-ui-r1.py', Path(__file__)]:
    shutil.copyfile(path, target / path.name)
    copies[path.name] = dict(source=str(path), sha256=digest(path))
# Retain preservation results without publishing personal configuration hashes
# or copied profile inventories. No settings database or raw VLC log is copied.
public_record = {k: v for k, v in record.items() if k not in ('humanBeforeSha256', 'priorProfileSha256')}
public_record['sanitization'] = dict(originalSha256=digest(source / 'verification.json'),
    omittedFields=['humanBeforeSha256', 'priorProfileSha256'])
(target / 'verification.json').write_text(json.dumps(public_record, indent=2) + '\n', encoding='utf-8', newline='\n')
(target / 'copy-verification.json').write_text(json.dumps(copies, indent=2) + '\n', encoding='utf-8', newline='\n')
(target / '.gitattributes').write_text('* -text\n', encoding='utf-8', newline='\n')
(target / 'NOTES.md').write_text('''# Ordinary history-action menus: 28 October 2026

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
''', encoding='utf-8', newline='\n')
hashes = {p.relative_to(target).as_posix(): digest(p) for p in sorted(target.rglob('*')) if p.is_file()}
(target / 'artifact-hashes.json').write_text(json.dumps(hashes, indent=2) + '\n', encoding='utf-8', newline='\n')
assert all(digest(target / name) == value for name, value in hashes.items())
print(json.dumps(dict(files=len(hashes), copiedOriginals=len(copies), exitCode=record['exitCode'], preservationGuards=True)))
