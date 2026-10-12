# GOLEM-010 / AST-001 — Asset and table storage review

AST-001 is complete and awaits Author review. Recommend retaining Rally's
install-once binary imports, adding explicit typed table rows and validating all
rows during import. Keep package-owned imports distinct from borrowed external
reservations and hide ordinary buffer placement. No compiler or shared-example
changes were made; subsequent subtasks have not started.

The source audit found that Rally's Table is a collection of fixed-width binary
records without a declared row schema. Typed validation occurs against each
lookup destination, whereas Asset Fields are validated during compilation.
Rally's consecutive 4000:4999 table window and private/user split are application
policy, not required language semantics. Golem already has allocator reservations
and owned cleanup suitable for extending the package contract.

The review records binary formats, exact size and range checks, ownership,
resource limits, proposed declarations and installation/recurring traffic.
A 228-byte selected row can be represented by a two-byte index, reducing the
illustrative Set-plus-call traffic from 245 to 19 bytes. This is an encoding
comparison; it does not add a new executable profile or change the current
21-byte float-update/call interface.

Rally's existing import regression passed warnings-as-errors and ASan/UBSan,
including fourteen negatives, in 41.962 seconds measured locally with a monotonic
clock. Generated binaries/fixtures stayed in GOLEM-010's ignored build directory;
the reference checkout remained clean. No emulator or hardware test was needed
for this source/storage review. Report and reproducible runner are in the silo.

[AST-001 findings](../tasks/GOLEM-010/assets-and-tables-review.md) and
[host evidence](../tasks/GOLEM-010/import-audit.json) are ready for review.
IDX-001, CPY-001, SYN-001 and REV-001 remain unfinished in the task plan.

The Author endorsed the storage direction and requested an AGNB follow-up.
The [AGNB addendum](../tasks/GOLEM-010/agnb-loading-review.md) records the primary
loading benefits: one sequential container read avoids repeated MOS directory
searches, and the build accounts for source filenames and embedded buffer IDs.
Existing image/audio loaders are reusable; raw/table forms need an extension.
Local adaptation is authorized, with useful general changes eligible for later
upstream proposal. No agon-utils files or compiler code were modified.
