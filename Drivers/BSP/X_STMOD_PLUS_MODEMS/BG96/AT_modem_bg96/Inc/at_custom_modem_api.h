/**
  ******************************************************************************
  * @file    at_custom_modem_api.h
  * @author  MCD Application Team
  * @brief   Header for at_custom_modem_api.c module for BG96
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2020-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef AT_CUSTOM_MODEM_API_H
#define AT_CUSTOM_MODEM_API_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "at_core.h"
#include "at_modem_api.h"
#include "at_sysctrl.h"

/** @addtogroup AT_CUSTOM AT_CUSTOM
  * @{
  */

/** @addtogroup AT_CUSTOM_QUECTEL_BG96 AT_CUSTOM QUECTEL_BG96
  * @{
  */

/** @addtogroup AT_CUSTOM_QUECTEL_BG96_API AT_CUSTOM QUECTEL_BG96 API
  * @{
  */

/** @defgroup AT_CUSTOM_QUECTEL_BG96_API_Exported_Functions AT_CUSTOM QUECTEL_BG96 API Exported Functions
  * @{
  */

void atcma_init_at_func_ptrs(atcustom_funcPtrs_t *funcPtrs);
void atcma_init_sysctrl_func_ptrs(sysctrl_funcPtrs_t *funcPtrs);

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* AT_CUSTOM_MODEM_API_H */
