# Standalone compiler coverage audit

The distributed compiler already implements the current language profiles. Its
four workers were current, and its six executables reproduce exactly from the
original ZIP. Refreshing this package mainly improves the user instructions and
proves the delivered source and examples, rather than adding another compiler.

| Implemented family | Maintained implementation | Distribution coverage |
| --- | --- | --- |
| Raw literal print, explicit uint16, addition, counted loops | ordinary.hpp | hello/loops examples; reference identifies wrapping and endpoint limits |
| Resident scalar floor/conversion, checked uint16 arithmetic and paired division | resident.hpp, scalar_backend.hpp | full scalar syntax and limitations in LANGUAGE and resident-scalar reference |
| Record/Table checked fetch and build-time packed data | resident_table.hpp | table.golem/records.dat, HOWTO exercise4, reference |
| Named resident drawing, uint8/sint16/uint16, bitmap reservations | resident_drawing.hpp | visible cube, complete drawing/bitmap signatures and ownership in reference |
| Six portable scalar types, checked arithmetic/conversions/Bounds, comparisons, short-circuit logic | portable/portable.hpp | counter/precision/overflow; complete initialized syntax in LANGUAGE |
| Portable If/ElseIf/Else and finite inclusive Range | portable/portable.hpp | initialized language-reference examples; counter demonstrates execution |
| Integer arrays, Values/Fill/file initialization and readonly/index checks | portable/portable.hpp | arrays.golem, --array-data and reference |
| uint32/sint32 right shift and uint16/uint32/sint32 divmod | portable/portable.hpp | complete initialized reference examples and exact signed semantics |
| Integer &, \|, ^, ~ and decimal str/print | portable/backends.hpp | bitwise/counter examples, HOWTO and reference |
| Stock float add/subtract/multiply/conversions and nine-digit printing | portable/backends.hpp | float-print/precision examples and documented normal/zero admission |
| Native-only float divide/sin/cos/floor | portable/backends.hpp | trig example and explicit native-C build distinction |
| Shared roots, explicit backend selection, external bindings/layout | portable/root.hpp, driver.hpp | detailed portable-compiler/backend-portability references and compiler options |
| Stock matrices, typed borrowed vertex buffers, perspective/copy, bounded calls/indexed loops | stock_matrix.hpp | matrix example plus stock-matrix reference including exact signatures/limits |
| Named assembly, external reservations, AGNB installation/load plans | assembly_output.hpp, load_plan.hpp | reference contracts and compiler-options summary |
| Native MOS compiler selection, response files, fresh-worker batches and runner | shipped port/ adapter | four workers, dispatcher/runner, HOWTO and complete rebuild source |

All maintained C++ sources, port adapter and selected numeric dependency sources
are packaged. The reference explicitly separates proposed Functions/locals,
multidimensional arrays, fixed-point, general Do/Switch/jump tables and extra math
from executable syntax. Native C output is not a compiled Xtensa VDPlet, and
matrix/borrowed-assets integration is not falsely offered as a standalone runner.

Confirmed corrections:

- DOC-001 [x] Explain that Print adds no newline but the supplied runner finishes
  the line before MOS. Show the complete initialized counter source in HOWTO.
- DOC-002 [x] Summarize all accepted integration options and explicit buffered
  selection in LANGUAGE/HOWTO; preserve the detailed shipped contracts.
- DOC-003 [x] Correct maintained CLI usage to include the already implemented
  `--target buffered`. This is help text only; rebuild candidate workers from
  that exact new source instead of silently retaining older source pins.
- DEL-001 [x] Add optional package output/report destinations so a review ZIP can
  be prepared without overwriting the accepted archive or its receipts.

Evidence: package-source-audit.json, archive rebuild result in build/, maintained
check log, and archive-parity-results.json (316 cases,75 accepted). Target/MOS
and candidate-ZIP review remain separate gates. This audit does not claim a new
physical test or that all language profiles can be combined in one program.
