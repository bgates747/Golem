# Ordinary-VDU copied PCM — protocol1

Expose the existing small copied-sample sink through an optional audio command,
using the same ring, rate, counters and cancellation rules as native callers.
Stock MOS accepts at most **16 payload bytes per VDP reply packet**. The proposed
state response therefore uses four small packets: one coherent snapshot, exactly
64 bytes including UART packet framing. Requests need at most23 bytes including
the VDU introducer. This protocol is frozen for the first implementation, accepted by the6502 owner on2026-10-30. Audio16 and INFO25 are local experimental allocations, not upstream reservations. Optional support remains explicitly discoverable.

## Scope and local allocations

1. Keep the current native sink, complete queue scheduler and default firmware
   selections intact. Add an opt-in `--pcm-wire` adapter to `lean` and
   `pixels-lean`; reject it for selections without the copied sink. No second
   ring, worker, sample scheduler, resampler or callback into uploaded code.
2. Command: `VDU 23,0,&85,channel,16,...`. Stock uses audio0..14;
   the retained full queue candidate uses15. Command16 is absent from the
   examined pinned source. This is not a claim of an upstream allocation.
3. Presence must be queried before sending command16. A build without its
   parser will not consume its operands as a length-framed request. Existing-runtime `INFO25` returns adapter protocol version1 when compiled,
   0 otherwise. Older runtimes reply UNSUPPORTED. `INFO25` is currently absent;
   root must resolve its allocation with other work before freezing it.
   Existing VDPL discovery/INFO needs no loaded module and uses the already
   supported short packet hook. First establish that the custom runtime exists
   with its operand-free `VDU23,0,&D9` discovery probe; only then issue its
   length-framed INFO query. A stock VDP must never receive INFO or command16
   operands as a blind capability probe. Sink capability128 alone does **not**
   prove that this ordinary-wire adapter exists.
4. No modification to stock MOS or to its two-byte audio-status packet.
   Detailed replies use the existing application keycode-packet hook, with a
   distinct marker and request token. Install that handler before discovery.

## Request bytes

All multibyte fields are little endian. No C structure layout is sent directly.
The optional adapter always replies to complete requests; no optional ACK flag
or unsolicited producer notification is needed in this first contract.

```text
23,0,0x85,channel:u8,16,
version:u8=1, operation:u8, token:u16, payloadBytes:u16, generation:u32,
payload[payloadBytes]
```

The fixed header is15 bytes including the VDU prefix; the largest payload is8,
so every current command is at most23 bytes. `token` is application correlation,
not ownership. Live ownership is producer domain + channel + nonreused32-bit
generation. Keep one outstanding request per client and do not reuse a token
while a matching reply can still be in flight. A timeout is not permission to
retry WRITE: its acceptance may already have occurred. Recover by querying state
with a fresh token and accounting against the last known accepted-frame counter,
or explicitly flush/close/reopen; do not blindly replay an ambiguous write.

| Operation | Channel/generation | Exact payload | Successful reply |
| --- | --- | --- | --- |
| DISCOVER0 | 255 / 0 | Empty | State snapshot with generation0, channel255, actual rate, maximum8192 capacity, prime1, zero flags/counters |
| OPEN1 | 0..31 / 0 | `sampleRate:u32, capacity:u16, prime:u16` | State snapshot; rate0 means current, capacity0 means1024, prime0 means1 |
| WRITE2 | Live channel/generation | `bufferId:u16, offset:u32, frames:u16` | Short copied-source ACK |
| STATE3 | Live channel/generation | Empty | State snapshot |
| RESET4 | Live channel/generation | Empty | State snapshot with new generation |
| MUTE5 | Live channel/generation | Empty | State snapshot with new generation |
| UNMUTE6 | Live channel/generation | Empty | State snapshot; generation unchanged |
| STOP7 | Live channel/generation | Empty | State snapshot with new generation; subsequent writes may prime again |
| CLOSE8 | Live channel/generation | Empty | Short ACK; closed generation cannot be used again |

Use the existing VDPL status values: OK0, ARGUMENT1, BUFFER2, BOUNDS3, ABI5,
MEMORY6, BUSY7, HANDLE8, UNSUPPORTED9, LIMIT13. Unknown protocol version is ABI;
unknown operation is UNSUPPORTED. Refused requests do not append, flush or change
ownership. DISCOVER/OPEN require generation0. Only DISCOVER permits channel255.

