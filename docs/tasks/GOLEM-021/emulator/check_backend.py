#!/usr/bin/env python3
"""Headless Fab ABI gate: native firmware boot, UART, VGA, keys and shutdown."""
from __future__ import annotations
import argparse
import ctypes
import json
import hashlib
import os
from pathlib import Path
import threading
import time

p=argparse.ArgumentParser()
p.add_argument('firmware',type=Path)
p.add_argument('--backend',type=Path,default=Path(__file__).parent/'build/flexe-vdp.so')
p.add_argument('--artifacts',type=Path,default=Path(__file__).parent/'build/abi-check')
p.add_argument('--engine',choices=['jit','interp'],default='jit')
p.add_argument('--timeout',type=float,default=150)
p.add_argument('--media',action='store_true',help='also qualify standard modes and DAC audio through the Fab ABI')
a=p.parse_args()
a.artifacts.mkdir(parents=True,exist_ok=True)
os.environ['AGON_FLEXE_FIRMWARE']=str(a.firmware.resolve())
os.environ['AGON_FLEXE_ENGINE']=a.engine
os.environ['AGON_FLEXE_ARTIFACTS']=str(a.artifacts.resolve())
lib=ctypes.CDLL(str(a.backend.resolve()))
lib.z80_uart0_is_cts.restype=ctypes.c_bool
lib.z80_recv_from_vdp.argtypes=[ctypes.POINTER(ctypes.c_uint8)]
lib.z80_recv_from_vdp.restype=ctypes.c_bool
lib.z80_send_to_vdp.argtypes=[ctypes.c_uint8]
lib.flexe_inject_ascii.argtypes=[ctypes.c_uint8]
lib.sendPS2KbEventToFabgl.argtypes=[ctypes.c_uint16,ctypes.c_uint8]
lib.getAudioSamples.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_uint32]
lib.flexe_vdp_failed.restype=ctypes.c_bool
lib.flexe_vdp_frames.restype=ctypes.c_uint64
lib.flexe_vdp_cycles.restype=ctypes.c_uint64
lib.copyVgaFramebuffer.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_float)]
inputs={'firmware_sha256':hashlib.sha256(a.firmware.read_bytes()).hexdigest(),'backend_sha256':hashlib.sha256(a.backend.read_bytes()).hexdigest()}
if os.environ.get('AGON_FLEXE_ELF'):inputs['elf_sha256']=hashlib.sha256(Path(os.environ['AGON_FLEXE_ELF']).read_bytes()).hexdigest()
lib.vdp_setup()
worker=threading.Thread(target=lib.vdp_loop)
worker.start()
received=bytearray()
started=time.monotonic()
last_status=started

def poll():
    global last_status
    if time.monotonic()-last_status>5:
        lib.dump_vdp_mem_stats()
        last_status=time.monotonic()
    byte=ctypes.c_uint8()
    while lib.z80_recv_from_vdp(ctypes.byref(byte)):
        received.append(byte.value)
    if lib.flexe_vdp_failed():raise RuntimeError('ESP32 backend failed')
    if time.monotonic()-started>a.timeout:raise TimeoutError(f'backend deadline; cycles={lib.flexe_vdp_cycles()} reply={received.hex()}')

def wait(predicate):
    while True:
        poll()
        if predicate():return
        time.sleep(.001)

def send(data):
    for byte in data:
        wait(lib.z80_uart0_is_cts)
        lib.z80_send_to_vdp(byte)

