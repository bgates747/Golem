#!/usr/bin/env python3
"""Build and qualify streamed Golem packages; stage a stopped review profile."""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import os
import shutil
import signal
import struct
import subprocess
import sys
import time
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
BUILD=HERE/'build'
PROFILE=ROOT/'.emulator/golem-046'
API=ROOT.parent/'agon-utils/examples/agnb/api'
ASSEMBLER=ROOT/'docs/tasks/GOLEM-044/native-math/build/assembler/bin/ez80asm'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()


def active_processes():
    active=[]
    for proc in Path('/proc').glob('[0-9]*'):
        try:
            if str(PROFILE/'sdcard').encode() in (proc/'cmdline').read_bytes().split(b'\0'):active.append(int(proc.name))
        except OSError:pass
    return active


def run(*args,**kw):subprocess.run(list(map(str,args)),check=True,**kw)
def packet(id,op):return bytes((23,0,160))+struct.pack('<HB',id,op)
def request(op,fields):return bytes((23,0,216))+struct.pack('<HBBHBB',6+len(fields),1,op,1,1,0)+fields

def build(native=False):
    BUILD.mkdir(exist_ok=True)
    run(ROOT/'build/golemc','--portable','--emit-load-plan',HERE/'large.golem',BUILD/'large')
    if native:
        run(sys.executable,ROOT.parent/'agon-vdplet/tools/build_module.py',BUILD/'large/native.c','--name','large','--output',BUILD/'large')
    package=[sys.executable,ROOT/'tools/package_agnb.py',BUILD/'large']
    if native:package+=['--module',BUILD/'large/large.vdpl']
    run(*package)
    run(ROOT/'build/golemc','--resident','--emit-load-plan','--reserve','1:4095','--reserve','60000:60031',HERE/'review.golem',BUILD/'visual')
    run(sys.executable,ROOT/'tools/package_agnb.py',BUILD/'visual')
    backend='native' if native else 'buffered'
    plan=json.loads((BUILD/'large/load-plan.json').read_text())
    c=next(c for c in plan['components'] if c['name']==backend)
    info=json.loads((BUILD/'large/interface.json').read_text())
    total=info['globals'][0]['offset']
    # Four status bytes + total and generation; observations independently checked.
    offsets=[total,total+1,12,13,14,15,20,21,22,23]
    def row(total,generation):return struct.pack('<HHHI',total,0,0,generation)
    expected=bytearray()
    scenario=[]
    params=['state_offsets: db '+','.join(map(str,offsets)),'state_offsets_end:']
    def append(cmd):scenario.append(cmd)
    for attempt in range(2):
        append(' call golem_agnb_'+backend+'_load\n call append_a')
        expected+=b'\0'
        append(' ld hl,golem_state_buffer\n ld (copy_id),hl ; ADL24 third byte is copy_offset, reset by observe_state\n call observe_state')
        expected+=row(0,0)
        if native:
            append(' ld hl,idle_packet\n ld bc,idle_packet_end-idle_packet\n call request\n ld a,(reply+8)\n call append_a\n ld a,(reply+9)\n call append_a')
            expected+=b'\0\0'
            append(' ld hl,load_packet\n ld bc,load_packet_end-load_packet\n call request\n call capture_handle\n call patch_handle')
        append(' call golem_agnb_'+backend+'_setup')
        for total_value,generation in ((45,1),(90,2)):
            append(' xor a\n call golem_'+backend+'_tick')
            if native:
                append(' call runtime_ok');expected+=b'\0\0'
            append(' call observe_state');expected+=row(total_value,generation)
        if native:append(' call unload_module')
        append(' call golem_agnb_'+backend+'_cleanup\n call observe_state')
        expected+=b'\x55'*10
    # The test picture comes only from the separately identified Golem source.
    append(' call golem_agnb_stock_load\n call append_a\n or a\n jp nz,failed')
    expected+=b'\0'
    append(' call compare_results\n ld hl,display_setup\n ld bc,display_setup_end-display_setup\n call send\n call golem_agnb_stock_setup\n xor a\n call golem_drawPackage\n call golem_agnb_stock_cleanup')
    # Drawing commands have copied their values to Canvas. A pixel query fences it.
    append(' res 2,(iy+4)\n ld hl,pixel_query\n ld bc,7\n call send\n ld a,(iy+0)\n ld d,a')
    append('wait_pixel:\n bit 2,(iy+4)\n jr nz,pixel_done\n ld a,(iy+0)\n sub d\n cp 240\n jp nc,failed\n jr wait_pixel\npixel_done:')
    # Don't bake pixel sysvar offsets here; the fence itself ensures ordering.
    (BUILD/'scenario.inc').write_text('\n'.join(scenario)+'\n')
    def rawlabel(name,raw):return name+':\n'+''.join(' db '+','.join(map(str,raw[i:i+24]))+'\n' for i in range(0,len(raw),24))+name+'_end:\n'
    params.append(rawlabel('idle_packet',request(4,struct.pack('<H',2))))
    params.append(rawlabel('info_packet',request(4,struct.pack('<H',3))))
    params.append(rawlabel('load_packet',request(1,struct.pack('<HIH',c['module_buffer'],(BUILD/'large/large.vdpl').stat().st_size if native else 1,0))))
    unload=request(3,b'\0\0')
    params+=['unload_packet: db '+','.join(map(str,unload[:-2])),'unload_handle: dw 0','unload_packet_end:']
    write=packet(c['entries'][0],5)+bytes((0xc2,24,0,2,0))
    params+=['handle_write: db '+','.join(map(str,write)),'handle_write_value: dw 0','handle_write_end:']
    # Mode0 640x480, white on black, physical graphics coordinates, top text pane.
    display=bytes((22,0,23,0,192,0,12,17,15,17,128))
    display+=b'GOLEM-046: streamed '+backend.encode()+b' package\r\n'
    display+=b'large.golem: nine 60,000-byte arrays; totals 45 then 90.\r\n'
    display+=b'review.golem: draws the nine-tier white pyramid below.\r\n'
    params.append(rawlabel('display_setup',display))
    params+=['pixel_query: db 23,0,132,64,1,48,0','success: db 31,0,40,"All streamed-package checks passed; assembly owns loading/calls.",13,10',
             ' db "Type dir at the MOS prompt and report to the agent that it works.",13,10','success_end:']
    (BUILD/'parameters.inc').write_text('\n'.join(params)+'\n')
    (BUILD/'expected.dat').write_bytes(expected)
    run(ASSEMBLER,'guest.asm','build/probe.bin','-c',cwd=HERE)
    return backend


