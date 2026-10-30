"""Generate independent flat and nested TTML timing references, without decoding."""
from fractions import Fraction
from pathlib import Path
import hashlib
import json
import os
import shutil

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'build-windows/beta-ttml-fps-20261029/media-r1'
VIDEO = ROOT / 'build-windows/beta-inline-webvtt-20261029/media-r1/frame-marker-black-bottom.mp4'


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def main():
    assert not OUT.exists() and shutil.disk_usage(ROOT).free >= 399864390452 + 128 * 1024**2
    assert sha(VIDEO) == '0f2e1f82444805a70ca1060bcdeb0ddc85b0f51ccb73a2a3d6080e1430ff8f3f'
    OUT.mkdir(parents=True)
    os.link(VIDEO, OUT / VIDEO.name)
    prefix = '''<?xml version="1.0" encoding="utf-8"?>
<tt xmlns="http://www.w3.org/ns/ttml" xmlns:tts="http://www.w3.org/ns/ttml#styling" tts:extent="960px 540px">
<head><layout><region xml:id="caption" tts:origin="10% 70%" tts:extent="80% 20%" tts:displayAlign="after" tts:textAlign="center" tts:fontFamily="Arial" tts:fontSize="32px"/></layout></head>
'''

    def stamp(seconds):
        us = round(seconds * 1_000_000)
        return f'{us//3600000000:02}:{us//60000000%60:02}:{us//1000000%60:02}.{us%1000000:06}'

    def p(begin, end, text, colour):
        return f'<p region="caption" begin="{begin}" end="{end}" tts:color="{colour}">{text}</p>'

    for name, ratio in [('flat', Fraction(1)), ('reference-50', Fraction(1, 2)),
                        ('reference-29_97', Fraction(25) / Fraction('29.97')),
                        ('reference-23_976', Fraction(25) / Fraction('23.976'))]:
        body = '<body><div>'
        for begin, end, text, colour in [(0,10,'ALPHA','#00FF00'), (10,20,'BETA','#FF00FF'), (20,50,'GAMMA','#FFFFFF')]:
            body += p(stamp(begin * ratio), stamp(end * ratio), text, colour)
        (OUT / (name + '.ttml')).write_text(prefix + body + '</div></body></tt>\n', newline='\n')
    nested = '<body begin="2s"><div begin="3s">' + p('5s', '15s', 'BETA', '#FF00FF') + '</div></body></tt>\n'
    (OUT / 'nested-par.ttml').write_text(prefix + nested, newline='\n')
    sequential = '''<body begin="2s"><div begin="3s" timeContainer="seq">
<p region="caption" dur="5s" tts:color="#00FF00">ALPHA</p>
<p region="caption" dur="10s" tts:color="#FF00FF">BETA</p>
</div></body></tt>
'''
    (OUT / 'nested-seq.ttml').write_text(prefix + sequential, newline='\n')
    record = dict(scope='Generated XML/clock references, not parser or playback acceptance',
                  generatorSha256=sha(__file__), sourceVideoSha256=sha(VIDEO),
                  expectedIntervals={'nested-par': [['BETA',10,20]], 'nested-seq': [['ALPHA',5,10],['BETA',10,20]]},
                  files={p.name:sha(p) for p in sorted(OUT.iterdir())})
    (OUT / 'verification.json').write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps(record['files']))


if __name__ == '__main__':
    main()
