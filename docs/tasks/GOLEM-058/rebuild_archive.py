#!/usr/bin/env python3
"""Rebuild exact shipped ZIP sources, never borrow repository compiler inputs."""
from pathlib import Path,PurePosixPath
import argparse,hashlib,json,os,subprocess,sys,time,zipfile
ROOT=Path(__file__).resolve().parents[3]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main(archive,output):
 archive,output=archive.resolve(),output.resolve()
 assert output.is_relative_to(ROOT/'build') and not output.exists()
 output.mkdir();tree=output/'extracted'
 with zipfile.ZipFile(archive)as z:
  names=z.namelist();assert len(names)==len(set(names))
  for n in names:
   p=PurePosixPath(n);assert not p.is_absolute() and '..' not in p.parts,n
  z.extractall(tree)
 manifest={}
 for line in(tree/'SHA256SUMS').read_text().splitlines():
  h,n=line.split('  ',1);assert sha(tree/n)==h,n;manifest[n]=h
 actual={str(p.relative_to(tree))for p in tree.rglob('*')if p.is_file()}
 assert actual==set(manifest)|{'SHA256SUMS'}
 source=tree/'source';task=source/'docs/tasks/GOLEM-051'
 command=[sys.executable,str(task/'port/build_all.py')]
 started=time.monotonic()
 with(output/'build.log').open('x')as log:
  rc=subprocess.run(command,cwd=source,env=dict(os.environ,AGONDEV_ROOT=os.environ.get('AGONDEV_ROOT','/Agon/agondev/release')),stdout=log,stderr=subprocess.STDOUT).returncode
 report=dict(passed=False,archive=str(archive),archive_sha256=sha(archive),manifest_files=len(manifest),command=command,returncode=rc,seconds=time.monotonic()-started,repository_compiler_inputs_used=False,comparisons=[])
 if not rc:
  pairs=[('support/golemc.bin','sdcard/mos/golemc.bin'),('support/golemrun.bin','sdcard/bin/golemrun.bin')]+[('port/ez80-'+p+'/golemc.bin','sdcard/golem/compiler/'+p+'.bin')for p in ['raw','resident','portable','stock-matrix']]
  for rebuilt,shipped in pairs:
   p=task/'build'/rebuilt;q=tree/shipped
   report['comparisons'].append(dict(shipped=shipped,bytes=p.stat().st_size,sha256=sha(p),expected_sha256=sha(q),same=p.read_bytes()==q.read_bytes()))
  report['passed']=all(r['same']for r in report['comparisons'])
 (output/'result.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report),flush=True)
 if not report['passed']:raise SystemExit(1)
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--archive',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();main(a.archive,a.output)
