# Flexe — an Xtensa execution route for vdplet testing

Flexe changes the available test options: it emulates the original ESP32 CPU and
already includes an Agon VDP/MOS integration fixture. It is a candidate for running
the actual Xtensa loader firmware and uploaded vdplet before hardware. Fab's
ordinary host-native VDP limitation still holds. This review inspected source
only; it establishes neither our loader compatibility nor a new passing run.

The Author supplied the MOS-peer source on 2026-10-12. The local checkout is
`/Agon/others/levkropp/flexe`, clean at
`fbda8e905ccff36a090e4e33b0ffc2337dd79c52`. It remains read-only.
Hashes of the reviewed files are in [local-source-identities.json](local-source-identities.json).

## What the linked tool does

[agon-mos-peer/main.rs](https://github.com/levkropp/flexe/blob/fbda8e905ccff36a090e4e33b0ffc2337dd79c52/tools/agon-mos-peer/src/main.rs)
hosts Fab's pinned eZ80 core in a separate process. It loads a MOS image and an SD
directory, listens on loopback TCP, and connects UART bytes, CTS and VSync to the
Flexe process. It does not supply a substitute VDP. Cargo pins the upstream
`agon-ez80-emulator` dependency to `02a23e67049cd68d5da77f0776cb375fa91a5e4a`.

The socket protocol uses tag/u16-length framing, with tags for UART, VSync, CTS
and shutdown. Frames are capped at 256 payload bytes. This is **host test plumbing**,
not proposed Agon UART framing or a vdplet protocol. Do not count TCP headers as
physical eZ80↔VDP wire traffic.

## What the wider project adds

Flexe's Agon compatibility record describes running pinned VDP 2.16.0 firmware
with its native FreeRTOS, firmware VDU handling, modeled UART and decoded I2S VGA
scanout. Its existing scenarios cover video, a DAC tone and serial-console input
in interpreter and JIT engines. They are upstream results, not tests repeated
here. The documented image differs from our VDP-003 custom firmware.
[Compatibility record](https://github.com/levkropp/flexe/blob/fbda8e905ccff36a090e4e33b0ffc2337dd79c52/docs/compatibility.md#agon-light--fabgl-end-to-end-regression).

The full-machine script pins MOS 3.0.2 and BBC BASIC alongside that VDP image.
Commands enter via the VDP serial console; decoded VSync reaches the eZ80 peer.
It checks shell/file/BASIC behavior and final pixels, retaining logs/captures.
Its stock-image hash gate deliberately rejects another firmware image. A vdplet
fixture would need its own explicit identities and expectations, rather than
weakening that gate. The runner also has a release-specific diagnostic breakpoint
that must be reviewed for any custom firmware.
[Test description](https://github.com/levkropp/flexe/blob/fbda8e905ccff36a090e4e33b0ffc2337dd79c52/docs/testing.md#agon-mos-and-bbc-basic).

## Implications for GOLEM-002

- FLX-001: Consider a bounded Flexe preflight before physical flashing: first
  reproduce its pinned Agon baseline, then run our separately identified loader
  firmware with a tiny module uploaded after boot. This remains a scope choice
  at Gate A, not an emulator setup/run authorized by this resource review.
- FLX-002: The same **Xtensa payload** could exercise relocation, register-window
  calls, literal/data access and resident results. That would be materially
  different evidence from compiling a separate ARM/x86 shared-library kernel.
  Compatibility with our SDK, allocations and module remains unverified.
- FLX-003: Include unload/reload with changed code and placement in interpreter
  and JIT modes. Dynamically written executable RAM and translation invalidation
  must be demonstrated; running stock firmware does not establish those cases.
- FLX-004: Keep physical acceptance for executable-memory headroom, timing and
  real rendering/input. Flexe's documented functional model has timing limits;
  physical PS/2 is unsupported because the ULP FSM is not executed. Its MOS peer
  uses host-backed SD access. No new hardware equivalence claim follows.
- FLX-005: This may reduce the need for a separate host-native vdplet emulator
  scheme. It does not change the C ABI, assembly ownership or initial numeric
  proof, and does not authorize a large emulator integration project.

No build, Cargo dependency fetch, test execution, emulator launch or upstream
modification occurred. GOLEM-002 remains stopped at Gate A.
