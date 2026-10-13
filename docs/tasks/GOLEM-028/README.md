# Typed Boolean values and a traffic-light review

The task-local compiler now supports initialized bool globals, true/false,
Boolean copies, assignment from all six uint16 comparisons and Boolean If/ElseIf
conditions on both backends. Host, stock VDP and actual Xtensa checks pass. The
visibly distinct traffic-light review was accepted by the Author. Retrieved
human results match all 533 observations/5,249 bytes. The Author explicitly approved commit/push on 2026-10-13; keyboard success
was not separately stated.

## Review

[lights.golem](lights.golem) is the complete initialized program. Golem stores
comparison results in red/amber/green, copies green into moving, and uses Boolean
conditions to choose lamp colours. A resident VDU routine draws the lamps;
assembly selects buffered/native execution and paces with VSYNC. The screen
identifies these roles. [Captured final screen](review-screen.png).

You should see a vertical traffic light cycle red, amber, green, then repeat.
Inactive lamps are grey. Backend labels alternate BUFFERED/NATIVE every twelve
frames, sharing the same Boolean/numeric state. After 600 frames the amber lamp
remains lit above MOS. This replaces the moving-square/meter pattern for this task.

On Oryx:

```bash
cd ~/Agon/mystuff/golem/docs/tasks/GOLEM-028/.emulator
./fab-agon-emulator
```

Follow the single onscreen keyboard instruction, close the emulator and report
whether it worked. The agent retrieves/verifies the saved results; no second
terminal or verifier is needed. The launcher removes previous results for a
safe rerun. Readable source and this report are deployed beside the profile.

## Language and storage

Globals require explicit true/false initializers. Within a Sub, a bool may receive
a literal, another bool or a uint16 comparison. Conditions accept those same
Boolean expressions, including a bare flag. Numeric arithmetic parentheses remain
supported inside comparison operands. Whole-Boolean parentheses and comparisons
between Boolean operands are outside this slice.

There are no implicit numeric/Boolean conversions, numeric truthiness or bool
arithmetic. And/Or/Not and other compound Boolean operators remain deferred.
Keywords and true/false are case-insensitive; identifiers retain case sensitivity.
Existing uint16 arithmetic, branches and bounded Range loops remain supported.

For this experimental profile a bool occupies two bytes with canonical values
0=false and 1=true, and distinct assembly type tag 6. This is not VBA's -1
representation. Invalid host-supplied encodings such as 2, 255, 256 and 65535
produce entry admission status 32 before source work. No coercion is performed.
An eventual storage change would require an explicit new compatible layout.

Comparison operands evaluate once left to right. If an operand fails, the Boolean
destination and subsequent statements are preserved/skipped according to the
existing error contract; earlier assignments remain committed. Selected/skipped
branches, loops and cleanup preserve the same semantics on both backends.

The traffic-light state is 20 bytes, with four Boolean fields. Both backends
share it; install the published initializer explicitly. Buffered installation
is 5,156 bytes. Native package size is 1,536 bytes, including 624 bytes of resident
text and four bytes data. Normal frames still send six-byte arithmetic and drawing
calls plus the three-byte VSYNC swap; review state queries/labels add traffic.

This task forks accepted GOLEM-027 in its own silo. Production compiler,
historical task artifacts, generic runtime/SDK and firmware remain unchanged.

## Build and evidence

From the project root:

```bash
.venv/bin/python docs/tasks/GOLEM-028/host_checks.py
.venv/bin/python docs/tasks/GOLEM-028/fixture.py all
.venv/bin/python docs/tasks/GOLEM-028/stage_oryx.py
```

The host compiler is C++; the project Python environment runs auxiliary tools.
Ignored build directories contain common interfaces/state, typed program trees,
command streams, assembly wrappers, native C and SDK-packaged Xtensa modules.
Canonical setup tooling generates isolated profiles with runtime/fixture hashes.
Legacy variant name meter-stock now runs numerical checks of lights.golem only.

[Host receipt](host-checks.json): 4,804 model/generated-C cases, 24 grammar/reference
acceptances, 64 rejections, ASan/UBSan, ELF admission and maintained checks.
Earlier arithmetic/condition/Range fixtures remain in the regression suite.
Cases include every comparison around byte boundaries/65535, Boolean copying,
literals, loops, skipped/selected failures, preserved destinations and invalid
external encodings. Host checks took about 56 seconds on the Pi.

[Target evidence](evidence/):

| Fixture | Named calls | Observations | Bytes |
| --- | ---: | ---: | ---: |
| Stock Boolean checks | 64 | 65 | 1,170 |
| Stock lamp logic/bounds | 24 | 25 | 500 |
| ESP32 Boolean checks, each engine | 128 | 1,165 | 11,650 |
| ESP32 relocated checks, each engine | 128 | 1,165 | 11,650 |
| ESP32 visual review, each engine | 600 | 533 | 5,249 |

JIT and interpreter pass. Full state is observed every twelve frames and after
private cleanup; all 56,320 final playfield pixels and nine guest pixel queries
match. The guest returns to actual MOS. Final words are frame 60, ticks 600,
redColour 21, amberColour 52, greenColour 21, red false, amber true, green false,
moving false, error 0.

Oryx review took about 19–21 seconds including startup/loading; numerical suites
took about ten seconds per ESP32 engine. Timing is functional emulator evidence,
not physical performance or perfectly paced presentation. No board was flashed
or graphical emulator launched by the agent.
