#!/usr/bin/env python3
"""Compile/link C++ on eZ80, and check host C/C++ packing and relocated entries."""
from pathlib import Path
import hashlib
import json
import os
import shutil
import subprocess
import generate_header

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
TOOLS = Path("/Agon/agondev/release/bin")
OUT = HERE / "build/abi"

HOST_TEST = r'''
#include "flower_golem.h"
#include <assert.h>
static uint16_t submitted;
void golem_host_call(uint16_t entry_buffer) { submitted=entry_buffer; }
int main(void) {
    uint8_t state[GOLEM_STATE_BYTES];memset(state,0xa5,sizeof state);
    golem_set_theta_init(state,22.5f);
    const uint8_t *p=state+GOLEM_theta_init_OFFSET;
    assert(p[0]==0 && p[1]==0 && p[2]==0xb4 && p[3]==0x41);
    assert(p[-1]==0xa5 && p[4]==0xa5);
    golem_set_batches(state,65535u);
    p=state+GOLEM_batches_OFFSET;assert(p[0]==0xff && p[1]==0xff);
    assert(p[-1]==0x41);
    golem_step(GOLEM_BUFFERED);assert(submitted==GOLEM_step_BUFFERED);
    golem_step(GOLEM_NATIVE);assert(submitted==GOLEM_step_NATIVE);
    golem_reset(GOLEM_BUFFERED);assert(submitted==GOLEM_reset_BUFFERED);
    golem_reset(GOLEM_NATIVE);assert(submitted==GOLEM_reset_NATIVE);
    return 0;
}
'''


def run(args, **kwargs):
    return subprocess.run([str(arg) for arg in args], check=True, capture_output=True, text=True, **kwargs)


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    evidence = HERE / "evidence"
    evidence.mkdir(exist_ok=True)
    environment = dict(os.environ, PATH=str(TOOLS) + os.pathsep + os.environ["PATH"])
    results = []
    for variant, extra in [("default", []), ("relocated", ["--reserve", "1:4095"])]:
        package = OUT / variant
        run([ROOT / "build/golemc", "--portable", *extra, HERE / "abi_probe.golem", package])
        header = package / "flower_golem.h"
        generate_header.generate(package, header)
        (package / "check.c").write_text(HOST_TEST)
        for compiler, language in [("cc", "c"), ("c++", "c++")]:
            binary = package / ("check-" + language)
            run([compiler, "-x", language, "-Wall", "-Wextra", "-Werror", "-fsanitize=address,undefined",
                 "-I", package, package / "check.c", "-o", binary])
            run([binary])
        target_header = HERE / "client/include/generated/flower_golem.h"
        target_header.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(header, target_header)
        run(["make", "clean"], cwd=HERE / "client", env=environment)
        build = run(["make"], cwd=HERE / "client", env=environment)
        run([TOOLS / "ez80-none-elf-clang", "-target", "ez80-none-elf", "-nostdinc",
             "-isystem", TOOLS.parent / "include", "-I", HERE / "client/include", "-Oz",
             "-S", HERE / "client/src/main.cpp", "-o", package / "client.s"])
        symbols = run([TOOLS / "ez80-none-elf-nm", HERE / "client/obj/main.o"]).stdout
        assert "_flower_abi_submission_probe" in symbols and "_vdp_adv_call_buffer" in symbols
        backend = json.loads((package / "backends.json").read_text())
        binary = HERE / "client/bin/flower_abi_probe.bin"
        results.append({"variant": variant, "state_buffer": backend["shared_state"],
                        "buffered_entries": backend["buffered_entries"],
                        "native_entries": backend["native_wrappers"],
                        "eZ80_int_bytes": 3, "eZ80_pointer_bytes": 3,
                        "uint16_bytes": 2, "uint32_bytes": 4, "float_bytes": 4,
                        "vdp_call_bytes": 6, "host_C_and_Cxx_sanitizers": "pass",
                        "target_binary_bytes": binary.stat().st_size,
                        "target_binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
                        "target_execution_tested": False})
        (package / "target-build.log").write_text(build.stdout + build.stderr)
        shutil.copyfile(header, evidence / (variant + "-flower_golem.h"))
    assert results[0]["state_buffer"] != results[1]["state_buffer"]
    (evidence / "abi.json").write_text(json.dumps({"variants": results}, indent=2) + "\n")
    print("C interface: host C/C++ sanitizer packing/call checks and real AgonDev C++ links pass at both layouts.")
    print("Target executables were not run; installation, completion and input transport remain implementation work.")


if __name__ == "__main__":
    main()
