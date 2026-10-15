# Flower integration feasibility

This report records the first feasibility tranche: a real C++ consumer links
at two buffer layouts and a mathematical module admits through the unchanged
loader. The [subsequent native implementation](native-math/README.md) now also
qualifies mathematical execution under actual ESP32 firmware. The [resident renderer](renderer/README.md) subsequently qualifies connected
lines from resident arrays with actual Pi JIT/interpreter pixels. The complete
interactive client remains ahead.

## C/C++ interface proof

[abi_probe.golem](abi_probe.golem) uses existing supported syntax and initializes
all seven Flower inputs plus a batch counter. Its two named Subs only reset or
increment that counter. It deliberately does not claim to calculate Flower.

[generate_header.py](generate_header.py) derives buffer IDs, field offsets,
widths, entry IDs, root-status offsets, native handle patch offset and package
lengths from `interface.json`, `root.json`, `backends.json` and the emitted files.
It generates explicit little-endian setters; float bits use memcpy instead of
aliasing/packing an eZ80 struct over the VDP state.

The resulting calls are `golem_reset(backend)` and `golem_step(backend)`. A tiny
C-linkage host adapter submits their generated entry buffer using AgonDev's
`vdp_adv_call_buffer()`. As with assembly wrappers, returning means submitted,
not completed. The host must install storage/private code first; native use also
requires loading the module and binding its fresh handle. Setters prepare bytes;
they do not upload them. Completion/status and real input installation remain
host transport work.

[check_abi.py](check_abi.py) verifies:

1. Real AgonDev C++ compilation/linking at default and reserved 1..4095 layouts;
   installed headers establish 24-bit int/pointers, 16/32-bit fixed integer
   types, binary32 float and a six-byte buffered call structure.
2. Host C and C++ sanitizer consumers check known 22.5 float bytes, neighbour
   preservation, uint16 packing and named dispatch to either backend.
3. Buffer relocation changes state/entry IDs while the unchanged C++ source
   still links. Input offsets and call IDs come from the current package.

See [abi.json](evidence/abi.json), the [generated example C header](evidence/default-flower_golem.h)
and [C++ consumer](client/src/main.cpp). No executable from this proof was
installed or run. Production emission and complete transport are not promoted.

## Native mathematical libraries

The unchanged SDK builder uses `-nostdlib` and does not link mathematical or
compiler support libraries automatically. A real float division produces an
undefined `__divsf3`; sinf/cosf produce two more imports. Firmware's possession
of these functions does not resolve them in an uploaded module.

[probe_math.py](probe_math.py) compiles a small ordinary C module with the pinned
Xtensa GCC 8.4 toolchain and tests three link variants against the actual SDK
ELF loader under host ASan/UBSan:

| Variant | Outcome |
| --- | --- |
| Existing SDK link flags | Rejects at link: __divsf3, sinf, cosf undefined |
| Add `-lm -lgcc`, export only vdplet_entry | Links with no undefined imports; loader accepts; 5296-byte package, 3296 text / 996 data bytes, 8 relative relocations |
| Also add `-Bsymbolic` | Also admits, but larger; unnecessary for this probe |

The narrow export list is retained. No firmware addresses, new math callback,
runtime ABI or loader relaxation were used. Recommend a narrowly scoped SDK
builder option for linked module math libraries, disabled by default to preserve
accepted package bytes. Qualify division/sin/cos/floor generated code and the
library's execution, relocation across separate IRAM/DRAM, stack high-water and
accuracy on actual Xtensa engines before using it for Flower. The declaration
of a 2048-byte stack envelope is not measured library-stack evidence; only the
probe entry's own 32-byte frame was reported by -fstack-usage.

The installed libraries bring their own ordinary argument-reduction/constants;
these are standard sinf/cosf implementation details, not an application-level
coarse sine table or phase representation. The resource numbers above describe
the small probe, not the final generated Flower module.

## Stock mathematics boundary

Stock affine command 32 operation 3 consumes radians and invokes firmware
sinf/cosf, storing cosine at matrix byte offset 0 and sine at offset 4. This is
a promising lowering route after enabling affine support and preventing an
unwanted combination with an existing transform. It still needs real command
qualification and an explicit cross-target accuracy contract.

Floating division remains unresolved. Generic matrix command 34 has no inverse
operation; affine inverse operates on 3x3/4x4 matrices. A proposed 1x1 generic
inverse is therefore not an implemented stock command. Reciprocal followed by
multiplication can round differently from direct division and is not a drop-in
implementation of the agreed quotient. Do not certify it as portable division
without solving and qualifying that difference.

Recommend implementing the native mathematical slice first, with an explicit
target capability boundary that rejects unsupported stock compilation. Keep
one frontend, source and storage layout. Do not fork a Flower dialect or change
the meaning of existing `--portable` output. Native-only target selection and
eventual stock division remain explicit work under MATH-001; no full portability
claim follows from this probe. Floor can reuse the earlier qualified resident
algorithm but needs portable expression/frontend integration.

## Drawing and batch design

The smallest useful application follows the already-qualified numeric/renderer
separation: Golem calculates at most 32 points into two resident sint16 arrays;
a stock VDU renderer consumes the arrays and connects their valid prefix.
Its start coordinate and count live in the same shared layout. The C++ client
transfers parameters and calls batches, never coordinates per point.

[flower.golem](flower.golem) is the complete initialized design prototype.
It uses current finite Range/If/array syntax plus the four mathematical
operations now implemented for explicit --target native. `prepare`
checks derived initialization before any screen clear; `batch` preserves angle,
radius and previous point across calls. It computes exactly the original
floor(Q)+1 segments, with no padded final segments or automatic closing line.

The Flower renderer has an explicit logical-coordinate contract. It emits an
initial PLOT move and connected PLOT lines, using the stock coordinate setup
from the audit. It does not change the existing resident drawing profile's
pixel-coordinate requirements. Begin with this bridge rather than requiring
general Golem Functions, floating arrays or Canvas services.

Budget a steady 6-byte compute call plus a 6-byte renderer call per batch,
before completion/status traffic. This is 12 outbound bytes for up to 32 new
segments; it is an interface budget, not a measured complete wire total.
Installed points and drawing command bytes stay on the VDP. Batch size is a
starting value; measure responsiveness, installation size and actual wire
traffic in qualification. No VSYNC pacing or concurrency dependency is needed
for drawing a static curve.

The renderer must check successful computation/root status before using new
points. A failed batch may leave earlier state effects; draw only a deliberately
committed valid prefix or suppress that failed batch, without rollback/replay.
For the first implementation, suppress a failed batch and leave the preceding
image. After successful preparation, drawing starts only after the client's
single clear. Future direct draw_line statements can use the same shared state
and lifecycle after this smaller application is accepted.
