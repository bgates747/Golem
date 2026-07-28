# 2026-07-28 — AGNB assembly reference collection

## Summary

Built a provisional local collection of eZ80 assembly reference material at
[reference/agnb-asm/](../../reference/agnb-asm/), sourced from
`/home/smith/Agon/mystuff/agon-utils/examples/agnb/` — currently the most
modern collection of "regular" (non-Golem, eZ80/MOS-targeting) Agon
application assembly available locally. This is a seed for Golem's own
authoritative reference material on the eZ80/MOS/VDU side of the ecosystem
(idioms, calling conventions, and runtime-library patterns worth porting),
not a build dependency of the Golem compiler or its VDP-targeted output.

The source tree contains **three separate example applications** that each
grew their own copy of a shared assembly "standard library":

- `audio/src/asm/` — AGNB audio-loader test harness.
- `images/container/src/asm/` — AGNB image-loader test harness (RIFF/AGNB
  container format).
- `images/loose/src/asm/` — an older, pre-AGNB-format image-loader harness.

Plus a separate `api/` folder containing the polished, published,
application-neutral `agnb_api.inc` (and a documentation-only
`agnb_dependencies.inc`).

## Method

1. Listed every `.asm`/`.inc`/`.txt` file under `examples/agnb` with
   modification time and size (`find -printf`).
2. Hashed (`sha1sum`) same-named files across the three example projects to
   separate byte-identical duplicates from genuine divergences.
