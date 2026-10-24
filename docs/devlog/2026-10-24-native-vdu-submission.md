# Native drawing byte submission: executable checkpoint

The runtime now lets a native worker submit complete move/line bytes to the
existing VDP interpreter. A complete Corsair consumer works through both
execution engines, preserves old modules and returns cleanly to MOS. Installation
data shrinks substantially; JIT frame time is unchanged, while the interpreter
comparison is faster. The stopped Oryx animation awaits human review.

1. Published the previously authorized work before implementation: stock matrix
   compiler `b48aae7`, accepted AGNB integration `ba1511c`, Golem plan `75f6880`,
   generic runtime plan `5ac110d`, and owned consultation/index slice `e30fffa`.
   Unrelated repository dirt/history was preserved.
2. Implemented generic VDPL-003 in agon-vdplet: optional ABI1.1/table extension,
   capability discovery, bounded borrowed spans, whole-span framing admission,
   original interpreter/rasterizer reuse and exact legacy table selection.
   Firmware grows 784 image bytes and 16 static DRAM bytes. Final source receipts
   and a rebuild preserve the qualified image/ELF hashes.
3. Each actual ESP32 engine passes 19 generic cases, 1,179,648 pixel comparisons
   and 500 observed bytes with zero execution faults. Old precompiled modules
   run unchanged; old firmware rejects new images cleanly. All four leases,
   outer buffered continuation, partial effects and lifecycle/state rejection
   are covered. These are emulator checks, not physical acceptance.
4. GOLEM-047 consumer proof includes the frozen owner C++ transform unchanged,
   adding a separate native drawing entry with 384-byte scratch and four leases.
   Six-pose composition observations match the original precompiled transform.
   Full MOS JIT/interpreter runs return/unload cleanly; all three final
   159,744-pixel playfields match. Source roles are explicit in the human demo.
5. Matched JIT full-frame medians are 780 ms each. Interpreter medians are
   1,760 ms resident versus 940 ms submitted. Both arms send 221 bytes per timed
   frame. Separate pure scene containers measure 216,960 versus 60,200 bytes;
   the paired timing runs coinstall both drawing paths to control allocation
   layout. Raw samples/pins/captures and the independent summary are retained.
   Effective MOS clock quantization is 20 ms. Isolated raster/generation/HUD
   splits and physical speed remain unmeasured.
6. Wireframe consultation-six explicitly resolves the stale relayed hold and
   accepts ordinary console tracing. The implementation/qualification handoff
   is durable mailbox message
   `2026-10-24_00-21-20_golem_golem-vdu-submit-qualified-seven`; no current peer
   question blocks review. Fsim source/application ownership remains intact.
7. Dedicated Oryx `.emulator/golem-047` stages a finite 24-pose white Corsair
   in mode19, using only submitted drawing. Its Flexe wrapper launches fresh
   results and the guest gives the sole `dir` instruction. Agent-only saved-run
   verification follows the Author's report. No interactive launch or flash.

Current detail: [GOLEM-047](../tasks/GOLEM-047.md),
[executable report/source/review](../tasks/GOLEM-047/README.md).
Portable Golem drawing syntax/emission remains unfinished; the runtime proof
does not dispose that work. New coupled code/profile/documentation changes
remain uncommitted pending human review and publication approval.


## Accepted review

The Author accepted the run and explicitly authorized commit/push on 2026-10-24.
Retrieved results match all 20 bytes and the completed log; runtime pins, zero
execution faults, actual MOS directory listing and stopped profile verify.
The human visual/keyboard gate is closed. Physical acceptance remains separate.
The Author prioritizes VDPlet rendering performance independent of source
language; compiler integration is secondary. Further rendering options are
discussion only until a subsequent implementation release.


Generic runtime/SDK publication: `5232938` on agon-vdplet
`codex/initial-runtime`. Actual rendering investigation takes precedence over
portable-language adoption under the Author's current direction.
