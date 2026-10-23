from pathlib import Path
import ast
work=Path(__file__).resolve().parent
for name in ['mkv-interval-r23c.mk','build-mkv-interval-r23c.sh']:
    target=work/name.replace('r23c','r23d');assert not target.exists()
    target.write_text((work/name).read_text(encoding='utf-8').replace('r23c','r23d'),encoding='utf-8',newline='\n')
text=(work/'prepare-mkv-interval-r23c.py').read_text(encoding='utf-8')
before='&& stop <= static_cast<long double>(VLC_TICK_MAX)'
assert text.count(before)==1
text=text.replace(before,'&& stop < static_cast<long double>(VLC_TICK_MAX)').replace('r23c','r23d')
ast.parse(text);target=work/'prepare-mkv-interval-r23d.py';assert not target.exists();target.write_text(text,encoding='utf-8',newline='\n')
