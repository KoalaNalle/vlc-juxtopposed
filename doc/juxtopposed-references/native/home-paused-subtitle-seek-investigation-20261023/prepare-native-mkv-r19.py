from pathlib import Path
from hashlib import sha256
import ast,json
work=Path(__file__).resolve().parent
digest=lambda p:sha256(p.read_bytes()).hexdigest()
text=(work/'native-tracks-r15.py').read_text(encoding='utf-8')
text=text.replace('diagnosticOnly=False,privateCore=False','candidateOnly=True,diagnosticOnly=False,privateCore=False,privateMkv=True')
text=text.replace('privateCoreSha256=', 'coreSha256=')
text=text.replace("coreBuildScopeSha256=digest(work/'scope-r14.json')", "coreBuildScopeSha256=digest(work/'scope-r14.json'),mkvPluginSha256=digest(runtime/'plugins/demux/libmkv_plugin.dll'),mkvSourceRecordSha256=digest(work/'mkv-spu-seek-r18b-source/source-record.json')")
ast.parse(text);target=work/'native-mkv-seek-r19.py';assert not target.exists();target.write_text(text,encoding='utf-8',newline='\n')
print('Prepared paired native worker',digest(target))
