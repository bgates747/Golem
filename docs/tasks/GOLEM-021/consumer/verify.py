"""Verify a saved native-module transcript without a running emulator."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct

HERE = Path(__file__).resolve().parent

def verify(result: Path, expected: Path | None = None):
    meta = json.loads((expected or HERE / 'build/expected.json').read_text())
    actual = result.read_bytes()
    if len(actual) != meta['reply_bytes']:
        raise ValueError(f'Expected {meta["reply_bytes"]} saved bytes, got {len(actual)}')
    handles, addresses = [], []
    stack_free = None
    entry_low = None
    for i, check in enumerate(meta['checks']):
        signature, version, kind, token, value = struct.unpack_from('<4sBBHH', actual, i * 10)
        if (signature, version, kind, token) != (b'VDPL', 1, check['kind'], check['token']):
            raise ValueError(f'Reply mismatch at {check["label"]}: {actual[i*10:i*10+10].hex()}')
        if check['value'] is not None and value != check['value']:
            raise ValueError(f'{check["label"]}: expected {check["value"]:#x}, got {value:#x}')
        if check['capture_handle']:
            if value == 0:
                raise ValueError('A successful module load must return a nonzero handle')
            handles.append(value)
        if check['label'].endswith('entry low'):
            entry_low = value
        if check['label'].endswith('entry high'):
            addresses.append(entry_low | value << 16)
        if check['label'] == 'command task minimum free stack bytes':
            if not value:
                raise ValueError('Command task reports no free stack margin')
            stack_free = value
    if len(handles) != meta.get('module_loads', 3) or len(set(handles)) != len(handles):
        raise ValueError(f'Reload did not receive distinct handles: {handles}')
    if len(addresses) != len(handles):
        raise ValueError('Every loaded module must report its actual native entry address')
    if not all(0x40080000 <= address < 0x400A0000 for address in addresses):
        raise ValueError(f'Native entries are outside classic ESP32 internal IRAM: {addresses}')
    receipt = dict(pass_transcript=True, checks=len(meta['checks']), bytes=len(actual),
                   observed_sha256=hashlib.sha256(actual).hexdigest(),
                   module_handles=handles, native_entry_addresses=[f'{a:#010x}' for a in addresses],
                   human_visual_review='pending')
    if len(addresses) >= 3:
        receipt['replaced_at_same_entry'] = addresses[0] == addresses[2]
    if stack_free is not None:
        receipt['command_task_minimum_free_stack_bytes'] = stack_free
    print(json.dumps(receipt, indent=2))
    return receipt

def verify_pixels(frame: Path, packed: Path):
    magic, dimensions, maximum, pixels = frame.read_bytes().split(b'\n', 3)
    width, height = map(int, dimensions.split())
    if (magic, width, height, maximum, len(pixels)) != (b'P6', 320, 240, b'255', 320*240*3):
        raise ValueError('Expected a complete decoded 320x240 RGB VGA frame')
    bits = packed.read_bytes()
    if len(bits) != 6720:
        raise ValueError('Expected the complete 280x192 packed source image')
    for y in range(192):
        for x in range(320):
            bit = (bits[y*35+x//8] >> (7-x%8)) & 1 if x < 280 else 0
            expected = bytes([255*bit])*3
            offset = (y*320+x)*3
            if pixels[offset:offset+3] != expected:
                raise ValueError(f'Native VGA mismatch at ({x},{y}): '
                                 f'{pixels[offset:offset+3].hex()} != {expected.hex()}')
    return dict(pass_native_pixels=True, pixel_checks=320*192,
                image_dimensions=[280, 192], screen_dimensions=[width, height],
                unchanged_black_margin=[280, 0, 319, 191],
                decoded_vga_sha256=hashlib.sha256(frame.read_bytes()).hexdigest(),
                packed_sha256=hashlib.sha256(bits).hexdigest())

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('result', type=Path)
    parser.add_argument('--expected', type=Path)
    args = parser.parse_args()
    verify(args.result, args.expected)
