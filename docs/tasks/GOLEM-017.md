# GOLEM-017 — Typed table selection from assembly

Implement a finite resident job that selects one typed record from a table,
without requiring the scalar profile's unrelated float/floor conversion.
Assembly owns installation, input updates, invocation and cleanup. The Author
accepted GOLEM-016's working assembly interface and authorized this next tranche.

- SYN-001 [x] Parse one flat Record of uint16/sint16 fields, one immutable
  zero-based Table, an initialized record destination and uint16 index, and
  one Sub containing whole-record selection and optional literal print markers.
- DAT-001 [x] Resolve a build-time --table-data NAME=PATH mapping, validate
  exact packed size, and allocate consecutive owned row buffers around reserves.
- VDP-001 [x] Check the original index before address calculation/copy. Invalid
  access preserves the destination, skips subsequent statements and exits with
  status 5. Reset status per invocation; do not verify copied bytes at runtime.
- ASM-001 [x] Export the named entry wrapper, index, record/field offsets,
  table layout, status and package payloads through --emit-asm and interface.json.
- TST-001 [x] Check rejection/publication behavior and allocator exhaustion;
  preserve prior scalar artifacts; qualify real generated commands and the
  assembly wrapper using stock VDP/MOS, including relocation and recovery.
- REV-001 [x] Stage a dedicated Oryx human review, document evidence and limits,
  and stop. Leave affected changes uncommitted pending human validation and
  explicit commit approval, as required by the canonical AGENTS.md.

Keep fixtures and reports in GOLEM-017/. Initial limits: one table, 1–1,024
rows, 1–32 two-byte fields; no field arithmetic, general arrays, nested records,
table mutation, source filenames, new AGNB format, or general functions.

Implemented and automatically qualified; [review instructions/evidence](GOLEM-017/README.md).
REV-001 covers staging and stopping. Oryx human review subsequently passed; the
agent verified all saved results. The Author explicitly approved commit and push;
GOLEM-017 is accepted and removed from TODO.md.
