"""Build a small independent stock MOS/VDP multiple-Sub qualification."""
from __future__ import annotations
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
PROJECT = HERE.parents[3]
OLD = PROJECT / 'docs/tasks/GOLEM-019'
BUILD = HERE / 'build'
QUERY, FOREIGN = 60000, 50000
WIDTH, HEIGHT = 512, 384

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def word(n):
    return struct.pack('<H', n)

def packet(key, op):
    return bytes([23, 0, 160]) + word(key) + bytes([op])

def write(key, data):
    return packet(key, 2) + packet(key, 0) + word(len(data)) + data

def update(key, x, y):
    # Only x/y change: colour at offset zero persists between all calls.
    return packet(key, 5) + b'\xc2' + word(1) + word(4) + struct.pack('<hh', x, y)

def marker(x, y):
    return bytes([18, 0, 11, 25, 4]) + struct.pack('<hh', x, y) + bytes([25, 5]) + struct.pack('<hh', 416, 200)

def bitmap(x, y):
    return bytes([23, 27, 32]) + word(8202) + bytes([23, 27, 3]) + struct.pack('<hh', x, y)

def setup():
    return (bytes([22, 20]) + b''.join(bytes([19, i, i, 0, 0, 0]) for i in range(64)) +
            bytes([23, 0, 192, 0, 29, 0, 0, 0, 0, 26, 23, 1, 0, 25, 4, 0, 0, 0, 0]))

def model_marker(screen, x, y):
    # Integer Bresenham is independent of the emitted patchable VDU templates.
    endx, endy = 416, 200
    dx, dy = abs(endx-x), -abs(endy-y)
    sx, sy = (1 if x < endx else -1), (1 if y < endy else -1)
    error = dx + dy
    while True:
        if 0 <= x < WIDTH and 0 <= y < HEIGHT:
            screen[y*WIDTH+x] = 11
        if (x, y) == (endx, endy):
            break
        twice = 2*error
        if twice >= dy:
            error += dy
            x += sx
        if twice <= dx:
            error += dx
            y += sy

def model_bitmap(screen, raw, x, y):
    for i, value in enumerate(raw):
        px, py = x+i%320, y+i//320
        if value >> 6 and 0 <= px < WIDTH and 0 <= py < HEIGHT:
            screen[py*WIDTH+px] = ((value & 3) << 4) | (value & 12) | ((value >> 4) & 3)

def points(x, y):
    # Small source-colour/edge grid plus marker and untouched background points.
    result = {(x+px, y+py) for py in (0, 7, 39, 79, 119, 159)
              for px in (0, 13, 63, 127, 191, 255, 319)}
    result.update({(96, 200), (200, 200), (416, 200), (0, 0), (511, 383),
                   (x-1, y), (x+320, y+80), (x+160, y-1), (x+160, y+160)})
    return sorted(result)

class Tape:
    def __init__(self):
        self.data, self.expected, self.checks = bytearray(), bytearray(), []
        self.calls = []

    def send(self, data):
        assert 0 < len(data) <= 65535
        self.data += b'\x01' + word(len(data)) + data

    def read(self, key, offset, expected, label):
        self.data += b'\x02' + word(key) + word(offset) + word(len(expected))
        self.checks.append(dict(label=label, start=len(self.expected), bytes=len(expected), id=key, offset=offset))
        self.expected += expected

    def call(self, name):
        self.data.append(3 if name == 'drawMarker' else 6)
        self.calls.append(name)

    def pixels(self, screen, selected, label):
        for x, y in selected:
            self.data += b'\x04' + word(x) + word(y)
            self.checks.append(dict(label=label, start=len(self.expected), bytes=1, pixel=[x, y]))
            self.expected.append(screen[y*WIDTH+x])

