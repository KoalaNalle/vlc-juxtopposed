from pathlib import Path
import bisect,json,re,struct,subprocess
work=Path(__file__).resolve().parent;out=work/'ordinary-resource-B1-r45'
gui=out/'runtime/plugins/gui/libqt_plugin.dll';data=gui.read_bytes();pe=struct.unpack_from('<I',data,0x3c)[0];opt=pe+24
assert struct.unpack_from('<H',data,opt)[0]==0x20b
base=struct.unpack_from('<Q',data,opt+24)[0]
symbolFile='/mnt/c/Modding/vlc-juxtopposed/build-windows/beta-timeline-thumbnails-20261021/ordinary-resource-B1-r45/runtime/plugins/gui/libqt_plugin.dll'
r=subprocess.run(['wsl','-d','VLC-Juxtopposed-Home-20261018','--','x86_64-w64-mingw32-nm','-n','-C',symbolFile],capture_output=True,text=True,check=True)
symbols=[]
for line in r.stdout.splitlines():
    m=re.match(r'([0-9a-fA-F]+)\s+[Tt]\s+(.*)',line)
    if m:symbols.append((int(m[1],16),m[2]))
symbols.sort();addresses=[a for a,n in symbols]
s=(out/'shutdown-stack.txt').read_text(errors='replace')
resolved={}
for value in set(re.findall(r'libqt_plugin\+0x([0-9a-fA-F]+)',s)):
    address=base+int(value,16);i=bisect.bisect_right(addresses,address)-1
    assert i>=0
    resolved[value]=dict(function=symbols[i][1],offset=hex(address-symbols[i][0]))
for value in sorted(resolved,key=lambda v:int(v,16)):
    s=s.replace('libqt_plugin+0x'+value+'\n','libqt_plugin!'+resolved[value]['function']+'+'+resolved[value]['offset']+'\n')
(out/'shutdown-stack-resolved.txt').write_text(s,encoding='utf-8')
(out/'shutdown-symbols.json').write_text(json.dumps(dict(preferredImageBase=hex(base),symbols=resolved),indent=2)+'\n',encoding='utf-8')
print('Resolved',len(resolved),'Qt offsets against exact selected DLL symbol table')
for value in ['804c27','4b53e9','c7deee','c7de7d','148a948','4e75d7','c7d154','1dba4c2','15cf8b7','3bed28','438e44','43c816','4350f2','332139']:
    print(value,resolved.get(value))
