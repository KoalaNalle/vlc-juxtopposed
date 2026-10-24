from pathlib import Path
import ast
work=Path(__file__).resolve().parent
ordinary=(work/'run-native-subtitle-ordinary-r36c.py').read_text(encoding='utf-8')
source=ordinary[ordinary.index('source=read('):ordinary.index("base=work/'ordinary-ui-r14/runtime'")]
plugins=ordinary[ordinary.index('plugins={'):ordinary.index('hashes={phase:')]
for original,target,root in [
    ('run-native-seek-policy-r33.py','run-ordinary-seek-policy-r38.py','ordinary-seek-policy-r38'),
    ('run-native-subtitle-cancel-r31b.py','run-ordinary-subtitle-cancel-r38.py','ordinary-subtitle-cancel-r38'),
]:
    text=(work/original).read_text(encoding='utf-8')
    start=text.index('source=read(');end=text.index("base=work/'ordinary-ui-r14/runtime'",start)
    text=text[:start]+source+text[end:]
    start=text.index('plugins={');end=text.index('hashes={phase:',start)
    text=text[:start]+plugins+text[end:]
    oldroot='native-seek-policy-r33' if 'policy-r33' in original else 'native-subtitle-cancel-r31b'
    text=text.replace("root=work/'"+oldroot+"'", "root=work/'"+root+"'")
    text=text.replace("for phase in ['before','after']:","for phase in ['after']:")
    text=text.replace('candidateOnly=True,privateMkv=True','candidateOnly=False,privateMkv=False')
    text=text.replace("sourceRecordSha256=digest(work/'mkv-headers-r29-source/source-record.json')", "sourceRecordSha256=digest(work/'inputs-r35c.json')")
    ast.parse(text);path=work/target;assert not path.exists();path.write_text(text,encoding='utf-8',newline='\n')
