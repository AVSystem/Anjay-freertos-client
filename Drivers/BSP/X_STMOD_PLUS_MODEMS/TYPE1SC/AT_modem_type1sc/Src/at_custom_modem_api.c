/**
  ******************************************************************************
  * @file    at_custom_modem_api.c
  * @author  MCD Application Team
  * @brief   This file provides all the specific code to the
  *          MURATA-TYPE1SC-EVK module (ALT1250 modem: LTE-cat-M1)
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

/** @addtogroup AT_CUSTOM_ALTAIR_T1SC AT_CUSTOM ALTAIR_T1SC
  * @{
  */

/** @addtogroup AT_CUSTOM_ALTAIR_T1SC_API AT_CUSTOM ALTAIR_T1SC API
  * @{
  */

/** @defgroup AT_CUSTOM_ALTAIR_T1SC_API_Exported_Functions AT_CUSTOM ALTAIR_T1SC API Exported Functions
  * @{
  */

/**
  * @brief  Initialize AT custom function pointers.
  * @param  funcPtrs Pointer to the structure of AT functions.
  * @retval none
  */
void atcma_init_at_func_ptrs(atcustom_funcPtrs_t *funcPtrs)
{
#if defined(USE_MODEM_TYPE1SC)
  /* initialize the structure of function pointers with TYPE1SC functions.
   * This structure will be used by common code to call specific modem functions.
   */
  funcPtrs->f_init = ATCustom_TYPE1SC_init;
  funcPtrs->f_checkEndOfMsgCallback = ATCustom_TYPE1SC_checkEndOfMsgCallback;
  funcPtrs->f_getCmd = ATCustom_TYPE1SC_getCmd;
  funcPtrs->f_extractElement = ATCustom_TYPE1SC_extractElement;
  funcPtrs->f_analyzeCmd = ATCustom_TYPE1SC_analyzeCmd;
  funcPtrs->f_analyzeParam = ATCustom_TYPE1SC_analyzeParam;
  funcPtrs->f_terminateCmd = ATCustom_TYPE1SC_terminateCmd;
  funcPtrs->f_get_rsp = ATCustom_TYPE1SC_get_rsp;
  funcPtrs->f_get_urc = ATCustom_TYPE1SC_get_urc;
  funcPtrs->f_get_error = ATCustom_TYPE1SC_get_error;
  funcPtrs->f_hw_event = ATCustom_TYPE1SC_hw_event;
#else
#error AT custom does not match with selected modem
#endif /* USE_MODEM_TYPE1SC */
}

/**
  * @brief  Initialize SYSCTRL custom function pointers.
  * @param  funcPtrs Pointer to the structure of SYSCTRL functions.
  * @retval none
  */
void atcma_init_sysctrl_func_ptrs(sysctrl_funcPtrs_t *funcPtrs)
{
#if defined(USE_MODEM_TYPE1SC)
  /* initialize the structure of function pointers with TYPE1SC functions.
   * This structure will be used by common code to call specific modem functions.
   */
  funcPtrs->f_getDeviceDescriptor = SysCtrl_TYPE1SC_getDeviceDescriptor;
  funcPtrs->f_open_channel =  SysCtrl_TYPE1SC_open_channel;
  funcPtrs->f_close_channel =  SysCtrl_TYPE1SC_close_channel;
  funcPtrs->f_power_on =  SysCtrl_TYPE1SC_power_on;
  funcPtrs->f_power_off = SysCtrl_TYPE1SC_power_off;
  funcPtrs->f_reset_device = SysCtrl_TYPE1SC_reset;
  funcPtrs->f_sim_select = SysCtrl_TYPE1SC_sim_select;
#else
#error SysCtrl does not match with selected modem
#endif /* USE_MODEM_TYPE1SC */
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
