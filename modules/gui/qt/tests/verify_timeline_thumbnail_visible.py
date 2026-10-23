# SPDX-License-Identifier: GPL-2.0-or-later
"""Check bounded saved preview UI evidence and current source bindings.

Does not run VLC or accept all media/layouts, blur, responsiveness, physical DPI,
final packaging or the complete beta roadmap.
"""
from pathlib import Path
from hashlib import sha256
import argparse,json
repo=Path(__file__).resolve().parents[4]
base=repo/'doc/juxtopposed-references/native/home-timeline-thumbnails-20261021/visible-r38'
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source-root',type=Path,default=repo)
args=parser.parse_args()
receipt=json.loads((base/'checkpoint.json').read_text())
def verify(directory,records):
    directory=directory.resolve()
    for name,expected in records.items():
        path=(directory/name).resolve()
        assert path.is_relative_to(directory),name
        assert sha256(path.read_bytes()).hexdigest()==expected,name
verify(args.source_root,receipt['sourceBindings']);verify(base,receipt['artifacts'])
cases={mode:json.loads((base/name).read_text()) for mode,name in receipt['cases'].items()}
for mode,case in cases.items():
    assert case['passed'] and case['normalExit'] and case['exitCode']==0
    assert case['sourceInputsUnchanged'] and case['identitiesUnchanged'] and not case['forcedCleanupUsed']
    assert not case['diagnostics'] and all(c['passed'] for c in case['journey']['checks'])
    assert case['identities']==receipt['privateIdentities']
    assert len(case['journey']['checks'])==receipt['checks'][mode]
    assert case['quitLogOrder']['wait']<case['quitLogOrder']['stopped']<case['quitLogOrder']['teardown']
    assert case['expectedDiagnostics']==(['main error: cannot replace configuration file (Windows error 5)'] if mode=='edit' else [])
    for preview in case['journey']['previews']:
        placement=preview['placement']
        assert abs(placement['actualX']-placement['expectedX'])<2
        assert abs(placement['actualY']-placement['expectedY'])<2
assert sum(len(c['journey']['previews']) for c in cases.values())==29
assert cases['extended']['journey']['singleEscapeLeftFullscreen']
assert len(cases['extended']['journey']['heldDrags'])==2
assert len(cases['geometry']['journey']['previews'])==6
assert all(p['geometryReference'] for p in cases['geometry']['journey']['previews'])
negative=json.loads((base/'before/cold-edge-r37.json').read_text())
assert not negative['passed'] and negative['normalExit'] and not negative['diagnostics']
placement=negative['journey']['previewPlacement']
assert placement['actualX']==0 and placement['expectedX']==260
ordinary=json.loads((base/'ordinary-r39.json').read_text())
assert ordinary['normalExit'] and ordinary['sourceInputsUnchanged'] and ordinary['identitiesUnchanged']
assert ordinary['thumbnailJourneyHooksAbsent'] and not ordinary['privateFixture'] and not ordinary['forcedCleanupUsed']
assert ordinary['pointerPreviewObserved'] and ordinary['rightEdgePlacementObserved'] and not ordinary['diagnostics']
assert ordinary['identities']==receipt['ordinaryIdentities']
assert not receipt['fullBetaAccepted'] and not receipt['fullFeatureAccepted']
assert not receipt['backdropBlurAccepted'] and not receipt['responsivenessAccepted'] and not receipt['physicalDpiAccepted']
print(f"Verified {len(receipt['sourceBindings'])} source/{len(receipt['artifacts'])} artifact bindings: UI{sum(receipt['checks'].values())},29 rendered cards, cold-edge negative, ordinary pointer/placement/Quit. Broader gates remain open.")
