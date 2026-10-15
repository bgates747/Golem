# Flower draws from resident endpoints

The native Flower calculation now feeds a small stock-command renderer without
sending each endpoint over the wire. Both Pi engines pass the same complete
fixture and produce identical curves. The interactive C++ application is next.

The Author accepted and authorized publication of the mathematical tranche:
Golem `52ed327` and SDK `580a5d8` are committed/pushed. The Author then released
the next tranche and restricted emulator work to the Pi 5 while Oryx undergoes
controlled thermal testing. No Oryx access occurred in this tranche.

DRAW-001 / RND-001..004 are complete under GOLEM-044. The task-local generator
derives state/array offsets and ownership from compiler metadata. Its four
reserved buffers retain 2187 bytes and install in 2243 wire bytes. Computation
and rendering submit in 12 bytes per batch; the tested status/remaining/plot
completion protocol sends 49 and receives 30 bytes. Full state/point readback
occurs only in qualification and is excluded from the batch measurement.

Each actual Xtensa engine passes 24,883,200 complete-image pixel comparisons,
eight full curves, default/relocated layouts and 92 measured finite batches.
The fixtures include counts 0/1/31/32/33/256/65535, distinct stale tails, signed
truncation, clipped/cardinal/zero-length segments, status/entry sentinels and a
real overflow after one endpoint. That failed batch draws nothing and preserves
the earlier picture and partial mathematical effects. Cleanup preserves shared
state, arrays and the picture. Two actual assembler consumers and sanitized
loader admission pass.

All 200610 retained VDPlet reply bytes and all curve images match between engines.
Default and 360-degree pictures match, including relocated defaults. Defaults
draw 396 segments in twelve 32-point batches and one 12-point batch. The native
default image retains the previously accepted mathematical hash. Runtime/SDK/
firmware/maintained compiler code and emulator sources are unchanged.

Measured batch protocol duration is 2.79..7.80 ms in modeled 240 MHz guest time;
whole-task free stack floor is 2120 bytes. Complete runs take 66.70 seconds JIT
and 103.66 seconds interpreter, including boot, diagnostic readback and images.
These are functional emulator measurements, not physical UART/cancellation
guarantees. No MOS/client run, keyboard/mode-restoration acceptance or physical
flashing is claimed.

The pixel fixture caught viewport reset clearing the origin; setup now resets
viewports before centring. Buffer comparisons also preserve upper sentinel
bytes, so the bridge uses buffer-backed constants and padded-byte external
guards, including all-ones values. Neither finding required firmware edits.
The host must still await/check native runtime success before rendering; a
rejected invocation can otherwise leave an old successful root.

See [renderer source/report](../tasks/GOLEM-044/renderer/README.md) and
[engine parity](../tasks/GOLEM-044/renderer/evidence/parity.json). Visual staging
remains deferred at the Author's request. New renderer work is uncommitted for
review; the authoritative TODO keeps GOLEM-044 open for C transport, interactive
input, visual review and remaining math/accuracy follow-ups.

The Author subsequently accepted retaining the transparent 32-point batching
and explicitly approved commit/push on 2026-10-15. The C++ client tranche is
released, with headless checks and a stopped Oryx review profile; Oryx is now
available and the Author will launch the interactive review themselves.
