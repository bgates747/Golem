"""GOLEM-005 host checks; preserves the GOLEM-003/004 historical evidence."""
from pathlib import Path
import hashlib
import json
import resource
import subprocess
import time

ROOT = Path(__file__).resolve().parent
PROJECT = ROOT.parents[2]
BUILD = ROOT / "build"
BASELINE = "6a2017c26968be2631c955f67508d9e3ad0d16d7"
FLAGS = ["-std=c++17", "-Wall", "-Wextra", "-Werror"]


def run(command, **kwargs):
    start = time.monotonic()
    result = subprocess.run(command, capture_output=True, text=True, **kwargs)
    assert result.returncode == 0, (command, result.stdout, result.stderr)
    return {"command": command, "stdout": result.stdout.strip(),
            "seconds": round(time.monotonic() - start, 3)}


def hashes(paths):
    return {str(p.relative_to(PROJECT)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(paths)}


def main():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    BUILD.mkdir(exist_ok=True)
    frozen = [p for p in (PROJECT / "docs/tasks/GOLEM-003").rglob("*")
              if p.is_file() and "build" not in p.relative_to(PROJECT).parts]
    before = hashes(frozen)
    old = subprocess.check_output(["git", "show", f"{BASELINE}:src/golemc.cpp"], cwd=PROJECT)
    (BUILD / "legacy.cpp").write_bytes(old)
    checks = {"baseline_commit": BASELINE, "builds": {}, "cli_checks": []}
    for name, source, flags in [
        ("parsers", ROOT / "test_parsers.cpp", ["-O1", "-g", "-fsanitize=address,undefined"]),
        ("ordinary", PROJECT / "src/golemc.cpp", ["-O2"]),
        ("scalar", ROOT / "golemc_scalar.cpp", ["-O2"]),
        ("legacy", BUILD / "legacy.cpp", ["-O2"]),
        ("legacy-scalar", PROJECT / "docs/tasks/GOLEM-003/golemc_scalar.cpp", ["-O2"]),
    ]:
        print(f"Building {name}", flush=True)
        checks["builds"][name] = run(["g++", *FLAGS, *flags, str(source), "-o", str(BUILD / name)])
        if name == "parsers":
            checks["parser_suite"] = run([str(BUILD / name)])
            print(checks["parser_suite"]["stdout"], flush=True)

    ordinary = BUILD / "ordinary.golem"
    ordinary.write_text('uint16 a=1 uint16 b b=a+\n2 print "ok;"')
    output = BUILD / "ordinary.bin"
    run([str(BUILD / "ordinary"), str(ordinary), str(output)])
    preserved = output.read_bytes()
    for source, message in [
        ('uint16 a=999999999999999999999999999999', 'does not fit'),
        ('uint16 a=1foo', 'malformed numeric literal'),
        ('uint16 a a=1+\nprint "foo"', 'line 2, column 1'),
        ('print "foo";', "remove ';'"),
        ('uint16 If', 'reserved identifier'),
        ('uint16 a=1 b=2', 'undeclared variable'),
    ]:
        ordinary.write_text(source)
        result = subprocess.run([str(BUILD / "ordinary"), str(ordinary), str(output)], capture_output=True, text=True)
        assert result.returncode == 1 and message in result.stderr, result
        assert output.read_bytes() == preserved
        checks["cli_checks"].append({"compiler": "ordinary", "source": source,
                                     "returncode": result.returncode, "stderr": result.stderr.strip()})

    old_output, next_output = BUILD / "old-scalar-output", BUILD / "scalar-output"
    run([str(BUILD / "legacy-scalar"), str(PROJECT / "docs/tasks/GOLEM-003/examples/floor.golem"),
         str(old_output), "-0.75"])
    run([str(BUILD / "scalar"), str(ROOT / "examples/floor.golem"), str(next_output), "-0.75"])
    for name in ("bootstrap.bin", "cleanup.bin", "invoke.bin", "update-call.bin"):
        assert (old_output / name).read_bytes() == (next_output / name).read_bytes(), name
    old_manifest = json.loads((old_output / "interface.json").read_text())
    next_manifest = json.loads((next_output / "interface.json").read_text())
    assert old_manifest.pop("experimental_task") == "GOLEM-003"
    assert next_manifest.pop("experimental_task") == "GOLEM-005"
    assert old_manifest == next_manifest
    saved = {p.name: p.read_bytes() for p in next_output.iterdir()}
    invalid = BUILD / "invalid-scalar.golem"
    valid = (ROOT / "examples/floor.golem").read_text()
    invalid.write_text(valid.replace('-0.00006103515625', '1e9999'))
    for source, value in [(invalid, "-0.75"), (ROOT / "examples/floor.golem", "1e-9999"),
                          (ROOT / "examples/floor.golem", "32768"), (ROOT / "examples/floor.golem", "NaN")]:
        result = subprocess.run([str(BUILD / "scalar"), str(source), str(next_output), value], capture_output=True, text=True)
        assert result.returncode == 1, result
        assert {p.name: p.read_bytes() for p in next_output.iterdir()} == saved
        checks["cli_checks"].append({"compiler": "scalar", "value": value,
                                     "returncode": result.returncode, "stderr": result.stderr.strip()})
    # Migrations use independently preserved legacy bytes, including actual
    # maintained examples from the pre-tranche Git revision.
    checks["example_matches"] = []
    for relative_name in ("examples/hello_golem/hello.golem", "examples/loop_golem/loop.golem"):
        source = PROJECT / relative_name
        relative = source.relative_to(PROJECT)
        original = subprocess.check_output(["git", "show", f"{BASELINE}:{relative}"], cwd=PROJECT)
        old_source = BUILD / "legacy-example.golem"
        old_source.write_bytes(original)
        run([str(BUILD / "legacy"), str(old_source), str(BUILD / "old-example.bin")])
        run([str(BUILD / "ordinary"), str(source), str(BUILD / "new-example.bin")])
        assert (BUILD / "old-example.bin").read_bytes() == (BUILD / "new-example.bin").read_bytes(), relative
        checks["example_matches"].append(str(relative))
    assert before == hashes(frozen), "Historical GOLEM-003 files changed"
    sources = [PROJECT / "src/golemc.cpp", PROJECT / "src/token_reader.hpp",
               *ROOT.glob("*.cpp"), *ROOT.glob("*.hpp"), ROOT / "run_checks.py", ROOT / "examples/floor.golem"]
    checks["source_sha256"] = hashes(sources)
    checks["historical_prototype_sha256"] = before
    checks["legacy_source_sha256"] = hashlib.sha256(old).hexdigest()
    checks["output_preserved_on_rejection"] = True
    checks["firmware_executed"] = False
    (ROOT / "host-checks.json").write_text(json.dumps(checks, indent=2) + "\n")
    print("Host checks passed; task-local host-checks.json written.", flush=True)


if __name__ == "__main__":
    main()
