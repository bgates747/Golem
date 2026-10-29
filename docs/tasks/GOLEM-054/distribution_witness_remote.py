"""Run the prepared distribution witness headlessly on the review host."""
import json
import shutil
import struct
import tarfile
import time
import qualify as f

p=f.PROFILE
with tarfile.open(p/'distribution-witness.tar') as archive:
    archive.extractall(f.ROOT,filter='data')
d=p/'sdcard/golem/witness'
shutil.rmtree(d/'package',ignore_errors=True)
for name in ('done.dat','results.dat','scene.dat','advance.dat','runner.dat'):(d/name).unlink(missing_ok=True)
auto='SET KEYBOARD 1\nVDU 22 0\ncd /golem/witness\ngolemc --portable float-print.golem package\nload review.bin\nrun .\nVDU 12\ngolemrun --portable package\nsave runner.dat 40000 3\n'
s=f.q.Session(auto,'float-054-distribution')
report={}
try:
    captures=[]
    for side in (0,1):
        deadline=time.monotonic()+180
        while True:
            try:data=(d/'scene.dat').read_bytes()
            except FileNotFoundError:data=b''
            if len(data)==2 and struct.unpack('<H',data)[0]==side:break
            if s.process.poll() is not None:raise RuntimeError('Emulator stopped')
            if time.monotonic()>deadline:s.capture('stalled');raise TimeoutError('Distribution scene')
            time.sleep(.02)
        captures.append(f.ppm.read(s.capture('expected' if not side else 'Golem')))
        (d/'advance.dat').write_bytes(f.word(side))
    assert captures[0]==captures[1]
    s.wait(d/'done.dat');time.sleep(.1)
    assert (d/'done.dat').read_bytes()==b'\0'
    reference=f.BUILD/'distribution-reference'
    matching=0
    for path in reference.iterdir():
        assert path.read_bytes()==(d/'package'/path.name).read_bytes(),path.name
        matching+=1
    # A fresh screen prevents the earlier test client's output being mistaken
    # for the unchanged distribution runner's own Golem output.
    # Autoexec waits for the application to return before its save command;
    # it cannot lose keystrokes while the runner is installing its buffers.
    s.wait(d/'runner.dat',30)
    screenshot=s.capture('unchanged-golemrun');f.ppm.convert(screenshot)
    width,height,rgb=f.ppm.read(screenshot)
    wanted=b'reading=1.5'
    w0,h0,ref=captures[0]
    assert width==w0 and height==h0 and width==640
    span=len(wanted)*8*3
    glyph=b''.join(ref[y*width*3:y*width*3+span] for y in range(8))
    assert any(glyph),'Empty expected text strip'
    matches=[]
    for y0 in range(0,height-7):
        strip=b''.join(rgb[(y0+y)*width*3:(y0+y)*width*3+span] for y in range(8))
        if strip==glyph:matches.append(y0)
    assert matches,'Unchanged golemrun did not render the exact distributed example text'
    report={'passed':True,'source':'GOLEM-051/examples/float-print.golem',
            'source_sha256':f.sha(d/'float-print.golem'),'worker_sha256':f.sha(p/'sdcard/golem/compiler/portable.bin'),
            'runner_sha256':f.sha(p/'sdcard/bin/golemrun.bin'),'actual_MOS_compilation':True,
            'matching_package_files':matching,'expected_text':wanted.decode(),
            'pixel_pairs':1,'pixels_compared':width*height,
            'unchanged_distribution_runner_text_pixel_match':True,'matching_runner_text_y':matches,
            'MOS_autoexec_continuation':True,'seconds':time.monotonic()-s.started,'human':'pending'}
finally:report['frontend_exit']=s.close()
(f.HERE/'distribution-witness-evidence.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report))
