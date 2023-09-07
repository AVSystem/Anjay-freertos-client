/**
  ******************************************************************************
  * @file    cellular_service_os.c
  * @author  MCD Application Team
  * @brief   This file defines functions for Cellular Service OS dependence
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
#include "rtosal.h"
#include "error_handler.h"
#include "cellular_service_task.h"
#include "cellular_service_os.h"


/* Private typedef -----------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static osMutexId CellularServiceMutexHandle;
static osMutexId CellularServiceGeneralMutexHandle;

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/
/**
  * @brief  Read the actual signal quality seen by Modem .
  * @note   Call CS_get_signal_quality with mutex access protection
  * @param  same parameters as the CS_get_signal_quality function
  * @retval CS_Status_t
  */
CS_Status_t osCS_get_signal_quality(CS_SignalQuality_t *p_sig_qual)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);

  result = CS_get_signal_quality(p_sig_qual);

  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/**
  * @brief  Allocate a socket among of the free sockets (maximum 6 sockets)
  * @note   Call CDS_socket_create with mutex access protection
  * @param  same parameters as the CDS_socket_create function
  * @retval Socket handle which references allocated socket
  */
socket_handle_t osCDS_socket_create(CS_IPaddrType_t addr_type,
                                    CS_TransportProtocol_t protocol,
                                    CS_PDN_conf_id_t cid)
{
  socket_handle_t socket_handle;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);

  socket_handle = CDS_socket_create(addr_type,
                                    protocol,
                                    cid);
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (socket_handle);
}

/**
  * @brief  Set the callbacks to use when data are received or sent.
  * @note   This function has to be called before to use a socket.
  * @note   Call CDS_socket_set_callbacks with mutex access protection
  * @param  same parameters as the CDS_socket_set_callbacks function
  * @retval CS_Status_t
  */
CS_Status_t osCDS_socket_set_callbacks(socket_handle_t sockHandle,
                                       cellular_socket_data_ready_callback_t data_ready_cb,
                                       cellular_socket_data_sent_callback_t data_sent_cb,
                                       cellular_socket_closed_callback_t remote_close_cb)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);

  result = CDS_socket_set_callbacks(sockHandle,
                                    data_ready_cb,
                                    data_sent_cb,
                                    remote_close_cb);

  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

#if defined(CSAPI_OPTIONAL_FUNCTIONS)
/**
  * @brief  Define configurable options for a created socket.
  * @note   This function is called to configure one parameter at a time.
  *         If a parameter is not configured with this function, a default value will be applied.
  * @note   Call CDS_socket_set_option with mutex access protection
  * @param  same parameters as the CDS_socket_set_option function
  * @retval CS_Status_t
  */

CS_Status_t osCDS_socket_set_option(socket_handle_t sockHandle,
                                    CS_SocketOptionLevel_t opt_level,
                                    CS_SocketOptionName_t opt_name,
                                    void *p_opt_val)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);

  result = CDS_socket_set_option(sockHandle,
                                 opt_level,
                                 opt_name,
                                 p_opt_val);

  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/**
  * @brief  Retrieve configurable options for a created socket.
  * @note   This function is called for one parameter at a time.
  * @note   Function not implemented yet
  * @note   Call CDS_socket_get_option with mutex access protection
  * @param  same parameters as the CDS_socket_get_option function
  * @retval CS_Status_t
  */

CS_Status_t osCDS_socket_get_option(void)
{
  CS_Status_t result = CS_ERROR;

  if (CST_get_state() == CST_MODEM_DATA_READY_STATE)
  {
    (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);

    result = CDS_socket_get_option();

    (void)rtosalMutexRelease(CellularServiceMutexHandle);
  }

  return (result);
}
#endif /* defined(CSAPI_OPTIONAL_FUNCTIONS) */

