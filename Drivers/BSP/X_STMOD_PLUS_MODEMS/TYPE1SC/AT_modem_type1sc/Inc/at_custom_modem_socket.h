/**
  ******************************************************************************
  * @file    at_custom_modem_socket.h
  * @author  MCD Application Team
  * @brief   Header for at_custom_modem_socket.c module for
  *          MURATA-TYPE1SC-EVK module
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

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)

/** @addtogroup AT_CUSTOM AT_CUSTOM
  * @{
  */

/** @addtogroup AT_CUSTOM_ALTAIR_T1SC AT_CUSTOM ALTAIR_T1SC
  * @{
  */

/** @addtogroup AT_CUSTOM_ALTAIR_T1SC_SOCKET AT_CUSTOM ALTAIR_T1SC SOCKET
  * @{
  */

/** @defgroup AT_CUSTOM_ALTAIR_T1SC_SOCKET_Exported_Types AT_CUSTOM ALTAIR_T1SC SOCKET Exported Types
  * @{
  */
/* Socket service type parameter */
typedef uint8_t atsocket_servicetype_t;
#define ATSOCKET_SERVICETYPE_TCP_CLIENT  (atsocket_servicetype_t)(0x0U)
#define ATSOCKET_SERVICETYPE_UDP_CLIENT  (atsocket_servicetype_t)(0x1U)
#define ATSOCKET_SERVICETYPE_TCP_SERVER  (atsocket_servicetype_t)(0x2U)
#define ATSOCKET_SERVICETYPE_UDP_SERVICE (atsocket_servicetype_t)(0x3U)

/**
  * @}
  */

/** @defgroup AT_CUSTOM_ALTAIR_T1SC_SOCKET_Exported_Functions AT_CUSTOM ALTAIR_T1SC SOCKET Exported Functions
  * @{
  */
/* TYPE1SC specific build commands */
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

/* TYPE1SC other exported functions */
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

#endif /* AT_CUSTOM_SOCKET_TYPE1SC_H */
