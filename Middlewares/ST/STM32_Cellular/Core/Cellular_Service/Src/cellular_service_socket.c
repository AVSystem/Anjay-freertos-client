/**
  ******************************************************************************
  * @file    cellular_service_socket.c
  * @author  MCD Application Team
  * @brief   This file defines functions for Cellular Data Service which is
  *          used when TCP/IP is offloaded in the modem.
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
#include "plf_config.h"
#include "cellular_service_control.h"
#include "cellular_service_socket.h"
#include "cellular_service_int.h"
#include "at_core.h"
#include "at_datapack.h"
#include "at_util.h"
#include "at_sysctrl.h"
#include "cellular_runtime_custom.h"

/** @addtogroup CELLULAR_SERVICE CELLULAR_SERVICE
  * @{
  */

/** @addtogroup CELLULAR_SERVICE_SOCKET_API CELLULAR_SERVICE_SOCKET API
  * @{
  */

/** @defgroup CELLULAR_SERVICE_SOCKET_API_Private_Macros CELLULAR_SERVICE_SOCKET API Private Macros
  * @{
  */

#if (USE_TRACE_CELLULAR_SERVICE == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_INFO(format, args...) TRACE_PRINT(DBG_CHAN_MAIN, DBL_LVL_P0, "CS:" format "\n\r", ## args)
#define PRINT_DBG(format, args...)  TRACE_PRINT(DBG_CHAN_MAIN, DBL_LVL_P1, "CS:" format "\n\r", ## args)
#define PRINT_API(format, args...)  TRACE_PRINT(DBG_CHAN_MAIN, DBL_LVL_P2, "CS API:" format "\n\r", ## args)
#define PRINT_ERR(format, args...)  TRACE_PRINT(DBG_CHAN_MAIN, DBL_LVL_ERR, "CS ERROR:" format "\n\r", ## args)
#else
#include <stdio.h>
#define PRINT_INFO(format, args...)  (void) printf("CS:" format "\n\r", ## args);
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_API(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(format, args...)   (void) printf("CS ERROR:" format "\n\r", ## args);
#endif /* USE_PRINTF */
#else
#define PRINT_INFO(...)   __NOP(); /* Nothing to do */
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_API(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(...)   __NOP(); /* Nothing to do */
#endif /* USE_TRACE_CELLULAR_SERVICE */

/**
  * @}
  */

/** @defgroup CELLULAR_SERVICE_SOCKET_API_Exported_Functions CELLULAR_SERVICE_SOCKET API Exported Functions
  * @{
  */

/* SOCKET API ----------------------------------------------------------------------------------------------- */
/**
  * @brief  Allocate a socket among of the free sockets (maximum 6 sockets)
  * @param  addr_type Specifies a communication domain.
  *         This parameter can be one of the following values:
  *         IPAT_IPV4 for IPV4 (default)
  *         IPAT_IPV6  for IPV6
  * @param  protocol Specified the transport protocol to be used with the socket.
  *         TCP_PROTOCOL
  *         UDP_PROTOCOL
  * @param  cid Specifies the identity of PDN configuration to be used.
  *         CS_PDN_PREDEF_CONFIG To use default PDN configuration.
  *         CS_PDN_USER_CONFIG_1-5 To use a dedicated PDN configuration.
  * @retval Socket handle which references allocated socket
  */
socket_handle_t CDS_socket_create(CS_IPaddrType_t addr_type,
                                  CS_TransportProtocol_t protocol,
                                  CS_PDN_conf_id_t cid)
{
  socket_handle_t sockhandle = csint_socket_allocateHandle();
  if (sockhandle == CS_INVALID_SOCKET_HANDLE)
  {
    PRINT_ERR("no free socket handle")
  }
  else if (csint_socket_create(sockhandle, addr_type, protocol, /* default local_port = 0 */ 0U, cid) != CELLULAR_OK)
  {
    /* socket creation error, deallocate handle */
    PRINT_ERR("socket creation failed")
    csint_socket_deallocateHandle(sockhandle);
    sockhandle = CS_INVALID_SOCKET_HANDLE;
  }
  else
  {
    /* socket created */
    PRINT_INFO("allocated socket handle=%ld (local)", sockhandle)
  }

  return (sockhandle);
}

/**
  * @brief  Bind the socket to a local port.
  * @note   If this function is not called, default local port value = 0 will be used.
  * @param  sockHandle Handle of the socket
  * @param  local_port Local port number.
  *         This parameter must be a value between 0 and 65535.
  * @retval CS_Status_t
  */
CS_Status_t CDS_socket_bind(socket_handle_t sockHandle,
                            uint16_t local_port)
{
  CS_Status_t res;

  /* check that socket has been allocated */
  if (cs_ctxt_sockets_info[sockHandle].state != SOCKETSTATE_CREATED)
  {
    PRINT_ERR("<Cellular_Service> socket bind allowed only after create/before connect %ld ", sockHandle)
    res = CELLULAR_ERROR;
  }
  else if (csint_socket_bind(sockHandle, local_port) != CELLULAR_OK)
  {
    PRINT_ERR("Socket Bind error")
    res = CELLULAR_ERROR;
  }
  else
  {
    res = CELLULAR_OK;
  }

  return res;
}

/**
  * @brief  Set the callbacks to use when data are received or sent.
  * @note   This function has to be called before to use a socket.
  * @param  sockHandle Handle of the socket
  * @param  data_ready_cb Pointer to the callback function to call when data are received
  * @param  data_sent_cb Pointer to the callback function to call when data has been sent
  *         This parameter is only used for asynchronous behavior (NOT IMPLEMENTED)
  * @retval CS_Status_t
  */
CS_Status_t CDS_socket_set_callbacks(socket_handle_t sockHandle,
                                     cellular_socket_data_ready_callback_t data_ready_cb,
                                     cellular_socket_data_sent_callback_t data_sent_cb,
                                     cellular_socket_closed_callback_t remote_close_cb)
{
  CS_Status_t retval;

  /* check that socket has been allocated */
  if (cs_ctxt_sockets_info[sockHandle].state == SOCKETSTATE_NOT_ALLOC)
  {
    PRINT_ERR("<Cellular_Service> invalid socket handle %ld (set cb)", sockHandle)
    retval = CELLULAR_ERROR;
  }
  /*PRINT_DBG("DBG: socket data ready callback=%p", cs_ctxt_sockets_info[sockHandle].socket_data_ready_callback)*/
  else if (data_ready_cb == NULL)
  {
    PRINT_ERR("data_ready_cb is mandatory")
    retval = CELLULAR_ERROR;
  }
  /*PRINT_DBG("DBG: socket remote closed callback=%p", cs_ctxt_sockets_info[sockHandle].socket_remote_close_callback)*/
  else if (remote_close_cb == NULL)
  {
    PRINT_ERR("remote_close_cb is mandatory")
    retval = CELLULAR_ERROR;
  }
  else
  {
    cs_ctxt_sockets_info[sockHandle].socket_data_ready_callback = data_ready_cb;
    cs_ctxt_sockets_info[sockHandle].socket_remote_close_callback = remote_close_cb;
    retval = CELLULAR_OK;
  }

  if (data_sent_cb != NULL)
  {
    /* NOT SUPPORTED - but does not return an error */
    PRINT_ERR("DATA sent callback not supported (only synch mode)")
  }

  return (retval);
}

/**
  * @brief  Define configurable options for a created socket.
  * @note   This function is called to configure one parameter at a time.
  *         If a parameter is not configured with this function, a default value will be applied.
  * @param  sockHandle Handle of the socket
  * @param  opt_level The level of TCP/IP stack component to be configured.
  *         This parameter can be one of the following values:
  *         SOL_IP
  *         SOL_TRANSPORT
  * @param  opt_name
  *         SON_IP_MAX_PACKET_SIZE Maximum packet size for data transfer (0 to 1500 bytes).
  *         SON_TRP_MAX_TIMEOUT Inactivity timeout (0 to 65535, in second, 0 means infinite).
  *         SON_TRP_CONNECT_SETUP_TIMEOUT Maximum timeout to setup connection with remote server
  *              (10 to 1200, in 100 of ms, 0 means infinite).
  *         SON_TRP_TRANSFER_TIMEOUT Maximum timeout to transfer data to remote server
  *              (1 to 255, in ms, 0 means infinite).
  *         SON_TRP_CONNECT_MODE To indicate if connection with modem will be dedicated (CM_ONLINE_MODE)
  *             or stay in command mode (CM_COMMAND_MODE) or stay in online mode until a
  *             suspend timeout expires (CM_ONLINE_AUTOMATIC_SUSPEND).
  *             Only CM_COMMAND_MODE is supported for the moment.
  *         SON_TRP_SUSPEND_TIMEOUT To define inactivty timeout to suspend online mode
  *             (0 to 2000, in ms , 0 means infinite).
  *         SON_TRP_RX_TIMEOUT Maximum timeout to receive data from remoteserver
  *             (0 to 255, in ms, 0 means infinite).
  * @param  p_opt_val Pointer to parameter to update (max size = uint16_t)
  * @retval CS_Status_t
  */
CS_Status_t CDS_socket_set_option(socket_handle_t sockHandle,
                                  CS_SocketOptionLevel_t opt_level,
                                  CS_SocketOptionName_t opt_name,
                                  void *p_opt_val)
{
  CS_Status_t retval;

  retval = csint_socket_configure(sockHandle, opt_level, opt_name, p_opt_val);
  return (retval);
}

/**
  * @brief  Connect to a remote server (for socket client mode).
  * @note   This function is blocking until the connection is setup or when the timeout to wait
  *         for socket connection expires.
  * @param  sockHandle Handle of the socket.
  * @param  ip_addr_type Specifies the type of IP address of the remote server.
  *         This parameter can be one of the following values:
  *         IPAT_IPV4 for IPV4 (default)
  *         IPAT_IPV6  for IPV6
  * @param  p_ip_addr_value Specifies the IP address of the remote server.
  * @param  remote_port Specifies the port of the remote server.
  * @retval CS_Status_t
  */
CS_Status_t CDS_socket_connect(socket_handle_t sockHandle,
                               CS_IPaddrType_t ip_addr_type,
                               CS_CHAR_t *p_ip_addr_value,
                               uint16_t remote_port)
{
  at_status_t err;
  CS_Status_t retval;

  retval = csint_socket_configure_remote(sockHandle, ip_addr_type, p_ip_addr_value, remote_port);
  if (retval == CELLULAR_OK)
  {
    /* Send socket information to ATcustom
    * no need to test sockHandle validity, it has been tested in csint_socket_configure_remote()
    */
    csint_socket_infos_t *socket_infos = &cs_ctxt_sockets_info[sockHandle];
    if (DATAPACK_writePtr(getCmdBufPtr(),
                          (uint16_t) CSMT_SOCKET_INFO,
                          (void *)socket_infos) == DATAPACK_OK)
    {
      if (socket_infos->trp_connect_mode == CS_CM_COMMAND_MODE)
      {
        err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_DIAL_COMMAND, getCmdBufPtr(), getRspBufPtr());
      }
      else
      {
        /* NOT SUPPORTED YET */
        err = ATSTATUS_ERROR;
        /* err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_DIAL_ONLINE, getCmdBufPtr(), getRspBufPtr());*/
      }

      if (err == ATSTATUS_OK)
      {
        /* update socket state */
        cs_ctxt_sockets_info[sockHandle].state = SOCKETSTATE_CONNECTED;
        retval = CELLULAR_OK;
      }
      else
      {
        PRINT_ERR("<Cellular_Service> error when socket connection")
        retval = CELLULAR_ERROR;
      }
    }
    else
    {
      retval = CELLULAR_ERROR;
    }
  }

  return (retval);
}

/**
  * @brief  Send data over a socket to a remote server.
  * @note   This function is blocking until the data is transferred or when the
  *         timeout to wait for transmission expires.
  * @param  sockHandle Handle of the socket
  * @param  p_buf Pointer to the data buffer to transfer.
  * @param  length Length of the data buffer.
  * @retval CS_Status_t
  */
CS_Status_t CDS_socket_send(socket_handle_t sockHandle,
                            const CS_CHAR_t *p_buf,
                            uint32_t length)
{
  CS_Status_t retval = CELLULAR_ERROR;

  /* check that size does not exceed maximum buffers size */
  if (length > DEFAULT_IP_MAX_PACKET_SIZE)
  {
    PRINT_ERR("<Cellular_Service> buffer size %ld exceed maximum value %d",
              length,
              DEFAULT_IP_MAX_PACKET_SIZE)
  }
  /* check that socket has been allocated */
  else if (cs_ctxt_sockets_info[sockHandle].state != SOCKETSTATE_CONNECTED)
  {
    PRINT_ERR("<Cellular_Service> socket not connected (state=%d) for handle %ld (send)",
              cs_ctxt_sockets_info[sockHandle].state,
              sockHandle)
  }
  else
  {
    csint_socket_data_buffer_t send_data_struct;
    (void) memset((void *)&send_data_struct, 0, sizeof(csint_socket_data_buffer_t));
    send_data_struct.socket_handle = sockHandle;
    send_data_struct.p_buffer_addr_send = p_buf;
    /* code sonar: useless assignment due to previous memset
     * send_data_struct.p_buffer_addr_rcv = NULL; */
    send_data_struct.buffer_size = length;
    send_data_struct.max_buffer_size = length;
    /* following parameters are not used (only in sendto) */
    /* code sonar: useless assignment due to previous memset
     * send_data_struct.ip_addr_type = CS_IPAT_INVALID; */
    /* send_data_struct.ip_addr_value already reset */
    /* send_data_struct.remote_port already reset */
    if (DATAPACK_writeStruct(getCmdBufPtr(),
                             (uint16_t) CSMT_SOCKET_DATA_BUFFER,
                             (uint16_t) sizeof(csint_socket_data_buffer_t),
                             (void *)&send_data_struct) == DATAPACK_OK)
    {
      at_status_t err;
      err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_SEND_DATA, getCmdBufPtr(), getRspBufPtr());
      if (err == ATSTATUS_OK)
      {
        /* <Cellular_Service> socket data sent */
        retval = CELLULAR_OK;
      }
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when sending data to socket")
  }
  return (retval);
}

/**
  * @brief  Send data over a socket to a remote server.
  * @note   This function is blocking until the data is transferred or when the
  *         timeout to wait for transmission expires.
  * @param  sockHandle Handle of the socket
  * @param  p_buf Pointer to the data buffer to transfer.
  * @param  length Length of the data buffer.
  * @param  ip_addr_type Specifies the type of the remote IP address.
  *         This parameter can be one of the following values:
  *         IPAT_IPV4 for IPV4
  *         IPAT_IPV6 for IPV6
  * @param  p_ip_addr_value Specifies the remote IP address.
  * @param  remote_port Specifies the remote port.
  * @retval CS_Status_t
  */
CS_Status_t CDS_socket_sendto(socket_handle_t sockHandle,
                              const CS_CHAR_t *p_buf,
                              uint32_t length,
                              CS_IPaddrType_t ip_addr_type,
                              CS_CHAR_t *p_ip_addr_value,
                              uint16_t remote_port)
{
  CS_Status_t retval = CELLULAR_ERROR;
  at_status_t err;
  size_t ip_addr_length;

  /* check that socket has been allocated */
  if (cs_ctxt_sockets_info[sockHandle].state != SOCKETSTATE_CONNECTED)
  {
    PRINT_ERR("<Cellular_Service> socket not connected (state=%d) for handle %ld (send)",
              cs_ctxt_sockets_info[sockHandle].state,
              sockHandle)
  }
  /* check that size does not exceed maximum buffers size */
  else if (length > DEFAULT_IP_MAX_PACKET_SIZE)
  {
    PRINT_ERR("<Cellular_Service> buffer size %ld exceed maximum value %d",
              length,
              DEFAULT_IP_MAX_PACKET_SIZE)
  }
  /* check p_ip_addr_value ptr */
  else if (p_ip_addr_value == NULL)
  {
    PRINT_ERR("<Cellular_Service> NULL ptr")
  }
  /* check p_ip_addr_value size */
  else
  {
    ip_addr_length = strlen((const CRC_CHAR_t *)p_ip_addr_value);
    if (ip_addr_length > MAX_IP_ADDR_SIZE)
    {
      PRINT_ERR("<Cellular_Service> IP address too long")
    }
    else
    {
      csint_socket_data_buffer_t send_data_struct;
      (void) memset((void *)&send_data_struct, 0, sizeof(csint_socket_data_buffer_t));
      send_data_struct.socket_handle = sockHandle;
      send_data_struct.p_buffer_addr_send = p_buf;
      /* code sonar: useless assignment due to previous memset
       * send_data_struct.p_buffer_addr_rcv = NULL; */
      send_data_struct.buffer_size = length;
      send_data_struct.max_buffer_size = length;
      /* sendto parameters specific */
      send_data_struct.ip_addr_type = ip_addr_type;
      (void) memcpy((void *)send_data_struct.ip_addr_value,
                    (const CS_CHAR_t *)p_ip_addr_value,
                    ip_addr_length);
      send_data_struct.remote_port = remote_port;
      if (DATAPACK_writeStruct(getCmdBufPtr(),
                               (uint16_t) CSMT_SOCKET_DATA_BUFFER,
                               (uint16_t) sizeof(csint_socket_data_buffer_t),
                               (void *)&send_data_struct) == DATAPACK_OK)
      {
        err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_SEND_DATA, getCmdBufPtr(), getRspBufPtr());
        if (err == ATSTATUS_OK)
        {
          /* <Cellular_Service> socket data sent (sendto) */
          retval = CELLULAR_OK;
        }
      }
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when sending data to socket (sendto)")
  }
  return (retval);
}


/**
  * @brief  Receive data from the connected remote server.
  * @note   This function is blocking until expected data length is received or a receive timeout has expired.
  * @param  sockHandle Handle of the socket
  * @param  p_buf Pointer to the data buffer to received data.
  * @param  max_buf_length Maximum size of receive data buffer.
  * @retval Size of received data (in bytes).
  */
int32_t CDS_socket_receive(socket_handle_t sockHandle,
                           CS_CHAR_t *p_buf,
                           uint32_t max_buf_length)
{
  int32_t returned_data_size;
  CS_Status_t status = CELLULAR_ERROR;
  uint32_t bytes_received = 0U;

  /* check that size does not exceed maximum buffers size */
  if (max_buf_length > DEFAULT_IP_MAX_PACKET_SIZE)
  {
    PRINT_ERR("<Cellular_Service> buffer size %ld exceed maximum value %d",
              max_buf_length,
              DEFAULT_IP_MAX_PACKET_SIZE)
  }
  /* check that socket has been allocated */
  else if (cs_ctxt_sockets_info[sockHandle].state != SOCKETSTATE_CONNECTED)
  {
    PRINT_ERR("<Cellular_Service> socket not connected (state=%d) for handle %ld (rcv)",
              cs_ctxt_sockets_info[sockHandle].state,
              sockHandle)
  }
  else
  {
    csint_socket_data_buffer_t receive_data_struct = {0};
    (void) memset((void *)&receive_data_struct, 0, sizeof(csint_socket_data_buffer_t));
    receive_data_struct.socket_handle = sockHandle;
    /* code sonar: useless assignment due to previous memset
     * receive_data_struct.p_buffer_addr_send = NULL; */
    receive_data_struct.p_buffer_addr_rcv = p_buf;
    /* code sonar: useless assignment due to previous memset
     * receive_data_struct.buffer_size = 0U; */
    receive_data_struct.max_buffer_size = max_buf_length;
    /* following parameters are not used (only in receivefrom) */
    /* code sonar: useless assignment due to previous memset
     * receive_data_struct.ip_addr_type = CS_IPAT_INVALID; */
    /* receive_data_struct.ip_addr_value already reset */
    /* receive_data_struct.remote_port already reset */
    if (DATAPACK_writeStruct(getCmdBufPtr(),
                             (uint16_t) CSMT_SOCKET_DATA_BUFFER,
                             (uint16_t) sizeof(csint_socket_data_buffer_t),
                             (void *)&receive_data_struct) == DATAPACK_OK)
    {
      at_status_t err;
      err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_RECEIVE_DATA, getCmdBufPtr(), getRspBufPtr());
      if (err == ATSTATUS_OK)
      {
        if (DATAPACK_readStruct(getRspBufPtr(),
                                (uint16_t) CSMT_SOCKET_RXDATA,
                                (uint16_t) sizeof(uint32_t),
                                &bytes_received) == DATAPACK_OK)
        {
          status = CELLULAR_OK;
        }
      }
    }
  }

  if (status == CELLULAR_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when receiving data from socket")
    returned_data_size = -1;
  }
  else
  {
    PRINT_INFO("Size of data received on the socket= %ld bytes", bytes_received)
    returned_data_size = ((int32_t)bytes_received);
  }

  return (returned_data_size);
}

/**
  * @brief  Receive data from the connected remote server.
  * @note   This function is blocking until expected data length is received or a receive timeout has expired.
  * @param  sockHandle Handle of the socket
  * @param  p_buf Pointer to the data buffer to received data.
  * @param  max_buf_length Maximum size of receive data buffer.
  * @param  ip_addr_type Specifies the type of the remote IP address.
  *         This parameter can be one of the following values:
  *         IPAT_IPV4 for IPV4
  *         IPAT_IPV6 for IPV6
  * @param  p_ip_addr_value Specifies the remote IP address.
  * @param  remote_port Specifies the remote port.
  * @retval Size of received data (in bytes).
  */
int32_t CDS_socket_receivefrom(socket_handle_t sockHandle,
                               CS_CHAR_t *p_buf,
                               uint32_t max_buf_length,
                               CS_IPaddrType_t *p_ip_addr_type,
                               CS_CHAR_t *p_ip_addr_value,
                               uint16_t *p_remote_port)
{
  int32_t returned_data_size;
  CS_Status_t status = CELLULAR_ERROR;
  uint32_t bytes_received = 0U;

  /* check that size does not exceed maximum buffers size */
  if (max_buf_length > DEFAULT_IP_MAX_PACKET_SIZE)
  {
    PRINT_ERR("<Cellular_Service> buffer size %ld exceed maximum value %d",
              max_buf_length,
              DEFAULT_IP_MAX_PACKET_SIZE)
  }
  /* check that socket has been allocated */
  else if (cs_ctxt_sockets_info[sockHandle].state != SOCKETSTATE_CONNECTED)
  {
    PRINT_ERR("<Cellular_Service> socket not connected (state=%d) for handle %ld (rcv)",
              cs_ctxt_sockets_info[sockHandle].state,
              sockHandle)
  }
  else
  {
    csint_socket_data_buffer_t receive_data_struct = {0};
    (void) memset((void *)&receive_data_struct, 0, sizeof(csint_socket_data_buffer_t));
    receive_data_struct.socket_handle = sockHandle;
    /* code sonar: useless assignment due to previous memset
     * receive_data_struct.p_buffer_addr_send = NULL; */
    receive_data_struct.p_buffer_addr_rcv = p_buf;
    /* code sonar: useless assignment due to previous memset
     * receive_data_struct.buffer_size = 0U; */
    receive_data_struct.max_buffer_size = max_buf_length;
    /* following parameters are returned parameters */
    /* code sonar: useless assignment due to previous memset
     * receive_data_struct.ip_addr_type = CS_IPAT_INVALID; */
    /* receive_data_struct.ip_addr_value already reset */
    /* receive_data_struct.remote_port already reset */
    if (DATAPACK_writeStruct(getCmdBufPtr(),
                             (uint16_t) CSMT_SOCKET_DATA_BUFFER,
                             (uint16_t) sizeof(csint_socket_data_buffer_t),
                             (void *)&receive_data_struct) == DATAPACK_OK)
    {
      at_status_t err;
      err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_RECEIVE_DATA_FROM, getCmdBufPtr(), getRspBufPtr());
      if (err == ATSTATUS_OK)
      {
        csint_socket_rxdata_from_t  rx_data_from;
        if (DATAPACK_readStruct(getRspBufPtr(),
                                (uint16_t) CSMT_SOCKET_RXDATA_FROM,
                                (uint16_t) sizeof(csint_socket_rxdata_from_t),
                                &rx_data_from) == DATAPACK_OK)
        {
          bytes_received = rx_data_from.bytes_received;
          /* recopy info to user */
          *p_ip_addr_type = rx_data_from.ip_addr_type;
          (void) memcpy((void *)p_ip_addr_value,
                        (void *)&rx_data_from.ip_addr_value,
                        strlen((CRC_CHAR_t *)rx_data_from.ip_addr_value));
          *p_remote_port = rx_data_from.remote_port;
          status = CELLULAR_OK;
        }
      }
    }

  }

  if (status == CELLULAR_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when receiving data from socket")
    returned_data_size = -1;
  }
  else
  {
    PRINT_INFO("Size of data received on the socket= %ld bytes", bytes_received)
    returned_data_size = ((int32_t)bytes_received);
  }

  return (returned_data_size);
}

