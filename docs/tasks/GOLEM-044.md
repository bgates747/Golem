# GOLEM-044 — Interactive Flower with Golem floating-point mathematics

Reproduce Flower's curve calculation in readable Golem using ordinary floating
division, sine and cosine on the VDP. An AgonDev C++ application supplies a
BASIC-style sequence of parameter prompts, retaining the user's decimal strings
between drawings. This task includes the language/runtime and C-call interface
work required for that application; the curve mathematics belongs on the ESP32.
The Author requested this task and the behavior below on 2026-10-15.

The native mathematics and resident renderer are accepted and published. The
[complete C++ application](GOLEM-044/app/README.md) now passes seventeen Oryx
headless scenarios, with its dedicated interactive profile staged and stopped.
Read the [Golem source](GOLEM-044/flower.golem) and [actual client](GOLEM-044/app/src/main.cpp).
Human visual acceptance and publication approval were given on 2026-10-15; stock mathematical
lowering, controlled fault injection and physical behavior are not claimed.

## Required application behavior

### Parameters and retained text

Use the actual original parameter order and these exact initial strings:

| Order | Parameter | Initial string | Meaning |
| --- | --- | --- | --- |
| 1 | petals | `3.03` | Radius oscillations per revolution; fractional values allowed |
| 2 | vectors | `1.98` | Sampling points per oscillation; fractional values allowed |
| 3 | depth | `0.6` | Radial modulation depth |
| 4 | periods | `66` | Revolutions used to determine the segment count |
| 5 | shrink | `0.8` | Fraction of initial base radius removed over the nominal run |
| 6 | radius_scale | `512` | Radius in the original Flower graphics units |
| 7 | theta_init | `0` | Initial angle, entered in decimal degrees |

1. Run as an interactive application, with parameters entered while it is
   running. Use AgonDev C++ for the eZ80 client. Prompt for one parameter, accept
   it, then prompt for the next. MOSlet/command-line argument parsing is not the
   interaction model.
2. Show the retained string as each field's current/default value. Proposed
   simple interaction: `Petals [3.03]:`; Enter with no replacement keeps that
   string and advances. Support normal bounded line editing and backspace.
3. Each accepted edit replaces that field's retained string. Keep the exact
   accepted numeric spelling, including decimal places and trailing zeros.
   For example, `3.0300` remains `3.0300` on the next entry cycle. Parsed numeric
   values are separate from display strings. Never regenerate the retained text
   from binary floating point or expand `3.03` into its binary approximation.
4. Seed defaults once at application launch. Retain accepted strings across
   parameter-entry/drawing cycles for the application's lifetime. Invalid input
   reports a useful field error and permits correction without losing previous
   accepted values or clearing the displayed curve.
5. All user-facing angles use decimal degrees on a 360-degree circle. Accept
   fractional degrees and make 360 degrees equivalent to 0 for the mathematics,
   while preserving the entered string. Use 0..360 inclusive for the initial
   angle field unless the Author later requests wrapping of other values.
   Golem converts degrees to radians before calling the trigonometric routines.

### Screen and keyboard lifecycle

1. At startup, detect and save the actual active screen mode before changing
   display state. Do not assume MOS started in a particular mode. Save relevant
   mode flags and use fresh mode information rather than stale cached values.
2. Establish mode 19: 1024x768, four colours, single buffered, with white text
   and curves on black. This is the Author's 2026-10-15 revision of the earlier
   640x480 requirement. Verify the actual mode and dimensions; preserve the
   existing legacy family and restore the original full mode byte on exit.
3. After the final parameter has been accepted and the complete parameter set
   passes validation, clear the screen and draw the curve. Do not clear early
   merely because the user has begun entering replacements.
4. Once drawing completes, any key other than Escape returns to the first
   parameter prompt, with all retained strings available. Do not clear the
   screen or change mode: the previous curve remains visible while the user
   enters the next set. Consume the return-triggering key so it does not become
   accidental input for the first field.
5. Keep prompt output/editing within a bounded text area or overlay that avoids
   scrolling/clearing the whole image. Some prompt text may cover part of the
   curve; returning to input must not erase the curve as a whole. Clear for the
   next drawing only after its final valid parameter.
