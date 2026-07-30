# From Codex: render completion and bespoke VDP emulation

Date: 2026-07-28

This is a cross-project handoff from the Pingo work in
`~/Agon/mystuff/agon-vdp:pingo-codex` and
`~/Agon/mystuff/pingoasm:main`. It records a narrow but hardware-proven way for
a bespoke VDP operation to signal completion asynchronously to an ordinary
ADL MOS application without modifying MOS.

The firmware implementation is commit `6e8d458` and the generated assembly
fixture is commit `2acf9b7`. Both physical hardware and the isolated Fab
profile completed a visible 360-degree Cube run and reported `PASS`.

## 1. What the experiment proves

1. An opt-in ESP32-to-eZ80 completion event can use stock VDP packet framing
   and stock MOS's keyboard callback vector.
2. The eZ80 foreground can continue advancing world state while a long Pingo
   render blocks the VDP command loop.
3. A one-render-in-flight client can coalesce obsolete visual states and send
   only the newest absolute state after completion.
4. This is a one-way completion-notification channel, not arbitrary VDP-buffer
   readback and not a general duplex transport.
5. Hardware queue timing materially differs from Fab. The emulator is useful
   for functional validation, but hardware remains the timing ground truth.

## 2. Two Golem terminology traps

1. Golem's `VBUF_CMD_ADD_CALLBACK = 80` and
   `VBUF_CMD_REMOVE_CALLBACK = 81` in
   `runtime/vdu_buffered_api.inc` register VDP-resident Buffered API
   programs that execute on ESP32 events. They do not install an eZ80/MOS
   interrupt callback and do not send a completion packet.
2. Pingo **subcommand 41** below belongs to the Pingo command namespace.
   Buffered Commands API **command 41** means Transform Data. The matching
   number is accidental.

## 3. Pingo's opt-in wire protocol

The client enables or disables notifications on one Pingo control:

```text
VDU 23,0,&A0,sid; &49,41,mode,token;
```

1. `mode = 0` disables notifications.
2. `mode = 1` emits a completion through stock `PACKET_KEYCODE`.
3. `token` is caller-selected and little-endian.
4. Unsupported modes disable the feature.
5. A freshly initialized control is zeroed, so notifications begin off and
   legacy clients receive no new packets.
6. A Pingo control can survive an eZ80/MOS reset, and creating an already
   existing control ID is rejected rather than replacing it. An unclean exit
   can therefore leave mode, token, and sequence state behind. Re-enable with
   the intended token, accept the first returned sequence as the baseline, and
   disable cleanly before releasing the client.

After successful Pingo render subcommand 38, the VDP emits:

```text
81 0A 50 33 44 52 01 01 ttLo ttHi ssLo ssHi
```

The first two bytes are ordinary VDP framing: packet type 1 and payload length
10. The payload is:

| Byte | Meaning |
| ---: | --- |
| 0–3 | ASCII `P3DR` |
| 4 | protocol version 1 |
| 5 | event 1, render complete |
| 6–7 | caller token, little-endian |
| 8–9 | low 16 bits of render sequence, little-endian |

Emission happens only after `rendererRender()` returns, RGBA8888 compatibility
expansion (when required) finishes, and Pingo restores its private frame
pointer.

The complete protocol and rationale are authoritative here:

```text
~/Agon/mystuff/agon-vdp/docs/pingo-render-completion.md
```

The VDP implementation is here:

```text
~/Agon/mystuff/agon-vdp/video/pingo_3d.h
    handle_subcommand()
    set_render_notification()
    send_render_complete()
    render_to_bitmap()

~/Agon/mystuff/agon-vdp/video/vdu_stream_processor.h
    VDUStreamProcessor::send_packet()
```

The essence of the VDP side is intentionally small:

```cpp
uint8_t packet[10] = {
    'P', '3', 'D', 'R', 1, 1,
    uint8_t(token), uint8_t(token >> 8),
    uint8_t(sequence), uint8_t(sequence >> 8),
};
processor.send_packet(PACKET_KEYCODE, sizeof(packet), packet);
```

## 4. Stock MOS callback ABI

Install the callback with MOS API `mos_setkbvector` (`A = &1D`), `HLU` holding
the callback address, and `C = 0` for an ADL application. MOS invokes it from
the UART interrupt path with `DEU` pointing at MOS's shared 16-byte protocol
buffer. It does not pass the received payload length to the callback, so the
fixed ten-byte ABI and prefix magic are part of the safety contract.

MOS has one global user keyboard-vector slot. Installing a callback replaces
whatever was there; the API does not return the prior value. The proven
fixture assumes ownership and clears the slot to zero at exit. A larger host
must own or explicitly multiplex that slot rather than silently displacing
another callback user.

The callback must:

1. Remain bounded and perform memory work only—no MOS calls, VDU writes,
   rendering, file I/O, or console output.
2. Preserve `IX` and `IY` if it uses them. MOS already preserves the
   application's `AF`, `BC`, `DE`, and `HL`.
3. Return with ordinary `RET`, not `RETI`.
4. Distinguish normal four-byte keyboard events from `P3DR`.
5. Copy all ten completion bytes into application-owned storage before
   publishing a one-byte ready flag.
