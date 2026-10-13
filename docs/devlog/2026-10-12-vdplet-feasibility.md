# vdplet feasibility research — Gate A review

The Author authorized GOLEM-002 Phases A–C. Research recommends a restricted
adaptation of pinned Espressif ELF loading on the original ESP32, with a C service
table and a tiny physical numeric proof before the packed renderer. Firmware
installation would be required once; subsequent module replacement is the aim.
Stop at Gate A with the report, ABI and proof proposal. No implementation or
flashing is authorized by this research tranche.

1. REC-001: SRC-001..003 and RES-001..007 are complete as research deliverables.
   ORG-001 is disposed by a standalone ownership recommendation and prototype
   isolation; agreement before shared integration remains pending at Gate A.
   DEL-001..003 are complete with the [report](../tasks/GOLEM-002/native-vdp-modules-feasibility.md),
   [ABI draft](../tasks/GOLEM-002/vdplet-abi-draft.md), [proof proposal](../tasks/GOLEM-002/physical-proof-proposal.md)
   and [evidence ledger](../tasks/GOLEM-002/research/evidence.md).
2. REC-002: Existing Oryx physical artifacts identify Arduino 2.0.14, IDF 4.4.6,
   GCC 8.4 and physical graphics ac2dd59. Nearby unversioned packages differ.
   Firmware/map/source identities are retained; the previous firmware hash
   matches. No device was queried or independently requalified.
3. REC-003: elf_loader 1.3.4 supports classic ESP32 and declares IDF >=4.4.3.
   Source inspection found bounded-validation and relocation-error adaptation
   needs, plus the open .got.loc import issue. Recommend zero external imports
   and a restricted relocation profile, not arbitrary shared libraries.
4. REC-004: Static linker remainder is 35,868 bytes of its IRAM region, not free
   executable heap. Actual largest blocks, stack and loader/module sizes await
   implementation and physical measurement. Planning envelopes are labelled.
5. REC-005: The packed renderer's existing XOR/RLE page transport is preserved.
   Dynamic loading brings deployment flexibility; no extra compression or
   measured performance gain is claimed. Wire counts separate upload, direct/
   resident calls, changed inputs, administration and optional result retrieval.
6. REC-006: Verified document links/IDs, source identities, field-size and wire
   arithmetic and independent proposed float bit patterns. No tests of native
   execution were possible or attempted under this scope. No firmware/module
   build, hardware/emulator operation, dependency install, other-project edit,
   agent contact, commit or push. Existing unrelated Golem edits are preserved.

TODO.md retains GOLEM-002 awaiting Gate A. Later loading, packed-blit and optional
work remain unstarted; the autonomous-sprite idea remains deferred.

The Author subsequently supplied Flexe's `agon-mos-peer` source. Located the clean
checkout at `/Agon/others/levkropp/flexe`, pinned its identity and reviewed the peer,
bridge, runner and documented Agon gates. [Supplementary notes](../tasks/GOLEM-002/research/flexe-agon-notes.md)
record the significant new option: actual ESP32/Xtensa firmware execution linked
to Fab's eZ80/MOS core. This could permit a pre-hardware test of the same module
payload, while ordinary Fab still runs a host-native VDP. No test was run and no
emulator integration was adopted. Gate A remains pending.
