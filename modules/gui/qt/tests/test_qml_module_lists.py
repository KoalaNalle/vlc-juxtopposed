# SPDX-License-Identifier: GPL-2.0-or-later
"""Check a list-only QML addition against the frozen stale-include and current recipes.

Actual production rules/generators drive QRC, qmldir and response metadata. Cache
compiler/RCC/library outputs are explicit metadata probes, not C++/Qt binaries.
GNU make and a POSIX shell are required for execution; prepare-only never passes.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import shlex
import shutil
import subprocess
import sys
import time
import xml.etree.ElementTree as ET
from test_qml_dependencies import production_rule

PROBE = r'''from pathlib import Path
import json,sys,xml.etree.ElementTree as ET
args=sys.argv[1:]
if args[0]=='package':
    members=args[2:]
    qrc=ET.parse('dialogs/res.qrc')
    aliases=[x.attrib['alias'] for x in qrc.findall('.//file')]
    result=dict(scope='metadata dependency closure, not a native library',members=members,
        resourceAliases=aliases,qmldir=Path('dialogs/qmldir').read_text(encoding='utf-8'),
        response=Path('dialogs/qmlcachegen.rsp').read_text(encoding='utf-8'))
    Path(args[1]).write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
else:
    out=Path(args[args.index('-o')+1]);out.parent.mkdir(parents=True,exist_ok=True)
    resource=next((x.split('=',1)[1] for x in args if x.startswith('--resource=')),None)
    if '--resource-name' in args:
        rsp=next(x[1:] for x in args if x.startswith('@'))
        result=dict(kind='cache-loader',response=Path(rsp).read_text(encoding='utf-8'),resource=resource)
    elif '--name' in args:
        inputs=[x for x in args if x.endswith('.qrc')]
        result=dict(kind='resource-compiler',resources=[Path(x).read_text(encoding='utf-8') for x in inputs])
    else:
        source=next((x for x in reversed(args) if x.endswith(('.qml','.js','.mjs'))),None)
        if not source or not resource:
            raise SystemExit('MISSING_CACHE_RESOURCE_ARGUMENT:'+str(source))
        aliases=[x.attrib['alias'] for x in ET.parse(resource).findall('.//file')]
        if Path(source).name not in aliases:
            raise SystemExit('MISSING_MODULE_RESOURCE:'+Path(source).name)
        result=dict(kind='source-cache',input=source,resource=resource)
    out.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
'''


def fixture_makefile(rules, new, shell, python, probe, scripts, broken_stamp=False):
    ordinary=['Old.qml']+(['New.qml','Script.js','Module.mjs'] if new else [])
    singletons=['SharedSingleton.qml'] if new else []
    # This changes the loaded goal graph, so a no-op qml.deps regeneration cannot
    # accidentally hide failure to reparse the remade main Makefile.
    outputs=[Path(p).with_suffix('.cpp').as_posix() for p in ordinary+singletons]
    text=('SHELL = '+shell+'\n'
          'libqml_module_dialogs_a_QML = '+' '.join(ordinary)+'\n'
          'libqml_module_dialogs_a_Singleton_QML = '+' '.join(singletons)+'\n'
          'libqml_module_dialogs_a_Module = VLC.Dialogs\n'
          'libqml_module_dialogs_a_ModulePath = VLC/Dialogs\n'
          'QML_RULES_DEPS = dialogs/qml.deps\n'
          'QML_MODULE_STAMPS = dialogs/qml_modules.stamp\n'
          'QML_MODULE_RES_QRC = dialogs/res.qrc\n'
          'QML_MODULE_QMLDIR = dialogs/qmldir\n'
          'QML_MODULE_QMLDIR_QRC = dialogs/qmldir.qrc\n'
          'QML_MODULE_CACHEGEN_RSP = dialogs/qmlcachegen.rsp\n'
          'QML_MODULE_CACHELOADER = dialogs/qmlcache_loader.cpp\n'
          'QML_MODULE_CACHEGEN_ASSETS = dialogs/qmlassets.cpp\n'
          'builddir = .\nsrcdir = .\nMKDIR_P = mkdir -p\nAM_V_at =\nAM_V_GEN =\n'
          'cachegen_verbose =\nQML_CACHEGEN_EXTRA_ARGS =\nRCC_EXTRA_PARAMS =\n'
          'PYTHON3 = '+python+'\nQMLCACHEGEN = '+python+' '+probe+'\nRCC = $(QMLCACHEGEN)\n'
          '.SUFFIXES: .qml .js .mjs .cpp\n'
          '$(file >>read-trace.log,restart=$(MAKE_RESTARTS) sources=$(libqml_module_dialogs_a_QML) singletons=$(libqml_module_dialogs_a_Singleton_QML))\n'
          'libqt_plugin.la: libqml_module_dialogs.a\n\tcp $< $@\n'
          'libqml_module_dialogs.a: '+' '.join(outputs)+' dialogs/qmlcache_loader.cpp dialogs/qmlassets.cpp\n'
          '\t$(PYTHON3) '+probe+' package $@ $^\n'
          'Makefile: Makefile.next\n\tcp $< $@\n')
    for name,rule in rules.items():
        if name=='stamp' and broken_stamp:
            rule=rule.replace('$(QML_MODULE_STAMPS): Makefile','$(QML_MODULE_STAMPS):')
        text+=rule+'\n'
    text+='-include $(QML_RULES_DEPS)\n'
    return text


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo-root',type=Path,default=Path(__file__).resolve().parents[4])
    parser.add_argument('--output-dir',type=Path,required=True)
    parser.add_argument('--make')
    parser.add_argument('--shell')
    parser.add_argument('--prepare-only',action='store_true')
    args=parser.parse_args();repo=args.repo_root.resolve();out=args.output_dir.resolve()
    out.mkdir(parents=True,exist_ok=False)
    source=repo/'modules/gui/qt/Makefile.am';text=source.read_text(encoding='utf-8')
    targets={'deps':'$(QML_RULES_DEPS): Makefile','stamp':'$(QML_MODULE_STAMPS): Makefile',
        'qrc':'$(QML_MODULE_RES_QRC):','qmldir':'$(QML_MODULE_QMLDIR):',
        'qmldirQrc':'$(QML_MODULE_QMLDIR_QRC):','response':'$(QML_MODULE_CACHEGEN_RSP):',
        'loader':'$(QML_MODULE_CACHELOADER):','assets':'$(QML_MODULE_CACHEGEN_ASSETS):',
        'qml':'.qml.cpp:','js':'.js.cpp:','mjs':'.mjs.cpp:'}
    rules={name:production_rule(text,target) for name,target in targets.items()}
    rules['resource-picker']=next(line for line in text.splitlines() if line.startswith('qml_resource ='))+'\n'
    scripts=repo/'modules/gui/qt/scripts'
    make=None;shell=None;version=None;env=os.environ.copy()
    if not args.prepare_only:
        names=[args.make] if args.make else ['make','gmake','mingw32-make']
        make=next((shutil.which(x) for x in names if x and shutil.which(x)),None)
        shell=shutil.which(args.shell or 'sh')
        if not make or not shell:raise SystemExit('GNU make and POSIX sh/utilities required; --prepare-only is not a passed test')
        check=subprocess.run([make,'--version'],capture_output=True,check=True,timeout=10)
        if b'GNU Make' not in check.stdout:raise SystemExit('GNU make required')
        version=check.stdout.decode('utf-8',errors='replace').splitlines()[0]
        env['PATH']=str(Path(shell).parent)+os.pathsep+env.get('PATH','')
        subprocess.run([shell,'-c','for tool in cmp cp mv rm mkdir; do command -v "$tool" >/dev/null || exit 1; done'],
            env=env,capture_output=True,check=True,timeout=10)
    shellname=Path(shell).name if shell else ('sh.exe' if os.name=='nt' else 'sh')
    python=shlex.quote(Path(sys.executable).as_posix())
    baseline_path=Path(__file__).resolve().parent/'fixtures/qml_dependencies_stable_cmp_recipe.inc'
    baseline_recipe=baseline_path.read_text(encoding='utf-8')
    records=[]
    for mode,broken in [('baseline-stable-cmp',False),('fault-no-stamp-input',True),('current-production-warm',False)]:
        case_rules={**rules,'deps':baseline_recipe} if mode=='baseline-stable-cmp' else rules
        case=out/mode;(case/'dialogs').mkdir(parents=True);(case/'scripts').mkdir()
        for name in ['genqrc.py','genqmldir.py','qrc2rsp.py']:shutil.copyfile(scripts/name,case/'scripts'/name)
        (case/'probe.py').write_text(PROBE,encoding='utf-8',newline='\n')
        for name in ['Old.qml','New.qml','Script.js','Module.mjs','SharedSingleton.qml']:
            (case/name).write_text(('pragma Singleton\n' if name=='SharedSingleton.qml' else '')+'// owned fixture only\n',encoding='utf-8',newline='\n')
        old=fixture_makefile(case_rules,False,shellname,python,'probe.py',scripts,broken)
        new=fixture_makefile(case_rules,True,shellname,python,'probe.py',scripts,broken)
        (case/'Makefile').write_text(old,encoding='utf-8',newline='\n')
        (case/'Makefile.next').write_text(old,encoding='utf-8',newline='\n')
        now=time.time();os.utime(case/'Makefile',(now-20,now-20));os.utime(case/'Makefile.next',(now-40,now-40))
        (case/'Makefile.next-after-warm').write_text(new,encoding='utf-8',newline='\n')
        if args.prepare_only:
            records.append(dict(mode=mode,prepared=True,makeRun=False,warmRun=False));continue
        command=[make,'--no-print-directory','-rR','-j8','-f','Makefile','libqt_plugin.la']
        warm=subprocess.run(command,cwd=case,env=env,capture_output=True,timeout=30)
        (case/'warm-stdout.txt').write_bytes(warm.stdout);(case/'warm-stderr.txt').write_bytes(warm.stderr)
        if warm.returncode:raise AssertionError((mode,'warm setup failed',warm.returncode,warm.stderr.decode('utf-8',errors='replace')))
        warmReceipt=json.loads((case/'libqt_plugin.la').read_text(encoding='utf-8'))
        if warmReceipt['resourceAliases']!=['Old.qml']:raise AssertionError(('warm actual resource generation',warmReceipt))
        now=time.time()
        for path in case.rglob('*'):
            if path.is_file():os.utime(path,(now-80,now-80))
        os.utime(case/'Makefile',(now-100,now-100))
        (case/'Makefile.next').write_text(new,encoding='utf-8',newline='\n');os.utime(case/'Makefile.next',(now-40,now-40))
        dependency=(case/'dialogs/qml.deps').read_bytes();dependencyTime=(case/'dialogs/qml.deps').stat().st_mtime_ns
        (case/'warm-qml.deps').write_bytes(dependency)
        result=subprocess.run(command,cwd=case,env=env,capture_output=True,timeout=30)
        (case/'stdout.txt').write_bytes(result.stdout);(case/'stderr.txt').write_bytes(result.stderr)
        sameBytes=(case/'dialogs/qml.deps').read_bytes()==dependency
        sameMtime=(case/'dialogs/qml.deps').stat().st_mtime_ns==dependencyTime
        receipt=json.loads((case/'libqt_plugin.la').read_text(encoding='utf-8')) if result.returncode==0 else None
        required={'Old.qml','New.qml','Script.js','Module.mjs','SharedSingleton.qml'}
        # Exactly the same actual resource/export/response/member closure oracle
        # applies to the frozen old and corrected recipes, including exit-zero
        # stale metadata being a failure of the desired outcome.
        desiredClosure=(result.returncode==0 and receipt is not None and set(receipt['resourceAliases'])==required
            and all(Path(x).with_suffix('.cpp').as_posix() in receipt['members'] for x in required)
            and all('/'+x in receipt['response'] for x in required)
            and all(Path(x).stem+' 1.0 '+x in receipt['qmldir'] for x in required))
        if mode=='baseline-stable-cmp':
            expectedControl=(not desiredClosure and result.returncode==0 and sameBytes and sameMtime
                and receipt is not None and receipt['resourceAliases']==['Old.qml']
                and 'New.cpp' not in receipt['members'] and 'New 1.0 New.qml' not in receipt['qmldir'])
        elif broken:
            expectedControl=(not desiredClosure and result.returncode!=0 and sameBytes and not sameMtime
                and any(('MISSING_MODULE_RESOURCE:'+name).encode() in result.stderr for name in required-{'Old.qml'}))
        else:
            expectedControl=desiredClosure and sameBytes and not sameMtime
        records.append(dict(mode=mode,makeRun=True,warmRun=True,warmExitCode=warm.returncode,exitCode=result.returncode,
            includedDependencyBytesUnchanged=sameBytes,includedDependencyMtimeUnchanged=sameMtime,
            desiredClosure=desiredClosure,expectedPairControl=expectedControl,packageMetadata=receipt,
            readTrace=(case/'read-trace.log').read_text(encoding='utf-8')))
    report=dict(scope=__doc__,makeRun=not args.prepare_only,makeVersion=version,cases=records,
        passed=None if args.prepare_only else all(row['expectedPairControl'] for row in records),
        productionRuleTargets=targets,sourceSha256={path.relative_to(repo).as_posix():hashlib.sha256(path.read_bytes()).hexdigest()
            for path in [source,scripts/'genqrc.py',scripts/'genqmldir.py',scripts/'qrc2rsp.py']},
        baselineRecipeSha256=hashlib.sha256(baseline_path.read_bytes()).hexdigest(),
        nativeCompilation=False,nativeArchive=False,exactNativeFailureReproduced=False)
    (out/'verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,indent=2))
    return 0 if args.prepare_only or report['passed'] else 1

if __name__=='__main__':raise SystemExit(main())
