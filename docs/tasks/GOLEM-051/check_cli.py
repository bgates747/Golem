#!/usr/bin/env python3
"""Check CLI-only seams: descriptor widths, response quoting and output options."""
from pathlib import Path
import json
import os
import shutil
import struct
import subprocess
import time
from qualify import HERE,ROOT,BUILD,Session,stage,sha

SOURCE='uint16 value=1 Sub tick value=value+1 EndSub\n'
CASES=[
 ('wide-binding',['--portable','--bind','1','30000','4294967290','5'],SOURCE,0),
 ('binding-overflow',['--portable','--bind','1','30000','4294967295','1'],SOURCE,1),
 ('binding-overflow-decimal',['--portable','--bind','1','30000','4294967296','1'],SOURCE,1),
 ('binding-duplicate',['--portable','--bind','1','30000','0','16','--bind','1','30001','0','16'],SOURCE,1),
 ('load-plan',['--portable','--emit-load-plan'],SOURCE,0),
 ('quoted-array',['--portable','--emit-load-plan','--array-data','samples=INPUT'],
  'uint16 index=1 uint32 result=0 Array uint32 samples(0:2) Sub fetch result=samples(index) EndSub\n',0),
 ('native-target',['--portable','--target','native','--emit-load-plan'],SOURCE,0),
 ('unknown-option',['--portable','--unrecognized'],SOURCE,1),
 ('invalid-reservation',['--portable','--reserve','65535:65535'],SOURCE,1),
 ('resident-asm',['--resident','--emit-asm','--emit-load-plan'],
  'float32 x Range(-32768,32767)=-0.75 sint16 result Sub compute result=sint16(floor(x)) EndSub\n',0),
 ('table-asm',['--resident','--emit-asm','--emit-load-plan','--table-data','samples=INPUT'],
  (HERE/'examples/table.golem').read_text(),0),
 ('matrix-plan',['--stock-matrix','--emit-load-plan'],
  'Matrix(3,3) model=Identity Sub clear matrix_identity(model) EndSub\n',0),
 ('raw-quoted',[], 'print "quoted source path"\n',0),
]

def files(path):
 if path.is_dir():return {str(p.relative_to(path)):sha(p) for p in path.rglob('*') if p.is_file()}
 return {'payload':sha(path)} if path.exists() else {}

def execute():
 app=stage();mos=app.parent/'mos'
 if mos.is_symlink():
  original=mos.resolve();mos.unlink();mos.mkdir()
  for p in original.iterdir():(mos/p.name).symlink_to(p)
 if (mos/'golemc.bin').is_symlink():(mos/'golemc.bin').unlink()
 shutil.copyfile(BUILD/'support/golemc.bin',mos/'golemc.bin')
 base=BUILD/'cli';base.mkdir(exist_ok=True);prepared=[];reports=[]
 for i,(name,flags,source,status) in enumerate(CASES):
  label=f'x{i:02d}';folder=base/label;folder.mkdir(exist_ok=True)
  native=app/'cli'/label;native.mkdir(parents=True,exist_ok=True)
  for d in (folder,native):(d/'source with spaces.golem').write_text(source)
  data=(HERE/'examples/records.dat').read_bytes() if name=='table-asm' else struct.pack('<III',1,4294967295,3)
  for d in (folder,native):(d/'data with spaces.bin').write_bytes(data)
  results=[]
  for kind,binary in [('reference',ROOT/'build/golemc'),('host',BUILD/'port/host/golemc'),('san',BUILD/'port/san/golemc')]:
   opts=[arg.replace('INPUT',str(folder/'data with spaces.bin')) for arg in flags]
   output=folder/(kind+('.bin' if not flags else ''))
   p=subprocess.run([str(binary),*opts,str(folder/'source with spaces.golem'),str(output)],capture_output=True,text=True,
                     env=dict(os.environ,ASAN_OPTIONS='detect_stack_use_after_return=0'))
   (folder/(kind+'.log')).write_text(p.stdout+p.stderr)
   assert p.returncode==status,(name,kind,p.returncode,p.stderr)
   assert not any(s in p.stderr for s in ('ERROR: AddressSanitizer','ERROR: LeakSanitizer','runtime error:'))
   results.append(files(output))
  assert results[0]==results[1]==results[2],name
  nopts=[arg.replace('INPUT','cli/'+label+'/data with spaces.bin') for arg in flags]
  output='out/cli-'+label+('.bin' if not flags else '')
  args=nopts+['cli/'+label+'/source with spaces.golem',output]
  (native/'compile.args').write_text(' '.join('"'+a+'"' if ' ' in a else a for a in args)+'\n')
  prepared.append({'name':name,'mode':flags[0][2:] if flags else 'raw','response':'cli/'+label+'/compile.args',
                   'output':output,'status':status,'expected_files':results[0]})
 for mode in ('raw','resident','portable','stock-matrix'):
  selected=[c for c in prepared if c['mode']==mode]
  (app/'extra.list').write_text(''.join(c['response']+'\n' for c in selected))
  shutil.copyfile(BUILD/'port'/('ez80-'+mode)/'golemc.bin',app/'compiler'/(mode+'.bin'))
  for name in ('batch.log','proof.dat'):(app/name).unlink(missing_ok=True)
  session=Session('SET KEYBOARD 1\nVDU 22 0\ncd /golem\ngolemc --batch extra.list\n','cli-'+mode)
  try:
   deadline=time.monotonic()+120
   while True:
    lines=(app/'batch.log').read_text().splitlines() if (app/'batch.log').exists() else []
    if len(lines)==len(selected):break
    if time.monotonic()>deadline:session.capture('timeout');raise TimeoutError(mode)
    time.sleep(.05)
   for case,line in zip(selected,lines):
    rc=int(line.split()[1]);actual=files(app/case['output'])
    assert rc==case['status'] and (rc or actual==case['expected_files']),(case['name'],rc,actual)
    reports.append({**case,'passed':True,'native_status':rc})
   time.sleep(.5);session.type('save proof.dat 40000 3\n');session.wait(app/'proof.dat',20)
  finally:assert session.close()==0
 report={'passed':True,'cases':len(reports),'host_and_sanitizers':True,'actual_eZ80_MOS':True,'results':reports}
 (HERE/'cli-evidence.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='results'}))

if __name__=='__main__':execute()
