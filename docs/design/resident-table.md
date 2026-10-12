# Maintained resident table profile

GOLEM-017 implements one finite typed table-selection job, compiled on the PC
and invoked by assembly. The VDP checks the index before copying a whole record.
An invalid index preserves the destination, exits the job and allows normal VDP
processing to resume. Stock Pi/Oryx headless qualification passes; Oryx human
review passed; the agent retrieved and verified all saved results. Existing scalar packages and their output bytes are unchanged.

## Source and build inputs

```golem
Record Sample
    uint16 tag
    sint16 delta
EndRecord
Table Sample samples(0:3)
Sample selected = Sample(65535, -32768)
uint16 index = 0
Sub fetch
    selected = samples(index)
    print "selected\n"
EndSub
```

Declare one flat Record with 1–32 fields, each uint16 or sint16 with its natural
bounds. Fields pack in declaration order, two little-endian bytes each, with
no padding. The initialized destination has that exact record type; constructor
values must fit their field types. Signed fields support storage/copy only.

Declare one immutable Table with inclusive bounds 0:upper, containing 1–1,024
rows. Declare the destination and uint16 index, then one Sub with exactly one
whole-record selection. Optional literal print statements may precede or follow
it. Print accepts ASCII text, LF and CR. Newlines are whitespace; there are no
statement terminators. Keywords are case-insensitive; identifiers are sensitive.
Record, EndRecord and Table are now reserved words in all compiler modes.

From the repository root:

```bash
make -C src
build/golemc --resident --emit-asm \
  --table-data samples=docs/tasks/GOLEM-017/records.dat \
  docs/tasks/GOLEM-017/table.golem build/table
```

The build mapping supplies packed rows (1,-2), (2,3), (3,-4), (4,5), totaling 16
bytes. The filename belongs only to the PC build command. Relative paths resolve
against its current directory. Exactly one NAME=PATH mapping is supported; its
name must match the declared table and file size must exactly match the schema.
No Golem instruction loads a file. The compiler embeds these bytes in the
bootstrap; assembly decides how to load/send it. AGNB raw/table extensions are
not required or implemented here.

An optional final CLI argument selects the example uint16 input in update-call.bin;
otherwise the source initializer is used. Out-of-table uint16 values are accepted
as inputs so the resident guard can handle them. Values beyond uint16 are rejected.
As before, --reserve FIRST:LAST protects external inclusive ID ranges before
compiler allocation. IDs 0 and 65535 are unavailable.

## Runtime behavior

Each invocation resets runtime_error to 0. The original uint16 index is compared
against the inclusive upper bound using zero-extended bytes, avoiding the stock
VDP's 0xFFFF conditional-operand sentinel. A failure sets status 5
(table_index_out_of_bounds), preserves the complete selected record, and skips
later statements. Earlier effects remain. Correcting the index allows a later
invocation to succeed without reinstallation or any host acknowledgement.

Once admitted, the job adds the index to the table's base buffer ID, patches the
source ID in a separate copy-command buffer, then executes that buffer. Row IDs
are consecutive and allocated around reservations as one complete run. The
source ID is reset on every successful selection; carry lands in allocated
scratch storage. The executing job stream is never patched. Trusted immutable
rows require no repeated value checks, copy verification or rollback machinery.

For the four-row example, installation is 528 bytes across 19 owned buffers.
A new index plus invocation is 19 bytes (13 update + 6 call). Invocation with an
unchanged index is six bytes. All selection work and record copying occur on
the VDP. Result queries in the test fixture are instrumentation, excluded from
these application traffic figures.

## Assembly interface and ownership

Output uses the same five filenames as the scalar profile: bootstrap.bin,
invoke.bin, update-call.bin, cleanup.bin and interface.json. Optional --emit-asm
adds golem.inc. The manifest has schema 1 and profile resident-table, so consumers
must distinguish it from resident-scalar. It publishes input, record output,
record fields, table bounds/base/row size, entry, status and owned/reserved IDs.

The generated include supplies:

- EXP-001: call golem_fetch, plus golem_fetch_buffer and its six-byte call packet.
- EXP-002: golem_index and golem_selected _buffer/_offset/_bytes/_type constants;
  type tags are 1=uint16, 2=sint16, 3=float32, 4=packed record.
- EXP-003: golem_selected_tag and golem_selected_delta constants with the same
  destination buffer ID and offsets 0 and 2 respectively; their scalar type tags
  are 1 and 2. These describe VDP storage, not eZ80 RAM.
- EXP-004: golem_samples_base_buffer, _lower, _upper, _rows and _row_bytes.
- EXP-005: golem_runtime_error storage constants and labeled package
  bootstrap/update-call/cleanup bytes and lengths, plus owned count.

Assembly owns loading, installation, serialized updates/calls, pacing, result
handling and cleanup. The ADL=1 wrapper sends through MOS RST 18h and returns
without acknowledging VDP completion. It clobbers HL/BC; other effects follow
MOS. Include once where execution cannot fall through into wrapper/data bytes.
One package per include remains the current limit. Case-folded symbol collisions
and symbols longer than ez80asm's 64-character limit reject before publication.
Semantic and input-file errors preserve existing output files; I/O publication
itself is not atomic.

Cleanup clears only compiler-owned buffers, including table rows; reserved
external IDs are untouched. Calls are serialized and nonreentrant. The assembly
caller must respect the published ownership/layout and immutable table contract.

## Limits and evidence

This separate small profile does not require a dummy float/floor conversion.
It does not combine scalar arithmetic with table jobs, implement field arithmetic,
Bounds narrowing, nonzero/signed/multidimensional tables, general arrays, table
writes, nested records, multiple tables/jobs, general functions or AGNB extensions.
Unsupported constructs fail compilation. Generalizing those features remains
later work; the broader examples remain proposals.

[GOLEM-017](../tasks/GOLEM-017/README.md) records 65,872 independent-model jobs,
67 maintained rejection cases with ASan/UBSan, five actual assembler consumers,
and 873 stock-emulator checks across 98 jobs. Default/relocated, 258-row,
near-ID-limit and singleton packages pass. The maximum 1,024-row/32-field shape
is host-model-qualified; stock execution covers up to 258 rows/two fields.
Physical ESP32 hardware and other firmware versions remain unqualified.
