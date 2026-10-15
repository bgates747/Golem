#!/usr/bin/env python3
"""Independent Flower algorithm audit: binary64 reference and rounded f32 model.

The f32 trig model rounds host libm's binary64 result. It is not a claim about
Xtensa libm accuracy or bit-for-bit agreement with the original BBC FPP.
"""
from pathlib import Path
import hashlib
import json
import math
import struct
import subprocess

HERE = Path(__file__).resolve().parent
DEFAULTS = ("3.03", "1.98", "0.6", "66", "0.8", "512", "0")
NAMES = ("petals", "vectors", "depth", "periods", "shrink", "radius_scale", "theta_init")


def f32(value):
    value = struct.unpack("<f", struct.pack("<f", value))[0]
    # Match the maintained portable profile's computed-zero/subnormal policy.
    return 0.0 if abs(value) < 2.0 ** -126 else value


def trace(strings, rounding=float):
    r = rounding
    p, v, d, t, s, scale, degrees = [r(float(value)) for value in strings]
    pv = r(p * v)
    raw = r(pv * t)
    count = math.floor(raw) + 1
    main_step = r(r(2 * math.pi) / pv)
    petal_step = r(r(2 * math.pi) / v)
    main = r(r(degrees * r(math.pi)) / r(180)) if degrees != 360 else 0.0
    petal = 0.0
    radius = r(scale / r(1 + d))
    shrink_step = r(0 - r(r(s * radius) / raw))
    initial_radius = radius

    def point():
        modulation = r(r(math.cos(petal)) * d)
        total_radius = r(r(modulation * radius) + radius)
        # BBC helper computes y first, then x. No FMA/refactoring.
        y = r(r(math.sin(main)) * total_radius)
        x = r(r(math.cos(main)) * total_radius)
        return x, y

    points = [point()]
    for _ in range(count):
        main = r(main_step + main)
        petal = r(petal_step + petal)
        radius = r(shrink_step + radius)
        points.append(point())
    return {"raw_steps": raw, "segments": count, "points": points,
            "initial_base_radius": initial_radius, "final_base_radius": radius,
            "main_step": main_step, "petal_step": petal_step, "shrink_step": shrink_step}


def case(name, **changes):
    values = dict(zip(NAMES, DEFAULTS))
    values.update({key: str(value) for key, value in changes.items()})
    strings = [values[key] for key in NAMES]
    reference, single = trace(strings), trace(strings, f32)
    matched = min(len(reference["points"]), len(single["points"]))
    errors = [math.hypot((single["points"][i][0] - reference["points"][i][0]) * 0.5,
                        (single["points"][i][1] - reference["points"][i][1]) * 480 / 1024)
              for i in range(matched)]
    summary = {"name": name, "strings": values,
               "binary64": {key: value for key, value in reference.items() if key != "points"},
               "float32_model": {key: value for key, value in single.items() if key != "points"},
               "max_unquantized_pixel_distance": max(errors),
               "float32_first": single["points"][0], "float32_last": single["points"][-1],
               "binary64_first": reference["points"][0], "binary64_last": reference["points"][-1]}
    return summary


def main():
    cases = [case("defaults"), case("cardinal-90", theta_init=90),
             case("cardinal-180", theta_init=180), case("cardinal-270", theta_init=270),
             case("full-circle", theta_init=360), case("fractional-angle", theta_init="22.5"),
             case("integer-product", petals=3, vectors=2, periods=2),
             case("noninteger-product", petals=3, vectors=2, periods="2.1"),
             case("zero-shrink", shrink=0), case("full-shrink", shrink=1),
             case("negative-shrink", shrink="-0.25"), case("over-shrink", shrink="1.25"),
             case("zero-radius", radius_scale=0),
             case("long-run", petals=3, vectors=2, periods=10000),
             case("count-near-integer", petals="1.00000001", vectors=1, periods=1)]
    by_name = {item["name"]: item for item in cases}
    assert by_name["defaults"]["binary64"]["segments"] == 396
    assert by_name["integer-product"]["binary64"]["segments"] == 13
    assert by_name["noninteger-product"]["binary64"]["segments"] == 13
    assert by_name["full-circle"]["float32_first"] == by_name["defaults"]["float32_first"]
    assert by_name["zero-radius"]["max_unquantized_pixel_distance"] == 0
    # Positive decimal values can straddle an integer only after f32 rounding.
    boundary = case("round-up-count", petals="1.99999999", vectors=1, periods=1)
    assert boundary["binary64"]["segments"] == 2
    assert boundary["float32_model"]["segments"] == 3
    cases.append(boundary)
    pinned = []
    for root, names in [
        (Path("/Agon/mystuff/agon-utils"), ["examples/flower/bbc/src/flower.asm",
          "examples/flower/bbc/src/fpp_ext.inc", "examples/flower/bbc/src/flower_demo.asm",
          "examples/flower/bs/src/main.inc"]),
        (Path("/Agon/agon-vdp"), ["video/vdu_buffered.h", "video/vdu.h", "video/vdu_sys.h", "video/agon_screen.h",
                                "video/context/viewport.h"]),
        (Path("/Agon/agondev"), ["release/include/agon/vdp.h", "release/include/agon/mos.h",
                               "src/lib/libkeyboard/keyboard.asm"])]:
        commit = subprocess.check_output(["git", "-C", str(root), "rev-parse", "HEAD"], text=True).strip()
        for name in names:
            path = root / name
            pinned.append({"path": str(path), "commit": commit,
                           "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
    evidence = HERE / "evidence"
    evidence.mkdir(exist_ok=True)
    report = {"reference": "host binary64 libm; not BBC-FPP emulation",
              "float32_model": "round every operation; host binary64 trig rounded to f32",
              "pixel_metric": "unquantized dx*0.5, dy*480/1024; not a raster comparison",
              "pinned_sources": pinned, "cases": cases}
    (evidence / "reference.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"Flower reference: {len(cases)} cases, original inclusive loop checked.")
    for name in ("defaults", "long-run", "round-up-count"):
        item = next(item for item in cases if item["name"] == name)
        print(f"{name}: {item['float32_model']['segments']} f32 segments; "
              f"max model/reference separation {item['max_unquantized_pixel_distance']:.6g} pixels")


if __name__ == "__main__":
    main()
