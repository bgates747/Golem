# GOLEM-010 / REV-001 — Final integration recommendation

GOLEM-010 review work is delivered and awaits final acceptance. Recommend adding
generated assembly output for already-qualified packages, then implementing a
small read-only typed table and whole-row selection entry. No compiler or emulator
changes were made; implementation awaits authorization.

The first output tranche should emit symbolic addresses and labeled command/data
bytes with lengths from the existing compiled layout, preserving current binary
artifacts. An assembled consumer and byte comparison provide focused validation.
The table tranche then adds flat uint16/sint16 row storage and index admission,
without signed arithmetic or broad conversion support.

Source inspection identified a prerequisite: the current resident grammar
requires a bounded float-floor assignment. A table-only entry should not require
a dummy float job. Generalize the finite package/entry path while retaining the
reviewed scalar profile. Allocate table record runs around external reservations,
check indices in VDP and preserve the selected row on error. Assembly still owns
installation, updates, calls, pacing and cleanup.

AGNB is useful for bulk assets; a new raw/table form is not needed merely to
include a 16-byte table payload in assembly. Build/configuration handles filenames
and IDs. Local extensions can be proposed upstream when useful. Keep input
checks proportionate and avoid redundant admission on trusted resident data.

The [recommendation](../tasks/GOLEM-010/implementation-recommendation.md) records
scope, dependencies, candidate source and acceptance evidence. All detailed
review subtasks are checked off; the authoritative TODO retains GOLEM-010 until
the Author accepts its final recommendation. No new implementation task has
been promoted or started, and no additional runtime tests were run.
