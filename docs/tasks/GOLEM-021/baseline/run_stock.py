#!/usr/bin/env python3
"""Run unmodified embedded upstream gates with the project Python interpreter."""

import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import time


HERE = Path(__file__).resolve().parent
PROJECT = HERE.parents[3]
PIN = "fbda8e905ccff36a090e4e33b0ffc2337dd79c52"


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=HERE / "build/flexe-source")
    parser.add_argument("--runner", type=Path, default=HERE / "build/cmake/flexe-agon-vdp-test")
    parser.add_argument("--vdp", type=Path, required=True)
    parser.add_argument("--mos", type=Path)
    parser.add_argument("--basic", type=Path)
    parser.add_argument("--peer", type=Path)
    parser.add_argument("--gate", choices=("all", "vdp", "mos"), default="all")
    parser.add_argument("--artifacts", type=Path, default=HERE / "build/artifacts")
    parser.add_argument("--receipt", type=Path, default=HERE / "stock-receipt.json")
    args = parser.parse_args()
    venv_python = PROJECT / ".venv/bin/python"
    if not venv_python.is_file():
        parser.error(f"missing project Python: {venv_python}")
    if args.mos or args.basic or args.peer:
        if not all((args.mos, args.basic, args.peer)):
            parser.error("--mos, --basic and --peer must be supplied together")
    if args.gate == "mos" and not args.peer:
        parser.error("--gate mos requires --mos, --basic and --peer")

    args.artifacts.mkdir(parents=True, exist_ok=True)
    receipt = {
        "schema": 1,
        "source_commit": PIN,
        "source_export": str(args.source.resolve()),
        "host": {"hostname": platform.node(), "platform": platform.platform(), "machine": platform.machine()},
        "images": {"vdp": {"path": str(args.vdp.resolve()), "sha256": sha(args.vdp)}},
        "runner": {"path": str(args.runner.resolve()), "sha256": sha(args.runner)},
        "core_source_files": {
            name: sha(args.source / name)
            for name in ("src/jit.c", "src/xtensa.c", "src/xtensa.h", "src/peripherals.c")
        },
        "python": str(venv_python),
        "started_utc": datetime.datetime.now(datetime.UTC).isoformat(),
        "gates": [],
        "gates_requested": args.gate,
        "limits": [
            "Automated headless emulation, not physical ESP32 proof or human visual/keyboard review.",
            "Stock Agon gate uses serial-console input; physical PS/2 is not covered.",
            "Retained timings are reproducibility receipts, not comparative performance claims.",
        ],
    }
    env = os.environ.copy()
    env["AGON_VDP_BIN"] = str(args.vdp.resolve())
    env["SOAK_CYCLES"] = "240000000"
    env["WARMUPS"] = "0"
    env["REPS"] = "1"
    gates = [] if args.gate == "mos" else [("vdp", "check-agon-vdp.sh", [str(args.runner.resolve()), str(args.vdp.resolve())])]
    if args.peer:
        for name, path in (("mos", args.mos), ("basic", args.basic)):
            receipt["images"][name] = {"path": str(path.resolve()), "sha256": sha(path)}
        receipt["peer"] = {"path": str(args.peer.resolve()), "sha256": sha(args.peer)}
        env["AGON_MOS_BIN"] = str(args.mos.resolve())
        env["AGON_BBC_BIN"] = str(args.basic.resolve())
        if args.gate != "vdp":
            gates.append(("mos", "check-agon-mos.sh", [str(args.runner.resolve()), str(args.peer.resolve())]))

    for name, filename, parameters in gates:
        script = args.source / "scripts" / filename
        text = script.read_text()
        marker = "<<'PY'\n"
        if text.count(marker) != 1 or not text.endswith("\nPY\n"):
            raise RuntimeError(f"unexpected upstream embedded-Python structure: {script}")
        code = text.split(marker, 1)[1][:-3]
        directory = args.artifacts / name
        env["ARTIFACTS"] = str(directory.resolve())
        command = [str(venv_python), "-", *parameters]
        started = datetime.datetime.now(datetime.UTC).isoformat()
        clock = time.monotonic()
        result = subprocess.run(command, input=code, text=True, capture_output=True, env=env, timeout=1300)
        output = result.stdout + result.stderr
        log = args.artifacts / f"{name}-gate.log"
        log.write_text(output)
        receipt["gates"].append({
            "name": name, "upstream_script": str(script.resolve()), "script_sha256": sha(script),
            "embedded_python_sha256": hashlib.sha256(code.encode()).hexdigest(),
            "command": command, "started_utc": started,
            "wall_seconds": time.monotonic() - clock, "returncode": result.returncode,
            "artifacts": str(directory.resolve()), "output": output,
        })
        print(output, end="", flush=True)
        receipt["ended_utc"] = datetime.datetime.now(datetime.UTC).isoformat()
        receipt["pass"] = all(item["returncode"] == 0 for item in receipt["gates"])
        args.receipt.write_text(json.dumps(receipt, indent=2) + "\n")
        if result.returncode:
            return result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
