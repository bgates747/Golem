# Current-language parity inventory

The native compiler keeps the existing profiles and rejects their existing
unsupported combinations. It does not turn design proposals into implemented
syntax. The standalone language reference describes their grammar; this table
records the implementation coverage being qualified for the port.

| Profile | Features retained | Output options retained |
| --- | --- | --- |
| Raw | Literal print, explicit uint16 state, assignment/addition, legacy counted For/Next | Original raw payload; extra private cleanup sidecar |
| Resident scalar | Bounded float32 floor-to-sint16, persistent uint16 state, checked + / - / *, paired division and finite failure exits | Reservations, assembly symbols/wrappers, load plan, bootstrap/invoke/update/cleanup/manifest |
| Resident table | Flat typed Record, immutable Table initialized from packed build data, initialized record value, checked whole-record selection | --table-data, reservations, assembly and load plan |
| Resident drawing | uint8/sint16/uint16 fields and Bounds; points, lines, quads, reserved native-size bitmaps; multiple named Subs | Existing layout/ownership and all resident outputs |
| Portable | uint16/sint16/uint32/sint32/float32/bool; checked arithmetic/conversions; comparisons, short-circuit logic, If/ElseIf/Else; finite Range; paired division; integer arrays/checked access; XOR and wide right shifts | Both/native target selection, mapped array data, reservations, 32-bit fixed descriptor bindings, shared state/layout, stock installation, native C, calls and load plan |
| Portable native math | Float32 /, sin, cos and floor using native-only selection | Same native C source and math-library requirements; no on-Agon Xtensa toolchain |
| Stock matrix | 3x3/4x4 storage, identity/rotate/scale/translate/multiply/invert/set; typed borrowed XYZ/XY buffers; read_float32, transform/copy; finite Sub calls and compact indexed Range loops | Reservations, all existing metadata, labeled assembly and load plan |

All profiles reuse the current source parsers/backends. The captured corpus
comes from the five maintained host contract suites: compiler, portable,
stock-matrix, stock loops and load plans. It contains 280 unique source inputs;
adding native-only variants gives 316 comparisons. Separate CLI cases exercise
bindings beyond 24 bits, response quoting, data mappings and output flags.

Ten initialized examples exercise the installed MOS command and actual
compiler workers. Stock execution checks retained integer/array/table/matrix
results, finite overflow, precise float bounds and a white wireframe cube.
The numerical adapter also compares IEEE decimal parsing/formatting and
adjacent float values against the host library. Counts and exact source/runtime
identities are recorded in qualification.json, rather than inferred from this
inventory. Headless evidence is separate from human and hardware review.
