# GOLEM-035 — D1a bounded typed arrays and retained tables

Add the first reusable game-storage primitive: bounded one-dimensional typed
arrays/SoA on both backends, with immutable retained tables and mutable opponent
state. This follows the [accepted contract](GOLEM-033/accepted-contract.md),
requires GOLEM-034's delivery/interface decisions, and awaits Author release.

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

- SYN-001 [ ] Freeze index types, inclusive bounds, element initialization, immutable/mutable declarations and checked read/write syntax.
- LAY-001 [ ] Define generated array/table descriptors and external installation/lifetime invariants with Rally fixtures.
- EMIT-001 [ ] Implement typed indexed reads/writes in the common frontend and both emitters without a native-only dialect.
- ERR-001 [ ] Report index status 5; preserve failed read destination/write element and adjacent data, retaining earlier effects.
- CHK-001 [ ] Verify first/last/out-of-range indices, nonzero bounds, read-only writes, arithmetic failures, persistence, relocated IDs and cleanup.
- CHK-002 [ ] Qualify six-opponent SoA and 257-entry sine cases against independent Rally data on stock and actual Xtensa engines.
- VIS-001 [ ] Stage a source-labelled Oryx review demonstrating indexed movement/data access, stopped.
- REV-001 [ ] Obtain human acceptance and explicit commit/push approval.

## Acceptance and extension boundary

Identical source/data layouts produce the same results/errors and persistent
state on supported backends. Borrowed assets survive cleanup. GOLEM-036 later
adds wider element support and requires extending these storage cases; this
initial tranche must not silently narrow Rally's world coordinates into 16 bits.
All multi-file task artifacts remain in GOLEM-035/ until agreed promotion.