/**
  * @brief  Free a socket handle.
  * @note   If a PDN is activated at socket creation, the socket will not be deactivated at socket closure.
  * @param  sockHandle Handle of the socket
  * @param  force Force to free the socket.
  * @retval CS_Status_t
  */
CS_Status_t CDS_socket_close(socket_handle_t sockHandle, uint8_t force)
{
  UNUSED(force);
  CS_Status_t retval = CELLULAR_ERROR;

  if (cs_ctxt_sockets_info[sockHandle].state == SOCKETSTATE_CONNECTED)
  {
    /* Send socket information to ATcustom */
    csint_socket_infos_t *socket_infos = &cs_ctxt_sockets_info[sockHandle];
    if (DATAPACK_writePtr(getCmdBufPtr(),
                          (uint16_t) CSMT_SOCKET_INFO,
                          (void *)socket_infos) == DATAPACK_OK)
    {
      at_status_t err;
      err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_SOCKET_CLOSE, getCmdBufPtr(), getRspBufPtr());
      if (err == ATSTATUS_OK)
      {
        /* deallocate socket handle and reinit socket parameters */
        csint_socket_deallocateHandle(sockHandle);
        retval = CELLULAR_OK;
      }
    }
  }
  else if (cs_ctxt_sockets_info[sockHandle].state == SOCKETSTATE_CREATED)
  {
    PRINT_INFO("<Cellular_Service> socket was not connected ")
    /* deallocate socket handle and reinit socket parameters */
    csint_socket_deallocateHandle(sockHandle);
    retval = CELLULAR_OK;
  }
  else if (cs_ctxt_sockets_info[sockHandle].state == SOCKETSTATE_NOT_ALLOC)
  {
    PRINT_ERR("<Cellular_Service> invalid socket handle %ld (close)", sockHandle)
  }
  else if (cs_ctxt_sockets_info[sockHandle].state == SOCKETSTATE_ALLOC_BUT_INVALID)
  {
    PRINT_INFO("<Cellular_Service> invalid socket state (after modem reboot) ")
    /* deallocate socket handle and reinit socket parameters */
    csint_socket_deallocateHandle(sockHandle);
    retval = CELLULAR_OK;
  }
  else
  {
    PRINT_ERR("<Cellular_Service> invalid socket state %d (close)", cs_ctxt_sockets_info[sockHandle].state)
  }

  if (retval == CELLULAR_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when closing socket")
  }
  return (retval);
}

