# Failed-display release: target results

The small failure-only correction restores usable video after the oversized
mode request in both instruction engines. The unchanged controls reproduce the
broken fallback. Candidates preserve the loaded worker, unload it normally and
recover the executable heap exactly. The patch is still a separate experiment;
it has not changed the maintained SDK or made the larger display mode fit.

| Check | Interpreter candidate | JIT candidate |
| --- | --- | --- |
| Requested mode 20, actual restored mode 11 | Pass | Pass |
| Three fresh decoded frames and ordinary readback | Pass | Pass |
| Retained 96-byte CPU/device projection,64 instructions/205 cycles | Pass | Pass |
| FINI, unload, owned cleanup, empty slot | Pass | Pass |
| Primary executable heap | 161,464→161,464 | 161,464→161,464 |
| Ordinary mode 21, fresh frames, black centre, return to mode 11 | Pass | Pass |
| Final executable heap | 161,464→161,464 | 161,464→161,464 |
| Failed/unhandled/unregistered/unmapped | 0/0/0/0 | 0/0/0/0 |

Exactly four fresh processes ran under exclusive generation 34 and released
before analysis. Controls remain strict failures with expected bounded null-table
reads; their missing FINI/heap cleanup is not waived. Candidate source/module/
client/backend pins and the original reproduction prefix remain unchanged.

The original result files contain a stale `cleanup_complete=false` field,
initialized but never assigned. They remain unmodified. The frozen strict gate
checks the actual cleanup-command, FINI, slot, mode and both heap constituents;
all are independently verified. Consumer aggregate `90333cba` and cleanup-witness
addendum `51c88597` explain this reporting defect without rerunning or relaxing tests.

Other memory pools retain differences: primary internal-byte pool−512 bytes,
final−592; PSRAM−44 at both points, identical across engines. Their cause is
unproved and pools overlap. Exact executable-heap recovery does not mean every
pool returns to its initial size. No full RAM, physical or speed claim follows.

The matched build cost remains 176 flashed bytes, 12 IRAM bytes and no static
DRAM. See [the build report](README.md), [root reconciliation](target-results.json)
and consumer aggregate at
`/Agon/mystuff/agon-6502/docs/validation/2026-10-30-vdp005-mode-repair.json`.
A small actual-MOS human review is preparing before a production-promotion decision.
