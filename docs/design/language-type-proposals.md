# Golem language design proposals: "what type of language is this?"

Status: **living document** - actively updated as decisions are made or
reconsidered. Not a frozen spec. See individual sections for current
status markers.

Started 2026-07-28, seeded from the discussion in
[docs/devlog/2026-07-28.md](../devlog/2026-07-28.md),
[docs/devlog/2026-07-28-execution-lifecycle.md](../devlog/2026-07-28-execution-lifecycle.md),
and [docs/devlog/2026-07-28-compiler-toolchain.md](../devlog/2026-07-28-compiler-toolchain.md).
Read those first for the platform constraints this document assumes
throughout: no MMU/indexed addressing (patch-then-execute instead), one
buffer block = one compiled instruction, 8/16-bit arithmetic as the fast
path with 32-bit "taxed" and comparisons hardware-capped at 16 bits, the
VDP is write-only from the eZ80's perspective (hard to introspect at
runtime), and the corrected `batch`/`event-resident`/`hosted` execution
lifecycle model.

This question is deliberately broken into independent axes - "what type of
language" bundles several genuinely separate decisions that don't have to
be resolved together or in this order.

## Axis 1: Paradigm

**Status: leaning imperative/procedural, not decided.**

- **Imperative/procedural, C/Pascal-like** (the working sketch since the
  first devlog). Maps directly onto the buffer/jump execution model:
  functions = buffers, control flow = conditional jumps, straight-line
  statement sequences. Lowest compiler-engineering lift, most familiar to
  write and reason about.
- **Structured assembler.** An even thinner layer - loops/`if`/functions
  as sugar over jumps, otherwise close to 1:1 with buffer commands, no
  real expression-level abstraction. Maximizes predictability of output
  size/timing (relevant given the 32-bit tax and 16-bit comparison cap -
  you can see exactly what a statement costs). Smallest possible compiler.
- **Stack-based / concatenative (Forth-like).** Worth naming explicitly
  given the retro fit, the "buffers as inert clay, animated by a command
  sequence" metaphor (a natural match for Forth's colon-definitions), and
  its historical track record on constrained hardware. Not part of the
  original sketch.
- **Event/reactive-first.** Given the `event-resident` lifecycle profile
  (VSYNC/input handled via bounded, VDP-dispatched callback buffers, not a
  permanently-running loop), a language that treats *handlers* as a
  primary top-level construct (`on vsync { ... }`, `on key { ... }`)
  rather than bolting event registration on as a library call could fit
  the platform's actual execution model unusually well.

## Axis 2: Type system strength

**Status: leaning Pascal-style static + range-checked, not decided.**

- **Minimal/weak, C-like.** Bytes/words/dwords, arrays, structs-as-byte-
  layout, no inference. Matches the metal closely, simplest compiler.
- **Pascal-style static + range-checked.** Stronger nominal typing,
  checked subranges/enums. Pascal's original selling point was catching
  bugs at compile time on hardware with a slow, awkward debug loop -
  that argument applies with unusual force here, since the VDP is
  write-only and genuinely hard to introspect at runtime (see the
  testability design note in
  [2026-07-28-vdu-buffered-api.md](../devlog/2026-07-28-vdu-buffered-api.md)).
  A stricter static type system is a deliberate mitigation for that: catch
  at compile time what can't easily be caught later.
- **Untyped/word-oriented (BCPL/Forth-style).** "Everything is a machine
  word." Fastest to compile, zero safety net. Given the same WOM argument,
  probably the riskiest option here, not just a stylistic one.

## Axis 3: Memory model

**Status: leaning static/arena-only, not decided.**

- **Fully static/arena only, no dynamic allocation.** Matches the original
  sketch ("globals get fixed bufferId/offset at compile time"). Simplest,
  fully deterministic, easiest to reason about with no runtime
  introspection available.
- **Region/bump allocation** (a "heap buffer," allocate-only, no free).
  Slightly more flexible for variable-lifetime data, still fully
  deterministic, no fragmentation/dangling-reference risk.
- **Manual alloc/free.** Most powerful, but fragmentation and dangling
  references are exactly the class of bug this platform is worst at
  helping find after the fact. Probably worth avoiding, or at least not
  defaulting to.

## Axis 4: Abstraction level / compiler ambition

**Status: leaning thin/structured-assembler for v1, not decided.**

- **Thin, literal, "structured assembler" for v1.** Smallest possible
  thing that could work; matches the Author's "start simple, probe
  boundaries" stance (see
  [2026-07-28-compiler-toolchain.md](../devlog/2026-07-28-compiler-toolchain.md))
  and is realistic for a first compiler written by someone still learning
  how compilers work.
- **A real small systems-language compiler** (proper IR, optimization
  passes). The more ambitious long-term target implied by the original
  devlog's three-address-code IR sketch, but a much bigger lift.

## Axis 5: Statement terminator

**Status: DECIDED - mandatory `;` as a statement separator.**

Unlike the axes above, this one is settled, not just leaning. `src/golemc.cpp`
originally treated the trailing `;` after its one statement form (`print
"...";`) as optional, which was harmless with only one statement form (the
next statement was always unambiguously recognizable by its leading
`print` keyword). That stops being true once more statement forms are
added that don't each start with a unique leading keyword (e.g. a bare
assignment or a function-call statement) - without a mandatory terminator,
the parser would need real lookahead/backtracking, or newline-significance
rules, to know where one statement ends and the next begins (the classic
"automatic semicolon insertion" trap JS is known for, where guessing
statement boundaries produces real, surprising bugs).

Decision: semicolons are a **mandatory statement separator**, Pascal/
Wirth-family style, not optional decoration and not newline-significant.
This is the simplest, least ambiguous choice for a recursive-descent
parser and matches the Pascal-style static-typing lean already adopted on
Axis 2 - both are instances of the same underlying preference (push
complexity/ambiguity out of the grammar and into explicit syntax, given
how hard this platform is to debug at runtime). Applied in
`src/golemc.cpp` (see
[docs/devlog/2026-07-28-golemc-milestone-and-vba-syntax-question.md](../devlog/2026-07-28-golemc-milestone-and-vba-syntax-question.md)
for the discussion this followed).

## Axis 6: First native integer type, and variable declaration semantics

**Status: DECIDED - `uint16` first, `sint16` next; naked declarations
zero-initialize.**

Grounded in `Buffered-Commands-API.md`'s command 5 (Adjust) and command 6
(Conditional) behavior: byte-wise two's-complement arithmetic can be
carry-chained to any width (16/24/32/40-bit add-with-carry are all
explicitly demonstrated), so width alone doesn't dictate "fast" - but
**comparisons are hardware-capped at 16 bits** (command 6's docs state
outright that larger comparisons "are not directly supported" and must be
decomposed across multiple buffers/conditionals), and **VDP Variables are
themselves natively 16-bit** (per command 48). 16 bits is therefore the
largest width where comparisons stay a single operation, and it matches
the native width of VDP Variables and most buffer IDs/offsets - making it
the natural default/native integer width for v1, not merely "an available
fast path."

Decision:

- **`uint16` is the first integer type to implement.** Unsigned avoids any
  question of comparison-operator sign semantics for the very first type,
  since command 6's LT/GT comparisons operate on raw bit patterns - the
  natural fit for unsigned values.
- **`sint16` follows next**, once `uint16` end-to-end codegen (declare,
  set, add, compare, conditional call/jump) is working. Two's-complement
  negation and addition (command 5's `Negate`/`Add`/`Add with carry`) are
  no different for signed values than unsigned - same bytes, same
  opcodes - but **signed ordering comparisons are an open verification
  item**: it's not yet confirmed whether command 6's LT/GT operate as a
  raw (unsigned) bit-pattern comparison or already account for the sign
  bit. If it's a raw comparison (the likely case for a simple buffered-
  command machine), Golem's compiler will need to emit extra instructions
  around signed `<`/`>`/`<=`/`>=` (e.g. an XOR-sign-bit trick, or explicit
  sign-bit checks) rather than a single Conditional command - check
  `vdu_buffered.h`'s `bufferConditionalCall`/comparison implementation
  before relying on either behavior, per `AGENTS.md`'s standing rule to
  treat the firmware source as the definitive tie-breaker over the prose
  docs.
