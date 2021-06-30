/**
  ******************************************************************************
  * @file    at_custom_modem_signalling.h
  * @author  MCD Application Team
  * @brief   Header for at_custom_modem_signalling.c module for
  *          MURATA-TYPE1SC-EVK module
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef AT_CUSTOM_SIGNALLING_TYPE1SC_H
#define AT_CUSTOM_SIGNALLING_TYPE1SC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "at_core.h"
#include "at_parser.h"
#include "at_modem_api.h"
#include "at_modem_common.h"
#include "at_custom_modem_specific.h"

/* Exported constants --------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* device specific parameters */

/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
/* TYPE1SC build commands overriding common function  or specific */
at_status_t fCmdBuild_ATD_TYPE1SC(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_PDNSET_TYPE1SC(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_SETCFG_TYPE1SC(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_GETCFG_TYPE1SC(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_SETSYSCFG_TYPE1SC(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_GETSYSCFG_TYPE1SC(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_SETBDELAY_TYPE1SC(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_PDNRDP_TYPE1SC(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_NOTIFYEV_TYPE1SC(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);

/* TYPE1SC specific analyze commands */
at_action_rsp_t fRspAnalyze_Error_TYPE1SC(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                          const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_CPIN_TYPE1SC(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                         const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_CFUN_TYPE1SC(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                         const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_CCID_TYPE1SC(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                         const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_GETCFG_TYPE1SC(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                           const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_GETSYSCFG_TYPE1SC(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                              const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_PDNRDP_TYPE1SC(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                           const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_NOTIFYEV_TYPE1SC(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                             const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);

#ifdef __cplusplus
}
#endif

#endif /* AT_CUSTOM_MODEM_TYPE1SC_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
