; Assembly owns installation, calls and cleanup. No buffer IDs are handwritten.
    .assume adl=1
    .org 0x040000
    jp start
    .align 64
    db "MOS",0,1
start:
    push af
    push bc
    push de
    push ix
    push iy
    ld hl,golem_package_bootstrap
    ld bc,golem_package_bootstrap_len
    rst.lil $18

    call golem_tick

; A real application uses/retrieves any results before its final cleanup.
; The VDP processes the call and following cleanup in stream order.
    ld hl,golem_package_cleanup
    ld bc,golem_package_cleanup_len
    rst.lil $18
    pop iy
    pop ix
    pop de
    pop bc
    pop af
    ld hl,0
    ret

; Include after the application's return so execution cannot fall into payloads.
    include "build/demo/golem.inc"
