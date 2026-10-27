# GOLEM-049 — Stock matrix primitives for perspective projection

Extend the maintained stock compiler just enough to encode the independently
proved perspective sequence. The host supplies controls and installed typed
buffers; stock VDP performs bulk float transforms and per-vertex inversion.
Wireframe owns its application, meshes, deployment and independent target/human
review. Compiler changes stay here, uncommitted pending the review/publication gate.

Author request verified in the wireframe chat on 2026-10-25: produce the described
stock perspective demo for heavy tank and Corsair. Mailbox proof-three supplies
an actual unchanged-stock2.16 93-byte inversion/conversion sequence, six final
conversion cases and the observed negative-to-zero conversion limit.

- PLN-001 [x] Record scope, actual-stock proof, ownership and finite source interface before implementation.
- CMP-001 [x] Add initialized3x3/4x4 matrices,3x3 inverse, typed buffer-fetched float operands, XY16/XYZF32 records and bounds-checked literal byte copies between declared borrowed buffers.
- CMP-002 [x] Support finite parameterless Sub calls so the consumer can generate bounded vertex chunks and invoke the full sequence once. Reject cycles, missing entries and excessive call depth; retain existing per-entry/ID bounds.
- CHK-001 [x] Preserve legacy matrix outputs byte-for-byte and run maintained tests, sanitizer checks, actual assembler consumers and load-plan shape/ownership checks.
- DOC-001 [x] Supply initialized examples, source/manifest/encoding contract and retained proof hashes; hand off to wireframe for its independent stock run and human staging.
- REV-001 [ ] Record the consumer's independent qualification/review and direct publication approval. No physical operations or agent graphical launch.

## Bounded interface

1. `Matrix(3,3) a = Identity` and `Matrix(4,4) clip = matrix_values(...)`.
   Literal initializers supply exactly dimension-squared finite float32 values.
   Existing4x4 identity outputs remain unchanged.
2. `Borrowed XYZF32 camera = Buffer(id,count)` uses packed12-byte triples;
   `Borrowed XY16 zero = Buffer(id,count)` uses4-byte pairs. Existing XYZ16
   remains6-byte triples. Entire borrowed buffers are host-initialized single
   blocks; declarations neither upload nor clear them.
3. `matrix_set_value(a,base,row,column,read_float32(camera,byteOffset))`
   copies a matrix and changes one element through stock command34/op0x21.
   Constants/global float controls are also valid final operands.
4. `matrix_invert(a)` is limited to3x3 and emits command32/op1. Existing4x4
   inverse is not qualified by this work. `matrix_identity`, dimensional
   rotate/scale/translate, multiplication and transform remain stock operations.
5. `transform_vertices(dest,m,source)` requires matching record type/count and
   matrix dimension. Format0 preserves float XYZ records; formatC0 preserves
   signed16 XY/XYZ. It never implicitly converts record formats.
6. `copy_bytes(dest,destOffset,source,sourceOffset,bytes)` copies declared borrowed
   storage using command5/E2. Literal spans must fit; unsafe overlapping self
   copies reject. The host installs/initializes any retained destination fields.
7. `chunkName()` calls a declared parameterless Sub. Generate chunks within the
   existing1024-operation/65535-byte limits and aggregate with finite calls;
   no new raw VDU escape, private compiler or runtime index language is needed.

## Limits

The quotient workaround assumes finite representable inputs, nonsingular
matrices and strictly positive depth. Final positive endpoints truncate under
the qualified stock firmware; negative values clamp to zero in the peer's
actual-stock proof. Do not promise signed truncation, clipping, error status,
rollback or unchecked negative-output portability. Use the consumer's positive
logical-screen domain. Generic compiler host checks do not establish target or
physical performance. Existing stock/native demos and generic VDPlet runtime
remain unchanged.

## Compiler checkpoint — 2026-10-25

Maintained implementation and host qualification are complete; [report](GOLEM-049/README.md).
The exact93-byte stock proof is reproduced. All68legacy package files remain
unchanged. Maintained regressions/51matrix rejection cases, ASan/UBSan, two
actual assembler consumers, load-plan ownership/sizes and2685-vertex chunking
pass. Both peer mesh sources compile read-only; no peer output/profile changed.
Mailbox compiler-handoff-three transfers independent application/target/human
qualification to fsim. REV-001 remains open; no publication or physical action.


- TGT-001 [x] Fsim independently executed the Golem-generated stock streams for
  both meshes, six poses each.32,580XYcomponents match the independent perspective
  reference within one logical unit; raster comparisons pass with no missing/
  extra pixels under the declared one-pixel boundary tolerance. Compiler hash
  matches the handoff. [Peer receipt](GOLEM-049/peer-target-evidence.json).
  Full MOS large-upload and human gates remain open; no physical claim.

## Consumer transport follow-up

The consumer reports a reproducible Pi full-MOS stall during the805668-byte
Corsair setup, before projection executes; the same complete stream and poses
pass VDP-only execution. Runtime-guidance-four identifies the independently
read, preserved Oryx stock runtime pair and existing qualification boundaries.
No current qualified Pi805668-byte MOS pair is claimed. This is not evidence
of a compiler arithmetic failure. No adapter, firmware, peer application or
profile was changed; fsim owns its same-fixture Oryx comparison.


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

## Publication checkpoint — 2026-10-27

The Author explicitly authorized committing and pushing all pending Golem work
in logical groups. Compiler publication is released. Wireframe-owned human
review remains a separate open evidence item; no new consumer or physical
qualification is inferred from this authorization.
