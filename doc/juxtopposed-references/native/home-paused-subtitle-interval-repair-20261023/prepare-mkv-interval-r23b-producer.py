from pathlib import Path
import ast
work=Path(__file__).resolve().parent
for name in ['mkv-interval-r23.mk','build-mkv-interval-r23.sh']:
    target=work/name.replace('r23','r23b');assert not target.exists()
    target.write_text((work/name).read_text(encoding='utf-8').replace('r23','r23b'),encoding='utf-8',newline='\n')
text=(work/'prepare-mkv-interval-r23.py').read_text(encoding='utf-8')
before='''        if( ms.BlockGet( block, simpleblock, additions, &key, &discardable, &duration ) )
        {
            delete block;
            delete additions;
            return false;
        }'''
after='''        if( ms.BlockGet( block, simpleblock, additions, &key, &discardable, &duration ) )
        {
            delete additions;
            return false;
        }'''
assert text.count(before)==1;text=text.replace(before,after).replace('r23','r23b')
ast.parse(text);target=work/'prepare-mkv-interval-r23b.py';assert not target.exists();target.write_text(text,encoding='utf-8',newline='\n')