/**
  * @brief  Bind the socket to a local port.
  * @note   If this function is not called, default local port value = 0 will be used.
  * @note   Call CDS_socket_bind with mutex access protection
  * @param  same parameters as the CDS_socket_bind function
  * @retval CS_Status_t
  */
CS_Status_t osCDS_socket_bind(socket_handle_t sockHandle,
                              uint16_t local_port)
{
  CS_Status_t result = CS_ERROR;

  if (CST_get_state() == CST_MODEM_DATA_READY_STATE)
  {
    (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);

    result = CDS_socket_bind(sockHandle,
                             local_port);

    (void)rtosalMutexRelease(CellularServiceMutexHandle);
  }

  return (result);
}

/**
  * @brief  Connect to a remote server (for socket client mode).
  * @note   This function is blocking until the connection is setup or when the timeout to wait
  *         for socket connection expires.
  * @note   Call CDS_socket_connect with mutex access protection
  * @param  same parameters as the CDS_socket_connect function
  * @retval CS_Status_t
  */
CS_Status_t osCDS_socket_connect(socket_handle_t sockHandle,
                                 CS_IPaddrType_t addr_type,
                                 CS_CHAR_t *p_ip_addr_value,
                                 uint16_t remote_port)
{
  CS_Status_t result = CS_ERROR;

  if (CST_get_state() == CST_MODEM_DATA_READY_STATE)
  {
    (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);

    result = CDS_socket_connect(sockHandle,
                                addr_type,
                                p_ip_addr_value,
                                remote_port);

    (void)rtosalMutexRelease(CellularServiceMutexHandle);
  }

  return (result);
}

/**
  * @brief  Listen to clients (for socket server mode).
  * @note   Function not implemented yet
  * @note   Call CDS_socket_listen with mutex access protection
  * @param  same parameters as the CDS_socket_listen function
  * @retval CS_Status_t
  */
CS_Status_t osCDS_socket_listen(socket_handle_t sockHandle)
{
  CS_Status_t result = CS_ERROR;

  if (CST_get_state() == CST_MODEM_DATA_READY_STATE)
  {
    (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);

    result = CDS_socket_listen(sockHandle);

    (void)rtosalMutexRelease(CellularServiceMutexHandle);
  }

  return (result);
}

/**
  * @brief  Send data over a socket to a remote server.
  * @note   This function is blocking until the data is transferred or when the
  *         timeout to wait for transmission expires.
  * @note   Call CDS_socket_send with mutex access protection
  * @param  same parameters as the CDS_socket_send function
  * @retval CS_Status_t
  */
CS_Status_t osCDS_socket_send(socket_handle_t sockHandle,
                              const CS_CHAR_t *p_buf,
                              uint32_t length)
{
  CS_Status_t result = CS_ERROR;

  if (CST_get_state() == CST_MODEM_DATA_READY_STATE)
  {
    (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);

    result = CDS_socket_send(sockHandle,
                             p_buf,
                             length);

    (void)rtosalMutexRelease(CellularServiceMutexHandle);
  }

  return (result);
}

/**
  * @brief  Receive data from the connected remote server.
  * @note   This function is blocking until expected data length is received or a receive timeout has expired.
  * @note   Call CDS_socket_receive with mutex access protection
  * @param  same parameters as the CDS_socket_receive function
  * @retval Size of received data (in bytes).
  */
int32_t osCDS_socket_receive(socket_handle_t sockHandle,
                             CS_CHAR_t *p_buf,
                             uint32_t  max_buf_length)
{
  int32_t result;

  result = 0;
  if (CST_get_state() == CST_MODEM_DATA_READY_STATE)
  {
    (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);

    result = CDS_socket_receive(sockHandle,
                                p_buf,
                                max_buf_length);

    (void)rtosalMutexRelease(CellularServiceMutexHandle);
  }

  return (result);
}

/**
  * @brief  Send data over a socket to a remote server.
  * @note   This function is blocking until the data is transferred or when the
  *         timeout to wait for transmission expires.
  * @note   Call CDS_socket_sendto with mutex access protection
  * @param  same parameters as the CDS_socket_sendto function
  * @retval CS_Status_t
  */
