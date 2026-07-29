; program.asm
;
; Hand-authored STAND-IN for what a real Golem compiler would emit as the
; compiled output of hello.golem's `print "..."` statement, in this
; directory's toy model of Golem's eventual output format: a flat binary
; containing *only* the raw bytes that will become a VDP buffer's payload
; (see loader.asm, which uploads this file's bytes into a buffer verbatim
; and calls it - it never parses or interprets them).
;
; Why raw text bytes are a valid "compiled instruction" here: per
; docs/devlog/2026-07-28.md, Golem's compiled programs run entirely inside
; VDP buffers, replayed through the VDP's normal VDU byte-stream dispatcher
; (Buffered Commands API command 1 == "call buffer" == "replay these bytes
; as if they'd been sent directly"). Printable ASCII bytes have no VDU
; escape-sequence meaning, so the VDP's default handling of them already
; *is* "print this character" - i.e. for a string-literal-only `print`,
; codegen degenerates to "emit the literal bytes". Anything fancier
; (formatting, expressions, control flow) would need real instruction
; encoding, which is exactly the unstarted IR/codegen work item 1 in
; AGENTS.md's checklist.
;
; This file is assembled as a plain flat binary (ez80asm's default output
; for a file with no MOS-header/jp-start boilerplate) purely as a
; convenient, exact way to hand-produce program.bin's bytes - it is NOT an
; eZ80 program and is never run as one; loader.asm only ever treats
; program.bin as opaque data.
;
; Build: ez80asm program.asm program.bin

    .org 0

program_start:
    db "Hello, World! (via a simulated Golem-compiled buffer, not hand-written eZ80 assembly)",13,10
program_end:
