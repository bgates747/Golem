#!/usr/bin/env python3
"""Stage the dedicated local review; optional qualification is headless only."""
import argparse
import json
import shutil
import time
from qualify import HERE,ROOT,BUILD,PROFILE,Session,stage,sha
from ppm_png import read,convert

def prepare():
 app=stage();sd=PROFILE/'sdcard';mos=sd/'mos'
 if mos.is_symlink():
  original=mos.resolve();mos.unlink();mos.mkdir()
  for p in original.iterdir():(mos/p.name).symlink_to(p)
 if (mos/'golemc.bin').is_symlink():(mos/'golemc.bin').unlink()
 shutil.copyfile(BUILD/'support/golemc.bin',mos/'golemc.bin')
 if (sd/'bin/golemrun.bin').is_symlink():(sd/'bin/golemrun.bin').unlink()
 shutil.copyfile(BUILD/'support/golemrun.bin',sd/'bin/golemrun.bin')
 for mode in ('raw','resident','portable','stock-matrix'):shutil.copyfile(BUILD/'port'/('ez80-'+mode)/'golemc.bin',app/'compiler'/(mode+'.bin'))
 for d in ('examples','output','cache'):(app/d).mkdir(exist_ok=True)
 for p in (HERE/'examples').iterdir():shutil.copyfile(p,app/'examples'/p.name)
 for name in ('review-done.dat','review-keyboard.dat'):(app/name).unlink(missing_ok=True)
 auto=(HERE/'examples/review.txt').read_text()
 (PROFILE/'sdcard/autoexec.txt').write_bytes(auto.replace('\n','\r\n').encode())
 return app,auto

def check():
 app,auto=prepare();session=Session(auto,'review')
 try:
  session.wait(app/'review-done.dat',120);time.sleep(.5);initial=convert(session.capture('initial'))
  session.type('dir\nsave review-keyboard.dat 40000 3\n');session.wait(app/'review-keyboard.dat',20)
  image=session.capture();w,h,rgb=read(image);assert (w,h)==(1024,768)
  for x,y in ((256,288),(576,288),(256,608),(576,608),(768,448)):
   assert rgb[(y*w+x)*3:(y*w+x)*3+3]==b'\xff\xff\xff'
  report={'passed':True,'seconds':time.monotonic()-session.started,'actual_installed_compiler':True,'actual_installed_runner':True,
          'MOS_dir_and_save':True,'picture':str(convert(image)),'initial_picture':str(initial),'profile':str(PROFILE),'human':'pending','physical':False}
 finally:report_exit=session.close()
 report['frontend_exit']=report_exit;report['dispatcher_sha256']=sha(BUILD/'support/golemc.bin');report['runner_sha256']=sha(BUILD/'support/golemrun.bin')
 (HERE/'review-evidence.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--check',action='store_true');a=p.parse_args()
 if a.check:check()
 else:prepare();print('Review staged and stopped at '+str(PROFILE))
