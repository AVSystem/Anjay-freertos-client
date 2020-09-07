/**
  ******************************************************************************
  * @file    at_modem_api.c
  * @author  MCD Application Team
  * @brief   This file provides api code for devices
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "at_modem_api.h"
#include "at_datapack.h"
#include "at_util.h"
#include "plf_config.h"
#include "cellular_runtime_custom.h"
#include "at_custom_modem_api.h"

/* Private typedef -----------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/
#if (USE_TRACE_ATCUSTOM_COMMON == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_INFO(format, args...) TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P0, "ATCustom:" format "\n\r", ## args)
#define PRINT_DBG(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P1, "ATCustom:" format "\n\r", ## args)
#define PRINT_ERR(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_ERR, "ATCustom ERROR:" format "\n\r", ## args)
#else
#include <stdio.h>
#define PRINT_INFO(format, args...)  (void) printf("ATCustom:" format "\n\r", ## args);
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(format, args...)   (void) printf("ATCustom ERROR:" format "\n\r", ## args);
#endif /* USE_PRINTF */
#else
#define PRINT_INFO(...)   __NOP(); /* Nothing to do */
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(...)   __NOP(); /* Nothing to do */
#endif /* USE_TRACE_ATCUSTOM_COMMON */

/* Private variables ---------------------------------------------------------*/
/* AT custom functions ptrs table */
static atcustom_funcPtrs_t at_custom_func[DEVTYPE_INVALID] = { 0 };

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/
/**
  * @brief  Initialize modem parsers.
  * @param  deviceType Device ID on which the event has been received.
  * @retval at_status_t
  */
