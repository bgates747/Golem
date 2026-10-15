"""Prepare an isolated Fab/Flexe profile using the canonical profile generator."""
from __future__ import annotations
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
from runpy import run_path
import shutil

HERE = Path(__file__).resolve().parent
PROJECT = HERE.parents[3]
TASK = HERE.parent
PROFILE = TASK / '.emulator'

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def prepare(profile, fab, adapter, firmware, elf=None, frontend=None, modules=None, setup_script=None):
    spec = importlib.util.spec_from_file_location('agon_profiles',
             setup_script or PROJECT.parent / 'agon-dev-env/scripts/setup_emulator.py')
    canonical = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(canonical)
    sdcard = canonical.link_shared_runtime(profile, fab)
    if frontend:
        canonical.replace_symlink(profile / 'fab-agon-emulator.bin', frontend)
    inputs = [('flexe-vdp.so', adapter), ('esp32-firmware.bin', firmware)]
    if elf:
        inputs.append(('esp32-firmware.elf', elf))
    for name, source in inputs:
        if source.resolve() != (profile / name).resolve():
            shutil.copyfile(source, profile / name)
    app = sdcard / 'vdplet'
    app.mkdir(exist_ok=True)
    for name in ('probe.bin', 'script.bin', 'expected.json'):
        shutil.copyfile(HERE / 'build' / name, app / name)
    meta = json.loads((HERE / 'build/expected.json').read_text())
    for name in meta['modules']:
        shutil.copyfile((modules or PROJECT / 'docs/tasks/GOLEM-002/proof/build') / name, app / name)
    extra_names = []
    if meta.get('packed_asset'):
        for name in ('loderunner.packed', 'assets.json'):
            shutil.copyfile(HERE / 'build/apple-assets' / name, app / name)
            extra_names.append(name)
    (app / 'results.dat').unlink(missing_ok=True)
    (sdcard / 'autoexec.txt').write_bytes(b'SET KEYBOARD 1\r\ncd /vdplet\r\nload probe.bin\r\nrun .\r\n')
    (profile / '.bespoke-vdp-profile').write_text('flexe-vdp.so\n')
    identity_names = (['fab-agon-emulator.bin', 'mos_platform.bin', 'mos_platform.map'] +
                      [name for name, _ in inputs] +
                      [f'sdcard/vdplet/{name}' for name in
                       ['probe.bin', 'script.bin', 'expected.json', *meta['modules'], *extra_names]])
    canonical.write_profile_launcher(profile, strict_bespoke=True)
    # Extend the generated canonical wrapper in this task only. Direct profile
    # entry must carry its own actual-firmware selection; no outer shell setup.
    launcher = profile / 'fab-agon-emulator'
    environment = '''
export AGON_FLEXE_FIRMWARE="$profile_dir/esp32-firmware.bin"
export AGON_FLEXE_ENGINE="${AGON_FLEXE_ENGINE:-jit}"
export AGON_FLEXE_ARTIFACTS="$profile_dir/last-run"
mkdir -p "$AGON_FLEXE_ARTIFACTS"
rm -f -- "$profile_dir/sdcard/vdplet/results.dat" "$profile_dir/sdcard/vdplet/failure.dat"
'''
    if elf:
        environment += 'export AGON_FLEXE_ELF="$profile_dir/esp32-firmware.elf"\n'
    else:
        environment += 'unset AGON_FLEXE_ELF\n'
    script = launcher.read_text()
    assert script.count('\nvdp_name=') == 1
    launcher.write_text(script.replace('\nvdp_name=', environment + '\nvdp_name=', 1))
    identity_names.append('fab-agon-emulator')
    (profile / '.runtime.sha256').write_text(''.join(f'{sha(profile/name)}  {name}\n' for name in identity_names))
    # Apply current product naming after the frozen template and pins.
    naming = run_path(str(Path.home() / 'Agon/mystuff/agon-dev-env/scripts/setup_emulator.py'))
    naming['name_flexe_profile'](profile)
    print(f'Prepared stopped profile: {profile}')
    return profile

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--profile', type=Path, default=PROFILE)
    p.add_argument('--fab', type=Path, default=PROJECT / '.emulator/runtime/fab-1.2.5')
    p.add_argument('--adapter', type=Path, default=TASK / 'emulator/build/flexe-vdp.so')
    p.add_argument('--frontend', type=Path, help='Owned Fab frontend binary; standard Fab supplies shared assets')
    p.add_argument('--firmware', type=Path, required=True)
    p.add_argument('--elf', type=Path)
    p.add_argument('--modules', type=Path, help='Alternative directory of compiled module packages')
    p.add_argument('--setup-script', type=Path, help='Exact canonical profile-generator snapshot on a staged host')
    a = p.parse_args()
    prepare(a.profile.resolve(), a.fab.resolve(), a.adapter.resolve(), a.firmware.resolve(),
            a.elf.resolve() if a.elf else None, a.frontend.resolve() if a.frontend else None,
            a.modules.resolve() if a.modules else None,
            a.setup_script.resolve() if a.setup_script else None)
