#!/usr/bin/env python3
"""Pi-only, silent/headless qualification of resident Flower line rendering."""
from pathlib import Path
import argparse
import binascii
import ctypes
import hashlib
import json
import os
import struct
import sys
import time
import zlib

HERE = Path(__file__).resolve().parent
MATH = HERE.parent / 'native-math'
sys.path.insert(0, str(MATH))
from host_checks import ROOT, SDK, ASSEMBLER, run, load, compare, reference
from qualify_headless import VDP, word, packet, write, call, update, sha
from build_renderer import generate, plot

WIDTH, HEIGHT = 640, 480
BLACK, YELLOW = bytes((0, 0, 0)), bytes((255, 255, 0))


def prepare():
    (HERE / 'build').mkdir(exist_ok=True)
    admit = HERE / 'build/admit'
    run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
         '-fsanitize=address,undefined', '-g', '-I', SDK / 'src', HERE.parent / 'admit_probe.cpp', '-o', admit])
    receipts = []
    for name in ('default', 'relocated'):
        package = HERE / 'build' / name
        args = [ROOT / 'build/golemc', '--portable', '--target', 'native']
        if name == 'relocated':
            args += ['--reserve', '1:4095']
        run(args + [HERE.parent / 'flower.golem', package])
        ids = (60002, 60003, 60004, 60005) if name == 'default' else (60020, 60021, 60022, 60023)
        meta = generate(package, package, ids)
        run([sys.executable, SDK / 'tools/build_module.py', package / 'native.c', '--math',
             '--name', 'flower', '--build-id', '0x474c3434', '--output', package])
        admission = json.loads(run([admit, package / 'flower.vdpl']))
        assert admission['status'] == 0, admission
        (package / 'consumer.asm').write_text(' .assume adl=1\n .org $40000\n include "layout.inc"\n'
                                             ' include "calls.inc"\n include "renderer.inc"\n'
                                             ' call golem_native_batch\n call flower_render\n ret\n')
        run([ASSEMBLER, 'consumer.asm', 'consumer.bin', '-c'], cwd=package)
        receipts.append({'layout': name, 'actual_assembler': True, 'loader': admission,
                         'module_sha256': sha(package / 'flower.vdpl'), 'renderer': meta})
    # Array buffer IDs are embedded in generated native C, so relocation changes its image.
    evidence = HERE / 'evidence'
    evidence.mkdir(exist_ok=True)
    (evidence / 'packages.json').write_text(json.dumps(receipts, indent=2) + '\n')


class Display:
    def __init__(self, vdp):
        self.vdp = vdp
        vdp.lib.flexe_vdp_frames.restype = ctypes.c_uint64
        vdp.lib.copyVgaFramebuffer.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32),
                                             ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(ctypes.c_float)]
        self.pixels = (ctypes.c_uint8 * (1024 * 768 * 3))()

    def barrier(self):
        # Stock screen-pixel query waits for queued Canvas plotting to complete.
        self.vdp.frames.clear()
        self.vdp.send(bytes((23, 0, 132, 0, 0, 0, 0)))
        self.vdp.wait(lambda: any(key == 132 for key, _ in self.vdp.frames))
        frames = list(self.vdp.frames)
        self.vdp.frames.clear()
        data = next(data for key, data in frames if key == 132)
        assert len(data) == 4

    def setup(self):
        self.vdp.send(bytes((22, 0, 23, 1, 0, 19, 0, 255, 0, 0, 0, 19, 1, 255, 255, 255, 0,
                             23, 0, 192, 1, 26, 29)) + word(640) + word(512) + bytes((18, 0, 1, 12)))
        self.barrier()

    def clear(self):
        self.vdp.send(bytes((16,)))
        self.barrier()

    def capture(self):
        self.barrier()
        first = self.vdp.lib.flexe_vdp_frames()
        self.vdp.wait(lambda: self.vdp.lib.flexe_vdp_frames() >= first + 3)
        w, h, rate = ctypes.c_uint32(), ctypes.c_uint32(), ctypes.c_float()
        self.vdp.lib.copyVgaFramebuffer(ctypes.byref(w), ctypes.byref(h), self.pixels, ctypes.byref(rate))
        assert (w.value, h.value) == (WIDTH, HEIGHT), (w.value, h.value)
        image = bytes(self.pixels[:WIDTH * HEIGHT * 3])
        assert all(image[i:i+3] in (BLACK, YELLOW) for i in range(0, len(image), 3)), 'unexpected palette or incomplete scanout'
        return image


