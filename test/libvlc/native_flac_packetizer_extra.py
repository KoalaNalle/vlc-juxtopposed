"""Run the actual loaded FLAC packetizer with bounded encoded/malformed fixtures."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import subprocess

CASES = ['flac_short_final_frame_and_repeated_drain',
         'flac_split_native_frames_and_complete_drain',
         'flac_every_truncated_short_frame_rejected',
         'flac_bad_crc_flush_then_valid_frame',
         'flac_flush_discards_partial_lookahead',
         'flac_truncated_blocksize_guarded_shared_header', 'flac_packetizer_native_extra_cleanup']


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', type=Path, required=True)
    parser.add_argument('--test-exe', type=Path, required=True)
    parser.add_argument('--fixtures', type=Path, required=True)
    parser.add_argument('--output-dir', type=Path, required=True)
    args = parser.parse_args()
    package, executable, fixtures, output = [path.resolve() for path in
        [args.package, args.test_exe, args.fixtures, args.output_dir]]
    output.mkdir(parents=True, exist_ok=False)
    report = {'startedUtc': datetime.now(timezone.utc).isoformat(), 'passed': False,
              'package': str(package), 'testExe': str(executable), 'fixtures': str(fixtures),
              'expectedCases': CASES, 'timeoutSeconds': 45,
              'scope': 'Actual loaded FLAC packetizer module using real native-encoded 1/B+1 frame fixtures, 1/2/3/7/17-byte feed boundaries, complete/repeated drain, original packet bytes/PTS, truncated/footer-CRC rejection and flush recovery. Explicit malformed two-byte blocksize header is tested against current shared native source with an inaccessible following page. Separate from full decoder/playback/GUI acceptance and actual system OOM.'}

    def save():
        (output / 'verification.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')

    try:
        required = [executable, package / 'libvlc.dll', package / 'libvlccore.dll',
                    package / 'plugins/packetizer/libpacketizer_flac_plugin.dll',
                    fixtures / 'boundary-one.flac', fixtures / 'boundary-B-plus-one.flac',
                    fixtures / 'verification.json']
        for path in required:
            if not path.is_file():
                raise RuntimeError('Missing native runtime or real boundary fixture: ' + str(path))
        gate = json.loads((fixtures / 'verification.json').read_text(encoding='utf-8-sig'))
        if gate.get('passed') is not True or len(gate.get('cases', [])) != 9 or not all(row.get('passed') is True for row in gate['cases']):
            raise RuntimeError('Reuse the preserved fully passing frozen native FLAC nine-case result')
        for name in ['libvlc.dll', 'libvlccore.dll']:
            adjacent = executable.parent / name
            if adjacent.exists():
                required.append(adjacent)
                if digest(adjacent) != digest(package / name):
                    raise RuntimeError('Adjacent core differs from selected package: ' + name)
        report['inputHashes'] = {str(path): digest(path) for path in required}
        root = Path(__file__).resolve().parents[2]
        report['sourceHashesAtRun'] = {str(path.relative_to(root)): digest(path) for path in
            [root / 'test/modules/packetizer/flac_eos.c', root / 'modules/packetizer/flac.h']}
        report['sourceBuildProof'] = 'Root must retain compilation log/manifest for this executable; current source identities alone do not prove that build.'
        userdata = output / 'userdata'
        userdata.mkdir()
        environment = os.environ.copy()
        environment.pop('VLC_PLUGIN_PATH', None)
        environment.update(PATH=str(package) + os.pathsep + environment.get('PATH', ''),
            VLC_LIB_PATH=str(package / 'plugins'), VLC_USERDATA_PATH=str(userdata),
            VLC_FLAC_BOUNDARY_DIR=str(fixtures))
        with (output / 'stdout.txt').open('wb') as stdout, (output / 'stderr.txt').open('wb') as stderr:
            process = subprocess.Popen([str(executable)], cwd=output, env=environment,
                stdout=stdout, stderr=stderr, creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
            report['ownedPid'] = process.pid
            save()
            try:
                report['exitCode'] = process.wait(timeout=45)
            except subprocess.TimeoutExpired:
                report['timedOut'] = True
                process.kill()
                report['exitCode'] = process.wait(timeout=10)
        content = (output / 'stdout.txt').read_text(encoding='utf-8-sig', errors='replace')
        actual = [line.removeprefix('PASS ').strip() for line in content.splitlines() if line.startswith('PASS ')]
        report['actualCases'] = actual
        log = output / 'packetizer-flac-extra.log'
        report['loadedFlacPacketizer'] = log.is_file() and 'using audio packetizer module "flac"' in log.read_text(encoding='utf-8-sig', errors='replace')
        report['passed'] = report['exitCode'] == 0 and actual == CASES and report['loadedFlacPacketizer'] and not report.get('timedOut', False)
        report['evidenceHashes'] = {path.name: digest(path) for path in
            [output / 'stdout.txt', output / 'stderr.txt', log] if path.is_file()}
    except Exception as error:
        report['error'] = str(error)
    report['finishedUtc'] = datetime.now(timezone.utc).isoformat()
    save()
    print(json.dumps({name: report.get(name) for name in ['passed', 'exitCode', 'timedOut', 'error']}, indent=2))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
