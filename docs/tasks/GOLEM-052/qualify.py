"""Actual MOS compilation and pixel-exact stock numeric printing checks."""
from pathlib import Path
import importlib.util,json,os,shutil,struct,sys,time
from check import HERE,ROOT,BUILD,cases,program,compile_source,run,word
sys.path.insert(0,str(ROOT/'docs/tasks/GOLEM-051'))
spec=importlib.util.spec_from_file_location('golem051_session',ROOT/'docs/tasks/GOLEM-051/qualify.py');q=importlib.util.module_from_spec(spec);spec.loader.exec_module(q)
from ppm_png import read,convert

def prepare():
 app=q.stage();dest=app/'printing';dest.mkdir(exist_ok=True)
 for d in ('cache','output'):(app/d).mkdir(exist_ok=True)
 compiler=ROOT/'docs/tasks/GOLEM-051/build/port/ez80-portable/golemc.bin'
 shutil.copyfile(compiler,app/'compiler/portable.bin')
 source=HERE/'numbers.golem';source.write_text(program());shutil.copyfile(source,dest/'numbers.golem');shutil.copyfile(HERE/'review.golem',dest/'counter.golem')
 meta,iface=compile_source(source,BUILD/'target-reference');fields={f['name']:f for f in iface['globals']}
 initial=(BUILD/'target-reference/state.bin').read_bytes();offsets=[12,13,14,15]+[f['offset']+i for f in iface['globals'] for i in range(f['bytes'])]
 def state_for(values):
  state=bytearray(initial)
  for name,value in values.items():
   f=fields[name];state[f['offset']:f['offset']+f['bytes']]=(value&((1<<(8*f['bytes']))-1)).to_bytes(f['bytes'],'little')
  return state
 records=[]
 for values in cases():
  state=state_for(dict(zip(('u','s','w','n'),values)))
  records.append((0,1,state,bytes(state[o] for o in offsets),'['+','.join(map(str,values))+']'))
 for total,reset in ((55,1),(110,0)):
  state=state_for({});expected=state_for({'i':10,'total':total})
  records.append((1,reset,state,bytes(expected[o] for o in offsets),f'i=10 total={total}'))
 state=state_for({'u':65535});expected=bytearray(state);struct.pack_into('<H',expected,14,1)
 records.append((2,1,state,bytes(expected[o] for o in offsets),'before:'))
 state=state_for({});expected=state_for({'i':2});records.append((3,1,state,bytes(expected[o] for o in offsets),'0x2'))
 data=b''.join(word(entry)+bytes((reset,len(text)))+state+expected+text.encode() for entry,reset,state,expected,text in records)
 (dest/'cases.dat').write_bytes(data)
 (HERE/'target-cases.json').write_text(json.dumps({'cases':len(records),'numeric_cases':len(cases()),'observations':len(records)*len(offsets),'expected_observations':b''.join(r[3] for r in records).hex(),'texts':[r[4] for r in records]},indent=2)+'\n')
 header=f'#define STATE_BYTES {len(initial)}\n#define STATE_ID {meta["shared_state"]}\n#define CASE_COUNT {len(records)}\n#define NUMERIC_CASES {len(cases())}\n#define OFFSET_COUNT {len(offsets)}\n'
 for name,values in [('offsets',offsets),('entries',meta['buffered_entries'])]:header+='static const unsigned '+name+'[]={'+','.join(map(str,values))+'};\n'
 (BUILD/'fixture.h').write_text(header)
 tools=Path('/Agon/agondev/release')
 run([tools/'bin/ez80-none-elf-clang','-target','ez80-none-elf','-mllvm','-z80-gas-style','-mllvm','-z80-print-zero-offset','-nostdinc','-isystem',tools/'include','-I'+str(BUILD),'-Oz','-DAGONDEV','-Wa,-march=ez80+full','-c',HERE/'guest.c','-o',BUILD/'guest.o'])
 run([tools/'bin/ez80-none-elf-ld','-defsym=RAM_START=0x40000','-defsym=RAM_SIZE=0x70000','-defsym=_has_exit_handler=0','-T',tools/'config/linker.conf','--oformat','binary','-o',BUILD/'review.bin',BUILD/'guest.o','-L'+str(tools/'lib'),'-lagon'])
 shutil.copyfile(BUILD/'review.bin',dest/'review.bin')
 for name in ('package','counter') :shutil.rmtree(dest/name,ignore_errors=True)
 for file in ('done.dat','results.dat','scene.dat','advance.dat','keyboard.dat'):(dest/file).unlink(missing_ok=True)
 auto='SET KEYBOARD 1\nVDU 22 0\ncd /golem/printing\ngolemc --portable numbers.golem package\ngolemc --portable counter.golem counter\nload review.bin\nrun .\n'
 return dest,auto,records,offsets

def execute():
 dest,auto,records,offsets=prepare();session=q.Session(auto,'printing-052');report={};pixels=0
 try:
  for i,r in enumerate(records):
   captures=[]
   for side in (0,1):
    n=i*2+side;deadline=time.monotonic()+180
    while True:
     p=dest/'scene.dat'
     if p.exists() and p.stat().st_size==2 and struct.unpack('<H',p.read_bytes())[0]==n:break
     if time.monotonic()>deadline:session.capture('stalled');raise TimeoutError(f'scene {n}')
     if session.process.poll() is not None:raise RuntimeError('emulator stopped')
     time.sleep(.02)
    # C3 ordered drawing completion precedes scene notification; cursor disabled.
    captures.append(read(session.capture(f'case-{i}-{side}')))
    tmp=dest/'advance.tmp';tmp.write_bytes(word(n));tmp.replace(dest/'advance.dat')
   assert captures[0]==captures[1],(i,r[4])
   pixels+=captures[0][0]*captures[0][1]
  session.wait(dest/'done.dat',30);session.wait(dest/'results.dat',10)
  assert (dest/'done.dat').read_bytes()==b'\0'
  assert (dest/'results.dat').read_bytes()==b''.join(r[3] for r in records)
  count=0
  for p in (BUILD/'target-reference').iterdir():assert p.read_bytes()==(dest/'package'/p.name).read_bytes(),p.name;count+=1
  session.type('dir\nsave keyboard.dat 40000 3\n');session.wait(dest/'keyboard.dat',20)
  report={'passed':True,'seconds':time.monotonic()-session.started,'actual_MOS_compilation':True,'matching_package_files':count,'pixel_pairs':len(records),'pixels_compared':pixels,'state_observations':len(records)*len(offsets),'MOS_keyboard_continuation':True,'human':'pending'}
 finally:report['frontend_exit']=session.close()
 (HERE/'stock-evidence.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':execute()
