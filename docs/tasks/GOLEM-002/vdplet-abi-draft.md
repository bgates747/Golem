# DRAFT — vdplet C services and assembly protocol

Use one finite, serialized native entry and a small service table supplied by the
VDP. Assembly owns upload, inputs, invocation and cleanup. Buffer services are
enough for the first proof; scoped framebuffer access is a later optional
capability. This document proposes field layouts for review; it allocates no VDU
opcode and implements no header or parser.

Prepared for [GOLEM-002](../GOLEM-002.md). See the
[feasibility report](native-vdp-modules-feasibility.md) for evidence and limits.

## Native entry and types

ABI v1 is classic ESP32, little-endian, 32-bit pointers, the firmware's **windowed
Xtensa calling convention** and exact pinned compiler family. A future call0 or
other-target module requires a different target identity; no thunk is proposed.
All function pointers follow this ABI. No C++ objects, references, exceptions,
`bool`, `long`, `size_t` or native enums cross the interface. Use fixed-width
integers, ordinary naturally aligned C structs and opaque `void *` host context.
Do not pack pointer-bearing structs. The wire format is separately serialized.

Proposed signatures, shown as a contract sketch rather than implementation:

```c
/* All structs begin with uint32_t struct_bytes. */
int32_t vdplet_entry(const vdplet_host_v1 *host,
                     const vdplet_request_v1 *request);

/* Function pointers in host; ctx is supplied by the host. */
int32_t buffer_acquire(void *ctx, uint16_t buffer_id, uint16_t access,
                       uint32_t offset, uint32_t bytes,
                       vdplet_view_v1 *out_view);
int32_t buffer_release(void *ctx, uint32_t lease);

/* Optional framebuffer capability; absent in the tiny proof. */
int32_t frame_begin(void *ctx, int32_t x, int32_t y,
                    uint32_t width, uint32_t height,
                    vdplet_frame_v1 *out_frame);
int32_t frame_row(void *ctx, uint32_t frame_lease, uint32_t relative_y,
                  vdplet_view_v1 *out_row);
int32_t frame_end(void *ctx, uint32_t frame_lease);
```

The entry has C linkage and is the single ELF entry offset. It is called through
a typed loader adapter, not the existing argc/argv wrapper. Proposed operations
INIT, RUN and FINI are fixed uint16 constants defined by the eventual SDK.
INIT and FINI return finite status; RUN dispatches a module-defined entry ID.
Arguments/results are bytes in declared VDP buffers, not native pointers from MOS.

| Structure | Fields, in order |
| --- | --- |
| `vdplet_host_v1` | struct_bytes:u32, abi_major:u16, abi_minor:u16, capabilities:u32, context:pointer, buffer_acquire:pointer, buffer_release:pointer, frame_begin:pointer, frame_row:pointer, frame_end:pointer |
| `vdplet_request_v1` | struct_bytes:u32, operation:u16, entry_id:u16, argument_buffer:u16, reserved:u16, argument_bytes:u32 |
| `vdplet_view_v1` | struct_bytes:u32, lease:u32, data:pointer, bytes:u32, access:u32 |
| `vdplet_frame_v1` | struct_bytes:u32, lease:u32, format:u32, width:u32, height:u32, row_bytes:u32 |

Under the proposed 32-bit natural layout these sizes are 36, 16, 20 and 24 bytes.
The future header must assert sizes/offsets and pointer width. Callers initialize
output `struct_bytes`; host fills only the known prefix. Reject wrong major or
short required prefixes. Append fields for compatible minor extensions; check
required capability bits and non-null functions. No heuristic ABI compatibility.

Module metadata lives in a fixed serialized envelope: magic, envelope version,
header length, total image length, target/toolchain ABI identity, required service
major/minor/capabilities, expected text/data/BSS/stack limits, module build identity
and ELF offset/length. Exact numeric magic/header offsets are a packaging decision
at Gate A, not invented accepted constants here. Every length is bounded against
the actual upload and the ELF is independently checked. Optional checksum is an
install-time integrity aid, never a per-frame copy verification mechanism.

## Buffer ownership and numeric access

