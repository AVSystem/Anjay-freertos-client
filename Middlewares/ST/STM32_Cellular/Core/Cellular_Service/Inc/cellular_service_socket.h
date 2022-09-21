/**
  ******************************************************************************
  * @file    cellular_service_socket.h
  * @author  MCD Application Team
  * @brief   Header for cellular_service_socket.c
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
#ifndef CELLULAR_SERVICE_SOCKET_H
#define CELLULAR_SERVICE_SOCKET_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include "cellular_service_control.h"
#include "at_core.h"
#include "plf_config.h"

/** @addtogroup CELLULAR_SERVICE_SOCKET CELLULAR_SERVICE_SOCKET
  * @{
  */

/** @addtogroup CELLULAR_SERVICE_SOCKET CELLULAR_SERVICE_SOCKET API
  * @{
  */

/** @defgroup CELLULAR_SERVICE_SOCKET_API_Exported_Macros CELLULAR_SERVICE_SOCKET API Exported Macros
  * @{
  */

#define CELLULAR_MAX_SOCKETS     (6U)
#define CS_INVALID_SOCKET_HANDLE ((socket_handle_t)-1)

/**
  * @}
  */

/** @defgroup CELLULAR_SERVICE_SOCKET_API_Exported_Types CELLULAR_SERVICE_SOCKET API Exported Types
  * @{
  */
typedef int32_t socket_handle_t;

typedef enum
{
  CS_TCP_PROTOCOL = 0,
  CS_UDP_PROTOCOL = 1,
} CS_TransportProtocol_t;

typedef uint16_t CS_ConnectionMode_t; /* uint16_t is used to keep API consistent */
#define CS_CM_COMMAND_MODE              (CS_ConnectionMode_t)(0x0U) /* remains in command line after each
                                                                     * packet transmission (tx or rx) - default */
#define CS_CM_ONLINE_MODE               (CS_ConnectionMode_t)(0x1U) /* NOT SUPPORTED - remains in online mode until
                                                                     * transfer over socket is closed */
#define CS_CM_ONLINE_AUTOMATIC_SUSPEND  (CS_ConnectionMode_t)(0x2U) /* NOT SUPPORTED - remains in online mode until
                                                                     * a suspend timeout expires */

typedef enum
{
  CS_SOL_IP        = 0,
  CS_SOL_TRANSPORT = 1,
} CS_SocketOptionLevel_t;

typedef uint16_t CS_SocketOptionName_t;
#define  CS_SON_NO_OPTION                 (CS_SocketOptionName_t)(0x00)
#define  CS_SON_IP_MAX_PACKET_SIZE        (CS_SocketOptionName_t)(0x01)  /* 0 to 1500 bytes */
#define  CS_SON_TRP_MAX_TIMEOUT           (CS_SocketOptionName_t)(0x02)  /* 0 to 65535, in second, 0 means infinite */
#define  CS_SON_TRP_CONNECT_SETUP_TIMEOUT (CS_SocketOptionName_t)(0x04)  /* 10 to 1200, in 100 of ms, 0 means infnite*/
#define  CS_SON_TRP_TRANSFER_TIMEOUT      (CS_SocketOptionName_t)(0x08)  /* 1 to 255, in ms, 0 means infinite */
#define  CS_SON_TRP_CONNECT_MODE          (CS_SocketOptionName_t)(0x10)  /**/
#define  CS_SON_TRP_SUSPEND_TIMEOUT       (CS_SocketOptionName_t)(0x20)  /* 0 to 2000, in ms , 0 means infinite */
#define  CS_SON_TRP_RX_TIMEOUT            (CS_SocketOptionName_t)(0x40)  /* 0 to 255, in ms, 0 means infinite */

typedef struct
{
  /* CS_SocketState_t    state; */ /* to add ? */
  CS_CHAR_t           loc_ip_addr_value[MAX_SIZE_IPADDR];
  uint16_t            loc_port;
  CS_CHAR_t           rem_ip_addr_value[MAX_SIZE_IPADDR];
  uint16_t            rem_port;
} CS_SocketCnxInfos_t;

typedef struct
{
  CS_CHAR_t           primary_dns_addr[MAX_SIZE_IPADDR];
} CS_DnsConf_t;

typedef struct
{
  CS_CHAR_t           host_name[MAX_SIZE_IPADDR];
} CS_DnsReq_t;

typedef struct
{
  CS_CHAR_t           host_addr[MAX_SIZE_IPADDR];
} CS_DnsResp_t;

