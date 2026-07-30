; ============================================================================
; VENDORED REFERENCE EXAMPLE - NOT YET IMPLEMENTED OR ADAPTED FOR GOLEM.
;
; This is a read-only copy of `parse_args.inc` from
; /home/smith/Agon/mystuff/agon-utils/examples/flower/src/parse_args.inc,
; kept here purely as a worked example of a reusable argv/argc tokenizer
; plus verb/keyword dispatch helpers for an Agon MOS command-line program
; (see this directory's README.md). Note this particular file is actually
; INCLUDEd by a *sibling* example, `flower_demo.asm` (a regular ORG
; 0x040000 program), not by the real `flower.asm` MOSlet vendored
; alongside it here (which has its own near-duplicate inline copy of the
; tokenizer). It is NOT built, included, or referenced by any Golem
; toolchain code, and NOT a Golem program. Do not edit in place - if
; Golem ever implements its own MOSlet-based loader (see
; docs/design/language-type-proposals.md, "Axis 17"), port and adapt the
; relevant logic there instead.
; ============================================================================
;
; Parse the parameter string into a C array
; Parameters
; - HL: Address of parameter string
; - IX: Address for array pointer storage
; Returns:
; -  C: Number of parameters parsed
;
_parse_params:		LD	BC, _exec_name
			LD	(IX+0), BC		; ARGV[0] = the executable name
			LEA     IX, IX+3
			CALL	_skip_spaces		; Skip HL past any leading spaces
;
			LD	BC, 1			; C: ARGC = 1 - also clears out top 16 bits of BCU
			LD	B, argv_ptrs_max - 1	; B: Maximum number of argv_ptrs
;
_parse_params_1:	
			PUSH	BC			; Stack ARGC	
			PUSH	HL			; Stack start address of token
			CALL	_get_token		; Get the next token
			LD	A, C			; A: Length of the token in characters
			POP	DE			; Start address of token (was in HL)
			POP	BC			; ARGC
			OR	A			; Check for A=0 (no token found) OR at end of string
			RET	Z
;
			LD	(IX+0), DE		; Store the pointer to the token
			PUSH	HL			; DE=HL
			POP	DE
			CALL	_skip_spaces		; And skip HL past any spaces onto the next character
			XOR	A
			LD	(DE), A			; Zero-terminate the token
			LEA  	IX, IX+3			; Advance to next pointer position
			INC	C			; Increment ARGC
			LD	A, C			; Check for C >= A
			CP	B
			JR	C, _parse_params_1	; And loop
			RET

; Get the next token
; Parameters:
; - HL: Address of parameter string
; Returns:
; - HL: Address of first character after token
; -  C: Length of token (in characters)
;
_get_token:		LD	C, 0			; Initialise length
@@:			LD	A, (HL)			; Get the character from the parameter string
			OR	A			; Exit if 0 (end of parameter string in MOS)
			RET 	Z
			CP	13			; Exit if CR (end of parameter string in BBC BASIC)
			RET	Z
			CP	' '			; Exit if space (end of token)
			RET	Z
			INC	HL			; Advance to next character
			INC 	C			; Increment length
			JR	@B
	
; Skip spaces in the parameter string
; Parameters:
; - HL: Address of parameter string
; Returns:
; - HL: Address of next none-space character
;    F: Z if at end of string, otherwise NZ if there are more tokens to be parsed
;
_skip_spaces:		LD	A, (HL)			; Get the character from the parameter string	
			CP	' '			; Exit if not space
			RET	NZ
			INC	HL			; Advance to next character
			JR	_skip_spaces		; Increment length

; Storage for the argv array pointers
min_args: equ 1
argv_ptrs_max:		EQU	16			; Maximum number of arguments allowed in argv
argv_ptrs:		    BLKP	argv_ptrs_max, 0		
_sps:			DS	3			; Storage for the stack pointer (used by BASIC)

; begin BASIC-specific end code
; This bit of code is called from STAR_BYE and returns us safely to MOS
_end:			LD		SP, (_sps)		; Restore the stack pointer 
; fall through to _main_end_ok
; end BASIC-specific end code

; ========== HELPER FUNCTIONS ==========
;
; get the next argument after ix as a floating point number
; inputs: ix = pointer to the argument string
; outputs: HLH'L'C = floating point number, ix points to the next argument
; destroys: everything except iy, including prime registers
get_arg_float:
    lea ix,ix+3 ; point to the next argument
    push ix ; preserve
    ld ix,(ix)  ; point to argument string
    call val_fp ; convert the string to a float
    pop ix ; restore
    ret ; return with the value in HLH'L'C

; get the next argument after ix as a floating point number and store it in buffer pointed to by iy
; inputs: ix = pointer to the argument string
; outputs: HLH'L'C = floating point number, ix points to the next argument
; destroys: everything except iy, including prime registers
store_arg_iy_float:
    lea ix,ix+3 ; point to the next argument
    push ix ; preserve
    ld ix,(ix)  ; point to argument string
    call val_fp ; convert the string to a float
    call store_float_iy_nor ; save the float in buffer
    pop ix ; restore
    ret ; return with the value in HLH'L'C
;
; get the next argument after ix as a string
; inputs: ix = pointer to the argument string
; outputs: HL = pointer to the argument string, ix points to the next argument
; destroys: a, h, l, f
get_arg_text:
    lea ix,ix+3 ; point to the next argument
    ld hl,(ix)  ; get the argument string
    ret
;
; match the next argument after ix to the dispatch table at iy
;   - arguments and dispatch entries are zero-terminated, case-sensitive strings
;   - final entry of dispatch table must be a 3-byte zero or bad things will happen
; returns: NO MATCH: iy=dispatch list terminator a=1 and zero flag reset
;          ON MATCH: iy=dispatch address, a=0 and zero flag set
; destroys: a, hl, de, ix, iy, flags
match_next:
    lea ix,ix+3         ; point to the next argument
@loop:
    ld hl,(iy)          ; pointer argument dispatch record
    sign_hlu            ; check for list terminator
    jp z,@no_match      ; if a=0, return error
    inc hl              ; skip over jp instruction
    inc hl
    ld de,(ix)          ; pointer to the argument string
    call str_equal      ; compare the argument to the dispatch table entry
    jp z,@match         ; if equal, return success
    lea iy,iy+3         ; if not equal, bump iy to next dispatch table entry
    jp @loop            ; and loop 
@no_match:
    inc a               ; no match so return a=1 and zero flag reset
    ret
@match:
    ld iy,(iy)          ; get the function pointer
    ret                 ; return a=0 and zero flag set

; same as match_next, but prints the parameter if a match is found
match_next_and_print:
    call match_next
    ret nz ; no match found
    lea ix,ix-3 
    call get_arg_text ; hl points to the operator string
    call print_param
    ret

; compare two zero-terminated strings for equality, case-sensitive
; hl: pointer to first string, de: pointer to second string
; returns: z if equal, nz if not equal
; destroys: a, hl, de
str_equal:
    ld a,(de)           ; get the first character
    cp (hl)             ; compare to the second character
    ret nz              ; if not equal, return
    or a
    ret z               ; if equal and zero, return
    inc hl              ; next character
    inc de
    jp str_equal        ; loop until end of string

; print the parameter string pointed to by ix
; destroys: a, hl
print_param:
    ld hl,(ix)          ; get the parameter pointer
    call printString    ; print the parameter string
    ld a,' '            ; print a space separator
    rst.lil $10         
    ret

; print the parameters
; inputs: b = number of parameters, ix = pointer to the parameters
; destroys: a, hl, bc
print_params:
    ld b,c              ; loop counter = number of parameters
    push ix             ; save the pointer to the parameters
@loop:
    push bc             ; save the loop counter
    call print_param    ; print the parameter
    lea ix,ix+3         ; next parameter pointer
    pop bc              ; get back the loop counter
    djnz @loop          ; loop until done
    pop ix              ; restore the pointer to the parameters
    ret