CS_Status_t osCDS_socket_sendto(socket_handle_t sockHandle,
                                const CS_CHAR_t *p_buf,
                                uint32_t length,
                                CS_IPaddrType_t addr_type,
                                CS_CHAR_t *p_ip_addr_value,
                                uint16_t remote_port)

{
  CS_Status_t result = CS_ERROR;

  if (CST_get_state() == CST_MODEM_DATA_READY_STATE)
  {
    (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);

    result = CDS_socket_sendto(sockHandle,
                               p_buf,
                               length,
                               addr_type,
                               p_ip_addr_value,
                               remote_port);

    (void)rtosalMutexRelease(CellularServiceMutexHandle);
  }

  return (result);
}

/**
  * @brief  Receive data from the connected remote server.
  * @note   This function is blocking until expected data length is received or a receive timeout has expired.
  * @note   Call CDS_socket_receivefrom with mutex access protection
  * @param  same parameters as the CDS_socket_receivefrom function
  * @retval Size of received data (in bytes).
  */
int32_t osCDS_socket_receivefrom(socket_handle_t sockHandle,
                                 CS_CHAR_t *p_buf,
                                 uint32_t max_buf_length,
                                 CS_IPaddrType_t *p_addr_type,
                                 CS_CHAR_t *p_ip_addr_value,
                                 uint16_t *p_remote_port)
{
  int32_t result;

  result = 0;
  if (CST_get_state() == CST_MODEM_DATA_READY_STATE)
  {
    (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);

    result = CDS_socket_receivefrom(sockHandle,
                                    p_buf,
                                    max_buf_length,
                                    p_addr_type,
                                    p_ip_addr_value,
                                    p_remote_port);

    (void)rtosalMutexRelease(CellularServiceMutexHandle);
  }

  return (result);
}

/**
  * @brief  Free a socket handle.
  * @note   If a PDN is activated at socket creation, the socket will not be deactivated at socket closure.
  * @note   Call CDS_socket_close with mutex access protection
  * @param  same parameters as the CDS_socket_close function
  * @retval CS_Status_t
  */
CS_Status_t osCDS_socket_close(socket_handle_t sockHandle,
                               uint8_t force)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);

  result = CDS_socket_close(sockHandle,
                            force);

  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/**
  * @brief  Get connection status for a given socket.
  * @note   If a PDN is activated at socket creation, the socket will not be deactivated at socket closure.
  * @note   Call CDS_socket_cnx_status with mutex access protection
  * @param  same parameters as the CDS_socket_cnx_status function
  * @retval CS_Status_t
  */
CS_Status_t osCDS_socket_cnx_status(socket_handle_t sockHandle,
                                    CS_SocketCnxInfos_t *infos)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);

  result = CDS_socket_cnx_status(sockHandle,
                                 infos);

  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/**
  * @brief  cellular service initialization
  * @param  none
  * @retval CS_Bool_t   cellular initialized or not
  */
CS_Bool_t osCDS_cellular_service_init(void)
{
  static CS_Bool_t CellularServiceInitialized = CS_FALSE;
  CS_Bool_t result;

  result = CS_TRUE;
  if (CellularServiceInitialized == CS_FALSE)
  {
    CellularServiceMutexHandle = rtosalMutexNew((const rtosal_char_t *)"CS_MUT_CTRL_PLANE");
    if (CellularServiceMutexHandle == NULL)
    {
      result = CS_FALSE;
      /* Platform is reset */
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 1, ERROR_FATAL);
    }
    CellularServiceGeneralMutexHandle = rtosalMutexNew((const rtosal_char_t *)"CS_MUT_DATA_PLANE");
    if (CellularServiceGeneralMutexHandle == NULL)
    {
      result = CS_FALSE;
      /* Platform is reset */
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 2, ERROR_FATAL);
    }

    /* To do next line of code not done under if result == CS_TRUE
       because if result == CS_FALSE platform is reset (avoid quality error)
     */
    CellularServiceInitialized = CS_TRUE;
  }

  return result;
}

