"""Collect the terminal native popup regression result without starting apps."""
import hashlib
import json
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / 'build-windows/beta-resume-20261029'
GUI = WORK / 'gui-r9'
BUILD = ROOT / 'build-windows/beta-record-feedback-20261028/ordinary-build-r9-20261029'
OUT = ROOT / 'doc/juxtopposed-references/native/playback-settings-dismissal-20261029'


def read(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    gui = read(GUI / 'verification.json')
    build = read(BUILD / 'result.json')
    assert gui['status'] == 'terminal' and gui['exitCode'] == 0 and gui['preservationPassed']
    assert gui['moduleCounts'] == ['603'] and gui['diagnostics'] == [] and not gui['forcedCleanupUsed']
    assert build['status'] == 'built-not-desktop-qualified' and not build['forcedCleanupUsed']
    assert build['originalSharedArtifactsRestored'] and build['protectedUnchanged']
    assert build['inputsUnchanged'] and build['fullBaselineRuntimeUnchanged']
    assert build['noLiveOwnedWriterGroups']
    closure = read(BUILD / 'actual-compile-closure.json')
    assert closure['count'] == 937 and closure['unqualified'] == []
    assert (BUILD / 'exit.txt').read_text().strip() == '0'
    candidate = read(BUILD / 'candidate-built.json')
    assert candidate['sha256'] == gui['runtimeSha256']['plugins/gui/libqt_plugin.dll']
    assert candidate['sha256'] == '7c7db5cb675edc20b3366120dc55282b09d2db2dc731b7c6ed8c1815e0706624'
    assert sha(GUI / 'runtime/plugins/gui/libqt_plugin.dll') == candidate['sha256']
    assert not OUT.exists()
    OUT.mkdir(parents=True)
    copies = []

    def copy(path, relative):
        target = OUT / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(path, target)
        assert sha(path) == sha(target)
        copies.append(dict(file=relative, sha256=sha(target), bytes=target.stat().st_size))

    observations = {}
    for path in sorted((GUI / 'observations').glob('*.json')):
        value = read(path)
        observations[path.stem] = value['accessibility']['tree']
        copy(path, 'observations/' + path.name)
        for image in value['screenshots']:
            copy(path.parent / image['file'], 'observations/' + image['file'])
    paused = observations['03-outside-dismiss-keeps-paused']
    released = observations['04-closed-popup-releases-video-click']
    playing_before = observations['05-playing-popup-open']
    playing_after = observations['06-outside-dismiss-keeps-playing']
    escaped = observations['07-escape-closes-keeps-paused']
    reopened = observations['08-reopened-subtitle-controls-work']
    checks = dict(
        outsidePaused='Play ID:' in paused and 'Elapsed time: 00:00:00' in paused and 'Player settings ID:' not in paused,
        closedReleasesClick='Pause ID:' in released and 'Player settings ID:' not in released,
        playingPopupOpens='Pause ID:' in playing_before and 'Player settings ID:' in playing_before,
        outsideKeepsPlaying='Pause ID:' in playing_after and 'Elapsed time: 00:00:34' in playing_after and 'Player settings ID:' not in playing_after,
        escapeKeepsPaused='Play ID:' in escaped and 'Elapsed time: 00:00:42' in escaped and 'Player settings ID:' not in escaped,
        reopenAndTab='Player settings ID:' in reopened and 'Subtitle frame rate Value: 0.000 fps' in reopened,
        cleanNormalExit=True, preservation=True, sourceAndBinaryBound=True)
    assert all(checks.values()), checks
    selected = ('libvlccore.dll', 'plugins/gui/libqt_plugin.dll', 'plugins/codec/libwebvtt_plugin.dll',
                'plugins/misc/libmedialibrary_plugin.dll', 'plugins/codec/libavcodec_plugin.dll')
    report = dict(scope='Actual ordinary Windows 11 popup input ownership, one generated fixture/profile and window topology; no full beta, broad DPI or final-package claim',
                  passed=True, checks=checks, runtimeSha256={name: gui['runtimeSha256'][name] for name in selected},
                  sourceSha256={name: gui['sourceSha256'][name] for name in (
                      'modules/gui/qt/player/qml/PlaybackSettingsMenu.qml',
                      'modules/gui/qt/tests/test_juxtopposed_settings.py',
                      'modules/gui/qt/menus/menus.cpp', 'modules/gui/qt/medialibrary/mlrecentvideomodel.cpp')},
                  ordinary=dict(exitCode=0, modules=603, diagnostics=[],
                                preservationGuards={k: v for k, v in gui.items() if k.endswith('Unchanged')}),
                  build=dict(result=build, candidate=candidate, actualCompilerClosure=closure,
                             manifestSha256=sha(BUILD / 'final-inputs.json')),
                  componentProof=read(ROOT / 'doc/juxtopposed-references/native/gui-webvtt-integration-20261029/popup-component-observations.json'),
                  limitations=['Supplied-state component tests are separate from ordinary checks.',
                               'No physical DPI matrix, multi-instance, all pages or final package acceptance.',
                               'No Record behavior or output qualification in this run.'])
    (OUT / 'verification.json').write_text(json.dumps(report, indent=2) + '\n')
    for name in ('generate-plan.txt', 'compile-plan.txt', 'archive-plan.txt', 'link-plan.txt',
                 'restoration.json', 'actual-compile-closure.json'):
        copy(BUILD / name, 'build/' + name)
    copy(WORK / 'run-gui-r9.py', 'producers/run-gui-r9.py')
    copy(__file__, 'producers/collect-popup-native-r1.py')
    (OUT / 'copy-verification.json').write_text(json.dumps(copies, indent=2) + '\n')
    (OUT / '.gitattributes').write_text('* -text\n')
    print(json.dumps(dict(receipt=str(OUT), checks=checks, nativeQtSha256=candidate['sha256'], copiedFiles=len(copies)), indent=2))


if __name__ == '__main__':
    main()
