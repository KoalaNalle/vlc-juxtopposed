#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Console-only verification of saved Audio defaults on real Windows WASAPI."""
import argparse
import ctypes
from hashlib import file_digest
import json
import math
import os
from pathlib import Path
import shutil
import subprocess
import struct
import sys
import wave
import xml.etree.ElementTree as ET


def digest(path):
    with path.open('rb') as stream:
        return file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', type=Path, required=True)
    parser.add_argument('--program', type=Path, required=True)
    parser.add_argument('--inputs', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--suite', choices=['startup', 'defaults'], default='startup')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[5]
    package, program, output = (p.resolve() for p in (args.package, args.program, args.output))
    assert os.name == 'nt'
    assert output != package and package not in output.parents
    assert not output.exists()
    inputs = json.loads(args.inputs.read_text(encoding='utf-8-sig'))

    def inputs_match():
        return all(digest(root / path) == value for path, value in inputs.items())

    assert inputs_match(), 'Source no longer matches the compiled input manifest'
    protected_names = ['vlc.exe', 'libvlc.dll', 'libvlccore.dll', 'plugins/gui/libqt_plugin.dll',
                       'plugins/audio_output/libmmdevice_plugin.dll',
                       'plugins/audio_output/libwasapi_plugin.dll']
    before = {name: digest(package / name) for name in protected_names}
    personal = Path(os.environ['APPDATA']) / 'vlc'
    personal_before = {name: digest(personal / name) if (personal / name).exists() else None
                       for name in ['vlcrc', 'vlc-qt-interface.ini']}
    output.mkdir(parents=True)
    results = output / 'results'
    results.mkdir()
    sys.path.insert(0, str(root / 'test/libvlc'))
    from native_http_runtime import stage_runtime, environment, regenerate_cache
    runtime = output / 'runtime'
    stage_runtime(package, runtime)
    destination = runtime / program.name
    assert not destination.exists()
    shutil.copyfile(program, destination)
    regenerate_cache(runtime, output / 'cache')
    fixture = results / ('silence.wav' if args.suite == 'startup' else 'tone.wav')
    with wave.open(str(fixture), 'wb') as stream:
        stream.setnchannels(2 if args.suite == 'startup' else 1)
        stream.setsampwidth(2)
        stream.setframerate(48000 if args.suite == 'startup' else 44100)
        if args.suite == 'startup':
            stream.writeframes(b'\0' * 4 * 48000 * 30)
        else:
            second = b''.join(struct.pack('<h', round(8000 * math.sin(2 * math.pi * 1000 * frame / 44100)))
                              for frame in range(44100))
            stream.writeframes(second * 30)
    env = environment(runtime, results)
    env.update(VLC_AUDIO_STARTUP_OUTPUT=str(results / 'owned'),
               VLC_AUDIO_STARTUP_WAV=str(fixture),
               VLC_AUDIO_DEFAULTS_OUTPUT=str(results / 'owned'),
               VLC_AUDIO_DEFAULTS_FIXTURE=str(fixture))
    command = [str(destination), '-o', str(results / 'qtest.txt') + ',txt',
               '-o', str(results / 'qtest.xml') + ',xunitxml']
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.SetErrorMode.argtypes = [ctypes.c_uint]
    kernel.SetErrorMode.restype = ctypes.c_uint
    previous = kernel.SetErrorMode(0x8003)
    child = None
    result = dict(passed=False, forcedCleanupUsed=False, scope='Console Qt model + real Core playlist + MMDevice/WASAPI; no desktop interaction or physical audibility proof')
    if args.suite == 'defaults':
        result['scope'] = 'Existing Audio defaults/rollback suite and actual AMEM PCM/filter callbacks; no desktop interaction or Windows output-session proof'
    try:
        with (results / 'stdout.txt').open('wb') as stdout, (results / 'stderr.txt').open('wb') as stderr:
            child = subprocess.Popen(command, cwd=runtime, env=env, stdout=stdout, stderr=stderr,
                                     creationflags=subprocess.CREATE_NO_WINDOW)
            try:
                result['exitCode'] = child.wait(timeout=90)
            except subprocess.TimeoutExpired:
                result['forcedCleanupUsed'] = True
                child.kill()
                result['exitCode'] = child.wait(timeout=5)
    finally:
        kernel.SetErrorMode(previous)
        if child is not None and child.poll() is None:
            result['forcedCleanupUsed'] = True
            child.kill()
            child.wait(timeout=5)
    result['normalExit'] = result.get('exitCode') == 0 and not result['forcedCleanupUsed']
    counts = dict(tests=0, failures=0, errors=0, skipped=0)
    names = set()
    if (results / 'qtest.xml').exists():
        xml = ET.parse(results / 'qtest.xml').getroot()
        for key in counts:
            counts[key] = int(xml.get(key, '0'))
        names = {node.get('name') for node in xml.iter('testcase')}
    text = (results / 'qtest.txt').read_text(encoding='utf-8', errors='replace') if (results / 'qtest.txt').exists() else ''
    diagnostics = [line for line in text.splitlines() if any(word in line for word in ['QWARN', 'QFATAL', 'QCRITICAL'])]
    logs = [path.read_text(encoding='utf-8', errors='replace') for path in (results / 'owned').rglob('vlc-open-*.log')]
    wasapi_logs = sum('using audio output module "mmdevice"' in log and 'using aout stream module "wasapi"' in log for log in logs)
    result.update(counts=counts, caseNames=sorted(names), diagnostics=diagnostics,
                  realWasapiOutputSessions=wasapi_logs, sourceInputsUnchanged=inputs_match(),
                  protectedRuntimeUnchanged=before == {name: digest(package / name) for name in before},
                  personalConfigUnchanged=personal_before == {name: digest(personal / name) if (personal / name).exists() else None for name in personal_before},
                  runtimeSha256=before, executableSha256=digest(destination), runnerSha256=digest(Path(__file__)),
                  inputManifestSha256=digest(args.inputs), generatedWaveSha256=digest(fixture),
                  uiConsumerGatesEnabled=False, fullBetaAccepted=False)
    expected = {'savedDeviceVolumeAndStepReachRealWasapi'} if args.suite == 'startup' else {
        'genuineSavedDefaultsDriveNewOutputAndLiveEqualizerPcm',
        'genuineOutputVisualizationCallbackSurvivesFilterRestart',
        'nativeFailureRestoresPairedFilterAndFloatThenRecovers'}
    result['passed'] = (result['normalExit'] and not diagnostics and counts['tests'] >= (3 if args.suite == 'startup' else 31)
                        and not any(counts[key] for key in ['failures', 'errors', 'skipped'])
                        and expected.issubset(names) and (wasapi_logs >= 2 if args.suite == 'startup' else True)
                        and result['sourceInputsUnchanged'] and result['protectedRuntimeUnchanged']
                        and result['personalConfigUnchanged'])
    (output / 'verification.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: result[key] for key in ['passed', 'normalExit', 'counts', 'realWasapiOutputSessions', 'diagnostics']}, indent=2))
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
