# FSIM rendering cost — source planning v1

Identify a small measurement that explains where the live FSIM client's frame
time goes before choosing renderer optimizations or adding runtime services.
The Olimex startup checks passed, but they neither reproduce Console8's mode
failure nor attribute its reported slow rendering. This tranche is source
planning only; it requires no new firmware, device operation or emulator run.

1. Work in the existing PINGO-066 consumer task and preserve its engine,
   physics, input, assets, renderer, display contract and source pins. Keep
   the completed Olimex startup cohort and Console8 reports distinct.
2. Inspect existing eZ80 timing/counter support and exact frame flow. Propose
   bounded counters at useful boundaries: simulation/pose preparation,
   background/HUD composition, native render call, ordered completion and
   presentation. State which boundaries include UART transport, parser wait,
   graphics queue consumption or VSYNC. Do not call an end-to-end wait isolated
   native rendering time.
3. Audit the proposed clock's units, rate, wrap, interrupt/coalescing limits and
   resolution. Prefer existing client facilities and accumulated counters
   emitted after exit. No per-frame filesystem/serial logging in the timed
   path. Host serial receive timing is not an independent frame-present clock.
4. If the present interfaces cannot separate native compute from wire/wait,
   explain that limit and the smallest consumer-private alternative. Do not
   allocate a generic service or change ABI, firmware or module at this stage.
5. Return one finite candidate plan with exact counters, start/end boundaries,
   workload/controls, expected duration, normal exit/cleanup and how raw output
   supports each proposed conclusion. Choose enough resolution to discriminate
   major costs without building a general profiler. Separate correctness from
   timing and do not require replaying already-passing scene matrices.
6. Source work on the Pi is allowed now. During the Author's generation76
   recovery review, no Oryx access or build/staging/testing is requested. No
   hardware access, flash, Console8 session, automatic test or implementation
   follows from returning the plan. Root reconciles it with the consumer's
   existing priorities before selecting further work.
