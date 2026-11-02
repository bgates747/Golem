# Wolf late-audio staging path correction

Generation78 failed before emulator startup: the unchanged runner requires
PROFILE/ring-output, but the stager created ring-output-late-corrected. Preserve
that terminal failure. Correct this small integration mismatch in a fresh
profile candidate, keeping the original runner and all measurement behavior.
This selects source/device-free preparation only, with no target run.

- PATH-001 [ ] Preserve generation78/r01 staging and failure records unchanged.
  Use a new profile name for the successor. Within that unique profile, use
  the original basename ring-output consistently in the stager, unchanged
  runner, wrapper observer environment and retrieval. The absolute directory
  is already isolated by the new profile; no compatibility symlink is needed.
- PATH-002 [ ] Keep runner f0ffe798, oracle ea4f7e64, observer backend943f432f,
  helper/reader/app/module/frontend/MOS/firmware and90..100swindow unchanged.
  Keep all existing gameplay/shot/time/ownership/first-failure limits. Freeze
  the few staging path changes and exact no-build dependency/pin closure.
- PATH-003 [ ] Add one device-free actual-runner prelaunch test with a fake
  profile: exercise its real freshness/path checks and mocked process boundary
  to prove the prepared directory, observer environment and retrieval all
  refer to the same location. Assert it reaches the launch boundary without
  launching anything. Preserve missing/nonfresh path rejection. This is a
  concrete integration check, not another broad test framework.
- PATH-004 [ ] Return source/readiness and the existing78terminal handoff for
  root review. No Oryx staging/SSH/emulator/audio/build/hardware/default/
  publication is selected. A separately reviewed finite target selection may
  follow. Direct Author work in the Wolf session takes priority; do not
  interrupt it or spawn another owner to satisfy this preparation request.
