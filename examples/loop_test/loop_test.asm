; loop_test.asm
;
; Hand-prototyped test of the VDP Buffered Commands API's looping/tail-call
; mechanism - NOT a Golem-compiled program (golemc has no loop/control-flow
; syntax yet - see docs/design/language-type-proposals.md's Axis 15, still
; undecided). This exercises AGENTS.md's "Current task" checklist item 2:
; "Hand-prototype the 'one instruction per block' encoding for a trivial
; loop; verify tail-call jump conversion behaves as documented (check
; against vdu_buffered.h, not just the prose docs)."
;
; This is a byte-for-byte hand-assembled version of the
; Buffered-Commands-API.md "Repeating a command" example (its "### Repeating
; a command" section), built using runtime/legacy-asm/vdu_buffered_api.inc
; instead of raw VDU byte literals, with one deliberate change: the loop
; count is 50 (not the doc example's 20), specifically to exceed the VDP's
; documented ~20-deep call-stack limit ("Stack depth" section of the same
; doc). If tail-call optimisation does NOT actually happen as documented,
; this program should crash/hang the VDP well before finishing; if it DOES
; happen (as vdu_buffered.h's bufferCall confirms - see the `inputStream->
; available() == 0` check right before its "tail-call optimise - turn the
; call into a jump" comment, around vdu_buffered.h line 337), the VDP
; converts each buffer 3 -> buffer 3 self-call into a jump automatically
; (since it is always the LAST command in buffer 3's command stream), so
; only one real call frame is ever used no matter the loop count.
;
; Four buffers are used - the doc example's three (call/counter/loop body)
; plus one extra:
;   buffer 1 (loop_print_buffer_id):  holds the literal text "Hello " -
;     calling it just prints those bytes (same trick as examples/hello/
;     hello.asm and golemc's own `print` codegen).
;   buffer 2 (loop_counter_buffer_id): a 1-byte iteration counter.
;   buffer 3 (loop_body_buffer_id): the loop body, built up from three
;     separately-written command blocks (call buffer 1; decrement the
;     counter; conditionally call buffer 3 itself while the counter is
;     still non-zero). The third block is deliberately the LAST thing
;     written to buffer 3, and nothing is ever appended after it, so the
;     VDP's tail-call check finds no remaining bytes and converts it to a
;     jump.
;   buffer 4 (loop_zero_buffer_id): a permanent 1-byte constant zero, used
;     as the conditional check's comparison operand (see below - this
;     works around a real firmware quirk, not a stylistic choice).
;
; A quirk found while building this test (worth recording explicitly,
; since it isn't documented anywhere): command 6's "basic" single-operand
; forms (VBUF_COND_EXISTS/NOT_EXISTS, or comparing a checked buffer byte
; against a literal operand) do NOT reliably detect a checked byte of
; exactly zero. vdu_buffered.h's bufferConditional() pre-initialises its
; `sourceValue` accumulator to -1 (0xFFFFFFFF) and then reads only the
; requested 1 (or 2) byte(s) into it via a plain memcpy (buffers.h's
; readBufferBytes) - the untouched upper bytes stay 0xFF, so a checked
; byte of 0 comes out as sourceValue == 0xFFFFFF00 (-256), which is
; non-zero, making COND_EXISTS wrongly report "exists" for a zero byte
; (confirmed empirically: an earlier version of this test using
; VBUF_COND_EXISTS against a 50-count literal ran the loop body 51 times,
; then hit "invalid source or operand value" on the following iteration
; once the counter wrapped to 255, since 0xFFFFFF00 | 0xFF happens to
; equal the same -1 sentinel used for "read failed"). The fix used here:
; compare the counter against a buffer-held zero (VBUF_COND_NE +
; VBUF_COND_F_BUFVALUE) instead of EXISTS/NOT_EXISTS or a literal operand
; - both sides then go through the same readBufferBytes/-1-init path, so
; their identical upper-byte contamination cancels out in the comparison.
;
; Verification plan (see docs/devlog/2026-07-29-emulator-debug-tooling.md
; for the available hooks): run under `--verbose` and confirm the log
; shows exactly 50 "bufferCall: buffer 1" lines (one per "Hello " print),
; with no VDP crash/hang - that's a real, hardware-accurate proof the
; tail-call conversion is doing its job, since 50 nested (non-optimised)
; calls would exceed the ~20-deep stack limit and crash.
;
; Build:   ez80asm loop_test.asm loop_test.bin
; Run:     see emulator/ in this directory (symlinked fab-agon-emulator +
;          firmware, per the same pattern as examples/hello/emulator and
;          examples/hello_golem/emulator) - `load loop_test.bin` + `run .`
;          from MOS, or via emulator/sdcard/autoexec.txt.

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

; Buffer IDs used for this example - arbitrary, just need to not collide;
; matches the doc example's own buffer numbering (1/2/3) for easy
; cross-reference against Buffered-Commands-API.md's "Repeating a command"
; section.
loop_print_buffer_id:   equ 1
loop_counter_buffer_id: equ 2
loop_body_buffer_id:    equ 3
loop_zero_buffer_id:    equ 4

; Deliberately > the VDP's documented ~20-deep call-stack limit, so this
; test only passes if tail-call optimisation is genuinely happening (see
; this file's header comment).
loop_count: equ 50

main:
    ; VDU 12: clear the text screen, so the repeated output is easy to see.
    ld hl,@cls
    ld bc,@cls_end-@cls
    rst.lil $18
    jp @after_cls
@cls:     db 12
@cls_end:
@after_cls:

    ; Buffer 1: the text a single loop iteration prints.
    ld hl,loop_print_buffer_id
    ld de,hello_text
    ld bc,hello_text_end-hello_text
    call vbuf_write

    ; Buffer 2: the iteration counter, a single byte initialised to
    ; loop_count.
    ld hl,loop_counter_buffer_id
    ld de,counter_init
    ld bc,1
    call vbuf_write

    ; Buffer 4: a permanent constant zero byte, used as the conditional
    ; check's comparison operand (see this file's header comment for why
    ; a literal 0 operand doesn't work reliably here).
    ld hl,loop_zero_buffer_id
    ld de,zero_byte
    ld bc,1
    call vbuf_write

    ; Buffer 3: the loop body, built up from three command blocks in
    ; order (see this file's header comment for why order/finality here
    ; matters for tail-call optimisation).
    ld hl,loop_body_buffer_id
    ld de,cmd_call_print
    ld bc,cmd_call_print_end-cmd_call_print
    call vbuf_write

    ld hl,loop_body_buffer_id
    ld de,cmd_decrement_counter
    ld bc,cmd_decrement_counter_end-cmd_decrement_counter
    call vbuf_write

    ld hl,loop_body_buffer_id
    ld de,cmd_cond_call_self
    ld bc,cmd_cond_call_self_end-cmd_cond_call_self
    call vbuf_write

    ; Kick off the loop. This returns once the counter has reached zero -
    ; the very last iteration's conditional call finds the "exists" check
    ; false and simply falls through to the end of buffer 3's stream,
    ; which returns control to this original (non-tail-call) `call`.
    ld hl,loop_body_buffer_id
    call vbuf_call

    ; If we get here, the whole loop ran to completion and returned
    ; normally - print confirmation so this is visible on screen even
    ; without reading the --verbose log.
    ld hl,@done
    ld bc,@done_end-@done
    rst.lil $18
    ret
@done:     db 13,10,"Done.",13,10
@done_end:

hello_text:     db "Hello "
hello_text_end:

counter_init: db loop_count
zero_byte:    db 0

; --- Buffer 3's three command blocks, hand-encoded byte-for-byte per
; Buffered-Commands-API.md's command 1/5/6 wire formats (cross-checked
; against this file's header comment and vdu_buffered_api.inc's constants
; instead of magic numbers, but otherwise identical to what the doc
; example's raw `VDU 23, 0, &A0, ...` lines would send). ---

; "VDU 23, 0, &A0, 1; 1" - call buffer 1 (print "Hello ").
cmd_call_print:
    db 23, 0, $A0
    dw loop_print_buffer_id
    db VBUF_CMD_CALL
cmd_call_print_end:

; "VDU 23, 0, &A0, 2; 5, 3, 0; -1" - adjust (add) buffer 2 offset 0 by -1
; (255 as an unsigned byte, i.e. decrement with wraparound).
cmd_decrement_counter:
    db 23, 0, $A0
    dw loop_counter_buffer_id
    db VBUF_CMD_ADJUST, VBUF_ADJ_ADD
    dw 0
    db 255
cmd_decrement_counter_end:

; "VDU 23, 0, &A0, 3; 6, &23, 2; 0; 4; 0" - conditionally call buffer 3
; (itself) while buffer 2 offset 0 is not equal to buffer 4 offset 0 (the
; permanent zero constant) - see this file's header comment for why a
; buffer-vs-buffer compare is used here instead of VBUF_COND_EXISTS or a
; literal operand. This is always the last block written to buffer 3,
; with nothing appended after it, so the VDP's tail-call check
; (vdu_buffered.h's bufferCall, `inputStream->available() == 0`) converts
; this into a jump instead of a real nested call.
cmd_cond_call_self:
    db 23, 0, $A0
    dw loop_body_buffer_id
    db VBUF_CMD_COND_CALL, VBUF_COND_NE | VBUF_COND_F_BUFVALUE
    dw loop_counter_buffer_id
    dw 0
    dw loop_zero_buffer_id
    dw 0
cmd_cond_call_self_end:
