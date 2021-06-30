/**
  ******************************************************************************
  * @file    at_custom_modem_socket.h
  * @author  MCD Application Team
  * @brief   Header for at_custom_modem_socket.c module for
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
#ifndef AT_CUSTOM_SOCKET_TYPE1SC_H
#define AT_CUSTOM_SOCKET_TYPE1SC_H

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

/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
void clear_ping_resp_struct(atcustom_modem_context_t *p_modem_ctxt);

/* TYPE1SC build commands overriding common function  or specific */
at_status_t fCmdBuild_PDNACT(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_SOCKETCMD_ALLOCATE(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_SOCKETCMD_ACTIVATE(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_SOCKETCMD_INFO(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_SOCKETCMD_DEACTIVATE(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_SOCKETCMD_DELETE(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_SOCKETDATA_SEND(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_SOCKETDATA_RECEIVE(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_DNSRSLV(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_PINGCMD(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);

/* TYPE1SC specific analyze commands */
at_action_rsp_t fRspAnalyze_PDNACT(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                   const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_SOCKETCMD(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                      const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_SOCKETDATA(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                       const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_SOCKETEV(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                     const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_DNSRSLV(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                    const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_PINGCMD(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                    const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
#ifdef __cplusplus
}
#endif

#endif /* AT_CUSTOM_SOCKET_TYPE1SC_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
