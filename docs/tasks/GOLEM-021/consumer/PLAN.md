# MOS consumer and integrated proof plan

The assembly application owns file loading, module upload, invocation and
cleanup. It runs after normal MOS/VDP boot and saves observations to SD. Neither
Flexe nor the emulator injects module code or numerical results into guest RAM.
Python builds the fixture and checks saved observations; it does not stand in
for eZ80 or ESP32 execution. All artifacts remain in this task silo.

- CON-001 [x] Read the existing MOS loader, saved-result and interrupt-mailbox idioms; agree the experimental wire ABI with the runtime worker.
- CON-002 [x] Write a small assembly tape runner that sends ordinary VDU packets, receives bounded VDPL replies through the MOS keyboard callback, correlates tokens and saves the reply transcript. The first assembly build passes.
- CON-003 [x] Build a deterministic numeric script: discovery, postboot module upload, inputs, repeated calls/state, unload/reload and different code at reused executable addresses. Initial fixture: 44 correlated replies; execution is tracked separately below.
- CON-004 [x] Run the script through the hybrid profile and check native float values, counters, handles, relocations and artifact identities. Final-adapter full MOS runs pass 44 replies/440 bytes in JIT 8.795 s and interpreter 15.723 s, with all three entries at 0x40097b64. Both observe the actual post-success MOS prompt before shutdown.
- CON-005 [x] Add the packed-row renderer proof after its service ABI is ready; use recognizable Apple II content and preserve compressed resident transfer accounting. Both engines pass actual Lode Runner pixels, native repeat calls and two preflight error cases; current final fixture also records command-task stack margin.
- CON-006 [x] Stage a stopped Oryx review with one launcher, readable source, clear on-screen roles and instructions. Gather machine results independently of the human visual check. Both final engines pass 56 replies and 61,440 pixels; the decoded screen was inspected to ensure every role/instruction line fits.
- DEP-001 [x] Build a portable consumer bundle containing the exact eZ80 script/application, module packages, actual firmware/diagnostic ELF and readable source. Verify transferred identities, generate the remote profile with the canonical tool, qualify it headlessly and leave it stopped. Use the canonical Oryx SSH name only; keep native frontend builds in emulator/ ownership. stage_oryx.py transfers 25 files, checks the bundle/per-file identities and retrieves independently hash-checked evidence.

## Receive and shutdown rules

Only one reply-requesting command is outstanding. The interrupt callback copies
the ten-byte `VDPL` payload into a mailbox and neutralizes its first four bytes
before ordinary MOS keyboard handling resumes. It makes no MOS/VDU calls.
Foreground code checks protocol, reply kind, request token and expected value.
Loading replies supply the handle used by subsequent calls; it is not assumed
to be a fixed firmware address or hard-coded handle.

All native calls are synchronous and finite by ABI contract. A missing response
is a failed proof, not a cancellation assertion. On failure the runner records
its stage and exits without issuing more native calls; it does not claim to
have safely cancelled arbitrary native execution. Normal completion unloads
the module before removing the callback and returning to MOS.

The proof requests diagnostic replies for review. Normal hot-loop invocation
can omit replies and keep results resident; proof traffic must not be presented
as the minimum production wire cost.

## Packed renderer integration, planned before implementation

Pack the actual agon-6502 Lode Runner snapshot's 280x192 HGR page into 6,720
MSB-first bytes. Record source and result identities. In mode 11, the separately
uploaded C module draws the image at physical (0,0). Assembly installs a silent
resident RUN frame and invokes it with the ordinary six-byte buffered call;
an INFO status query is a test-only completion fence. Preserve the existing
compressed-page architecture in the service offer; this fixture installs one
static image and does not claim to benchmark a running 6502 game.

Add script records for a bounded VSync wait and for patching the runtime module
handle into a resident command before installation. Wait for scanout before
finishing; verify actual decoded VGA against the source pixels. Keep review
labels and the MOS prompt below the image in a separate text viewport. The
human review contains only the native module drawing being reviewed.
