# SPDX-License-Identifier: GPL-2.0-or-later
"""Verify the bounded preview checkpoint's saved evidence and current sources.

This does not execute playback or establish full feature/design/beta acceptance.
"""
from pathlib import Path
from hashlib import sha256
import argparse
import json

repo = Path(__file__).resolve().parents[4]
base = repo / 'doc/juxtopposed-references/native/home-timeline-thumbnails-20261021'
receipt = json.loads((base / 'checkpoint.json').read_text(encoding='utf-8'))
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source-root', type=Path, default=repo,
                    help='Explicit historical source snapshot, when current work has advanced.')
args = parser.parse_args()

def verify(directory, records):
    for name, expected in records.items():
        path = (directory / name).resolve()
        assert path.is_relative_to(directory.resolve()), name
        assert sha256(path.read_bytes()).hexdigest() == expected, name

verify(args.source_root, receipt['sourceBindings'])
verify(base, receipt['artifacts'])
assert receipt['fullBetaAccepted'] is False and receipt['fullFeatureAccepted'] is False
native = json.loads((base / 'native-r17.json').read_text())
assert native['passed'] and native['sourceInputsUnchanged'] and not native['forcedCleanupUsed']
assert [p['counts']['tests'] for p in native['processes']] == [11, 26, 14, 14]
assert all(p['exitCode'] == 0 and p['passed'] and all(p['counts'][k] == 0 for k in ['failures', 'errors', 'skipped']) for p in native['processes'])
for mode, count in [('edit', 135), ('cold-on', 67), ('cold-off', 80)]:
    case = json.loads((base / ('ui-' + mode + '-r15.json')).read_text())
    assert case['passed'] and case['normalExit'] and case['sourceInputsUnchanged'] and case['identitiesUnchanged']
    assert not case['forcedCleanupUsed'] and not case['diagnostics']
    assert len(case['journey']['checks']) == count and all(c['passed'] for c in case['journey']['checks'])
    assert case['expectedDiagnostics'] == (['main error: cannot replace configuration file (Windows error 5)'] if mode == 'edit' else [])
ordinary = json.loads((base / 'ordinary-r16.json').read_text())
assert ordinary['normalExit'] and ordinary['identitiesUnchanged'] and ordinary['sourceInputsUnchanged']
assert ordinary['thumbnailJourneyHooksAbsent'] and not ordinary['privateFixture'] and not ordinary['forcedCleanupUsed']
optional = json.loads((base / 'missing-fixture-r17.json').read_text())
assert optional['expectedSkipObserved'] and optional['functionalQualification'] is False
print(f"Verified {len(receipt['sourceBindings'])} source and {len(receipt['artifacts'])} saved artifact bindings; native65, selected UI282 and ordinary observation remain bounded. Full feature/beta gates stay open.")
