# Flower reference and integration tranche

Flower's original logic and interactive requirements now have concrete source
and interface prototypes. C++ can submit named Golem calls, and ordinary
Xtensa math libraries can fit the current loader without firmware-address
imports. The runnable application and target math qualification remain ahead.

The Author approved clearing on exit as an exception so the original mode can
be restored, then authorized publishing all pending work and proceeding with
GOLEM-044's first tranche. Accepted XOR implementation is published as
`850c3f5`; research/Flower plans as `7b9e07f`, both pushed.

REF-001, UX-001 and feasibility items FEA-001..003 are complete. The original
loop is inclusive (396 segments at defaults); negative coordinate conversion
uses floor. The float32 model's default separation from binary64 is 0.108 pixels,
while long runs expose significant accumulated angle error. Preserve the
original logical scaling rather than treating radius 512 as physical pixels.

Real AgonDev C++ compile/link and host C/C++ sanitizer packing/dispatch checks
pass at two layouts. A libm/libgcc-linked Xtensa probe admits through the actual
unchanged SDK loader under host sanitizers: 5296 package bytes, 3296 resident
text / 996 data bytes and eight relative relocations. This does not establish
Xtensa execution, library stack usage, math accuracy or complete portability.

The proposed batch renderer consumes resident arrays; C++ never calculates or
streams per-point coordinates. Native replies and keyboard input require one
demultiplexing MOS callback; installing AgonDev's keyboard hook independently
would overwrite the transport hook. Screen input re-entry must preserve the
curve; exit restores the captured mode with its permitted clear.

Next is the native float division/sin/cos/floor implementation and qualification,
with an explicit target capability boundary. Stock trig has a source-backed
candidate; direct rounded stock division remains unresolved. No emulator,
physical deployment, production compiler or SDK source changed this tranche.
See the [report](../tasks/GOLEM-044/README.md), [audit](../tasks/GOLEM-044/reference-audit.md)
and [initialized prototype](../tasks/GOLEM-044/flower.golem).
