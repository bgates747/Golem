# Native compiler RAM improvement review

The short two-float program that previously exhausted the Agon's compiler RAM
now compiles normally. This review compiles that exact source on the emulated
Agon, then runs its resident buffered commands on stock VDP. Golem multiplies
the inputs and prints every value; the C client installs the package, supplies
inputs and separates the rows.

Read [two-floats.golem](two-floats.golem). The same source is present on the
emulated SD card beside the client and generated package.

```bash
cd ~/Agon/mystuff/golem/.emulator/golem-055
./fab-agon-emulator
```

Expect these five lines below the explanation:

```text
reading=1.5 scaled=3
reading=1.5 scaled=3
reading=-2.25 scaled=-9
reading=3.125 scaled=1.5625
reading=0 scaled=0
```

The first call uses the declared initial values. The second repeats the call
without resetting state. The remaining calls receive `(reading, scale)` pairs
`(-2.25, 4)`, `(3.125, 0.5)` and `(0, -8)` from the C client.
Follow the single onscreen keyboard instruction, close the emulator, and report
whether the output and keyboard worked. The agent retrieves the saved results.

This is actual Fab Agon Emulator 1.2.5, MOS 3.0.2 and its stock native VDP module.
The existing GOLEM-054 review instance stays separate. No graphical instance is
launched by the agent, and no hardware is flashed.

The automated checks compare independent expected-text and Golem framebuffers,
all input/status observations, every generated package file, the ordinary
dispatcher and optional response-file paths, and the unchanged distribution
runner. Automated evidence does not replace the Author's visual review.

Agent-only qualification/staging commands:

```bash
.venv/bin/python docs/tasks/GOLEM-055/qualify.py
.venv/bin/python docs/tasks/GOLEM-055/qualify.py --response-edges
.venv/bin/python docs/tasks/GOLEM-055/stage_oryx.py --qualify
.venv/bin/python docs/tasks/GOLEM-055/stage_oryx.py --review
```

The Author accepted this test and explicitly approved commit/push on 2026-10-30.
The saved human results and subsequent cursor correction are recorded in
[human-review.json](human-review.json); automated cursor checks cover normal and
error returns. The launch commands above remain available for reruns.
