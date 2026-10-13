# Fab/Flexe experimental backend

The desktop uses an owned export of Fab Agon Emulator 1.2.5: its eZ80/MOS core,
host-backed SD, SDL window and audio device. A small optional interface makes
eZ80 VSync follow actual decoded firmware frames and closes the backend cleanly. The VDP is a new native adapter executing the
actual ESP32 firmware in Flexe, including native FreeRTOS and uploaded Xtensa
code. No module operation is replaced by a host C/C++ implementation.

## Build

From the Golem root, using the project Python environment:

```bash
bash docs/tasks/GOLEM-021/emulator/prepare_core.sh
bash docs/tasks/GOLEM-021/emulator/build.sh
bash docs/tasks/GOLEM-021/emulator/build_fab.sh
```

The source export is pinned to Flexe `fbda8e905ccff36a090e4e33b0ffc2337dd79c52`.
`prepare_core.py` records its small integration patches and hashes. Upstream
checkouts are never edited. Build products and captures stay under `build/`.

The portable Oryx path packages those verified source exports, transfers them
through the canonical SSH route and builds only this task's emulator files:

```bash
bash docs/tasks/GOLEM-021/emulator/stage_oryx.sh
```

It requires the existing project Python environment and native CMake/Cargo,
OpenSSL/zlib/SDL3 development tools. It uses the canonical local SDL3 prefix and
does not modify the consumer profile or launch an emulator. The resulting
`build/native-build.json` reports the two Oryx binary paths and hashes for root's
profile generator. `build_bundle.py` can create the portable archive separately;
`source-bundle.json` records its identity. No upstream checkout is needed on
Oryx because both pinned exports travel with the bundle.

The native Oryx build and both engines' direct media gates now pass. Retained
[build identity](oryx-native-build.json) and [qualification](oryx-qualification.json)
include x86_64 binary hashes, tool versions, all nine video modes, keyboard input
and DAC audio. Oryx required `libssl-dev` and `libsdl3-dev`; their ordinary package
dependencies were also installed. Root's separate consumer profile qualifies
the complete MOS/SD/module path: both engines pass 56 replies/560 bytes, all
61,440 packed-frame pixels and an observed MOS prompt. The profile is stopped
for human launch; no graphical review instance was launched by an agent.

The portable source bundle includes both upstream license files: Flexe's MIT
license and Fab's GPL version 3 text. The owned exports retain their original
source notices, and the source identity markers identify the modified files.

## Profile interface

The dedicated profile launcher selects the adapter with Fab's ordinary `--vdp`
option and supplies these environment variables:

| Variable | Meaning |
|---|---|
| `AGON_FLEXE_FIRMWARE` | Required absolute path to actual ESP32 firmware.bin. |
| `AGON_FLEXE_ELF` | Optional matching ELF, loaded separately for panic breakpoints; firmware compatibility hooks stay disabled. |
| `AGON_FLEXE_ENGINE` | `jit` (default) or `interp`. |
| `AGON_FLEXE_ARTIFACTS` | Existing directory for logs, final RGB PPM and stats. |

The profile must pin the adapter, firmware, matching ELF, newest tagged MOS and
MOS map. A missing selected input must fail before launch; never fall back to a
host-native stock VDP. Root owns profile generation and human Oryx staging.

The backend exports Fab's complete required VDP ABI, plus diagnostic
`flexe_vdp_frames()`, `flexe_vdp_cycles()`, `flexe_vdp_failed()` and
`flexe_inject_ascii(byte)` for bounded headless qualification. UART transcripts
are raw physical-direction bytes (`mos-to-vdp.bin`, `vdp-to-mos.bin`), separate
from ESP32 debug UART0 (`esp32-uart0.log`). `final.ppm` is decoded physical VGA,
and `stats.json` records instruction/cycle/frame counts and unresolved accesses.
`vdp-to-mos-delivered.bin` records the bytes actually removed by the eZ80 UART
frontend, allowing receive-queue problems to be distinguished from MOS parsing.

## Current limitations

- Keyboard characters enter the VDP's documented serial console through modeled
  UART0. Physical PS/2's ULP processor is not emulated. Ordinary letters, digits,
  punctuation, modifiers, Enter, Backspace, Tab, Escape and arrows have explicit mappings.
  Raw physical keyboard fidelity, mouse events and Fab `--osk` are not provided.
- VGA cropping supports standard 640x480, 640x240, 320x240, 320x200, 512x384,
  640x512, 640x256, 800x600 and 1024x768 modelines. Custom modes need their own
  timing description; no framebuffer or drawing kernel is read from guest memory.
- eZ80 VSync follows complete decoded frames matching the selected modeline;
  transient DMA sync edges during mode changes do not count. The frontend
  may coalesce frames when the host is busy; this is functional pacing, not a
  claim of cycle-exact cross-CPU timing.
- This remains a functional emulator, with physical hardware acceptance still
  required for timing, executable-heap capacity and hardware input behavior.

All emulator-related changes remain uncommitted until explicit human review.
The retained [qualification summary](qualification.json) identifies the source
patches/binaries and saves both engines' media and native-module receipts.

## Execution coherence

Linux ARM64 JIT code generation and branch chaining require host instruction-cache
maintenance. Clean upstream Flexe omitted it outside Apple Silicon and faulted
with SIGILL on this Pi; the owned export uses `__builtin___clear_cache` in both
paths. This correction is independent of vdplets.

Uploaded executable RAM requires guest predecode/JIT invalidation. The integration
observes architectural ISYNC, compares internal executable IRAM against its prior
image, and calls Flexe's existing invalidation API only when bytes changed. ISYNC
is preserved in the canonical interpreter, cached interpreter and JIT fallback
paths. This covers real module writes through ordinary stores, generated JIT
stores, ROM memcpy and RAM aliases; it does not replace module execution.

The diagnostic ELF is deliberately **not** passed as Flexe's session ELF: that
option installs firmware compatibility function hooks, including heap substitutes.
Our adapter independently reads symbols only to identify panic breakpoints.

CTS admission counts queued bytes plus modeled UART FIFO bytes against its real
128-byte capacity. An arbitrary lower cutoff can deadlock below the firmware's
120-byte FIFO interrupt threshold when RX timeout interrupts are disabled.
Classic ESP32 RX idle timeout now uses Flexe's existing deferred guest-clock
events and the configured baud/timeout registers. W1C rearms the timeout when
unread tail bytes remain. Clock gating and divider changes during an active RX
timeout are not qualified; those transitions do not currently reschedule it.

`check_tape.py` can drive the assembly consumer's unchanged transaction tape
through the same VDP ABI to diagnose loader execution before MOS integration is
complete. It exercises actual firmware and uploaded instructions, but does not
qualify the eZ80, host-backed SD card or desktop review.
