#!/usr/bin/env python3
"""Package a compiler --emit-load-plan output using generic AGNB host tools.

Compile with C++ golemc first. Native .vdpl construction/loading is a separate
SDK step; --module optionally adds an already built image at the assigned ID.
"""
from pathlib import Path
import argparse
import importlib.util
import json
import sys

ROOT=Path(__file__).resolve().parents[1]


def package(directory,api=None,module=None,backend=None):
    directory=Path(directory).resolve()
    api=Path(api or ROOT.parent/'agon-utils/examples/agnb/api').resolve()
    sys.path.insert(0,str(api))
    spec=importlib.util.spec_from_file_location('golem_generic_agnb',api/'buffers.py')
    generic=importlib.util.module_from_spec(spec);sys.modules[spec.name]=generic;spec.loader.exec_module(generic)
    if generic.VERSION!=b'\0\3':raise ValueError('Golem load plans require AGNB plain-buffer version0.3')
    plan=json.loads((directory/'load-plan.json').read_text())
    if plan.get('schema')!=1 or plan.get('agnb_version')!=[0,3]:raise ValueError('unsupported load plan')
    choices=plan['components'];names=[c['name'] for c in choices]
    if not names or len(set(names))!=len(names) or not set(names)<={'stock','buffered','native'}:raise ValueError('invalid component names')
    if backend and backend not in names:raise ValueError('backend unavailable in package')
    if module and ('native' not in names or backend not in (None,'native')):raise ValueError('--module requires a native component')
    images=[];receipts=[]
    for c in choices:
        if backend and c['name']!=backend:continue
        owned=c['owned']
        if len(set(owned))!=len(owned) or any(type(id) is not int or not 1<=id<=65534 for id in owned):raise ValueError('invalid component ownership')
        records=[]
        for row in c['records']:
            path=(directory/row['path']).resolve()
            if not path.is_relative_to(directory):raise ValueError('load payload escapes package directory')
            data=path.read_bytes()
            if len(data)!=row['bytes'] or row['buffer_id'] not in owned:raise ValueError('load record shape/ownership mismatch')
            records.append(generic.BufferRecord(row['buffer_id'],data))
        if c['name']=='native' and module:
            if c['module_buffer'] not in owned:raise ValueError('module staging ID is not owned')
            records.append(generic.BufferRecord(c['module_buffer'],Path(module).read_bytes()))
        raw=generic.build_container(records)
        filename='program-'+c['name']+'.agnb'
        images.append((filename,raw))
        receipts.append({'backend':c['name'],'filename':filename,'container_bytes':len(raw),
                         'payload_bytes':sum(len(r.data) for r in records),'record_count':len(records),
                         'setup_entry':c['setup_entry'],'cleanup_entry':c['cleanup_entry'],
                         'module_buffer':c['module_buffer'],'module_included':bool(module and c['name']=='native')})
    # Finish validation for all selected components before writing any container.
    for filename,raw in images:(directory/filename).write_bytes(raw)
    receipt={'schema':1,'generic_api':str(api),'profile':plan['profile'],'packages':receipts,
             'load_does_not_activate':True,'native_module_requires_fresh_runtime_handle':True}
    (directory/'agnb-package.json').write_text(json.dumps(receipt,indent=2)+'\n')
    return receipt


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('directory',type=Path)
    p.add_argument('--agnb-api',type=Path);p.add_argument('--module',type=Path)
    p.add_argument('--backend',choices=('stock','buffered','native'))
    a=p.parse_args();print(json.dumps(package(a.directory,a.agnb_api,a.module,a.backend),indent=2))


if __name__=='__main__':main()
