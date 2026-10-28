"""Agent-only retrieval of human numeric-printing results; no keyboard prompt."""
from pathlib import Path
import hashlib,json,subprocess
HERE=Path(__file__).resolve().parent;BUILD=HERE/'build';BUILD.mkdir(exist_ok=True)
HOST='oryx-linuxlite';REMOTE='Agon/mystuff/golem/.emulator/golem-052'
def main():
 subprocess.run(['ssh',HOST,'cd ~/'+REMOTE+' && sha256sum --check --status .runtime.sha256'],check=True)
 for name in ('results.dat','done.dat'):subprocess.run(['scp',HOST+':'+REMOTE+'/sdcard/golem/printing/'+name,str(BUILD/('human-'+name))],check=True)
 oracle=json.loads((HERE/'target-cases.json').read_text());raw=bytes.fromhex(oracle['expected_observations']);size=len(raw)//oracle['cases']
 indices=list(range(6))+list(range(oracle['numeric_cases'],oracle['cases']))
 expected=b''.join(raw[i*size:(i+1)*size] for i in indices)
 assert (BUILD/'human-results.dat').read_bytes()==expected
 assert (BUILD/'human-done.dat').read_bytes()==b'\0'
 report={'saved_results_passed':True,'bytes':len(expected),'sha256':hashlib.sha256(expected).hexdigest()}
 (HERE/'human-evidence.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
