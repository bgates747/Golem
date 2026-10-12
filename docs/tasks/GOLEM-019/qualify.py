"""Qualify resident Wolf bitmaps through stock MOS, AGNB and VDP pixel queries."""

import hashlib
import importlib.util
import json
import os
import shutil
import signal
import struct
import subprocess
import sys
import time
from pathlib import Path

sys.dont_write_bytecode = True
TASK = Path(__file__).resolve().parent
PROJECT = TASK.parents[2]
BUILD = TASK / "build"
PROFILE = PROJECT / ".emulator"
APP = PROFILE / "sdcard/golem-019"
QUERY, TRANSFORM, FOREIGN = 60000, 60001, 50000
spec = importlib.util.spec_from_file_location("geometry_transport", TASK.parent / "GOLEM-018/qualify.py")
transport = importlib.util.module_from_spec(spec)
spec.loader.exec_module(transport)
Tape, word, packet, write = (getattr(transport, n) for n in ("Tape", "word", "packet", "write"))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def update(buffer_id, data, offset=0):
    return packet(buffer_id, 5) + b"\xc2" + word(offset) + word(len(data)) + data


def draw(buffer_id, x, y):
    # Independent direct stock path, bypassing compiler templates and patches.
    return bytes([23,27,32]) + word(buffer_id) + bytes([23,27,3]) + struct.pack("<hh", x, y)


def placements(values, assets):
    return [(values[0], values[1], values[2])] + [
        (a["buffer_id"], a["x"], a["y"]) for a in assets[1:]
    ]


def reference(values, background, assets, pixels):
    """RGBA2222 pixels to physical RGB222, with binary alpha and screen clipping."""
    screen = bytearray([background]) * (320 * 240)
    by_id = {a["buffer_id"]:a for a in assets}
    for buffer_id, x, y in placements(values, assets):
        a = by_id[buffer_id]
        for index, value in enumerate(pixels[buffer_id]):
            px, py = x + index % a["width"], y + index // a["width"]
            if value >> 6 and 0 <= px < 320 and 0 <= py < 240:
                screen[py * 320 + px] = ((value & 3) << 4) | (value & 12) | ((value >> 4) & 3)
    return screen


