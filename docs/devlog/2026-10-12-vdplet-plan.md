# C/C++ vdplet research and development plan

GOLEM-002 now contains a phased plan for a standalone runtime-loaded C/C++ VDP
facility. Physical Agon is the feasibility target, agon-6502's packed monochrome
renderer motivates the first substantial module, and optional Fab/Golem support
is deferred until after a working physical loader and service interface. This
turn writes the plan only and stops for Author review and a thinking-level change.

The earlier math task's SRC, RES and DEL identifiers are preserved. Its numerical
motivation remains, but the scope now separates loader research, a tiny native
hardware proof, the packed renderer port, packaging and optional numeric/emulator/
Golem follow-ons. The recommended first authorized tranche would cover Phases A-C
and stop at Gate A before implementation or flashing. Subsequent gates require
review of real ESP32 execution and the useful rendering application.

The plan distinguishes the current custom firmware function from a runtime-loaded
payload, and the stock resident XOR/RLE decoder from the native packed row copy.
It preserves existing Buffered Commands API semantics and leaves generic extension
framing/opcodes for research. It also distinguishes a possible one-time custom
firmware installation from subsequent module updates without reflash.

TODO.md points to the revised plan. The already-recorded autonomous-sprite idea
remains deferred. No loader research, builds, downloads, runtime/source changes
in other projects, hardware/emulator operations, agent contact, commit or push
were performed. Existing drawing-design/devlog edits are preserved.
