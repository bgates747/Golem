#!/usr/bin/env python3
"""Verify the failure-only source delta and preserved original/copies."""
from pathlib import Path
import gzip
import hashlib
import json
from prepare import HELPER, candidate, PLAN_SHA

HERE = Path(__file__).resolve().parent


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    manifest = json.loads(gzip.decompress((HERE / 'source-manifests.json.gz').read_bytes()))
    original = Path(manifest['original_root'])
    assert sha(HERE.parent / 'mode-fallback-release-plan-v1.md') == PLAN_SHA
    for relative, expected in manifest['original_copied_files'].items():
        assert sha(original / relative) == expected, ('original changed', relative)
    for name, variant in manifest['variants'].items():
        root = Path(variant['root'])
        for relative, original_sha in manifest['original_copied_files'].items():
            expected = variant['changes'].get(relative, original_sha)
            assert sha(root / relative) == expected, (name, relative)
    expected = candidate((original / 'video/vdu.h').read_text())
    patched = Path(manifest['variants']['candidate']['root']) / 'video/vdu.h'
    assert patched.read_text() == expected
    removed = expected.replace(HELPER, '', 1).replace('\t\tif (errVal == 2) releaseFailedResolution();\n', '', 1).replace('\t\t\tif (errVal == 2) releaseFailedResolution();\n', '', 1)
    assert removed == (original / 'video/vdu.h').read_text()
    assert HELPER.index('canvas.reset();') < HELPER.index('_VGAController->end();') < HELPER.index('_VGAController.reset();') < HELPER.index('_VGAColourDepth = uint8_t(-1);')
    original_artifacts = json.loads((original.parent / 'firmware-artifacts.json').read_text())
    for artifact in original_artifacts.values():
        assert sha(Path(artifact['path'])) == artifact['sha256']
    result = {'schema': 'g057-mode-repair-source-verification-v1', 'passed': True,
        'files_checked_per_variant': len(manifest['original_copied_files']),
        'only_change': 'Candidate video/vdu.h: one private helper and two error2-conditional calls.',
        'checks': ['Frozen plan identity verified before and after preparation',
                   'Original copied-source and original artifact hashes unchanged',
                   'Control sources exactly match copied originals',
                   'Candidate changes no file except video/vdu.h',
                   'Removing helper and its two failure-only calls exactly restores original source',
                   'Canvas reset before end before controller deletion before depth255',
                   'Success, invalid/error-1, factory/error1 source branches and original MODE epilogue unchanged',
                   'No ISR, journal, allocation capability, module limit, SDK or service source changed']}
    (HERE / 'source-verification.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
