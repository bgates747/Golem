; Include layout.inc separately. Explicit backend wrappers send six bytes only.
golem_buffered_InitRace:
 ld hl,golem_buffered_InitRace_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_InitRace_call: db 23,0,160,138,0,1
golem_native_InitRace:
 ld hl,golem_native_InitRace_call
 ld bc,6
 rst.lil $18
 ret
golem_native_InitRace_call: db 23,0,160,144,0,1
golem_buffered_Tick120:
 ld hl,golem_buffered_Tick120_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_Tick120_call: db 23,0,160,139,0,1
golem_native_Tick120:
 ld hl,golem_native_Tick120_call
 ld bc,6
 rst.lil $18
 ret
golem_native_Tick120_call: db 23,0,160,145,0,1
golem_buffered_Render30:
 ld hl,golem_buffered_Render30_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_Render30_call: db 23,0,160,140,0,1
golem_native_Render30:
 ld hl,golem_native_Render30_call
 ld bc,6
 rst.lil $18
 ret
golem_native_Render30_call: db 23,0,160,146,0,1
golem_buffered_ResetRace:
 ld hl,golem_buffered_ResetRace_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_ResetRace_call: db 23,0,160,141,0,1
golem_native_ResetRace:
 ld hl,golem_native_ResetRace_call
 ld bc,6
 rst.lil $18
 ret
golem_native_ResetRace_call: db 23,0,160,147,0,1
golem_buffered_StopRace:
 ld hl,golem_buffered_StopRace_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_StopRace_call: db 23,0,160,142,0,1
golem_native_StopRace:
 ld hl,golem_native_StopRace_call
 ld bc,6
 rst.lil $18
 ret
golem_native_StopRace_call: db 23,0,160,148,0,1
