"""Copy built fixtures and prepare the stopped Oryx review; no graphical launch."""
from pathlib import Path
import argparse
import shutil
import subprocess

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
REMOTE='Agon/mystuff/golem/docs/tasks/GOLEM-025/'

def stage():
    # A literal snapshot of the canonical setup tool, not a competing generator.
    shutil.copyfile(ROOT.parent/'agon-dev-env/scripts/setup_emulator.py',HERE/'build/setup_emulator.py')
    subprocess.run(['rsync','-a','--exclude=.emulator*','--exclude=__pycache__','--exclude=evidence',
                    '--exclude=compiler','--exclude=compiler-asan','--exclude=native-host',
                    str(HERE)+'/', 'oryx-linuxlite:'+REMOTE],check=True)
    subprocess.run(['ssh','-o','BatchMode=yes','oryx-linuxlite',
                    'cd ~/Agon/mystuff/golem && .venv/bin/python docs/tasks/GOLEM-025/profile.py review'],check=True)

def retrieve():
    (HERE/'evidence').mkdir(exist_ok=True)
    subprocess.run(['rsync','-a','oryx-linuxlite:'+REMOTE+'evidence/',str(HERE/'evidence')+'/'],check=True)
    subprocess.run(['rsync','-a','oryx-linuxlite:'+REMOTE+'review-screen.png',str(HERE/'review-screen.png')],check=True)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--retrieve',action='store_true',help='Retrieve automated receipts and captures only')
    args=parser.parse_args()
    retrieve() if args.retrieve else stage()
