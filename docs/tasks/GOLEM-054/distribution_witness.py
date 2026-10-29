"""Prepare the actual distributed float-print example for stock Fab/MOS proof."""
from pathlib import Path
import json
import shutil
import struct
import subprocess
import tarfile
import qualify as f

HERE,ROOT,BUILD=f.HERE,f.ROOT,f.BUILD

def prepare():
    source=ROOT/'docs/tasks/GOLEM-051/examples/float-print.golem'
    reference=BUILD/'distribution-reference'
    f.command([ROOT/'build/golemc','--portable',source,reference])
    meta=json.loads((reference/'backends.json').read_text())
    iface=json.loads((reference/'interface.json').read_text())
    state=(reference/'state.bin').read_bytes()
    offsets=[12,13,14,15]+[field['offset']+i for field in iface['globals'] for i in range(field['bytes'])]
    observed=bytes(state[o] for o in offsets)
    text=b'reading=1.5'
    dest=BUILD/'distribution-input';dest.mkdir(exist_ok=True)
    (dest/'cases.dat').write_bytes(f.word(0)+bytes((1,len(text)))+state+observed+text)
    shutil.copyfile(source,dest/'float-print.golem')
    header=f'#define STATE_BYTES {len(state)}\n#define STATE_ID {meta["shared_state"]}\n#define CASE_COUNT 1\n#define NUMERIC_CASES 1\n#define OFFSET_COUNT {len(offsets)}\n'
    for name,values in [('offsets',offsets),('entries',meta['buffered_entries'])]:
        header+='static const unsigned '+name+'[]={'+','.join(map(str,values))+'};\n'
    (BUILD/'fixture.h').write_text(header)
    tools=Path('/Agon/agondev/release')
    f.command([tools/'bin/ez80-none-elf-clang','-target','ez80-none-elf','-mllvm','-z80-gas-style','-mllvm','-z80-print-zero-offset','-nostdinc','-isystem',tools/'include','-I'+str(BUILD),'-Oz','-DAGONDEV','-Wa,-march=ez80+full','-c',HERE/'guest.c','-o',BUILD/'guest.o'])
    f.command([tools/'bin/ez80-none-elf-ld','-defsym=RAM_START=0x40000','-defsym=RAM_SIZE=0x70000','-defsym=_has_exit_handler=0','-T',tools/'config/linker.conf','--oformat','binary','-o',dest/'review.bin',BUILD/'guest.o','-L'+str(tools/'lib'),'-lagon'])
    bundle=BUILD/'distribution-witness.tar'
    with tarfile.open(bundle,'w') as a:
        for p in dest.iterdir():a.add(p,arcname='.emulator/golem-054/sdcard/golem/witness/'+p.name)
        for p in reference.iterdir():a.add(p,arcname='docs/tasks/GOLEM-054/build/distribution-reference/'+p.name)
        a.add(ROOT/'docs/tasks/GOLEM-051/build/support/golemrun.bin',arcname='.emulator/golem-054/sdcard/bin/golemrun.bin')
    return bundle

if __name__=='__main__':print(prepare())
