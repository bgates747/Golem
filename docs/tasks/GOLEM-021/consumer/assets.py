"""Extract one real Apple II monochrome HGR page for the vdplet review."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib

HERE = Path(__file__).resolve().parent
DEFAULT = HERE.parents[4] / 'agon-6502/build/game004/loderunner/mission.bin'

def chunk(tag, data):
    return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data))

def prepare(source=DEFAULT, page=1):
    memory = source.read_bytes()
    if len(memory) != 131072 or page not in (1, 2):
        raise ValueError('Expected a 128 KiB Apple II memory snapshot and HGR page 1 or 2')
    packed = bytearray(6720)
    pixels = bytearray(280 * 192)
    for y in range(192):
        row = page * 0x2000 + ((y & 7) << 10) + ((y & 0x38) << 4) + (y >> 6) * 40
        for x in range(280):
            bit = (memory[row + x // 7] >> (x % 7)) & 1
            packed[y * 35 + x // 8] |= bit << (7 - x % 8)
            pixels[y * 280 + x] = 255 * bit
    out = HERE / 'build/apple-assets'
    out.mkdir(parents=True, exist_ok=True)
    (out / 'loderunner.packed').write_bytes(packed)
    raw = b''.join(b'\0' + pixels[y*280:(y+1)*280] for y in range(192))
    png = (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', 280, 192, 8, 0, 0, 0, 0)) +
           chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b''))
    (out / 'loderunner.png').write_bytes(png)
    meta = dict(name='Lode Runner', source=str(source.resolve()),
                source_sha256=hashlib.sha256(memory).hexdigest(), hgr_page=page,
                packed_sha256=hashlib.sha256(packed).hexdigest(), bytes=len(packed),
                width=280, height=192, bits='MSB-first packed pixels; Apple HGR seven-bit source bytes',
                colour_policy='monochrome; artifact colour/high source bit ignored')
    (out / 'assets.json').write_text(json.dumps(meta, indent=2) + '\n')
    print(f'Prepared Lode Runner HGR page {page}: {len(packed)} packed bytes.')
    return meta

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source', type=Path, default=DEFAULT)
    p.add_argument('--page', type=int, choices=[1, 2], default=1)
    a = p.parse_args()
    prepare(a.source, a.page)
