# Native compiler qualification

The current Golem compiler now runs on the Agon as AgonDev-built eZ80 programs.
All implemented profiles and native C emission are retained. The standalone
package supplies a MOS command, runner, ten examples, how-to, language reference
and rebuildable source. Headless qualification passes; human and hardware review
remain separate. No firmware was flashed and Oryx was not contacted.

## Evidence

| Check | Result |
| --- | --- |
| Current desktop vs adapted host | 316 matching comparisons: 75 accepted, 241 rejected |
| Adapted host ASan/UBSan | All 316 comparisons clean, including rejection paths |
| Actual eZ80/MOS compilation | All 316 match, with no resource-limited corpus cases |
| CLI seams | 13 pass on host, sanitizer build and eZ80/MOS |
| Native-compiled examples | All ten packages match desktop bytes |
| Stock execution | 56 result bytes and 13 white-cube anchors match |
| Numerical adapter | 17,966 decimal/float-step comparisons pass |
| Isolated package-source rebuild | All six executable binaries match byte for byte |
| Installed compiler/runner review script | White cube, MOS dir/save and normal frontend exit pass |

The captured corpus comes from the five maintained contract suites and contains
280 unique sources plus native-only variants. It covers raw, resident scalar,
table, drawing, portable arithmetic/control/arrays and stock matrices/loops.
It includes the 1,024-operation matrix loop boundary. Separate CLI checks cover
32-bit descriptor limits, quoted response-file paths, mapped data, reservations,
assembly output and load plans. See [inventory.md](inventory.md),
[qualification.json](qualification.json) and the individual evidence files.

These comparisons establish port fidelity to the current compiler, rather than
an independent proof of every language behavior. The example runner additionally
observes known numerical results on stock VDP before cleanup. Native C is emitted
and compared here; this task does not provide an on-Agon C-to-Xtensa toolchain.

## Runtime footprint

| Worker | Executable bytes | Initial heap bytes | Reserved stack |
| --- | ---: | ---: | ---: |
| Raw | 61,528 | 380,690 | 16,384 |
| Resident | 176,527 | 265,689 | 16,384 |
| Portable | 277,538 | 164,677 | 16,384 |
| Stock matrix | 120,108 | 322,109 | 16,384 |

Only one worker is loaded at a time below the dispatcher at 0xB0000. The
10,795-byte dispatcher loads a fresh worker for each ordinary or supervised
batch command; the runner is 12,746 bytes. Larger programs can still exhaust
finite RAM. Profile limits are unchanged, rather than widened by this port.
Wall-clock timings are retained in the evidence under shared-host load and
must not be read as controlled performance benchmarks.

## Implementation choices

The original C++ parser/backend sources are pinned and mechanically adapted,
with a purpose-built library surface. This avoids maintaining another grammar.
Explicit recovery handles the known exception sites that AgonDev cannot unwind.
Binary64 compiler arithmetic uses selected SoftFloat routines and exact decimal
conversion; explicitly wide descriptors avoid the target's 24-bit int width.
A local adjacent-float implementation avoids the installed SDK nextafterf bug.
Exact small integers bypass rational conversion without changing precision.

Memory lifetime changes release completed intermediate maps and operation
vectors, spool backend metadata to SD, write binary arrays directly and move
completed results instead of copying them. Repeated compilation inside one
worker can fragment its heap; the supported dispatcher batch path reloads the
worker exactly as ordinary command use does. This does not affect generated
program execution or wire traffic.

## Review and publication

The dedicated Pi Fab 1.2.5/MOS 3.0.2 profile is staged and stopped at
`.emulator/golem-051`. Its autoexec compiles Hello World and cube.golem on the
emulated Agon, then runs the installed golemrun application. All twelve white
cube edges come from cube.golem; a bottom text viewport preserves the picture
while MOS lists files. [Initial screen](build/runs/review/initial.png).

Oryx was off, so no Oryx graphical session was staged or launched. The portable
package includes `examples/review.txt` for a later emulator or SD-card review.
Human acceptance and explicit publication approval remain open; all coupled
repository changes remain uncommitted. The physical bench stayed off.
