# Flower becomes an interactive application

The ordinary AgonDev C++ client now runs Flower from seven prompts, retains
exact decimal strings across drawings and restores the original screen mode on
Escape. Its actual eZ80/MOS execution and native mathematics pass headless checks;
the stopped Oryx profile is ready for the Author's review.

The accepted renderer was committed/pushed as 4b18679 before this tranche. The
Author accepted keeping 32-point batching transparent, then released the client
and restored Oryx availability. No graphical launch was made.

ABI-001, APP-001 and CLI-001..005 are implemented. Native-only C interface
generation derives IDs/offsets/lengths from the shared metadata, preserving both
earlier dual-target ABI headers byte-for-byte. The 34760-byte executable embeds
the unchanged 13668-byte module and installation/cleanup bytes. Golem calculates
all coordinates; the existing resident bridge draws; C++ owns input/transport.
The maintained compiler, generic SDK/runtime, actual firmware and emulator
sources are unchanged.

Seventeen final headless scenarios pass. Both engines run two complete curves,
retained text/corrections, rejected NaN and derived overflow, held-trigger
suppression, F1 re-entry and failed-preparation picture retention. Additional
scenarios cover all seven prompt exits, startup Escape, long-curve Escape,
legacy 640x480 selection, relocated buffers, full-circle input, 31-character
editing, double-buffered restoration and busy/absent runtime. All native cases
execute a real MOS dir afterward. Stock control observes controlled exit, the
original mode and captured compatibility message; it has no wire-trace hook.

There are 2580480 exact playfield comparisons; all four settled workflow images
and application logs match between engines. 95 complete batch sequences in wire
transcripts each send 49 bytes including runtime/remaining/plot completion. Host
ASan/UBSan parsing/editor/packing/dispatch checks pass. Default and relocated
eZ80 binaries compile/link with the actual AgonDev toolchain.

Early test-harness issues were corrected: counting a changed retained prompt as
though its old text appeared repeatedly, reading a partially written log line,
and assuming stock native VDP exports the hybrid wire trace. The first cross-
engine error-pane capture was too early for interpreter text painting; queued
glyphs now settle before the full-image comparison. These are not treated as
passing old receipts; final reruns and exact source/binary identities are retained.

The callback demultiplexes native replies/keys without libc or drawing in IRQ
context, suppresses MOS character input and preserves physical key/release state.
The client checks native success before rendering and confirms completion before
unloading/cleanup. Ownership that cannot be confirmed, or an unresponsive VDP,
requires reset while preserving resources. Missing-ACK/hung-module/mode-failure
injection and physical qualification remain open under CHK-002. The vendor eZ80
parser's tested 3.0300 conversion differs by one ULP from the compiled initial
3.03f; exact retained text is unaffected.

The profile at docs/tasks/GOLEM-044/.emulator is staged/stopped on Oryx with
copied accepted runtime bytes, current MOS 3.0.2 and the same actual firmware.
Its canonical launcher is the only normal entry. The Author follows the guest's
single keyboard instruction, closes/reports; the agent retrieves saved results.
New client/profile work remains uncommitted for human review and explicit
publication approval. See the [application report](../tasks/GOLEM-044/app/README.md)
and [qualification summary](../tasks/GOLEM-044/app/evidence/summary.json).


### Mode 19 and white foreground

The Author requested mode 19 (1024x768, four colours) and white throughout.
The client now selects and verifies that mode and maps its shared text/graphics
foreground to pure white on black. The previous palette entry was yellow;
its blue channel is now 255. Stock logical-coordinate scaling handles the
resolution change without changing Golem curve math, module bytes or resident
renderer bytes. Original mode restoration and the dynamic prompt pane remain.
The first human preview was visually accepted as fast; its saved log contains
a completed curve but no controlled Escape exit, so it is preserved separately
without claiming a complete lifecycle review. New mode 19 review remains pending.


Mode 19 qualification passes all 17 Oryx headless scenarios. Both engines
produce identical complete workflow pictures and application logs. There are
3,014,656 exact playfield comparisons and 94 observed complete 49-byte batches
(the cancellation test may stop at a different batch). Default pictures also
match with relocated IDs, the legacy family and angle 360. All captured
workflow text and curves are pure white on black. The mathematical module and
resident renderer bytes remain identical to the earlier accepted renderer.
Host sanitizer/interface checks pass. The final default 34,669-byte executable
is staged with checked runtime identities on the stopped Oryx Flexe profile.
Human mode 19 review and explicit publication approval remain pending.


### Acceptance and publication — 2026-10-15

The Author accepted the mode 19 white-on-black review and explicitly requested
commit and push of all pending work. REV-001 is satisfied. Retrieval of the
saved human log found no controlled Escape exit, so no separate human exit or
MOS keyboard pass is claimed; the 17 passing headless scenarios retain the
independent lifecycle evidence. Earlier pending-publication statements above
are historical and superseded by this approval. Controlled fault injection
and physical qualification remain open.
