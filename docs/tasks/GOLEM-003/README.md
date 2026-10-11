# GOLEM-003 — Readable scalar-floor prototype

The readable example compiles directly through an extracted C++ numeric emitter
to the same installation and cleanup bytes as Rally. It hides IDs, fields and
scalar matrices from the source. Installation is 1,008 bytes, followed by
21 bytes per bounded input update and job call. This is a task-local prototype;
its evidence is host compilation, byte comparison and command decoding, not a
new VDP execution result. The next decision is whether to promote this boundary
and a small supported syntax slice into the maintained compiler.

## Build and use

From the repository root:

```sh
make -C docs/tasks/GOLEM-003
docs/tasks/GOLEM-003/build/golemc-scalar \
  docs/tasks/GOLEM-003/examples/floor.golem \
  docs/tasks/GOLEM-003/build/example -0.75
make -C docs/tasks/GOLEM-003 test
```

The optional last argument selects the value in the example update/call packet.
It does not change the source's initial value or cause execution. The build
needs a C++17 host compiler; verification uses the project's `.venv` and g++
with strict warnings plus address/undefined-behavior sanitizers. Build and
generated files stay in the ignored `build/` directory.

The readable source is [examples/floor.golem](examples/floor.golem):

```golem
float32 x Range(-32768,32767) = -0.00006103515625;
sint16 result;
Sub compute;
    result = sint16(floor(x));
EndSub;
```

Its equivalent [Rally source](examples/floor-rally.golem) spells out two data
buffers, fields, two scalar matrices and three intrinsic operations. The
negative fractional initial input should produce -1 when executed; the
optional -0.75 update has the same expected integer result.

## Source and backend boundary

| File | Role |
|---|---|
| `scalar_frontend.hpp` | Parses the deliberately small readable grammar and creates a typed `scalar::Spec` |
| `scalar_backend.hpp` | Allocates storage/helpers and emits scalar load, true floor and signed-word conversion; no parser or Rally source generation |
| `golemc_scalar.cpp` | Task-local CLI and output publication |
| `reference/` | Frozen Rally compiler and relevant tests, identified and hashed in `reference/provenance.json` |
| `test_scalar.cpp` | Full-stream comparisons to Rally, syntax/range rejections and bounded host updates |
| `run_checks.py` | Sanitizer builds, production compatibility baseline, preserved Rally boundary test, independent structural decoder and wire accounting |

The emission algorithms are extracted from Rally's `loadScalar`, `MatFloor` and
signed `storeWordFloor`. The backend uses typed field formats and scalar ranges;
it has no dependency on Rally's parser, Node/Arg representation, application
tables or game ABI. Allocation and packaging are intentionally minimal.

The floor expansion preserves the sign and detects fractional magnitude before
converting back to float. A tiny negative value must floor to -1, not zero.
Only after obtaining an integer-valued float does the signed word store apply
its bias/transform/XOR conversion. Directly substituting Rally's biased store
for true floor would lose this important behavior.

## Experimental syntax and numeric contract

One source file contains, in order, one `float32` declaration with an explicit
`Range(lo,hi)`, one zero-initialized `sint16` output declaration, and one named
`Sub ... EndSub` with the exact floor/conversion assignment shown above.
The float initializer is optional and defaults to zero. Bounds and values are
finite decimal literals; exponent notation, whitespace and `//` comments work.
Keywords and builtin names are case-insensitive; identifiers are case-sensitive
and cannot reuse reserved words. Names must be distinct. The current prototype
requires semicolons on all statements, including block delimiters. The language
specification was revised on 2026-10-11 to omit all statement terminators; that parser
change is explicitly deferred. `floor.golem` retains executable prototype syntax;
[examples.golem](examples/examples.golem) illustrates the revised design convention.

The input range must lie within -32768..32767, inclusive. This is deliberately
narrower than all values that could floor into a signed word. Both the initial
value and its binary32 rounding must satisfy the bounds. The host-facing
`scalar::inputUpdate(result,value)` performs the same finite/range checks for
later packets. Finite underflow to binary32 zero is allowed when zero satisfies
the bounds; signed zero and subnormal input bytes are preserved where representable.

The bounds are a host obligation. Raw external writes bypass these checks; the
generated job contains no float input-admission code. There is no automatic
cast, arbitrary expression parser, integer printing, general local-frame model,
recursion or callback support in this slice. The production compiler still
accepts its existing language and does not accept this experimental source.

