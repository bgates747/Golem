# GOLEM-018 — First resident geometry and pixel demo

The Author accepted GOLEM-011's geometry scope and authorized implementation.
Implement points, lines and convex quads with packed initialized inputs and named
assembly calls, then stage the dedicated Oryx demo for human review. Assembly
owns display setup, installation, input updates, invocation, pacing and cleanup.

- GEO-001 [x] Implement and document the standalone resident drawing profile.
- CHK-001 [x] Qualify grammar, emission, assembly exports and prior profiles.
- PIX-001 [x] Qualify geometry and persistent updates against stock VDP drawing.
- ORY-001 [x] Stage and verify the Oryx fixture, leaving its emulator stopped.
- HUM-001 [x] Obtain human pixel/MOS keyboard review and verify saved results.

Supporting fixtures and evidence live under `docs/tasks/GOLEM-018/`. Leave changes
uncommitted until human validation and explicit commit approval. This tranche
does not implement general PLOT, bitmap transforms or viewport/scroll operations.

[Qualification and launch instructions](GOLEM-018/README.md): 5,704 pixel pairs,
11,451 checks, 12,589 saved bytes; host/sanitizer, assembler and Oryx headless
checks pass. The Author confirmed dir worked; the agent retrieved and verified
all 12,589 bytes. The Author requested a shape reference after seeing cyan
geometry at the upper left; this is the intended clipping case. A reference is
now supplied in the README. The Author confirmed that reference matches what
they saw. GOLEM-018 is accepted and removed from the active TODO; commit and push were explicitly approved on 2026-10-12.
