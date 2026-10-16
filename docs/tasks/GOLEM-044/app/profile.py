#!/usr/bin/env python3
"""Prepare only this task's stopped Oryx application profile."""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import shutil
import json

HERE=Path(__file__).resolve().parent
TASK=HERE.parent
ROOT=TASK.parents[2]
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()


def prepare(layout='default',stock=False,start_mode=8,legacy=False,busy=False):
    profile=TASK/('.emulator-stock' if stock else '.emulator')
    for process in Path('/proc').glob('[0-9]*'):
        try:
            command=(process/'cmdline').read_bytes().split(b'\0')
            if str(profile/'sdcard').encode() in command:
                raise SystemExit('Profile is running; close it before staging: '+str(profile))
        except OSError:pass
    # Restaging an owned, already migrated profile: verify it before replacing
    # its generated launcher, then let the canonical tool name the new one.
    if (profile/'flexe-agon-emulator').exists():
        import subprocess
        subprocess.run(['sha256sum','--check','--status','.runtime.sha256'],cwd=profile,check=True)
        for name in ('flexe-agon-emulator','flexe-agon-emulator.bin','.runtime.sha256'):
            (profile/name).unlink()
    spec=importlib.util.spec_from_file_location('canonical',HERE/'build/setup_emulator.py')
    canonical=importlib.util.module_from_spec(spec);spec.loader.exec_module(canonical)
    sd=canonical.link_shared_runtime(profile,ROOT/'.emulator/runtime/fab-1.2.5')
    inputs=['fab-agon-emulator.bin','mos_platform.bin','mos_platform.map']
    if not stock:
        source=ROOT/'docs/tasks/GOLEM-037/binding/build/runtime'
        assert sha(source/'flexe-agon-emulator')=='28f50e87190f00de4039dc90c94432347f01c4771c4ea4a5dea8595d6cef7670'
        assert sha(source/'flexe-vdp.so')=='200f28cbe399b1fdc60cba1e5fed4c2bb86042b5e92f6e422539a5388c079a30'
        runtime=profile/'runtime';runtime.mkdir(exist_ok=True)
        shutil.copy2(source/'flexe-agon-emulator',runtime/'flexe-agon-emulator')
        canonical.replace_symlink(profile/'fab-agon-emulator.bin',runtime/'flexe-agon-emulator')
        shutil.copyfile(source/'flexe-vdp.so',profile/'flexe-vdp.so')
        for name in ('firmware.bin','firmware.elf'):
            shutil.copyfile(HERE/'build/runtime-inputs'/name,profile/('esp32-'+name))
        (profile/'.bespoke-vdp-profile').write_text('flexe-vdp.so\n')
        inputs+=['flexe-vdp.so','esp32-firmware.bin','esp32-firmware.elf']
    else:
        inputs+=['firmware/vdp_platform.so']
    app=sd/'flower';app.mkdir(exist_ok=True)
    shutil.copyfile(HERE/'build'/layout/'flower.bin',app/'flower.bin')
    shutil.copyfile(TASK/'flower.golem',app/'flower.golem')
    shutil.copyfile(HERE/'src/main.cpp',app/'main.cpp')
    (app/'flower.log').unlink(missing_ok=True)
    if not busy and (app/'busy.bin').exists():
        assert sha(app/'busy.bin')==sha(HERE/'build/busy.bin'),'unexpected busy fixture'
        (app/'busy.bin').unlink()
    startup='SET KEYBOARD 1\r\n'
    if legacy:startup+='VDU 23 0 193 1\r\n'
    startup+=f'VDU 22 {start_mode}\r\ncd /flower\r\n'
    if busy:
        shutil.copyfile(HERE/'build/busy.bin',app/'busy.bin')
        startup+='load busy.bin\r\nrun .\r\n'
    startup+='load flower.bin\r\nrun .\r\n'
    (sd/'autoexec.txt').write_bytes(startup.encode())
    canonical.write_profile_launcher(profile,strict_bespoke=not stock)
    launcher=profile/'fab-agon-emulator';script=launcher.read_text()
    if not stock:
        env='''
export AGON_FLEXE_FIRMWARE="$profile_dir/esp32-firmware.bin"
export AGON_FLEXE_ELF="$profile_dir/esp32-firmware.elf"
export AGON_FLEXE_ENGINE="${AGON_FLEXE_ENGINE:-jit}"
export AGON_FLEXE_ARTIFACTS="$profile_dir/last-run"
mkdir -p "$AGON_FLEXE_ARTIFACTS"
'''
        script=script.replace('\nvdp_name=',env+'\nvdp_name=',1)
    launcher.write_text(script);launcher.chmod(0o755)
    inputs+=['fab-agon-emulator','sdcard/autoexec.txt','sdcard/flower/flower.bin','sdcard/flower/flower.golem','sdcard/flower/main.cpp']
    if not stock:
        canonical.name_flexe_profile(profile)
        inputs=[name.replace('fab-agon-emulator','flexe-agon-emulator') for name in inputs]
    (profile/'.runtime.sha256').write_text(''.join(sha(profile/name)+'  '+name+'\n' for name in inputs))
    receipt={'layout':layout,'stock':stock,'start_mode':start_mode,'legacy':legacy,'busy':busy,'profile':str(profile),'stopped':True,
             'identities':{name:sha(profile/name) for name in inputs}}
    evidence=HERE/'evidence';evidence.mkdir(exist_ok=True)
    (evidence/('stock-profile.json' if stock else 'review-profile.json')).write_text(json.dumps(receipt,indent=2)+'\n')
    print('Prepared stopped profile:',profile)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--layout',default='default',choices=('default','relocated'));p.add_argument('--stock',action='store_true')
    a=p.parse_args();prepare(a.layout,a.stock)