## Resident layout and lifecycle

The provisional compatibility layout uses input ID 1000, output ID 1001,
scalar source/integral IDs 1100/1101, and job ID 2000. Private buffers/helpers
occupy IDs 2..21. These are compiler-selected implementation values chosen to
permit exact Rally comparison, not programmer-authored source requirements or
an accepted asset namespace. Installing another prototype with this policy
replaces those same IDs; multi-module allocation/linking is not implemented.

The root job holds 259 bytes of consecutive commands in one block. Three finite
conditional helper programs implement negative-fraction correction. Their
logical call graph is acyclic, with at most three program levels; this is not
a physical ESP32 stack qualification. Blocks contain command sequences and fall
through normally; there is no one-instruction-per-block requirement. Multiple
blocks within a procedure remain available as a later layout extension.

Every reference in this slice is a fixed buffer ID/offset, so there are no
computed-address instruction templates or patch relocation sites. Temporary
comparison words begin zero-filled so copying one byte leaves a clean upper
byte. Conditions compare buffer-fetched 16-bit words on both sides, retaining
the stock-firmware sentinel workaround. Each job reloads the input and resets
the state its conversion needs. Scratch is private to one serialized job;
reentrant or concurrent calls are outside the contract.

| Generated file | Purpose |
|---|---|
| `bootstrap.bin` | Raw VDU stream: enable stock affine support, clear/install owned buffers and programs, set initial input; does not call the job |
| `interface.json` | Named input/output and entry IDs, types, bounds and byte counts |
| `invoke.bin` | Six-byte call of the resident job |
| `update-call.bin` | Fifteen-byte input adjustment followed by the six-byte call |
| `cleanup.bin` | Clears exactly the 25 owned IDs; 150 bytes |

These files are raw VDU streams, not MOS executables. No loader, card deployment,
emulator profile or firmware was changed or run. Results remain in the output
buffer for subsequent VDP-side work; output readback is not included. The job
is finite and returns; block boundaries themselves do not yield to outer event
processing. Cleanup clears owned buffers; it does not restore the global affine
feature flag. Compilation and range failures preserve existing output files;
file I/O failure during publication can still leave a partial output set.

## Wire accounting

Installation includes the feature flag, all data/scratch/helper/root blocks,
clear headers and the initial value. Recurring input transfer is one four-byte
float32 adjustment plus a call. Optional cleanup and any output use/readback are
separate.

| Invocations | Install once + update/call each time | Reinstall identical bootstrap + call each time |
|---|---:|---:|
| 1 | 1,029 | 1,014 |
| 2 | 1,050 | 2,028 |
| 10 | 1,218 | 10,140 |
| 100 | 3,108 | 101,400 |

The comparison is an explicit reinstall-every-time baseline using identical
semantics and the same initialized input value. The resident path charges an
extra update even on its first call; using the installed initial value would
avoid that first 15-byte update. For different inputs, rebuilding/reinstalling
with that input costs the same byte count. This shows amortization for this
fixture, not measured application UART savings or a speed claim. The readability
change produces the same bytes as Rally; residency provides the recurring saving.

## Evidence and next increment

Qualification is recorded in [host-checks.json](host-checks.json), including
source fingerprints, exact compile commands and durations. Tests compare full
bootstrap/cleanup output for boundary values, tiny negative/subnormal inputs,
signed zero and narrowed domains; reuse Rally's exhaustive binary32-integer
boundary model; and run Rally's ordinary-language expectations against the
actual maintained compiler. The independent decoder checks all installed
blocks, command boundaries, copy/fetch spans, helper targets, cleanup ownership
and host packet formats. Negative CLI cases preserve existing outputs.

These checks validate extraction and generated structure. The frozen Rally
boundary test is a host mathematical model, and neither it nor the decoder
executes stock VDP firmware. Before production promotion, qualify repeated
invocations and numeric results with the real command interpreter under the
project's human validation workflow.

The next useful increment is to review this concrete source/backend boundary,
then integrate an agreed small slice with main's existing frontend and storage
policy. Generalize the typed numeric interface one operation at a time. Review
float input admission and the output's next VDP-side use before widening the
external interface; do not start a language-wide optimizer or native-module
project as a dependency.
