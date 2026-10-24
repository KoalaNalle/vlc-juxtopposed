from pathlib import Path
import ast
work=Path(__file__).resolve().parent
repo=work.parent.parent
old=repo/'build-windows/beta-caption-accessibility-20261023'
text=(old/'run-ordinary-ui-r4.py').read_text(encoding='utf-8')
text=text.replace('import ctypes,json,os,re,shutil,subprocess,sys','import argparse,ctypes,json,os,re,shutil,subprocess,sys')
text=text.replace("manifest=work/'inputs-ordinary-r3.json';inputs=read(manifest)['files']", "parser=argparse.ArgumentParser();parser.add_argument('--case',choices=['before','after','ass','legacy'],required=True);args=parser.parse_args()\n+phase='before' if args.case=='before' else 'after'\n+manifest=work/('inputs-'+phase+'-r1.json');inputs=read(manifest)['files']".replace('\n+','\n'))
text=text.replace('assert len(inputs)==2232 and all(', 'assert len(inputs)>=2232 and all(')
text=text.replace("extraMedia=old/'media-generator-smoke-r35/colored-long.mkv'", "extraMedia=old/('media-generator-smoke-r35/ass-overlap.mkv' if args.case=='ass' else 'media-generator-smoke-r35/colored-overlap.mkv')")
text=text.replace("out=work/'ordinary-ui-r4'", "out=work/('ui-'+args.case+'-r1')")
text=text.replace("base=old/'ordinary-ui-r37/runtime'", "base=repo/'build-windows/beta-caption-accessibility-20261023/ordinary-ui-r4/runtime'")
text=text.replace("gui=runtime/'plugins/gui/libqt_plugin.dll';gui.unlink();shutil.copyfile(work/'ordinary-r3-bin/libqt_plugin.dll',gui)", "if phase=='after':\n+    gui=runtime/'plugins/gui/libqt_plugin.dll';gui.unlink();shutil.copyfile(work/'ordinary-r1-bin/libqt_plugin.dll',gui)".replace('\n+','\n'))
text=text.replace('qt-controls-auto-hide=0\\n', 'qt-controls-auto-hide=0\\nqt-pin-controls=0\\n')
text=text.replace("'--start-paused',str(old/'media-generator-smoke-r35/colored-long.mkv'),str(media/'multi-language.mkv')", "'--start-paused','--fullscreen',str(extraMedia)]\n+if args.case=='legacy':command.insert(-1,'--no-qt-juxtopposed-controls')\n+command=command[:-1]+[command[-1]".replace('\n+','\n'))
text=text.replace("sourceManifest='build-windows/beta-caption-accessibility-20261023/inputs-ordinary-r3.json'", "sourceManifest=manifest.relative_to(repo).as_posix()")
text=text.replace("scope='Ordinary Windows11 progress value-setting through desktop accessibility, paused coarse frame colors, keyboard/pointer compatibility and resume. No private GUI, exact frame PTS, screen-reader speech, caption clearance or full-beta acceptance.'", "scope='Ordinary fullscreen redesigned/legacy bottom shading and overlapping SubRip/ASS observations. No caption-placement, exact frame PTS, screen-reader speech or full-beta acceptance.',case=args.case,baselineCompiledManifest='build-windows/beta-caption-accessibility-20261023/inputs-ordinary-r3.json'")
text=text.replace("print('Owned track/subtitle workflow PID',child.pid,flush=True)", "print('Owned caption layout PID',child.pid,flush=True)")
text=text.replace("print(json.dumps(result,indent=2))", "print(json.dumps({k:result[k] for k in ['status','case','pid','exitCode','normalExit','diagnostics','sourceInputsUnchanged','runtimeUnchanged','mediaUnchanged']}))")
ast.parse(text)
assert "--fullscreen" in text and "ordinary-r1-bin" in text
(work/'run-ui-r1.py').write_text(text,encoding='utf-8',newline='\n')
make=(old/'ordinary-accessibility-r3.mk').read_text(encoding='utf-8').replace('owned-ordinary-accessibility-r3','owned-ordinary-caption-layout-r1')
(work/'ordinary-r1.mk').write_text(make,encoding='utf-8',newline='\n')
build=(old/'build-ordinary-accessibility-r3.sh').read_text(encoding='utf-8')
for before,after in [('beta-caption-accessibility-20261023','beta-caption-layout-20261023'),('owned-ordinary-accessibility-r3','owned-ordinary-caption-layout-r1'),('build-ordinary-r3','build-ordinary-r1'),('ordinary-accessibility-r3.mk','ordinary-r1.mk'),('ordinary-r3-bin','ordinary-r1-bin')]:build=build.replace(before,after)
(work/'build-r1.sh').write_text(build,encoding='utf-8',newline='\n')
print('Prepared scoped ordinary GUI runner and build helper')
