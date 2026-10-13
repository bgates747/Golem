; Include layout.inc separately. Explicit backend wrappers send six bytes only.
golem_buffered_addUnsigned:
 ld hl,golem_buffered_addUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_addUnsigned_call: db 23,0,160,166,1,1
golem_native_addUnsigned:
 ld hl,golem_native_addUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_addUnsigned_call: db 23,0,160,205,1,1
golem_buffered_subtractUnsigned:
 ld hl,golem_buffered_subtractUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_subtractUnsigned_call: db 23,0,160,167,1,1
golem_native_subtractUnsigned:
 ld hl,golem_native_subtractUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_subtractUnsigned_call: db 23,0,160,206,1,1
golem_buffered_addSigned:
 ld hl,golem_buffered_addSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_addSigned_call: db 23,0,160,168,1,1
golem_native_addSigned:
 ld hl,golem_native_addSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_addSigned_call: db 23,0,160,207,1,1
golem_buffered_subtractSigned:
 ld hl,golem_buffered_subtractSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_subtractSigned_call: db 23,0,160,169,1,1
golem_native_subtractSigned:
 ld hl,golem_native_subtractSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_subtractSigned_call: db 23,0,160,208,1,1
golem_buffered_compareUnsigned:
 ld hl,golem_buffered_compareUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_compareUnsigned_call: db 23,0,160,170,1,1
golem_native_compareUnsigned:
 ld hl,golem_native_compareUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_compareUnsigned_call: db 23,0,160,209,1,1
golem_buffered_compareSigned:
 ld hl,golem_buffered_compareSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_compareSigned_call: db 23,0,160,171,1,1
golem_native_compareSigned:
 ld hl,golem_native_compareSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_compareSigned_call: db 23,0,160,210,1,1
golem_buffered_unsignedToSigned:
 ld hl,golem_buffered_unsignedToSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_unsignedToSigned_call: db 23,0,160,172,1,1
golem_native_unsignedToSigned:
 ld hl,golem_native_unsignedToSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_unsignedToSigned_call: db 23,0,160,211,1,1
golem_buffered_signedToUnsigned:
 ld hl,golem_buffered_signedToUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_signedToUnsigned_call: db 23,0,160,173,1,1
golem_native_signedToUnsigned:
 ld hl,golem_native_signedToUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_signedToUnsigned_call: db 23,0,160,212,1,1
golem_buffered_narrowUnsigned:
 ld hl,golem_buffered_narrowUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_narrowUnsigned_call: db 23,0,160,174,1,1
golem_native_narrowUnsigned:
 ld hl,golem_native_narrowUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_narrowUnsigned_call: db 23,0,160,213,1,1
golem_buffered_narrowSigned:
 ld hl,golem_buffered_narrowSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_narrowSigned_call: db 23,0,160,175,1,1
golem_native_narrowSigned:
 ld hl,golem_native_narrowSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_narrowSigned_call: db 23,0,160,214,1,1
golem_buffered_signedToNarrowUnsigned:
 ld hl,golem_buffered_signedToNarrowUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_signedToNarrowUnsigned_call: db 23,0,160,176,1,1
golem_native_signedToNarrowUnsigned:
 ld hl,golem_native_signedToNarrowUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_signedToNarrowUnsigned_call: db 23,0,160,215,1,1
golem_buffered_unsignedToNarrowSigned:
 ld hl,golem_buffered_unsignedToNarrowSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_unsignedToNarrowSigned_call: db 23,0,160,177,1,1
golem_native_unsignedToNarrowSigned:
 ld hl,golem_native_unsignedToNarrowSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_unsignedToNarrowSigned_call: db 23,0,160,216,1,1
golem_buffered_widenUnsigned:
 ld hl,golem_buffered_widenUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_widenUnsigned_call: db 23,0,160,178,1,1
golem_native_widenUnsigned:
 ld hl,golem_native_widenUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_widenUnsigned_call: db 23,0,160,217,1,1
