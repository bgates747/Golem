"""Stage this task's stopped Oryx Flexe profile using accepted runtime bytes."""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import shutil

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]


def prepare(variant):
    profile = HERE / ('.emulator' if variant == 'review' else '.emulator-' + variant)
    setup = HERE / 'build/setup_emulator.py'
    if not setup.exists(): setup = ROOT.parent / 'agon-dev-env/scripts/setup_emulator.py'
    spec = importlib.util.spec_from_file_location('canonical', setup)
    canonical = importlib.util.module_from_spec(spec); spec.loader.exec_module(canonical)
    sd = canonical.link_shared_runtime(profile, ROOT / '.emulator/runtime/fab-1.2.5')
    runtime = ROOT / 'docs/tasks/GOLEM-037/binding/build'
    canonical.replace_symlink(profile / 'fab-agon-emulator.bin', runtime / 'runtime/flexe-agon-emulator')
    for name, source in [('flexe-vdp.so', runtime / 'runtime/flexe-vdp.so'),
                         ('esp32-firmware.bin', runtime / 'firmware-stage-b/firmware.bin'),
                         ('esp32-firmware.elf', runtime / 'firmware-stage-b/firmware.elf')]:
        shutil.copyfile(source, profile / name)
    (profile / '.bespoke-vdp-profile').write_text('flexe-vdp.so\n')
    app = sd / 'golem041'; app.mkdir(exist_ok=True)
    for name in ['probe.bin', 'script.bin', 'expected.json', 'expected.dat', 'mask.dat']:
        shutil.copyfile(HERE / 'build' / variant / name, app / name)
    for name in ['probe.golem', 'animation.golem']: shutil.copyfile(HERE / name, app / name)
    for name in ['results.dat', 'failure.dat']: (app / name).unlink(missing_ok=True)
    (sd / 'autoexec.txt').write_bytes(b'SET KEYBOARD 1\r\ncd /golem041\r\nload probe.bin\r\nrun .\r\n')
    canonical.write_profile_launcher(profile, strict_bespoke=True)
    launcher = profile / 'fab-agon-emulator'; script = launcher.read_text()
    environment = '''
rm -f -- "$profile_dir/sdcard/golem041/results.dat" "$profile_dir/sdcard/golem041/failure.dat"
export AGON_FLEXE_FIRMWARE="$profile_dir/esp32-firmware.bin"
export AGON_FLEXE_ELF="$profile_dir/esp32-firmware.elf"
export AGON_FLEXE_ENGINE="${AGON_FLEXE_ENGINE:-jit}"
export AGON_FLEXE_ARTIFACTS="$profile_dir/last-run"
mkdir -p "$AGON_FLEXE_ARTIFACTS"
'''
    assert script.count('\nvdp_name=') == 1
    script = script.replace('\nvdp_name=', environment + '\nvdp_name=', 1)
    launcher.unlink(); launcher = profile / 'flexe-agon-emulator'
    launcher.write_text(script); launcher.chmod(0o755)
    inputs = ['fab-agon-emulator.bin', 'flexe-vdp.so', 'esp32-firmware.bin', 'esp32-firmware.elf',
              'mos_platform.bin', 'mos_platform.map', 'flexe-agon-emulator', 'sdcard/autoexec.txt']
    inputs += ['sdcard/golem041/' + name for name in
               ['probe.bin', 'script.bin', 'expected.json', 'expected.dat', 'mask.dat', 'probe.golem', 'animation.golem']]
    (profile / '.runtime.sha256').write_text(''.join(hashlib.sha256((profile / name).read_bytes()).hexdigest() +
                                                   '  ' + name + '\n' for name in inputs))
    print('Prepared stopped profile:', profile)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('variant', choices=('hybrid', 'relocated', 'review'))
    prepare(parser.parse_args().variant)
