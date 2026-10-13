"""Build the MOS-side runtime-loading proof; never execute a GUI emulator."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess

HERE = Path(__file__).resolve().parent
PROJECT = HERE.parents[3]
BUILD = HERE / 'build'
MODULES = PROJECT / 'docs/tasks/GOLEM-002/proof/build'
STAGING, INPUT, OUTPUT, ARGS = 50000, 50001, 50002, 50003
PIXELS, WRAPPER, BAD_PIXELS = 50004, 50005, 50006

def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()

def packet(buffer: int, opcode: int) -> bytes:
    return bytes([23, 0, 160]) + struct.pack('<HB', buffer, opcode)

def write(buffer: int, data: bytes) -> bytes:
    assert len(data) <= 65535
    return packet(buffer, 2) + packet(buffer, 0) + struct.pack('<H', len(data)) + data

class Script:
    def __init__(self):
        self.data = bytearray()
        self.checks = []
        self.token = 0
        self.wire = 0

    def send(self, wire: bytes):
        assert len(wire) <= 65535
        self.data += bytes([1]) + struct.pack('<H', len(wire)) + wire
        self.wire += len(wire)

    def wait_frames(self, count: int):
        assert 0 <= count <= 65535
        self.data += bytes([3]) + struct.pack('<H', count)

    def send_patched(self, wire: bytes, offset: int):
        assert 0 <= offset <= len(wire) - 2 and len(wire) <= 65535
        self.data += bytes([4]) + struct.pack('<HH', offset, len(wire)) + wire
        self.wire += len(wire)

    def transaction(self, op: int, fields: bytes, kind: int, value: int | None,
                    label: str, *, handle=False, capture=False):
        self.token += 1
        payload = struct.pack('<BBHBB', 1, op, self.token, 1, 0) + fields
        wire = bytes([23, 0, 0xD8]) + struct.pack('<H', len(payload)) + payload
        self.receive(wire, self.token, kind, value, label,
                     patch=11 if handle else 65535, capture=capture)

    def receive(self, wire: bytes, token: int, kind: int, value: int | None,
                label: str, *, patch=65535, capture=False):
        flags = 1 if capture else 2 if value is None else 0
        self.data += bytes([2]) + struct.pack('<HBBHHH', token, kind, flags,
                                              value or 0, patch, len(wire)) + wire
        self.wire += len(wire)
        self.checks.append(dict(label=label, token=token, kind=kind,
                                value=value, capture_handle=capture))

    def info(self, field: int, value: int | None, label: str):
        self.transaction(4, struct.pack('<H', field), 4, value, label)

    def load(self, image: Path, label: str):
        data = image.read_bytes()
        assert len(data) <= 16384
        self.send(write(STAGING, data))
        self.transaction(1, struct.pack('<HIH', STAGING, len(data), 0),
                         2, None, label, capture=True)
        self.info(6, None, label + ' entry low')
        self.info(7, None, label + ' entry high')

    def run(self, x: float, y: float, count: int, label: str, *, update=True):
        if update:
            self.send(write(INPUT, struct.pack('<f', x)))
        self.transaction(2, struct.pack('<HHH', 0, 0, ARGS), 3, 0,
                         label + ' call', handle=True)
        expected = struct.pack('<fI', y, count)
        for offset in range(0, 8, 2):
            word = struct.unpack_from('<H', expected, offset)[0]
            self.transaction(5, struct.pack('<HI', OUTPUT, offset), 4, word,
                             label + f' result word {offset // 2}')

    def unload(self):
        self.transaction(3, b'\0\0', 3, 0, 'unload', handle=True)

def generate(packed=False):
    BUILD.mkdir(parents=True, exist_ok=True)
    images = [MODULES / f'numeric-{n}.vdpl' for n in (1, 2)]
    for image in images:
        if not image.is_file():
            raise SystemExit(f'Build the vdplet fixtures first: missing {image}')
    s = Script()
    s.send(bytes([22, 8, 12]) +
           b'Fab + Flexe: real ESP32 firmware\r\n'
           b'MOS assembly uploads native vdplets after boot.\r\n'
           b'Float work runs on the ESP32, results stay resident.\r\n')
    s.receive(bytes([23, 0, 0xD9]), 0, 1, None, 'discovery')
    s.info(0, 0x0100, 'protocol version')
    s.info(2, 0, 'initial handle absent')
    s.send(write(OUTPUT, bytes(8)) + write(ARGS, struct.pack('<HH', INPUT, OUTPUT)))
    s.load(images[0], 'numeric 1 initial load')
    s.info(8, 1, 'load count 1')
    s.run(2.0, 3.25, 1, 'x=2')
    s.run(-4.0, -5.75, 2, 'x=-4')
    s.run(-4.0, -5.75, 3, 'repeat keeps native state', update=False)
    s.unload()
    s.load(images[0], 'numeric 1 reload')
    s.info(8, 2, 'load count 2')
    s.run(0.0, 0.25, 1, 'reload resets bss')
    s.unload()
    s.load(images[1], 'numeric 2 replacement')
    s.info(8, 3, 'load count 3')
    s.run(2.0, 4.5, 1, 'changed native code')
    s.unload()
    asset = None
    if packed:
        image = MODULES / 'packed-mono.vdpl'
        if not image.is_file():
            raise SystemExit(f'Build the packed vdplet fixture first: missing {image}')
        images.append(image)
        asset_path = BUILD / 'apple-assets/loderunner.packed'
        asset = json.loads((asset_path.parent / 'assets.json').read_text())
        pixels = asset_path.read_bytes()
        assert len(pixels) == 6720 and sha(asset_path) == asset['packed_sha256']
        s.load(image, 'packed renderer load')
        s.info(8, 4, 'load count 4')
        s.info(1, 3, 'buffer and packed framebuffer capabilities')
        # Opposite pixels make any erroneous partial copy visible to the
        # independent final VGA oracle, even though the source is one byte short.
        s.send(write(PIXELS, pixels) + write(BAD_PIXELS, bytes(v ^ 255 for v in pixels[:-1])))
        args = struct.pack('<HhhHH', PIXELS, 0, 0, 280, 192)
        s.send(write(ARGS, args))
        s.transaction(2, struct.pack('<HHH', 0, 0, ARGS), 5, 9,
                      'packed rejects non-PALETTE2 mode', handle=True)
        # The only image drawing in this fixture is uploaded native C code.
        s.send(bytes([22, 11, 12, 23, 1, 0]))
        # Six bytes call the preinstalled silent RUN packet. Its dynamic handle
        # is patched by assembly before the packet is installed, not by Python.
        body = bytes([23, 0, 0xD8]) + struct.pack('<HBBHBBHHH', 12, 1, 2, 0, 0, 0,
                                               0, 0, ARGS)
        assert len(body) == 17
        s.send_patched(write(WRAPPER, body), 14 + 11)
        s.send(packet(WRAPPER, 1))
        s.info(3, 0, 'first silent packed invocation completed')
        s.send(packet(WRAPPER, 1))
        s.info(3, 0, 'repeat silent packed invocation completed')
        s.send(write(ARGS, struct.pack('<HhhHH', BAD_PIXELS, 0, 0, 280, 192)))
        s.transaction(2, struct.pack('<HHH', 0, 0, ARGS), 5, 3,
                      'packed rejects short inverted source without changing pixels', handle=True)
        s.send(write(ARGS, struct.pack('<HhhHH', PIXELS, -8, 0, 280, 192)))
        s.transaction(2, struct.pack('<HHH', 0, 0, ARGS), 5, 3,
                      'packed rejects negative geometry without changing pixels', handle=True)
        s.send(write(ARGS, args))
        s.info(17, None, 'command task minimum free stack bytes')
        s.wait_frames(2)
        s.unload()
        # Preserve the native-rendered pixels while MOS uses the bottom six
        # text rows. No diagnostic direct drawing is hidden in this harness.
        labels = (b'VDPLET: packed_mono.cpp draws image\r\n'
                  b'ASM: probe.asm loads, calls, unloads.\r\n')
        assert all(len(line) < 40 for line in labels.split(b'\r\n'))
        s.send(bytes([28, 0, 29, 39, 24, 31, 0, 0]) + labels)
    s.info(2, 0, 'final handle absent')
    for ident in (STAGING, INPUT, OUTPUT, ARGS, *((PIXELS, WRAPPER, BAD_PIXELS) if packed else ())):
        s.send(packet(ident, 2))
    s.data.append(0)
    assert len(s.data) < 65536 and len(s.checks) * 10 <= 4096
    (BUILD / 'script.bin').write_bytes(s.data)
    subprocess.run([str(PROJECT / 'docs/tasks/GOLEM-007/build/ez80asm'),
                    'probe.asm', 'build/probe.bin', '-l', '-c'], cwd=HERE, check=True)
    meta = dict(checks=s.checks, reply_bytes=10 * len(s.checks),
                request_wire_bytes=s.wire, reply_wire_bytes=12 * len(s.checks),
                script_bytes=len(s.data), source='probe.asm',
                modules={p.name: dict(bytes=p.stat().st_size, sha256=sha(p)) for p in images},
                probe_sha256=sha(BUILD / 'probe.bin'), script_sha256=sha(BUILD / 'script.bin'),
                native_formula_variants=['1.5*x+0.25', '2*x+0.5'],
                fixture='packed' if packed else 'numeric',
                module_loads=4 if packed else 3, packed_asset=asset,
                minimum_normal_invocation_bytes=17,
                resident_invocation_bytes=6 if packed else None,
                notes='Diagnostic replies and resident result reads belong to the proof, not the hot loop.')
    (BUILD / 'expected.json').write_text(json.dumps(meta, indent=2) + '\n')
    print(f'Built {len(s.checks)} reply checks; script {len(s.data)} bytes.')
    return meta

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--packed', action='store_true', help='Include uploaded native packed-row renderer')
    generate(parser.parse_args().packed)