def mapped(point):
    x, y = point
    return 320 + int(x / 2.0), 239 + int(-y / (1024.0 / 480.0))


def axis_image(points):
    """Independent exact oracle for cardinal lines, including clipping/endpoints."""
    pixels = set()
    for a, b in zip(points, points[1:]):
        x, y = mapped(a)
        xx, yy = mapped(b)
        assert x == xx or y == yy
        dx, dy = (xx > x) - (xx < x), (yy > y) - (yy < y)
        while True:
            if 0 <= x < WIDTH and 0 <= y < HEIGHT:
                pixels.add((x, y))
            if (x, y) == (xx, yy):
                break
            x += dx
            y += dy
    image = bytearray(BLACK * (WIDTH * HEIGHT))
    for x, y in pixels:
        at = (y * WIDTH + x) * 3
        image[at:at+3] = YELLOW
    return bytes(image)


def direct(points):
    return b'' if len(points) < 2 else plot(4, *points[0]) + b''.join(plot(5, *p) for p in points[1:])


def save_png(path, image):
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', binascii.crc32(kind + data))
    rows = b''.join(b'\0' + image[y*WIDTH*3:(y+1)*WIDTH*3] for y in range(HEIGHT))
    path.write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', WIDTH, HEIGHT, 8, 2, 0, 0, 0))
                     + chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b''))


def expect_image(a, b, label):
    assert len(a) == len(b) == WIDTH * HEIGHT * 3
    mismatches = sum(a[i:i+3] != b[i:i+3] for i in range(0, len(a), 3))
    assert mismatches == 0, (label, 'mismatched pixels', mismatches)
    return WIDTH * HEIGHT


