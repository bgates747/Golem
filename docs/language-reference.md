# Golem language reference

Golem describes work that runs on the Agon VDP. It is explicitly typed,
procedural and imperative: statements change retained state, finite Subs are
entry points, and the application chooses when to call them. The compiler can
emit stock buffered commands or C for a native VDPlet. This reference separates
implemented profiles from proposed language features. It is included in the
on-Agon compiler distribution so an Internet connection is unnecessary.

## Choosing a profile

The current compiler has several implemented profiles. They share conventions,
but their statements cannot freely be combined in one source file.

| Invocation | Implemented purpose | Output |
| --- | --- | --- |
| `golemc source.golem payload.bin` | Legacy literal text, uint16 variables/addition and counted loops | A raw VDU command stream |
| `golemc --resident source.golem directory` | A bounded floor conversion with checked uint16 arithmetic; or the separate record/table or drawing grammar below | Installation, invocation, cleanup and layout files |
| `golemc --portable source.golem directory` | Checked typed arithmetic, conditions, loops, integer arrays and printing | Shared state and stock commands; native C where supported (printing is stock-only) |
| `golemc --portable --target native source.golem directory` | Portable grammar plus float division/trigonometry/floor | Shared state and native C source; no stock implementation |
| `golemc --stock-matrix source.golem directory` | Stock matrix and vertex-buffer operations, finite calls and indexed loops | Installation, invocation, cleanup, metadata and assembly |

The on-Agon port uses the same current parsers and emitters. Its implementation
and qualification are tracked in GOLEM-051. Native C output still needs a
separate C-to-Xtensa build to become an executable VDPlet. The on-Agon compiler
does not contain that toolchain. Compilation never requires an LLM.

## Lexical rules

1. Keywords and builtins ignore case: `Sub`, `SUB` and `sub` mean the same thing.
   User identifiers retain case: `angle` and `Angle` differ. Exported assembly
   symbols must also survive the assembler's case-folding/collision rules.
2. Declare variables explicitly before using them. An assignment or For header
   never creates a variable. Current compiled profiles use globals; general
   local scopes and parameterized routines are proposed.
3. There are no statement terminators. A semicolon is an error. Newlines are
   whitespace; grammar determines boundaries, so multiple statements can share
   a line. Prefer one statement per line for readability.
4. `//` begins a comment through the end of the line. Identifiers start with a
   letter or underscore and continue with letters, digits or underscores.
5. Numbers are decimal. Floating forms include `1.5`, `0.25`, `1e-3` and negative
   literals where the profile allows them. Hex/binary literal syntax is absent.
6. Literal strings use double quotes. Supported escapes are `\"`, `\\` and `\n`.
   `\n` emits LF only; it does not return the VDP text cursor to the left margin.
   `\r` is not currently a recognized escape. Print does not append a newline.

## Integer printing in the calculation profile

`print "literal text"` emits the literal bytes. `print str(integer-expression)`
converts one integer expression to ordinary decimal text and emits it. This
works for uint16, sint16, uint32 and sint32, including signed minima. Positive
values have no plus sign; zero prints `0`; leading zeros are omitted. There is
no implicit spacing or newline, and no concatenation or print-list operator.
Consecutive print statements build output in order:

```golem
uint16 i = 0
uint16 total = 0
Sub accumulate
    For i In Range(1:10)
        total = total + i
    Next i
    print "i=" print str(i) print " total=" print str(total)
EndSub
```

The first call prints `i=10 total=55`; a second resident call prints
`i=10 total=110`. Variables remain explicitly declared and retain their values.
Printing itself does not modify its source value. A literal-only Sub can omit
global declarations. Context-free integer expressions are tried in the existing
uint16/sint16/uint32/sint32 order; variables must match the selected type, and
mixed types require explicit conversion. Bool/float formatting, optional formats,
stored strings and general str expressions are not implemented. `str(...)` is
currently accepted only as the operand of print. A failing expression emits no
characters for that print; previous successful prints remain visible.

Printing currently targets stock buffered commands. `golemc --portable` defaults
to a stock-only resident package for a program containing print; explicit
`--target buffered` does the same. Native/both selections reject such a program
before writing output. Programs without print retain their existing default dual
outputs. A target-shape change needs a fresh output directory. The package's
`available_backends` and generated wrappers describe only actual implementations.
The eZ80 application still owns installation, calls, pacing and cleanup; it does
not read values back to format them. The standalone runner supplies its ordinary
final newline after the Golem call.

## Portable scalar types and declarations

