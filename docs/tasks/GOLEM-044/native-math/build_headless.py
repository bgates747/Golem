#!/usr/bin/env python3
"""Build the previously qualified VDP-only adapter in this task's ignored cache.

No desktop frontend or maintained emulator checkout is modified. This reuses
the frozen GOLEM-021 source export/integration for actual ESP32 execution.
"""
from pathlib import Path
import hashlib
import json
import shutil
import subprocess
import sys
import time

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
OLD=ROOT/'docs/tasks/GOLEM-021/emulator'
BUILD=HERE/'build/headless-runtime'


def main():
    started=time.monotonic();BUILD.mkdir(parents=True,exist_ok=True)
    source=BUILD/'source';core=BUILD/'core'
    with (BUILD/'build.log').open('w') as log:
        for command in [
            [sys.executable,OLD/'prepare_core.py','--reference',ROOT.parent/'flexe-agon-emulator/vendor/flexe','--source',source],
            ['cmake','-S',source,'-B',core,'-DCMAKE_BUILD_TYPE=Release','-DFLEXE_LTO=OFF',
             '-DCMAKE_POSITION_INDEPENDENT_CODE=ON','-DCMAKE_C_FLAGS=-Wno-error=stringop-truncation'],
            ['cmake','--build',core,'--target','xtensa-emu-lib','-j2'],
            ['cc','-std=c17','-O2','-fPIC','-shared','-Wall','-Wextra','-Werror','-I'+str(source/'src'),
             OLD/'flexe_vdp.c',core/'libxtensa-emu-lib.a','-o',BUILD/'flexe-vdp.so',
             '-lssl','-lcrypto','-lz','-lm','-lpthread']]:
            subprocess.run(list(map(str,command)),check=True,stdout=log,stderr=subprocess.STDOUT)
    receipt={'source_export':json.loads((source/'.golem-flexe-source.json').read_text()),
             'adapter_source_sha256':hashlib.sha256((OLD/'flexe_vdp.c').read_bytes()).hexdigest(),
             'backend_sha256':hashlib.sha256((BUILD/'flexe-vdp.so').read_bytes()).hexdigest(),
             'host_machine':subprocess.check_output(['uname','-m'],text=True).strip(),
             'elapsed_seconds':round(time.monotonic()-started,3),
             'scope':'frozen VDP-only adapter; no desktop frontend build or source change'}
    evidence=HERE/'evidence';evidence.mkdir(exist_ok=True)
    (evidence/'headless-build.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print('Built task-local VDP-only emulator:',BUILD/'flexe-vdp.so')


if __name__=='__main__':main()