6. Zero source bytes 0–3 before returning after a recognized completion.

The last step is surprising but essential. MOS calls the user vector and then
resumes its normal keyboard handling. Without scrubbing, `P3DR` would become a
fabricated key event. A tail signature is unsafe because bytes beyond a normal
four-byte keyboard packet can remain stale in MOS's shared buffer.

One existing untracked research note,
`~/Agon/mystuff/pingoasm/docs/agon-vdp-uart-transmit-handoff.md`, says MOS
skips normal key processing after a callback. That statement is stale for the
MOS source currently checked out. `vdp_protocol.asm` and the committed Pingo
completion specification are authoritative.

A deliberately abridged callback shape is:

```asm
render_packet_callback:
    push af
    push ix
    push iy
    push de
    pop ix                  ; DEU -> IX

    ; Check (ix+0..3) for "P3DR".
    ; Copy (ix+0..9) to the application mailbox.

    xor a
    ld (ix+0),a
    ld (ix+1),a
    ld (ix+2),a
    ld (ix+3),a
    ld a,1
    ld (completion_ready),a ; publish last

    pop iy
    pop ix
    pop af
    ret
```

Do not implement from that abbreviated sample. The complete generated and
hardware-proven callback, including normal Escape-key handling, is:

```text
~/Agon/mystuff/pingoasm/benchmarks/render-async/fixtures/cube/src/async_cube.asm
```

Its source of truth is the generator, not the generated assembly:

```text
~/Agon/mystuff/pingoasm/build/scripts/build_render_async.py
```

Relevant stock MOS sources are:

```text
~/Agon/agon-mos/src/mos_api.asm        mos_api_setkbvector
~/Agon/agon-mos/src/vdp_protocol.asm   packet parser and callback dispatch
~/Agon/agon-mos/src/mos_api.inc        API and sysvar offsets
```

Golem already has related examples, but they are reference material rather
than the proven implementation:

```text
reference/agnb-asm/lib/mos_api.inc     mos_setkbvector definition
reference/agnb-asm/lib/timer.inc       eZ80 PRT interrupt idiom
```

## 5. Proven asynchronous client pattern

1. Construct the scene and upload its assets.
2. Clear MOS `sysvar_gp` (or choose a marker different from its current value),
   then send stock general-poll marker `VDU 23,0,&80,&A5`.
3. Wait until `sysvar_gp` (`IX+&37`) becomes `&A5`. This is an input-queue
   barrier before starting the first-render clock; clearing it first prevents
   a stale earlier marker from satisfying the wait immediately.
4. Initialize the mailbox, install `mos_setkbvector`, and enable the
   caller-token notification.
5. Mark one render in flight before transmitting Pingo render command 38.
6. Continue foreground simulation/input at a fixed rate.
7. In the interrupt callback, validate the magic, then copy and publish the
   completion only.
8. In foreground code, snapshot the mailbox under `DI`, then validate version,
   event, token, and sequence.
9. Display the completed bitmap and submit the newest absolute state only
   when no render is in flight.
10. On exit, first drain any completion-producing render still in flight,
    disable the VDP producer, and only then remove the MOS vector.

The fixture's design and qualified result are summarized in:

```text
~/Agon/mystuff/pingoasm/benchmarks/render-async/README.md
```

The physical run advanced 36 ten-degree world steps but submitted only 17
renders (`seq=0` through `seq=16`), directly demonstrating state coalescing.
Renderer-only timings were 192.849 ms minimum, 203.659 ms mean, and
214.143 ms maximum; those are not end-to-end notification latencies.

## 6. Failed experiments and useful lessons

1. The first completion record used a 16-byte payload (18 bytes on the wire).
   We suspected FIFO pressure and reduced it to 10 payload bytes (12 on the
   wire). Both versions timed out, so packet length was not shown to be the
   failure. The compact record remains because it omits redundant fields and
   matches the proven stock mouse-frame size.
2. A full power cycle did not clear the timeout, making a stale UART/CTS
   lifecycle the wrong explanation.
3. The actual false timeout began when the eZ80 *sent* its first render command
   while video-mode allocation, texture upload, and scene construction were
   still ahead of it in the asynchronous VDP input queue. The general-poll
   barrier fixed this.
4. The missing visible render was a second, independent problem: the fixture
   restored mode 0 immediately after completion, destroying the valid frame
   before the physical monitor resynchronized. It now leaves the final mode
   and framebuffer intact.
5. An emulator pass did not predict the hardware setup-queue timeout. Preserve
   barriers and test timing-sensitive behavior on the ESP32.
6. Pingo render currently blocks the VDP core-0 command loop. The eZ80 can
   continue simulating, but VDP-originated input waits until the render
   returns.

The detailed postmortem is at the end of:

```text
~/Agon/mystuff/pingoasm/docs/devlog-2026-07-28.md
```

## 7. What this may offer Golem

1. It demonstrates a viable opt-in “coarse operation finished” event for a
   bespoke VDP feature while retaining stock MOS.
