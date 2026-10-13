"""Build/check the task-local common frontend. Never deploys or runs a target."""
from pathlib import Path
import hashlib
import json
import os
import struct
import subprocess
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BUILD = HERE / "build"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, **kwargs):
    return subprocess.run([str(item) for item in command], check=True, text=True,
                          capture_output=True, **kwargs)


def main():
    BUILD.mkdir(exist_ok=True)
    started = time.monotonic()
    flags = ["-std=c++17", "-Wall", "-Wextra", "-Werror", "-ffp-contract=off"]
    compiler = os.environ.get("CXX", "g++")
    planner = BUILD / "planner"
    run([compiler, *flags, "-O2", HERE/"planner.cpp", "-o", planner])
    test = BUILD / "test_portable"
    run([compiler, *flags, "-g", "-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
         HERE/"test_portable.cpp", "-o", test])
    environment = os.environ.copy()
    environment["ASAN_OPTIONS"] = "detect_leaks=1"
    contracts = run([test, HERE/"shared-example.golem"], env=environment)
    common = ("interface.json", "program.json", "state.bin", "layout.inc")
    comparisons = []
    for label, options, expected_id in (("default", [], 1),
                                         ("relocated", ["--reserve", "1:99", "--reserve", "150:159"], 100)):
        outputs = {}
        for target in ("buffered", "vdplet"):
            output = BUILD / f"{label}-{target}"
            run([planner, "--target", target, *options, HERE/"shared-example.golem", output])
            outputs[target] = output
            target_info = json.loads((output/"target.json").read_text())
            assert target_info["target"] == target
            assert target_info["requires_vdplet_runtime"] == (target == "vdplet")
            assert target_info["executable_output"] is False
            assert target_info["target_execution_qualified"] is False
            manifest = json.loads((output/"interface.json").read_text())
            assert manifest["state_buffer"] == expected_id
            assert manifest["state_bytes"] == 24
            assert manifest["state_owner"] == "application_shared"
            assert manifest["external_reservations"] == ([] if label == "default" else
                   [{"first":1,"last":99},{"first":150,"last":159}])
            assert manifest["error"]["offset"] == 22
            assert [(entry["name"],entry["logical_id"]) for entry in manifest["entries"]] == [("advance",0),("compute",1)]
            assert [item["offset"] for item in manifest["globals"]] == [0,2,4,6,10,14,18]
            assert (output/"state.bin").read_bytes() == struct.pack("<HHHffffH", 0, 3, 0, 2.0, 1.5, 0.25, 0.0, 0)
            ast = json.loads((output/"program.json").read_text())
            assert ast["procedures"][1]["assignments"][0]["value"]["type"] == "float32"
            assert ast["procedures"][1]["assignments"][0]["value"]["operator"] == "+"
        for name in common:
            assert (outputs["buffered"]/name).read_bytes() == (outputs["vdplet"]/name).read_bytes(), (label,name)
        comparisons.append({"layout":label,"state_buffer":expected_id,
                            "identical_common_files":{name:sha(outputs["buffered"]/name) for name in common}})

    assembler = ROOT/"docs/tasks/GOLEM-007/build/ez80asm"
    assembly_checks = []
    for label in ("default", "relocated"):
        output = BUILD/f"{label}-buffered"
        manifest = json.loads((output/"interface.json").read_text())
        symbols = ["golem_state_buffer", "golem_state_bytes", "golem_runtime_error_offset"]
        expected = [manifest["state_buffer"], 24, 22]
        for field in manifest["globals"]:
            stem = "golem_" + field["name"]
            symbols += [stem+suffix for suffix in ("_buffer","_offset","_bytes","_type")]
            expected += [manifest["state_buffer"],field["offset"],field["bytes"],{"uint16":1,"float32":3}[field["type"]]]
        symbols += ["golem_advance_entry_id", "golem_compute_entry_id"]
        expected += [0,1]
        consumer = BUILD/f"consumer-{label}.asm"
        binary = BUILD/f"consumer-{label}.bin"
        consumer.write_text('.assume adl=1\n.org $40000\ninclude "'+str(output/"layout.inc")+'"\n'+
                            ''.join('dl '+symbol+'\n' for symbol in symbols))
        run([assembler,consumer.name,binary.name,"-c"],cwd=BUILD)
        data = binary.read_bytes()
        assert len(data) == len(expected)*3
        assert [int.from_bytes(data[i:i+3],"little") for i in range(0,len(data),3)] == expected
        assembly_checks.append({"layout":label,"equates_checked":len(expected),"binary_bytes":len(data),"sha256":sha(binary)})

    # Error paths must reject without overwriting or creating output artifacts.
    output = BUILD / "default-buffered"
    before = {path.name:path.read_bytes() for path in output.iterdir()}
    bad_sources = (
        "uint16 x=0 Sub go x=65535+1 EndSub",
        "uint16 x=0 float32 y=1 Sub go x=y EndSub",
        "uint16 X=0 uint16 x=0 Sub go x=X EndSub",
        "uint16 x=0 Sub go x=x/2 EndSub",
    )
    rejected = 0
    for index, source in enumerate(bad_sources):
        path = BUILD / f"invalid-{index}.golem"
        path.write_text(source)
        result = subprocess.run([str(planner),"--target","buffered",str(path),str(output)], text=True,capture_output=True)
        assert result.returncode == 1 and result.stderr.startswith("planner:")
        assert {path.name:path.read_bytes() for path in output.iterdir()} == before
        rejected += 1
    for options in (("--target","unknown"), ("--target","buffered","--target","vdplet"),
                    ("--target","buffered","--reserve","0:1"),
                    ("--target","vdplet","--reserve","1:65534"),
                    ("--target","buffered","--reserve","1:3","--reserve","3:5")):
        result = subprocess.run([str(planner),*options,str(HERE/"shared-example.golem"),str(output)], text=True,capture_output=True)
        assert result.returncode == 1
        assert {path.name:path.read_bytes() for path in output.iterdir()} == before
        rejected += 1
    absent = BUILD/"rejected-output-must-not-exist"
    assert not absent.exists()
    result = subprocess.run([str(planner),"--target","buffered",str(BUILD/"invalid-0.golem"),str(absent)], text=True,capture_output=True)
    assert result.returncode == 1 and not absent.exists()
    rejected += 1
    maintained = run(["make", "-C", ROOT/"src", "check"])
    receipt = {
        "purpose":"Shared frontend/layout and HOST reference only; no backend emission or target execution",
        "base_commit":run(["git","-C",ROOT,"rev-parse","HEAD"]).stdout.strip(),
        "compiler":run([compiler,"--version"]).stdout.splitlines()[0],
        "source_sha256":{name:sha(HERE/name) for name in
                         ("portable.hpp","planner.cpp","test_portable.cpp","run_checks.py","shared-example.golem")},
        "production_dependency_sha256":{name:sha(ROOT/"src"/name) for name in ("token_reader.hpp","buffer_allocator.hpp")},
        "sanitized_host_contracts":contracts.stdout.strip(),
        "target_artifact_comparisons":comparisons,
        "real_assembler_consumers":assembly_checks,
        "cli_rejection_preserves_outputs":rejected,
        "maintained_checks":maintained.stdout.splitlines(),
        "elapsed_build_and_checks_seconds":round(time.monotonic()-started,3),
        "hardware_executed":False,"backend_emitters_implemented":False,
    }
    (HERE/"checks.json").write_text(json.dumps(receipt,indent=2)+"\n")
    print(contracts.stdout.strip())
    print(f"Two target selections: {len(comparisons)*len(common)} identical common artifact pairs; {rejected} CLI rejection cases pass.")
    print(f"Actual ez80asm: {len(assembly_checks)} consumers validate the exported layout equates.")
    print("Maintained compiler checks pass. Receipt: docs/tasks/GOLEM-024/checks.json")


if __name__ == "__main__":
    main()
