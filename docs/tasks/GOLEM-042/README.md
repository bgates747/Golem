# Portable integer XOR

Golem now supports `^` for all four maintained integer types, with the same
source and results on stock VDP or a native VDPlet. Host conformance and fresh
target qualification pass. A three-lane animation demonstrates mirrored motion
and direction/colour toggles computed in Golem. The Author accepted Oryx review
on 2026-10-15; retrieved results and final pixels pass. The Author subsequently
explicitly approved committing/pushing all pending work on 2026-10-15.

The accepted release-error fix was committed/pushed as `cd930ae`, followed by
this task's plan as `ecb981f`, before XOR implementation started.

## Language and implementation

1. `a ^ b` requires matching uint16, sint16, uint32 or sint32 operands and returns
   that type. It combines fixed-width bit patterns; signed sign bits participate.
   XOR itself cannot overflow. Destination Bounds still apply with status 32.
2. Both operands evaluate once, left to right. Failed operands preserve their
   existing statuses and the failed destination, retaining earlier effects.
   Chaining associates left to right. Precedence is multiplication, addition/
   subtraction, right shift, XOR, comparisons, Not, And, Or.
3. Scalar/array assignments, array indices, conversions, comparisons and divmod
   arguments accept XOR. Bool/float XOR and implicit mixed integer types reject.
   Explicit conversions retain numeric range checks. `**` remains future power
   syntax; other bitwise operators and left shift are separate work.
4. Stock emission copies the left result and applies buffer-adjust operation 7
   across two/four bytes. Native C uses unsigned stored bit patterns, masking
   narrow results. No signed C overflow/narrowing or per-bit runtime loop is used.
5. Parser/reference/serialization live in `src/portable/portable.hpp`; both
   emitters live in `src/portable/backends.hpp`. The maintained independent model
   and focused tests live in `tests/portable/`. No production component imports
   task implementations. SDK, firmware, peer code and root schema are unchanged.

Read [the small initialized example](../../../examples/portable/xor.golem) or
[the complete animation](qualification/animation.golem). The first yields 6
from 5 ^ 3 and 2147483647 from signed -1 ^ -2147483648.

## Evidence

| Check | Result |
| --- | --- |
| Maintained raw/resident and portable contracts | Pass; 132 legacy and 46 portable rejection cases |
| C++ frontend/driver ASan/UBSan | Pass, including XOR type, folding and precedence cases |
| Independent model/buffered machine/generated C, ASan/UBSan | 468 default + 468 relocated cases pass |
| Real eZ80 assembly consumers | Both layouts pass |
| No-XOR output parity against accepted `cd930ae` compiler | All 55 package files equal, including native C |
| Historical frozen fixture provenance | All 58 files intact |
| Official stock VDP default/relocated | 28 calls, 1,025 observations, 2,050 bytes per run pass |
| Actual Xtensa JIT/interpreter default/relocated | 56 calls, 2,223 observations, 22,230 bytes per run pass |
| Actual Xtensa animation, both engines | 300 calls, 270 observations, 2,646 bytes and 56,320 final playfield pixels per run pass |
| Human review | Accepted 2026-10-15; saved results/pixels, identities and MOS return verify |

The focused host suite covers zero/all-ones, individual sign bits, alternating
patterns, deterministic random values, all four integer types, constant/runtime
expressions, precedence, comparisons, arrays, conversion/range failures,
divmod arguments, Bounds preservation, prior effects, operand failure order and
short-circuit skips. Explicit expected syntax results supplement the serialized
tree oracle. Observed language statuses are 0,1,2,3,4,5,32. Release behavior is
the accepted GOLEM-041 contract; existing no-XOR native output is identical.

See [host/resources](evidence/host.json), [accepted baseline hashes](evidence/baseline.json),
[parity receipt](evidence/parity.json), [module receipts](qualification/packages.json)
and [target receipts/results](qualification/evidence/). Each target variant has
one compact expected-check description, expected bytes and mask; each engine has
its own result bytes and receipt. Retrieved results were independently verified
against the generated expectations. Actual Flexe runs return to MOS with no
backend errors. Stock qualification verifies saved observations and clean
shutdown; it does not separately claim a captured MOS prompt.

