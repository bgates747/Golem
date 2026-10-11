# Shared allocation and experimental resident scalar integration

Ordinary uint16 state and the selected scalar floor/conversion now share checked
buffer allocation in one finite resident job. The task-local integrated CLI has
an explicit resident mode; ordinary payload output retains its existing bytes.
Host checks pass for compatibility, relocation, ownership and repeated updates.
The counted example needs 1120 installation bytes and 21 bytes per fresh input
and call, with counters and math executed on the VDP. Firmware qualification
remains the next boundary before scalar production promotion.

## Build and use

From the project root:

```sh
make -C src
make -C docs/tasks/GOLEM-006
# Ordinary source: existing raw payload for the existing loader.
docs/tasks/GOLEM-006/build/golemc-integrated \
  examples/loop_golem/loop.golem /tmp/loop.bin
# Resident source: installation, interface, call and cleanup streams.
docs/tasks/GOLEM-006/build/golemc-integrated --resident \
  --reserve 1:8 --reserve 20:24 \
  docs/tasks/GOLEM-006/examples/counted-floor.golem \
  docs/tasks/GOLEM-006/build/example -0.75
make -C docs/tasks/GOLEM-006 check
```

The maintained `build/golemc` CLI still emits ordinary payloads only. The
integrated CLI reuses its parser/emitter directly; it does not maintain another
ordinary parser or make maintained source depend on task files. Adapted scalar
emission, resident parsing and packaging stay in this silo. The integrated
binary is experimental pending firmware qualification and agreed promotion.

## Implemented source

[counted-floor.golem](examples/counted-floor.golem) is a complete independent
example:

```golem
uint16 calls = 0
uint16 previous = 0
float32 x Range(-32768,32767) = -0.00006103515625
sint16 result
Sub compute
    previous = calls
    calls = calls + 1
    result = sint16(floor(x))
    print "done\n"
EndSub
```

Ordinary uint16 globals precede one bounded float32 input, one sint16 output
and one Sub/EndSub job. Globals initialize at installation and persist between
calls; bare declarations zero-initialize. Existing uint16 initializer expressions
work, including references to earlier globals. The job allows literal print and
uint16 assignments/addition before or after exactly one scalar floor/conversion.
Names across globals/input/output/entry must be distinct, with case-sensitive
identifier spelling and case-insensitive keywords. No statement terminators;
newlines remain whitespace.

This slice has one job and one scalar expression shape. It does not introduce
general float expressions, conversions of output into uint16 expressions,
parameters, user calls or local variables. Resident For/Next is explicitly
rejected while its endpoint/sentinel behavior remains unqualified; ordinary
For/Next output is preserved. General If/Do/Switch and Range extensions remain
separate. Declaration Range retains continuous finite interval semantics.

## Allocation and ownership

Maintained `src/buffer_allocator.hpp` supplies compilation-local claims and
inclusive external reservations. IDs 0 (the existing loader root) and 65535
(the stock special ID) cannot be allocated or cleared as owned resources.
Reservations must precede compiler claims; overlapping reservations, duplicate
claims, invalid endpoints and namespace exhaustion report errors. Reservation
ranges do not become part of the installed program.

Ordinary default compilation retains accumulator ID 1 and allocation from 2.
Its data, loop buffers and accumulator now use checked ownership. An injected
allocator relocates the accumulator and ordinary globals. The scalar adaptation
uses that same allocator for input/output, matrices, entry, scratch, constants
and three helpers. IDs are passed to typed emission calls; no encoded bytes are
patched after emission. Its test-only Rally policy claims the original named
IDs and allocates private buffers from 2 to retain exact compatibility bytes.
These fixed IDs are not a resident ABI.

Repeat `--reserve FIRST:LAST` before resident source/output arguments to reserve
known external assets/interfaces. Endpoints are decimal IDs in 1..65534. Caller
and loader must coordinate those reservations and use the generated manifest's
IDs; the compiler does not discover live VDP ownership. A source or reservation
change can relocate all resource IDs.

The manifest lists only installed resources in `owned`, with role, byte length
and data/program kind. A claimed but unused accumulator is not installed or
included in cleanup. Cleanup clears exactly the installed ownership set, never
external reservations or the loader root. Ordinary globals, their accumulator
when used, scalar data, entry and helpers occupy disjoint IDs. Ordinary
initializers run after clearing their owned buffers and before scalar installation.

