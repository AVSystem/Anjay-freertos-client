/**
  ******************************************************************************
  * @file    at_modem_api.h
  * @author  MCD Application Team
  * @brief   Header for at_modem_api.c module
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2018-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef AT_MODEM_API_H
#define AT_MODEM_API_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "at_core.h"
#include "at_parser.h"
#include "at_sysctrl.h"
#include "ipc_common.h"

/** @addtogroup AT_CORE AT_CORE
  * @{
  */

/** @addtogroup AT_CORE_API AT_CORE API
  * @{
  */

/** @defgroup AT_CORE_API_Exported_Defines AT_CORE API Exported Defines
  * @{
  */
#define CMD_AT_INVALID ((uint32_t) 0xFFFFFFFFU)
/**
  * @}
  */

/** @defgroup AT_CORE_API_Exported_Types AT_CORE API Exported Types
  * @{
  */
typedef enum
{
  INTERMEDIATE_CMD = 0,
  FINAL_CMD        = 1,
} atcustom_FinalCmd_t;

typedef void (*ATC_initTypeDef)(atparser_context_t *p_atp_ctxt);
typedef uint8_t (*ATC_checkEndOfMsgCallbackTypeDef)(uint8_t rxChar);
typedef at_status_t (*ATC_getCmdTypeDef)(at_context_t *p_at_ctxt,
                                         uint32_t *p_ATcmdTimeout);
typedef at_endmsg_t (*ATC_extractElementTypeDef)(atparser_context_t *p_atp_ctxt,
                                                 const IPC_RxMessage_t *p_msg_in,
                                                 at_element_info_t *const element_infos);
typedef at_action_rsp_t (*ATC_analyzeCmdTypeDef)(at_context_t *p_at_ctxt,
                                                 const IPC_RxMessage_t *p_msg_in,
                                                 at_element_info_t *element_infos);
typedef at_action_rsp_t (*ATC_analyzeParamTypeDef)(at_context_t *p_at_ctxt,
                                                   const IPC_RxMessage_t *p_msg_in,
                                                   at_element_info_t *element_infos);
typedef at_action_rsp_t (*ATC_terminateCmdTypedef)(atparser_context_t *p_atp_ctxt, at_element_info_t *element_infos);
typedef at_status_t (*ATC_get_rsp)(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf);
typedef at_status_t (*ATC_get_urc)(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf);
typedef at_status_t (*ATC_get_error)(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf);
typedef at_status_t (*ATC_hw_event)(sysctrl_device_type_t deviceType, at_hw_event_t hwEvent, GPIO_PinState gstate);

typedef struct
{
  uint8_t                            initialized;
  ATC_initTypeDef                    f_init;
  ATC_checkEndOfMsgCallbackTypeDef   f_checkEndOfMsgCallback;
  ATC_getCmdTypeDef                  f_getCmd;
  ATC_extractElementTypeDef          f_extractElement;
  ATC_analyzeCmdTypeDef              f_analyzeCmd;
  ATC_analyzeParamTypeDef            f_analyzeParam;
  ATC_terminateCmdTypedef            f_terminateCmd;
  ATC_get_rsp                        f_get_rsp;
  ATC_get_urc                        f_get_urc;
  ATC_get_error                      f_get_error;
  ATC_hw_event                       f_hw_event;

} atcustom_funcPtrs_t;
/**
  * @}
  */

/** @defgroup AT_CORE_API_Exported_Types AT_CORE API Exported Functions
  * @{
  */
at_status_t atcc_initParsers(sysctrl_device_type_t device_type);
void atcc_init(at_context_t *p_at_ctxt);
ATC_checkEndOfMsgCallbackTypeDef atcc_checkEndOfMsgCallback(const at_context_t *p_at_ctxt);
at_status_t atcc_getCmd(at_context_t *p_at_ctxt, uint32_t *p_ATcmdTimeout);
at_endmsg_t atcc_extractElement(at_context_t *p_at_ctxt,
                                const IPC_RxMessage_t *p_msg_in,
                                at_element_info_t *element_infos);
at_action_rsp_t atcc_analyzeCmd(at_context_t *p_at_ctxt,
                                const IPC_RxMessage_t *p_msg_in,
                                at_element_info_t *element_infos);
at_action_rsp_t atcc_analyzeParam(at_context_t *p_at_ctxt,
                                  const IPC_RxMessage_t *p_msg_in,
                                  at_element_info_t *element_infos);
at_action_rsp_t atcc_terminateCmd(at_context_t *p_at_ctxt, at_element_info_t *element_infos);
at_status_t atcc_get_rsp(at_context_t *p_at_ctxt, at_buf_t *p_rsp_buf);
at_status_t atcc_get_urc(at_context_t *p_at_ctxt, at_buf_t *p_rsp_buf);
at_status_t atcc_get_error(at_context_t *p_at_ctxt, at_buf_t *p_rsp_buf);
void atcc_hw_event(sysctrl_device_type_t deviceType, at_hw_event_t hwEvent, GPIO_PinState gstate);

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

#endif /* AT_MODEM_API_H */
