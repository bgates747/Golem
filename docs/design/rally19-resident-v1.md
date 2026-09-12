# R19-05 resident data, admission and lifecycle

This implements the resident portion of `rally19-hosted-v1.md`. The authoritative
execution register is `../AgonArcade-rally19/TODO.md`; this is a specification,
not another checklist. Hosted compilation remains part of the existing C++
compiler and ordinary language behavior remains unchanged.

## Imports and indexed records

```text
Asset palette(4100,"palette.dat",256);
Table coefficients(4000,228,90,"oval-coefficients.dat");
LoadIndexed(selectedRecord,coefficients,index,lookupOK);
```

Asset/Table paths are quoted, source-relative and cannot contain parent traversal
or be absolute. The compiler checks exact file size before reading. Assets are
immutable and never callable. Table record IDs are consecutive and must fit
4000..4999 without colliding with declarations in either source order. A record
width is 1..65535 bytes. Imported bytes are data, not Golem source or commands.
Maps identify the path, size, count and FNV-1a-64 fingerprint; qualification drivers
record SHA256 as the stronger source/artifact identity.

Each destination record field is checked against every imported record. Nonfinite
floats or out-of-range values reject compilation. Runtime lookup requires u16
index storage separate from the destination. The four-argument form clears a
0..1 u16 status before testing `index < count`; invalid indices retain the old
destination. Index 65535 is rejected, including stock firmware's sentinel behavior.
The three-argument form requires a declared index range wholly within the table;
it does not make an unvalidated host field safe by declaration alone.

Admitted lookup adds the index to a three-byte copy of the first record ID,
patches the source-ID operand of a **different** private copy-instruction buffer,
then calls that instruction. It neither mutates a currently executing program nor
calculates the selected record on eZ80. The map identifies the guard and the exact
source-ID relocation. The native fixture crosses 4095 to 4096 and checks invalid
indices 4, 65534 and 65535 against a four-record table.

## Full-width input validation and typed copy

```text
Set(valid,1);
RequireRange(position,valid);              // inclusive declared bounds
RequireRange(position,0,575999,valid);     // explicit inclusive bounds
RequireEqual(sequence,expected,valid);    // identical fixed storage type
CallIf(commit,valid,eq,one);
CopyRecord(active,staging);               // inside admitted commit only
```

The `Require` intrinsics are **sticky rejection**: failure sets status to zero;
success leaves its prior value unchanged. Status must be a writable 0..1 u16
field in storage separate from checked operands. They do not update a field's
static type or turn arbitrary user programs into a verified protocol. Application
entrypoints must check every external input before arithmetic/CopyRecord and only
call their render program after all guards. Golem is not a security boundary
against another program calling raw VDP buffer IDs directly.

`RequireRange` supports integer/fixed fields. Fixed bounds are converted to raw
inclusive integer bounds by ceil(low*256)/floor(high*256); an empty representable
range fails compilation. Float admission is intentionally unsupported because
this wire ABI has no float fields. Signed raw values are ordered by flipping the
sign bit in private scratch. Each comparison byte is zero-extended to a fetched
u16 word, avoiding the stock 0xFFFF missing-value sentinel. A finite lexicographic
chain tests each bound. Valid negative -1 and positions whose low word is 0xFFFF
therefore work. `RequireEqual` similarly compares zero-extended bytes and requires
matching fixed types. Neither path performs the checked computation on the host.

`CopyRecord` requires distinct same-size buffers whose declared fields cover all
bytes exactly once, with matching offsets/types and source ranges contained in
destination ranges. Gaps, overlapping aliases, incomplete layouts and type/range
mismatches fail compilation. It lowers to one stock checked-span copy. Its input
range precondition remains the admitting application's responsibility.

