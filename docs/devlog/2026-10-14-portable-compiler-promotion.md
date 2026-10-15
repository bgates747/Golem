# Maintained portable compiler ready for review

After publishing the audit at f2ed568, the Author released the additive
implementation. GOLEM-040 PRO-001/002/003, TST-001, DOC-001 and QUA-001 are
complete; PRO-004 is disposed as deferred binding work/already-maintained
drawing support. PUB-001 awaits acceptance and explicit publication approval.

Four accepted generic files now live under src/portable/ behind --portable.
Shared language, typed model, emitters, root/interface/storage and assembly
artifacts are maintained without required task-code or SDK build dependencies.
Named options validate decimal/range/collision/mapping input. Existing raw and
resident modes remain available. README, portable guide and initialized example
explain actual supported features and external native packaging.

Normal and sanitized C++ checks pass all 55 accepted output comparisons and
new CLI/reference checks. Legacy CLI comparisons pass 37 files. Optional
model/sanitized-native integration passes 383 cases per default/relocated layout
(766 total), with every language-status family observed. A real eZ80 assembler
consumer passes. No target executable bytes changed in the accepted fixture
set, no native package was built/deployed and no emulator was run; prior target
evidence is reused without relabelling host checks.

Generic release-result propagation and target filtering remain separate work.
No renderer glue, SDK/firmware, Rally source, historical fixture or emulator
profile was changed. New implementation/tests/docs remain uncommitted.
See docs/tasks/GOLEM-040/implementation.md and implementation-checks.json.

The Author subsequently accepted the promotion and explicitly requested commit/push before GOLEM-041 execution. PUB-001 is complete.