- **Variable declarations may either set an initial value or be
  "naked"** (`uint16 foo;` vs `uint16 foo = 5;`). A naked declaration's
  value is zero. This is a direct consequence of not yet having any
  representation for "undefined" (no null/uninitialized-tracking - see
  Axis 3's static/arena-only memory lean, which has no facility for that
  kind of metadata anyway). Zero is also *free* to obtain, not just a
  convenient default: command 3 ("create a writeable buffer") explicitly
  zero-initializes on creation ("This new buffer will be a single empty
  single block upon creation, containing zeros"). If Golem's globals arena
  is allocated once via command 3 at program init, every naked declaration
  is correctly zeroed with no codegen at all - only declarations *with* an
  initializer need an emitted instruction (command 5, operation 2, `Set
  value`) to overwrite that zero with the initializer at load time.

## Axis 7: `print` is monomorphic; value-to-text conversion is explicit

**Status: DECIDED - `print` only accepts string-typed operands; printing
a non-string value (e.g. a `uint16`) requires an explicit conversion.**

Raised by `examples/scratch.golem`'s trick final line (`print qux;` where
`qux` is `uint16`). Real Pascal's `write`/`writeln` are polymorphic per
argument's static type (no `.toString()` needed), which would be a
defensible option here too - but it was rejected in favor of keeping
`print`'s codegen (and the type-dispatch decision) monomorphic and
explicit, consistent with the explicit-over-implicit bias already applied
on Axis 5 (mandatory `;`) and the IR's explicit-widening passes: an
explicit conversion (`print str(qux);`, exact spelling TBD) keeps the
value→text conversion logic in one reusable, visible place instead of
hidden inside `print`'s overload resolution - which matters more here than
in Pascal, since on this platform that conversion isn't free: there is no
VDP opcode for binary-to-decimal-ASCII conversion (command 5 has no
divide/mod), so it has to be a real runtime routine (repeated divide-
by-10/mod-10 to peel off digits, then reverse them) - see the "Standing
open questions" note immediately below.

### Standing open questions (not yet decided)

- **Does Golem have a first-class `string` type yet, or is `str()`
  compiler magic special-cased only in `print`'s argument position?**
  Today, a string literal in `print "...";` is just opaque bytes with no
  associated type in a type system - there's no design decision yet on
  what a `string` *value* is (a length-prefixed buffer? a fixed-size
  array of bytes? something else), what `str(qux)` actually produces, or
  whether strings become a real, nameable type at all in v1.
- **Does Golem need general integer divide/mod before it can do
  int-to-decimal conversion**, or is a hand-rolled, conversion-specific
  divide-by-10 routine acceptable for v1 without a general `/`/`%`
  operator existing yet? This is the first feature to need a genuine
  resident runtime routine (per `AGENTS.md`'s architecture notes on a
  runtime library living in VDP buffers) rather than single-instruction
  or literal-byte codegen.

## Axis 8: `str` type and its storage/allocation model

**Status: DECIDED (v1 scope) - `str` is a real type, named `str`, doubling
as its own conversion function (`str(intExpr[, fmt])`); each `str(...)`
call site gets its own compile-time-assigned buffer ID, populated on the
fly and cleared when done. General heap allocation is left open for
later, not ruled out.**

This resolves Axis 7's two standing open questions, and follows directly
from re-checking `Buffered-Commands-API.md` rather than assuming the
memory model needs to stay static/arena-only forever (Axis 3's lean was
about v1 simplicity, not a claim that the hardware can't support more):

- **Buffers already support create-on-write/grow/free primitives
  directly** - no synthesis needed. Command 0 ("Write block") auto-
  creates a buffer on first write and *"[writing] to the same buffer ID
  multiple times will add new blocks to that buffer"*, with **"the total
  size of a buffer... is not restricted to [65535 bytes]"** (only a single
  block-write call is capped at that). Command 2 ("Clear a buffer")
  *"clears out all of the blocks sent to a buffer... If the buffer does
  not exist then this command will do nothing"* - a genuine free, not
  just a zero-fill. So a heap-like allocate/grow/free model is directly
  supported by existing commands, confirming the Author's suggestion that
  a heap memory model is not hardware-precluded.
- **Important nuance this surfaces**: growth is by *appending a new
  block*, not resizing an existing block in place, and command 2 frees an
  entire buffer ID's worth of blocks at once - there is no "free just one
  block within a buffer." This makes "one shared buffer with many
  independently-lived blocks" a poor fit for a heap (freeing one
  allocation would clear all of them). The natural fit instead is **one
  buffer ID per allocation** - there are 65534 general-purpose buffer IDs
  available, so this is not a scarce resource for v1's needs.
