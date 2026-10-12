"""Build/qualify the human demo: one Golem picture, without diagnostic scenes."""

import hashlib
import importlib.util
import json
import os
import shutil
import signal
import subprocess
import sys
import time
from pathlib import Path

sys.dont_write_bytecode = True
TASK = Path(__file__).resolve().parent
PROJECT = TASK.parents[2]
BUILD = TASK / "build/demo"
PROFILE = PROJECT / ".emulator"
APP = PROFILE / "sdcard/golem-019"
spec = importlib.util.spec_from_file_location("bitmap_qualification",TASK / "qualify.py")
qualification = importlib.util.module_from_spec(spec)
spec.loader.exec_module(qualification)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def generate():
    BUILD.mkdir(parents=True,exist_ok=True)
    options = ["--reserve","8195:8195","--reserve","8201:8202","--reserve","8463:8463","--reserve","60000:60000"]
    subprocess.run([str(PROJECT / "build/golemc"),"--resident","--emit-asm",*options,
                    str(TASK / "review.golem"),str(BUILD)],check=True)
    tape = qualification.Tape()
    # Print labels after drawing: text output changes the stock Canvas clip.
    tape.send(qualification.display_setup(20))
    tape.data.append(5)
    tape.send(qualification.write(qualification.QUERY,bytes([23,0,128,0])))
    tape.send((BUILD / "bootstrap.bin").read_bytes())
    tape.data.append(3); tape.sent += 6  # Exactly one generated golem_drawWolf call.
    assets = json.loads((TASK / "assets.json").read_text())["assets"]
    pixels = {a["buffer_id"]:(TASK / "assets" / (a["name"]+".rgba2")).read_bytes() for a in assets}
    screen = qualification.reference([8202,0,0],0,assets,pixels)
    for x,y in qualification.sample_points([8202,0,0],assets,pixels,screen):
        tape.pixel(x+96,y+16,"Golem review.golem picture",screen[y*320+x])
    # Observation packets do not draw. Compiler cleanup preserves the assets.
    tape.send((BUILD / "cleanup.bin").read_bytes())
    tape.data.append(0)
    assert len(tape.expected) <= 2048
    (BUILD / "tape.bin").write_bytes(tape.data)
    (TASK / "demo-expected.dat").write_bytes(tape.expected)
    meta = dict(checks=len(tape.checks),fixture=dict(checks=tape.checks),observation_bytes=len(tape.expected),
                expected_sha256=sha(TASK / "demo-expected.dat"),golem_entry_calls=1,
                direct_bitmap_draws=0,diagnostic_scenes=0,
                golem_source="review.golem",assembly_source="demo_guest.asm",
                display=dict(width=512,height=384,art_offset=[96,16],text_top=256))
    subprocess.run([str(PROJECT / "docs/tasks/GOLEM-007/build/ez80asm"),"demo_guest.asm","build/demo/probe.bin","-l","-c"],cwd=TASK,check=True)
    (TASK / "demo-checks.json").write_text(json.dumps(meta,indent=2)+"\n")
    return meta


def execute(meta):
    runtime = (PROFILE / "fab-agon-emulator.bin").resolve()
    assert not (PROFILE / "fab-agon-emulator").is_symlink()
    assert not (PROFILE / ".bespoke-vdp-profile").exists()
    for p in Path("/proc").glob("[0-9]*"):
        try:
            if (p / "exe").resolve() == runtime: raise RuntimeError("Close the emulator before qualification")
        except (FileNotFoundError,PermissionError): pass
    accepted = json.loads((TASK / "emulator-checks.json").read_text())["runtime_identity"]
    inputs = [PROJECT / p for p in accepted if p.startswith(".emulator/")]
    inputs += [BUILD / "probe.bin",BUILD / "tape.bin",TASK / "review.golem",TASK / "demo_guest.asm",
               TASK / "prepare_demo.py",TASK / "agnb_api.inc",TASK / "agnb_support.inc",TASK / "assets/wolf-review.agnb"]
    before = {str(p.relative_to(PROJECT)):sha(p) for p in inputs}
    for p,h in accepted.items():
        if p.startswith(".emulator/"): assert before[p] == h, p
    APP.mkdir(exist_ok=True)
    shutil.copyfile(BUILD / "probe.bin",APP / "probe.bin")
    shutil.copyfile(TASK / "assets/wolf-review.agnb",APP / "wolf-review.agnb")
    result = APP / "results.dat"; result.unlink(missing_ok=True)
    startup = PROFILE / "sdcard/autoexec.txt"
    previous = startup.read_bytes() if startup.exists() else None
    env = {k:v for k,v in os.environ.items() if k not in ("LD_PRELOAD","BASH_ENV")}
    env.update(SDL_VIDEODRIVER="dummy",SDL_AUDIODRIVER="dummy")
    started = time.monotonic()
    process = None; mapped = set(); actual_exe = None
    try:
        startup.write_bytes(b"SET KEYBOARD 1\r\ncd /golem-019\r\nload probe.bin\r\nrun .\r\n")
        with (BUILD / "emulator.log").open("w") as log:
            process = subprocess.Popen(["./fab-agon-emulator","--renderer","sw"],cwd=PROFILE,env=env,
                                       stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
            while not result.exists() or result.stat().st_size < meta["observation_bytes"]:
                if process.poll() is not None: raise RuntimeError("Emulator exited before demo results")
                maps = Path(f"/proc/{process.pid}/maps").read_text()
                mapped.update(Path(line.split(maxsplit=5)[5]).resolve() for line in maps.splitlines() if "/vdp_" in line and ".so" in line)
                if mapped:
                    assert mapped == {(PROFILE / "firmware/vdp_platform.so").resolve()}
                    actual_exe = Path(f"/proc/{process.pid}/exe").resolve(); assert actual_exe == runtime
                if time.monotonic()-started > 60: raise TimeoutError("Inspect build/demo/emulator.log")
                time.sleep(.05)
    finally:
        if process and process.poll() is None:
            os.killpg(process.pid,signal.SIGTERM)
            try: process.wait(timeout=5)
            except subprocess.TimeoutExpired: os.killpg(process.pid,signal.SIGKILL);process.wait()
        if previous is None: startup.unlink(missing_ok=True)
        else: startup.write_bytes(previous)
    assert mapped and actual_exe
    assert all(sha(PROJECT / p) == h for p,h in before.items())
    actual = result.read_bytes()
    assert actual == (TASK / "demo-expected.dat").read_bytes(), "Demo pixels differ from the Wolf reference"
    (TASK / "demo-observed.dat").write_bytes(actual)
    meta.update(pass_headless=True,execution_seconds=round(time.monotonic()-started,3),
                runtime_identity=before,loaded_vdp=str(next(iter(mapped))),executed_fab=str(actual_exe),
                human_validation="pending",graphical_emulator_launched=False)
    (TASK / "demo-checks.json").write_text(json.dumps(meta,indent=2)+"\n")
    print(f'Golem-only demo passes: {meta["checks"]} pixels, one Golem call, {meta["execution_seconds"]}s.')


if __name__ == "__main__":
    execute(generate())
