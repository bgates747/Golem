#!/usr/bin/env python3
"""Compare the port against the current maintained compiler on captured cases."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import subprocess
import time

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
BUILD=HERE/'build'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()

def cases():
    out=[]
    for directory in sorted((BUILD/'capture/cases').iterdir()):
        options=directory.joinpath('options.txt').read_text().splitlines()
        mode=options[0][2:] if options and options[0].startswith('--') else 'raw'
        out.append({'key':directory.name,'mode':mode,'source':directory/'source.golem','options':options,'variant':'both'})
        if mode=='portable':out.append({'key':directory.name+'-native','mode':mode,'source':directory/'source.golem','options':options[:1]+['--target','native']+options[1:],'variant':'native'})
    return out

def options(case,native=False,prefix=None):
    args=[]
    for arg in case['options']:
        if arg.endswith('=@data.bin'):
            path=(prefix/'data.bin') if native else case['source'].parent/'data.bin'
            arg=arg[:-9]+str(path)
        args.append(arg)
    return args

def check(sanitize=False):
    binary=BUILD/'port'/('san' if sanitize else 'host')/'golemc'
    folder=BUILD/('parity-san' if sanitize else 'parity-all');folder.mkdir(exist_ok=True)
    started=time.monotonic();reports=[];env=dict(os.environ,ASAN_OPTIONS='detect_stack_use_after_return=0')
    for index,case in enumerate(cases()):
        item=folder/f'c{index:03d}';item.mkdir(exist_ok=True)
        results=[]
        for label,compiler in [('reference',ROOT/'build/golemc'),('port',binary)]:
            output=item/(label+'.bin' if case['mode']=='raw' else label)
            command=[str(compiler),*options(case),str(case['source']),str(output)]
            p=subprocess.run(command,capture_output=True,text=True,env=env)
            (item/(label+'.log')).write_text(p.stdout+p.stderr)
            result={'status':p.returncode,'files':{}}
            if sanitize and label=='port' and any(word in p.stderr for word in ('ERROR: AddressSanitizer','ERROR: LeakSanitizer','runtime error:')):
                result['sanitizer_failure']=True
            if output.is_dir():result['files']={str(x.relative_to(output)):sha(x) for x in output.rglob('*') if x.is_file()}
            elif output.is_file():result['files']={'payload':sha(output)}
            results.append(result)
        same=results[0]==results[1]
        reports.append({'index':index,'key':case['key'],'mode':case['mode'],'variant':case['variant'],'same':same,'reference':results[0],'port':results[1]})
        if not same:
            print('MISMATCH',index,case['key'],case['mode'],results,flush=True)
    report={'passed':all(r['same'] for r in reports),'cases':len(reports),'seconds':time.monotonic()-started,
            'accepted':sum(r['reference']['status']==0 for r in reports),'sanitizers':sanitize,'results':reports}
    (HERE/('parity-san.json' if sanitize else 'parity-host.json')).write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='results'}));return report

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--sanitize',action='store_true');a=p.parse_args()
    result=check(a.sanitize);raise SystemExit(0 if result['passed'] else 1)