def finish(tape, directory, mode, meta):
    tape.data.append(0)
    assert len(tape.expected) <= 2048
    (directory / 'tape.bin').write_bytes(tape.data)
    (directory / 'expected.dat').write_bytes(tape.expected)
    active = BUILD / 'active'
    active.mkdir(exist_ok=True)
    for name in ('golem.inc', 'tape.bin'):
        shutil.copyfile(directory / name, active / name)
    if mode == 'review':
        # Six text lines plus returned MOS prompt fit the 16-row text viewport.
        lines = ['GOLEM review.golem: drawWolf = Wolf image',
                 'GOLEM drawMarker = cyan line below image',
                 'ASM: loads once; updates x/y; calls A/B/A',
                 'Type dir at the MOS prompt below.',
                 'Close emulator; tell agent images/dir OK.']
        text = 'success: db 17,63,28,0,47,63,28,31,0,0\n'
        text += ''.join('    db ' + json.dumps(line) + ',13,10\n' for line in lines)
        text += 'success_end:\n'
    else:
        text = 'success: db 13,10,"GOLEM-022 automated checks saved",13,10\nsuccess_end:\n'
    (active / 'success.inc').write_text(text)
    subprocess.run([str(PROJECT / 'docs/tasks/GOLEM-007/build/ez80asm'),
                    'guest.asm', str(directory.relative_to(HERE) / 'probe.bin'), '-l', '-c'],
                   cwd=HERE, check=True)
    shutil.copyfile(HERE / 'guest.lst', directory / 'guest.lst')
    binary = (directory / 'probe.bin').read_bytes()
    listing = (directory / 'guest.lst').read_text()
    def address(label):
        found = re.findall(r'^([0-9A-F]{6}).*\b'+re.escape(label)+r':\s*$', listing, re.M)
        assert len(found) == 1, (label, found)
        return int(found[0], 16)
    def bytes_at(label, size):
        offset = address(label)-0x040000
        return binary[offset:offset+size]
    # Both generated entry wrappers and their six-byte call packets are present
    # in an actual assembled consumer, including the relocated-ID fixture.
    for entry in meta['entries']:
        name = 'golem_'+entry['name']
        call = packet(entry['id'], 1)
        assert len(call) == 6 and bytes_at(name+'_call', 6) == call
        wrapper = (b'\x21'+address(name+'_call').to_bytes(3, 'little')+
                   b'\x01\x06\x00\x00\x5b\xdf\xc9')
        assert bytes_at(name, len(wrapper)) == wrapper
    for suffix, filename in (('bootstrap', 'bootstrap.bin'),
                             ('update_call', 'update-call.bin'), ('cleanup', 'cleanup.bin')):
        payload = (directory/filename).read_bytes()
        assert bytes_at('golem_package_'+suffix, len(payload)) == payload
    meta.update(mode=mode, checks=tape.checks, observation_bytes=len(tape.expected),
                entry_calls=tape.calls, expected_sha256=sha(directory / 'expected.dat'),
                probe_sha256=sha(directory / 'probe.bin'),
                assembled_wrappers_and_package_payloads_match=True,
                human_validation='pending', graphical_emulator_launched=False)
    (directory / 'expected.json').write_text(json.dumps(meta, indent=2) + '\n')

