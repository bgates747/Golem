# GOLEM-041 — Report native buffer-release failures

The generic native emitter currently discards buffer-release results on several
exit paths. Make those failures visible to the caller while still attempting
all remaining cleanup. Preserve source-language behavior, layouts, successful
results and buffered command bytes. The Author released this tranche on
2026-10-14 after requesting publication of the accepted compiler promotion.

## Contract and scope

Golem owns this generated-C change and focused conformance tests. No SDK/firmware,
Rally renderer, scheduler, UART or other-project source change is required.
Keep task fixtures, reports and qualification scripts under `docs/tasks/GOLEM-041/`;
promote useful generic checks into maintained `tests/portable/`.

1. On every exit after acquiring root/argument views, attempt each acquired
   view's release once, root first then argument. A cleanup failure must not
   skip the other release. Do not release zero/unacquired tokens or retry a
   failed callback. The runtime retains responsibility for outstanding lease
   cleanup after normal native return.
2. Return the first cleanup error if any; otherwise retain the primary result.
   This matches the existing composed helper's cleanup-error precedence without
   importing its application glue. Preserve language/interface status fields
   already written while the root is valid. Do not write root bytes after its
   release attempt or promise rollback of earlier effects.
3. Final release follows successful-generation publication. If it fails, the
   invocation's native return is failure even if root success markers advanced.
   The caller must gate success on the runtime result as well as root fields.
   Changing publication order or the wire/field schema is outside this task.
4. Existing checked temporary-array/bound-view releases already report errors;
   retain their behavior, including propagation through root/argument cleanup.
   Keep normal successful calls, INIT/FINI and pre-acquisition validation intact.
5. Native C/code bytes may change. Buffered output, shared initial state, root/
   array layouts, entry IDs, wrappers and assembly contracts must remain equal.
   Preserve frozen accepted fixture files; explicitly separate the intentional
   native-C change from parity checks rather than rewriting historical evidence.

## Subtasks

- SRC-001 [ ] Trace acquired-view exits and implement one bounded generated cleanup path, with error precedence and no post-release root access.
- CHK-001 [ ] Add maintained generated-C callback-failure cases for success, language/interface errors, root-acquire failure and checked temporary views; test both released-but-error and refused-release behavior.
- CHK-002 [ ] Verify cleanup of both views when either/both callbacks fail, first-error precedence, unchanged failed destinations/prior effects, and final-generation/runtime-result gating.
- CHK-003 [ ] Run maintained contracts and focused sanitizer/model/generated-C/default-relocated checks; verify all non-native artifact/layout parity and preserve frozen fixture provenance.
- QUA-001 [ ] Build/pin changed native output and perform bounded actual-Xtensa qualification against the unchanged SDK/runtime, separating injected host failures from actual runtime behavior.
- VIS-001 [ ] If deploying an updated emulator fixture, stage a complete stopped Oryx human review in this task, preserving accepted profiles. Automated sessions must be both windowless and silent; only the Author launches the visual instance.
- DOC-001 [ ] Document cleanup precedence, runtime-result obligation, source/target evidence and limits; update the maintained guide and task register.
- REV-001 [ ] Return results for review and obtain applicable human acceptance and explicit commit/push approval before publishing new implementation.

## Acceptance

Cleanup failures cannot produce an overall successful native return, other
acquired views are still released once, and normal successful program state is
unchanged. Focused negative checks cover all newly handled exit families without
new runtime retries, readback, checksums, rollback or unrelated refactoring.
Any actual-target/visual qualification uses separately owned task artifacts and
does not imply physical hardware testing or repair of the unrelated UART defect.
