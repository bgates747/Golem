#!/usr/bin/env python3
"""Build using only the exact source files selected for the distribution."""
from pathlib import Path
import hashlib
import importlib.util
import json
import shutil
import subprocess
import sys
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BUILD=HERE/'build'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def check():
 tree=BUILD/'rebuild-check/source'
 if tree.exists():shutil.rmtree(tree)
 def put(p,target):target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,target)
 for p in (ROOT/'src').rglob('*'):
  if p.suffix in ('.hpp','.cpp'):put(p,tree/'src'/p.relative_to(ROOT/'src'))
 task=tree/'docs/tasks/GOLEM-051'
 for p in (HERE/'port').iterdir():
  if p.is_file() and p.suffix in ('.py','.hpp','.cpp','.c'):put(p,task/'port'/p.name)
 sys.path.insert(0,str(HERE/'port'))
 import build
 vendor=HERE/'vendor/softfloat'
 selected=set(build.sources())|set((vendor/'source/include').glob('*.h'))|{vendor/'source/8086-SSE/specialize.h'}
 selected|={vendor/p for p in ('COPYING.txt','README.txt','platform.h','PROVENANCE.json')}
 for p in selected:put(p,task/'vendor/softfloat'/p.relative_to(vendor))
 with (BUILD/'rebuild-check.log').open('w') as log:
  subprocess.run([str(ROOT/'.venv/bin/python'),str(task/'port/build_all.py')],cwd=tree,stdout=log,stderr=subprocess.STDOUT,check=True)
 comparisons=[]
 for rel in ['support/golemc.bin','support/golemrun.bin']+['port/ez80-'+p+'/golemc.bin' for p in ('raw','resident','portable','stock-matrix')]:
  actual=task/'build'/rel;expected=BUILD/rel
  comparisons.append({'file':rel,'same':actual.read_bytes()==expected.read_bytes(),'sha256':sha(actual)})
 assert all(c['same'] for c in comparisons),comparisons
 report={'passed':True,'binaries':len(comparisons),'standalone_source_tree':True,'results':comparisons}
 (HERE/'rebuild-evidence.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':check()
