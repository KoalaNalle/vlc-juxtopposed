# SPDX-License-Identifier: GPL-2.0-or-later
"""Create a fresh synthetic WAV for the real native PCM/controller diagnostic."""
from pathlib import Path
import argparse,hashlib,json,math,struct,wave
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args();target=args.output.resolve()
assert not target.exists(),target
target.parent.mkdir(parents=True,exist_ok=True)
rate=44100;seconds=16
with wave.open(str(target),'wb') as output:
    output.setparams((1,2,rate,rate*seconds,'NONE','not compressed'))
    for start in range(0,rate*seconds,rate):
        output.writeframes(b''.join(struct.pack('<h',round(8192*math.sin(2*math.pi*1000*i/rate)))
            for i in range(start,min(start+rate,rate*seconds))))
print(json.dumps(dict(path=str(target),sampleRate=rate,channels=1,bits=16,seconds=seconds,
    sha256=hashlib.sha256(target.read_bytes()).hexdigest(),humanMediaUsed=False),indent=2))
