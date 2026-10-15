; Include layout.inc separately. Explicit backend wrappers send six bytes only.
golem_buffered_shiftUnsigned:
 ld hl,golem_buffered_shiftUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_shiftUnsigned_call: db 23,0,160,241,0,1
golem_native_shiftUnsigned:
 ld hl,golem_native_shiftUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_shiftUnsigned_call: db 23,0,160,3,1,1
golem_buffered_shiftSigned:
 ld hl,golem_buffered_shiftSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_shiftSigned_call: db 23,0,160,242,0,1
golem_native_shiftSigned:
 ld hl,golem_native_shiftSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_shiftSigned_call: db 23,0,160,4,1,1
golem_buffered_chain:
 ld hl,golem_buffered_chain_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_chain_call: db 23,0,160,243,0,1
golem_native_chain:
 ld hl,golem_native_chain_call
 ld bc,6
 rst.lil $18
 ret
golem_native_chain_call: db 23,0,160,5,1,1
golem_buffered_precedence:
 ld hl,golem_buffered_precedence_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_precedence_call: db 23,0,160,244,0,1
golem_native_precedence:
 ld hl,golem_native_precedence_call
 ld bc,6
 rst.lil $18
 ret
golem_native_precedence_call: db 23,0,160,6,1,1
golem_buffered_parentheses:
 ld hl,golem_buffered_parentheses_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_parentheses_call: db 23,0,160,245,0,1
golem_native_parentheses:
 ld hl,golem_native_parentheses_call
 ld bc,6
 rst.lil $18
 ret
golem_native_parentheses_call: db 23,0,160,7,1,1
golem_buffered_nested:
 ld hl,golem_buffered_nested_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_nested_call: db 23,0,160,246,0,1
golem_native_nested:
 ld hl,golem_native_nested_call
 ld bc,6
 rst.lil $18
 ret
golem_native_nested_call: db 23,0,160,8,1,1
golem_buffered_countExpression:
 ld hl,golem_buffered_countExpression_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_countExpression_call: db 23,0,160,247,0,1
golem_native_countExpression:
 ld hl,golem_native_countExpression_call
 ld bc,6
 rst.lil $18
 ret
golem_native_countExpression_call: db 23,0,160,9,1,1
golem_buffered_boundedShift:
 ld hl,golem_buffered_boundedShift_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_boundedShift_call: db 23,0,160,248,0,1
golem_native_boundedShift:
 ld hl,golem_native_boundedShift_call
 ld bc,6
 rst.lil $18
 ret
golem_native_boundedShift_call: db 23,0,160,10,1,1
golem_buffered_priorEffect:
 ld hl,golem_buffered_priorEffect_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_priorEffect_call: db 23,0,160,249,0,1
golem_native_priorEffect:
 ld hl,golem_native_priorEffect_call
 ld bc,6
 rst.lil $18
 ret
golem_native_priorEffect_call: db 23,0,160,11,1,1
golem_buffered_argumentOrder:
 ld hl,golem_buffered_argumentOrder_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_argumentOrder_call: db 23,0,160,250,0,1
golem_native_argumentOrder:
 ld hl,golem_native_argumentOrder_call
 ld bc,6
 rst.lil $18
 ret
golem_native_argumentOrder_call: db 23,0,160,12,1,1
golem_buffered_unsignedArray:
 ld hl,golem_buffered_unsignedArray_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_unsignedArray_call: db 23,0,160,251,0,1
golem_native_unsignedArray:
 ld hl,golem_native_unsignedArray_call
 ld bc,6
 rst.lil $18
 ret
golem_native_unsignedArray_call: db 23,0,160,13,1,1
golem_buffered_signedArray:
 ld hl,golem_buffered_signedArray_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_signedArray_call: db 23,0,160,252,0,1
golem_native_signedArray:
 ld hl,golem_native_signedArray_call
 ld bc,6
 rst.lil $18
 ret
golem_native_signedArray_call: db 23,0,160,14,1,1
golem_buffered_countArray:
 ld hl,golem_buffered_countArray_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_countArray_call: db 23,0,160,253,0,1
golem_native_countArray:
 ld hl,golem_native_countArray_call
 ld bc,6
 rst.lil $18
 ret
golem_native_countArray_call: db 23,0,160,15,1,1
golem_buffered_storeUnsigned:
 ld hl,golem_buffered_storeUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_storeUnsigned_call: db 23,0,160,254,0,1
golem_native_storeUnsigned:
 ld hl,golem_native_storeUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_storeUnsigned_call: db 23,0,160,16,1,1
golem_buffered_storeSigned:
 ld hl,golem_buffered_storeSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_storeSigned_call: db 23,0,160,255,0,1
golem_native_storeSigned:
 ld hl,golem_native_storeSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_storeSigned_call: db 23,0,160,17,1,1
golem_buffered_skipped:
 ld hl,golem_buffered_skipped_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_skipped_call: db 23,0,160,0,1,1
golem_native_skipped:
 ld hl,golem_native_skipped_call
 ld bc,6
 rst.lil $18
 ret
golem_native_skipped_call: db 23,0,160,18,1,1
golem_buffered_constants:
 ld hl,golem_buffered_constants_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_constants_call: db 23,0,160,1,1,1
golem_native_constants:
 ld hl,golem_native_constants_call
 ld bc,6
 rst.lil $18
 ret
golem_native_constants_call: db 23,0,160,19,1,1