| Type | Value domain | Packed bytes |
| --- | --- | --- |
| `uint16` | 0 through 65535 | 2 |
| `sint16` | -32768 through 32767 | 2 |
| `uint32` | 0 through 4294967295 | 4 |
| `sint32` | -2147483648 through 2147483647 | 4 |
| `float32` | Bounded binary32 environment described below | 4 |
| `bool` | `false` or `true` | 2 (0 or 1) |

Portable globals require initializers. `Bounds(low,high)` narrows the inclusive
value domain; it does not declare an array or a loop range.

```golem
uint16 count Bounds(0,100) = 0
sint16 displacement = -12
uint32 distance = 65536
float32 scale Bounds(0.0,4.0) = 1.5
bool ready = false
Sub update
    count = count + 1
    ready = count >= 10
EndSub
```

Assignments require matching types. Integer literals acquire the expression's
required type; variables of another type require an explicit conversion.
Use `uint16(x)`, `sint16(x)`, `uint32(x)`, `sint32(x)` or `float32(x)` for the
implemented conversions. Integer-to-integer conversions check range. Conversion
between float32 and 16-bit integers is supported; wide-integer/float conversion
is deferred. Float-to-integer conversion truncates toward zero and checks range.
There is no numeric truthiness or implicit bool conversion.

## Expressions and checked arithmetic

From tighter to looser binding:

| Operations | Meaning |
| --- | --- |
| Parentheses, builtin calls and conversions | Explicit grouping |
| Unary `~` | Integer bitwise complement, right associative |
| `*`, native float `/` | Multiplication/division, left associative |
| `+`, `-` | Addition/subtraction, left associative |
| `>>` | uint32 logical or sint32 arithmetic right shift |
| `&` | Integer bitwise AND, left associative |
| `^` | Integer bitwise XOR, left associative |
| `\|` | Integer bitwise OR, left associative |
| `==`, `!=`, `<`, `>`, `<=`, `>=` | Same-type comparisons yielding bool |
| `Not` | Boolean negation |
| `And` | Short-circuit Boolean conjunction |
| `Or` | Short-circuit Boolean disjunction |

Arithmetic operands evaluate once, left to right. Integer operations check every
intermediate result; a later operation cannot repair an earlier overflow.
`&`, `|` and `^` require matching uint16, sint16, uint32 or sint32 operands;
`~` accepts one of those integer types. They preserve the declared type and
operate on exactly its 16 or 32 bits, including signed two's-complement bits.
They reject bool/float operands and implicit mixed-type conversion. For example,
uint16 `~0` is 65535, while sint16 `~0` is -1. The operations cannot overflow,
but destination Bounds and operand evaluation/conversion errors still apply.
Binary bitwise operations evaluate both operands once, left to right; Boolean
`And` and `Or` instead short-circuit. Unary `~` binds above arithmetic (at most
16 consecutive prefixes); bitwise precedence is `&`, then `^`, then `|`, all
above comparisons. Thus `flags & mask == expected` compares the masked value.
These operations are available in procedure expressions; declaration initializers
remain literals. `&&`, `||`, compound assignment and left shift are not supported.

```golem
uint16 flags = 5
uint16 mask = 3
uint16 both = 0
uint16 either = 0
uint16 different = 0
uint16 inverted = 0
bool masked = false
Sub inspectBits
    both = flags & mask
    either = flags | mask
    different = flags ^ mask
    inverted = ~flags
    masked = flags & mask == 1
EndSub
```

The results are 1, 7, 6, 65530 and true, respectively.

`^` is XOR, not exponentiation. Shift counts are uint16 in 0..31; invalid counts
fail rather than being masked. A signed right shift rounds toward negative
infinity. Widen a 16-bit operand explicitly before shifting.

```golem
uint32 numerator = 100
uint32 denominator = 7
uint32 quotient = 0
uint32 remainder = 0
uint16 bits = 2
uint32 shifted = 0
bool valid = false
Sub compute
    (quotient, remainder) = divmod(numerator, denominator)
    shifted = numerator >> bits
    valid = denominator != 0 And quotient == 14
EndSub
```

`divmod` supports matching uint16, uint32 or sint32 inputs/global destinations.
The two destinations must differ; arguments evaluate once before either result
is committed. Signed quotient truncates toward zero and the remainder has the
numerator's sign or is zero. Zero divisors and signed minimum/-1 fail. Sint16
pairs and array destinations are not supported.

## Conditions and finite loops

```golem
uint16 i = 0
uint16 total = 0
bool ready = false
Sub accumulate
    For i In Range(0:10:2)
        total = total + i
    Next i
    If total > 40 Then
        ready = true
    ElseIf total == 30 Then
        ready = true
    Else
        ready = false
    EndIf
EndSub
```

