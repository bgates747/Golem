#!/usr/bin/env python3
"""Build an owned stock gate runner with prompt-aware command scheduling."""

import argparse
import hashlib
import json
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent


def replace_once(text, old, new):
    if text.count(old) != 1:
        raise RuntimeError(f"upstream gate changed; expected exactly one occurrence of {old!r}")
    return text.replace(old, new)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=HERE / "build/flexe-source")
    parser.add_argument("--core", type=Path, default=HERE.parent / "emulator/build/core")
    parser.add_argument("--headers", type=Path, default=HERE.parent / "emulator/build/flexe-source/src")
    parser.add_argument("--output", type=Path, default=HERE / "build/prompt-runner")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    harness = (args.source / "tools/agon_vdp_test.c").read_text()
    bridge = (args.source / "tools/agon_mos_bridge.h").read_text()
    bridge = replace_once(bridge, "    size_t key = 0;", "    size_t key = 0, command_start = 0;")
    bridge = replace_once(bridge, "            stage = 1; last_key = now;",
                          "            stage = 1; last_key = now; command_start = used;")
    bridge = replace_once(bridge, """                uint8_t ch = (uint8_t)command[key];
                if (periph_uart_rx_inject_num(p, 0, &ch, 1) != 1u) break;
                key++; last_key = now;""", """                /* Keep one unacknowledged key in flight. A busy native VDP
                 * can otherwise batch queued console keys when it returns
                 * from rendering, overwriting MOS's single-key state. */
                bool echoed = key == 0 ||
                    (used >= command_start + key &&
                     memcmp(transcript + command_start, command, key) == 0);
                if (echoed) {
                    uint8_t ch = (uint8_t)command[key];
                    if (periph_uart_rx_inject_num(p, 0, &ch, 1) != 1u) break;
                    key++; last_key = now;
                }""")
    bridge = replace_once(bridge, "            } else if (now - last_rx > 500000000ull) {\n                stage++; key = 0;\n            }",
                          """            } else {
                /* A quiet UART is not proof MOS has finished its command.
                 * Wait for the actual prompt emitted after this command. */
                const char *prompt = stage < 4u ? "/ *" : ">";
                size_t prompt_len = strlen(prompt);
                bool prompt_seen = used > command_start + strlen(command) &&
                    used >= prompt_len &&
                    memcmp(transcript + used - prompt_len, prompt, prompt_len) == 0;
                if (prompt_seen && now - last_rx > 100000000ull) {
                    stage++; key = 0; command_start = used; last_key = now;
                }
            }""")
    source = args.output / "agon_vdp_test.c"
    source.write_text(harness)
    (args.output / "agon_mos_bridge.h").write_text(bridge)
    command = ["cc", "-O2", "-I", str(args.headers), str(source),
               str(args.core / "libxtensa-emu-lib.a"), "-o", str(args.output / "flexe-agon-vdp-test"),
               "-lm", "-lz", "-lpthread", "-lssl", "-lcrypto"]
    subprocess.run(command, check=True)
    receipt = {
        "command": command,
        "source_commit": "fbda8e905ccff36a090e4e33b0ffc2337dd79c52",
        "harness_change": "Test command scheduling waits for actual MOS/BASIC prompt and each printable key's echo; guest firmware and every assertion remain unchanged.",
        "files": {str(path.resolve()): hashlib.sha256(path.read_bytes()).hexdigest()
                  for path in (source, args.output / "agon_mos_bridge.h", args.output / "flexe-agon-vdp-test",
                               args.core / "libxtensa-emu-lib.a", args.headers / "xtensa.c", args.headers / "xtensa.h", args.headers / "jit.c", args.headers / "peripherals.c")},
    }
    (args.output / "build-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(args.output / "flexe-agon-vdp-test")


if __name__ == "__main__":
    main()
