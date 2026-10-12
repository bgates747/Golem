# GOLEM-002 — Existing completion hooks and a possible vdplet return channel

Pingo Earth Party and Wolf3DOrig provide concrete examples of custom VDP firmware
sending completion records back to MOS for pacing. Reuse their small-message,
interrupt-mailbox and foreground-validation pattern when researching a **vdplet**
(the Author’s working name, consistent with MOSlet). These examples establish a
working custom-firmware return route; they do not establish a stock-firmware
native loader or an exported service callable by runtime-loaded Xtensa code.
This is a source review dated 2026-10-11, not a new runtime qualification.

## Pingo worked example

The Author called this the Pingo World Party demo. The maintained example found
locally is [Earth Party textured](/Agon/mystuff/pingoasm/apps/earth-party-tex/src/earth-party.asm),
with its callback in [render-async.inc](/Agon/mystuff/pingoasm/apps/earth-party-tex/src/render-async.inc).

1. EP-001: Scene setup is fenced with a general-poll response before notifications
   and the first render. The program installs `mos_setkbvector`, enables Pingo
   notification subcommand 41 (decimal), and publishes its in-flight flag before
   transmitting the render. The enable bytes are `23,0,0xA0,sid:u16,0x49,41,1,token:u16`;
   this is a custom Pingo extension, not stock buffered subcommand 41.
2. EP-002: Firmware sends ten payload bytes: ASCII `P3DR`, version, event,
   little-endian registration token and little-endian low 16 bits of the render
   sequence. [Pingo producer](/Agon/mystuff/agon-vdp-pingo-v216-promotion/video/pingo_3d.h:608)
   uses `send_packet(PACKET_KEYCODE, 10, packet)`. Framing adds code `0x81` and
   length `10`, giving **12 return-wire bytes per completion**. The producer
   deliberately keeps this below the eZ80’s 16-byte UART FIFO. That size choice
   is evidence for this message, not a guarantee that arbitrary larger replies work.
3. EP-003: The MOS UART callback recognizes the signature, copies the payload to
   a mailbox and publishes a ready flag. It changes the received first four bytes
   into a null key-up record because ordinary MOS keyboard processing follows.
   Other keyboard records continue normally. It performs no MOS or VDU calls.
4. EP-004: Foreground code snapshots the mailbox with interrupts disabled briefly,
   then validates version 1, event 1, token and sequence. It accepts the first
   observed sequence and requires consecutive 16-bit values thereafter, including
   wraparound. Only a valid completion releases the in-flight state and permits
   display/flip and submission of the next render. This is render completion,
   not evidence of a physical display/VSYNC acknowledgement.
5. EP-005: There is only one render in flight. On exit the program drains that
   render, disables the producer, then removes the callback before returning to
   MOS. Invalid records retain the in-flight state and request exit; this is not
   a bounded recovery mechanism. A missing or rejected completion can leave it
   waiting. Do not silently promote that behavior into a general module protocol.

## Wolf corroboration

[Wolf firmware](/Agon/mystuff/agon-vdp-wolf3d/video/wolf3d.h:555)
explicitly mirrors Pingo notification setup and sends a ten-byte `W3DR` payload
through the same `PACKET_KEYCODE` route. It distinguishes render completion from
VDP-owned fizzle completion. Render completion follows renderer/HUD queue drain;
foreground application code owns presentation and the next submission.
[Wolf receiver](/Agon/mystuff/Wolf3dOrig/src/asm/wolf3d/src/wolf3d_render_async.inc)
installs `mos_setkbvector`, routes records into mailboxes and neutralizes their
keyboard interpretation. This confirms the Author’s recollection of both projects.

The [packet writer](/Agon/mystuff/agon-vdp-wolf3d/video/vdu_stream_processor.h:519)
writes code-plus-0x80, one length byte and payload. It can suppress a packet through
`VDPVAR_VDPP_SUPPRESSNEXT`; delivery policy and stream ownership therefore need
explicit treatment in a reusable return service. The MOS parser itself was not
audited here: accepted lengths, callback contract and coexistence with keyboard
traffic must be checked against the actual MOS version before extending payloads.

## Implications for vdplet research

1. VD-001: Treat loading, invocation and return transport as separate facilities.
   Existing C++ renderers are compiled into custom firmware. Their callbacks do
   not prove that stock firmware can load a vdplet or export `send_packet` to it.
   Avoiding an upstream merge is the desired deployment outcome, still to establish.
2. VD-002: Consider a versioned host C service for bounded completion/status
   messages, rather than importing private VDP C++ objects or addresses. Correlate
   requests using registration/job tokens and sequence numbers; distinguish
   completion, numeric result and error events. Specify formats and ownership.
   This is a candidate ABI direction, not an adopted wire format.
3. VD-003: Retain resident results and error handling by default. A tiny completion
   or status packet can pace the host without streaming full numeric buffers back.
   Meaningful scalar/status replies could be optional when needed. Count setup,
   requests, replies and any result retrieval separately when comparing wire costs.
4. VD-004: Preserve interrupt-only copying and foreground processing, bounded
   mailboxes, clear backpressure and producer-before-consumer teardown. Audit
   loss, duplicate/stale records, sequence wrap, cancellation and safe unloading.
   A host wait timeout does not establish that native code has stopped executing.
5. VD-005: Bounds checks and their handling must work wholly on the VDP with stock
   WOM operation; an optional return channel may report an outcome later. The
   earlier discussion of preserving the destination and invoking a resident
   handler remains a proposal for semantic review, not an implemented policy.

## Source identities and limits

1. REF-001: PingoASM HEAD `d7399ee6c58f2be32a52d9dc443cc0f9e1e9a4d8`;
   unrelated working changes exist; the two inspected Earth Party files are clean.
2. REF-002: Pingo promotion firmware HEAD
   `6e6e15f9a90bcfecebc214d4c91151b38c607961`, clean checkout.
   The older v216-userspace checkout lacks the notification producer; it must
   not be used as evidence for this feature.
3. REF-003: Wolf application HEAD `30927b2a982ee5569b6c9fd159e5dc3b246230f5`;
   working changes exist, including main application code. The inspected async
   include is clean. Wolf firmware HEAD
   `1dc462ed0e0b681235ad01eaf49a9a8f763e9d10`, clean checkout.
4. REF-004: The [combined-firmware coordination record](/Agon/mystuff/agon-dev-env/cross-agent/wolf_pingo_vdp/README.md)
   records prior interactive and hardware acceptance and combined commit
   `3c07e75`. That is historical qualification evidence, not a test rerun here.
   The current `/Agon/mystuff/agon-vdp` working branch is `audio_fix`, so this
   review used the explicit Pingo promotion and Wolf firmware sources above.

Broader loader/toolchain feasibility, stock integration and a stable return ABI
remain GOLEM-002 research. No firmware, application or emulator was changed.