6. Escape at any point exits: during parameter editing, validation, drawing or
   the post-draw wait. Stop submitting work, finish any already-running bounded
   batch, release owned resources and return to MOS. Clearing the screen on exit
   is an explicitly approved exception (2026-10-15), including the clear inherent
   in restoring the original mode. Non-Escape keys during drawing do not interrupt it.
7. Restore the saved pre-launch screen mode on application exit, including
   controlled error exits after display setup. Restore usable cursor/input
   behavior. Never restore a hardcoded mode or leave module/buffer ownership
   attached to a terminated client.

**Exit policy approved on 2026-10-15:** the current VDP `vdu_mode()` clears the
screen even for a repeated mode command. The Author explicitly permits clearing
on exit, so restoring the saved mode takes precedence over preserving the curve
at exit. No screen snapshot/redraw mechanism is required for this exception.
Returning to parameter entry must still preserve the image without a mode
change or full-screen clear.

## Mathematical behavior and precision

Use the original BBC Flower algorithm as the primary logic reference, with the
binary16 implementation as additional provenance and test material. The latter
intentionally changes arithmetic precision, angle representation, radius update
and segment count. Those adaptations are not the target of this port.

Let P, V, D, T, S, R and A denote petals, vectors, depth, periods, shrink,
radius_scale and theta_init respectively. The original source calculates:

1. Raw step count Q = P * V * T and integer count N = floor(Q).
2. Main angle increment = 2*pi / (P*V); petal angle increment = 2*pi / V.
3. Initial main angle = A*pi/180; initial petal angle = 0.
4. Initial base radius R0 = R / (1+D).
5. Base-radius increment = -(S*R0)/Q, using the raw count before truncation.
6. At each point, radius = base_radius * (1 + D*cos(petal_angle)),
   x = radius*cos(main_angle), y = radius*sin(main_angle).
7. Move to the initial point, then advance both angles and the base radius and
   draw successive connected line segments. The original BBC loop is inclusive
   and draws N+1 segments; the binary16 variant draws N segments. Preserve and
   explicitly test the selected original behavior, especially integer Q cases.

Use the source's actual operation order when comparing finite-precision
results. Any numerically improved reformulation, such as computing the radius
directly from the point index to reduce accumulation error, must be documented
and reviewed as a deliberate change rather than slipped in with the port.

1. Add normal float division and sin/cos support needed by readable Golem source.
   Use appropriate standard floating-point mathematics on ESP32. The installed
   Xtensa toolchain contains sinf/cosf and sin/cos, and VDP rotation code already
   uses sinf/cosf. Their availability in firmware does not automatically export
   them to dynamically loaded modules.
2. Select compatible library linking or versioned runtime math services after
   checking module relocation, dependencies, stack/image costs and ownership.
   Do not hardcode firmware addresses or silently depend on unresolved imports.
   Generic SDK/runtime changes belong in agon-vdplet; compiler changes in Golem.
3. Begin the design with the maintained float32 type, standard floating-point
   operations and explicit error/accuracy rules. Compare against a higher
   precision reference and the original BBC implementation. Evaluate whether
   float32 is sufficient for the intended parameters; propose float64 explicitly
   if needed. Do not claim that float32 necessarily matches BBC's precision or
   reproduce binary16's rounding. Round to drawing coordinates only at the
   defined final conversion boundary.
4. Convert user-entered degrees on the VDP. Avoid binary16 calculations,
   application-level coarse trig lookup tables, quantized phase accumulators,
   host-precomputed reciprocals or precomputed curve coordinates as substitutes
   for missing language features. The client's numeric parsing is input handling;
   the curve's division, trig, angles and radius calculations execute in Golem.
5. Retain original logical graphics units and the centred origin, or publish a
   mathematically equivalent mapping into the 640x480 display. Original origin
   is (640,512) in the 1280x1024 logical coordinate system. Do not reinterpret
   the default radius 512 as 512 physical pixels and inadvertently crop the
   intended default curve. Document axis direction, scaling, clipping and
   signed coordinate conversion.
6. Validate positive petals/vectors/periods, depth > -1, radius >= 0 and finite
   values/derived quantities. Establish explicit useful limits on work and
   coordinates, including large inputs and shrink values outside 0..1. Do not
   inherit 65504 or decimal-exponent restrictions solely from binary16.
   Preserve the existing controlled Golem error model and retained input text.

