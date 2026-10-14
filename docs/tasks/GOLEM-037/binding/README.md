# Real renderer bindings — released implementation tranche

Author release in Rally thirty-two supersedes the prior Stage B hold. Golem owns
compiler/SDK/firmware and narrow emulator-clock repair; Rally owns the MOS demo,
assets and dedicated Oryx deployment. No physical flash, broad stress or commit.

- CFG-001 [x] Implement strict schema1 rally-render-v1 config, explicit native-only rejection, typed maps and reservations.
- GEN-001 [x] Emit real helper calls before root success, early argument release, completion publication/failure behavior and generated wrapper offsets.
- SDK-001 [x] Build coherent enlarged-policy SDK/firmware with focused admission/cleanup controls.
- PKG-001 [x] Deliver consumable candidate/package/maps/firmware identities promptly to Rally for independent driver integration.
- CLK-001 [x] Repair frontend MOS frame-clock progression during static native intervals without editing another owner's active profile.
- CHK-001 [x] Qualify obvious host/actual-Xtensa integration errors and preserve actual failures; cooperate with Rally's target qualification.
- REV-001 [x] Rally stages a stopped recognizable human demo; retrieve results after review, obtain explicit publication approval before commit.

## Qualification and review handoff — 2026-10-14

Candidate `48eac7efe8fa51f4` combines the actual 17-entry Golem program and
Rally renderer in one native module: 30,428 code bytes, 980 private-data bytes,
and a 32,964-byte image. Its appended export receipt verifies all 32 delivered
files; the original build manifest's omitted object intermediate is explicitly
accounted for without changing the snapshot.

Owner host controls pass default, sanitized and relocated runs: 60 rendered
frames, 240 source ticks, 317 calls, three simultaneous leases at peak, and
exact command agreement with Rally's frozen renderer oracle. Failure controls
cover source/helper/publication/release errors. All 23 configuration/target
rejections, SDK admission/cleanup checks and maintained compiler checks pass.

Rally independently qualifies the actual Xtensa candidate on Oryx in full and
viewing modes: 900 rendered frames and 3,600 physics steps each, matching its
independent final world state. Reported minimum free command-stack space is
2,260 bytes. Unload and MOS return pass; runtime error counters remain zero.
Viewing throughput is 24.04 nominal MOS frames/second, below the 30 target;
these are emulator observations, not physical-hardware timing guarantees.
See Rally's `native-demo/RESULTS.md` and received messages thirty-nine–forty-one.

The independent-VSYNC patch is pinned against an exported frontend commit;
concurrent maintained emulator work was left untouched. SDK policy increases
are implemented and qualified for this candidate, not merely proposed.
Physical flashing, broad capacity stress and publication remain outside this tranche.

Rally owns the staged, stopped human profile and retrieves review logs:

```bash
cd ~/Agon/mystuff/AgonArcade/.emulator/rally25-native-demo
./flexe-agon-emulator
```

Human visual acceptance and explicit commit/push approval remain pending.

## Human review accepted

Rally forty-two records the Author's visual acceptance of this exact flat-background
fixture and verification of retrieved Oryx logs: 900 frames, 3,600 ticks and
2,913 retained replies, with final globals/arrays matching the independent model.
Prior PRT vector and runtime identities are preserved. Missing background was
understood as outside the fixture. Commit/push approval remains pending; no
separate keyboard, full-game, physical-hardware or 30-Hz acceptance is inferred.
Rally owns HUMAN-REVIEW.json and the raw human evidence.

The Author explicitly approved commit and push on 2026-10-14. This completes the bounded composition tranche; physical qualification, broad capacity stress and autonomous scheduling remain separate work.
