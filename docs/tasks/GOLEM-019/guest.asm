; Generic MOS/UART test transport. The host supplies a binary command tape.
; Tape: 1,u16 byteCount,raw VDU bytes | 2,u16 id,u16 offset,u16 count | 0.
; Opcode 3 calls the generated golem_drawWolf wrapper; 4 reads a stock pixel.
; Read observations are saved verbatim; no scalar arithmetic runs on the CPU.
    .assume adl=1
    .org 0x040000
    jp start
    .align 64
    db "MOS",0,1
    include "../../../reference/agnb-asm/lib/mos_api.inc"

start:
    push af
    push bc
    push de
    push hl
    push ix
    push iy
    ld (saved_sp),sp           ; ADL24 OK: three-byte stack address.
    MOSCALL mos_sysvars
    push ix
    pop iy
    ld ix,tape
    ld hl,observations
    ld (cursor),hl             ; ADL24 OK: cursor is a three-byte address.
next_record:
    ld a,(ix+0)
    inc ix
    or a
    jp z,complete
    cp 1
    jp z,send_record
    cp 5
    jp z,load_assets
    cp 4
    jp z,pixel_record
    cp 3
    jp z,named_call
    cp 2
    jp nz,failed
    ld a,(ix+0)
    ld (copy_id),a
    ld a,(ix+1)
    ld (copy_id+1),a
    ld a,(ix+2)
    ld (copy_offset),a
    ld a,(ix+3)
    ld (copy_offset+1),a
    lea ix,ix+4
    call tape_word
read_loop:
    push bc
    call query_byte
    jp c,failed
    ld hl,(cursor)             ; ADL24 OK: three-byte address.
    ld (hl),a
    inc hl
    ld (cursor),hl             ; ADL24 OK: three-byte address.
    ld a,(copy_offset)
    inc a
    ld (copy_offset),a
    jr nz,offset_done
    ld a,(copy_offset+1)
    inc a
    ld (copy_offset+1),a
offset_done:
    pop bc
    dec bc
    ld a,b                     ; Tape count is u16; BCU stays zero.
    or c
    jr nz,read_loop
    jp next_record

pixel_record:
    ld a,(ix+0)
    ld (pixel_x),a
    ld a,(ix+1)
    ld (pixel_x+1),a
    ld a,(ix+2)
    ld (pixel_y),a
    ld a,(ix+3)
    ld (pixel_y+1),a
    lea ix,ix+4
    res 2,(iy+sysvar_vpd_pflags)
    ld hl,pixel_packet
    ld bc,7
    call send
    ld a,(iy+0)
    ld d,a
wait_pixel:
    bit 2,(iy+sysvar_vpd_pflags)
    jr nz,got_pixel
    ld a,(iy+0)
    sub d
    cp 240
    jp nc,failed
    jr wait_pixel
got_pixel:
    ld a,(iy+sysvar_scrpixelIndex)
    ld hl,(cursor)
    ld (hl),a
    inc hl
    ld (cursor),hl
    jp next_record
pixel_packet: db 23,0,132
pixel_x: dw 0
pixel_y: dw 0

load_assets:
    push ix
    push iy
    ld de,asset_filename
    call agnb_load_images
    pop iy
    pop ix
    jp nz,failed
    jp next_record
asset_filename: asciz "wolf-review.agnb"

named_call:
    push ix
    push iy
    call golem_drawWolf
    pop iy
    pop ix
    jp next_record

send_record:
    call tape_word
    push ix
    pop hl
    push hl
    add hl,bc
    push hl
    pop ix
    pop hl
    call send
    jp next_record

tape_word:
    ld bc,0
    ld c,(ix+0)
    ld b,(ix+1)
    inc ix
    inc ix
    ret

send:
    push ix
    push iy
    xor a
    rst.lil 18h
    pop iy
    pop ix
    ret

query_byte:
    ld a,15
    ld (query_mask),a
    ld a,16
    ld (query_tag),a
    call query_nibble
    ret c
    and 15
    ld (low_nibble),a
    ld a,240
    ld (query_mask),a
    ld a,8
    ld (query_tag),a
    call query_nibble
    ret c
    and 240
    ld b,a
    ld a,(low_nibble)
    or b
    ret

query_nibble:
    ld a,255
    ld (iy+0x37),a
    ld hl,query_packet
    ld bc,query_end-query_packet
    call send
    ld a,(iy+0)
    ld d,a
wait_gp:
    ld a,(iy+0x37)
    cp 255
    jr nz,got_gp
    ld a,(iy+0)
    sub d
    cp 240
    jr c,wait_gp
    scf
    ret
got_gp:
    ld e,a
    ld a,(query_tag)
    cp 16
    ld a,e
    jr nz,check_high
    and 240
    cp 16
    jr check_tag
check_high:
    and 15
    cp 8
check_tag:
    jr nz,bad_tag
    ld a,e
    or a
    ret
bad_tag:
    scf
    ret

complete:
    ld hl,(cursor)             ; ADL24 OK: three-byte address.
    ld de,observations
    or a
    sbc hl,de
    push hl
    pop bc
    ld hl,result_filename
    push iy
    MOSCALL mos_save
    pop iy
    or a
    jr nz,save_failed
    ld hl,success
    ld bc,success_end-success
    call send
    ld hl,0
    jr finish
failed:
    ; Restore SP directly: failure may occur inside a read record.
    ld hl,failure
    ld bc,failure_end-failure
    call send
    ld hl,1
    jr finish
save_failed:
    ld hl,2
finish:
    ld (exit_code),hl           ; ADL24 OK: three-byte return code.
    ld sp,(saved_sp)            ; ADL24 OK: three-byte stack address.
    pop iy
    pop ix
    pop hl
    pop de
    pop bc
    pop af
    ld hl,(exit_code)           ; ADL24 OK: three-byte return code.
    ret

query_packet:
    db 23,0,160
    dw 60000
    db 5,2
    dw 3
    db 0x55
    db 23,0,160
    dw 60000
    db 5,0xe2
    dw 3,1
copy_id: dw 0
copy_offset: dw 0
    db 23,0,160
    dw 60000
    db 5,5
    dw 3
query_mask: db 15
    db 23,0,160
    dw 60000
    db 5,6
    dw 3
query_tag: db 16
    db 23,0,160
    dw 60000
    db 1
query_end:
low_nibble: db 0
cursor: dl 0
saved_sp: dl 0
exit_code: dl 0
result_filename: asciz "results.dat"
; Keep MOS text/dir scrolling below the artwork on the 512x384 review screen.
success: db 17,63,28,0,47,63,32,31,0,1
    db "GOLEM-019: Wolf bitmaps at native sizes.",13,10
    db "Type dir at the MOS prompt below.",13,10
    db "Then close this emulator and tell the agent",13,10
    db "whether dir worked and the images look right.",13,10
success_end:
failure: db 13,10,"GOLEM-019: transport FAILED",13,10
failure_end:
    include "agnb_api.inc"
    include "agnb_support.inc"
    include "build/default/golem.inc"
tape: incbin "build/tape.bin"
observations: blkb 24000,0