def qualify(engine, timeout):
    assert os.uname().machine == 'aarch64', 'this tranche is authorized for the Pi 5 only'
    output = HERE / 'build' / ('pi-' + engine)
    vdp = VDP(engine, output, timeout)
    display = Display(vdp)
    report = {'engine': engine, 'host': os.uname().nodename, 'scope': 'Pi VDP-only actual Xtensa plus stock command renderer; no MOS/client/physical test',
              'graphical_launch': False, 'oryx_accessed': False, 'firmware_sha256': sha(vdp.firmware),
              'elf_sha256': sha(vdp.elf), 'adapter_sha256': sha(vdp.backend),
              'renderer_cases': [], 'curves': [], 'pixel_comparisons': 0, 'timings': []}
    passed = False
    failure = None
    try:
        display.setup()
        for layout in ('default', 'relocated'):
            package = HERE / 'build' / layout
            iface, ast, root, backend = load(package)
            render = json.loads((package / 'renderer.json').read_text())
            fields = {f['name']: f for f in iface['globals']}
            arrays = {a['name']: a for a in iface['arrays']}
            initial = (package / 'state.bin').read_bytes()
            vdp.install(package, backend, 'flower')
            vdp.send((package / 'renderer-install.bin').read_bytes())
            batch_entry = render['batch_entry']
            remaining_at = fields['remaining']['offset']

            def set16(state, name, value):
                at = fields[name]['offset'] if name in fields else root['fields'][name]
                struct.pack_into('<H', state, at, value & 65535)

            def install_state(state, points):
                vdp.send(write(backend['shared_state'], state))
                for name, dim in [('pointsX', 0), ('pointsY', 1)]:
                    vdp.send(write(arrays[name]['buffer'], struct.pack('<32h', *(p[dim] for p in points))))

            # Tail endpoints are deliberately hostile: a renderer must not consume padding.
            path = []
            x, y = -300, 200
            for slot in range(32):
                if slot % 2:
                    x += 20
                else:
                    y = -y
                path.append((x, y))
            for count in (0, 1, 31, 32, 33, 256, 65535):
                state = bytearray(initial)
                for name, value in [('last_success_entry', batch_entry), ('requested_entry', batch_entry),
                                    ('batchCount', count), ('startX', -300), ('startY', 200)]:
                    set16(state, name, value)
                install_state(state, path)
                display.clear()
                vdp.send(call(render['entry']))
                actual = display.capture()
                points = [(-300, 200)] + path[:count] if 1 <= count <= 32 else []
                report['pixel_comparisons'] += expect_image(actual, axis_image(points), (layout, count))
                assert vdp.read(backend['shared_state'], len(state)) == state
                for name, dim in [('pointsX', 0), ('pointsY', 1)]:
                    assert vdp.read(arrays[name]['buffer'], 64) == struct.pack('<32h', *(p[dim] for p in path))
                # A repeated call must use current resident data and preserve mathematical state.
                vdp.send(call(render['entry']))
                report['pixel_comparisons'] += expect_image(actual, display.capture(), (layout, count, 'repeat'))
                report['renderer_cases'].append({'layout': layout, 'count': count, 'pixels_exact': True, 'state_arrays_preserved': True})

            # Signed scaling, clipped lines and zero-length segments have an independent oracle.
            points = [(-2000, 0), (2000, 0), (2000, 2000), (0, 2000), (0, -2000), (0, -2000)]
            state = bytearray(initial)
            for name, value in [('last_success_entry', batch_entry), ('requested_entry', batch_entry),
                                ('batchCount', len(points)-1), ('startX', points[0][0]), ('startY', points[0][1])]:
                set16(state, name, value)
            install_state(state, points[1:] + [(32767, -32768)] * (33-len(points)))
            display.clear()
            vdp.send(call(render['entry']))
            report['pixel_comparisons'] += expect_image(display.capture(), axis_image(points), (layout, 'clipping'))

            points = [(-5, -7), (5, -7), (5, 7), (-5, 7)]
            for name, value in [('batchCount', len(points)-1), ('startX', points[0][0]), ('startY', points[0][1])]:
                set16(state, name, value)
            install_state(state, points[1:] + [(32767, -32768)] * (33-len(points)))
            display.clear()
            vdp.send(call(render['entry']))
            report['pixel_comparisons'] += expect_image(display.capture(), axis_image(points), (layout, 'signed truncation'))

            # Fail closed on either byte of each status, and on the wrong successful/requested entry.
            for name, value in [('interface_status', 1), ('interface_status', 256), ('interface_status', 65535),
                                ('language_status', 33), ('language_status', 256), ('language_status', 65535),
                                ('runtime_result', 1), ('runtime_result', 256), ('runtime_result', 65535),
                                ('last_success_entry', 0), ('requested_entry', 0),
                                ('last_success_entry', 65535), ('requested_entry', 65535)]:
                failed = bytearray(state)
                set16(failed, name, value)
                vdp.send(write(backend['shared_state'], failed))
                display.clear()
                vdp.send(call(render['entry']))
                report['pixel_comparisons'] += expect_image(display.capture(), BLACK * (WIDTH * HEIGHT), (layout, name, value))
            failed = bytearray(state)
            struct.pack_into('<I', failed, root['fields']['runtime_result'], 65536)
            vdp.send(write(backend['shared_state'], failed))
            display.clear()
            vdp.send(call(render['entry']))
            report['pixel_comparisons'] += expect_image(display.capture(), BLACK * (WIDTH * HEIGHT), (layout, 'runtime high word'))

            trials = [('defaults', {}), ('fractional-angle', {'theta_init': 22.5}), ('full-circle', {'theta_init': 360}),
                      ('integer-count', {'petals': 3, 'vectors': 2, 'periods': 2}),
                      ('zero-shrink', {'shrink': 0}), ('full-shrink', {'shrink': 1}),
                      ('zero-radius', {'radius_scale': 0})]
            if layout == 'relocated':
                trials = trials[:1]
            for name, changes in trials:
                state = bytearray(initial)
                for field, value in changes.items():
                    struct.pack_into('<f', state, fields[field]['offset'], value)
                installed_arrays = {a['buffer']: bytearray((package / a['initial_data']).read_bytes()) for a in iface['arrays']}
                install_state(state, [(0, 0)] * 32)
                vdp.send(call(backend['native_wrappers'][0]))
                vdp.transaction(4, word(3), 4, 0)
                wanted = reference(iface, ast, state, 0, root, arrays=installed_arrays)
                state = vdp.read(backend['shared_state'], len(initial))
                worst = compare(wanted, state, iface, flower=True)
                display.clear()  # Real client clears only after successful preparation.
                points = [(struct.unpack_from('<h', state, fields['previousX']['offset'])[0],
                           struct.unpack_from('<h', state, fields['previousY']['offset'])[0])]
                segments = 0
                batch_sizes = []
                while struct.unpack_from('<H', state, remaining_at)[0]:
                    before, cycles, started = vdp.sent, vdp.lib.flexe_vdp_cycles(), time.monotonic()
                    vdp.send(call(backend['native_wrappers'][batch_entry]))
                    vdp.transaction(4, word(3), 4, 0)  # Runtime success before any render submission.
                    vdp.send(call(render['entry']))
                    remaining = vdp.transaction(5, struct.pack('<HI', backend['shared_state'], remaining_at), 4)
                    display.barrier()
                    sent = vdp.sent - before
                    assert sent == 49, sent
                    report['timings'].append({'layout': layout, 'case': name, 'outbound_bytes': sent,
                                              'guest_cycles': vdp.lib.flexe_vdp_cycles()-cycles,
                                              'wall_seconds': time.monotonic()-started})
                    expected_arrays = {k: bytearray(v) for k, v in installed_arrays.items()}
                    wanted = reference(iface, ast, state, batch_entry, root, arrays=expected_arrays)
                    state = vdp.read(backend['shared_state'], len(initial))
                    worst = max(worst, compare(wanted, state, iface, flower=True))
                    assert struct.unpack_from('<H', state, remaining_at)[0] == remaining
                    count = struct.unpack_from('<H', state, fields['batchCount']['offset'])[0]
                    assert 1 <= count <= 32
                    assert (struct.unpack_from('<h', state, fields['startX']['offset'])[0],
                            struct.unpack_from('<h', state, fields['startY']['offset'])[0]) == points[-1]
                    coords = []
                    for a in iface['arrays']:
                        data = vdp.read(a['buffer'], a['bytes'])
                        actual = struct.unpack('<32h', data)
                        expected = struct.unpack('<32h', expected_arrays[a['buffer']])
                        assert all(abs(x-y) <= 1 for x, y in zip(actual, expected))
                        installed_arrays[a['buffer']] = bytearray(data)
                        coords.append(actual)
                    points += list(zip(*coords))[:count]
                    segments += count
                    batch_sizes.append(count)
                resident = display.capture()
                display.clear()
                vdp.send(direct(points))
                report['pixel_comparisons'] += expect_image(resident, display.capture(), (layout, name, 'direct/resident'))
                assert segments == (13 if name == 'integer-count' else 396)
                if name == 'zero-radius':
                    report['pixel_comparisons'] += expect_image(resident, axis_image(points), 'zero radius')
                # Calling an exhausted batch changes count to zero and must not add a padded line.
                vdp.send(call(backend['native_wrappers'][batch_entry]))
                vdp.transaction(4, word(3), 4, 0)
                vdp.send(call(render['entry']))
                report['pixel_comparisons'] += expect_image(resident, display.capture(), (layout, name, 'exhausted'))
                save_png(HERE / 'evidence' / f'{layout}-{name}-{engine}.png', resident)
                report['curves'].append({'layout': layout, 'case': name, 'segments': segments, 'batch_sizes': batch_sizes,
                                         'pixels_sha256': hashlib.sha256(resident).hexdigest(),
                                         'max_float_ulps': worst, 'direct_stock_pixels_identical': True})
                print(engine, layout, name, segments, 'segments / all pixels match', flush=True)

            # An actual calculation may fail after writing one point. Suppress the entire batch.
            state = bytearray(initial)
            for name, value in [('remaining', 32), ('previousX', 0), ('previousY', 0)]:
                set16(state, name, value)
            struct.pack_into('<f', state, fields['mainStep']['offset'], 3.4028234663852886e38)
            install_state(state, [(321, -123)] * 32)
            display.clear()
            old_picture = [(-400, -300), (400, -300)]
            vdp.send(direct(old_picture))
            vdp.send(call(backend['native_wrappers'][batch_entry]))
            vdp.transaction(4, word(3), 4, 1)
            failed = vdp.read(backend['shared_state'], len(initial))
            assert struct.unpack_from('<H', failed, fields['batchCount']['offset'])[0] == 1
            assert struct.unpack_from('<H', failed, root['fields']['language_status'])[0] == 33
            vdp.send(call(render['entry']))  # Deliberately challenge the resident guard, despite runtime failure.
            report['pixel_comparisons'] += expect_image(display.capture(), axis_image(old_picture), (layout, 'partial failure preserves picture'))
            assert vdp.read(backend['shared_state'], len(initial)) == failed

            retained = [vdp.read(backend['shared_state'], len(initial))] + [vdp.read(a['buffer'], a['bytes']) for a in iface['arrays']]
            vdp.send((package / 'renderer-cleanup.bin').read_bytes())
            vdp.transaction(3, word(vdp.handle), 3, 0)
            vdp.send((package / 'native-cleanup.bin').read_bytes())
            assert retained == [vdp.read(backend['shared_state'], len(initial))] + [vdp.read(a['buffer'], a['bytes']) for a in iface['arrays']]
            report['pixel_comparisons'] += expect_image(display.capture(), axis_image(old_picture), (layout, 'cleanup preserves picture'))
            report.setdefault('lifecycle', []).append({'layout': layout, 'partial_failed_batch_count': 1,
                'partial_failed_language_status': 33, 'failure_preserves_prior_picture': True,
                'cleanup_preserves_state_arrays_picture': True})
            # Root/array cleanup belongs to the application; renderer/native cleanup must not own it.
            for key in [backend['shared_state']] + [a['buffer'] for a in iface['arrays']]:
                vdp.send(packet(key, 2))
        report['stack_floor_bytes'] = vdp.transaction(4, word(17), 4)
        assert report['stack_floor_bytes'] >= 512
        passed = True
    except BaseException as exc:
        failure = f'{type(exc).__name__}: {exc}'
        raise
    finally:
        vdp.close()
        stats = json.loads((output / 'stats.json').read_text())
        report.update(passed=passed, failure=failure, elapsed_seconds=time.monotonic()-vdp.start, backend_stats=stats,
                      outbound_bytes=vdp.sent, vdplet_reply_bytes=len(vdp.observed))
        if passed:
            assert not any(stats.get(key) for key in ('failed', 'unhandled', 'unregistered', 'unmapped')), stats
        evidence = HERE / 'evidence'
        evidence.mkdir(exist_ok=True)
        (evidence / ('pi-' + engine + '.json')).write_text(json.dumps(report, indent=2) + '\n')
        (evidence / ('pi-' + engine + '-results.dat')).write_bytes(vdp.observed)
        print(engine, 'passed:', passed, 'pixel comparisons:', report['pixel_comparisons'], flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine', choices=('jit', 'interp'), default='jit')
    parser.add_argument('--timeout', type=float, default=900)
    parser.add_argument('--skip-build', action='store_true')
    parser.add_argument('--build-only', action='store_true')
    args = parser.parse_args()
    if not args.skip_build:
        prepare()
    if not args.build_only:
        qualify(args.engine, args.timeout)
