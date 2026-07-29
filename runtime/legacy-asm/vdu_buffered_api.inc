; vdu_buffered_api.inc
;
; eZ80 assembly wrapper library for the Agon Console8 VDP's "Buffered
; Commands API" (`VDU 23, 0, &A0, bufferId; command, [args]`).
;
; This is ORIGINAL Golem toolchain code (not vendored) - see
; reference/agnb-asm/ for the vendored eZ80/MOS idiom reference this file's
; conventions are drawn from (RST.LIL $18 as the raw "send N bytes from
; (HL) to the VDP" primitive, dw24 for 24-bit scratch cells, self-modifying
; command templates a la vdu_load_buffer in reference/agnb-asm/lib/includes.inc).
;
; Purpose: primarily, this is a standalone library for hand-written eZ80/MOS
; assembly applications that want to use the Buffered Commands API directly
; - build a command sequence once, upload it to a buffer, and thereafter
; trigger it with a short "call buffer" (command 2) instead of re-streaming
; the whole sequence over the (slow) UART on every use. That's a real win
; independent of Golem: setting up a non-trivial buffered command (an affine
; transform, a multi-field bitmap draw, a conditional/looping sequence) by
; hand, byte by byte, gets tedious and error-prone fast, and repeatedly
; re-sending long byte streams just to re-trigger the same operation ties up
; the eZ80 on a slow synchronous link when it could be doing other work.
;
; Secondarily, several of these routines (and the calling convention below)
; are intended to double as a baseline for a future Golem compiler/loader
; component (running on the eZ80 side, in ADL/24-bit mode) that constructs
; and uploads Buffered Commands API command blocks as part of Golem's own
; build/load pipeline. This is NOT part of Golem's compiled program output
; (which runs entirely inside VDP buffers, per docs/devlog/2026-07-28.md) -
; it's tooling/runtime support code for whatever builds and uploads those
; buffers from the eZ80 side, whether that's this library used directly by
; a human-written program, or a future compiler backend reusing its idioms.
;
; Every routine here was written and cross-checked against BOTH:
;   - /home/smith/Agon/agon-docs/docs/vdp/Buffered-Commands-API.md (prose docs)
;   - /home/smith/Agon/agon-vdp/video/vdu_buffered.h (firmware source, the
;     definitive source of truth when the two disagree - see golem/AGENTS.md)
; as of VDP firmware version referenced by that checkout (2.9.0/2.10.0-era,
; per the affine-transform commands' "added in" notes in the docs).
;
; See docs/devlog/2026-07-28-vdu-buffered-api.md for a per-command
; straightforward/needs-inspection/punted classification and rationale.
;
; ---------------------------------------------------------------------------
; CALLING CONVENTION (applies to every routine below unless noted otherwise)
; ---------------------------------------------------------------------------
;   hl = the command's primary bufferId (the buffer the command applies to;
;        this is always the "bufferId;" that appears right after `&A0` in
;        the VDU sequence). 65535 (VBUF_NONE) means "the currently-executing
;        buffer" wherever the docs say that's valid.
;   a  = an 8-bit operation/options/flags byte, when the command has one.
;   de = a secondary buffer ID, a 16-bit numeric argument, or (for a few
;        commands) a 24-bit "advanced offset" value, per each routine's own
;        header comment.
;   bc = a third numeric argument (length/count/width/offset/checkOffset),
;        per each routine's own header comment.
;   ix, iy = used as extra argument slots (plain 16-bit values, or pointers
;        to caller-built scratch data) only for the handful of commands that
;        need more than 3 register-pairs of input. Always documented
;        per-routine when used.
;
; For arguments that are inherently variable-shape (terminated buffer-ID
; lists, "advanced"/buffer-fetched operand encodings, matrix arguments,
; bitmap-expansion mapping tables, ...) this library does NOT allocate or
; manage scratch memory on the caller's behalf. Instead the caller (in
; practice, eventually, the Golem compiler's own backend) is expected to
; build the correctly-shaped tail of bytes into its own bounded scratch
; buffer and hand this library a pointer + length. See `vbuf_raw` below -
; every command can be expressed through it directly if no friendlier
; wrapper is provided, or if you need a modifier-bit combination a
; friendlier wrapper doesn't cover.
;
; All routines destroy at least af, bc, hl; most also destroy de (and, when
; noted, ix/iy). None preserve any register not explicitly documented as
; surviving. None of these routines touch the actual VDP reply/input stream
; (RST.LIL $18 is fire-and-forget, matching how the rest of this codebase's
; VDU helpers work); callers are responsible for the usual MOS/VDP protocol
; considerations (e.g. not overrunning the VDP's input buffer).
;
; ---------------------------------------------------------------------------


; ===========================================================================
; Command number constants
; ===========================================================================
VBUF_CMD_WRITE:             equ 0
VBUF_CMD_CALL:               equ 1
VBUF_CMD_CLEAR:              equ 2
VBUF_CMD_CREATE:             equ 3
VBUF_CMD_SET_OUTPUT:         equ 4
VBUF_CMD_ADJUST:             equ 5
VBUF_CMD_COND_CALL:          equ 6
VBUF_CMD_JUMP:               equ 7
VBUF_CMD_COND_JUMP:          equ 8
VBUF_CMD_JUMP_OFFSET:        equ 9
VBUF_CMD_COND_JUMP_OFFSET:   equ 10
VBUF_CMD_CALL_OFFSET:        equ 11
VBUF_CMD_COND_CALL_OFFSET:   equ 12
VBUF_CMD_COPY:               equ 13
VBUF_CMD_CONSOLIDATE:        equ 14
VBUF_CMD_SPLIT:              equ 15
VBUF_CMD_SPLIT_INTO:         equ 16
VBUF_CMD_SPLIT_FROM:         equ 17
VBUF_CMD_SPLIT_BY:           equ 18
VBUF_CMD_SPLIT_BY_INTO:      equ 19
VBUF_CMD_SPLIT_BY_FROM:      equ 20
VBUF_CMD_SPREAD_INTO:        equ 21
VBUF_CMD_SPREAD_FROM:        equ 22
VBUF_CMD_REVERSE_BLOCKS:     equ 23
VBUF_CMD_REVERSE:            equ 24
VBUF_CMD_COPY_REF:           equ 25
VBUF_CMD_COPY_CONSOLIDATE:   equ 26
VBUF_CMD_MATRIX2D:           equ 32
VBUF_CMD_MATRIX3D:           equ 33
VBUF_CMD_MATRIX:             equ 34
VBUF_CMD_TRANSFORM_BITMAP:   equ 40
VBUF_CMD_TRANSFORM_DATA:     equ 41
VBUF_CMD_READ_VARIABLE:      equ 48
VBUF_CMD_COMPRESS:           equ 64
VBUF_CMD_DECOMPRESS:         equ 65
VBUF_CMD_EXPAND_BITMAP:      equ 72
VBUF_CMD_ADD_CALLBACK:       equ 80
VBUF_CMD_REMOVE_CALLBACK:    equ 81
VBUF_CMD_DEBUG_INFO:         equ 128

; Common sentinel: -1 as an unsigned 16-bit value. Used as "current buffer"
; for several bufferId arguments, as the terminator for buffer-ID lists, and
; as the "all types" wildcard for callback commands.
VBUF_NONE: equ 65535

; --- Command 5 (Adjust) operation codes (low 3 bits of the operation byte) -
VBUF_ADJ_NOT: equ 0
VBUF_ADJ_NEG: equ 1
VBUF_ADJ_SET: equ 2
VBUF_ADJ_ADD: equ 3
VBUF_ADJ_ADC: equ 4
VBUF_ADJ_AND: equ 5
VBUF_ADJ_OR:  equ 6
VBUF_ADJ_XOR: equ 7
; --- Command 5 (Adjust) modifier flag bits (upper bits of the operation byte)
VBUF_ADJ_F_ADVANCED: equ $10 ; use advanced (24-bit) offsets
VBUF_ADJ_F_BUFVALUE: equ $20 ; operand is fetched from a buffer, not the stream
VBUF_ADJ_F_MULTI_TGT: equ $40 ; adjust `count` target bytes, not just one
VBUF_ADJ_F_MULTI_OP:  equ $80 ; read `count` operand values, not just one

; --- Commands 6/8/10/12 (Conditional) operation codes (low 4 bits) ---------
VBUF_COND_EXISTS:     equ 0
VBUF_COND_NOT_EXISTS: equ 1
VBUF_COND_EQ:         equ 2
VBUF_COND_NE:         equ 3
VBUF_COND_LT:         equ 4
VBUF_COND_GT:         equ 5
VBUF_COND_LE:         equ 6
VBUF_COND_GE:         equ 7
VBUF_COND_AND:        equ 8
VBUF_COND_OR:         equ 9
; --- Commands 6/8/10/12 (Conditional) modifier flag bits -------------------
VBUF_COND_F_ADVANCED: equ $10 ; use advanced (24-bit) offsets
VBUF_COND_F_BUFVALUE: equ $20 ; operand is fetched from a buffer, not the stream
VBUF_COND_F_VARIABLE: equ $40 ; check a VDP variable/feature flag, not a buffer byte
VBUF_COND_F_16BIT:    equ $80 ; source/operand are 16-bit, not 8-bit

; --- Command 24 (Reverse) options bits --------------------------------------
VBUF_REV_16BIT:   equ 1
VBUF_REV_32BIT:   equ 2
VBUF_REV_SIZE:    equ 3 ; bits 0+1 both set = explicit valueSize follows
VBUF_REV_CHUNKED: equ 4
VBUF_REV_BLOCKS:  equ 8

; --- Command 48 (Read VDP variable) options bits ----------------------------
VBUF_RDVAR_BIGENDIAN: equ $01
VBUF_RDVAR_ADVANCED:  equ $10
VBUF_RDVAR_DEFAULT:   equ $40
VBUF_RDVAR_16BIT:     equ $80

; --- Command 72 (Expand bitmap) options bits (bits 0-2 = bpp, 0 means 8) ---
VBUF_EXPAND_F_ALIGN:  equ $08 ; explicit `width;` follows sourceBufferId;
VBUF_EXPAND_F_MAPBUF: equ $10 ; mapping data comes from a buffer, not inline

; --- Commands 32/33 (2D/3D affine transform) operation codes ----------------
VBUF_MATRIX_SET_IDENTITY:   equ 0  ; no arguments
VBUF_MATRIX_INVERT:         equ 1  ; no arguments
VBUF_MATRIX_ROTATE_DEG:     equ 2  ; 1 arg (3 for 3D) - float/fixed
VBUF_MATRIX_ROTATE_RAD:     equ 3  ; 1 arg (3 for 3D) - float/fixed
VBUF_MATRIX_MULTIPLY_ALL:   equ 4  ; 1 arg - float/fixed
VBUF_MATRIX_SCALE:          equ 5  ; 2 args (3 for 3D) - float/fixed
VBUF_MATRIX_TRANSLATE_PX:   equ 6  ; 2 args (3 for 3D) - float/fixed
VBUF_MATRIX_TRANSLATE_UNIT: equ 7  ; 2 args (3 for 3D) - float/fixed
VBUF_MATRIX_SHEAR:          equ 8  ; 2 args (3 for 3D) - float/fixed
VBUF_MATRIX_SKEW_DEG:       equ 9  ; 2 args (3 for 3D) - float/fixed
VBUF_MATRIX_SKEW_RAD:       equ 10 ; 2 args (3 for 3D) - float/fixed
VBUF_MATRIX_COMBINE:        equ 11 ; 6 args (12 for 3D) - float/fixed
VBUF_MATRIX_TRANSLATE_BMP:  equ 12 ; bitmapId; + 2 args - float/fixed
; --- Commands 32/33 modifier flag bits --------------------------------------
VBUF_MATRIX_F_ADVANCED:  equ $10 ; use advanced (24-bit) offsets (buffer-fetched args)
VBUF_MATRIX_F_BUFVALUE:  equ $20 ; fetch argument value(s) from a buffer
VBUF_MATRIX_F_SEP_FMT:   equ $40 ; each argument gets its own format byte
; --- Commands 32/33/34/41 argument "format" byte bits -----------------------
VBUF_FMT_SHIFT_MASK: equ $1F ; fixed-point shift amount (ignored for float)
VBUF_FMT_FIXED:      equ $40 ; fixed-point, not IEEE-754 float
VBUF_FMT_16BIT:      equ $80 ; 16-bit values, not 32-bit

; --- Command 34 (arbitrary matrix) operation codes --------------------------
VBUF_MX_SET_VALUES:   equ 0  ; format, <values...>
VBUF_MX_SET_ELEMENT:  equ 1  ; sourceBufferId; row, column, format, <value>
VBUF_MX_FILL:         equ 2  ; format, <value>
VBUF_MX_DIAGONAL:     equ 3  ; format, <value>
VBUF_MX_ADD:          equ 4  ; sourceBufferId1; sourceBufferId2;
VBUF_MX_SUBTRACT:     equ 5  ; sourceBufferId1; sourceBufferId2;
VBUF_MX_MULTIPLY:     equ 6  ; sourceBufferId1; sourceBufferId2;
VBUF_MX_SCALE:        equ 7  ; sourceBufferId; format, <value>
VBUF_MX_SUBMATRIX:    equ 8  ; sourceBufferId; row, column
VBUF_MX_INSERT_ROW:   equ 9  ; sourceBufferId; row
VBUF_MX_INSERT_COL:   equ 10 ; sourceBufferId; column
VBUF_MX_REMOVE_ROW:   equ 11 ; sourceBufferId; row
VBUF_MX_REMOVE_COL:   equ 12 ; sourceBufferId; column
; --- Command 34 modifier flag bits ------------------------------------------
VBUF_MX_F_ADVANCED: equ $10
VBUF_MX_F_BUFVALUE: equ $20

; --- Command 40 (transformed bitmap) options bits ---------------------------
VBUF_XFORM_BMP_RESIZE:         equ 1
VBUF_XFORM_BMP_EXPLICIT_SIZE:  equ 2 ; width; height; follow sourceBitmapId;
VBUF_XFORM_BMP_AUTO_TRANSLATE: equ 4

; --- Command 41 (transform data) options bits -------------------------------
VBUF_XFORM_DATA_HAS_SIZE:   equ $01 ; explicit `size` (1 byte) follows format
VBUF_XFORM_DATA_HAS_OFFSET: equ $02 ; explicit `offset;` follows
VBUF_XFORM_DATA_HAS_STRIDE: equ $04 ; explicit `stride;` follows
VBUF_XFORM_DATA_HAS_LIMIT:  equ $08 ; explicit `limit;` follows
VBUF_XFORM_DATA_ADVANCED:   equ $10 ; use advanced (24-bit) offsets
VBUF_XFORM_DATA_BUF_ARGS:   equ $20 ; size/offset/stride/limit come from buffers
VBUF_XFORM_DATA_PER_BLOCK:  equ $40 ; apply transform block-by-block

; --- Commands 80/81 (callbacks) event type constants ------------------------
VBUF_CB_VSYNC:       equ 0
VBUF_CB_MODE_CHANGE: equ 1
VBUF_CB_KEYBOARD:    equ 2
VBUF_CB_MOUSE:       equ 3
VBUF_CB_PALETTE:     equ 4
VBUF_CB_READ_PIXEL:  equ 5
; VBUF_NONE (65535) is also valid as the `type` argument to command 81, to
; remove a buffer's registration from every callback type at once.


; ===========================================================================
; Low-level primitives
; ===========================================================================

; Emit the common ``VDU 23, 0, &A0, bufferId; command`` prefix shared by
; every Buffered Commands API command.
; inputs: hl = bufferId, a = command number
; destroys: af, bc, hl
vbuf_header:
    ld (@bufferId),hl
    ld (@command),a
    ld hl,@cmd
    ld bc,@end-@cmd
    rst.lil $18
    ret
@cmd:      db 23,0,$A0
@bufferId: dw 0
@command:  db 0
@end:

; Generic escape hatch: emit the standard prefix for `command`, then blast a
; caller-supplied, pre-built tail of arbitrary bytes. This can express *any*
; Buffered Commands API command - use it directly for commands with no
; friendlier wrapper below, or to reach a modifier-bit combination a
; friendlier wrapper doesn't implement. The caller owns building `tail` (this
; is the "bounded dynamic scratch memory" referred to throughout this file).
; inputs: hl = bufferId, a = command number, de = pointer to tail bytes,
;         bc = length of tail (may be 0)
; destroys: af, bc, de, hl
vbuf_raw:
    ld (@tail_ptr),de
    ld (@tail_len),bc
    call vbuf_header
    ld hl,(@tail_ptr)
    ld bc,(@tail_len)
    ld a,b
    or c
    ret z
    rst.lil $18
    ret