- **`str` is a real type** (not just compiler magic confined to `print`'s
  argument position), and reuses its own name as a conversion function,
  Python-`str(x)`/`int(x)`-style: `str` is a reserved type keyword that is
  also legal in call position as a builtin conversion. The parser needs a
  small, deliberate carve-out for this, but it avoids inventing a second
  name for the same concept ("laborious casting expressions are...
  laborious").
- **Allocation strategy for v1: compile-time-assigned buffer ID per call
  site**, not a general runtime free-list allocator. Every `str(...)` call
  site in the source is assigned one fixed buffer ID at compile time;
  executing that call site writes fresh digit bytes into it (command 0,
  after clearing any prior contents with command 2) and the caller uses it
  immediately. This directly matches the "create it, use it, throw it
  away" pattern (simple cleanup, no allocator bookkeeping) at the cost of
  aliasing across recursion depth if a call site's result were ever held
  past the call that produced it - acceptable for v1 since `str()`'s
  result is meant to be consumed immediately (e.g. passed straight to
  `print`), not stored.
- **A true general heap (runtime free-list allocation, arbitrary
  lifetimes, arbitrary count) is explicitly left open for later**, not
  precluded - the hardware doesn't rule it out (see above), but it's a
  real runtime-subsystem undertaking (a searchable free-list structure)
  that isn't needed to unblock `str()` and shouldn't be scoped into v1 by
  default.
- **Signature: `str(intExpr[, fmt])`** - a plain function call with an
  optional second argument for a future format spec, rather than the
  VBA-flavored postfix-dot idea (`str(foo)."fmt"`) floated earlier. This
  reuses ordinary function-call-with-arguments grammar (new to golemc,
  but the conventional kind of "new" - unlike inventing postfix-dot
  syntax) and generalizes to any future builtin needing a literal config
  argument. **The format spec itself (`fmt`'s contents/semantics) remains
  entirely TBD** - only the call shape is decided here.

## Axis 9: Assignment/comparison operator spellings

**Status: DECIDED - C-family spellings: `=` assignment, `==` equality,
`!=` not-equal, `<=`/`>=`/`<`/`>` ordering comparisons.**

Raised while working through `qux = foo + bar;`: `=` is already doing
declaration-initializer duty (`uint16 foo = 10;`) and plain-assignment
duty, and Golem will eventually need an equality-comparison operator too
(for conditionals). VBA/BASIC-family languages let `=` mean both
assignment and equality, disambiguated only by context - explicitly
rejected here in favor of distinct tokens for each: `=` for
assignment/initialization, `==` for equality, `!=` for not-equal,
`<=`/`>=`/`<`/`>` for ordering.

Unlike C, `=` and `==` are used consistently for one job each (Golem never
overloads `=` to also mean equality the way BASIC does), so the classic
"context-dependent meaning" ambiguity C-likes are sometimes accused of
doesn't apply here in the same form. The residual risk is the narrower,
well-known C-family one: a programmer typo'ing `=` where `==` was meant
(e.g. inside a future `if` condition) is syntactically valid and changes
meaning silently. This is a known, accepted trade-off of choosing
familiar C-family spellings, not something this decision claims to solve
- flagging it here so it isn't mistaken for an oversight later.

## Axis 10: Arithmetic overflow policy

**Status: DECIDED - unchecked, silent wraparound; the programmer is on
the hook, same as hand-written assembly.**

This narrows Axis 2's "Pascal-style static typing with range checks"
language, which was mainly about catching type mismatches and wrong
assignments at compile time, not about runtime-checking arithmetic
bounds. Concretely: `uint16`/`sint16` arithmetic wraps silently on
overflow, matching the raw two's-complement byte behavior of command 5's
`Add`/`Add with carry`/`Negate` operations directly - no extra
overflow-detection instructions are emitted, and no runtime
error/trap mechanism exists (or is planned) to catch it. This is a
deliberate scope decision, not an oversight: building any kind of runtime
overflow trap would require inventing an error-signaling/abort mechanism
that doesn't exist anywhere on this platform yet, which is well beyond
what's needed to compile arithmetic expressions. Future range-checking
(if any) is more likely to apply to things like array-bounds access than
to plain arithmetic overflow.

### Addendum: eZ80-style flags, checked explicitly by the programmer

**Status: DECIDED (principle) - Golem should expose CPU-flags-style
overflow/carry information after arithmetic, but only as something the
programmer can explicitly check, never as an automatic trap.** "Unchecked"
above means the compiler doesn't *force* a check or *react* to overflow on
the programmer's behalf - it doesn't mean the information is thrown away.

This is a very natural fit for command 5's actual hardware behavior:
**"Add with carry" stores its carry-out in the byte immediately following
the target value** - *"the carry value will be stored in the byte at the
_next_ offset"* - and this is exactly how the docs show multi-byte adds
being chained (16/24/32/40-bit).

Correction to an earlier draft of this addendum: "produces a carry-out"
and "consumes a carry-in" are independent, just like real ADD vs. ADC on
the eZ80, and got conflated above. Decided split:

- **`foo + bar` for `uint16` is an ordinary, standalone add - it does not
  consume an incoming carry.** This is the common case and needs no
  carry-in from anywhere else.
- **Internal/runtime-library code (e.g. building wider arithmetic by
  chaining separately-stored 16-bit words) will explicitly distinguish
  add-with-carry-in from plain add, as the situation demands** - i.e.
  the ADD-vs-ADC choice is a real, meaningful choice at that level, not
  something to collapse into "always use with carry."
- Either way, a carry-*out* is still a natural byproduct of doing the add
  at all (nothing extra needs to be requested for it to exist) - it's
  produced regardless of whether a carry-in was consumed, mirroring how a
  real CPU's ADD instruction still sets the carry flag even though it
  didn't read it as input.

Following the real-CPU analogy through: this is most faithful as **one
shared "flags" location, updated by the most recent arithmetic op**
(like an eZ80 status register), not a permanent per-variable field -
consistent with assembly semantics where you check flags right after the
instruction that set them, before anything else clobbers them. This
avoids reserving extra storage per variable.

**Booleans can be set from the value of any flag, not just
carry/overflow** - `bool` is confirmed as the mechanism for capturing a
flag's value (e.g. something like `bool ovf = <flag>;`), generalizing
beyond carry/overflow to whatever other flag bits Golem ends up
maintaining (zero, sign, etc. - see open items below).

**v1 scope, decided:** only `uint16` addition detects overflow. No other
operation (`AND`/`OR`/`XOR`/`NOT`/`Negate`, or `sint16` arithmetic once it
exists) produces a checkable flag yet. Whether/how to extend flag
production to other operations and to zero/sign flags generally is a good
question but explicitly marked as a **follow-up, not needed now**.

**Flag lifetime, decided:** a flag is clobbered by the *next* operation,
exactly like real assembly - there is no grace period. This directly
raises the question of multi-operation expressions (e.g. `a + b + c`):
**decided that Golem does allow multiple operations within a single
expression, following standard operator-precedence/order-of-operations
rules** - which means only the *last* operation's flag survives to be
checked afterward; any intermediate operation's flag is transient and
unobservable unless the expression is deliberately broken up into
separate statements. This is a real, accepted consequence, not an
oversight: **if the programmer needs to check an intermediate step's
overflow, they must decompose the computation into separate statements
themselves** - the same discipline hand-written assembly already demands,
consistent with Axis 10's "programmer is on the hook" framing generally.

**Definite next-version (v2+) enhancement, not v1: a flags stack.**
Directly precedented by the eZ80's own `PUSH AF`/`POP AF` idiom - real
assembly already solves "I need to do other work before I can check this
flag" by saving the whole flags register onto the stack first and
restoring it later. Golem's future equivalent would let the programmer
push a flag snapshot before it would otherwise be clobbered by an
intervening operation, then review/clear entries on their own schedule,
rather than being limited to "only the immediately preceding operation's
flag is visible." Deliberately not designed further here (push/pop vs.
ring-buffer semantics, exact review/clear syntax, and which flags it
captures once more than carry/overflow exist are all open) - just
recorded so the idea isn't lost before v1 ships.

**Open items, not yet decided:**

- **Exact read syntax for a flag** is undecided (**deferred** - a named
  pseudo-variable read (`bool ovf = carry;`) vs. a builtin query call
  (`bool ovf = carry();`) are both plausible; Axis 8 leans toward
  function-call syntax for builtins generally, but flags read more like
  CPU-register state than a computation, so a pseudo-variable spelling
  isn't obviously wrong either).
- **Which flags actually exist "for free" vs. need real extra
  instructions to compute, beyond the v1 uint16-addition-only carry flag
  decided above** (**follow-up** - carry/overflow is a free byproduct of
  using Add-with-carry; nothing else in command 5's docs is described as
  auto-producing a zero flag, sign flag, or similar; those would likely
  need an explicit follow-up comparison against 0 via command 6, a real
  extra instruction, not a free byproduct).
- **`uint16` gets true overflow detection for free** this way - the
  add-with-carry carry-out *is* the unsigned overflow flag. **`sint16` is
  not free**: a carry-out of the top bit is not the same thing as signed
  overflow (which depends on the operand signs vs. the result sign, the
  classic "carry into MSB XOR carry out of MSB" computation) - this is
  the same family of open question as Axis 6's deferred signed-comparison
  verification item, and needs its own follow-up once `sint16` codegen is
  underway, not solved by this addendum.

## Axis 13: The addition primitive - concrete design

**Status: DECIDED and IMPLEMENTED (v1 scope: `uint16`, `+` only) in
`src/golemc.cpp`.** Answers Axis 11's deferred "where do expression
temporaries live" question for the one case that actually exists so far.

**The correctness problem this solves:** command 5's `Add with carry`
*always* writes its carry-out to the byte immediately following its
target run - the hardware gives no way to suppress that. If an add wrote
directly into a destination variable's own storage, and that variable
isn't the last thing in memory, the carry-out write would silently
clobber whatever is laid out right after it. Combined with Axis 10's
decision *not* to give every variable its own spare padding byte, this
means **a destination variable can never safely be the direct target of
a carry-producing add.**

**The fix: route every addition through one dedicated scratch
accumulator (`ACC`), never directly into the named destination.** `ACC`
is a 3-byte buffer: offset 0-1 is the running value, offset 2 is the
carry-out landing byte - which **is** the shared "flags" location from
Axis 10's addendum, not a separate concept. Lowering for
`dest = term1 + term2 + ... + termN;` (Axis 10's addendum already decided
multi-operation expressions are allowed):

1. `Set` (operation 2) `term1` into `ACC` (offset 0, count 2).
2. `Add with carry` (operation 4) each subsequent `term_i` into `ACC`, in
   order. The final carry-out lands harmlessly at `ACC` offset 2 - safe,
   because `ACC` is its own dedicated buffer with guaranteed spare room
   there, unlike a named variable.
3. `Set` (operation 2) `ACC` (offset 0, count 2) into `dest`.

**A useful refinement found while implementing this: `Set` has no
trailing side-effect** (only `Add with carry` writes an extra byte), so a
**plain copy/single-term assignment** (`dest = foo;`, or a declaration
initialized with a single literal/identifier) **can `Set` directly into
`dest`, skipping `ACC` entirely** - `ACC` is only needed once an actual
addition (two or more terms) is involved.

**Wire encoding used (all via command 5's "multiple target values"
(`&40`) + "multiple operand values" (`&80`) modifiers together, always
targeting a 2-byte run):**

- Literal operand (term is an integer literal): operation byte =
  `base | 0x40 | 0x80` (`0xC2` for `Set`, `0xC4` for `Add with carry`),
  followed by `offset;`, `count;` (=2), then the 2 literal operand bytes
  (little-endian) - this is the exact shape `Buffered-Commands-API.md`
  demonstrates for chained 16/24/32/40-bit literal adds, just reusing it
  for `Set` too.
- Buffer-fetched operand (term is another variable): operation byte =
  `base | 0x40 | 0x80 | 0x20` (`0xE2` for `Set`, `0xE4` for
  `Add with carry`), followed by `offset;`, `count;` (=2), then the
  source `bufferId;offset;` pair - mirroring the docs' cross-buffer `AND`
  example (there for operation `AND`; here for `Set`/`Add with carry`).
  **Caveat, not fully verified:** the docs demonstrate this exact
  modifier combination for `AND`, not for `Set` - using it for `Set` too
  is inferred by analogy (the docs frame the modifier bits as generically
  combinable with any operation) rather than confirmed verbatim. Worth
  checking against `vdu_buffered.h` before trusting it further, per this
  project's standing rule for ambiguous hardware behavior.

**Buffer ID allocation (this toy compiler only):** each declared `uint16`
variable gets its own dedicated 2-byte buffer (offset 0 always) rather
than a shared/offset-tracked globals arena - buffer IDs are abundant
(65534 available), so this sidesteps building an arena allocator just to
get real declarations working. Buffer ID 0 is reserved (it's the compiled
program's own buffer, per `loader.asm`'s `program_buffer_id`); `ACC` is
buffer ID 1 (allocated/zero-initialized lazily, only the first time an
addition actually appears in a program, so `print`-only programs like
`hello.golem` are unaffected); variables are assigned buffer IDs 2, 3, 4,
... in declaration order. A real (non-toy) compiler would likely want the
shared-arena model Axis 3/6 already leans toward instead, once buffer-ID
volume or cross-program layout stability actually matters.

## Axis 11: Expression-temporary placement is a compiler concern

**Status: DECIDED (principle, not yet a concrete mechanism) - where
intermediate values live during expression evaluation is entirely up to
the compiler; the only thing the programmer should ever observe is that a
named variable (e.g. `qux` in `qux = foo + bar;`) keeps its identity.**

Command 5 (Adjust) is fundamentally destructive-in-place ("add value Y
into buffer X's existing bytes"), not a pure `a op b -> result` operation,
so even this trivial example already needs at least two steps under the
hood (copy `foo`'s value into `qux`'s storage, then Adjust `qux` by adding
`bar`'s value) - and nested/more complex expressions will need actual
temporary storage that doesn't correspond to any named source variable at
all. The decision here is only that this is entirely the compiler's
problem to solve, invisible to Golem source code - **not** a decision
about the concrete mechanism (e.g. reusing a fixed pool of scratch buffer
IDs, a stack-like temp allocator, or something else).

**Open problem flagged, explicitly deferred, not needed to compile
`qux = foo + bar;`:** the real constraint on any temp-allocation strategy
isn't correctness, it's the **finite buffer-ID budget** (65534 general-
purpose IDs, per `Buffered-Commands-API.md`'s introduction) and the
finite total buffer memory budget (see Axis 12). A naive "hand out one
fresh buffer ID per sub-expression, forever" strategy could exhaust
either budget under deep expression nesting or recursion. Whatever
mechanism eventually gets built needs to reuse/bound its buffer
consumption rather than growing unboundedly - left as a real, named,
future problem rather than something this axis claims to solve.

## Axis 12: Runtime-library inclusion is dependency-driven, not declared by the programmer

**Status: DECIDED (direction) - Author provisionally agrees this is the
right direction.** Resident runtime-library support (float arithmetic,
wide-int helpers, string conversion, and whatever else gets added) is
included in a compiled program automatically, based on what the source
actually uses - **no `import`/`use`/`include` statement is required or
planned for this**, and the compiler should not simply ship the entire
runtime library "for safety" either.

Reasoning:

- **There's nothing for an import statement to *declare* here.** Golem
  has no separate compilation units and no user-extensible standard
  library - every runtime routine is a compiler intrinsic the compiler
  already knows about. In C, `#include` exists to tell the compiler a
  declaration exists, which is a different job from what the *linker*
  does (only pulling in object code actually referenced). Golem doesn't
  have that declaration/definition split at all, so the only real
  question is linking granularity, not whether some declaration needs
  announcing.
- **The type system itself is a sufficient, unambiguous trigger.** The
  moment source declares or operates on a `float` (once it exists), the
  compiler already knows, mechanically, that float support is needed -
  the same way it already knows a `uint16 +` needs the add-with-carry
  runtime path. No separate signal from the programmer is needed for
  anything mechanically tied to the type system.
- **Unused runtime code isn't just wasted space here, it's wasted
  transfer time on every single run.** Per `AGENTS.md`'s loader model,
  the whole compiled program ships as one blob that gets decompressed and
  fanned out to buffers, and sending data to the VDP is an explicitly
  documented bottleneck (`Buffered-Commands-API.md`'s own advice to send
  large data in small chunks specifically because of how long it takes).
  This is a sharper cost than "extra disk space" on a desktop OS, and
  argues for real dead-code elimination, not just avoiding it out of
  tidiness.
- **Preferred granularity: function-level, not whole-module**, i.e. a
  real usage/call-graph analysis (including transitively - float-divide
  might itself call a shared shift/multiply helper), so that e.g. a
  program only calling float-multiply doesn't also pay to ship
  float-divide. This is more implementation work than "did this program
  touch the float type at all -> ship the whole float module," and
  whole-module inclusion is an acceptable fallback if function-level
  analysis turns out to be more than v1 wants to build - but function-
  level is the target, not whole-module.
- **This applies to core, type-triggered primitives only.** If Golem
  ever grows genuinely optional higher-level library content that isn't
  mechanically tied to the type system (a sorting routine, a graphics
  helper library, something a whole program could easily exist without
  touching), that's a different case where "did the programmer actually
  want this" isn't mechanically derivable the same way, and an explicit
  `use`/`import` might legitimately be warranted there. Not needed for
  v1 - nothing like that exists yet - and not something this axis
  decides one way or the other.

**Total buffer memory budget (confirmed by the Author, not documented in
the API docs themselves):** there is no fixed total - it's whatever VDP
RAM is left over after a given firmware build is flashed, observed to be
in the **~4MB range**, but **the system has been observed getting unstable
well before that ("antsy") somewhat above ~3MB**, based on symptoms seen
when loading it up with image bitmaps. Two practical consequences for
Golem: (1) this budget is not fixed across firmware versions/hardware, so
nothing should hard-code an assumed ceiling; (2) a comfortable working
budget for a compiled program's total resident footprint (runtime
library + program buffers + globals + heap/scratch usage) is meaningfully
less than the nominal ~4MB, likely under ~3MB, though the exact safe
threshold hasn't been characterized (no root cause identified yet for the
"antsy" symptoms - could be VDP-side memory pressure generally, not
necessarily a hard buffer-specific limit). Worth real measurement once
there's enough of a compiled program to test with, rather than trusting a
number pulled from anecdotal bitmap-loading symptoms.

## Axis 14: Comment syntax

**Status: line comments leaning `#`, not yet decided; block comment
convention entirely open.**

- **Line comments: `#` is the Author's stated preference** ("I really want
  to use `#` to comment out individual lines"), not yet formally decided
  but strong enough to record as the working lean. Note this is a change
  from what the toy compiler currently implements - `src/golemc.cpp` and
  `examples/hello_golem/hello.golem` both use C-style `//` today (carried
  over unreflectively from the host implementation language, not a
  considered Golem-syntax decision) - so adopting `#` is a real (if small)
  breaking change to the one example program and to `golemc.cpp`'s
  `skipWhitespaceAndComments`, not just documentation.
- **Block comments: genuinely undecided, open for discussion.** Options
  worth putting on the table when this comes back up:
  - **No block comments at all** - only `#` line comments, "comment out a
    range" done by prefixing every line. Simplest possible lexer (no
    nesting-vs-non-nesting question to answer), consistent with this
    project's general bias toward minimal/unambiguous grammar (e.g. Axis
    5's mandatory statement terminator), but loses a genuinely convenient
    editing affordance.
  - **Paired delimiters, C-style (`/* ... */`, non-nesting)** - familiar,
    but reintroduces a `/`-based delimiter alongside a `#`-based line
    comment, two unrelated comment sigils in one grammar.
  - **Paired delimiters keyed off `#` itself** (e.g. something like
    `#( ... )#` or a repeated-`#` fence) - keeps a single "this is a
    comment" sigil family instead of mixing `#` and `/`. Nesting behavior
    (does a block comment nest across another block-comment start inside
    it?) would still need its own decision either way.
  - **Pascal-style paired delimiters** (`{ ... }` or `(* ... *)`) - fits
    this project's general Pascal-family leanings elsewhere (Axis 2, Axis
    5), but `{`/`}` may be wanted later for actual block-statement syntax
    depending on how Axis 1's paradigm question resolves, which would
    make reusing them for comments ambiguous.
  - No lean expressed yet toward any of these - listed only to give the
    next discussion a concrete menu instead of starting from a blank page.

## Axis 15: Block/control-flow statement syntax

**Status: leaning away from curly-brace blocks, toward VBA/BASIC-style
keyword-delimited blocks - "at least i don't think so yet" (Author's own
hedge), so treat as a strong lean, not yet a final decision. Exact keyword
spellings within each construct are open.**

- **No `{ }` block enclosures.** Explicitly rejects the C-brace-block
  convention that Axis 1's "C/Pascal-like" framing might otherwise have
  implied by default - this axis is really about *block delimiting*
  specifically, independent of Axis 2's typing-style lean (which stays
  Pascal-ish regardless of how blocks are spelled).
- **`If ... Then ... Else ... EndIf`** (VBA idiom) is the preferred
  conditional form. Open: exact closing-keyword spelling (`EndIf` vs
  `End If` as two tokens vs something else), and whether a single-line
  `If ... Then <stmt>` (no `Else`/`EndIf` needed) form is also allowed the
  way classic BASIC dialects often do.
- **`Do While ... Loop` / `Do Until ... Loop`** (VBA idiom) preferred for
  condition-checked-first loops. Open: whether the post-condition variants
  VBA also has (`Do ... Loop While ...` / `Do ... Loop Until ...`,
  condition checked *after* the body runs at least once) are in scope too,
  or whether v1 only supports the pre-condition form.
- **`For i = 1 To 100 ... Next i`** (VBA idiom) preferred for counted
  loops, with the Author explicitly noting the appeal of **naming the
  loop variable again at `Next`** ("`Next i`") as "informatively
  self-closing" - i.e. a loop's close is self-documenting and
  cheaply self-checking (a mismatched `Next i` vs `Next j` in nested loops
  is an obvious, easy-to-catch authoring error) rather than a bare `Next`/
  `}` that gives no such signal. Open: whether repeating the variable name
  at `Next` is mandatory or optional, and whether a `Step` clause (VBA's
  `For i = 1 To 100 Step 2`) is in scope for v1 or deferred.
