# Maintained portable implementation — ready for review

The normal compiler now exposes the accepted portable language and both emitters
through `--portable`. The new mode produces exactly the accepted compiler's
files; existing modes also retain their output bytes. Users no longer need a
task compiler or task scripts to compile this language. Implementation and
qualification are complete; publication awaits Author review.

## Changes and use

Four generic files were promoted into `src/portable/`, with local maintained
includes, provenance and a callable driver. `golemc` dispatches explicit portable
mode while retaining legacy dispatch. The public CLI uses named reservations,
fixed bindings and array mappings with strict decimal/range/collision validation.
The old implicit reservation and root/status/type/interface tags remain intact.
No parser, typed model, emission algorithm or generated-native semantics were
changed. Native renderer bindings were not promoted.

```bash
make -C src check
build/golemc --portable examples/portable/counter.golem build/counter
```

[Guide](../../design/portable-compiler.md),
[complete example](../../../examples/portable/counter.golem),
[maintained integration checks](../../../tests/portable/README.md).
The compiler and its normal C++ checks use no SDK or task implementation.
Native output is C source; SDK/Xtensa packaging remains optional and separate.

## Qualification

1. Maintained raw/resident checks pass with their existing 132 rejections.
   New C++ portable checks pass 55 byte-identical frozen outputs, 35 counted
   rejections plus missing-argument checks, mapped/reserved/bound inputs and a
   mixed-type reference example. The same portable suite passes ASan/UBSan.
2. CLI-level before/after comparison passes 37 files across seven raw, scalar,
   table and multiple-drawing-Sub default/relocated cases. See
   [legacy-parity.json](legacy-parity.json).
3. Default and relocated integration each pass 383 cases through the buffered
   command model and sanitized generated native C: 766 total. All language
   status families 0/1/2/3/4/5/6/32/33 appear, including partial effects, paired
   division Bounds, shifts, indices, float input/rounding/conversion and skipped
   failures. Both layouts have the same expected result digest.
4. A real eZ80 assembler consumes generated equates and buffered/native wrappers.
   Its location is an explicit integration option; maintained tests do not
   require the historical task's ignored assembler installation.
5. The standalone maintained example compiles successfully. Quoted includes of
   production compiler/normal tests have no task implementation dependency.
   Source/receipt/link/fixture-identity checks and diff whitespace checks pass.

The durable [qualification receipt](implementation-checks.json) records inputs,
observations, timings and the public-header identity. Raw build/test logs remain
under the ignored task `build/` directory. Expected test fixture identities are
maintained under `tests/portable/fixtures/provenance.json`.

## Target/human boundary

All 55 selected accepted portable artifacts, including buffered install/cleanup,
shared data/layout, assembly and generated C, match byte for byte. Earlier stock
and actual-Xtensa qualification therefore applies to those unchanged emitted
programs. This tranche did not build/deploy a new Xtensa package or change any
emulator/profile/firmware. No fresh emulator or human visual run was needed.
Host model/generated-C checks are not relabelled as new target execution.

Native API release-result propagation retains the generic baseline limitation
recorded in the guide. Target filtering, generic native bindings, legacy-language
unification, scheduler and UART repair remain separate work. The compatibility
JSON profile name/experimental flag are retained for output parity and do not
imply that the maintained compiler is still sourced from a task directory.

No SDK/firmware/Rally source or historical fixture/profile was changed. New
implementation, tests, examples and documentation remain uncommitted for review.

The Author accepted and explicitly approved publishing this implementation on 2026-10-14.