result={**inputs,'passed':False,'engine':a.engine}
try:
    wait(lib.z80_uart0_is_cts)
    send(bytes([23,0,0x80,0x42]))
    handshake=bytes.fromhex('80014286088002e001503c1000')
    wait(lambda:handshake in received)
    print('Native ESP32/Fab ABI handshake passed.',flush=True)
    send(bytes([22,8,23,1,0,17,132,12])+b'FLEXE'+bytes([23,0,0x83,0,0,0,0,23,0,0x84,100,0,100,0]))
    expected=bytes.fromhex('86084001f000281e400883014684040000aa04')
    wait(lambda:expected in received)
    frames_before=lib.flexe_vdp_frames()
    wait(lambda:lib.flexe_vdp_frames()>=frames_before+3)
    w,h=ctypes.c_uint32(),ctypes.c_uint32();rate=ctypes.c_float()
    pixels=(ctypes.c_uint8*(1024*768*3))()
    lib.copyVgaFramebuffer(ctypes.byref(w),ctypes.byref(h),pixels,ctypes.byref(rate))
    assert (w.value,h.value)==(320,240),(w.value,h.value)
    rgb=bytes(pixels[:w.value*h.value*3])
    background=bytes([0,0,170])
    bad=sum(rgb[i:i+3]!=background for i in range(0,len(rgb),3) if i//3//320>=8 or i//3%320>=40)
    assert not bad,f'{bad} unexpected pixels outside five glyphs'
    foreground=sum(rgb[i:i+3]!=background for i in range(0,len(rgb),3))
    assert 60<foreground<200,foreground
    print(f'Decoded VGA passed: {w.value}x{h.value}, {foreground} glyph pixels.',flush=True)
    lib.sendPS2KbEventToFabgl(0x1c,1)
    lib.sendPS2KbEventToFabgl(0x1c,0)
    wait(lambda:bytes.fromhex('810461000001') in received)
    print('Firmware serial-console keyboard passed.',flush=True)
    result={**inputs,'passed':False,'engine':a.engine,'width':w.value,'height':h.value,'glyph_pixels':foreground,'frames':lib.flexe_vdp_frames(),'cycles':lib.flexe_vdp_cycles(),'wall_seconds':time.monotonic()-started,'uart_bytes':len(received)}
    if a.media:
        modes=[]
        for mode,width,height in [(0,640,480),(3,640,240),(8,320,240),
                                  (12,320,200),(16,800,600),(18,1024,768),
                                  (20,512,384),(24,640,512),(27,640,256)]:
            received.clear()
            send(bytes([22,mode,23,1,0,17,129,12,17,0])+b'FLEXE'+
                 bytes([23,0,0x84,100,0,100,0]))
            wait(lambda:bytes([0x84,4]) in received)
            point=received.index(bytes([0x84,4]))
            wait(lambda:len(received)>=point+6)
            background=bytes(received[point+2:point+5])
            frames_before=lib.flexe_vdp_frames()
            wait(lambda:lib.flexe_vdp_frames()>=frames_before+3)
            lib.copyVgaFramebuffer(ctypes.byref(w),ctypes.byref(h),pixels,ctypes.byref(rate))
            assert (w.value,h.value)==(width,height),f'mode {mode}: got {w.value}x{h.value}'
            rgb=bytes(pixels[:width*height*3])
            bad=sum(rgb[i:i+3]!=background for i in range(0,len(rgb),3)
                    if i//3//width>=8 or i//3%width>=40)
            assert not bad,f'mode {mode}: {bad} unexpected pixels outside glyphs'
            foreground=sum(rgb[i:i+3]!=background for i in range(0,len(rgb),3))
            assert foreground==115,f'mode {mode}: {foreground} glyph pixels'
            modes.append(dict(mode=mode,width=width,height=height,glyph_pixels=foreground,
                              background=list(background),rate=rate.value))
            print(f'Mode {mode}: {width}x{height} pixels passed.',flush=True)
        result['modes']=modes
        # Drain prior silence, run one second of guest time and sample its
        # finite 500 ms tone through the exact ABI used by SDL playback.
        audio=(ctypes.c_uint8*65536)()
        lib.getAudioSamples(audio,len(audio))
        send(bytes([23,0,0x85,0,0,127,0,4,0xf4,1]))
        first_cycle=lib.flexe_vdp_cycles()
        wait(lambda:lib.flexe_vdp_cycles()>=first_cycle+240000000)
        lib.getAudioSamples(audio,16384)
        samples=bytes(audio[:16384])
        rises=[i for i in range(1,len(samples)) if samples[i]>127 and samples[i-1]<127]
        frequency=(len(rises)-1)*16384/(rises[-1]-rises[0]) if len(rises)>1 else 0
        non_silent=sum(value!=127 for value in samples)
        assert bytes([0x85,2,0,1]) in received,'tone acknowledgement absent'
        assert 1020<frequency<1028,(frequency,len(rises))
        assert 7800<non_silent<8600,non_silent
        assert min(samples)<100 and max(samples)>154,(min(samples),max(samples))
        assert samples[-1]==127,'tone did not return to silence'
        result['audio']=dict(samples=len(samples),non_silent=non_silent,
                             frequency=frequency,minimum=min(samples),maximum=max(samples))
        result.update(frames=lib.flexe_vdp_frames(),cycles=lib.flexe_vdp_cycles(),
                      wall_seconds=time.monotonic()-started,uart_bytes=len(received))
        print(f'Fab audio ABI passed: {frequency:.3f} Hz.',flush=True)
    result['passed']=True
except BaseException as exc:
    result['passed']=False
    result['failure']=f'{type(exc).__name__}: {exc}'
    raise
finally:
    lib.vdp_shutdown();worker.join(timeout=10)
    if worker.is_alive():raise RuntimeError('backend did not stop')
    (a.artifacts/'check.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
