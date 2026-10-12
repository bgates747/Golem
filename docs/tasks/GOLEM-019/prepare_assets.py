"""Select unchanged Wolf AGNB records and render their native-size reference."""

import argparse
import hashlib
import importlib.util
import json
import sqlite3
import struct
import zlib
from pathlib import Path


HERE = Path(__file__).resolve().parent
# Positions in a 320x240 display; no scaling or bitmap transforms.
SELECTION = (
    ("core", "splash", 0, 0),
    ("core", "bj_100", 16, 184),
    ("bj", "bj_pistol_00", 96, 168),
    ("core", "lower_panel_pistol", 200, 190),
)


def png(path, width, height, rgb):
    def chunk(tag, data):
        return (struct.pack(">I", len(data)) + tag + data
                + struct.pack(">I", zlib.crc32(tag + data)))

    scanlines = b"".join(
        b"\0" + bytes(rgb[y * width * 3:(y + 1) * width * 3])
        for y in range(height)
    )
    path.write_bytes(
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(scanlines)) + chunk(b"IEND", b"")
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--wolf", type=Path, default=HERE.parents[3] / "AgonWolf3D")
    args = parser.parse_args()
    wolf = args.wolf.resolve()
    validator = wolf / "build/scripts/validate_ui_agnb.py"
    spec = importlib.util.spec_from_file_location("wolf_ui_validator", validator)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    container = wolf / "tgt/ui.agnb"
    actual = {r[0]: r for r in module.read_image_records(container)}
    catalog = {}
    with sqlite3.connect(f"file:{wolf}/build/data/build.db?mode=ro", uri=True) as db:
        for family, table, first in (
            ("core", "tbl_91b_UI", 0x2000), ("bj", "tbl_91c_UI_BJ", 0x2100)
        ):
            rows = db.execute(
                f"SELECT panel_base_filename,dim_x,dim_y FROM {table} "
                "ORDER BY panel_base_filename"
            ).fetchall()
            for index, (name, width, height) in enumerate(rows):
                catalog[family, name] = first + index, width, height

    assets = HERE / "assets"
    assets.mkdir(exist_ok=True)
    canvas = bytearray(320 * 240 * 3)
    manifest = []
    for family, name, x, y in SELECTION:
        buffer_id, width, height = catalog[family, name]
        relative = f"build/ui/rgba2/{family}/{name}.rgba2"
        pixels = (wolf / relative).read_bytes()
        assert len(pixels) == width * height
        assert actual[buffer_id] == (buffer_id, width, height, pixels)
        assert x + width <= 320 and y + height <= 240
        assert {p >> 6 for p in pixels} <= {0, 3}, "reference expects binary alpha"
        (assets / f"{name}.rgba2").write_bytes(pixels)
        for index, value in enumerate(pixels):
            if value >> 6:
                offset = ((y + index // width) * 320 + x + index % width) * 3
                canvas[offset:offset + 3] = bytes(
                    ((value & 3) * 85, ((value >> 2) & 3) * 85,
                     ((value >> 4) & 3) * 85)
                )
        manifest.append(dict(
            name=name, source=relative, buffer_id=buffer_id,
            width=width, height=height, x=x, y=y, bytes=len(pixels),
            sha256=hashlib.sha256(pixels).hexdigest(),
        ))

    # Copy each selected LIST BUFR chunk verbatim, retaining its embedded ID.
    data = container.read_bytes()
    offset = 12
    chunks = {}
    version = None
    while offset < len(data):
        tag, payload, end = module._chunk(data, offset, len(data))
        if tag == b"VERS":
            version = data[offset:end]
        else:
            _, bhdr, _ = module._chunk(payload, 4, len(payload))
            chunks[struct.unpack("<H", bhdr)[0]] = data[offset:end]
        offset = end
    body = b"AGNB" + version + b"".join(chunks[r["buffer_id"]] for r in manifest)
    subset = assets / "wolf-review.agnb"
    subset.write_bytes(b"RIFF" + struct.pack("<I", len(body)) + body)
    assert module.read_image_records(subset) == [
        actual[r["buffer_id"]] for r in manifest
    ]
    (HERE / "assets.json").write_text(json.dumps(dict(
        source_project="AgonWolf3D", source_container="tgt/ui.agnb",
        source_container_sha256=hashlib.sha256(data).hexdigest(),
        display=dict(width=320, height=240, background="black"), assets=manifest,
    ), indent=2) + "\n")
    png(HERE / "reference-layout.png", 320, 240, canvas)
    print(f"Prepared {len(manifest)} unchanged Wolf bitmaps; {subset.stat().st_size} AGNB bytes.")


if __name__ == "__main__":
    main()
