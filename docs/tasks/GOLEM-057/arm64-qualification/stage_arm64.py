#!/usr/bin/env python3
"""Stage one new stopped canonical ARM64 profile after a root artifact pin."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import platform
import shutil
import struct
import sys

SDK = Path('/Agon/mystuff/agon-vdplet')
PROFILE = SDK / '.emulator/vdpl012-arm64-qualification'
FIRMWARE = SDK / 'build/vdpl-012/rows-r1/firmware-src/.pio/build/esp32dev/firmware.bin'
SETUP = SDK.parent / 'agon-dev-env/scripts/setup_emulator.py'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def arm64(path):
    header = path.read_bytes()[:20]
    assert header[:6] == b'\x7fELF\x02\x01'
    assert struct.unpack_from('<H', header, 18)[0] == 183


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--backend', type=Path, required=True)
    parser.add_argument('--backend-sha256', required=True)
    parser.add_argument('--runtime', type=Path,
                        default=Path('/Agon/mystuff/golem/.emulator/runtime/fab-1.2.5'))
    parser.add_argument('--frontend', type=Path,
                        default=SDK / '.emulator/vdpl-009e/runtime/flexe-agon-emulator')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert platform.machine().lower() in ('aarch64', 'arm64')
    assert not PROFILE.exists(), 'preserve any existing profile'
    assert sha(args.backend) == args.backend_sha256
    arm64(args.backend)
    arm64(args.frontend)
    assert sha(FIRMWARE) == '8d980025f891484cf7e6cc5f2992cf517cb80e0123ff7de712ae8d7f2dc1c807'
    assert sha(FIRMWARE.with_suffix('.elf')) == '1a7c44482d4ba8c5bfb49976c0bd664b9a9c2a635c7027b30954929ca567f814'
    assert sha(SETUP) == '844ab629cb604daf2469c2c1c9c5dab89d8e9abc9c62f0a19c3eceb6f960a98a'
    sys.dont_write_bytecode = True
    spec = importlib.util.spec_from_file_location('canonical_setup', SETUP)
    setup = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(setup)
    sdcard = setup.link_shared_runtime(PROFILE, args.runtime)
    runtime = PROFILE / 'runtime'
    runtime.mkdir()
    shutil.copyfile(args.frontend, runtime / 'flexe-agon-emulator')
    (runtime / 'flexe-agon-emulator').chmod(0o755)
    setup.replace_symlink(PROFILE / 'fab-agon-emulator.bin', runtime / 'flexe-agon-emulator')
    shutil.copyfile(args.backend, PROFILE / 'flexe-vdp.so')
    shutil.copyfile(FIRMWARE, PROFILE / 'esp32-firmware.bin')
    shutil.copyfile(FIRMWARE.with_suffix('.elf'), PROFILE / 'esp32-firmware.elf')
    (PROFILE / '.bespoke-vdp-profile').write_text('flexe-vdp.so\n')
    (PROFILE / '.vdpl012-arm64-qualification').write_text('Owned Pi ARM64 profile; staged stopped.\n')
    setup.write_profile_launcher(PROFILE, strict_bespoke=True)
    launcher = PROFILE / 'fab-agon-emulator'
    env = ('\nexport AGON_FLEXE_FIRMWARE="$profile_dir/esp32-firmware.bin"\n'
           'export AGON_FLEXE_ELF="$profile_dir/esp32-firmware.elf"\n'
           'export AGON_FLEXE_ENGINE="${AGON_FLEXE_ENGINE:-jit}"\n'
           'export AGON_FLEXE_ARTIFACTS="$profile_dir/last-run/manual"\n'
           'mkdir -p "$AGON_FLEXE_ARTIFACTS"\n')
    launcher.write_text(launcher.read_text().replace('\nvdp_name=', env + '\nvdp_name=', 1))
    setup.name_flexe_profile(PROFILE)
    (sdcard / 'autoexec.txt').write_bytes(b'SET KEYBOARD 1\r\n')
    files = ('flexe-agon-emulator', 'flexe-agon-emulator.bin', 'mos_platform.bin',
             'mos_platform.map', 'flexe-vdp.so', 'esp32-firmware.bin',
             'esp32-firmware.elf', 'sdcard/autoexec.txt')
    pins = {name: sha(PROFILE / name) for name in files}
    (PROFILE / '.runtime.sha256').write_text(''.join(digest + '  ' + name + '\n' for name, digest in pins.items()))
    result = dict(stopped=True, emulator_instantiated=False, architecture=platform.machine(),
                  profile=str(PROFILE), pins=pins, canonical_setup_sha256=sha(SETUP),
                  scope='Staging only. Root Pi permit still required before direct-library execution; no Oryx lease.')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
