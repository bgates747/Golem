"""Headless actual-target checks with a clean shutdown and retained evidence."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import signal
import subprocess
import time
from verify import verify,screen

HERE=Path(__file__).resolve().parent

def qualify(variant,engine,timeout):
    stock=variant in ('stock','meter-stock')
    profile=HERE/('.emulator' if variant=='review' else '.emulator-'+variant)
    app=profile/'sdcard/golem036d';artifacts=profile/'last-run';artifacts.mkdir(exist_ok=True)
    for name in ('results.dat','failure.dat'):(app/name).unlink(missing_ok=True)
    for name in ('frontend.log','esp32-uart0.log','stats.json','final.ppm'):(artifacts/name).unlink(missing_ok=True)
    subprocess.run(['sha256sum','--check','--status','.runtime.sha256'],cwd=profile,check=True)
    env={k:v for k,v in os.environ.items() if k not in ('LD_PRELOAD','BASH_ENV')}
    env.update(SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',AGON_FLEXE_ENGINE=engine)
    start=time.monotonic();mapped=set();process=None
    try:
        with (artifacts/'frontend.log').open('w') as log:
            process=subprocess.Popen([('./fab-agon-emulator' if stock else './flexe-agon-emulator'),'--renderer','sw'],cwd=profile,env=env,
                                     stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
            while True:
                if (app/'failure.dat').exists():raise RuntimeError('Guest reported failure; inspect '+str(artifacts))
                if process.poll() is not None:raise RuntimeError('Frontend exited: '+str(process.returncode))
                if time.monotonic()-start>timeout:raise TimeoutError('No complete run in '+str(timeout)+'s')
                for child in Path('/proc').glob('[0-9]*'):
                    try:
                        if os.getpgid(int(child.name))==process.pid:
                            mapped.update(line.split(maxsplit=5)[5] for line in (child/'maps').read_text().splitlines()
                                          if '.so' in line and ('vdp' in line or 'VDP' in line))
                    except (OSError,PermissionError):pass
                if (app/'results.dat').exists():
                    size=json.loads((app/'expected.json').read_text())['result_bytes']
                    if (app/'results.dat').stat().st_size==size:
                        if stock:break
                        uart=artifacts/'esp32-uart0.log'
                        if uart.exists() and b'/golem036d *' in uart.read_bytes().partition(b'GOLEM-036D: results saved.')[2]:break
                time.sleep(.1)
            receipt=verify(app)
            if stock:time.sleep(.3)
    finally:
        if process and process.poll() is None:
            for child in Path('/proc').glob('[0-9]*'):
                try:
                    pid=int(child.name)
                    if os.getpgid(pid)==process.pid and (child/'exe').resolve()==(profile/('fab-agon-emulator.bin' if stock else 'flexe-agon-emulator.bin')).resolve():
                        os.kill(pid,signal.SIGTERM)
                except OSError:pass
            try:process.wait(timeout=10)
            except subprocess.TimeoutExpired:os.killpg(process.pid,signal.SIGKILL);process.wait()
    assert mapped,'No VDP module observed'
    assert process.returncode==0,process.returncode
    if not stock:
        assert any(str(profile/'flexe-vdp.so') in path for path in mapped),mapped
        stats=json.loads((artifacts/'stats.json').read_text())
        assert not any(stats.get(k) for k in ('failed','unhandled','unregistered','unmapped')),stats
        receipt['backend_stats']=stats
        if variant=='review':receipt['playfield_pixels']=screen(artifacts/'final.ppm',HERE/'review-screen.png')
    else:
        assert any(Path(path).resolve()==(profile/'firmware/vdp_platform.so').resolve() for path in mapped),mapped
    receipt.update(engine=engine if not stock else 'stock-native',elapsed_seconds=round(time.monotonic()-start,3),
                   mapped_vdp_libraries=sorted(mapped),returned_to_mos_prompt=not stock,
                   graphical_instance_launched=False,runtime_identities=(profile/'.runtime.sha256').read_text().splitlines())
    evidence=HERE/'evidence';evidence.mkdir(exist_ok=True)
    stem=variant+'-'+engine
    (evidence/(stem+'.json')).write_text(json.dumps(receipt,indent=2)+'\n')
    for source,name in [(app/'results.dat',stem+'.dat'),(app/'expected.json',stem+'-expected.json')]:
        (evidence/name).write_bytes(source.read_bytes())
    if (artifacts/'final.ppm').exists():(evidence/(stem+'.ppm')).write_bytes((artifacts/'final.ppm').read_bytes())
    print(json.dumps({k:receipt[k] for k in ('variant','calls','observations','result_bytes','elapsed_seconds')},indent=2))
    return receipt

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('variant',choices=('stock','meter-stock','hybrid','relocated','review'))
    parser.add_argument('--engine',choices=('jit','interp'),default='jit')
    parser.add_argument('--timeout',type=float,default=600)
    args=parser.parse_args();qualify(args.variant,args.engine,args.timeout)