/**
  * @brief  Get connection status for a given socket.
  * @note   If a PDN is activated at socket creation, the socket will not be deactivated at socket closure.
  * @param  sockHandle Handle of the socket
  * @param  p_infos Pointer of infos structure.
  * @retval CS_Status_t
  */
CS_Status_t CDS_socket_cnx_status(socket_handle_t sockHandle,
                                  CS_SocketCnxInfos_t *p_infos)
{
  CS_Status_t retval = CELLULAR_ERROR;

  /* check that socket has been allocated */
  if (cs_ctxt_sockets_info[sockHandle].state != SOCKETSTATE_CONNECTED)
  {
    PRINT_ERR("<Cellular_Service> socket not connected (state=%d) for handle %ld (status)",
              cs_ctxt_sockets_info[sockHandle].state,
              sockHandle)
  }
  else
  {
    /* Send socket information to ATcustom */
    csint_socket_cnx_infos_t socket_cnx_infos;
    socket_cnx_infos.socket_handle = sockHandle;
    socket_cnx_infos.infos = p_infos;
    (void) memset((void *)p_infos, 0, sizeof(CS_SocketCnxInfos_t));
    if (DATAPACK_writePtr(getCmdBufPtr(),
                          (uint16_t) CSMT_SOCKET_CNX_STATUS,
                          (void *)&socket_cnx_infos) == DATAPACK_OK)
    {
      at_status_t err;
      err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_SOCKET_CNX_STATUS, getCmdBufPtr(), getRspBufPtr());
      if (err == ATSTATUS_OK)
      {
        /* <Cellular_Service> socket cnx status received */
        retval = CELLULAR_OK;
      }
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when requesting socket cnx status")
  }
  return (retval);
}