## Golem/backend and C/C++ integration

1. Keep one computational source and a shared typed state/call layout. Maintain
   the accepted stock/native portability direction. The native ESP32 path is
   the initial Flower target; plan and qualify stock lowering for new portable
   math operations without silently substituting lower-precision algorithms.
   Any interim native-only capability must be explicit and rejected by an
   unsupported stock target, rather than advertised as portable completion.
2. Make drawing usable with the arithmetic in this program. The current
   resident drawing profile cannot be mixed directly into portable calculations.
   Select and qualify an integrated line-drawing path with the original initial
   move/connected-segment behavior. Keep per-point curve work and drawing on
   the VDP; the eZ80 client must not stream a newly calculated coordinate pair
   for every point.
3. Use finite bounded drawing batches, with completion/status visible to the
   client, so it can service Escape and normal VDP processing can resume. Choose
   and measure a batch size that gives useful cancellation responsiveness.
   Existing constant-bound Range batches plus a retained remaining-count/done
   state may suffice; dynamic loop syntax is needed only if a concrete remaining
   gap warrants it. No background worker or GOLEM-043 concurrency dependency.
4. Provide a thin C-callable interface for named entries and typed inputs,
   callable from the AgonDev C++ client. Golem executes on the VDP; a C/C++ call
   submits its generated command entry rather than invoking ESP32 code on the
   eZ80. AgonDev already exposes `vdp_adv_call_buffer()` for the basic six-byte
   call. Existing generated assembly wrappers/equates are not yet a C header.
5. Generate or otherwise derive C entry IDs, field offsets/types and package
   lengths from the same compiler layout used by assembly/JSON. Avoid handwritten
   duplicated offsets and unsafe casts of native C structs onto packed storage.
   Verify AgonDev integer/float widths, byte order, linkage and any assembly ABI
   boundary with a real C++ consumer. Do not add Golem Functions/parameters/locals
   merely to hide a host-wrapper gap.
6. The client owns installation, state updates, calls, synchronization and
   cleanup. Returning from a six-byte submission is not VDP completion. Define
   bounded completion/error handling before unload, mode restoration or another
   batch. Do not replay failed drawing automatically, evict another application's
   native module, or flash firmware as part of startup. Specify unavailable/
   busy runtime behavior and separately qualify any automatic capability probe.

## Work tranches

