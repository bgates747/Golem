#!/usr/bin/env python3
"""Use established captured cases without rewriting earlier qualification."""
from pathlib import Path
import importlib.util,hashlib,json,subprocess,os,time
ROOT=Path(__file__).resolve().parents[3];TASK=ROOT/'docs/tasks/GOLEM-051'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 spec=importlib.util.spec_from_file_location('old_parity_cases',TASK/'check_parity.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
 binary=ROOT/'build/g058-archive-rebuild-r01/extracted/source/docs/tasks/GOLEM-051/build/port/host/golemc'
 out=ROOT/'build/g058-archive-parity-r01';out.mkdir(exist_ok=False);rows=[];start=time.monotonic()
 for i,case in enumerate(m.cases()):
  folder=out/str(i);folder.mkdir();results=[]
  for label,compiler in [('maintained',ROOT/'build/golemc'),('archive-adapted',binary)]:
   dest=folder/(label+'.bin'if case['mode']=='raw'else label)
   p=subprocess.run([str(compiler),*m.options(case),str(case['source']),str(dest)],capture_output=True)
   (folder/(label+'.log')).write_bytes(p.stdout+p.stderr)
   files={str(f.relative_to(dest)):sha(f)for f in dest.rglob('*')if f.is_file()}if dest.is_dir()else {'payload':sha(dest)}if dest.is_file()else{}
   results.append(dict(returncode=p.returncode,files=files))
  rows.append(dict(case=case['key'],same=results[0]==results[1],results=results))
 report=dict(passed=all(r['same']for r in rows),cases=len(rows),accepted=sum(r['results'][0]['returncode']==0 for r in rows),seconds=time.monotonic()-start,exact_archive_host_binary_sha256=sha(binary),results=rows)
 (out/'result.json').write_text(json.dumps(report,indent=2)+'\n')
 (ROOT/'docs/tasks/GOLEM-058/archive-parity-results.json').write_text(json.dumps({k:v for k,v in report.items()if k!='results'},indent=2)+'\n')
 print(json.dumps({k:v for k,v in report.items()if k!='results'}));assert report['passed']
if __name__=='__main__':main()
