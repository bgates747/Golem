# Native Golem first research tranche

GOLEM-023's first research tranche recommends a native-only source module,
translated to C and packaged through the separate VDPlet SDK. The review centres
on module-level selection, authoritative VDP-buffer globals and a small initial
numeric/control-flow subset. No new backend or runtime service is implemented.

The [report](../tasks/GOLEM-023/README.md) includes initialized source, named
assembly exports, caller-owned Function parameter/result storage, handle binding,
state lifetime and recurring wire costs. OPT-001, LNG-001 and ABI-001 are complete
as research proposals; Author acceptance and further research remain open.

A pinned GCC 8.4 compile-only inspection confirms native float add/subtract/
multiply and comparison instructions. Division requires __divsf3; builtin floor
and sqrt require floorf/sqrtf. The SDK's zero-import profile therefore needs
separately qualified support before these become native source features.
Per-function compiler frames were 32 bytes; these are not full call-chain bounds.
The receipt and reproduction script remain in the task silo.

Before research, all pending work was committed and pushed: Golem checkpoint
97c1651 and VDPlet Runtime checkpoint 33a554c. The latter is in the new private
`bgates747/agon-vdplet` GitHub repository. Research artifacts produced after that
checkpoint remain uncommitted for review. No emulator was launched and no hardware
was flashed. Physical performance and outstanding human reviews remain unclaimed.
