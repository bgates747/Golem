#!/usr/bin/env python3
"""Summarize retained, matched full-MOS runs; never launch an emulator."""
from pathlib import Path
import hashlib
import json
import statistics
import struct
import sys

HERE = Path(__file__).resolve().parent
BUILD = HERE / "build"
ROOT = HERE.parents[3]
sys.path.insert(0, str(ROOT.parent / "agon-utils/examples/agnb/api"))
from buffers import BufferRecord, build_container, read_container


def sha(data):
    return hashlib.sha256(data).hexdigest()


def playfield(name):
    data = (BUILD / name).read_bytes()
    kind, dimensions, maximum, rgb = data.split(b"\n", 3)
    assert (kind, dimensions, maximum) == (b"P6", b"512 384", b"255")
    assert len(rgb) == 512 * 384 * 3
    # The final distance-six pose fits inside this area. Exclude both HUDs,
    # MOS text and its blinking cursor, not any part of the wireframe.
    return rgb[16 * 512 * 3:328 * 512 * 3]


def main():
    names = ("benchmark-resident-jit", "benchmark-submit-jit", "benchmark-submit-interp")
    runs = {n: json.loads((HERE / (n + "-evidence.json")).read_text()) for n in names}
    fields = [playfield(n + ".ppm") for n in names]
    assert fields[0] == fields[1] == fields[2] and any(fields[0])
    timings = {}
    for engine in ("jit", "interp"):
        rows = [row for n, r in runs.items() if r["engine"] == engine for row in r["samples"]]
        timings[engine] = {}
        for arm, label in ((0, "resident"), (1, "submit")):
            selected = [row for row in rows if row[2] == arm]
            timings[engine][label] = {"samples": len(selected)}
            for at, key in ((3, "transform_status"), (4, "drawing_and_fence"), (5, "full_frame")):
                ms = [row[at] * 10 for row in selected]
                timings[engine][label][key + "_ms"] = {
                    "minimum": min(ms), "median": statistics.median(ms), "maximum": max(ms)
                }
    # Measure actual eZ80->VDP bytes between consecutive frame input updates.
    # The first24 intervals match the24 paired timed samples; frame25 provides
    # the terminating boundary and a common final picture.
    update = bytes((23, 0, 160, 1, 0, 5, 0xc2)) + struct.pack("<HH", 112, 32)
    wire = {}
    for name, run in runs.items():
        data = (BUILD / (name + "-mos-to-vdp.bin")).read_bytes()
        starts = []
        at = 0
        while (at := data.find(update, at)) != -1:
            starts.append(at)
            at += len(update)
        assert len(starts) == 25, (name, len(starts))
        sizes = [b - a for a, b in zip(starts, starts[1:])]
        by_arm = {str(arm): sorted({size for row, size in zip(run["samples"], sizes) if row[2] == arm}) for arm in (0, 1)}
        assert by_arm["0"] == by_arm["1"], by_arm
        wire[name] = {"frame_bytes_by_arm": by_arm, "tx_sha256": sha(data)}
    # A separate pure install-size calculation removes the extra edge table
    # and composed worker from the resident arm; timed runs coinstall both.
    common = read_container((BUILD / "scene.agnb").read_bytes())
    baseline = read_container((BUILD / "baseline.agnb").read_bytes())
    old = [r for r in common if r.buffer_id not in (59020, 59021, 59000)]
    old.append(BufferRecord(59000, (BUILD / "transform-original.vdpl").read_bytes()))
    old.extend(baseline)
    old_bytes = build_container(old)
    new_bytes = (BUILD / "scene.agnb").read_bytes()
    report = {
        "passed": True,
        "pixel_comparison": {"pixels_per_image": len(fields[0]) // 3, "images": list(names),
                             "matching_rgb_sha256": sha(fields[0]), "crop": "x0:511,y16:327 inclusive"},
        "timings": timings,
        "clock": "MOS centisecond units, effective20ms quantization; no physical speed claim",
        "frame_wire": wire,
        "pure_scene_container_bytes": {"resident": len(old_bytes), "submit": len(new_bytes),
                                       "saved": len(old_bytes) - len(new_bytes)},
        "resident_drawing_command_bytes": 176508,
        "native_edge_table_bytes": 19612,
        "native_raw_generated_plot_bytes_per_frame": 58836,
        "timing_memory_layout": "Both drawing paths and edge table coinstalled in each matched run",
        "unmeasured": ["physical hardware", "isolated raster time", "separate clear/HUD or generation-only time",
                       "comparison with a direct native primitive callback implementation"],
    }
    (HERE / "comparison.json").write_text(json.dumps(report, indent=2) + "\n")
    print("Matched pictures, frame wire counts and timing/install-size receipts pass.")


if __name__ == "__main__":
    main()
