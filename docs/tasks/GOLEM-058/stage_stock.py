#!/usr/bin/env python3
"""Build a new stopped stock-firmware Flexe instance; preserve old profiles."""
import argparse
import hashlib
import importlib.util
import json
import shutil
from pathlib import Path

PINS = {'flexe-agon-emulator.bin': '7e78e991a32c7bc94e84d70e8ad1792e3b840c48adc2e3aae7f635a88f4878ad',
        'flexe-vdp.so': '200f28cbe399b1fdc60cba1e5fed4c2bb86042b5e92f6e422539a5388c079a30',
        'mos_platform.bin': 'd564243283972690933a4554296ad6202ca4ef54572279533a942960846bebae',
        'mos_platform.map': 'd69e60bbce61a7b4b3eef318ba395f11c4e1a5b585755755113992dc94edcb86',
        'esp32-firmware.bin': 'b807beef35823b13a0a056f11b7464cd1b1c6356dce0e4098b78ebe059ded35f'}


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m


def stage(reference, profile, cohort):
    assert not profile.exists()
    plan = json.loads((cohort / 'cohort.json').read_text())
    for name, h in plan['files'].items():
        assert sha(cohort / name) == h, name
    for name, h in PINS.items():
        assert sha(reference / name) == h, name
    shared = Path.home() / 'Agon/mystuff/agon-dev-env'
    setup = load('setup', shared / 'scripts/setup_emulator.py')
    runtime = profile / 'owned-runtime'
    (runtime / 'firmware').mkdir(parents=True)
    shutil.copyfile(reference / 'flexe-agon-emulator.bin', runtime / 'fab-agon-emulator')
    (runtime / 'fab-agon-emulator').chmod(0o755)
    for name in ('mos_platform.bin', 'mos_platform.map'):
        shutil.copyfile(reference / name, runtime / 'firmware' / name)
    for name in ('bin', 'mos'):
        shutil.copytree(reference / 'sdcard' / name, runtime / 'sdcard' / name)
    shutil.copyfile(reference / 'esp32-firmware.bin', runtime / 'sdcard/firmware.bin')
    shutil.copyfile(reference / 'mos_platform.bin', runtime / 'sdcard/MOS.bin')
    sd = setup.link_shared_runtime(profile, runtime)
    # Replace only the new profile's MOS link before overlaying packaged moslets.
    (sd / 'mos').unlink()
    shutil.copytree(runtime / 'sdcard/mos', sd / 'mos')
    for source in (cohort / 'unpacked/sdcard').rglob('*'):
        if source.is_dir():
            (sd / source.relative_to(cohort / 'unpacked/sdcard')).mkdir(parents=True, exist_ok=True)
            continue
        if source.is_file():
            dest = sd / source.relative_to(cohort / 'unpacked/sdcard')
            dest.parent.mkdir(parents=True, exist_ok=True)
            if dest.is_symlink():
                dest.unlink()
            shutil.copyfile(source, dest)
    assert all((sd / 'golem' / name).is_dir() for name in ('cache', 'output', 'compiler', 'examples'))
    shutil.copyfile(cohort / 'autoexec.txt', sd / 'autoexec.txt')
    for name in ('flexe-vdp.so', 'esp32-firmware.bin'):
        shutil.copyfile(reference / name, profile / name)
    (profile / '.bespoke-vdp-profile').write_text('flexe-vdp.so\n')
    setup.write_profile_launcher(profile, strict_bespoke=True)
    launcher = profile / 'fab-agon-emulator'
    text = launcher.read_text().replace('FSIM runtime', 'Golem stock runtime').replace('FSIM review profile', 'Golem stock review profile')
    text = text.replace('\nvdp_name=', '\nexport AGON_FLEXE_FIRMWARE="$profile_dir/esp32-firmware.bin"\nunset AGON_FLEXE_ELF\nexport AGON_FLEXE_ENGINE=jit\nexport AGON_FLEXE_ARTIFACTS="$profile_dir/last-run"\nmkdir -p "$AGON_FLEXE_ARTIFACTS"\n\nvdp_name=', 1)
    launcher.write_text(text)
    launcher.chmod(0o755)
    setup.name_flexe_profile(profile)
    (runtime / 'fab-agon-emulator').rename(runtime / 'flexe-agon-emulator')
    setup.replace_symlink(profile / 'flexe-agon-emulator.bin', runtime / 'flexe-agon-emulator')
    checks = load('canonical_flexe', cohort / 'canonical_flexe_profile.py')
    checks.verify_profile_executables(profile / 'flexe-agon-emulator', profile / 'flexe-agon-emulator.bin')
    paths = ['flexe-agon-emulator', 'flexe-agon-emulator.bin', 'flexe-vdp.so', 'esp32-firmware.bin', 'mos_platform.bin', 'mos_platform.map', '.mos-release', '.mos-release.sha256', '.bespoke-vdp-profile', 'sdcard/autoexec.txt']
    paths += ['sdcard/' + str(p.relative_to(cohort / 'unpacked/sdcard')) for p in (cohort / 'unpacked/sdcard').rglob('*') if p.is_file()]
    pins = {n: sha(profile / n) for n in paths}
    (profile / '.runtime.sha256').write_text(''.join(h + '  ' + n + '\n' for n, h in pins.items()))
    result = dict(stopped=True, profile=str(profile), cohort_sha256=sha(cohort / 'cohort.json'), archive_sha256=plan['archive_sha256'], pins=pins, source_component_pins=PINS, setup_sha256=sha(shared / 'scripts/setup_emulator.py'))
    (profile / 'stock-profile.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(dict(stopped=True, profile=str(profile), pins=len(pins), archive_sha256=plan['archive_sha256'])))


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('reference', 'profile', 'cohort'):
        p.add_argument('--' + name, type=Path, required=True)
    a = p.parse_args()
    stage(a.reference.resolve(), a.profile.resolve(), a.cohort.resolve())
