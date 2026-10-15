# GOLEM-044 — Interactive Flower with Golem floating-point mathematics

Reproduce Flower's curve calculation in readable Golem using ordinary floating
division, sine and cosine on the VDP. An AgonDev C++ application supplies a
BASIC-style sequence of parameter prompts, retaining the user's decimal strings
between drawings. This task includes the language/runtime and C-call interface
work required for that application; the curve mathematics belongs on the ESP32.
The Author requested this task and the behavior below on 2026-10-15.

The reference/interaction/integration feasibility tranche and native
mathematical implementation are complete. See the [native math report](GOLEM-044/native-math/README.md)
and initialized [Flower source](GOLEM-044/flower.golem). Real C++ compile/link
and actual ESP32 headless JIT/interpreter checks pass. Stock math, drawing
integration and the interactive application remain ahead; visual staging is
deferred until after the Author's sleep. The native mathematical tranche was accepted and authorized for publication on 2026-10-15.

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
2. Establish a 640x480 application display. The original Flower MOSlet inherits
   its caller's mode; the separate historical demo selects 1024x768. Neither is
   a suitable fixed default for this new requirement. Proposed application mode
   is stock mode 0: 640x480, 16 colours, single-buffered. Verify actual dimensions
   and mode availability. Avoid redundant mode changes when already configured.
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
- ABI-001 [ ] Define the minimal C/C++ entry/input/status/completion interface and generate it from compiler metadata; prove it with an AgonDev C++ consumer.
- MATH-001 [ ] Add and qualify required floating division, degree-to-radian conversion and sin/cos support; choose module linking/runtime services and establish native/stock capability boundaries and accuracy/error rules.
- MTH-001 [x] Implement typed float division/sin/cos/floor, reference evaluation and native emission; explicitly select native output and reject unsupported stock math without changing existing default packages. See the native mathematical capability in the maintained contract.
- MTH-002 [x] Add an opt-in math-library SDK link setting with unchanged default images and loader/runtime ABI; generic SDK ownership is VDPL-002. SDK implementation was accepted and authorized for publication on 2026-10-15.
- MTH-003 [x] Qualify parser/type/error/precedence cases, generated C and full Flower math against independent host references; 1,244 sanitized generated-C cases, C++ sanitizers, three actual assembler consumers, 55 prior unchanged files and three default SDK images pass. First-tranche C++ proof remains valid; complete C transport remains ABI-001.
- MTH-004 [x] Execute installed Xtensa math and relocated packages under both engines, including library argument reduction, cleanup and command-task stack headroom. Pi JIT and matched Oryx JIT/interpreter each pass 278 calls / 27,092 replies, including six Flower parameter sets. This is VDP-only execution; MOS/SD and physical hardware are not qualified by these runs.
- MTH-005 [ ] Stage a finite visual Flower mathematics review on Oryx, identifying the Golem source and surrounding renderer. Deferred by the Author until after sleep (2026-10-15); current work uses headless correctness runs only. Full interactive input remains APP-001.
- DRAW-001 [ ] Integrate portable calculations with VDP-resident connected-line drawing and finite batches, retaining shared storage and useful Escape responsiveness.
- APP-001 [ ] Build the ordinary AgonDev C++ Flower client with sequential prompts, preserved strings, startup mode capture, 640x480 setup and owned resource lifecycle.
- CHK-001 [ ] Compare numeric traces and geometry with the independent higher-precision reference, separating intentional float32 differences from legacy binary16 differences and algorithm mistakes. Test cardinal/fractional degree inputs, all defaults, noninteger/integer step products, shrink cases, domain errors and repeated redraws.
- CHK-002 [ ] Test exact text round trips, Enter-keeps-value, corrections/backspace, rejected values, return-key consumption, every Escape phase including drawing, mode failures and completion-before-unload/restore. Verify retained curves during parameter entry and the agreed exit display policy.
- QUA-001 [ ] Run appropriate maintained host/sanitizer/C++/assembler checks and actual stock/native target qualification for supported capabilities; record module resources, installation and steady-state wire traffic separately. Preserve prior outputs/profiles and distinguish emulator evidence from physical hardware.
- VIS-001 [ ] Stage the complete interactive 640x480 application on a dedicated stopped Oryx profile with readable Golem/client source links. The human review exercises prompts, fractional degrees, multiple curves, retained strings, no-clear re-entry and Escape/mode restoration. Automated runs are both windowless and silent.
- DOC-001 [ ] Record build/run instructions, the user-visible contract, supported numeric limits and exact qualification evidence; promote generic compiler/SDK/C-interface improvements only through their agreed ownership boundaries.
- REV-001 [ ] Obtain applicable human acceptance and explicit publication approval before committing/pushing emulator-related implementation.

First execution tranche, released by the Author on 2026-10-15: REF-001 and UX-001 plus the ABI/math/drawing
feasibility needed to choose a small implementation sequence. Deliver concrete
decisions and initialized source/interface prototypes before starting the full
application. Writing this task is not a claim that its missing features exist.

That tranche is now complete. Next implementation tranche: add and qualify the
native float division/sin/cos/floor slice, preserving current semantics and
outputs, with an explicit unsupported-stock capability boundary. Add an opt-in
SDK math-library link option rather than changing the runtime ABI. Stock float
division remains an investigation within MATH-001, not a completed lowering.
Follow with the resident batch renderer and full C++ interactive client; include
the full visual review when those executable application changes are staged.
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
