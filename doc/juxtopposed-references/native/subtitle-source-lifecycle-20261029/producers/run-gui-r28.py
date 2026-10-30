"""Stage the reviewed ordinary GUI candidate in a copied generated test profile."""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / 'build-windows/beta-record-feedback-20261028'
BUILD = WORK / 'ordinary-build-r9-20261029'
BASE = ROOT / 'build-windows/beta-home-sections-20261028/history-actions-ui-r4/runtime'
PROFILE = ROOT / 'build-windows/beta-resume-20261029/gui-r27'
OUT = ROOT / 'build-windows/beta-resume-20261029/gui-r28'
sys.path.insert(0, str(ROOT / 'test/libvlc'))
import native_http_runtime as support


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def load(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))


def main():
    assert len(sys.argv) == 1
    result = load(BUILD / 'result.json')
    assert result['status'] == 'built-not-desktop-qualified'
    assert result['originalSharedArtifactsRestored'] and result['protectedUnchanged']
    assert result['inputsUnchanged'] and result['fullBaselineRuntimeUnchanged']
    assert result['noLiveOwnedWriterGroups'] and not result['forcedCleanupUsed']
    assert (BUILD / 'exit.txt').read_text().strip() == '0'
    manifest = load(BUILD / 'final-inputs.json')
    candidate = BUILD / 'candidate/libqt_plugin.dll'
    candidate_sha = load(BUILD / 'candidate-built.json')['sha256']
    assert sha(candidate) == candidate_sha
    assert all(sha(ROOT / name) == value for name, value in manifest['sources'].items())
    baseline = manifest['baselineRuntime']['files']
    assert all(sha(BASE / name) == value['sha256'] for name, value in baseline.items())
    profile_result = load(PROFILE / 'verification.json')
    assert profile_result['status']=='terminal' and profile_result['exitCode']==0 and profile_result['preservationPassed']
    assert shutil.disk_usage(ROOT).free >= 399864390452 + 256 * 1024**2
    subtitle_checks = load(ROOT / 'build-windows/beta-record-transition-20261029/paused-selection-r6/verification.json')
    assert subtitle_checks['status']=='terminal' and subtitle_checks['inputsUnchanged'] and subtitle_checks['runtimeUnchanged']
    assert all(r['passed'] for r in subtitle_checks['children'])
    OUT.mkdir()
    runtime = OUT / 'runtime'
    runtime.mkdir()
    expected = {}
    for name, item in baseline.items():
        rel = Path(name)
        assert not rel.is_absolute() and '..' not in rel.parts
        if rel.parts[0] == 'portable' or rel.name == 'plugins.dat':
            continue
        source, target = BASE / rel, runtime / rel
        assert source.is_file() and not source.is_symlink()
        target.parent.mkdir(parents=True, exist_ok=True)
        os.link(source, target)
        expected[name] = item['sha256']
    core_work = ROOT / 'build-windows/beta-preset-frame-20261029'
    core_result = load(core_work / 'core-build-r1/verification.json')
    native_result = load(core_work / 'after-r1/verification.json')
    assert native_result['status']=='terminal' and native_result['allFramesPreserved']
    assert native_result['inputsUnchanged'] and native_result['runtimeUnchanged'] and native_result['fallbackUnchanged']
    assert core_result['status']=='built-not-qualified' and core_result['sharedTreeRestored']
    assert core_result['inputsUnchanged'] and core_result['sourcesUnchanged'] and core_result['fallbackUnchanged']
    core_candidate = core_work / 'core-build-r1/candidate/libvlccore.dll'
    assert sha(core_candidate)==core_result['candidateSha256']
    (runtime / 'libvlccore.dll').unlink()
    shutil.copyfile(core_candidate,runtime / 'libvlccore.dll')
    expected['libvlccore.dll']=core_result['candidateSha256']
    target = runtime / 'plugins/gui/libqt_plugin.dll'
    target.unlink()
    shutil.copyfile(candidate, target)
    expected['plugins/gui/libqt_plugin.dll'] = candidate_sha
    plugin_build = ROOT / 'build-windows/beta-fps-webvtt-20261028/plugin-r2-20261029/build'
    plugin_receipt = load(plugin_build / 'verification.json')
    assert plugin_receipt['success'] and not plugin_receipt['forcedCleanupUsed']
    plugin = plugin_build / 'libwebvtt_plugin.dll'
    assert sha(plugin) == plugin_receipt['pluginSha256']
    target = runtime / 'plugins/codec/libwebvtt_plugin.dll'
    target.unlink()
    shutil.copyfile(plugin, target)
    expected['plugins/codec/libwebvtt_plugin.dll'] = plugin_receipt['pluginSha256']
    manifest['sources']['modules/demux/webvtt.c'] = '433da57aa248b6be9b7bb65a4ddedd6c940eb1fb4f3fd3c9c6b14c25674df4e6'
    assert sha(ROOT / 'modules/demux/webvtt.c') == manifest['sources']['modules/demux/webvtt.c']
    manifest['sources']['src/player/input.c'] = '94442df44bdd10073dc315aa357c6ae74e8b5b69ccf8e4e6b674d210711cec8d'
    assert sha(ROOT / 'src/player/input.c') == manifest['sources']['src/player/input.c']
    manifest['sources']['src/player/player.h'] = '664eb6dd9d23a7e0c6c1763bb0a318e293b442516209e74f7c89b96cdb12bb57'
    assert sha(ROOT / 'src/player/player.h') == '664eb6dd9d23a7e0c6c1763bb0a318e293b442516209e74f7c89b96cdb12bb57'
    manifest['sources']['src/player/timer.c'] = 'fa061a3c2fbaa4c24ae0ec00269998f574b07b616c5118ca58a862ff9c739c48'
    assert sha(ROOT / 'src/player/timer.c') == 'fa061a3c2fbaa4c24ae0ec00269998f574b07b616c5118ca58a862ff9c739c48'
    ttml_work = ROOT / 'build-windows/beta-ttml-fps-20261029'
    ttml_build = load(ttml_work / 'plugin-build-r4/verification.json')
    ttml_native = load(ttml_work / 'after-r5/verification.json')
    assert ttml_build['success'] and ttml_build['ownedArtifactsRestored'] and ttml_build['inputsUnchanged']
    assert ttml_native['passed'] and ttml_native['inputsUnchanged'] and ttml_native['runtimeUnchanged']
    ttml_plugin = ttml_work / 'plugin-build-r4/libttml_plugin.dll'
    assert sha(ttml_plugin) == ttml_build['pluginSha256']
    target = runtime / 'plugins/codec/libttml_plugin.dll'
    target.unlink()
    shutil.copyfile(ttml_plugin, target)
    expected['plugins/codec/libttml_plugin.dll'] = ttml_build['pluginSha256']
    for name in ['modules/demux/ttml.c', 'modules/codec/ttml/genttml.c', 'modules/codec/ttml/ttml.h', 'modules/codec/ttml/ttml.c']:
        value = ttml_native['inputs'][str(ROOT / name)]
        assert sha(ROOT / name) == value
        manifest['sources'][name] = value
    media = OUT / 'media'
    media.mkdir()
    fixture_source = ROOT / 'build-windows/beta-subtitle-roles-20261029/gui-media-r1'
    fixture_manifest = load(fixture_source / 'verification.json')
    for name in ['frame-marker.mp4','subtitle-style.srt']:
        assert sha(fixture_source / name)==fixture_manifest['files'][name]
        os.link(fixture_source / name,media / name)
    os.link(media/'frame-marker.mp4',media/'frame-marker-b.mp4')
    fixtures = {str(file): sha(file) for file in media.iterdir()}
    recordings = OUT / 'recordings'
    recordings.mkdir()
    (runtime / 'portable').mkdir()
    preserved = {}
    for source, target in ((PROFILE / 'userdata', OUT / 'userdata'),
                           (PROFILE / 'runtime/portable', runtime / 'portable')):
        for file in source.rglob('*'):
            if file.is_file():
                assert not file.is_symlink()
                preserved[str(file)] = sha(file)
                dest = target / file.relative_to(source)
                dest.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(file, dest)
    config = OUT / 'vlcrc'
    original_config = PROFILE / 'vlcrc'
    preserved[str(original_config)] = sha(original_config)
    config_text = original_config.read_text(encoding='utf-8')
    assert config_text.count('sub-language=fin')==1
    config_text=config_text.replace('sub-language=fin','sub-language=eng')
    config.write_text(config_text, encoding='utf-8', newline='\n')
    support.regenerate_cache(runtime, OUT / 'cache')
    assert all(sha(runtime / name) == value for name, value in expected.items())
    human = {str(Path(os.environ['APPDATA']) / 'vlc' / name): None
             for name in ('vlcrc', 'vlc-qt-interface.ini')}
    human = {name: sha(name) if Path(name).is_file() else None for name in human}
    inputs = {str(path): sha(path) for path in (Path(__file__), Path(support.__file__),
              BUILD / 'result.json', BUILD / 'final-inputs.json', PROFILE / 'verification.json', candidate, plugin, plugin_build / 'verification.json',
              core_candidate, core_work / 'core-build-r1/verification.json', core_work / 'after-r1/verification.json',
              ttml_plugin, ttml_work / 'plugin-build-r4/verification.json', ttml_work / 'after-r5/verification.json')}
    record = dict(status='starting', scope='Ordinary saved subtitle language/current preservation and same-player Next/Previous automatic selection; generated two-item queue; no beta/package acceptance',
                  runtimeSha256=expected, sourceSha256=manifest['sources'],
                  inputs=inputs, priorProfileSha256=preserved, humanBeforeSha256=human,
                  started=time.time(), forcedCleanupUsed=False, fixtures=fixtures)

    def save():
        temp = OUT / 'verification.next.json'
        temp.write_text(json.dumps(record, indent=2) + '\n')
        os.replace(temp, OUT / 'verification.json')

    env = support.environment(runtime, OUT)
    for name in ('VLC_PLUGIN_PATH', 'QT_PLUGIN_PATH', 'QML_IMPORT_PATH', 'QML2_IMPORT_PATH'):
        env.pop(name, None)
    command = [str(runtime / 'vlc.exe'), '--intf=qt', '--no-one-instance', '--media-library',
               '--config=' + str(config), '--no-crashdump', '--no-qt-privacy-ask', '--no-qt-updates-notif',
               '--no-metadata-network-access', '--no-ml-network-discovery', '--no-qt-video-autoresize',
               '--no-qt-remember-window-geometry', '--file-logging', '--verbose=2', '--log-verbose=2',
               '--logfile=' + str(OUT / 'vlc.log'), '--qt-juxtopposed-controls',
               '--input-record-path=' + str(recordings), '--no-sub-autodetect-file',
               '--start-paused', str(media / 'frame-marker.mp4'), ':start-time=12', ':start-paused',
               str(media / 'frame-marker-b.mp4'), ':start-time=12', ':start-paused']
    record['command'] = command
    save()
    with (OUT / 'stdout.txt').open('wb') as stdout, (OUT / 'stderr.txt').open('wb') as stderr:
        child = subprocess.Popen(command, cwd=runtime, env=env, stdout=stdout, stderr=stderr,
                                 creationflags=subprocess.CREATE_NO_WINDOW)
        record.update(status='running', pid=child.pid)
        save()
        print('OWNED_GUI_RUNNING ' + str(child.pid), flush=True)
        try:
            code = child.wait(timeout=1800)
        except subprocess.TimeoutExpired:
            record.update(status='observation-timeout-process-left-running')
            save()
            raise SystemExit('Inspect exact owned PID; do not restart.')
    text = '\n'.join((OUT / name).read_text(errors='replace') for name in ('vlc.log', 'stderr.txt'))
    record.update(status='terminal', exitCode=code, normalExit=code == 0, ended=time.time(),
                  fixturesUnchanged=all(sha(name) == value for name, value in fixtures.items()),
                  sourcesUnchanged=all(sha(ROOT / name) == value for name, value in manifest['sources'].items()),
                  runtimeUnchanged=all(sha(runtime / name) == value for name, value in expected.items()),
                  fallbackBytesUnchanged=all(sha(BASE / name) == value['sha256'] for name, value in baseline.items()),
                  priorProfileUnchanged=all(sha(name) == value for name, value in preserved.items()),
                  inputsUnchanged=all(sha(name) == value for name, value in inputs.items()),
                  humanConfigUnchanged=all((sha(name) if Path(name).is_file() else None) == value for name, value in human.items()),
                  moduleCounts=re.findall(r'plug-ins loaded: (\d+) modules', text),
                  diagnostics=[line for line in text.splitlines() if re.search(r'\berror:|Assertion failed|Binding loop|ReferenceError|TypeError|is not a type|Cannot assign', line)])
    record['preservationPassed'] = all(value for key, value in record.items() if key.endswith('Unchanged'))
    save()
    print(json.dumps({key: record[key] for key in ('status', 'exitCode', 'preservationPassed', 'moduleCounts', 'diagnostics')}, indent=2))
    assert code == 0 and record['preservationPassed'] and not record['diagnostics']


if __name__ == '__main__':
    main()
