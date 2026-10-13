# GOLEM-035 — D1a bounded typed arrays and retained tables

Add the first reusable game-storage primitive: bounded one-dimensional typed
arrays/SoA on both backends, with immutable retained tables and mutable opponent
state. This follows the [accepted contract](GOLEM-033/accepted-contract.md),
requires accepted GOLEM-034. The Author released the next storage tranche after
D0 commit/push on 2026-10-13; human review remains a separate publication gate.

## Scope

Use the agreed Array sint16 lateral(0:5) direction and lateral(index) access,
with inclusive explicit bounds. Start with uint16/sint16 elements and define
explicit initialization/build-supplied data before samples claim executable
syntax. Full Records, heterogeneous tables, dynamic allocation, multidimensional
arrays and general pointers are not prerequisites. Six opponent slots, six route
records represented as SoA, and 257 sine entries are required Rally fixtures;
these counts are not language-wide capacity ceilings.

Assembly/build owns retained data installation, IDs and lifetime. Compiler
manifests describe exact element widths, bounds, lengths, offsets, access rights
and external ownership. Stock code may rely on truthful, established installation
invariants; it must not pretend it can query arbitrary actual buffer lengths.
Native acquisition/shape checks and borrowed buffer cleanup require explicit
matching contracts. No data filenames or MOS file-loading logic enters Golem.

- SYN-001 [x] Freeze index types, inclusive bounds, element initialization, immutable/mutable declarations and checked read/write syntax.
- LAY-001 [x] Define generated array/table descriptors and external installation/lifetime invariants with Rally fixtures.
- EMIT-001 [x] Implement typed indexed reads/writes in the common frontend and both emitters without a native-only dialect.
- ERR-001 [x] Report index status 5; preserve failed read destination/write element and adjacent data, retaining earlier effects.
- CHK-001 [x] Verify first/last/out-of-range indices, nonzero bounds, read-only writes, arithmetic failures, persistence, relocated IDs and cleanup.
- CHK-002 [x] Qualify six-opponent SoA and 257-entry sine cases against independent Rally data on stock and actual Xtensa engines.
- VIS-001 [x] Stage a source-labelled Oryx review demonstrating indexed movement/data access, stopped.
- REV-001 [x] Obtain human acceptance and explicit commit/push approval.

## Acceptance and extension boundary

Identical source/data layouts produce the same results/errors and persistent
state on supported backends. Borrowed assets survive cleanup. GOLEM-036 later
adds wider element support and requires extending these storage cases; this
initial tranche must not silently narrow Rally's world coordinates into 16 bits.
All multi-file task artifacts remain in GOLEM-035/ until agreed promotion.

## First implementation tranche

One-dimensional uint16/sint16 arrays live in separate generated, application-owned
retained buffers. The root-v1 header remains unchanged; optional generated array
descriptors extend the interface manifest. Assembly installs their exact initial
bytes and keeps them alive across calls/backend changes and private cleanup.
Native preflight checks physical shape; stock uses the installed invariant.

`Array sint16 lateral(0:5) = Fill(0)` supplies explicit uniform initialization;
`= Values(...)` supplies exactly one typed literal per element. A declaration
without an initializer requires build-time `--array-data NAME=PATH`, an exact
packed little-endian image; source contains no filenames. `ReadOnly Array` rejects
source writes. uint16/sint16 index expressions use their numeric value, including
negative declared bounds; no implicit type conversion or reverse indexing.
Bounds are inclusive, consecutive and ascending, within -32768..65535, with
at most 32767 two-byte elements in each initial single-block array.

For a store, evaluate/check the index first, then the RHS, then store once.
Checked expressions keep their arithmetic status; a validly evaluated but
out-of-range index reports language status 5. Invalid literal indices reject at
compile time. A failed load preserves its scalar destination; a failed store
preserves the element/neighbours and earlier effects. Paired divmod outputs and
For counters remain scalars. Float/wide/multidimensional arrays are later work.

Rally accepted the concrete storage profile without mismatch in message
2026-10-13_17-26-57_rally_rally25-d1a-profile-accepted-thirteen.
[Implementation/source/qualification report](GOLEM-035/README.md) and
[immutable delivery](GOLEM-035/delivery.json) record the completed first tranche.
Human review and publication approval remain pending.

GOLEM-035 human review subsequently passed: the Author reported the tests look
good. Retrieved results match all 1,302 observations/12,912 bytes; staged runtime
and fixture identities pass. See docs/tasks/GOLEM-035/evidence/human-review.json.
Explicit commit/push approval remains pending.

The Author explicitly approved committing/pushing all pending work and
proceeding to the next tranche on 2026-10-13. GOLEM-035 is accepted.
