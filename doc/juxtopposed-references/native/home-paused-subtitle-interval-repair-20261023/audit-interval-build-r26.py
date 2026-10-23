from pathlib import Path
from hashlib import sha256
import json,re
work=Path(__file__).resolve().parent
digest=lambda p:sha256(p.read_bytes()).hexdigest()
record=json.loads((work/'mkv-interval-r23d-source/source-record.json').read_text(encoding='utf-8-sig'))
assert (work/'build-mkv-interval-r23d-exit.txt').read_text().strip()=='0'
names='util virtual_segment matroska_segment matroska_segment_parse matroska_segment_seeker demux events Ebml_parser chapters chapter_command chapter_command_dvd chapter_command_script chapter_command_script_common stream_io_callback lzokay mkv'.split()
log=(work/'build-mkv-interval-r23d.log').read_text(encoding='utf-8',errors='replace')
links=[l for l in log.splitlines() if l.startswith('libtool: link:') and '-shared' in l and 'libmkv_plugin.dll' in l]
assert len(links)==2
phases={}
for phase in ['before','after']:
    root=work/('mkv-interval-'+phase+'-r23d-bin');deps=sorted(root.glob('*.d'))
    assert len(deps)==16 and {p.stem for p in deps}==set(names)
    expected='/mnt/c/Modding/vlc-juxtopposed/build-windows/beta-tracks-subtitles-20261023/mkv-interval-r23d-source/'+phase+'/'
    rows=[]
    for dep in deps:
        text=dep.read_text(encoding='utf-8').replace('\\\n',' ')
        assert expected+dep.stem+'.cpp' in text
        class_headers=[p for p in text.split() if p.endswith(('matroska_segment.hpp','matroska_segment_seeker.hpp'))]
        assert all(p.startswith(expected) for p in class_headers),(phase,dep.name,class_headers)
        rows.append(dict(unit=dep.stem,dependencySha256=digest(dep),privateClassHeaders=class_headers))
    link=next(l for l in links if 'owned-mkv-interval-'+phase+'-r23d/' in l)
    for name in names:
        assert '/owned-mkv-interval-'+phase+'-r23d/.libs/'+name+'.o' in link
        assert 'demux/mkv/.libs/libmkv_plugin_la-'+name+'.o' not in link
    assert 'demux/mkv/.libs/libmkv_plugin_la-vlc_colors.o' in link and 'packetizer/.libs/libmkv_plugin_la-dts_header.o' in link
    phases[phase]=dict(cppUnitsRebuilt=16,unchangedCObjects=2,rows=rows,dllSha256=digest(root/'libmkv_plugin.dll'),linkLineSha256=sha256(link.encode()).hexdigest())
assert all(digest(work/'mkv-interval-r23d-source'/phase/name)==h for phase,files in record['privateSources'].items() for name,h in files.items())
output=dict(passed=True,sourceRecordSha256=digest(work/'mkv-interval-r23d-source/source-record.json'),phases=phases,scope='All sixteen private C++ units and any class-layout header dependencies use their matching phase; two unchanged C objects remain shared. Compiler/configured dependency records are retained; no ordinary runtime promotion.')
(work/'interval-build-audit-r26.json').write_text(json.dumps(output,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(dict(passed=True,phases={p:dict(cppUnitsRebuilt=r['cppUnitsRebuilt'],unchangedCObjects=r['unchangedCObjects'],dllSha256=r['dllSha256']) for p,r in phases.items()}),indent=2))