def sample_points(values, assets, pixels, screen):
    points = {(x,y) for y in range(0,240,24) for x in range(0,320,24)}
    by_id = {a["buffer_id"]:a for a in assets}
    for buffer_id, x, y in placements(values, assets):
        a = by_id[buffer_id]
        width, height = a["width"], a["height"]
        stride = 16 if width == 320 else 4
        points.update((x+px,y+py) for py in range(0,height,stride) for px in range(0,width,stride))
        # Pixel boundaries and both transparent and opaque parts of each image.
        points.update((x+px,y+py) for px in (-1,0,width-1,width) for py in (-1,0,height-1,height))
        for opaque in (False, True):
            selected = [i for i,p in enumerate(pixels[buffer_id]) if bool(p >> 6) == opaque]
            if selected:
                points.update((x+i%width,y+i//width) for i in selected[::max(1,len(selected)//20)])
    # Include every output colour represented on the independently composed frame.
    colours = {}
    for index, value in enumerate(screen):
        colours.setdefault(value, (index % 320, index // 320))
    points.update(colours.values())
    return sorted((x,y) for x,y in points if 0 <= x < 320 and 0 <= y < 240)


def installed(directory):
    raw = (directory / "bootstrap.bin").read_bytes()
    assert raw[:7] == bytes([23,0,248,1,0,1,0])
    pos, blocks = 7, {}
    while pos < len(raw):
        assert raw[pos:pos+3] == bytes([23,0,160])
        buffer_id = struct.unpack_from("<H",raw,pos+3)[0]
        if raw[pos+5] == 2:
            pos += 6
        else:
            assert raw[pos+5] == 0
            size = struct.unpack_from("<H",raw,pos+6)[0]
            blocks[buffer_id] = raw[pos+8:pos+8+size]
            pos += 8 + size
    return blocks


def display_setup(mode):
    data = bytes([22,mode]) + b"".join(bytes([19,i,i,0,0,0]) for i in range(64))
    return data + bytes([23,0,192,0,29,0,0,0,0,26,23,1,0,25,4,0,0,0,0])


def generate():
    BUILD.mkdir(exist_ok=True)
    assets = json.loads((TASK / "assets.json").read_text())["assets"]
    pixels = {a["buffer_id"]:(TASK / "assets" / (a["name"]+".rgba2")).read_bytes() for a in assets}
    assert all(hashlib.sha256(pixels[a["buffer_id"]]).hexdigest() == a["sha256"] for a in assets)
    tape = Tape()
    # Identity palette makes the returned logical pixel index equal RGB222.
    # This changes logical numbering, not the RGB appearance of bitmap pixels.
    tape.send(display_setup(8))
    tape.data.append(5)  # Assembly opens the one AGNB and registers all four IDs.
    tape.send(write(QUERY,bytes([23,0,128,0])))
    packages = []
    initial = [8202,0,0]
    for name, extra in (("default", []), ("relocated", ["--reserve","1:254","--reserve","260:264"])):
        directory = BUILD / name
        options = ["--reserve","8195:8195","--reserve","8201:8202","--reserve","8463:8463",
                   "--reserve","50000:50000","--reserve","60000:60001", *extra]
        subprocess.run([str(PROJECT / "build/golemc"),"--resident","--emit-asm",*options,
                        str(TASK / "bitmaps.golem"),str(directory)],check=True)
        m = json.loads((directory / "interface.json").read_text())
        blocks = installed(directory)
        canaries = {0:b"root",FOREIGN:b"borrowed"}
        if name == "relocated": canaries.update({1:b"low",260:b"hole"})
        for buffer_id,data in canaries.items(): tape.send(write(buffer_id,data))
        # Stock bootstrap must be sufficient when affine support starts disabled.
        tape.send(bytes([23,0,248,1,0,0,0]) + (directory / "bootstrap.bin").read_bytes())

        def call():
            if name == "default":
                tape.data.append(3)
                tape.sent += 6
            else:
                tape.send((directory / "invoke.bin").read_bytes())

        cases = [("initial",initial,0), ("changed-id",[8463,0,0],0),
                 ("changed-position",[8463,66,16],0), ("clipped",[8202,-17,-9],0),
                 ("transparent",initial,21), ("prior-transform",initial,0)]
        jobs = 0
        for label, values, background in cases:
            raw = struct.pack("<Hhh", *values)
            if label == "changed-id": tape.send(update(m["input"]["id"],raw[:2]))
            elif label == "changed-position": tape.send(update(m["input"]["id"],raw[2:],2))
            elif label != "initial": tape.send(update(m["input"]["id"],raw))
            tape.send(bytes([18,0,background+128,16]))
            if label == "prior-transform":
                tape.send(write(TRANSFORM,struct.pack("<9f",1,0,40,0,1,20,0,0,1)))
                tape.send(bytes([23,0,150,1]) + word(TRANSFORM) + draw(8195,0,0))
                # A nonblack source pixel translated beyond the untransformed
                # 32-pixel width proves this transform is active before the call.
                control_index = next(i for i,p in enumerate(pixels[8195]) if p >> 6 and p & 63)
                p = pixels[8195][control_index]
                tape.pixel(40+control_index%32,20+control_index//32,f"{name} active transform control",
                           ((p & 3) << 4) | (p & 12) | ((p >> 4) & 3))
                tape.send(b"\x10")
            call(); jobs += 1
            if label == "initial": call(); jobs += 1  # Repeat with no input upload.
            tape.read(m["input"]["id"],0,raw,f"{name} {label} persistent inputs")
            screen = reference(values,background,assets,pixels)
            points = sample_points(values,assets,pixels,screen)
            refs = [tape.pixel(x,y,f"{name} {label} resident",screen[y*320+x]) for x,y in points]
            if label in ("initial","clipped","transparent"):
                tape.send(bytes([18,0,background+128,16,23,0,150,1,255,255])
                          + b"".join(draw(*p) for p in placements(values,assets)))
                for (x,y), ref in zip(points,refs):
                    tape.pixel(x,y,f"{name} {label} direct",reference=ref)
        tape.read(m["entry"]["id"],0,blocks[m["entry"]["id"]],f"{name} unchanged entry")
        tape.send((directory / "cleanup.bin").read_bytes())
        for buffer_id in blocks: tape.read(buffer_id,0,b"\x55",f"{name} cleanup absence")
        for buffer_id,data in canaries.items(): tape.read(buffer_id,0,data,f"{name} external preserved")
        for a in assets:
            data = pixels[a["buffer_id"]]
            tape.read(a["buffer_id"],0,data[:8],f"{name} asset prefix after cleanup")
            tape.read(a["buffer_id"],len(data)-8,data[-8:],f"{name} asset suffix after cleanup")
        # Existing registered bitmaps must still draw after compiler cleanup.
        tape.send(bytes([18,0,128,16]) + b"".join(draw(*p) for p in placements(initial,assets)))
        screen = reference(initial,0,assets,pixels)
        for x,y in sample_points(initial,assets,pixels,screen):
            tape.pixel(x,y,f"{name} borrowed bitmap after cleanup",screen[y*320+x])
        tape.send((directory / "bootstrap.bin").read_bytes())
        tape.read(m["input"]["id"],0,struct.pack("<Hhh",*initial),f"{name} reinstall initial inputs")
        tape.send((directory / "cleanup.bin").read_bytes())
        packages.append(dict(name=name,manifest=m,jobs=jobs,generated_wrapper=name=="default",
                             cases=[c[0] for c in cases],artifacts_sha256={p.name:sha(p) for p in directory.iterdir()}))
    # A larger final screen leaves a separate text area below native-size art.
    # This package has the same entry ID and uses the same generated wrapper.
    directory = BUILD / "review"
    options = ["--reserve","8195:8195","--reserve","8201:8202","--reserve","8463:8463",
               "--reserve","50000:50000","--reserve","60000:60001"]
    subprocess.run([str(PROJECT / "build/golemc"),"--resident","--emit-asm",*options,
                    str(TASK / "review.golem"),str(directory)],check=True)
    review = json.loads((directory / "interface.json").read_text())
    assert review["entry"]["id"] == packages[0]["manifest"]["entry"]["id"]
    tape.send(display_setup(20) + (directory / "bootstrap.bin").read_bytes())
    tape.data.append(3); tape.sent += 6
    screen = reference(initial,0,assets,pixels)
    for x,y in sample_points(initial,assets,pixels,screen):
        tape.pixel(x+96,y+16,"final centred review picture",screen[y*320+x])
    tape.send((directory / "cleanup.bin").read_bytes())
    tape.data.append(0)
    assert len(tape.expected) <= 24000
    (BUILD / "tape.bin").write_bytes(tape.data)
    (BUILD / "fixed-expected.dat").write_bytes(tape.expected)
    meta = dict(packages=packages,checks=tape.checks,observation_bytes=len(tape.expected),
                tape_bytes=len(tape.data),tape_sent_vdu_bytes=tape.sent,agnb_loads=1,
                pixel_pairs=sum(c.get("reference") is not None for c in tape.checks),
                pixel_checks=sum("pixel" in c for c in tape.checks),
                review_manifest=review,review_display=dict(width=512,height=384,art_offset=[96,16],text_top=256))
    (BUILD / "fixture.json").write_text(json.dumps(meta,indent=2)+"\n")
    subprocess.run([str(PROJECT / "docs/tasks/GOLEM-007/build/ez80asm"),"guest.asm","build/probe.bin","-l","-c"],cwd=TASK,check=True)
    assert (BUILD / "probe.bin").stat().st_size < 0x40000, "fixture exceeds physical user RAM window"
    return meta


def identity():
    paths = [PROFILE / p for p in ("fab-agon-emulator","fab-agon-emulator.bin","firmware/vdp_platform.so","mos_platform.bin","mos_platform.map")]
    paths += [BUILD / "probe.bin",BUILD / "tape.bin",PROJECT / "build/golemc",
              TASK / "qualify.py",TASK / "guest.asm",TASK / "agnb_api.inc",TASK / "agnb_support.inc",
              TASK / "bitmaps.golem",TASK / "review.golem",TASK / "assets/wolf-review.agnb"]
    paths += sorted((PROJECT / "src").glob("*.hpp")) + [PROJECT / "src/golemc.cpp"]
    return {str(p.relative_to(PROJECT)):sha(p) for p in paths}


def execute(meta):
    assert not (PROFILE / ".bespoke-vdp-profile").exists()
    assert not (PROFILE / "fab-agon-emulator").is_symlink()
    assert "Generated by agon-dev-env setup_emulator.py" in (PROFILE / "fab-agon-emulator").read_text()
    runtime = (PROFILE / "fab-agon-emulator.bin").resolve()
    for p in Path("/proc").glob("[0-9]*"):
        try:
            if (p / "exe").resolve() == runtime: raise RuntimeError("Close the emulator before qualification")
        except (FileNotFoundError,PermissionError): pass
    APP.mkdir(exist_ok=True)
    shutil.copyfile(BUILD / "probe.bin",APP / "probe.bin")
    shutil.copyfile(TASK / "assets/wolf-review.agnb",APP / "wolf-review.agnb")
    result = APP / "results.dat"
    result.unlink(missing_ok=True)
    startup = PROFILE / "sdcard/autoexec.txt"
    previous = startup.read_bytes() if startup.exists() else None
    before = identity()
    accepted = json.loads((TASK.parent / "GOLEM-018/emulator-checks.json").read_text())["runtime_identity"]
    for p,h in before.items():
        if p.startswith(".emulator/"): assert accepted[p] == h, f"stock runtime drift: {p}"
    env = {k:v for k,v in os.environ.items() if k not in ("LD_PRELOAD","BASH_ENV")}
    env.update(SDL_VIDEODRIVER="dummy",SDL_AUDIODRIVER="dummy")
    started = time.monotonic()
    process = None
    mapped = set()
    actual_exe = None
    try:
        startup.write_bytes(b"SET KEYBOARD 1\r\ncd /golem-019\r\nload probe.bin\r\nrun .\r\n")
        with (BUILD / "emulator.log").open("w") as log:
            process = subprocess.Popen(["./fab-agon-emulator","--renderer","sw"],cwd=PROFILE,env=env,
                                       stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
            while not result.exists() or result.stat().st_size < meta["observation_bytes"]:
                if process.poll() is not None: raise RuntimeError("Emulator exited before results; inspect build/emulator.log")
                maps = Path(f"/proc/{process.pid}/maps").read_text()
                mapped.update(Path(line.split(maxsplit=5)[5]).resolve() for line in maps.splitlines() if "/vdp_" in line and ".so" in line)
                if mapped:
                    assert mapped == {(PROFILE / "firmware/vdp_platform.so").resolve()}
                    actual_exe = Path(f"/proc/{process.pid}/exe").resolve()
                    assert actual_exe == runtime
                if time.monotonic()-started > 180: raise TimeoutError("Inspect build/emulator.log")
                time.sleep(.05)
    finally:
        if process and process.poll() is None:
            os.killpg(process.pid,signal.SIGTERM)
            try: process.wait(timeout=5)
            except subprocess.TimeoutExpired: os.killpg(process.pid,signal.SIGKILL);process.wait()
        if previous is None: startup.unlink(missing_ok=True)
        else: startup.write_bytes(previous)
    assert mapped and actual_exe and identity() == before
    actual = result.read_bytes()
    transport.BUILD = BUILD
    failures = transport.validate(meta,actual)
    report = dict(pass_headless=not failures,checks=len(meta["checks"]),pixel_pairs=meta["pixel_pairs"],
                  pixel_checks=meta["pixel_checks"],observed_bytes=len(actual),execution_seconds=round(time.monotonic()-started,3),
                  fab_tag="1.2.5",mos_release="v3.0.2 Arthur",architecture=os.uname().machine,
                  runtime_identity=before,loaded_vdp=str(next(iter(mapped))),executed_fab=str(actual_exe),
                  fixture=meta,failures=failures,human_validation="pending",hardware_qualified=False,
                  scope="Stock AGNB loader, model/direct/resident pixels, persistent inputs and asset preservation.")
    (TASK / "observed.dat").write_bytes(actual)
    if not failures:
        (TASK / "expected.dat").write_bytes(actual)
        report["expected_sha256"] = sha(TASK / "expected.dat")
    (TASK / "emulator-checks.json").write_text(json.dumps(report,indent=2)+"\n")
    if failures: raise AssertionError(f"{len(failures)} failed checks: {failures[:3]}")
    print(f'Stock Wolf bitmaps pass: {len(meta["checks"])} checks, {len(actual)} bytes, {report["execution_seconds"]}s')


def main():
    subprocess.run(["make","-C",str(PROJECT / "src")],check=True)
    execute(generate())


if __name__ == "__main__":
    main()
