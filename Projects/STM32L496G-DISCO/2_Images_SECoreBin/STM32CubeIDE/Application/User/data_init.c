/**
  ******************************************************************************
  * @file    data_init.c
  * @author  MCD Application Team
  * @brief   Data section (RW + ZI) initialization.
  *          This file provides set of firmware functions to manage SE low level
  *          interface functionalities.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file in
  * the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stdint-gcc.h"

#ifndef vu32
#	define vu32 volatile uint32_t
#endif

/**
  * @brief  Copy initialized data from ROM to RAM.
  * @param  None.
  * @retval None.
  */

void LoopCopyDataInit(void)
{
	extern uint8_t _sidata asm("_sidata");
	extern uint8_t _sdata asm("_sdata");
	extern uint8_t _edata asm("_edata");

	vu32* src = (vu32*) &_sidata;
	vu32* dst = (vu32*) &_sdata;

	vu32 len = (&_edata - &_sdata) / 4;

	for(vu32 i=0; i < len; i++)
		dst[i] = src[i];
}

/**
  * @brief  Clear the zero-initialized data section.
  * @param  None.
  * @retval None.
  */
void LoopFillZerobss(void)
{
	extern uint8_t _sbss asm("_sbss");
	extern uint8_t _ebss asm("_ebss");

	vu32* dst = (vu32*) &_sbss;
	vu32 len = (&_ebss - &_sbss) / 4;

	for(vu32 i=0; i < len; i++)
		dst[i] = 0;
}

/**
  * @brief  Data section initialization.
  * @param  None.
  * @retval None.
  */
void __gcc_data_init(void) {
	LoopFillZerobss();
	LoopCopyDataInit();
}
