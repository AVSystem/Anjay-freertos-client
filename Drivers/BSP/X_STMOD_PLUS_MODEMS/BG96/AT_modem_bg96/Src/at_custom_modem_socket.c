/**
  ******************************************************************************
  * @file    at_custom_modem_socket.c
  * @author  MCD Application Team
  * @brief   This file provides all the 'socket mode' code to the
  *          BG96 Quectel modem: LTE-cat-M1 or LTE-cat.NB1(=NB-IOT) or GSM
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

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "at_modem_api.h"
#include "at_modem_common.h"
#include "at_modem_signalling.h"
#include "at_modem_socket.h"
#include "at_custom_modem_socket.h"
#include "at_custom_modem_specific.h"
#include "at_datapack.h"
#include "at_util.h"
#include "cellular_runtime_standard.h"
#include "cellular_runtime_custom.h"
#include "plf_config.h"
#include "plf_modem_config.h"
#include "error_handler.h"

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

/** @defgroup AT_CUSTOM_QUECTEL_BG96_SOCKET_Private_Macros AT_CUSTOM QUECTEL_BG96 SOCKET Private Macros
  * @{
  */
#if (USE_TRACE_ATCUSTOM_SPECIFIC == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_INFO(format, args...) TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P0, "BG96:" format "\n\r", ## args)
#define PRINT_DBG(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P1, "BG96:" format "\n\r", ## args)
#define PRINT_API(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P2, "BG96 API:" format "\n\r", ## args)
#define PRINT_ERR(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_ERR, "BG96 ERROR:" format "\n\r", ## args)
#define PRINT_BUF(pbuf, size)       TRACE_PRINT_BUF_CHAR(DBG_CHAN_ATCMD, DBL_LVL_P1, (const CRC_CHAR_t *)pbuf, size);
#else
#define PRINT_INFO(format, args...)  (void) printf("BG96:" format "\n\r", ## args);
#define PRINT_DBG(...)  __NOP(); /* Nothing to do */
#define PRINT_API(...)  __NOP(); /* Nothing to do */
#define PRINT_ERR(format, args...)   (void) printf("BG96 ERROR:" format "\n\r", ## args);
#define PRINT_BUF(...)  __NOP(); /* Nothing to do */
#endif /* USE_PRINTF */
#else
#define PRINT_INFO(...)  __NOP(); /* Nothing to do */
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_API(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(...)   __NOP(); /* Nothing to do */
#define PRINT_BUF(...)   __NOP(); /* Nothing to do */
#endif /* USE_TRACE_ATCUSTOM_SPECIFIC */

/* START_PARAM_LOOP and END_PARAM_LOOP macros are used to loop on all fields
*  received in a message.
*  Only non-null length fields are analysed.
*  End the analyze when the end of the message or an error has been detected.
*/
#define START_PARAM_LOOP()  uint8_t exitcode = 0U;\
  do {\
    if (atcc_extractElement(p_at_ctxt, p_msg_in, element_infos) != ATENDMSG_NO) {exitcode = 1U;}\
    if (element_infos->str_size != 0U)\
    {\

#define END_PARAM_LOOP()  }\
  if (retval == ATACTION_RSP_ERROR) {exitcode = 1U;}\
  } while (exitcode == 0U);

#define APN_EMPTY_STRING ""

/**
  * @}
  */

/** @defgroup AT_CUSTOM_QUECTEL_BG96_SOCKET_Private_Functions_Prototypes
  *    AT_CUSTOM QUECTEL_BG96 SOCKET Private Functions Prototypes
  * @{
  */
static void clear_ping_resp_struct(atcustom_modem_context_t *p_modem_ctxt);
/**
  * @}
  */

/** @defgroup AT_CUSTOM_QUECTEL_BG96_SOCKET_Exported_Functions AT_CUSTOM QUECTEL_BG96 SOCKET Exported Functions
  * @{
  */

/* Build command functions ------------------------------------------------------- */

/**
  * @brief  Build specific modem command : QIACT.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_QIACT_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_QIACT_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
    uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
    PRINT_INFO("user cid = %d, modem cid = %d", (uint8_t)current_conf_id, modem_cid)
    /* check if this PDP context has been defined */
    if (p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].conf_id == CS_PDN_NOT_DEFINED)
    {
      PRINT_INFO("PDP context not explicitly defined for conf_id %d (using modem params)", current_conf_id)
    }

    (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d",  modem_cid);
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : QIOPEN.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_QIOPEN_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  /* Array to convert AT+QIOPEN service type parameter to a string
  *  need to be aligned with atsocket_servicetype_t
  */
  static const AT_CHAR_t *const bg96_array_QIOPEN_service_type[] =
  {
    ((uint8_t *)"TCP"),             /* start TCP connection as client */
    ((uint8_t *)"UDP"),             /* start UDP connection as client */
    ((uint8_t *)"TCP LISTENER"),    /* start TCP server to listen TCP connection */
    ((uint8_t *)"UDP SERVICE")      /* start UDP service: UDP not connected mode */
  };

  at_status_t retval = ATSTATUS_OK;
  atsocket_servicetype_t service_type_index;

  PRINT_API("enter fCmdBuild_QIOPEN_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (p_modem_ctxt->socket_ctxt.p_socket_info != NULL)
    {
      /* IP AT Commands manual - LTE Module Series - V1.0
      * AT+QIOPEN=<contextID>,<connectId>,<service_type>,<IP_address>/<domain_name>,<remote_port>[,
      *           <local_port>[,<access_mode>]]
      *
      * <contextID> is the PDP context ID
      */
      /* convert user cid (CS_PDN_conf_id_t) to PDP modem cid (value) */
      uint8_t pdp_modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist,
                                                          p_modem_ctxt->socket_ctxt.p_socket_info->conf_id);
      PRINT_DBG("user cid = %d, PDP modem cid = %d",
                (uint8_t)p_modem_ctxt->socket_ctxt.p_socket_info->conf_id, pdp_modem_cid)
      uint8_t access_mode = 0U; /* 0=buffer access mode, 1=direct push mode, 2=transparent access mode */

      if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_DIAL_COMMAND)
      {
        /* Server mode or Client mode ? */
        if (strcmp((CRC_CHAR_t const *)p_modem_ctxt->socket_ctxt.p_socket_info->ip_addr_value,
                   (CRC_CHAR_t const *)CONFIG_MODEM_UDP_SERVICE_CONNECT_IP) == 0)
        {
          /* Server mode - "TCP LISTENER" or "UDP SERVICE" ? */
          service_type_index = ((p_modem_ctxt->socket_ctxt.p_socket_info->protocol) == CS_TCP_PROTOCOL) ? \
                               ATSOCKET_SERVICETYPE_TCP_SERVER : ATSOCKET_SERVICETYPE_UDP_SERVICE;
        }
        else
        {
          /* Client mode - "TCP" or "UDP" ? */
          service_type_index = ((p_modem_ctxt->socket_ctxt.p_socket_info->protocol) == CS_TCP_PROTOCOL) ? \
                               ATSOCKET_SERVICETYPE_TCP_CLIENT : ATSOCKET_SERVICETYPE_UDP_CLIENT;
        }

        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,%ld,\"%s\",\"%s\",%d,%d,%d",
                       pdp_modem_cid,
                       atcm_socket_get_modem_cid(p_modem_ctxt, p_modem_ctxt->socket_ctxt.p_socket_info->socket_handle),
                       bg96_array_QIOPEN_service_type[service_type_index],
                       p_modem_ctxt->socket_ctxt.p_socket_info->ip_addr_value,
                       p_modem_ctxt->socket_ctxt.p_socket_info->remote_port,
                       p_modem_ctxt->socket_ctxt.p_socket_info->local_port,
                       access_mode);

        /* waiting for +QIOPEN now */
        bg96_shared.QIOPEN_waiting = AT_TRUE;
      }
      /* QIOPEN for server mode (not supported yet)
      *  else if (curSID == ?) for server mode (corresponding to CDS_socket_listen)
      */
      else
      {
        /* error */
        retval = ATSTATUS_ERROR;
      }
    }
    else
    {
      retval = ATSTATUS_ERROR;
    }
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : QICLOSE.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_QICLOSE_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_QICLOSE_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (p_modem_ctxt->socket_ctxt.p_socket_info != NULL)
    {
      /* IP AT Commands manual - LTE Module Series - V1.0
      * AT+QICLOSE=connectId>[,<timeout>]
      */
      uint32_t connID = atcm_socket_get_modem_cid(p_modem_ctxt, p_modem_ctxt->socket_ctxt.p_socket_info->socket_handle);
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%ld", connID);
    }
    else
    {
      retval = ATSTATUS_ERROR;
    }
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : QISEND.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_QISEND_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_QISEND_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /*IP AT Commands manual - LTE Module Series - V1.0
    * send data in command mode:
    * AT+QISEND=<connId>,<send_length><CR>
    * > ...DATA...
    *
    * DATA are sent using fCmdBuild_QISEND_WRITE_DATA_BG96()
    */
    if (p_modem_ctxt->SID_ctxt.socketSendData_struct.ip_addr_type == CS_IPAT_INVALID)
    {
      /* QISEND format for "TCP", "UDP" or "TCP INCOMING" :
       *   AT+QISEND=<connectID>,<send_length>
       */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%ld,%ld",
                     atcm_socket_get_modem_cid(p_modem_ctxt,
                                               p_modem_ctxt->SID_ctxt.socketSendData_struct.socket_handle),
                     p_modem_ctxt->SID_ctxt.socketSendData_struct.buffer_size
                    );
    }
    else
    {
      /* QISEND format for "UDP SERVICE"
       *   AT+QISEND=<connectID>,<send_length>,<remoteIP>,<remote_port>
       */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%ld,%ld,\"%s\",%d",
                     atcm_socket_get_modem_cid(p_modem_ctxt,
                                               p_modem_ctxt->SID_ctxt.socketSendData_struct.socket_handle),
                     p_modem_ctxt->SID_ctxt.socketSendData_struct.buffer_size,
                     p_modem_ctxt->SID_ctxt.socketSendData_struct.ip_addr_value,
                     p_modem_ctxt->SID_ctxt.socketSendData_struct.remote_port
                    );
    }
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : QISEND (write data).
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_QISEND_WRITE_DATA_BG96(atparser_context_t *p_atp_ctxt,
                                             atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_QISEND_WRITE_DATA_BG96()")

  /* after having send AT+QISEND and prompt received, now send DATA */

  /* only for raw command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_RAW_CMD)
  {
    if (p_modem_ctxt->SID_ctxt.socketSendData_struct.p_buffer_addr_send != NULL)
    {
      uint32_t str_size = p_modem_ctxt->SID_ctxt.socketSendData_struct.buffer_size;
      (void) memcpy((void *)p_atp_ctxt->current_atcmd.params,
                    (const CS_CHAR_t *)p_modem_ctxt->SID_ctxt.socketSendData_struct.p_buffer_addr_send,
                    (size_t) str_size);

      /* set raw command size */
      p_atp_ctxt->current_atcmd.raw_cmd_size = str_size;
    }
    else
    {
      PRINT_ERR("ERROR, send buffer is a NULL ptr !!!")
      retval = ATSTATUS_ERROR;
    }
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : QIRD.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_QIRD_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_QIRD_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (p_modem_ctxt->socket_ctxt.socket_receive_state == SocketRcvState_RequestSize)
    {
      /* requesting socket data size (set length = 0) */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%ld,0",
                     atcm_socket_get_modem_cid(p_modem_ctxt,
                                               p_modem_ctxt->socket_ctxt.socketReceivedata.socket_handle));
    }
    else if (p_modem_ctxt->socket_ctxt.socket_receive_state == SocketRcvState_RequestData_Header)
    {
      uint32_t requested_data_size;
      /* the value socket_rx_expected_buf_size used to fill this field has been already
       * checked in fRspAnalyze_QIRD_BG96()
       */
      requested_data_size = p_modem_ctxt->socket_ctxt.socket_rx_expected_buf_size;

      /* requesting socket data with correct size */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%ld,%ld",
                     atcm_socket_get_modem_cid(p_modem_ctxt,
                                               p_modem_ctxt->socket_ctxt.socketReceivedata.socket_handle),
                     requested_data_size);

      /* ready to start receive socket buffer */
      p_modem_ctxt->socket_ctxt.socket_RxData_state = SocketRxDataState_waiting_header;
    }
    else
    {
      PRINT_ERR("Unexpected socket receiving state")
      retval = ATSTATUS_ERROR;
    }
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : QISTATE.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_QISTATE_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_QISTATE_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* AT+QISTATE=<query_type>,<connectID>
    *
    * <query_type> = 1
    */
    if (p_modem_ctxt->socket_ctxt.p_socket_cnx_infos != NULL)
    {
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "1,%ld",
                     atcm_socket_get_modem_cid(p_modem_ctxt,
                                               p_modem_ctxt->socket_ctxt.p_socket_cnx_infos->socket_handle));
    }
    else
    {
      PRINT_ERR("No socket cnx infos context")
      retval = ATSTATUS_ERROR;
    }
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : QIDNSCFG.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_QIDNSCFG_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_QIDNSCFG_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* cf TCP/IP AT commands manual v1.0
    *  1- Configure DNS server address for specified PDP context
    *  *  AT+QIDNSCFG=<contextID>,<pridnsaddr>[,<secdnsaddr>]
    *
    *  2- Query DNS server address of specified PDP context
    *  AT+QIDNSCFG=<contextID>
    *  response:
    *  +QIDNSCFG: <contextID>,<pridnsaddr>,<secdnsaddr>
    *
    */
    if (p_modem_ctxt->SID_ctxt.p_dns_request_infos != NULL)
    {
      CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
      uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
      /* configure DNS server address for the specified PDP context */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,\"%s\"",
                     modem_cid,
                     p_modem_ctxt->SID_ctxt.p_dns_request_infos->dns_conf.primary_dns_addr);
    }
    else
    {
      retval = ATSTATUS_ERROR;
    }
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : QIDNSGIP.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_QIDNSGIP_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_QIDNSGIP_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* cf TCP/IP AT commands manual v1.0
    * AT+QIDNSGIP=<contextID>,<hostname>
    */
    if (p_modem_ctxt->SID_ctxt.p_dns_request_infos  != NULL)
    {
      CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
      uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,\"%s\"",
                     modem_cid,
                     p_modem_ctxt->SID_ctxt.p_dns_request_infos->dns_req.host_name);
    }
    else
    {
      retval = ATSTATUS_ERROR;
    }
  }
  return (retval);
}

