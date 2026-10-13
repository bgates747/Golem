# Executable RAM and Flexe reloads

Flexe's stock JIT translates ESP32 IRAM, but ordinary RAM stores do not
invalidate existing translations. A first load at a previously unexecuted
address can work while a replacement vdplet at the same address executes stale
host code. Qualification must therefore include a replacement payload with a
different result; the owned emulator integration needs an explicit invalidation
policy. This finding was sent to the adapter and runtime agents before their
implementation work. No upstream source has been edited.

Source examined: Flexe commit
`fbda8e905ccff36a090e4e33b0ffc2337dd79c52`.

| Source | Finding |
| --- | --- |
| `src/jit.c:288` | JIT admission follows every executable range in the target descriptor, including original ESP32 IRAM. |
| `src/memory.h:233` | Fast `mem_write8`, `mem_write16` and `mem_write32` update mapped backing bytes directly without invalidating decoded or translated code. |
| `src/rom_stubs.c:996` | Original ESP32 `Cache_Flush` is a return-only stub. Calling it does not flush Flexe's JIT. |
| `src/xtensa.c:329` | `xtensa_invalidate_code(cpu, addr, len)` clears overlapping predecode entries and invokes the installed code-invalidation hook. |
| `src/jit.c:4559` | The JIT invalidation callback conservatively flushes its whole cache. |
| `src/jit.c:4614` | Flushing while translated code is active is deferred through `invalidate_pending`, avoiding freeing the active instruction stream. |
| `src/peripherals.c:2445` | Existing flash-MMU invalidation knows that two session CPUs share predecode/JIT objects and avoids a duplicate flush. Ordinary IRAM writes do not pass through this path. |

This concerns the host emulator's translation cache. It is separate from the
ESP32 loader's alignment, executable allocation and real hardware memory
requirements. A test passing only in interpreter mode cannot establish JIT
replacement correctness.

Two independent cache problems were found. Linux AArch64 also needs host
instruction-cache maintenance after Flexe generates or patches native code.
The pristine stock gate failed with SIGILL; tiny regenerated MOVI blocks could
instead keep returning the old value even after a guest translation flush.
The adapter agent added `__builtin___clear_cache` at the host code-write
boundaries. With that correction, stock interpreter/JIT checks pass and the
guest-RAM stale-translation issue becomes reproducible without host-cache noise.

The adapter-owned reload policy observes the guest's architectural ISYNC
boundary. It compares a shadow of its configured original ESP32 IRAM range
(`0x40080000..0x400AA000`) and calls `xtensa_invalidate_code` when the bytes
change. This covers normal memory stores, translated guest stores and ROM
memcpy backing-pointer writes without instrumenting each possible write path.
The firmware's native call path emits `memw; isync` before executing loaded
code. The observer belongs to the emulator; hardware synchronization and
loader memory requirements remain separate responsibilities.

The JIT ends a translated block before ISYNC so the canonical interpreter
executes the observer. Both cached-interpreter routes need the same behavior:
the quick decoded path must call it, and predecode must classify ISYNC as
`PD_SLOW` rather than `PD_NOP`. The independent tests caught both bypasses;
the adapter agent corrected them before final qualification.

`iram-qualified-receipt.json` records fourteen passing cases against the exact
corrected source/library hashes:

| Cases | Evidence |
| --- | --- |
| Six raw replacement paths | Byte, halfword and word host stores; direct backing memcpy; translated and interpreted guest S32I. The interpreter returns replacement value 43, JIT returns stale 42, and explicit invalidation makes JIT return 43. |
| Six matching ISYNC paths | The same writes followed by real ISYNC run replacement code immediately, including predecoded interpreter barriers. |
| First load into unused predecoded IRAM | ISYNC invalidates the predecoded empty bytes before initial execution. |
| Translated S32I/NOP → ISYNC → RET | A genuinely translated store prefix reaches the observer through the JIT fallback and executes replacement code. |

The fixture shadows the eight bytes being changed, while the adapter shadows
its full configured IRAM region. These synthetic checks establish translation
coherence for those paths; they do not establish actual vdplet service/lifetime
behavior, arbitrary self-modifying programs without ISYNC, or physical ESP32
execution. The parent task owns the full MOS/native-module proof.
