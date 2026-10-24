#!/usr/bin/env python3
"""Reuse the qualified profile mechanics for a separate C++ consumer profile."""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import shutil
import subprocess
import sys
HERE=Path(__file__).resolve().parent
TASK=HERE.parent
ROOT=TASK.parents[2]
spec=importlib.util.spec_from_file_location('golem_agnb_profile',TASK/'qualify.py')
base=importlib.util.module_from_spec(spec);spec.loader.exec_module(base)
base.HERE=HERE;base.BUILD=HERE/'build';base.PROFILE=ROOT/'.emulator/golem-046-cpp'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()

def prepare(runtime,native):
    meta=base.prepare(runtime,native)
    app=base.PROFILE/'sdcard/golem046'
    for name in ('bad.agnb','truncated.agnb','late.agnb','probe.agnb'):
        shutil.copyfile(HERE/'build'/name,app/name)
        meta['pins']['sdcard/golem046/'+name]=sha(app/name)
    assert not (app/'absent.agnb').exists()
    # The caller's accepted assembly profile and its saved human results remain untouched.
    launcher=base.PROFILE/('flexe-agon-emulator' if native else 'fab-agon-emulator')
    script=launcher.read_text().replace('rm -f -- sdcard/golem046/results.dat sdcard/golem046/failure.dat',
                                      'rm -f -- sdcard/golem046/results.dat sdcard/golem046/failure.dat sdcard/golem046/cpp.log')
    launcher.write_text(script);meta['pins'][launcher.name]=sha(launcher)
    (base.PROFILE/'.runtime.sha256').write_text(''.join(value+'  '+name+'\n' for name,value in meta['pins'].items()))
    (HERE/'build/profile.json').write_text(json.dumps(meta,indent=2)+'\n')
    return meta

def execute(meta,engine):
    base.execute(meta,engine)
    log=(base.PROFILE/'sdcard/golem046/cpp.log').read_text()
    assert log.split() and log.split()[-1]=='PASS',log
    path=HERE/(base.os.uname().nodename+'-'+meta['backend']+'-'+engine+'-evidence.json')
    report=json.loads(path.read_text());report.update(application='AgonDev C++',generic_reader='AGNB-002',
        rejection_and_recovery_checks=True,log=log,embedded_installation=False)
    path.write_text(json.dumps(report,indent=2)+'\n')
    print('C++ loader log and malformed-file/recovery paths pass.',flush=True)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--runtime',type=Path,required=True)
    p.add_argument('--native',action='store_true');p.add_argument('--no-build',action='store_true');p.add_argument('--stage-only',action='store_true')
    p.add_argument('--engine',choices=('jit','interp'),default='jit');a=p.parse_args()
    if not a.no_build:
        spec=importlib.util.spec_from_file_location('cpp_build',HERE/'build_app.py')
        builder=importlib.util.module_from_spec(spec);spec.loader.exec_module(builder);builder.build(a.native)
    m=prepare(a.runtime.resolve(),a.native)
    if not a.stage_only:execute(m,a.engine)