/**
  * @brief  Build specific modem command : QPING.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_QPING_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_QPING_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* cf TCP/IP AT commands manual v1.0
    *  Ping a remote server:
    *  AT+QPING=<contextID>,<host<[,<timeout>[,<pingnum>]]
    */
    CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
    uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
    (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,\"%s\",%d,%d",
                   modem_cid,
                   p_modem_ctxt->SID_ctxt.ping_infos.ping_params.host_addr,
                   p_modem_ctxt->SID_ctxt.ping_infos.ping_params.timeout,
                   p_modem_ctxt->SID_ctxt.ping_infos.ping_params.pingnum);
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : QICSGP.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_QICSGP_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_QICSGP_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* cf BG96 TCP/IP AT commands manual v1.0
    *  AT+QICSGP: Configure parameters of a TCP/IP context
    *  AT+QICSGP=<cid>[,<context_type>,<APN>[,<username>,<password>)[,<authentication>]]]
    *  - cid: context id (rang 1 to 16)
    *  - context_type: 1 for IPV4, 2 for IPV6
    *  - APN: string for access point name
    *  - username: string
    *  - password: string
    *  - authentication: 0 for NONE, 1 for PAP, 2 for CHAP, 3 for PAP or CHAP
    *
    * example: AT+QICSGP=1,1,"UNINET","","",1
    */

    if (bg96_shared.QICGSP_config_command == AT_TRUE)
    {
      CS_CHAR_t *p_apn;
      /* Write command is a config command */
      CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
      uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
      PRINT_INFO("user cid = %d, modem cid = %d", (uint8_t)current_conf_id, modem_cid)

      uint8_t context_type_value;
      uint8_t authentication_value;

      /* convert context type to numeric value */
      switch (p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].pdn_conf.pdp_type)
      {
        case CS_PDPTYPE_IP:
          context_type_value = 1U;
          break;
        case CS_PDPTYPE_IPV6:
        case CS_PDPTYPE_IPV4V6:
          context_type_value = 2U;
          break;

        default :
          context_type_value = 1U;
          break;
      }

      /*  authentication : 0,1,2 or 3 - cf modem AT cmd manuel - Use 0 for None */
      if (p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].pdn_conf.username[0] == 0U)
      {
        /* no username => no authentication */
        authentication_value = 0U;
      }
      else
      {
        /* username => PAP or CHAP authentication type */
        authentication_value = 3U;
      }



      /* build command */
      if (p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].apn_present == CELLULAR_TRUE)
      {
        /* use the APN explicitly providedby user */
        p_apn = (CS_CHAR_t *) &p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].apn;
      }
      else
      {
        /* no APN provided by user: use empty APN string (BG96 specific) to force the network to
        *  select the appropriate APN.
        */
        p_apn = (CS_CHAR_t *) &APN_EMPTY_STRING;
      }

      /* use the APN value given by user */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,%d,\"%s\",\"%s\",\"%s\",%d",
                     modem_cid,
                     context_type_value,
                     p_apn,
                     p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].pdn_conf.username,
                     p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].pdn_conf.password,
                     authentication_value
                    );
    }
    else
    {
      /* Write command is a query command */
      CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
      uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
      PRINT_INFO("user cid = %d, modem cid = %d", (uint8_t)current_conf_id, modem_cid)
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d", modem_cid);
    }

  }

  return (retval);
}