3. For files that differed, extracted per-label bodies (regex on
   `^label:` lines) and diffed body-by-body (after trimming trailing
   blank/comment lines, which otherwise produce false-positive diffs from
   a *neighboring* routine's leading doc-comment) to separate cosmetic
   reformatting from real behavioral changes.
4. Cross-checked each `app.asm`'s actual `include` list against the
   `api/README.md`'s documented external-dependency contract for
   `agnb_api.inc` to understand which files are truly load-bearing vs.
   vestigial in each project.

## Findings: what's identical vs. what diverged

Byte-identical across all three example projects (no collision risk,
picked arbitrarily): `mos_api.inc`, `input.inc`, `timer.inc`.

Byte-identical between `audio/` and `images/container/` only (the two later,
more-developed projects; `images/loose/` predates or lacks these):
`agnb.inc` (embedded copy, since superseded by `api/agnb_api.inc`),
`macros.inc`, `debug.inc`.

Genuinely diverged between `images/loose/` (older, 2026-07-19) and the
`images/container/`+`audio/` lineage (newer, 2026-07-23/24):

- **`functions.inc`** — 40 shared labels, only 2 with real body differences
  (`u24_to_ascii`, `u168_to_ascii`; see collision #1 below). The newer
  lineage also *removed* `printDec`, `printString`, `printHexA`, and all
  `dump*`/`stepRegistersHex` routines (moved to `includes.inc` and the new
  `debug.inc` respectively) and *added* `clear_mem`, `one_digit`,
  `printDec8`, `_printDec8Buffer`, `printDecS`, `printDecS8`,
  `printStringIX`, `u8_to_ascii`.
- **`maths.inc`** — 18 shared labels, 7 differ only in whitespace
  (tabs→spaces) except `hlu_sdiv256` (see collision #2). The newer lineage
  added `umul24`, `udiv3216`, `udiv3223`, `udiv8`, `add_uhl_a_signed`, and a
  small PRNG (`prng24`, `seed1`, `seed2`).
- **`includes.inc`** — 17 shared labels, **zero** real body differences.
  The newer lineage's file is a strict subset (dropped `bufferId0`,
  `bufferId1`, `u24_to_ascii`, `vdu_load_buffer_from_file`, `vdu_load_img` —
  all superseded by `api/agnb_api.inc`'s generic container loader).
- **`vdu.inc`** — only exists in `images/loose/`. 8 of its routines were
  never promoted anywhere else and are otherwise undocumented elsewhere:
  `vdu_clg`, `vdu_cursor_forward`, `vdu_flip`, `vdu_home_cursor`,
  `vdu_load_img_rgba2_to_8`, `vdu_rgba2_to_8`, `vdu_set_gfx_viewport`,
  `vdu_vblank`.
- **`images.inc`** — diverges heavily between `images/loose/` and
  `images/container/`, but this is **application data** (per-image
  generated `buf_NN_NNN`/`fn_NN_NNN` buffer-ID tables and dimensions), not
  reusable API code. Deliberately excluded from the reference collection.

## Breaking collisions (real, would-be assembly errors if combined naively)

1. **`u24_to_ascii`/`u168_to_ascii` restructuring.** The older
   (`images/loose`) `functions.inc` implements `u24_to_ascii` with a local
   `@one_digit` label (scoped, `@`-prefixed). The newer lineage promotes
   this to a plain global `one_digit:` label and falls through it from a
   *second*, new entry point `u8_to_ascii:` (for values known to fit in 8
   bits). This is a deliberate refactor, not a bug — but it means the two
   versions are not drop-in compatible: the newer file introduces a global
   `one_digit` label that must not collide with any other same-named local
   elsewhere, and the new `u8_to_ascii` entry point only exists in the
   newer file.

2. **`agnb_dependencies.inc` vs. `maths.inc` + `includes.inc`.**
   `api/agnb_dependencies.inc` is explicitly documented ("DO NOT include
   this file wholesale") as a *reference* collecting the exact
   implementations the proven harnesses use to satisfy `agnb_api.inc`'s
   external-dependency contract — it is **not** meant to be `include`d
   alongside the files it was copied from. Verified byte-for-byte:
   `umul24` in `agnb_dependencies.inc` is identical to the one added to the
   newer `maths.inc`; `vdu_load_buffer`, `vdu_clear_buffer`,
   `vdu_consolidate_buffer`, `vdu_buff_select`, and `vdu_bmp_create` in
   `agnb_dependencies.inc` correspond to the same-named routines in
   `includes.inc`. Including both `agnb_dependencies.inc` and
   `maths.inc`/`includes.inc` in the same assembly produces duplicate-label
   errors. An application should pick **one** source per symbol — either
   its own `maths.inc`/`includes.inc`, or copy the missing pieces out of
   `agnb_dependencies.inc`, never both.

3. **`includes.inc` is an extraction of `functions.inc` (+ `vdu.inc` for the
   loose lineage), not a complement to it.** Its own header comment says so
   ("Routines selected from functions.inc / Retained in original
   source-file order"). The container/audio lineage's `app.asm` includes
   *both* `includes.inc` **and** `functions.inc`/`maths.inc` together, and
   this only works because that lineage's `functions.inc` was pruned of
   every routine that migrated into `includes.inc` (confirmed: no
   remaining label overlap between the two in that lineage). The original
   `images/loose` project's `functions.inc` was **never pruned** this way —
   it still defines `printString`, `printDec`, `u24_to_ascii`, etc., which
   also exist in loose's own `includes.inc`. `images/loose/app.asm`
   actually avoids this by never including `functions.inc`/`maths.inc` at
   all (dead files in that project, confirmed via `grep` on its
   `include` list). **Do not** combine loose's raw `functions.inc` with
   any lineage's `includes.inc` — always follow one lineage's actual
   include list, never mix loose's pool files with the container/audio
   lineage's curated `includes.inc`, or vice versa.

4. **`hlu_sdiv256` macro dependency change.** The older `maths.inc` defines
   and uses its own inline `sign_hlu` macro. The newer `maths.inc` instead
   calls `SIGN_UHL`, which only exists in the separate `macros.inc` file
   (new in the container/audio lineage). The newer `maths.inc` is therefore
   **not self-contained** — it silently depends on `macros.inc` being
   included first. `images/container/src/asm/app.asm` and
   `audio/src/asm/app.asm` both include `macros.inc` before anything else,
   with an explicit comment noting the ordering requirement.

## Collection built

See [reference/agnb-asm/README.md](../../reference/agnb-asm/README.md) for
the full file-by-file provenance table and layout. Summary of picks: newest
(container/audio) lineage won for every file with real divergence;
byte-identical files were taken from whichever copy was convenient;
`vdu.inc` was kept solely for its 8 never-promoted routines despite being
otherwise superseded by `api/agnb_api.inc` + the newer `includes.inc`.
`api/agnb_api.inc` (the officially published copy, newest of all — dated
after even the embedded `agnb.inc` copies) was preferred over the
in-project embedded `agnb.inc` copies for the core loader.

## Next steps

- [ ] When Golem's own runtime library design starts, mine
      `reference/agnb-asm/lib/maths.inc` (`umul24`, `udiv3216`, `udiv3223`,
      `udiv8`) and `lib/functions.inc` for shift-add multiply / restoring
      division idioms — directly relevant to the devlog's identified gap
      (no VDP hardware multiply/divide).
- [ ] Read `docs/ez80_hacks.md`'s `dec r; inc.s r` HLU-clearing idiom if/when
      Golem ever needs an eZ80-side stub or loader stage.
- [ ] Treat this collection as provisional/curated, not canonical — it is a
      snapshot for reference and idiom-mining, not a dependency Golem's own
      code should `include`.

## Addendum (same day): input.inc generalized, keys.inc added

`lib/input.inc` turned out to still be application-specific despite being
byte-identical across all three example projects: it mixed the generic
MOS virtual-keyboard-map polling pattern (`keyboard_masks`/`set_keys`/
`reset_keys`) with the `images/loose` slideshow demo's own state
(`dithering_type`, `current_image_id`, `current_image_index`, a
`tmr_slideshow_*` countdown timer, and direct jumps to that demo's
`app.asm` flow labels `rendbmp`/`main_end`/`no_move`). Stripped it down to
just the reusable polling pattern plus a clearly-labeled, non-functional
example `do_input` skeleton (Escape-style quit check, Left/Right-style
adjust-a-value check) that documents itself as "copy and replace, not
call directly."

That skeleton's `bit n,(ix+m)` checks need the keyboard map's per-key
(byte offset, bit) table, which lived in a separate `keys.inc`/`keys.asm`
file not part of the agnb example tree at all. Found three copies under
`~/Agon/mystuff`:

- `nurples/src/asm/keys.inc` (2026-07-20 14:32) — newest.
- `agon-utils/examples/jukebox/src/asm/keys.inc` (2026-07-19 21:02) —
  byte-identical to the nurples copy.
- `agon-utils/examples/midi/organ/keys.asm` (2026-07-19 21:02) — a
  differently-styled variant using named local labels (`@Shift:`,
  `@Ctrl:`, ...) and `jr` instead of anonymous `@@`/`jp`; arguably more
  readable as a template but not the newest, so not used here.

AgonWolf3D has no equivalent assembly keymap file (only a deprecated
Python `mos_keys.py` script under `dev/deprecated/scripts/`).

Brought in the nurples copy (verified byte-identical to the canonical
agon-utils one) as `reference/agnb-asm/lib/keys.inc`. Verified the example
key checks retained in `input.inc` (Escape/113, Left/26, Right/122) match
`keys.inc`'s documented offsets exactly before keeping them.
