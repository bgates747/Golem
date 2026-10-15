# Maintained portable compiler

`golemc --portable` compiles the same initialized, typed program into stock VDP
buffered commands and native C for VDPlets. Shared data and named entries let
an assembly application choose either backend without resetting program state.
The compiler builds with an ordinary host C++ compiler; packaging native C into
an Xtensa module is an optional external SDK step.

## Build and compile

```bash
make -C src check
build/golemc --portable examples/portable/counter.golem build/counter
```

Read [the complete example](../../examples/portable/counter.golem). Calling its
`step` entry sums three retained values, computes signed -65 >> 6 as -2,
converts 1.5 to integer 1, and sets `ready` true. Execution is finite. The
compiler does not run this program on hardware or load its files through MOS.

```text
golemc --portable [--reserve FIRST:LAST ...]
                 [--bind ROLE ID OFFSET BYTES ...]
                 [--array-data NAME=PATH ...]
                 SOURCE OUTPUT_DIRECTORY
```

Options precede the two paths; `--` ends option parsing. Numeric options are
decimal. Reservations are inclusive IDs in 1..65534; overlapping ranges or
bindings reject. IDs 0 and 65535 are unavailable. The compatibility profile
also reserves 60000..60031 implicitly; do not overlap that range. The same
source/options produce the same shared layout and backend-private IDs.

An array declared without Fill/Values requires `--array-data NAME=PATH` with
exactly the declared packed bytes. Unknown/duplicate mappings and conflicting
source initializers reject. The path is build input, not a runtime filename.
Optional fixed `--bind` roles are input snapshot=1, event mailbox=2, table=3 and
asset-directory metadata=4. Roles 1/3/4 are read; role 2 is read/write. These
descriptors declare externally installed ranges, not working synchronization,
event transport or automatic asset loading.

The current mode emits both backends. `--target` filtering and native renderer
bindings are outside this maintained profile. Existing raw and `--resident`
commands keep their own grammar and output contracts.

## Supported language

1. Initialized globals: uint16, sint16, uint32, sint32, float32 and bool, with
   optional inclusive numeric Bounds. Bool uses true/false and two-byte 0/1
   storage. Variables require explicit types; assignments do not declare them.
2. Finite parameterless named Subs containing assignments, If/ElseIf/Else/EndIf,
   and For counter In Range(start:end[:step]) / Next [counter]. The counter is
   a previously initialized uint16 global; ranges/step are compile-time literals,
   inclusive, with nonzero step pointing toward the endpoint. It retains the
   last visited value, and the body cannot assign an active counter.
3. Same-type checked +, -, *, all six comparisons, explicit integer conversions,
   and float32 conversions to/from 16-bit integers. Mixed variable types require
   explicit conversion. Wide integer/float conversion is not supported.
4. Typed Not/And/Or with short-circuiting; no numeric truthiness. Multiplication
   precedes addition/subtraction, then >>, comparisons, Not, And, Or.
   Parentheses override grouping; arithmetic operands evaluate left to right.
5. `(q,r)=divmod(n,d)` with distinct matching uint16, uint32 or sint32 global
   destinations. Arguments evaluate once; both outputs must pass checks before
   either is stored. Signed quotient truncates toward zero and remainder has
   the numerator's sign or zero. Sint16 pairs and array destinations reject.
6. uint32 logical and sint32 arithmetic >> with a uint16 count in 0..31.
   Invalid counts fail rather than being masked; signed shifts round toward
   negative infinity. Widen a 16-bit operand explicitly before shifting.
7. One-dimensional Array / ReadOnly Array of uint16/sint16/uint32/sint32, with
   ascending inclusive bounds and Fill(literal), Values(literals) or build data.
   Indices are uint16/sint16 expressions. Bounds lie within -32768..65535 and
   each array's packed bytes fit a single block. Array writes check the index
   before evaluating the value. ReadOnly is source permission, not protection
   against trusted assembly changing storage while execution is stopped.

Statements have no terminators; newlines are whitespace. Keywords are case
insensitive and user identifiers retain case, with generated assembly-name
collision checks. Current admission limits include eight structured nesting
levels, 64 ElseIf arms, Boolean depth 16 and 262144 worst-case work visits per
entry. These are finite-work limits, not deadlines or physical memory promises.

