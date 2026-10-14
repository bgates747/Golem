; GOLEM-036D: transport/dispatch only. Golem calculates all moving coordinates.
; script.bin is loaded after boot. It contains ordinary VDU bytes and expectations.
; 0=end; 1=u16 bytes,raw VDU; 2=transaction metadata(10),raw VDU.
; 3=u16 VSync count; 4=u16 handle patch offset,u16 bytes,raw VDU.
; Metadata: token:u16,kind:u8,flags:u8,value:u16,handle-patch:u16,bytes:u16.
; flags: 1=capture nonzero handle, 2=accept arbitrary value; otherwise exact value.
    .assume adl=1
    .org 0x040000
    jp start
    .align 64
    db "MOS",0,1
    include "../../../../reference/agnb-asm/lib/mos_api.inc"

start:
    push af
    push bc
    push de
    push hl
    push ix
    push iy
    ld (saved_sp),sp             ; ADL24 OK: saved stack address.
    MOSCALL mos_sysvars
    push ix
    pop iy
    ld hl,script_name
    ld de,0x050000
    ld bc,0x020000
    push iy
    MOSCALL mos_load
    pop iy
    or a
    jp nz,failed
    ld hl,packet_callback
    ld c,0
    push iy
    MOSCALL mos_setkbvector
    pop iy
    ld a,1
    ld (installed),a
    ld hl,observations
    ld (cursor),hl               ; ADL24 OK: transcript address.
    ld ix,0x050000

next_record:
    ld a,(ix+0)
    inc ix
    or a
    jp z,complete
    cp 1
    jp z,raw_record
    cp 3
    jp z,wait_frames
    cp 4
    jp z,patched_record
    cp 6
    jp z,named_record
    cp 7
    jp z,read_record
    cp 8
    jp z,pixel_record
    cp 2
    jp nz,failed
    ld a,(ix+0)
    ld (expected_token),a
    ld a,(ix+1)
    ld (expected_token+1),a
    ld a,(ix+2)
    ld (expected_kind),a
    ld a,(ix+3)
    ld (reply_flags),a
    ld a,(ix+4)
    ld (expected_value),a
    ld a,(ix+5)
    ld (expected_value+1),a
    ld de,0
    ld e,(ix+6)
    ld d,(ix+7)
    lea ix,ix+8
    call tape_word
    push ix
    pop hl
    ; FFFF means no dynamic handle patch; other offsets are packet-relative.
    ld a,d
    and e
    cp 255
    jr z,packet_ready
    push hl
    add hl,de
    ld a,(module_handle)
    ld (hl),a
    inc hl
    ld a,(module_handle+1)
    ld (hl),a
    pop hl
packet_ready:
    push hl
    add hl,bc
    push hl
    pop ix
    pop hl
    xor a
    ld (reply_ready),a
    call send
    ld de,(iy+sysvar_time)       ; ADL24 OK: low 24 bits of the centisecond clock.
wait_reply:
    ld a,(reply_ready)
    or a
    jr nz,got_reply
    ld hl,(iy+sysvar_time)       ; ADL24 OK: modular elapsed clock, not a pointer.
    or a
    sbc hl,de
    ld bc,12000                ; 120 s for the deliberate full-word loop proof.
    or a
    sbc hl,bc
    jr c,wait_reply
    jp failed
got_reply:
    di
    ld hl,mailbox
    ld de,snapshot
    ld bc,10
    ldir
    xor a
    ld (reply_ready),a
    ei
    ld a,(snapshot+4)
    cp 1
    jp nz,failed
    ld a,(expected_kind)
    ld hl,snapshot+5
    cp (hl)
    jp nz,failed
    ld a,(expected_token)
    inc hl
    cp (hl)
    jp nz,failed
    ld a,(expected_token+1)
    inc hl
    cp (hl)
    jp nz,failed
    ld a,(reply_flags)
    bit 0,a
    jr z,check_value
    ld a,(snapshot+8)
    ld (module_handle),a
    ld b,a
    ld a,(snapshot+9)
    ld (module_handle+1),a
    or b
    jp z,failed
    jr append_reply
check_value:
    bit 1,a
    jr nz,append_reply
    ld a,(expected_value)
    ld hl,snapshot+8
    cp (hl)
    jp nz,failed
    ld a,(expected_value+1)
    inc hl
    cp (hl)
    jp nz,failed
append_reply:
    ld hl,snapshot
    ld de,(cursor)              ; ADL24 OK: transcript address.
    ld bc,10
    ldir
    ld (cursor),de              ; ADL24 OK: transcript address.
    ld hl,transactions
    inc (hl)
    jp next_record

named_record:
    ld a,(ix+0)
    inc ix
    push ix
    push iy
    ld bc,0
    ld c,a
    ld hl,0
    ld l,a
    add hl,hl
    add hl,bc
    ld de,dispatch_table
    add hl,de
    push hl
    pop ix
    ld hl,(ix+0)               ; ADL24 OK: generated table contains dl code pointers.
    call indirect_call
    pop iy
    pop ix
    jp next_record
