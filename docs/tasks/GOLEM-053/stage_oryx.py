"""Stage a separate stock Flexe review, preserving the active walkthrough."""
from pathlib import Path
import json,subprocess,tarfile
from check import HERE,ROOT,BUILD,sha
HOST='oryx-linuxlite'
REMOTE='Agon/mystuff/golem/.emulator/golem-053'
def ssh(code):
    subprocess.run(['ssh','-o','BatchMode=yes',HOST,'python3 -'],input=code.encode(),check=True)
def stage():
    # Canonical generator and the already verified stock firmware/runtime.
    ssh('''from pathlib import Path
import runpy,shutil,hashlib,subprocess
base=Path.home()/'Agon/mystuff';p=base/'golem/.emulator/golem-053'
assert not p.exists(), 'Preserve existing profile'
a=runpy.run_path(str(base/'agon-dev-env/scripts/setup_emulator.py'))
sd=a['link_shared_runtime'](p,base/'golem/.emulator/runtime/fab-1.2.5')
source=base/'golem/.emulator/golem-051-stock'
subprocess.run(['sha256sum','--check','--status','.runtime.sha256'],cwd=source,check=True)
runtime=p/'runtime';runtime.mkdir()
for n in ('flexe-agon-emulator.bin','flexe-vdp.so'):
 shutil.copyfile(source/'runtime'/n,runtime/n);(runtime/n).chmod(0o755)
shutil.copyfile(source/'esp32-firmware.bin',p/'esp32-firmware.bin')
assert hashlib.sha256((p/'esp32-firmware.bin').read_bytes()).hexdigest()=='b807beef35823b13a0a056f11b7464cd1b1c6356dce0e4098b78ebe059ded35f'
a['replace_symlink'](p/'fab-agon-emulator.bin',runtime/'flexe-agon-emulator.bin')
a['replace_symlink'](p/'flexe-vdp.so',runtime/'flexe-vdp.so')
(p/'.bespoke-vdp-profile').write_text('flexe-vdp.so\\n')
a['write_profile_launcher'](p,strict_bespoke=True)
w=p/'fab-agon-emulator';s=w.read_text();s=s.replace('vdp_name=',\'''export AGON_FLEXE_FIRMWARE="$profile_dir/esp32-firmware.bin"
unset AGON_FLEXE_ELF
export AGON_FLEXE_ENGINE=jit
export AGON_FLEXE_ARTIFACTS="$profile_dir/last-run"
mkdir -p "$AGON_FLEXE_ARTIFACTS"
sha256sum --check --status "$profile_dir/.runtime.sha256" || exit 1
vdp_name=\''',1);w.write_text(s)
a['name_flexe_profile'](p)
mos=sd/'mos';origin=mos.resolve();mos.unlink();shutil.copytree(origin,mos)
''')
    bundle=BUILD/'oryx.tar'
    sd=ROOT/'.emulator/golem-051/sdcard'
    with tarfile.open(bundle,'w') as tar:
        for p in (sd/'golem/bitwise').rglob('*'):
            if not p.is_file() or p.name in ('done.dat','keyboard.dat','results.dat'):continue
            tar.add(p,arcname=str(p.relative_to(sd)),recursive=False)
        for name in ('golemc','golemrun'):
            tar.add(ROOT/'docs/tasks/GOLEM-051/build/support'/(name+'.bin'),arcname=('mos/' if name=='golemc' else 'bin/')+name+'.bin')
        tar.add(ROOT/'docs/tasks/GOLEM-051/build/port/ez80-portable/golemc.bin',arcname='golem/compiler/portable.bin')
    subprocess.run(['scp',str(bundle),HOST+':'+REMOTE+'/review-files.tar'],check=True)
    ssh('''from pathlib import Path
import tarfile,hashlib,json
p=Path.home()/'Agon/mystuff/golem/.emulator/golem-053';sd=p/'sdcard'
with tarfile.open(p/'review-files.tar') as tar:tar.extractall(sd,filter='data')
for n in ('cache','output'):(sd/'golem'/n).mkdir(exist_ok=True)
(sd/'autoexec.txt').write_bytes((sd/'golem/bitwise/review.txt').read_bytes())
names=['flexe-agon-emulator','flexe-agon-emulator.bin','flexe-vdp.so','esp32-firmware.bin','mos_platform.bin','mos_platform.map']
pins={n:hashlib.sha256((p/n).read_bytes()).hexdigest() for n in names}
(p/'.runtime.sha256').write_text(''.join(f'{h}  {n}\\n' for n,h in pins.items()))
(p/'identity.json').write_text(json.dumps(pins,indent=2)+'\\n')
print('Staged and stopped:',p)
''')
    (HERE/'deployment.json').write_text(json.dumps({'host':HOST,'profile':REMOTE,'gui_launched':False,'stock_vdp':'2.16','compiler_sha256':sha(ROOT/'docs/tasks/GOLEM-051/build/port/ez80-portable/golemc.bin')},indent=2)+'\n')
if __name__=='__main__':stage()
