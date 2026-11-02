# Bounded failed-mode release experiment

The allocation journal now captures the cause of the recorded fallback failure:
the previous two-colour mode asks for a 2,048-byte lookup table while the failed
64-colour controller is still installed, receives null, and publishes that null
table before the ISR uses it. Prepare a small separate firmware experiment that
releases the failed display before constructing its fallback. Keep successful
transitions and all existing module limits unchanged. This is not a general
out-of-memory recovery or physical-hardware claim.

## Evidence and frozen scope

The original control and journal artifacts remain untouched. Root independently
read both returned result files and recorded their hashes and six actual events
in `display-diagnostic/actual-journal-summary.json`. The instrumented result is
`8e39fd977fa215ab290985a77095b986cf0c7ddab28c4d30046170654b42fd48`;
raw UART0 is `ee53239f5ee412f2d0c531e873954447f84b05fdcf82dcd55fc92da0f2012aa0`.
Its six records are contiguous and unsaturated, the foreground MODE checkpoint
completes, and all 32 stored ISR origins uniquely match the same-run fallback
controller. Both observations still fail product/cleanup qualification at their
bounds. No unrecorded suffix or physical allocation cause is inferred.

Implement the source-only proposal in `mode-fallback-release-proposal.md`:
after `changeMode` returns height error 2, dispose Canvas, call the installed
controller's `end()`, reset it, invalidate cached depth to 255, then invoke the
existing previous-mode fallback. If that fallback also returns 2, do the same
before the existing final fallback. Do not dispose on invalid-mode, teletext
failure or factory error. Do not add allocation probes, retries, plane guards,
ISR changes, journal fields, capabilities, services, background tasks or limits.

## Preparation and gates

- FIX-001 [ ] Hash this plan before implementation. Use task-local paired
  control/candidate copies of the original010 source and pinned cached GL/tool
  dependencies; no upstream or maintained SDK/default edit. Use equal-length
  build-root names and the same flags so resource deltas are interpretable.
  Preserve original010, macrooff/macro-on diagnostics and current SDK images.
- FIX-002 [ ] Apply only the failure-disposal helper/two call-site conditions to
  the candidate. Verify unchanged success/error1/invalid paths, Canvas-before-
  controller order, stop-before-delete, deletion-before-factory and invalidated
  cached depth. A focused fake-owner test should include same-depth failure;
  no allocator refactor or exhaustive fault-injection framework.
- FIX-003 [ ] Build matched control/candidate with the existing offline Xtensa
  toolchain and two jobs, collect image/ELF/map/static/IRAM/stack/source pins and
  report actual flash/RAM costs. Existing dependency/source checks suffice;
  do not fetch libraries or invoke the evolving SDK exporter.
- FIX-004 [ ] Send the frozen candidate and precise failure-only behavior to6502.
  Its owner prepares a separate finite source/host/stopped-profile contract:
  same original module/client/ROM/history and mode20 refusal, actual restored11,
  three fresh decoded VGA frames, retained module state and normal FINI/unload,
  independent exact heap/cleanup status. Both JIT/interpreter and one successful
  unchanged mode transition are the target gate; preserve first unexpected
  failure, normal stop/join and no new unexpected memory events. Root reviews
  readiness and grants new exclusive Oryx execution; no launch during build.
- FIX-005 [ ] Analyze whether disposal removes fallback allocation pressure and
  restores a usable display. Record residual heap differences honestly; no
  rollback/pixel-preservation/general recovery promise. Only then decide whether
  the narrow patch belongs in the maintained SDK export and needs wider review.
  Human validation and publication remain separate.

Preparation is released under GOLEM-057. Target execution, physical flashing,
publication and SDK/default promotion are not released by this plan.
