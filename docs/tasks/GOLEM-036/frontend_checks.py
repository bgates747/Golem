"""Verify first-wide-tranche admission, integer widths and explicit exclusions."""
import json,subprocess
from build_delivery import HERE,BUILD,ROOT

def main():
    output=BUILD/'syntax';output.mkdir(exist_ok=True);rejected=[]
    def attempt(source,valid=False):
        p=output/'source.golem';p.write_text(source)
        result=subprocess.run([str(BUILD/'compiler'),str(p),str(output/'package')],capture_output=True,text=True)
        assert bool(result.returncode)!=valid,(source,result.stderr)
        if not valid:rejected.append(result.stderr.strip())
    for decl in ['uint32 a=-1','uint32 a=4294967296','sint32 a=-2147483649','sint32 a=2147483648',
                 'uint32 a Bounds(0.5,9)=1','sint32 a Bounds(-2147483649,0)=0',
                 'Array uint32 a(0:16383)=Fill(0)','Array sint32 a(0:1)=Fill(2147483648)']:
        attempt('uint16 x=0 '+decl+' Sub f x=0 EndSub')
    for body in ['a=a*a','(a,b)=divmod(a,b)','a=b+1','a=uint32(f)','f=float32(a)']:
        attempt('uint32 a=1 sint32 b=1 float32 f=1.0 Sub job '+body+' EndSub')
    attempt('uint32 i=0 uint32 selected=0 Array uint32 a(0:1)=Fill(0) Sub f selected=a(i) EndSub')
    attempt('uint32 a=0 ReadOnly Array uint32 b(0:1)=Fill(0) Sub f b(0)=a EndSub')
    # Explicit conversion does not widen an already typed narrow addition.
    attempt('uint16 a=65535 uint32 b=0 Sub f b=uint32(a+1) EndSub',valid=True)
    for decl,body in [('uint32 a=4294967295','a=a-1'),('sint32 a=-2147483648','a=a+1'),
                      ('Array uint32 a(65535:65535)=Fill(4294967295)','a(65535)=65536'),
                      ('Array sint32 a(-32768:-32768)=Fill(-2147483648)','a(-32768)=a(-32768)+1')]:
        attempt(decl+' Sub f '+body+' EndSub',valid=True)
    source=output/'consumer.asm';source.write_text(' .assume adl=1\n .org $40000\n include "package/layout.inc"\n include "package/calls.inc"\n call golem_buffered_f\n ret\n')
    subprocess.run([str(ROOT/'docs/tasks/GOLEM-007/build/ez80asm'),'consumer.asm','consumer.bin','-c'],cwd=output,check=True)
    receipt={'rejections':len(rejected),'diagnostics':rejected,'wide_extreme_literals_and_arrays_admitted':True,'actual_assembler_consumer':True}
    (HERE/'frontend-checks.json').write_text(json.dumps(receipt,indent=2)+'\n');print(len(rejected),'wide grammar/admission rejections pass.')
if __name__=='__main__':main()
