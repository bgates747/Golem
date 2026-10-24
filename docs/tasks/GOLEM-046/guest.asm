; GOLEM-046: actual generic AGNB loader + generated thin integration.
 .assume adl=1
 .org $40000
 jp start
 .align 64
 db "MOS",0,1
start:
 push af
 push bc
 push de
 push hl
 push ix
 push iy
 ld (saved_sp),sp
 ld a,8
 rst.lil $08
 push ix
 pop iy
 ld hl,observations
 ld (cursor),hl
 ld hl,packet_callback
 ld c,0
 ld a,$1d
 rst.lil $08
 ld hl,query_setup
 ld bc,query_setup_end-query_setup
 call send
 include "build/scenario.inc"
 ld hl,0
 ld c,0
 ld a,$1d
 rst.lil $08
 ld hl,(cursor)
 ld de,observations
 or a
 sbc hl,de
 push hl
 pop bc
 ld hl,result_name
 ld a,2
 rst.lil $08
 ld hl,success
 ld bc,success_end-success
 call send
 jp finish
failed:
 ld hl,0
 ld c,0
 ld a,$1d
 rst.lil $08
 ld hl,failure_name
 ld de,reply
 ld bc,10
 ld a,2
 rst.lil $08
 ld hl,failure
 ld bc,failure_end-failure
 call send
finish:
 ld sp,(saved_sp)
 pop iy
 pop ix
 pop hl
 pop de
 pop bc
 pop af
 ld hl,0
 ret
send:
 push ix
 push iy
 xor a
 rst.lil $18
 pop iy
 pop ix
 ret
append_a:
 ld hl,(cursor)
 ld (hl),a
 inc hl
 ld (cursor),hl
 ret
compare_results:
 ld hl,observations
 ld de,expected
 ld bc,expected_end-expected
compare_next:
 ld a,(de)
 cp (hl)
 jp nz,failed
 inc hl
 inc de
 dec bc
 ld a,b
 or c
 jr nz,compare_next
 ret
observe:
 call query_byte
 jp c,failed
 jp append_a
; Read total, interface/language status, and the successful-call generation.
observe_state:
 ld hl,state_offsets
 ld b,state_offsets_end-state_offsets
observe_next:
 push bc
 push hl
 ld a,(hl)
 ld (copy_offset),a
 xor a
 ld (copy_offset+1),a
 call observe
 pop hl
 inc hl
 pop bc
 djnz observe_next
 ret
; Send one fixed D8 request; the fixture patches tokens and module handles.
request:
 xor a
 ld (reply_ready),a
 call send
 ld a,(iy+0)
 ld d,a
wait_reply:
 ld a,(reply_ready)
 or a
 jr nz,got_reply
 ld a,(iy+0)
 sub d
 cp 240
 jr c,wait_reply
 jp failed
got_reply:
 ld a,(reply+4)
 cp 1
 jp nz,failed
 ld a,(reply+5)
 cp 5
 jp z,failed
 ret
capture_handle:
 ld a,(reply+8)
 ld (module_handle),a
 ld b,a
 ld a,(reply+9)
 ld (module_handle+1),a
 or b
 jp z,failed
 ret
patch_handle:
 ld hl,0
 ld a,(module_handle)
 ld l,a
 ld a,(module_handle+1)
 ld h,a
 call golem_agnb_native_bind
 ret
runtime_ok:
 ld hl,info_packet
 ld bc,info_packet_end-info_packet
 call request
 ld a,(reply+8)
 call append_a
 ld a,(reply+9)
 jp append_a
unload_module:
 ld hl,module_handle
 ld de,unload_handle
 ld bc,2
 ldir
 ld hl,unload_packet
 ld bc,unload_packet_end-unload_packet
 call request
 ld a,(reply+5)
 cp 3
 jp nz,failed
 ret
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
    ld de,reply
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

query_setup:
 db 23,0,160
 dw 60010
 db 2
 db 23,0,160
 dw 60010
 db 0
 dw 4
 db 23,0,128,0
query_setup_end:
cursor: dl 0
saved_sp: dl 0
module_handle: dw 0
reply_ready: db 0
reply: blkb 10,0
result_name: db "results.dat",0
failure_name: db "failure.dat",0
failure: db "AGNB Golem test failed. Preserve this run for the agent.",13,10
failure_end:
query_packet:
    db 23,0,160
    dw 60010
    db 5,2
    dw 3
    db 0x55
    db 23,0,160
    dw 60010
    db 5,0xe2
    dw 3,1
copy_id: dw 0
copy_offset: dw 0
    db 23,0,160
    dw 60010
    db 5,5
    dw 3
query_mask: db 15
    db 23,0,160
    dw 60010
    db 5,6
    dw 3
query_tag: db 16
    db 23,0,160
    dw 60010
    db 1
query_end:
low_nibble: db 0

 include "build/parameters.inc"
 include "build/large/agnb.inc"
 include "build/visual/agnb.inc"
 include "../../../../agon-utils/examples/agnb/api/agnb_dependencies.inc"
 include "../../../../agon-utils/examples/agnb/api/agnb_api.inc"
expected: incbin "build/expected.dat"
expected_end:
observations: blkb 256,0
