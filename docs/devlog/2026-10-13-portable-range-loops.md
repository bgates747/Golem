# Bounded Range loops and four-step Golem batches

GOLEM-027 adds bounded uint16 For/Range loops to both task-local backends, with
stock and actual Xtensa qualification passing. The animation now performs four
simulation steps per host call. Its Oryx review is staged/stopped with source;
human acceptance and explicit commit/push approval remain pending.

The Author approved proceeding after checkpoint 76bf86e. This task forks accepted
GOLEM-026, preserving historical tasks, production compiler and generic runtime
sources. Constant inclusive endpoints and signed steps produce a PC-computed
trip count/last value. Normal completion retains the last counter without a final
overflowing increment. Body writes to active counters are rejected; nesting and
worst-case source-work limits keep each invocation finite.

Buffered header/body/latch blocks use jumps, without unrolling or call-stack
growth. Native C uses a wider induction value and writes the same visited counter.
Errors in selected bodies/conditions retain counters and earlier effects, then
exit the whole invocation. The byte-command model now implements tail jumps
iteratively so long loops do not consume Python call frames.

The first full traversal through the ESP32 firmware exceeded the guest's old
240-tick reply deadline. The saved stage showed a pending first state read after
the buffered fullAscending entry; emulator execution reported no unresolved
instructions. A 24-bit elapsed-clock wait of 12000 ticks fixes this qualification
deadline. Full ascending and descending traversals then pass both engines. No
runtime or compiler behavior was changed to accommodate that diagnostic failure.

Canonical language review also confirmed optional Next names. Grammar supports
unnamed Next followed by another assignment, including assignment to the just-closed
counter. This adjustment leaves all 80 executable/common package artifacts
byte-identical to the target-qualified named-Next versions.

Host ASan/UBSan and model/generated-C checks pass 2488 cases, 22 grammar/reference
acceptances, 51 rejections, ELF admission and maintained checks. The host suite
takes about 52 seconds on the Pi. Stock VDP loops/batched checks take about 2.5/2.2
seconds. Oryx hybrid full-loop suites take about 25 seconds JIT and 54 interpreter;
relocated suites take about 9.3/9.5 seconds. Animation runs take about 19–22 seconds
including boot/loading. MOS timer and emulator presentation are functional, not
physical throughput evidence.

The complete meter.golem performs four unit steps inside Range(0:3), then computes
final coordinates. It uses a 22-byte layout, a 4965-byte buffered installation and
a 1628-byte native package (716 text bytes plus four data bytes). Six arithmetic
call bytes replace 24 when comparing one four-step batch with four separate
invocations. GOLEM-026 already used one call per frame, so this is added local
work capacity rather than a measured before/after reduction from that demo.

The 600-frame review produces 2400 steps, matching 585 observations/5769 bytes
and all 56320 final playfield pixels, with actual MOS return and shared-state
retention after cleanup. Source/report/reference image are deployed beside the
dedicated Oryx profile. No board was flashed or graphical emulator launched.

The Author reports the review worked, while noting its visual similarity to the
previous demo. Retrieved results identify GOLEM-027: 600 calls, 2400 simulation
steps, batch counter 3 and all 585 observations/5769 bytes matching. Runtime
identities pass. Human acceptance is recorded; explicit commit/push approval
remains pending. Future reviews should make their distinguishing behavior more
visible rather than relying on similar animation and small text labels.

The Author approved the commit/push checkpoint and proceeding with the Boolean
tranche on 2026-10-13. GOLEM-027 is accepted and leaves the active register.
