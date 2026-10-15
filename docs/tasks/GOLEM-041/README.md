# Native release-error propagation

Native programs now report failed buffer releases instead of silently returning
success, and still attempt the remaining cleanup. The maintained compiler fix
passes injected host failures and fresh Xtensa execution on Oryx. The Author
accepted the three-lane animation review on 2026-10-15, and the retrieved run
passes every saved observation and final playfield pixel. This changes native
cleanup behavior, not animation features or source syntax.

The accepted compiler promotion was committed/pushed as `b1186be`, followed by
this task's plan as `8e3e815`, before implementation. The new implementation and
its qualification were subsequently explicitly approved for commit/push on
2026-10-15, before executing the integer-XOR tranche.

## Behavior

1. Generated native code attempts root then argument release once each. A failed
   release cannot skip the other acquired view, and zero tokens are skipped.
2. The first root/argument exit-cleanup error wins; otherwise the primary result
   is retained. Existing temporary-view release errors remain primary failures
   and can be superseded by an exit-cleanup error. There is no callback retry.
3. Interface/language status is written while root storage is valid. Cleanup
   never accesses root bytes after its release attempt. Earlier source effects
   are retained; the failed destination/pair still follows its existing rule.
4. Successful-generation fields advance before final cleanup. A final release
   error can coexist with advanced success markers, so assembly must also check
   the native runtime result. No rollback or field/schema change is introduced.
5. Outstanding leases after a refused release remain the runtime's normal-return
   cleanup responsibility. SDK, firmware, application renderer and UART code
   are unchanged.

The bounded helper lives in `src/portable/root.hpp`. Maintained conformance lives
in `tests/portable/check_release_errors.py` and `release_host.c`. Production
code/tests import no historical task implementation.

## Qualification

| Check | Result |
| --- | --- |
| Maintained C++ contracts | Pass; legacy checks retained, 52 frozen non-native-C files equal, 35 portable rejection cases plus CLI checks |
| Fault-injected generated C, ASan/UBSan | 80 default + 80 relocated cases pass |
| Normal model/generated C, ASan/UBSan | 383 default + 383 relocated cases pass; statuses 0,1,2,3,4,5,6,32,33 observed |
| Actual assembly consumer | Pass |
| Xtensa arithmetic, JIT and interpreter, default and relocated | 52 calls / 1,569 observations / 15,690 bytes per run pass |
| Xtensa animation, JIT and interpreter | 300 calls / 382 observations / 3,766 bytes and all 56,320 final playfield pixels per run pass |
| Actual MOS return | Pass in all six completed target runs |
| Human review | Accepted 2026-10-15; saved results, final pixels, identities and MOS return verified |

The fault suite covers normal success, source arithmetic failure, invalid root
schema, root-acquire failure, wrong root length, temporary-array preflight/read/
write release failure, argument-acquire failure and pre-acquisition rejection.
Each runs with neither/either/both final releases failing, and with callbacks
that either release despite returning error or refuse release. Poisoned root/
argument storage detects access after a release attempt. Exact states, prior
effects, cleanup order, return precedence and outstanding leases are checked.
These are host-injected failures, not claims of firmware fault injection.

All 58 provenance-labelled frozen source/output files remain unchanged, including
the three historical native-C outputs. Current native C intentionally differs;
the other 52 emitted artifacts retain parity. See [host receipts](evidence/host.json)
and [target receipts](qualification/evidence/). Only the native root emitter
changes in production; layout and buffered commands are preserved.

Target elapsed times, including startup, verification and shutdown, were
7.287/8.022 seconds for default JIT/interpreter, 7.291/8.025 for relocated,
and 16.573/33.792 for the animation. These are functional run durations, not
hardware performance measurements. One interpreter animation attempt stalled
during startup before fixture execution; its [log and receipt](qualification/evidence/review-interp-boot-stall/receipt.json)
are retained. A clean retry with identical runtime/fixture bytes passed. No
emulator/UART repair or timing workaround was applied.

## Packages and provenance

