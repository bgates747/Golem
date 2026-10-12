; Task-local dependencies adapted from agon-utils/examples/agnb/api/
; agnb_dependencies.inc. mos_api.inc already supplies FFSCALL and FatFS names.
; Word fields use exact byte accesses; application remains in native ADL=1.

umul24:
    push de
    push bc
    push af
    push hl
    pop bc
    ld a,24
    ld hl,0
@loop:
    add hl,hl
    ex de,hl
    add hl,hl
    ex de,hl
    jr nc,@no_add
    add hl,bc
@no_add:
    dec a
    jr nz,@loop
    pop af
    pop bc
    pop de
    ret

; HL=buffer ID, BC=length, DE=data. Retain length in three-byte local storage.
vdu_load_buffer:
    ld (@saved_length),bc      ; ADL24 OK: dl transfer length.
    ld a,c
    ld (@length),a
    ld a,b
    ld (@length+1),a
    push de
    ld a,l
    ld (@bufferId),a
    ld a,h
    ld (@bufferId+1),a
    ld hl,@cmd
    ld bc,@end-@cmd
    rst.lil $18
    pop hl
    ld bc,(@saved_length)      ; ADL24 OK: dl transfer length.
    rst.lil $18
    ret
@cmd: db 23,0,$a0
@bufferId: dw 0
    db 0
@length: dw 0
@end:
@saved_length: dl 0

vdu_clear_buffer:
    ld a,l
    ld (@bufferId),a
    ld a,h
    ld (@bufferId+1),a
    ld hl,@cmd
    ld bc,@end-@cmd
    rst.lil $18
    ret
@cmd: db 23,0,$a0
@bufferId: dw 0
    db 2
@end:

vdu_consolidate_buffer:
    ld a,l
    ld (@bufferId),a
    ld a,h
    ld (@bufferId+1),a
    ld hl,@cmd
    ld bc,@end-@cmd
    rst.lil $18
    ret
@cmd: db 23,0,$a0
@bufferId: dw 0
    db 14
@end:

vdu_buff_select:
    ld a,l
    ld (@bufferId),a
    ld a,h
    ld (@bufferId+1),a
    ld hl,@cmd
    ld bc,@end-@cmd
    rst.lil $18
    ret
@cmd: db 23,27,$20
@bufferId: dw 0
@end:

; A=format, BC=width, DE=height; no wide stores into adjacent u16 fields.
vdu_bmp_create:
    ld (@format),a
    ld a,c
    ld (@width),a
    ld a,b
    ld (@width+1),a
    ld a,e
    ld (@height),a
    ld a,d
    ld (@height+1),a
    ld hl,@cmd
    ld bc,@end-@cmd
    rst.lil $18
    ret
@cmd: db 23,27,$21
@width: dw 0
@height: dw 0
@format: db 0
@end:

; Audio is unused by this fixture, but the canonical API references this helper.
vdu_buffer_to_sound:
    add a,8
    ld (@format),a
    ld a,l
    ld (@bufferId),a
    ld a,h
    ld (@bufferId+1),a
    ld a,e
    ld (@sampleRate),a
    ld a,d
    ld (@sampleRate+1),a
    ld hl,@cmd
    ld bc,@end-@cmd
    rst.lil $18
    ret
@cmd: db 23,0,$85,0,5,2
@bufferId: dw 0
@format: db 0
@sampleRate: dw 0
@end:
