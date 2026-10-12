# Maintained resident scalar profile

Resident uint16 addition now checks overflow and ends a failed job without
changing that assignment's destination. Host checks and stock Fab/MOS execution
pass, and the Author confirmed GOLEM-012 review on Oryx worked. Fresh-input/call traffic
remains 21 bytes. Broader numeric operations remain proposed.

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
LF for `\n`. The reader also accepts escaped quotes and backslashes; it does
not currently recognize a `\r` escape. LF alone does not return the cursor to
the left margin.

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
bytes across 25 buffers; counted-floor now installs 1238 bytes across 32 buffers.
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

## Checked addition and job errors

Resident uint16 sums must stay within 0:65535. Every addition in a chain checks
carry before proceeding, so an early overflow cannot disappear in a later term.
The destination changes only after the complete sum succeeds, including for
self-referential assignments. An overflow stops the job, skips later statements
and returns control to normal VDP processing. Earlier successful writes remain;
there is no rollback. A floor result already written before a later overflow
also remains. Custom handlers and Resume/retry are future work.

Packages containing runtime checked additions expose an optional `runtime_error`
uint16 field in `interface.json`. Code 0 means no error; code 1 means uint16
addition overflow. The entry resets this field on every invocation. A later call
can therefore succeed after the caller corrects the input globals. This field is
resident state, not an automatic acknowledgement to MOS. Error exit needs no
host readback or extra recurring commands.

Known constant sums are folded and compile-time overflow is rejected. Global
initializer additions currently require constant operands; move variable sums
into the job so their errors can use its finite exit. Simple global copies remain
supported. Programs without runtime checked additions omit the status and error
helper. The pure floor package retains all five earlier artifact bytes.

This implementation covers the current single, finite resident job. Source
routine calls, loops, other integer operations, general conversions and nested
error propagation are not implemented. Legacy raw-mode uint16 addition still
wraps and remains outside this checked slice.

## Evidence

[GOLEM-008](../tasks/GOLEM-008.md) promotes the qualified GOLEM-006 implementation.
Its checks compare all four command artifacts for each of the four GOLEM-007
review packages against their recorded hashes. Only manifest provenance changes
from `experimental-resident-scalar` to `resident-scalar`, with the historical task
field removed. The stock Fab 1.2.5/MOS 3.0.2 qualification and successful Oryx
keyboard review therefore apply to the same command bytes. Physical hardware
and other firmware versions remain outside that evidence.

[GOLEM-012](../tasks/GOLEM-012/README.md) qualifies the changed resident addition
and error-exit bytes: maintained checks, ASan/UBSan, 88 independently modeled
jobs, and 1,238 stock Fab/MOS checks across 88 jobs. The Oryx fixture also passes
headless verification; the Author confirmed the human keyboard review worked. The earlier
GOLEM-007 human acceptance applies to its historical packages; it does not
replace acceptance of this changed counted-floor implementation.
