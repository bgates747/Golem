# Typed right shifts —2026-10-13

The released GOLEM-036 right-shift tranche implements uint32 logical and sint32
arithmetic `>>` with uint16 counts 0..31. Host/stock/actual-Xtensa qualification
passes; Oryx review is staged/stopped with human/publication approval pending.
Division and the shift plan/ownership correction were committed/pushed as df7a647
first. Rally owns game-specific replay/checks; Golem supplies qualified primitives.

1. Signed results use floor/sign extension, unsigned results zero fill. Invalid
   constant counts reject; dynamic counts fail with 32 before destination store.
   Precedence/grouping, left-to-right order, prior effects and failed-store
   preservation retain the shared source contract. Existing division artifacts
   remain byte-identical; SDK/runtime/production src are unchanged.
2. One bounded stock byte-table round runs at most 31 times; native uses unsigned
   shifts and explicit sign fill, avoiding implementation-dependent negative
   shifts and undefined shift-by-32. No source 64-bit types or new source operators
   beyond >> are added. Peak views3; ordinary frame wire 15 bytes.
3. Host 2,032 cases cover all counts/extrema/order/arrays and 20 supplied primitive
   vectors per layout, with sanitized C/metadata/parser and real assembler checks.
   Stock and both actual Xtensa engines/default-relocated pass. The 300-frame
   three-lane demonstration passes 382 observations/3766 bytes and56320 final
   pixels/MOS return. Profiles were qualified sequentially.
4. Rally reports its own foundation host integration passes 2,924 generated-C
   cases, actual frozen exports/metadata and malformed-adapter cases. Its package
   identity/boundary ledger were read/acknowledged; no independently repeated
   peer qualification or game replay is claimed here.

[Task plan](../tasks/GOLEM-036.md), [source/resources/evidence](../tasks/GOLEM-036/right-shift/README.md).
TODO.md owns overall status. Wider-helper needs and D2 implementation remain
separately scoped; new emulator work remains uncommitted for Author review.

GOLEM-036 right-shift human review subsequently passed: the Author reported 'looks good'; retrieved results match all 382 observations / 3,766 bytes and staged runtime/fixture identities. See docs/tasks/GOLEM-036/right-shift/evidence/human-review.json. The Author explicitly approved commit/push after an empty mailbox check. Rally retains ownership of game-specific replay and validation.
