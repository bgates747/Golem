#!/usr/bin/env python3
"""Finite original 6502 toy gate using the unchanged VDPL-012 transport.

Preparation imports no emulator library. Execution requires the coordinator's
separate Pi permit and its actual native ARM64 artifact pin; no Oryx lease is
created or inferred by this helper.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import platform
import struct
import sys
import time

HERE = Path(__file__).resolve().parent
SDK = Path('/Agon/mystuff/agon-vdplet')
QUALIFIER = SDK / 'docs/tasks/VDPL-012/qualification'
CONSUMER = Path('/Agon/mystuff/agon-6502')
PACKAGE = CONSUMER / 'build/vdp005/module/vdp005-text32k.vdpl'
ORACLE = CONSUMER / 'build/vdp005/color/maintained-controls/vdp005-text32k-jit-r01/toy-state.bin'
FIRMWARE = SDK / 'build/vdpl-010/pixels/firmware-src/.pio/build/esp32dev/firmware.bin'
EXPECTED = {
    'target.py': '66ad78c3838179703e8f9f5149028b68317f778995e463c95b4f1c9cc93d5228',
    'package': '7113d107a7c91fc56c6e65169c1d3a17fc7cc7771a125373505c421cf46f68ef',
    'oracle': 'ab5a0785dc71ee37fbc2e7af8fbe75fb55fe207a09801f518945f93a52a9e9e2',
    'firmware': 'b42ae55f69900c44acacfbf0d665ef278e87b277b3b9fd4208e17ebf378f74b6',
    'elf': 'a10f7989d5c1f9974fc999428865ca74771fb65a0d4d694a2ac2b81c19df9902',
}
WORK, MEMORY, RESULT, ARGS, IMAGE = 58501, 58502, 58503, 58504, 58506


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def checked_inputs():
    paths = {'target.py': QUALIFIER / 'target.py', 'package': PACKAGE,
             'oracle': ORACLE, 'firmware': FIRMWARE, 'elf': FIRMWARE.with_suffix('.elf')}
    for name, path in paths.items():
        assert sha(path) == EXPECTED[name], (name, path)
    assert PACKAGE.stat().st_size == 35876 and ORACLE.stat().st_size == 64
    assert struct.calcsize('<4H6I') == 32
    return {name: dict(path=str(path), bytes=path.stat().st_size, sha256=sha(path))
            for name, path in paths.items()}


def resources(target, word):
    def wide(field):
        return target.transaction(4, word(field), 4) | (target.transaction(4, word(field + 1), 4) << 16)
    return dict(executable_free_bytes=wide(9), executable_largest_bytes=wide(11),
                internal_free_bytes=wide(13), internal_largest_bytes=wide(15),
                spiram_free_bytes=wide(18), spiram_largest_bytes=wide(20),
                command_stack_free_bytes=target.transaction(4, word(17), 4),
                audio_stack_free_bytes=target.transaction(4, word(22), 4),
                tasks=target.transaction(4, word(23), 4))


def native_backend(path, expected):
    assert platform.machine().lower() in ('aarch64', 'arm64'), platform.machine()
    assert len(expected) == 64 and all(c in '0123456789abcdef' for c in expected)
    assert sha(path) == expected, 'native backend must match root-supplied pin'
    header = path.read_bytes()[:20]
    assert header[:6] == b'\x7fELF\x02\x01' and struct.unpack_from('<H', header, 18)[0] == 183


def run(args, pins):
    native_backend(args.backend, args.backend_sha256)
    args.output.mkdir(parents=True, exist_ok=False)
    # Importing this frozen Python module defines Target but does not load a .so.
    sys.dont_write_bytecode = True
    spec = importlib.util.spec_from_file_location('vdpl012_frozen_target', QUALIFIER / 'target.py')
    transport = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(transport)
    Target, upload, word, command = transport.Target, transport.upload, transport.word, transport.command
    report = dict(passed=False, scope='Pi ARM64 finite mode11 CPU/lifecycle mechanism gate; no app rendering or physical timing',
                  architecture=platform.machine(), engine=args.engine, inputs=pins,
                  backend=dict(path=str(args.backend), sha256=args.backend_sha256),
                  harness_sha256=sha(__file__), calls=[], no_oryx_lease_used=True)
    (args.output / 'toy_gate-executed.py').write_bytes(Path(__file__).read_bytes())
    target, handle, started = None, 0, time.monotonic()
    try:
        target = Target(args.engine, args.output, args.backend, FIRMWARE, args.timeout)
        target.send(bytes((22, 11)))
        target.transaction(4, word(0), 4, 0x0100)
        target.transaction(4, word(1), 4, 31)
        target.transaction(4, word(2), 4, 0)
        report['before_load'] = resources(target, word)
        handle = target.load(PACKAGE, IMAGE)
        assert handle
        report['loaded_handle'], report['init_complete'] = handle, True
        report['text_bytes'] = target.transaction(4, word(4), 4)
        report['data_bytes'] = target.transaction(4, word(5), 4)
        assert (report['text_bytes'], report['data_bytes']) == (35072, 4)
        for key, data in ((WORK, bytes(64)), (MEMORY, bytes(65536 + 8)),
                          (RESULT, bytes(16 + 8)), (ARGS, bytes(32))):
            target.send(upload(key, data))
        for entry, budget in ((0, 0), (1, 8)):
            body = struct.pack('<4H6I', WORK, MEMORY, RESULT, 0,
                               budget, 65536, 16, 0, 8, 0)
            target.send(upload(ARGS, body))
            target.run(handle, entry, arguments=ARGS)
            report['calls'].append(dict(entry=entry, budget=budget, arguments_hex=body.hex()))
        state = target.read(WORK, 64)
        (args.output / 'toy-state.bin').write_bytes(state)
        assert state == ORACLE.read_bytes(), 'complete 64-byte toy state differs from generation21 oracle'
        assert struct.unpack_from('<I', state, 8)[0] == 3
        assert struct.unpack_from('<II', state, 16) == (8, 29)
        assert state[-4:] == struct.pack('<I', 0xc35aa53c)
        report.update(toy_state_sha256=hashlib.sha256(state).hexdigest(), toy_steps=8,
                      toy_cycles=29, complete_64byte_oracle_match=True)
        target.unload(handle)
        handle = 0
        report['fini_unload_complete'] = True
        for key in (WORK, MEMORY, RESULT, ARGS, IMAGE):
            target.send(command(key, 2))
        target.transaction(4, word(2), 4, 0)
        report['after_unload'] = resources(target, word)
        assert report['before_load']['executable_free_bytes'] == report['after_unload']['executable_free_bytes']
        assert report['after_unload']['command_stack_free_bytes'] > 0
        assert report['after_unload']['tasks'] == report['before_load']['tasks']
        report.update(empty_slot=True, same_mode_executable_heap_recovered=True, passed=True)
    except BaseException as exc:
        report['failure'] = f'{type(exc).__name__}: {exc}'
        raise
    finally:
        if target is not None:
            if handle:
                try:
                    target.unload(handle)
                except BaseException as exc:
                    report['cleanup_unload_failure'] = str(exc)
            try:
                report['execution_stats'] = target.close()
                assert not any(report['execution_stats'].get(key) for key in ('failed', 'unhandled', 'unregistered', 'unmapped'))
            except BaseException as exc:
                report['passed'] = False
                report['close_failure'] = str(exc)
                raise
            finally:
                report.update(elapsed_seconds=round(time.monotonic() - started, 3),
                              result_bytes=len(target.observed), outbound_bytes=target.sent)
                (args.output / 'evidence.json').write_text(json.dumps(report, indent=2) + '\n')
        else:
            report['elapsed_seconds'] = round(time.monotonic() - started, 3)
            (args.output / 'evidence.json').write_text(json.dumps(report, indent=2) + '\n')
    print('PASS', args.engine, 'original toy: 8 steps / 29 cycles, all 64 state bytes and finite lifecycle', flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepare-only', action='store_true')
    parser.add_argument('--backend', type=Path)
    parser.add_argument('--backend-sha256')
    parser.add_argument('--engine', choices=('jit', 'interp'))
    parser.add_argument('--output', type=Path)
    parser.add_argument('--timeout', type=float, default=600)
    args = parser.parse_args()
    pins = checked_inputs()
    if args.prepare_only:
        print(json.dumps(dict(prepared=True, emulator_instantiated=False, inputs=pins), indent=2))
        return
    if any(getattr(args, name) is None for name in ('backend', 'backend_sha256', 'engine', 'output')):
        parser.error('execution needs --backend, --backend-sha256, --engine, --output and a separate root Pi permit')
    args.backend, args.output = args.backend.resolve(), args.output.resolve()
    run(args, pins)


if __name__ == '__main__':
    main()
