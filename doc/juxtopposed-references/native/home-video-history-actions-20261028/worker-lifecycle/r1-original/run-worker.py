"""One-use ordinary-runtime native QObject suite; no GUI window or new build."""
from pathlib import Path
from collections import Counter
import argparse,json,os,subprocess,sys,time,xml.etree.ElementTree as ET
from worker_support import digest,save,storage,seconds_left,check_map,stage_subset
import native_http_runtime as helper

original_environment=helper.environment
def environment(runtime,output):
    value=original_environment(runtime,output)
    value.pop('VLC_PLUGIN_PATH',None)
    value['QTEST_FUNCTION_TIMEOUT']='30000'
    return value
helper.environment=environment

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--execute-reviewed-run',action='store_true',required=True)
parser.parse_args()
prep=Path(__file__).resolve().parent
repo=prep.parents[2]
out=prep/'run'; out.mkdir(exist_ok=False)
record=dict(status='preflight',started=time.time(),scope='Actual MLRecentVideoModel QObject/QThread lifecycle with real native ML/player; no GUI claim',
    deadlineLocal='2026-10-28T23:35:00+02:00',forcedCleanupUsed=False,errors=[])
def write(): save(out/'verification.json',record)
write()

def verify_linux(label):
    assert seconds_left()>150, 'Insufficient 23:35 cleanup reserve'
    command=['wsl','-d','VLC-Juxtopposed-Home-20261018','--','python3',
        '/mnt/c/Modding/vlc-juxtopposed/'+prep.relative_to(repo).as_posix()+'/verify-compile-inputs.py']
    result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=60)
    (out/(label+'.stdout.txt')).write_bytes(result.stdout)
    (out/(label+'.stderr.txt')).write_bytes(result.stderr)
    assert result.returncode==0, 'Exact compiler input verification failed: '+label
    return json.loads(result.stdout.decode())['success']

