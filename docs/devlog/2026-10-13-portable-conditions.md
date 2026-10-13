# Golem-controlled reversal on buffered and native targets

Checkpoint 60a4363 committed and pushed all prior dirt as explicitly requested.
GOLEM-026 then added task-local uint16 comparisons and structured branches to
both executable backends. Automated stock and actual-Xtensa checks pass; a
single-tick visual review is staged/stopped on Oryx. This new tranche remains
uncommitted for human review and explicit approval.

The fork preserves GOLEM-024/025 artifacts and the maintained compiler. Comparison
operands are checked left to right; ElseIf and branch bodies evaluate only when
reached. Byte-based lexicographic comparisons avoid the stock 65535 sentinel.
Jump-connected straight-line blocks preserve finite whole-invocation failures,
including condition arithmetic and Bounds errors after earlier assignments.

The complete meter.golem now has one tick Sub and a persistent uint16 direction.
It reverses at phase 0/96 itself. Assembly calls tick on every frame, selects
buffered/native execution and invokes resident drawing plus VSYNC; it no longer
selects grow/shrink. The layout grows explicitly from 18 to 20 bytes. Buffered
installation is 4,558 bytes; native package is 1,484 bytes, including 572 resident
text bytes and four data bytes. No runtime or SDK source changed.

Host ASan/UBSan and model/generated-C checks pass 2,236 cases across default and
relocated packages, plus eight grammar/reference acceptances, 26 rejections,
native ELF admission and maintained contracts. Selected-branch Bounds failure
and valid-but-step-misaligned meter inputs are included. Stock VDP qualifies 89
conditional calls and 26 tick/bounds calls. Each actual Xtensa engine qualifies
178 conditional calls, 114 relocated calls and the 600-frame alternating demo.

The visual matches 533 observations/5,249 bytes and all 56,320 final playfield
pixels with actual MOS return. Oryx JIT/interpreter review runs take about 19
seconds including boot/loading; stock condition checks take about 3.2 seconds.
These are functional emulator measurements, not physical performance claims.
No board was flashed. Source, report and a final-screen reference are deployed
beside the dedicated Oryx profile; no graphical emulator was launched by the agent.

The Author accepted the visual review with “looks good”. Retrieved human results
match all 533 observations/5,249 bytes, and staged runtime identities still pass.
Keyboard success was not separately stated. Work remains uncommitted pending
explicit commit/push approval.

The Author explicitly authorized commit and push on 2026-10-13. GOLEM-026 is
accepted and leaves the authoritative TODO register. Its implementation remains
task-local; this checkpoint does not promote production code or claim a separately
reported keyboard test.