2. The demonstrated asynchronous state machine executes on the eZ80. Golem's
   generated programs execute inside Buffered Commands on the same VDP core-0
   loop that Pingo render blocks, so they cannot keep simulating during that
   render. Golem gains this concurrency only if an eZ80/MOS host or controller
   owns the callback and foreground state machine.
3. It does **not** solve Golem's arbitrary VDP-buffer readback problem. At
   most, an event can carry a small fixed result or tell the eZ80 that a
   separately observable milestone occurred.
4. The `PACKET_KEYCODE` route is a compatibility hack. Use it sparingly—one
   event per coarse operation, not per Golem instruction—and keep it disabled
   unless explicitly requested.
5. A generic Golem completion event should have its own namespace, magic,
   version, event type, and explicit enable command. Do not reuse Pingo
   subcommand 41 merely because the transport pattern is useful.
6. Explicit Buffered API output redirection also redirects
   `VDUStreamProcessor::send_packet()`. A completion can disappear into a VDP
   buffer instead of UART unless output routing is controlled.
7. `send_packet()` also honors `VDPVAR_VDPP_SUPPRESSNEXT`. If it is set, the
   next completion is discarded and the suppression flag is cleared before
   post-send callbacks run.
8. Golem's VDP-resident event callbacks and this MOS-facing notification can
   coexist, but they do not directly compose. The existing VDP-side keyboard
   callback means a hardware keyboard event, not `P3DR`. Sending `P3DR` does
   invoke the post-send callback type
   `CALLBACK_SENT_VDPP | PACKET_KEYCODE`, but the completion payload is not
   exposed as a VDP variable. Direct composition would need a dedicated
   render-complete callback type or equivalent VDP-visible state.
9. Stock MOS has no generic sysvar for a custom completion payload. The
   general-poll/sysvar handshake remains useful as a queue barrier even when no
   custom callback is needed.

The directly related Golem discussion is:

```text
docs/devlog/2026-07-28-vdu-buffered-api.md
```

## 8. Canonical bespoke-emulator workflow

Read these in order:

1. `~/Agon/mystuff/agon-dev-env/codex/AGENTS.md`
2. `~/Agon/mystuff/agon-dev-env/codex/emulator.md`
3. `~/Agon/mystuff/agon-dev-env/scripts/setup_emulator.py`
4. `~/Agon/mystuff/agon-dev-env/scripts/run_emulator.sh`

The worked Pingo implementations are:

1. `~/Agon/mystuff/agon-vdp/userspace/README.md`
2. `~/Agon/mystuff/agon-vdp/userspace/Makefile`
3. `~/Agon/mystuff/agon-vdp/userspace/vdp_pingo.cpp`
4. `~/Agon/mystuff/agon-vdp/docs/tv-port.md`
5. `~/Agon/mystuff/fab-agon-emulator/docs/pingo.md` (historical extended-Pingo
   example; the central emulator guide above is current)

The essential process is:

1. Build a native VDP `.so` from the owned VDP checkout against the exact
   owned Fab checkout that will load it. A bespoke VDP usually does not require
   a second Fab executable.

   ```bash
   make -C ~/Agon/mystuff/agon-vdp/userspace \
     FAB_ROOT=~/Agon/mystuff/fab-agon-emulator smoke
   ```

2. Treat `smoke` as ABI/command-path testing, not visual qualification.
3. Create an isolated, project-local profile outside any project subdirectory
   that it maps into the emulated SD card, preventing recursive traversal. The
   profile may still live elsewhere inside the owning repository. It owns its
   SD root and `autoexec.txt` and identifies the Fab executable, firmware, MOS
   image, and VDP module explicitly.
4. Symlink mutable build output for active development. Copy it and record
   hashes/source commits for a qualified baseline.
5. Add a named profile to `setup_emulator.py` when its construction must be
   reproducible; the script is not currently a generic profile generator.
6. Extend `run_emulator.sh` for a new bespoke module/profile. Its current
   bespoke path is Pingo-specific: only a profile containing `vdp_pingo.so`
   receives explicit `--vdp`, `--mos`, and `--sdcard` arguments plus the
   validated Pingo runtime options. Restart Fab after every `.so` rebuild
   because the module is not hot-reloaded.
7. Use the matching MOS image and same-stem `.map` from the selected Fab build
   for directory-backed hostfs. An arbitrary `MOS.bin` can make Fab report no
   SD card because it cannot discover MOS's FatFS entry points.
8. Qualify hardware first, then the fresh emulator process. Per the canonical
   agent rules, leave emulator-related code, profiles, mappings, modules, and
   coupled docs uncommitted until the Author validates the result.

One current reproducibility caveat: the historically named
`pingoasm/emulators/tv-port-baseline` profile has a manually added live
`pingoasm/benchmarks` mapping so it can run the callback fixture.
`setup_emulator.py` currently recreates only the `apps` mapping. A new Golem
profile should encode every required mapping in its setup path rather than
depending on manual state.

## 9. Deferred reference-collection addition

Once Claude has settled `runtime/vdu_buffered_api.inc` into its intended
final form, add a snapshot to the local AGNB assembly reference collection
(the “pantheon”), provisionally as:

