;*******************************************************************************
;* File Name          : se_key.s
;* Author             : MCD Application Team
;* Description        : ReadKey from PCROP area.
;*******************************************************************************
;*
;* Copyright (c) 2017 STMicroelectronics.
;* All rights reserved.
;*
;* This software is licensed under terms that can be found in the LICENSE file in
;* the root directory of this software component.
;* If no LICENSE file comes with this software, it is provided AS-IS.
;*
;*******************************************************************************
;
;
; Cortex-M version
;
	section .SE_Key_Data:CODE
	EXPORT SE_ReadKey
SE_ReadKey
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

	EXPORT SE_ReadKey_Pub
SE_ReadKey_Pub
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
	END
