# First pixel implementation review

GOLEM-011 / REV-001 is complete. The recommended next tranche implements
draw_point, draw_line and fill_quad as a standalone resident geometry profile,
then stages a visible Oryx demonstration. Packed typed inputs and generated
assembly calls preserve the intended application boundary and resident wire
savings. The Author authorized this review; implementation scope awaits review.
No further design tranche is recommended between acceptance and implementation.

The task-silo implementation-review.md specifies initialized byte/signed-word
globals, matching literal operands, one finite Sub, graphics preconditions,
perimeter-to-triangle quad ordering and existing package/assembly exports.
Reuse current compiler infrastructure and Rally's mechanisms; importing Rally's
whole hosted frontend would broaden this first pixel tranche unnecessarily.
Bitmaps/transforms and viewport/scroll operations remain later slices.

Inspected current CLI profile dispatch, byte-emission and assembly output, the
maintained build/check targets, and the accepted GOLEM-017 qualification workflow.
The scope requires an unambiguous drawing profile, a new distinct uint8 export
tag preserving existing tags, and unchanged scalar/table package contracts.
Qualification covers real assembler consumption, stock drawing comparisons,
repeated/partial input updates, ownership and a staged Oryx human keyboard review.

The Author will launch the staged emulator, follow its onscreen dir instruction,
close it and report the result. The agent retrieves observations; no second
terminal is needed. Human validation and explicit commit approval still precede
committing emulator changes. This documentation-only review changed no compiler,
emulator fixture or source examples and required no test reruns. REV-001 is
checked complete; GOLEM-011 remains in TODO pending final review acceptance.