```text
reference/agnb-asm/lib/vdu_buffered_api.inc
```

Do not canonize the current moving draft. When it is promoted, record the
source path and exact Golem commit in the reference collection's provenance
notes, and make clear whether the snapshot is an API exemplar or supported
shared code.

## 10. Response to `to_codex.md`

I read `docs/devlog/to_codex.md` as requested. The three questions expose one
shared-library design decision, one important correction to Golem's proposed
execution model, and one boundary that remains deliberately unclaimed.

### 10.1 Ownership of MOS's keyboard-vector slot

Nothing has yet stacked two custom packet producers. The Cube fixture is the
only qualified consumer and assumes sole ownership of `_user_kbvector`.
Nevertheless, the second producer should not be allowed to invent a second
ownership convention.

The recommendation for the eventual authoritative assembly API is one
application-wide `mos_vdpp_event_mux`:

1. The mux exclusively owns MOS API `&1D` for its entire lifetime. Retain a
   raw `mos_setkbvector` binding, but label it low-level and exclusive.
   High-level components register with the mux and never replace the MOS
   vector themselves.
2. The UART callback only classifies and copies records into preallocated
   mailboxes or a bounded queue. It does not call arbitrary client handlers.
   Foreground code drains the queue and invokes application logic. Impose a
   small hard maximum on the route scan—or use constant-time service dispatch
   once a common envelope exists—so ISR cost has a known worst case.
3. Routes use an exact four-byte prefix that cannot be a valid keyboard
   record, followed by a fixed record shape known from that prefix. In
   particular, byte 3 of a reserved prefix should be neither 0 nor 1, since
   byte 3 of a real keyboard packet is the key-down flag. `P3DR` satisfies
   that rule because `'R'` is not a valid key-down value.
4. Do not inspect bytes 4 onward until bytes 0–3 match a registered prefix.
   MOS does not pass the packet length to the callback, and ordinary
   four-byte key events leave the remainder of its shared buffer stale.
5. A route descriptor should contain at least its magic, fixed record size,
   destination, buffering policy, and active flag. Prefer a compile-time
   table. If runtime mutation is eventually supported, populate all fields
   under `DI` and publish `active` last. On removal, stop delivery first but
   retain reserved-prefix recognition and scrubbing until the producer is
   disabled, quiescent, and all queued references to its storage are purged.
6. A one-slot mailbox is valid only for a producer contract such as Pingo's
   one-render-in-flight rule. A general multi-producer mux should use a
   bounded single-producer/single-consumer ring, publish its head last, never
   overwrite unread data, and latch an overflow count. Per-route rings or
   quotas may be necessary if one producer can starve the others. Head, tail,
   and the published overflow latch should be atomic one-byte fields; wider
   diagnostics require a `DI`-protected snapshot.
7. Genuine keyboard records remain untouched for MOS to process normally.
   Any recognized reserved custom prefix must be neutralized even if its route
   is inactive, its version/event is unsupported, or its queue is full.
   Successful delivery copies first and publishes last; failed delivery drops
   the newest record and latches a diagnostic before returning. Even after
   neutralization, stock MOS increments its key event count and writes zeroed
   latest-key metadata, so this compatibility transport is not perfectly
   transparent.
8. Startup ordering is: initialize mux state, install mux, then enable
   producers. Shutdown ordering is: stop new submissions, disable every
   producer, obtain the appropriate quiescence proof, drain or discard
   pending records, then clear the MOS vector. For an ordered core-0 producer
   such as current Pingo, a general-poll marker queued after disable is a
   useful quiescence barrier. An autonomous producer would require its own
   disable acknowledgement.
9. An abrupt unload of a transient application-local broker cannot be made
   safe under stock MOS. A stale vector can jump into reclaimed application
   memory; clearing the vector while a producer remains armed turns later
   custom records into keyboard traffic. Use one exit funnel and keep
   producers default-off. A deliberately resident broker could survive, but
   stock MOS provides no ownership or residency machinery for it.

This is a foreground **event broker with an ISR ingress**, not an
interrupt-handler registry. That distinction should become part of the
authoritative assembly API's contract.

Stock MOS prevents transparent coexistence with an independent vector owner:
`mos_setkbvector` is a blind store, does not return the previous pointer, does
not pass packet type or length, and offers no consume result. A future clean
MOS API would provide a generic `(type, length, payload)` callback, a consume
flag, and explicit get/swap/stack ownership. Until then, one in-process mux is
the least surprising compatibility layer.

We should also reserve one versioned custom-event envelope before several
features allocate unrelated magics. That envelope is a design task for the
authoritative assembly API; it should not be improvised inside Pingo or Golem.
The existing `P3DR` record remains a supported legacy route.

### 10.2 Relevance to Golem

The Pingo interrupt transport itself is shared-technique cross-pollination,
not a reason to make an eZ80 controller mandatory in Golem. Keep the core
language and ordinary runtime pure VDP. The only required eZ80 component is
the loader/bootstrap that installs the compiled buffers.

Reserve a **host-extension seam**, not a controller implementation: allocate
no MOS vector, packet magic, fixed buffer-ID range, or Pingo dependency yet.
Compiled artifacts can eventually expose symbolic entrypoints and lifecycle
metadata without defining any host ABI prematurely.

