from pathlib import Path
import ast
work=Path(__file__).resolve().parent
parent=(work/'run-native-mkv-headers-r30.py').read_text(encoding='utf-8')
start=parent.index("source=read(work/'mkv-headers-r29-source/source-record.json')")
end=parent.index("base=work/'ordinary-ui-r14/runtime'",start)
parent=parent[:start]+"""source=read(work/'inputs-r35.json');inputs=source['files']
assert source['sourceCount']==2205 and all(digest(repo/p)==h for p,h in inputs.items())
assert all(digest(work/'source-r35'/p)==h for p,h in inputs.items())
assert (work/'build-ordinary-mkv-r35-exit.txt').read_text().strip()=='0'
"""+parent[end:]
old="plugins={phase:work/('mkv-headers-'+phase+'-r29-bin/libmkv_plugin.dll') for phase in ['before','after']}"
assert parent.count(old)==1
parent=parent.replace(old,"plugins={'before':base/'plugins/demux/libmkv_plugin.dll','after':work/'ordinary-mkv-r35-bin/libmkv_plugin.dll'}\nassert digest(plugins['before'])=='469751a13e8547291bbfc3bcc28e59cf0bebd7e8f6354c5bb00dfeb7c7cd359b'")
parent=parent.replace("worker=work/'native-mkv-interval-r25.py'", "worker=repo/'test/libvlc/native_paused_subtitle_seek.py'")
parent=parent.replace("root=work/'native-mkv-headers-r30'", "root=work/'native-subtitle-ordinary-r36'")
parent=parent.replace('candidateOnly=True,privateMkv=True', 'candidateOnly=False,privateMkv=False')
parent=parent.replace("sourceRecordSha256=digest(work/'mkv-headers-r29-source/source-record.json')", "sourceRecordSha256=digest(work/'inputs-r35.json')")
old="[sys.executable,str(worker),'--scenario',str(out/'scenario.json'),'--runtime',str(runtime),'--output',str(out)]"
assert parent.count(old)==1
parent=parent.replace(old,"[sys.executable,str(worker),'--scenario',str(out/'scenario.json'),'--runtime',str(runtime),'--output',str(out),'--fixture-root',str(work)]")
last=parent.index("write(root/'comparison.json'")
parent=parent[:last]+"""write(root/'comparison.json',dict(passed=all(r['passed'] and r['normalExit'] and r['runtimeUnchanged'] and r['sourceInputsUnchanged'] and r['producerUnchanged'] and r['fixturesUnchanged'] for r in records if r['phase']=='after'),ordinaryBuild=True,repairAccepted=False,fullBeta=False,control='Unfixed ordinary Matroska DLL versus the standard-rule ordinary source build. Original failures retained; acceptance requires candidate behavioral results and complete identity checks.',allProcessesNormal=all(r['normalExit'] for r in records),records=records))
"""
ast.parse(parent)
assert 'r29' not in parent and 'r23d' not in parent
target=work/'run-native-subtitle-ordinary-r36.py';assert not target.exists()
target.write_text(parent,encoding='utf-8',newline='\n')
