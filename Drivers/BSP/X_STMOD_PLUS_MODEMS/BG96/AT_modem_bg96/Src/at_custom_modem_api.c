/**
  ******************************************************************************
  * @file    at_custom_modem_api.c
  * @author  MCD Application Team
  * @brief   This file provides all the specific code to the
  *          BG96 Quectel modem: LTE-cat-M1 or LTE-cat.NB1(=NB-IOT) or GSM
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

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "at_custom_modem_api.h"
#include "at_custom_modem_specific.h"
#include "at_custom_sysctrl.h"
#include "plf_config.h"

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

/**
  * @brief  Initialize AT custom function pointers.
  * @param  funcPtrs Pointer to the structure of AT functions.
  * @retval none
  */
void atcma_init_at_func_ptrs(atcustom_funcPtrs_t *funcPtrs)
{
#if defined(USE_MODEM_BG96)
  /* initialize the structure of function pointers with TYPE1SC functions.
   * This structure will be used by common code to call specific modem functions.
   */
  funcPtrs->f_init = ATCustom_BG96_init;
  funcPtrs->f_checkEndOfMsgCallback = ATCustom_BG96_checkEndOfMsgCallback;
  funcPtrs->f_getCmd = ATCustom_BG96_getCmd;
  funcPtrs->f_extractElement = ATCustom_BG96_extractElement;
  funcPtrs->f_analyzeCmd = ATCustom_BG96_analyzeCmd;
  funcPtrs->f_analyzeParam = ATCustom_BG96_analyzeParam;
  funcPtrs->f_terminateCmd = ATCustom_BG96_terminateCmd;
  funcPtrs->f_get_rsp = ATCustom_BG96_get_rsp;
  funcPtrs->f_get_urc = ATCustom_BG96_get_urc;
  funcPtrs->f_get_error = ATCustom_BG96_get_error;
  funcPtrs->f_hw_event = ATCustom_BG96_hw_event;
#else
#error AT custom does not match with selected modem
#endif /* USE_MODEM_BG96 */
}

/**
  * @brief  Initialize SYSCTRL custom function pointers.
  * @param  funcPtrs Pointer to the structure of SYSCTRL functions.
  * @retval none
  */
void atcma_init_sysctrl_func_ptrs(sysctrl_funcPtrs_t *funcPtrs)
{
#if defined(USE_MODEM_BG96)
  /* initialize the structure of function pointers with TYPE1SC functions.
   * This structure will be used by common code to call specific modem functions.
   */
  funcPtrs->f_getDeviceDescriptor = SysCtrl_BG96_getDeviceDescriptor;
  funcPtrs->f_open_channel =  SysCtrl_BG96_open_channel;
  funcPtrs->f_close_channel =  SysCtrl_BG96_close_channel;
  funcPtrs->f_power_on =  SysCtrl_BG96_power_on;
  funcPtrs->f_power_off = SysCtrl_BG96_power_off;
  funcPtrs->f_reset_device = SysCtrl_BG96_reset;
  funcPtrs->f_sim_select = SysCtrl_BG96_sim_select;
#else
#error SysCtrl does not match with selected modem
#endif /* USE_MODEM_BG96 */
}

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
