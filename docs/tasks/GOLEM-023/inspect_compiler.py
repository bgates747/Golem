"""Record pinned Xtensa compiler behavior; does not build or execute a module."""
from pathlib import Path
import hashlib
import json
import subprocess

HERE = Path(__file__).resolve().parent
SDK = HERE.parents[3] / "agon-vdplet"
COMPILER = Path.home() / ".platformio/packages/toolchain-xtensa-esp32/bin/xtensa-esp32-elf-gcc"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    version = subprocess.check_output([str(COMPILER), "--version"], text=True).splitlines()[0]
    if "8.4.0" not in version or "esp-2021r2-patch5" not in version:
        raise SystemExit(f"Unexpected compiler: {version}")
    out = HERE / "build"
    out.mkdir(exist_ok=True)
    source = HERE / "compiler_probe.c"
    obj = out / "compiler_probe.o"
    flags = ["-Os", "-fPIC", "-mlongcalls", "-mtext-section-literals",
             "-fvisibility=hidden", "-ffunction-sections", "-fdata-sections",
             "-fno-builtin", "-ffp-contract=off", "-fstack-usage", "-fno-common"]
    command = [str(COMPILER), *flags, "-c", str(source), "-o", str(obj)]
    subprocess.run(command, check=True)
    prefix = str(COMPILER).removesuffix("gcc")
    imports = subprocess.check_output([prefix + "nm", "--undefined-only", str(obj)], text=True)
    disassembly = subprocess.check_output([prefix + "objdump", "-dr", str(obj)], text=True)
    sizes = subprocess.check_output([prefix + "size", "-A", str(obj)], text=True)
    (out / "disassembly.txt").write_text(disassembly)
    (out / "sizes.txt").write_text(sizes)
    stack = (out / "compiler_probe.su").read_text()
    receipt = {
        "purpose": "Compile-only research; no Xtensa execution or hardware measurement",
        "compiler": version, "command": command, "source_sha256": digest(source),
        "sdk_commit": subprocess.check_output(["git", "-C", str(SDK), "rev-parse", "HEAD"], text=True).strip(),
        "undefined_symbols": [line.split()[-1] for line in imports.splitlines() if line.strip()],
        "stack_report": stack.splitlines(), "object_sha256": digest(obj),
        "disassembly_sha256": hashlib.sha256(disassembly.encode()).hexdigest(),
        "section_sizes": sizes.splitlines(),
    }
    (HERE / "compiler-inspection.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps(receipt, indent=2))


if __name__ == "__main__":
    main()