golem_buffered_widenSigned:
 ld hl,golem_buffered_widenSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_widenSigned_call: db 23,0,160,179,1,1
golem_native_widenSigned:
 ld hl,golem_native_widenSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_widenSigned_call: db 23,0,160,218,1,1
golem_buffered_widenSignedToUnsigned:
 ld hl,golem_buffered_widenSignedToUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_widenSignedToUnsigned_call: db 23,0,160,180,1,1
golem_native_widenSignedToUnsigned:
 ld hl,golem_native_widenSignedToUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_widenSignedToUnsigned_call: db 23,0,160,219,1,1
golem_buffered_readUnsigned:
 ld hl,golem_buffered_readUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_readUnsigned_call: db 23,0,160,181,1,1
golem_native_readUnsigned:
 ld hl,golem_native_readUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_readUnsigned_call: db 23,0,160,220,1,1
golem_buffered_readSigned:
 ld hl,golem_buffered_readSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_readSigned_call: db 23,0,160,182,1,1
golem_native_readSigned:
 ld hl,golem_native_readSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_readSigned_call: db 23,0,160,221,1,1
golem_buffered_storeUnsigned:
 ld hl,golem_buffered_storeUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_storeUnsigned_call: db 23,0,160,183,1,1
golem_native_storeUnsigned:
 ld hl,golem_native_storeUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_storeUnsigned_call: db 23,0,160,222,1,1
golem_buffered_storeSigned:
 ld hl,golem_buffered_storeSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_storeSigned_call: db 23,0,160,184,1,1
golem_native_storeSigned:
 ld hl,golem_native_storeSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_storeSigned_call: db 23,0,160,223,1,1
golem_buffered_boundedUnsigned:
 ld hl,golem_buffered_boundedUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_boundedUnsigned_call: db 23,0,160,185,1,1
golem_native_boundedUnsigned:
 ld hl,golem_native_boundedUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_boundedUnsigned_call: db 23,0,160,224,1,1
golem_buffered_boundedSigned:
 ld hl,golem_buffered_boundedSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_boundedSigned_call: db 23,0,160,186,1,1
golem_native_boundedSigned:
 ld hl,golem_native_boundedSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_boundedSigned_call: db 23,0,160,225,1,1
golem_buffered_indexConversion:
 ld hl,golem_buffered_indexConversion_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_indexConversion_call: db 23,0,160,187,1,1
golem_native_indexConversion:
 ld hl,golem_native_indexConversion_call
 ld bc,6
 rst.lil $18
 ret
golem_native_indexConversion_call: db 23,0,160,226,1,1
golem_buffered_EqualUnsigned:
 ld hl,golem_buffered_EqualUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_EqualUnsigned_call: db 23,0,160,188,1,1
golem_native_EqualUnsigned:
 ld hl,golem_native_EqualUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_EqualUnsigned_call: db 23,0,160,227,1,1
golem_buffered_NotEqualUnsigned:
 ld hl,golem_buffered_NotEqualUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_NotEqualUnsigned_call: db 23,0,160,189,1,1
golem_native_NotEqualUnsigned:
 ld hl,golem_native_NotEqualUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_NotEqualUnsigned_call: db 23,0,160,228,1,1
golem_buffered_LessUnsigned:
 ld hl,golem_buffered_LessUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_LessUnsigned_call: db 23,0,160,190,1,1
golem_native_LessUnsigned:
 ld hl,golem_native_LessUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_LessUnsigned_call: db 23,0,160,229,1,1
golem_buffered_GreaterUnsigned:
 ld hl,golem_buffered_GreaterUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_GreaterUnsigned_call: db 23,0,160,191,1,1
golem_native_GreaterUnsigned:
 ld hl,golem_native_GreaterUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_GreaterUnsigned_call: db 23,0,160,230,1,1
