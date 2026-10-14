; Include layout.inc separately. Explicit backend wrappers send six bytes only.
golem_buffered_divideUnsigned:
 ld hl,golem_buffered_divideUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_divideUnsigned_call: db 23,0,160,36,2,1
golem_native_divideUnsigned:
 ld hl,golem_native_divideUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_divideUnsigned_call: db 23,0,160,48,2,1
golem_buffered_divideSigned:
 ld hl,golem_buffered_divideSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_divideSigned_call: db 23,0,160,37,2,1
golem_native_divideSigned:
 ld hl,golem_native_divideSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_divideSigned_call: db 23,0,160,49,2,1
golem_buffered_aliasUnsigned:
 ld hl,golem_buffered_aliasUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_aliasUnsigned_call: db 23,0,160,38,2,1
golem_native_aliasUnsigned:
 ld hl,golem_native_aliasUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_aliasUnsigned_call: db 23,0,160,50,2,1
golem_buffered_aliasSigned:
 ld hl,golem_buffered_aliasSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_aliasSigned_call: db 23,0,160,39,2,1
golem_native_aliasSigned:
 ld hl,golem_native_aliasSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_aliasSigned_call: db 23,0,160,51,2,1
golem_buffered_boundedPair:
 ld hl,golem_buffered_boundedPair_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_boundedPair_call: db 23,0,160,40,2,1
golem_native_boundedPair:
 ld hl,golem_native_boundedPair_call
 ld bc,6
 rst.lil $18
 ret
golem_native_boundedPair_call: db 23,0,160,52,2,1
golem_buffered_priorEffect:
 ld hl,golem_buffered_priorEffect_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_priorEffect_call: db 23,0,160,41,2,1
golem_native_priorEffect:
 ld hl,golem_native_priorEffect_call
 ld bc,6
 rst.lil $18
 ret
golem_native_priorEffect_call: db 23,0,160,53,2,1
golem_buffered_argumentOrder:
 ld hl,golem_buffered_argumentOrder_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_argumentOrder_call: db 23,0,160,42,2,1
golem_native_argumentOrder:
 ld hl,golem_native_argumentOrder_call
 ld bc,6
 rst.lil $18
 ret
golem_native_argumentOrder_call: db 23,0,160,54,2,1
golem_buffered_unsignedArray:
 ld hl,golem_buffered_unsignedArray_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_unsignedArray_call: db 23,0,160,43,2,1
golem_native_unsignedArray:
 ld hl,golem_native_unsignedArray_call
 ld bc,6
 rst.lil $18
 ret
golem_native_unsignedArray_call: db 23,0,160,55,2,1
golem_buffered_signedArray:
 ld hl,golem_buffered_signedArray_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_signedArray_call: db 23,0,160,44,2,1
golem_native_signedArray:
 ld hl,golem_native_signedArray_call
 ld bc,6
 rst.lil $18
 ret
golem_native_signedArray_call: db 23,0,160,56,2,1
golem_buffered_skipped:
 ld hl,golem_buffered_skipped_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_skipped_call: db 23,0,160,45,2,1
golem_native_skipped:
 ld hl,golem_native_skipped_call
 ld bc,6
 rst.lil $18
 ret
golem_native_skipped_call: db 23,0,160,57,2,1
golem_buffered_repeated:
 ld hl,golem_buffered_repeated_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_repeated_call: db 23,0,160,46,2,1
golem_native_repeated:
 ld hl,golem_native_repeated_call
 ld bc,6
 rst.lil $18
 ret
golem_native_repeated_call: db 23,0,160,58,2,1