at_status_t atcc_initParsers(sysctrl_device_type_t device_type)
{
  at_status_t retval;

  /* check if device is already initialized */
  if (at_custom_func[device_type].initialized == 0U)
  {
    /* Init  AT functions pointers */
    atcma_init_at_func_ptrs(&at_custom_func[device_type]);
    /* device is initialized now */
    at_custom_func[device_type].initialized = 1U;
    retval = ATSTATUS_OK;
  }
  else
  {
    PRINT_ERR("Device type %d AT functions already initialized", device_type);
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Call modem initialization function.
  * @param  p_at_ctxt Pointer to the modem context.
  * @retval none
  */
void atcc_init(at_context_t *p_at_ctxt)
{
  (* at_custom_func[p_at_ctxt->device_type].f_init)(&p_at_ctxt->parser);
}

/**
  * @brief  Callback modem function to check end of message.
  * @note  This function is called by the IPC to determine if a complete message has been received.
  * @param  p_at_ctxt Pointer to the modem context.
  * @retval none
  */
ATC_checkEndOfMsgCallbackTypeDef atcc_checkEndOfMsgCallback(const at_context_t *p_at_ctxt)
{
  /* called under interruption, do not put trace here */
  return (at_custom_func[p_at_ctxt->device_type].f_checkEndOfMsgCallback);
}

/**
  * @brief  Call modem function to retrieve next AT command to send for the requested service.
  * @note   This functions can be called many times for a service if required.
  * @param  p_at_ctxt Pointer to the modem context.
  * @param  p_ATcmdTimeout Timeout value to apply for this command.
  * @retval at_status_t
  */
at_status_t atcc_getCmd(at_context_t *p_at_ctxt, uint32_t *p_ATcmdTimeout)
{
  at_status_t retval;

  retval = (*at_custom_func[p_at_ctxt->device_type].f_getCmd)(p_at_ctxt, p_ATcmdTimeout);

  PRINT_DBG("atcc_getCmd returned status = %d", retval)
  return (retval);
}

/**
  * @brief  Call modem function to extract a element from a received message.
  * @param  p_at_ctxt Pointer to the modem context.
  * @param  p_msg_in Pointer to the received message.
  * @param  element_infos Pointer to the structure with extracted informations.
  * @retval at_endmsg_t Indicates if this is the last message.
  */
at_endmsg_t atcc_extractElement(at_context_t *p_at_ctxt,
                                const IPC_RxMessage_t *p_msg_in,
                                at_element_info_t *element_infos)
{
  at_endmsg_t retval;

  retval = (*at_custom_func[p_at_ctxt->device_type].f_extractElement)(&p_at_ctxt->parser, p_msg_in, element_infos);

  PRINT_DBG("start idx=%d  end idx=%d  size=%d rank=%d",
            element_infos->str_start_idx, element_infos->str_end_idx,
            element_infos->str_size, element_infos->param_rank)
  PRINT_DBG("atcc_extractElement returned endmsg = %d", (retval == ATENDMSG_YES) ? 1 : 0)
  return (retval);
}

/**
  * @brief  Call modem function to analyze command from a received message.
  * @param  p_at_ctxt Pointer to the modem context.
  * @param  p_msg_in Pointer to the received message.
  * @param  element_infos Pointer to the structure with extracted informations.
  * @retval at_action_rsp_t Indicates next action.
  */
at_action_rsp_t atcc_analyzeCmd(at_context_t *p_at_ctxt,
                                const IPC_RxMessage_t *p_msg_in,
                                at_element_info_t *element_infos)
{
  at_action_rsp_t retval;

  retval = (*at_custom_func[p_at_ctxt->device_type].f_analyzeCmd)(p_at_ctxt, p_msg_in, element_infos);

  PRINT_DBG("atcc_analyzeCmd returned action = 0x%x", retval)
  return (retval);
}

/**
  * @brief  Call modem function to analyze command parameters from a received message.
  * @param  p_at_ctxt Pointer to the modem context.
  * @param  p_msg_in Pointer to the received message.
  * @param  element_infos Pointer to the structure with extracted informations.
  * @retval at_action_rsp_t Indicates next action.
  */
at_action_rsp_t atcc_analyzeParam(at_context_t *p_at_ctxt,
                                  const IPC_RxMessage_t *p_msg_in,
                                  at_element_info_t *element_infos)
{
  at_action_rsp_t retval_final;

  retval_final = (*at_custom_func[p_at_ctxt->device_type].f_analyzeParam)(p_at_ctxt, p_msg_in, element_infos);

  PRINT_DBG("atcc_analyzeParam returned action = 0x%x", retval_final)
  return (retval_final);
}

/**
  * @brief  Call modem function to terminate a command.
  * @param  p_at_ctxt Pointer to the modem context.
  * @param  element_infos Pointer to the structure with extracted informations.
  * @retval at_action_rsp_t Indicates next action.
  */
at_action_rsp_t atcc_terminateCmd(at_context_t *p_at_ctxt, at_element_info_t *element_infos)
{
  at_action_rsp_t retval;

  retval = (*at_custom_func[p_at_ctxt->device_type].f_terminateCmd)(&p_at_ctxt->parser, element_infos);

  PRINT_DBG("atcc_terminateCmd returned action = 0x%x", retval)
  return (retval);
}

/**
  * @brief  Call modem function to retrieve modem response to a send command.
  * @param  p_at_ctxt Pointer to the modem context.
  * @param  p_rsp_buf Pointer to the buffer to return the message.
  * @retval at_status_t
  */
at_status_t atcc_get_rsp(at_context_t *p_at_ctxt, at_buf_t *p_rsp_buf)
{
  at_status_t retval;

  retval = (*at_custom_func[p_at_ctxt->device_type].f_get_rsp)(&p_at_ctxt->parser, p_rsp_buf);

  PRINT_DBG("atcc_get_rsp returned status = %d", retval)
  return (retval);
}

/**
  * @brief  Call modem function to retrieve modem URC.
  * @param  p_at_ctxt Pointer to the modem context.
  * @param  p_rsp_buf Pointer to the buffer to return the message.
  * @retval at_status_t
  */
at_status_t atcc_get_urc(at_context_t *p_at_ctxt, at_buf_t *p_rsp_buf)
{
  at_status_t retval;

  retval = (*at_custom_func[p_at_ctxt->device_type].f_get_urc)(&p_at_ctxt->parser, p_rsp_buf);

  PRINT_DBG("atcc_get_urc returned status = %d", retval)
  return (retval);
}

/**
  * @brief  Call modem function to retrieve modem error.
  * @param  p_at_ctxt Pointer to the modem context.
  * @param  p_rsp_buf Pointer to the buffer to return the message.
  * @retval at_status_t
  */
at_status_t atcc_get_error(at_context_t *p_at_ctxt, at_buf_t *p_rsp_buf)
{
  at_status_t retval;

  retval = (*at_custom_func[p_at_ctxt->device_type].f_get_error)(&p_at_ctxt->parser, p_rsp_buf);

  PRINT_DBG("atcc_get_error returned status = %d", retval)
  return (retval);
}

/**
  * @brief  Report an Hw event
  * @note   this function is usually called from GPIO interrupt handler
  * @param  deviceType Device ID..
  * @param  hwEvent Hardware event indication.
  * @param  gstate GPIO ppin state.
  * @retval none
  */
void atcc_hw_event(sysctrl_device_type_t deviceType, at_hw_event_t hwEvent, GPIO_PinState gstate)
{
  /* Do not add traces (called under interrupt if GPIO event)
   * device type = modem
   */
  if (deviceType == DEVTYPE_MODEM_CELLULAR)
  {
    (void)(*at_custom_func[DEVTYPE_MODEM_CELLULAR].f_hw_event)(deviceType, hwEvent, gstate);
  }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
