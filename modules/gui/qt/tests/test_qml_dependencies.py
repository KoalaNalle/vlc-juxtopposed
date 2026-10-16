# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise actual deferred QML resource dependencies through GNU make remake.

No Qt runtime, C++ compiler or native VLC build is needed. The historical eager
recipe must fail meaningfully; the current production recipe must supply the
resource prerequisite to added QML, JS, MJS and singleton compilation targets.
Windows needs GNU make and a POSIX shell/utilities (for example Git or MSYS2).
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


def production_rule(text, target):
    lines = text.splitlines(True)
    indices = [i for i, line in enumerate(lines) if line.rstrip('\r\n') == target]
    if len(indices) != 1:
        raise AssertionError(('Expected one actual production make rule', target, len(indices)))
    start = indices[0]
    end = start + 1
    while end < len(lines) and (lines[end].startswith('\t') or not lines[end].strip()):
        end += 1
    return ''.join(lines[start:end]).rstrip() + '\n'


def makefile(recipe, stamp, generator, new, shell):
    sources = ['Old.qml'] + (['New.qml', 'Script.js', 'Module.mjs'] if new else [])
    singletons = ['SharedSingleton.qml'] if new else []
    outputs = [Path(name).with_suffix('.cpp').as_posix() for name in sources + singletons]
    text = ('SHELL = ' + shell + '\n'
        'libqml_module_dialogs_a_QML = ' + ' '.join(sources) + '\n'
        'libqml_module_dialogs_a_Singleton_QML = ' + ' '.join(singletons) + '\n'
        'libqml_module_dialogs_a_Module = VLC.Dialogs\n'
        'libqml_module_dialogs_a_ModulePath = VLC/Dialogs\n'
        'QML_RULES_DEPS = dialogs/qml.deps\n'
        'QML_MODULE_STAMPS = dialogs/qml_modules.stamp\n'
        'builddir = .\nMKDIR_P = mkdir -p\nAM_V_at =\nAM_V_GEN =\n'
        '.SUFFIXES: .qml .js .mjs .cpp\n'
        'all: ' + ' '.join(outputs) + '\n'
        'Makefile: Makefile.next\n\tcp $< $@\n'
        + recipe + '\n' + stamp + '\n'
        'dialogs/res.qrc:\n\t'
        + shlex.quote(Path(sys.executable).as_posix()) + ' ' + shlex.quote(generator.as_posix())
        + ' -o $@ -s . --prefix qt/qml/VLC/Dialogs --sources '
        '$(libqml_module_dialogs_a_QML) $(libqml_module_dialogs_a_Singleton_QML)\n'
        '-include $(QML_RULES_DEPS)\n')
    for suffix in ['qml', 'js', 'mjs']:
        text += ('.' + suffix + '.cpp:\n'
            '\t@case "$^" in *res.qrc*) printf \'%s\\n\' "$^" > $@;; '
            '*) echo MISSING_RESOURCE:$@; exit 13;; esac\n')
    return text


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo-root', type=Path, default=Path(__file__).resolve().parents[4])
    parser.add_argument('--output-dir', type=Path, required=True)
    parser.add_argument('--make', help='GNU make executable; auto-detect make/gmake/mingw32-make otherwise')
    parser.add_argument('--shell', help='POSIX sh executable; auto-detect otherwise')
    parser.add_argument('--prepare-only', action='store_true', help='Emit fixtures without claiming a make result')
    args = parser.parse_args()
    repo = args.repo_root.resolve()
    output = args.output_dir.resolve()
    output.mkdir(parents=True, exist_ok=False)
    source = repo/'modules/gui/qt/Makefile.am'
    text = source.read_text(encoding='utf-8')
    current = production_rule(text, '$(QML_RULES_DEPS): Makefile')
    stamp = production_rule(text, '$(QML_MODULE_STAMPS): Makefile')
    eager_path = Path(__file__).resolve().parent/'fixtures/qml_dependencies_eager_recipe.inc'
    eager = eager_path.read_text(encoding='utf-8')
    generator = repo/'modules/gui/qt/scripts/genqrc.py'
    environment = os.environ.copy()
    make, shell, make_version = None, None, None
    if not args.prepare_only:
        candidates = [args.make] if args.make else ['make', 'gmake', 'mingw32-make']
        make = next((shutil.which(name) for name in candidates if name and shutil.which(name)), None)
        shell = shutil.which(args.shell or 'sh')
        if not make or not shell:
            raise SystemExit('GNU make and POSIX sh/utilities are required; preparation alone is --prepare-only, not a passing regression test')
        version = subprocess.run([make,'--version'],capture_output=True,timeout=10,check=True)
        if b'GNU Make' not in version.stdout:
            raise SystemExit('This regression requires GNU make; nmake/BSD make is not equivalent')
        make_version = version.stdout.decode('utf-8',errors='replace').splitlines()[0]
        environment['PATH'] = str(Path(shell).parent) + os.pathsep + environment.get('PATH','')
        subprocess.run([shell,'-c','for tool in cmp cp mv rm mkdir; do command -v "$tool" >/dev/null || exit 1; done'],env=environment,
                       capture_output=True,timeout=10,check=True)
    shell_name = Path(shell).name if shell else ('sh.exe' if os.name == 'nt' else 'sh')
    records = []
    for mode, recipe in [('historical-eager',eager),('current-production',current)]:
        case = output/mode
        (case/'dialogs').mkdir(parents=True)
        files = {'Old.qml':'// Old input\n', 'New.qml':'// Added QML input\n',
                 'Script.js':'// Added JS input\n', 'Module.mjs':'// Added MJS input\n',
                 'SharedSingleton.qml':'pragma Singleton\n// Added singleton input\n',
                 'Makefile':makefile(recipe,stamp,generator,False,shell_name),
                 'Makefile.next':makefile(recipe,stamp,generator,True,shell_name),
                 'dialogs/qml_modules.stamp':'VLC.Dialogs VLC/Dialogs Old.qml \n',
                 'dialogs/qml.deps':'Old.cpp: ./dialogs/res.qrc\n'}
        for name, content in files.items():
            (case/name).write_text(content,encoding='utf-8',newline='\n')
        subprocess.run([sys.executable,str(generator),'-o',str(case/'dialogs/res.qrc'),'-s',str(case),
                        '--prefix','qt/qml/VLC/Dialogs','--sources','Old.qml'],capture_output=True,timeout=10,check=True)
        now = time.time()
        for path in case.rglob('*'):
            if path.is_file(): os.utime(path,(now-60,now-60))
        os.utime(case/'Makefile',(now-40,now-40))
        os.utime(case/'Makefile.next',(now-20,now-20))
        if args.prepare_only:
            records.append(dict(mode=mode,prepared=True,makeRun=False))
            continue
        result = subprocess.run([make,'--no-print-directory','-rR','-f','Makefile','all'],cwd=case,
                                env=environment,capture_output=True,timeout=30)
        (case/'stdout.txt').write_bytes(result.stdout)
        (case/'stderr.txt').write_bytes(result.stderr)
        deps = (case/'dialogs/qml.deps').read_text(encoding='utf-8')
        prerequisites = {name:((case/name).read_text(encoding='utf-8') if (case/name).exists() else None)
            for name in ['Old.cpp','New.cpp','Script.cpp','Module.cpp','SharedSingleton.cpp']}
        aliases = [node.attrib['alias'] for node in ET.parse(case/'dialogs/res.qrc').findall('.//file')]
        resource_updated = set(aliases) == {'Old.qml','New.qml','Script.js','Module.mjs','SharedSingleton.qml'}
        if mode == 'historical-eager':
            oracle = (result.returncode != 0 and b'MISSING_RESOURCE:New.cpp' in result.stdout
                      and 'New.cpp:' not in deps and resource_updated)
        else:
            oracle = (result.returncode == 0 and resource_updated and all(
                value is not None and 'res.qrc' in value for value in prerequisites.values()))
        records.append(dict(mode=mode,makeRun=True,exitCode=result.returncode,desiredOracle=oracle,
            resourceAliases=aliases,generatedDependencyText=deps,cppPrerequisites=prerequisites,
            stampText=(case/'dialogs/qml_modules.stamp').read_text(encoding='utf-8')))
    report = dict(scope=__doc__,makeRun=not args.prepare_only,makeVersion=make_version,
                  cases=records,passed=None if args.prepare_only else all(row['desiredOracle'] for row in records),
                  sourceSha256={str(path.relative_to(repo)):hashlib.sha256(path.read_bytes()).hexdigest()
                                for path in [source,generator]},nativeCompilation=False)
    (output/'verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,indent=2))
    return 0 if args.prepare_only or report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
