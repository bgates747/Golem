"""Host qualification for GOLEM-003; does not execute VDP firmware."""
from pathlib import Path
import hashlib
import json
import struct
import subprocess
import time

ROOT = Path(__file__).resolve().parent
PROJECT = ROOT.parents[2]
BUILD = ROOT / "build"
FLAGS = ["-std=c++17", "-Wall", "-Wextra", "-Werror", "-O1", "-g",
         "-fsanitize=address,undefined"]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, **kwargs):
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, **kwargs)
    if result.returncode:
        raise RuntimeError(f"{command}:\n{result.stdout}\n{result.stderr}")
    return result.stdout.strip()


def u16(data, offset):
    return struct.unpack_from("<H", data, offset)[0]


def buffered(data, offset):
    assert data[offset:offset + 3] == bytes([23, 0, 160]), (offset, data[offset:offset + 8])
    return u16(data, offset + 3), data[offset + 5]


def decode_install(data):
    assert data[:7] == bytes([23, 0, 248, 1, 0, 1, 0])
    blocks, cleared, updates = {}, set(), []
    pos = 7
    while pos < len(data):
        target, op = buffered(data, pos)
        if op == 2:
            assert target not in cleared
            cleared.add(target)
            pos += 6
        elif op == 0:
            size = u16(data, pos + 6)
            assert target in cleared and target not in blocks and size > 0
            blocks[target] = data[pos + 8:pos + 8 + size]
            assert len(blocks[target]) == size
            pos += 8 + size
        else:
            assert op == 5 and data[pos + 6] == 0xc2 and u16(data, pos + 7) == 0
            count = u16(data, pos + 9)
            updates.append((target, data[pos + 11:pos + 11 + count]))
            pos += 11 + count
    assert pos == len(data) and cleared == blocks.keys()
    assert len(updates) == 1 and updates[0][0] == 1000 and len(updates[0][1]) == 4
    return blocks


def check_programs(blocks):
    # Independent decoder of this slice's stock command formats. Validate every
    # copy span, fetched field and conditional target; do not search payloads
    # for command-looking bytes or treat this as a firmware execution test.
    executable = {11, 12, 13, 2000}
    edges = {identifier: set() for identifier in executable}
    count = 0

    def span(identifier, offset, size):
        assert identifier in blocks
        assert offset >= 0 and size > 0 and offset + size <= len(blocks[identifier])

    def field(data, pos):
        fmt = data[pos]
        assert fmt in (0, 0x40, 0xc0)
        span(u16(data, pos + 1), u16(data, pos + 3), 2 if fmt == 0xc0 else 4)

    for owner in executable:
        data, pos = blocks[owner], 0
        while pos < len(data):
            count += 1
            target, op = buffered(data, pos)
            assert target in blocks
            if op == 5:
                adjust = data[pos + 6]
                offset = u16(data, pos + 7)
                if adjust & 0x40:
                    size = u16(data, pos + 9)
                    span(target, offset, size)
                    if adjust & 0x20:
                        span(u16(data, pos + 11), u16(data, pos + 13), size)
                        pos += 15
                    else:
                        assert len(data[pos + 11:pos + 11 + size]) == size
                        pos += 11 + size
                else:
                    assert adjust == 7  # Single-byte XOR of the word sign bit.
                    span(target, offset, 1)
                    pos += 10
            elif op == 6:
                assert target in executable and data[pos + 6] in (0xa2, 0xa3)
                span(u16(data, pos + 7), u16(data, pos + 9), 2)
                span(u16(data, pos + 11), u16(data, pos + 13), 2)
                edges[owner].add(target)
                pos += 15
            elif op == 32:
                span(target, 0, 36)
                operation = data[pos + 6]
                assert operation in (0, 0x66)
                if operation == 0x66:
                    field(data, pos + 7)
                    field(data, pos + 12)
                pos += 7 if operation == 0 else 17
            elif op == 34:
                assert data[pos + 6:pos + 9] == bytes([0x20, 1, 1])
                span(target, 0, 4)
                field(data, pos + 9)
                pos += 14
            elif op == 41:
                assert data[pos + 6] == 8 and data[pos + 7] in (0x40, 0xc0)
                width = 8 if data[pos + 7] == 0x40 else 4
                span(target, 0, width)
                span(u16(data, pos + 8), 0, 36)
                span(u16(data, pos + 10), 0, width)
                assert u16(data, pos + 12) == 1
                pos += 14
            else:
                raise AssertionError((owner, pos, op))
        assert pos == len(data)

    def depth(owner, ancestors=()):
        assert owner not in ancestors, "recursive helper graph"
        return 1 + max((depth(child, (*ancestors, owner)) for child in edges[owner]), default=0)

    return {"decoded_commands": count, "program_blocks": len(executable),
            "root_bytes": len(blocks[2000]), "max_logical_call_depth": depth(2000),
            "computed_address_patch_sites": 0}


