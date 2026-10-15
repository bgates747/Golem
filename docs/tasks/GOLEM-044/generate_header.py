#!/usr/bin/env python3
"""Task-local C header prototype derived from an existing portable package."""
from pathlib import Path
import argparse
import json

TYPES = {"uint16": ("uint16_t", 2), "sint16": ("int16_t", 2),
         "uint32": ("uint32_t", 4), "sint32": ("int32_t", 4),
         "float32": ("float", 4), "bool": ("bool", 2)}


def generate(package, output):
    interface = json.loads((package / "interface.json").read_text())
    backend = json.loads((package / "backends.json").read_text())
    root = json.loads((package / "root.json").read_text())
    if interface["state_buffer"] != backend["shared_state"] or interface["state_bytes"] != backend["state_bytes"]:
        raise ValueError("inconsistent shared package layout")
    lines = ["/* Generated from compiler metadata; task-local C interface prototype. */",
             "/* golem_NAME submits a call. It does not wait for VDP completion. */",
             "#ifndef FLOWER_GOLEM_GENERATED_H", "#define FLOWER_GOLEM_GENERATED_H",
             "#include <stdint.h>", "#include <stdbool.h>", "#include <string.h>",
             "#include <float.h>", "#ifdef __cplusplus", 'extern "C" {',
             "#define GOLEM_ASSERT static_assert", "#else", "#define GOLEM_ASSERT _Static_assert", "#endif",
             'GOLEM_ASSERT(sizeof(uint16_t)==2 && sizeof(uint32_t)==4, "Golem integer storage");',
             'GOLEM_ASSERT(sizeof(float)==4 && FLT_RADIX==2 && FLT_MANT_DIG==24 && FLT_MAX_EXP==128, "Golem binary32 storage");',
             "typedef enum { GOLEM_BUFFERED, GOLEM_NATIVE } golem_backend;",
             "/* Implement this tiny transport adapter in the host application. */",
             "void golem_host_call(uint16_t entry_buffer);",
             f"#define GOLEM_STATE_BUFFER {interface['state_buffer']}u",
             f"#define GOLEM_STATE_BYTES {interface['state_bytes']}u",
             f"#define GOLEM_ERROR_OFFSET {interface['error']['offset']}u",
             f"#define GOLEM_NATIVE_HANDLE_OFFSET {backend['native_handle_offset']}u",
             "static inline void golem_put16(uint8_t *out,uint16_t n) { out[0]=(uint8_t)n;out[1]=(uint8_t)(n>>8); }",
             "static inline void golem_put32(uint8_t *out,uint32_t n) { for(unsigned i=0;i<4;++i)out[i]=(uint8_t)(n>>(8*i)); }"]
    for name, offset in root["fields"].items():
        lines.append(f"#define GOLEM_ROOT_{name.upper()}_OFFSET {offset}u")
    for filename in ("common-install.bin", "buffered-install.bin", "native-install.bin",
                     "buffered-cleanup.bin", "native-cleanup.bin"):
        lines.append(f"#define GOLEM_{filename.removesuffix('.bin').replace('-', '_').upper()}_BYTES {(package / filename).stat().st_size}u")
    for field in interface["globals"]:
        name, kind, size = field["name"], field["type"], field["bytes"]
        ctype, expected_size = TYPES[kind]
        if size != expected_size or field["offset"] + size > interface["state_bytes"]:
            raise ValueError("invalid field layout")
        lines += [f"#define GOLEM_{name}_OFFSET {field['offset']}u",
                  f"static inline void golem_set_{name}(uint8_t *state,{ctype} value) {{"]
        if kind == "float32":
            lines += ["uint32_t bits;memcpy(&bits,&value,4);", f"golem_put32(state+GOLEM_{name}_OFFSET,bits);"]
        else:
            lines.append(f"golem_put{size * 8}(state+GOLEM_{name}_OFFSET,(uint{size * 8}_t)value);")
        lines.append("}")
    for entry in interface["entries"]:
        index, name = entry["logical_id"], entry["name"]
        lines += [f"#define GOLEM_{name}_LOGICAL_ID {index}u",
                  f"#define GOLEM_{name}_BUFFERED {backend['buffered_entries'][index]}u",
                  f"#define GOLEM_{name}_NATIVE {backend['native_wrappers'][index]}u",
                  f"static inline void golem_{name}(golem_backend backend) {{",
                  f"golem_host_call(backend==GOLEM_NATIVE?GOLEM_{name}_NATIVE:GOLEM_{name}_BUFFERED);", "}"]
    lines += ["#undef GOLEM_ASSERT", "#ifdef __cplusplus", "}", "#endif", "#endif", ""]
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("\n".join(lines))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("package", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    generate(args.package, args.output)