- REF-001 [x] Audit and pin original Flower formulas, operation order, segment count, shrink behavior, defaults and coordinate mapping; define reference cases and the numeric precision contract. See the reference audit and 16-case model receipt; actual BBC/Xtensa numeric comparisons remain CHK-001/QUA-001.
- UX-001 [x] Specify the interactive field editor/state transitions, exact string retention, blank-entry defaults, validation and Escape behavior, including the approved exit-clear exception. See interaction.md; implementation/human tests remain APP-001/CHK-002/VIS-001.
- FEA-001 [x] Generate a task-local C metadata/setter/named-entry header and compile/link a real AgonDev C++ consumer at default/relocated layouts. Host C/C++ sanitizer checks pass; runtime installation/completion transport remains ABI-001.
- FEA-002 [x] Probe ordinary Xtensa division/sinf/cosf linking and actual unchanged-loader admission. Static libm/libgcc candidate admits without imports or runtime ABI changes; target execution/stack/accuracy remains MATH-001/QUA-001.
- FEA-003 [x] Select a finite resident-array/VDU renderer bridge and write the initialized Flower prototype, with pending math operations labeled. Full drawing integration and wire/latency measurements remain DRAW-001.
- ABI-001 [x] Define the minimal C/C++ entry/input/status/completion interface and generate it from compiler metadata; prove it with an AgonDev C++ consumer.
- MATH-001 [ ] Add and qualify required floating division, degree-to-radian conversion and sin/cos support; choose module linking/runtime services and establish native/stock capability boundaries and accuracy/error rules.
- MTH-001 [x] Implement typed float division/sin/cos/floor, reference evaluation and native emission; explicitly select native output and reject unsupported stock math without changing existing default packages. See the native mathematical capability in the maintained contract.
- MTH-002 [x] Add an opt-in math-library SDK link setting with unchanged default images and loader/runtime ABI; generic SDK ownership is VDPL-002. SDK implementation was accepted and authorized for publication on 2026-10-15.
- MTH-003 [x] Qualify parser/type/error/precedence cases, generated C and full Flower math against independent host references; 1,244 sanitized generated-C cases, C++ sanitizers, three actual assembler consumers, 55 prior unchanged files and three default SDK images pass. First-tranche C++ proof remains valid; complete C transport remains ABI-001.
- MTH-004 [x] Execute installed Xtensa math and relocated packages under both engines, including library argument reduction, cleanup and command-task stack headroom. Pi JIT and matched Oryx JIT/interpreter each pass 278 calls / 27,092 replies, including six Flower parameter sets. This is VDP-only execution; MOS/SD and physical hardware are not qualified by these runs.
- MTH-005 [x] Disposed by staging the complete interactive Flower application under VIS-001 after the Author released Oryx review. The scene identifies Golem mathematics, C++ input/calls and resident VDP drawing; human acceptance remains REV-001.
- DRAW-001 [x] Integrate native portable calculations with a stock VDP-resident connected-line bridge and finite 32-point batches. Pi JIT/interpreter qualify rendering, shared storage and bounded batch duration. Actual Escape/input behavior remains APP-001/CHK-002; see renderer/README.md.
- RND-001 [x] Generate a task-local stock command renderer from Flower's common metadata: own disjoint buffers, patch start/end coordinates residently, draw only the valid batch prefix and suppress zero/error/invalid-count batches without modifying mathematical state.
- RND-002 [x] Qualify count boundaries 0/1/31/32/33, stale array tails, signed coordinates, clipping, repeated calls, failure suppression and cleanup retention; assemble the generated symbolic interface at default and relocated layouts.
- RND-003 [x] Run actual uploaded Flower plus resident rendering on the Pi 5 headlessly. Compare complete curves with direct stock rendering from observed coordinates and independent geometry expectations, including default/fractional/full-circle/integer/shrink cases. Record finite batch duration and steady wire bytes separately from diagnostics.
- RND-004 [x] Document renderer ownership, logical-coordinate mapping, completion and error boundaries, retained evidence and remaining C++/human work. Leave this new tranche uncommitted pending review.
- CLI-001 [x] Generate a native-only C interface and embedded application package from the same compiler/renderer metadata, preserving the earlier ABI proof.
- CLI-002 [x] Implement one ISR-safe keyboard/reply demultiplexer, bounded discovery/completion, installation/binding and owned cleanup with saved-mode restoration.
- CLI-003 [x] Build the actual AgonDev C++ field editor and drawing lifecycle with exact retained strings, domain validation, no-clear re-entry and Escape in every phase; keep batching internal.
- CLI-004 [x] Qualify shared parsing/editing under host sanitizers and execute the real eZ80 client with scripted SDL key input under both engines. Check repeated curves, key release, mode restoration and return to usable MOS; retain evidence.
- CLI-005 [x] Stage the qualified application/source in a stopped dedicated Oryx profile, document launch/review commands and complete technical documentation. Leave new emulator-related changes uncommitted for human review.
- APP-001 [x] Build the ordinary AgonDev C++ Flower client with sequential prompts, preserved strings, startup mode capture, 640x480 setup and owned resource lifecycle.
- CHK-001 [ ] Compare numeric traces and geometry with the independent higher-precision reference, separating intentional float32 differences from legacy binary16 differences and algorithm mistakes. Test cardinal/fractional degree inputs, all defaults, noninteger/integer step products, shrink cases, domain errors and repeated redraws.
- CHK-002 [ ] Test exact text round trips, Enter-keeps-value, corrections/backspace, rejected values, return-key consumption, every Escape phase including drawing, mode failures and completion-before-unload/restore. Verify retained curves during parameter entry and the agreed exit display policy.
- QUA-001 [x] Run appropriate maintained host/sanitizer/C++/assembler checks and actual stock/native target qualification for supported capabilities; record module resources, installation and steady-state wire traffic separately. Preserve prior outputs/profiles and distinguish emulator evidence from physical hardware.
- VIS-001 [x] Stage the complete interactive 640x480 application on a dedicated stopped Oryx profile with readable Golem/client source links. The human review exercises prompts, fractional degrees, multiple curves, retained strings, no-clear re-entry and Escape/mode restoration. Automated runs are both windowless and silent.
- DOC-001 [x] Record build/run instructions, the user-visible contract, supported numeric limits and exact qualification evidence; promote generic compiler/SDK/C-interface improvements only through their agreed ownership boundaries.
- REV-001 [x] Author accepted the mode 19 visual review and explicitly approved commit/push on 2026-10-15. Saved human log lacks a controlled exit; exit/keyboard qualification remains automated evidence.