@tail_ptr: dw24 0
@tail_len: dw24 0


; ===========================================================================
; Commands with no arguments beyond the bufferId+command prefix
; (STRAIGHTFORWARD)
; ===========================================================================

; Command 1: call a buffer.
; inputs: hl = bufferId (65535 = current buffer)
; destroys: af, bc, hl
vbuf_call:
    ld a,VBUF_CMD_CALL
    jp vbuf_header

; Command 2: clear (delete) a buffer, or all buffers if hl = 65535.
; inputs: hl = bufferId (65535 = all buffers)
; destroys: af, bc, hl
vbuf_clear:
    ld a,VBUF_CMD_CLEAR
    jp vbuf_header

; Command 7: jump to a buffer (like call, but does not push a return address;
; the VDP will convert a trailing conditional/unconditional call into a jump
; automatically, so explicit use of this is mostly for non-tail jumps).
; inputs: hl = bufferId (65535 = current buffer)
; destroys: af, bc, hl
vbuf_jump:
    ld a,VBUF_CMD_JUMP
    jp vbuf_header

; Command 14: consolidate all blocks in a buffer into a single block.
; inputs: hl = bufferId
; destroys: af, bc, hl
vbuf_consolidate:
    ld a,VBUF_CMD_CONSOLIDATE
    jp vbuf_header