def main():
    BUILD.mkdir(exist_ok=True)
    sources = [p for p in ROOT.rglob("*") if p.is_file() and "build" not in p.relative_to(ROOT).parts
               and p.name != "host-checks.json"]
    sources.append(PROJECT / "src/golemc.cpp")
    before = {str(p.relative_to(PROJECT)): digest(p) for p in sorted(sources)}
    provenance = json.loads((ROOT / "reference/provenance.json").read_text())
    for name, expected in provenance["sha256"].items():
        assert digest(ROOT / "reference" / name) == expected
        original = subprocess.check_output(["git", "show", f"{provenance['commit']}:{name}"], cwd=PROJECT)
        assert hashlib.sha256(original).hexdigest() == expected

    # Reuse Rally's ordinary-mode expectations against the real production
    # compiler. Keep the frozen reference test immutable.
    baseline = (ROOT / "reference/tests/test_existing.cpp").read_text().replace(
        '#include "../src/golemc.cpp"', f'#include "{PROJECT / "src/golemc.cpp"}"')
    (BUILD / "test_main.cpp").write_text(baseline)
    suites = {"main_compatibility": BUILD / "test_main.cpp",
              "scalar_oracle": ROOT / "test_scalar.cpp",
              "rally_floor_boundaries": ROOT / "reference/tests/test_floor.cpp"}
    results = {}
    for name, source in suites.items():
        command = ["g++", *FLAGS, str(source), "-o", str(BUILD / name)]
        start = time.monotonic()
        print(f"Building {name}", flush=True)
        run(command)
        stdout = run([str(BUILD / name)])
        results[name] = {"command": command, "stdout": stdout,
                         "build_and_run_seconds": round(time.monotonic() - start, 3)}
        print(stdout, flush=True)
    run(["make"])
    cli = BUILD / "golemc-scalar"
    output = BUILD / "checked-example"
    stdout = run([str(cli), "examples/floor.golem", str(output), "-0.75"])
    manifest = json.loads((output / "interface.json").read_text())
    install = (output / "bootstrap.bin").read_bytes()
    blocks = decode_install(install)
    structure = check_programs(blocks)
    cleanup = (output / "cleanup.bin").read_bytes()
    cleanup_ids = []
    for pos in range(0, len(cleanup), 6):
        identifier, op = buffered(cleanup, pos)
        assert op == 2
        cleanup_ids.append(identifier)
    assert cleanup_ids == sorted(blocks)
    invoke = (output / "invoke.bin").read_bytes()
    packet = (output / "update-call.bin").read_bytes()
    assert invoke == bytes([23, 0, 160, 208, 7, 1])
    assert packet[:11] == bytes([23, 0, 160, 232, 3, 5, 0xc2, 0, 0, 4, 0])
    assert packet[11:15] == struct.pack("<f", -0.75) and packet[15:] == invoke
    assert manifest["input"]["id"] == 1000 and manifest["output"]["id"] == 1001
    assert manifest["entry"]["id"] == 2000 and manifest["entry"]["bytes"] == structure["root_bytes"]
    assert manifest["bootstrap_bytes"] == len(install)
    assert manifest["cleanup_bytes"] == len(cleanup)

    preserved = {p.name: p.read_bytes() for p in output.iterdir()}
    invalid = BUILD / "invalid.golem"
    invalid.write_text("float32 x Range(-1,1) = 2; sint16 result; Sub compute; "
                       "result=sint16(floor(x)); EndSub;")
    for command in ([str(cli), str(invalid), str(output)],
                    [str(cli), "examples/floor.golem", str(output), "32768"],
                    [str(cli), "examples/floor.golem", str(output), "nan"]):
        failure = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        assert failure.returncode == 1 and "golemc-scalar:" in failure.stderr
        assert {p.name: p.read_bytes() for p in output.iterdir()} == preserved

    results["structure"] = {**structure, "owned_buffers": len(blocks),
                            "cleanup_exact_ownership": True, "failed_compile_preserves_outputs": True}
    results["wire"] = {"cli_stdout": stdout, "installation": len(install),
                       "input_update": 15, "invoke": len(invoke), "update_call": len(packet),
                       "cleanup": len(cleanup), "output_readback_included": False,
                       "comparison": "reinstall identical bootstrap then invoke on every job",
                       "totals": {str(n): {"resident": len(install) + n * len(packet),
                                           "reinstall_each_time": n * (len(install) + len(invoke))}
                                  for n in (1, 2, 10, 100)}}
    after = {str(p.relative_to(PROJECT)): digest(p) for p in sorted(sources)}
    assert before == after, "Source changed during qualification"
    results["source_sha256"] = after
    results["firmware_executed"] = False
    (BUILD / "host-checks.json").write_text(json.dumps(results, indent=2) + "\n")
    print("Host checks passed; build/host-checks.json", flush=True)


if __name__ == "__main__":
    main()
