# GOLEM-010 / IDX-001 — Indexed access review

IDX-001 is complete and awaits Author review. Recommend ordinary parenthesized
array/table access with inclusive bounds checks inside VDP. Failure preserves
the assignment destination and ends the finite job under the existing error
model. No compiler or shared-example changes were made; CPY-001 has not begun.

Rally already supplies two guard-then-patch-then-call paths: table record ID
selection and packed element byte-offset selection. Its status-flag failure
returns to the caller, so direct transfer would require application boilerplate
and could let subsequent code use a stale result. The Golem proposal uses the
current resident halt-on-error policy instead; an explicitly named try-access
operation can be considered later if needed.

The review proposes checks before normalization/address arithmetic, one-time
left-to-right index evaluation, complete-row selection and future per-dimension
checks. Signed lower bounds do not mean Python-style reverse indices. Full-width
counts, conditional sentinel collisions, offsets beyond Rally's 65535-byte
source cap and arbitrary AGNB physical IDs require explicit backend handling.
The logical language need not expose buffer IDs, byte strides or filenames.

Rally's existing element regression passed warnings-as-errors and ASan/UBSan in
42.002 seconds (local monotonic clock). Generated binaries stayed under the task
silo; source fingerprints match and the reference worktree remains clean. This
is host regression/source review evidence, not qualification of new Golem
runtime operations. No emulator or hardware was launched.

See the [indexed-access proposal](../tasks/GOLEM-010/indexed-access-review.md) and
[index audit](../tasks/GOLEM-010/index-audit.json). Pause for review before CPY-001.