Portable Range endpoints and optional step are compile-time literals. Endpoints
are inclusive. The initialized counter is uint16; a nonzero step points toward
the endpoint and may be negative. An omitted step is +1. The last visited value
is retained, with no one-past increment. `Next` may omit the counter name in
portable mode. A body cannot assign its active counter; nested loops use distinct
counters. Current limits include eight structured nesting levels, 64 ElseIf
arms, Boolean depth 16 and 262144 worst-case work visits per entry.

The agreed `For i = first To last Step step` alternative is not implemented in
portable mode. It exists in the older raw profile with its separate limitations.
General Do/Loop, Switch/Case and Range jump tables are proposed, not executable.

## Integer arrays

```golem
uint16 index = 0
sint32 selected = 0
Array sint32 measurements(-1:1) = Values(-65,0,100)
ReadOnly Array uint16 weights(0:2) = Fill(3)
Sub fetch
    selected = measurements(sint16(index))
    measurements(-1) = selected + 1
EndSub
```

Current portable arrays are one-dimensional and homogeneous, with uint16,
sint16, uint32 or sint32 elements. Bounds are ascending and inclusive, inside
-32768..65535; packed bytes must fit one stock block. Index expressions are
uint16 or sint16. `Values` must supply every element; `Fill` repeats one literal.
A declaration without either initializer needs exact packed build data through
`--array-data NAME=PATH`. Source and build-file initialization cannot conflict.
An array write checks the index before evaluating its value. ReadOnly is a
source permission; trusted assembly can still change storage between calls.

## Float semantics and native mathematics

Float operators round independently to binary32. Inputs admit finite normal
values and signed zero; NaN, infinity and subnormal inputs reject. Computed zero
and subnormal results normalize to positive zero. Do not use fast-math or fused
contraction when building generated native C.

With `--target native`, `/`, `sin`, `cos` and `floor` accept and return float32.
Trig takes radians. Floor returns float32; integer conversion is separate.

```golem
float32 degrees = 30.0
float32 radians = 0.0
float32 sine = 0.0
float32 fraction = 0.0
sint16 whole = 0
Sub compute
    radians = degrees * 0.017453292519943295
    sine = sin(radians)
    fraction = sine / 2.0
    whole = sint16(floor(fraction))
EndSub
```

Stock lowering of these four math operations is not qualified, so default/both
compilation rejects them, even in skipped branches. Native C calls sinf/cosf/
floorf; trig is not folded with the compiler host's library. General sqrt,
exponentiation, complex numbers and fixed-point types remain proposed.

## Runtime errors

Portable failures end the finite invocation, preserve the failed destination
or divmod pair, and retain earlier effects. Root metadata separates interface,
language and native runtime results. There is no rollback or implemented source
error handler.

| Language status | Meaning |
| --- | --- |
| 1 | Addition overflow |
| 2 | Subtraction overflow/underflow |
| 3 | Multiplication overflow |
| 4 | Zero divisor |
| 5 | Array/table index outside bounds |
| 6 | Signed division minimum/-1 overflow |
| 32 | Bounds, conversion or shift-count failure |
| 33 | Invalid/nonfinite float input or result |

Stock VDP does not automatically report these values to MOS. The application
owns any result transport and completion handling. A wrapper returning after
sending bytes is not a completion acknowledgement.

## Legacy raw text and uint16 profile

```golem
print "Hello, world!"
```

This is an entire valid raw program. Compile it without a mode flag. `print`
accepts literal strings, not expressions or numeric formatting. Raw also admits
uint16 declarations (default zero), assignment, addition and declared-counter
`For i = first To last ... Next i`. Its addition wraps at 16 bits; use portable
mode for checked arithmetic. Legacy loop emission increments after the body and
has known 65535/wrap and stock-comparison limitations; use portable Range loops
for correctly bounded endpoint behavior. Raw has no Subs, drawing calls or
portable branches. A raw payload is not a resident package.

## Resident scalar and table profiles

Resident scalar admits exactly one bounded float32 input and a sint16 floor
result, plus optional persistent uint16 globals and checked + / - / * / divmod.
The historical `Range(low,high)` spelling on its float declaration is a scalar
admission range, not an array declaration. This legacy profile requires that spelling; portable declarations use Bounds.

```golem
float32 x Range(-32768,32767) = -0.75
sint16 result
Sub compute
    result = sint16(floor(x))
EndSub
```

The separate table grammar admits one flat Record (1..32 uint16/sint16 fields),
one immutable zero-based Table (1..1024 rows), an initialized record destination,
a uint16 index and one checked whole-record fetch. Optional literal prints may
surround the fetch. `--table-data NAME=PATH` supplies exact packed rows at compile
time; Golem is not asking the VDP to open a file.

