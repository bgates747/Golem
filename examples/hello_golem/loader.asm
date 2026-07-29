; loader.asm
;
; Minimal, GENERIC eZ80/MOS "loader" (bootstrap) for Golem-compiled program
; blobs. Unlike examples/hello/hello.asm (which hand-encodes its own
; greeting directly, in eZ80 code, using runtime/legacy-asm/vdu_buffered_api.inc),
; this loader has NO knowledge of what program.bin contains - it just:
;
;   1. reads program.bin (a flat binary standing in for whatever a real
;      Golem compiler would eventually emit - see program.asm/hello.golem
;      in this directory) into memory via the MOS file API,
;   2. writes it verbatim into VDP buffer 0 (Buffered Commands API command
;      0, via vbuf_write - the loader never inspects or parses the bytes),
;   3. calls that buffer (command 1, via vbuf_call).
;
; This is a quick proof-of-concept for the intended split between "compiled
; Golem program" (opaque buffer-payload bytes) and "loader" (a generic
; upload+call bootstrap that is the same for every Golem program) described
; in docs/devlog/2026-07-28.md. It is NOT the real loader: a real one will
; need to handle multi-block/compressed programs, split/spread them across
; many buffer IDs, and likely won't need MOS-side file I/O at all once
; Golem has its own build/upload pipeline (see the devlog's "Loader/linker"
; note). What this DOES prove: program.bin could be swapped for the output
; of any future Golem compiler without changing a single line of this file
; - the loader is generic, not hello-world-specific.
;
; Build: ez80asm loader.asm loader.bin
; Run:   copy both loader.bin and program.bin to the SD card (see
;        examples/hello/emulator for a working emulator profile), then
;        `load loader.bin` + `run .` from MOS.

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
    include "../../reference/agnb-asm/lib/mos_api.inc"

; Buffer ID the loaded program will live in - arbitrary, just needs to be a
; value not otherwise in use.
program_buffer_id: equ 0

; Max bytes read from program.bin in one go. Plenty for a trivial
; hello-world-sized blob; a real loader would loop over fixed-size chunks
; (see vdu_load_buffer_from_file in reference/agnb-asm/lib/vdu.inc for the
; chunked/looping version this quick demo intentionally simplifies away).
max_program_size: equ 512

main:
    ; Open program.bin for reading.
    ld hl,program_filename
    ld c,fa_read
    MOSCALL mos_fopen
    or a
    ret z ; couldn't open the file - bail out quietly

    ld (@filehandle),a
    ld c,a
    ld hl,program_data
    ld de,max_program_size
    MOSCALL mos_fread
    ld (@bytes_read),de

    ld a,(@filehandle)
    ld c,a
    MOSCALL mos_fclose

    ; Write the loaded bytes into the program's VDP buffer, exactly as
    ; read - the loader never interprets them.
    ld hl,program_buffer_id
    ld de,program_data
    ld bc,(@bytes_read)
    call vbuf_write

    ; Execute the loaded program.
    ld hl,program_buffer_id
    call vbuf_call

    ret

@filehandle: db 0
@bytes_read: dw24 0

program_filename: db "program.bin",0

program_data:
    blkb max_program_size,0
