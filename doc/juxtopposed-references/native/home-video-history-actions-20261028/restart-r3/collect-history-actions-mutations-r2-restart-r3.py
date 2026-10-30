"""Publish terminal, owner-approved UI evidence only; never launch an app/test."""
from pathlib import Path
from hashlib import sha256
import json
import shutil

repo = Path(__file__).resolve().parents[2]
work = Path(__file__).resolve().parent
receipt = repo / 'doc/juxtopposed-references/native/home-video-history-actions-20261028'
runs = [('history-actions-ui-r2', 'ordinary-r2', 'run-history-actions-ui-r2.py'),
        ('history-actions-restart-r3', 'restart-r3', 'run-history-actions-restart-r3.py')]

def digest(path):
    return sha256(Path(path).read_bytes()).hexdigest()

records = {}
for name, destination, producer_name in runs:
    record = json.loads((work / name / 'verification.json').read_text())
    assert record['status'] == 'terminal' and record['exitCode'] == 0
    assert record['moduleCounts'] == ['603'] and not record['diagnostics']
    guards = {key:value for key,value in record.items() if key.endswith('Unchanged')}
    assert len(guards) == 7 and all(guards.values())
    assert digest(work / producer_name) == record['producerSha256']
    assert (work / name / 'observations').is_dir()
    assert not (receipt / destination).exists()
    records[name] = record
for binary in ['libvlccore.dll','plugins/gui/libqt_plugin.dll','plugins/misc/libmedialibrary_plugin.dll']:
    assert records[runs[0][0]]['runtimeSha256'][binary] == records[runs[1][0]]['runtimeSha256'][binary]

notes_r2 = '''# Ordinary history mutations: 28 October 2026

The owner explicitly approved history deletion through the test interface. This
ordinary VLC run uses a copied profile containing generated media. It exits 0,
loads 603 modules, has no checked diagnostics, and passes all seven preservation
guards. Only history entries are removed; the library and media remain.

## Observed actions

- Continue Watching's individual **Remove from this list** action is selected
  from its actual item menu. That entry disappears; Watch Again's entry remains.
- On Watch Again's full list, an unmatched search hides its entry. The header
  menu still offers **Clear all items**, and the confirmation identifies the
  selected section and includes entries hidden by search. The owner-approved
  confirmation is accepted; removing the search still shows an empty history.
- Home shows both history sections empty while its library header retains
  **3 items**. Both remain empty after navigating away/back and reloading the
  Home models. The owned application closes normally after these operations.

The 12 numbered screenshot/accessibility observations preserve the actual menus,
confirmation and outcomes. The isolated ordinary app's real QObject worker
success path is exercised here; this is not a native-helper-only demonstration.
An ordinary worker failure, retry after real storage failure, or closing while a
large edit is still busy remains outside this run. Opposite-section preservation
is observed for the individual removal; native evidence separately covers both
section directions and large lists. Keyboard modifier delivery, Windows 10/DPI,
design scoring and final-package readiness are not established.

The [separate cold restart](../restart-r3/NOTES.md) opens the saved post-mutation
profile in a fresh process and confirms persistence. Original menu-only evidence
under ordinary-r1 remains a historical receipt; its pending approval was later
answered and does not describe this approved mutation run.

All captures are of the owned test window/menus and generated media. Original
capture filenames and bytes are retained, including JPEG data saved with .png
names by the capture helper. The verification omits personal configuration hashes
and copied-profile inventories while retaining their preservation booleans.
Raw VLC logs, media, binaries and settings databases are not copied. The frozen
producer uses one-use local paths; adapt into a fresh directory before any rerun.
'''
notes_r3 = '''# Ordinary cold persistence: 28 October 2026

A fresh ordinary VLC process opens a copy of the saved, owner-approved
post-mutation r2 profile. No additional history mutation is requested here. It
exits 0, loads 603 modules, has no checked diagnostics, and passes all seven
source/runtime/baseline/prior-profile/fixture/producer/human-configuration guards.

The root observer recorded three states before closing the owned window normally
through its visible title-bar Close button:

1. On cold startup, Continue Watching and Watch Again are both empty; the library
   header still reports **3 items**.
2. Maximizing the window retains the empty history sections and library count.
3. The Videos route lists all **three named generated media entries** after the
   restart, showing that clearing history retained the indexed library.

The three numbered screenshot/accessibility observations preserve those states.
This qualifies persistence across a new process using the saved generated-data
profile; it is not a fresh database, a merely reloaded QML model, or a claim about
another person's media/configuration. The mutation itself is documented in
[ordinary-r2](../ordinary-r2/NOTES.md). All captures concern the owned application.

Real storage-error recovery, worker close-under-load, modifier-chord delivery,
Windows 10/DPI, design fidelity and final-package acceptance remain separate.
Original capture bytes/names are preserved, including JPEG data under .png names.
Personal configuration hashes and copied-profile inventories are omitted from
the public verification; all preservation booleans and binary/source inventories
remain. No raw VLC logs, media, binaries or settings database are included. The
collector copies existing evidence only and does not open/control an application.
'''

