/**
  ******************************************************************************
  * @file    at_custom_socket.c
  * @author  MCD Application Team
  * @brief   This file provides common socket code for the modems
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


/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "at_core.h"
#include "at_modem_common.h"
#include "at_modem_signalling.h"
#include "at_modem_socket.h"
#include "at_datapack.h"
#include "at_util.h"
#include "cellular_runtime_standard.h"
#include "cellular_runtime_custom.h"
#include "at_sysctrl.h"
#include "plf_config.h"

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)

/** @addtogroup AT_CORE AT_CORE
  * @{
  */

/** @addtogroup AT_CORE_SOCKET AT_CORE SOCKET
  * @{
  */

/** @defgroup AT_CORE_SOCKET_Private_Macros AT_CORE SOCKET Private Macros
  * @{
  */
#if (USE_TRACE_ATCUSTOM_MODEM == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_INFO(format, args...) TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P0, "ATCModem:" format "\n\r", ## args)
#define PRINT_DBG(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P1, "ATCModem:" format "\n\r", ## args)
#define PRINT_API(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P2, "ATCModem API:" format "\n\r", ## args)
#define PRINT_ERR(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_ERR, "ATCModem ERROR:" format "\n\r", ## args)
#else
#define PRINT_INFO(format, args...)  (void) printf("ATCModem:" format "\n\r", ## args);
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_API(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(format, args...)   (void) printf("ATCModem ERROR:" format "\n\r", ## args);
#endif /* USE_PRINTF */
#else
#define PRINT_INFO(...)   __NOP(); /* Nothing to do */
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_API(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(...)   __NOP(); /* Nothing to do */
#endif /* USE_TRACE_ATCUSTOM_MODEM */
/**
  * @}
  */

/** @defgroup AT_CORE_SOCKET_Exported_Functions AT_CORE SOCKET Exported Functions
  * @{
  */

/**
  * @brief  This function reserve a modem connection Id (ID shared between at-custom and the modem)
  *         to a socket handle (ID shared between upper layers and at-custom).
  * @note   This function is used for modems using CID chosen by Host.
  * @param  p_modem_ctxt Pointer to modem context.
  * @param  sockHandle Socket handle.
  * @retval at_status_t
  */
at_status_t atcm_socket_reserve_modem_cid(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle)
{
  at_status_t retval;

  PRINT_DBG("atcm_socket_reserve_modem_cid for socket handle %ld", sockHandle)

  if (sockHandle == CS_INVALID_SOCKET_HANDLE)
  {
    PRINT_INFO("socket handle %ld not valid", sockHandle)
    retval = ATSTATUS_ERROR;
  }
  else
  {
    /* reset socket parameters */
    p_modem_ctxt->persist.socket[sockHandle].socket_connected = AT_FALSE;
    p_modem_ctxt->persist.socket[sockHandle].socket_data_pending_urc = AT_FALSE;
    p_modem_ctxt->persist.socket[sockHandle].socket_data_available = AT_FALSE;
    p_modem_ctxt->persist.socket[sockHandle].socket_closed_pending_urc = AT_FALSE;
    retval = ATSTATUS_OK;
  }

  return (retval);
}

/**
  * @brief  This function release a modem connection Id (ID shared between at-custom and the modem)
  *         to a socket handle (ID shared between upper layers and at-custom).
  * @note   This function is used for modems using CID chosen by Host.
  * @param  p_modem_ctxt Pointer to modem context.
  * @param  sockHandle Socket handle.
  * @retval at_status_t
  */
at_status_t atcm_socket_release_modem_cid(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle)
{
  at_status_t retval;

  PRINT_DBG("atcm_socket_release_modem_cid for socket handle %ld", sockHandle)

  if (sockHandle == CS_INVALID_SOCKET_HANDLE)
  {
    PRINT_INFO("socket handle %ld not valid", sockHandle)
    retval = ATSTATUS_ERROR;
  }
  else
  {
    if ((p_modem_ctxt->persist.socket[sockHandle].socket_data_pending_urc == AT_TRUE) ||
        (p_modem_ctxt->persist.socket[sockHandle].socket_closed_pending_urc == AT_TRUE))
    {
      /* Trace only */
      PRINT_INFO("Warning, there was pending URC for socket handle %ld: (%d)data pending urc,(%d) closed by remote urc",
                 sockHandle,
                 p_modem_ctxt->persist.socket[sockHandle].socket_data_pending_urc,
                 p_modem_ctxt->persist.socket[sockHandle].socket_closed_pending_urc)
    }

    /* reset socket parameters */
    p_modem_ctxt->persist.socket[sockHandle].socket_connected = AT_FALSE;
    p_modem_ctxt->persist.socket[sockHandle].socket_data_pending_urc = AT_FALSE;
    p_modem_ctxt->persist.socket[sockHandle].socket_data_available = AT_FALSE;
    p_modem_ctxt->persist.socket[sockHandle].socket_closed_pending_urc = AT_FALSE;
    retval = ATSTATUS_OK;
  }

  return (retval);
}

/**
  * @brief  This function assign a modem connection Id (ID shared between at-custom and the modem)
  *         to a socket handle (ID shared between upper layers and at-custom)
  * @note   This function is used for modems using CID chosen by themselves.
  * @param  p_modem_ctxt Pointer to modem context.
  * @param  sockHandle Socket handle.
  * @param  modemcid Modem CID value.
  * @retval at_status_t
  */
at_status_t atcm_socket_assign_modem_cid(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle,
                                         uint32_t modemcid)
{
  at_status_t retval;

  PRINT_DBG("atcm_socket_assign_modem_cid for socket handle %ld", sockHandle)

  if (sockHandle == CS_INVALID_SOCKET_HANDLE)
  {
    PRINT_INFO("socket handle %ld not valid", sockHandle)
    retval = ATSTATUS_ERROR;
  }
  else
  {
    p_modem_ctxt->persist.socket[sockHandle].socket_connId_value = (uint8_t) modemcid;

    /* reset socket parameters */
    p_modem_ctxt->persist.socket[sockHandle].socket_connected = AT_FALSE;
    p_modem_ctxt->persist.socket[sockHandle].socket_data_pending_urc = AT_FALSE;
    p_modem_ctxt->persist.socket[sockHandle].socket_data_available = AT_FALSE;
    p_modem_ctxt->persist.socket[sockHandle].socket_closed_pending_urc = AT_FALSE;
    retval = ATSTATUS_OK;
  }

  return (retval);
}

/**
  * @brief  This function unassign a modem connection Id (ID shared between at-custom and the modem)
  *         to a socket handle (ID shared between upper layers and at-custom)
  * @note   This function is used for modems using CID chosen by themselves.
  * @param  p_modem_ctxt Pointer to modem context.
  * @param  sockHandle Socket handle.
  * @retval at_status_t
  */
at_status_t atcm_socket_unassign_modem_cid(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle)
{
  at_status_t retval;

  PRINT_DBG("atcm_socket_unassign_modem_cid for socket handle %ld", sockHandle)

  if (sockHandle == CS_INVALID_SOCKET_HANDLE)
  {
    PRINT_INFO("socket handle %ld not valid", sockHandle)
    retval = ATSTATUS_ERROR;
  }
  else
  {
    if ((p_modem_ctxt->persist.socket[sockHandle].socket_data_pending_urc == AT_TRUE) ||
        (p_modem_ctxt->persist.socket[sockHandle].socket_closed_pending_urc == AT_TRUE))
    {
      /* Trace only */
      PRINT_INFO("Warning, there was pending URC for socket handle %ld: (%d)data pending urc,(%d) closed by remote urc",
                 sockHandle,
                 p_modem_ctxt->persist.socket[sockHandle].socket_data_pending_urc,
                 p_modem_ctxt->persist.socket[sockHandle].socket_closed_pending_urc)
    }

    p_modem_ctxt->persist.socket[sockHandle].socket_connId_value = UNDEFINED_MODEM_SOCKET_ID;

    /* reset socket parameters */
    p_modem_ctxt->persist.socket[sockHandle].socket_connected = AT_FALSE;
    p_modem_ctxt->persist.socket[sockHandle].socket_data_pending_urc = AT_FALSE;
    p_modem_ctxt->persist.socket[sockHandle].socket_data_available = AT_FALSE;
    p_modem_ctxt->persist.socket[sockHandle].socket_closed_pending_urc = AT_FALSE;
    retval = ATSTATUS_OK;
  }

  return (retval);
}

/**
  * @brief  This function returns the modem connection Id (ID shared between at-custom and the modem)
  *         corresponding to a socket handle (ID shared between upper layers and at-custom)
  * @param  p_modem_ctxt Pointer to modem context.
  * @param  sockHandle Socket handle.
  * @retval uint32_t CID value.
  */
uint32_t atcm_socket_get_modem_cid(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle)
{
  uint32_t cid;

  if (sockHandle == CS_INVALID_SOCKET_HANDLE)
  {
    PRINT_INFO("socket handle %ld not valid", sockHandle)
    cid = 0U;
  }
  else
  {
    /* find  connectid corresponding to this socket_handle */
    cid = (uint32_t)(p_modem_ctxt->persist.socket[sockHandle].socket_connId_value);
  }

  return (cid);
}

/**
  * @brief  This function returns the socket handle (ID shared between upper layers and at-custom)
  *         corresponding to modem connection Id (ID shared between at-custom and the modem)
  * @param  p_modem_ctxt Pointer to modem context.
  * @param  modemcid Modem CID value.
  * @retval socket_handle_t Affected socket handle.
  */
socket_handle_t atcm_socket_get_socket_handle(const atcustom_modem_context_t *p_modem_ctxt, uint32_t modemCID)
{
  socket_handle_t sockHandle = CS_INVALID_SOCKET_HANDLE;

  for (uint8_t i = 0U; i < CELLULAR_MAX_SOCKETS; i++)
  {
    const atcustom_persistent_SOCKET_context_t *p_tmp;
    p_tmp = &p_modem_ctxt->persist.socket[i];
    if (p_tmp->socket_connId_value == modemCID)
    {
      sockHandle = (socket_handle_t)i;
    }
  }

  if (sockHandle == CS_INVALID_SOCKET_HANDLE)
  {
    /* Trace only */
    PRINT_INFO("Can not find valid socket handle for modem CID=%ld", modemCID)
  }

  return (sockHandle);
}

/**
  * @brief  This function set the "socket data received" URC for a
  *         socket handle (ID shared between upper layers and at-custom)
  * @param  p_modem_ctxt Pointer to modem context.
  * @param  sockHandle Socket handle.
  * @retval at_status_t
  */
at_status_t atcm_socket_set_urc_data_pending(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle)
{
  at_status_t retval = ATSTATUS_OK;

  PRINT_API("enter atcm_socket_set_urc_data_pending sockHandle=%ld", sockHandle)

  if (sockHandle != CS_INVALID_SOCKET_HANDLE)
  {
    /* update flags */
    p_modem_ctxt->persist.socket[sockHandle].socket_data_pending_urc = AT_TRUE;
    p_modem_ctxt->persist.socket[sockHandle].socket_data_available = AT_TRUE;
    p_modem_ctxt->persist.urc_avail_socket_data_pending = AT_TRUE;
  }
  else
  {
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  This function set the "socket closed by remote" URC for a
  *         socket handle (ID shared between upper layers and at-custom)
  * @param  p_modem_ctxt Pointer to modem context.
  * @param  sockHandle Socket handle.
  * @retval at_status_t
  */
at_status_t atcm_socket_set_urc_closed_by_remote(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle)
{
  at_status_t retval = ATSTATUS_OK;

  PRINT_API("enter atcm_socket_set_urc_closed_by_remote sockHandle=%ld", sockHandle)

  if (sockHandle != CS_INVALID_SOCKET_HANDLE)
  {
    p_modem_ctxt->persist.socket[sockHandle].socket_closed_pending_urc = AT_TRUE;
    p_modem_ctxt->persist.urc_avail_socket_closed_by_remote = AT_TRUE;
  }
  else
  {
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  This function returns the socket handle of "socket data received" URC
  *         and clears it
  * @param  p_modem_ctxt Pointer to modem context.
  * @retval socket_handle_t Socket handle
  */
socket_handle_t atcm_socket_get_hdle_urc_data_pending(atcustom_modem_context_t *p_modem_ctxt)
{
  socket_handle_t sockHandle = CS_INVALID_SOCKET_HANDLE;

  PRINT_API("enter atcm_socket_get_hdle_urc_data_pending")

  for (uint8_t i = 0U; i < CELLULAR_MAX_SOCKETS; i++)
  {
    atcustom_persistent_SOCKET_context_t *p_tmp;
    p_tmp = &p_modem_ctxt->persist.socket[i];
    if (p_tmp->socket_data_pending_urc == AT_TRUE)
    {
      sockHandle = (socket_handle_t)i;
      /* clear this URC */
      p_tmp->socket_data_pending_urc = AT_FALSE;
      break;
    }
  }

  return (sockHandle);
}

/**
  * @brief  This function returns the socket handle of "socket closed by remote" URC
  *         and clears it
  * @param  p_modem_ctxt Pointer to modem context.
  * @retval socket_handle_t Socket handle
  */
socket_handle_t atcm_socket_get_hdle_urc_closed_by_remote(atcustom_modem_context_t *p_modem_ctxt)
{
  socket_handle_t sockHandle = CS_INVALID_SOCKET_HANDLE;

  PRINT_API("enter atcm_socket_get_hdle_urc_closed_by_remote")

  for (uint8_t i = 0U; i < CELLULAR_MAX_SOCKETS; i++)
  {
    atcustom_persistent_SOCKET_context_t *p_tmp;
    p_tmp = &p_modem_ctxt->persist.socket[i];
    if (p_tmp->socket_closed_pending_urc == AT_TRUE)
    {
      sockHandle = (socket_handle_t)i;
      /* clear this URC */
      p_tmp->socket_closed_pending_urc = AT_FALSE;
      break;
    }
  }

  return (sockHandle);
}

/**
  * @brief  This function returns if there are pending "socket data received" URC
  * @param  p_modem_ctxt Pointer to modem context.
  * @retval at_bool_t Returns true if pending "socket data received" URC.
  */
at_bool_t atcm_socket_remaining_urc_data_pending(const atcustom_modem_context_t *p_modem_ctxt)
{
  PRINT_API("enter atcm_socket_remaining_urc_data_pending")
  at_bool_t remain = AT_FALSE;

  for (uint8_t i = 0U; i < CELLULAR_MAX_SOCKETS; i++)
  {
    const atcustom_persistent_SOCKET_context_t *p_tmp;
    p_tmp = &p_modem_ctxt->persist.socket[i];
    if (p_tmp->socket_data_pending_urc == AT_TRUE)
    {
      /* at least one remaining URC */
      remain = AT_TRUE;
      break;
    }
  }

  return (remain);
}

/**
  * @brief  This function returns if there are pending "socket closed by remote" URC
  * @param  p_modem_ctxt Pointer to modem context.
  * @retval at_bool_t Returns true if pending "socket closed by remote" URC.
  */
at_bool_t atcm_socket_remaining_urc_closed_by_remote(const atcustom_modem_context_t *p_modem_ctxt)
{
  PRINT_API("enter atcm_socket_remaining_urc_closed_by_remote")

  at_bool_t remain = AT_FALSE;

  for (uint8_t i = 0U; i < CELLULAR_MAX_SOCKETS; i++)
  {
    const atcustom_persistent_SOCKET_context_t *p_tmp;
    p_tmp = &p_modem_ctxt->persist.socket[i];
    if (p_tmp->socket_closed_pending_urc == AT_TRUE)
    {
      /* at least one remaining URC */
      remain = AT_TRUE;
      break;
    }
  }

  return (remain);
}

/**
  * @brief  This function returns if data are available for given socket handle.
  * @param  p_modem_ctxt Pointer to modem context.
  * @retval at_bool_t Returns true if socket is connected.
  */
at_bool_t atcm_socket_request_available_data(atcustom_modem_context_t *p_modem_ctxt)
{
  PRINT_API("enter atcm_socket_request_available_data")

  /* Note: to avoid optimization and recover old behavior, modify this function to always
   *       return AT_TRUE.
   */
  at_bool_t data_available = AT_FALSE;

  /* get the socket handle for the current data request */
  socket_handle_t sockHandle = p_modem_ctxt->socket_ctxt.socketReceivedata.socket_handle;
  if (sockHandle != CS_INVALID_SOCKET_HANDLE)
  {
    atcustom_persistent_SOCKET_context_t *p_tmp;
    p_tmp = &p_modem_ctxt->persist.socket[sockHandle];
    if (p_tmp->socket_data_available == AT_TRUE)
    {
      data_available = AT_TRUE;
    }
  }

  return (data_available);
}

/**
  * @brief  This function clear the flag data available for given socket handle.
  * @param  p_modem_ctxt Pointer to modem context.
  * @retval none.
  */
void atcm_socket_clear_available_data_flag(atcustom_modem_context_t *p_modem_ctxt)
{
  PRINT_API("enter atcm_socket_clear_available_data_flag")

  /* get the socket handle for the current data request */
  socket_handle_t sockHandle = p_modem_ctxt->socket_ctxt.socketReceivedata.socket_handle;
  if (sockHandle != CS_INVALID_SOCKET_HANDLE)
  {
    /* clear the flag */
    p_modem_ctxt->persist.socket[sockHandle].socket_data_available = AT_FALSE;
  }
}

/**
  * @brief  This function returns if socket is connected for given socket handle.
  * @param  p_modem_ctxt Pointer to modem context.
  * @param  sockHandle Socket handle.
  * @retval at_bool_t Returns true if socket is connected.
  */
at_bool_t atcm_socket_is_connected(const atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle)
{
  at_bool_t retval = AT_FALSE;

  PRINT_API("enter atcm_socket_is_connected sockHandle=%ld", sockHandle)

  if (sockHandle != CS_INVALID_SOCKET_HANDLE)
  {
    if (p_modem_ctxt->persist.socket[sockHandle].socket_connected == AT_TRUE)
    {
      /* socket is currently connected */
      retval = AT_TRUE;
    }
  }

  return (retval);
}

/**
  * @brief  The socket to connected for given socket handle.
  * @param  p_modem_ctxt Pointer to modem context.
  * @param  sockHandle Socket handle.
  * @retval at_status_t
  */
at_status_t atcm_socket_set_connected(atcustom_modem_context_t *p_modem_ctxt, socket_handle_t sockHandle)
{
  at_status_t retval = ATSTATUS_OK;

  PRINT_API("enter atcm_socket_set_connected sockHandle=%ld", sockHandle)

  if (sockHandle != CS_INVALID_SOCKET_HANDLE)
  {
    p_modem_ctxt->persist.socket[sockHandle].socket_connected = AT_TRUE;
  }

  return (retval);
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

#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */
