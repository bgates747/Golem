# Wolf bitmap animation — GOLEM-020

The requested animation is implemented with existing compiler features, qualified
and staged on Oryx. Golem owns frame clearing and every bitmap draw; assembly owns
asset loading, bouncing position arithmetic and VSYNC presentation. This boundary
is stated in both linked sources and the onscreen labels. Human review is pending;
no graphical emulator was launched and nothing was committed or pushed.

The program installs seven compiler buffers once (360 bytes), loads the unchanged
Wolf AGNB once and runs 600 frames. Three native-size images bounce independently
over the native splash in a 320x192 playfield. Mode136 supplies a 320x240 64-colour
double-buffered display. Both banks receive labels in their bottom 48 pixels.
One twelve-byte position update plus a resident call and swap costs 32 wire bytes
per frame. The Golem drawing backend still has no arithmetic/branches; those
features are not implied by this example.

Assembly reflection preserves overshoot and is verified against an independent
model at frames 1, 90, 180, 270, 360, 450 and 600. The first qualification found
that Fab's readScreen observes the front buffer; moving checks after presentation
fixed the fixture. Mid-animation pixel probes were then removed from the human
run to keep motion uninterrupted. Only final-frame pixels are queried, before and
after returning to single-buffered mode8 for visible MOS/dir output. The exact
human program passes 841 checks / 946 bytes locally and on Oryx.

The first version combined a MOS vblank change wait and a stock VSYNC swap. The
final version uses the swap as its sole presentation clock, with UART flow control
preserving packet order. This removes redundant host timing; it did not materially
change measured execution in this emulator. Runtime is not assumed to equal the
nominal ten seconds for 600 frames at 60 Hz. Final execution including setup and
observations took 22.481s locally and 21.927s on Oryx. These are
fixture durations, not hardware frame-rate claims. The assembly width audit found
23 wide accesses with no hazards or unresolved review items.

Oryx runtime/fixture hashes and all observations match; the emulator is stopped
and qualification results removed for a fresh human run. Both Golem and assembly
sources, generated golem.inc and interface.json are available on Oryx. The original
GOLEM-019 results remain intact; its corrected static run was also retrieved and
matches all 864 bytes, with visual acceptance recorded. No explicit keyboard
confirmation or commit approval was inferred from that acceptance.

The Author reported success after the staged animation review. The agent retrieved
human-results.dat and verified all 841 checks / 946 bytes, with unchanged staged
runtime/fixture identities and the emulator stopped. human-review.json records
animation and MOS keyboard acceptance. GOLEM-020 is accepted and removed from
the active TODO. GOLEM-019 is also closed: its static visuals and saved results
already passed, and its remaining separate keyboard check is superseded by this
successful animation/MOS review. Both IDs and evidence remain in their task silos.
Changes remain uncommitted and unpushed pending explicit commit approval.

The Author explicitly instructed “commit and push” after the successful review.
This authorizes the accumulated GOLEM-011 drawing audit, GOLEM-018 geometry,
GOLEM-019 bitmaps and GOLEM-020 animation work. Maintained compiler checks pass
(114 rejection cases), diff whitespace checks pass and origin was fetched with
no branch divergence. Human evidence records this explicit commit approval.

The Author subsequently suggested autonomous TI-99/4A-like sprite motion, then
explicitly requested recording the idea without implementing it. The deferred
note in [the drawing design](../design/resident-drawing.md#deferred-idea-autonomous-sprite-motion)
captures persistent position/velocity, possible boundary behaviours and the goal
of setup/change traffic without eZ80 frame calls. Scheduling-hook feasibility
remains unverified. No compiler, runtime or emulator changes were made, and this
idea was not promoted into the active task queue.
