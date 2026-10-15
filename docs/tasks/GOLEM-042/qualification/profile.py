"""Stage this task's stopped Oryx Flexe profile using accepted runtime bytes."""
from pathlib import Path
from runpy import run_path
import argparse
import hashlib
import importlib.util
import shutil

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]


def prepare(variant):
    profile = HERE / ('.emulator' if variant == 'review' else '.emulator-' + variant)
    stock = variant in ('stock','stock-relocated')
    setup = HERE / 'build/setup_emulator.py'
    if not setup.exists(): setup = ROOT.parent / 'agon-dev-env/scripts/setup_emulator.py'
    spec = importlib.util.spec_from_file_location('canonical', setup)
    canonical = importlib.util.module_from_spec(spec); spec.loader.exec_module(canonical)
    sd = canonical.link_shared_runtime(profile, ROOT / '.emulator/runtime/fab-1.2.5')
    inputs = ['fab-agon-emulator.bin','mos_platform.bin','mos_platform.map']
    if not stock:
        runtime = ROOT / 'docs/tasks/GOLEM-037/binding/build'
        canonical.replace_symlink(profile / 'fab-agon-emulator.bin', runtime / 'runtime/flexe-agon-emulator')
        for name, source in [('flexe-vdp.so', runtime / 'runtime/flexe-vdp.so'),
                             ('esp32-firmware.bin', runtime / 'firmware-stage-b/firmware.bin'),
                             ('esp32-firmware.elf', runtime / 'firmware-stage-b/firmware.elf')]:
            shutil.copyfile(source, profile / name);inputs.append(name)
        (profile / '.bespoke-vdp-profile').write_text('flexe-vdp.so\n')
    else: inputs.append('firmware/vdp_platform.so')
    app = sd / 'golem042'; app.mkdir(exist_ok=True)
    for name in ['probe.bin', 'script.bin', 'expected.json', 'expected.dat', 'mask.dat']:
        shutil.copyfile(HERE / 'build' / variant / name, app / name)
    for name in ['probe.golem', 'animation.golem']: shutil.copyfile(HERE / name, app / name)
    for name in ['results.dat', 'failure.dat']: (app / name).unlink(missing_ok=True)
    (sd / 'autoexec.txt').write_bytes(b'SET KEYBOARD 1\r\ncd /golem042\r\nload probe.bin\r\nrun .\r\n')
    canonical.write_profile_launcher(profile, strict_bespoke=not stock)
    launcher = profile / 'fab-agon-emulator'; script = launcher.read_text()
    environment = '''
rm -f -- "$profile_dir/sdcard/golem042/results.dat" "$profile_dir/sdcard/golem042/failure.dat"
'''
    if not stock:
        environment += '''
export AGON_FLEXE_FIRMWARE="$profile_dir/esp32-firmware.bin"
export AGON_FLEXE_ELF="$profile_dir/esp32-firmware.elf"
export AGON_FLEXE_ENGINE="${AGON_FLEXE_ENGINE:-jit}"
export AGON_FLEXE_ARTIFACTS="$profile_dir/last-run"
mkdir -p "$AGON_FLEXE_ARTIFACTS"
'''
        assert script.count('\nvdp_name=') == 1
        script = script.replace('\nvdp_name=', environment + '\nvdp_name=', 1)
    else:
        start=script.index('\n',script.index('profile_dir='))
        script=script[:start+1]+environment+script[start+1:]
    launcher.write_text(script)
    inputs += ['fab-agon-emulator', 'sdcard/autoexec.txt']
    inputs += ['sdcard/golem042/' + name for name in
               ['probe.bin', 'script.bin', 'expected.json', 'expected.dat', 'mask.dat', 'probe.golem', 'animation.golem']]
    (profile / '.runtime.sha256').write_text(''.join(hashlib.sha256((profile / name).read_bytes()).hexdigest() +
                                                   '  ' + name + '\n' for name in inputs))
    # Apply current product naming after the frozen template and pins.
    if not stock:
        naming = run_path(str(Path.home() / 'Agon/mystuff/agon-dev-env/scripts/setup_emulator.py'))
        naming['name_flexe_profile'](profile)
    print('Prepared stopped profile:', profile)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('variant', choices=('stock', 'stock-relocated', 'hybrid', 'relocated', 'review'))
    prepare(parser.parse_args().variant)
