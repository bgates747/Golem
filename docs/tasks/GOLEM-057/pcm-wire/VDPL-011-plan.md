# VDPL-011 — Ordinary VDU access to the copied PCM sink

Let an eZ80 application stream copied U8 samples without loading a VDPlet.
The optional adapter reuses the small existing sink, retains stock MOS, and
keeps the larger audio scheduler out of minimal firmware. Native and wire
ownership must remain separate so unrelated module cleanup cannot stop a stream.

## Contract and release

GOLEM-057 PCM-001 implements CNS-003. The Author directly authorizes coordinated
implementation/testing in turn01a18ee7-60ae-7d20-953b-5a197cde2193 of Golem chat
01a12c79-f74c-74d2-b6da-de2e34eb1bdd. The6502 owner accepted the wire contract in
2026-10-30_18-54-04_agon-6502_golem057-pcm-contract-accepted-three. Read
[protocol1](../pcm-sink-wire.md) and `include/vdp_pcm_sink_wire.h` before changes.

1. Add opt-in `--pcm-wire` only to lean/pixels-lean. Preserve old selections,
   config bytes when absent, exact native ABI/default module images and ring.
   Existing runtime discovery then INFO25 reports0/1; audio16 is never blindly
   probed. Existing full queue15 stays independent and unchanged.
2. Exact15-byte header/<=8payload, explicit LE, token16/generation32; coherent
   state32 in four14-byte packet bodies (64UART bytes), ACK14 (16UART bytes).
   Stock MOS receive limit16 is mandatory. All complete malformed payloads drain
   boundedly; truncation has no effects. No callback into native module code.
3. Internal none/native/wire producer domain. Native failedRUN/INIT/unload cleanup
   closes only native-owned sink. Explicit stock channel/mixer cancellation
   remains global. Other domain requests get HANDLE without mutation/disclosure.
4. WRITE validates existing contiguous stock-buffer span, copies all orBUSY,
   drops source before ACK and retains no pointer. Counters, generation and
   controls match native core. OPEN requires unallocated stock channel.
5. Clients copy fragments inside interrupt callback, correlate/order strictly,
   preserve genuine keyboard/restore previous handler and cursor on exit. No
   blind WRITE retry. Known MOS transient last-key/keycount effects documented.

## Work and evidence

- PLN-001 [x] Register task, owner acceptance and frozen protocol before code.
- WIR-001 [x] Freeze header and independent byte vectors; release6502 client prep.
- OWN-001 [ ] Separate native/wire ownership in shared core; prove both takeover
  refusals and unrelated module cleanup preservation with actual adapter tests.
- ADP-001 [ ] Add bounded parser/serializer, guarded firmware integration and
  explicit discovery without changing no-option configuration/defaults.
- HST-001 [ ] Host/sanitizer request truncation/oversize/unknown/bounds/span,
  all-orBUSY/immediate poisoning, coherent fragments/token/order/key interleave.
- BLD-001 [ ] Build identical lean baseline and wire adapter; measure image,
  staticDRAM/IRAM and runtime allocation deltas with pinned configuration.
- TGT-001 [ ] Actual target native/wire parity, module-failure/unload preservation,
  global cancellation, source reuse, bounded memory and normal teardown.
- MOS-001 [ ] Coordinate module-free stock-MOS6502 consumer qualification under
  central Oryx lease; source/firmware/backend pins and keyboard cleanup evidence.
- HUM-001 [ ] Stage stopped useful human review; obtain emulator acceptance and
  explicit approval before committing coupled implementation/profile work.

No physical flashing, GUI launch or upstream mutation. Keep generic source and
artifacts here; Golem owns coordination,6502 owns its consumer synthesis/client.
