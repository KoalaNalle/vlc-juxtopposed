#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Check default track consumers using a console Core player and owned media."""
import argparse,ctypes,json,os,re,shutil,subprocess,sys
from pathlib import Path
from hashlib import file_digest
import xml.etree.ElementTree as ET

def digest(path):
    with path.open('rb') as stream:return file_digest(stream,'sha256').hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ['package','program','inputs','fixture','output']:
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--hls-fixture',type=Path)
    parser.add_argument('--short-fixtures',type=Path)
    args=parser.parse_args();repo=Path(__file__).resolve().parents[5]
    package,program,fixture,output=(p.resolve() for p in [args.package,args.program,args.fixture,args.output])
    assert os.name=='nt' and not output.exists() and output!=package and package not in output.parents
    inputs=json.loads(args.inputs.read_text(encoding='utf-8-sig'))
    def unchanged():return all(digest(repo/p)==h for p,h in inputs.items())
    assert unchanged()
    protected=['vlc.exe','libvlc.dll','libvlccore.dll','plugins/gui/libqt_plugin.dll',
               'plugins/audio_output/libmmdevice_plugin.dll','plugins/audio_output/libwasapi_plugin.dll',
               'plugins/video_output/libwdummy_plugin.dll','plugins/video_output/libvdummy_plugin.dll',
               'plugins/logger/libfile_logger_plugin.dll']
    before={p:digest(package/p) for p in protected};fixture_before=digest(fixture)
    hls=args.hls_fixture.resolve() if args.hls_fixture else None
    hls_before={}
    if hls:
        assert hls.is_relative_to(repo/'build-windows') and hls.is_file()
        hls_before={p.relative_to(hls.parent).as_posix():digest(p) for p in hls.parent.rglob('*') if p.suffix in ['.m3u8','.ts']}
    short=args.short_fixtures.resolve() if args.short_fixtures else None
    short_before={}
    if short:
        assert short.is_relative_to(repo/'build-windows') and short.is_dir()
        short_before={p.name:digest(p) for p in short.glob('*.wav')}
        assert len(short_before)==3
    personal=Path(os.environ['APPDATA'])/'vlc'
    personal_before={n:digest(personal/n) if (personal/n).exists() else None for n in ['vlcrc','vlc-qt-interface.ini']}
    output.mkdir(parents=True);results=output/'results';results.mkdir()
    sys.path.insert(0,str(repo/'test/libvlc'))
    from native_http_runtime import stage_runtime,environment,regenerate_cache
    runtime=output/'runtime';stage_runtime(package,runtime)
    target=runtime/program.name;assert not target.exists();shutil.copyfile(program,target)
    regenerate_cache(runtime,output/'cache')
    env=environment(runtime,results)
    env.update(VLC_PLAYBACK_CONSUMER_OUTPUT=str(results/'owned'),VLC_PLAYBACK_CONSUMER_FIXTURE=str(fixture))
    if hls:env['VLC_PLAYBACK_HLS_FIXTURE']=str(hls)
    if short:env['VLC_PLAYBACK_SHORT_FIXTURES']=str(short)
    command=[str(target),'-o',str(results/'qtest.txt')+',txt','-o',str(results/'qtest.xml')+',xunitxml']
    kernel=ctypes.WinDLL('kernel32',use_last_error=True);kernel.SetErrorMode.argtypes=[ctypes.c_uint];kernel.SetErrorMode.restype=ctypes.c_uint
    old=kernel.SetErrorMode(0x8003);child=None;report={'passed':False,'forcedCleanupUsed':False}
    try:
        with (results/'stdout.txt').open('wb') as stdout,(results/'stderr.txt').open('wb') as stderr:
            child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
            try:report['exitCode']=child.wait(timeout=90)
            except subprocess.TimeoutExpired:
                report['forcedCleanupUsed']=True;child.kill();report['exitCode']=child.wait(timeout=5)
    finally:
        kernel.SetErrorMode(old)
        if child is not None and child.poll() is None:
            report['forcedCleanupUsed']=True;child.kill();child.wait(timeout=5)
    counts=dict(tests=0,failures=0,errors=0,skipped=0);names=set()
    xml_error=None
    if (results/'qtest.xml').exists():
        try:
            xml=ET.parse(results/'qtest.xml').getroot();counts={k:int(xml.get(k,'0')) for k in counts}
            names={row.get('name') for row in xml.iter('testcase')}
        except ET.ParseError as error:
            xml_error=str(error);counts['errors']=1
    text=(results/'qtest.txt').read_text(encoding='utf-8',errors='replace') if (results/'qtest.txt').exists() else ''
    diagnostics=[line for line in text.splitlines() if any(word in line for word in ['QWARN','QFATAL','QCRITICAL','QSYSTEM'])]
    native_logs=[p.read_text(encoding='utf-8',errors='replace') for p in (results/'owned').rglob('vlc-open-*.log')]
    native_text='\n'.join(native_logs)
    modules=lambda kind:sorted(set(re.findall(r'using '+re.escape(kind)+r' module "([^"]+)"',native_text)))
    native_errors=[line for line in native_text.splitlines() if re.search(r'\berror:',line)]
    native_warnings=[line for line in native_text.splitlines() if re.search(r'\bwarning:',line)]
    # avcodec declines unvalidated text subtitles during normal module probing;
    # the selected subsdec decoder then handles these owned embedded SRT tracks.
    expected_probe='avcodec warning: refusing to decode non validated subtitle codec'
    unexpected_warnings=[line for line in native_warnings if line!=expected_probe]
    hardware_lines=[line for line in native_text.splitlines() if re.match(r'(?:d3d11va|dxva2)\s',line)]
    isolation=dict(windowModules=modules('vout window'),displayModules=modules('vout display'),
                   audioModules=modules('audio output'),nativeLogCount=len(native_logs),
                   nativeErrors=native_errors,unexpectedNativeWarnings=unexpected_warnings,
                   expectedSubtitleProbeWarningCount=native_warnings.count(expected_probe),
                   hardwareDecoderLines=hardware_lines)
    isolation['passed']=(bool(native_logs) and isolation['windowModules']==['wdummy']
        and isolation['displayModules']==['vdummy'] and isolation['audioModules']==['adummy']
        and not native_errors and not unexpected_warnings and not hardware_lines)
    report.update(normalExit=report.get('exitCode')==0 and not report['forcedCleanupUsed'],counts=counts,caseNames=sorted(names),diagnostics=diagnostics,xmlParseError=xml_error,
      sourceInputsUnchanged=unchanged(),sourceRuntimeUnchanged=before=={p:digest(package/p) for p in protected},
      fixtureUnchanged=fixture_before==digest(fixture),fixtureSha256=fixture_before,
      hlsFixturesUnchanged=all(digest(hls.parent/p)==value for p,value in hls_before.items()) if hls else True,
      hlsFixtureSha256=hls_before,
      shortFixturesUnchanged=all(digest(short/p)==value for p,value in short_before.items()) if short else True,
      shortFixtureSha256=short_before,
      personalConfigUnchanged=personal_before=={n:digest(personal/n) if (personal/n).exists() else None for n in personal_before},
      runtimeSha256=before,executableSha256=digest(target),runnerSha256=digest(Path(__file__)),inputManifestSha256=digest(args.inputs),
      scope='Actual production Playback/Audio models and Core main playlist: tracks, local adaptive quality, start policy, saved rate, audio startup policy, finite-input repeat/shuffle/stop and owned-instance finish-action exit notifications. Paused independence, same-player next input and cold reopen. Highest adaptive logic isolates resolution filtering from bandwidth variation; no HTTP/service or rendering/desktop/package acceptance. Dummy sinks create no window or audible output. Finish-action callbacks observe only the isolated instance; no desktop process is closed.',
      fullBetaAccepted=False)
    expected={'actualSelectedTracksConsumeSavedDefaultsOnReopen',
              'actualStartPlaybackPolicyAffectsNextInputWithoutResumingCurrent',
              'actualSavedRateRequiresNewPlayerAndPreservesCurrentRate',
              'actualRepeatCurrentRestartsOnlyTheSameFiniteInput',
              'actualRepeatAllTraversesFiniteInputsThenWraps',
              'actualStopAfterEachItemDoesNotStartTheNextQueuedInput',
              'actualFinishActionRequestsExitAfterFiniteQueueEnd',
              'actualFinishActionChangesLiveAndManualStopRequestsExit',
              'actualStopFinishActionRestoresSavedStopAfterWithoutExit',
              'actualTimeShiftConstructorConsumesNextInputDefaults',
              'actualTimeShiftConstructorUsesOwnedSystemTempAndNativeChunkDefault',
              'actualShuffleUsesRandomOrderWithoutRepeatingOrLosingInputs',
              'actualPreferredAudioLanguageUsesSavedStartupDefault',
              'actualAudioEnabledIsAStartupPolicyAndPreservesCurrentTrack',
              'actualSavedAudioDelayIsConsumedByNextInput'}
    if hls:expected.add('actualAdaptiveResolutionConsumesSavedMaximumOnNextInput')
    report['nativeFixtureIsolation']=isolation
    report['timeShiftDiskSpoolingAccepted']=False
    report['timeShiftScope']='Actual Core input-constructor log messages show saved folder/chunk consumption, paused independence, same-player next input, reopen and owned system-temp/native default fallback. No live-source pause-buffer disk writes or chunk rollover accepted.'
    report['passed']=report['normalExit'] and not diagnostics and isolation['passed'] and counts['tests']>=(18 if hls else 17) and not any(counts[k] for k in ['failures','errors','skipped']) and expected.issubset(names) and all(report[k] for k in ['sourceInputsUnchanged','sourceRuntimeUnchanged','fixtureUnchanged','hlsFixturesUnchanged','shortFixturesUnchanged','personalConfigUnchanged'])
    (output/'verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({k:report[k] for k in ['passed','normalExit','counts','diagnostics']},indent=2))
    return 0 if report['passed'] else 1

if __name__=='__main__':raise SystemExit(main())
