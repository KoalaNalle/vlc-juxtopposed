# SPDX-License-Identifier: GPL-2.0-or-later
"""Copy the exact pending-seek producer into the isolated native fixture."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
source=args.source.read_text(encoding='utf-8')
begin=source.index('void\nvlc_player_UpdateTimerSeekState(')
end=source.index('\nstatic void\nvlc_player_UpdateTimerSource(',begin)
body=source[begin:end]
assert body.count('vlc_player_UpdateTimerSeekState(')==1
args.output.write_text(body,encoding='utf-8',newline='\n')
args.output.with_suffix('.json').write_text(json.dumps(dict(sourceSha256=sha256(args.source.read_bytes()).hexdigest(),extractedSha256=sha256(args.output.read_bytes()).hexdigest(),scope='Exact complete production function; supplied player fields and a recording callback are outside this extraction.'),indent=2)+'\n',encoding='utf-8',newline='\n')
