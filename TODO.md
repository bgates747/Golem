# Golem tasks

This is the authoritative register of promoted, unfinished work. Detailed plans
and task artifacts live under `docs/tasks/`.

GOLEM-035 was accepted and explicitly approved for commit/push on 2026-10-13.
Outstanding human/physical reviews below remain open; this checkpoint does not
claim those reviews occurred.

- [ ] **GOLEM-051:** Native AgonDev/MOS compiler for all implemented profiles,
  runner and standalone distribution. [Plan](docs/tasks/GOLEM-051.md),
  [results](docs/tasks/GOLEM-051/RESULTS.md), [how-to](docs/tasks/GOLEM-051/HOWTO.md).
  All 316 native comparisons, 13 CLI cases, ten examples and six-binary isolated
  rebuild pass. Oryx stock instance boot/keyboard accepted; distribution walkthrough
  is in progress. GOLEM-053 adds integer bitwise operators to the package.
  Publication was explicitly authorized on 2026-10-27.

- [ ] **GOLEM-050:** Compact stock-matrix runtime loops/indexed access. [Plan](docs/tasks/GOLEM-050.md), [contract](docs/tasks/GOLEM-050/contract.md). Contract v0.2 mutually accepted (SHA406ffc96), frozen/pushed as89b9670; remote/hash verified and receipt sent. Compact implementation/host and bounded actual-stock checks pass; both matrix installs1047bytes with unchanged32-byte float controls. Independent consumer/timing/human gates remain with wireframe; publication authorized on 2026-10-27. Pending GOLEM-049 implementation is excluded from freeze authority.

- [ ] **GOLEM-049:** Bounded maintained stock perspective primitives for wireframe:3x3 inverse, buffer-backed matrix values, float XYZ/XY16 transforms and finite compiled chunks. [Plan](docs/tasks/GOLEM-049.md). Implemented: exact93-byte stock proof,51matrix rejections, ASan/UBSan,68legacy files unchanged, two assembler consumers and2685-vertex source generation pass. Both peer generated streams and full Pi/Oryx MOS application/control/cleanup gates pass; fsim owns the open human gate. Publication authorized on 2026-10-27; human review remains peer-owned.

- [ ] **GOLEM-047:** Native drawing through a generic VDU-byte submission service. [Task](docs/tasks/GOLEM-047.md), [executable report/review](docs/tasks/GOLEM-047/README.md). Runtime ABI1.1 and isolated C++ Corsair consumer pass compatibility, both execution engines, full MOS, matched pixels/wire and size checks. Oryx visual/keyboard review accepted; saved results and actual MOS dir verify; commit/push expressly approved on 2026-10-24. JIT frame times unchanged; interpreter comparison faster. Portable Golem frontend/backend adoption is expressly secondary to VDPlet rendering performance; generic service belongs to agon-vdplet VDPL-003.


- [ ] **GOLEM-045:** Bounded stock-only matrix/vertex interface for fsim implemented; maintained regressions, 26 rejection cases, sanitizer and two assembler consumers pass. Fsim independently qualifies the unchanged compiler on stock VDP: cube and supplied tank each pass1080frames, projection/PLOT/control/reset and cleanup checks. Human visual review remains fsim-owned; compiler publication is explicitly approved. No physical authority. [Task](docs/tasks/GOLEM-045.md).

- [ ] **GOLEM-044:** Interactive Flower native Golem math, resident rendering and AgonDev C++ client implemented; mode 19 (1024x768), white on black requested and implemented. [Task](docs/tasks/GOLEM-044.md), [application/review](docs/tasks/GOLEM-044/app/README.md). Seventeen Oryx headless scenarios pass; dedicated interactive profile staged/stopped for the Author. Mode 19 visual review accepted and commit/push explicitly approved on 2026-10-15. Controlled fault-injection and physical follow-up remain open; saved human log does not establish controlled exit.

- [ ] **GOLEM-036:** D1b faithful wide integer primitives, signed division and checked narrowing.
  [Plan](docs/tasks/GOLEM-036.md). First wide storage/add/subtract/comparison/conversion tranche qualified; human review accepted and saved results verified; commit/push explicitly approved.
  Checked multiplication human review accepted and saved results verified; commit/push explicitly approved; paired division accepted; commit/push explicitly approved. Right-shift human review accepted; saved results verified and commit/push explicitly approved. Golem supplies language features/packages; Rally owns game-specific replay/validation. Wider helper needs remain separately scoped.

