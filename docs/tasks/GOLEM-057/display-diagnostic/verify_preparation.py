#!/usr/bin/env python3
"""Verify frozen originals, prepared files and inactive-source equivalence."""
from pathlib import Path
import gzip
import hashlib
import json
import re

HERE = Path(__file__).resolve().parent


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def choose(text, enabled):
    result = []
    active = []
    for line in text.splitlines(True):
        if line.strip() == '#if G057_DIAG_ENABLED':
            active.append(enabled)
        elif active and line.strip() == '#else':
            active[-1] = not active[-1]
        elif active and line.strip() == '#endif':
            active.pop()
        elif all(active):
            if line.strip() not in ['#include <g057_diag.h>', '#include "g057_diag.h"',
                                    '#define G057_DIAG_IMPLEMENTATION']:
                result.append(line)
    assert not active
    return ''.join(result)


def body(text, name):
    start = text.index(name)
    first = text.index('{', start)
    nesting = 1
    end = first + 1
    while nesting:
        nesting += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


def main():
    m = json.loads(gzip.decompress((HERE / 'source-manifests.json.gz').read_bytes()))
    original = Path(m['original'])
    checked = {}
    for variant, manifest in m['variants'].items():
        root = Path(manifest['root'])
        for p, expected in manifest['copied_original_files'].items():
            assert digest(original / p) == expected, ('original changed', p)
        for p, expected in manifest['prepared_source_files'].items():
            assert digest(root / p) == expected, ('prepared source changed', variant, p)
        checked[variant] = len(manifest['prepared_source_files'])
        for p in ['video/vdu.h', 'video/video.ino', '.pio/libdeps/esp32dev/vdp-gl/src/dispdrivers/vgapalettedcontroller.cpp']:
            off = choose((root / p).read_text(), False)
            assert re.sub(r'\s+', '', off) == re.sub(r'\s+', '', (original / p).read_text()), ('macrooff differs', p)
        gl = choose((root / '.pio/libdeps/esp32dev/vdp-gl/src/dispdrivers/vgapalettedcontroller.cpp').read_text(), True)
        assert body(gl, 'bool VGAPalettedController::createPalette(').count('heap_caps_malloc(') == 1
        assert body(gl, 'PaletteListItem * VGAPalettedController::createSignalList(').count('heap_caps_malloc(') == 1
        resolution = body(gl, 'void VGAPalettedController::setResolution(')
        assert resolution.index('g057_flush();') < resolution.index('startGPIOStream();') < resolution.index('esp_intr_alloc_pinnedToCore(')
        assert 'g057_' not in (root / '.pio/libdeps/esp32dev/vdp-gl/src/dispdrivers/vga2controller.cpp').read_text()
        assert 'g057_' not in body(gl, 'VGAPalettedController::getSignalsForScanline(')
    result = {'schema': 'g057-display-diag-source-verification-v1', 'passed': True,
              'checked_prepared_source_files': checked,
              'checks': ['Frozen original copied-file hashes unchanged', 'Prepared-file hashes unchanged',
                         'Macro-off three changed source files equal original after whitespace removal',
                         'Macro-on palette and list allocation calls each remain exactly once',
                         'Journal emission precedes original GPIO then interrupt-allocation calls',
                         'No hooks in VGA2 ISR source or getSignalsForScanline']}
    (HERE / 'source-verification.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
