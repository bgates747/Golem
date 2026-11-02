# Direct framebuffer audit and owner handoff

Wolf's production renderer still copies a complete colour output image into
the drawing framebuffer. The generic direct-row mechanism already exists and
its bounded Wolf prototype uses it correctly, but that prototype was not a
production migration. FSIM's current live native renderer does render into the
borrowed rows. Root237 sends Wolf the concrete correction; Wolf owns its renderer
changes and tests. Golem retains generic service and documentation ownership.

## Actual paths inspected

| Path | Pixel destination | Extra colour publication pass |
| --- | --- | --- |
| Wolf `agonport/vdplet/production/wolf3d_module.cpp`, `apply_and_render` | `render_world` writes caller workspace; `pixels_write` copies to drawing page | Yes: full source-value validation, then mapped copy of 76,800 pixels |
| Wolf `agonport/vdplet/wolf3d_scene_rows.cpp`, `render` / `store_pixel` | Borrowed hidden native rows with lane mapping and sync-bit preservation | No separate output-image copy; this is the bounded scene, not full game |
| FSIM `utilities/pingo-vdplet-flight/module.c`, `pf_rows`, using the rows engine adapter | Pingo fragment stores target borrowed native rows | No separate colour output-image copy in this path; depth and row metadata remain necessary |
| Bitmap plotting | Renderer-owned bitmap backing followed by the stock bitmap draw operation | Distinct path; no same-fixture bitmap timing was established in this audit |

The FSIM evidence is source inspection of `pingo-vdplet-rows/engine.patch` and
its `module.c` adapter, used by the live flight builder: native mode sets the
colour backing pointer null and redirects the fragment store into scanline
pointers. The flight module sets `renderer.clear=false` so composed background
survives, keeps depth storage, ends its lease and clears row pointers. Existing
legacy copied experiments remain separate. Background bitmap composition does
not itself imply that the aircraft renderer copies a completed colour frame.

Direct rendering removes a publication copy. It does not eliminate depth
clears, texture reads, background composition, transparent/occluded fragments or
overdraw, and therefore does not promise exactly one write per final pixel.

## Generic mechanism and limits

1. `agon-vdplet/src/vdplet_pixels.h` implements the copied path: `validate_pixels`
   visits the complete source, then `write_pixels` maps/copies its pixels.
   `src/vdplet_color.h` pauses/drains Canvas around that operation. This is a
   supported fallback, never zero-copy rendering.
2. `src/vdplet_color_rows.h` and `src/vdplet_runtime.h` implement the optional
   colour lease. BEGIN requires actual double-buffered SBGR2222 storage, pauses
   and drains pending drawing, then resolves and validates the resulting rows.
   ROW returns native row addresses; END resumes drawing. There is no pixel
   allocation/copy in these lease operations.
3. The row view is scoped to synchronous RUN. Concurrent native VDU submission,
   lines and copied pixels refuse BUSY. Normal/error runtime cleanup closes a
   remaining lease. The caller must preserve sync bits, discard pointers before
   END/swap and avoid presenting a partial page after failure. Trusted native
   code has no memory-isolation guarantee.
4. Ordinary host presentation remains separate. The mechanism does not swap,
   promise all screen formats, or make a rendering-speed claim. No missing
   generic service is demonstrated by the present production copy.

## Measurement correction

Wolf's `qualification/qualify_rows.py::timed_arm` measures a foreground RENDER
RUN/reply transaction. The copied arm includes its `pixels_write` validation
and copy; the direct arm includes row access and native rendering. C3/swap is
outside the interval, as are load/configuration/readback/unload. Thus the saved
3.31M versus 4.40M modelled-cycle medians remain a valid bounded transaction
observation; they establish neither whole production-frame cost nor physical
hardware speed. Golem's earlier recommendation to retain copied pixels as the
first integration choice did not complete the direct-rendering objective.

The latest FSIM counter pair is also not a copied/direct benchmark: its
89.8684% renderer share is inside the on-arm RENDER counter span, with modelled
idle/preemption and unqualified wrap limits. It cannot select Wolf's renderer
or establish a hardware gain from direct rows.

## Corrections and ownership

1. Wolf owns replacing the production output target with the existing scoped
   rows, retaining genuinely required scratch and an explicitly copied fallback
   if useful, then independently checking images, failure cleanup and complete
   comparable frame/presentation costs. Report any specific missing generic
   hook. Root does not implement, stage or run this consumer correction.
2. Golem corrects its current results/checkpoint wording to distinguish bounded
   proof from adoption, keeps old results intact and records the Author's owner
   delegation policy. Shared service documentation explicitly names copying.
3. Existing bitmap results remain bitmap results. Do not invent a third
   implementation solely to complete a benchmark table. No new speed number,
   target run, firmware change, default, commit or publication follows this audit.

Durable response: `2026-11-01_21-46-39_golem_g057-direct-framebuffer-owner-correction-237`,
thread `wolf-vdplet-direct-framebuffer`; immediate Wolf chat notification delivered.