First execution tranche, released by the Author on 2026-10-15: REF-001 and UX-001 plus the ABI/math/drawing
feasibility needed to choose a small implementation sequence. Deliver concrete
decisions and initialized source/interface prototypes before starting the full
application. Writing this task is not a claim that its missing features exist.

The feasibility and native mathematical tranches are accepted and published as
f17f805 and 52ed327; SDK library linking is published as 580a5d8 in agon-vdplet.
The following resident renderer tranche now passes Pi headless qualification;
see [its report](GOLEM-044/renderer/README.md). The complete C transport
and interactive application (ABI-001/APP-001) are now implemented and staged. Stock float division remains an
investigation within MATH-001, not a completed lowering. Include full visual
review when executable application changes can be staged under the Author's
current host/visual-review instructions.
The Author subsequently deferred visual staging until after sleep and directed
headless emulator correctness checks in the meantime. Do not stage or launch a
visual review during this interval; this supersedes the earlier same-tranche
staging instruction for the mathematical slice.

## Artifact layout and acceptance

Keep task-local source, client, build files, reports, fixtures and evidence under
`docs/tasks/GOLEM-044/`, including an AgonDev-style `client/src/`, generated build
storage and separately ignored emulator profiles. Use the project .venv for
auxiliary tools; build the compiler with the host C++ toolchain, the client with
AgonDev and the module with the pinned Xtensa SDK. Preserve the original Flower
projects as references. Promote a useful example/application only when agreed.

Acceptance is an interactive Flower application whose curve computation comes
from readable Golem using the agreed floating-point math, with the specified
degrees input, exact retained decimal strings, complete keyboard lifecycle,
640x480 display and agreed restoration behavior. A familiar-looking picture
alone does not establish numerical or UI correctness. Include the entire visual
review in its implementation tranche; do not stop with staging still unfinished.

## References

1. [Flower overview and defaults](/Agon/mystuff/agon-utils/examples/flower/README.md).
2. [Original BBC initialization and loop](/Agon/mystuff/agon-utils/examples/flower/bbc/src/flower.asm:270) and [curve routine](/Agon/mystuff/agon-utils/examples/flower/bbc/src/flower.asm:460).
3. [Binary16 implementation](/Agon/mystuff/agon-utils/examples/flower/bs/src/main.inc:88) and [provenance](/Agon/mystuff/agon-utils/examples/flower/bs/PROVENANCE.md).
4. [Current portable contract](../design/portable-compiler.md), [backend portability requirements](../design/backend-portability.md), and [separate resident drawing profile](../design/resident-drawing.md).
5. [AgonDev C buffer call](/Agon/agondev/src/lib/libvdp/vdp_adv_call_buffer.c:9), [screen-mode query](/Agon/agondev/src/lib/libmos/mos_getsysvar_scrMode.src:6), and [C++ build conventions](/Agon/mystuff/agondev-tests/README.md).
6. [VDP mode command clears the screen](/Agon/agon-vdp/video/vdu.h:299), [stock screen modes](/Agon/agon-docs/docs/vdp/Screen-Modes.md:70), and [existing firmware trig use](/Agon/agon-vdp/video/vdu_buffered.h:1657).
7. [Current module builder/link boundary](/Agon/mystuff/agon-vdplet/tools/build_module.py:49) and [public runtime services](/Agon/mystuff/agon-vdplet/include/vdplet.h:69).

## Resident renderer tranche — released 2026-10-15

