# Paired wide division — 2026-10-13

GOLEM-036's third tranche adds shared-source paired uint32/sint32 quotient and
remainder. Host, stock and actual Xtensa/default-relocated qualification pass;
Oryx visual review is staged/stopped with human/publication approval pending.
Accepted multiplication was committed/pushed as 1ddfd64 after the initial mailbox
check found no response. Earlier sources/packages remain intact.

1. Signed quotient truncates toward zero; remainder follows numerator sign or
   zero. Status 4 means zero divisor, 6 minimum-signed/-1 overflow. Both result
   Bounds pass before either store; status 32 preserves both outputs. Earlier
   effects and input/output aliases retain the agreed finite execution contract.
2. A bounded shared 32-round stock kernel computes magnitudes and restores signs;
   guarded native 32-bit C avoids undefined division. Host 2,528 cases include all
   100 Rally division and 600 supplied product-to-delta comparisons per layout.
   These are partial numeric checks, not full game/route/contact replay.
3. Previous multiplication byte parity, sanitizer metadata admission, actual
   assembler, SDK limits and maintained compiler checks pass. One parallel JIT
   profile made no final observation file; logs/disposition are preserved. Fresh
   serialized JIT/interpreter pass; cause is unestablished, no runtime fix claimed.
4. Both engines' 300-frame review passes 488 observations/4826 bytes, 56,320 final
   pixels and MOS return. Golem owns quadratic x/64 and signed y/8 with fractions;
   assembly draws/paces VSYNC. Ordinary frame wire remains 15 bytes. No flashing,
   capacity change, helper composition or scheduler implementation is included.

[Task plan](../tasks/GOLEM-036.md), [source/resources/evidence](../tasks/GOLEM-036/division/README.md).
TODO.md owns overall status. Wide shifts/exact helpers/full Rally replay remain
open; new emulator-related work stays uncommitted for Author review.

GOLEM-036 division human review subsequently passed: the Author reported 'looks good.' Retrieved results match all 488 observations / 4,826 bytes and staged runtime/fixture identities. See docs/tasks/GOLEM-036/division/evidence/human-review.json. WID-002 is accepted; the Author explicitly approved commit/push. Wide shifts/exact helpers and full Rally replay remain open.
