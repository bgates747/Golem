# Retained arrays qualified for the Rally storage milestone

GOLEM-035 implements the released first array tranche on both task-local backends.
Stock and actual Xtensa checks pass, including the six route records, 257 sine values and
six-slot motion fixture. Immutable packages/source are prepared for Rally; Oryx
review is staged/stopped. Human acceptance and explicit commit/push approval remain
pending, so GOLEM-035 stays in TODO. No physical target was flashed.

The Author explicitly approved committing/pushing all D0/planning dirt before
this work. D0 is published through 1a1f303 (main 5c016a9); a follow-up corrected the
broad binary-ignore rule so every immutable bundle artifact is tracked. Rally
independently verified its complete 76-file delivery, manifests, six compiler
exports and adapter mappings. The published D0 bundle remains unchanged.

GOLEM-035 adds typed uint16/sint16 indexed retained buffers, explicit Fill/Values
or exact build-supplied initialization, ReadOnly source permissions, and inclusive
numeric bounds. Store index evaluation/admission precedes RHS evaluation and a
single write. Logical index failure reports 5, preserving the failed destination/
element and earlier effects; scalar arithmetic/conversion rules remain intact.
The shared root-v1 header is unchanged. Native validates actual array shapes and
holds at most argument+root+one-array leases; stock depends on exact installed
storage/lifetime. Array contents stay outside the native module. Both private
cleanups/unload retain application root/arrays. No whole-array admission scan or
runtime readback/copy verification was introduced.

1. Host ASan/UBSan/model/generated-C checks pass 2,838 default/relocated array cases,
   866 Rally/six-slot cases, eight Bounds/conversion cases and 23 grammar/build
   rejections. Scalar executable/layout output matches D0. Source-only arrays,
   signed bounds and actual assembler exports pass; maintained checks pass.
2. Stock access passes 1,090 observations/2,180 bytes. Stock Rally qualification
   passes 1,053 observations/2,106 bytes, all 257 sine lookups, six route records
   and 60 six-slot movement steps. Exact source values come from the verified
   pinned Rally handoff, not regenerated trigonometry or narrowed game units.
3. Actual Xtensa JIT/interpreter default and relocated packages each pass 2,046
   observations/20,460 bytes, including missing/short/long native array rejection
   and persistent state/private cleanup. Both engines' 600-frame visual passes 1,302
   observations/12,912 bytes, 56,320 playfield pixels and actual MOS return.
   Oryx JIT 36.159s / interpreter 62.867s are qualification durations, not calibrated
   hardware frame timing. Human review remains pending.
4. Three independent rebuilds preserve 17/17/24 executable/storage artifacts.
   The frozen exported C++ source closure independently rebuilds 14 matching
   artifacts without working-task dependencies. Metadata was augmented with
   explicit index/zero-divisor codes and source-data hashes; qualified executable
   and source-program identities remain unchanged by those receipt additions.
5. Animation native text 6,992/data 268/package 8,264 bytes and module static frames 160
   remain inside existing admission limits. Arrays 634/root 142 bytes are retained.
   A frame sends calculate 6 + resident draw 6 + swap 3 = 15 wire bytes, without
   per-marker position updates. Caption/installation traffic is separate.

Rally accepted the storage contract without mismatch in message
2026-10-13_17-26-57_rally_rally25-d1a-profile-accepted-thirteen. Its accepted
signed-division overflow mapping 6 is recorded in GOLEM-036 (not implemented here).
Renderer ABI proposals and the future early-argument-release/completion-storage
review are recorded in GOLEM-037. D1b/D2/firmware implementation remain separate
release gates. The new renderer proposal does not expand this tranche.

Source, complete instructions and immutable package pointer:
docs/tasks/GOLEM-035/README.md and delivery.json. The Author launches its separate
Oryx wrapper, watches all six markers, follows the onscreen dir instruction,
closes and reports; the agent retrieves and verifies saved results.

Immutable D1a candidate 2156756263f8baffa52e3c14a57e7838d90336bc9ee3d5bbde78566ed722ff8f
was sent to Rally in message
2026-10-13_18-00-02_golem_rally25-d1a-immutable-delivery-seven.
The exported source rebuild matches 14 compiler artifacts (export-checks.json).
Concrete renderer v0.2 was received and retained for D2 review; a separate
application-owned completion buffer is recommended to preserve root-v1 semantics.

GOLEM-035 human review subsequently passed: the Author reported the tests look
good. Retrieved results match all 1,302 observations/12,912 bytes; staged runtime
and fixture identities pass. See docs/tasks/GOLEM-035/evidence/human-review.json.
Explicit commit/push approval remains pending.

The Author subsequently explicitly authorized committing/pushing all pending
work and proceeding. GOLEM-035 is accepted and removed from TODO; the next
released tranche is the GOLEM-036 wide integer foundation.