## Output and lifecycle contract

| Artifact | Meaning |
|---|---|
| bootstrap.bin | Raw VDU installation stream: enable stock affine support, clear/write owned resources, run ordinary initializers and set initial input; does not invoke the job |
| interface.json | Input/output/entry IDs, widths/bounds, uint16 globals, external reservations, installed ownership and wire sizes |
| invoke.bin | Six-byte call of the resident entry; uses currently stored input |
| update-call.bin | Example 15-byte input update followed by the six-byte call, using the optional CLI example input or declared initial value |
| cleanup.bin | Clear exactly the installed owned IDs |

These are raw VDU streams, not MOS executable files or ordinary loader payloads.
Mode selection is explicit; resident source passed to raw mode rejects instead
of silently changing the output contract. Parse, type/range, reservation,
allocation and generation errors complete before output files are touched.
File publication still is not atomic if a later filesystem write fails.

Every host input, including binary32 rounding, must satisfy the declared finite
interval. `resident_scalar::inputUpdate` validates that contract. Declaration
bounds do not insert runtime float admission, so an arbitrary external write can
violate it. Jobs are serialized and nonreentrant; scratch/helpers are shared for
that installation. A finite call must return to let the VDP service its outer
queue. The output stays in its declared VDP buffer; these byte totals exclude
result readback and do not add a new stock Buffered Commands API command.

| Example | Installation | Fresh input + call | Cleanup | Installed buffers |
|---|---:|---:|---:|---:|
| floor.golem | 1008 | 21 | 150 | 25 |
| counted-floor.golem | 1120 | 21 | 168 | 28 |

For 100 fresh calls, the counted resident package emits 1120 + 100×21 = 3220
bytes. A controlled baseline reinstalling an equally sized bootstrap with each
new initial input and then invoking it emits 100×1126 = 112600 bytes. These are
encoded byte counts, not measured application traffic or speed results. Counter
updates and the five screen-output bytes in the job do not add eZ80 wire bytes.

## Host evidence

[host-checks.json](host-checks.json) records commands, source fingerprints and
reusable build/run durations. `make check` uses the project Python environment
and writes build artifacts only under this silo's ignored build/ directory.
It leaves historical GOLEM-003/004/005 files and their reports unchanged.

| Check | Evidence |
|---|---|
| Maintained parser regression | Original GOLEM-005 sanitizer expectations: 20 ordinary and 22 scalar byte matches, 69 rejection checks, source spans/casing/defaults |
| Adapted scalar compatibility | 22 complete bootstrap/cleanup/invoke/update matches against unchanged GOLEM-003, including float/double subnormals and signed bounds |
| Allocation/integration failures | 34 checks for collisions, reserved IDs, exhaustion, late reservations, duplicate names and unsupported resident forms |
| Relocated operation identity | Independent decoder normalizes only actual ID operands; two scalar layouts match compatibility and two mixed layouts match one another |
| Host execution model | Seven packages × 807 jobs = 5649 modeled calls; floor values, fresh inputs, counters, word wrap, code immutability, reinstall reset and cleanup/external assets pass |
| CLI contract | Sixteen failure cases preserve existing output; raw loop output matches pre-tranche compiler; raw mode rejects resident source |

The decoder validates all owned spans, carry landing bytes, matrix fields,
conditional targets, install/clear ownership and the acyclic four-program helper
graph (maximum logical depth three). The bounded model executes decoded command
forms used by this slice, with explicit binary32 rounding and fixed-word
conversion. It is deliberately not a complete VDP emulator. Its math semantics
were checked against the local stock `video/vdu_buffered.h` and
`video/types.h` at the firmware revision below. Source-level parity and model
results are host evidence only; no firmware or emulator was executed/deployed.

[provenance.json](provenance.json) fingerprints the read-only references and
verifies they match their recorded commits. Stock VDP revision
`c7ac293d2aa81ddfa693390549bcd909069c8fc3`, Buffered Commands API documentation,
and GOLEM-003's frozen Rally provenance. Stock-firmware qualification must verify
actual tiny-negative/signed-boundary results, repeated inputs and finite return
behavior before promoting the experimental scalar/profile code into maintained
production sources.
