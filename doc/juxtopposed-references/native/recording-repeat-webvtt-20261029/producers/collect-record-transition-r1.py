"""Collect generated-media evidence, omitting raw profiles, personal logs and binaries."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / 'build-windows/beta-record-transition-20261029'
GUI = ROOT / 'build-windows/beta-resume-20261029'
OUT = ROOT / 'doc/juxtopposed-references/native/recording-repeat-webvtt-20261029'


def load(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    assert not OUT.exists()
    before, after = [load(WORK / r / 'verification.json') for r in ['before-r2', 'after-r2']]
    assert before['exitCode'] == 1 and 'RESULT cases=7 failures=5' in before['stdout']
    assert after['exitCode'] == 0 and 'RESULT cases=7 failures=0' in after['stdout']
    assert all(v['normalTestResult'] and v['inputsUnchanged'] and v['runtimeUnchanged'] for v in [before, after])
    core = load(WORK / 'core-build-r1/verification.json')
    assert core['sharedTreeRestored'] and core['sourcesUnchanged'] and core['fallbackUnchanged']
    assert sha(ROOT / 'src/player/input.c') == 'fff61ed602e032b16837233157db38f61a20b29b8e827fd974fa796f96b1cdc1'
    assert sha(ROOT / 'test/src/player/recording.c') == after['build']['sourceSha256']
    OUT.mkdir(parents=True)
    copied = []

    def write(name, value):
        p = OUT / name
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(json.dumps(value, indent=2) + '\n')

    def copy(source, name):
        target = OUT / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        assert sha(source) == sha(target)
        copied.append(dict(file=name, sha256=sha(target), bytes=target.stat().st_size))

    gui_summary = {}
    runtime_keys = ['libvlccore.dll', 'plugins/gui/libqt_plugin.dll', 'plugins/codec/libwebvtt_plugin.dll',
                    'plugins/misc/libmedialibrary_plugin.dll', 'plugins/codec/libavcodec_plugin.dll']
    for run in ['gui-r10', 'gui-r11', 'gui-r13']:
        v = load(GUI / run / 'verification.json')
        assert v['status'] == 'terminal' and v['exitCode'] == 0 and v['preservationPassed']
        assert v['moduleCounts'] == ['603']
        log = (GUI / run / 'vlc.log').read_text(errors='replace').splitlines()
        diagnostic_indices = [i for i,l in enumerate(log) if 'error:' in l]
        assert all(log[i] == 'main error: cannot add this stream' and log[i-1] == 'avi debug: adding input'
                   for i in diagnostic_indices)
        assert len(v['diagnostics']) == len(diagnostic_indices)
        for p in sorted((GUI / run / 'observations').iterdir()):
            if p.is_file():
                copy(p, run + '/observations/' + p.name)
        events = [dict(line=i+1, text=l) for i,l in enumerate(log)
                  if l in ['qt debug: on_player_record_changed', 'qt debug: on_player_current_media_changed',
                           'main debug: EOF reached', 'main debug: using spu decoder module "webvtt"']
                  or "ES track selected (forced): 'spu/3' (fourcc: 'wvtt')" in l]
        gui_summary[run] = dict(childExitCode=v['exitCode'], modules=603,
            preservationPassed=v['preservationPassed'], diagnostics=v['diagnostics'],
            launcherExitCode=0 if not v['diagnostics'] else 1,
            diagnosticClassification='Rejected AVI format probes followed by successful MP4 selection' if diagnostic_indices else 'No captured error diagnostics',
            runtimeSha256={k:v['runtimeSha256'][k] for k in runtime_keys}, events=events)
    for label,v in [('before-r2',before),('after-r2',after)]:
        errors = [l for l in (WORK / label / 'stderr.txt').read_text(errors='replace').splitlines() if 'error:' in l]
        assert len(errors) == 7 and all('main window error: window closed' in l for l in errors)
        copy(WORK / label / 'stdout.txt', 'native/' + label + '.txt')
        write('native/' + label + '.json', dict(exitCode=v['exitCode'], inputsUnchanged=v['inputsUnchanged'],
              runtimeUnchanged=v['runtimeUnchanged'], normalTestResult=v['normalTestResult'],
              coreSha256=v['runtimeSha256']['libvlccore.dll'], test=v['build'],
              teardownWindowCloseDiagnostics=len(errors)))
    copy(WORK / 'after-r1/stdout.txt', 'native/initial-after-r1.txt')
    copy(WORK / 'test-build-r2/recording.c', 'native/initial-test.c')
    for name in ['verification.json', 'source-delta-from-f09.json', 'command.json', 'build.log']:
        copy(WORK / 'core-build-r1' / name, 'core-build/' + name)
    for name in ['verification.json', 'compile.command.json', 'compile.log', 'link.command.json', 'link.log']:
        copy(WORK / 'test-build-r3' / name, 'test-build/' + name)
    copy(ROOT / 'test/src/player/recording.c', 'producers/recording.c')
    for name in ['build-recording-transition-r3.py', 'build-recording-core-r1.py',
                 'run-recording-transition-before-r2.py', 'run-recording-transition-after-r2.py',
                 'run-gui-r10.py', 'run-gui-r11.py', 'run-gui-r13.py']:
        copy(GUI / name, 'producers/' + name)
    copy(Path(__file__), 'producers/' + Path(__file__).name)
    decoded = load(GUI / 'gui-r13/output-decode/verification.json')
    assert len(decoded['files']) == 2
    assert all(r['probeExit'] == 0 and r['decodeExit'] == 0 and not r['decodeErrors'] for r in decoded['files'])
    copy(GUI / 'gui-r13/output-decode/verification.json', 'output-decode.json')
    recorded = GUI / 'gui-r11/media/recorded-wvtt.mp4'
    command = ['wsl','-d','VLC-Juxtopposed-Home-20261018','--','ffprobe','-v','error',
               '-show_entries','stream=index,codec_name,codec_type,codec_tag_string,start_time,duration:format=duration,size',
               '-of','json','/mnt/c/' + recorded.relative_to(Path('C:/')).as_posix()]
    result = subprocess.run(command, capture_output=True, text=True, timeout=30)
    assert result.returncode == 0 and not result.stderr
    write('recorded-webvtt-probe.json', dict(fileSha256=sha(recorded), probe=load_text(result.stdout)))
    write('verification.json', dict(scope='Seven actual Core state cases and scoped ordinary Windows 11 recording/replay journeys; no full beta or content-tail claim',
          nativeCases=7, originalFailures=5, candidateFailures=0, coreBuild=core,
          ordinary=gui_summary, observations=dict(
              baseline='Record off works before repeat; stale checked state persists after repeat and an off request',
              candidate='An already-open menu clears at repeat; Record can be re-enabled and manually cleared while playback continues',
              replay='Embedded wvtt track available and selectable; EXTERNAL LAST CUE rendered during playback without external discovery'),
          limitations=['R10/R13 launcher assertions preserve rejected AVI probe diagnostics; children exit 0 and guards pass.',
                       'Native dummy-window teardown emits seven window-closed diagnostics for either Core.',
                       'Initial native after-r1 fixed the notification failures but two follow-up waits assumed StartPaused also affected seamless successors; corrected fixture uses explicit per-media start-paused and retains all strict assertions.',
                       'Recorded embedded cue selection on a held frame did not immediately render; paused refresh and exact recorded cue timing remain unqualified.',
                       'A/V decode checks available outputs, not complete source coverage, exact tails, all subtitle cues, broad format or package acceptance.']))
    write('copy-verification.json', copied)
    (OUT / '.gitattributes').write_text('* -text\n')
    print(json.dumps(dict(receipt=str(OUT), copiedFiles=len(copied), nativeCases=7, originalFailures=5, candidateFailures=0)))


def load_text(text):
    return json.loads(text)


if __name__ == '__main__':
    main()
