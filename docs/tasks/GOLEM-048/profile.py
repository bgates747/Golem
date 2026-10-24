#!/usr/bin/env python3
"""Dedicated full-MOS review/benchmark profile; no interactive launch by the agent."""
from pathlib import Path
import argparse,hashlib,importlib.util,json,os,shutil,signal,struct,subprocess,time
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];SDK=ROOT.parent/'agon-vdplet';BUILD=HERE/'build';PROFILE=ROOT/'.emulator/golem-048'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()

def active():
 result=[]
 for p in Path('/proc').glob('[0-9]*'):
  try:
   if str(PROFILE/'sdcard').encode() in (p/'cmdline').read_bytes().split(b'\0'):result.append(int(p.name))
  except OSError:pass
 return result

def stage(variant,runtime,label=""):
 if active():raise RuntimeError('Owned profile is running')
 if PROFILE.exists() and not(PROFILE/'.golem-048').exists():raise RuntimeError('Unowned profile')
 if(PROFILE/'.runtime.sha256').exists():
  subprocess.run(['sha256sum','--check','--status','.runtime.sha256'],cwd=PROFILE,check=True)
  (PROFILE/'.runtime.sha256').unlink()
 for name in ('flexe-agon-emulator','flexe-agon-emulator.bin'):(PROFILE/name).unlink(missing_ok=True)
 spec=importlib.util.spec_from_file_location('canonical_setup',ROOT.parent/'agon-dev-env/scripts/setup_emulator.py');setup=importlib.util.module_from_spec(spec);spec.loader.exec_module(setup)
 sd=setup.link_shared_runtime(PROFILE,runtime)
 (PROFILE/'.golem-048').write_text('Owned GOLEM-048 native command submission fixture\n')
 frozen=ROOT/'docs/tasks/GOLEM-037/binding/build/runtime'
 assert sha(frozen/'flexe-agon-emulator')=='28f50e87190f00de4039dc90c94432347f01c4771c4ea4a5dea8595d6cef7670'
 assert sha(frozen/'flexe-vdp.so')=='200f28cbe399b1fdc60cba1e5fed4c2bb86042b5e92f6e422539a5388c079a30'
 local=PROFILE/'runtime';local.mkdir(exist_ok=True);shutil.copyfile(frozen/'flexe-agon-emulator',local/'flexe-agon-emulator');(local/'flexe-agon-emulator').chmod(0o755)
 setup.replace_symlink(PROFILE/'fab-agon-emulator.bin',local/'flexe-agon-emulator');shutil.copyfile(frozen/'flexe-vdp.so',PROFILE/'flexe-vdp.so')
 for name in ('firmware.bin','firmware.elf'):shutil.copyfile(BUILD/'runtime-inputs'/name,PROFILE/('esp32-'+name))
 (PROFILE/'.bespoke-vdp-profile').write_text('flexe-vdp.so\n');setup.write_profile_launcher(PROFILE,strict_bespoke=True)
 launcher=PROFILE/'fab-agon-emulator';s=launcher.read_text();env='''
export AGON_FLEXE_FIRMWARE="$profile_dir/esp32-firmware.bin"
export AGON_FLEXE_ELF="$profile_dir/esp32-firmware.elf"
export AGON_FLEXE_ENGINE="${AGON_FLEXE_ENGINE:-jit}"
export AGON_FLEXE_ARTIFACTS="$profile_dir/last-run"
mkdir -p "$AGON_FLEXE_ARTIFACTS"
'''
 launcher.write_text(s.replace('\nvdp_name=',env+'\nvdp_name=',1));setup.name_flexe_profile(PROFILE)
 launcher=PROFILE/'flexe-agon-emulator';s=launcher.read_text().replace('cd "${profile_dir}"\n','cd "${profile_dir}"\nsha256sum --check --status .runtime.sha256\nrm -f -- sdcard/golem048/results.dat sdcard/golem048/failure.dat sdcard/golem048/review.log sdcard/golem048/benchmark.csv\n',1);launcher.write_text(s)
 app=sd/'golem048';app.mkdir(exist_ok=True)
 for name in ('results.dat','failure.dat','review.log','benchmark.csv'):(app/name).unlink(missing_ok=True)
 for name,source in {'probe.bin':BUILD/(variant+'.bin'),'scene.agnb':BUILD/'scene.agnb'}.items():shutil.copyfile(source,app/name)
 (sd/'autoexec.txt').write_bytes(b'SET KEYBOARD 1\r\ncd /golem048\r\nload probe.bin\r\nrun .\r\n')
 names=['flexe-agon-emulator','flexe-agon-emulator.bin','mos_platform.bin','mos_platform.map','flexe-vdp.so','esp32-firmware.bin','esp32-firmware.elf','sdcard/autoexec.txt','sdcard/golem048/probe.bin','sdcard/golem048/scene.agnb']
 pins={n:sha(PROFILE/n) for n in names};(PROFILE/'.runtime.sha256').write_text(''.join(value+'  '+name+'\n' for name,value in pins.items()))
 meta={'variant':variant,'pins':pins,'profile':str(PROFILE),'stopped':True,'label':label};(BUILD/'profile.json').write_text(json.dumps(meta,indent=2)+'\n');return meta