def generate():
    BUILD.mkdir(exist_ok=True)
    assets = json.loads((OLD / 'assets.json').read_text())
    (HERE / 'assets.json').write_text(json.dumps(assets, indent=2) + '\n')
    raw = (OLD / 'assets/splash.rgba2').read_bytes()
    assert hashlib.sha256(raw).hexdigest() == assets['assets'][0]['sha256']
    shutil.copyfile(OLD / 'assets/wolf-review.agnb', BUILD / 'wolf-review.agnb')
    shutil.copyfile(PROJECT.parent / 'agon-dev-env/scripts/setup_emulator.py', BUILD / 'canonical-setup-emulator.py')
    reservations = ['8195:8195', '8201:8202', '8463:8463', '50000:50000', '60000:60000']
    for variant in ('default', 'relocated'):
        directory = BUILD / variant
        options = [item for bounds in reservations for item in ('--reserve', bounds)]
        if variant == 'relocated':
            options += ['--reserve', '1:254', '--reserve', '260:264']
        subprocess.run([str(PROJECT / 'build/golemc'), '--resident', '--emit-asm', *options,
                        str(HERE / 'review.golem'), str(directory)], check=True)
        manifest = json.loads((directory / 'interface.json').read_text())
        assert [e['name'] for e in manifest['entries']] == ['drawMarker', 'drawWolf']
        assert manifest['entry'] == manifest['entries'][0]
        assert (directory / 'invoke.bin').read_bytes() == packet(manifest['entry']['id'], 1)
        assert manifest['input']['bytes'] == 5 and len({g['id'] for g in manifest['globals']}) == 1
        assert [g['offset'] for g in manifest['globals']] == [0, 1, 3]
        owned = {e['id'] for e in manifest['owned']}
        assert not owned.intersection({8195, 8201, 8202, 8463, QUERY, FOREIGN})
        assert (directory / 'bootstrap.bin').read_bytes()[:7] == bytes([23, 0, 248, 1, 0, 1, 0])
        for mode in (('automated', 'review') if variant == 'default' else ('automated',)):
            output = directory if mode == 'automated' else BUILD / 'review'
            if mode == 'review':
                output.mkdir(exist_ok=True)
                for name in ('bootstrap.bin', 'cleanup.bin', 'invoke.bin', 'update-call.bin', 'interface.json', 'golem.inc'):
                    shutil.copyfile(directory / name, output / name)
            tape = Tape()
            tape.send(setup())
            tape.data.append(5)  # Unchanged canonical assembly AGNB loader.
            tape.send(write(QUERY, bytes([23, 0, 128, 0])))
            tape.send(write(FOREIGN, b'canary'))
            tape.send((directory / 'bootstrap.bin').read_bytes())
            screen = bytearray(WIDTH*HEIGHT)
            tape.call('drawMarker'); model_marker(screen, 96, 200)
            selected = points(112, 32)
            if mode == 'automated':
                tape.pixels(screen, selected, 'A only: no implicit later bitmap')
                tape.read(manifest['input']['id'], 0, bytes([11])+struct.pack('<hh', 96, 200), 'A persistent initial inputs')
            x, y = (112, 32) if mode == 'automated' else (96, 16)
            tape.send(update(manifest['input']['id'], x, y))
            tape.call('drawWolf'); model_bitmap(screen, raw, x, y)
            if mode == 'automated':
                tape.pixels(screen, selected, 'B uses updated shared x/y')
                tape.read(manifest['input']['id'], 0, bytes([11])+struct.pack('<hh', x, y), 'B persistent changed inputs')
            else:
                # Restore only the shared positions so the second A stays below
                # the recognizable Wolf picture; no direct drawing is hidden.
                tape.send(update(manifest['input']['id'], 96, 200))
                x, y = 96, 200
            tape.call('drawMarker'); model_marker(screen, x, y)
            tape.pixels(screen, selected if mode == 'automated' else points(96, 16), 'A again without reinstall')
            tape.read(manifest['input']['id'], 0, bytes([11])+struct.pack('<hh', x, y), 'Final shared inputs persist')
            if mode == 'automated':
                # Independent direct VDU controls are confined to this tape.
                # They reproduce the three phases, with separate fresh canvases.
                control = bytearray(WIDTH*HEIGHT)
                tape.send(b'\x10' + marker(96, 200)); model_marker(control, 96, 200)
                tape.pixels(control, selected, 'Independent direct A')
                tape.send(bitmap(112, 32)); model_bitmap(control, raw, 112, 32)
                tape.pixels(control, selected, 'Independent direct B')
                tape.send(marker(112, 32)); model_marker(control, 112, 32)
                tape.pixels(control, selected, 'Independent direct A again')
            tape.send((directory / 'cleanup.bin').read_bytes())
            for key in sorted(owned):
                tape.read(key, 0, b'\x55', 'Cleanup removes compiler-owned buffer')
            for asset in assets['assets']:
                borrowed = (OLD / 'assets' / (asset['name']+'.rgba2')).read_bytes()
                tape.read(asset['buffer_id'], 0, borrowed[:8], 'Cleanup preserves borrowed Wolf bytes')
            tape.read(FOREIGN, 0, b'canary', 'Cleanup preserves unrelated external buffer')
            if mode == 'automated':
                tape.send(b'\x10' + bitmap(112, 32))
                after = bytearray(WIDTH*HEIGHT); model_bitmap(after, raw, 112, 32)
                tape.pixels(after, selected, 'Borrowed bitmap remains drawable after cleanup')
            finish(tape, output, mode, dict(variant=variant, entries=manifest['entries'],
                   installs=1, named_call_wire_bytes=6, shared_update_wire_bytes=21,
                   owned_buffers=sorted(owned), borrowed_ids=[a['buffer_id'] for a in assets['assets']],
                   source='review.golem', assembly='guest.asm',
                   direct_bitmap_draws=2 if mode == 'automated' else 0))
    provenance = {
        'adapted_transport': {'source': 'docs/tasks/GOLEM-019/demo_guest.asm', 'sha256': sha(OLD/'demo_guest.asm')},
        'frozen_copies': {str(p.relative_to(PROJECT)): sha(p) for p in
                         [OLD/'agnb_api.inc', OLD/'agnb_support.inc', PROJECT/'reference/agnb-asm/lib/mos_api.inc']},
        'assets': assets,
        'compiler_sha256': sha(PROJECT/'build/golemc'),
        'canonical_setup_sha256': sha(BUILD/'canonical-setup-emulator.py'),
    }
    (HERE/'provenance.json').write_text(json.dumps(provenance, indent=2)+'\n')
    print('Prepared default/relocated stock diagnostics and separate Golem-only human scene.')

if __name__ == '__main__':
    generate()
