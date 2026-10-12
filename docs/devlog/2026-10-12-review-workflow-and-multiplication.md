# Review workflow and checked multiplication

GOLEM-013 subtraction passed human review and all 1,248 saved Oryx checks. The
Author authorized commit/push and the next multiplication tranche. The accepted
review workflow is: the Author launches, follows onscreen instructions, closes
the emulator and reports success; the agent retrieves and verifies results.
The verifier no longer repeats `dir` instructions. GOLEM-013 is removed from TODO.

Checked resident uint16 multiplication is authorized as GOLEM-014. Human review
and explicit approval remain required before committing that new tranche.

## GOLEM-014 delivery

GOLEM-013 and the simplified review workflow were committed/pushed as 964d546.
GOLEM-014 IMP-001, TST-001 and VDP-001 are delivered; REV-001 awaits human review.
Binary uint16 * has higher precedence than + / -, with checked intermediates.
Overflow preserves the failed destination, exits with code 3 and skips later
statements; prior writes persist. Constant overflow is rejected even before a
later * 0. Variable initializer arithmetic must move into the job.

A shared finite 16-step VDP kernel computes an exact uint32 product. The job
checks upper bytes after the helper returns, so an error cannot return into later
source statements. Masks/high-byte checks avoid 0xFFFF conditional values. Owned
scratch/helpers relocate and participate in cleanup/reinstall; conservative call
depth is three. Sharing reduces the five-site fixture's installation from a
qualified preliminary 6952-byte inline version to 3344 bytes across 50 buffers.
New-input/call remains 21 bytes, plain call 6 bytes. Installation wire traffic,
rather than execution performance, motivated sharing.

Initial tests caught a declaration shortcut treating literal-starting products
as bare literals. The shortcut now requires no following factors. Maintained
checks pass with warnings as errors and 36 rejection cases; ASan/UBSan pass.
The independent model checks 120 jobs in two layouts and 14 additional expression
cases. Stock Fab 1.2.5/MOS v3.0.2 passes 1,670 checks over 120 jobs, producing
8,016 expected bytes; Oryx's accepted x86-64 runtime produces identical bytes.
GP queries confirm parser progress after success/error. Coverage includes all
multiplier bits, full-width values, precedence, chained products, aliasing,
preservation, skipped statements, repeated errors/recovery and ownership/lifecycle.

Pure floor retains all five artifacts. Reviewed addition/subtraction fixtures
retain all four command artifacts against 964d546; manifests add error code 3.
Four raw outputs are identical; three rejected packages preserve existing output.
Physical hardware and other firmware remain outside qualification.

Host qualification took 36.908 seconds including rebuild/sanitizers. Pi preparation
took 0.031 seconds and execution/collection 10.963 seconds. Oryx execution,
collection/verifier took 15.596 seconds. These use local monotonic clocks.

Oryx is staged/stopped and automated observations removed. The guest instructs
the Author to type dir, close the emulator and report whether it worked. The
agent retrieves/verifies saved results afterward; the verifier does not repeat
keyboard instructions. No GUI or alert was launched. [Review notes](../tasks/GOLEM-014/README.md)
are ready. This tranche remains uncommitted pending human validation and explicit
commit approval under canonical Agon AGENTS.md.

## Human acceptance and result retrieval

The Author reported that GOLEM-014 worked and authorized result retrieval. The
agent ran the Oryx verifier, retrieved results.dat and independently compared
all 8,016 bytes locally: all 1,670 checks across 120 jobs match. Guest/result
hashes and the Author confirmation are recorded in GOLEM-014/human-review.json.
Human review passed; explicit commit approval is still pending, so this tranche
remains uncommitted.

The Author subsequently explicitly authorized commit and push, then requested
the next recommendation. GOLEM-014 REV-001 is accepted and removed from TODO.
No further implementation is authorized by that status request.
