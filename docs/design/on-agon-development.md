# Development directly on the Agon

Golem can provide an edit, compile and run cycle on an Agon without a PC or an
LLM. The first implementation targets the stock Buffered Commands API. Native
VDPlet compilation is a separate possible extension, not a requirement for this
first compiler. This revisits the host-only decision of 2026-07-28; the existing
desktop compiler remains supported.

## Options

| Route | What runs on the Agon | What still needs another computer | Cost and limits |
| --- | --- | --- | --- |
| 1. Buffered-command compiler | Source reader, parser, type checks, command emission and a fixed MOS loader | Building the compiler executable itself; ordinary users install the supplied binary | Most direct route. The Author subsequently requires all currently implemented profiles, within actual working RAM; proposed syntax is excluded. |
| 2. Buffered compiler plus native library | The above, with calls to supplied precompiled VDPlets | Adding or changing native library primitives | Useful native maths/rendering without a local Xtensa compiler. Requires VDPlet-enabled firmware for those calls; buffered-only programs retain stock support. |
| 3. Direct Xtensa backend | A Golem compiler producing new machine code and VDPlet packages | Building the compiler executable itself | Avoids GCC on the Agon, but requires instruction selection, register management, ABI, relocation and packaging work. Start with a small typed subset if pursued. |
| 4. Port the existing host compiler | Existing C++ frontend/backend on the eZ80, after adaptation | Initial cross-build and substantial C++ runtime adaptation | Maximizes potential source sharing. Current filesystem, containers, exceptions and host-sized intermediate representations make this a porting investigation, not a simple rebuild. |
| 5. Bring a C/Xtensa toolchain onto the Agon | Golem-to-C followed by an on-machine C cross-compiler/linker | Initial toolchain bootstrap | Preserves the generated-C route but has the largest toolchain and memory burden. No claim that current Espressif GCC can run unchanged on the eZ80. |
| 6. Compiler service in the VDP | eZ80 editing/files plus a compiler implemented as firmware or a VDPlet | Supplying that compiler service and firmware support | Uses ESP32 resources, but moves compilation into the VDP and does not serve the stock-firmware requirement. It is still on-machine development, not eZ80-native compilation. |

Options 4–6 describe alternatives to implementing options 1–3 with a compact
eZ80 compiler; they are not additional currently promoted tasks. A source
interpreter could also provide an interactive environment, but it would not
answer the specific request to compile resident buffered programs.

## First implementation

GOLEM-051 owns option 1. The Author expanded scope to every currently implemented
language feature and specified C/C++ targeting AgonDev. The implementation now
combines option 1's deployment with option 4's source reuse: actual maintained
C++ parsers/backends, a platform-library adapter and a standalone MOS runner.
The initial subset exploration is superseded and retained in its task silo.
It emits real buffered programs; expressions, loops and drawing execute on
the VDP, not as an eZ80 source interpreter. Files and installation remain MOS
responsibilities. Source syntax follows the existing Golem conventions.

The package must include executables, initialized examples, a quick start and
a language reference distinguishing implemented profiles from proposed syntax.
Exact memory limits and evidence belong to the task and delivered reference.
Direct Xtensa generation, native-library binding, compiler self-hosting and an
editor are outside this first task. Existing native C emission remains in scope;
compiling that C into a VDPlet is a separate step.

## Terminology

The compiler executable runs natively on the eZ80 but targets the VDP command
machine. This is native *development on Agon* and cross-compilation between its
processors. It is not a self-hosting compiler: its implementation is C++, not
Golem. The present desktop compiler also never requires an LLM to operate.