- **Relationship to Axis 5 (mandatory `;` terminator): DECIDED - every
  *statement* ends in `;`, including block-delimiting keyword statements
  themselves, no exceptions.** Raised concretely while drafting a worked
  `For` example in `examples/scratch.golem` (see
  [docs/devlog/2026-07-29-loop-tail-call-prototype.md](../devlog/2026-07-29-loop-tail-call-prototype.md)'s
  follow-on discussion): the original phrasing above ("these are
  block-*delimiting* keywords, not statement terminators") left it
  ambiguous whether `For i = 1 To 50` and `Next i` themselves needed a
  trailing `;`. Leaving that ambiguous is exactly the kind of "some
  statements need it, some don't, good luck remembering which"
  inconsistency Axis 5 was adopted specifically to avoid (same underlying
  rationale: push ambiguity out of the grammar/author's memory and into
  explicit, uniform syntax). Resolution: `;` is required after **every**
  statement without exception - `For i = 1 To 50;` ... `Next i;`,
  `If x Then;` ... `Else;` ... `EndIf;`, `Do While x;` ... `Loop;` - not
  just the statements nested inside a block. One rule, zero special
  cases, nothing to memorize per-construct.
  **Important distinction (statement vs. line, not to be conflated):**
  this is a statement-terminator rule, not a line-terminator rule -
  Axis 5 already decided `;` is "not newline-significant", and that is
  unaffected here. Line breaks (as used in `examples/scratch.golem`'s
  formatting, e.g. putting `For`, its body, and `Next` each on their own
  line) are purely a readability convention with zero grammatical
  meaning. It is equally valid - if not stylistically encouraged - to
  write the entire `For` loop, or an entire program, as one physical
  line: `For i = 1 To 50; print "Hello "; Next i;` is exactly as valid
  as the multi-line form.
- **Note for Axis 14 (comment syntax):** Axis 14 flagged `{`/`}` as
  potentially wanted later for block-statement syntax, which would have
  made reusing them for a block-comment delimiter ambiguous. This axis's
  lean away from curly-brace blocks removes that particular concern, for
  whichever of Axis 14's block-comment options end up chosen.

## Axis 16: Increment/decrement operators `++` / `--`

**Status: DECIDED in principle (Author: "i do like those" - a firm want),
concrete lowering NOT yet designed - has a real open dependency, noted
below, that needs resolving before this can actually be implemented.**

Notable eclecticism worth naming explicitly: this reaches for C-style
`++`/`--` operator tokens in the same breath as Axis 15's BASIC/VBA-style
block keywords - Golem is not committing to one single "family" for
surface syntax wholesale, borrowing whichever idiom is preferred per
construct. Not a problem, just worth being honest about rather than
implying a purity that isn't the actual design goal.

**The real open dependency: `--` (and binary `-` generally) has no
primitive to lower onto yet.** Axis 13 designed addition (`+`) in detail
against command 5's actual base operations - `NOT`(0), `Negate`(1),
`Set`(2), `Add`(3), `Add with carry`(4), `AND`(5), `OR`(6), `XOR`(7) -
and there is **no dedicated subtract/subtract-with-borrow operation** in
that list. Subtraction will need its own designed lowering (most likely
two's-complement: `Negate` the subtrahend, then `Add with carry`, mirroring
how real CPUs without a dedicated SUB instruction handle it) before `--`
can be implemented for real - this has **not** been designed yet and
should get the same treatment Axis 13 got (verify against
`Buffered-Commands-API.md`/`vdu_buffered.h` before trusting any encoding
guess), not be quietly improvised as a side effect of adding `++`/`--`.

**`++`/`--` as sugar, once subtraction exists:** the natural design (not
yet formally decided, but the obvious shape) is for `x++;`/`x--;` to
desugar to `x = x + 1;`/`x = x - 1;` and reuse whatever the addition/
subtraction primitives already do - **not** a special-cased direct
`Add with carry`/`Negate` straight into `x`'s own 2-byte variable buffer.
This matters for the same reason Axis 13 exists at all: `x`'s buffer is
exactly 2 bytes (Axis 13's per-variable allocation scheme), so a carry-out
write landing at offset 2 would spill outside that buffer's allocated
block - going through `ACC` (as any multi-term addition already does)
sidesteps that hazard for free, rather than needing a second bespoke
"direct in-place adjust" code path just for `++`/`--`.

**Open items, not yet resolved:**
- Prefix vs. postfix vs. both (`++x` vs `x++`) - and, if both are
  supported, whether Golem bothers distinguishing pre/post *value*
  semantics in an expression context (`y = x++;`) or only supports `++`/
  `--` as a standalone statement (`x++;` on its own line), sidestepping
  that whole question. The statement-only reading is simpler and matches
  this project's general "thin, unambiguous grammar" bias (Axis 5, Axis
  15's block-keyword leaning) - worth strongly considering, but not yet
  decided either way.
- The subtraction/borrow primitive design itself (see above) - blocks
  real implementation of `--` and binary `-` alike.

## Axis 17: The loader's ultimate target shape - a command-line-driven MOSlet

**Status: leaning, far-future - not needed for anything currently being
built. Today's loader (`examples/hello_golem/loader.asm`) is explicitly a
"quick and dirty hack to get our feet wet" (Author's own framing) and is
fine to remain exactly that for now; this axis just records where it's
ultimately headed so that direction isn't lost.**

Today's loader is a fixed, single-purpose MOS program: it always reads a
file literally named `program.bin` and always writes it into buffer 0,
with no command-line input at all (see `loader.asm`'s own header comment
acknowledging it's a "quick proof-of-concept," not the real thing).

**The ultimate goal: the loader becomes a "MOSlet"** - a real, documented
MOS concept (`/home/smith/Agon/agon-docs/docs/MOS.md`'s "Moslets" section
and `docs/mos/Executables.md`/`docs/mos/Modules.md`), not a project-coined
term: MOSlets are small programs whose purpose is specifically **to add
new star-commands to MOS**. Concretely, per the official docs:

- **Fixed load address `0x0B0000`** (the dedicated "moslet area" of the
  Agon memory map), not the ordinary standalone-program load address
  `0x040000` that today's `loader.asm` actually targets (`.org 0x040000`)
  - becoming a real MOSlet means retargeting that origin address, not
    just a naming/packaging change.
- **Hard size cap: must fit in 32KB**, since the moslet area is exactly
  `0xB0000`-`0xB7FFF`.
- **Discovery by convention, not registration**: MOS looks in a
  dedicated moslet folder (`/mos/` by default, overridable via the
  `Moslet$Path`/`Run$Path` system variables from MOS 3.0 onward) for a
  file named `<command>.bin` whenever the user types `<command>` and it
  isn't a built-in - i.e. dropping a `golem.bin` moslet into that folder
  is what makes typing `golem ...` work as a star-command at all, with no
  separate install/registration step.
- **Caveat surfaced by this research, not previously considered**:
  MOSlets share their fixed memory region with MOS's own module-loading
  mechanism (`docs/mos/Modules.md`) - a moslet must declare itself "module
  safe" (doesn't touch the moslet area for its own data) or "module
  compatible" (MOS transparently saves/restores the moslet area around a
  module load) in its executable header. This is a real constraint a
  future `golem` MOSlet will need to account for, not just a size limit.

**That said, this remains genuinely far-future and not yet a plan** -
the sketched CLI shape below is unaffected by these facts (it's about
what arguments the MOSlet accepts, not the loading mechanism), they just
mean "make it a MOSlet" is now a concretely scoped retargeting job (org
address, size budget, module-safety flag) rather than a vague label.

**Command-line parsing prototype**: see
[reference/flower-moslet-cli/](../../reference/flower-moslet-cli/) - a
vendored (not yet implemented/adapted) copy of the `agon-utils` `flower`
example's argv/argc tokenizer and verb-dispatch helpers, kept as a
concrete starting point for whatever eventually parses `golem load` /
`golem run` / `golem clear`'s command-line arguments inside a real
MOSlet.

Sketched syntax (exact spelling/verb set not decided, just the shape):

- `golem load "hello.golem"` followed separately by `golem run` - load and
  run as two distinct steps.
- `golem run "hello.golem"` - load-and-run as one combined convenience
  step (presumably the common case).
- `golem clear` - unload/free all buffers currently in use by a loaded
  Golem program (a thin wrapper over the Buffered Commands API's command 2
  "Clear a buffer," called across whatever buffer IDs the running program
  is known to occupy).

Note the argument in these examples is written as `"hello.golem"` (the
*source* file), which would imply either the MOSlet invokes `golemc`
itself (unlikely to be MOS-side, given `golemc` is a Linux host-side C++
tool per the compiler-toolchain devlog) or, more plausibly, that by this
point "compile" and "load" have merged into one conceptual step from the
user's perspective even though compilation still genuinely happens on a
desktop host beforehand - exact division of labor between `golemc` and
`golem run` is not decided, just flagged as a wrinkle worth resolving
before this is built for real.

**Why this matters beyond convenience: it's a stepping stone toward
runtime program chaining.** If loading/running/clearing are themselves
ordinary MOSlet-invocable actions rather than baked into one fixed
load-then-call sequence, a *running* Golem program could in principle
shell out to `golem run "next.golem"` (or equivalent) to hand off to
another compiled program at runtime, rather than every possible program
needing to be one single compiled unit. **Explicitly flagged by the
Author as "a very far future design, but something i am thinking about"**
- not being designed further here (what state, if any, survives a
hand-off; whether the outgoing program's buffers need explicit `golem
clear`ing first given no per-block free exists per Axis 8; whether this
is even compatible with the `batch`/`event-resident`/`hosted` execution
lifecycle model) - recorded only so the direction is on the record before
any of today's quick-and-dirty loader hacking accidentally forecloses it.

## Axis 18: Case sensitivity - keywords insensitive, identifiers and filenames sensitive

**Status: DECIDED and IMPLEMENTED in `src/golemc.cpp` - reserved keywords
(`print`, `uint16`, `For`, `To`, `Next`, ...) are matched case-
insensitively; variable identifiers are case-sensitive; filenames are
case-sensitive.**

This is a deliberate three-way split, not a single blanket rule:

- **Keywords are case-insensitive** (`for`, `For`, and `FOR` are all the
  same keyword). This matches the BASIC/VBA-family ergonomics Golem's
  block-statement syntax already leans on (Axis 15/16's `For`/`To`/`Next`,
  chosen partly for that familiarity) - real BASIC dialects are
  traditionally forgiving about keyword casing, and there's no benefit to
  Golem being stricter here than the family of languages it's borrowing
  surface syntax from. Implemented in `matchesKeywordAt` via an ASCII
  `tolower` comparison, rather than requiring an exact byte match.
- **Variable identifiers are case-sensitive** (`foo` and `Foo` are two
  distinct variables). This is the opposite lean from Pascal (which
  case-folds identifiers too) and matches the C-family convention
  instead, consistent with this project's broader pattern of picking
  C-family spellings for things a programmer types often (see Axis 9's
  `==`/`!=`/etc.) while picking BASIC/Pascal-family spellings for
  structural/keyword syntax. `parseIdentifier` and the `variables_` map
  in `Compiler` already worked this way with no change needed - only
  keyword matching needed updating.
- **A consequence worth stating explicitly, not a separate rule:** since
  keyword matching happens unconditionally before an unmatched token
  falls through to identifier parsing, a variable can never be named
  `print`, `uint16`, `for`, `to`, or `next` in *any* casing - the keyword
  check matches first regardless of the casing used, so e.g. `uint16
  Print;` is still a redeclaration-of-a-reserved-word error, not a
  legal variable named `Print`.
- **Filenames are case-sensitive** - this isn't really a Golem-language
  design decision so much as an observation that `golemc` treats its
  command-line arguments as literal host-filesystem paths with zero
  interpretation (see this file's header comment on being extension-
  agnostic), and Linux filesystems are case-sensitive - so filename
  casing behavior is inherited from the host OS, not enforced or
  special-cased by `golemc` itself. Worth flagging as a possible future
  asymmetry if Golem tooling ever runs somewhere case-insensitive
  (MOS/FAT-based filesystems are traditionally case-insensitive) rather
  than claiming it as a considered cross-platform decision.

## Axis 19: Identifier syntax - no type-sigil decoration

**Status: DECIDED - already matches `src/golemc.cpp`'s existing
`isIdentStart`/`isIdentCont`/`parseIdentifier`, no code change needed.**

Raised directly: BASIC/VBA-family languages (the same family Golem
borrows `For`/`To`/`Next` surface syntax from, per Axis 15/18) often
encode a variable's type into its *name* via a trailing sigil (`foo$` for
string, `foo%` for integer). Golem does **not** do this, and shouldn't -
Axis 8 already gave `str` a real, declared, nameable type (`str foo;`,
same shape as `uint16 foo;`), so a sigil would be pure redundant
decoration duplicating information the declaration already states, not a
functional necessity the way it was in classic line-numbered BASIC
(which often had no separate declaration statement for a variable to
carry that information on). On this axis Golem follows its Pascal/C-
family typing lean (Axis 2), not its BASIC-family surface-syntax
borrowings elsewhere - the two are independent choices, and nothing
requires them to be resolved the same way.

Identifier character rules (already implemented, stated here as a formal
decision rather than an implementation accident):

- **First character:** an ASCII letter or `_`. Never a digit - this
  removes any lexing ambiguity between a leading-digit identifier and a
  numeric literal (`1foo` unambiguously starts as the numeric literal
  `1`, not an identifier) without needing lookahead to disambiguate.
- **Subsequent characters:** ASCII letters, digits, or `_`.
- **No operator-, punctuation-, or quote-like characters are ever legal
  in an identifier** - only letters/digits/`_`, full stop. This is a
  blanket rule, not a case-by-case denylist: anything that isn't a
  letter, digit, or `_` simply isn't in the identifier character set at
  all, so it's automatically excluded, including every current and future
  operator/concatenator spelling (`+`, `-`, `.`, etc.) and both quote
  characters (`"`, `'`) - the latter also has the nice side effect that
  an identifier can never be lexically confused with the start of a
  string literal.
- **`_` is the one non-alphanumeric character allowed.** It's
  conventional word-joining punctuation in every C/Pascal-family
  language, not "operator-like" despite its dash-like glyph.
- **No sigil characters are reserved for future type-tagging use** - since
  Axis 8/2 already settled that types are a declaration-time property of
  a name, not encoded in its spelling, there's no anticipated future need
  to reserve e.g. `$`/`%`/`&`/`!` the way some BASIC dialects do for
  other type suffixes (`long%`, `single!`, etc.).

## Axis 20: String literal escape sequences

**Status: DECIDED (v1 scope) - already matches `parseStringLiteral`'s
existing implementation: `\\`, `\"`, and `\n` are the only recognized
escapes. No sigil, doubled-quote (`""`), or other escaping mechanism
exists or is planned instead.**

Raised directly: how does a Golem string literal include a literal `"`
character? Backslash-escaping, C-style: `\"` for a literal quote, `\\`
for a literal backslash (needed so a literal backslash is never
ambiguous with the start of some other escape), `\n` for a newline. This
was already implemented alongside `print`'s original string-literal
support and simply hadn't been written up as a formal decision until
now.

- **`'` is not a special character anywhere in the grammar and never
  needs escaping.** Golem has exactly one string delimiter (`"`) and no
  character-literal type, so a bare `'` inside a `"..."` string is just
  an ordinary character, and `'` cannot itself be used as an alternate
  string delimiter (unlike e.g. Python or shell).
- **Not yet decided, explicitly deferred, not needed for anything built
  so far:** additional escape letters (`\t`, `\r`, `\0`), hex/numeric
  escapes (`\xNN`), or a "doubled delimiter" alternative (`""` meaning a
  literal `"`, as in Pascal) are all reasonable future additions that
  don't conflict with anything decided here - this axis only fixes what
  currently exists, not a ceiling on what could be added.

## Axis 21: `print` never appends a CR/LF

**Status: DECIDED and IMPLEMENTED (changed 2026-07-29) in
`src/golemc.cpp` - `print "...";` emits exactly the string literal's
decoded bytes and nothing else. The original implementation
unconditionally appended a trailing `13, 10` (CR LF) after every
`print`'s text; that behavior has been removed.**

Rationale: a programmer issuing several `print` statements in a row
(most concretely, inside a `For` loop body - see `examples/loop_golem/`)
should get their output concatenated on one line by default, not one
line per `print` call whether they wanted that or not. Forcing a
linefeed on every `print` would make the common "build up one line
piece by piece" pattern impossible without an awkward workaround (e.g. a
single giant literal, or string concatenation before any concatenation
operator even exists). This also makes `print` consistent with Axis 20's
escape sequences: a newline is now something the programmer asks for
explicitly (`print "line one\n";`), the same way any other character in
the output is explicit, rather than a hidden side effect bolted onto one
particular statement form.

This is a real behavior change from the original toy compiler (not just
a doc update) - `examples/hello_golem/hello.golem`'s single `print`
literal was updated to end in an explicit `\n` to preserve its prior
on-screen appearance (a compiled program returning to the MOS prompt on
a fresh line), and `examples/loop_golem/loop.golem`'s `print "Hello ";`
was left as-is, since the whole point of that example is now to
demonstrate 50 concatenated `print`s producing one line of repeated text
rather than 50 separate lines.

## Axis 22: Null-terminated strings - compiler-appended, not programmer-written; *where* it's needed is still open

**Status: DECIDED (principle only) - if/where Golem strings carry a
trailing `0x00` terminator, it is always the *compiler* that appends it
as part of codegen, never something the Golem programmer writes in
source (no `\0` needed, expected, or meaningful to type at the language
level). Exactly *where* and *when* the compiler needs to append one is
explicitly left open - not needed for anything built so far, and
deliberately not decided prematurely.**

Raised directly, matching established MOS/eZ80-assembly convention: the
Author's own firsthand assembly experience confirms MOS/VDP-adjacent code
commonly uses null-terminated strings, and this repo's own vendored
reference material independently corroborates it -
`reference/flower-moslet-cli/flower.asm` explicitly documents its argv/
dispatch tokens as "zero-terminated, case-sensitive strings" (see its
`Zero-terminate the token` comments and its zero-terminated-string
compare routine). Precedent also exists one level up, in the host
language `golemc` itself is written in: a C string literal (`"foo"`)
already gets an implicit, compiler-appended trailing `'\0'` that the
programmer never writes either - so this isn't only an MOS-specific
idiom being imported, it's the same convention C itself already uses for
exactly this reason (a string's own bytes carry their own end-marker, so
code consuming it via a raw pointer doesn't need a separate length
value passed alongside).

**A second valid convention also confirmed to exist, not to be
forgotten:** per the Author's own experience writing MOS/VDP assembly,
an explicit pointer-plus-length calling convention (telling a routine
exactly how many characters to output, rather than having it scan for a
terminator) is also genuinely used in practice at the assembly level -
it's not null-termination-or-nothing. The Author's stated preference is
still to reach for null-termination "almost always... because it works
so much better" (one pointer argument beats a pointer+length pair for
most call sites), which is a real ergonomic argument worth recording in
favor of null-termination as Golem's likely eventual default - but
without foreclosing the length-passing alternative for whichever
specific case turns out to need it.

**Why "where" genuinely can't be answered yet, and the key distinction
that will decide it:** it depends entirely on *which* underlying
mechanism ends up consuming a given string, and Golem currently has (at
least) two structurally different cases that could easily want different
answers:

- **Compile-time-literal `print` text (today's actual codegen)** is
  embedded directly as raw bytes inside a command-0 "write block," and
  displayed by simply being *replayed* as VDU stream bytes when its
  buffer is called (see Axis 21 and this file's header comment on
  `print`'s literal-byte-passthrough codegen) - the Buffered Commands
  API's own block-length field already tells the replay mechanism
  exactly how many bytes exist, so nothing scans for an end-marker here
  at all. Worth noting as a real hazard, not just "unnecessary": byte
  value `0x00` is itself a meaningful low VDU control code in this
  protocol, so if a null terminator were ever naively appended to
  *literal print text specifically*, it risks being misinterpreted as a
  control byte by the VDU parser rather than sitting inertly at the end
  - a concrete reason this case may need to stay terminator-free even
  once other cases get one, not just "doesn't need it, but harmless if
  present."
- **A future runtime-computed string (e.g. a `str(qux)` call site's
  buffer, Axis 8, or any future string variable)** is the case this
  question was actually raised about. **The open sub-question that will
  actually decide this axis:** whatever future mechanism Golem uses to
  "output this runtime-computed buffer's text" - does it reuse the same
  buffer-call/replay path literal `print` already uses (in which case
  it's *also* self-delimited by the buffer's own tracked length, and a
  terminator is redundant there too), or does it go through some
  different, still-undesigned routine that takes a bare pointer/address
  and scans for a terminator (mirroring the classic null-terminated
  calling convention the Author described from real assembly work) - in
  which case appending one at the point that buffer's contents are
  finalized becomes load-bearing, not optional? **This has not been
  decided because the output mechanism itself hasn't been designed yet**
  - nothing currently in `src/golemc.cpp` produces or consumes a
  runtime-computed string at all (`str()` remains unimplemented per Axis
  7/8's standing open questions).

**Recorded for whenever this comes back up, not acted on now:** once
Golem's `str()`/runtime-string-output mechanism is actually designed,
revisit this axis to decide, case by case, whether each string-producing
construct needs a compiler-appended terminator, and update this section
from "open" to "decided" at that point rather than leaving it stale.

## Axis 23: Reserved buffer-ID space for programmer-owned assets

**Status: leaning, further discussion needed as we get into it - not a
decision.** Raised directly: programmers will want buffer IDs of their
own to load assets (sprite frames, audio samples, image tiles) into,
indexed from a known base - e.g. `spriteBase + frame` to select
animation frame `frame` - a standard assembly trick, and one `golemc`
can support "for free" as ordinary integer arithmetic on a compile-time
constant, with no new type or runtime lookup required. Author's stated
lean is toward *explicit* programmer-declared reservations (a name and a
count, yielding a base ID) over having the compiler silently decide
asset placement, specifically so that base-plus-offset indexing is
possible at all - a compiler-hidden, non-contiguous, or
reshuffled-between-builds allocation would break it.

The wrinkle the Author also flagged: `golemc` already auto-allocates
buffer IDs for its own purposes today (`Compiler::nextBufferId_` in
`src/golemc.cpp`, a flat incrementing counter starting at 2, with buffer
ID 1 reserved as the `ACC` accumulator - used for every global, every
`For` loop's bound/body buffers, and any other future internal
temporary). Programmer-declared asset ranges and the compiler's own
internal counter must not be allowed to collide. Author's follow-up
clarification: this asset-reservation question is distinct from
ordinary variable storage (globals, arrays, locals) - those are, and
should stay, allocated by the compiler "behind the scenes" via the
existing internal counter, and are not something a programmer
reserves/indexes by raw buffer ID. Asset reservations are a separate,
programmer-facing concern layered alongside that existing mechanism, not
a replacement for it.

**Current leaning (flipped from an earlier draft of this axis, which had
proposed the opposite arrangement) - further discussion still needed:**
Author's preference is for the **programmer-reservable region to be the
low end of the ID space**, e.g. `0` through `4095` as an illustrative
starting point (not a final number), since low round numbers are the
more natural/expected range for a programmer to reach for and reason
about when hand-declaring asset ranges - with `golemc`'s own internal
counter (globals/locals/array storage/temporaries) occupying space
above that, out of the programmer's way.

A second, possibly complementary mechanism was also floated: rather than
(or in addition to) a fixed numeric split, let the programmer declare
their asset reservations up front (e.g. near the top of a source file),
and have the compiler read those declarations first and simply *work
around* them - i.e. `golemc`'s own internal allocator would skip any ID
already claimed by a declared reservation, wherever it happens to sit,
rather than relying purely on a hardcoded boundary. This is more
flexible (no arbitrary ceiling to pick or later regret) but adds
complexity (the compiler needs the full set of reservations up front,
before it can safely start handing out its own internal IDs) - which of
the two mechanisms (fixed low range as convention, or up-front
declare-and-work-around, or both together) is worth further discussion,
not decided here.

**A further "cheeky hack" idea, explicitly deferred, not decided, KISS
for now:** since arrays/variables already get a compiler-chosen buffer
behind the scenes, a natural extension would be letting a programmer
*pin* a specific array (or other variable) declaration to an explicit,
named buffer ID of their own choosing, instead of always accepting
whatever `golemc` picks - e.g. to deliberately overlay a variable on top
of (or alongside) a reserved asset range for some low-level trick. Noted
as a plausible future capability worth keeping in mind, but explicitly
**not** part of this axis's leaning for now - the rule for v1 is Keep It
Simple: ordinary variables stay fully compiler-managed with no manual
pinning, and this idea is parked for whenever asset reservations
themselves are actually implemented and better understood in practice.

**Left open, not decided:**
- Whether the low-range convention (e.g. `0`-`4095`), the up-front
  declare-and-work-around mechanism, or some combination of both is the
  actual mechanism `golemc` implements - needs more discussion once
  asset loading itself is closer to being designed.
- Where exactly any fixed boundary/ceiling would go, if one is used.
  Not something to guess precisely yet - it depends on how many buffers
  `golemc`'s own codegen tends to consume per program, which isn't
  knowable from a toy compiler with only `print`/`uint16`/`For`
  implemented. With 65536 IDs total (transmitted as a 16-bit value per
  `emitBufferHeader`/`emitLE16`) this can stay a generous, round,
  revisit-later number rather than something requiring precision now.
- The exact reservation-declaration syntax and keyword.
- Whether a reservation is untyped (programmer manages the raw bytes
  inside each reserved buffer entirely themselves, C-array-of-buffers
  style) or whether Golem ever grows an asset-aware declaration (e.g.
  distinguishing a "sprite frame range" from a raw reservation at the
  type level) - leaning toward untyped/raw for v1, consistent with this
  language's "thin structured-assembler abstraction" lean elsewhere in
  this document, but not decided.
- The "pin a variable/array to an explicit buffer ID" hack noted above -
  parked, not decided, not needed for v1.
- Should be a hard compile error (not yet implemented, since no
  reservation construct exists yet) if two reservations overlap, or if
  a reservation would collide with the compiler-internal region.

## Current lean (not a decision)

Procedural/imperative + Pascal-style static typing with range checks +
static/arena-only memory + thin structured-assembler abstraction for v1,
with event-handler declarations treated as a first-class top-level form
once the `event-resident` profile gets built. The typing axis is the one
place worth actively pushing back against "just do C-style" - the
debuggability argument is concrete and specific to this platform, not
generic language-design taste. The Author has indicated agreement with
this lean as of 2026-07-28, but it is not being treated as locked in.

## Standing note: upstream contribution is on the table

If Golem's design surfaces something genuinely valuable at the VDP/MOS
level - a new Buffered Commands API feature, a generalized version of the
`mos_vdpp_event_mux` pattern from `docs/devlog/from_codex.md`, a fix for
the MOS packet-discard bug noted there, or anything else - an upstream
pull request to the relevant project (agon-vdp, agon-mos) is a real,
intentional possibility, not just a hypothetical. Design work here
shouldn't assume everything must be worked around purely inside Golem;
"this would require a firmware change" is a legitimate proposal outcome,
not necessarily a dead end. Worth flagging candidate items here as they
come up, so they don't get lost.

**Candidates so far:** none formally proposed yet. (The MOS
oversized-payload discard bug documented in
[from_codex.md](../devlog/from_codex.md) §10.3 is arguably already a
bug-report-worthy finding, independent of any Golem-specific feature -
worth considering as a first candidate, separate from Golem's own design.)
