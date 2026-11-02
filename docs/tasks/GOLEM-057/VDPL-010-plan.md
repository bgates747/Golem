# VDPL-010 — Independent pixel and PCM feature composition

Allow applications to use generic colour pixels without installing the larger
PCM/queue engine, optionally alongside the small copied PCM sink. Preserve old
table layouts, existing helpers, firmware selections and wire protocol. This
implements GOLEM-057 PIX-001 and documents existing discovery/fence behavior.

## Contract

1. Add `vdplet_pixels_host` and `vdplet_pcm_host`, each checking ABI1.3 extent,
   its own capability bit and its own required callbacks. Preserve the combined
   `vdplet_media_host` and all field offsets/structure sizes.
2. Add `--features pixels` (runtime mask31) and `pixels-lean` (mask159). Existing
   full127, base15, lean143, stock and audio selections/defaults stay unchanged.
   Pixels-only excludes richer PCM/queue code; pixels-lean adds copied sink128.
   Preserve exact legacy minor0/1/2 views and report compiled optional services
   explicitly in later views. New combinations use existing capability bits.
3. Use the existing `surface_info`/`pixels_write` source formats, rectangles,
   copying/phase/error/lifetime behavior. No new wire operation or native worker.
   A module needing buffers+pixels declares17/minor3; pixels+sink145/minor5.
4. Keep admission/stack/lease limits, old builder default image outputs and
   unrelated source unchanged. Add tests for independent helpers/profile
   selection, actual module admission/missing-capability rejection and existing
   legacy modules. Qualify real copied pixels and pixels+sink on Oryx under the
   central exclusive lease; human review remains required before publication.
5. Document INFO18–23 and conditional ordered GP draw draining with source
   caveats. Native pixel copying, queued VDU interpretation, raster completion,
   page presentation and audio consumption remain distinct.

## Work

- PLN-001 [ ] Record plan and task register before implementation; preserve
  baseline source/build evidence at runtime2541f10.
- SDK-001 [ ] Add independent optional accessors and rejection/compatibility tests.
- BLD-001 [ ] Add pixel and pixel+sink selections with minimum headers/hooks,
  preserving old selections/configurations and actual capability views.
- DOC-001 [ ] Update maintained build/API documentation and existing INFO/fence fields.
- CHK-001 [ ] Run host/sanitizer/admission and real toolchain checks; verify old
  behavior and independently absent callbacks/capability refusals.
- TGT-001 [ ] Build matched firmware and report linked flash/DRAM/IRAM costs;
  qualify actual uploaded modules, source reuse, conflicts and cleanup on Oryx.
- HUM-001 [ ] Stage meaningful stopped human review with readable source and
  expected behavior, obtain acceptance before committing coupled work.

Generic runtime artifacts belong under docs/tasks/VDPL-010/ in agon-vdplet.
GOLEM-057 owns coordination only; this compiler repository is not a production
dependency. No physical flashing or graphical launch is authorized.
