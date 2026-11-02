#!/usr/bin/env python3
"""Prepare paired original010 failure-disposal sources, without building/running."""
from pathlib import Path
import difflib
import gzip
import hashlib
import json
import shutil

HERE = Path(__file__).resolve().parent
TASK = HERE.parent
ORIGINAL = Path('/Agon/mystuff/agon-vdplet/build/vdpl-010/pixels/firmware-src')
BUILD = TASK / 'build/display-mode-repair'
PLAN_SHA = 'ff84eb791917a8a4432187b0035c306c924296d521d2a5e5f29abd04eaebda53'
VDU_SHA = '24923b53a83d77ebf729795d7ecb7343e6f2fed0e9d748d48fc83c8b75e805cb'
HELPER = '''// Private failure-only disposal: reconstruct the fallback after releasing
// the installed controller whose resolution returned height error 2.
static void releaseFailedResolution() {
    canvas.reset();
    if (_VGAController) {
        _VGAController->end();
        _VGAController.reset();
    }
    _VGAColourDepth = uint8_t(-1);
}

'''


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def ignore(directory, names):
    # Preserve cached GL VCS metadata so PlatformIO recognizes the offline pin.
    return [n for n in names if (n == '.git' and Path(directory).name != 'vdp-gl') or
            (n == 'build' and Path(directory).name == '.pio') or
            Path(n).suffix in ('.o', '.a', '.elf', '.bin', '.map')]


def inventory(root):
    return {str(p.relative_to(root)): sha(p) for p in sorted(root.rglob('*'))
            if p.is_file() and '.git' not in p.relative_to(root).parts}


def once(text, old, new):
    assert text.count(old) == 1, (old, text.count(old))
    return text.replace(old, new, 1)


def candidate(original):
    text = once(original, '// VDU 22 Handle MODE\n', HELPER + '// VDU 22 Handle MODE\n')
    text = once(text, '\t\terrVal = changeMode(videoMode);',
                '\t\tif (errVal == 2) releaseFailedResolution();\n\t\terrVal = changeMode(videoMode);')
    text = once(text, '\t\t\tvideoMode = 1;\n\t\t\tchangeMode(1);',
                '\t\t\tif (errVal == 2) releaseFailedResolution();\n\t\t\tvideoMode = 1;\n\t\t\tchangeMode(1);')
    return text


def main():
    assert sha(TASK / 'mode-fallback-release-plan-v1.md') == PLAN_SHA
    assert sha(ORIGINAL / 'video/vdu.h') == VDU_SHA
    assert not BUILD.exists(), 'Refuse to overwrite prior source/build preparation'
    assert len('control00') == len('candidate')
    BUILD.mkdir(parents=True)
    original = (ORIGINAL / 'video/vdu.h').read_text()
    changed = candidate(original)
    patch = ''.join(difflib.unified_diff(original.splitlines(True), changed.splitlines(True),
        fromfile='a/video/vdu.h', tofile='b/video/vdu.h'))
    (HERE / 'source.patch').write_text(patch)
    inventories = {}
    for name in ['control00', 'candidate']:
        target = BUILD / name
        shutil.copytree(ORIGINAL, target, ignore=ignore, symlinks=True)
        inventories[name] = inventory(target)
        if name == 'candidate':
            (target / 'video/vdu.h').write_text(changed)
    assert inventories['control00'] == inventories['candidate']
    original_files = inventories['control00']
    source_receipt = {'schema': 'g057-mode-repair-source-manifest-v1',
        'original_root': str(ORIGINAL), 'original_copied_files': original_files,
        'variants': {'control00': {'root': str(BUILD / 'control00'), 'changes': {}},
                     'candidate': {'root': str(BUILD / 'candidate'),
                                   'changes': {'video/vdu.h': sha(BUILD / 'candidate/video/vdu.h')}}}}
    archive = HERE / 'source-manifests.json.gz'
    archive.write_bytes(gzip.compress(json.dumps(source_receipt, sort_keys=True).encode(), mtime=0))
    manifest = {'schema': 'g057-mode-repair-preparation-v1', 'frozen_plan_sha256': PLAN_SHA,
        'original_root': str(ORIGINAL), 'build_root': str(BUILD),
        'equal_length_variant_names': ['control00', 'candidate'],
        'source_files_per_variant': len(original_files),
        'only_changed_file': 'candidate/video/vdu.h',
        'original_vdu_sha256': VDU_SHA, 'candidate_vdu_sha256': sha(BUILD / 'candidate/video/vdu.h'),
        'patch_sha256': sha(HERE / 'source.patch'),
        'source_manifest': {'path': archive.name, 'bytes': archive.stat().st_size, 'sha256': sha(archive)},
        'dependency_policy': 'Original cached .pio/libdeps including GL VCS metadata copied; current SDK exporter never invoked.',
        'execution_scope': 'Source, host tests and offline matched builds only; no target/emulator/deployment.'}
    (HERE / 'preparation.json').write_text(json.dumps(manifest, indent=2) + '\n')
    state = BUILD / 'platformio-state'
    state.mkdir()
    for name in ['packages', 'platforms']:
        (state / name).symlink_to(Path('/home/smith/.platformio') / name)
    print(json.dumps(manifest, indent=2))


if __name__ == '__main__':
    main()
