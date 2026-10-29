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

The default mode emits both backends. `--target native` explicitly emits only
native source/install/cleanup and named native wrappers, retaining the common
typed state/root/layout. `--target both` is the default. Native-only packages
include targets.json and capability/library metadata; they omit buffered
install/cleanup and buffered wrappers. Changing selection requires a fresh
output directory so stale artifacts cannot imply support. Native renderer
bindings remain outside this profile. Existing raw and `--resident` commands
keep their own grammar and output contracts.

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
   precedes addition/subtraction, then >>, ^, comparisons, Not, And, Or.
   Parentheses override grouping; arithmetic operands evaluate left to right.
   Native float division shares multiplication precedence and associates left.
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
8. Same-type integer `^` is bitwise XOR for uint16/sint16/uint32/sint32.
   It evaluates both operands once, left to right, and chains left to right.
   Signed operands use their full-width two's-complement bit patterns; XOR
   cannot overflow. Destination Bounds checks still apply. Bool/float32 XOR
   and implicit mixed-type XOR reject. Existing conversions are numeric range
   conversions, not bit reinterpretation. See [the initialized example](../../examples/portable/xor.golem).

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

The XOR example produces toggled=6 and signedResult=2147483647. `a + b ^ c`
means `(a + b) ^ c`; `a >> count ^ b` means `(a >> count) ^ b`; and
`a ^ b == c` means `(a ^ b) == c`. Parentheses override these rules. XOR is
available throughout current integer expressions, including array indices,
reads/writes, conversions, comparisons and divmod arguments. It uses stock
bytewise buffer adjustment or native unsigned bit patterns, with identical
shared storage and no extra per-operation host traffic.

## Native mathematical capability

With explicit `--target native`, float32 expressions additionally support `/`,
`sin(x)`, `cos(x)` and `floor(x)`. Arguments/results are float32, with explicit
conversion required for other types. Sin/cos take radians; degree conversion
is ordinary Golem arithmetic. Floor returns float32, so `sint16(floor(x))`
performs the separate checked integer conversion. Keywords/builtins are
case-insensitive; integer `/` is rejected in favour of divmod.

Division evaluates its two operands once, left to right, then checks a zero
divisor (including signed zero), reporting language status 4. Nonfinite input
or output reports 33; destination Bounds still report 32. Each assignment
preserves its failed destination and retains earlier source effects. Existing
normalization/subnormal and short-circuit rules apply to all four operations.
Native C uses ordinary `/`, sinf/cosf/floorf with contraction disabled. Constant
division retains checked binary32 folding; trig calls are not folded using the
build host's library. Qualify trig accuracy against a higher-precision reference;
portable bit identity across different mathematical libraries is not assumed.

Compile the emitted C with the SDK's opt-in `--math` setting. The compiler does
not build or load the module. Stock lowering of these new mathematical
operations is not qualified: default/both compilation rejects them, including
operations in skipped branches. This capability does not change accepted
programs' default dual-backend outputs. See [Flower's mathematical tranche](../tasks/GOLEM-044/native-math/README.md)
for exact execution scope, accuracy cases and evidence.

General Functions/parameters/locals/inter-Sub calls, Do/Loop, Switch/jump tables,
stored strings, fixed-point, multidimensional arrays/matrices, left shifts
and sqrt/pow remain outside this profile.
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
On exit, native code attempts each acquired root/argument release once, root
first, and still attempts the other release after a failure. It returns the
first exit-cleanup error, or the primary result if cleanup succeeds. Previously
written interface/language status and earlier source effects remain intact;
root bytes are never accessed after their release attempt. Temporary array and
bound-view release errors also propagate through this exit cleanup.

Success markers advance before final cleanup. A release failure can therefore
coexist with an advanced success generation: assembly must check the native
runtime result as well as root status/generation. This contract provides no
rollback or callback retry; the runtime handles outstanding leases after normal
entry return. See [GOLEM-041](../tasks/GOLEM-041.md).

For native packaging, use the separately maintained
[VDPlet Runtime SDK](/Agon/mystuff/agon-vdplet/docs/API-reference.md). It requires
its supported Xtensa toolchain and loader-enabled firmware; stock firmware can
use the buffered package. No firmware rebuild or flashing occurs in this
compiler build. Pin SDK/settings when comparing native package hashes.

## Maintained checks

