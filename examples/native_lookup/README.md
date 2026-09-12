# Resident lookup/lifecycle probe (R19-05)

This example uses the existing C++ Golem compiler's hosted mode. The editable
`make_assets.py` produces four signed record pairs and an all-byte fixture.
`kernel.golem` imports them; each finite fetch selects an eight-byte resident
record on VDP and multiplies its two fields using stock matrix arithmetic.
The eZ80 loader supplies only the index and diagnostic transport.

From this repository:

```sh
.venv/bin/python examples/native_lookup/make_assets.py
make -C src
build/golemc --hosted examples/native_lookup/kernel.golem build/lookup.bin
.venv/bin/python tests/run_hosted.py
```

The Rally execution worktree owns native test orchestration/evidence:

```sh
cd /home/smith/Agon/mystuff/AgonArcade-rally19
.venv/bin/python docs/tasks/RALLY-19/probe_lookup.py UNIQUE_RUN_NAME
```

Requires the canonical stock Fab 1.2.4/MOS 3.0.2 runtime, agondev on PATH and
project Python environments. Every run uses a fresh build/profile, normal CPU,
dummy SDL video/audio and the generated profile-local wrapper. No GUI or beep.

`Table name(firstId,recordBytes,count,"relative/path");` imports exact-size
records into consecutive reserved 4000..4999 IDs. `Asset name(id,"path",size);`
is immutable data. Paths are source-relative, without absolute/parent traversal.
The map records FNV-1a fingerprints; the test driver records SHA256 identities.

`LoadIndexed(destination,table,index[,status]);` validates every imported record
against the destination's fields. The runtime guard admits indices below count,
patches a separate copy instruction's source ID, then calls it. Invalid indices
leave the destination unchanged and clear the optional 0..1 u16 status. A source
index with a wider declared domain requires status; gate subsequent computation
on success. Index 65535 is rejected despite the stock sentinel collision.

The compiler emits `output.clear`: direct-stream stock clear commands for every
owned ID, including table records and private temporaries, excluding bootstrap
buffer 0 and external resources. Never execute cleanup from a buffer it clears.
The loader clears bootstrap 0 separately. Cleanup is explicit, not per frame.

The passing initial probe selects 72 records across nine bootstrap reloads/mode
resets, checks retained output for invalid indices, reads all 256 possible byte
values nine times and clears owned resources three times while preserving an
external canary. Diagnostic byte readback uses two tagged nibbles, neither of
which can equal its 0xFF wait sentinel. A missing-buffer copy leaves a known
0x55 tag; all tested owned first bytes differ from that tag before cleanup.

These are parser/buffer observations, not rendering-completion evidence. ID and
payload ledgers alone do not establish allocator/metadata balance. The expanded
probe below supplies live allocation observations; the full 80-byte admission
probe is documented in `examples/native_admission/README.md`. Full-game stability
remains later work in the shared authoritative Rally root TODO.md.

The expanded R19-05 fixture adds a 65535-byte asset and a multiblock bootstrap
larger than 64 KiB. It samples asset offsets 0/255/256/32767/32768/65534, and table
IDs 4094..4097 exercise low-byte address carry. `probe_lookup.py UNIQUE --memory`
adds nine 4096-call batches, externally sampled live allocations, and a headless
Escape injection proving return to outer keyboard processing. See
`docs/design/rally19-resident-v1.md` for measured limits and diagnostic scope.