- BUF-001: Assembly pre-creates input/output/argument buffers using stock commands.
  Acquire never creates, resizes or consolidates storage implicitly. Initial v1
  requires one contiguous block and verifies `offset <= size` and
  `bytes <= size-offset`. Access flags are READ or WRITE; unsupported flags fail.
- BUF-002: The host retains the owning block while a view is live. Pointer access
  is limited to the requested span. READ views are immutable by contract; WRITE
  grants only that span. The API cannot enforce this against buggy trusted code.
- BUF-003: An argument buffer is a read-only view for RUN and its exact size is
  validated by the module. Host refuses writable overlap with it and conflicting
  live views. Input/output aliasing is rejected in the tiny proof. No concurrent
  interpreter command may mutate borrowed data. External callbacks/owners must
  respect the same exclusion before a shared integration can be accepted.
- BUF-004: Leases end on explicit release or normal entry return, including a
  reported failure. Pointers must never be saved in persistent module state.
  Cleanup is not a recovery guarantee after native CPU faults or non-return.
- BUF-005: Byte offsets need not align to float32. Numeric code must move bytes
  into aligned local objects before arithmetic; no unchecked float pointer cast.
  Proposed numeric buffers use IEEE binary32 little-endian encodings. Establish
  this with compile-time checks and physical bit-pattern results in the proof.
- BUF-006: No module heap service. Small persistent state uses module data/BSS;
  larger objects remain VDP buffers owned by assembly. Status 0 means success;
  proposed positive fixed codes cover bad arguments, missing buffer, bounds,
  format, ABI/capability, memory, busy, stale handle and unsupported operation.
  Module-specific codes occupy a documented range; numeric values await SDK work.

## Optional packed-framebuffer services

- FB-001: `frame_begin` checks PALETTE2, single buffering, positive dimensions,
  byte-aligned X/width and the whole rectangle within the physical viewport.
  Reject unsupported format/mode before exposing pointers. Initial format is
  opaque MSB-left packed monochrome, with width/8 bytes per requested row.
- FB-002: The host owns `beginUpdate`, draining previous Canvas work without
  vblank waiting, and any required backend lock. One transaction at a time. It
  must use actual controller row mappings, not pretend rows are contiguous.
- FB-003: `frame_row` returns a writable span beginning at X/8 for the requested
  relative row, with capacity exactly width/8. Its lease is subordinate to the
  frame transaction; `frame_end` invalidates every row view and releases host
  coordination. No allocations or new graphics primitives per row.
- FB-004: The module validates its exact-sized single-block source **before**
  opening the frame, then performs the packed copy loop itself. The host must
  preflight row availability so a defined argument/service failure does not
  produce a half-drawn image. No vblank/swap, GCOL, transparency, scaling, pixel
  expansion or full-image scratch allocation is added. Scanout can still tear.
- FB-005: On defined error return the host releases outstanding frame coordination.
  It cannot roll back writes made by arbitrary crashing native code. A framebuffer
  API does not imply a background render scheduler or autonomous sprites.

These services deliberately hide `_VGAController`, Canvas/FabGL classes and the
buffer map. They expose access/coordination, not a firmware implementation of the
complete packed blit.

## State and ordering

| State / operation | Defined behavior |
| --- | --- |
| EMPTY → LOAD | Validate staging image, allocate text/data, relocate, zero BSS, INIT. Publish a fresh handle only after success. |
| LOAD failure | No runnable handle; release partial allocations/views, leave application buffers and any ready module unchanged. Reject LOAD while another module is ready. |
| READY → RUN → READY | Invoke one finite entry synchronously on the command task; publish last status after all views/transactions close. |
| READY → UNLOAD → EMPTY | Run finite FINI, close leases, free image storage, invalidate handle. A defined FINI error is recorded; no resource-retaining background work is permitted. |
| Busy/reentrant request | Reject; do not recursively invoke the module or interpreter. |
| CPU fault / infinite loop | Ordinary firmware failure; no guaranteed recovery, cancellation or safe hot replacement. |

Handle is a nonzero u16 boot-local generation, never an address. Increment on
successful installation; do not reuse on wrap within one boot. Entry IDs are
module-defined u16 values published in the PC manifest/assembly include. Future
named C/Golem wrappers can resolve them at build time; no runtime name lookup.

