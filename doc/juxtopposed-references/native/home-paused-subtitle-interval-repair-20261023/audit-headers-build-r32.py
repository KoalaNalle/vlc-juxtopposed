from pathlib import Path
from hashlib import sha256
import json,re
work=Path(__file__).resolve().parent
digest=lambda p:sha256(p.read_bytes()).hexdigest()
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
record=read(work/'mkv-headers-r29-source/source-record.json')
assert (work/'build-mkv-headers-r29-exit.txt').read_text().strip()=='0'
names='util virtual_segment matroska_segment matroska_segment_parse matroska_segment_seeker demux events Ebml_parser chapters chapter_command chapter_command_dvd chapter_command_script chapter_command_script_common stream_io_callback lzokay mkv'.split()
log=(work/'build-mkv-headers-r29.log').read_text(encoding='utf-8',errors='replace')
links=[l for l in log.splitlines() if '--mode=link' in l and 'libmkv_plugin.la' in l and 'owned-mkv-headers-' in l]
compiles=[l for l in log.splitlines() if '--mode=compile' in l and 'owned-mkv-headers-' in l]
assert len(links)==2 and len(compiles)==32
phases={}
for phase in ['before','after']:
    root=work/('mkv-headers-'+phase+'-r29-bin')
    deps=sorted(root.glob('*.d'))
    assert len(deps)==16 and {p.stem for p in deps}==set(names)
    object_hashes={}
    for line in (root/'object-hashes.txt').read_text(encoding='utf-8-sig').splitlines():
        match=re.fullmatch(r'([a-f0-9]{64})\s+(.+)',line);assert match,line
        object_hashes[match.group(2)]=match.group(1)
    assert len(object_hashes)==16
    expected='/mnt/c/Modding/vlc-juxtopposed/build-windows/beta-tracks-subtitles-20261023/mkv-headers-r29-source/'+phase+'/'
    rows=[]
    for dep in deps:
        text=dep.read_text(encoding='utf-8').replace('\\\n',' ')
        assert expected+dep.stem+'.cpp' in text
        headers=[p for p in text.split() if p.endswith(('matroska_segment.hpp','matroska_segment_seeker.hpp'))]
        assert all(p.startswith(expected) for p in headers),(phase,dep.name,headers)
        object_path='/root/vlc-juxtopposed-build/owned-mkv-headers-'+phase+'-r29/.libs/libmkv_plugin_la-'+dep.stem+'.o'
        assert object_path in object_hashes
        command=next(l for l in compiles if '/owned-mkv-headers-'+phase+'-r29/libmkv_plugin_la-'+dep.stem+'.lo' in l)
        assert expected+dep.stem+'.cpp' in command
        rows.append(dict(unit=dep.stem,dependencySha256=digest(dep),privateClassHeaders=headers,
            objectSha256=object_hashes[object_path],recordedCompileCommandSha256=sha256(command.encode()).hexdigest(),
            objectBasename='libmkv_plugin_la-'+dep.stem+'.lo',derivedModuleString='mkv'))
    link=next(l for l in links if 'owned-mkv-headers-'+phase+'-r29/' in l)
    for name in names:
        assert '/owned-mkv-headers-'+phase+'-r29/libmkv_plugin_la-'+name+'.lo' in link
        assert 'demux/mkv/libmkv_plugin_la-'+name+'.lo' not in link
    assert 'demux/mkv/libmkv_plugin_la-vlc_colors.lo' in link and 'packetizer/libmkv_plugin_la-dts_header.lo' in link
    phases[phase]=dict(cppUnitsRebuilt=16,unchangedCObjects=2,rows=rows,dllSha256=digest(root/'libmkv_plugin.dll'),
        recordedDoltlibtoolLinkCommandSha256=sha256(link.encode()).hexdigest())
assert all(digest(work/'mkv-headers-r29-source'/phase/name)==h for phase,files in record['privateSources'].items() for name,h in files.items())
output=dict(passed=True,sourceRecordSha256=digest(work/'mkv-headers-r29-source/source-record.json'),phases=phases,
    scope='All sixteen C++ objects rebuilt and linked with matching private class headers in each phase. Ordinary-style libmkv_plugin_la names derive MODULE_STRING=mkv under modules/common.am. Only the two unchanged C objects are shared. Production source/runtime are unchanged; no ordinary GUI acceptance.')
(work/'headers-build-audit-r32.json').write_text(json.dumps(output,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(dict(passed=True,phases={p:dict(cppUnitsRebuilt=r['cppUnitsRebuilt'],unchangedCObjects=r['unchangedCObjects'],dllSha256=r['dllSha256']) for p,r in phases.items()})))
