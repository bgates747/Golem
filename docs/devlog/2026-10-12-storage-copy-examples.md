# GOLEM-010 / SYN-001 — Storage and copy examples

SYN-001 is complete and awaits review. The shared examples now show typed tables,
opaque assets, signed Array indexing, scalar/record/field assignments and indexed
stores. Initialization is explicit; assembly installs configured data and emitted
initializers. No compiler functionality changed and REV-001 has not begun.

EX-018 now shows initialization requirements and multidimensional store/read.
EX-023 through EX-028 add the remaining reviewed forms. Each block stands alone,
with typed declarations and its own valid initial state or assembly loading
precondition. A companion coverage note records exact configured payloads and
expected success/failure behavior, keeping detailed commentary out of the code.
The examples contain no ImportBinary source operation or filename lookup logic.

The agreed hobby-project budget remains explicit: useful input/bounds checks,
reuse of established validity for internal copies, no post-copy readback,
checksum or rollback mechanism. Assembly owns loading, updates and invocation;
generated assembly symbols/payloads remain an intended future output mode.
Existing AGNB image/audio forms do not imply raw/table support already exists.

Whitespace/diff review completed. These are non-executable design examples;
no compiler, emulator or hardware tests were run. Previous Rally audit evidence
remains in the task silo and does not establish implemented Golem syntax.

See [examples.golem](../tasks/GOLEM-003/examples/examples.golem) and the
[SYN-001 coverage note](../tasks/GOLEM-010/examples-review.md).
Stop for Author review before the final REV-001 synthesis.