1. Read the fixed header and declared payload with the processor's bounded
   readers. Validate exact lengths and channel/version/op before effects.
2. Consume the entire declared payload of a complete malformed/oversize request
   before replying, without allocating its declared size. Keep only8 payload
   bytes locally; drain invalid extras in bounded chunks. Truncation/timeouts
   return without side effects; do not reinterpret missing operands as commands.
3. WRITE resolves one existing, non-null contiguous stock buffer block and
   checks `offset <= size` and `frames <= size-offset`. Never compute an
   unchecked `offset+frames`. Follow current native BUFFER/BOUNDS distinctions.
4. Supply its transient pointer to the shared copied sink. All frames copy or
   none are accepted. No blocking wait for room. Capacity and mute refusal use
   BUSY; a count beyond capacity also uses BUSY, like the native sink.
5. Drop the local source-owner/view before emitting the first ACK byte.
   Successful ACK authorizes immediate source overwrite or buffer clear; it
   says nothing about playback completion. No source pointer remains in the
   ring. Zero-frame WRITE still validates the referenced span under this wire
   contract; native pointer validation remains unchanged.

## Reply bytes and coherent state

The MOS parser's16-byte limit applies to each packet body. A state
fragment is `0x81,14,"PS1n",token:u16,data[8]`, with `n` the ASCII fragment
index0..3. Marker `PS1` encodes this protocol/version; the fourth marker byte
encodes fragment identity. Four full four-byte markers avoid relying on packet
length, which the public MOS hook does not supply. They cannot be ordinary
four-byte key packets: their fourth byte is an ASCII digit, not key-up/down0/1.

Serialize **one** native state snapshot into32 bytes under the sink guard,
then release that guard before sending any packet. Do not read live counters
again for later fragments. Send the four eight-byte slices in index order.
Their four14-byte bodies plus four2-byte packet headers total64 wire bytes.
Even if existing packet callbacks issue further work during sending, these
fragments describe the same captured state.

| Logical offset | Field |
| --- | --- |
| 0 | generation:u32 |
| 4 | actual sampleRate:u32 |
| 8 | capacity:u16 |
| 10 | priming threshold:u16 |
| 12 | queued frames:u16 |
| 14 | free frames:u16 |
| 16 | accepted frames:u32 |
| 20 | consumed frames:u32 |
| 24 | underrun output frames:u32 |
| 28 | flags:u8 (opened1, running2, muted4) |
| 29 | channel:u8;255 means no selected channel |
| 30 | status:u8 |
| 31 | operation:u8 |

The16-bit capacity/count fields are exact because the existing maximum is8192.
The flags field is exact for the existing three flag bits. Future widening
requires another protocol version, not silent truncation.

The short reply is `0x81,14,"PS1A",token:u16,operation:u8,status:u8,
generation:u32,freeFrames:u16` (16 wire bytes). WRITE success returns the live
generation and free count captured after publication. CLOSE success echoes the
generation just closed, with free0. All errors use this short reply and echo
the **request** generation with free0; error echoes do not prove that a stream
is live or reveal another owner's live generation.

The client interrupt hook copies/reassembles recognized packets into fixed
storage, clears their first four bytes before returning to MOS as the existing
VDPL/AUDQ worked examples do, and leaves genuine keyboard/other service packets
untouched. Accept state only after all four fragments have the outstanding
token, correct marker/version/order, and final expected operation/status.
Drop mismatched, duplicate and out-of-order fragments; start a fresh assembly
only from matching fragment0. Restore the previous callback and text cursor on
every graceful application exit. The client must copy in the hook, not poll
one overwritten14-byte mailbox for all four rapid fragments.

This transport shares MOS's keyboard hook. It preserves its keymap and genuine
packets but the existing mechanism still increments the MOS key event counter
and clears the transient last-key fields for recognized service replies.
Applications must use the keymap or their own queued key events rather than
equate keycount increments with new physical keys. Do not promise a perfectly
independent keyboard event channel. A new MOS packet router is outside scope.

## Producer ownership and lifecycle: mandatory shared-core change

1. Add an internal owner domain `none/native/wire` to the existing adapter.
   Native request callback uses `native`; ordinary VDU adapter uses `wire`.
   DISCOVER is non-owning. OPEN refuses an already-open ring, and successful
   OPEN assigns exactly one domain. All live calls check the domain before
   accepting channel/generation. Wrong domain returns HANDLE; it must not
   steal, flush, inspect private live state or close the other producer.
   The public native descriptor/table ABI need not change.
