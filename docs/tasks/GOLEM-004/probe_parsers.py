"""Observe existing parsers only; no proposed grammar is implemented here."""
from pathlib import Path
import hashlib
import json
import resource
import subprocess
import time

ROOT = Path(__file__).resolve().parent
PROJECT = ROOT.parents[2]
SOURCES = [PROJECT / "src/golemc.cpp", *sorted((PROJECT / "docs/tasks/GOLEM-003").glob("*.hpp")),
           PROJECT / "docs/tasks/GOLEM-003/golemc_scalar.cpp"]


def fingerprints():
    return {str(p.relative_to(PROJECT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in SOURCES}


def main():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    build = ROOT / "build"
    build.mkdir(exist_ok=True)
    before = fingerprints()
    results = {"baseline_commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=PROJECT).decode().strip(),
               "compiler_builds": {}, "observations": []}
    flags = ["-std=c++17", "-Wall", "-Wextra", "-Werror", "-O2"]
    for name, source in [("main", SOURCES[0]), ("scalar", SOURCES[-1])]:
        command = ["g++", *flags, str(source), "-o", str(build / name)]
        start = time.monotonic()
        subprocess.run(command, check=True)
        results["compiler_builds"][name] = {"command": command,
                                             "seconds": round(time.monotonic() - start, 3)}

    scalar = "float32 x Range(-1,1)=0.25; sint16 result; Sub compute; result=sint16(floor(x)); EndSub;"
    cases = [
        ("main", "legacy_adjacent_prints", 'print "foo"; print "bar";', True),
        ("main", "terminator_free_adjacent_prints", 'print "foo" print "bar"', False),
        ("main", "terminator_free_multiline_prints", 'print "foo"\nprint "bar"', False),
        ("main", "legacy_multiline_addition", 'uint16 a=1; uint16 b; b=a+\n2;', True),
        ("main", "terminator_free_declarations_assignments", 'uint16 a=1 uint16 b b=a+2', False),
        ("main", "legacy_for", 'uint16 i; For i=1 To 2; print "tick"; Next i;', True),
        ("main", "terminator_free_for", 'uint16 i For i=1 To 2 print "tick" Next i', False),
        ("main", "future_keyword_identifier", 'uint16 If=1;', True),
        ("main", "range_identifier", 'uint16 Range=1;', True),
        ("main", "loop_max_endpoint_compile_only", 'uint16 i; For i=65535 To 65535; print "tick"; Next i;', True),
        ("main", "missing_operand", 'uint16 a=1; a=1+; print "tick";', False),
        ("main", "huge_literal_diagnostic", 'uint16 a=999999999999999999999999999999;', False),
        ("scalar", "legacy_scalar_bounds", scalar, True),
        ("scalar", "terminator_free_scalar", scalar.replace(';', ''), False),
        ("scalar", "future_keyword_identifier", scalar.replace(' x ', ' If ').replace('floor(x)', 'floor(If)'), True),
        ("scalar", "bounds_not_sequence", scalar.replace('0.25', '0.75'), True),
        ("scalar", "alternate_bounds_name", scalar.replace('Range', 'Bounds'), False),
        ("scalar", "stepped_decl_bounds", scalar.replace('Range(-1,1)', 'Range(-1,1,1)'), False),
    ]
    for compiler, name, text, accepted in cases:
        case = build / "cases" / compiler / name
        case.mkdir(parents=True, exist_ok=True)
        source = case / "source.golem"
        source.write_text(text)
        output = case / "output"
        sentinel = b"preserved output"
        if compiler == "main":
            output.write_bytes(sentinel)
        else:
            output.mkdir(exist_ok=True)
            (output / "bootstrap.bin").write_bytes(sentinel)
        command = [str(build / compiler), str(source), str(output)]
        result = subprocess.run(command, capture_output=True, text=True)
        assert (result.returncode == 0) == accepted, (name, result.stderr)
        if not accepted:
            artifact = output if compiler == "main" else output / "bootstrap.bin"
            assert artifact.read_bytes() == sentinel, name
        results["observations"].append({"compiler": compiler, "name": name, "source": text,
                                         "accepted": accepted, "returncode": result.returncode,
                                         "stdout": result.stdout.strip(), "stderr": result.stderr.strip()})
    assert fingerprints() == before, "Reviewed compiler source changed during probes"
    results["source_sha256"] = before
    results["firmware_executed"] = False
    (ROOT / "parser-observations.json").write_text(json.dumps(results, indent=2) + "\n")
    print(f"{len(cases)} existing-parser observations recorded; compiler source unchanged.")


if __name__ == "__main__":
    main()
