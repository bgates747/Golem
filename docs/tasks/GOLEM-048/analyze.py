#!/usr/bin/env python3
"""Compare retained production pictures/wire/timing and separate profiling runs."""
from pathlib import Path
import hashlib
import json
import re
import statistics
import struct

HERE=Path(__file__).resolve().parent
BUILD=HERE/'build'
NAMES=['queue_calls','queue_cycles','line_calls','line_cycles','state_calls','state_cycles',
       'trace_calls','trace_cycles','wait_calls','wait_cycles','submit_calls','submit_cycles','entry_cycles','entry_id']


def picture(name):
    kind,dimensions,maximum,rgb=(BUILD/(name+'.ppm')).read_bytes().split(b'\n',3)
    assert (kind,dimensions,maximum)==(b'P6',b'512 384',b'255')
    assert len(rgb)==512*384*3
    return rgb[16*512*3:328*512*3]


def main():
    names=('benchmark-vdu-jit','benchmark-lines-jit','benchmark-lines-interp')
    reports={n:json.loads((HERE/(n+'-evidence.json')).read_text()) for n in names}
    images=[picture(n) for n in names]
    assert images[0]==images[1]==images[2] and any(images[0])
    result={'passed':True,'pixel_comparison':{'images':list(names),'pixels_per_image':len(images[0])//3,
            'matching_rgb_sha256':hashlib.sha256(images[0]).hexdigest(),'crop':'x0:511,y16:327 inclusive'},
            'timings':{},'frame_wire':{},'physical_acceptance':False}
    for engine in ('jit','interp'):
        samples=[row for r in reports.values() if r['engine']==engine for row in r['samples']]
        result['timings'][engine]={}
        for arm,label in ((0,'vdu'),(1,'lines')):
            rows=[r for r in samples if r[2]==arm]
            result['timings'][engine][label]={'samples':len(rows)}
            for at,key in ((3,'transform_status_ms'),(4,'draw_fence_ms'),(5,'full_frame_ms')):
                values=[r[at]*10 for r in rows]
                result['timings'][engine][label][key]={'minimum':min(values),'median':statistics.median(values),'maximum':max(values)}
        result['timings'][engine]['full_frame_ratio']=result['timings'][engine]['vdu']['full_frame_ms']['median']/result['timings'][engine]['lines']['full_frame_ms']['median']
    update=bytes((23,0,160,1,0,5,0xc2))+struct.pack('<HH',112,32)
    for name,report in reports.items():
        wire=(BUILD/(name+'-mos-to-vdp.bin')).read_bytes()
        starts=[m.start() for m in re.finditer(re.escape(update),wire)]
        assert len(starts)==25,(name,len(starts))
        sizes=[b-a for a,b in zip(starts,starts[1:])]
        arms={str(arm):sorted({size for row,size in zip(report['samples'],sizes) if row[2]==arm}) for arm in (0,1)}
        assert arms['0']==arms['1']==[221],arms
        result['frame_wire'][name]={'bytes_by_arm':arms,'sha256':hashlib.sha256(wire).hexdigest()}
    result['profiling']={}
    for label in ('instrumented-off','instrumented-on'):
        path=BUILD/(label+'-benchmark-lines-jit-esp32-uart0.log')
        rows=[dict(zip(NAMES,map(int,raw.split()))) for raw in re.findall(rb'VP ((?:[0-9]+ ){13}[0-9]+)',path.read_bytes())]
        assert len(rows)==25,(label,len(rows))
        # Last row is the common final pose, outside the24 paired samples.
        summary={}
        for entry,arm in ((1,'vdu'),(2,'lines')):
            selected=[row for row in rows[:-1] if row['entry_id']==entry]
            assert len(selected)==12
            assert all(row['line_calls']==4903 and row['submit_calls']==154 for row in selected)
            summary[arm]={n:statistics.median(row[n] for row in selected) for n in NAMES}
            summary[arm]['generation_lease_and_admission_cycles']=statistics.median(row['entry_cycles']-row['submit_cycles'] for row in selected)
        result['profiling'][label]={'samples':rows,'medians':summary,'diagnostic_only':True,
            'scope':'HUD drained before counter reset; counters read after C3/GP; same existing rasterizer',
            'units':'local-core CCOUNT elapsed cycles, nominal240MHz emulator; intervals overlap and are not additive'}
    result['clock']='Production MOS centisecond units, effective20ms quantization; no physical speed prediction'
    result['unmeasured']=['physical hardware','isolated scheduler/context-switch CPU cost','new pixel rasterizer comparison']
    (HERE/'comparison.json').write_text(json.dumps(result,indent=2)+'\n')
    print('Production pictures, matched frame wire/timing and separate phase counters pass.')
    print('JIT ratio:',result['timings']['jit']['full_frame_ratio'],'interpreter:',result['timings']['interp']['full_frame_ratio'])


if __name__=='__main__':main()