/**
  * @brief  Ping an IP address on the network
  * @note   Usually, the command AT is sent and OK is expected as response
  * @param  address or full parameters set
  * @param  ping_callback Handle on user callback that will be used to analyze
  *                              the ping answer from the network address.
  * @retval CS_Status_t
  */
CS_Status_t CDS_ping(CS_PDN_conf_id_t cid, CS_Ping_params_t *ping_params,
                     cellular_ping_response_callback_t cs_ping_rsp_cb)
{
  CS_Status_t retval = CELLULAR_ERROR;

  /* build internal structure to send */
  csint_ping_params_t loc_ping_params;
  (void) memset((void *)&loc_ping_params, 0, sizeof(csint_ping_params_t));
  loc_ping_params.conf_id = cid;
  (void) memcpy((void *)&loc_ping_params.ping_params, ping_params, sizeof(CS_Ping_params_t));

  /* save the callback */
  urc_ping_rsp_callback = cs_ping_rsp_cb;

  if (DATAPACK_writeStruct(getCmdBufPtr(),
                           (uint16_t) CSMT_PING_ADDRESS,
                           (uint16_t) sizeof(csint_ping_params_t),
                           (void *)&loc_ping_params) == DATAPACK_OK)
  {
    at_status_t err;
    err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_PING_IP_ADDRESS, getCmdBufPtr(), getRspBufPtr());
    if (err == ATSTATUS_OK)
    {
      /* <Cellular_Service> Ping address OK */
      /* Check if ping response received from lower layer
       * Two cases are possible:
       *  1) ping reports are received as URC (will be received later)
       *  2) ping reports received inside ping transaction: final report should be received now
       */
      CS_Ping_response_t ping_rsp;
      (void) memset((void *)&ping_rsp, 0, sizeof(CS_Ping_response_t));
      if (DATAPACK_readStruct(getRspBufPtr(),
                              (uint16_t) CSMT_URC_PING_RSP,
                              (uint16_t) sizeof(CS_Ping_response_t),
                              (void *)&ping_rsp) == DATAPACK_OK)
      {
        if (ping_rsp.index != PING_INVALID_INDEX)
        {
          PRINT_INFO("<Cellular_Service> Ping transaction finished")
          if (urc_ping_rsp_callback != NULL)
          {
            (* urc_ping_rsp_callback)(ping_rsp);
          }
        }
        else
        {
          PRINT_INFO("<Cellular_Service> Waiting for ping reports")
        }
        retval = CELLULAR_OK;
      }
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error during ping request")
  }
  return (retval);
}

