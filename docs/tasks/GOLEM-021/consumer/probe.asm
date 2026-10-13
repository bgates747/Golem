; GOLEM-021: MOS-side native module proof. No numerical kernel runs on the eZ80.
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
    ld bc,0x010000
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
    ld a,(iy+sysvar_time)
    ld d,a
wait_reply:
    ld a,(reply_ready)
    or a
    jr nz,got_reply
    ld a,(iy+sysvar_time)
    sub d
    cp 240
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
    db "VDPLET: uploaded native code passed.",13,10
    db "Type dir at the MOS prompt below.",13,10
    db "Close window; report display and dir.",13,10
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
observations: blkb 4096,0
