"""Check paired admission, signed overflow metadata and real assembly consumers."""
import json,subprocess
from build_delivery import HERE,BUILD,ROOT

def main():
 d=BUILD/'syntax';d.mkdir(exist_ok=True);rejections=[]
 for text in ['uint32 q=0 sint32 r=0 Sub f (q,r)=divmod(1,1) EndSub',
              'uint32 q=0 Sub f (q,q)=divmod(1,1) EndSub',
              'sint16 q=0 sint16 r=0 Sub f (q,r)=divmod(1,1) EndSub',
              'uint32 q=0 uint32 r=0 sint32 n=1 Sub f (q,r)=divmod(n,1) EndSub',
              'uint32 q=0 Array uint32 a(0:1)=Fill(0) Sub f (q,a(0))=divmod(1,1) EndSub',
              'uint16 i=0 uint16 r=0 Sub f For i In Range(0:1) (i,r)=divmod(1,1) Next i EndSub']:
  p=d/'source.golem';p.write_text(text);r=subprocess.run([str(BUILD/'compiler'),str(p),str(d/'package')],capture_output=True,text=True);assert r.returncode;rejections.append(r.stderr.strip())
 p=d/'signed.golem';p.write_text('sint32 q=0 sint32 r=0 Sub f If true Then (q,r)=divmod(-2147483648,-1) EndIf EndSub')
 subprocess.run([str(BUILD/'compiler-asan'),str(p),str(d/'signed')],check=True,stdout=subprocess.DEVNULL)
 codes=json.loads((d/'signed/interface.json').read_text())['error']['codes'];assert codes['zero_divisor']==4 and codes['signed_division_overflow']==6
 (d/'consumer.asm').write_text(' .assume adl=1\n .org $40000\n include "../probe/layout.inc"\n include "../probe/calls.inc"\n call golem_buffered_divideUnsigned\n call golem_native_divideSigned\n ret\n')
 subprocess.run([str(ROOT/'docs/tasks/GOLEM-007/build/ez80asm'),'consumer.asm','consumer.bin','-c'],cwd=d,check=True)
 (HERE/'frontend-checks.json').write_text(json.dumps({'rejections':rejections,'signed_zero_overflow_metadata':codes,'nested_pair_no_array_sanitizer_admission':True,'actual_assembler_consumer':True},indent=2)+'\n')
 print('Paired division typing/metadata/assembler checks pass.')
if __name__=='__main__':main()