try:
    assert seconds_left()>360, 'Need six minutes before the 23:35 stop bound'
    budget=storage(repo); record['storageBefore']=budget
    # A completed build already occupies its outputs. Only run/database/cache
    # space is additional here; hardlinked immutable DLLs/EXE consume no copy.
    run_budget=112*2**20
    record['runPeakBudgetBytes']=run_budget
    assert storage(repo,run_budget)['fits'], 'Run would cross owner 80% floor'
    producers=json.loads((prep/'producer-identities.json').read_text())
    assert not check_map(prep,producers), 'Frozen producer changed'
    build=json.loads((prep/'build/verification.json').read_text())
    assert build['status']=='terminal' and build['success'] and build['exitCode']==0
    assert build['writerLockReleased'] and build['writerCleanupConfirmed'] and not build['forcedBuildCleanupUsed']
    assert build['strippedDebugVerified'] and build['actualDependenciesGuarded']
    record['compilerInputsBefore']=verify_linux('compiler-inputs-before')
    baseline=json.loads((prep/'baseline.json').read_text())
    base=repo/baseline['relativeRoot']
    assert not check_map(base,baseline['runtimeSha256']), 'Immutable baseline changed'
    runtime=out/'runtime'
    record['stage']=stage_subset(base,runtime,baseline['runtimeSha256'])
    # Match the approved f09/e834 runtime explicitly. Unlink every destination
    # before replacement: staged DLLs are hardlinks to preserved runtimes.
    for name in ['libvlccore.dll','plugins/misc/libmedialibrary_plugin.dll']:
        destination=runtime/name
        if destination.exists(): destination.unlink()
        os.link(base/name,destination)
    exe_source=Path(build['exe'].replace('/mnt/c/','C:/'))
    assert digest(exe_source)==build['testSha256']
    exe=runtime/'test_qt_history_worker.exe'
    assert not exe.exists(); os.link(exe_source,exe)
    # Cache generation is owned and isolated; remove an inherited cache before
    # writing a replacement so a hardlinked baseline can never be modified.
    cache_file=runtime/'plugins/plugins.dat'
    if cache_file.exists(): cache_file.unlink()
    assert seconds_left()>180 and storage(repo,80*2**20)['fits']
    record['cacheVerification']=helper.run_owned([runtime/'vlc-cache-gen.exe',runtime/'plugins'],runtime,out/'cache',timeout=30)
    record['forcedCleanupUsed']=record['cacheVerification']['forcedCleanupUsed']
    write()
    assert record['cacheVerification']['normalExit'], 'Owned cache generation failed'
    runtime_expected={name:expected for name,expected in baseline['runtimeSha256'].items()
        if Path(name).name!='plugins.dat' and Path(name).parts[0]!='portable'}
    assert not check_map(runtime,runtime_expected)
    record.update(status='running',runtimeSha256=runtime_expected,testSha256=build['testSha256'],
        buildVerificationSha256=digest(prep/'build/verification.json'),producerSha256=digest(__file__),
        expectedRows=[[name,tag] for name,tag in [('initTestCase',''),
            *[(name,tag) for name in ['busyStateAndCapturedMode','workerFailureAndRetry','deletionDrainsActiveWorker']
              for tag in ['continue','watch-again']],('cleanupTestCase','')]])
    write()
    assert seconds_left()>150
    command=[exe,'-o',str(out/'result.xml')+',xml','-o',str(out/'result.txt')+',txt']
    record['command']=[str(arg) for arg in command]
    record.update(helper.run_owned(command,runtime,out,timeout=min(120,int(seconds_left()-20))))
    text='\n'.join((out/name).read_text(errors='replace') for name in ['stdout.txt','stderr.txt'])
    vlc_log=runtime/'history-worker-native.log'
    if vlc_log.exists(): text+='\n'+vlc_log.read_text(errors='replace')
    import re
    record['moduleCounts']=re.findall(r'plug-ins loaded: (\d+) modules',text)
    record['diagnostics']=[line for line in text.splitlines() if re.search(
        r'Assertion failed|QThread: Destroyed while thread|Binding loop|ReferenceError|TypeError|Cannot assign',line)]
    tree=ET.parse(out/'result.xml').getroot()
    incidents=[dict(name=function.get('name'),dataTag=incident.findtext('DataTag',incident.get('dataTag','')),
        type=incident.get('type'),description=incident.findtext('Description',''))
        for function in tree.findall('TestFunction') for incident in function.findall('Incident')]
    record['incidents']=incidents
    actual=Counter((item['name'],item['dataTag']) for item in incidents)
    expected=Counter(map(tuple,record['expectedRows']))
    record['rawXmlRowsMatch']=actual==expected
    record['allEightIncidentsPass']=len(incidents)==8 and all(item['type']=='pass' for item in incidents)
    record['compilerInputsAfter']=verify_linux('compiler-inputs-after')
    record['runtimeUnchanged']=not check_map(runtime,runtime_expected)
    record['baselineUnchanged']=not check_map(base,baseline['runtimeSha256'])
    record['testUnchanged']=digest(exe)==build['testSha256'] and digest(exe_source)==build['testSha256']
    record['buildVerificationUnchanged']=digest(prep/'build/verification.json')==record['buildVerificationSha256']
    record['producersUnchanged']=not check_map(prep,producers)
    assert record['normalExit'] and not record['forcedCleanupUsed']
    assert record['rawXmlRowsMatch'] and record['allEightIncidentsPass']
    assert record['moduleCounts']==['603'] and not record['diagnostics']
    assert all(value for key,value in record.items() if key.endswith('Unchanged'))
    record['success']=True
except BaseException as error:
    record['success']=False; record['errors'].append(repr(error))
finally:
    record['status']='terminal'; record['ended']=time.time(); record['storageAfter']=storage(repo,0)
    if not record['storageAfter']['fits']:
        record['success']=False; record['errors'].append('C: below owner 20% free floor at finish')
    write()
print(json.dumps({key:value for key,value in record.items() if key!='runtimeSha256'},indent=2))
raise SystemExit(0 if record['success'] else 1)
