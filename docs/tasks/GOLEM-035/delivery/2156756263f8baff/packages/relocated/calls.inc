; Include layout.inc separately. Explicit backend wrappers send six bytes only.
golem_buffered_readSigned:
 ld hl,golem_buffered_readSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_readSigned_call: db 23,0,160,177,1,1
golem_native_readSigned:
 ld hl,golem_native_readSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_readSigned_call: db 23,0,160,188,1,1
golem_buffered_readUnsigned:
 ld hl,golem_buffered_readUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_readUnsigned_call: db 23,0,160,178,1,1
golem_native_readUnsigned:
 ld hl,golem_native_readUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_readUnsigned_call: db 23,0,160,189,1,1
golem_buffered_store:
 ld hl,golem_buffered_store_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_store_call: db 23,0,160,179,1,1
golem_native_store:
 ld hl,golem_native_store_call
 ld bc,6
 rst.lil $18
 ret
golem_native_store_call: db 23,0,160,190,1,1
golem_buffered_increment:
 ld hl,golem_buffered_increment_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_increment_call: db 23,0,160,180,1,1
golem_native_increment:
 ld hl,golem_native_increment_call
 ld bc,6
 rst.lil $18
 ret
golem_native_increment_call: db 23,0,160,191,1,1
golem_buffered_copy:
 ld hl,golem_buffered_copy_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_copy_call: db 23,0,160,181,1,1
golem_native_copy:
 ld hl,golem_native_copy_call
 ld bc,6
 rst.lil $18
 ret
golem_native_copy_call: db 23,0,160,192,1,1
golem_buffered_readSine:
 ld hl,golem_buffered_readSine_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_readSine_call: db 23,0,160,182,1,1
golem_native_readSine:
 ld hl,golem_native_readSine_call
 ld bc,6
 rst.lil $18
 ret
golem_native_readSine_call: db 23,0,160,193,1,1
golem_buffered_skipped:
 ld hl,golem_buffered_skipped_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_skipped_call: db 23,0,160,183,1,1
golem_native_skipped:
 ld hl,golem_native_skipped_call
 ld bc,6
 rst.lil $18
 ret
golem_native_skipped_call: db 23,0,160,194,1,1
golem_buffered_indexFirst:
 ld hl,golem_buffered_indexFirst_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_indexFirst_call: db 23,0,160,184,1,1
golem_native_indexFirst:
 ld hl,golem_native_indexFirst_call
 ld bc,6
 rst.lil $18
 ret
golem_native_indexFirst_call: db 23,0,160,195,1,1
golem_buffered_expressionIndex:
 ld hl,golem_buffered_expressionIndex_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_expressionIndex_call: db 23,0,160,185,1,1
golem_native_expressionIndex:
 ld hl,golem_native_expressionIndex_call
 ld bc,6
 rst.lil $18
 ret
golem_native_expressionIndex_call: db 23,0,160,196,1,1
golem_buffered_accumulate:
 ld hl,golem_buffered_accumulate_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_accumulate_call: db 23,0,160,186,1,1
golem_native_accumulate:
 ld hl,golem_native_accumulate_call
 ld bc,6
 rst.lil $18
 ret
golem_native_accumulate_call: db 23,0,160,197,1,1