indirect_call:
    jp (hl)

read_record:
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
    ld hl,(cursor)
    ld (hl),a
    inc hl
    ld (cursor),hl
    ld a,(copy_offset)
    inc a
    ld (copy_offset),a
    jr nz,read_offset_done
    ld a,(copy_offset+1)
    inc a
    ld (copy_offset+1),a
read_offset_done:
    pop bc
    dec bc
    ld a,b
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
pixel_wait:
    bit 2,(iy+sysvar_vpd_pflags)
    jr nz,pixel_received
    ld a,(iy+0)
    sub d
    cp 240
    jp nc,failed
    jr pixel_wait
pixel_received:
    ld a,(iy+sysvar_scrpixelIndex)
    ld hl,(cursor)
    ld (hl),a
    inc hl
    ld (cursor),hl
    jp next_record
pixel_packet: db 23,0,132
pixel_x: dw 0
pixel_y: dw 0

raw_record:
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

patched_record:
    call tape_word
    push bc
    call tape_word
    pop de
    push ix
    pop hl
    push hl
    add hl,de
    ld a,(module_handle)
    ld (hl),a
    inc hl
    ld a,(module_handle+1)
    ld (hl),a
    pop hl
    push hl
    add hl,bc
    push hl
    pop ix
    pop hl
    call send
    jp next_record

wait_frames:
    call tape_word
    ld a,b
    or c
    jp z,next_record
    ld a,(iy+sysvar_time)
    ld d,a
wait_next_frame:
    ld a,(iy+sysvar_time)
    cp d
    jr z,wait_next_frame
    ld d,a
    dec bc
    ld a,b
    or c
    jr nz,wait_next_frame
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

; The callback only copies the small payload. MOS receives DE=payload address.
; The first four bytes become a null key-up record before MOS handles the key.
packet_callback:
    push af
    push bc
    push de
    push hl
    push ix
    push iy
    push de
    pop ix
    ld a,(ix+0)
    cp 'V'
    jr nz,callback_done
    ld a,(ix+1)
    cp 'D'
    jr nz,callback_done
    ld a,(ix+2)
    cp 'P'
    jr nz,callback_done
    ld a,(ix+3)
    cp 'L'
    jr nz,callback_done
    push ix
    pop hl
    ld de,mailbox
    ld bc,10
    ldir
    xor a
    ld (ix+0),a
    ld (ix+1),a
    ld (ix+2),a
    ld (ix+3),a
    inc a
    ld (reply_ready),a
callback_done:
    pop iy
    pop ix
    pop hl
    pop de
    pop bc
    pop af
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
    call remove_callback
    ld hl,(cursor)              ; ADL24 OK: transcript address.
    ld de,observations
    or a
    sbc hl,de
    push hl
    pop bc
    ld hl,result_name
    push iy
    MOSCALL mos_save
    pop iy
    or a
    jp nz,failed
    ld hl,success
    ld bc,success_end-success
    call send
    ld hl,0
    jr finish
failed:
    call remove_callback
    ; Save a compact stage/mailbox diagnostic before any potentially blocked VDU.
    ld hl,failure_name
    ld de,transactions
    ld bc,failure_state_end-transactions
    push iy
    MOSCALL mos_save
    pop iy
    ld hl,failure
    ld bc,failure_end-failure
    call send
    ld hl,1
finish:
    ld (exit_code),hl           ; ADL24 OK: return code.
    ld sp,(saved_sp)            ; ADL24 OK: restore stack on nested failure.
    pop iy
    pop ix
    pop hl
    pop de
    pop bc
    pop af
    ld hl,(exit_code)           ; ADL24 OK: return code.
    ret
remove_callback:
    ld a,(installed)
    or a
    ret z
    xor a
    ld (installed),a
    ld hl,0
    ld c,0
    push iy
    MOSCALL mos_setkbvector
    pop iy
    ret

script_name: asciz "script.bin"
result_name: asciz "results.dat"
failure_name: asciz "failure.dat"
success:
    db "GOLEM-036D: results saved.",13,10
    db "Type dir at the MOS prompt below.",13,10
    db "Close window; report animation and dir.",13,10
success_end:
failure:
    db 13,10,"VDPLET proof failed. Keep this run for the agent.",13,10
failure_end:
installed: db 0
reply_ready: db 0
transactions: db 0
reply_flags: db 0
expected_kind: db 0
expected_token: dw 0
expected_value: dw 0
module_handle: dw 0
mailbox: blkb 10,0
snapshot: blkb 10,0
failure_state_end:
saved_sp: dl 0
cursor: dl 0
exit_code: dl 0
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

include "build/active/calls.inc"
include "build/active/dispatch.inc"
observations: blkb 24576,0
