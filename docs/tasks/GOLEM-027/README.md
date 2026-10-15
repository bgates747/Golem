# Bounded Range loops on buffered and native targets

The task-local compiler now emits bounded For/Range loops for both backends.
One Golem call can perform multiple iterations without unrolling the body or
requiring repeated host calls. Host, stock VDP and actual Xtensa qualification
pass, including full 0–65535 traversal. A four-step moving-meter review is
staged and stopped on Oryx. The Author reports it worked; retrieved human results
match all 585 observations/5,769 bytes. The Author explicitly approved commit/push on 2026-10-13.

## Review the program

[meter.golem](meter.golem) is the complete initialized source. Its single tick
Sub loops over Range(0:3), choosing direction and moving one phase unit on each
iteration. It then calculates the final drawing coordinates. Each host call
therefore performs four simulation steps. After 600 frames, ticks is 2400 and
the batch counter retains 3.

The movement deliberately resembles the previous demo. Cyan means buffered
execution and orange means native; backend choice changes every twelve frames
while sharing the same state. Golem computes motion/reversal. The resident VDU
routine in [fixture.py](fixture.py) draws the final coordinates, and
[guest.asm](guest.asm) selects the backend, invokes drawing and paces with VSYNC.
There are no hidden direct drawing scenes. [Final screen reference](review-screen.png).

On Oryx:

```bash
cd ~/Agon/mystuff/golem/docs/tasks/GOLEM-027/.emulator
./flexe-agon-emulator
```

Follow the single onscreen keyboard instruction, close the emulator and report
whether it worked. The agent retrieves/verifies saved results. No second terminal
or verifier is required. Reruns remove old results automatically. Source and this
report are deployed beside the Oryx profile.

## Loop rules

The form is For counter In Range(start:end:step), followed by Next with an optional
counter name. Step defaults to 1. The counter must already be an initialized
uint16 global. Range parameters in this slice are integer literals; endpoints
are 0–65535, with a signed nonzero step of magnitude at most 65535.

Bounds are inclusive, but the endpoint is visited only when reached exactly:
Range(0:9:2) visits 0,2,4,6,8; Range(9:0:-2) visits 9,7,5,3,1. Equal endpoints
execute once with either step sign. Reject zero steps and steps pointing away
from the endpoint. There is no implicit counter declaration or reversed empty range.

The PC computes trip count and last value. All visited values must fit the
counter's Bounds; an unvisited endpoint need not fit those narrower Bounds.
For example, Bounds(10,14) permits Range(10:15:2), which visits 10,12,14.
The body may read its counter but cannot assign it, including inside branches
or nested loops. Nested loops need distinct counters; reuse after Next is allowed.

Normal completion retains the last visited value and never increments beyond it.
Full ascending and descending uint16 ranges therefore finish without wrapping.
Failure in a body or condition exits the whole invocation, preserving the current
counter and earlier effects. Untaken loops leave their counter unchanged. Entry
admission still checks narrowed globals before any source work, even when a loop
would subsequently initialize its counter.

Limits are eight nested loops and 262144 worst-case work visits per entry.
Assignments/conditions count once; each loop adds its header and each iteration
plus its body. Sequential work sums, branches take the larger path, and nested
loops multiply. The common interface publishes worst_case_visits; the typed
program publishes start/end/step/trips/last. This is a simple finite-work bound,
not a target instruction count or timing guarantee. Buffered jobs do not yield:
keep real animation batches small so input and presentation remain responsive.

Newlines remain whitespace and no semicolons are accepted. Optional Next names
are disambiguated from subsequent assignments, including the one-line form
Next i = value, which means unnamed Next followed by an assignment to i.
Signed/float counters, runtime Range parameters, Break/Continue and source drawing
remain outside this tranche. Existing uint16 arithmetic and structured If remain.

## Wire traffic and package sizes

Four separately invoked simulation steps would send 24 arithmetic-call bytes;
this batch sends six. That saves 18 arithmetic bytes per four-step batch, or 75%.
With the existing six-byte drawing call and three-byte VSYNC swap, the ordinary
frame uses 15 outgoing bytes instead of 33 for four separate arithmetic calls.
Review-only state queries and colour/label updates add traffic.

GOLEM-026 already sent one arithmetic call per displayed frame. This tranche
adds more work within that call; the comparison above is against invoking four
small steps separately, not a measured traffic reduction from the old demo.

The new meter layout is 22 bytes, adding batch before error status; explicitly
install it rather than reusing a previous 20-byte state buffer unchanged. Both
backends share that new layout. Buffered installation is 4,965 bytes. The native
package is 1,628 bytes, with 716 bytes resident text and four bytes data. The
generic runtime/SDK, historical task artifacts and production compiler are unchanged.

## Build and evidence

From the project root:

```bash
.venv/bin/python docs/tasks/GOLEM-027/host_checks.py
.venv/bin/python docs/tasks/GOLEM-027/fixture.py all
.venv/bin/python docs/tasks/GOLEM-027/stage_oryx.py
```

This forks accepted GOLEM-026 under its own task silo. Ignored build directories
contain the shared interface/model/state, assembly wrappers, installation and
cleanup streams, native C and SDK-packaged Xtensa modules. Staging uses the
canonical setup tool and existing Oryx-owned runtime binaries. The generated
review wrapper checks runtime/fixture hashes before launch.

[Host receipt](host-checks.json): 2,488 model/generated-C cases under ASan/UBSan,
22 grammar/reference acceptances, 51 rejections, native ELF admission, relocated
IDs, prior arithmetic/condition regression and maintained compiler checks. The
suite takes about 52 seconds on the development Pi, including deliberate full-range
traversals. The optional-Next grammar adjustment leaves all 80 executable/common
package artifacts byte-identical to the target-qualified versions.

[Actual-target evidence](evidence/):

| Fixture | Named calls | Observations | Bytes |
| --- | ---: | ---: | ---: |
| Stock VDP loops | 33 | 34 | 612 |
| Stock VDP batched movement/bounds | 28 | 29 | 638 |
| ESP32 loops, each engine | 66 | 607 | 6,070 |
| ESP32 relocated loops, each engine | 62 | 571 | 5,710 |
| ESP32 alternating animation, each engine | 600 | 585 | 5,769 |

JIT and interpreter pass. Tests cover positive/negative/unaligned/singleton/extreme
ranges, full traversals, nested loops and branches, selected/skipped failures,
counter retention, counter reuse and Bounds. Animation state is checked every
twelve frames and after cleanup; all 56,320 final playfield pixels and nine guest
pixel queries match, with actual MOS return. Final words are phase 96, step 1,
x 240, y 56, right 252, bottom 68, barRight 212, ticks 2400, direction 0, batch 3,
error 0.

The first hybrid full-range run exceeded the guest's inherited 240-tick reply
deadline. Its compact failure and emulator stats are retained under evidence/diagnostic.
The qualification guest now waits at most 12000 MOS clock ticks using a 24-bit
elapsed timer. Both full-range engines then pass; no runtime or compiler change
was needed for that diagnostic fix. MOS clock increments follow VBLANK, so this
deadline is not a wall-clock performance requirement.

Oryx full-loop suites took about 25 seconds in JIT and 54 seconds in interpreter;
the visual review took about 19–22 seconds including boot/loading. These timings
establish functionality, not physical throughput or frame-perfect presentation.
No physical board was flashed and no graphical emulator was launched by the agent.