def prepare(runtime,native=False):
    backend='native' if native else 'buffered'
    if active_processes():raise RuntimeError('Close the GOLEM-046 profile before staging')
    if PROFILE.exists() and not (PROFILE/'.golem-046').exists():raise RuntimeError('Unowned profile')
    # Only this owned profile may be restaged. Verify any previous identity first.
    if (PROFILE/'.runtime.sha256').exists():
        run('sha256sum','--check','--status','.runtime.sha256',cwd=PROFILE)
        (PROFILE/'.runtime.sha256').unlink()
    for name in ('flexe-agon-emulator','flexe-agon-emulator.bin'):
        (PROFILE/name).unlink(missing_ok=True)
    spec=importlib.util.spec_from_file_location('canonical',ROOT.parent/'agon-dev-env/scripts/setup_emulator.py')
    canonical=importlib.util.module_from_spec(spec);spec.loader.exec_module(canonical)
    sd=canonical.link_shared_runtime(PROFILE,runtime)
    (PROFILE/'.golem-046').write_text('Owned GOLEM-046 streamed package fixture\n')
    pins=['fab-agon-emulator','fab-agon-emulator.bin','mos_platform.bin','mos_platform.map']
    if native:
        source=ROOT/'docs/tasks/GOLEM-037/binding/build/runtime'
        assert sha(source/'flexe-agon-emulator')=='28f50e87190f00de4039dc90c94432347f01c4771c4ea4a5dea8595d6cef7670'
        assert sha(source/'flexe-vdp.so')=='200f28cbe399b1fdc60cba1e5fed4c2bb86042b5e92f6e422539a5388c079a30'
        target=PROFILE/'runtime';target.mkdir(exist_ok=True)
        shutil.copyfile(source/'flexe-agon-emulator',target/'flexe-agon-emulator');(target/'flexe-agon-emulator').chmod(0o755)
        canonical.replace_symlink(PROFILE/'fab-agon-emulator.bin',target/'flexe-agon-emulator')
        shutil.copyfile(source/'flexe-vdp.so',PROFILE/'flexe-vdp.so')
        firmware_pins={'firmware.bin':'8da2ebfdfa3b5b00f4a1929ad4885c838845b160f570259792ef370b69d4f473',
                       'firmware.elf':'b41e4cb37bd44633d14b2e6eea615cbbbdd29a1d4b49c3258b4f49f2e215e567'}
        for name in ('firmware.bin','firmware.elf'):
            assert sha(ROOT/'docs/tasks/GOLEM-044/app/build/runtime-inputs'/name)==firmware_pins[name]
            shutil.copyfile(ROOT/'docs/tasks/GOLEM-044/app/build/runtime-inputs'/name,PROFILE/('esp32-'+name))
        (PROFILE/'.bespoke-vdp-profile').write_text('flexe-vdp.so\n')
        canonical.write_profile_launcher(PROFILE,strict_bespoke=True)
        launcher=PROFILE/'fab-agon-emulator';script=launcher.read_text()
        env='''
export AGON_FLEXE_FIRMWARE="$profile_dir/esp32-firmware.bin"
export AGON_FLEXE_ELF="$profile_dir/esp32-firmware.elf"
export AGON_FLEXE_ENGINE="${AGON_FLEXE_ENGINE:-jit}"
export AGON_FLEXE_ARTIFACTS="$profile_dir/last-run"
mkdir -p "$AGON_FLEXE_ARTIFACTS"
'''
        launcher.write_text(script.replace('\nvdp_name=',env+'\nvdp_name=',1))
        canonical.name_flexe_profile(PROFILE)
        pins=[p.replace('fab-agon-emulator','flexe-agon-emulator') for p in pins]+['flexe-vdp.so','esp32-firmware.bin','esp32-firmware.elf']
    else:
        (PROFILE/'.bespoke-vdp-profile').unlink(missing_ok=True)
        pins+=['firmware/vdp_platform.so']
    # Each launch starts a fresh review transcript and rejects altered inputs.
    launcher=PROFILE/('flexe-agon-emulator' if native else 'fab-agon-emulator')
    launch=launcher.read_text().replace('cd "${profile_dir}"\n',
        'cd "${profile_dir}"\nsha256sum --check --status .runtime.sha256\n'
        'rm -f -- sdcard/golem046/results.dat sdcard/golem046/failure.dat\n',1)
    launcher.write_text(launch)
    app=sd/'golem046';app.mkdir(exist_ok=True)
    files={name:BUILD/'large'/name for name in ('program-buffered.agnb','program-native.agnb') if (BUILD/'large'/name).exists()}
    files.update({'program-stock.agnb':BUILD/'visual/program-stock.agnb','probe.bin':BUILD/'probe.bin','large.golem':HERE/'large.golem','review.golem':HERE/'review.golem'})
    for name,path in files.items():shutil.copyfile(path,app/name);pins.append('sdcard/golem046/'+name)
    for name in ('results.dat','failure.dat'):(app/name).unlink(missing_ok=True)
    (sd/'autoexec.txt').write_bytes(b'SET KEYBOARD 1\r\ncd /golem046\r\nload probe.bin\r\nrun .\r\n')
    pins+=['sdcard/autoexec.txt']
    (PROFILE/'.runtime.sha256').write_text(''.join(sha(PROFILE/name)+'  '+name+'\n' for name in pins))
    expected=(BUILD/'expected.dat').read_bytes()
    container=app/('program-'+backend+'.agnb')
    metadata={'backend':backend,'profile':str(PROFILE),'container_bytes':container.stat().st_size,'app_bytes':(app/'probe.bin').stat().st_size,
              'transfer_window_bytes':8192,'expected_bytes':len(expected),'pins':{name:sha(PROFILE/name) for name in pins},'stopped':True}
    assert metadata['container_bytes']>0x70000 and metadata['app_bytes']<65536
    (BUILD/'profile.json').write_text(json.dumps(metadata,indent=2)+'\n')
    return metadata