Qualification uses unchanged SDK commit
`9fbb21dd09402e2a3835067dbb4427f2c20dec3c` and its actual image validator.
The module build ID is `0x474c3431`; declared limits are 32 KiB text, 4 KiB data,
2 KiB module stack and 64 KiB image. [Package receipts](qualification/packages.json)
pin generated C, policy, ELF layout and package hashes.

| Module | Image bytes | Resident text | Resident data |
| --- | ---: | ---: | ---: |
| Probe | 4,188 | 2,880 | 268 |
| Relocated probe | 4,192 | 2,884 | 268 |
| Animation | 6,348 | 5,052 | 268 |

The compiler reports static frames of 128 bytes for entry, 48 for cleanup and
32 each for get32/put32. These figures exclude firmware/SDK callers and are not
a complete physical stack proof. Public SDK header SHA256 is
`fa8385e2906f07d37372d80c40f73d419defdbefe7a3458461ac9d5fe0820d38`.

Oryx uses accepted GOLEM-037 runtime bytes: frontend `28f50e87…`, adapter
`200f28cb…`, firmware `8da2ebfd…`, ELF `b41e4cb3…`. Full hashes and every
fixture/launcher/MOS identity are retained in each target receipt. No physical
target was flashed. Every automated run forced both dummy video and dummy audio;
no graphical or audible instance was launched by the agent.

The finite fixture/guest/model adapter derives from the accepted
GOLEM-036/right-shift qualification at `b32c596`, adapted to the maintained
compiler and current unchanged SDK. The model adapter imports maintained
`tests/portable/command_model.py`, not the historical compiler. Profiles are new
and task-owned, leaving accepted profiles intact. Staging uses the canonical
`agon-dev-env/scripts/setup_emulator.py`; an exact copy was transferred into
ignored build storage because that script was absent on Oryx. No canonical
environment file was modified.

## Human review on Oryx

```bash
cd ~/Agon/mystuff/golem/docs/tasks/GOLEM-041/qualification/.emulator
./flexe-agon-emulator
```

Expect three red/green squares moving along grey guides for 300 frames, with
buffered/native captions alternating. This is the previous motion example
recompiled with the cleanup fix. [animation.golem](qualification/animation.golem)
computes all coordinates and colours; [guest.asm](qualification/guest.asm)
installs, draws and paces with VSYNC. The screen identifies both roles.

When rerunning the review, follow the onscreen instruction to type `dir`, close the emulator, and report whether
the animation and prompt worked. The agent retrieves/verifies the saved results;
no second terminal or verifier command is required. The wrapper removes old
results before a rerun and defaults to JIT.

The Author reported "passed" on 2026-10-15. Retrieved evidence matches all 382
observations / 3,766 bytes and all 56,320 final playfield pixels. Runtime/fixture
identities and actual MOS return pass; the emulator is stopped. See the
[human receipt](qualification/evidence/human-run/receipt.json). Human acceptance
and implementation commit/push approval are complete as of 2026-10-15.

## Reproduction

```bash
make -C src check
.venv/bin/python tests/portable/check_release_errors.py \
  --compiler build/golemc --sdk ../agon-vdplet --output build/portable-release
.venv/bin/python tests/portable/check_integration.py \
  --compiler build/golemc --sdk ../agon-vdplet --output build/portable-integration
.venv/bin/python docs/tasks/GOLEM-041/qualification/build_packages.py
.venv/bin/python docs/tasks/GOLEM-041/qualification/fixture.py all
```

Use `--relocated` for both maintained Python suites and optionally
`--assembler /path/to/ez80asm` for integration. Fixture assembly needs ez80asm
at the normal user-local path or an explicit `EZ80ASM=/path/to/ez80asm`.
Target profiles additionally need the pinned accepted Oryx runtime and exact
canonical setup helper in the paths used by `qualification/profile.py`.
After deployment, stage `hybrid`, `relocated`, and `review` with that script,
then use `qualification/qualify.py VARIANT --engine jit|interp` for bounded,
silent headless checks. Source/fixtures are reproducible task artifacts; build
directories and emulator profiles remain ignored.
