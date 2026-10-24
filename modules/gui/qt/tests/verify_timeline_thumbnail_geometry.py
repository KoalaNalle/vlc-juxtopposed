# SPDX-License-Identifier: GPL-2.0-or-later
"""Verify saved native geometry repair evidence and its current source bindings.

Does not execute VLC or establish new-Core visible GUI, all media, design, DPI,
latency, final package or full beta acceptance.
"""
from pathlib import Path
from hashlib import sha256
import argparse
import json
import xml.etree.ElementTree as ET

repo=Path(__file__).resolve().parents[4]
base=repo/'doc/juxtopposed-references/native/home-timeline-thumbnails-20261021/geometry-r30'
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
verify(args.source_root,receipt['sourceBindings'])
verify(base,receipt['artifacts'])
after=json.loads((base/'native-after.json').read_text())
before=json.loads((base/'native-before.json').read_text())
assert after['passed'] and after['normalExit'] and after['exitCode']==0
assert after['counts']==dict(tests=17,failures=0,errors=0,skipped=0)
assert not before['passed'] and before['exitCode']==3
assert before['counts']==dict(tests=17,failures=3,errors=0,skipped=0)
assert all(r['sourceInputsUnchanged'] and not r['forcedCleanupUsed'] for r in [before,after])
same=['test_qt_timeline_thumbnails.exe','plugins/gui/libqt_plugin.dll','plugins/demux/libmp4_plugin.dll']
assert all(before['runtimeHashes'][p]==after['runtimeHashes'][p] for p in same)
assert before['runtimeHashes']['libvlccore.dll']!=after['runtimeHashes']['libvlccore.dll']
failures=ET.parse(base/'results-before.xml').getroot().findall('.//testcase[failure]')
assert len(failures)==3 and all('geometryMatchesIndependentReference' in p.attrib['name'] and 'rotate-' in p.attrib['name'] for p in failures)
manifest=json.loads((base/'fixture-manifest.json').read_text())
assert manifest['syntheticOnly'] and len(manifest['fixtures'])==6
for name,record in manifest['fixtures'].items():
    stem=Path(name).stem
    assert sha256((base/'references'/f'{stem}.png').read_bytes()).hexdigest()==record['referenceSha256']
    if stem.startswith('rotate-'):
        rotations=[s['rotation']%360 for s in record['probe']['streams'][0].get('side_data_list',[]) if 'rotation' in s]
        assert rotations==[int(stem.split('-')[1])]
audit=json.loads((base/'rejected-fixtures-r25.json').read_text())
assert audit['rotationQualificationAccepted'] is False
assert not receipt['fullBetaAccepted'] and not receipt['fullFeatureAccepted'] and not receipt['newCoreVisibleGuiAccepted']
print(f"Verified {len(receipt['sourceBindings'])} source and {len(receipt['artifacts'])} artifact bindings: native17/17, paired old Core3 rotation failures, six synthetic geometry fixtures. Broader gates remain open.")
