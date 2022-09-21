/**
  ******************************************************************************
  * @file    at_custom_modem_socket.h
  * @author  MCD Application Team
  * @brief   Header for at_custom_modem_socket.c module
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
#ifndef AT_CUSTOM_SOCKET_MONARCH_H
#define AT_CUSTOM_SOCKET_MONARCH_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "at_core.h"
#include "at_parser.h"
#include "at_modem_api.h"
#include "at_modem_signalling.h"
#include "at_custom_modem_specific.h"

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)

/** @addtogroup AT_CUSTOM AT_CUSTOM
  * @{
  */

/** @addtogroup AT_CUSTOM_SEQUANS_MONARCH AT_CUSTOM SEQUANS_MONARCH
  * @{
  */

/** @addtogroup AT_CUSTOM_SEQUANS_MONARCH_SOCKET AT_CUSTOM SEQUANS_MONARCH SOCKET
  * @{
  */

/** @defgroup AT_CUSTOM_SEQUANS_MONARCH_SOCKET_Exported_Functions AT_CUSTOM SEQUANS_MONARCH SOCKET Exported Functions
  * @{
  */

/* MONARCH specific build commands */
at_status_t fCmdBuild_SQNSCFG_MONARCH(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_SQNSCFGEXT_MONARCH(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_SQNSD_MONARCH(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_SQNSH_MONARCH(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_SQNSI_MONARCH(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_SQNSRECV_MONARCH(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_SQNSSENDEXT_MONARCH(atparser_context_t *p_atp_ctxt,
                                          atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_SQNSSEND_WRITE_DATA_MONARCH(atparser_context_t *p_atp_ctxt,
                                                  atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_PING_MONARCH(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);

/* MONARCH specific analyze commands */
at_action_rsp_t fRspAnalyze_SQNSRING_MONARCH(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                             const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_SQNSI_MONARCH(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                          const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_SQNSS_MONARCH(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                          const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_SQNSH_MONARCH(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                          const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_SQNSRECV_MONARCH(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                             const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_SQNSRECV_data_MONARCH(at_context_t *p_at_ctxt,
                                                  atcustom_modem_context_t *p_modem_ctxt,
                                                  const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_PING_MONARCH(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                         const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);

/* other functions */
void clear_ping_resp_struct(atcustom_modem_context_t *p_modem_ctxt);

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

#endif /* AT_CUSTOM_SOCKET_MONARCH_H */

