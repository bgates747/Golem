# Golem chooses the direction on either backend

The task-local compiler implements six uint16 comparisons and structured
If/ElseIf/Else/EndIf for buffered and native execution. The meter now has one
tick Sub: Golem chooses the direction and calculates all coordinates. Stock
VDP, uploaded Xtensa and host checks pass. The separate Oryx review is staged
and stopped. The Author accepted the visual review ("looks good"); retrieved
human results match all 533 observations/5,249 bytes. This tranche remains
approved for commit/push by the Author on 2026-10-13.

## Review

[meter.golem](meter.golem) is the complete program with initialized globals.
At phase 96 it selects downward movement; at phase 0 it selects upward movement.
A subsequent If chooses addition or subtraction, then calculates coordinates
and increments ticks. Every frame invokes the same named tick entry.

The picture resembles GOLEM-025: twelve journeys up/right and back, then a final
climb. Cyan means buffered; orange means native. Backend selection changes every
twelve frames without resetting state. Golem handles movement and reversal.
The resident VDU routine in [fixture.py](fixture.py) copies coordinates into
drawing commands. [guest.asm](guest.asm) installs, selects the backend, draws and
sends VSYNC; it does not calculate movement or select grow/shrink. The screen
identifies these roles. [Final screen reference](review-screen.png).

On Oryx:

```bash
cd ~/Agon/mystuff/golem/docs/tasks/GOLEM-026/.emulator
./fab-agon-emulator
```

Follow the single onscreen keyboard instruction after animation, close the
emulator and report whether it worked. The agent retrieves/verifies saved results;
no second terminal or verifier is needed. Rerunning is safe: previous results
are removed first. Source/report are also deployed beside the Oryx profile.

## Contract

Conditions accept checked uint16 expressions on both sides of == != < > <= >=.
Operands evaluate once left to right. ElseIf tests run only after preceding
tests fail; only the selected body executes. Empty branches and omitted Else
are allowed. Limits are eight nested If levels and 64 ElseIf alternatives per
chain. Two-character operators must be contiguous. Arithmetic parentheses work;
parentheses around the whole comparison are outside this slice.

Status resets per invocation; narrowed globals are admitted before source work.
Overflow, underflow or Bounds failure preserves the failed destination, retains
earlier assignments and skips every remaining statement. This also applies inside
conditions and selected branches. Unselected conditions/bodies have no runtime
effects. Invalid constant arithmetic is rejected during compilation, including
in source branches that would not be selected.

Buffered comparisons use high/low bytes in zero-extended scratch words to avoid
the stock 65535 sentinel. Straight-line assignments share blocks; branches/joins
use jumps so errors cannot return into later statements. Native C follows the
same typed tree. Both backends share authoritative packed globals and entry order;
native unload and private cleanup preserve state.

The 20-byte meter layout adds direction before error status. Install this layout
explicitly; it cannot reuse an installed GOLEM-025 18-byte state unchanged.
Buffered installation is 4,558 bytes. Native package size is 1,484 bytes, with
572 bytes resident text and four bytes data. No physical speedup is claimed.

This task forks the accepted GOLEM-024 frontend and GOLEM-025 emitters/tooling.
Historical tasks, production compiler, generic SDK and firmware are unchanged.
Source bool storage, Boolean composition, signed/float comparisons, loops,
source drawing, parameters and inter-procedure calls remain unsupported.

## Build and evidence

From the project root:

```bash
.venv/bin/python docs/tasks/GOLEM-026/host_checks.py
.venv/bin/python docs/tasks/GOLEM-026/fixture.py all
.venv/bin/python docs/tasks/GOLEM-026/stage_oryx.py
```

The standalone SDK packages generated C. Ignored build directories hold code,
interfaces, initialized state, command streams, native packages and named assembly
wrappers. Staging uses an exact canonical setup-script snapshot and existing
Oryx-owned runtime binaries. The generated launcher checks runtime/fixture hashes.

[Host receipt](host-checks.json): 2,236 model/generated-C cases under ASan/UBSan,
default/relocated packages, eight grammar/reference acceptances, 26 rejections,
ELF admission and maintained compiler checks. GOLEM-025 arithmetic source remains
in the regression suite. Cases include byte boundaries and 65535, nested/empty
joins, ElseIf order, skipped failures, condition failures, selected-branch Bounds
failures and earlier committed effects.

[Actual-target evidence](evidence/):

| Fixture | Named calls | Observations | Bytes |
| --- | ---: | ---: | ---: |
| Stock VDP conditions | 89 | 90 | 1,080 |
| Stock VDP tick/bounds | 26 | 27 | 540 |
| ESP32 conditions, each engine | 178 | 1,078 | 10,780 |
| ESP32 relocated conditions, each engine | 114 | 694 | 6,940 |
| ESP32 alternating animation, each engine | 600 | 533 | 5,249 |

JIT and interpreter pass. Whole state is checked every twelve frames and after
private cleanup. All 56,320 final playfield pixels, nine guest pixel queries and
actual MOS return pass. Final words are phase 96, step 4, x 240, y 56, right 252,
bottom 68, barRight 212, ticks 600, direction 0 and error 0.

The review takes about 19 seconds including startup/loading. Emulator timing
establishes functionality; physical throughput and perfectly paced presentation
remain unmeasured. No hardware was flashed. Visual acceptance and saved human results are recorded;
Keyboard success was not separately stated. The Author explicitly approved
commit/push on 2026-10-13.