; Command 23: reverse the order of the blocks within a buffer (does not
; affect the byte order of data within each block - see vbuf_reverse/command
; 24 for that).
; inputs: hl = bufferId
; destroys: af, bc, hl
vbuf_reverse_blocks:
    ld a,VBUF_CMD_REVERSE_BLOCKS
    jp vbuf_header

; Command 128: emit a debug-info log line on the VDP for a buffer (useful
; only when the VDP firmware's debug logging is enabled).
; inputs: hl = bufferId
; destroys: af, bc, hl
vbuf_debug_info:
    ld a,VBUF_CMD_DEBUG_INFO
    jp vbuf_header

; Command 4: set/restore/detach the VDU output stream to write into a
; buffer. NB per the API docs this is a fairly heavyweight/unusual thing to
; do; a VDP variable is usually the better tool for capturing small amounts
; of VDP-computed data. No further arguments beyond the prefix.
; inputs: hl = bufferId (65535 = detach current capture, 0 = restore default
;         output stream - see Buffered-Commands-API.md#command-4 for the
;         full semantics of the special values)
; destroys: af, bc, hl
vbuf_set_output_stream:
    ld a,VBUF_CMD_SET_OUTPUT
    jp vbuf_header


; ===========================================================================
; Commands with one or two fixed 16-bit arguments (STRAIGHTFORWARD)
; ===========================================================================

; Command 3: create a new, empty, writeable buffer of a given size.
; inputs: hl = bufferId, bc = length (max size in bytes)
; destroys: af, bc, hl
vbuf_create:
    ld (@length),bc
    ld a,VBUF_CMD_CREATE
    call vbuf_header
    ld hl,@length
    ld bc,2
    rst.lil $18
    ret
@length: dw24 0

; Command 0: write a data block to a buffer (creates the buffer, or adds a
; new block to an existing one).
; inputs: hl = bufferId, de = pointer to data, bc = data length (1-65535)
; destroys: af, bc, de, hl
vbuf_write:
    ld (@data_ptr),de
    ld (@length),bc
    ld a,VBUF_CMD_WRITE
    call vbuf_header
    ld hl,@length
    ld bc,2
    rst.lil $18
    ld hl,(@data_ptr)
    ld bc,(@length)
    rst.lil $18
    ret
@data_ptr: dw24 0
@length:   dw24 0

; Command 15: split a buffer in-place into fixed-size blocks.
; inputs: hl = bufferId, bc = blockSize
; destroys: af, bc, hl
vbuf_split:
    ld (@blockSize),bc
    ld a,VBUF_CMD_SPLIT
    call vbuf_header
    ld hl,@blockSize
    ld bc,2
    rst.lil $18
    ret
@blockSize: dw24 0

; Command 17: split a buffer into fixed-size blocks and spread them across a
; run of consecutive target buffer IDs starting at `targetBufferId`.
; inputs: hl = bufferId, bc = blockSize, de = targetBufferId
; destroys: af, bc, de, hl
vbuf_split_from:
    ; blockSize is not the last field in the combined 4-byte send below, so
    ; it must be written byte-by-byte (a 24-bit `ld (nn),bc` store would
    ; spill its upper byte into targetBufferId's first byte).
    ld a,c
    ld (@blockSize),a
    ld a,b
    ld (@blockSize+1),a
    ld (@targetBufferId),de
    ld a,VBUF_CMD_SPLIT_FROM
    call vbuf_header
    ld hl,@blockSize
    ld bc,4
    rst.lil $18
    ret
@blockSize:      dw 0
@targetBufferId: dw24 0

; Command 18: split a buffer in-place by width into a given number of
; blocks/chunks.
; inputs: hl = bufferId, bc = width, de = blockCount
; destroys: af, bc, de, hl
vbuf_split_by:
    ; width is not the last field in the combined 4-byte send below, so it
    ; must be written byte-by-byte (see vbuf_split_from for why).
    ld a,c
    ld (@width),a
    ld a,b
    ld (@width+1),a
    ld (@blockCount),de
    ld a,VBUF_CMD_SPLIT_BY
    call vbuf_header
    ld hl,@width
    ld bc,4
    rst.lil $18
    ret
@width:      dw 0
@blockCount: dw24 0

; Command 20: split a buffer by width into a given number of blocks/chunks,
; and spread them across a run of consecutive target buffer IDs.
; inputs: hl = bufferId, bc = width, de = blockCount, ix = targetBufferId
; destroys: af, bc, de, hl
vbuf_split_by_from:
    ; width and blockCount are not the last field in the combined 6-byte
    ; send below, so both must be written byte-by-byte (see
    ; vbuf_split_from for why).
    ld a,c
    ld (@width),a
    ld a,b
    ld (@width+1),a
    ld a,e
    ld (@blockCount),a
    ld a,d
    ld (@blockCount+1),a
    push ix
    pop hl
    ld (@targetBufferId),hl
    ld a,VBUF_CMD_SPLIT_BY_FROM
    call vbuf_header
    ld hl,@width
    ld bc,6
    rst.lil $18
    ret
@width:          dw 0
@blockCount:     dw 0
@targetBufferId: dw24 0

; Command 22: spread the blocks of a buffer across a run of consecutive
; target buffer IDs starting at `targetBufferId` (one block per buffer, no
; splitting).
; inputs: hl = bufferId, de = targetBufferId
; destroys: af, bc, de, hl
vbuf_spread_from:
    ld (@targetBufferId),de
    ld a,VBUF_CMD_SPREAD_FROM
    call vbuf_header
    ld hl,@targetBufferId
    ld bc,2
    rst.lil $18
    ret
@targetBufferId: dw24 0

; Command 64: compress a buffer's blocks into a new buffer (replaces the
; target buffer).
; inputs: hl = bufferId (destination), de = sourceBufferId
; destroys: af, bc, de, hl
vbuf_compress:
    ld (@sourceBufferId),de
    ld a,VBUF_CMD_COMPRESS
    call vbuf_header
    ld hl,@sourceBufferId
    ld bc,2
    rst.lil $18
    ret
@sourceBufferId: dw24 0

; Command 65: decompress a (previously compressed) buffer into a new buffer
; (replaces the target buffer).
; inputs: hl = bufferId (destination), de = sourceBufferId
; destroys: af, bc, de, hl
vbuf_decompress:
    ld (@sourceBufferId),de
    ld a,VBUF_CMD_DECOMPRESS
    call vbuf_header
    ld hl,@sourceBufferId
    ld bc,2
    rst.lil $18
    ret
@sourceBufferId: dw24 0

; Command 80: register a buffer to be called back on a VDP event.
; NB: this is a VDP-internal mechanism only - it registers a buffer to be
; *executed by the VDP itself* when a VDP-side event (vsync, mode change,
; keyboard, mouse, palette, read-pixel) fires. It does NOT install, invoke,
; or otherwise touch an eZ80/MOS interrupt vector, and does NOT cause any
; packet to be sent back to the eZ80 - the eZ80 has no visibility into
; whether or when this callback buffer actually ran. In particular,
; VBUF_CB_KEYBOARD here is unrelated to MOS's own `mos_setkbvector`
; keyboard-callback ABI: the former runs a VDP buffer when the VDP sees a
; keystroke; the latter is a completely separate eZ80-side mechanism for
; MOS to hand a received UART packet to application code (as exploited,
; e.g., by the Pingo render-completion notification described in
; docs/devlog/from_codex.md, which repurposes MOS's stock keyboard-vector
; ABI and PACKET_KEYCODE framing to carry an unrelated one-way completion
; signal - nothing to do with this command at all).
; inputs: hl = bufferId, de = type (VBUF_CB_*)
; destroys: af, bc, de, hl
vbuf_add_callback:
    ld (@type),de
    ld a,VBUF_CMD_ADD_CALLBACK
    call vbuf_header
    ld hl,@type
    ld bc,2
    rst.lil $18
    ret
@type: dw24 0

; Command 81: remove a buffer's registration for a callback event type.
; inputs: hl = bufferId, de = type (VBUF_CB_*, or VBUF_NONE for all types)
; destroys: af, bc, de, hl
vbuf_remove_callback:
    ld (@type),de
    ld a,VBUF_CMD_REMOVE_CALLBACK
    call vbuf_header
    ld hl,@type
    ld bc,2
    rst.lil $18
    ret
@type: dw24 0


; ===========================================================================
; Commands taking a terminated list of 16-bit buffer IDs (STRAIGHTFORWARD -
; via the generic vbuf_raw primitive, since the entire tail *is* the list)
; ===========================================================================

; Command 13: copy blocks from multiple source buffers into this buffer (by
; value - each source's blocks are copied in turn).
; inputs: hl = bufferId (destination), de = pointer to a caller-built list
;         of 16-bit source buffer IDs terminated with VBUF_NONE (65535),
;         bc = length of that list in bytes (including the terminator)
; destroys: af, bc, de, hl
vbuf_copy:
    ld a,VBUF_CMD_COPY
    jp vbuf_raw

; Command 21: spread the blocks of this buffer across a caller-supplied list
; of target buffer IDs (one block per listed buffer).
; inputs: hl = bufferId, de = pointer to a caller-built list of 16-bit
;         target buffer IDs terminated with VBUF_NONE, bc = length of that
;         list in bytes (including the terminator)
; destroys: af, bc, de, hl
vbuf_spread_into:
    ld a,VBUF_CMD_SPREAD_INTO
    jp vbuf_raw

; Command 25: copy blocks from multiple source buffers into this buffer "by
; reference" (blocks are shared, not duplicated - see the API docs for the
; buffer-lifetime implications of this before using it).
; inputs: hl = bufferId (destination), de = pointer to a caller-built list
;         of 16-bit source buffer IDs terminated with VBUF_NONE, bc = length
;         of that list in bytes (including the terminator)
; destroys: af, bc, de, hl
vbuf_copy_ref:
    ld a,VBUF_CMD_COPY_REF
    jp vbuf_raw

; Command 26: copy blocks from multiple source buffers into this buffer and
; then consolidate the result into a single block.
; inputs: hl = bufferId (destination), de = pointer to a caller-built list
;         of 16-bit source buffer IDs terminated with VBUF_NONE, bc = length
;         of that list in bytes (including the terminator)
; destroys: af, bc, de, hl
vbuf_copy_consolidate:
    ld a,VBUF_CMD_COPY_CONSOLIDATE
    jp vbuf_raw

; Command 16: split a buffer into fixed-size blocks and spread them across a
; caller-supplied list of target buffer IDs.
; inputs: hl = bufferId, bc = blockSize, de = pointer to a caller-built list
;         of 16-bit target buffer IDs terminated with VBUF_NONE, ix = length
;         of that list in bytes (including the terminator)
; destroys: af, bc, de, hl, ix
vbuf_split_into:
    ld (@blockSize),bc
    ld (@list_ptr),de
    push ix
    pop hl
    ld (@list_len),hl
    ld a,VBUF_CMD_SPLIT_INTO
    call vbuf_header
    ld hl,@blockSize
    ld bc,2
    rst.lil $18
    ld hl,(@list_ptr)
    ld bc,(@list_len)
    rst.lil $18
    ret
@blockSize: dw24 0
@list_ptr:  dw24 0
@list_len:  dw24 0

; Command 19: split a buffer by width into blocks (one per listed buffer)
; and spread them across a caller-supplied list of target buffer IDs (the
; number of chunks is implied by the length of the list - no separate count
; argument is sent).
; inputs: hl = bufferId, bc = width, de = pointer to a caller-built list of
;         16-bit target buffer IDs terminated with VBUF_NONE, ix = length of
;         that list in bytes (including the terminator)
; destroys: af, bc, de, hl, ix
vbuf_split_by_into:
    ld (@width),bc
    ld (@list_ptr),de
    push ix
    pop hl
    ld (@list_len),hl
    ld a,VBUF_CMD_SPLIT_BY_INTO
    call vbuf_header
    ld hl,@width
    ld bc,2
    rst.lil $18
    ld hl,(@list_ptr)
    ld bc,(@list_len)
    rst.lil $18
    ret
@width:     dw24 0
@list_ptr:  dw24 0
@list_len:  dw24 0


; ===========================================================================
; Commands 9/11: jump/call to an offset within a buffer (NEEDS INSPECTION -
; the offset for these two commands is *always* sent in "advanced offset"
; form, i.e. a 24-bit value; friendly wrappers below only cover the common
; "no explicit block index" case)
; ===========================================================================

; Command 9: jump to an offset in a buffer (offset within the buffer's own
; block layout - bit 23 of the 24-bit offset must be clear for this form).
; For the block-index form (bit 23 set, followed by a 16-bit block index)
; build the 5-byte offset yourself and use vbuf_raw - see
; Buffered-Commands-API.md#command-9 and vdu_buffered.h's
; `getOffsetFromStream`.
; inputs: hl = bufferId (65535 = current buffer), de = 24-bit offset (bit 23
;         clear)
; destroys: af, bc, de, hl
vbuf_jump_offset:
    ld (@offset),de
    ld a,VBUF_CMD_JUMP_OFFSET
    call vbuf_header
    ld hl,@offset
    ld bc,3
    rst.lil $18
    ret
@offset: dw24 0

; Command 11: call an offset in a buffer (same offset caveats as
; vbuf_jump_offset/command 9 above).
; inputs: hl = bufferId (65535 = current buffer), de = 24-bit offset (bit 23
;         clear)
; destroys: af, bc, de, hl
vbuf_call_offset:
    ld (@offset),de
    ld a,VBUF_CMD_CALL_OFFSET
    call vbuf_header
    ld hl,@offset
    ld bc,3
    rst.lil $18
    ret
@offset: dw24 0


; ===========================================================================
; Commands 5/6/8 - Adjust and Conditional call/jump (NEEDS INSPECTION -
; friendly wrappers below only cover the "basic" (non-advanced, non-
; buffer-fetched-operand, single value) form of the operation byte; use
; vbuf_raw for any of the &10/&20/&40/&80 modifier-bit combinations, per
; the VBUF_ADJ_F_*/VBUF_COND_F_* constants above and
; Buffered-Commands-API.md#command-5 / #command-6)
; ===========================================================================

; Command 5: adjust buffer contents (basic form - single byte target,
; non-advanced 16-bit offset, non-buffer-fetched operand, single operand).
; inputs: hl = bufferId (65535 = current buffer), a = operation (VBUF_ADJ_*,
;         no modifier flag bits set), de = offset (16-bit), c = operand
;         (ignored for VBUF_ADJ_NOT/VBUF_ADJ_NEG)
; destroys: af, bc, de, hl
vbuf_adjust:
    ld (@op),a
    ld (@offset),de
    ld a,c
    ld (@operand),a
    ld a,VBUF_CMD_ADJUST
    call vbuf_header
    ld hl,@op
    ld bc,3
    rst.lil $18
    ld a,(@op)
    cp VBUF_ADJ_NEG+1
    ret c
    ld hl,@operand
    ld bc,1
    rst.lil $18
    ret
@op:      db 0
@offset:  dw24 0
@operand: db 0

; Command 6: conditionally call a buffer (basic form - check value is an
; 8-bit byte fetched from another buffer at a non-advanced offset).
; inputs: hl = bufferId to call (65535 = current buffer), a = operation
;         (VBUF_COND_*, no modifier flag bits set), de = checkBufferId,
;         bc = checkOffset (16-bit), ixl = operand (ignored for
;         VBUF_COND_EXISTS/VBUF_COND_NOT_EXISTS)
; destroys: af, bc, de, hl
vbuf_cond_call:
    ld (@op),a
    ; checkBufferId is not the last field in the combined 5-byte send
    ; below, so it must be written byte-by-byte (a 24-bit `ld (nn),de`
    ; store would spill its upper byte into checkOffset's first byte).
    ld a,e
    ld (@checkBufferId),a
    ld a,d
    ld (@checkBufferId+1),a
    ld (@checkOffset),bc
    ld a,ixl
    ld (@operand),a
    ld a,VBUF_CMD_COND_CALL
    call vbuf_header
    ld hl,@op
    ld bc,5
    rst.lil $18
    ld a,(@op)
    cp VBUF_COND_NOT_EXISTS+1
    ret c
    ld hl,@operand
    ld bc,1
    rst.lil $18
    ret
@op:            db 0
@checkBufferId: dw 0
@checkOffset:   dw24 0
@operand:       db 0

; Command 8: conditionally jump to a buffer (same argument shape as command
; 6 - see vbuf_cond_call above).
; inputs: as vbuf_cond_call
; destroys: af, bc, de, hl
vbuf_cond_jump:
    ld (@op),a
    ; checkBufferId is not the last field in the combined 5-byte send
    ; below, so it must be written byte-by-byte (see vbuf_cond_call above).
    ld a,e
    ld (@checkBufferId),a
    ld a,d
    ld (@checkBufferId+1),a
    ld (@checkOffset),bc
    ld a,ixl
    ld (@operand),a
    ld a,VBUF_CMD_COND_JUMP
    call vbuf_header
    ld hl,@op
    ld bc,5
    rst.lil $18
    ld a,(@op)
    cp VBUF_COND_NOT_EXISTS+1
    ret c
    ld hl,@operand
    ld bc,1
    rst.lil $18
    ret
@op:            db 0
@checkBufferId: dw 0
@checkOffset:   dw24 0
@operand:       db 0


; ===========================================================================
; Commands 10/12 - Conditional jump/call to an offset (NEEDS INSPECTION -
; combines the command-9-style 24-bit offset with the command-6-style
; condition tail; given the register pressure of combining both, the
; condition tail here is always a caller-built blob, same idea as vbuf_raw)
; ===========================================================================

; Command 10: conditionally jump to an offset in a buffer.
; inputs: hl = bufferId (65535 = current buffer), de = 24-bit offset (bit 23
;         clear; see vbuf_jump_offset for the block-index caveat), ix =
;         pointer to a pre-built condition tail (operation byte, with any
;         modifier flag bits set, followed by checkBufferId;checkOffset;
;         [operand...], or vduVariableId; if VBUF_COND_F_VARIABLE is set -
;         see Buffered-Commands-API.md#command-6 for the exact shape per
;         flag combination), bc = length of that tail
; destroys: af, bc, de, hl
vbuf_cond_jump_offset:
    ld (@offset),de
    ld (@tail_len),bc
    push ix
    pop de
    ld (@tail_ptr),de
    ld a,VBUF_CMD_COND_JUMP_OFFSET
    call vbuf_header
    ld hl,@offset
    ld bc,3
    rst.lil $18
    ld hl,(@tail_ptr)
    ld bc,(@tail_len)
    rst.lil $18
    ret
@offset:   dw24 0
@tail_len: dw24 0
@tail_ptr: dw24 0

; Command 12: conditionally call an offset in a buffer (same argument shape
; as command 10 - see vbuf_cond_jump_offset above).
; inputs: as vbuf_cond_jump_offset
; destroys: af, bc, de, hl
vbuf_cond_call_offset:
    ld (@offset),de
    ld (@tail_len),bc
    push ix
    pop de
    ld (@tail_ptr),de
    ld a,VBUF_CMD_COND_CALL_OFFSET
    call vbuf_header
    ld hl,@offset
    ld bc,3
    rst.lil $18
    ld hl,(@tail_ptr)
    ld bc,(@tail_len)
    rst.lil $18
    ret
@offset:   dw24 0
@tail_len: dw24 0
@tail_ptr: dw24 0


; ===========================================================================
; Command 24 - Reverse data (STRAIGHTFORWARD - fully implemented; the
; optional trailing fields are conditional on the options byte, but that
; condition is resolvable entirely at run time with no caller-supplied tail
; needed)
; ===========================================================================

; Command 24: reverse the order of data within the blocks of a buffer (byte
; order within blocks - see vbuf_reverse_blocks/command 23 to instead
; reverse the order of the blocks themselves).
; inputs: hl = bufferId, a = options (VBUF_REV_* bits combined), de =
;         valueSize (only sent if bits 0+1 of options are both set, i.e.
;         (options & VBUF_REV_SIZE) == VBUF_REV_SIZE), bc = chunkSize (only
;         sent if VBUF_REV_CHUNKED is set)
; destroys: af, bc, de, hl
vbuf_reverse:
    ld (@options),a
    ld (@valueSize),de
    ld (@chunkSize),bc
    ld a,VBUF_CMD_REVERSE
    call vbuf_header
    ld hl,@options
    ld bc,1
    rst.lil $18
    ld a,(@options)
    and VBUF_REV_SIZE
    cp VBUF_REV_SIZE
    jr nz,@no_valuesize
    ld hl,@valueSize
    ld bc,2
    rst.lil $18
@no_valuesize:
    ld a,(@options)
    and VBUF_REV_CHUNKED
    ret z
    ld hl,@chunkSize
    ld bc,2
    rst.lil $18
    ret
@options:   db 0
@valueSize: dw24 0
@chunkSize: dw24 0


; ===========================================================================
; Command 48 - Read a VDP variable into a buffer (STRAIGHTFORWARD for the
; non-advanced-offset form; use vbuf_raw with VBUF_RDVAR_ADVANCED set for
; the advanced-offset form)
; ===========================================================================

; Command 48: read a VDP variable/feature flag into a buffer.
; inputs: hl = bufferId, a = options (VBUF_RDVAR_* bits; leave
;         VBUF_RDVAR_ADVANCED clear - use vbuf_raw for that form), de =
;         offset (16-bit, non-advanced), bc = variableId, ix = default value
;         (only sent if VBUF_RDVAR_DEFAULT is set; only the low byte of ix
;         is sent unless VBUF_RDVAR_16BIT is also set, in which case both
;         bytes are sent)
; destroys: af, bc, de, hl
vbuf_read_variable:
    ld (@options),a
    ; offset is not the last field in the combined 5-byte send below, so it
    ; must be written byte-by-byte (a 24-bit `ld (nn),de` store would spill
    ; its upper byte into variableId's first byte).
    ld a,e
    ld (@offset),a
    ld a,d
    ld (@offset+1),a
    ld (@variableId),bc
    push ix
    pop de
    ld (@default),de
    ld a,VBUF_CMD_READ_VARIABLE
    call vbuf_header
    ld hl,@options
    ld bc,5
    rst.lil $18
    ld a,(@options)
    and VBUF_RDVAR_DEFAULT
    ret z
    ld a,(@options)
    and VBUF_RDVAR_16BIT
    ld bc,1
    jr z,@send_default
    ld bc,2
@send_default:
    ld hl,@default
    rst.lil $18
    ret
@options:    db 0
@offset:     dw 0
@variableId: dw24 0
@default:    dw24 0


; ===========================================================================
; Command 72 - Expand a bitmap (NEEDS INSPECTION - the mapping-data tail's
; shape depends on both the bpp bits and the VBUF_EXPAND_F_MAPBUF bit, so it
; is always a caller-built blob; the certain, fixed-shape prefix is fully
; implemented)
; ===========================================================================

; Command 72: expand a bitmap to 8-bits-per-pixel (RGBA2222), remapping
; source pixel values through a mapping table.
; inputs: hl = bufferId (destination), a = options (bits 0-2 = bits-per-
;         source-pixel, 0 meaning 8; VBUF_EXPAND_F_* flag bits), de =
;         sourceBufferId, bc = width (only sent if VBUF_EXPAND_F_ALIGN is
;         set; ignored otherwise), ix = pointer to the mapping-data tail
;         (either 2^bpp inline mapping byte values, or a single 16-bit
;         mappingDataBufferId if VBUF_EXPAND_F_MAPBUF is set - see
;         Buffered-Commands-API.md#command-72), iy = length of that tail in
;         bytes
; destroys: af, bc, de, hl
vbuf_expand_bitmap:
    ld (@options),a
    ld (@sourceBufferId),de
    ld (@width),bc
    push ix
    pop de
    ld (@tail_ptr),de
    push iy
    pop de
    ld (@tail_len),de
    ld a,VBUF_CMD_EXPAND_BITMAP
    call vbuf_header
    ld hl,@options
    ld bc,3
    rst.lil $18
    ld a,(@options)
    and VBUF_EXPAND_F_ALIGN
    jr z,@no_width
    ld hl,@width
    ld bc,2
    rst.lil $18
@no_width:
    ld hl,(@tail_ptr)
    ld bc,(@tail_len)
    rst.lil $18
    ret
@options:        db 0
@sourceBufferId: dw24 0
@width:          dw24 0
@tail_ptr:       dw24 0
@tail_len:       dw24 0


; ===========================================================================
; Commands 32/33 - 2D/3D affine transform matrix (PUNTED, mostly)
;
; Every operation except "set identity" (0) and "invert" (1) requires
; floating-point or fixed-point argument values (IEEE-754 or shifted-integer
; encoded), and Golem has not yet decided on a numeric/float representation
; or runtime encoding routines (see docs/devlog/2026-07-28.md's "no general
; floating point initially" note) - so only those two argument-free
; operations get a friendly wrapper below. Everything else (rotate, scale,
; translate, shear, skew, combine, bitmap-relative translate) needs
; hand-built format-byte + value bytes and should go through vbuf_raw; see
; Buffered-Commands-API.md#commands-32-and-33 for the operation/format-byte
; tables (reproduced above as the VBUF_MATRIX_*/VBUF_FMT_* constants) before
; attempting one.
;
; This command family is also experimental and gated behind the VDP's
; affine-transforms test flag: `VDU 23, 0, &F8, 1; 1;`. Command 33 (3D) was
; only added in VDP 2.10.0. Verify feature availability before relying on
; either command.
; ===========================================================================

; Command 32: 2D affine transform matrix - identity/invert only (see the
; block comment above for why the other operations are not wrapped here).
; inputs: hl = bufferId, a = VBUF_MATRIX_SET_IDENTITY or VBUF_MATRIX_INVERT
; destroys: af, bc, hl
vbuf_matrix2d_simple:
    ld (@op),a
    ld a,VBUF_CMD_MATRIX2D
    call vbuf_header
    ld hl,@op
    ld bc,1
    rst.lil $18
    ret
@op: db 0

; Command 33: 3D affine transform matrix - identity/invert only (see
; vbuf_matrix2d_simple above).
; inputs: hl = bufferId, a = VBUF_MATRIX_SET_IDENTITY or VBUF_MATRIX_INVERT
; destroys: af, bc, hl
vbuf_matrix3d_simple:
    ld (@op),a
    ld a,VBUF_CMD_MATRIX3D
    call vbuf_header
    ld hl,@op
    ld bc,1
    rst.lil $18
    ret
@op: db 0


; ===========================================================================
; Command 34 - Arbitrary-dimension matrix (TOTALLY PUNTED)
;
; No operation of this command is argument-free: even the "simplest" ones
; (fill, diagonal) need a format byte + a floating-point/fixed-point value,
; and several others need row/column bytes plus one or two source buffer
; IDs in varying combinations (see the VBUF_MX_* constants above and
; Buffered-Commands-API.md#command-34 for the exact per-operation shape).
; Given the same "no float representation decided yet" reason as commands
; 32/33 above, and that this command is experimental/test-flag-gated, no
; friendly wrapper is provided at all - use vbuf_raw with a hand-built tail
; of `operation, rows, columns, <arguments...>`.
; ===========================================================================


; ===========================================================================
; Command 40 - Create a transformed bitmap (NEEDS INSPECTION - experimental/
; test-flag-gated, but mechanically fully implementable; no floating point
; values appear in this command's own arguments, only buffer/bitmap IDs and
; dimensions)
; ===========================================================================

; Command 40: create a transformed bitmap by applying an affine transform
; matrix (from another buffer) to a source bitmap.
; inputs: hl = bufferId (destination), a = options (VBUF_XFORM_BMP_* bits),
;         de = transformBufferId, bc = sourceBitmapId, ix = width, iy =
;         height (width/height only sent if VBUF_XFORM_BMP_EXPLICIT_SIZE is
;         set - see Buffered-Commands-API.md#command-40)
; destroys: af, bc, de, hl
vbuf_transform_bitmap:
    ld (@options),a
    ; transformBufferId is not the last field in the combined 5-byte send
    ; below, so it must be written byte-by-byte (a 24-bit `ld (nn),de`
    ; store would spill its upper byte into sourceBitmapId's first byte).
    ld a,e
    ld (@transformBufferId),a
    ld a,d
    ld (@transformBufferId+1),a
    ld (@sourceBitmapId),bc
    push ix
    pop de
    ; width is not the last field in the combined 4-byte send below
    ; (height follows), so it must also be written byte-by-byte.
    ld a,e
    ld (@width),a
    ld a,d
    ld (@width+1),a
    push iy
    pop de
    ld (@height),de
    ld a,VBUF_CMD_TRANSFORM_BITMAP
    call vbuf_header
    ld hl,@options
    ld bc,5
    rst.lil $18
    ld a,(@options)
    and VBUF_XFORM_BMP_EXPLICIT_SIZE
    ret z
    ld hl,@width
    ld bc,4
    rst.lil $18
    ret
@options:           db 0
@transformBufferId: dw 0
@sourceBitmapId:    dw24 0
@width:             dw 0
@height:            dw24 0


; ===========================================================================
; Command 41 - Apply a transform matrix to data in a buffer (NEEDS
; INSPECTION / mostly PUNTED - the fixed, certain prefix is implemented; the
; four optional trailing fields (size/offset/stride/limit) each independently
; vary in byte width and can *also* be redirected to come from a buffer+
; offset instead of the command stream when VBUF_XFORM_DATA_BUF_ARGS is set,
; per vdu_buffered.h's bufferTransformData - verified too variable to
; usefully fix in registers, so that whole tail is a caller-built blob)
; ===========================================================================

; Command 41: apply a transform matrix (from another buffer) to sets of
; values (e.g. coordinate pairs) held in a buffer, writing the transformed
; result to this buffer.
;
; No bespoke wrapper is provided - use vbuf_raw directly with
; VBUF_CMD_TRANSFORM_DATA. Build the tail
; `options, format, transformBufferId; sourceBufferId; [<optional fields per
; options, see VBUF_XFORM_DATA_* above and
; Buffered-Commands-API.md#command-41>]` into your own scratch buffer, then:
;   ld hl,bufferId \ ld de,tail_ptr \ ld bc,tail_len
;   ld a,VBUF_CMD_TRANSFORM_DATA \ call vbuf_raw
