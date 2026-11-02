# VDPlet rollout review checkpoint

The shared runtime changes have working independent consumer proofs. The current
FSIM diagnostic comparison is complete. Wolf owns its outstanding renderer
integration and late-audio work; Golem supplies generic hooks and records
consumer results without taking over their implementation or tests.
Neither is a promise to finish each application's development or solve every
emulator performance issue. Publication remains a separate Author decision.

| Project | Established result | Remaining selected work |
| --- | --- | --- |
| VDPlet runtime/SDK | Independent pixel/PCM access, ordinary PCM wire transport, scoped colour rows, resource/completion documentation and optional build/layout support are implemented and qualified within their recorded profiles. Optional failed-display cleanup passes current-image regression and human recovery review. | Preserve default compatibility; reconcile final consumer evidence and explicit publication gate. No new firmware service is selected by the diagnostics. |
| Bounded buffer return | The Author separately approves optional eight-byte readback; actual dispatcher sanitizer/fullSDK and both-engine mechanism checks pass.6502 independently passes all six MOS/key/disk cases, including failed-save dirty retention and retry. Exact512-byte exchange7424→2368B; optional432flash/0staticDRAM plus32disabled-discovery bytes. | Root251 accepts owner report817ffccd. Root257 matches revised visible-feedback report3dc4c085/readiness ef9007dd: bounded headless UI/MOS check passes, release89 complete, r02 staged/stopped. Revised utility human review accepted via owner report6d42fbd6; actual release91 and explicit Golem-owned commit/push approval recorded. No extra root consumer test selected. |
| 6502 | Current-image interpreter/JIT recovery regressions and actual human recovery/MOS review pass. Root verifies retained state, full executable-heap return and release76. | No further recovery run selected. Full-colour display fit, physical qualification and optional bulk-readback implementation remain distinct work. |
| FSIM | Native live renderer/application works; finite control/A/B client cohort passes. Separate native diagnostics pass actual host/target-layout/stack review; shared firmware increase is zero. | Actual off/on pair passes root raw trace/state/record/cleanup verification:44result files,412source witnesses/14staged inputs intact and1364B minimum stack both arms. Renderer occupies89.8684% of on-arm RENDER counter span; conditional modelled counters are not isolated CPU time or copied/direct gain. No further run or renderer optimization selected. |
| Rally | One maintained086 full cached900/3600 fixture passes exact state, all2024 native replies, full722201B command stream and cleanup. Root independently reproduces saved audits. | No further target run selected. MOS/PRT clock calibration and the old startup failure's cause remain unresolved; neither prevents accepting this bounded functional result. |
| Wolf | Bounded copied/direct rendering proofs and production play/exit review pass; severe gameplay audio delay remains reported. Corrected startup observer qualifies a narrow residence measurement. | Generation78 late attempt stopped before emulator startup on an output-path mismatch. Root227 supplies an isolated five-line staging candidate with4actual-runner mocked prelaunch tests; owner adoption/target readiness remain pending. Production renderer still composes a complete workspace then calls copied pixels_write. Root237 tells Wolf to own the correction to existing scoped rows and its independent comparison/tests; bounded row proof was not production adoption. Direct Author work takes priority. |
| Flexe | Maintained controls/current runtime and corrected private observer have scoped build and consumer evidence. Current086 passes Rally's formerly failing fixture once. | No new Flexe runtime change selected. Audio delay and legacy-stall cause are not explained by that pass. |

## Logical publication groups for later review

These describe the current worktree, not a staging or commit instruction.
Consumer agents retain their repository and direct-Author publication ownership.

1. **Runtime/media interfaces:** SDK public helpers, generic pixel/PCM service
   composition, ordinary PCM transport and colour-row access; matching focused
   tests, API/start documentation and VDPL-010/011/012 task evidence. Changes
   share public header/runtime/build files, so separate commits must preserve
   compilable intermediate states rather than stage whole shared files blindly.
2. **Optional failed-display recovery:** SDK firmware-exporter switch and
   VDPL-013 evidence; keep default behavior unchanged. Current matched builds,
   four actual recovery cases and the accepted human result support this
   specific opt-in change, not arbitrary allocation recovery.
3. **Module tooling:** optional linker fragment/allocation layout preflight,
   actual-loader agreement, default-output parity and VDPL-014 tests. No loader
   policy increase or firmware change is required.
4. **Golem coordination/research documentation:** GOLEM-043/056/057 records,
   accepted results and current handoff. There is no compiler source change in
   this rollout's current Golem worktree.
5. **Shared environment:** agon-dev-env owns the scheduling helpers/live-state
   protocol, shared contracts, mail and durable result receipts. Coordination
   records never replace consumer TODO ownership or confer publication rights.
6. **Optional bounded readback:** exact operation6, discovery, fixed replies,
   builder option, focused tests and VDPL-015 evidence. Its separate human
   review and explicit publication approval remain open.

No files are committed or pushed by this checkpoint. The current source status
is inspected again before any eventual grouping; unrelated owner edits are
preserved. Full or physical performance, all-catalog implementation, async
workers, P4 feasibility and arbitrary bulk-readback consumers are not certified by
these selected consumer proofs. Their existing research/task records remain.


Publication release2026-11-02: the Author directly requests commits/push of
Golem-owned dirt and continuation. The revised readback human gate has passed.
The current SDK29-file source closure is byte-identical to the qualified
snapshot. Commit only owned changes; shared files contain unrelated agent
work that must remain unstaged. Live state and build products stay ignored.
Prior pending-publication statements above are historical checkpoint context.
