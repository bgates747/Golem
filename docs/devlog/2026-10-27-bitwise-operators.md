# Integer bitwise operators and review

GOLEM-053 implements stock resident &, | and unary ~ beside existing ^ for all
four supported integer types. The language reference and on-Agon distribution
are updated. The Author reports PASS and supplies a screenshot showing correct
values and successful MOS dir. All 4,968 fresh saved bytes match independently.

Maintained contracts/sanitizers, generated C and both Pi stock Fab and Oryx stock
ESP32 Flexe checks pass; detailed evidence is under docs/tasks/GOLEM-053/.
Human review is accepted. The Author explicitly approved commit/push of all
pending Golem work in logical groups on 2026-10-27. GOLEM-053 is complete
and removed from the authoritative TODO.
Numeric printing remains queued as GOLEM-052; concatenation is still discussion.
