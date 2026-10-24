#!/usr/bin/env python3
"""Verify an old, unchanged firmware refuses the new package cleanly."""
from pathlib import Path
import json,struct
from qualify import old,BUILD,HERE,SDK,upload,word,sha
firmware=SDK/'docs/tasks/VDPL-004/build/baseline/firmware.bin'
v=old.VDP('jit',BUILD/'old-host',90,firmware=firmware)
try:
 assert v.transaction(4,word(1),4)==7
 package=SDK/'build/modules/line-batch.vdpl';v.send(upload(60023,package.read_bytes()))
 v.transaction(1,struct.pack('<HIH',60023,package.stat().st_size,0),5,10)
 assert v.transaction(4,word(2),4)==0
 p=SDK/'docs/tasks/VDPL-004/build/baseline/numeric-1.vdpl';v.send(upload(60023,p.read_bytes()))
 h=v.transaction(1,struct.pack('<HIH',60023,p.stat().st_size,0),2)
 v.send(upload(60020,struct.pack('<f',2.0))+upload(60021,b'\0'*8)+upload(60022,word(60020)+word(60021)))
 v.transaction(2,struct.pack('<HHH',h,0,60022),3,0)
 assert v.read(60021,8)==struct.pack('<fI',3.25,1)
 v.transaction(3,word(h),3,0)
finally:v.close()
report={'passed':True,'firmware_sha256':sha(firmware),'new_package_rejected':10,'slot_empty_after_rejection':True,'legacy_execution_after_rejection':True,'scope':'unchanged pre-extension firmware, actual ESP32 execution; no hardware claim'}
(HERE/'old-host-evidence.json').write_text(json.dumps(report,indent=2)+'\n')
print('Old firmware rejects ABI1.2, retains empty slot and executes legacy image afterward.')