golem_buffered_LessEqualUnsigned:
 ld hl,golem_buffered_LessEqualUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_LessEqualUnsigned_call: db 23,0,160,192,1,1
golem_native_LessEqualUnsigned:
 ld hl,golem_native_LessEqualUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_LessEqualUnsigned_call: db 23,0,160,231,1,1
golem_buffered_GreaterEqualUnsigned:
 ld hl,golem_buffered_GreaterEqualUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_GreaterEqualUnsigned_call: db 23,0,160,193,1,1
golem_native_GreaterEqualUnsigned:
 ld hl,golem_native_GreaterEqualUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_GreaterEqualUnsigned_call: db 23,0,160,232,1,1
golem_buffered_EqualSigned:
 ld hl,golem_buffered_EqualSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_EqualSigned_call: db 23,0,160,194,1,1
golem_native_EqualSigned:
 ld hl,golem_native_EqualSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_EqualSigned_call: db 23,0,160,233,1,1
golem_buffered_NotEqualSigned:
 ld hl,golem_buffered_NotEqualSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_NotEqualSigned_call: db 23,0,160,195,1,1
golem_native_NotEqualSigned:
 ld hl,golem_native_NotEqualSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_NotEqualSigned_call: db 23,0,160,234,1,1
golem_buffered_LessSigned:
 ld hl,golem_buffered_LessSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_LessSigned_call: db 23,0,160,196,1,1
golem_native_LessSigned:
 ld hl,golem_native_LessSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_LessSigned_call: db 23,0,160,235,1,1
golem_buffered_GreaterSigned:
 ld hl,golem_buffered_GreaterSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_GreaterSigned_call: db 23,0,160,197,1,1
golem_native_GreaterSigned:
 ld hl,golem_native_GreaterSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_GreaterSigned_call: db 23,0,160,236,1,1
golem_buffered_LessEqualSigned:
 ld hl,golem_buffered_LessEqualSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_LessEqualSigned_call: db 23,0,160,198,1,1
golem_native_LessEqualSigned:
 ld hl,golem_native_LessEqualSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_LessEqualSigned_call: db 23,0,160,237,1,1
golem_buffered_GreaterEqualSigned:
 ld hl,golem_buffered_GreaterEqualSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_GreaterEqualSigned_call: db 23,0,160,199,1,1
golem_native_GreaterEqualSigned:
 ld hl,golem_native_GreaterEqualSigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_GreaterEqualSigned_call: db 23,0,160,238,1,1
golem_buffered_identityUnsigned:
 ld hl,golem_buffered_identityUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_identityUnsigned_call: db 23,0,160,200,1,1
golem_native_identityUnsigned:
 ld hl,golem_native_identityUnsigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_identityUnsigned_call: db 23,0,160,239,1,1
golem_buffered_identitySigned:
 ld hl,golem_buffered_identitySigned_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_identitySigned_call: db 23,0,160,201,1,1
golem_native_identitySigned:
 ld hl,golem_native_identitySigned_call
 ld bc,6
 rst.lil $18
 ret
golem_native_identitySigned_call: db 23,0,160,240,1,1
golem_buffered_priorEffect:
 ld hl,golem_buffered_priorEffect_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_priorEffect_call: db 23,0,160,202,1,1
golem_native_priorEffect:
 ld hl,golem_native_priorEffect_call
 ld bc,6
 rst.lil $18
 ret
golem_native_priorEffect_call: db 23,0,160,241,1,1
golem_buffered_skippedWideConversion:
 ld hl,golem_buffered_skippedWideConversion_call
 ld bc,6
 rst.lil $18
 ret
golem_buffered_skippedWideConversion_call: db 23,0,160,203,1,1
golem_native_skippedWideConversion:
 ld hl,golem_native_skippedWideConversion_call
 ld bc,6
 rst.lil $18
 ret
golem_native_skippedWideConversion_call: db 23,0,160,242,1,1
