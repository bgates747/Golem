# Checked resident addition and error exit

GOLEM-012 implements the first accepted checked-arithmetic slice: uint16 addition
inside the resident job. Overflow preserves the failed destination, skips later
statements and returns to normal VDP processing. Automated host and stock-emulator
qualification pass, including the dedicated Oryx profile. Human review and commit
approval remain pending; fresh-input/call traffic remains 21 bytes.

## Review disposition

GOLEM-009 REV-001 is disposed by the Author's conversational review and explicit
GOLEM-012 authorization. The living language document records conversions,
ByVal/default composite value semantics, caller-owned returned objects, lexical
scope/no shadowing, definite assignment, For counter ownership and acyclic calls;
integer division/remainder, paired divmod, checked arithmetic and finite resident
error exit are selected. Custom handlers, Resume/retry and ByRef are deferred.
Concrete composite ABI implementation is later feature work. The audit is removed
from TODO; this does not authorize implementing all its syntax proposals.

GOLEM-012 DEC-001, IMP-001, TST-001 and VDP-001 are delivered. REV-001 remains open
for human acceptance. GOLEM-010 assets/tables and GOLEM-011 typed drawing remain
deferred. GOLEM-002 native-module research remains separately tracked.

## Implementation and evidence

The compiler checks accumulator carry after every add and commits the destination
only after the whole expression succeeds. An owned error helper sets code 1 and
ends after a conditional jump replaces the job stream. Carry checks compare two
owned words using buffer operands, avoiding the known literal/sentinel pitfalls.
The entry clears optional runtime error status on every call. Earlier successful
writes persist and a corrected later call succeeds without host acknowledgement.

Constant overflow is rejected before publication; variable initializer sums must
move into the job. The backend owns, relocates, installs and cleans up all status,
scratch and error-helper buffers. The pure floor package retains all five artifact
bytes and representative raw output remains byte-identical to c83463b.

Maintained checks pass with warnings as errors, including 29 rejection cases;
ASan/UBSan pass. An independent command model executes 88 boundary/repeated/recovery
jobs across two layouts. Stock Fab 1.2.5/MOS v3.0.2 executes the same 88 jobs and
passes 1,238 checks with 4,022 observation bytes. Follow-up GP queries confirm
VDP parser progress after success and overflow. Both layouts preserve unrelated
buffers, clean up all owned buffers and reset correctly on reinstall.

Pi qualification took 5.925 seconds after 0.027 seconds preparation; host checks
including sanitizer compilation took 22.213 seconds. Oryx headless verification
of the identical guest took 8.327 seconds including collection and verifier.
Timings use each host's monotonic clock. See the task reports for pinned identities.

Pure floor remains 1008 installation bytes/25 buffers. Counted floor grows from
1120/28 to 1238/32. New-input calls remain 21 bytes; plain calls remain 6 bytes.
Physical hardware and general arithmetic/handler support remain outside this
qualification.

## Human review boundary

The dedicated Oryx `.emulator` retains its accepted local runtime and MOS identity.
The new guest/verifier/expectations are staged under GOLEM-012; prior startup is
backed up in its local ignored build directory. Automated output was removed and
the emulator is stopped. No GUI or attention alert was launched. The Author must
launch locally, run the verifier and type `dir` at the MOS prompt. Exact commands
and evidence are in [the task README](../tasks/GOLEM-012/README.md).

The canonical Agon AGENTS.md explicitly requires human testing and commit approval
for emulator-related changes. All work remains uncommitted and unpushed until
that gate is satisfied; earlier GOLEM-007 acceptance does not cover these new bytes.

## Human result and instruction placement

The Author subsequently confirmed that the Oryx review worked. This establishes
human acceptance of the tested behavior, but no explicit commit instruction has
been given. The Author also pointed out that the test program should print the
`dir` instruction itself. The guest completion message now tells the user to
type `dir` at the MOS prompt and report that it works; the numerical fixture and
compiler implementation are unchanged. The updated guest is rebuilt and staged
on Oryx with automated qualification; it remains uncommitted.

The Author subsequently explicitly authorized committing and pushing all work,
then proceeding to checked resident uint16 subtraction. GOLEM-012 REV-001 is
accepted and removed from TODO. The instruction-message revision passed the
same 1,238 automated checks on both Pi and Oryx before this authorization.