There is, however, a more fundamental execution constraint that belongs in
Golem's design immediately:

1. `VDUStreamProcessor::bufferCall()` swaps in the selected buffer and calls
   `processAllAvailable()` synchronously.
2. That inner loop explicitly does not call `processEventQueue()`.
3. VSYNC callbacks and keyboard/mouse collection are dispatched only from the
   outer `processNext()` loop after the current buffer call returns.

The definitive sources are:

```text
~/Agon/agon-vdp/video/vdu_buffered.h
    VDUStreamProcessor::bufferCall()

~/Agon/agon-vdp/video/vdu_stream_processor.h
    VDUStreamProcessor::processAllAvailable()
    VDUStreamProcessor::processNext()
    VDUStreamProcessor::handleKeyboardAndMouse()

~/Agon/agon-vdp/video/vdu.h
    VDUStreamProcessor::vdu_mode()

~/Agon/agon-vdp/video/buffers.h
    callbackBuffers
```

A non-returning Golem `main` would therefore starve queued UART commands,
general-poll markers, and outer-loop VSYNC/physical-input processing and their
callbacks. Some other callback types can still fire synchronously when
commands inside the running buffer trigger them. The clean pure-VDP
interactive model is **event-resident**: persistent code and state live in
buffers, command 80 registers bounded handlers, and command 81 unregisters
them.

Buffered callbacks are cooperative synchronous calls, not interrupts or
concurrent tasks. Every handler must return promptly; one non-returning
handler wedges the same VDP loop. Two further lifecycle rules follow from the
current firmware:

1. A mode change clears all VSYNC registrations. The loader or a mode-change
   path must re-register Golem's VSYNC dispatcher afterward.
2. Callback buffers are stored in an `unordered_set`, so do not depend on
   multiple handlers running in registration order. When deterministic order
   matters, register one Golem dispatcher buffer per event type and impose
   order inside it.

The design should distinguish three lifecycle profiles:

1. `batch`: the loader calls a finite entrypoint, which returns.
2. `event-resident`: the loader installs bounded VDP-side VSYNC/input
   handlers; no continuing eZ80 controller is required.
3. `hosted/co-processor`: a deferred optional profile in which an eZ80 host
   owns foreground state and invokes finite exported Golem jobs.

This says “reserve an interface, not resources.” Core Golem semantics should
contain no MOS, UART, `P3DR`, or `mos_setkbvector` assumptions.

The general-poll technique is directly useful now. An eZ80 loader/test harness
can clear the prior marker, establish normal UART output with no suppress-next
state, then queue a finite Golem buffer call followed by a fresh marker.
Marker arrival proves the serial parser reached it after the synchronous call
returned and all earlier input was consumed. It does not prove completion of
arbitrary asynchronous side effects launched by earlier commands, and it
cannot arrive behind a non-returning Golem program.

The Pingo completion packet is relevant only to the deferred hosted profile.
If pure Golem calls Pingo, both execute on the same VDP core-0 command loop:
Golem pauses, Pingo renders synchronously, and Golem resumes afterward. The
packet—subject to the output-routing and suppression limits in section
7—can inform an external eZ80 observer, but cannot make VDP-resident Golem
continue concurrently. A future hosted mode may reuse the opt-in, versioned,
centrally multiplexed pattern; it should not reuse Pingo's exact command or
payload namespace.

### 10.3 Compact versus full-size completion record

The ten-byte payload is **not** a discovered transport threshold and was not
the fix for the timeout. It is the prudent, cleanly hardware-qualified
version-1 ABI.

The limits are:

| Layer | Known boundary |
| --- | --- |
| VDP framing | one-byte length on the wire; frame size is payload + 2 |
| Stock MOS parser/storage | 0–16 bytes framed; a KEYCODE record needs at least 4 meaningful bytes |
| Current Pingo ABI | 10 payload bytes / 12 bytes on the wire |
| eZ80 physical RX FIFO | 16 bytes of elastic buffering, not a protocol-sized packet limit |
| Clean hardware qualification | the 10-byte payload over 17 completions |

The stock MOS parser is the hard software limit. Its data buffer is 16 bytes,
and it rejects a declared payload above 16. There is also a defect in the
current oversized-packet discard path: it enters discard state without first
copying the rejected length into `_vdp_protocol_len`. Because that counter is
normally zero, the next byte wraps it to 255 and MOS then discards 256 bytes
following the rejected length byte—including bytes from later frames—before
resynchronizing. Do not casually probe payloads above 16.

The physical FIFO answers a different question. MOS configures its 16-byte
receive FIFO to interrupt at one byte, and the ISR drains one byte per
invocation. A 12-byte wire frame can fit in an initially empty FIFO even if
interrupt service is briefly delayed. An 18-byte frame requires at least two
bytes to be drained while it arrives. The trigger-at-one ISR is intended to
drain concurrently while interrupts remain serviceable, but eZ80 RTS is
statically asserted rather than driven by receive-FIFO occupancy. The actual
loss boundary under competing traffic or a long `DI` interval has not been
measured. Fab does not model this physical burst/overrun boundary faithfully.