Float operators round independently to binary32. Inputs accept finite normal
values and signed zero; NaN, infinity and subnormal inputs reject. Computed zero
and subnormal results become positive zero. Float-to-integer conversion truncates
toward zero and checks range. Build relevant host/native code with contraction
disabled and without fast-math. This is a bounded float environment, not all
IEEE-754 behaviors.

General Functions/parameters/locals/inter-Sub calls, Do/Loop, Switch/jump tables,
strings/print, fixed-point, multidimensional arrays/matrices, left shifts/other
bitwise operators and sqrt/pow/float division remain outside this profile.
The legacy resident floor/table/drawing profiles remain separately available.

## Generated files and assembly use

| Files | Purpose |
| --- | --- |
| `interface.json`, `root.json`, `program.json`, `layout.inc` | Types, inclusive bounds, buffers/offsets, entry order, root fields, structured source and assembly equates |
| `state.bin`, `array-*.bin`, `common-install.bin`, `arrays-install.bin` when arrays exist | Application-owned initial storage; common installation initializes root and arrays |
| `buffered-install.bin`, `buffered-cleanup.bin` | Stock command implementation and its private cleanup |
| `native.c`, `native-install.bin`, `native-cleanup.bin` | Native source and retained argument/call wrappers; no executable module is built by golemc |
| `backends.json`, `calls.inc` | Private ownership, entry buffers, generated handle-patch offset, named eZ80 wrappers |

Assembly installs common storage once, installs the chosen private backend,
updates inputs, and calls `golem_buffered_step` or `golem_native_step` for the
example. These wrappers send six bytes; their return does not acknowledge VDP
completion. Native assembly first loads a compatible SDK-packaged module and
patches each retained RUN wrapper with the fresh handle using generated
`native_handle_offset`. Do not copy a patch offset from a different profile.

Shared storage uses the existing 112-byte root-v1 header. Its important fields
are session, requested entry, interface status, language status, host-owned
runtime result and last-success generation/entry/session. Four optional fixed
range descriptors remain present even when unbound. Native preflight checks
actual root/array/bound ranges through leases; stock depends on truthful assembly
installation and lifetime invariants. Neither mechanism freezes a concurrent
player snapshot. Source/helper views must not outlive a native entry.

Language statuses are add overflow=1, subtract overflow/underflow=2,
multiply overflow=3, zero divisor=4, array index=5, signed minimum/-1 division
overflow=6, Bounds/conversion/count=32 and float input/nonfinite=33. A failure
ends the invocation and preserves its failed destination/pair, retaining earlier
effects. Check interface/language/transport results separately; successful
generation is not a simulation tick and does not mean a frame was displayed.

Private cleanup/native unload preserve shared data; reinstalling common data
resets it. Complete pending execution before switching implementations or
unloading. Assembly owns updates, synchronization, pacing and all file loading.
The generic native emitter retains its qualified limitation of ignoring release
return values on several exit paths; error-propagation improvement is separate
work, not silently included in this promotion.

For native packaging, use the separately maintained
[VDPlet Runtime SDK](/Agon/mystuff/agon-vdplet/docs/API-reference.md). It requires
its supported Xtensa toolchain and loader-enabled firmware; stock firmware can
use the buffered package. No firmware rebuild or flashing occurs in this
compiler build. Pin SDK/settings when comparing native package hashes.

## Maintained checks

`make -C src check` runs legacy contracts and portable grammar/reference/CLI
checks, including 55 frozen accepted output files. It needs only the host C++
toolchain and repository files. Fixture provenance is under `tests/portable/fixtures/`.

Optional model/generated-C/sanitizer integration, with the SDK checkout supplying
only its public header:

```bash
.venv/bin/python tests/portable/check_integration.py \
  --compiler build/golemc --sdk ../agon-vdplet --output build/portable-integration
```

Add `--relocated` for relocated allocation and `--assembler /path/to/ez80asm`
for a real assembly consumer. This checks host models and generated C, not target
execution. The promotion retains accepted emitted bytes and earlier target
evidence; it did not launch an emulator or deploy a new native package.
