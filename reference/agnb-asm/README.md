# AGNB assembly reference collection (provisional)

This is a **provisional, curated snapshot** of eZ80 assembly source pulled
from `/home/smith/Agon/mystuff/agon-utils/examples/agnb/` (the most modern
collection of "regular" Agon application assembly currently available),
intended as a **seed for Golem's own authoritative reference material** on
the eZ80/MOS/VDU side of the ecosystem — not as a build dependency of Golem
itself (Golem targets the VDP Buffered Commands API directly; this is
background/idiom reference, and a source of runtime-library patterns to
port).

Everything here is a **read-only vendored copy**. Do not edit these files in
place; if Golem needs a modified version of a routine, copy it into
`runtime/legacy-asm/` and adapt it there.

For the full analysis of how these files relate to each other, which
versions were chosen and why, and the collisions discovered between
same-named routines, see
[docs/devlog/2026-07-28-agnb-asm-reference-collection.md](../../docs/devlog/2026-07-28-agnb-asm-reference-collection.md).

## Layout

```
api/     The published, application-neutral AGNB loader API (newest,
         most authoritative tier).
lib/     Supporting utility/math/VDU/debug/input routines used by the AGNB
         example applications (audio + image loaders), plus keys.inc (a
         generic keyboard-map reference pulled in from outside the agnb
         tree, not part of the AGNB example lineage itself).
audio/   Audio-specific scaffolding (only needed if/when Golem cares about
         VDP audio buffers).
docs/    Supporting reference docs (eZ80 idioms, AGNB container format).
```

## Provenance summary

| File | Source (winner) | Why |
| --- | --- | --- |
| `api/agnb_api.inc` | `agnb/api/agnb_api.inc` | Newest (2026-07-24 22:12), the one published/generic copy; supersedes the near-duplicate `agnb.inc` embedded in `audio/src/asm` and `images/container/src/asm`. |
| `api/agnb_dependencies.inc` | `agnb/api/agnb_dependencies.inc` | Reference implementations of every symbol `agnb_api.inc` expects the host app to provide. **Do not include verbatim** — see collision notes below. |
| `lib/macros.inc` | `agnb/images/container/src/asm/macros.inc` | Identical to `audio/src/asm/macros.inc`; loose's project has no equivalent (self-contained inline macros instead). |
| `lib/includes.inc` | `agnb/images/container/src/asm/includes.inc` | Newer, pruned lineage vs. `images/loose`'s copy (which still had now-obsolete `vdu_load_img`/`vdu_load_buffer_from_file`, superseded by `agnb_api.inc`). |
| `lib/functions.inc` | `agnb/images/container/src/asm/functions.inc` | Identical to `audio/src/asm/functions.inc`; newer than, and a refactor of, `images/loose`'s copy (dump/step routines split out to `debug.inc`; added `clear_mem`, `u8_to_ascii`, `printStringIX`, decimal-8 variants). |
| `lib/maths.inc` | `agnb/images/container/src/asm/maths.inc` | Identical to `audio/src/asm/maths.inc`; newer than, and a superset of, `images/loose`'s copy (adds `umul24`, `udiv3216`, `udiv3223`, `udiv8`, `add_uhl_a_signed`, `prng24`/`seed1`/`seed2`). |
| `lib/debug.inc` | `agnb/images/container/src/asm/debug.inc` | Only exists in the container/audio lineage; dev-only register/memory dump scaffolding, explicitly marked removable for production in `app.asm`. |
| `lib/mos_api.inc`, `lib/timer.inc` | container lineage (arbitrary — byte-identical across all three example projects) | No divergence found; any copy is equally authoritative. |
| `lib/input.inc` | container lineage, **hand-generalized** | The original (byte-identical across all three example projects) mixed a generic keyboard-mask-polling pattern with slideshow-specific state (`dithering_type`, `current_image_id`, `current_image_index`, a `tmr_slideshow_*` countdown, and jumps to app.asm flow labels). Stripped down to the reusable `keyboard_masks`/`set_keys`/`reset_keys` pattern plus a documented example-only `do_input` skeleton. See devlog. |
| `lib/keys.inc` | `/home/smith/Agon/mystuff/nurples/src/asm/keys.inc` | **Not from the agnb tree.** The keyboard-map (byte offset, bit) table for every named key, needed by `input.inc`'s `bit n,(ix+m)` checks. Newest of three known copies (nurples 2026-07-20; byte-identical to `agon-utils/examples/jukebox/src/asm/keys.inc` 2026-07-19). A fourth variant at `agon-utils/examples/midi/organ/keys.asm` uses named local labels (`@Shift:`) and `jr` instead of anonymous `@@`/`jp` — arguably more readable, but older; not used here since the newest copy was requested. |
| `lib/vdu.inc` | `agnb/images/loose/src/asm/vdu.inc` | **Only** copy that exists (container/audio dropped it). Mostly superseded by `agnb_api.inc` + `lib/includes.inc`, but 8 routines were never promoted anywhere else: `vdu_clg`, `vdu_cursor_forward`, `vdu_flip`, `vdu_home_cursor`, `vdu_load_img_rgba2_to_8`, `vdu_rgba2_to_8`, `vdu_set_gfx_viewport`, `vdu_vblank`. |
| `audio/audio.inc`, `audio/vdu_sound.inc` | `agnb/audio/src/asm/` | Only exist in the audio example; `vdu_sound.inc` provides `vdu_buffer_to_sound`, one of `agnb_api.inc`'s documented external dependencies. |
| `docs/ez80_hacks.md` | `agnb/docs/ez80_hacks.md` | Cross-project eZ80 idiom notes (e.g. the `dec r; inc.s r` HLU-clearing trick). |
| `docs/file-format.md` | `agnb/api/file-format.md` | Normative AGNB RIFF container layout spec. |

Deliberately **not** copied: `app.asm`/`app.lst` (demo entry points, not
reusable library code), `images.inc` (per-application generated
image-asset descriptor tables — data, not API), `routine-manifest.txt`
(a line-number index scoped to the `images/loose` project's exact file
layout, not to this collection's file mix).

## Known collisions — summary (see devlog for full detail)

- **`agnb_dependencies.inc` vs. `lib/maths.inc` + `lib/includes.inc`**: both
  define `umul24`, `vdu_load_buffer`, `vdu_clear_buffer`,
  `vdu_consolidate_buffer`, `vdu_buff_select`, and `vdu_bmp_create`.
  `agnb_dependencies.inc` is a *reference*, not a file meant to be
  `include`d alongside `lib/maths.inc`/`lib/includes.inc` — doing both would
  produce duplicate-label assembly errors. Pick one source per symbol.
- **`lib/includes.inc` vs. `lib/functions.inc`/`lib/vdu.inc`**: `includes.inc`
  is explicitly "routines selected from functions.inc" (and, in the loose
  lineage, from `vdu.inc`) — a curated extraction, not an independent file.
  The container/audio lineage's `functions.inc` was pruned of every routine
  that migrated into `includes.inc` specifically to avoid this collision;
  the original loose `functions.inc` was **not** pruned and duplicates
  labels also present in loose's own `includes.inc` (do not combine them).
- **`lib/maths.inc`'s `hlu_sdiv256`**: the container/audio version calls a
  `SIGN_UHL` macro that only exists in `lib/macros.inc`; the loose version
  defined its own inline `sign_hlu` macro instead. `lib/maths.inc` in this
  collection therefore requires `lib/macros.inc` to be included first.
