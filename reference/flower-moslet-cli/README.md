# Flower MOSlet CLI-parsing reference (vendored, provisional)

**Status: reference material only. Nothing here is built, adapted, or
wired into Golem yet.** This is a curated snapshot of two files from
`/home/smith/Agon/mystuff/agon-utils/examples/flower/src/`, kept here as a
concrete worked example of command-line argument parsing inside a real
Agon MOSlet, to inform Golem's own future MOSlet-based loader (see
[docs/design/language-type-proposals.md](../../docs/design/language-type-proposals.md)'s
"Axis 17: The loader's ultimate target shape" section).

Everything in this directory is a **read-only vendored copy**, same
convention as [reference/agnb-asm/](../agnb-asm/). Do not edit in place;
if/when Golem actually implements a MOSlet-based loader, port and adapt
the relevant logic into `runtime/legacy-asm/` (or wherever the real
loader eventually lives) instead of building on these copies directly.

## Files

- **`flower.asm`** — the real, working `flower` MOSlet (`ORG 0x0B0000`,
  proper "MOS" header, discovered by MOS via the `/mos/` folder
  convention). Contains an *inline* argv/argc tokenizer
  (`_parse_params`/`_get_token`/`_skip_spaces`) plus float-argument
  helpers (`get_arg_float`, `store_arg_iy_float`), and a fully worked
  example of positional-argument dispatch with compiled-in defaults
  (`load_input`, `input_params` table, `min_args` usage-error gate).
  This is the best example of the **full moslet header/entry/argv
  integration shape**.
- **`parse_args.inc`** — a separately-factored, standalone copy of the
  same tokenizer plus the argument-matching helpers `get_arg_text`,
  `match_next`, `match_next_and_print`, `print_params` (verb/keyword
  dispatch against a jump table, not just positional floats). Note this
  file is *not* actually `INCLUDE`d by `flower.asm` itself (which has its
  own inline, slightly older copy of the same routines) — it's used by a
  sibling file in the same source tree, `flower_demo.asm`, which is
  itself a regular program (`ORG 0x040000`), not a MOSlet. It's vendored
  here anyway because `match_next`/`match_next_and_print` are the closest
  available prototype for verb-style dispatch (e.g. Golem's own sketched
  `golem load` / `golem run` / `golem clear`), which `flower.asm`'s purely
  positional-float scheme doesn't demonstrate at all.

## Why this matters for Golem

Today's `examples/hello_golem/loader.asm` is a fixed, no-arguments loader
(hard-codes `program.bin` into buffer 0). Axis 17 sketches a future
`golem load "hello.golem"` / `golem run "hello.golem"` / `golem clear`
MOSlet CLI. These two files together cover both halves of that future
implementation:

1. Tokenizing MOS's raw command-line string into `argv`/`argc`
   (`flower.asm`'s inline copy, or `parse_args.inc`'s standalone copy —
   functionally near-identical).
2. Dispatching on a verb/keyword rather than positional numeric
   arguments (`parse_args.inc`'s `match_next`/`match_next_and_print`),
   which is the shape Golem's `load`/`run`/`clear` verbs actually need,
   unlike `flower.asm`'s own all-positional-floats `load_input`.

See [docs/devlog/](../../docs/devlog/) for the session that pulled these
in, and repo memory
(`/memories/repo/moslet-arg-parsing-flower-reference.md`, not part of
this git repo) for a denser working summary of both files.