/* =========================================================
   ===========      Mode Command services        ===========
   ========================================================= */


/**
  * @brief  Read the latest registration state to the Cellular Network.
  * @note   Call CS_get_net_status with mutex access protection
  * @param  same parameters as the CS_get_net_status function
  * @retval CS_Status_t
  */
CS_Status_t osCDS_get_net_status(CS_RegistrationStatus_t *p_reg_status)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_get_net_status(p_reg_status);
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/**
  * @brief  Return information related to modem status.
  * @note   Call CS_get_device_info with mutex access protection
  * @param  same parameters as the CS_get_device_info function
  * @retval CS_Status_t
  */
CS_Status_t osCDS_get_device_info(CS_DeviceInfo_t *p_devinfo)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_get_device_info(p_devinfo);
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/**
  * @brief  Register to an event change notification related to Network status.
  * @note   This function should be called for each event the user wants to start monitoring.
  * @param  same parameters as the CS_subscribe_net_event function
  * @param  urc_callback Handle on user callback that will be used to notify a
  *                      change on requested event.
  * @retval CS_Status_t
  */
CS_Status_t osCDS_subscribe_net_event(CS_UrcEvent_t event, cellular_urc_callback_t urc_callback)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_subscribe_net_event(event,  urc_callback);
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/**
  * @brief  Register to specified modem events.
  * @note   This function should be called once with all requested events.
  * @note   Call CS_subscribe_modem_event with mutex access protection
  * @param  same parameters as the CS_subscribe_modem_event function
  *         change on requested event.
  * @retval CS_Status_t
  */
CS_Status_t osCDS_subscribe_modem_event(CS_ModemEvent_t events_mask, cellular_modem_event_callback_t modem_evt_cb)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_subscribe_modem_event(events_mask, modem_evt_cb);
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/**
  * @brief  Power ON the modem
  * @note   Call CS_power_on with mutex access protection
  * @param  none
  * @retval CS_Status_t
  */
