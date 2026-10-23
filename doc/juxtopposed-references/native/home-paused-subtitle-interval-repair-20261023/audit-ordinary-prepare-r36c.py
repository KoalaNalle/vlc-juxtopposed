from pathlib import Path
from hashlib import sha256
import ast,json,posixpath,re
work=Path(__file__).resolve().parent;repo=work.parent.parent
digest=lambda p:sha256(p.read_bytes()).hexdigest()
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
assert (work/'finalize-ordinary-build-r35c-exit.txt').read_text().strip()=='0'
source=read(work/'inputs-r35c.json');assert source['sourceCount']==2210
assert all(digest(repo/p)==h and digest(work/'source-r35c'/p)==h for p,h in source['files'].items())
root=work/'ordinary-mkv-r35c-bin'
log=(work/'build-ordinary-mkv-r35b.log').read_text(encoding='utf-8',errors='replace')
compiles=[l for l in log.splitlines() if '--mode=compile' in l]
assert len(compiles)==16
links=[l for l in log.splitlines() if '--mode=link' in l and 'libmkv_plugin.la' in l]
assert len(links)==1
names='util virtual_segment matroska_segment matroska_segment_parse matroska_segment_seeker demux events Ebml_parser chapters chapter_command chapter_command_dvd chapter_command_script chapter_command_script_common stream_io_callback lzokay mkv'.split()
object_hashes={}
for line in (root/'object-hashes.txt').read_text(encoding='utf-8').splitlines():
    match=re.fullmatch(r'([a-f0-9]{64})\s+(.+)',line);assert match,line
    object_hashes[match.group(2)]=match.group(1)
assert len(object_hashes)==18
rows=[]
for name in names:
    dep=root/('libmkv_plugin_la-'+name+'.Plo')
    text=dep.read_text(encoding='utf-8').replace('\\\n',' ')
    paths=[posixpath.normpath(p.rstrip(':')) for p in text.split()]
    expected='/mnt/c/Modding/vlc-juxtopposed/modules/demux/mkv/'
    assert expected+name+'.cpp' in paths
    headers=sorted({p for p in paths if p.endswith(('matroska_segment.hpp','matroska_segment_seeker.hpp'))})
    assert all(p.startswith(expected) for p in headers)
    obj='/root/vlc-juxtopposed-build/win64/modules/demux/mkv/.libs/libmkv_plugin_la-'+name+'.o'
    assert obj in object_hashes
    command=next(l for l in compiles if '-o demux/mkv/libmkv_plugin_la-'+name+'.lo' in l)
    assert 'demux/mkv/libmkv_plugin_la-'+name+'.lo' in links[0]
    rows.append(dict(unit=name,dependencySha256=digest(dep),classHeaders=headers,objectSha256=object_hashes[obj],compileCommandSha256=sha256(command.encode()).hexdigest()))
for name,h in read(work/'source-application-r34.json')['productionFiles'].items():
    assert digest(repo/'modules/demux/mkv'/name)==h
audit=dict(passed=True,sourceCount=2210,sourceManifestSha256=digest(work/'inputs-r35c.json'),cppUnitsRebuilt=16,unchangedCObjects=2,
    rows=rows,objectSha256=object_hashes,dllSha256=digest(root/'libmkv_plugin.dll'),linkCommandSha256=sha256(links[0].encode()).hexdigest(),
    scope='Configured ordinary rules, standard object paths and module labels; all sixteen C++ units rebuilt. Additional dependency flags do not alter code; configured final -MF emitted standard real .Plo files. Initial stale-object linker failure and post-link provenance-copy failure retained. Final make -q succeeded and collection completed without another recompile.')
(work/'ordinary-mkv-build-audit-r36c.json').write_text(json.dumps(audit,indent=2)+'\n',encoding='utf-8',newline='\n')
parent=(work/'run-native-subtitle-ordinary-r36b.py').read_text(encoding='utf-8')
parent=parent.replace('inputs-r35b','inputs-r35c').replace('source-r35b','source-r35c')
parent=parent.replace("source['sourceCount']==2208","source['sourceCount']==2210")
parent=parent.replace('build-ordinary-mkv-r35b-exit','finalize-ordinary-build-r35c-exit')
parent=parent.replace('ordinary-mkv-r35b-bin','ordinary-mkv-r35c-bin')
parent=parent.replace("root=work/'native-subtitle-ordinary-r36b'", "root=work/'native-subtitle-ordinary-r36c'")
ast.parse(parent)
target=work/'run-native-subtitle-ordinary-r36c.py';assert not target.exists()
target.write_text(parent,encoding='utf-8',newline='\n')
print(json.dumps(dict(passed=True,cppUnitsRebuilt=16,unchangedCObjects=2,dllSha256=audit['dllSha256'])))
