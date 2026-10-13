"""Exercise array grammar/build admission without duplicating emitter logic."""
from pathlib import Path
import json,subprocess
from build_delivery import HERE,BUILD,ROOT,sha

def main():
    rejected=[];output=BUILD/'syntax';output.mkdir(exist_ok=True)
    def attempt(source,options=(),valid=False):
        p=output/'source.golem';p.write_text(source)
        result=subprocess.run([str(BUILD/'compiler'),str(p),str(output/'package'),*map(str,options)],capture_output=True,text=True)
        assert bool(result.returncode)!=valid,(source,result.stderr)
        if not valid:rejected.append(result.stderr.strip())
    pre='uint16 i=0 uint16 value=0 '
    declarations=['Array float32 a(0:1)=Fill(0.0)','Array bool a(0:1)=Fill(false)',
      'Array uint16 a(1:0)=Fill(0)','Array uint16 a(-32769:-32768)=Fill(0)',
      'Array uint16 a(0:32767)=Fill(0)','Array uint16 a(0:65536)=Fill(0)',
      'Array uint16 a(0:1:1)=Fill(0)','Array uint16 a(0:1,0:1)=Fill(0)',
      'Array uint16 a(0:1)=Values(0)','Array uint16 a(0:1)=Values(0,1,2)',
      'Array uint16 a(0:1)=Fill(-1)','Array sint16 a(0:1)=Fill(32768)',
      'Array uint16 a(0:1)']
    for declaration in declarations:attempt(pre+declaration+' Sub f value=0 EndSub')
    for body in ['a(0)=1','value=a(2)','value=a(-1)','value=a(0,1)','value=a(true)','value=a(0.5)']:
        attempt(pre+'ReadOnly Array uint16 a(0:1)=Fill(0) Sub f '+body+' EndSub')
    attempt(pre+'Array sint16 a(0:1)=Fill(0) Sub f value=a(i) EndSub')
    data=HERE/'data/sine.dat'
    for options in (['--array-data','unknown='+str(data)],['--array-data','a='+str(data)],['--array-data','a=x','--array-data','a=y']):
        attempt(pre+'Array uint16 a(0:1)=Fill(0) Sub f value=0 EndSub',options)
    for declaration,body in [('Array uint16 a(65535:65535)=Values(65535)','a(65535)=1'),
                             ('Array uint16 a(0:0)=Fill(0)','a(0)=a(0)+1'),
                             ('Array sint16 a(-32768:-32768)=Values(-32768)','a(-32768)=0')]:
        attempt(declaration+' Sub f '+body+' EndSub',valid=True)
    # Real assembly accepts generated array equates including negative lower bounds.
    source=output/'consumer.asm';source.write_text(' .assume adl=1\n .org $40000\n include "package/layout.inc"\n include "package/calls.inc"\n call golem_buffered_f\n ret\n')
    subprocess.run([str(ROOT/'docs/tasks/GOLEM-007/build/ez80asm'),'consumer.asm','consumer.bin','-c'],cwd=output,check=True)
    receipt={'rejections':len(rejected),'messages':rejected,'array_only_programs':3,'actual_assembler_consumer':True}
    (HERE/'frontend-checks.json').write_text(json.dumps(receipt,indent=2)+'\n');print(receipt['rejections'],'array grammar/build rejections pass.')
if __name__=='__main__':main()
