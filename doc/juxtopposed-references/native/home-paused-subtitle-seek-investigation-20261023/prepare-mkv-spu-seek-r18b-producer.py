from pathlib import Path
import ast
work=Path(__file__).resolve().parent
for name in ['mkv-spu-seek-r18.mk','build-mkv-spu-seek-r18.sh']:
    target=work/name.replace('r18','r18b');assert not target.exists()
    target.write_text((work/name).read_text(encoding='utf-8').replace('r18','r18b'),encoding='utf-8',newline='\n')
text=(work/'prepare-mkv-spu-seek-r18.py').read_text(encoding='utf-8')
assert text.count('original.replace(before,addition+before)')==1
text=text.replace('original.replace(before,addition+before)','original.replace(before,before+addition)')
text=text.replace('r18','r18b')
ast.parse(text)
target=work/'prepare-mkv-spu-seek-r18b.py';assert not target.exists();target.write_text(text,encoding='utf-8',newline='\n')
