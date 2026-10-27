#!/usr/bin/env python3
"""Collect final qualification and pin the actual delivered source/toolchain."""
from pathlib import Path
import hashlib
import json
import re
import subprocess

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
BUILD=HERE/'build'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def read(name):return json.loads((HERE/name).read_text())
def report():
 host=read('parity-host.json');san=read('parity-san.json');cli=read('cli-evidence.json');examples=read('examples-evidence.json')
 rebuild=read('rebuild-evidence.json');review=read('review-evidence.json')
 assert host['passed'] and san['passed'] and cli['passed'] and examples['passed'] and rebuild['passed'] and review['passed']
 for c in rebuild['results']:assert sha(BUILD/c['file'])==c['sha256']
 for mode,digest in examples['compiler_binaries'].items():assert sha(BUILD/'port'/('ez80-'+mode)/'golemc.bin')==digest
 for evidence in (examples,review):
  assert sha(BUILD/'support/golemc.bin')==evidence['dispatcher_sha256']
  assert sha(BUILD/'support/golemrun.bin')==evidence['runner_sha256']
 pins=json.loads((BUILD/'port/source-pins.json').read_text())
 assert pins=={str(p.relative_to(ROOT/'src')):sha(p) for p in sorted((ROOT/'src').rglob('*')) if p.suffix in ('.hpp','.cpp')}
 native=[]
 for mode in ('raw','resident','portable','stock-matrix'):
  check=read('native-'+mode+'.json');assert check['matched']==check['cases'] and not check['resource_limited'],mode
  binary=BUILD/'port'/('ez80-'+mode)/'golemc.bin';assert sha(binary)==check['binary_sha256']
  receipt=json.loads((binary.parent/'build.json').read_text());assert receipt['source_pins']==pins
  maptext=(binary.parent/'golemc.map').read_text();limits={}
  for key in ('___heapbot','___heaptop','__stack'):
   m=re.search(r'^\s*(0x[0-9a-f]+)\s+'+key+r'\s*=',maptext,re.M);limits[key]=int(m[1],16)
  native.append({k:check[k] for k in ('profile','cases','matched','seconds','binary_sha256','frontend_exit','MOS_keyboard_continuation')})
  native[-1].update(binary_bytes=binary.stat().st_size,heap_bytes=limits['___heaptop']-limits['___heapbot'],stack_bytes=limits['__stack']-limits['___heaptop'])
 assert sum(c['cases'] for c in native)==host['cases']==san['cases']
 tools=Path('/Agon/agondev/release')
 toolchain={str(p.relative_to(tools)):sha(p) for p in [tools/'bin/ez80-none-elf-clang',tools/'bin/ez80-none-elf-ld',tools/'lib/libagon.a',tools/'config/linker.conf']}
 number=int(re.search(r': (\d+) checks passed', (BUILD/'numeric-checks.log').read_text())[1])
 result={'passed':True,'scope':'all currently implemented raw/resident scalar/table/drawing/portable/stock-matrix profiles and native C emission',
         'repository_head':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),'working_tree_source_snapshot':True,
         'emulator':'Fab Agon Emulator 1.2.5 ARM64, stock VDP module','MOS':'3.0.2',
         'host_cases':host['cases'],'accepted':host['accepted'],'rejected':host['cases']-host['accepted'],
         'host_sanitizers':'ASan/UBSan including rejected inputs','native_eZ80_cases':sum(c['cases'] for c in native),'native_profiles':native,
         'cli_cases':cli['cases'],'example_packages':examples['example_packages'],'stock_result_bytes':examples['observations'],
         'white_cube_anchors':examples['white_cube_anchors'],'numeric_host_checks':number,'standalone_rebuild_binaries':rebuild['binaries'],'installed_runner_review':review['passed'],
         'toolchain':toolchain,'source_pins':pins,
         'adapter_pins':{str(p.relative_to(HERE)):sha(p) for p in sorted((HERE/'port').iterdir()) if p.is_file()},
         'human':'pending','physical_hardware':'not run','firmware_flashing':False,'Oryx_contacted':False,
         'limitations':['Finite eZ80 RAM; larger sources may exhaust working memory.',
                        'Current profile restrictions are the same as the desktop compiler.',
                        'Native C emission needs a separate C-to-Xtensa build; no on-Agon Xtensa compiler.',
                        'Stock standalone runner invokes the first Sub and requires caller-provided borrowed assets.',
                        'Headless timings are wall-clock observations under shared-host load, not performance benchmarks.']}
 (HERE/'qualification.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ('toolchain','source_pins','adapter_pins','native_profiles')}))
 return result
if __name__=='__main__':report()
