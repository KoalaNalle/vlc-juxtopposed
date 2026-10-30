"""Prepared WebVTT integration probe using the existing actual player/pixel test.

Run only after plugin qualification and the sole shared-tree GUI writer finish.
This is not ordinary-GUI, inline-color-transition, or final-package acceptance.
"""
import argparse, hashlib, json, os, re, shutil, sys, time
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
WORK=ROOT/'build-windows/beta-fps-webvtt-20261028'
FPS=ROOT/'build-windows/beta-fps-20261028'
BASE=ROOT/'build-windows/beta-follow-20261027/runtime-r7'
MANIFEST=BASE.parent/'runtime-r7-manifest.json'
SOURCE=ROOT/'modules/gui/qt/tests/test_player_subtitle_delay.cpp'
EXE=FPS/'test-r23/test_qt_player_subtitle_delay.exe'
PLUGIN_BUILD=WORK/'plugin-r2-20261029/build'
SELECTORS={'associatedFpsRetimesHeldCaption','firstDefaultFpsChangeRetimesPlayingCaption',
           'inheritedFpsDisplaysFirstAssociatedCue'}
sys.path.insert(0,str(ROOT/'test/libvlc'))
import native_http_runtime as support

def sha(p):
    with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()

def read(p):return json.loads(Path(p).read_text(encoding='utf-8-sig'))

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('variant',choices=['baseline','candidate'])
    parser.add_argument('selector',choices=sorted(SELECTORS))
    parser.add_argument('label')
    args=parser.parse_args()
    assert re.fullmatch(r'[a-z0-9_-]+',args.label)
    assert sha(SOURCE)=='80838e7b8198e5cd4f92d7b400016e536ed6d08c6e5820e2a10d5e8d7439bf65'
    assert sha(EXE)=='64846efd9fab959687dc99a6c8571b731900f5ff7d512ef61bca7ba907ad241b'
    assert sha(ROOT/'modules/demux/webvtt.c')=='433da57aa248b6be9b7bb65a4ddedd6c940eb1fb4f3fd3c9c6b14c25674df4e6'
    qualification=read(PLUGIN_BUILD/'verification.json')
    assert qualification['success'] and not qualification['forcedCleanupUsed']
    plugin=PLUGIN_BUILD/'libwebvtt_plugin.dll'
    assert sha(plugin)==qualification['pluginSha256']
    manifest=read(MANIFEST)
    assert len(manifest['files'])==533
    assert all(sha(BASE/n)==v['sha256'] for n,v in manifest['files'].items())
    original_webvtt='e6d599aabf0e035d6bce10b9a01e10b70bb1c889ca331604267823461aad9393'
    assert manifest['files']['plugins/codec/libwebvtt_plugin.dll']['sha256']==original_webvtt
    replacements={
        'libvlccore.dll':(FPS/'core-r5/libvlccore.dll','f8fc980bbe03da57908f79045f266710dd27a16c6ee66b6c45d60cd1f9b93598'),
        'plugins/codec/liblibass_plugin.dll':(FPS/'ass-plugin-r2/liblibass_plugin.dll','b622871c205345373343a463f193271b12cf875853e7c913c707acadb16e9b40'),
        'plugins/demux/libsubtitle_plugin.dll':(FPS/'subtitle-plugin-r1/libsubtitle_plugin.dll','1e8abb4593f82b5badf9dc9d103f655d4a0f30334c28f89e24b102b301139ea8')}
    if args.variant=='candidate':replacements['plugins/codec/libwebvtt_plugin.dll']=(plugin,qualification['pluginSha256'])
    assert all(sha(p)==h for p,h in replacements.values())
    assert shutil.disk_usage(ROOT).free>=399864390452+256*1024**2
    out=WORK/args.label;out.mkdir(exist_ok=False)
    runtime=out/'runtime';runtime.mkdir()
    for name in manifest['files']:
        rel=Path(name)
        assert not rel.is_absolute() and '..' not in rel.parts
        if rel.name=='plugins.dat' or rel.parts[0]=='portable':continue
        source=BASE/rel;target=runtime/rel
        assert source.is_file() and not source.is_symlink()
        target.parent.mkdir(parents=True,exist_ok=True)
        os.link(source,target) # Strict: a link-limit failure stops before launching.
    (runtime/'portable').mkdir()
    for name,(source,expected) in replacements.items():
        target=runtime/name
        target.unlink() # Never overwrite a hardlinked baseline DLL.
        shutil.copyfile(source,target)
        assert sha(target)==expected
    os.link(EXE,runtime/EXE.name)
    support.regenerate_cache(runtime,out/'cache')
    media=out/'media';media.mkdir()
    for name in ('frame-marker.mp4','duration-cues.srt'):
        os.link(FPS/'media-formats'/name,media/name)
    vtt='WEBVTT\n\n00:00:00.000 --> 00:00:19.999\nEXTERNAL FIRST CUE\n\n00:00:20.000 --> 00:00:39.999\nEXTERNAL MIDDLE CUE\n\n00:00:40.000 --> 00:00:59.900\nEXTERNAL LAST CUE\n'
    (media/'subtitle-style.vtt').write_text(vtt,encoding='utf-8',newline='\n')
    snaps=out/'snapshots';snaps.mkdir()
    expected={n:(replacements[n][1] if n in replacements else v['sha256'])
              for n,v in manifest['files'].items() if not n.endswith('plugins.dat') and not n.startswith('portable/')}
    assert all(sha(runtime/n)==h for n,h in expected.items())
    fixtures={p.name:sha(p) for p in media.iterdir()}
    inputs={str(p):sha(p) for p in (SOURCE,EXE,MANIFEST,plugin,Path(__file__),Path(support.__file__),PLUGIN_BUILD/'verification.json')}
    env=support.environment(runtime,out)
    for name in ('VLC_PLUGIN_PATH','QT_PLUGIN_PATH','QML_IMPORT_PATH','QML2_IMPORT_PATH'):env.pop(name,None)
    env.update(VLC_TEST_PLUGIN_PATH=str(runtime/'plugins'),VLC_SUBTITLE_FIXTURE=str(media/'frame-marker.mp4'),
               VLC_FPS_SNAPSHOTS=str(snaps),VLC_TEST_FIRST_FPS_FRESH='1',VLC_TEST_PRIMARY_SUBTITLE='subtitle-style.vtt',
               VLC_TEST_NATIVE_D3D11='1',QT_QPA_PLATFORM='offscreen',QT_QPA_FONTDIR=str(ROOT/'modules/gui/qt/pixmaps/juxtopposed'))
    if args.selector=='inheritedFpsDisplaysFirstAssociatedCue':env['VLC_TEST_INITIAL_SUBTITLE_FPS']='50'
    # run_owned supplies its own isolated environment; explicitly run this test
    # with the additional fixture settings, without changing the parent process.
    import ctypes, subprocess
    record={'status':'running','scope':'existing actual PlayerController and native pixel test on a generated three-cue external WebVTT; no ordinary UI or inline-style claim',
            'variant':args.variant,'selector':args.selector,'inputs':inputs,'fixtures':fixtures,'runtime':expected,'started':time.time()}
    def save():(out/'verification.json').write_text(json.dumps(record,indent=2)+'\n')
    save();child=None
    kernel=ctypes.WinDLL('kernel32',use_last_error=True)
    kernel.SetErrorMode.argtypes=[ctypes.c_uint];kernel.SetErrorMode.restype=ctypes.c_uint
    previous=kernel.SetErrorMode(0x8003)
    try:
        command=[str(runtime/EXE.name),args.selector,'-o',str(out/'result.xml')+',xml','-o',str(out/'result.txt')+',txt']
        with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
            child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
            record['pid']=child.pid;save()
            try:record['exitCode']=child.wait(timeout=180)
            except subprocess.TimeoutExpired:
                record.update(status='observation-timeout-process-left-running');save()
                raise SystemExit('Inspect this owned PID; do not restart the test.')
    finally:kernel.SetErrorMode(previous)
    record.update(status='terminal',ended=time.time(),inputsUnchanged=all(sha(p)==h for p,h in inputs.items()),
                  fixturesUnchanged=all(sha(media/n)==h for n,h in fixtures.items()),
                  originalRuntimeUnchanged=all(sha(BASE/n)==v['sha256'] for n,v in manifest['files'].items()),
                  stagedRuntimeUnchanged=all(sha(runtime/n)==h for n,h in expected.items()),
                  sourceReplacementsUnchanged=all(sha(p)==h for p,h in replacements.values()))
    tree=ET.parse(out/'result.xml').getroot()
    record['functions']=[{'name':f.get('name'),'incidents':[{'type':i.get('type'),'description':i.findtext('Description','')} for i in f.findall('Incident')]} for f in tree.findall('TestFunction')]
    selected=[f for f in record['functions'] if f['name']==args.selector]
    record['selectedFunctionExecuted']=len(selected)==1
    record['testPassed']=record['selectedFunctionExecuted'] and record['exitCode']==0 and not tree.findall('.//Message[@type="skip"]') and all(i['type']=='pass' for f in record['functions'] for i in f['incidents'])
    record['preservationPassed']=all(v for k,v in record.items() if k.endswith('Unchanged'))
    setup_ok=all(i['type']=='pass' for f in record['functions'] if f['name']!=args.selector for i in f['incidents'])
    expected_failure={'associatedFpsRetimesHeldCaption':'Live FPS/reset must update',
                      'firstDefaultFpsChangeRetimesPlayingCaption':'first default->50 FPS edit',
                      'inheritedFpsDisplaysFirstAssociatedCue':'pre-set 50 FPS'}[args.selector]
    record['negativeControlValid']=record['selectedFunctionExecuted'] and setup_ok and record['exitCode']==1 and any(i['type']=='fail' and expected_failure in i['description'] for f in selected for i in f['incidents'])
    save();print(json.dumps({k:record[k] for k in ('status','variant','selector','exitCode','testPassed','preservationPassed')},indent=2))
    assert record['preservationPassed']
    assert record['testPassed'] if args.variant=='candidate' else record['negativeControlValid']

if __name__=='__main__':main()
