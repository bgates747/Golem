# GOLEM-040 — Promote accepted compiler work into the maintained toolchain

Useful language features now live in accepted task prototypes while the normal
compiler still offers a much smaller language. Consolidate the accepted work
into a compiler people can build and use through the maintained toolchain,
with clear documentation and existing regression evidence. Preserve the stock
buffered and native targets' shared language, storage and error behavior.
First inventory the implementations and agree the integration boundary; then
promote in coherent, testable slices. This document is a plan for review, not
authorization to begin implementation.

## Ownership and limits

This is Golem compiler/toolchain work. It does not depend on Rally's current
application, emulator/UART diagnosis or scheduler design. Generic native
packaging uses the existing agon-vdplet SDK as an external dependency; that
repository continues to own firmware, loader and public module ABI.

Preserve historical task snapshots and published packages. Production compiler
code and required tests must not include or import task implementations after
promotion. Keep investigation reports, source maps and exploratory scripts in
`docs/tasks/GOLEM-040/`; promote only the files justified by an agreed slice.
Keep C++ as the compiler implementation language, with Python for auxiliary
tooling. Avoid a new optimizer, wholesale rewrite or unnecessary test framework.

Do not treat syntax in examples or a successful build as feature acceptance.
Record source identity, acceptance evidence, remaining limitations and whether
an implementation is portable, native-only, legacy-profile-only or still proposed.
Preserve existing raw/resident usage while designing the portable entry point;
any unavoidable incompatibility must be presented explicitly before changing it.

## Proposed first tranche — complete inventory and integration design

This tranche can be completed without emulator use or a Rally response. Its
result is a concrete promotion sequence and file/test boundary, ready for review.
It does not change production code or promise that all prototypes can be merged
as independent pieces.

- INV-001 [ ] Map maintained `src/`, build/CLI, tests and examples to their current supported contracts.
- INV-002 [ ] Inventory accepted shared frontend, scalar and control-flow work from GOLEM-024–032; locate final implementations and acceptance/qualification evidence rather than reviewing every intermediate artifact.
- INV-003 [ ] Trace root-interface, typed-array and wide-integer successor implementations from GOLEM-034–036, including multiplication, division and right shifts; identify inherited code, duplicate forks and cross-task includes.
- INV-004 [ ] Classify GOLEM-022's multiple drawing Subs and GOLEM-037's renderer binding extension separately; verify their review status and keep application-specific code out of the portable core.
- DEP-001 [ ] Select a pinned candidate source base for promotion and map its complete dependency closure. Distinguish generic compiler code, maintained helpers, historical fixtures and external SDK tooling.
- CON-001 [ ] Record the contracts promotion must preserve: explicit types/initialization, terminator-free grammar, evaluation order, checked errors/prior effects, inclusive bounds, entry IDs, packed state, reservations, ownership and generated assembly/layout artifacts.
- CLI-001 [ ] Propose the maintained user-facing build/target-selection commands, dependency setup and coexistence with current raw/resident profiles. Identify any interface/versioning decision requiring review; do not silently pick a breaking replacement.
- MAP-001 [ ] Produce a feature/evidence matrix and exact proposed file moves/extractions. Recommend a coherent first implementation slice, followed by ordered dependent slices; shared scalar syntax/types/layout are the starting point unless actual coupling requires a larger slice.
- CHK-001 [ ] Map existing useful checks to those slices: compiler rejection/contracts, byte/layout parity, generated-C/native checks, assembler consumers and selected prior target evidence. Separate immediately reusable checks from fixtures that need adaptation or new qualification.
- REV-001 [ ] Return a concise integration recommendation, uncertainties and acceptance criteria to the Author. Stop before production edits, test execution, emulator deployment or publication.

## Subsequent implementation boundaries

These boundaries guide the first-tranche recommendation; they are not a fixed
claim about how the current forked sources can be separated. The inventory must
establish the actual dependency order before implementation begins.

- PRO-001 [ ] Promote an agreed common frontend/types/layout slice into maintained source and tests, preserving existing compiler modes. Supply initialized standalone examples and a documented build path.
- PRO-002 [ ] Integrate the accepted generic buffered/native emitters and scalar/control-flow behavior through that shared boundary. Keep native SDK packaging outside ordinary host-compiler prerequisites where practical.
- PRO-003 [ ] Integrate the accepted root/interface, indexed storage and wider numeric features in the dependency order established by the inventory. Preserve public layouts and named assembly contracts or explicitly document a reviewed migration.
- PRO-004 [ ] Decide separately whether generic native-binding support or additional drawing-profile changes are ready for promotion. Rally-specific helper implementations, assets and game policies remain outside the core compiler.
- TST-001 [ ] Move necessary regression coverage into maintained tests without task-code dependencies. Reuse established oracles and qualify changed integration paths; do not rerun every historical emulator demo by default.
- DOC-001 [ ] Update README, maintained language/backend contracts, examples and handoff so supported features, proposed syntax and external requirements are accurately distinguished.
- QUA-001 [ ] For any implementation that changes emitted commands, layouts, native behavior or emulator deployment, define and complete focused target qualification. Stage any required Oryx human review fully, stopped, with dummy video/audio for automated runs and the Author launching the visual fixture.
- PUB-001 [ ] Obtain applicable acceptance and explicit commit/push approval, then publish coherent implementation groups. Leave frozen historical evidence unchanged.

## Acceptance

A normal documented build exposes the promoted features through maintained
source, with no compiler or required test implementation drawn from task silos.
Existing raw/resident contracts continue to work, and supported portable source
retains matching semantics and storage across buffered/native selection. Native
output states its external SDK/firmware requirements clearly; unsupported
target-specific features reject explicitly.

Documentation and examples describe the actual maintained compiler. Meaningful
regressions and any necessary integration qualification pass. Promotion does not
claim a new language feature, faster runtime, autonomous scheduler, new firmware
service or resolution of the unrelated UART defect.

## Estimated tranche size and review boundary

One substantial first tranche can complete INV-001–004, DEP-001, CON-001,
CLI-001, MAP-001, CHK-001 and REV-001: the full inventory/dependency audit, pinned
baseline recommendation, proposed maintained command/file boundary, ordered
promotion slices and focused validation plan. **High thinking** is appropriate.
No hardware, emulator run, cross-agent delegation or user input should be needed
unless the audit exposes a concrete compatibility choice that available
contracts do not resolve. Present that choice with a recommendation and finish
the independent inventory work before stopping.

The next review is of that concrete integration recommendation. Implementation
is a later tranche, sized from the evidence rather than promised in advance.

The Author released the first inventory/design tranche on 2026-10-14 with an instruction to commit/push first, then execute. This does not release production implementation.
