# Golem tasks

This is the authoritative register of promoted, unfinished work. Detailed plans
and task artifacts live under `docs/tasks/`.

GOLEM-035 was accepted and explicitly approved for commit/push on 2026-10-13.
Outstanding human/physical reviews below remain open; this checkpoint does not
claim those reviews occurred.


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

- [ ] **GOLEM-002:** Research and develop a standalone C/C++ vdplet facility,
  targeting physical Agon; packed monochrome rendering is the first substantial
  application, with numeric modules and Golem calls later. [Detailed plan](docs/tasks/GOLEM-002.md).
  Loader, numeric and packed modules pass actual Xtensa emulation through
  GOLEM-021. Physical qualification/adoption remains open; flashing is outside
  the current authorization. CAP-001–009 add substantial single-module/Wolf-inspired
  capacity, stress/lifecycle protocol, generic services and upstream-adoption
  investigation. Stage A inventory/resource builds are complete; [numerical proposal](docs/tasks/GOLEM-002/capacity/stage-a/RESULTS.md)
  is followed by accepted GOLEM-037 composition with implemented SDK policy increases and actual-target/human qualification. CAP-001–003 research/documentation complete; broad stress and physical qualification remain open.

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
