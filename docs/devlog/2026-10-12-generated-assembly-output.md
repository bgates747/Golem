# GOLEM-016 — Generated assembly output and named calls

GOLEM-016 implements optional --resident --emit-asm output. The generated include
exports a named eZ80 wrapper for the single resident job, storage constants and
labeled command payloads. Ten actual assembled consumers preserve all five
existing binary/manifest artifacts against e52a427. Maintained checks and
sanitizers pass. The tranche awaits Author review; no emulator was launched.

The Author accepted named assembly wrappers and authorized proceeding to the
recommended next task, accepting GOLEM-010's review direction. GOLEM-010 was
removed from the authoritative unfinished register; its findings/examples remain
under their silos. GOLEM-016 was promoted before implementation began.

A source Sub tick becomes call golem_tick. Its generated wrapper loads HL/BC,
transmits the six-byte entry packet with rst.lil $18 and returns. This is a real
eZ80 wrapper around VDP execution, not native compilation of the Golem body or a
completion acknowledgement. Assembly owns installation, updates, pacing, result
handling and cleanup. The named-call MOS consumer demonstrates installation,
named invocation and final cleanup without handwritten buffer IDs.

The emitter shares the compiled Result/layout with interface metadata. The flag
adds golem.inc without modifying existing files. Assembly symbol collisions under
case folding and names exceeding ez80asm's 64-character limit fail semantic
publication before output writes. The restrictions apply only to assembly output.
Current output covers the existing scalar profile, not future Table/Array rows.

Qualification compared pure/count floor, checked subtraction, multiplication and
divmod in default and relocated layouts. Actual assembler output matches all
payloads, location/type constants, owned counts and wrapper opcodes. A separate
named-call application also assembles. Rejected publication preserves existing
files. Warnings-as-errors and ASan/UBSan pass with 46 rejection cases. The final
runner took 27.369 seconds on a local monotonic clock; this includes rebuild,
baseline comparison, assembly and sanitizers, not runtime performance.

Only host compilation/assembly was performed. VDP command bytes are unchanged,
so earlier stock qualification remains relevant to those bytes; the new eZ80
wrapper was assembled/inspected and has not been newly executed in the emulator.
No new Oryx fixture was deployed and no upstream reference checkout changed.

See [usage](../tasks/GOLEM-016/README.md),
[evidence](../tasks/GOLEM-016/host-checks.json) and
[consumer](../tasks/GOLEM-016/consumer.asm). The next recommended implementation
is a small typed table entry with no mandatory float-floor job; it has not begun.