CS_Status_t osCDS_power_on(void)
{
  CS_Status_t result = CS_OK;

  if (cst_context.modem_on == false)
  {
    (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
    result = CS_power_on();
    (void)rtosalMutexRelease(CellularServiceMutexHandle);
    if (result == CS_OK)
    {
      cst_context.modem_on = true;
    }
  }
  return (result);
}

/**
  * @brief  Power OFF the modem
  * @note   Call CS_power_off with mutex access protection
  * @param  none
  * @retval CS_Status_t
  */
CS_Status_t osCDS_power_off(void)
{
  CS_Status_t result = CS_OK;

  if (cst_context.modem_on == true)
  {
    (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
    result = CS_power_off();
    (void)rtosalMutexRelease(CellularServiceMutexHandle);
    if (result == CS_OK)
    {
      cst_context.modem_on = false;
    }
  }
  return (result);
}

/**
  * @brief  Request to reset the device.
  * @note   Call CS_reset with mutex access protection
  * @param  same parameters as the CS_reset function
  * @retval CS_Status_t
  */
CS_Status_t osCDS_reset(CS_Reset_t rst_type)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_reset(rst_type);
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/**
  * @brief  Initialize the service and configures the Modem FW functionalities
  * @note   Used to provide PIN code (if any) and modem function level.
  * @note   Call CS_init_modem with mutex access protection
  * @param  same parameters as the CS_init_modem function
  * @retval CS_Status_t
  */
CS_Status_t osCDS_init_modem(CS_ModemInit_t init,
                             CS_Bool_t reset,
                             const CS_CHAR_t *pin_code)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_init_modem(init,  reset, pin_code);
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/**
  * @brief
  * @param  none
  * @retval none
  */
void osCCS_get_wait_cs_resource(void)
{
  (void)rtosalMutexAcquire(CellularServiceGeneralMutexHandle, RTOSAL_WAIT_FOREVER);
}

/**
  * @brief
  * @param  none
  * @retval none
  */
void osCCS_get_release_cs_resource(void)
{
  (void)rtosalMutexRelease(CellularServiceGeneralMutexHandle);
}

/**
  * @brief  Request the Modem to register to the Cellular Network.
  * @note   This function is used to select the operator. It returns a detailed
  *         network registration status.
  * @note   Call CS_register_net with mutex access protection
  * @param  same parameters as the CS_register_net function
  * @retval CS_Status_t
  */
CS_Status_t osCDS_register_net(CS_OperatorSelector_t *p_operator,
                               CS_RegistrationStatus_t *p_reg_status)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_register_net(p_operator, p_reg_status);
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/**
  * @brief  Request detach from packet domain.
  * @param  none.
  * @retval CS_Status_t
  */
CS_Status_t osCS_detach_PS_domain(void)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_detach_PS_domain();
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/**
  * @brief  Request for packet attach status.
  * @note   Call CDS_socket_set_callbacks with mutex access protection
  * @param  same parameters as the CDS_socket_set_callbacks function
  * @retval CS_Status_t
  */
CS_Status_t osCDS_get_attach_status(CS_PSattach_t *p_attach)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_get_attach_status(p_attach);
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}


/**
  * @brief  Request attach to packet domain.
  * @note   Call CS_attach_PS_domain with mutex access protection
  * @param  none.
  * @retval CS_Status_t
  */
CS_Status_t osCDS_attach_PS_domain(void)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_attach_PS_domain();
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}


/**
  * @brief  Define internet data profile for a configuration identifier
  * @note   Call CS_define_pdn with mutex access protection
  * @param  same parameters as the CS_define_pdn function
  * @retval CS_Status_t
  */
CS_Status_t osCDS_define_pdn(CS_PDN_conf_id_t cid,
                             const CS_CHAR_t *apn,
                             CS_PDN_configuration_t *pdn_conf)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_define_pdn(cid, apn, pdn_conf);
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/**
  * @brief  Register to event notifications related to internet connection.
  * @note   This function is used to register to an event related to a PDN
  *         Only explicit config id (CS_PDN_USER_CONFIG_1 to CS_PDN_USER_CONFIG_5) are
  *         supported and CS_PDN_PREDEF_CONFIG
  * @note   Call CS_register_pdn_event with mutex access protection
  * @param  same parameters as the CS_register_pdn_event function
  * @retval CS_Status_t
  */
CS_Status_t osCDS_register_pdn_event(CS_PDN_conf_id_t cid,
                                     cellular_pdn_event_callback_t pdn_event_callback)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_register_pdn_event(cid,  pdn_event_callback);
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/**
  * @brief  Select a PDN among of defined configuration identifier(s) as the default.
  * @note   By default, PDN_PREDEF_CONFIG is considered as the default PDN.
  * @note   Call CS_set_default_pdn with mutex access protection
  * @param  same parameters as the CS_set_default_pdn function
  * @retval CS_Status_t
  */
CS_Status_t osCDS_set_default_pdn(CS_PDN_conf_id_t cid)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_set_default_pdn(cid);
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/**
  * @brief  Activates a PDN (Packet Data Network Gateway) allowing communication with internet.
  * @note   This function triggers the allocation of IP public WAN to the device.
  * @note   Only one PDN can be activated at a time.
  * @note   Call CS_activate_pdn with mutex access protection
  * @param  same parameters as the CS_activate_pdn function
  * @retval CS_Status_t
  */
CS_Status_t osCDS_activate_pdn(CS_PDN_conf_id_t cid)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_activate_pdn(cid);
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}