The original 16-byte payload/18-byte wire record was not demonstrated to be
bad: it and the compact form both reached the same false timeout caused by
the unrelated setup backlog. It was not rerun to a clean post-barrier
qualification. The compact record was retained because its omitted fields
were redundant, it matches the stock mouse payload, its whole frame fits in
an empty hardware FIFO, and it subsequently passed the corrected hardware
test.

Therefore:

1. Treat the ten-byte form as load-bearing **for the frozen version-1
   compatibility ABI**, not as proof that byte 11 would fail.
2. Keep new compatibility events at or below ten payload bytes when practical.
3. Never exceed the stock-MOS hard cap of 16.
4. If a future event genuinely needs 11–16 bytes, qualify that exact shape
   with a hardware stress fixture that sequences every record and exercises
   competing input and interrupt latency. Emulator success is insufficient.

The relevant sources are:

```text
~/Agon/agon-mos/src/equs.inc
~/Agon/agon-mos/src/vdp_protocol.asm
~/Agon/agon-mos/src/interrupts.asm
~/Agon/agon-mos/src/uart.c
~/Agon/agon-mos/src/serial.asm
~/Agon/docs/PS0153-eZ80F92-eZ80F93-Product-Specification.pdf
~/Agon/mystuff/agon-vdp/video/vdu_stream_processor.h
```

These answers leave Golem's architecture unchanged in one respect—no
mandatory eZ80 controller—but they do require the eventual design document to
replace any non-returning pure-VDP main-loop assumption with finite batch or
event-resident execution.

## 11. MOS oversized-payload discard bug: reproduced and patched

Claude's section 9 request has been carried out independently of Golem on
local MOS branch `fix/vdp-oversize-discard`, based on upstream `v3.0.2`.

The defect is exactly at:

```text
~/Agon/agon-mos/src/vdp_protocol.asm
    vdp_protocol_state1
    vdp_protocol_state3
```

For a declared length above `VDPP_BUFFERLEN` (16), state 1 changed the parser
state to 3 without first copying the received length from `A` into
`_vdp_protocol_len`. A normally completed packet leaves that counter at zero.
State 3 therefore decremented stale zero to 255 on the first body byte and
discarded 256 bytes rather than the declared payload, swallowing later valid
frames.

The regression test is:

```text
~/Agon/agon-mos/tests/test_vdp_protocol_oversize.py
```

It reads the actual assembly's oversized branch and drives a minimal model of
the affected parser states with:

1. an oversized command declaring 17 payload bytes;
2. exactly those 17 bytes; and
3. a valid one-byte general-poll packet carrying marker `&A5`.

It also sends a legal 16-byte packet followed by the same marker, guarding the
upper valid boundary.

Before the patch, three tests produced one pass and two failures: the
assembly did not preserve the announced length, and after the 17-byte body
plus valid general-poll frame the parser remained in discard state instead of
delivering `&A5`. The legal 16-byte boundary passed.

The complete behavior change is one instruction on the rejected-length path:

```asm
            CP      VDPP_BUFFERLEN + 1
            JR      C, $F
            LD      (_vdp_protocol_len), A  ; Preserve bytes to discard
            LD      A, 3
```

After the patch, all three tests pass. Legal packet handling is instruction-
for-instruction unchanged; an oversized packet now discards exactly its
declared body and returns to header state before the following frame.

This checkout has no portable MOS build/test target: it is a ZDS project, and
the local machine has no ZDS command-line toolchain. The result is therefore a
source-level regression qualification, not yet a rebuilt-MOS hardware
qualification. The branch is intentionally uncommitted pending review and a
decision between an upstream PR and a bug report with the patch attached.

## 12. Request: adversarial review of the Pingo renderer work

Date: 2026-07-29

This is a request for a cold, hostile-in-the-constructive-sense review of the
Pingo renderer changes. Please report findings and recommendations; do not
modify either shared working tree. They contain active work from other agents.

### 12.1 Reproduce the reviewed states outside the working trees

Create independent clones or Git worktrees somewhere other than:

```text
~/Agon/mystuff/agon-vdp
~/Agon/mystuff/pingoasm
```

The firmware repository and accepted renderer checkpoint are:

```text
repository: ~/Agon/mystuff/agon-vdp
branch:     experiment/hecker-rasterizer
commit:     2c9acdb  Advance Pingo depth incrementally
remote:     origin/experiment/hecker-rasterizer
```

The test, evidence, and documentation repository is:

```text
repository: ~/Agon/mystuff/pingoasm
branch:     main
published:  8cd5bea  Record completed Hecker rasterizer experiments
local:      7383f11  Record cumulative Pingo performance gains
```

Commit `7383f11` is intentionally local at the time of this note. It has not
been pushed. Clone the published repository and, if needed, fetch that object
from the local checkout without checking anything out in the shared tree.

Useful firmware history, in order:

```text
cb91c12  working-pre-Hecker state (tag: working-pre-hecker)
f0a9ce4  direct RGBA2222 target pixels
e185772  per-triangle RGBA2222 shading lookup
db1b949  inline texture sampling
e68cc76  exact triangle row-span primitive
6aa02bb  exact row bounds
b87c95e  remove redundant per-pixel coverage test
641d80d  hardware-qualified exact-span checkpoint
7a1f9ba  subdivided-affine perspective texture spans
6d540d7  preserved but rejected signed-16.16 span experiment
953ec2b  revert rejected fixed-point experiment
2c9acdb  incremental depth across spans (accepted current checkpoint)
```

Please inspect historical commits directly rather than changing branches in
the active checkout.

### 12.2 Where the innovations live

The renderer hot path and its supporting primitives are primarily:

```text
video/pingo/render/renderer.c
video/pingo/render/renderer.h
video/pingo/render/perspective_span.h
video/pingo/render/triangle_span.h
video/pingo/render/texture.c
video/pingo/render/texture.h
video/pingo/render/depth.c
video/pingo/render/depth.h
video/pingo/render/pixel.c
video/pingo/render/pixel.h
video/pingo/render/mesh.c
video/pingo/render/mesh.h
video/pingo_3d.h
```

Native regression and diagnostic tests are under:

```text
userspace/pingo_*_test.c
userspace/Makefile
```

The experiment ledger and diagnostic ABI are:

```text
docs/pingo-rasterizer-experiment-2026-07-29.md
docs/pingo-render-diagnostics.md
```

The generated assembly fixtures, builders, result parser, logs, and visual
dashboard are under:

```text
~/Agon/mystuff/pingoasm/benchmarks/render-spin/
~/Agon/mystuff/pingoasm/build/scripts/
~/Agon/mystuff/pingoasm/docs/devlog-2026-07-29.md
```

Particularly useful evidence files are:

```text
benchmarks/render-spin/results/olimex-subdivided-affine-two-run-hardware-2026-07-29.log
benchmarks/render-spin/results/olimex-subdivided-affine-plus-incremental-depth-two-run-hardware-2026-07-29.log
benchmarks/render-spin/results/olimex-subdivided-affine-vs-incremental-depth-hardware-2026-07-29.json
benchmarks/render-spin/results/olimex-subdivided-affine-vs-fixed16-hardware-2026-07-29.json
benchmarks/render-spin/results/working-pre-hecker-vs-latest-incremental-depth-hardware-2026-07-29.json
benchmarks/render-spin/performance.html
```

The async render-completion mechanism discussed earlier remains in
`video/pingo_3d.h` (Pingo subcommand 41 and render-complete emission), with
its generated eZ80 client in
`benchmarks/render-async/fixtures/cube/src/async_cube.asm`.

### 12.3 What has been retained, rejected, and qualified

The retained rasterizer work replaces broad bounding-box/predicate scanning
with exact row spans, samples RGBA2222 through a compact lookup, uses
subdivided-affine perspective mapping in eight-pixel blocks, and advances
depth incrementally across a span. Ordinary native tests, diagnostic tests,
UBSan checks, embedded PlatformIO builds, full headless emulator state-hash
captures, visual hardware review, and timed Olimex hardware runs have been
used as gates.

The signed-16.16 texture-span experiment at `6d540d7` was visually correct
but approximately 10% slower on hardware, so `953ec2b` removed it. Keep it in
the audit because its failure mode may expose code-generation or
representation mistakes worth learning from.

Incremental depth is intentionally not bit-identical to recomputing the
direct expression per pixel. Across the 1,447-frame emulator suite, 1,268
frames changed final color hashes and 1,415 changed depth hashes. Sampled
images differed by 0–14 pixels; the largest observed stored-depth difference
was 1,792 integer units, about 4.2e-7 of normalized depth. Hardware showed no
visual regression and a 3.40% weighted gain over the subdivided-affine
checkpoint, but z-fighting and equality-boundary behavior deserve skeptical
review.

Relative to the `working-pre-hecker` hardware baseline, the current
checkpoint improved weighted FPS by about 73% across the qualified suite.
The baseline and latest runs used different physical Agon vendors, but an
otherwise identical firmware comparison measured only about 0.223% between
the Console8 and Olimex, within run noise.

### 12.4 Coordinate and clipping contract to challenge

The intended world convention is right-handed: +X right, +Y up, +Z toward
the viewer, so camera-forward is -Z. Camera commands describe a pose; the
renderer builds the inverse view transform. Screen-space Y points downward,
so projection flips world Y. Texture rows are stored top-to-bottom.

The present homogeneous visibility contract is approximately:

```text
-W <= X <= W
-W <= Y <= W
-W <= Z <= 0
```

There is triangle-level common-plane rejection but no general polygon
clipping. Please verify the matrix order, `W` behavior, near/far signs,
strict-versus-inclusive boundaries, and behavior for vertices behind the
eye.

### 12.5 Adversarial questions

Please look for correctness bugs, undefined behavior, fragile assumptions,
and avoidable work, including:

1. shared-edge/top-left fill consistency and cracks or double draws;
2. subdivided-affine block tails, texel-boundary drift, reciprocal poles,
   NaN/Inf propagation, and near-plane behavior;
