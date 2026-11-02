#!/usr/bin/env python3
"""Collect matched build/source/section/helper-frame receipts; never build/run firmware."""
from pathlib import Path
import hashlib
import json
import re
import struct
import subprocess

HERE = Path(__file__).resolve().parent
TASK = HERE.parent
BUILD = TASK / 'build/display-origin-diagnostic'
ORIGINAL = Path('/Agon/mystuff/agon-vdplet/build/vdpl-010/pixels/firmware-src')
TOOLS = Path('/home/smith/.platformio/packages/toolchain-xtensa-esp32/bin/xtensa-esp32-elf-')


def sha(data):
    return hashlib.sha256(data).hexdigest()


def artifact(path):
    data = path.read_bytes()
    return {'path': str(path), 'bytes': len(data), 'sha256': sha(data)}


def sections(path):
    data = path.read_bytes()
    assert data[:7] == b'\x7fELF\x01\x01\x01'
    header = struct.unpack_from('<16sHHIIIIIHHHHHH', data)
    shoff, shsize, shcount, names_index = header[6], header[11], header[12], header[13]
    headers = [struct.unpack_from('<10I', data, shoff + shsize * i) for i in range(shcount)]
    names_h = headers[names_index]
    names = data[names_h[4]:names_h[4] + names_h[5]]
    result = {}
    for nameidx, stype, flags, addr, offset, size, _, _, _, _ in headers:
        if not flags & 2:
            continue
        name = names[nameidx:names.index(b'\0', nameidx)].decode()
        result[name] = {'bytes': size, 'address': hex(addr), 'type': stype,
                        'sha256': None if stype == 8 else sha(data[offset:offset + size])}
    return result


def main():
    result = {'schema': 'g057-display-diag-build-results-v1', 'variants': {}}
    for name, root in [('original010', ORIGINAL), ('control', BUILD / 'control'),
                       ('instrumented', BUILD / 'instrumented')]:
        objdir = root / '.pio/build/esp32dev'
        elf = objdir / 'firmware.elf'
        symbols = subprocess.check_output([str(TOOLS) + 'nm', '-S', '-C', str(elf)], text=True)
        disassembly = subprocess.check_output([str(TOOLS) + 'objdump', '-d', '-C', str(elf)], text=True)
        (BUILD / f'{name}-symbols.txt').write_text(symbols)
        (BUILD / f'{name}-disassembly.txt').write_text(disassembly)
        diag_symbols = [line for line in symbols.splitlines() if 'g057_' in line]
        frames = {}
        caller_frames = {}
        function = None
        for line in disassembly.splitlines():
            match = re.match(r'^[0-9a-f]+ <(.+)>:$', line)
            if match:
                function = match[1] if ('g057_' in match[1] or any(p in match[1] for p in
                    ['VGAPalettedController::createPalette(', 'VGAPalettedController::createSignalList(',
                     'VGAPalettedController::setResolution(', 'VDUStreamProcessor::vdu_mode('])) else None
            elif function and 'entry' in line:
                match = re.search(r'entry\s+a1,\s*(0x[0-9a-f]+|\d+)', line)
                if match:
                    if function.startswith('g057_'):
                        frames[function] = int(match[1], 0)
                    elif not function.startswith('_GLOBAL_'):
                        caller_frames[function] = int(match[1], 0)
                    function = None
        result['variants'][name] = {
            'root': str(root), 'artifacts': {f: artifact(objdir / f) for f in
                ['firmware.bin', 'firmware.elf', 'firmware.map', 'bootloader.bin', 'partitions.bin']},
            'sections': sections(elf), 'diagnostic_symbols': diag_symbols,
            'helper_entry_frames_bytes': frames,
            'affected_caller_entry_frames_bytes': caller_frames,
        }
    original, control, diag = (result['variants'][n] for n in ['original010', 'control', 'instrumented'])
    assert not original['diagnostic_symbols'] and not control['diagnostic_symbols']
    assert any('g057_records' in s and '00000200' in s for s in diag['diagnostic_symbols'])
    assert any('g057_control' in s and '00000020' in s for s in diag['diagnostic_symbols'])
    result['section_delta_diag_minus_control'] = {n: s['bytes'] - control['sections'].get(n, {}).get('bytes', 0)
        for n, s in diag['sections'].items() if s['bytes'] != control['sections'].get(n, {}).get('bytes', 0)}
    result['control_vs_original_sections'] = {n: {'size_delta': s['bytes'] - original['sections'].get(n, {}).get('bytes', 0),
        'address_equal': s['address'] == original['sections'].get(n, {}).get('address'),
        'data_equal': s['sha256'] == original['sections'].get(n, {}).get('sha256')}
        for n, s in control['sections'].items()}
    result['control_binary_equals_original'] = control['artifacts']['firmware.bin']['sha256'] == original['artifacts']['firmware.bin']['sha256']
    result['image_delta_bytes'] = diag['artifacts']['firmware.bin']['bytes'] - control['artifacts']['firmware.bin']['bytes']
    result['stack_scope'] = 'Compiler entry-frame sizes from Xtensa disassembly only; not full call-chain high-water. UART/Arduino/FreeRTOS nested stack is unmeasured.'
    result['toolchain'] = {}
    for p in [Path('/home/smith/.platformio/platforms/espressif32/platform.json'),
              Path('/home/smith/.platformio/packages/toolchain-xtensa-esp32/package.json'),
              Path('/home/smith/.platformio/packages/framework-arduinoespressif32/package.json'),
              Path('/home/smith/.platformio/packages/tool-esptoolpy/package.json')]:
        result['toolchain'][str(p)] = {'sha256': sha(p.read_bytes()), 'metadata': json.loads(p.read_text())}
    result['source_files'] = {p.name: artifact(p) for p in HERE.iterdir() if p.is_file() and p.suffix in ['.h', '.py', '.cpp', '.patch']}
    result['platformio_state'] = str(BUILD / 'platformio-state')
    result['build_commands'] = {name: ['env', 'PLATFORMIO_CORE_DIR=' + str(BUILD / 'platformio-state'),
        'PLATFORMIO_SETTING_ENABLE_TELEMETRY=no', '/Agon/mystuff/agon-vdplet/.venv/bin/python',
        '-m', 'platformio', 'run', '-d', str(BUILD / name), '-e', 'esp32dev', '-j', '2']
        for name in ['control', 'instrumented']}
    result['flags'] = 'Original platformio.ini unchanged in both variants; macro is selected in the added private header only.'
    result['executions'] = 'Host journal and parser checks only. Firmware builds only; no emulator, target or hardware execution.'
    (HERE / 'build-results.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({'image_delta': result['image_delta_bytes'], 'section_delta': result['section_delta_diag_minus_control'],
                      'helper_frames': diag['helper_entry_frames_bytes'], 'control_equals_original': result['control_binary_equals_original']}, indent=2))


if __name__ == '__main__':
    main()
