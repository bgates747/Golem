# R25-G0/root-v1 — Accepted D0 interface

The peer accepted this 112-byte header and its semantics without amendments.
GOLEM-032/25901e5 is the committed compiler pin; the original proposal and
acceptance receipts preserve the negotiation history. D0 adds a separate
envelope and retains the committed scalar/float grammar.

Root is packed little endian. Header is 112 bytes; source globals follow it.
Existing language status moves to header offset14. One argument word still
identifies the root buffer; source Sub names map to generated logical entry IDs.

| Offset | Field | Bytes | Owner/meaning |
| --- | --- | --- | --- |
| 0 | magic | 2 | fixed 0x4752 |
| 2 | schema | 2 | fixed 1 |
| 4 | header_bytes | 2 | fixed 112 |
| 6 | root_bytes | 2 | generated exact installed total |
| 8 | session | 4 | host; opaque nonzero identity |
| 12 | interface_status | 2 | envelope validation, separate from language |
| 14 | language_status | 2 | existing compiler status semantics |
| 16 | runtime_result | 4 | host-owned signed SDK/transport result; Golem does not infer or publish it |
| 20 | last_success_generation | 4 | wrapper increments modulo 2^32 only after successful source return |
| 24 | last_success_entry | 2 | wrapper; initial 65535 |
| 26 | descriptor_count | 2 | fixed 4 |
| 28 | requested_entry | 2 | named wrapper writes generated ID; generic dispatch validates it |
| 30 | reserved | 2 | fixed zero |
| 32 | last_success_session | 4 | wrapper copies session only on successful return |
| 36 | entry_count | 2 | generated exact count |
| 38 | globals_bytes | 2 | generated source-global bytes |
| 40 | reserved | 4 | fixed zero |
| 44 | contract_tag | 4 | fixed 0x52323530 (R25-G0 family) |
| 48 | four descriptors | 64 | ordered roles below |

Each descriptor is 16 bytes: role:u16, buffer:u16, offset:u32, length:u32,
access:u16, reserved:u16. Roles are 1=input snapshot (read), 2=event mailbox
(read/write), 3=table data (read), 4=asset-directory metadata (read). The asset
role is a directory/view binding, not a claim all pixels fit in one buffer.
Buffer0 means explicitly unbound, with offset/length/access0 and role retained.
Bound descriptors are compile/build-time bindings declared in an external JSON
manifest; installed root descriptor bytes must match it exactly. IDs/reservations,
lengths/offsets/access are published, not guessed by adapters. Changing a binding
requires recompile/rebind while quiescent, not arbitrary live descriptor writes.
This is deliberately a first fixed-binding profile; dynamic views are later.

Native preflight validates actual root length and each declared bound range by
acquiring/releasing one extra view at a time. Stock cannot inspect actual lengths:
assembly must install the exact buffers/ranges and lifetime promised by the
manifest. Both check the declared header/bindings before source effects. Native
acquisition failure is an interface/runtime rejection, not a language arithmetic
error. Declared descriptor guards do not claim stock physical-buffer validation.

Interface statuses: 0 accepted, 1 magic, 2 schema, 3 shape/count, 4 session zero,
5 binding mismatch, 6 reserved/contract-tag, 7 unknown requested entry, 8 actual
bound-range acquisition failure (native only). Native request IDs outside the
exported SDK dispatcher return VDPL_E_UNSUPPORTED without acquiring/mutating root.
A physically too-short native root returns VDPL_E_BOUNDS without unsafe writes.
For direct SDK invocation, request.entry_id must match root.requested_entry; a
mismatch is interface status 7. Named resident wrappers set that field themselves.
A usable root with rejected metadata changes only interface diagnostics (and the
named wrapper's requested-entry field); source globals, prior language status
and all last-success fields remain unchanged. A well-formed invocation clears
language status as before; a language error may retain earlier source effects
but does not publish a new success generation/entry/session. No rollback claim.

runtime_result is deliberately written by the invocation/transport owner after
observing completion. Buffered calls alone provide no SDK return code; zero is
not a substitute for language_status or last-success checks. D0 leaves this field
untouched and tests that behavior. Generation/session fields are opaque protocol
metadata, not source-level uint32 support. D0 demonstrates wrap explicitly;
production episode/wrap/rearm policies still belong to the later game/runtime
sub-contract.

D0 fixture exposes InitRace, Tick120, Render30, ResetRace and StopRace, all labelled
scalar/synchronous probes. It uses placeholder retained input/event/table/asset-
directory buffers solely for binding admission/lifetime checks. It does not
interpret player packets, publish collision events, simulate opponents or claim
120/30 Hz scheduling. Assembly drives a finite visual review and VSYNC.

Bindings and source fields are generated in each delivered root.json/interface.json;
use delivery-manifest.json and state.bin together. The scalar fixture uses 126 total
bytes and five entries; neither value is a universal ABI constant.

The native module checks physical root length before writes. A resident named
wrapper first writes requested_entry, then invokes that module. The physically
short-root qualification therefore uses direct SDK dispatch to test the module
admission boundary. Native lease admission is not snapshot synchronization.

All source globals undergo the existing entry admission, including InitRace and
ResetRace. Those routines cannot repair an invalid external source value: the
host must restore admitted state or reinstall the generated initial bytes.
Changing session does not implicitly reset source state or success generation.
The event read/write descriptor uses SDK WRITE acquisition (value 2), whose view
is readable and writable; descriptor access 3 does not pass an unsupported SDK
access mask. One additional bound lease is released before the next is acquired.