/**
  * @brief  Request to suspend DATA mode.
  * @note   Call CS_suspend_data with mutex access protection
  * @param  none
  * @retval CS_Status_t
  */
CS_Status_t osCDS_suspend_data(void)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_suspend_data();
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/**
  * @brief  Request to resume DATA mode.
  * @note   Call CS_resume_data with mutex access protection
  * @param  none
  * @retval CS_Status_t
  */
CS_Status_t osCDS_resume_data(void)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_resume_data();
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}


/**
  * @brief  DNS request
  * @note   Get IP address of the specified hostname
  * @note   Call CS_dns_request with mutex access protection
  * @param  same parameters as the CS_dns_request function
  * @retval CS_Status_t
  */
CS_Status_t osCDS_dns_request(CS_PDN_conf_id_t cid,
                              CS_DnsReq_t *dns_req,
                              CS_DnsResp_t *dns_resp)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CDS_dns_request(cid, dns_req, dns_resp);
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}


/**
  * @brief  Ping an IP address on the network
  * @note   Usually, the command AT is sent and OK is expected as response
  * @note   Call CDS_ping with mutex access protection
  * @param  same parameters as the CDS_ping function
  * @retval CS_Status_t
  */
CS_Status_t osCDS_ping(CS_PDN_conf_id_t cid,
                       CS_Ping_params_t *ping_params,
                       cellular_ping_response_callback_t cs_ping_rsp_cb)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CDS_ping(cid, ping_params, cs_ping_rsp_cb);
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}


/**
  * @brief  Send a string will which be sended as it is to the modem (termination char will be added automatically)
  * @note   The termination char will be automatically added by the lower layer
  * @note   Call CS_direct_cmd with mutex access protection
  * @param  same parameters as the CS_direct_cmd function
  * @retval CS_Status_t
  */
CS_Status_t osCDS_direct_cmd(CS_direct_cmd_tx_t *direct_cmd_tx,
                             cellular_direct_cmd_callback_t direct_cmd_callback)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result =  CS_direct_cmd(direct_cmd_tx, direct_cmd_callback);
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/**
  * @brief  Get the IP address allocated to the device for a given PDN.
  * @note   Call osCDS_get_dev_IP_address with mutex access protection
  * @param  same parameters as the osCDS_get_dev_IP_address function
  * @retval CS_Status_t
  */
CS_Status_t osCDS_get_dev_IP_address(CS_PDN_conf_id_t cid,
                                     CS_IPaddrType_t *ip_addr_type,
                                     CS_CHAR_t *p_ip_addr_value)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_get_dev_IP_address(cid, ip_addr_type, p_ip_addr_value);
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}


/**
  * @brief  Select SIM slot to use.
  * @note   Only one SIM slot is active at a time.
  *         Call CS_sim_select with mutex access protection
  * @param  same parameters as the CS_sim_select function
  * @retval CS_Status_t
  */
CS_Status_t osCS_sim_select(CS_SimSlot_t simSelected)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_sim_select(simSelected);
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/**
  * @brief  Send a SIM generic command to the modem
  * @note   Call CS_sim_generic_access with mutex access protection
  * @param  sim_generic_access pointer on different buffers:\n
  *         command to send, response received\n
  *         size in bytes for all these buffers
  * @retval int32_t error or size in bytes of the response
  */