for name, destination, producer_name in runs:
    source = work / name
    target = receipt / destination
    target.mkdir()
    copies = []
    def copy(path, relative):
        output = target / relative
        output.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(path, output)
        assert Path(path).read_bytes() == output.read_bytes()
        copies.append(dict(source=Path(path).relative_to(repo).as_posix(), destination=relative,
            sha256=digest(output), size=output.stat().st_size, byteIdentical=True))
    images = []
    observations = []
    for path in sorted((source / 'observations').iterdir()):
        assert path.suffix in ('.json','.png')
        copy(path, 'observations/' + path.name)
        if path.suffix == '.png':
            data=path.read_bytes()
            actual_format = 'JPEG' if data.startswith(b'\xff\xd8\xff') else 'PNG' if data.startswith(b'\x89PNG\r\n\x1a\n') else None
            assert actual_format
            images.append(dict(file=path.name, actualFormat=actual_format, originalNameAndBytesPreserved=True))
        else:
            observation = json.loads(path.read_text())
            observations.append(dict(file=path.name, observedAt=observation['observedAt'],
                screenshotMetadata=observation['screenshots']))
    assert len(observations) == (12 if destination=='ordinary-r2' else 3)
    copy(work / producer_name, producer_name)
    copy(__file__, 'collect-history-actions-mutations-r2-restart-r3.py')
    record=records[name]
    omitted=[key for key in ('humanBeforeSha256','priorProfileSha256') if key in record]
    public={key:value for key,value in record.items() if key not in omitted}
    public['sanitization']=dict(originalSha256=digest(source/'verification.json'),omittedFields=omitted)
    (target/'verification.json').write_text(json.dumps(public,indent=2)+'\n',encoding='utf-8',newline='\n')
    (target/'NOTES.md').write_text(notes_r2 if destination=='ordinary-r2' else notes_r3,encoding='utf-8',newline='\n')
    (target/'.gitattributes').write_text('* -text\n',encoding='utf-8',newline='\n')
    summary=dict(sourceRun=name,status=record['status'],exitCode=record['exitCode'],
        moduleCounts=record['moduleCounts'],diagnostics=record['diagnostics'],
        guards={key:value for key,value in record.items() if key.endswith('Unchanged')},
        scope=record['scope'],ownerApprovedMutation=(destination=='ordinary-r2'),
        libraryItemsRetained=3,continueWatchingEmpty=True,watchAgainEmpty=True,
        screenshots=images,observations=observations,
        originalVerificationSha256=digest(source/'verification.json'),
        personalHashesAndProfileInventoriesOmitted=True,rawLogsMediaBinariesSettingsCopied=False)
    (target/'summary.json').write_text(json.dumps(summary,indent=2)+'\n',encoding='utf-8',newline='\n')
    (target/'copy-verification.json').write_text(json.dumps(dict(byteIdenticalCopies=copies,
        sanitizedVerification=public['sanitization']),indent=2)+'\n',encoding='utf-8',newline='\n')
    inventory=[dict(path=path.relative_to(target).as_posix(),size=path.stat().st_size,sha256=digest(path))
        for path in sorted(target.rglob('*')) if path.is_file() and path.name!='artifact-hashes.json']
    (target/'artifact-hashes.json').write_text(json.dumps(dict(algorithm='SHA-256',
        scope='All receipt files except this inventory itself',files=inventory),indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(destination=str(target),byteIdenticalCopies=len(copies),
        screenshotCount=len(images),observationCount=len(observations),inventoryFiles=len(inventory),
        omittedFields=omitted,terminalExitCode=record['exitCode'],allSevenGuards=True)))
