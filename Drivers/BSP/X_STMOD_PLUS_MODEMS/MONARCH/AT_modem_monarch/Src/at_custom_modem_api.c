/**
  ******************************************************************************
  * @file    at_custom_modem_api.c
  * @author  MCD Application Team
  * @brief   This file provides all the specific code to the
  *          Sequans MONARCH modem
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
#include "string.h"
#include "at_custom_modem_api.h"
#include "at_custom_modem_specific.h"
#include "at_custom_sysctrl.h"
#include "plf_config.h"

/** @addtogroup AT_CUSTOM AT_CUSTOM
  * @{
  */

/** @addtogroup AT_CUSTOM_SEQUANS_MONARCH AT_CUSTOM SEQUANS_MONARCH
  * @{
  */

/** @addtogroup AT_CUSTOM_SEQUANS_MONARCH_API AT_CUSTOM SEQUANS_MONARCH API
  * @{
  */

/** @defgroup AT_CUSTOM_SEQUANS_MONARCH_API_Exported_Functions AT_CUSTOM SEQUANS_MONARCH API Exported Functions
  * @{
  */
/**
  * @brief  Initialize AT custom function pointers.
  * @param  funcPtrs Pointer to the structure of AT functions.
  * @retval none
  */
void atcma_init_at_func_ptrs(atcustom_funcPtrs_t *funcPtrs)
{
#if defined(USE_MODEM_GM01Q)
  /* initialize the structure of function pointers with TYPE1SC functions.
   * This structure will be used by common code to call specific modem functions.
   */
  funcPtrs->f_init = ATCustom_MONARCH_init;
  funcPtrs->f_checkEndOfMsgCallback = ATCustom_MONARCH_checkEndOfMsgCallback;
  funcPtrs->f_getCmd = ATCustom_MONARCH_getCmd;
  funcPtrs->f_extractElement = ATCustom_MONARCH_extractElement;
  funcPtrs->f_analyzeCmd = ATCustom_MONARCH_analyzeCmd;
  funcPtrs->f_analyzeParam = ATCustom_MONARCH_analyzeParam;
  funcPtrs->f_terminateCmd = ATCustom_MONARCH_terminateCmd;
  funcPtrs->f_get_rsp = ATCustom_MONARCH_get_rsp;
  funcPtrs->f_get_urc = ATCustom_MONARCH_get_urc;
  funcPtrs->f_get_error = ATCustom_MONARCH_get_error;
  funcPtrs->f_hw_event = ATCustom_MONARCH_hw_event;
#else
#error AT custom does not match with selected modem
#endif /* USE_MODEM_GM01Q */
}

/**
  * @brief  Initialize SYSCTRL custom function pointers.
  * @param  funcPtrs Pointer to the structure of SYSCTRL functions.
  * @retval none
  */
void atcma_init_sysctrl_func_ptrs(sysctrl_funcPtrs_t *funcPtrs)
{
#if defined(USE_MODEM_GM01Q)
  /* initialize the structure of function pointers with TYPE1SC functions.
   * This structure will be used by common code to call specific modem functions.
   */
  funcPtrs->f_getDeviceDescriptor = SysCtrl_MONARCH_getDeviceDescriptor;
  funcPtrs->f_open_channel =  SysCtrl_MONARCH_open_channel;
  funcPtrs->f_close_channel =  SysCtrl_MONARCH_close_channel;
  funcPtrs->f_power_on =  SysCtrl_MONARCH_power_on;
  funcPtrs->f_power_off = SysCtrl_MONARCH_power_off;
  funcPtrs->f_reset_device = SysCtrl_MONARCH_reset;
  funcPtrs->f_sim_select = SysCtrl_MONARCH_sim_select;
#else
#error SysCtrl does not match with selected modem
#endif /* USE_MODEM_GM01Q */
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
