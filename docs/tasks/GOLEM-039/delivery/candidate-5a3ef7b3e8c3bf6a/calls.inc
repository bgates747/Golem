; Include layout.inc separately. Explicit backend wrappers send six bytes only.
golem_native_Sine:
 ld hl,golem_native_Sine_call
 ld bc,6
 rst.lil $18
 ret
golem_native_Sine_call: db 23,0,160,10,0,1
golem_native_RouteTick:
 ld hl,golem_native_RouteTick_call
 ld bc,6
 rst.lil $18
 ret
golem_native_RouteTick_call: db 23,0,160,11,0,1
golem_native_Contact:
 ld hl,golem_native_Contact_call
 ld bc,6
 rst.lil $18
 ret
golem_native_Contact_call: db 23,0,160,12,0,1
golem_native_Tick120:
 ld hl,golem_native_Tick120_call
 ld bc,6
 rst.lil $18
 ret
golem_native_Tick120_call: db 23,0,160,13,0,1
golem_native_Normalize:
 ld hl,golem_native_Normalize_call
 ld bc,6
 rst.lil $18
 ret
golem_native_Normalize_call: db 23,0,160,14,0,1
golem_native_MultiplySigned:
 ld hl,golem_native_MultiplySigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_MultiplySigned_call: db 23,0,160,15,0,1
golem_native_MultiplyUnsigned:
 ld hl,golem_native_MultiplyUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_MultiplyUnsigned_call: db 23,0,160,16,0,1
golem_native_DivideSigned:
 ld hl,golem_native_DivideSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_DivideSigned_call: db 23,0,160,17,0,1
golem_native_DivideUnsigned:
 ld hl,golem_native_DivideUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_DivideUnsigned_call: db 23,0,160,18,0,1
golem_native_BoundedPair:
 ld hl,golem_native_BoundedPair_call
 ld bc,6
 rst.lil $18
 ret
golem_native_BoundedPair_call: db 23,0,160,19,0,1
golem_native_AliasPair:
 ld hl,golem_native_AliasPair_call
 ld bc,6
 rst.lil $18
 ret
golem_native_AliasPair_call: db 23,0,160,20,0,1
golem_native_ShiftSigned:
 ld hl,golem_native_ShiftSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_ShiftSigned_call: db 23,0,160,21,0,1
golem_native_ShiftUnsigned:
 ld hl,golem_native_ShiftUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_ShiftUnsigned_call: db 23,0,160,22,0,1
golem_native_InitRace:
 ld hl,golem_native_InitRace_call
 ld bc,6
 rst.lil $18
 ret
golem_native_InitRace_call: db 23,0,160,23,0,1
golem_native_ResetRace:
 ld hl,golem_native_ResetRace_call
 ld bc,6
 rst.lil $18
 ret
golem_native_ResetRace_call: db 23,0,160,24,0,1
golem_native_Render30:
 ld hl,golem_native_Render30_call
 ld bc,6
 rst.lil $18
 ret
golem_native_Render30_call: db 23,0,160,25,0,1
golem_native_StopRace:
 ld hl,golem_native_StopRace_call
 ld bc,6
 rst.lil $18
 ret
golem_native_StopRace_call: db 23,0,160,26,0,1