def execute(meta,engine='jit'):
    env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',AGON_FLEXE_ENGINE=engine)
    for key in ('LD_PRELOAD','BASH_ENV'):env.pop(key,None)
    target=PROFILE/'sdcard/golem046';result=target/'results.dat';failure=target/'failure.dat'
    for p in (result,failure):p.unlink(missing_ok=True)
    start=time.monotonic();launcher='./flexe-agon-emulator' if meta['backend']=='native' else './fab-agon-emulator'
    with (BUILD/'emulator.log').open('w') as log:
        process=subprocess.Popen([launcher],cwd=PROFILE,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
        try:
            while not result.exists() or result.stat().st_size<meta['expected_bytes']:
                if failure.exists():raise RuntimeError('Guest failed: '+failure.read_bytes().hex())
                if process.poll() is not None:raise RuntimeError('Emulator exited before results')
                if time.monotonic()-start>180:raise TimeoutError('See build/emulator.log')
                time.sleep(.05)
            if meta['backend']=='native':
                wire=PROFILE/'last-run/mos-to-vdp.bin';deadline=time.monotonic()+10
                while b'/golem046 *' not in wire.read_bytes().partition(b'Type dir at the MOS prompt')[2]:
                    if time.monotonic()>deadline:raise TimeoutError('No actual MOS return after results')
                    time.sleep(.05)
                time.sleep(1.0) # Let presentation catch up after the finite Canvas fence.
        finally:
            if process.poll() is None:
                os.killpg(process.pid,signal.SIGTERM)
                try:process.wait(timeout=5)
                except subprocess.TimeoutExpired:os.killpg(process.pid,signal.SIGKILL);process.wait()
    # The generated shell wrapper may exit before its native child finishes
    # shutdown. Wait for the owned profile to stop before restaging/claiming it.
    drain=time.monotonic()+10
    while active_processes():
        if time.monotonic()>drain:raise RuntimeError('Owned emulator child did not finish shutdown')
        time.sleep(.02)
    actual=result.read_bytes();expected=(BUILD/'expected.dat').read_bytes()
    assert actual==expected,(actual.hex(),expected.hex())
    run('sha256sum','--check','--status','.runtime.sha256',cwd=PROFILE)
    stats={}
    if meta['backend']=='native':
        stats=json.loads((PROFILE/'last-run/stats.json').read_text())
        assert not any(stats.get(key) for key in ('failed','unhandled','unregistered','unmapped')),stats
        raw=(PROFILE/'last-run/final.ppm').read_bytes();header,pixels=raw.split(b'\n255\n',1)
        assert header==b'P6\n640 480' and len(pixels)==640*480*3
        for x,y in ((320,48),(104,280),(536,280),(320,74)):
            at=(y*640+x)*3;assert pixels[at:at+3]==b'\xff\xff\xff',(x,y,pixels[at:at+3])
        assert pixels[(200*640+320)*3:(200*640+320)*3+3]==b'\0\0\0'

    evidence=dict(meta,passed=True,backend_stats=stats,mos_return_observed=meta['backend']=='native',engine=engine if meta['backend']=='native' else 'native-stock-vdp',host=os.uname().nodename,
                  elapsed_seconds=round(time.monotonic()-start,3),graphical_launch=False,physical=False,human_review='pending')
    (HERE/(os.uname().nodename+'-'+meta['backend']+'-'+engine+'-evidence.json')).write_text(json.dumps(evidence,indent=2)+'\n')
    result.unlink();print('Streamed Golem qualification passes:',evidence['host'],evidence['backend'],evidence['engine'],evidence['elapsed_seconds'],'seconds',flush=True)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--runtime',type=Path,required=True);p.add_argument('--native',action='store_true')
    p.add_argument('--no-build',action='store_true');p.add_argument('--stage-only',action='store_true');p.add_argument('--engine',choices=('jit','interp'),default='jit')
    a=p.parse_args()
    if not a.no_build:build(a.native)
    metadata=prepare(a.runtime.resolve(),a.native)
    if not a.stage_only:execute(metadata,a.engine)
