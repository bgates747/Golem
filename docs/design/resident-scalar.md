# Maintained resident scalar profile

`make -C src check` builds and checks the host compiler. Raw mode remains
`build/golemc source.golem payload.bin`. Resident mode is explicit:

```bash
build/golemc --resident examples/resident/counted-floor.golem build/counted-floor -0.75
```

The maintained [floor example](../../examples/resident/floor.golem) and
[counted-floor example](../../examples/resident/counted-floor.golem) declare
and initialize their inputs. This profile accepts one finite `float32` input
with a two-endpoint `Range`, one `sint16` output and one `Sub` containing exactly
one `result = sint16(floor(x))`. Optional persistent `uint16` globals must precede
the float declaration. The job can assign/copy/add these globals and print
literal strings before or after the conversion. Resident loops, local variables,
general float expressions and additional jobs are not implemented.

Keywords and builtins ignore case; identifiers retain case. Statements have no
terminators and newlines are whitespace. `print` emits literal bytes, including
LF for `\n`; use `\r\n` when a display needs carriage return too.

## Package and host contract

The directory contains five files:

- `bootstrap.bin`: VDU command stream that installs buffers and initializes state.
- `invoke.bin`: six-byte resident entry call, reusing the installed input.
- `update-call.bin`: example input update followed by entry call, 21 bytes total.
- `cleanup.bin`: clears every owned buffer; send after the last invocation.
- `interface.json`: schema 1, profile `resident-scalar`, input/output/global
  locations, ownership, declared bounds and lifecycle requirements.

Send bootstrap once, then update/call for each new input. Read the signed
little-endian result from the manifest's output location using the host's
chosen transport; no result transport is added by this compiler. Persistent
globals survive calls and reset on reinstall. The pure example installs 1008
bytes across 25 buffers; counted-floor installs 1120 bytes across 28 buffers.
The fresh-input/call traffic is 21 bytes for either example, excluding result
retrieval and transport framing.

The host must validate finite, representable float32 inputs against the declared
bounds, both before and after float32 rounding. Bounds must keep `floor(input)`
within sint16. The CLI validates its example input; it does not install runtime
admission checks for later host updates. Calls and updates must be serialized:
the self-modifying buffers are nonreentrant. Finish each call before updating,
reinstalling or cleaning up. Cleanup must precede reusing owned buffer IDs.

Reserve external buffers with repeated inclusive `--reserve FIRST:LAST` options
before the source argument. IDs 0 and 65535 are always unavailable. Overlapping,
invalid or exhausting reservations fail compilation. The allocator relocates all
owned buffers and records the resulting layout in the manifest. The host remains
responsible for declaring other owners and respecting the published ownership.

Semantic validation completes before output writes. Filesystem publication is
not atomic if an I/O operation fails. Raw payloads and resident packages have
different installation contracts; mode is never inferred from source.

## Evidence

[GOLEM-008](../tasks/GOLEM-008.md) promotes the qualified GOLEM-006 implementation.
Its checks compare all four command artifacts for each of the four GOLEM-007
review packages against their recorded hashes. Only manifest provenance changes
from `experimental-resident-scalar` to `resident-scalar`, with the historical task
field removed. The stock Fab 1.2.5/MOS 3.0.2 qualification and successful Oryx
keyboard review therefore apply to the same command bytes. Physical hardware
and other firmware versions remain outside that evidence.
