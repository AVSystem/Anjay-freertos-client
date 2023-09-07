/**
  ******************************************************************************
  * @file    at_modem_socket.h
  * @author  MCD Application Team
  * @brief   Header for at_modem_socket.c module
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
#ifndef AT_MODEM_SOCKET_H
#define AT_MODEM_SOCKET_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "at_core.h"
#include "at_parser.h"
#include "at_modem_common.h"
#include "cellular_service.h"
#include "cellular_service_int.h"
#include "ipc_common.h"
#include "plf_config.h"

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)

/** @addtogroup AT_CORE AT_CORE
  * @{
  */

/** @addtogroup AT_CORE_SOCKET AT_CORE SOCKET
  * @{
  */

/** @defgroup AT_CORE_SOCKET_Exported_Defines AT_CORE SOCKET Exported Defines
  * @{
  */
#define UNDEFINED_MODEM_SOCKET_ID  ((uint8_t) 255U)
/**
  * @}
  */

/** @defgroup AT_CORE_SOCKET_Exported_Functions AT_CORE SOCKET Exported Functions
  * @{
  */
at_status_t     atcm_socket_reserve_modem_cid(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle);
at_status_t     atcm_socket_release_modem_cid(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle);
at_status_t     atcm_socket_assign_modem_cid(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle,
                                             uint32_t modemcid);
at_status_t     atcm_socket_unassign_modem_cid(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle);
uint32_t        atcm_socket_get_modem_cid(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle);
socket_handle_t atcm_socket_get_socket_handle(const atcustom_modem_context_t *p_modem_ctxt, uint32_t modemCID);
at_status_t     atcm_socket_set_urc_data_pending(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle);
at_status_t     atcm_socket_set_urc_closed_by_remote(atcustom_modem_context_t *p_modem_ctxt,
                                                     socket_handle_t sockHandle);
socket_handle_t atcm_socket_get_hdle_urc_data_pending(atcustom_modem_context_t *p_modem_ctxt);
socket_handle_t atcm_socket_get_hdle_urc_closed_by_remote(atcustom_modem_context_t *p_modem_ctxt);
at_bool_t       atcm_socket_remaining_urc_data_pending(const atcustom_modem_context_t *p_modem_ctxt);
at_bool_t       atcm_socket_remaining_urc_closed_by_remote(const atcustom_modem_context_t *p_modem_ctxt);
at_bool_t       atcm_socket_request_available_data(atcustom_modem_context_t *p_modem_ctxt);
void            atcm_socket_clear_available_data_flag(atcustom_modem_context_t *p_modem_ctxt);
at_bool_t       atcm_socket_is_connected(const atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle);
at_status_t     atcm_socket_set_connected(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle);

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

#endif /* AT_MODEM_SOCKET_H */
