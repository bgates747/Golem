# Resident uint16 paired quotient/remainder

GOLEM-015 implements paired resident uint16 divmod. Host/sanitizer and stock
Pi/Oryx headless qualification pass: 2,126 checks across 156 jobs. New-input/call
traffic remains 21 bytes. The Author confirmed Oryx human review succeeded and
the agent retrieved and independently verified every saved byte. The Author explicitly approved
commit and push after human validation.

## Implementation

`(quotient, remainder) = divmod(numerator, denominator)` requires distinct uint16
global outputs. Arguments evaluate once, left to right, using the current checked
expression grammar. Private snapshots support input/output aliases. Argument
errors and zero divisors preserve both outputs, skip later statements and return
to normal VDP processing; earlier writes remain. Zero divisor has status 4.

The backend shares a finite 16-step restoring division kernel. Conditions use
zero-extended bytes to avoid 0xFFFF sentinel operands. A low-byte comparator tail
jump limits helper depth to three. The caller checks zero before entering the
kernel, leaving no source error propagation through nested helper calls.

## Evidence and review boundary

Maintained warnings-as-errors checks and ASan/UBSan pass with 43 rejection cases.
Independent modeling covers 156 default/relocated jobs plus 16 argument/order/alias
cases. Python integer divmod supplies the oracle. Full-width operands, all bits,
zero divisors at both sites, destination preservation, repeated failure/recovery,
cleanup/reinstall, relocation and external-buffer preservation pass. GP queries
confirm parser progress after both success and failure.

Stock Fab 1.2.5/MOS v3.0.2 on Pi and the dedicated Oryx runtime produce identical
12,232-byte results. The two-site fixture installs 5,086 bytes across 55 buffers.
All five pure-floor artifacts and all four reviewed arithmetic command artifacts
remain unchanged against 0c311ba; only arithmetic manifests add error code 4.

Host qualification took 33.404 seconds; Pi execution/collection took 15.996
seconds; Oryx execution/collection/verifier took 23.712 seconds. Local monotonic
timings are not calibrated benchmarks. No upstream checkout was modified.

The Author launches Oryx, follows the onscreen dir instruction, closes it and
reports keyboard behavior. The agent then collects saved results. Headless
qualification does not replace human review. No new tranche commit is authorized
until that review passes and the Author explicitly approves committing.

See [GOLEM-015 instructions and evidence](../tasks/GOLEM-015/README.md) and the
[maintained resident contract](../design/resident-scalar.md). Signed division,
/ and % syntax, general functions and raw-mode divmod remain deferred.

Human review outcome: the Author reported “success”. The agent retrieved the
12,232-byte result file and independently matched it against the expected bytes;
all 2,126 checks across 156 jobs pass. The remote guest hash matches the staged
fixture. [Retrieval evidence](../tasks/GOLEM-015/human-review.json) records hashes.
The Author explicitly approved commit and push.

GOLEM-015 is accepted and removed from the authoritative TODO register following
human review, verified results and explicit approval to commit and push.