```golem
Record Sample
    uint16 tag
    sint16 delta
EndRecord
Table Sample samples(0:3)
Sample selected = Sample(0,0)
uint16 index = 2
Sub fetch
    selected = samples(index)
EndSub
```

## Resident drawing profile

```golem
uint8 colour Bounds(0,63) = 15
sint16 x = 40
sint16 y = 30
Sub drawDemo
    draw_point(colour,x,y)
    draw_line(colour,60,30,140,90)
    fill_quad(colour,180,40,260,40,280,100,160,100)
EndSub
```

This profile admits initialized uint8/sint16/uint16 globals, optional inclusive
Bounds and multiple finite named Subs. Colour fields need uint8 Bounds within
0..63; coordinates are sint16; matching literals are accepted. No assignments,
loops, branches, parameters, locals or inter-Sub calls are implemented here.

`draw_bitmap(bufferId,x,y)` draws a separately installed native-size bitmap.
Its uint16 ID must lie in reserved, loaded assets (1..65534); dynamic ID fields
need matching Bounds. Borrowed assets survive package cleanup. Assembly owns
mode, origin, pixel-coordinate setup, loading, updates, calls and pacing.
Point/line/convex-quad calls use stock GCOL/PLOT, including stock clipping.

## Stock matrices and borrowed vertex buffers

```golem
float32 angle = 0
Matrix(4,4) model = Identity
Borrowed XYZ16 source = Buffer(1000,8)
Borrowed XYZ16 target = Buffer(1001,8)
Sub transformFrame
    matrix_identity(model)
    matrix_rotate(model,0,angle,0)
    transform_vertices(target,model,source)
EndSub
```

Compile with `--stock-matrix --reserve 1000:1001`. Matrices are owned 3x3 or
4x4 binary32 storage, initialized with Identity or exactly 9/16 row-major
`matrix_values(...)` literals. Float32 controls are initialized packed inputs.
Borrowed XYZ16/XYZF32/XY16 records have 6/12/4 bytes per record and must already
exist in reserved, distinct buffers. Their complete packed length fits 65535
bytes. Cleanup preserves borrowed data.

Implemented calls are `matrix_identity`, `matrix_rotate`, `matrix_scale`,
`matrix_translate`, `matrix_multiply(dest,left,right)`, 3x3-only `matrix_invert`,
`matrix_set_value(dest,source,row,column,value)`, `transform_vertices` and bounded
`copy_bytes(dest,destOffset,source,sourceOffset,length)`. Operation dimensions
must agree. Rotation takes decimal degrees; composition pre-multiplies. Typed
`read_float32(buffer,byteOffset)` supplies aligned float operands. Transforms
preserve record format; they do not implicitly narrow float XYZ to integer XY.
Stock final-coordinate conversion has qualified positive-domain behavior;
negative values may clamp to zero, not signed truncation.

Named parameterless calls may refer forward; cycles reject and Sub call depth
is at most four. Declared uint16 counters support inclusive ascending literal
unit Range loops, including 65535, retaining the last visited value. Indexed
offsets use `stride*activeCounter[+bias]` with positive literal stride and optional
nonnegative bias. Static span/alignment proofs precede emission. No nested loops,
body Sub calls or active-counter assignments are admitted in this profile.
At most 1024 static operations per Sub/body and 65536 primitive executions per
lexical loop are admitted. These loops reduce installation size; they do not
promise a frame-speed gain. See the included detailed stock-matrix contract for
exact signatures, storage and firmware boundaries.

## Files, ownership and application integration

Resident packages contain bootstrap.bin, invoke.bin, update-call.bin, cleanup.bin
and interface.json. `--emit-asm` supplies named eZ80 wrappers/equates; stock-matrix
always emits golem.inc. `--reserve FIRST:LAST` protects inclusive external buffer
IDs; overlaps reject and IDs 0/65535 are unavailable to resident allocation.
Optional `--emit-load-plan` publishes installation records and AGNB integration
metadata. Filenames and IDs are build accounting, not VDP-side file lookup.

Portable packages additionally include shared state/array initializers, root and
program metadata, layout.inc, calls.inc and backend-specific install/cleanup.
Native C is source, not an already compiled Xtensa image. The caller loads a
compatible VDPlet, binds its fresh handle and selects a named backend wrapper.
Shared state survives backend-private cleanup. Finish execution before updating,
switching or unloading. Call frequency and VSYNC pacing remain application work.

Proposed Functions with object/multiple returns, local scopes, multidimensional
arrays, generalized mixed-field tables, automatic sprites, Do/Loop, Switch/Case,
jump tables, fixed-point, extra math and left shift are not silently
accepted by the implemented profiles. The examples/proposals collection records
their intended syntax separately; it is not a promise that each sample compiles.