Rally's `admission.golem` checks all 80 bytes: version, length, loaded track,
expected/duplicate sequence, seal, progress, signed steering/lateral/lanes,
reserved zeros and HUD fields. Oval's seven positions receive the tighter oval
bounds. Track cannot change through a payload; reload selects the loaded track.
The admitted copy invokes a finite numerical proof, then advances accepted count
and modulo-65535 sequence. The seal is cleared after every submit, accepted or
rejected, so a subsequent short prefix cannot reuse the previous seal.
The proof currently doubles admitted position; it is not the full scene renderer.

The host still sends one ordered 97-byte update/call packet. This is not a new
transactional UART parser. Interrupted writes must not be followed by a trigger.
Native diagnostic tests cut headers, payloads and triggers, leaving 500 ms idle
for payload cuts (2000 ms for header/trigger cuts), then prove stock parser
recovery before continuing. This timeout is deliberately
absent from ordinary frames. GP readback proves parser/buffer progress, not raster
completion or a custom callback.

## Resources, cleanup and diagnostics

The compiler enforces aggregate 512 KiB imports, 128 KiB resident programs,
64 KiB scratch/state/matrices and 1024 IDs; resident plus bootstrap payload must
fit 1 MiB. Executable private templates count as programs. Maps separate these
categories, list every owned ID, and report bootstrap/cleanup sizes. These are
payload bounds, not allocator overhead or hardware RAM guarantees. Syntax nesting
is limited to 32 before recursive parsing; hosted call depth remains capped at 16.

`output.clear` contains direct-stream stock clear commands for every owned ID,
including each table record and private temporary. It excludes bootstrap buffer 0
and external artwork/other applications. Never execute this cleanup from a buffer
that it clears. The loader clears buffer 0 separately. The large native fixture
streams a 66,288-byte bootstrap into multiple blocks in buffer 0, proving that
aggregate bootstrap size may exceed a single 16-bit write-block length. The test
loader caps that aggregate at 1 MiB and invokes it only after a successful read.

The diagnostic reader copies a byte into a GP token, then reads two tagged
nibbles. Neither possible tagged reply equals the 0xFF wait sentinel, allowing
all 256 values without expected-answer-dependent waits. Missing-source copies
leave a 0x55 template tag; the lifecycle fixture checks every tested owned first
byte differs before using that tag to verify cleanup. This is not a universal
buffer-existence API.

Rally's Linux-only `memory_probe_linux.cpp` interposes stock heap_caps_malloc and
actual libc frees, including delete[] frees. It samples a locked tracking map
at explicit guest idle phases and injects Escape through SDL after the finite
jobs. The original VDP module is unchanged. This records tracked allocation
counts/bytes, not all host allocations, fragmentation or physical ESP32 memory.
It must not be used for timing. In the large fixture nine 4096-call batches retain
239 allocations / 222600 bytes both before and after each batch. The three cleared
cycles each retain 32 allocations / 83832 bytes; compared with initial 31 / 83600,
232 bytes of warmed container capacity persist but do not grow across reloads.
Peak tracked bytes are 304498, including mode changes and the retained bootstrap.
No full-game, ten-minute, cache/bitmap or hardware stability claim follows from
this small resource probe; those remain later contract stages.

## Reproduction

```sh
# Golem worktree
make -C src
.venv/bin/python tests/run_hosted.py
.venv/bin/python examples/native_lookup/make_assets.py

# Rally execution worktree
cd /home/smith/Agon/mystuff/AgonArcade-rally19
.venv/bin/python docs/tasks/RALLY-19/probe_lookup.py UNIQUE --memory
.venv/bin/python docs/tasks/RALLY-19/probe_admission.py UNIQUE_OVAL
.venv/bin/python docs/tasks/RALLY-19/probe_admission.py UNIQUE_FUJI --track fuji
```

Requires agondev, both local Python environments, stock official Fab 1.2.4 /
MOS 3.0.2 and canonical profile generation. Memory diagnostics additionally need
Linux glibc, g++, SDL3 headers and libdl. All runs are headless, use the generated
profile wrapper and normal eZ80/UART limits, and leave original worktrees and
upstream source alone. Raw native reports and failed attempts are retained in
Rally's RALLY-19 evidence directories.
