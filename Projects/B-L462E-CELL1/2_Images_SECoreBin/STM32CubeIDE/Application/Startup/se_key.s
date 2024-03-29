	.section .SE_Key_Data,"a",%progbits
	.syntax unified
	.thumb 
	.global SE_ReadKey_1
SE_ReadKey_1:
	PUSH {R1-R5}
	MOVW R1, #0x454f
	MOVT R1, #0x5f4d
	MOVW R2, #0x454b
	MOVT R2, #0x5f59
	MOVW R3, #0x4f43
	MOVT R3, #0x504d
	MOVW R4, #0x4e41
	MOVT R4, #0x3159
	STM R0, {R1-R4}
	POP {R1-R5}
	BX LR

	.global SE_ReadKey_1_Pub
SE_ReadKey_1_Pub:
	PUSH {R1-R5}
	MOVW R1, #0xf2ba
	MOVT R1, #0xf897
	MOVW R2, #0xe33e
	MOVT R2, #0xdc07
	MOVW R3, #0xc316
	MOVT R3, #0x7871
	MOVW R4, #0xf11d
	MOVT R4, #0x3eb0
	STM R0, {R1-R4}
	ADD R0, R0,#16
	MOVW R1, #0x95f0
	MOVT R1, #0x44b0
	MOVW R2, #0x1254
	MOVT R2, #0x4881
	MOVW R3, #0x2cfb
	MOVT R3, #0xb966
	MOVW R4, #0x3d54
	MOVT R4, #0x4aa5
	STM R0, {R1-R4}
	ADD R0, R0,#16
	MOVW R1, #0x26e8
	MOVT R1, #0x7604
	MOVW R2, #0x37b7
	MOVT R2, #0x3c8b
	MOVW R3, #0xd846
	MOVT R3, #0x6afd
	MOVW R4, #0x6163
	MOVT R4, #0x467c
	STM R0, {R1-R4}
	ADD R0, R0,#16
	MOVW R1, #0x7dc3
	MOVT R1, #0x46e9
	MOVW R2, #0x3144
	MOVT R2, #0xd76e
	MOVW R3, #0x6de1
	MOVT R3, #0x70ba
	MOVW R4, #0x44ed
	MOVT R4, #0x2ba
	STM R0, {R1-R4}
	POP {R1-R5}
	BX LR

	.global SE_ReadKey_2_Pub
SE_ReadKey_2_Pub:
	PUSH {R1-R5}
	MOVW R1, #0x5bc7
	MOVT R1, #0x609a
	MOVW R2, #0xfd32
	MOVT R2, #0xb3e4
	MOVW R3, #0x8b8b
	MOVT R3, #0x213a
	MOVW R4, #0xcf52
	MOVT R4, #0x5d4b
	STM R0, {R1-R4}
	ADD R0, R0,#16
	MOVW R1, #0x4d0d
	MOVT R1, #0xfda7
	MOVW R2, #0xabee
	MOVT R2, #0x9774
	MOVW R3, #0x91df
	MOVT R3, #0xaff9
	MOVW R4, #0x7910
	MOVT R4, #0x1529
	STM R0, {R1-R4}
	ADD R0, R0,#16
	MOVW R1, #0x2d11
	MOVT R1, #0x3315
	MOVW R2, #0xd443
	MOVT R2, #0x8966
	MOVW R3, #0xde80
	MOVT R3, #0xcac4
	MOVW R4, #0x9af7
	MOVT R4, #0x2a65
	STM R0, {R1-R4}
	ADD R0, R0,#16
	MOVW R1, #0x7447
	MOVT R1, #0x452c
	MOVW R2, #0x8b09
	MOVT R2, #0xb2e0
	MOVW R3, #0x6e5b
	MOVT R3, #0xb140
	MOVW R4, #0xba14
	MOVT R4, #0x77b5
	STM R0, {R1-R4}
	POP {R1-R5}
	BX LR

    .end
