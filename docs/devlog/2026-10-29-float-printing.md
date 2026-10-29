# Resident float32 printing

Float results can now be printed entirely by stock resident buffered commands,
using the same explicit conversion syntax as integers. The desktop compiler and
native Agon compiler both implement it. Numerical qualification passes; the
fourteen-row Oryx display is prepared for the Author's visual/keyboard review.

## Publication and scope

Before implementation, the Author requested publishing pending work in logical
groups. Agon-vdplet stock-only audio baseline1da8572 and official-Fab comparison
2541f10 are pushed; Golem task plan46ade43 is pushed. Fab listening was clean in
all cases, whereas stock Flexe distorted in all cases; the Author also heard
faster note succession in Fab. Exact inputs and source pins were retained.
Flexe received the durable comparison and verified CLI chat-wakeup instructions;
its audio/timing investigation remains separate from this language work.

GOLEM-054 adds `print str(float32-expression)`, nine significant decimal digits
with nearest/ties-even rounding, trimmed fractional zeros and general notation.
Decimal exponents -4 through8 use ordinary notation; others use signed-exponent
scientific notation. Zero is canonical. Integer output is unchanged. Fixed-point
types/formatting and precision options remain follow-up; no concatenation,
general strings, native text backend, firmware or SDK change was introduced.

## Implementation and qualification

The emitter converts the binary32 significand/exponent into exact decimal digits
using bounded integer/table operations, then rounds once. It reuses existing
integer conversion machinery and stock buffer commands rather than approximate
float scaling. A113-byte coefficient buffer bounds every normal binary32 value;
each print site consumes and clears16 text bytes. Loops tail-jump with finite
counters. Shared state and resident call/cleanup ownership remain unchanged.

The independent exact-rational oracle covers1,934 values and every normal binary
exponent. Default/relocated emitted execution passes3,958 calls and197,817,640
decoded commands, including faults, status recovery, source preservation and
cleared temporaries. Maintained checks and ASan/UBSan pass. LeakSanitizer itself
cannot run under the tool execution tracer and is explicitly disabled; no leak
check is claimed. Legacy executable artifacts remain identical. One historical
schema-only metadata difference was already present in the parent revision and
is recorded without rewriting old evidence.

Actual Pi/Oryx Fab/MOS compiles the one-input formatter on the eZ80 and matches
all11 package files to desktop output. Each checks72 full-screen text pairs,
22,118,400 pixels and576 state/error bytes, followed by MOS keyboard continuation.
Relocated stock execution also passes. The human fixture prints only Golem's
numbers; C supplies inputs, calls the entry and inserts row breaks. Automated
reference drawings are kept separate.

## Distribution and limits

The native worker is319,489 bytes. A first example with two float globals,
arithmetic and two float print sites exhausted compilation RAM while building
stock wrappers and returned an ordinary MOS error. That attempt is preserved.
The bundled one-input example compiles on Agon, matches desktop output and runs
through unchanged `golemrun`, printing `reading=1.5`. The how-to describes that
helper-heavy programs can hit native compiler RAM limits even with short source;
larger programs can use the desktop compiler. No allocator redesign was added.

The refreshed ZIP contains current source, worker, example, how-to and language
reference; every archive checksum verifies. A minimal float-print installation
uses17,571 buffered bytes plus130 common bytes; private code/data payload totals
14,666 bytes, excluding firmware allocation overhead. These are storage costs,
not a performance claim. Future calls retain the existing resident entry path.

[GOLEM-054 results](../tasks/GOLEM-054/README.md) index the evidence and the
stopped Oryx review. No GUI was launched or hardware flashed. New implementation
is uncommitted pending human review and explicit publication approval.

The Author separately approved publishing both coordination reports to
agon-dev-env after automatic review requested that specific destination approval.
Only those two messages were committed/pushed as04772a8; other agents' work was
not staged. The new Golem implementation remains uncommitted for human review.


GOLEM-054 human review passed on2026-10-29: the Author's screenshot shows all
fourteen expected rows, PASS and successful MOS dir. Fresh agent retrieval
verifies112 saved bytes, runtime pins and stopped Oryx profile. Evidence is
docs/tasks/GOLEM-054/human-evidence.json. Explicit publication approval remains
outstanding; implementation is still uncommitted.


GOLEM-054 publication approved on2026-10-29: the Author explicitly requests
commit and push after review. This supersedes its earlier publication hold.
The feature is accepted/complete and removed from TODO.md. Future native
compiler RAM work and precision options require their own next tranche.
