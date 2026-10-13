"""Build independent real-MOS consumers of the supplied Apple F2/F3 vdplet."""
from __future__ import annotations

import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import subprocess

HERE = Path(__file__).resolve().parent
TASK = HERE.parent
PROJECT = HERE.parents[3]
OWNER = PROJECT.parent / 'agon-6502/build/perf020/vdplet'
BUILD = HERE / 'build'
MODULE_SHA = 'fd1aa6a5d1c4d51f0f27fd191de896cee4c7e93480f8a611e6e96ac798fd6092'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def support(name):
    spec = importlib.util.spec_from_file_location('consumer_' + name,
                                                TASK / 'consumer' / (name + '.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def unpack_image(raw):
    """Independent pixel-order oracle: Apple bit zero is the leftmost dot."""
    assert len(raw) == 7680
    packed = bytearray(6720)
    for y in range(192):
        for x in range(280):
            bit = raw[y * 40 + x // 7] >> (x % 7) & 1
            packed[y * 35 + x // 8] |= bit << (7 - x % 8)
    return bytes(packed)


def decode(raw, encoded, entry):
    """Validate first, then independently apply the owner's token convention."""
    changes, pos, at = [], 0, 0
    while at < len(encoded):
        token = encoded[at]
        at += 1
        literal = token >= 128
        count = 1 if entry == 3 and literal else (token & 127) + 1
        if pos + count > len(raw):
            return 3, raw
        if literal:
            if entry == 3:
                changes.append((pos, token & 127))
            else:
                if at + count > len(encoded):
                    return 4, raw
                changes.extend((pos + i, encoded[at + i] & 127) for i in range(count))
                at += count
        pos += count
    if pos != len(raw):
        return 4, raw
    result = bytearray(raw)
    for index, value in changes:
        result[index] ^= value
    return 0, bytes(result)


def generate():
    BUILD.mkdir(parents=True, exist_ok=True)
    supplied = BUILD / 'supplied'
    if supplied.exists():
        shutil.rmtree(supplied)
    shutil.copytree(OWNER / 'fixtures', supplied / 'fixtures')
    shutil.copytree(OWNER / 'source', supplied / 'source')
    for name in ('modules.json', 'apple-codecs.vdpl', 'apple-codecs.su',
                 'apple-codecs.readelf.txt', 'native-probes.json'):
        shutil.copyfile(OWNER / name, supplied / name)
    module = supplied / 'apple-codecs.vdpl'
    assert sha(module) == MODULE_SHA and module.stat().st_size == 2288
    fixtures = supplied / 'fixtures'
    contract = json.loads((fixtures / 'fixtures.json').read_text())
    for name, identity in contract['artifacts'].items():
        assert sha(fixtures / name) == identity['sha256']
        assert (fixtures / name).stat().st_size == identity['bytes']
    assert unpack_image((fixtures / 'lode.raw').read_bytes()) == (fixtures / 'lode.packed').read_bytes()
    assert unpack_image((fixtures / 'initial.raw').read_bytes()) == bytes(6720)
    assert (fixtures / 'lode.screen').read_bytes() == b''.join(
        (fixtures / 'lode.packed').read_bytes()[y*35:(y+1)*35] + bytes(5)
        for y in range(192)) + bytes(40 * 48)

    # The enlarged task-local buffers stay wholly in ordinary 512 KiB RAM.
    source = (TASK / 'consumer/probe.asm').read_text()
    assert source.count('ld bc,0x010000') == 1
    assert source.count('observations: blkb 4096,0') == 1
    source = source.replace('ld bc,0x010000', 'ld bc,0x030000')
    source = source.replace('observations: blkb 4096,0',
                            'observations: blkb 40000,0\nobservations_end:')
    (HERE / 'probe.asm').write_text(source)
    subprocess.run([str(PROJECT / 'docs/tasks/GOLEM-007/build/ez80asm'),
                    'probe.asm', 'build/probe.bin', '-l', '-c'], cwd=HERE, check=True)
    assert (BUILD / 'probe.bin').stat().st_size < 0x10000
    common = support('build')
    variants = []
    for suite in contract['suites']:
        entry = suite['entry_id']
        # Each prefix gets an independent physical VGA capture. Only the final
        # prefix and mode rejection read every retained word, outside hot calls.
        for end in range(1, len(suite['steps']) + 1):
            variants.append((f'f{entry}-step{end:02}', suite, end, False))
        variants.append((f'f{entry}-mode8', suite, 0, True))
    summary = []
    for name, suite, end, unsupported in variants:
        out = BUILD / 'cases' / name / 'build'
        out.mkdir(parents=True, exist_ok=True)
        s = common.Script()
        s.receive(bytes([23, 0, 0xD9]), 0, 1, None, 'discovery')
        s.info(0, 0x0100, 'protocol version')
        s.info(1, 3, 'buffer and packed framebuffer capabilities')
        s.info(2, 0, 'initial handle absent')
        s.load(module, 'Apple F2/F3 codec load')
        entry, target = suite['entry_id'], suite['target_buffer']
        source_id, arguments = suite['source_buffer'], suite['argument_buffer']
        raw = (fixtures / ('lode.raw' if unsupported else suite['initial_target'])).read_bytes()
        s.send(common.write(target, raw))
        s.send(bytes([22, 8 if unsupported else 11, 12, 23, 1, 0]))
        steps = suite['independent_service_cases'] if unsupported else suite['steps'][:end]
        for step in steps:
            args = (fixtures / step['argument_file']).read_bytes()
            tid, sid, length, x, y, width, height = struct.unpack('<HHHhhHH', args)
            assert tid == target and (not length or sid == source_id)
            if step['source_file']:
                encoded = (fixtures / step['source_file']).read_bytes()
                assert length <= len(encoded)
                s.send(common.write(source_id, encoded))
            else:
                s.send(common.packet(source_id, 2))
            status, predicted = (0, raw) if not length else decode(raw, encoded[:length], entry)
            if not status and unsupported:
                status, predicted = 9, raw
            elif not status and (x < 0 or y < 0 or x+width > 320 or y+height > 240):
                status, predicted = 3, raw
            assert status == step['expected_status'], step['name']
            assert predicted == (fixtures / step['expected_target']).read_bytes(), step['name']
            raw = predicted
            s.send(common.write(arguments, args))
            s.transaction(2, struct.pack('<HHH', 0, entry, arguments),
                          3 if status == 0 else 5, status, step['name'], handle=True)
            # First/middle/last and live image words observe retention after
            # every call. A complete retained comparison follows final cases.
            offsets = (0, 38, 40, 158, 1000, 2048, 3838, 3840, 6000, 7678)
            for offset in offsets:
                word = struct.unpack_from('<H', raw, offset)[0]
                s.transaction(5, struct.pack('<HI', target, offset), 4, word,
                              step['name'] + f' retained word {offset}')
        complete = unsupported or end == len(suite['steps'])
        if complete:
            for offset in range(0, len(raw), 2):
                word = struct.unpack_from('<H', raw, offset)[0]
                s.transaction(5, struct.pack('<HI', target, offset), 4, word,
                              f'complete retained word {offset}')
        s.info(17, None, 'command task minimum free stack bytes')
        s.wait_frames(2)
        s.unload()
        s.info(2, 0, 'final handle absent')
        for ident in (common.STAGING, target, source_id, arguments):
            s.send(common.packet(ident, 2))
        labels = (f'VDPLET: Apple F{entry} native codec\r\n'
                  'ASM: loads, calls, unloads module.\r\n').encode()
        assert all(len(line) < 40 for line in labels.split(b'\r\n'))
        s.send(bytes([28, 0, 29, 39, 24, 31, 0, 0]) + labels)
        s.data.append(0)
        assert len(s.data) <= 0x030000 and len(s.checks)*10 <= 40000
        (out / 'script.bin').write_bytes(s.data)
        shutil.copyfile(BUILD / 'probe.bin', out / 'probe.bin')
        pixels = bytes(6720) if unsupported else unpack_image(raw)
        assets = out / 'apple-assets'
        assets.mkdir(exist_ok=True)
        (assets / 'loderunner.packed').write_bytes(pixels)
        asset = dict(packed_sha256=sha(assets / 'loderunner.packed'),
                     source='independent bit-order oracle from owner retained fixtures')
        (assets / 'assets.json').write_text(json.dumps(asset, indent=2)+'\n')
        meta = dict(checks=s.checks, reply_bytes=len(s.checks)*10,
                    request_wire_bytes=s.wire, reply_wire_bytes=12*len(s.checks),
                    script_bytes=len(s.data), source='codec/probe.asm',
                    probe_sha256=sha(out / 'probe.bin'), script_sha256=sha(out / 'script.bin'),
                    fixture=name, module_loads=1, packed_asset=asset,
                    modules={module.name:dict(bytes=module.stat().st_size, sha256=sha(module))},
                    codec_entry=entry, sequence_prefix=end, unsupported_mode_case=unsupported,
                    retained_bytes_observed_completely=len(raw) if complete else 0,
                    retained_sha256=hashlib.sha256(raw).hexdigest(),
                    notes='All readback is test-only. Uploaded native code performs decode/render.')
        (out / 'expected.json').write_text(json.dumps(meta, indent=2)+'\n')
        summary.append({k:v for k,v in meta.items() if k != 'checks'})
    (BUILD / 'cases.json').write_text(json.dumps(summary, indent=2)+'\n')
    (BUILD / 'source-pins.json').write_text(json.dumps(dict(
        owner=str(OWNER), module_sha256=MODULE_SHA,
        files={str(p.relative_to(supplied)):sha(p) for p in sorted(supplied.rglob('*')) if p.is_file()},
        independent_oracle='decode validates before applying; per-pixel bit mapping instead of module lookup table'),
        indent=2)+'\n')
    print(f'Built {len(summary)} independent MOS/VGA cases from supplied Xtensa package.')


if __name__ == '__main__':
    generate()