All automated instances forced dummy video and dummy audio and ran sequentially.
The native runtime bytes match the accepted GOLEM-037 frontend/adapter/firmware;
no physical target was flashed. Task tooling derives from accepted GOLEM-041
qualification, adapted to XOR source and the maintained model. The canonical
setup script was copied unchanged into ignored build storage for Oryx staging.
Existing profiles and historical evidence remain intact.

## Resources and traffic

The unchanged SDK is pinned to `9fbb21dd09402e2a3835067dbb4427f2c20dec3c`.
Its actual image validator admits all three modules. Build ID is `0x474c3432`;
declared limits remain 32 KiB text, 4 KiB data, 2 KiB module stack, 64 KiB image.

| Module | Image bytes | Resident text | Resident data |
| --- | ---: | ---: | ---: |
| Probe | 5,072 | 3,764 | 268 |
| Relocated probe | 5,092 | 3,784 | 268 |
| Animation | 4,072 | 2,788 | 268 |

The animation shares 114 root/global bytes plus 36 array bytes. Common
installation is 248 bytes; buffered private installation is 13,556 bytes;
native private wrappers are 60 bytes plus separately uploaded module bytes.
Ordinary frames send 15 bytes: six to call the chosen Golem entry, six to call
the drawing buffer, three to swap at VSYNC. All coordinate/direction/colour
updates stay on the VDP. Backend caption changes and qualification readback
are additional diagnostic traffic, not part of that ordinary-frame count.

A dynamic stock XOR contributes one 15-byte copy and one 15-byte adjustment to
resident code, plus its two/four-byte result storage and normal allocation/
installation framing. Repeated XOR does not itself add host wire traffic.
Static module frames are entry 128, cleanup 48, get32 32, and probe put32 32
bytes. These exclude outer SDK/firmware callers and are not physical stack proof.
Run durations in the target receipts include boot, verification and shutdown;
they are functional qualification times, not hardware performance measurements.

## Human review on Oryx

```bash
cd ~/Agon/mystuff/golem/docs/tasks/GOLEM-042/qualification/.emulator
./flexe-agon-emulator
```

Expect three red/green squares moving along grey guides for 300 frames. The
middle lane mirrors its phase with `^ 127`; direction flips with `^ 1` and
colour flips with `^ 60` at the edges. Buffered/native captions alternate every
12 frames while sharing the same retained state.

The screen names `animation.golem / step`. That source computes the motion and
colour; [guest.asm](qualification/guest.asm) installs, draws and paces with
VSYNC. The guest gives the keyboard instruction once. Follow it, close the
emulator and report whether the review worked; the agent retrieves the saved
results. No second terminal/verifier command is needed. A rerun removes old
results automatically.

The Author reported "passed" on 2026-10-15. All 270 observations / 2,646 bytes,
all 56,320 final playfield pixels, runtime/fixture identities and actual MOS
return verify. See the [human receipt](qualification/evidence/human-run/receipt.json).
Human acceptance and explicit implementation publication approval are complete
(2026-10-15).

## Reproduction

```bash
make -C src check
.venv/bin/python tests/portable/check_xor.py \
  --compiler build/golemc --sdk ../agon-vdplet --output build/portable-xor
.venv/bin/python docs/tasks/GOLEM-042/qualification/build_packages.py
EZ80ASM=/path/to/ez80asm .venv/bin/python docs/tasks/GOLEM-042/qualification/fixture.py all
```

Add `--relocated` and optionally `--assembler /path/to/ez80asm` to the host
suite. The SDK needs its supported Xtensa toolchain. Oryx staging additionally
needs the pinned accepted runtime and canonical setup helper in the paths used
by `qualification/profile.py`. Stage stock, stock-relocated, hybrid, relocated
and review with that script, then run `qualify.py VARIANT --engine jit|interp`
for silent/windowless automation. Build outputs and profiles remain ignored.
