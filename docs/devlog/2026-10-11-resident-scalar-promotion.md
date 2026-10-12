# Resident scalar promotion

The Author explicitly approved committing GOLEM-007 and proceeding with scalar
production promotion after the Oryx review passed, including MOS keyboard
response. Qualification was committed/pushed as `ec91999`. Shared review
protocol documentation was committed/pushed in agon-dev-env as `9e89035`.

GOLEM-008 promotes the qualified backend and resident CLI into maintained source,
with separate reusable ordinary and resident APIs. Initialized maintained
examples and `docs/design/resident-scalar.md` document the supported slice and
host contract. `make -C src check` now runs maintained tests.

Promotion checks passed strict builds, sanitizers, boundary/relocation parity,
semantic rejection before writes and standalone source-only compilation.
Every command artifact in all four GOLEM-007 packages matches its reviewed hash;
manifest provenance alone changes. No new graphical review or firmware changes
were needed. See `docs/tasks/GOLEM-008/host-checks.json` for exact checks,
commands, source hashes and durations.

GOLEM-003's promotion follow-up is satisfied and GOLEM-008 is complete.
GOLEM-002 native-module research remains queued. No broader proposed syntax or
numeric operations were implemented in this tranche.
