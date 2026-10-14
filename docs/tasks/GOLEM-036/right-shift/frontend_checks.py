"""Check shift grammar/count types/adjacency and comparison precedence."""
import json,subprocess
from build_delivery import HERE,BUILD,ROOT

def main():
 d=BUILD/'syntax';d.mkdir(exist_ok=True);rejections=[]
 for text in ['uint32 a=1 Sub f a=a >>32 EndSub','uint32 a=1 Sub f a=a >>-1 EndSub',
              'sint32 a=1 Sub f a=a >>65535 EndSub','uint16 a=1 Sub f a=a >>1 EndSub',
              'sint16 a=1 Sub f a=a >>1 EndSub','float32 a=1.0 Sub f a=a >>1 EndSub',
              'uint32 a=1 uint32 bits=1 Sub f a=a >>bits EndSub',
              'uint32 a=1 Sub f a=a > >1 EndSub','uint32 a=1 Sub f a=a >>>1 EndSub',
              'uint32 a=1 Sub f If false Then a=a >>32 EndIf EndSub']:
  p=d/'source.golem';p.write_text(text);r=subprocess.run([str(BUILD/'compiler-asan'),str(p),str(d/'package')],capture_output=True,text=True);assert r.returncode;rejections.append(r.stderr.strip())
 p=d/'valid.golem';p.write_text('sint32 a=-65 uint16 b=6 bool f=false uint32 u=4294967295 Sub valid a=a >> b >> 0 f=a >> b < 0 f=u >> b >= 0 f=a > -66 EndSub')
 subprocess.run([str(BUILD/'compiler-asan'),str(p),str(d/'valid')],check=True,stdout=subprocess.DEVNULL)
 ast=json.loads((d/'valid/program.json').read_text());nodes=ast['procedures'][0]['assignments'];assert nodes[0]['value']['operator']=='>>' and nodes[0]['value']['left']['operator']=='>>';assert nodes[1]['value']['operator']=='<' and nodes[1]['value']['left']['operator']=='>>'
 (d/'consumer.asm').write_text(' .assume adl=1\n .org $40000\n include "../probe/layout.inc"\n include "../probe/calls.inc"\n call golem_buffered_shiftUnsigned\n call golem_native_shiftSigned\n ret\n')
 subprocess.run([str(ROOT/'docs/tasks/GOLEM-007/build/ez80asm'),'consumer.asm','consumer.bin','-c'],cwd=d,check=True)
 (HERE/'frontend-checks.json').write_text(json.dumps({'rejections':rejections,'sanitized_chain_comparison_grammar':True,'actual_assembler_consumer':True},indent=2)+'\n');print('Shift grammar/count/assembler checks pass.')
if __name__=='__main__':main()
