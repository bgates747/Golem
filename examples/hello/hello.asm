; hello.asm
;
; Minimal hand-assembled "hello world" for Golem. Not a Golem-compiled
; program (there's no compiler yet - see docs/devlog/2026-07-28.md's "Next
; steps" checklist, item 2) - this is a hand-written eZ80/MOS application
; that exercises the exact mechanism Golem's compiled programs will rely on
; throughout: write a command sequence into a VDP-resident buffer once
; (Buffered Commands API command 0), then trigger it with a short "call
; buffer" (command 1), via runtime/legacy-asm/vdu_buffered_api.inc.
;
; The buffer's payload here is just plain printable ASCII (no VDU escape
; sequences), so "calling" it has the VDP interpret those bytes exactly as
; if they'd been sent directly - i.e. it prints the text. This is the
; smallest possible end-to-end proof that "store a command sequence in a
; buffer, then execute it by reference" actually works, before building any
; more of the compiler around that assumption.
;
; Build:   ez80asm hello.asm hello.bin
; Run:     copy hello.bin to an emulator/hardware SD card and run it from
;          MOS (see docs/devlog/2026-07-28-compiler-toolchain.md for the
;          project's emulator status - no Golem-specific profile exists
;          yet).

    .assume adl=1
    .org 0x040000

    jp start

    .align 64
    .db "MOS"
    .db 00h
    .db 01h

start:
    push af
    push bc
    push de
    push ix
    push iy

    call main

    pop iy
    pop ix
    pop de
    pop bc
    pop af
    ld hl,0
    ret

    include "../../runtime/legacy-asm/vdu_buffered_api.inc"

; Buffer ID used for this whole example - arbitrary, just needs to be a
; value not otherwise in use.
hello_buffer_id: equ 1

main:
    ; VDU 12: clear the text screen, so the output is easy to spot.
    ld hl,@cls
    ld bc,@cls_end-@cls
    rst.lil $18
    jp @after_cls
@cls:     db 12
@cls_end:
@after_cls:

    ; Write the greeting into a VDP buffer (command 0 - creates the buffer).
    ld hl,hello_buffer_id
    ld de,hello_text
    ld bc,hello_text_end-hello_text
    call vbuf_write

    ; Execute that buffer's contents (command 1) - the VDP replays the
    ; stored bytes as if they were sent directly, printing the text.
    ld hl,hello_buffer_id
    call vbuf_call

    ret

hello_text: db "Hello, World, from a VDP buffer!",0
hello_text_end:
