/**
  ******************************************************************************
  * @file    at_custom_modem_api.c
  * @author  MCD Application Team
  * @brief   This file provides all the specific code to the
  *          MURATA-TYPE1SC-EVK module (ALT1250 modem: LTE-cat-M1)
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "at_custom_modem_api.h"
#include "at_custom_modem_specific.h"
#include "sysctrl_specific.h"
#include "plf_config.h"

/* Private typedef -----------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/
void atcma_init_at_func_ptrs(atcustom_funcPtrs_t *funcPtrs)
{
#if defined(USE_MODEM_TYPE1SC)
  /* init function pointers with TYPE1SC functions */
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

void atcma_init_sysctrl_func_ptrs(sysctrl_funcPtrs_t *funcPtrs)
{
#if defined(USE_MODEM_TYPE1SC)
  /* init function pointers with TYPE1SC functions */
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
