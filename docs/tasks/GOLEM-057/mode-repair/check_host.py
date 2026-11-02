#!/usr/bin/env python3
"""Compile real frozen MODE/helper/controller functions against ownership fakes."""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]
DEFAULT_BASE = REPO / "docs/tasks/GOLEM-057/build/display-mode-repair"
ORIGINAL = Path("/Agon/mystuff/agon-vdplet/build/vdpl-010/pixels/firmware-src")


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def extract(text: str, signature: str) -> str:
    start = text.index(signature)
    brace = text.index("{", start)
    depth = 0
    # Ignore comment/string braces while retaining exact source text.
    token = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]', re.S)
    for match in token.finditer(text, brace):
        if match.group() == "{":
            depth += 1
        elif match.group() == "}":
            depth -= 1
            if depth == 0:
                return text[start:match.end()]
    raise ValueError(f"unterminated function: {signature}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate", type=Path, default=DEFAULT_BASE / "candidate")
    parser.add_argument("--control", type=Path, default=DEFAULT_BASE / "control00")
    parser.add_argument("--original", type=Path, default=ORIGINAL)
    parser.add_argument("--output", type=Path, default=DEFAULT_BASE / "host")
    parser.add_argument("--cxx", default=os.environ.get("CXX", "c++"))
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    roots = {"original": args.original, "control": args.control, "candidate": args.candidate}
    sources = {}
    pins = {}
    for name, root in roots.items():
        vdu_bytes = (root / "video/vdu.h").read_bytes()
        screen_bytes = (root / "video/agon_screen.h").read_bytes()
        vdu = vdu_bytes.decode()
        mode = extract(vdu, "void VDUStreamProcessor::vdu_mode(uint8_t mode)")
        update = extract(screen_bytes.decode(), "bool updateVGAController(uint8_t colours)")
        helper = extract(vdu, "static void releaseFailedResolution()") if name == "candidate" else ""
        sources[name] = dict(mode=mode, update=update, helper=helper)
        pins[name] = {"root": str(root.resolve()), "vdu_sha256": sha(vdu_bytes),
                      "agon_screen_sha256": sha(screen_bytes),
                      "extracted_mode_sha256": sha(mode.encode()),
                      "extracted_update_sha256": sha(update.encode()),
                      "extracted_helper_sha256": sha(helper.encode()) if helper else None}
    if sources["original"] != sources["control"]:
        raise SystemExit("original/control extracted functions differ")
    if sources["candidate"]["update"] != sources["original"]["update"]:
        raise SystemExit("candidate unexpectedly changes controller replacement")
    unchanged = re.sub(r'(?m)^[ \t]*if \(errVal == 2\) releaseFailedResolution\(\);\n',
                       '', sources["candidate"]["mode"])
    if unchanged != sources["original"]["mode"]:
        raise SystemExit("candidate MODE changes extend beyond the two release conditions")
    results = {}
    compiler = subprocess.check_output([args.cxx, "--version"], text=True).splitlines()[0]
    for arm in ("original", "control", "candidate", "candidate-sanitized"):
        name = "candidate" if arm.startswith("candidate") else arm
        directory = args.output / arm
        directory.mkdir(exist_ok=True)
        parts = sources[name]
        header = directory / "functions_under_test.hpp"
        header.write_text(parts["update"] + "\n" + parts["helper"] + "\n" + parts["mode"] + "\n")
        executable = directory / "check_release"
        command = [args.cxx, "-std=c++17", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                   f"-DREPAIR_ENABLED={int(name == 'candidate')}", "-I", str(directory),
                   str(HERE / "check_release.cpp"), "-o", str(executable)]
        if arm.endswith("sanitized"):
            command += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
        build = subprocess.run(command, capture_output=True, text=True)
        (directory / "build.log").write_text(build.stdout + build.stderr)
        if build.returncode:
            raise SystemExit(f"{arm} compile failed: {build.stderr}")
        env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0:halt_on_error=1",
                   UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        run = subprocess.run([str(executable)], capture_output=True, text=True, env=env)
        (directory / "stdout.txt").write_text(run.stdout)
        (directory / "stderr.txt").write_text(run.stderr)
        if run.returncode:
            raise SystemExit(f"{arm} ownership checks failed ({run.returncode}): {run.stderr}")
        lines = run.stdout.splitlines()
        cases = dict(line.split(":", 1) for line in lines if not line.startswith("checks="))
        checks = int(lines[-1].removeprefix("checks="))
        results[arm] = dict(checks=checks, cases=cases, command=command,
                            executable_sha256=sha(executable.read_bytes()),
                            stdout_sha256=sha(run.stdout.encode()), stderr_sha256=sha(run.stderr.encode()))
    if results["original"]["cases"] != results["control"]["cases"]:
        raise SystemExit("original/control behavior differs")
    invariant_cases = ("success", "invalid_mode", "teletext_refusal", "factory_error", "nonheight_second_error")
    for case in invariant_cases:
        if results["candidate"]["cases"][case] != results["control"]["cases"][case]:
            raise SystemExit(f"candidate changed untouched case: {case}")
    if results["candidate"]["cases"] != results["candidate-sanitized"]["cases"]:
        raise SystemExit("sanitizer build changed ownership behavior")
    report = {"schema": "g057-mode-release-host-v1", "passed": True,
              "compiler": compiler, "source_pins": pins,
              "test_pins": {p.name: sha(p.read_bytes()) for p in (Path(__file__), HERE / "check_release.cpp")},
              "original_control_match": True, "unchanged_cases": list(invariant_cases),
              "sanitizer": "ASan+UBSan; detect_leaks=0; halt_on_error=1",
              "limitation": "Ownership fakes test extracted control flow and lifetime ordering; no real allocation, firmware, ISR, emulator or physical proof.",
              "runs": {arm: {**result, "cases": list(result["cases"])} for arm, result in results.items()}}
    destination = args.output / "host-results.json"
    destination.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"passed": True, "cases_per_run": len(results["candidate"]["cases"]),
                      "assertions": {arm: result["checks"] for arm, result in results.items()},
                      "report": str(destination)}, indent=2))


if __name__ == "__main__":
    main()