def verify(meta):
 app=PROFILE/'sdcard/golem048';data=(app/'results.dat').read_bytes();assert len(data)==20 and data[:4]==b'LIN1',data.hex()
 cap,frames,generation,stack,slot,bench,end,status=struct.unpack_from('<8H',data,4)
 expected=120 if meta['variant']=='review' else 25
 assert (cap,frames,generation,slot,bench,end,status)==(15,expected,expected,0,int(meta['variant']!='review'),int(meta['variant']!='benchmark-vdu'),0),(cap,frames,generation,slot,bench,end,status)
 assert stack>256,stack
 assert not(app/'failure.dat').exists();assert(app/'review.log').read_text().split()[-1]=='PASS'
 subprocess.run(['sha256sum','--check','--status','.runtime.sha256'],cwd=PROFILE,check=True)
 result={'passed':True,'variant':meta['variant'],'frames':frames,'result_bytes':len(data),'stack_free_bytes':stack,'results_sha256':sha(app/'results.dat'),'runtime_pins_verified':True,'application_log_passed':True}
 if bench:
  lines=[x for x in (app/'benchmark.csv').read_text().splitlines() if x.strip()]
  assert len(lines)==25,lines
  rows=[tuple(map(int,line.split(','))) for line in lines[1:]];assert all(len(x)==6 and x[5]==x[3]+x[4] for x in rows)
  assert {(a,b,c) for a,b,c,*_ in rows}=={(a,b,c) for a in (0,1) for b in range(6) for c in (0,1)}
  result['samples']=rows;result['clock']='MOS centisecond units; effective20ms resolution';result['physical']=False
 return result

def execute(meta,engine):
 stem=(meta.get('label','')+'-' if meta.get('label') else '')+meta['variant']+'-'+engine
 app=PROFILE/'sdcard/golem048'
 for name in ('results.dat','failure.dat','review.log','benchmark.csv'):(app/name).unlink(missing_ok=True)
 start=time.monotonic();env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',AGON_FLEXE_ENGINE=engine)
 for key in ('LD_PRELOAD','BASH_ENV'):env.pop(key,None)
 with (BUILD/(stem+'.log')).open('w') as log:
  process=subprocess.Popen(['./flexe-agon-emulator'],cwd=PROFILE,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
  try:
   while not(app/'results.dat').exists() or(app/'results.dat').stat().st_size<20:
    if(app/'failure.dat').exists():raise RuntimeError('Guest failed: '+(app/'failure.dat').read_bytes().hex())
    if process.poll() is not None:raise RuntimeError('Emulator exited before results')
    if time.monotonic()-start>600:raise TimeoutError('Guest did not finish; see owned profile log')
    time.sleep(.05)
   deadline=time.monotonic()+20;wire=PROFILE/'last-run/mos-to-vdp.bin'
   while b'/golem048 *' not in wire.read_bytes().partition(b'Type dir at the MOS prompt')[2]:
    if time.monotonic()>deadline:raise TimeoutError('No actual MOS return')
    time.sleep(.05)
   time.sleep(1.0)
  finally:
   if process.poll() is None:
    os.killpg(process.pid,signal.SIGTERM)
    try:process.wait(timeout=5)
    except subprocess.TimeoutExpired:os.killpg(process.pid,signal.SIGKILL);process.wait()
 deadline=time.monotonic()+10
 while active():
  if time.monotonic()>deadline:raise RuntimeError('Owned emulator child still active')
  time.sleep(.02)
 result=verify(meta);result.update(elapsed_seconds=round(time.monotonic()-start,3),engine=engine,mos_return=True,stopped=True,pins=meta['pins'],label=meta.get('label',''))
 stats=json.loads((PROFILE/'last-run/stats.json').read_text());assert not any(stats.get(k) for k in ('failed','unhandled','unregistered','unmapped')),stats;result['execution_stats']=stats
 for name in ('mos-to-vdp.bin','vdp-to-mos.bin','esp32-uart0.log'):
  shutil.copyfile(PROFILE/'last-run'/name,BUILD/(stem+'-'+name))
 shutil.copyfile(PROFILE/'last-run/final.ppm',BUILD/(stem+'.ppm'))
 shutil.copyfile(app/'results.dat',BUILD/(stem+'-results.dat'))
 for name in ('review.log','benchmark.csv'):
  if(app/name).exists():shutil.copyfile(app/name,BUILD/(stem+'-'+name))
 (HERE/(stem+'-evidence.json')).write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))

if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--runtime',type=Path,required=True);p.add_argument('--variant',choices=('review','benchmark-vdu','benchmark-lines'),default='review');p.add_argument('--engine',choices=('jit','interp'),default='jit');p.add_argument('--stage-only',action='store_true');p.add_argument('--label',default='',choices=('','instrumented-off','instrumented-on'));a=p.parse_args();m=stage(a.variant,a.runtime.resolve(),a.label)
 if not a.stage_only:execute(m,a.engine)