The Author accepted the native mathematical slice, authorized its commit/push,
and released the next tranche. Execute RND-001 through RND-004 as DRAW-001's
renderer integration: finite 32-point batches, stock VDU drawing, resident
coordinates and a generated task-local interface. Keep the complete interactive
client for APP-001. Use only the Pi 5 for emulator work and leave Oryx untouched
during controlled thermal testing. Visual staging remains deferred; qualification
in this tranche is windowless and silent, with no physical flashing.

RND-001..004 are complete: both Pi engines pass 24,883,200 pixel comparisons
each, eight full curves each, two assembler consumers and sanitized module
admission. All 200,610 retained reply bytes and all curve images match between
engines. Renderer installation is 2,243 wire bytes / 2,187 resident bytes.
A measured compute/status/render/remaining/plot-completion sequence uses 49
outbound / 30 inbound bytes per batch, with no point coordinates transmitted.
The 92 measured batches per engine return in 2.79..7.80 ms of modeled guest
time; this is not physical UART or Escape-latency qualification. No Oryx access,
visual staging, MOS client run or physical deployment occurred. The renderer
tranche was subsequently accepted and published as 4b18679.

## C++ client tranche — released 2026-10-15

The Author accepts retaining the transparent 32-point batch design and explicitly
authorizes publication of the renderer. Execute ABI-001/APP-001/CHK-002 and
applicable QUA/VIS/DOC items: implement the actual interactive application,
headlessly qualify it, and stage a stopped dedicated Oryx review instance in
the same tranche. Oryx is available again. Never launch its interactive session
for the Author; provide CLI launch commands and expected review behavior.

## C++ application result — 2026-10-15

The renderer is published as `4b18679`. The actual application is implemented
and staged, stopped, on Oryx at `docs/tasks/GOLEM-044/.emulator`. Launch only its
generated `./fab-agon-emulator`. See [application/source/review instructions](GOLEM-044/app/README.md)
and [saved qualification](GOLEM-044/app/evidence/summary.json). The user keeps
seven exact strings; batching is internal, with no settings or point traffic.
The complete executable is 34,760 bytes, including the unchanged 13,668-byte
native mathematical image. Compiler/SDK/firmware/emulator source is unchanged.

Seventeen headless scenarios pass: both-engine full edit/redraw/error workflows,
startup Escape, seven prompt exits, a long-curve cancellation, legacy mode
selection, relocated buffers, full-circle input, length correction, busy slot
and absent runtime. There are 2,580,480 exact playfield pixel comparisons; all
four settled workflow pictures and the application log match across engines.
Ninety-five complete batches independently measure 49 outbound bytes in wire
transcripts. Sixteen native scenarios include a successful MOS dir after exit;
the stock control separately checks controlled return, original mode and screen.
The application preserves MOS physical key/release state while withholding its
characters, and handles non-ASCII keys such as F1 for re-entry.

CHK-002 remains open only for deliberately injected missing replies, hung
modules and mode failures. These are documented boundaries, not claimed passes;
normal input, retained text, corrections, failure-preserved drawing, full saved
mode restoration and controlled cleanup are qualified. Native calls/ownership
that cannot be confirmed retain resources and require reset. Human review and
explicit publication approval remain REV-001. New application/profile changes
are uncommitted. No graphical instance or physical flashing was performed.


### Flower display revision — 2026-10-15

The Author replaced the earlier 640x480 display preference with mode 19:
1024x768, four colours, white text and curves on black. The application checks
mode/dimensions, retains the original mode family and restores the saved mode
on exit. Golem math, logical coordinates and resident bridge bytes are unchanged.
The four-row prompt pane derives its placement from the actual text dimensions.
Current headless qualification and images live under
`docs/tasks/GOLEM-044/app/evidence/mode19/`; earlier 640x480 evidence is historical.
Review uses the stopped task profile's `./flexe-agon-emulator` generated wrapper.
No graphical instance is launched by the agent; changes remain uncommitted.


### Acceptance and publication — 2026-10-15

The Author accepted the mode 19 white-on-black review and explicitly requested
commit and push of all pending work. REV-001 is satisfied. Retrieval of the
saved human log found no controlled Escape exit, so no separate human exit or
MOS keyboard pass is claimed; the 17 passing headless scenarios retain the
independent lifecycle evidence. Earlier pending-publication statements above
are historical and superseded by this approval. Controlled fault injection
and physical qualification remain open.
