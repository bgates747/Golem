#!/usr/bin/env python3
"""Compare completed old/new transform observations from the wire transcript.

The later VDP-only bulk upload was interrupted; this does not qualify it.
The full MOS fixture is the authoritative consumer qualification instead.
"""
from pathlib import Path
import hashlib,json,struct
HERE=Path(__file__).resolve().parent
trace=HERE.parent/'wireframe/build/jit'
tx=(trace/'mos-to-vdp.bin').read_bytes();rx=(trace/'vdp-to-mos.bin').read_bytes()
replies={};at=0
while at+2<=len(rx):
 if rx[at]<128:at+=1;continue
 size=rx[at+1];body=rx[at+2:at+2+size];at+=2+size
 if len(body)==10 and body.startswith(b'VDPL'):replies[struct.unpack_from('<H',body,6)[0]]=(body[5],body[8:10])
groups=[];current=None;at=0
while at<len(tx):
 assert tx[at:at+2]==bytes((23,0)),at
 selector=tx[at+2]
 if selector==128:at+=4
 elif selector==216:
  size=struct.unpack_from('<H',tx,at+3)[0];body=tx[at+5:at+5+size];at+=5+size
  operation=body[1];token=struct.unpack_from('<H',body,2)[0]
  if operation==2:
   h,entry,args=struct.unpack_from('<HHH',body,6);assert entry==0 and replies[token]==(3,b'\0\0')
   current={'handle':h,'rows':[]};groups.append(current)
  elif operation in (1,3):current=None
  elif operation==5:
   key,offset=struct.unpack_from('<HI',body,6);kind,data=replies[token];assert kind==4
   if current is not None and key in (1,3):current['rows'].append((key,offset,data))
 elif selector==160:
  key,operation=struct.unpack_from('<HB',tx,at+3)
  if key==59002 and operation==2:break
  if operation in (2,14):at+=6
  elif operation==0:at+=8+struct.unpack_from('<H',tx,at+6)[0]
  elif operation==5:
   assert tx[at+6]==194;at+=11+struct.unpack_from('<H',tx,at+9)[0]
  else:raise AssertionError((at,key,operation))
 else:raise AssertionError((at,selector))
assert len(groups)==12,len(groups)
poses=[]
for i in range(6):
 a,b=groups[i],groups[i+6];assert a['handle']==1 and b['handle']==2
 assert a['rows']==b['rows'] and len(a['rows'])==178
 raw=b''.join(row[2] for row in a['rows']);poses.append({'pose':i,'matching_observed_bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest()})
report={'passed':True,'poses':poses,'bytes_compared':sum(p['matching_observed_bytes'] for p in poses),'observation':'all224 root bytes plus22 XYZ16 vertices per pose; exact old precompiled vs composed transform','tx_sha256':hashlib.sha256(tx).hexdigest(),'rx_sha256':hashlib.sha256(rx).hexdigest(),'later_bulk_upload':'interrupted; no VDP-only full-fixture pass claimed','full_consumer_authority':'wireframe/profile.py full MOS qualification'}
(HERE/'transform-trace-evidence.json').write_text(json.dumps(report,indent=2)+'\n')
print('Completed transform transcript matches:',report['bytes_compared'],'bytes across six poses.')