UNLOAD placed after RUN in the **same ordered command stream** cannot execute
until RUN returns. Assembly may queue that order, but a returned MOS transmit
call does not prove completion. Use an administrative response or a stock general
poll fence before assuming cleanup completed. A missing response means unknown
progress, not permission to free live code by a second channel. No asynchronous
unload, background callbacks or borrowed pointers survive return.

## Symbolic assembly protocol and byte counts

No opcode is allocated. `D` and `X` below are undecided VDU 23,0 selectors in a
user-owned firmware extension, separate from buffered subcommands. Audit the
chosen values against the supported firmware versions before implementation.
Private agon-6502 buffered `0xF0` is not this allocation.

Discovery proposal: a **zero-argument** `23,0,D` produces one small identification
reply on the custom firmware. On explicitly checked stock versions where D is
unassigned, those three bytes are ignored without trailing data becoming VDU
instructions. A host timeout means unsupported/unknown and sends no module
frames. This is not a universal probe for every firmware fork. If no safe selector
can be agreed, require explicit known-compatible firmware identity instead.

After successful discovery only, a module frame is:

```text
23, 0, X, payload_bytes:u16, payload[payload_bytes]
payload common prefix:
    abi_major:u8, operation:u8, request_token:u16, flags:u8, reserved:u8
```

All wire integers are little-endian. Header = 5 bytes; common prefix = 6.
Maximum first-version payload = 32 bytes. The receiver consumes the declared
frame before semantic dispatch, with bounded storage. Unknown operations/ABI
versions fail after consuming their frame. Oversize frames are discarded with
bounded reads under the firmware's existing stream timeout policy, not buffered
into an unlimited allocation. A truncated frame never invokes code; transport
resynchronization must be covered in focused parser checks.

| Operation-specific fields | Request bytes including header/prefix |
| --- | --- |
| LOAD: staging_buffer:u16, envelope_bytes:u32, reserved:u16 | 19 |
| RUN: handle:u16, entry_id:u16, argument_buffer:u16 | 17 |
| UNLOAD: handle:u16 | 13 |
| INFO/STATUS: field_id:u16 | 13 |
| READ16: buffer:u16, byte_offset:u32 | 17 |

LOAD requires an exact single-block staged image and an empty module slot. Host
owns loaded text/data independently afterward; it does not silently clear the
assembly application's staging buffer. RUN initially takes the entire argument
block (bounded, aim <=32 bytes); entry-specific validation decides its layout.
INFO/STATUS exposes version/capabilities, handle, last status and requested memory
counter halves. READ16 is optional diagnostic result retrieval with checked
bounds, not an automatic readback feature in the render loop.

Replies are opt-in via a request flag, except the identification probe. Proposed
keycode-packet payload is exactly ten bytes:

```text
"VDPL"[4], protocol_version:u8, reply_kind:u8, request_token:u16, value:u16
```

Existing packet framing adds code and length: **12 bytes** on the return wire.
Kinds distinguish identification/capability, successful load (value=handle),
operation completion, query data and failure (value=status). Request token
correlates one outstanding administrative request; probe uses token zero. Numeric
results larger than a word use explicit READ16 requests, counted individually.
Ordinary RUN defaults to no reply; status remains in host state/results in buffers.

Assembly installs a keyboard callback before discovery, recognizes the fixed
signature/version, copies into a mailbox and neutralizes normal key handling for
that record. Other keyboard input continues normally. Foreground consumes the
mailbox. Disable the notification producer/drain outstanding administration before
removing the callback. Verify the exact MOS parser and packet suppression policy
before physical deployment; stock MOS modifications are not presumed necessary.
Replies must not bypass application buffer-output routing or accidentally become
captured by a resident output stream. Firmware owns delivery; native modules have
no raw serial/reply API in the first ABI.

Stock WRITE has eight overhead bytes per chunk: `23,0,A0,id:u16,0,size:u16,data`.
Clear is six bytes. Use one WRITE for the first bounded image so it is one block;
chunked uploads would require explicit consolidation and its extra traffic/memory.
A resident wrapper containing a RUN frame is installed once;
its normal stock buffer-call request is six bytes. Input/argument updates,
installation and result retrieval are separate traffic. No recurring code upload.

All of this remains **DRAFT** until Gate A chooses the scope and an implementation
establishes real fields, constants, framing and supported firmware identities.
