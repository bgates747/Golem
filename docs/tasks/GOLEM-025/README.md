# Same Golem source, buffered commands or native VDPlet

The task-local compiler emits executable checked uint16 buffered commands and a
native VDPlet from the same typed program and packed state layout. Stock VDP and
actual uploaded Xtensa execution pass automated qualification on Oryx. A labelled
moving-square/meter review is staged and stopped there. The Author accepted the visual tranche and authorized commit/push on 2026-10-13.
Retrieved human results match all 481 observations; keyboard success was not
separately stated. Production promotion and physical flashing remain outside
this tranche.

## See the actual program

[meter.golem](meter.golem) is the complete Golem program. Its grow/shrink Subs
calculate phase, square coordinates, meter extent and a tick count. Both backends
compile these exact bodies. Eight uint16 globals plus the language error field
occupy the same 18-byte VDP state buffer.

[fixture.py](fixture.py) builds the resident VDU renderer. It copies Golem's
coordinate words into drawing commands and draws the square/meter, without
movement arithmetic. [guest.asm](guest.asm) installs packages, selects the named
entry/backend, invokes drawing and sends the stock VSYNC swap. Assembly also
reads results for qualification. The screen identifies these roles; drawing is
not yet part of this portable Golem grammar.

The animation runs 600 frames: twelve journeys up/right and back, then a final
climb. Cyan means buffered execution; orange means native execution. Selection
changes every twelve frames, continuing the same state. Both Subs remain finite.
The final orange square sits near the upper right and the orange meter is nearly
full. The scene remains above MOS. [Captured final screen](review-screen.png).

## Oryx human review

In a terminal on Oryx:

```bash
cd ~/Agon/mystuff/golem/docs/tasks/GOLEM-025/.emulator
./flexe-agon-emulator
```

Watch the square move and the meter fill/empty, alternating cyan and orange.
Follow the single on-screen `dir` instruction when MOS returns. Close the
emulator and report whether animation and keyboard worked. No second terminal
or verifier command is needed. The launcher removes previous results before
each run, so rerunning is safe.

The deployed source is also at
`~/Agon/mystuff/golem/docs/tasks/GOLEM-025/meter.golem`. After review the agent
retrieves `.emulator/sdcard/golem025/results.dat`, verifies it against staged
expectations and records the human report. Closing the emulator first is fine.

## Implemented contract

- Explicit initialized uint16 globals, optional inclusive Bounds, finite named
  parameterless Subs, assignments, parentheses and checked + / - / *.
- Shared frontend, evaluation order, packed offsets, initial state and logical
  entry order. Unsupported types/constructs are rejected, including float output.
- Each call resets language status and admits all narrowed globals. Statuses are
  addition overflow 1, subtraction underflow 2, multiplication overflow 3 and
  Bounds failure 32. Failure preserves the failed destination, retains earlier
  committed assignments and skips remaining statements.
- Common installation is separate from both private installations/cleanups.
  Both implementations coexist; native unloading and private cleanup retain
  shared state. The existing standalone SDK/runtime is unchanged.
- Native arguments contain the shared state buffer ID. After LOAD, assembly
  explicitly patches the fresh handle into resident RUN wrappers. Generated
  `golem_buffered_grow` / `golem_native_grow` and corresponding shrink wrappers
  transmit six-byte calls. The application chooses the wrapper.
- Host return alone is not completion. The review serializes computation,
  drawing and VSYNC through the VDP stream and confirms state at backend changes.
  Unload follows completed observations. There is no capability probe, automatic
  replay, concurrent mutation or mid-call replacement.

The meter's buffered installation is 4,938 bytes. Its native package is 1,472
bytes, including 560 bytes of resident text and four bytes of data. Different
loader overheads apply; this is not a speed comparison. An ordinary frame sends
15 wire bytes for arithmetic, resident drawing and VSYNC. Review-only state
queries and periodic colour/label updates add traffic. The host does not send
changing coordinates each frame.

## Build and evidence

From the project root, with its existing virtual environment and SDK/toolchain:

```bash
.venv/bin/python docs/tasks/GOLEM-025/host_checks.py
.venv/bin/python docs/tasks/GOLEM-025/fixture.py all
.venv/bin/python docs/tasks/GOLEM-025/stage_oryx.py
```

The builder uses the separate agon-vdplet SDK. Ignored build directories contain
common interface/program JSON, state, layout equates, separate installation and
cleanup streams, generated C, native packages and named assembly wrappers.
The compiler CLI is `build/compiler source.golem output-directory
[reserved-prefix-last]`; fixture IDs 60000–60031 are reserved.

Staging copies an exact canonical emulator-setup script snapshot and uses
existing Oryx-owned stock/hybrid binaries and enabled firmware. It creates no
graphical window or changes to other profiles. The review wrapper checks runtime
and fixture hashes before starting.

On Oryx the agent can prepare a variant with profile.py, then run
`qualify.py VARIANT [--engine jit|interp]`. Variants are stock, meter-stock,
hybrid, relocated and review. The first two use official stock VDP; the others
execute actual ESP32 firmware and uploaded Xtensa code. These are agent tools;
human review requires only the launch command above.

[host-checks.json](host-checks.json) records 218 model/generated-C cases across
default and relocated packages, ASan/UBSan, ELF admission and maintained compiler
contracts. [Actual-target receipts and saved observations](evidence/) cover:

| Target/fixture | Calls | Observations | Saved bytes |
| --- | ---: | ---: | ---: |
| Stock VDP, arithmetic | 46 | 47 | 564 |
| Stock VDP, meter source | 63 | 64 | 1,152 |
| ESP32 arithmetic, each engine | 92 | 562 | 5,620 |
| ESP32 relocated arithmetic, each engine | 22 | 142 | 1,420 |
| ESP32 alternating animation, each engine | 600 | 481 | 4,729 |

Both JIT and interpreter pass. The animation independently verifies all 56,320
final playfield pixels, nine guest pixel probes, whole state every twelve frames
and after private cleanups, and actual MOS return. Fresh LOAD handles and IRAM
entry addresses are checked. Final state is phase 96, step 4, x 240, y 56, right
252, bottom 68, barRight 212, ticks 600, error 0.

The Oryx animation run takes about 19 seconds including boot/loading; 600 VSYNC
frames nominally represent ten seconds. Emulator timing establishes functionality,
not physical throughput or frame-perfect presentation. No board was flashed;
the maintained production compiler is unchanged. Portable drawing, broader
language growth, production promotion and selection UX remain later tranches.
