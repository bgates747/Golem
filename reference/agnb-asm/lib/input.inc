; ============================================================
; Keyboard polling template (generic — not tied to any application)
; ============================================================
; Pattern: once per frame/tick, OR the live MOS virtual keyboard map into a
; sticky local mask (set_keys), test whichever bits the application cares
; about (do_input), then clear the mask (reset_keys) once input has been
; consumed:
;
;     call set_keys
;     call do_input
;     call reset_keys
;
; See keys.inc for the (byte offset, bit) pair of every named key within
; the 16-byte virtual keyboard map used by `bit n,(ix+m)` below.

keyboard_masks: ds 16 ; 16 bytes for the keyboard masks

; check system virtual keyboard map for keys down and or them with local keyboard masks
; inputs: none
; outputs: none
; clobbers: a, b, ix, iy
set_keys:
    MOSCALL mos_getkbmap ; ix points to the virtual keyboard map
    ld iy,keyboard_masks 
    ld b,16 ; loop counter
@loop:
    ld a,(ix)
    or (iy)
    ld (iy),a
    inc ix
    inc iy
    djnz @loop
    ret

; reset the local keyboard masks to zero
; inputs: none
; outputs: none
; clobbers: a, b, iy
reset_keys:
    ld iy,keyboard_masks
    xor a
    ld b,16 ; loop counter
@loop:
    ld (iy),a
    inc iy
    djnz @loop
    ret

; ============ KEYBOARD INPUT: EXAMPLE TEMPLATE ============
; This is a skeleton, not a reusable routine: copy it into an application
; and replace the example checks with real handling. `ix` must point at
; keyboard_masks (as set up below) before any `bit n,(ix+m)` check.
do_input:
    ld ix,keyboard_masks

; ------ example: a "quit"-style key ------
; 113 Escape
    bit 0,(ix+14)
    jp nz,@quit_pressed ; replace @quit_pressed with the application's exit path

; ------ example: a pair of "adjust a value left/right" keys ------
; 26 Left
    bit 1,(ix+3)
    jp z,@Left
    ; ... application-specific "decrement" handling goes here ...
@Left:

; 122 Right
    bit 1,(ix+15)
    jp z,@Right
    ; ... application-specific "increment" handling goes here ...
@Right:
    ret

@quit_pressed:
    ret