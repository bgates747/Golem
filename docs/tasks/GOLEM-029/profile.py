"""Prepare isolated, stopped stock or uploaded-Xtensa review profiles."""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import shutil

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]

def prepare(variant):
    profile=HERE/('.emulator' if variant=='review' else '.emulator-'+variant)
    stock=variant in ('stock','meter-stock')
    setup=HERE/'build/setup_emulator.py'
    if not setup.exists():setup=ROOT.parent/'agon-dev-env/scripts/setup_emulator.py'
    spec=importlib.util.spec_from_file_location('canonical',setup)
    canonical=importlib.util.module_from_spec(spec);spec.loader.exec_module(canonical)
    sdcard=canonical.link_shared_runtime(profile,ROOT/'.emulator/runtime/fab-1.2.5')
    inputs=['fab-agon-emulator.bin','mos_platform.bin','mos_platform.map']
    if not stock:
        previous=ROOT/'docs/tasks/GOLEM-021'
        canonical.replace_symlink(profile/'fab-agon-emulator.bin',previous/'emulator/build/fab-source/target/release/fab-agon-emulator')
        for name,source in [('flexe-vdp.so',previous/'emulator/build/flexe-vdp.so'),
                            ('esp32-firmware.bin',previous/'.emulator/esp32-firmware.bin'),
                            ('esp32-firmware.elf',previous/'.emulator/esp32-firmware.elf')]:
            shutil.copyfile(source,profile/name);inputs.append(name)
        (profile/'.bespoke-vdp-profile').write_text('flexe-vdp.so\n')
    else:
        inputs.append('firmware/vdp_platform.so')
    app=sdcard/'golem029';app.mkdir(exist_ok=True)
    for name in ('probe.bin','script.bin','expected.json','expected.dat','mask.dat'):
        shutil.copyfile(HERE/'build'/variant/name,app/name)
        inputs.append('sdcard/golem029/'+name)
    for name in ('lights.golem','bools.golem'):
        shutil.copyfile(HERE/name,app/name);inputs.append('sdcard/golem029/'+name)
    for name in ('results.dat','failure.dat'):(app/name).unlink(missing_ok=True)
    (sdcard/'autoexec.txt').write_bytes(b'SET KEYBOARD 1\r\ncd /golem029\r\nload probe.bin\r\nrun .\r\n')
    inputs.append('sdcard/autoexec.txt')
    canonical.write_profile_launcher(profile,strict_bespoke=not stock)
    launcher=profile/'fab-agon-emulator'
    script=launcher.read_text()
    environment='\nrm -f -- "$profile_dir/sdcard/golem029/results.dat" "$profile_dir/sdcard/golem029/failure.dat"\n'
    if not stock:
        environment+='''export AGON_FLEXE_FIRMWARE="$profile_dir/esp32-firmware.bin"
export AGON_FLEXE_ELF="$profile_dir/esp32-firmware.elf"
export AGON_FLEXE_ENGINE="${AGON_FLEXE_ENGINE:-jit}"
export AGON_FLEXE_ARTIFACTS="$profile_dir/last-run"
mkdir -p "$AGON_FLEXE_ARTIFACTS"
'''
        assert script.count('\nvdp_name=')==1
        script=script.replace('\nvdp_name=',environment+'\nvdp_name=',1)
    else:
        # Canonical stock launcher has no bespoke selection clause.
        marker='profile_dir=';start=script.index('\n',script.index(marker))
        script=script[:start+1]+environment+script[start+1:]
    launcher.write_text(script)
    inputs.append('fab-agon-emulator')
    (profile/'.runtime.sha256').write_text(''.join(hashlib.sha256((profile/name).read_bytes()).hexdigest()+'  '+name+'\n' for name in inputs))
    print('Prepared stopped profile:',profile)
    return profile

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('variant',choices=('stock','meter-stock','hybrid','relocated','review'))
    prepare(parser.parse_args().variant)
