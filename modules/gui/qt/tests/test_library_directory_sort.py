#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile the production directory comparator adapter and check its contract.

This portable check complements actual Windows vlc_scandir/provider runs. It
does not emulate directory enumeration or establish live-service availability.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

root=Path(__file__).resolve().parents[4]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source',type=Path,default=root/'src/text/filesystem.c')
parser.add_argument('--output-dir',type=Path,required=True)
args=parser.parse_args();args.output_dir.mkdir(parents=True,exist_ok=False)
source=args.source.read_text(encoding='utf-8')
start=source.index('static int compar_void(')
body=source[start:source.index('\n/**',start)].strip()
harness=r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int (*callback)(const char **,const char **);
static const char *known[]={"03_lastfm.luac","01_googleimage.luac","02_frenchtv.luac","00_musicbrainz.luac"};
static int invalid=0;
static int names(const char **a,const char **b) {
    int valid_a=0,valid_b=0;
    for(size_t i=0;i<4;i++) { valid_a|=*a==known[i];valid_b|=*b==known[i]; }
    if(!valid_a||!valid_b) { invalid++;return 0; }
    return strcmp(*a,*b);
}
'''+body+r'''
static int compare(const void *a,const void *b) { return compar_void(a,b,callback); }
int main(void) {
    const char *items[4];memcpy(items,known,sizeof(items));callback=names;
    qsort(items,4,sizeof(*items),compare);
    if(invalid) { fprintf(stderr,"Comparator received %d arguments pointing to array slots instead of filename strings\n",invalid);return 1; }
    const char *expected[]={"00_musicbrainz.luac","01_googleimage.luac","02_frenchtv.luac","03_lastfm.luac"};
    for(size_t i=0;i<4;i++) if(strcmp(items[i],expected[i])) return 2;
    puts("Filename pointer contract and provider priority order pass");return 0;
}
'''
compiler=next((shutil.which(c) for c in (os.environ.get('CC','cc'),'gcc','clang') if shutil.which(c)),None)
if not compiler:raise SystemExit('A C compiler is required')
with tempfile.TemporaryDirectory(prefix='vlc-directory-sort-') as temp:
    temp=Path(temp);cfile=temp/'sort.c';binary=temp/('sort.exe' if os.name=='nt' else 'sort')
    cfile.write_text(harness,encoding='utf-8')
    compiled=subprocess.run([compiler,'-std=c11','-Wall','-Wextra',str(cfile),'-o',str(binary)],capture_output=True,text=True)
    (args.output_dir/'compile.txt').write_text(compiled.stdout+compiled.stderr,encoding='utf-8');compiled.check_returncode()
    result=subprocess.run([str(binary)],capture_output=True,text=True)
(args.output_dir/'result.txt').write_text(result.stdout+result.stderr,encoding='utf-8')
report={'extractedSha256':hashlib.sha256(body.encode()).hexdigest(),'exitCode':result.returncode,'passed':result.returncode==0,'compiledProductionBody':True}
(args.output_dir/'verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps(report));raise SystemExit(result.returncode)
