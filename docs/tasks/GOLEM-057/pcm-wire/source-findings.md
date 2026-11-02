# Copied-PCM source findings

The small audio sink already provides the required buffering semantics. Its
wire adapter needs compact MOS-safe replies and explicit producer-domain
ownership; adding a parser alone would leave cross-route cleanup unsafe.
The sources below were inspected read-only. Draft field numbers are not frozen.

| Finding | Primary local source |
| --- | --- |
| Stock MOS protocol body is limited to16 bytes; larger replies are discarded before the application hook | `/Agon/agon-mos/src/equs.inc:22`, `src/vdp_protocol.asm:97` |
| Packet callback receives data pointer in DE; stock code then interprets first4 bytes as keyboard state. No documented length argument | `/Agon/agon-mos/src/vdp_protocol.asm:173`, `/Agon/agon-mos/src/mos_api.asm:830` |
| Zero virtual keycode does not update held-key bitmap, but callback return still changes last-key fields and keycount | `/Agon/agon-mos/src/keyboard.asm:27`, `src/vdp_protocol.asm:184` |
| Existing short magic-tagged callback worked examples | `/Agon/mystuff/agon-vdplet/docs/tasks/VDPL-007/qualification/src/packets.src:1` and `VDPL-006/qualification/src/packets.src:1` |
| Existing ordinary full queue command15 and14-byte `AUDQ` response | `/Agon/mystuff/agon-vdplet/include/vdp_audio_queue.h:5`, `src/vdp_audio_queue_vdu.h:31` |
| Existing native VDPL length32 request cap and10-byte response | `/Agon/mystuff/agon-vdplet/src/vdplet_runtime.h:491` |
| Native discovery, open, generation checking, flush and copy operations | `/Agon/mystuff/agon-vdplet/src/vdplet_pcm_sink.h:112`, `:137`, `:157` |
| Native sink has one ring,32-bit modulo counters,8192 maximum and1..8192 priming bounds | `/Agon/mystuff/agon-vdplet/include/vdplet_pcm_sink.h:17`, `src/vdplet_pcm_sink_core.h:13` |
| Source pointer exists only during memcpy, then publication; consumer metadata guards do not cover bulk copy | `/Agon/mystuff/agon-vdplet/src/vdplet_pcm_sink.h:137` |
| Failure/INIT failure/UNLOAD call unconditional sink force-close | `/Agon/mystuff/agon-vdplet/src/vdplet_runtime.h:198`, `:267`, `:289`, `:309` |
| Global mixer replacement and selected stock reset/disable cancel the sink | `/Agon/mystuff/agon-vdplet/tools/build_firmware.py:226` |
| Stock commands0..14 only; MAX_AUDIO_CHANNELS32; packet keycode1 | Pinned export `/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/video/agon.h:93`, `:107`, `:113` |
| `channelEnabled` tests non-null allocated object; disable goIdle does not free | Same export `video/agon_audio.h:115`, `:221` |
| `send_packet` writes byte code+0x80, byte length, body then packet callbacks; reply fragments must use one pre-captured snapshot | Same export `video/vdu_stream_processor.h:520` |
| Current maintained INFO table ends at24; unknown IDs return UNSUPPORTED | `/Agon/mystuff/agon-vdplet/src/vdplet_runtime.h:315` |

The firmware export receipt pins upstream to
`1056ee38ec1cc68ed327bd32ee47d04b1c30597c` and graphics to
`ac2dd5986daf496c43ae8e7fe41836274aec54a0`. This export is an owned lean integration,
not the read-only upstream checkout. Its exact receipt is recorded in
`source-pins.json`. The local `/Agon/agon-vdp` checkout does not contain that
commit; its current working tree was only an initial cross-check, not evidence
that it is the exact pin. No upstream source was modified or fetched.

The current MOS checkout is recorded separately by commit and file hashes.
Actual target qualification must use a pinned stock MOS image and independently
verify that it has this receive limit/hook behavior; local source inspection is
not a substitute for running the client under that image.
