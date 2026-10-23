from pathlib import Path
import ast
work=Path(__file__).resolve().parent
for name in ['mkv-interval-r23b.mk','build-mkv-interval-r23b.sh']:
    target=work/name.replace('r23b','r23c');assert not target.exists()
    target.write_text((work/name).read_text(encoding='utf-8').replace('r23b','r23c'),encoding='utf-8',newline='\n')
text=(work/'prepare-mkv-interval-r23b.py').read_text(encoding='utf-8')
before='''                if( !p_track->b_no_duration && span >= 1
                 && span <= static_cast<long double>(VLC_TICK_MAX) - pts )
                    end_pts = pts + static_cast<vlc_tick_t>(span);'''
after='''                const long double stop = static_cast<long double>(pts) + span;
                if( !p_track->b_no_duration && span >= 1
                 && stop >= static_cast<long double>(VLC_TICK_MIN)
                 && stop <= static_cast<long double>(VLC_TICK_MAX) )
                    end_pts = static_cast<vlc_tick_t>(stop);'''
assert text.count(before)==1;text=text.replace(before,after).replace('r23b','r23c')
ast.parse(text);target=work/'prepare-mkv-interval-r23c.py';assert not target.exists();target.write_text(text,encoding='utf-8',newline='\n')
