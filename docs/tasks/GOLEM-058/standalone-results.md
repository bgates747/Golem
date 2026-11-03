# Standalone compiler refresh — accepted

The refreshed distribution compiles and runs its examples directly on the Agon
with stock VDP. Its source rebuilds the delivered executables exactly, and the
Author accepted the walkthrough's picture, numerical output, cursor and MOS
keyboard continuation. Commit and push were explicitly approved2026-11-03.

The usable local archive remains `dist/golem-on-agon-GOLEM-051.zip`,556961bytes,
SHA2560edce1573beb32fd3f0acb0b04ea7dfcfd06b04206289d5650b594446e4dbc91.
It contains115files: compiler workers, dispatcher/runner, initialized examples,
root and installed how-to/language reference, complete rebuild sources and
114file checksums. No internal evidence directory is shipped. The previous ZIP
is preserved under ignored build storage; no SDK/runtime change is part of this
standalone refresh. Generated archives remain ignored artifacts, not Git source.

## What changed

1. The how-to explains Print versus the runner's final newline, includes the
   full initialized counter source, and explains explicit buffered/native/both
   target selection. The language reference summarizes all implemented compiler
   integration options. Proposed features remain identified as proposed.
2. Compiler help now lists the already implemented buffered target. The four
   native workers were rebuilt from that exact source. This does not change
   language semantics or emitted program bytes.
3. The package builder accepts optional output/archive/report paths, allowing
   a candidate to be reviewed without overwriting accepted artifacts. Default
   paths remain compatible. Current sources, manuals and examples are included.

## Evidence

1. Both previous and candidate ZIPs rebuild all six executables exactly from
   their own shipped source. Candidate rebuild16.47seconds. Maintained compiler
   contracts pass, and316existing adapted-host cases match,75accepted.
2. Actual stockMOS compiles13jobs through all four native workers, matches196
   emitted files and executes76 independently expected state/status bytes.
   Thirteen white cube anchors, actualDIR/SAVE continuation and zero backend
   faults verify.214returned files are independently hash-checked.
3. The final walkthrough compiles five shipped examples in32.24seconds and
   shows Hello, integer/float/bitwise output and the twelve-edge white cube.
   The Author reports pass and closed emulator;49fresh files and40native output
   files verify. Cursor/DIR pass. Oryx218is released; no reservation is retained.
4. An earlier staging failure discarded empty directories while copying files;
   the ZIP already contained them. Failed28-file evidence remains preserved.
   Corrected staging copies directories and stops at the first batch error.
   Neither archive nor compiler was changed to conceal the fixture failure.

Exact identities and scripts are indexed by standalone-final-package.json,
standalone-human-review.json, stock-native-results-root.json,
candidate-archive-rebuild-results.json, archive-parity-results.json and the
frozen stock-target-contract-r01.md. Physical and timing claims are outside
this functional standalone acceptance. VDPlet-native C emission still needs a
separate PC Xtensa toolchain; ordinary buffered programs do not.
