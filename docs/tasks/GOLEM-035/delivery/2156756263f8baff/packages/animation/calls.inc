; Include layout.inc separately. Explicit backend wrappers send six bytes only.
golem_buffered_Init:
 ld hl,golem_buffered_Init_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_Init_call: db 23,0,160,62,1,1
golem_native_Init:
 ld hl,golem_native_Init_call
 ld bc,6
 rst.lil $18
 ret
golem_native_Init_call: db 23,0,160,67,1,1
golem_buffered_step:
 ld hl,golem_buffered_step_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_step_call: db 23,0,160,63,1,1
golem_native_step:
 ld hl,golem_native_step_call
 ld bc,6
 rst.lil $18
 ret
golem_native_step_call: db 23,0,160,68,1,1
golem_buffered_readRoute:
 ld hl,golem_buffered_readRoute_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_readRoute_call: db 23,0,160,64,1,1
golem_native_readRoute:
 ld hl,golem_native_readRoute_call
 ld bc,6
 rst.lil $18
 ret
golem_native_readRoute_call: db 23,0,160,69,1,1
golem_buffered_readSine:
 ld hl,golem_buffered_readSine_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_readSine_call: db 23,0,160,65,1,1
golem_native_readSine:
 ld hl,golem_native_readSine_call
 ld bc,6
 rst.lil $18
 ret
golem_native_readSine_call: db 23,0,160,70,1,1