2. Preserve the **unallocated channel** rule. The examined firmware's
   `channelEnabled(c)` means `audioChannels[c] != nullptr`, even for an idle
   stock voice. DISABLE calls `goIdle()` and does not free it. Channel31 after
   a fresh boot is the usual choice; do not say any disabled channel qualifies.
3. Split runtime module cleanup from global cancellation. Current
   `vdpl_runtime::audio_close()` calls `pcm_sink::force_close()` after failed
   module calls, failed INIT and UNLOAD. Change its sink path to close only a
   **native-owned** stream. Otherwise an unrelated module failure/UNLOAD would
   destroy a wire stream even if that module never used audio.
4. Keep unconditional cancellation for stock channel reset/disable of the
   selected channel, global mixer replacement and firmware shutdown. Those are
   explicit global host controls and must serialize with a consumer before
   freeing the ring. Clear owner when it closes. A stale wire request after
   cancellation returns HANDLE; a future OPEN gets the next generation.
5. Keep RESET/STOP/MUTE generation changes, counter reset and priming exactly as
   the native core. UNMUTE does not replay flushed samples. Rate replacement
   cancels; never play old bytes at a silently changed rate. Generation epochs
   must not wrap/reuse. Stable firmware voice lifetime and short critical
   sections remain unchanged; no additional permanent thread is required.
6. The wire protocol admits one foreground producer. It does not arbitrate
   mutually untrusted MOS applications that deliberately share/guess the live
   token; current firmware is not an isolation boundary. Native-versus-wire
   ownership prevents accidental cross-route cleanup and takeover.

## Implementation and qualification requirements

1. Freeze command, markers, discovery field and the above error/ownership rules
   with6502 first. Publish a protocol header and independently decodable sample
   vectors. Let6502 prepare a module-free client while root implements adapter.
2. Add source-gated wire parser/serializer and internal owner tagging, retaining
   one existing ring and native ABI. Measure adapter-only flash/BSS deltas
   against the identical lean and pixels-lean source/configuration pins.
3. Host-qualify full length draining, bad version/op/channel, truncation,
   overflow, noncontiguous buffers, all-or-BUSY and immediate source poisoning
   after success. Check request<=23 and each packet<=16, full state wire64,
   one coherent snapshot, mismatched tokens, duplicate/order failures and real
   keyboard interleaving. These codec checks do not qualify the VDP parser.
4. Exercise native/wire parity with identical samples and deterministic
   consumption, both route takeover refusals, stale generations, occupied idle
   stock voice refusal, native errors/INIT failure/UNLOAD preserving a wire
   stream, wire CLOSE preserving unrelated native/module state, cancellation
   by selected stock channel and mixer replacement, wrap, underrun and memory
   release. No callback into module code is permitted after return/unload.
5. Under the central Oryx lease, run actual stock MOS with no loaded module,
   immediate buffer reuse after WRITE ACK, interleaved keyboard, long streaming
   with bounded heap, repeated cleanup and restored prompt/cursor. Compare
   exact producer bytes and state with native control using the same runtime.
   Listening, host audio delivery, scheduling and physical hardware remain
   separate evidence. This draft runs no emulator and assigns no peer work.

## Freeze and provenance

The6502 owner accepted the exact four-packet state, short ACK, request lengths,
discovery, producer ownership and keyboard caveats in mailbox
`2026-10-30_18-54-04_agon-6502_golem057-pcm-contract-accepted-three`.
The examined pinned upstream/maintained sources allocate audio0..14, optional
queue15 and runtime INFO0..24; audio16 and INFO25 have no competing local claim.
This first adapter is supported only with the existing loader and lean or
pixels-lean sink, behind `--pcm-wire`; absent profiles retain existing defaults.
No implicit stock-VDP fallback or sink-only discovery is added.

The protocol header is `include/vdp_pcm_sink_wire.h`; independently decoded
sample vectors are `docs/tasks/VDPL-011/wire-vectors.json`. Both are pinned in
the dispatch receipt. Layout changes require a new explicit contract/version,
never an unannounced edit to these accepted request/reply fields. Implementation
and target qualification remain open; this freeze does not claim runtime success.
Source findings and layout proof are retained in GOLEM-057/pcm-wire.
