#!/usr/bin/env python3
"""Test unchanged SDK loader admission of ordinary Xtensa float math libraries.

This does not execute Xtensa instructions or establish math/stack correctness.
"""
from pathlib import Path
import hashlib
import json
import os
import struct
import subprocess

HERE = Path(__file__).resolve().parent
SDK = HERE.parents[3] / "agon-vdplet"
CC = Path.home() / ".platformio/packages/toolchain-xtensa-esp32/bin/xtensa-esp32-elf-gcc"
OUT = HERE / "build/math"


def run(command, log=None):
    result = subprocess.run([str(arg) for arg in command], text=True, capture_output=True)
    if log:
        log.write_text(result.stdout + result.stderr)
    return result


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    prefix = str(CC).removesuffix("gcc")
    flags = ["-I", SDK / "include", "-Os", "-fPIC", "-mlongcalls", "-mtext-section-literals",
             "-fvisibility=hidden", "-ffunction-sections", "-fdata-sections", "-fno-builtin",
             "-ffp-contract=off", "-fstack-usage", "-fno-common"]
    obj = OUT / "math.o"
    result = run([CC, *flags, "-c", HERE / "math_probe.c", "-o", obj], OUT / "compile.log")
    result.check_returncode()
    run([prefix + "objdump", "-dr", obj], OUT / "math-object.txt").check_returncode()
    undefined = run([prefix + "nm", "-u", obj]).stdout.splitlines()
    admit = OUT / "admit"
    result = run([os.environ.get("CXX", "c++"), "-std=c++17", "-Wall", "-Wextra", "-Werror",
                  "-fsanitize=address,undefined", "-g", "-I", SDK / "src", HERE / "admit_probe.cpp",
                  "-o", admit], OUT / "admit-build.log")
    result.check_returncode()
    report = {"compiler": run([CC, "--version"]).stdout.splitlines()[0],
              "sdk_commit": run(["git", "-C", SDK, "rev-parse", "HEAD"]).stdout.strip(),
              "source_sha256": hashlib.sha256((HERE / "math_probe.c").read_bytes()).hexdigest(),
              "undefined_in_object": undefined, "execution_tested": False, "variants": {}}
    for name, extras in [("sdk-default", []), ("static-libraries", ["-lm", "-lgcc"]),
                         ("bound-libraries", ["-Wl,-Bsymbolic", "-lm", "-lgcc"])]:
        elf, upload = OUT / (name + ".elf"), OUT / (name + ".upload.elf")
        command = [CC, "-nostdlib", "-nostartfiles", "-shared", "-Wl,--gc-sections",
                   "-Wl,-e,vdplet_entry", "-Wl,--no-undefined",
                   "-Wl,--version-script=" + str(SDK / "tools/module.exports"),
                   obj, *extras, "-o", elf]
        result = run(command, OUT / (name + "-link.log"))
        info = {"link_exit": result.returncode, "link_options": extras}
        report["variants"][name] = info
        if result.returncode:
            info["diagnostic"] = result.stderr.strip()
            continue
        run([prefix + "strip", "--strip-unneeded", *["--remove-section=" + section for section in
             (".got.loc", ".got", ".dynamic", ".comment", ".xt.lit", ".xt.prop", ".xtensa.info")],
             "-o", upload, elf]).check_returncode()
        data = bytearray(upload.read_bytes())
        phoff = struct.unpack_from("<I", data, 28)[0]
        phsize, phcount = struct.unpack_from("<HH", data, 42)
        for index in range(phcount):
            at = phoff + index * phsize
            if struct.unpack_from("<I", data, at)[0] == 2:
                struct.pack_into("<I", data, at, 0)
        upload.write_bytes(data)
        envelope = struct.pack("<4sHHIIHHIIIIIIIIIII", b"VDPL", 1, 64, 64 + len(data),
                               0x32505345, 1, 0, 1, 32768, 4096, 2048,
                               0x464C5752, 64, len(data), 80400, 0, 0, 0)
        package = OUT / (name + ".vdpl")
        package.write_bytes(envelope + data)
        result = run([admit, package], OUT / (name + "-admission.log"))
        result.check_returncode()
        info.update(json.loads(result.stdout))
        info["package_bytes"] = package.stat().st_size
        info["sha256"] = hashlib.sha256(package.read_bytes()).hexdigest()
        info["undefined_after_link"] = run([prefix + "nm", "-u", elf]).stdout.splitlines()
        run([prefix + "readelf", "-hSWr", upload], OUT / (name + "-elf.txt")).check_returncode()
    evidence = HERE / "evidence"
    evidence.mkdir(exist_ok=True)
    (evidence / "math-link.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
