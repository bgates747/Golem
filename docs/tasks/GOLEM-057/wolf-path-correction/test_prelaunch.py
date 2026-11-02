"""Exercise the original runner's actual prelaunch path, with no process spawn."""
from pathlib import Path
from unittest.mock import patch
import hashlib,importlib.util,json,os,tempfile,unittest
HERE=Path(__file__).resolve().parent
RUNNER=Path('/Agon/mystuff/Wolf3dOrig/agonport/vdplet/audio-ring/runner.py')
SHA='f0ffe79892f9a93edc41f590bb02551583580e899b7232545253b1141962ca98'
def load(name,path):
 s=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
prepare=load('root_wolf_prepare',HERE/'prepare_profile.py')
stage=load('root_wolf_stage',HERE/'stage_oryx.py')
class LaunchBoundary(Exception):pass
class Prelaunch(unittest.TestCase):
 def runner(self,profile):
  self.assertEqual(hashlib.sha256(RUNNER.read_bytes()).hexdigest(),SHA)
  r=load('actual_wolf_runner',RUNNER)
  r.PROFILE=profile;r.RUN=profile/'audio-ring-run';r.ARTIFACTS=profile/'last-run';r.OUTPUT=profile/'ring-output';r.EVENTS=r.RUN/'events.txt';r.REPORT=r.RUN/'session.json'
  return r
 def test_accepted_runner_reaches_launch_with_corrected_output(self):
  with tempfile.TemporaryDirectory() as d:
   profile=Path(d)/prepare.PROFILE.name;profile.mkdir();r=self.runner(profile)
   output=profile/prepare.OUTPUT.name;output.mkdir()
   launcher=prepare.rewrite_launcher('#!/bin/bash\nexport FLEXE_RING_OBSERVER_DIR="old/ring-output"\nexport FLEXE_RING_WINDOW_AFTER_MS="90000"\n','old/ring-output',str(output))
   self.assertEqual(output,r.OUTPUT)
   self.assertIn('export FLEXE_RING_OBSERVER_DIR="'+str(r.OUTPUT)+'"',launcher)
   calls=[]
   def boundary(argv,**kw):
    calls.append((argv,kw));kw['stdout'].close();raise LaunchBoundary()
   with patch.dict(os.environ,{'SDL_AUDIODRIVER':'pulseaudio'},clear=True),patch.object(r.subprocess,'run') as check,patch.object(r.subprocess,'Popen',side_effect=boundary):
    with self.assertRaises(LaunchBoundary):r.main()
    check.assert_called_once_with(['sha256sum','--check','--status','.runtime.sha256'],cwd=profile,check=True)
   self.assertEqual(len(calls),1);argv,kw=calls[0]
   self.assertEqual(argv,['./flexe-agon-emulator']);self.assertEqual(kw['cwd'],profile)
   self.assertEqual(kw['env']['SDL_AUDIODRIVER'],'pulseaudio');self.assertTrue(kw['start_new_session'])
   self.assertEqual(Path(stage.REMOTE_PROFILE).name,profile.name)
   self.assertTrue((r.RUN/'events.txt').is_file());self.assertFalse(any(r.OUTPUT.iterdir()))
 def test_old_mismatch_fails_before_process_boundary(self):
  with tempfile.TemporaryDirectory() as d:
   profile=Path(d);r=self.runner(profile);(profile/'ring-output-late-corrected').mkdir()
   with patch.dict(os.environ,{'SDL_AUDIODRIVER':'pulseaudio'},clear=True),patch.object(r.subprocess,'run'),patch.object(r.subprocess,'Popen') as spawn:
    with self.assertRaises(FileNotFoundError):r.main()
    spawn.assert_not_called()
 def test_nonfresh_correct_path_fails_before_process_boundary(self):
  with tempfile.TemporaryDirectory() as d:
   profile=Path(d);r=self.runner(profile);r.OUTPUT.mkdir();(r.OUTPUT/'evidence').write_text('keep')
   with patch.dict(os.environ,{'SDL_AUDIODRIVER':'pulseaudio'},clear=True),patch.object(r.subprocess,'run'),patch.object(r.subprocess,'Popen') as spawn:
    with self.assertRaisesRegex(SystemExit,'not fresh'):r.main()
    spawn.assert_not_called();self.assertEqual((r.OUTPUT/'evidence').read_text(),'keep')
 def test_target_paths_and_original_pins_remain_consistent(self):
  self.assertEqual(prepare.OUTPUT.name,'ring-output')
  self.assertTrue(prepare.PROFILE.name.endswith('-r02'));self.assertEqual(Path(stage.REMOTE_PROFILE).name,prepare.PROFILE.name)
  source=(HERE/'stage_oryx.py').read_text()
  self.assertIn('test -d ring-output',source);self.assertIn('find ring-output -mindepth',source)
  self.assertNotIn('ring-output-late-corrected',source)
  self.assertEqual(prepare.PINS['runner'],SHA)
  self.assertEqual(prepare.PINS['backend'],'943f432fecb96689f3a0d2ae80c72d08e20903dfe2a58a00ba88d8c7727ad400')
  original=Path('/Agon/mystuff/Wolf3dOrig/agonport/vdplet/audio-ring-late-corrected/target')
  pins=json.loads((HERE/'inputs.json').read_text())
  for name,pin in pins.items():
   self.assertEqual(hashlib.sha256((original/name).read_bytes()).hexdigest(),pin['original_sha256'])
   self.assertEqual(hashlib.sha256((HERE/name).read_bytes()).hexdigest(),pin['candidate_sha256'])
if __name__=='__main__':unittest.main()
