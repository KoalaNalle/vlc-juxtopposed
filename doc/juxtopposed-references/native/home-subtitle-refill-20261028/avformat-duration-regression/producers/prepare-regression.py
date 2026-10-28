"""Freeze new runtime inputs only after the duration plugin has qualified; never run VLC."""
from pathlib import Path
import argparse,ast,hashlib,json,sys
sys.dont_write_bytecode=True
from runtime_support import manifest
ROOT=Path(__file__).resolve().parent
REPO=Path(r'C:\Modding\vlc-juxtopposed')
TAIL=ROOT.parent.parent
parser=argparse.ArgumentParser();parser.add_argument('--freeze-after-qualified-build',action='store_true',required=True);parser.parse_args()
def digest(p):
    h=hashlib.sha256()
    with Path(p).open('rb') as f:
        for b in iter(lambda:f.read(1048576),b''):h.update(b)
    return h.hexdigest()
def load(p):return json.loads(Path(p).read_text(encoding='utf-8-sig'))
assert not (ROOT/'regression-config.json').exists(),'Never replace frozen configuration'
rows=load(ROOT.parent/'source-identities.json')
assert isinstance(rows,list) and len(rows)==4
sources={row['path']:row['sha256'] for row in rows};assert len(sources)==4
assert sources['modules/demux/avformat/mux.c']=='f2a90481441d369949b4132e0ef67614ab462a5b41f61d3542ca2d952a3c372c'
assert all(digest(REPO/p)==h for p,h in sources.items())
buildpath=ROOT.parent/'native-qualification-r2/build/verification.json'
assert buildpath.is_file(),'Duration plugin is not qualified yet; preparation must stay deferred'
build=load(buildpath);assert build['status']=='terminal' and build['success'] and build['exitCode']==0
guards=['sourceUnchanged','buildInputGuards','dependencyInputGuards','producerUnchanged','originalBackupsUnchanged',
    'untouchedBuildBinariesUnchanged','writerLockReleased','actualCompileObserved','actualLinkObserved','writerCleanupConfirmed','sharedCandidatePluginUnchanged']
assert all(build[k] is True for k in guards)
assert not build['cleanupErrors'] and not build['restoreErrors'] and not build['verificationErrors']
assert not build['forcedBuildCleanupUsed'] and not build['remainingOwnedWriterPids']
assert build['priorPluginBackupPolicy']=='verified-immutable-C-hardlink'
plugin=buildpath.parent/'libavcodec_plugin.dll';assert digest(plugin)==build['pluginSha256']
assert build['pluginSha256']!='79a565a9398756aaf22ceb194771a9599c499771971638bfdfb06e9fc531882f'
old=load(TAIL/'runtime-config.json');short=load(TAIL/'writer-smoke-r2/smoke-config.json')
unitpath=TAIL/'native-qualification-r1/unit/verification.json';unit=load(unitpath)
unitguards=old['requiredUnitGuards']
assert unit['success'] and unit['normalExit'] and unit['cases']==41 and unit['checks']==5772 and unit['failures']==0
assert not unit['inspectionErrors'] and all(unit[k] for k in unitguards)
assert build['unitVerificationSha256']==digest(unitpath)
assert all(unit['sourceSha256'][p]==sources[p] for p in ('modules/demux/avformat/avformat_mux_queue.h','test/modules/demux/avformat_mux_queue.c'))
assert manifest(REPO/old['preservedRun']/'runtime')==old['preservedRuntimeInventory']
cases={'original':{'fixture':old['fixture'],'fixtureSha256':old['fixtureSha256'],'expectedAvDeletionOrder':['h264','flac']},
    'reversed':dict(old['alternateFixtures']['reversed'],expectedAvDeletionOrder=['flac','h264'])}
assert all(digest(REPO/c['fixture'])==c['fixtureSha256'] for c in cases.values())
assert all(digest(REPO/c['fixture'])==c['fixtureSha256'] for c in short['cases'])
for p in ROOT.glob('*.py'):ast.parse(p.read_text(),filename=str(p))
relative=lambda p:p.relative_to(REPO).as_posix()
cfg={k:old[k] for k in ('preservedRun','preservedRuntimeInventory','runtimeHelperSha256','runtimeStorageBudget','controlPluginSha256')}
cfg.update(status='prepared-not-executed',sourceSha256=sources,sourceIdentityInputShape='four-row array normalized to unique path/hash mapping',
    longCases=cases,shortCases={c['name']:c for c in short['cases']},shortStorageBudget=short['storageBudget'],
    toolsSha256=short['toolsSha256'],distro=short['distro'],outputMaxBytes=short['outputMaxBytes'],runTimeoutSeconds=30,
    stopByUnix=short['stopByUnix'],priorRuntimeConfig=relative(TAIL/'runtime-config.json'),priorRuntimeConfigSha256=digest(TAIL/'runtime-config.json'),
    buildReceipt=relative(buildpath),buildReceiptSha256=digest(buildpath),requiredBuildGuards=guards,
    unitReceipt=relative(unitpath),unitReceiptSha256=digest(unitpath),requiredUnitGuards=unitguards,
    unitReuseScope=build['unitReuseScope'],candidatePlugin=relative(plugin),candidatePluginSha256=digest(plugin))
producers={p.name:digest(p) for p in ROOT.iterdir() if p.is_file() and p.name in
    ('prepare-regression.py','run-long.py','analyze-long.py','run-short.py','runtime_support.py','adapt-producers.py','PREPARATION.md')}
assert len(producers)==7
cfg['producerSha256']=cfg['producers']=producers
(ROOT/'regression-config.json').write_text(json.dumps(cfg,indent=2)+'\n')
print(json.dumps(dict(status=cfg['status'],configSha256=digest(ROOT/'regression-config.json'),
    candidatePluginSha256=cfg['candidatePluginSha256'],sourceFiles=len(sources),longCases=2,independentShortCases=3,
    longPeakBudget=cfg['runtimeStorageBudget']['peakAdditionalBytes'],shortPeakBudget=cfg['shortStorageBudget']['peakAdditionalBytes'],
    noApplicationOrCompilerExecuted=True),indent=2))
