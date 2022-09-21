/**
  ******************************************************************************
  * @file    at_custom_modem_socket.h
  * @author  MCD Application Team
  * @brief   Header for at_custom_modem_socket.c module for BG96
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
#ifndef AT_CUSTOM_SOCKET_BG96_H
#define AT_CUSTOM_SOCKET_BG96_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "at_core.h"
#include "at_parser.h"
#include "at_modem_api.h"
#include "at_modem_common.h"
#include "at_custom_modem_specific.h"

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)

/** @addtogroup AT_CUSTOM AT_CUSTOM
  * @{
  */

/** @addtogroup AT_CUSTOM_QUECTEL_BG96 AT_CUSTOM QUECTEL_BG96
  * @{
  */

/** @addtogroup AT_CUSTOM_QUECTEL_BG96_SOCKET AT_CUSTOM QUECTEL_BG96 SOCKET
  * @{
  */

/** @defgroup AT_CUSTOM_QUECTEL_BG96_SOCKET_Exported_Functions AT_CUSTOM QUECTEL_BG96 SOCKET Exported Functions
  * @{
  */

/* BG96 build commands overriding common function  or specific */
at_status_t fCmdBuild_QIACT_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_QIOPEN_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_QICLOSE_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_QISEND_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_QISEND_WRITE_DATA_BG96(atparser_context_t *p_atp_ctxt,
                                             atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_QIRD_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_QISTATE_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_QIDNSCFG_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_QIDNSGIP_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_QPING_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_QICSGP_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);

/* BG96 specific analyze commands */
at_action_rsp_t fRspAnalyze_QIURC_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                       const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_QIACT_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                       const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_QIOPEN_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                        const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_QIRD_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                      const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_QIRD_data_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                           const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_QISTATE_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                         const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_QPING_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                       const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);

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

#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */

#ifdef __cplusplus
}
#endif

#endif /* AT_CUSTOM_SOCKET_BG96_H */
