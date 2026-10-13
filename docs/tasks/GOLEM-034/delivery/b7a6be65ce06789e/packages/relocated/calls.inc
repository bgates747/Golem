; Include layout.inc separately. Explicit backend wrappers send six bytes only.
golem_buffered_InitRace:
 ld hl,golem_buffered_InitRace_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_InitRace_call: db 23,0,160,136,1,1
golem_native_InitRace:
 ld hl,golem_native_InitRace_call
 ld bc,6
 rst.lil $18
 ret
golem_native_InitRace_call: db 23,0,160,142,1,1
golem_buffered_Tick120:
 ld hl,golem_buffered_Tick120_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_Tick120_call: db 23,0,160,137,1,1
golem_native_Tick120:
 ld hl,golem_native_Tick120_call
 ld bc,6
 rst.lil $18
 ret
golem_native_Tick120_call: db 23,0,160,143,1,1
golem_buffered_Render30:
 ld hl,golem_buffered_Render30_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_Render30_call: db 23,0,160,138,1,1
golem_native_Render30:
 ld hl,golem_native_Render30_call
 ld bc,6
 rst.lil $18
 ret
golem_native_Render30_call: db 23,0,160,144,1,1
golem_buffered_ResetRace:
 ld hl,golem_buffered_ResetRace_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_ResetRace_call: db 23,0,160,139,1,1
golem_native_ResetRace:
 ld hl,golem_native_ResetRace_call
 ld bc,6
 rst.lil $18
 ret
golem_native_ResetRace_call: db 23,0,160,145,1,1
golem_buffered_StopRace:
 ld hl,golem_buffered_StopRace_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_StopRace_call: db 23,0,160,140,1,1
golem_native_StopRace:
 ld hl,golem_native_StopRace_call
 ld bc,6
 rst.lil $18
 ret
golem_native_StopRace_call: db 23,0,160,146,1,1