typedef struct
{
  CS_CHAR_t  host_addr[MAX_SIZE_IPADDR];
  uint8_t  timeout;
  uint8_t  pingnum;
} CS_Ping_params_t;

typedef struct
{
  uint8_t      index;
  CS_Status_t  ping_status;
  CS_Bool_t    is_final_report;

  /* Intermediate ping response parameters (if is_final_report = false) */
  CS_CHAR_t  ping_addr[MAX_SIZE_IPADDR];
  uint16_t   ping_size; /* size in bytes */
  uint32_t   time;
  uint8_t    ttl;

  /* Final ping response parameters (if is_final_report = true)
    *  no parameters reported in this case
    */

} CS_Ping_response_t;

/* callbacks */
typedef void (* cellular_socket_data_ready_callback_t)(socket_handle_t sockHandle);
typedef void (* cellular_socket_data_sent_callback_t)(socket_handle_t sockHandle);
typedef void (* cellular_socket_closed_callback_t)(socket_handle_t sockHandle);
typedef void (* cellular_ping_response_callback_t)(CS_Ping_response_t ping_response);

/**
  * @}
  */

/** @defgroup CELLULAR_SERVICE_SOCKET_API_Exported_Functions CELLULAR_SERVICE_SOCKET API Exported Functions
  * @{
  */

/* SOCKET API */
socket_handle_t CDS_socket_create(CS_IPaddrType_t addr_type,
                                  CS_TransportProtocol_t protocol,
                                  CS_PDN_conf_id_t cid);
CS_Status_t CDS_socket_bind(socket_handle_t sockHandle,
                            uint16_t local_port);
CS_Status_t CDS_socket_set_callbacks(socket_handle_t sockHandle,
                                     cellular_socket_data_ready_callback_t data_ready_cb,
                                     cellular_socket_data_sent_callback_t data_sent_cb,
                                     cellular_socket_closed_callback_t remote_close_cb);
CS_Status_t CDS_socket_set_option(socket_handle_t sockHandle,
                                  CS_SocketOptionLevel_t opt_level,
                                  CS_SocketOptionName_t opt_name,
                                  void *p_opt_val);
CS_Status_t CDS_socket_connect(socket_handle_t sockHandle,
                               CS_IPaddrType_t ip_addr_type,
                               CS_CHAR_t *p_ip_addr_value,
                               uint16_t remote_port); /* for socket client mode */
CS_Status_t CDS_socket_send(socket_handle_t sockHandle,
                            const CS_CHAR_t *p_buf,
                            uint32_t length);
CS_Status_t CDS_socket_sendto(socket_handle_t sockHandle,
                              const CS_CHAR_t *p_buf,
                              uint32_t length,
                              CS_IPaddrType_t ip_addr_type,
                              CS_CHAR_t *p_ip_addr_value,
                              uint16_t remote_port);
int32_t CDS_socket_receive(socket_handle_t sockHandle,
                           CS_CHAR_t *p_buf,
                           uint32_t max_buf_length);
int32_t CDS_socket_receivefrom(socket_handle_t sockHandle,
                               CS_CHAR_t *p_buf,
                               uint32_t max_buf_length,
                               CS_IPaddrType_t *p_ip_addr_type,
                               CS_CHAR_t *p_ip_addr_value,
                               uint16_t *p_remote_port);
CS_Status_t CDS_socket_close(socket_handle_t sockHandle,
                             uint8_t force);
CS_Status_t CDS_socket_cnx_status(socket_handle_t sockHandle,
                                  CS_SocketCnxInfos_t *p_infos);
CS_Status_t CDS_ping(CS_PDN_conf_id_t cid, CS_Ping_params_t *ping_params,
                     cellular_ping_response_callback_t cs_ping_rsp_cb);
CS_Status_t CDS_dns_config(CS_PDN_conf_id_t cid, CS_DnsConf_t *dns_conf); /* not implemented yet  */
CS_Status_t CDS_dns_request(CS_PDN_conf_id_t cid, CS_DnsReq_t *dns_req, CS_DnsResp_t *dns_resp);

CS_Status_t CDS_socket_get_option(void); /* not implemented yet  */
CS_Status_t CDS_socket_listen(socket_handle_t sockHandle); /* not implemented yet,
                                                            * for socket server mode, parameters to clarify */

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

#endif /* CELLULAR_SERVICE_SOCKET_H */

