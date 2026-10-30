"""Reuse the identified regression client against current Core/codec bytes.

This is an existing compiled PlayerController client, not a fresh ordinary GUI
build or proof that every controller source dependency has been rebuilt.
"""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'build-windows/beta-subtitle-roles-20261029/current-r2'
BASE=ROOT/'build-windows/beta-resume-20261029/gui-r27/runtime'
BASE_RESULT=BASE.parent/'verification.json'
EXE=ROOT/'build-windows/beta-fps-20261028/test-r23/test_qt_player_subtitle_delay.exe'
SOURCE=ROOT/'modules/gui/qt/tests/test_player_subtitle_delay.cpp'
MEDIA=ROOT/'build-windows/beta-fps-20261028/media-formats'
sys.path.insert(0,str(ROOT/'test/libvlc'))
import native_http_runtime as support


def sha(p):
    with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()


def main():
    assert not OUT.exists() and shutil.disk_usage(ROOT).free>=399864390452+256*1024**2
    assert sha(SOURCE)=='80838e7b8198e5cd4f92d7b400016e536ed6d08c6e5820e2a10d5e8d7439bf65'
    assert sha(EXE)=='64846efd9fab959687dc99a6c8571b731900f5ff7d512ef61bca7ba907ad241b'
    base=json.loads(BASE_RESULT.read_text(encoding='utf-8'))
    assert base['status']=='terminal' and base['exitCode']==0 and base['preservationPassed']
    assert all(sha(BASE/p)==h for p,h in base['runtimeSha256'].items())
    OUT.mkdir(parents=True)
    runtime=OUT/'runtime';support.stage_runtime(BASE,runtime)
    os.link(EXE,runtime/EXE.name)
    support.regenerate_cache(runtime,OUT/'cache')
    media=OUT/'media';media.mkdir()
    for name in ['frame-marker.mp4','subtitle-style.srt','duration-cues.srt']:
        os.link(MEDIA/name,media/name)
    inputs={str(p):sha(p) for p in [Path(__file__),Path(support.__file__),BASE_RESULT,EXE,SOURCE,*sorted(media.iterdir())]}
    expected=dict(base['runtimeSha256']);expected[EXE.name]=sha(EXE)
    record=dict(status='running',scope='Identified existing regression client on repaired Coreccb6/SRT/ASS/WebVTT38/TTML9d, not fresh ordinary Qt or full source-closure acceptance',
                inputs=inputs,runtimeSha256=expected,cases=[],forcedCleanupUsed=False)
    save=lambda:(OUT/'verification.json').write_text(json.dumps(record,indent=2)+'\n')
    save()
    selectors=['categoryRelativeAdjustmentsAndResetNotifications',
               'largeTrackDelaysKeepHeldFrameAndResume:own-secondary',
               'largeTrackDelaysKeepHeldFrameAndResume:inherited-secondary',
               'firstFpsChangePreservesEmbeddedSecondary',
               'firstPlayingFpsChangePreservesEmbeddedSecondary',
               'firstPlayingFpsChangeRetimesBothTrackDelays:opposed-secondary-ended',
               'estimatorDirectionsAndCancellation','estimatorCancellationDoesNotPublishAnAdjustment',
               'estimatorAppliesMediaTime','timingContextCancellation']
    for i,selector in enumerate(selectors):
        run=OUT/f'case-{i}';run.mkdir();snaps=run/'snapshots';snaps.mkdir()
        env=support.environment(runtime,run)
        for name in ['VLC_PLUGIN_PATH','QT_PLUGIN_PATH','QML_IMPORT_PATH','QML2_IMPORT_PATH']:env.pop(name,None)
        env.update(VLC_TEST_PLUGIN_PATH=str(runtime/'plugins'),VLC_SUBTITLE_FIXTURE=str(media/'frame-marker.mp4'),
                   VLC_FPS_SNAPSHOTS=str(snaps),VLC_TEST_FIRST_FPS_FRESH='1',VLC_TEST_NATIVE_D3D11='1',
                   QT_QPA_PLATFORM='offscreen',QT_QPA_FONTDIR=str(ROOT/'modules/gui/qt/pixmaps/juxtopposed'))
        command=[str(runtime/EXE.name),selector,'-o',str(run/'result.xml')+',xml','-o',str(run/'result.txt')+',txt']
        row=dict(selector=selector,status='starting',command=command);record['cases'].append(row);save()
        with (run/'stdout.txt').open('wb') as stdout,(run/'stderr.txt').open('wb') as stderr:
            child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
            row.update(status='running',pid=child.pid);save();print('OWNED_ROLE_TEST '+str(child.pid)+' '+selector,flush=True)
            try:code=child.wait(timeout=180)
            except subprocess.TimeoutExpired:
                row['status']='observation-timeout-process-left-running';save();raise SystemExit('Inspect same PID, do not restart')
        tree=ET.parse(run/'result.xml').getroot()
        row.update(status='terminal',exitCode=code,functions=[dict(name=f.get('name'),incidents=[dict(type=v.get('type'),description=v.findtext('Description','')) for v in f.findall('Incident')]) for f in tree.findall('TestFunction')])
        selected=[f for f in row['functions'] if f['name']==selector.split(':',1)[0]]
        row['passed']=code==0 and len(selected)==1 and not tree.findall('.//Message[@type="skip"]') and all(v['type']=='pass' for f in row['functions'] for v in f['incidents'])
        save()
        print(json.dumps(dict(selector=selector,exitCode=code,passed=row['passed'])),flush=True)
        if not row['passed']:break
    record.update(status='terminal',inputsUnchanged=all(sha(p)==h for p,h in inputs.items()),
                  runtimeUnchanged=all(sha(runtime/p)==h for p,h in expected.items()),
                  fallbackUnchanged=all(sha(BASE/p)==h for p,h in base['runtimeSha256'].items()))
    record['passed']=len(record['cases'])==len(selectors) and all(r['passed'] for r in record['cases'])
    record['passed'] &= record['inputsUnchanged'] and record['runtimeUnchanged'] and record['fallbackUnchanged']
    save();print(json.dumps({k:record[k] for k in ['passed','inputsUnchanged','runtimeUnchanged','fallbackUnchanged']},indent=2));assert record['passed']


if __name__=='__main__':main()
