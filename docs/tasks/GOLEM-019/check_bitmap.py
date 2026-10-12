"""Host qualification for the plain bitmap compiler and assembly interface."""

import hashlib
import json
import subprocess
import time
from pathlib import Path

TASK = Path(__file__).resolve().parent
PROJECT = TASK.parents[2]
BUILD = TASK / "build"
ARTIFACTS = ("bootstrap.bin", "invoke.bin", "update-call.bin", "cleanup.bin", "interface.json", "golem.inc")
RESERVE = ("--reserve", "8195:8195", "--reserve", "8201:8202", "--reserve", "8463:8463")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    BUILD.mkdir(exist_ok=True)
    started = time.monotonic()
    commands = []

    def run(args, cwd=PROJECT, ok=True):
        tick = time.monotonic()
        p = subprocess.run(list(map(str, args)), cwd=cwd, capture_output=True, text=True)
        commands.append(dict(args=list(map(str, args)), seconds=round(time.monotonic()-tick, 3)))
        if ok and p.returncode:
            raise RuntimeError(p.stdout + p.stderr)
        return p

    run(["make", "-C", "src", "check", "CXXFLAGS=-std=c++17 -Wall -Wextra -Werror -O2"])
    compiler = PROJECT / "build/golemc"
    sanitizer = BUILD / "test-sanitized"
    run(["g++", "-std=c++17", "-Wall", "-Wextra", "-Werror", "-O1", "-g",
         "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
         PROJECT / "tests/test_compiler.cpp", "-o", sanitizer])
    run([sanitizer])

    # Preserve all six previously accepted geometry artifacts, including JSON/asm.
    geometry = json.loads((TASK / "geometry-before.json").read_text())
    for name, options in (("geometry", ()), ("relocated", ("--reserve", "1:254", "--reserve", "260:264"))):
        pkg = BUILD / f"after-{name}"
        run([compiler, "--resident", "--emit-asm", *options,
             PROJECT / "docs/tasks/GOLEM-018/geometry.golem", pkg])
        assert {a:sha(pkg/a) for a in ARTIFACTS} == geometry[name], name

    # Exercise CLI backend selection for every prior scalar/table family.
    prior = []
    for source in ("examples/resident/floor.golem", "examples/resident/counted-floor.golem",
                   "docs/tasks/GOLEM-013/checked-subtract.golem",
                   "docs/tasks/GOLEM-014/checked-multiply.golem",
                   "docs/tasks/GOLEM-015/checked-divmod.golem", "docs/tasks/GOLEM-017/table.golem"):
        options = ["--table-data", f"samples={PROJECT}/docs/tasks/GOLEM-017/records.dat"] if source.endswith("table.golem") else []
        pkg = BUILD / f"prior-{len(prior)}"
        run([compiler, "--resident", "--emit-asm", *options, PROJECT/source, pkg])
        manifest = json.loads((pkg / "interface.json").read_text())
        assert manifest["profile"] != "resident-drawing", source
        prior.append(source)

    # Assemble real consumers, including word IDs/offsets and the named wrapper.
    assembler = PROJECT / "docs/tasks/GOLEM-007/build/ez80asm"
    cases = []
    for name, extra in (("default", ()), ("relocated", ("--reserve", "1:254", "--reserve", "260:264"))):
        pkg = BUILD / name
        run([compiler, "--resident", "--emit-asm", *RESERVE, *extra, TASK/"bitmaps.golem", pkg])
        m = json.loads((pkg / "interface.json").read_text())
        borrowed = {r["buffer_id"] for r in json.loads((TASK / "assets.json").read_text())["assets"]}
        assert set(m["initial_bitmap_ids"]) == borrowed
        assert borrowed.isdisjoint(r["id"] for r in m["owned"])
        assert m["call_bytes"] == 6 and m["input_update_bytes"] + 6 == 23
        entry = "golem_" + m["entry"]["name"]
        symbols = [entry, entry+"_call", entry+"_call_len", "golem_package_bootstrap", "golem_package_bootstrap_len",
                   "golem_package_update_call", "golem_package_update_call_len", "golem_package_cleanup", "golem_package_cleanup_len"]
        expected = []
        for g in m["globals"]:
            symbols += ["golem_"+g["name"]+suffix for suffix in ("_buffer", "_offset", "_bytes", "_type")]
            expected += [g["id"], g["offset"], g["bytes"], 1 if g["type"] == "uint16" else 2]
        source = BUILD / f"{name}-consumer.asm"
        binary = source.with_suffix(".bin")
        source.write_text(".assume adl=1\n.org $40000\n" + "".join("dl "+s+"\n" for s in symbols)
                          + f'include "{name}/golem.inc"\n')
        run([assembler, source.name, binary.name, "-c"], cwd=BUILD)
        data = binary.read_bytes()
        values = [int.from_bytes(data[i*3:i*3+3], "little") for i in range(len(symbols))]
        assert values[9:] == expected
        offset = values[0] - 0x40000
        assert data[offset:offset+11] == b"\x21"+values[1].to_bytes(3,"little")+b"\x01\x06\0\0\x5b\xdf\xc9"
        for i, artifact in zip((1,3,5,7), ("invoke.bin","bootstrap.bin","update-call.bin","cleanup.bin")):
            assert data[values[i]-0x40000:values[i]-0x40000+values[i+1]] == (pkg/artifact).read_bytes()
        cases.append(dict(name=name, installation=m["bootstrap_bytes"], input_bytes=m["input"]["bytes"],
                          call_bytes=m["call_bytes"], owned=len(m["owned"])))

    # Missing ownership and bad field contracts reject before publication.
    bad = BUILD / "bad.golem"
    keep = BUILD / "preserve"
    keep.mkdir(exist_ok=True)
    original = (TASK / "bitmaps.golem").read_text()
    for text, options in ((original, ()), (original.replace("Bounds(1,65534)", "Bounds(0,65535)"), RESERVE),
                          (original.replace("Sub drawWolf", "Sub package_bootstrap"), RESERVE)):
        bad.write_text(text)
        for a in ARTIFACTS:
            (keep/a).write_bytes(b"preserve")
        p = run([compiler, "--resident", "--emit-asm", *options, bad, keep], ok=False)
        assert p.returncode == 1 and all((keep/a).read_bytes() == b"preserve" for a in ARTIFACTS), p.stderr
    report = dict(passed=True, seconds=round(time.monotonic()-started,3), prior_cli_profiles=prior,
                  geometry_artifacts_unchanged=12, assembler_consumers=cases,
                  prepublication_rejections=3, sanitizers="address,undefined", commands=commands,
                  source_sha256={str(p.relative_to(PROJECT)):sha(p) for p in (
                      PROJECT/"src/resident_drawing.hpp", PROJECT/"src/assembly_output.hpp",
                      PROJECT/"tests/test_compiler.cpp", TASK/"bitmaps.golem", TASK/"check_bitmap.py")})
    (TASK / "bitmap-host-checks.json").write_text(json.dumps(report,indent=2)+"\n")
    print(f'Bitmap host checks pass: {len(cases)} assembler consumers; {report["seconds"]}s. Emulator qualification remains separate.')


if __name__ == "__main__":
    main()