- [ ] **GOLEM-039:** Build/deliver Rally's new full-scene native source package using accepted bindings/helper/SDK. [Plan](docs/tasks/GOLEM-039/README.md). Delivered through Golem twenty-eight: new package 5a3ef7b3..., loader/layout and bounded host checks pass; Oryx export verified. Rally reports identical client package and passing 60-frame target smoke; full-scene/human qualification remains peer-owned. Golem package publication explicitly approved. Graphical startup stall remains unresolved; [offline UART correlation](docs/tasks/GOLEM-039/uart-investigation/README.md) returned to Rally, no runtime repair applied.

- [ ] **GOLEM-038:** D3 scheduler/snapshot/collision-event design and ownership gates.
  [Plan](docs/tasks/GOLEM-038.md). [Generic runtime review](docs/tasks/GOLEM-038/runtime-findings.md) complete: existing finite calls/lifetimes are reusable; scheduler/publication/events/quiescence remain Rally-owned service design. Await Rally draft and Author policy review; no generic feature implementation released.

- [ ] **GOLEM-023:** Research friendly Golem source compiled to native VDPlets:
  compare native library calls, explicit native procedures, whole native modules
  and automatic mixed compilation; recommend a bounded first milestone.
  [Research task](docs/tasks/GOLEM-023.md). First research tranche is ready:
  [recommendation, initialized example and assembly interface](docs/tasks/GOLEM-023/README.md).
  Generated C remains the candidate native route. The accepted shared-source,
  stock/native portability requirement is in [the requirements/roadmap](docs/design/backend-portability.md);
  the first common implementation is GOLEM-024.

- [ ] **GOLEM-043:** Investigate concurrent native computation alongside buffered commands. [Research plan](docs/tasks/GOLEM-043.md). Deferred follow-up: possible overlap gains versus buffer/drawing races, lifecycle, resource and timing problems. Filed only; research execution and implementation are not released.

- [ ] **GOLEM-002:** Research and develop a standalone C/C++ vdplet facility,
  targeting physical Agon; packed monochrome rendering is the first substantial
  application, with numeric modules and Golem calls later. [Detailed plan](docs/tasks/GOLEM-002.md).
  Loader, numeric and packed modules pass actual Xtensa emulation through
  GOLEM-021. Physical qualification/adoption remains open; flashing is outside
  the current authorization. CAP-001–009 add substantial single-module/Wolf-inspired
  capacity, stress/lifecycle protocol, generic services and upstream-adoption
  investigation. Stage A inventory/resource builds are complete; [numerical proposal](docs/tasks/GOLEM-002/capacity/stage-a/RESULTS.md)
  is followed by accepted GOLEM-037 composition with implemented SDK policy increases and actual-target/human qualification. CAP-001–003 research/documentation complete; broad stress and physical qualification remain open.
  CAP-006 color framebuffer/continuous PCM follow-up is planned in
  [agon-vdplet VDPL-005](../agon-vdplet/docs/tasks/VDPL-005.md); the Author approved
  planning/mailbox consultation and later implementation on 2026-10-28. Public
  interfaces must be generic across modes/formats; mode20 is a test case.

- [ ] **GOLEM-021:** Eight-hour Flexe/Fab hybrid emulator, working native vdplets,
  then service offer to agon-6502; optional Golem tranche afterward.
  [Plan](docs/tasks/GOLEM-021.md). Hybrid/native proofs and the owner's F2/F3
  codec pass both engines; service evidence delivered. Oryx visual review is
  staged/stopped. Human acceptance remains pending; commit/push is authorized.

- [ ] **GOLEM-022:** Multiple finite named drawing Subs sharing resident globals,
  with named assembly calls and unchanged single-Sub packages.
  [Plan](docs/tasks/GOLEM-022.md). Implemented; host/sanitizer/parity/assembler
  and focused stock Oryx checks pass. Separate visual review is staged/stopped;
  human acceptance remains pending; commit/push is authorized.