`make -C src check` runs legacy contracts and portable grammar/reference/CLI
checks, including 52 frozen non-native-C output files. The three historical
native-C fixtures remain unchanged; current native output intentionally differs
for release-error propagation and has separate fault checks. The normal checks
need only the host C++ toolchain and repository files. Fixture provenance is
under `tests/portable/fixtures/`.

Optional model/generated-C/sanitizer integration, with the SDK checkout supplying
only its public header:

```bash
.venv/bin/python tests/portable/check_integration.py \
  --compiler build/golemc --sdk ../agon-vdplet --output build/portable-integration
```

Add `--relocated` for relocated allocation and `--assembler /path/to/ez80asm`
for a real assembly consumer. This checks host models and generated C, not target
execution. The promotion itself retained accepted emitted bytes and earlier
target evidence without launching an emulator. GOLEM-041 separately qualifies
its changed native packages through actual Xtensa execution on Oryx.

Optional generated-C release fault checks use the same public SDK header:

```bash
.venv/bin/python tests/portable/check_release_errors.py \
  --compiler build/golemc --sdk ../agon-vdplet --output build/portable-release
```

Add `--relocated` to repeat with changed IDs. These sanitizer checks inject
release failures and refused releases; they do not imply hardware fault tests.


## Integer bitwise family — GOLEM-053

The maintained typed profile supports `&` (AND), `|` (OR), unary `~`
(complement) and existing `^` (XOR) for all four integer types. Matching types
are required; results retain their 16/32-bit width and signed bit pattern.
Unary complement binds above arithmetic; after arithmetic and right shift,
binary precedence is &, ^, |, then comparisons. All binary operators associate
left and evaluate both operands in order. Boolean And/Or/Not remain separate.
Bounds admission and failure preservation are unchanged. Stock emission uses
bytewise Adjust operations; native C uses unsigned storage with width masking.
See the language reference for initialized examples. Left shift remains deferred.


## Resident integer printing — GOLEM-052

Calculation programs accept literal print and print str(integer-expression).
Each expression evaluates once, before output, with normal faults. Consecutive
statements insert no separator/newline. Decimal output covers all four integer
types; bool, stored strings, concatenation and print lists remain deferred.
GOLEM-054 additionally accepts float32, with the exact formatting contract below.
One width-specific resident formatter reuses existing unsigned divmod helpers.
Signed magnitude, including minima, is formed in carry-guarded unsigned scratch.
Digits are written backward into fixed text; the leading offset selects only
actual digits/sign. Each print site owns a 6/11-byte temporary, copied from that
shared formatter, emitted with stock offset CALL and zeroed after use. All helper
storage is private and participates in ordinary buffered cleanup.

Printing currently makes the default target stock-only. --target buffered also
selects this output explicitly, including for ordinary calculation programs.
Explicit native/both with print rejects before output. No-print default packages
remain unchanged. New stock-only packages have targets.json, available_backends
[buffered], no native.c/argument/wrappers, and only stock calls/load-plan components.
Changing backend shape requires a fresh output directory. This does not change
the root ABI or any application loading/call/completion responsibility.

Programs with print use typed-program JSON schema5 (literal byte or integer
expression print statements); no-print programs retain schema4 and exact bytes.
Float32 print programs use schema6 and a `float32` expression field; old integer
print programs retain their schema5 bytes.

## Resident float32 printing — GOLEM-054

`print str(float32-expression)` evaluates once and formats the stored binary32
value to nine significant decimal digits, nearest/ties-to-even. Trailing
fractional zeros and an unnecessary point are omitted. Rounded decimal
exponents -4 through 8 use ordinary decimal notation; all others use lowercase
scientific notation with an explicit sign and two exponent digits. Zero is `0`.
This is a deterministic nine-digit representation, not shortest-decimal output.

Conversion unpacks the integer significand and binary exponent. Positive binary
shifts multiply a decimal coefficient by two; negative shifts multiply by five
and track a decimal exponent. The exact coefficient needs at most112 digits.
Digit/carry lookup tables and bounded tail loops avoid rounded float scaling
and 16-bit integer conversion limits. Existing input admission, expression
normalization and status propagation remain unchanged; source bytes are borrowed
only for evaluation and are never modified by formatting. Shared helper storage
is private; each print site immediately consumes and clears its16-byte text
temporary. The application retains ordinary installation/call/cleanup ownership.
Fixed-point types, format arguments and native printing remain follow-up work.
