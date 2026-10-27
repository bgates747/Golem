# Maintained stock perspective primitives

The compiler now encodes the independently proved stock perspective route for
wireframe's heavy-tank/Corsair experiment. Existing package bytes stay unchanged.
Host and assembler checks pass; independent application execution and human
review belong to wireframe. Changes remain uncommitted pending review and
publication approval.

1. The actual Author request was verified in wireframe's chat. Mailbox
   implementation-one, scope-one, complete-proof-three, interface-two and
   source-ready-four establish ownership and the accepted bounded syntax.
   GOLEM-049 was planned before implementation.
2. New maintained support:3x3/4x4 matrices with Identity/exact matrix_values,
   dimensional affine operations,3x3 inversion, typed buffer-fetched matrix
   values, packed XYZF32/XY16 records, static borrowed-buffer byte copies and
   finite parameterless Sub calls. Existing limits remain:1024ops/Sub,
   64Subs,65535commandbytes/block; call cycles/depth>4 reject.
3. A shared bulk matrix computes viewport numerators. Per-vertex four set-value
   calls restore/build A, inversion computes quotients and a zero XY16 point
   selects float inverse translation with final stock integer conversion. No
   implicit record narrowing or host per-vertex floating-point arithmetic.
4. Fsim's unchanged-stock2.16 proof reports six final-conversion cases and
   repeated A replacement. Negative output clamps to zero rather than signed
   truncation. The first fixture uses positive depth/positive final coordinates;
   singular/near-plane/clipping behavior is not generalized.
5. Maintained tests reproduce the exact93-byte independent stock sequence.
   All68files across six legacy packages, including load plans, remain identical.
   Regression tests/51matrix rejections, ASan/UBSan, two actual assembler
   consumers and rejected-output preservation pass. Load-plan metadata now uses
   declared3x3/float-record byte sizes instead of hard-coded4x4/XYZ16 widths.
6. A2685vertex generated source compiles into21chunks and one parent, with
   290447installationbytes and13824bytes in the largest entry. This is a source
   capacity check, not a memory/performance claim. Both actual peer sources
   compile read-only; their hashes and outputs are retained separately.
7. Compiler-handoff-three publishes the executable/source hash pins to fsim
   through the durable mailbox. No peer source/output/profile was mutated;
   no graphical/target emulator or hardware was launched. REV-001 remains
   open for independent application qualification, human acceptance and direct
   publication approval. Generic VDPlet runtime is untouched.

[Task](../tasks/GOLEM-049.md), [report/example/evidence](../tasks/GOLEM-049/README.md),
[maintained contract](../design/stock-matrix.md).

## Independent stock execution and MOS transport follow-up

Fsim generated-target-five reports both complete Golem-generated mesh streams
pass unchanged-stock2.16 VDP execution, six poses each.32,580XYcomponents match
the independent double perspective within one logical unit, with raster
comparisons under a one-pixel boundary tolerance. Compiler hash is unchanged;
[peer-target-evidence.json](../tasks/GOLEM-049/peer-target-evidence.json) preserves
the reports. This qualifies the generated mechanism, not full MOS/human review.

The actual Pi MOS client stalls reproducibly381735bytes into805668-byte Corsair
setup before any projection invocation. The same complete setup runs VDP-only;
no compiler arithmetic failure is shown. Runtime-guidance-four identifies the
retained Oryx stock pair (frontend7e78e991/adapter200f28cb), independently read
without modifications/launch, and separates it from old mixed Pi artifacts,
the accepted Golem native pair and the current central build. No current Pi
large-upload pair or repair is claimed. Fsim owns the same-fixture Oryx test;
FX-006's known UART race is context, not attribution of this new stall.


## Full MOS consumer checkpoint — supersedes the transport follow-up

Fsim deployed-eight reports successful full applications on Pi and Oryx, both
meshes, all controls/reset/repeat, clean exits and handled missing-setup exits.
The same stock firmware/runtime remains; bounded opaque96-byte acknowledged
installation avoids continuous ingress without modifying the projection stream
or claiming a diagnosed UART repair. Prior orthographic/runtime inputs verify
unchanged; both Oryx initial images match Pi. GOLEM-049/peer-mos-evidence.json
retains source provenance and Oryx observations. Human visual acceptance and
direct publication approval remain pending. No compiler/runtime change follows
from this evidence update.
