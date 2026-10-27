#!/usr/bin/env python3
"""Execute captured inputs through each real eZ80 compiler worker under MOS."""
from pathlib import Path
import argparse
import json
import shutil
import time
from qualify import Session,stage,HERE,ROOT,BUILD,sha
from check_parity import cases,options

def prepare():
    app=stage();sd=app.parent;mos=sd/'mos'
    if mos.is_symlink():
        original=mos.resolve();mos.unlink();mos.mkdir()
        for p in original.iterdir():(mos/p.name).symlink_to(p)
    if (mos/'golemc.bin').is_symlink():(mos/'golemc.bin').unlink()
    shutil.copyfile(BUILD/'support/golemc.bin',mos/'golemc.bin')
    reference=json.loads((HERE/'parity-host.json').read_text())['results']
    prepared=[]
    for i,case in enumerate(cases()):
        label=f'c{i:03d}';directory=app/'cases'/label;directory.mkdir(exist_ok=True)
        shutil.copyfile(case['source'],directory/'source.golem')
        data=case['source'].parent/'data.bin'
        if data.exists():shutil.copyfile(data,directory/'data.bin')
        native_options=options(case,True,Path('cases')/label)
        output='out/'+label+('.bin' if case['mode']=='raw' else '')
        args=[*native_options,'cases/'+label+'/source.golem',output]
        (directory/'compile.args').write_text(' '.join('"'+arg+'"' if ' ' in arg else arg for arg in args)+'\n')
        prepared.append({'label':label,'key':case['key'],'mode':case['mode'],'output':output,
                         'response':'cases/'+label+'/compile.args','reference':reference[i]['reference']})
    for mode in ('raw','resident','portable','stock-matrix'):
        worker=BUILD/'port'/('ez80-'+mode)/'golemc.bin'
        shutil.copyfile(worker,app/'compiler'/(mode+'.bin'))
        (app/(mode+'.list')).write_text(''.join(p['response']+'\n' for p in prepared if p['mode']==mode))
    return app,prepared

def run(mode='all'):
    app,prepared=prepare();reports=[]
    modes=('raw','resident','portable','stock-matrix') if mode=='all' else (mode,)
    for current in modes:
        selected=[p for p in prepared if p['mode']==current]
        for name in ('batch.log','batch-output.log','batch-errors.log','proof.dat'):(app/name).unlink(missing_ok=True)
        session=Session('SET KEYBOARD 1\nVDU 22 0\ncd /golem\ngolemc --batch '+current+'.list\n',current+'-corpus')
        observed=[];started=time.monotonic();last_change=started
        try:
            while len(observed)<len(selected):
                log=app/'batch.log'
                lines=[line for line in log.read_text().splitlines(keepends=True) if line.endswith('\n')] if log.exists() else []
                if len(lines)>len(observed):observed=lines;last_change=time.monotonic()
                if session.process.poll() is not None:raise RuntimeError('Emulator stopped before batch completion')
                if time.monotonic()-last_change>600:
                    session.capture('stalled');raise TimeoutError(f'{current} case {len(observed)} stalled')
                time.sleep(.05)
            time.sleep(.5)
            session.type('save proof.dat 40000 3\n');session.wait(app/'proof.dat',20)
            picture=session.capture()
        finally:exitcode=session.close()
        for name in ('batch.log','batch-output.log','batch-errors.log','proof.dat'):
            if (app/name).exists():shutil.copyfile(app/name,session.art/name)
        errors=(app/'batch-errors.log').read_text() if (app/'batch-errors.log').exists() else ''
        results=[]
        for position,(case,line) in enumerate(zip(selected,observed)):
            status=int(line.split()[1]);ref=case['reference'];out=app/case['output'];files={}
            if out.is_dir():files={str(p.relative_to(out)):sha(p) for p in out.rglob('*') if p.is_file()}
            elif out.is_file():files={'payload':sha(out)}
            err=errors.partition(f'[case {position} ')[2].partition(f'[case {position+1} ')[0]
            limited=status!=ref['status'] and any(word in err for word in ('memory','container exceeds','string exceeds'))
            same=status==ref['status'] and (ref['status']!=0 or files==ref['files'])
            result={**case,'native_status':status,'same':same,'resource_limited':limited,'native_files':files,'diagnostics':err[:2000]}
            results.append(result)
        report={'profile':current,'cases':len(results),'matched':sum(p['same'] for p in results),
                'resource_limited':sum(p['resource_limited'] for p in results),'seconds':time.monotonic()-started,
                'frontend_exit':exitcode,'MOS_keyboard_continuation':True,'picture':str(picture),'human':'pending',
                'binary_sha256':sha(app/'compiler'/(current+'.bin')),'results':results}
        (HERE/('native-'+current+'.json')).write_text(json.dumps(report,indent=2)+'\n')
        print(json.dumps({k:v for k,v in report.items() if k!='results'}),flush=True);reports.append(report)
    return reports

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--mode',default='all',choices=('all','raw','resident','portable','stock-matrix'));a=p.parse_args();run(a.mode)
