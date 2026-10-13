# Golem tasks

This is the authoritative register of promoted, unfinished work. Detailed plans
and task artifacts live under `docs/tasks/`.

GOLEM-032 was accepted and explicitly approved for commit/push on 2026-10-13.
Outstanding human/physical reviews below remain open; this checkpoint does not
claim those reviews occurred.

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
  the current authorization.

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
