# Fab/Flexe emulator implementation plan

Use the official Fab SDL frontend, eZ80/MOS core and host-backed SD card with a
new shared-library VDP backend that executes actual ESP32 firmware in Flexe.
The adapter preserves Fab's existing VDP ABI; upstream checkouts stay read-only.
This is an experimental task artifact, requiring human Oryx review before commit.

- ARC-001 [x] Inspect both integration boundaries and select an architecture.
  Selected an ABI adapter rather than a replacement desktop UI or UART protocol.
- ARC-002 [x] Record supported video/input/audio behavior and limitations.
- ADP-001 [x] Implement the shared-library backend, physical UART queues, VGA
  decode, VSync signaling, audio samples and bounded shutdown.
- ADP-002 [x] Expose firmware selection and diagnostics through explicit runtime
  configuration; refuse missing firmware and avoid native-firmware fallback.
- ADP-003 [x] Build against an owned pinned Flexe export; reuse the baseline
  source identity and initial build, then own the required integration patches.
- CHK-001 [x] Qualify boot, MOS host-backed SD, serial-console keyboard, decoded
  VGA and audio without launching a graphical desktop instance.
  Stock peer gates and actual Fab MOS/SD numerical proof pass; the adapter's
  own keyboard, all nine advertised video geometries and audio ABI also pass.
- CHK-002 [x] Execute independently compiled vdplet payloads through the same
  backend when runtime firmware and root's assembly guest become available.
  Actual Fab assembly consumer and direct ABI gates pass all 44 checks,
  including three loads at one IRAM entry and changed replacement instructions.
- CHK-003 [x] Run the assembly consumer's unchanged transaction tape directly
  through the Fab VDP ABI to isolate native module execution from MOS startup.
  Keep this as diagnostic evidence; the final integrated gate still uses Fab.
- DEL-001 [x] Provide root with launcher/profile inputs, pixel/transcript captures,
  source identities and human-review instructions. Leave changes uncommitted.
  Root owns the final human-review instructions and canonical profile. The Oryx
  native paths/hashes and media evidence were delivered; the complete packed
  module fixture passes on both engines and is stopped for human launch.
- DEL-002 [x] Package pinned patched sources and a native Oryx build/staging
  script that touches only this emulator silo and leaves profile/review launch
  to root. Verify bundle contents locally while canonical SSH is unavailable.
  A local extraction verifies every file and both source markers without using
  either reference checkout. Native Oryx compilation remains in FAB-004.

## Interfaces agreed before implementation

Fab loads `flexe-vdp.so` with its ordinary `--vdp` option. The backend reads
`AGON_FLEXE_FIRMWARE` (required absolute firmware.bin), optional
`AGON_FLEXE_ELF` (diagnostic symbols only), `AGON_FLEXE_ENGINE=jit|interp`, and
an optional artifact directory. No guest application calls or memory patches
stand in for firmware behavior. Root owns the isolated profile generator and
assembly/SD fixtures. The baseline agent owns stock Flexe qualification.

The adapter exports every symbol required by Fab 1.2.5's VdpInterface, even
where a feature needs an explicit unsupported diagnostic. Physical PS/2 ULP
execution is not implemented by Flexe, so initial keyboard input enters the
firmware's supported serial console. Mouse/joystick fidelity is deferred unless
source inspection reveals a simple genuine hardware route.

## Execution-engine corrections discovered during implementation

- ENG-001 [x] Fix Linux ARM64 instruction-cache maintenance after JIT code
  generation and branch chaining. Clean upstream JIT faults with SIGILL in both
  the baseline runner and shared backend; interpreter already passes the ABI gate.
- ENG-002 [x] Model ISYNC as a native translation boundary with an optional
  synchronization observer. Compare internal executable RAM at that boundary,
  invalidate translated code only when changed, and cover unchanged/new/reloaded
  payloads. This catches ordinary/JIT stores and ROM memcpy without guest patches.
- ENG-003 [x] Retain clean-upstream failure and corrected interpreter/JIT receipts.
- ENG-004 [x] Match CTS admission to the modeled 128-byte RX FIFO capacity.
  The initial 96-byte cutoff deadlocked below firmware's 120-byte interrupt
  threshold; root's MOS debugger and the direct upload gate isolated it.
- ENG-005 [x] Schedule classic ESP32 UART idle timeout on the existing guest
  peripheral clock, including reassertion after W1C with unread bytes. Qualify
  tail-byte delivery with both execution engines; do not patch guest handlers.
  Real interpreter module upload/reload proof and stock interpreter/JIT MOS gates
  pass. Idle timeout now survives the ISR clearing TOUT with tail bytes pending.

## Small Fab frontend adaptation

- FAB-001 [x] Export official Fab 1.2.5 into the owned build silo and keep its
  eZ80/MOS/SD/SDL behavior, adding optional backend frame-counter discovery.
- FAB-002 [x] Pulse eZ80 VSync only after decoded firmware frames when the
  backend exposes that counter; ordinary native VDP backends retain prior behavior.
- FAB-003 [x] Add graceful SIGINT/SIGTERM shutdown and join the Flexe worker so
  saved traces, final pixels and counters are complete. Label the hybrid window.
- FAB-004 [x] Verify headless MOS/SD and stage the matching host build for root's
  Oryx profile without launching a graphical review.
  Oryx native compilation and both direct media gates pass. Root's full Fab/MOS
  packed proof passes both engines: 56 replies/560 bytes and all 61,440 frame
  pixels, with the actual MOS prompt observed. No graphical instance launched.

## Media qualification

- VID-001 [x] Exercise every advertised standard crop geometry with actual
  firmware mode changes and uniform-background/glyph pixel checks.
- VID-002 [x] Count only complete rasters matching the selected modeline for
  eZ80 VSync; exclude transient DMA edges during firmware mode changes.
- AUD-001 [x] Verify the Fab audio ABI returns the actual 1,024 Hz DAC tone,
  including sample amplitude/frequency and return to silence.