/* Analyze command functions ------------------------------------------------------- */
/**
  * @brief  Analyze specific modem response : QIURC.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_QIURC_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                       const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_QIURC_BG96()")

  /* memorize which is current QIURC received */
  ATCustom_BG96_QIURC_function_t bg96_current_qiurc_ind = QIURC_UNKNOWN;

  /*IP AT Commands manual - LTE Module Series - V1.0
  * URC
  * +QIURC:"closed",<connectID> : URC of connection closed
  * +QIURC:"recv",<connectID> : URC of incoming data
  * +QIURC:"incoming full" : URC of incoming connection full
  * +QIURC:"incoming",<connectID> ,<serverID>,<remoteIP>,<remote_port> : URC of incoming connection
  * +QIURC:"pdpdeact",<contextID> : URC of PDP deactivation
  *
  * for DNS request:
  * header: +QIURC: "dnsgip",<err>,<IP_count>,<DNS_ttl>
  * infos:  +QIURC: "dnsgip",<hostIPaddr>]
  */

  /* this is an URC */
  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    AT_CHAR_t line[32] = {0};

    /* init param received info */
    bg96_current_qiurc_ind = QIURC_UNKNOWN;

    PRINT_BUF((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    /* copy element to line for parsing */
    if (element_infos->str_size <= 32U)
    {
      (void) memcpy((void *)&line[0],
                    (const void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
                    (size_t) element_infos->str_size);

      /* extract value and compare it to expected value */
      if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "closed") != NULL)
      {
        PRINT_DBG("+QIURC closed info received")
        bg96_current_qiurc_ind = QIURC_CLOSED;
      }
      else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "recv") != NULL)
      {
        PRINT_DBG("+QIURC recv info received")
        bg96_current_qiurc_ind = QIURC_RECV;
      }
      else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "incoming full") != NULL)
      {
        PRINT_DBG("+QIURC incoming full info received")
        bg96_current_qiurc_ind = QIURC_INCOMING_FULL;
      }
      else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "incoming") != NULL)
      {
        PRINT_DBG("+QIURC incoming info received")
        bg96_current_qiurc_ind = QIURC_INCOMING;
      }
      else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "pdpdeact") != NULL)
      {
        PRINT_DBG("+QIURC pdpdeact info received")
        bg96_current_qiurc_ind = QIURC_PDPDEACT;
      }
      else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "dnsgip") != NULL)
      {
        PRINT_DBG("+QIURC dnsgip info received")
        bg96_current_qiurc_ind = QIURC_DNSGIP;
        if (p_atp_ctxt->current_SID != (at_msg_t) SID_CS_DNS_REQ)
        {
          /* URC not expected */
          retval = ATACTION_RSP_URC_IGNORED;
        }
      }
      else
      {
        PRINT_ERR("+QIURC field not managed")
      }
    }
    else
    {
      PRINT_ERR("param ignored (exceed maximum size)")
      retval = ATACTION_RSP_IGNORED;
    }
  }
  else if (element_infos->param_rank == 3U)
  {
    uint32_t connectID;
    socket_handle_t sockHandle;

    switch (bg96_current_qiurc_ind)
    {
      case QIURC_RECV:
        /* <connectID> */
        connectID = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        sockHandle = atcm_socket_get_socket_handle(p_modem_ctxt, connectID);
        (void) atcm_socket_set_urc_data_pending(p_modem_ctxt, sockHandle);
        PRINT_DBG("+QIURC received data for connId=%ld (socket handle=%ld)", connectID, sockHandle)
        /* last param */
        retval = ATACTION_RSP_URC_FORWARDED;
        break;

      case QIURC_CLOSED:
        /* <connectID> */
        connectID = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        sockHandle = atcm_socket_get_socket_handle(p_modem_ctxt, connectID);
        (void) atcm_socket_set_urc_closed_by_remote(p_modem_ctxt, sockHandle);
        PRINT_DBG("+QIURC closed for connId=%ld (socket handle=%ld)", connectID, sockHandle)
        /* last param */
        retval = ATACTION_RSP_URC_FORWARDED;
        break;

      case QIURC_INCOMING:
        /* <connectID> */
        PRINT_DBG("+QIURC incoming for connId=%ld",
                  ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
        break;

      case QIURC_PDPDEACT:
#if 0
        /* we do not process this event as it is also reported by modem when it enters in PSM.
         * So we do support only +CGEV to detect PDP deactivation
         */

        /* <contextID> */
        uint32_t contextID = ATutil_convertStringToInt(
                               &p_msg_in->buffer[element_infos->str_start_idx],
                               element_infos->str_size);
        PRINT_DBG("+QIURC pdpdeact for contextID=%ld", contextID)
        /* Need to inform  upper layer if pdn event URC has been subscribed
         * apply same treatment than CGEV NW PDN DEACT
        */
        p_modem_ctxt->persist.pdn_event.conf_id = atcm_get_configID_for_modem_cid(&p_modem_ctxt->persist,
                                                                                  (uint8_t)contextID);
        /* Indicate that an equivalent to +CGEV URC has been received */
        p_modem_ctxt->persist.pdn_event.event_origine = CGEV_EVENT_ORIGINE_NW;
        p_modem_ctxt->persist.pdn_event.event_scope = CGEV_EVENT_SCOPE_PDN;
        p_modem_ctxt->persist.pdn_event.event_type = CGEV_EVENT_TYPE_DEACTIVATION;
        p_modem_ctxt->persist.urc_avail_pdn_event = AT_TRUE;
        /* last param */
        retval = ATACTION_RSP_URC_FORWARDED;
#else
        retval = ATACTION_RSP_IGNORED;
#endif  /* 0 */
        break;

      case QIURC_DNSGIP:
        /* <err> or <hostIPaddr>]> */
        if (bg96_shared.QIURC_dnsgip_param.wait_header == AT_TRUE)
        {
          /* <err> expected */
          bg96_shared.QIURC_dnsgip_param.error =
            ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
          PRINT_DBG("+QIURC dnsgip with error=%ld", bg96_shared.QIURC_dnsgip_param.error)
          if (bg96_shared.QIURC_dnsgip_param.error != 0U)
          {
            /* Error when trying to get host address */
            bg96_shared.QIURC_dnsgip_param.finished = AT_TRUE;
            retval = ATACTION_RSP_ERROR;
          }
        }
        else
        {
          /* <hostIPaddr> expected
          *  with the current implementation, in case of many possible host IP address, we use
          *  the last one received
          */
          (void) memcpy((void *)bg96_shared.QIURC_dnsgip_param.hostIPaddr,
                        (const void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
                        (size_t) element_infos->str_size);
          PRINT_DBG("+QIURC dnsgip Host address #%ld =%s", bg96_shared.QIURC_dnsgip_param.ip_count,
                    bg96_shared.QIURC_dnsgip_param.hostIPaddr)
          bg96_shared.QIURC_dnsgip_param.ip_count--;
          if (bg96_shared.QIURC_dnsgip_param.ip_count == 0U)
          {
            /* all expected URC have been reecived */
            bg96_shared.QIURC_dnsgip_param.finished = AT_TRUE;
            /* last param */
            retval = ATACTION_RSP_FRC_END;
          }
          else
          {
            retval = ATACTION_RSP_IGNORED;
          }
        }
        break;

      case QIURC_INCOMING_FULL:
      default:
        /* no parameter expected */
        PRINT_ERR("parameter not expected for this URC message")
        break;
    }
  }
  else if (element_infos->param_rank == 4U)
  {
    switch (bg96_current_qiurc_ind)
    {
      case QIURC_INCOMING:
        /* <serverID> */
        PRINT_DBG("+QIURC incoming for serverID=%ld",
                  ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
        break;

      case QIURC_DNSGIP:
        /* <IP_count> */
        if (bg96_shared.QIURC_dnsgip_param.wait_header == AT_TRUE)
        {
          /* <QIURC_dnsgip_param.ip_count> expected */
          bg96_shared.QIURC_dnsgip_param.ip_count =
            ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
          PRINT_DBG("+QIURC dnsgip IP count=%ld", bg96_shared.QIURC_dnsgip_param.ip_count)
          if (bg96_shared.QIURC_dnsgip_param.ip_count == 0U)
          {
            /* No host address available */
            bg96_shared.QIURC_dnsgip_param.finished = AT_TRUE;
            retval = ATACTION_RSP_ERROR;
          }
        }
        break;

      case QIURC_RECV:
      case QIURC_CLOSED:
      case QIURC_PDPDEACT:
      case QIURC_INCOMING_FULL:
      default:
        /* no parameter expected */
        PRINT_ERR("parameter not expected for this URC message")
        break;
    }
  }
  else if (element_infos->param_rank == 5U)
  {
    AT_CHAR_t remoteIP[32] = {0};

    switch (bg96_current_qiurc_ind)
    {
      case QIURC_INCOMING:
        /* <remoteIP> */
        (void) memcpy((void *)&remoteIP[0],
                      (const void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
                      (size_t) element_infos->str_size);
        PRINT_DBG("+QIURC remoteIP for remoteIP=%s", remoteIP)
        break;

      case QIURC_DNSGIP:
        /* <DNS_ttl> */
        if (bg96_shared.QIURC_dnsgip_param.wait_header == AT_TRUE)
        {
          /* <QIURC_dnsgip_param.ttl> expected */
          bg96_shared.QIURC_dnsgip_param.ttl =
            ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
          PRINT_DBG("+QIURC dnsgip time-to-live=%ld", bg96_shared.QIURC_dnsgip_param.ttl)
          /* no error, now waiting for URC with IP address */
          bg96_shared.QIURC_dnsgip_param.wait_header = AT_FALSE;
        }
        break;

      case QIURC_RECV:
      case QIURC_CLOSED:
      case QIURC_PDPDEACT:
      case QIURC_INCOMING_FULL:
      default:
        /* no parameter expected */
        PRINT_ERR("parameter not expected for this URC message")
        break;
    }
  }
  else if (element_infos->param_rank == 6U)
  {
    switch (bg96_current_qiurc_ind)
    {
      case QIURC_INCOMING:
        /* <remote_port> */
        PRINT_DBG("+QIURC incoming for remote_port=%ld",
                  ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
        /* last param */
        retval = ATACTION_RSP_URC_FORWARDED;
        break;

      case QIURC_RECV:
      case QIURC_CLOSED:
      case QIURC_PDPDEACT:
      case QIURC_INCOMING_FULL:
      case QIURC_DNSGIP:
      default:
        /* no parameter expected */
        PRINT_ERR("parameter not expected for this URC message")
        break;
    }
  }
  else
  {
    /* parameter ignored */
    __NOP(); /* to avoid warning */
  }
  END_PARAM_LOOP()

  return (retval);
}

/**
  * @brief  Analyze specific modem response : QIACT.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_QIACT_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                       const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)

{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_QIACT_BG96()")

  if (p_atp_ctxt->current_atcmd.type == ATTYPE_READ_CMD)
  {
    /* Answer to AT+QIACT?
    *  Returns the list of current activated contexts and their IP addresses
    *  format: +QIACT: <cid>,<context_state>,<context_type>,[,<IP_address>]
    *
    *  where:
    *  <cid>: context ID, range is 1 to 16
    *  <context_state>: 0 for deactivated, 1 for activated
    *  <context_type>: 1 for IPV4, 2 for IPV6
    *  <IP_address>: string type
    */

    START_PARAM_LOOP()
    if (element_infos->param_rank == 2U)
    {
      /* analyze <cid> */
      uint32_t modem_cid = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                     element_infos->str_size);
      PRINT_DBG("+QIACT cid=%ld", modem_cid)
      p_modem_ctxt->CMD_ctxt.modem_cid = modem_cid;
    }
    else if (element_infos->param_rank == 3U)
    {
      /* analyze <context_state> */
      uint32_t context_state = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                         element_infos->str_size);
      PRINT_DBG("+QIACT context_state=%ld", context_state)

      /* If we are trying to activate a PDN and we see that it is already active, do not
       *  request the activation to avoid an error */
      if ((p_atp_ctxt->current_SID == (at_msg_t) SID_CS_ACTIVATE_PDN) &&
          (context_state == 1U))
      {
        /* is it the required PDN to activate ? */
        CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
        uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
        if (p_modem_ctxt->CMD_ctxt.modem_cid == modem_cid)
        {
          PRINT_DBG("Modem CID to activate (%d) is already activated", modem_cid)
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
          bg96_shared.pdn_already_active = AT_TRUE;
#endif /* USE_SOCKETS_TYPE */
        }
      }
    }
    else if (element_infos->param_rank == 4U)
    {
      /* analyze <context_type> */
      PRINT_DBG("+QIACT context_type=%ld",
                ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    else if (element_infos->param_rank == 5U)
    {
      /* analyze <IP_address> */
      csint_ip_addr_info_t  ip_addr_info;
      (void) memset((void *)&ip_addr_info, 0, sizeof(csint_ip_addr_info_t));

      /* retrieve IP address value */
      (void) memcpy((void *) & (ip_addr_info.ip_addr_value),
                    (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                    (size_t) element_infos->str_size);
      PRINT_DBG("+QIACT addr=%s", (AT_CHAR_t *)&ip_addr_info.ip_addr_value)

      /* determine IP address type */
      ip_addr_info.ip_addr_type = atcm_get_ip_address_type((AT_CHAR_t *)&ip_addr_info.ip_addr_value);

      /* save IP address infos in modem_cid_table */
      atcm_put_IP_address_infos(&p_modem_ctxt->persist, (uint8_t)p_modem_ctxt->CMD_ctxt.modem_cid, &ip_addr_info);
    }
    else
    {
      /* other parameters ignored */
      __NOP(); /* to avoid warning */
    }
    END_PARAM_LOOP()
  }

  return (retval);
}

/**
  * @brief  Analyze specific modem response : QIOPEN.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_QIOPEN_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                        const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_QIOPEN_BG96()")
  uint32_t bg96_current_qiopen_connectId = 0U;

  /* are we waiting for QIOPEN ? */
  if (bg96_shared.QIOPEN_waiting == AT_TRUE)
  {
    START_PARAM_LOOP()
    if (element_infos->param_rank == 2U)
    {
      uint32_t connectID;
      connectID = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      bg96_current_qiopen_connectId = connectID;
      bg96_shared.QIOPEN_current_socket_connected = 0U;
    }
    else if (element_infos->param_rank == 3U)
    {
      uint32_t err_value;
      err_value = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);

      /* compare QIOPEN connectID with the value requested by user (ie in current SID)
      *  and check if err=0
      */
      if (p_modem_ctxt->socket_ctxt.p_socket_info != NULL)
      {
        if ((bg96_current_qiopen_connectId ==
             atcm_socket_get_modem_cid(p_modem_ctxt, p_modem_ctxt->socket_ctxt.p_socket_info->socket_handle)) &&
            (err_value == 0U))
        {
          PRINT_INFO("socket (connectId=%ld) opened", bg96_current_qiopen_connectId)
          bg96_shared.QIOPEN_current_socket_connected = 1U;
          bg96_shared.QIOPEN_waiting = AT_FALSE;
          retval = ATACTION_RSP_FRC_END;
        }
        else
        {
          if (err_value != 0U)
          {
            PRINT_ERR("+QIOPEN returned error #%ld", err_value)
          }
          else
          {
            PRINT_ERR("+QIOPEN problem")
          }
          retval = ATACTION_RSP_ERROR;
        }
      }
      else
      {
        PRINT_ERR("No socket info context")
        retval = ATACTION_RSP_ERROR;
      }
    }
    else
    {
      /* other parameters ignored */
      __NOP(); /* to avoid warning */
    }
    END_PARAM_LOOP()
  }
  else
  {
    PRINT_INFO("+QIOPEN not expected, ignore it")
  }

  return (retval);
}

/**
  * @brief  Analyze specific modem response : QIRD.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_QIRD_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                      const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_QIRD_BG96()")

  /*IP AT Commands manual - LTE Module Series - V1.0
  *
  * Received after having send AT+QIRD=<connectID>[,<read_length>]
  *
  * if <read_length> was present and equal to 0, it was a request to get status:
  * +QIRD:<total_receive_length>,<have_read_length>,<unread_length>
  *
  * if <read_length> was absent or != 0
  * +QIRD:<read_actual_length><CR><LF><data>
  *
  */
  if (p_modem_ctxt->socket_ctxt.socket_receive_state == SocketRcvState_RequestSize)
  {
    START_PARAM_LOOP()
    if (element_infos->param_rank == 2U)
    {
      /* <total_receive_length> */
      PRINT_INFO("+QIRD: total_receive_length = %ld",
                 ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    else if (element_infos->param_rank == 3U)
    {
      /* <have_read_length> */
      PRINT_INFO("+QIRD: have_read_length = %ld",
                 ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    else if (element_infos->param_rank == 4U)
    {
      /* <unread_length> */
      uint32_t buff_in = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                   element_infos->str_size);
      PRINT_INFO("+QIRD: unread_length = %ld", buff_in)

      /* check that the size to read does not exceed maximum size ( = client buffer size) */
      if (buff_in > p_modem_ctxt->socket_ctxt.socketReceivedata.max_buffer_size)
      {
        /* The size to read exceed client buffer size.
         * Limit the value to max client buffer size.
         */
        p_modem_ctxt->socket_ctxt.socket_rx_expected_buf_size =
          p_modem_ctxt->socket_ctxt.socketReceivedata.max_buffer_size;
        PRINT_INFO("Limit request to maximum buffer size (%ld) whereas more data are available (%ld)",
                   p_modem_ctxt->socket_ctxt.socketReceivedata.max_buffer_size,
                   buff_in)
      }
      else
      {
        /* Update size to read */
        p_modem_ctxt->socket_ctxt.socket_rx_expected_buf_size = buff_in;
      }
    }
    else
    {
      /* parameters ignored */
      __NOP(); /* to avoid warning */
    }
    END_PARAM_LOOP()
  }
  else  if (p_modem_ctxt->socket_ctxt.socket_receive_state == SocketRcvState_RequestData_Header)
  {
    START_PARAM_LOOP()
    /* Receiving socket DATA HEADER which include data size received */
    if (element_infos->param_rank == 2U)
    {
      /* <read_actual_length> */
      PRINT_INFO("+QIRD: received data size = %ld",
                 ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
      /* NOTE !!! the size is purely informative in current implementation
      *  indeed, due to real time constraints, the socket data header is analyzed directly
      *  in ATCustom_BG96_checkEndOfMsgCallback()
      */
      /* data header analyzed, ready to analyze data payload */
      p_modem_ctxt->socket_ctxt.socket_receive_state = SocketRcvState_RequestData_Payload;

    }
    else if (element_infos->param_rank == 3U)
    {
      /* <remoteIP> */
      (void) memset((void *)&p_modem_ctxt->socket_ctxt.socketReceivedata.ip_addr_value[0],
                    0, MAX_IP_ADDR_SIZE);
      (void) memcpy((void *)&p_modem_ctxt->socket_ctxt.socketReceivedata.ip_addr_value[0],
                    (const void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
                    (size_t) element_infos->str_size);
      PRINT_INFO("+QIRD: remote IP address = %s", p_modem_ctxt->socket_ctxt.socketReceivedata.ip_addr_value)

      /* determine IP address type */
      p_modem_ctxt->socket_ctxt.socketReceivedata.ip_addr_type =
        atcm_get_ip_address_type((AT_CHAR_t *)&p_modem_ctxt->socket_ctxt.socketReceivedata.ip_addr_value);

    }
    else if (element_infos->param_rank == 4U)
    {
      /* <remotePort> */
      p_modem_ctxt->socket_ctxt.socketReceivedata.remote_port =
        (uint16_t) ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                             element_infos->str_size);
      PRINT_INFO("+QIRD: remote port = %d", p_modem_ctxt->socket_ctxt.socketReceivedata.remote_port)
    }
    else { /* nothing to do */ }

    END_PARAM_LOOP()
  }
  else
  {
    PRINT_ERR("+QIRD: should not fall here")
  }

  return (retval);
}

/**
  * @brief  Analyze specific modem response : QIRD (data part).
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_QIRD_data_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                           const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_QIRD_data_BG96()")

  PRINT_DBG("DATA received: size=%ld vs %d", p_modem_ctxt->socket_ctxt.socket_rx_expected_buf_size,
            element_infos->str_size)

  /* Recopy data to client buffer if:
  *   - pointer on data buffer exists
  *   - and size of data <= maximum size
  */
  if ((p_modem_ctxt->socket_ctxt.socketReceivedata.p_buffer_addr_rcv != NULL) &&
      (element_infos->str_size <= p_modem_ctxt->socket_ctxt.socketReceivedata.max_buffer_size))
  {
    /* recopy data to client buffer */
    (void) memcpy((void *)p_modem_ctxt->socket_ctxt.socketReceivedata.p_buffer_addr_rcv,
                  (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                  (size_t) element_infos->str_size);
    p_modem_ctxt->socket_ctxt.socketReceivedata.buffer_size = element_infos->str_size;
  }
  else
  {
    PRINT_ERR("ERROR (receive buffer is a NULL ptr or data exceed buffer size)")
    retval = ATACTION_RSP_ERROR;
  }

  return (retval);
}

/**
  * @brief  Analyze specific modem response : QISTATE.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_QISTATE_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                         const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_QISTATE_BG96()")
  at_bool_t bg96_qistate_for_requested_socket = AT_FALSE;

  /* format:
  * +QISTATE:<connectID>,<service_type>,<IP_adress>,<remote_port>,<local_port>,<socket_state>,
  *          <contextID>,<serverID>,<access_mode>,<AT_port>
  *
  * where:
  *
  * exple: +QISTATE: 0,"TCP","220.180.239.201",8705,65514,2,1,0,0,"usbmodem"
  */

  START_PARAM_LOOP()

  if (p_modem_ctxt->socket_ctxt.p_socket_cnx_infos != NULL)
  {
    if (element_infos->param_rank == 2U)
    {
      /* <connId> */
      uint32_t connId = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                  element_infos->str_size);
      socket_handle_t sockHandle = atcm_socket_get_socket_handle(p_modem_ctxt, connId);
      /* if this connection ID corresponds to requested socket handle, we will report the following infos */
      if (sockHandle == p_modem_ctxt->socket_ctxt.p_socket_cnx_infos->socket_handle)
      {
        bg96_qistate_for_requested_socket = AT_TRUE;
      }
      else
      {
        bg96_qistate_for_requested_socket = AT_FALSE;
      }
      PRINT_DBG("+QISTATE: <connId>=%ld (requested=%d)", connId,
                ((bg96_qistate_for_requested_socket == AT_TRUE) ? 1 : 0))
    }
    else if (element_infos->param_rank == 3U)
    {
      /* <service_type> */
      AT_CHAR_t serviceType[16] = {0};
      (void) memcpy((void *)&serviceType[0],
                    (const void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
                    (size_t) element_infos->str_size);
      PRINT_DBG("+QISTATE: <service_type>=%s", serviceType)
    }
    else if (element_infos->param_rank == 4U)
    {
      /* <IP_adress> */
      atcm_extract_IP_address((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx],
                              (uint16_t) element_infos->str_size,
                              (uint8_t *) p_modem_ctxt->socket_ctxt.p_socket_cnx_infos->infos->rem_ip_addr_value);
      PRINT_DBG("+QISTATE: <remote IP_adress>=%s",
                p_modem_ctxt->socket_ctxt.p_socket_cnx_infos->infos->rem_ip_addr_value)
    }
    else if (element_infos->param_rank == 5U)
    {
      /* <remote_port> */
      uint32_t remPort = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                   element_infos->str_size);
      PRINT_DBG("+QISTATE: <remote_port>=%ld", remPort)
      if (bg96_qistate_for_requested_socket == AT_TRUE)
      {
        p_modem_ctxt->socket_ctxt.p_socket_cnx_infos->infos->rem_port = (uint16_t) remPort;
      }
    }
    else if (element_infos->param_rank == 6U)
    {
      /* <local_port> */
      uint32_t locPort = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                   element_infos->str_size);
      PRINT_DBG("+QISTATE: <local_port>=%ld", locPort)
      if (bg96_qistate_for_requested_socket == AT_TRUE)
      {
        p_modem_ctxt->socket_ctxt.p_socket_cnx_infos->infos->rem_port = (uint16_t) locPort;
      }
    }
    else if (element_infos->param_rank == 7U)
    {
      /*<socket_state> */
      PRINT_DBG("+QISTATE: <socket_state>=%ld",
                ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    else if (element_infos->param_rank == 8U)
    {
      /* <contextID> */
      PRINT_DBG("+QISTATE: <contextID>=%ld",
                ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    else if (element_infos->param_rank == 9U)
    {
      /* <serverID> */
      PRINT_DBG("+QISTATE: <serverID>=%ld",
                ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    else if (element_infos->param_rank == 10U)
    {
      /* <access_mode> */
      PRINT_DBG("+QISTATE: <access_mode>=%ld",
                ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    else if (element_infos->param_rank == 11U)
    {
      /* <AT_port> */
      AT_CHAR_t _ATport[16] = {0};
      (void) memcpy((void *)&_ATport[0],
                    (const void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
                    (size_t) element_infos->str_size);
      PRINT_DBG("+QISTATE: <AT_port>=%s", _ATport)
    }
    else
    {
      /* parameter ignored */
      __NOP(); /* to avoid warning */
    }
  }
  else
  {
    PRINT_ERR("No socket cnx infos context available")
    retval = ATACTION_RSP_ERROR;
  }

  END_PARAM_LOOP()

  return (retval);
}

/**
  * @brief  Analyze specific modem response : QPING.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_QPING_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                       const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  /*UNUSED(p_at_ctxt);*/

  at_action_rsp_t retval = ATACTION_RSP_URC_FORWARDED;
  PRINT_API("enter fRspAnalyze_QPING_BG96()")

  /* intermediate ping response format:
  * +QPING: <result>[,<IP_address>,<bytes>,<time>,<ttl>]
  *
  * last ping response format:
  * +QPING: <finresult>[,<sent>,<rcvd>,<lost>,<min>,<max>,<avg>]
  */

  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    /* new ping response: clear ping response structure (except the index) */
    clear_ping_resp_struct(p_modem_ctxt);

    /* <result> or <finresult> */
    uint32_t result = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                element_infos->str_size);
    PRINT_DBG("Ping result = %ld", result)

    p_modem_ctxt->persist.urc_avail_ping_rsp = AT_TRUE;
    /* check if this is the first ping response */
    if (p_modem_ctxt->persist.ping_resp_urc.index == PING_INVALID_INDEX)
    {
      /* initialize index to 1 if this is the first ping response */
      p_modem_ctxt->persist.ping_resp_urc.index = 1;
    }
    else
    {
      /* increment index */
      p_modem_ctxt->persist.ping_resp_urc.index++;
    }
    p_modem_ctxt->persist.ping_resp_urc.ping_status = (result == 0U) ? CELLULAR_OK : CELLULAR_ERROR;
  }
  else if (element_infos->param_rank == 3U)
  {
    /* check if this is an intermediate report or the final report */
    if (p_msg_in->buffer[element_infos->str_start_idx] == 0x22U)
    {
      /* this is an IP address: intermediate ping response */
      if (p_modem_ctxt->persist.ping_resp_urc.is_final_report == CELLULAR_TRUE)
      {
        PRINT_DBG("intermediate ping report")
        p_modem_ctxt->persist.ping_resp_urc.is_final_report = CELLULAR_FALSE;
      }
    }
    else
    {
      /* this is not an IP address: final ping response */
      if (p_modem_ctxt->persist.ping_resp_urc.is_final_report == CELLULAR_FALSE)
      {
        PRINT_DBG("final ping report")
        p_modem_ctxt->persist.ping_resp_urc.is_final_report = CELLULAR_TRUE;
      }
    }

    if (p_modem_ctxt->persist.ping_resp_urc.is_final_report  == CELLULAR_FALSE)
    {
      /* <IP_address> */
      atcm_extract_IP_address((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx],
                              (uint16_t) element_infos->str_size,
                              (uint8_t *)p_modem_ctxt->persist.ping_resp_urc.ping_addr);
      PRINT_DBG("+QIPING: <ping IP_adress>=%s", p_modem_ctxt->persist.ping_resp_urc.ping_addr)
    }
    else
    {
      /* <sent> */
      /* parameter ignored */
      __NOP(); /* to avoid warning */
    }
  }
  else if (element_infos->param_rank == 4U)
  {
    if (p_modem_ctxt->persist.ping_resp_urc.is_final_report  == CELLULAR_FALSE)
    {
      /* <bytes> */
      uint32_t ping_bytes = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                      element_infos->str_size);
      p_modem_ctxt->persist.ping_resp_urc.ping_size = (uint16_t)ping_bytes;
    }
    else
    {
      /* <rcvd> */
      /* parameter ignored */
      __NOP(); /* to avoid warning */
    }
  }
  else if (element_infos->param_rank == 5U)
  {
    if (p_modem_ctxt->persist.ping_resp_urc.is_final_report  == CELLULAR_FALSE)
    {
      /* <time>*/
      uint32_t timeval = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                   element_infos->str_size);
      p_modem_ctxt->persist.ping_resp_urc.time = (uint32_t)timeval;
    }
    else
    {
      /* <lost> */
      /* parameter ignored */
      __NOP(); /* to avoid warning */
    }
  }
  else if (element_infos->param_rank == 6U)
  {
    if (p_modem_ctxt->persist.ping_resp_urc.is_final_report  == CELLULAR_FALSE)
    {
      /* <ttl>*/
      uint32_t ttl = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                               element_infos->str_size);
      p_modem_ctxt->persist.ping_resp_urc.ttl = (uint8_t)ttl;
    }
    else
    {
      /* <min_time>*/
      /* parameter ignored */
      __NOP(); /* to avoid warning */
    }
  }
  else if (element_infos->param_rank == 7U)
  {
    if (p_modem_ctxt->persist.ping_resp_urc.is_final_report  == CELLULAR_TRUE)
    {
      /* <max_time>*/
      /* parameter ignored */
      __NOP(); /* to avoid warning. Parameter ignored but may be used in the futur or for debug */

    }
  }
  else if (element_infos->param_rank == 8U)
  {
    if (p_modem_ctxt->persist.ping_resp_urc.is_final_report  == CELLULAR_TRUE)
    {
      /* <avg_time>*/
      /* parameter ignored */
      __NOP(); /* to avoid warning. Parameter ignored but may be used in the futur or for debug */
    }
  }
  else
  {
    /* parameter ignored */
  }
  END_PARAM_LOOP()

  return (retval);
}

/**
  * @}
  */

/** @defgroup AT_CUSTOM_QUECTEL_BG96_SOCKET_Private_Functions AT_CUSTOM QUECTEL_BG96 SOCKET Private Functions
  * @{
  */
/**
  * @brief  Clear the structure used for PING information.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval none
  */
static void clear_ping_resp_struct(atcustom_modem_context_t *p_modem_ctxt)
{
  /* clear all parameters except the index:
   * save the index, reset the structure and recopy saved index
   */
  uint8_t saved_idx = p_modem_ctxt->persist.ping_resp_urc.index;
  (void) memset((void *)&p_modem_ctxt->persist.ping_resp_urc, 0, sizeof(CS_Ping_response_t));
  p_modem_ctxt->persist.ping_resp_urc.index = saved_idx;
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

/**
  * @}
  */

#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */

