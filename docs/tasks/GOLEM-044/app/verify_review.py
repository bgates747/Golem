#!/usr/bin/env python3
"""Read saved application results after the Author closes the emulator."""
from pathlib import Path
import argparse
import json
import re
import subprocess

def verify(profile):
    subprocess.run(['sha256sum','--check','--status','.runtime.sha256'],cwd=profile,check=True)
    log=(profile/'sdcard/flower/flower.log').read_text()
    assert re.search(r'DONE \d+ segments=\d+',log),'No completed Flower drawing was saved'
    result=re.search(r'EXIT saved=(\d+) restored=(\d+) cleanup=(\d+) overflow=(\d+) curves=(\d+)',log)
    assert result,'No controlled application exit was saved'
    saved,restored,cleanup,overflow,curves=map(int,result.groups())
    assert saved==restored and cleanup==1 and overflow==0,(saved,restored,cleanup,overflow)
    stats=json.loads((profile/'last-run/stats.json').read_text())
    assert not any(stats.get(k) for k in ('failed','unhandled','unregistered','unmapped')),stats
    report={'passed':True,'completed_curves':curves,'saved_mode':saved,'restored_mode':restored,
            'cleanup':True,'keyboard_ring_overflow':False,'backend_stats':stats,
            'human_visual_and_keyboard_confirmation':'provided separately by the Author'}
    print(f'Saved review passes: {curves} completed curve(s), original mode restored, resources cleaned.')
    return report

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('profile',type=Path);verify(p.parse_args().profile)
