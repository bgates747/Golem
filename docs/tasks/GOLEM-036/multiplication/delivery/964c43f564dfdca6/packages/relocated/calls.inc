; Include layout.inc separately. Explicit backend wrappers send six bytes only.
golem_buffered_multiplyUnsigned:
 ld hl,golem_buffered_multiplyUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_multiplyUnsigned_call: db 23,0,160,24,2,1
golem_native_multiplyUnsigned:
 ld hl,golem_native_multiplyUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_multiplyUnsigned_call: db 23,0,160,39,2,1
golem_buffered_multiplySigned:
 ld hl,golem_buffered_multiplySigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_multiplySigned_call: db 23,0,160,25,2,1
golem_native_multiplySigned:
 ld hl,golem_native_multiplySigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_multiplySigned_call: db 23,0,160,40,2,1
golem_buffered_nestedUnsigned:
 ld hl,golem_buffered_nestedUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_nestedUnsigned_call: db 23,0,160,26,2,1
golem_native_nestedUnsigned:
 ld hl,golem_native_nestedUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_nestedUnsigned_call: db 23,0,160,41,2,1
golem_buffered_nestedSigned:
 ld hl,golem_buffered_nestedSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_nestedSigned_call: db 23,0,160,27,2,1
golem_native_nestedSigned:
 ld hl,golem_native_nestedSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_nestedSigned_call: db 23,0,160,42,2,1
golem_buffered_precedence:
 ld hl,golem_buffered_precedence_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_precedence_call: db 23,0,160,28,2,1
golem_native_precedence:
 ld hl,golem_native_precedence_call
 ld bc,6
 rst.lil $18
 ret
golem_native_precedence_call: db 23,0,160,43,2,1
golem_buffered_priorEffect:
 ld hl,golem_buffered_priorEffect_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_priorEffect_call: db 23,0,160,29,2,1
golem_native_priorEffect:
 ld hl,golem_native_priorEffect_call
 ld bc,6
 rst.lil $18
 ret
golem_native_priorEffect_call: db 23,0,160,44,2,1
golem_buffered_storeUnsigned:
 ld hl,golem_buffered_storeUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_storeUnsigned_call: db 23,0,160,30,2,1
golem_native_storeUnsigned:
 ld hl,golem_native_storeUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_storeUnsigned_call: db 23,0,160,45,2,1
golem_buffered_storeSigned:
 ld hl,golem_buffered_storeSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_storeSigned_call: db 23,0,160,31,2,1
golem_native_storeSigned:
 ld hl,golem_native_storeSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_storeSigned_call: db 23,0,160,46,2,1
golem_buffered_skipped:
 ld hl,golem_buffered_skipped_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_skipped_call: db 23,0,160,32,2,1
golem_native_skipped:
 ld hl,golem_native_skipped_call
 ld bc,6
 rst.lil $18
 ret
golem_native_skipped_call: db 23,0,160,47,2,1
golem_buffered_narrowFirst:
 ld hl,golem_buffered_narrowFirst_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_narrowFirst_call: db 23,0,160,33,2,1
golem_native_narrowFirst:
 ld hl,golem_native_narrowFirst_call
 ld bc,6
 rst.lil $18
 ret
golem_native_narrowFirst_call: db 23,0,160,48,2,1
golem_buffered_wideFirst:
 ld hl,golem_buffered_wideFirst_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_wideFirst_call: db 23,0,160,34,2,1
golem_native_wideFirst:
 ld hl,golem_native_wideFirst_call
 ld bc,6
 rst.lil $18
 ret
golem_native_wideFirst_call: db 23,0,160,49,2,1
golem_buffered_boundedProduct:
 ld hl,golem_buffered_boundedProduct_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_boundedProduct_call: db 23,0,160,35,2,1
golem_native_boundedProduct:
 ld hl,golem_native_boundedProduct_call
 ld bc,6
 rst.lil $18
 ret
golem_native_boundedProduct_call: db 23,0,160,50,2,1
golem_buffered_constantSuccess:
 ld hl,golem_buffered_constantSuccess_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_constantSuccess_call: db 23,0,160,36,2,1
golem_native_constantSuccess:
 ld hl,golem_native_constantSuccess_call
 ld bc,6
 rst.lil $18
 ret
golem_native_constantSuccess_call: db 23,0,160,51,2,1
golem_buffered_evaluationOrder:
 ld hl,golem_buffered_evaluationOrder_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_evaluationOrder_call: db 23,0,160,37,2,1
golem_native_evaluationOrder:
 ld hl,golem_native_evaluationOrder_call
 ld bc,6
 rst.lil $18
 ret
golem_native_evaluationOrder_call: db 23,0,160,52,2,1
