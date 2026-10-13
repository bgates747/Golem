#!/usr/bin/env python3
"""Qualify raw IRAM replacement and the owned ISYNC coherence observer."""

import argparse
import datetime
import hashlib
import json
from pathlib import Path
import platform
import subprocess
import time

HERE = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--core", type=Path, default=HERE.parent / "emulator/build/core")
    parser.add_argument("--headers", type=Path, default=HERE.parent / "emulator/build/flexe-source/src")
    parser.add_argument("--output", type=Path, default=HERE / "build/iram_reload_qualified")
    parser.add_argument("--receipt", type=Path, default=HERE / "iram-qualified-receipt.json")
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    source = HERE / "iram_reload.c"
    command = ["cc", "-Wall", "-Wextra", "-Werror", "-O2", "-DWITH_ISYNC_OBSERVER",
               "-I", str(args.headers), str(source), str(args.core / "libxtensa-emu-lib.a"),
               "-o", str(args.output), "-lm", "-lz", "-lpthread", "-lssl", "-lcrypto"]
    subprocess.run(command, check=True)
    receipt = {
        "source_commit": "fbda8e905ccff36a090e4e33b0ffc2337dd79c52",
        "host": {"hostname": platform.node(), "machine": platform.machine(), "platform": platform.platform()},
        "build_command": command,
        "files": {str(path.resolve()): hashlib.sha256(path.read_bytes()).hexdigest()
                  for path in (source, args.output, args.core / "libxtensa-emu-lib.a",
                               args.headers / "xtensa.c", args.headers / "xtensa.h", args.headers / "jit.c", args.headers / "peripherals.c")},
        "runs": [],
        "limits": ["Synthetic CPU/translation tests; actual vdplet load/call/reload and physical ESP32 proof are separate.",
                   "Observer fixture shadows the changed eight-byte region; the adapter separately shadows its configured IRAM range."],
    }
    for mode in ([], ["--instruction-sync"]):
        command = [str(args.output), *mode]
        started = time.monotonic()
        result = subprocess.run(command, text=True, capture_output=True, timeout=30)
        rows = [json.loads(line) for line in result.stdout.splitlines()]
        receipt["runs"].append({"command": command, "utc": datetime.datetime.now(datetime.UTC).isoformat(),
                                "wall_seconds": time.monotonic() - started, "returncode": result.returncode,
                                "cases": rows, "stderr": result.stderr})
        receipt["pass"] = result.returncode == 0 and len(rows) == (8 if mode else 6) and all(row["pass"] for row in rows)
        args.receipt.write_text(json.dumps(receipt, indent=2) + "\n")
        if not receipt["pass"]:
            raise SystemExit(result.stdout + result.stderr)
    print(f"IRAM qualification passes {sum(len(run['cases']) for run in receipt['runs'])} cases.")


if __name__ == "__main__":
    main()
