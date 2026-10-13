# Multiple drawing Subs — focused stock review

Stock Fab/MOS on Oryx passes independent pixel, shared-input and ownership
checks for both ordinary and relocated buffer IDs. The separate human fixture
is staged and stopped. It shows the actual two Golem Subs, with a recognizable
Wolf image and cyan marker line. Human visual and MOS keyboard approval remain
pending; no GUI was launched and nothing was committed.

The complete Golem source is [review.golem](review.golem). `drawMarker` is the
first/default entry and `drawWolf` is the second. Both read the same persistent
`x`/`y` globals. Assembly in [guest.asm](guest.asm) loads the Wolf AGNB, installs
once, invokes the generated named wrappers and updates the shared inputs.

The human scene calls `drawMarker` with `(96,200)`, changes only x/y to `(96,16)`
and calls `drawWolf`, then restores `(96,200)` and calls `drawMarker` again. You
should see the Wolf splash at its native 320x160 size, centered at `(96,16)` in
a 512x384 display, with a cyan horizontal line from `(96,200)` to `(416,200)`.
There are no hidden direct bitmap draws or intermediate diagnostic scenes in
this human fixture. The readable program is also copied to Oryx at the same
project-relative location.

On Oryx, launch the stopped review instance:

```bash
cd ~/Agon/mystuff/golem/docs/tasks/GOLEM-022/qualification/.emulator
rm -f sdcard/golem022/results.dat
./fab-agon-emulator --renderer sw
```

Follow the on-screen instruction: type `dir` at MOS, close the emulator, and
tell the agent whether the image/line and `dir` worked. The agent retrieves and
verifies the saved results. No second terminal or user-run verifier is needed.

The machine-only default and relocated tapes each perform A/update/B/A without
reinstallation, then independent direct-VDU controls. Selected pixels agree with
an independent RGBA2222/Bresenham oracle. Reads confirm the colour byte survives
the x/y-only update, all five compiler-owned buffers disappear after cleanup,
four borrowed Wolf buffer prefixes and an unrelated canary survive, and the
borrowed splash remains drawable. All actual assembled wrappers/call packets and
package byte payloads match the compiler artifacts. The automatic host checks
cannot certify human keyboard response; that stays an explicit review step.

Rebuild/stage commands, for the agent in the project root:

```bash
.venv/bin/python docs/tasks/GOLEM-022/qualification/prepare.py
.venv/bin/python docs/tasks/GOLEM-022/qualification/stage_oryx.py --bundle-only
.venv/bin/python docs/tasks/GOLEM-022/qualification/stage_oryx.py
```

`evidence/default.json`, `relocated.json` and `review.json` record fresh Oryx
durations, exact runtime/fixture hashes and matched saved observations. The
first two runs each pass 370 checks/415 bytes; the human-scene headless run
passes 62 checks/99 bytes. `provenance.json` pins the reused transport, canonical
AGNB loader, MOS include, compiler, helper and borrowed Wolf assets. This profile
uses the accepted stock Fab 1.2.5 native VDP and MOS 3.0.2; it does not use or
change GOLEM-021's separate hybrid/vdplet profile.
