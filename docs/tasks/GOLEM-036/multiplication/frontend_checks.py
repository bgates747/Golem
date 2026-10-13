"""Check explicit multiplication typing and remaining numeric exclusions."""
import json,subprocess
from build_delivery import HERE,BUILD,ROOT

def main():
 d=BUILD/'syntax';d.mkdir(exist_ok=True);rejections=[]
 for text in ['uint32 a=0 Sub f a=4294967295*2 EndSub', 'sint32 a=0 Sub f a=-2147483648*-1 EndSub','uint32 a=1 sint32 b=2 Sub f a=a*b EndSub',
              'uint32 a=1 float32 b=2.0 Sub f a=a*uint32(b) EndSub',
              'uint32 a=1 uint32 b=2 Sub f (a,b)=divmod(a,b) EndSub',
              'uint16 i=0 ReadOnly Array uint32 a(0:1)=Fill(1) Sub f a(i)=a(i)*2 EndSub']:
  p=d/'source.golem';p.write_text(text);r=subprocess.run([str(BUILD/'compiler'),str(p),str(d/'package')],capture_output=True,text=True);assert r.returncode;rejections.append(r.stderr.strip())
 (d/'consumer.asm').write_text(' .assume adl=1\n .org $40000\n include "../probe/layout.inc"\n include "../probe/calls.inc"\n call golem_buffered_multiplyUnsigned\n call golem_native_multiplySigned\n ret\n')
 subprocess.run([str(ROOT/'docs/tasks/GOLEM-007/build/ez80asm'),'consumer.asm','consumer.bin','-c'],cwd=d,check=True)
 (HERE/'frontend-checks.json').write_text(json.dumps({'rejections':rejections,'actual_assembler_consumer':True},indent=2)+'\n')
 print('Multiplication typing/exclusion/assembler checks pass.')
if __name__=='__main__':main()
