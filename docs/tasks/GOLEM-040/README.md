# Compiler promotion recommendation

Promote the latest accepted general compiler as an additional maintained mode,
keeping today's raw/resident modes intact. Its cumulative implementation is
only four task-local C++ files, about 2,000 lines, using three existing maintained
headers. Moving that coherent implementation is simpler and safer than
reconstructing each old feature tranche. Application-specific renderer bindings
and emulator work should stay outside this promotion.

The first inventory/design tranche is complete. No compiler builds, tests,
emulator runs or production changes occurred. The decision now is whether to
proceed with the additive integration described in
[the implementation sequence](integration-plan.md).

## Findings that determine the approach

1. **The general compiler has a clear baseline.** Use the four files under
   `GOLEM-036/right-shift/` at accepted commit `b32c596`. They include earlier
   portable scalar, Boolean, branch/loop, float, root-interface, array and wide
   arithmetic work. Their full quoted-include closure has seven files and no
   includes of older task implementations. Exact paths, line counts and SHA256
   values are in [source-inventory.json](source-inventory.json).
2. **The latest renderer work did not fork the generic core.** Its portable,
   backend and root headers are byte-identical to that baseline. Its changed
   driver and extra binding header introduce Rally-specific hooks and resource
   configuration. Select the general driver, not that renderer extension.
3. **Existing modes need explicit preservation.** Raw and resident profiles
   have different grammar/storage contracts, including legacy floor conversion,
   record/table selection and drawing. They are not all expressible through
   the portable prototype. Use an explicit new mode rather than source guessing
   or replacing the current CLI.
4. **Multiple drawing Subs already reside in `src/`.** GOLEM-022 changed
   maintained source and tests at `97c1651`; it is not another missing compiler
   promotion. Its task still records human review as pending. Preserve the code
   and separate status; do not infer visual acceptance or merge it into the
   portable grammar by accident.
5. **Historical qualification remains useful but scripts need adaptation.**
   Current SDK policy/build files differ from the old pinned SDK. The old
   right-shift builder will reject that pin mismatch, and couples compiler
   generation to native packaging, a task-local validator and stack assertions.
   Port its useful checks rather than running it against a moving SDK or
   silently updating its historical receipts.
6. **The first implementation can deliver a usable compiler.** Keep the
   accepted generic parser/model/backend/root layers together initially. Their
   boundaries are sufficient for promotion; moving arrays/wide types out of
   this cumulative parser would be a new refactor with no demonstrated benefit.
   Split the headers further only when an actual maintenance need warrants it.

## Recommended user-facing boundary

Proposed commands, not yet implemented:

```bash
make -C src check
build/golemc --portable input.golem output-directory
```

Existing raw and `--resident` commands retain their meanings. The new mode emits
common initialized storage, interface/layout/assembly metadata, both buffered
packages and generated native C through one frontend. Generating those files
requires the host C++ toolchain, not PlatformIO or an emulator.

Compiling native C into an Xtensa VDPlet remains a separate optional SDK build.
Applications choose which emitted implementation to install/call. The first
promotion need not add output filtering or a new runtime selector to meet this
existing two-package contract. Later `--target` filtering must preserve shared
layout and deliberate private allocation rather than borrowing the renderer's
native-only allocation path.

Keep named entries, layout metadata, little-endian images, root-v1 fields and
error semantics as qualified. Use named `--reserve FIRST:LAST`, `--bind` and
`--array-data` options instead of exposing the prototype's positional reservation
prefix. Compatibility of generated artifacts, not historical scratch-command
syntax, is the constraint. Detailed defaults and validation are in
[the integration plan](integration-plan.md).

## What this makes maintained

[The feature/evidence matrix](feature-matrix.md) distinguishes the six scalar
types, checked operations, comparisons/Boolean logic, finite branches/ranges,
typed arrays, shared root/state and assembly interfaces from legacy profiles,
native-only renderer bindings and proposed syntax.

It does not implement general Functions/parameters/locals, strings, Do loops,
Switch/jump tables, matrices, multidimensional arrays, fixed-point types,
sqrt/pow or general floating division. Syntax discussions/examples are not
acceptance evidence for those features. The legacy floor profile remains
available through its existing mode.

## Integration risks and qualification

1. Keep the existing `src/` tests and output contracts intact while adding
   maintained portable tests. New required tests must not import task code,
   include frozen task headers or require an emulator/SDK installation.
2. Compare emitted commands, shared state, interface/layout and native C against
   copied, provenance-labelled accepted fixtures at default/relocated IDs.
   Preserve header/type/error tags and initial-value semantics. Do not rename
   the existing `R25-G0` contract identifiers merely for cosmetic branding.
3. Native packaging has changed policy defaults since the old qualification.
   Equal generated C is not a claim that a newly packaged `.vdpl` must match an
   old image hash. Pin the chosen SDK/settings and qualify any newly deployed
   package separately; the public module header/runtime sources still match
   the historical pin in this audit.
4. The generic root generator rewrites generated C through guarded text anchors.
   Preserve those anchors during the initial move. Refactoring that mechanism
   is separate from promotion and would need its own parity checks.
5. The generic native layer ignores release return values on several exit paths.
   The renderer extension checks its final root/publication releases under its
   separate tested contract. Record this difference; do not quietly import
   renderer glue or claim its failure coverage applies to the generic package.
   A narrowly scoped generic release-result improvement can follow the parity
   checkpoint, with explicit changed-behavior tests.
6. The prototype driver reserves IDs 60000–60031 even without explicit user
   reservations. Retain/document that compatibility default initially so the
   copied fixtures remain comparable; removing it is a later allocator-policy
   decision, not an incidental cleanup.

No unresolved compatibility choice blocks the recommended initial integration.
Its boundary is explicit and preserves existing commands/artifacts. Following
implementation, run maintained host checks, targeted sanitizer/generated-C and
assembler/parity checks. Any changed executable/layout or newly deployed native
package needs focused target qualification and applicable human review; do not
revive every historical visual demo by default. Automated emulator sessions,
if later released, must be both windowless and silent.

The report and supporting receipts are newly written and uncommitted. The plan
was committed/pushed first at `43e072e`; that release covered this audit only.

The Author subsequently released implementation after audit publication at f2ed568. The [maintained mode and qualification](implementation.md) are now ready for review; this report above preserves the audit rationale. New implementation remains uncommitted.