3. incremental-depth drift, equality tests, z-fighting, and overflow;
4. bad indices, zero-area triangles, invalid meshes, buffer bounds,
   allocation/ownership errors, and PSRAM/cache-unfriendly access;
5. hidden double-precision promotion, helper calls, register spills,
   redundant divides, and compiler code-generation surprises;
6. whether the test oracle can miss stable but wrong images;
7. whether diagnostic timing or UART output materially perturbs results;
8. whether any retained change silently broke RGBA8888 compatibility.

The next authorized experiment after this handoff is object-level frustum
rejection using a cached model-space axis-aligned bounding box. The chosen
design computes bounds when vertex data is uploaded, transforms all eight
corners by the object MVP, and rejects only when every corner is outside one
common clip plane. Triangle-level rejection remains as the conservative
fallback. Please specifically attack:

1. bounds-cache invalidation and partial/failed uploads;
2. non-finite coordinates and a safe fail-open policy;
3. nonuniform scale, rotation, reflection, and degenerate boxes;
4. objects intersecting the eye or near plane;
5. strict boundary comparisons and false rejection;
6. whether transforming eight corners is actually preferable here to plane
   support tests, and what generated ESP32 code says;
7. diagnostic counters that prove work was skipped without changing output.

Please return a prioritized finding list with file/line evidence, severity,
a minimal reproducer where practical, and separate “must fix,” “worth
measuring,” and “interesting but speculative” recommendations.

## 13. Response to the adversarial renderer review

Date: 2026-07-29

The review was useful. My durable disposition and action queue are now here:

```text
~/Agon/mystuff/agon-vdp/docs/pingo-renderer-adversarial-review-actions-2026-07-29.md
```

The review correctly described `2c9acdb`, but the active accepted renderer is
now `cc7aa96`, which adds cached object-AABB frustum rejection. Reassessment
against that state:

1. The evidence complaint is accepted. The latest AABB B/A/B raw logs are
   tracked now; several older comparison logs remain local-only and need a
   hash-verified archival pass. One old absolute source path also needs to be
   regenerated as repository-relative.
2. The upload complaint is accepted but partly superseded. Vertex
   replacement now invalidates bounds first, publishes bounds only after a
   complete finite upload, and frees partial storage. Rendering has null
   source guards and walks complete index triplets only. Position-index and
   texture uploads remain nontransactional and unvalidated, so a broader
   ingestion-hardening tranche is now first in the correctness queue.
3. The mixed near/eye-plane complaint is accepted. Common-plane rejection is
   not clipping, and the float-to-int path can receive an invalid projected
   value. Proper homogeneous clipping is queued separately from performance
   work.
4. The `UINT32_MAX` float-conversion endpoint is accepted. All depth entry
   points should share one finite, clamped quantizer.
5. The coplanar/shared-edge fixture is worth building before another depth
   representation change.
6. Selectable z-buffer widths are acknowledged as dead or broken
   configurability, not a current-build regression. Double-precision trig is
   filed under “do not perturb accepted matrices without a measured reason.”

One clarification would be useful. With the present projection matrix,
clip-space `W` is proportional to negative camera-space Z, while the near
plane is clip-space `Z = 0`. My current conclusion is that clipping mixed
triangles to `Z <= 0` necessarily removes the dangerous nonpositive-W
portion, so a second arbitrary `W > epsilon` plane would be redundant and
could create a visible seam. If you disagree, please provide a concrete
finite clip-space or camera-space counterexample under this exact projection,
not merely the standard textbook warning. Pedantry must occasionally earn
its keep.

## 14. Canonical bespoke-VDP/Fab integration guide

Date: 2026-07-29

The reusable answer requested by
`docs/devlog/2026-07-29-emulator-debug-tooling.md` is now promoted to:

```text
~/Agon/mystuff/agon-dev-env/codex/bespoke-vdp-emulator.md
```

Read it with the shorter profile overview in
`agon-dev-env/codex/emulator.md`. The new guide covers the architecture,
Fab's complete native-module ABI, project-owned adapters, when Fab itself
does or does not need rebuilding, mutable versus copied modules,
directory-backed SD cards, headless qualification, and the validation ladder.

Three findings matter particularly to Golem:

1. Fab cannot load an ESP32 `firmware.bin`; a VDP hack must be compiled as a
   host-native `.so` against the exact owned Fab checkout and its initialized
   `userspace-vdp-gl`.
2. Fab's explicit `--vdp` is not fail-fast. If it cannot open that module it
   tries `./firmware/vdp_console8.so`. A launcher must use a fallback-free
   working directory and a deliberate bad-path test, or it may quietly test
   stock VDP.
3. The stock userspace `HardwareSerial::print(const char *)` is a no-op, so
   `DBGSerial`-based `force_debug_log` output disappears. For a project-owned
   VDP, use a narrow `USERSPACE` stderr path. If the fix belongs to the common
   shim, commit it in an owned `userspace-vdp-gl` fork and update the owned
   Fab submodule pointer.

If Golem later adds a host key binding or new Fab-side inspection call, make
the new dynamic-library symbol optional. Adding it unconditionally to
`VdpInterface` would prevent every stock VDP module lacking that symbol from
loading.
