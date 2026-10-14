; Include layout.inc separately. Explicit backend wrappers send six bytes only.
golem_buffered_step:
 ld hl,golem_buffered_step_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_step_call: db 23,0,160,53,1,1
golem_native_step:
 ld hl,golem_native_step_call
 ld bc,6
 rst.lil $18
 ret
golem_native_step_call: db 23,0,160,55,1,1
