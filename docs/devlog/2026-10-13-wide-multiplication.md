# Checked wide multiplication — 2026-10-13

GOLEM-036's second tranche adds checked uint32/sint32 multiplication to the same
stock/native source contract. Host/stock/actual-Xtensa qualification passes;
source-labelled Oryx review is staged/stopped, with human acceptance/publication
pending. Prior foundation sources/packages remain intact. Division and remaining
wide numeric work stay open.

1. One bounded shared 32-round buffered magnitude kernel computes the full private
   product before range checking. Signed minimum, unsigned maximum and nested
   intermediate retention are explicit. Native/host intermediates avoid signed
   overflow. All failures preserve the failed destination and prior effects.
2. Host checks pass 1,682 cases, including all six independent Rally multiplication
   vectors in both layouts. Accepted foundation executable/layout/C artifacts
   remain byte-identical; production compiler checks and SDK policy are unchanged.
3. Stock probes/motion and both actual Xtensa engines/default-relocated layouts
   pass. The 300-frame quadratic/signed-product animation has 460 observations/
   4,546 bytes and 56,320 checked final pixels. Ordinary frame wire remains 15
   bytes; no per-marker host updates or physical timing claims.
4. Rally's separate host integration-harness handoff was received/hash checked.
   It exercises v0.3 publication/failure/draw gates as a host model, with no new
   ABI request or generated-glue/target qualification. D2 remains unreleased.

[Task plan](../tasks/GOLEM-036.md), [source/evidence/resources](../tasks/GOLEM-036/multiplication/README.md).
TODO.md owns overall status. New emulator-related work remains uncommitted for
Author review; baseline receipt mail was committed/pushed as d362376 first.

GOLEM-036 multiplication human review subsequently passed: the Author accepted it; retrieved results match all 460 observations / 4,546 bytes, with runtime/fixture identities verified. See docs/tasks/GOLEM-036/multiplication/evidence/human-review.json. The Author subsequently explicitly approved commit/push; division is a separate next tranche. Rally acknowledged the candidate identity and is planning its own bounded host qualification using generated metadata; this is not independent target acceptance or a shared ABI change.