/**
  * @brief  DNS request
  * @note   Get IP address of the specified hostname
  * @param  cid Configuration identifier number.
  * @param  *dns_req Handle to DNS request structure.
  * @param  *dns_resp Handle to DNS response structure.
  * @retval CS_Status_t
  */
CS_Status_t CDS_dns_request(CS_PDN_conf_id_t cid, CS_DnsReq_t *dns_req, CS_DnsResp_t *dns_resp)
{
  CS_Status_t retval = CELLULAR_ERROR;
  uint32_t dns_ip_addr_length;

  /* build internal structure to send */
  csint_dns_request_t loc_dns_req;
  (void) memset((void *)&loc_dns_req, 0, sizeof(csint_dns_request_t));
  /* set CID */
  loc_dns_req.conf_id = cid;
  /* recopy dns_req */
  (void) memcpy((void *)&loc_dns_req.dns_req, dns_req, sizeof(CS_DnsReq_t));

  /* set DNS primary address to use */
  dns_ip_addr_length = strlen((const CRC_CHAR_t *)PLF_CELLULAR_DNS_SERVER_IP_ADDR);
  if (dns_ip_addr_length > MAX_IP_ADDR_SIZE)
  {
    PRINT_ERR("<Cellular_Service> DNS IP address too long")
  }
  else
  {
    (void) memcpy((void *)&loc_dns_req.dns_conf.primary_dns_addr,
                  (const CS_CHAR_t *)PLF_CELLULAR_DNS_SERVER_IP_ADDR,
                  dns_ip_addr_length);
    if (DATAPACK_writePtr(getCmdBufPtr(),
                          (uint16_t) CSMT_DNS_REQ,
                          (void *)&loc_dns_req) == DATAPACK_OK)
    {
      at_status_t err;
      err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_DNS_REQ, getCmdBufPtr(), getRspBufPtr());
      if (err == ATSTATUS_OK)
      {
        if (DATAPACK_readStruct(getRspBufPtr(),
                                (uint16_t) CSMT_DNS_REQ,
                                (uint16_t) sizeof(dns_resp->host_addr),
                                dns_resp->host_addr) == DATAPACK_OK)
        {
          /* <Cellular_Service> DNS configuration done */
          retval = CELLULAR_OK;
        }
      }
    }
  }

  if (retval == CELLULAR_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error during DNS request")
  }
  return (retval);
}

/**
  * @brief  Configure DNS settings
  * @note   Function not implemented yet
  * @param  cid Configuration identifier number.
  * @param  *dns_conf Handle to DNS configuration structure.
  * @retval CS_Status_t
  */
CS_Status_t CDS_dns_config(CS_PDN_conf_id_t cid, CS_DnsConf_t *dns_conf)
{
  UNUSED(cid);
  UNUSED(dns_conf);
  return (CELLULAR_NOT_IMPLEMENTED);
}

/**
  * @brief  Retrieve configurable options for a created socket.
  * @note   This function is called for one parameter at a time.
  * @note   Function not implemented yet
  * @retval CS_Status_t
  */
CS_Status_t CDS_socket_get_option(void)
{
  return (CELLULAR_NOT_IMPLEMENTED);
}

/**
  * @brief  Listen to clients (for socket server mode).
  * @note   Function not implemented yet
  * @param  sockHandle Handle of the socket
  * @retval CS_Status_t
  */
CS_Status_t CDS_socket_listen(socket_handle_t sockHandle)
{
  UNUSED(sockHandle);

  /* for socket server mode */
  return (CELLULAR_NOT_IMPLEMENTED);
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


