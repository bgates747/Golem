#!/usr/bin/env python3
"""Execute the unchanged MOS proof tape through Fab's VDP ABI for diagnosis.

This isolates uploaded Xtensa code from eZ80 boot; it does not qualify MOS/SD.
The tape and expected replies remain owned by the assembly consumer.
"""
from __future__ import annotations
import argparse
import ctypes
from collections import deque
import hashlib
import json
import os
from pathlib import Path
import struct
import threading
import time

HERE = Path(__file__).resolve().parent
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('firmware', type=Path)
p.add_argument('--backend', type=Path, default=HERE / 'build/flexe-vdp.so')
p.add_argument('--tape', type=Path, default=HERE.parent / 'consumer/build/script.bin')
p.add_argument('--expected', type=Path, default=HERE.parent / 'consumer/build/expected.json')
p.add_argument('--engine', choices=['jit', 'interp'], default='jit')
p.add_argument('--artifacts', type=Path, default=HERE / 'build/tape-check')
p.add_argument('--timeout', type=float, default=240)
a = p.parse_args()
a.artifacts.mkdir(parents=True, exist_ok=True)
os.environ['AGON_FLEXE_FIRMWARE'] = str(a.firmware.resolve())
os.environ['AGON_FLEXE_ENGINE'] = a.engine
os.environ['AGON_FLEXE_ARTIFACTS'] = str(a.artifacts.resolve())

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

identity = dict(firmware_sha256=sha(a.firmware), backend_sha256=sha(a.backend),
                tape_sha256=sha(a.tape), expected_sha256=sha(a.expected))
if os.environ.get('AGON_FLEXE_ELF'):
    identity['elf_sha256'] = sha(Path(os.environ['AGON_FLEXE_ELF']))
tape = a.tape.read_bytes()
expected = json.loads(a.expected.read_text())
if identity['tape_sha256'] != expected['script_sha256']:
    raise ValueError('Consumer expected.json does not describe this tape')
lib = ctypes.CDLL(str(a.backend.resolve()))
lib.z80_uart0_is_cts.restype = ctypes.c_bool
lib.z80_recv_from_vdp.argtypes = [ctypes.POINTER(ctypes.c_uint8)]
lib.z80_recv_from_vdp.restype = ctypes.c_bool
lib.z80_send_to_vdp.argtypes = [ctypes.c_uint8]
lib.flexe_vdp_failed.restype = ctypes.c_bool
lib.flexe_vdp_cycles.restype = ctypes.c_uint64
lib.flexe_vdp_instruction_syncs.restype = ctypes.c_uint64
lib.flexe_vdp_code_invalidations.restype = ctypes.c_uint64
lib.vdp_setup()
worker = threading.Thread(target=lib.vdp_loop)
worker.start()
started = last_status = time.monotonic()
raw = bytearray()
packets = deque()
parser = bytearray()
observations = bytearray()
passed = False
failure = None

def poll():
    global last_status
    now = time.monotonic()
    if now - last_status > 5:
        lib.dump_vdp_mem_stats()
        last_status = now
    byte = ctypes.c_uint8()
    while lib.z80_recv_from_vdp(ctypes.byref(byte)):
        value = byte.value
        raw.append(value)
        if not parser and value < 128:
            continue
        parser.append(value)
        if len(parser) >= 2 and len(parser) == parser[1] + 2:
            packets.append((parser[0], bytes(parser[2:])))
            parser.clear()
    if lib.flexe_vdp_failed():
        raise RuntimeError('ESP32 backend stopped or faulted')
    if now - started > a.timeout:
        raise TimeoutError(f'Proof deadline at cycles={lib.flexe_vdp_cycles()}')

def wait(predicate):
    while True:
        poll()
        if predicate():
            return
        time.sleep(.001)

def send(wire):
    for byte in wire:
        wait(lib.z80_uart0_is_cts)
        lib.z80_send_to_vdp(byte)

try:
    wait(lib.z80_uart0_is_cts)
    send(bytes([23, 0, 0x80, 0x42]))
    wait(lambda: (0x80, b'\x42') in packets)
    packets.clear()
    offset = index = handle = 0
    while True:
        kind = tape[offset]
        offset += 1
        if kind == 0:
            break
        if kind == 1:
            length = struct.unpack_from('<H', tape, offset)[0]
            offset += 2
            send(tape[offset:offset + length])
            offset += length
            continue
        if kind != 2:
            raise ValueError(f'Invalid tape record {kind}')
        token, reply_kind, flags, value, patch, length = struct.unpack_from('<HBBHHH', tape, offset)
        offset += 10
        wire = bytearray(tape[offset:offset + length])
        offset += length
        if patch != 65535:
            struct.pack_into('<H', wire, patch, handle)
        check = expected['checks'][index]
        if (token, reply_kind) != (check['token'], check['kind']):
            raise ValueError('Tape metadata differs from expected check')
        send(wire)
        reply = None
        while reply is None:
            wait(lambda: bool(packets))
            header, payload = packets.popleft()
            if header == 0x81 and payload.startswith(b'VDPL'):
                reply = payload
        if len(reply) != 10:
            raise ValueError(f'Malformed vdplet reply: {reply.hex()}')
        signature, version, observed_kind, observed_token, observed_value = struct.unpack('<4sBBHH', reply)
        if (version, observed_kind, observed_token) != (1, reply_kind, token):
            raise ValueError(f'{check["label"]}: wrong reply {reply.hex()}')
        if flags & 1:
            if not observed_value:
                raise ValueError(f'{check["label"]}: zero module handle')
            handle = observed_value
        elif not flags & 2 and observed_value != value:
            raise ValueError(f'{check["label"]}: expected {value:#x}, got {observed_value:#x}')
        observations.extend(reply)
        index += 1
        print(f'{index}: {check["label"]}: {observed_value:#06x}', flush=True)
    if index != len(expected['checks']) or offset != len(tape):
        raise ValueError('Incomplete tape execution')
    passed = True
except BaseException as exc:
    failure = f'{type(exc).__name__}: {exc}'
    raise
finally:
    receipt = dict(**identity, passed=passed, failure=failure, engine=a.engine,
                   checks=len(observations) // 10, reply_bytes=len(observations),
                   observed_sha256=hashlib.sha256(observations).hexdigest(),
                   cycles=lib.flexe_vdp_cycles(),
                   instruction_syncs=lib.flexe_vdp_instruction_syncs(),
                   code_invalidations=lib.flexe_vdp_code_invalidations(),
                   wall_seconds=time.monotonic() - started,
                   qualification_scope='Direct Fab VDP ABI; MOS/SD remains a separate gate')
    lib.vdp_shutdown()
    worker.join(timeout=10)
    if worker.is_alive():
        raise RuntimeError('Backend did not stop')
    (a.artifacts / 'results.dat').write_bytes(observations)
    (a.artifacts / 'check.json').write_text(json.dumps(receipt, indent=2) + '\n')
print(json.dumps(receipt, indent=2))
