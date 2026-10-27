"""Cache the public data symbols matching the installed ntdll; install nothing."""
from pathlib import Path
import hashlib,json,struct,uuid,urllib.request

work=Path(__file__).resolve().parent;out=work/'os-symbols-r71';out.mkdir(exist_ok=True)
binary=Path('C:/Windows/System32/ntdll.dll');data=binary.read_bytes()
u16=lambda n:struct.unpack_from('<H',data,n)[0]
u32=lambda n:struct.unpack_from('<I',data,n)[0]
pe=u32(0x3c);assert data[pe:pe+4]==b'PE\0\0'
optional=pe+24;assert u16(optional)==0x20b
sections=optional+u16(pe+20)
def raw(rva):
    for i in range(u16(pe+6)):
        s=sections+40*i;virtual=u32(s+12)
        if virtual<=rva<virtual+max(u32(s+8),u32(s+16)):return u32(s+20)+rva-virtual
    raise ValueError('Debug RVA is outside file sections')
rva=u32(optional+112+6*8);length=u32(optional+112+6*8+4);directory=raw(rva)
records=[]
for pos in range(directory,directory+length,28):
    if u32(pos+12)!=2:continue
    entry=u32(pos+24)
    if data[entry:entry+4]!=b'RSDS':continue
    name=data[entry+24:entry+u32(pos+16)].split(b'\0',1)[0].decode('ascii').replace('\\','/').split('/')[-1]
    if name.lower()=='ntdll.pdb':records.append((uuid.UUID(bytes_le=data[entry+4:entry+20]).hex.upper(),u32(entry+20),name))
assert len(records)==1
guid,age,name=records[0];identity=guid+format(age,'X')
url='https://msdl.microsoft.com/download/symbols/'+name+'/'+identity+'/'+name
target=out/name;assert not target.exists(),'Preserve a prior symbol download.'
result=dict(binary=str(binary),binarySha256=hashlib.sha256(data).hexdigest(),pdb=name,guidAge=identity,url=url,installedSoftware=False)
try:
    with urllib.request.urlopen(url,timeout=25) as response:body=response.read()
    assert body.startswith(b'Microsoft C/C++ MSF 7.00'), 'Unexpected PDB data header'
    target.write_bytes(body);result.update(status='downloaded',bytes=len(body),sha256=hashlib.sha256(body).hexdigest())
except Exception as error:
    result.update(status='unavailable',error=str(error));raise
finally:(out/'manifest.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8',newline='\n')
print('Cached matching public ntdll symbols:',len(body),'bytes;',identity)
