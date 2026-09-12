"""Host regression/golden/negative checks; not a substitute for native VDP tests."""
from pathlib import Path
import subprocess,json,hashlib
ROOT=Path(__file__).resolve().parents[1]
def main():
    results={}
    for name in ['existing','hosted','imports','admission','elements']:
        command=['g++','-std=c++17','-Wall','-Wextra','-Werror','-g','-O1','-fsanitize=address,undefined',str(ROOT/'tests'/('test_'+name+'.cpp')),'-o',str(ROOT/'build'/('test_'+name))]
        subprocess.run(command,check=True)
        results[name]={'command':command,'stdout':subprocess.check_output([str(ROOT/'build'/('test_'+name))]).decode()}
    subprocess.run(['make','-C',str(ROOT/'src')],check=True)
    cases=json.loads((ROOT/'tests/hosted-design-cases.json').read_text())
    work=ROOT/'build/design-cases';work.mkdir(exist_ok=True)
    for src in cases['accept']:
        (work/'input').write_text(src)
        subprocess.run([str(ROOT/'build/golemc'),'--hosted',str(work/'input'),str(work/'output')],check=True,stdout=subprocess.DEVNULL)
    for case in cases['reject']:
        (work/'input').write_text(case['source']);(work/'output').write_bytes(b'preserve')
        result=subprocess.run([str(ROOT/'build/golemc'),'--hosted',str(work/'input'),str(work/'output')],capture_output=True)
        assert result.returncode==1 and (work/'output').read_bytes()==b'preserve',case
    results['design_cases']={'accepted':len(cases['accept']),'rejected':len(cases['reject']),'failure_preserves_output':True}
    results['source_hashes']={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [ROOT/'src/golemc.cpp',ROOT/'src/hosted.hpp',ROOT/'tests/test_existing.cpp',ROOT/'tests/test_hosted.cpp',ROOT/'tests/test_imports.cpp',ROOT/'tests/test_admission.cpp',ROOT/'tests/test_elements.cpp',ROOT/'tests/run_hosted.py']}
    (ROOT/'build/hosted-tests.json').write_text(json.dumps(results,indent=2)+'\n')
    print('Host sanitizer/golden/negative tests passed; build/hosted-tests.json')
if __name__=='__main__':main()
