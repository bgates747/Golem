# GOLEM-031 — Portable signed 16-bit integers

Add explicitly initialized sint16 globals with inclusive Bounds, checked + / - /
* arithmetic, same-type comparisons and explicit sint16(...)/uint16(...)
conversions. Two-byte two's-complement storage uses existing signed type tag 2.
Conversions admit numeric values, without wrapping or bit reinterpretation.
Arithmetic failures retain statuses 1/2/3 by operator; conversion Bounds failure
is status 32. Failed destinations and prior effects follow the existing contract.
Mixed arithmetic/comparisons require an explicit conversion. For counters and
paired divmod remain uint16; signed division and unary variable negation are
outside this tranche (0 - value supplies checked negation). Production compiler
and generic VDPlet runtime/SDK remain unchanged.

- SYN-001 [x] Add signed declarations/literals, typed comparisons and explicit scalar conversions.
- EMIT-001 [x] Implement checked signed arithmetic, Bounds, conversions and comparisons on both targets.
- CHK-001 [x] Qualify signed boundaries, sign crossings, multiplication, conversions, errors and prior regressions.
- CHK-002 [x] Run stock and actual Xtensa engines with relocated IDs and shared-state retention.
- VIS-001 [x] Stage a signed-motion animation with complete source on Oryx, stopped.
- REV-001 [x] Obtain human review and explicit commit/push approval.

## Evidence

[Report/source/review command](GOLEM-031/README.md),
[host receipt](GOLEM-031/host-checks.json), [target receipts](GOLEM-031/evidence/).
Stock and both actual Xtensa engines pass. Human review is staged/stopped on
Oryx. The Author accepted review with “looks good”; saved results pass all 481
observations/4,729 bytes and runtime identities pass. The Author explicitly approved commit/push on 2026-10-13.
