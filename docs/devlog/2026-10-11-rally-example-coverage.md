# Rally example coverage — GOLEM-009 / INV-002

The Author reviewed INV-001 and requested commit, push and execution of INV-002
only. The reconciliation was committed/pushed as `bcea0c2`. INV-002 is complete
and awaits review before SYN-001; no examples or compiler code were changed.

`docs/tasks/GOLEM-009/example-coverage.md` accounts for all 32 reconciled inventory
rows. Central `examples.golem` covers 3, partly covers 3 and lacks examples for
18; 3 concern host/tooling, 1 optimization row is parked and 4 rows are deferred.
These are grouped capability counts, not counts of missing functions. Existing
floor/conversion, matrix/rotation, procedure/call and float-product proposals
outside that file are identified for reuse rather than redesign.

Comparators and structured conditionals/loops remain the syntax baseline.
Assets/indexing/checked copying and typed drawing remain deferred to GOLEM-010/011.
The coverage evidence records file hashes and 52 checked line anchors; sources
and examples match the committed checkpoint. Only document/reference checks
were performed; no compiler tests, emulator or hardware runs were needed.
