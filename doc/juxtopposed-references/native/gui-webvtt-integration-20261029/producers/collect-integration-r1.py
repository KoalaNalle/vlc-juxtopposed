"""Collect scoped terminal evidence; never start VLC or alter old receipts."""
import datetime
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import xml.etree.ElementTree as ET
from PySide6.QtGui import QImage

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / 'build-windows/beta-fps-webvtt-20261028'
OUT = ROOT / 'doc/juxtopposed-references/native/gui-webvtt-integration-20261029'


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def read(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))


def pixels(path):
    image = QImage(str(path))
    assert not image.isNull() and image.width() == 960 and image.height() == 540
    marker = sum((1 << bit) for bit in range(11) if image.pixelColor(32 + 64 * bit, 20).red() > 128)
    bright = sum(1 for y in range(405, 540) for x in range(960)
                 if all(channel > 160 for channel in image.pixelColor(x, y).getRgb()[:3]))
    return dict(frameMarker=marker, bottomBrightPixels=bright, sha256=sha(path))


def main():
    assert not OUT.exists()
    OUT.mkdir(parents=True)
    copies = []

    def copy(source, relative):
        target = OUT / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        assert sha(source) == sha(target)
        copies.append(dict(path=relative, sha256=sha(target), bytes=target.stat().st_size))

    cases = []
    for variant in ('candidate', 'baseline'):
        for label in ('held', 'playing', 'inherited'):
            source = WORK / f'player-{variant}-{label}-r2-20261029'
            record = read(source / 'verification.json')
            assert record['status'] == 'terminal' and record['preservationPassed']
            xml = ET.parse(source / 'result.xml').getroot()
            functions = [{'name': f.get('name'), 'incidents': [
                {'type': i.get('type'), 'description': i.findtext('Description') or ''}
                for i in f.findall('Incident')]} for f in xml.findall('TestFunction')]
            assert functions == record['functions']
            assert not xml.findall('.//Message[@type="skip"]')
            if variant == 'candidate':
                assert record['testPassed'] and record['exitCode'] == 0
            else:
                assert record['exitCode'] == 1 and not record['negativeControlValid']
            log = (source / 'stderr.txt').read_text(errors='replace')
            assert 'using demux module "webvtt"' in log and 'using spu decoder module "webvtt"' in log
            errors = [line for line in log.splitlines() if ' error:' in line]
            assert len(errors) == 1 and re.fullmatch(r'\[[0-9a-f]+\] main window error: window closed', errors[0])
            case = dict(variant=variant, case=label, exitCode=record['exitCode'],
                        functions=functions, testPassed=record['testPassed'],
                        allPreservationGuards=True, actualWebvttDemuxAndDecoderSelected=True,
                        expectedFpsNegativeControlQualified=record['negativeControlValid'],
                        diagnostics=['Window-close notification during native harness teardown; observed in both variants'],
                        fixtures=record['fixtures'], snapshots={})
            for image in sorted((source / 'snapshots').glob('*.png')):
                case['snapshots'][image.name] = pixels(image)
            for name in ('result.xml', 'result.txt'):
                copy(source / name, f'native/{variant}-{label}/{name}')
            cases.append(case)

    ordinary = []
    for label in ('r6', 'r7', 'r8'):
        source = ROOT / f'build-windows/beta-resume-20261029/gui-{label}'
        record = read(source / 'verification.json')
        assert record['status'] == 'terminal' and record['exitCode'] == 0 and record['preservationPassed']
        assert record['moduleCounts'] == ['603'] and not record['forcedCleanupUsed']
        guards = {name: value for name, value in record.items() if name.endswith('Unchanged')}
        if label != 'r8':
            assert record['diagnostics'] == []
        else:
            assert record['diagnostics'] == ['main error: cannot add this stream'] * 2
            lines = (source / 'vlc.log').read_text(errors='replace').splitlines()
            start = next(i for i, line in enumerate(lines) if 'probing muxer avi' in line)
            end = next(i for i, line in enumerate(lines) if 'probing muxer mp4' in line)
            assert all(start < i < end for i, line in enumerate(lines) if ' error:' in line)
            assert any('using muxer mp4 with extension mp4 (3/3 streams accepted)' in line for line in lines)
        ordinary.append(dict(run=label, vlcExitCode=0, preservationGuards=guards,
                             moduleCounts=record['moduleCounts'], diagnostics=record['diagnostics'],
                             interpretation='Two AVI format-probe rejections before successful MP4 output' if label == 'r8' else 'No detected error diagnostics'))

    recording_root = ROOT / 'build-windows/beta-resume-20261029/gui-r8/recordings'
    recordings = list(recording_root.iterdir())
    assert len(recordings) == 1 and recordings[0].is_file()
    recording = recordings[0]
    linux_path = '/mnt/c/' + recording.relative_to(Path('C:/')).as_posix()
    prefix = ['wsl', '-d', 'VLC-Juxtopposed-Home-20261018', '--']
    probe = subprocess.run(prefix + ['ffprobe', '-v', 'error', '-count_frames', '-show_entries',
        'stream=index,codec_name,codec_type,codec_tag_string,nb_read_frames,duration',
        '-show_entries', 'format=duration,size', '-of', 'json', linux_path],
        stdin=subprocess.DEVNULL, capture_output=True, timeout=30)
    assert probe.returncode == 0 and not probe.stderr
    decode = subprocess.run(prefix + ['ffmpeg', '-v', 'error', '-i', linux_path,
        '-map', '0:v:0', '-map', '0:a:0', '-f', 'null', '-'],
        stdin=subprocess.DEVNULL, capture_output=True, timeout=30)
    assert decode.returncode == 0 and not decode.stderr
    record_info = dict(bytes=recording.stat().st_size, sha256=sha(recording),
                       probe=json.loads(probe.stdout), avDecodeExitCode=decode.returncode,
                       avDecodeErrors=[], subtitlePlaybackQualified=False,
                       scope='This manually started output decodes A/V; no exact tail, full input, subtitle playback, or manual Record-off acceptance')
    summary = dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
                   scope='New ordinary GUI and WebVTT integration only; no complete beta or final package claim',
                   native=cases, ordinary=ordinary, recording=record_info,
                   ordinaryFpsObservations='Playback-settings 50 FPS changed held first cue to middle; synchronization Reset restored normal timing and reflected 0 FPS back in playback settings; generated clip had start-time=12, so displayed elapsed time is relative to that start',
                   recordObservations='Checked state appeared after enabling; native Stop returned Home and disabled/cleared the action. Manual off while repeat was active did not produce a confirmed clear state; keep open.',
                   openInteraction='PlaybackSettingsMenu lacks modal: true; observed outside dismissal also toggling video underneath. Needs controlled before/after qualification.',
                   nativeBaselineLimit='All original-plugin runs failed before the intended FPS-specific assertion. Held failed a missing reference caption; playing/inherited failed initial track-role setup. None qualifies the runner\'s intended FPS negative control.',
                   inputs={str(path.relative_to(ROOT)): sha(path) for path in (
                       ROOT / 'modules/gui/qt/menus/menus.cpp', ROOT / 'modules/gui/qt/medialibrary/mlrecentvideomodel.cpp',
                       ROOT / 'modules/demux/webvtt.c', ROOT / 'modules/gui/qt/tests/test_player_subtitle_delay.cpp')})
    (OUT / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
    build = ROOT / 'build-windows/beta-record-feedback-20261028/ordinary-build-r6-20261029'
    for name in ('result.json', 'candidate-built.json', 'restoration.json'):
        copy(build / name, 'gui-build/' + name)
    copy(WORK / 'plugin-r2-20261029/build/verification.json', 'webvtt-build.json')
    copy(WORK / 'run-player-r2.py', 'producers/run-player-r2.py')
    copy(__file__, 'producers/collect-integration-r1.py')
    observations = ROOT / 'build-windows/beta-resume-20261029/gui-r8/observations'
    selected = ('03-fps-fifty-committed', '06-cross-editor-reset-held-cue',
                '07-reset-reflected-in-playback-editor', '08-record-menu-before-toggle',
                '09-record-enabled-checkmark', '12-record-disabled-after-stop')
    for label in selected:
        data = read(observations / (label + '.json'))
        copy(observations / (label + '.json'), 'ordinary-observations/' + label + '.json')
        for image in data['screenshots']:
            copy(observations / image['file'], 'ordinary-observations/' + image['file'])
    (OUT / 'copy-verification.json').write_text(json.dumps(copies, indent=2) + '\n')
    (OUT / '.gitattributes').write_text('* -text\n')
    print(json.dumps(dict(receipt=str(OUT), candidateNativePasses=3, intendedBaselineNegativesQualified=0,
                         ordinaryVlcNormalExits=3, avDecodePassed=True, copiedFiles=len(copies)), indent=2))


if __name__ == '__main__':
    main()
