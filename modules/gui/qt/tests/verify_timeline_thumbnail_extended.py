# SPDX-License-Identifier: GPL-2.0-or-later
"""Verify the bounded R22 preview journey and its retained negative evidence.

This checks saved results and current source identities; it does not run VLC or
establish physical DPI, responsiveness, blur, ordinary hover or beta acceptance.
"""
from pathlib import Path
from hashlib import sha256
import argparse
import json

repo = Path(__file__).resolve().parents[4]
base = repo / 'doc/juxtopposed-references/native/home-timeline-thumbnails-20261021/extended-r22'
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source-root', type=Path, default=repo)
args = parser.parse_args()
receipt = json.loads((base / 'checkpoint.json').read_text(encoding='utf-8'))

def verify(directory, records):
    directory = directory.resolve()
    for name, expected in records.items():
        path = (directory / name).resolve()
        assert path.is_relative_to(directory), name
        assert sha256(path.read_bytes()).hexdigest() == expected, name

verify(args.source_root, receipt['sourceBindings'])
verify(base, receipt['artifacts'])
assert not receipt['fullFeatureAccepted'] and not receipt['fullBetaAccepted']
case = json.loads((base / 'ui-extended-r22.json').read_text())
assert case['passed'] and case['normalExit'] and case['exitCode'] == 0
assert case['sourceInputsUnchanged'] and case['identitiesUnchanged']
assert not case['forcedCleanupUsed'] and not case['diagnostics']
journey = case['journey']
assert len(journey['checks']) == 262 and all(c['passed'] for c in journey['checks'])
expected_names = {'preview-red', 'preview-green', 'preview-blue', 'fullscreen-preview', 'after-fullscreen'}
expected_names.update(f'{theme}-{edge}' for theme in ['dark-standard', 'dark-compact', 'light-compact-125', 'light-standard-125'] for edge in ['left', 'right'])
assert {p['name'] for p in journey['previews']} == expected_names
assert journey['singleEscapeLeftFullscreen'] is True
assert [p['precise'] for p in journey['heldDrags']] == [True, False]
assert all(0 <= p['presentationWaitMs'] < 3000 for p in journey['previews'])
for revision in [19, 20, 21]:
    negative = json.loads((base / f'before/ui-extended-r{revision}.json').read_text())
    assert not negative['passed'] and negative['normalExit']
ordinary = json.loads((base / 'ordinary-r23.json').read_text())
assert ordinary['normalExit'] and ordinary['sourceInputsUnchanged'] and ordinary['identitiesUnchanged']
assert ordinary['thumbnailJourneyHooksAbsent'] and not ordinary['privateFixture']
assert not ordinary['forcedCleanupUsed'] and not ordinary['diagnostics']
assert ordinary['thumbnailHoverAccepted'] is False
assert not receipt['physicalWindowsDpiAccepted'] and not receipt['backdropBlurAccepted']
print(f"Verified {len(receipt['sourceBindings'])} source and {len(receipt['artifacts'])} artifact bindings: R22 held/edge/theme/interface-scale/fullscreen, ordinary startup/quit only. Broader gates remain open.")