int32_t osCS_sim_generic_access(CS_sim_generic_access_t *sim_generic_access)
{
  int32_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_sim_generic_access(sim_generic_access);
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/* =========================================================
   ===========   Low Power Functions BEGIN       ===========
   ========================================================= */
#if (USE_LOW_POWER == 1)

/**
  * @brief  Initialise power configuration. Called systematically first.
  * @note
  * @param  power_config Pointer to the structure describing the power parameters
  * @retval CS_Status_t
  */
CS_Status_t osCS_InitPowerConfig(CS_init_power_config_t *p_power_config,
                                 cellular_power_status_callback_t power_status_callback)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_InitPowerConfig(p_power_config, power_status_callback);
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/**
  * @brief  Request Power WakeUp
  * @note
  * @param  wakeup_origin indicates if wake-up comes from HOST or from MODEM
  * @retval CS_Status_t
  */
CS_Status_t osCS_PowerWakeup(CS_wakeup_origin_t wakeup_origin)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_PowerWakeup(wakeup_origin);
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/**
  * @brief  Cancel sleep procedure
  * @note   if something goes wrong: for example, the URC indicating that modem entered
  *         in Low Power has not been received.
  * @param  none
  * @retval CS_Status_t
  */
CS_Status_t osCS_SleepCancel(void)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_SleepCancel();
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/**
  * @brief  Request sleep procedure
  * @note
  * @param  none
  * @retval CS_Status_t
  */
CS_Status_t osCS_SleepRequest(void)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_SleepRequest();
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/**
  * @brief  Complete sleep procedure
  * @note
  * @param  none
  * @retval CS_Status_t
  */
CS_Status_t osCS_SleepComplete(void)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_SleepComplete();
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/**
  * @brief  Send power configuration (PSM & DRX) to apply to the modem
  * @note
  * @param  power_config Pointer to the structure describing the power parameters
  * @retval CS_Status_t
  */
CS_Status_t osCS_SetPowerConfig(CS_set_power_config_t *p_power_config)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_SetPowerConfig(p_power_config);
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}
/* =========================================================
   ===========   Low Power Functions END         ===========
   ========================================================= */


#endif  /* (USE_LOW_POWER == 1) */

/* =========================================================
   ===========   Com MDM Functions BEGIN       =============
   ========================================================= */
#if defined(USE_COM_MDM)

/**
  * @brief  Register a callback for MDM URC messages from modem
  * @note
  * @param  commdm_urc_cb Pointer to the call backfunction
  * @retval CS_Status_t
  */CS_Status_t osCS_ComMdm_subscribe_event(CS_comMdm_callback_t commdm_urc_cb)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_ComMdm_subscribe_event(commdm_urc_cb);
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/**
  * @brief  Send a command to the modem and wait for the network response
  * @note
  * @param  txBuf Pointer to the structure describing data to transmit
  * @param  rxBuf Pointer to the structure describing received response data
  * @param  errorCode Pointer to an integer representing the error status associated with the response data
  * @retval CS_Status_t
  */
CS_Status_t osCS_ComMdm_transaction(CS_Tx_Buffer_t *txBuf, CS_Rx_Buffer_t *rxBuf, int32_t *errorCode)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_ComMdm_transaction(txBuf, rxBuf, errorCode);
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/**
  * @brief  Send a command to the modem without waiting any response
  * @note
  * @param  txBuf Pointer to the structure describing data to transmit
  * @param  errorCode Pointer to an integer representing the error status
  * @retval CS_Status_t
  */
CS_Status_t osCS_ComMdm_send(CS_Tx_Buffer_t *txBuf, int32_t *errorCode)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_ComMdm_send(txBuf, errorCode);
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/**
  * @brief  Read data previously received by the modem
  * @note
  * @param  rxBuf Pointer to the structure describing received data
  * @param  errorCode Pointer to an integer representing the error status associated with the received data
  * @retval CS_Status_t
  */
CS_Status_t osCS_ComMdm_receive(CS_Rx_Buffer_t *rxBuf, int32_t *errorCode)
{
  CS_Status_t result;

  (void)rtosalMutexAcquire(CellularServiceMutexHandle, RTOSAL_WAIT_FOREVER);
  result = CS_ComMdm_receive(rxBuf, errorCode);
  (void)rtosalMutexRelease(CellularServiceMutexHandle);

  return (result);
}

/* =========================================================
   ===========   Com MDM Functions end       ===============
   ========================================================= */

#endif /* defined(USE_COM_MDM) */


