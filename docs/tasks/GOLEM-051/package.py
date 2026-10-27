#!/usr/bin/env python3
"""Assemble a standalone SD distribution and reproducible source ZIP."""
from pathlib import Path
import hashlib
import importlib.util
import json
import re
import shutil
import zipfile

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
BUILD=HERE/'build'

def put(source,destination):
    destination.parent.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(source,destination)

def package():
    report=json.loads((HERE/'qualification.json').read_text())
    if not report['passed']:raise RuntimeError('Qualification has not passed')
    tree=BUILD/'distribution'
    if tree.exists():shutil.rmtree(tree)
    tree.mkdir()
    sd=tree/'sdcard';app=sd/'golem'
    for d in ('cache','output','compiler','examples','reference'):(app/d).mkdir(parents=True,exist_ok=True)
    for mode in ('raw','resident','portable','stock-matrix'):
        put(BUILD/'port'/('ez80-'+mode)/'golemc.bin',app/'compiler'/(mode+'.bin'))
    put(BUILD/'support/golemc.bin',sd/'mos/golemc.bin')
    put(BUILD/'support/golemrun.bin',sd/'bin/golemrun.bin')
    for p in (HERE/'examples').iterdir():put(p,app/'examples'/p.name)
    for name in ('HOWTO.md',):put(HERE/name,app/name)
    put(ROOT/'docs/language-reference.md',app/'LANGUAGE.md')
    put(BUILD/'runs/review/initial.png',app/'reference-image.png')
    for name in ('portable-compiler','resident-scalar','resident-table','resident-drawing','stock-matrix','backend-portability','agnb-packages'):
        put(ROOT/'docs/design'/(name+'.md'),app/'reference'/(name+'.md'))
    # Detailed contracts retain useful repository cross-references. Keep
    # packaged links clickable; label references to files outside this ZIP.
    for document in (app/'reference').glob('*.md'):
        def link(match):
            label,target=match.groups()
            if '://' in target or target.startswith('#'):return match[0]
            relative=target.split('#',1)[0]
            if (document.parent/relative).exists():return match[0]
            return label+' (repository reference `'+target+'`)'
        text=re.sub(r'\[([^\]]+)\]\(([^)]+)\)',link,document.read_text())
        # Qualification bookkeeping stays in the repository, not consumer docs.
        text=re.sub(r'Physical(?: ESP32)? hardware\s+and other firmware versions remain [^.]+\.', '', text)
        text=re.sub(r'No physical or performance acceptance\s+is inferred from these functional emulator checks\.', '', text)
        text=text.replace('; physical hardware and performance are not claimed.', '.')
        text=text.replace('IEEE exception/subnormal support and physical qualification are not claimed.',
                          'Full IEEE exception/subnormal support is not provided.')
        text=text.replace('No production readback, physical result or frame-speed claim.',
                          'No production readback or frame-speed claim.')
        text=text.replace('they do not imply hardware fault tests.', 'they cover software release behavior.')
        document.write_text(text)
    put(HERE/'HOWTO.md',tree/'HOWTO.md')
    put(ROOT/'docs/language-reference.md',tree/'LANGUAGE.md')
    (tree/'README.md').write_text("""# Golem on Agon

Compile Golem programs directly on your Agon and run their buffered commands
on stock VDP. This package includes the compiler, runner, examples, documentation
and rebuildable source.

1. **[How to install and use Golem](HOWTO.md)** — start here; includes Hello,
   world, drawing examples and compiler commands.
2. **[Language reference](LANGUAGE.md)** — implemented syntax, types and the
   limits of each compiler profile.
3. **[Rebuilding from source](REBUILD.md)** — AgonDev requirements and commands.
4. **[Third-party notices](THIRD-PARTY.md)** and [repository owner](OWNER.txt).

Copy the **contents of `sdcard/`** into your SD card's root, merging directories.
The same how-to and language reference are also installed under `/golem/`.
Example programs are in `sdcard/golem/examples/`; compiler source is in `source/`.
""")
    put(HERE/'REBUILD.md',tree/'REBUILD.md')
    put(HERE/'THIRD-PARTY.md',tree/'THIRD-PARTY.md')
    source=tree/'source'
    for p in (ROOT/'src').rglob('*'):
        if p.is_file() and p.suffix in ('.hpp','.cpp'):put(p,source/'src'/p.relative_to(ROOT/'src'))
    task=source/'docs/tasks/GOLEM-051'
    for p in (HERE/'port').iterdir():
        if p.is_file() and p.suffix in ('.py','.cpp','.hpp','.c'):put(p,task/'port'/p.name)
    spec=importlib.util.spec_from_file_location('portbuild',HERE/'port/build.py')
    import sys
    sys.path.insert(0,str(HERE/'port'))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    vendor=HERE/'vendor/softfloat'
    selected=set(module.sources())|set((vendor/'source/include').glob('*.h'))|{vendor/'source/8086-SSE/specialize.h'}
    selected|={vendor/p for p in ('COPYING.txt','platform.h','README.txt','PROVENANCE.json')}
    for p in sorted(selected):put(p,task/'vendor/softfloat'/p.relative_to(vendor))
    (tree/'OWNER.txt').write_text('Golem — repository owner: Brandon R. Gates\nhttps://github.com/bgates747/Golem\n\nThis distribution preserves the repository source and third-party notices.\nNo new project-wide license grant is made by this task. See THIRD-PARTY.md.\n')
    sums={str(p.relative_to(tree)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(tree.rglob('*')) if p.is_file()}
    (tree/'SHA256SUMS').write_text(''.join(d+'  '+p+'\n' for p,d in sums.items()))
    archive=ROOT/'dist/golem-on-agon-GOLEM-051.zip';archive.parent.mkdir(exist_ok=True)
    with zipfile.ZipFile(archive,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for p in sorted(tree.rglob('*')):
            name=str(p.relative_to(tree))+('/' if p.is_dir() else '')
            item=zipfile.ZipInfo(name,date_time=(2026,10,26,0,0,0));item.compress_type=zipfile.ZIP_DEFLATED
            item.external_attr=((0o40755 if p.is_dir() else 0o100644)<<16)
            z.writestr(item,b'' if p.is_dir() else p.read_bytes())
    result={'archive':str(archive.relative_to(ROOT)),'bytes':archive.stat().st_size,'sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),
            'files':len(sums)+1,'human':'pending','physical':'not run','source':'current maintained C++ with task-local AgonDev adapter'}
    (HERE/'package.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
    return tree

if __name__=='__main__':package()
