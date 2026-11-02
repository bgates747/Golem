#!/usr/bin/env python3
"""Collect offline matched build costs and pins; no target execution."""
from pathlib import Path
import ast
import hashlib
import json
import re
import struct
import subprocess

HERE = Path(__file__).resolve().parent
BUILD = HERE.parent / 'build/display-mode-repair'
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
    h = struct.unpack_from('<16sHHIIIIIHHHHHH', data)
    headers = [struct.unpack_from('<10I', data, h[6] + h[11] * i) for i in range(h[12])]
    nh = headers[h[13]]
    names = data[nh[4]:nh[4] + nh[5]]
    result = {}
    for nameidx, stype, flags, addr, offset, size, _, _, _, _ in headers:
        if flags & 2:
            name = names[nameidx:names.index(b'\0', nameidx)].decode()
            result[name] = {'bytes': size, 'address': hex(addr),
                           'sha256': None if stype == 8 else sha(data[offset:offset + size])}
    return result


def environment_flags(path):
    text = path.read_text()
    result = {}
    for key, value in re.findall(r"(?ms)^  '(CCFLAGS|CXXFLAGS|CFLAGS|CPPDEFINES|LINKFLAGS)': (.*?)(?=^  '[A-Z_]+':|\Z)", text):
        value = value.strip().rstrip(',')
        if value.startswith('deque('):
            value = value[6:-1]
        result[key] = ast.literal_eval(value)
    assert len(result) == 5
    return result


def main():
    result = {'schema': 'g057-mode-repair-offline-build-results-v1', 'variants': {}}
    for name, root in [('original010', ORIGINAL), ('control00', BUILD / 'control00'), ('candidate', BUILD / 'candidate')]:
        objdir = root / '.pio/build/esp32dev'
        elf = objdir / 'firmware.elf'
        symbols = subprocess.check_output([str(TOOLS) + 'nm', '-S', '-C', str(elf)], text=True)
        disassembly = subprocess.check_output([str(TOOLS) + 'objdump', '-d', '-C', str(elf)], text=True)
        (BUILD / f'{name}-symbols.txt').write_text(symbols)
        (BUILD / f'{name}-disassembly.txt').write_text(disassembly)
        frames = {}
        current = None
        for line in disassembly.splitlines():
            match = re.match(r'^[0-9a-f]+ <(.+)>:$', line)
            if match:
                current = match[1] if any(p in match[1] for p in ['releaseFailedResolution', 'VDUStreamProcessor::vdu_mode(', 'updateVGAController(']) else None
            elif current and 'entry' in line:
                match = re.search(r'entry\s+a1,\s*(0x[0-9a-f]+|\d+)', line)
                if match:
                    frames[current] = int(match[1], 0)
                    current = None
        entry = {'root': str(root), 'artifacts': {f: artifact(objdir / f) for f in
            ['firmware.bin', 'firmware.elf', 'firmware.map', 'bootloader.bin', 'partitions.bin']},
            'sections': sections(elf), 'affected_entry_frames_bytes': frames,
            'repair_symbols': [s for s in symbols.splitlines() if 'releaseFailedResolution' in s],
            'journal_symbols': [s for s in symbols.splitlines() if 'g057_' in s]}
        if name != 'original010':
            entry['expanded_flags'] = environment_flags(BUILD / f'{name}-envdump.log')
            entry['platformio_configuration'] = artifact(root / 'platformio.ini')
            log = BUILD / f'{name}-build.log'
            entry['build_log'] = artifact(log)
            duration = re.search(r'\[SUCCESS\] Took ([0-9.]+) seconds', log.read_text())
            entry['platformio_reported_build_seconds'] = float(duration[1]) if duration else None
        result['variants'][name] = entry
    orig, control, candidate = (result['variants'][n] for n in ['original010', 'control00', 'candidate'])
    assert control['expanded_flags'] == candidate['expanded_flags']
    assert control['platformio_configuration']['sha256'] == candidate['platformio_configuration']['sha256']
    assert not any(v['journal_symbols'] for v in result['variants'].values())
    result['matched_flags_equal'] = True
    result['equal_length_build_root_names'] = len('control00') == len('candidate')
    result['candidate_minus_control'] = {'image_bytes': candidate['artifacts']['firmware.bin']['bytes'] - control['artifacts']['firmware.bin']['bytes'],
        'sections': {n: s['bytes'] - control['sections'].get(n, {}).get('bytes', 0)
                     for n, s in candidate['sections'].items() if s['bytes'] != control['sections'].get(n, {}).get('bytes', 0)}}
    result['control_vs_original'] = {'image_bytes': control['artifacts']['firmware.bin']['bytes'] - orig['artifacts']['firmware.bin']['bytes'],
        'binary_equal': control['artifacts']['firmware.bin']['sha256'] == orig['artifacts']['firmware.bin']['sha256'],
        'section_size_differences': {n: s['bytes'] - orig['sections'].get(n, {}).get('bytes', 0)
                                    for n, s in control['sections'].items() if s['bytes'] != orig['sections'].get(n, {}).get('bytes', 0)}}
    result['stack_limit'] = 'Xtensa compiler entry-frame sizes only; full RTOS/end/destructor call-chain high-water is not measured.'
    result['toolchain'] = {str(p): artifact(p) for p in [
        Path('/home/smith/.platformio/platforms/espressif32/platform.json'),
        Path('/home/smith/.platformio/packages/toolchain-xtensa-esp32/package.json'),
        Path('/home/smith/.platformio/packages/framework-arduinoespressif32/package.json'),
        Path('/home/smith/.platformio/packages/tool-esptoolpy/package.json'),
        Path(str(TOOLS) + 'g++')]}
    result['commands'] = {name: ['env', 'PLATFORMIO_CORE_DIR=' + str(BUILD / 'platformio-state'),
        'PLATFORMIO_SETTING_ENABLE_TELEMETRY=no', '/Agon/mystuff/agon-vdplet/.venv/bin/python', '-m', 'platformio',
        'run', '-d', str(BUILD / name), '-e', 'esp32dev', '-j', '2'] for name in ['control00', 'candidate']}
    result['source_and_test_files'] = {p.name: artifact(p) for p in HERE.iterdir() if p.is_file() and p.suffix in ['.py', '.cpp', '.patch']}
    result['scope'] = 'Source preparation, host tests and offline builds only. No firmware/emulator/target execution, deployment or promotion.'
    (HERE / 'build-results.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({'candidate_minus_control': result['candidate_minus_control'],
        'candidate_artifacts': candidate['artifacts'], 'frames': candidate['affected_entry_frames_bytes']}, indent=2))


if __name__ == '__main__':
    main()
