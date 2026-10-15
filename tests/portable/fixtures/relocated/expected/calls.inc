; Include layout.inc separately. Explicit backend wrappers send six bytes only.
golem_buffered_shiftUnsigned:
 ld hl,golem_buffered_shiftUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_shiftUnsigned_call: db 23,0,160,239,1,1
golem_native_shiftUnsigned:
 ld hl,golem_native_shiftUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_shiftUnsigned_call: db 23,0,160,1,2,1
golem_buffered_shiftSigned:
 ld hl,golem_buffered_shiftSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_shiftSigned_call: db 23,0,160,240,1,1
golem_native_shiftSigned:
 ld hl,golem_native_shiftSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_shiftSigned_call: db 23,0,160,2,2,1
golem_buffered_chain:
 ld hl,golem_buffered_chain_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_chain_call: db 23,0,160,241,1,1
golem_native_chain:
 ld hl,golem_native_chain_call
 ld bc,6
 rst.lil $18
 ret
golem_native_chain_call: db 23,0,160,3,2,1
golem_buffered_precedence:
 ld hl,golem_buffered_precedence_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_precedence_call: db 23,0,160,242,1,1
golem_native_precedence:
 ld hl,golem_native_precedence_call
 ld bc,6
 rst.lil $18
 ret
golem_native_precedence_call: db 23,0,160,4,2,1
golem_buffered_parentheses:
 ld hl,golem_buffered_parentheses_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_parentheses_call: db 23,0,160,243,1,1
golem_native_parentheses:
 ld hl,golem_native_parentheses_call
 ld bc,6
 rst.lil $18
 ret
golem_native_parentheses_call: db 23,0,160,5,2,1
golem_buffered_nested:
 ld hl,golem_buffered_nested_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_nested_call: db 23,0,160,244,1,1
golem_native_nested:
 ld hl,golem_native_nested_call
 ld bc,6
 rst.lil $18
 ret
golem_native_nested_call: db 23,0,160,6,2,1
golem_buffered_countExpression:
 ld hl,golem_buffered_countExpression_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_countExpression_call: db 23,0,160,245,1,1
golem_native_countExpression:
 ld hl,golem_native_countExpression_call
 ld bc,6
 rst.lil $18
 ret
golem_native_countExpression_call: db 23,0,160,7,2,1
golem_buffered_boundedShift:
 ld hl,golem_buffered_boundedShift_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_boundedShift_call: db 23,0,160,246,1,1
golem_native_boundedShift:
 ld hl,golem_native_boundedShift_call
 ld bc,6
 rst.lil $18
 ret
golem_native_boundedShift_call: db 23,0,160,8,2,1
golem_buffered_priorEffect:
 ld hl,golem_buffered_priorEffect_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_priorEffect_call: db 23,0,160,247,1,1
golem_native_priorEffect:
 ld hl,golem_native_priorEffect_call
 ld bc,6
 rst.lil $18
 ret
golem_native_priorEffect_call: db 23,0,160,9,2,1
golem_buffered_argumentOrder:
 ld hl,golem_buffered_argumentOrder_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_argumentOrder_call: db 23,0,160,248,1,1
golem_native_argumentOrder:
 ld hl,golem_native_argumentOrder_call
 ld bc,6
 rst.lil $18
 ret
golem_native_argumentOrder_call: db 23,0,160,10,2,1
golem_buffered_unsignedArray:
 ld hl,golem_buffered_unsignedArray_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_unsignedArray_call: db 23,0,160,249,1,1
golem_native_unsignedArray:
 ld hl,golem_native_unsignedArray_call
 ld bc,6
 rst.lil $18
 ret
golem_native_unsignedArray_call: db 23,0,160,11,2,1
golem_buffered_signedArray:
 ld hl,golem_buffered_signedArray_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_signedArray_call: db 23,0,160,250,1,1
golem_native_signedArray:
 ld hl,golem_native_signedArray_call
 ld bc,6
 rst.lil $18
 ret
golem_native_signedArray_call: db 23,0,160,12,2,1
golem_buffered_countArray:
 ld hl,golem_buffered_countArray_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_countArray_call: db 23,0,160,251,1,1
golem_native_countArray:
 ld hl,golem_native_countArray_call
 ld bc,6
 rst.lil $18
 ret
golem_native_countArray_call: db 23,0,160,13,2,1
golem_buffered_storeUnsigned:
 ld hl,golem_buffered_storeUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_storeUnsigned_call: db 23,0,160,252,1,1
golem_native_storeUnsigned:
 ld hl,golem_native_storeUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_storeUnsigned_call: db 23,0,160,14,2,1
golem_buffered_storeSigned:
 ld hl,golem_buffered_storeSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_storeSigned_call: db 23,0,160,253,1,1
golem_native_storeSigned:
 ld hl,golem_native_storeSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_storeSigned_call: db 23,0,160,15,2,1
golem_buffered_skipped:
 ld hl,golem_buffered_skipped_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_skipped_call: db 23,0,160,254,1,1
golem_native_skipped:
 ld hl,golem_native_skipped_call
 ld bc,6
 rst.lil $18
 ret
golem_native_skipped_call: db 23,0,160,16,2,1
golem_buffered_constants:
 ld hl,golem_buffered_constants_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_constants_call: db 23,0,160,255,1,1
golem_native_constants:
 ld hl,golem_native_constants_call
 ld bc,6
 rst.lil $18
 ret
golem_native_constants_call: db 23,0,160,17,2,1
