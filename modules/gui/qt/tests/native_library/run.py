#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Run the bounded Windows opening/folder or real-process Library acceptance.

Uses an ordinary package plus private test executables/modules. Everything is
copied into a fresh owned directory; the supplied package is never modified.
The metadata scope uses owned providers with cross-process trace receipts.
"""
import argparse
from hashlib import file_digest
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import time
import wave
import xml.etree.ElementTree as ET
import zlib

OPENING_CASES = [
    'exactApiAndDefaultGates', 'legacyOpeningProfileRemainsUnmapped',
    'typedInvalidInputsLeaveEverythingUnchanged', 'genuineNativeStorageAndFreshReads',
    'realLockedSaveRollbackAndRetry', 'actualInstanceDecision', 'actualSenderMode',
    'actualReceiverQueue', 'invalidExplicitOpenDoesNotMutate', 'hiddenNativeFolderEntries',
    'playingDirectoryUsesHiddenPreference',
    'realQueueRecursion', 'disablingLibraryPreservesRealRootsPlaylistFavoriteHistoryAndHome',
    'externalFolderSubtreeRetainsSubmittedItem',
    'externalDirectoryPolicyChangesBetweenRequests',
    'externalFolderExpansionDoesNotWaitForOptionalArtwork',
]
METADATA_CASES = [
    'metadataFetchLifecycle', 'automaticQueueMetadataScope',
    'automaticLibraryMetadataScope', 'libraryEnrichmentTimeoutKeepsEmbeddedMetadataAndTracks',
    'libraryShutdownDuringEnrichmentCanResume',
    'externalMetadataProviderBridge', 'queueEnrichmentTimeoutKeepsParsedDetails',
    'removingQueueItemCancelsLookupAndAllowsNextItem',
]


def digest(path):
    with path.open('rb') as stream:
        return file_digest(stream, 'sha256').hexdigest()


def fixtures(folder):
    folder.mkdir()
    (folder / 'sub').mkdir()
    files = ['one.wav', 'two.wav', 'visible.wav', 'hidden.wav', '.dot.wav', 'sub/child.wav']
    for name in files:
        path = folder / name
        with wave.open(str(path), 'wb') as output:
            output.setnchannels(1)
            output.setsampwidth(2)
            output.setframerate(8000)
            output.writeframes(b'\0\0' * 8000 * 60)
        raw = bytearray(path.read_bytes())
        title = b'Owned embedded title\0'
        info = b'INFOINAM' + struct.pack('<I', len(title)) + title
        if len(title) % 2:
            info += b'\0'
        raw.extend(b'LIST' + struct.pack('<I', len(info)) + info)
        struct.pack_into('<I', raw, 4, len(raw) - 8)
        path.write_bytes(raw)

    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
    png = b'\x89PNG\r\n\x1a\n'
    png += chunk(b'IHDR', struct.pack('>IIBBBBB', 1, 1, 8, 2, 0, 0, 0))
    png += chunk(b'IDAT', zlib.compress(b'\0\xd3\x79\x18')) + chunk(b'IEND', b'')
    (folder / 'art.png').write_bytes(png)
    identities = {name: digest(folder / name) for name in files + ['art.png']}
    (folder / 'fixture-identities.json').write_text(json.dumps(identities, indent=2) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('scope', choices=('opening', 'process', 'metadata', 'metadata-lifecycle'))
    parser.add_argument('--package', type=Path, required=True)
    parser.add_argument('--native-build', type=Path, required=True)
    parser.add_argument('--module-build', type=Path)
    parser.add_argument('--core', type=Path)
    parser.add_argument('--medialibrary', type=Path)
    parser.add_argument('--preparser', type=Path)
    parser.add_argument('--output-dir', type=Path, required=True)
    args = parser.parse_args()
    if os.name != 'nt':
        parser.error('Run under Windows against a matching Windows x64 package')
    if args.scope != 'process' and args.module_build is None:
        parser.error('Opening acceptance requires --module-build')
    package, build, out = args.package.resolve(), args.native_build.resolve(), args.output_dir.resolve()
    if out == package or package in out.parents:
        parser.error('Use an output directory outside the supplied package')
    out.mkdir(parents=True, exist_ok=False)
    runtime, results = out / 'runtime', out / 'results'
    runtime.mkdir()
    (runtime / "portable").mkdir()
    results.mkdir()
    media = out / 'media'
    fixtures(media)
    personal = Path(os.environ['APPDATA']) / 'vlc'
    before = {name: digest(personal / name) if (personal / name).exists() else None
              for name in ('vlcrc', 'vlc-qt-interface.ini')}
    shutil.copytree(package / 'plugins', runtime / 'plugins', ignore=shutil.ignore_patterns('plugins.dat'))
    for name in ('libvlc.dll', 'libvlccore.dll', 'vlc.exe', 'vlc-preparser.exe', 'vlc-cache-gen.exe'):
        shutil.copyfile(package / name, runtime / name)
    for source, destination in ((args.core, 'libvlccore.dll'),
                                (args.medialibrary, 'plugins/misc/libmedialibrary_plugin.dll'),
                                (args.preparser, 'vlc-preparser.exe')):
        if source is not None:
            shutil.copyfile(source.resolve(), runtime / destination)
    program = 'test_qt_library_preferences.exe' if args.scope != 'process' else 'test_library_opening_process.exe'
    shutil.copyfile(build / '.libs' / program, runtime / program)
    env = os.environ.copy()
    for name in list(env):
        if name.startswith('VLC_LIBRARY_FIXTURE_'):
            env.pop(name)
    for name in ('VLC_PLUGIN_PATH', 'VLC_LIB_PATH', 'VLC_LIBRARY_OUTPUT', 'VLC_LIBRARY_MEDIA'):
        env.pop(name, None)
    if args.scope != 'process':
        modules = args.module_build.resolve() / '.libs'
        shutil.copyfile(modules / 'liblibrary_fixture_fetcher_plugin.dll',
                        runtime / 'plugins/meta_engine/liblibrary_fixture_fetcher_plugin.dll')
        probe = runtime / 'liblibrary_winmsg_probe_plugin.dll'
        shutil.copyfile(modules / probe.name, probe)
        env['VLC_LIBRARY_WINMSG_PLUGIN'] = str(probe)
        env['VLC_LIBRARY_OUTPUT'] = str(results / 'generated')
    else:
        env.update(VLC_LIBRARY_SENDER=str(runtime / 'vlc.exe'),
                   VLC_LIBRARY_PROCESS_TRACE=str(results / 'process-trace.txt'),
                   VLC_LIBRARY_PROCESS_OUTPUT=str(results))
    env['VLC_USERDATA_PATH'] = str(results / 'cache-data')
    cache = subprocess.run([str(runtime / 'vlc-cache-gen.exe'), str(runtime / 'plugins')],
                           cwd=runtime, env=env, capture_output=True, timeout=60,
                           creationflags=subprocess.CREATE_NO_WINDOW)
    (results / 'cache-stderr.txt').write_bytes(cache.stderr)
    cache.check_returncode()
    env.update(VLC_PLUGIN_PATH=str(runtime / 'plugins'), VLC_LIB_PATH=str(runtime / 'plugins'),
               VLC_USERDATA_PATH=str(results / 'userdata'), VLC_LIBRARY_MEDIA=str(media),
               QT_QPA_PLATFORM='windows')
    command = [str(runtime / program), '-o', str(results / 'qtest.txt') + ',txt',
               '-o', str(results / 'qtest.xml') + ',xunitxml']
    if args.scope == 'opening':
        command += OPENING_CASES
    elif args.scope == 'metadata':
        command += METADATA_CASES
    elif args.scope == 'metadata-lifecycle':
        command += ['metadataFetchLifecycle']
    names = [program, 'libvlc.dll', 'libvlccore.dll', 'vlc.exe', 'vlc-preparser.exe',
             'plugins/gui/libqt_plugin.dll', 'plugins/control/libwin_msg_plugin.dll',
             'plugins/access/libfilesystem_plugin.dll',
             'plugins/misc/libpreparserserializer_json_plugin.dll']
    names += ['plugins/misc/libmedialibrary_plugin.dll']
    if args.scope != 'process':
        names += ['liblibrary_winmsg_probe_plugin.dll',
                  'plugins/meta_engine/liblibrary_fixture_fetcher_plugin.dll']
    identities = {name: digest(runtime / name) for name in names}
    (results / 'launch.json').write_text(json.dumps({'command': command, 'runtimeSha256': identities}, indent=2) + '\n')
    started = time.monotonic()
    timed_out = False
    with (results / 'stdout.txt').open('wb') as stdout, (results / 'stderr.txt').open('wb') as stderr:
        process = subprocess.Popen(command, cwd=runtime, env=env, stdout=stdout, stderr=stderr,
                                   creationflags=subprocess.CREATE_NO_WINDOW)
        (results / 'pid.txt').write_text(str(process.pid))
        try:
            code = process.wait(timeout=300)
        except subprocess.TimeoutExpired:
            # Preserve the PID/output and report an incomplete result. Never
            # silently turn an unconfirmed termination into acceptance.
            timed_out, code = True, process.poll()
    counts = {}
    if (results / 'qtest.xml').exists():
        try:
            suite = ET.parse(results / 'qtest.xml').getroot()
            counts = {key: int(suite.attrib[key]) for key in ('tests', 'failures', 'errors', 'skipped')}
        except ET.ParseError:
            pass
    unchanged = before == {name: digest(personal / name) if (personal / name).exists() else None for name in before}
    report = {'scope': args.scope, 'exitCode': code, 'terminationConfirmed': process.poll() is not None,
              'timedOut': timed_out, 'forcedCleanupUsed': False, 'elapsedSeconds': time.monotonic() - started,
              'counts': counts, 'runtimeSha256': identities, 'personalConfigUnchanged': unchanged,
              'metadataScopeIncluded': args.scope in ('metadata','metadata-lifecycle')}
    (results / 'verification.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))
    expected = {'opening':42, 'process':9, 'metadata':18, 'metadata-lifecycle':8}[args.scope]
    assert not timed_out and code == 0 and unchanged and report['terminationConfirmed']
    assert counts == {'tests': expected, 'failures': 0, 'errors': 0, 'skipped': 0}


if __name__ == '__main__':
    main()
