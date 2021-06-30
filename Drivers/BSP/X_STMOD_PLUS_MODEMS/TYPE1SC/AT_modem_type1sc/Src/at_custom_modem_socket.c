/**
  ******************************************************************************
  * @file    at_custom_modem_socket.c
  * @author  MCD Application Team
  * @brief   This file provides all the 'socket' code to the
  *          MURATA-TYPE1SC-EVK module (ALT1250 modem: LTE-cat-M1)
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

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "at_core.h"
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

/* Private typedef -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/
#if (USE_TRACE_ATCUSTOM_SPECIFIC == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_INFO(format, args...) TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P0, "TYPE1SC:" format "\n\r", ## args)
#define PRINT_DBG(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P1, "TYPE1SC:" format "\n\r", ## args)
#define PRINT_API(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P2, "TYPE1SC API:" format "\n\r", ## args)
#define PRINT_ERR(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_ERR, "TYPE1SC ERROR:" format "\n\r", ## args)
#define PRINT_BUF(pbuf, size)       TRACE_PRINT_BUF_CHAR(DBG_CHAN_ATCMD, DBL_LVL_P1, (const CRC_CHAR_t *)pbuf, size);
#else
#define PRINT_INFO(format, args...)  (void) printf("TYPE1SC:" format "\n\r", ## args);
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_API(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(format, args...)   (void) printf("TYPE1SC ERROR:" format "\n\r", ## args);
#define PRINT_BUF(...)   __NOP(); /* Nothing to do */
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

/* Private defines -----------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static uint8_t convertToASCII(uint8_t nbr);
static void convertCharToHEX(uint8_t val, uint8_t *p_msd, uint8_t *p_lsd);
static at_status_t convertDigitToValue(uint8_t digit, uint8_t *res);
static at_status_t convertHEXToChar(uint8_t MSD, uint8_t LSD, uint8_t *res);

/* Functions Definition ------------------------------------------------------*/

/* Build command functions ---------------------------------------------------*/

/* Analyze command functions -------------------------------------------------*/
at_status_t fCmdBuild_PDNACT(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_PDNACT()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
    uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);

    /* check if this PDP context has been defined */
    if (p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].conf_id == CS_PDN_NOT_DEFINED)
    {
      PRINT_INFO("PDP context not explicitly defined for conf_id %d (using modem params)", current_conf_id)
    }

    /* AT+PDNACTCGACT=<act>,[<sessionID>][,<apnname>]
     * <act>: 0 to deactivate, 1 to activate
     * <sessionID>: numeric value of the session identifier
     * <apnname>: string, indicates APN name configured for the PDN
    */
    (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,%d",
                   ((p_modem_ctxt->CMD_ctxt.pdn_state == PDN_STATE_ACTIVATE) ? 1 : 0),
                   modem_cid);
  }

  return (retval);
}

at_status_t fCmdBuild_SOCKETCMD_ALLOCATE(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  /* Array to convert AT%SOCKETCMD="ALLOCATE" service type parameter to a string
  *  need to be aligned with atsocket_servicetype_t
  */
  static const AT_CHAR_t *const type1sc_array_ALLOCATE_service_type[] =
  {
    ((uint8_t *)"OPEN"),       /* start TCP connection as client */
    ((uint8_t *)"OPEN"),       /* start UDP connection as client */
    ((uint8_t *)"LISTENP"),    /* start TCP server to listen TCP connection */
    ((uint8_t *)"LISTEN")      /* start UDP connection as service           */
  };

  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_SOCKETCMD_ALLOCATE()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (p_modem_ctxt->socket_ctxt.socket_info != NULL)
    {
      /* allocates socket session
      * AT%SOCKETCMD="ALLOCATE",<param1>,<param2>,<param3>,<param4>,<param5>,<param6>,<param7>
      * <param1>: decimal, the "session ID"
      * <param2>: string, "TCP" or "UDP"
      * <param3>: string, "OPEN" "LISTEN" "LISTENP"
      * <param4>: string, destination IPV4
      * <param5>: decimal, destination UDP/TCP port number (1-65535, empty for TCP/UDP server mode)
      * <param6>: decimal, local UDP/TCP port number (1-65535, 0 means auto)
      * <param7>: decimal, packet size to use for data sending (1-1500, 0 for default value)
      *
      * function will return the affected modem socket id
      */

      /* <param1> */
      /* convert user cid (CS_PDN_conf_id_t) to PDP modem cid (value) */
      uint8_t pdp_modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist,
                                                          p_modem_ctxt->socket_ctxt.socket_info->conf_id);
      PRINT_INFO("user cid = %d, PDP modem cid = %d", (uint8_t)p_modem_ctxt->socket_ctxt.socket_info->conf_id,
                 pdp_modem_cid)

      /* <param3> */
      /* set the right string for socket open mode client / server */
      atsocket_servicetype_t service_type_index;

      if (strcmp((CRC_CHAR_t const *)p_modem_ctxt->socket_ctxt.socket_info->ip_addr_value,
                 (CRC_CHAR_t const *)CONFIG_MODEM_UDP_SERVICE_CONNECT_IP) == 0)
      {
        /* TCP or UDP as server */
        /* To Do: TCP server in mode IPv6 is not yet managed */
        service_type_index = ((p_modem_ctxt->socket_ctxt.socket_info->protocol) == CS_TCP_PROTOCOL) ? \
                             ATSOCKET_SERVICETYPE_TCP_SERVER : ATSOCKET_SERVICETYPE_UDP_SERVICE;
      }
      else
      {
        /* TCP or UDP as client */
        service_type_index = ((p_modem_ctxt->socket_ctxt.socket_info->protocol) == CS_TCP_PROTOCOL) ? \
                             ATSOCKET_SERVICETYPE_TCP_CLIENT : ATSOCKET_SERVICETYPE_UDP_CLIENT;
      }

      /* in case of server, remote port <param5> is empty */
      if ((service_type_index == ATSOCKET_SERVICETYPE_TCP_SERVER)
          || (service_type_index == ATSOCKET_SERVICETYPE_UDP_SERVICE))
      {
        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"%s\",%d,\"%s\",\"%s\",\"%s\",,%d,%ld",
                       "ALLOCATE",
                       pdp_modem_cid,
                       ((p_modem_ctxt->socket_ctxt.socket_info->protocol == CS_TCP_PROTOCOL) ? "TCP" : "UDP"),
                       type1sc_array_ALLOCATE_service_type[service_type_index],
                       p_modem_ctxt->socket_ctxt.socket_info->ip_addr_value,
                       p_modem_ctxt->socket_ctxt.socket_info->local_port,
                       CONFIG_MODEM_MAX_SOCKET_TX_DATA_SIZE);
      }
      else
      {
        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"%s\",%d,\"%s\",\"%s\",\"%s\",%d,%d,%ld",
                       "ALLOCATE",
                       pdp_modem_cid,
                       ((p_modem_ctxt->socket_ctxt.socket_info->protocol == CS_TCP_PROTOCOL) ? "TCP" : "UDP"),
                       type1sc_array_ALLOCATE_service_type[service_type_index],
                       p_modem_ctxt->socket_ctxt.socket_info->ip_addr_value,
                       p_modem_ctxt->socket_ctxt.socket_info->remote_port,
                       p_modem_ctxt->socket_ctxt.socket_info->local_port,
                       CONFIG_MODEM_MAX_SOCKET_TX_DATA_SIZE);
      }
    }
    else
    {
      PRINT_ERR("No socket context for fCmdBuild_SOCKETCMD_ALLOCATE")
    }
  }

  return (retval);
}

at_status_t fCmdBuild_SOCKETCMD_ACTIVATE(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_SOCKETCMD_ACTIVATE()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (p_modem_ctxt->socket_ctxt.socket_info != NULL)
    {
      /* activates the predefined socket
      * AT%SOCKETCMD="ACTIVATE",<param1>
      * <param1>: decimal, the socket ID of the specified socket
      */
      uint32_t socketID = atcm_socket_get_modem_cid(p_modem_ctxt, p_modem_ctxt->socket_ctxt.socket_info->socket_handle);
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"%s\",%ld",
                     "ACTIVATE",
                     socketID);
    }
    else
    {
      PRINT_ERR("No socket context for fCmdBuild_SOCKETCMD_ACTIVATE")
    }
  }

  return (retval);
}

at_status_t fCmdBuild_SOCKETCMD_INFO(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_SOCKETCMD_INFO()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (p_modem_ctxt->socket_ctxt.socket_info != NULL)
    {
      /* returns the details of the specific socket ID
      * AT%SOCKETCMD="INFO",<param1>
      * <param1>: decimal, the socket ID for which info is requested
      */
      /* warning ! information related to SID_CS_SOCKET_CNX_STATUS
       * are referring to p_modem_ctxt->socket_ctxt.socket_cnx_infos,
       * especially for the socket_handle parameter.
       */
      uint32_t socketID = atcm_socket_get_modem_cid(p_modem_ctxt,
                                                    p_modem_ctxt->socket_ctxt.socket_cnx_infos->socket_handle);
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"%s\",%ld",
                     "INFO",
                     socketID);
    }
    else
    {
      PRINT_ERR("No socket context for fCmdBuild_SOCKETCMD_INFO")
    }
  }
  return (retval);
}

at_status_t fCmdBuild_SOCKETCMD_DEACTIVATE(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_SOCKETCMD_DEACTIVATE()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (p_modem_ctxt->socket_ctxt.socket_info != NULL)
    {
      /* Request to deactivate the specific socket ID and release its resources
      * AT%SOCKETCMD="DEACTIVATE",<param1>
      * <param1>: decimal, the socket ID to be closed
      */
      uint32_t socketID = atcm_socket_get_modem_cid(p_modem_ctxt, p_modem_ctxt->socket_ctxt.socket_info->socket_handle);

      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"%s\",%ld",
                     "DEACTIVATE",
                     socketID);
    }
    else
    {
      PRINT_ERR("No socket context for fCmdBuild_SOCKETCMD_DEACTIVATE")
    }
  }

  return (retval);
}

at_status_t fCmdBuild_SOCKETCMD_DELETE(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_SOCKETCMD_DELETE()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (p_modem_ctxt->socket_ctxt.socket_info != NULL)
    {
      /* Request to delete specific socket ID allocation
      * AT%SOCKETCMD="DELETE",<param1>
      * <param1>: decimal, the socket ID to be deleted
      */
      uint32_t socketID = atcm_socket_get_modem_cid(p_modem_ctxt, p_modem_ctxt->socket_ctxt.socket_info->socket_handle);

      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"%s\",%ld",
                     "DELETE",
                     socketID);
    }
    else
    {
      PRINT_ERR("No socket context for fCmdBuild_SOCKETCMD_DELETE")
    }
  }

  return (retval);
}

at_status_t fCmdBuild_SOCKETDATA_SEND(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_SOCKETDATA_SEND()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* write to the socket
    * AT%SOCKETDATA="SEND",<param1>,<param2>,<param3>,<param4>,<param5>
    * <param1>: decimal, the socket ID
    * <param2>: decimal, the length in bytes of the data which needs to be written (1-3000)
    * <param3>: string, the data in HEX format (in quotes)
    * <param4>: string, destination IPv4 or IPv6 address(in quotes) for UDP datagram only
    * <param5>: decimal, destination port number (1-65535) for UDP datagram only
    */
    if (p_modem_ctxt->SID_ctxt.socketSendData_struct.p_buffer_addr_send != NULL)
    {
      uint32_t socketID = atcm_socket_get_modem_cid(p_modem_ctxt,
                                                    p_modem_ctxt->SID_ctxt.socketSendData_struct.socket_handle);
      uint16_t str_size = (uint16_t) p_modem_ctxt->SID_ctxt.socketSendData_struct.buffer_size;

      /* Convert buffer in HEX format */

      /* build first part of the command: AT%SOCKETDATA="SEND",<param1>,<param2>," */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"SEND\",%ld,%d,\"",
                     socketID,
                     str_size);

      /* now copy the buffer */
      uint16_t cmd_params_size = (uint16_t) strlen((CRC_CHAR_t *)&p_atp_ctxt->current_atcmd.params);
      for (uint16_t idx = 0U; idx < str_size; idx++)
      {
        /* char by char */
        uint8_t onechar = p_modem_ctxt->SID_ctxt.socketSendData_struct.p_buffer_addr_send[idx];
        PRINT_DBG("[ %c -> HEX= %2x / DEC= %2d]", onechar, onechar, onechar)

        /* convert this char to its hexadecimal value (example 'A' is converted to '65') - ASCII */
        uint8_t ms;
        uint8_t ls;
        convertCharToHEX(onechar, &ms, &ls);
        (void) memcpy((void *) &p_atp_ctxt->current_atcmd.params[cmd_params_size + (2U * idx)],
                      (const CS_CHAR_t *)&ms,
                      1);
        (void) memcpy((void *) &p_atp_ctxt->current_atcmd.params[cmd_params_size + (1U + (2U * idx))],
                      (const CS_CHAR_t *)&ls,
                      1);
      }

      /* Don't use strlen for next instruction due to data buffer */
      cmd_params_size += (2U * str_size);

      /* For UDP socket and if provided
         copy ,<remoteIP>,<remote_port> and close the data string with "  */
      if (p_modem_ctxt->SID_ctxt.socketSendData_struct.ip_addr_type != CS_IPAT_INVALID)
        /* check if socket is in UDP protocol to filter <remoteIP>,<remote_port> seems not needed
         * already filtered by COM module
         */
      {
        (void) sprintf((CRC_CHAR_t *)(&(p_atp_ctxt->current_atcmd.params[cmd_params_size])),
                       "\",\"%s\",%d",
                       p_modem_ctxt->SID_ctxt.socketSendData_struct.ip_addr_value,
                       p_modem_ctxt->SID_ctxt.socketSendData_struct.remote_port);
      }
      else
      {
        /* or just finally close the data string with " */
        (void) memcpy((void *) &p_atp_ctxt->current_atcmd.params[cmd_params_size],
                      (const CS_CHAR_t *)"\"",
                      (size_t) 1);
      }
    }
    else
    {
      PRINT_ERR("ERROR, send buffer is a NULL ptr !!!")
      retval = ATSTATUS_ERROR;
    }
  }

  return (retval);
}

at_status_t fCmdBuild_SOCKETDATA_RECEIVE(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_SOCKETDATA_RECEIVE()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* read from the socket
    * AT%SOCKETDATA="RECEIVE",<param1>,<param2>
    * <param1>: decimal, the socket ID
    * <param2>: decimal, the max length of the data buffer to be read from the socket (1-3000)
    */
    uint32_t socketID = atcm_socket_get_modem_cid(p_modem_ctxt,
                                                  p_modem_ctxt->socket_ctxt.socketReceivedata.socket_handle);
    uint32_t requested_data_size;
    requested_data_size = p_modem_ctxt->socket_ctxt.socketReceivedata.max_buffer_size;
    (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"RECEIVE\",%ld,%ld",
                   socketID,
                   requested_data_size);
  }

  return (retval);
}

at_status_t fCmdBuild_DNSRSLV(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_DNSRSLV()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* provides a request to solve a specific domain name
     * AT%DNSRSLV=<sessionID>,<domain_name>
     * <sessionID>: decimal, PDN
     * <domain_name>: string, domain to solve
     */
    CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
    uint8_t pdp_modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
    /* configure DNS server address for the specified PDP context */
    (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,\"%s\"",
                   pdp_modem_cid,
                   p_modem_ctxt->SID_ctxt.dns_request_infos->dns_req.host_name);
  }

  return (retval);
}

at_status_t fCmdBuild_PINGCMD(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_PINGCMD()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* validate ping request
     * As PING request are synchronous for this modem, always validate PING request before to send PING command.
     */
    atcm_validate_ping_request(p_modem_ctxt);
    /* execute PING services
     * AT%PINGCMD=<ip_type>,<dst_ip>[,<count>[,<packetsize>,<timeout>]]
     *   <ip_type>: decimal, 0 for IPv4 and 1 for IPv6
     *   <dst_ip>: string, destination IPv4 or IPv6 address
     *   <count>: decimal, the number of ping request retries (default = 1)
     *   <packetsize>: decimal, number of data bytes to be sent (default = 56), TYPE1SC_PING_LENGTH
     *   <timeout>: decimal, time to wait for a response (in seconds)
     */
    (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,\"%s\",%d,%d,%d",
                   0 /* IPv4 */,
                   p_modem_ctxt->SID_ctxt.ping_infos.ping_params.host_addr,
                   p_modem_ctxt->SID_ctxt.ping_infos.ping_params.pingnum,
                   TYPE1SC_PING_LENGTH,
                   p_modem_ctxt->SID_ctxt.ping_infos.ping_params.timeout);
  }

  return (retval);
}

/* TYPE1SC specific analyze commands */
at_action_rsp_t fRspAnalyze_PDNACT(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                   const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_modem_ctxt);
  UNUSED(p_msg_in);
  UNUSED(element_infos);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_PDNACT()")

  return (retval);
}

at_action_rsp_t fRspAnalyze_SOCKETCMD(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                      const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_SOCKETCMD()")

  START_PARAM_LOOP()

  /* for "ALLOCATE" command:
   * %SOCKETCMD:<socket_id>
   *  <socket_id>: decimal, the socket identifier
   */
  if (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_SOCKETCMD_ALLOCATE)
  {
    if (element_infos->param_rank == 2U)
    {
      /* <socket_id> */
      uint32_t affected_socket_ID = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                              element_infos->str_size);
      PRINT_INFO("<affected socket_id> = %ld", affected_socket_ID)
      type1sc_shared.SocketCmd_Allocated_SocketID = AT_TRUE;
      /* save this socket ID received from the modem */
      (void) atcm_socket_set_modem_cid(p_modem_ctxt, p_modem_ctxt->socket_ctxt.socket_info->socket_handle,
                                       affected_socket_ID);
    }
    else
    {
      /* other parameters are ignored */
    }
  }
  /* for "INFO" command:
   * %SOCKETCMD:<socket_stat>,<socket_type>,<src_ip>,<dst_ip>,<src_port>,<dst_port>[,<socket_dir>,<socket_to>]]
   *  <socket_stat>: string, "DEACTIVATED" or "ACTIVATED"
   *  <socket_type>: string, "TCP" or "UDP"
   *  <src_ip>:  string, source IP address
   *  <dst_ip>: string, destination IP address
   *  <src_port>: string, source UDP/TCP port number in the range 1-65535
   *  <dst_port>:  string, destination UDP/TCP port number in the range 1-65535
   *  <socket_dir>: decimal, the direction of the TCP socket (0:No set, 1:Dialer)
   *  <socket_to>: decimal, TCP connection setup timeout as specified in the "OPEN" command
   */
  else if (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_SOCKETCMD_INFO)
  {
    /* warning ! information related to SID_CS_SOCKET_CNX_STATUS
     * are referring to p_modem_ctxt->socket_ctxt.socket_cnx_infos
     * especially the socket_handle to
     */
    if (element_infos->param_rank == 2U)
    {
      /* <socket_stat> */
      /* info ignored */
    }
    else if (element_infos->param_rank == 3U)
    {
      /* <socket_type> */
      /* info ignored */
    }
    else if (element_infos->param_rank == 4U)
    {
      /* <src_ip> */
      atcm_extract_IP_address((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx],
                              (uint16_t) element_infos->str_size,
                              (uint8_t *)p_modem_ctxt->socket_ctxt.socket_cnx_infos->infos->loc_ip_addr_value);
    }
    else if (element_infos->param_rank == 5U)
    {
      /* <dst_ip> */
      atcm_extract_IP_address((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx],
                              (uint16_t) element_infos->str_size,
                              (uint8_t *)p_modem_ctxt->socket_ctxt.socket_cnx_infos->infos->rem_ip_addr_value);
    }
    else if (element_infos->param_rank == 6U)
    {
      /* <src_port> */
      uint32_t src_port = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                    element_infos->str_size);
      PRINT_DBG("<src_port>=%ld", src_port)
      p_modem_ctxt->socket_ctxt.socket_cnx_infos->infos->loc_port = (uint16_t) src_port;
    }
    else if (element_infos->param_rank == 7U)
    {
      /* <dst_port> */
      uint32_t dst_port = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                    element_infos->str_size);
      PRINT_DBG("<dst_port>=%ld", dst_port)
      p_modem_ctxt->socket_ctxt.socket_cnx_infos->infos->rem_port = (uint16_t) dst_port;
    }
    else if (element_infos->param_rank == 8U)
    {
      /* <socket_dir> */
      /* info ignored */
    }
    else if (element_infos->param_rank == 9U)
    {
      /* <socket_to> */
      /* info ignored */
    }
    else
    {
      /* other parameters are ignored */
    }
  }
  /* for "LASTERROR" command:
   * %SOCKETCMD:<socket_err>
   *  <socket_err>: decimal, error value as defined by 3GPP TS 27.007 subclause 9.2
   */
  else if (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_SOCKETCMD_LASTERROR)
  {
    PRINT_INFO("CMD_AT_SOCKETCMD_LASTERROR")
    if (element_infos->param_rank == 2U)
    {
      /* <socket_err> */
      PRINT_INFO("<last socket_err> = %ld", ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                                      element_infos->str_size))
      /* information not reported for the moment */
    }
    else
    {
      /* other parameters are ignored */
    }
  }
  else if ((p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_SOCKETCMD_FASTEND) ||
           (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_SOCKETCMD_CONFSEND) ||
           (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_SOCKETCMD_SSLINFO))
  {
    PRINT_INFO("analyze of this command not implemented")
  }
  else
  {
    PRINT_INFO("unexpected command")
  }

  END_PARAM_LOOP()

  return (retval);
}

at_action_rsp_t fRspAnalyze_SOCKETDATA(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                       const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_SOCKETDATA()")
  uint32_t rlength = 0U;

  /*
   *  for "SEND" command:
   *  %SOCKETDATA:<socket_id>[,<wlength>]]
   *     <socket_id>: decimal, the socket ID
   *     <wlength>: the actual length in bytes of the data written to the socket
   *
   *  for "RECEIVE" command:
   *  %SOCKETDATA:<socket_id>[,<rlength>,<moreData>[,<rdata>]]]
   *     <socket_id>: decimal, the socket ID
   *     <rlength>: decimal, the actual length in bytes of the data which was actually read
   *     <moreData>: decimal, the length in bytes of the data left in the RX buffer
   *     <rdata>: string the read data in HEX format (in quotes)
   *     <src_ip>: string, source IPv4 or IPv6 address(in quotes) for UDP datagram only
   *     <src_port>: decimal, source port number (1-65535) for UDP datagram only
   */
  START_PARAM_LOOP()


  if (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_SOCKETDATA_SEND)
  {
    /* for a SEND command, we have:
    *   %SOCKETDATA:<socket_id>[,<wlength>]]
    *
    *     <socket_id> is the socket ID.
    *     <wlength> is length in bytes of data written to the socket.
    */
    if (element_infos->param_rank == 2U)
    {
      /* <socket_id> */
      PRINT_DBG("<SOCKETDATA_SEND: socket_id> = %ld",
                ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                          element_infos->str_size))
    }
    else if (element_infos->param_rank == 3U)
    {
      /* <wlength> */
      PRINT_DBG("<SOCKETDATA_SEND: wlength> = %ld",
                ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    else
    {
      /* other parameters are ignored */
    }
  }

  /* for a RECEIVE command, we have:
  *   %SOCKETDATA:<socket_id>[,<rlength>,<modeData>[,<rdata>]]]
  *
  *     <socket_id> is the socket ID.
  *     <rlength> is length in bytes of data which was actually read.
  *     <modeData> is length in bytes of data left in the RX buffer.
  *     <rdata> the read data in HEX format (in quotes).
  *     <src_ip> is source IPv4 or IPv6 address(in quotes) for UDP datagram only
  *     <src_port> is source port number (1-65535) for UDP datagram only
  */
  else if (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_SOCKETDATA_RECEIVE)
  {
    if (element_infos->param_rank == 2U)
    {
      /* <socket_id> */
      uint32_t socketId =  ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                     element_infos->str_size);
      PRINT_DBG("<SOCKETDATA_RECEIVE: socket_id> = %ld", socketId)
      uint32_t expected_socketID = atcm_socket_get_modem_cid(p_modem_ctxt,
                                                             p_modem_ctxt->socket_ctxt.socketReceivedata.socket_handle);
      /* are data received on the expected socket ID ? */
      if (socketId != expected_socketID)
      {
        PRINT_ERR("<SOCKETDATA_RECEIVE> on socket_id= %ld whereas expected socket_id= %ld",
                  socketId,
                  p_modem_ctxt->socket_ctxt.socketReceivedata.socket_handle)
        retval = ATACTION_RSP_ERROR;
      }
    }
    else if (element_infos->param_rank == 3U)
    {
      /* <rlength> */
      rlength = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      PRINT_DBG("<SOCKETDATA_RECEIVE: rlength> = %ld", rlength)
    }
    else if (element_infos->param_rank == 4U)
    {
      /* <moreData> - only for information */
      PRINT_DBG("<SOCKETDATA_RECEIVE: moreData> = %ld",
                ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    else if (element_infos->param_rank == 5U)
    {
      /* <rdata> */

      /* check that rlength announced matches size of received data */
      uint16_t data_size = (element_infos->str_size - 2U) >> 1; /* remove first and last quote (-2) then divide by 2 */
      if (rlength != data_size)
      {
        PRINT_ERR("Buffer size received (%d) does not match expected size (%ld)", data_size, rlength)
        /* only informative for debug purpose
        * we will use size of buffer really received
        */
      }
      PRINT_DBG("<SOCKETDATA_RECEIVE: rdata> computed data_size = %d", data_size)

      /* check that received data size does not exceed client buffer size */
      if (data_size <= p_modem_ctxt->socket_ctxt.socketReceivedata.max_buffer_size)
      {
        uint16_t idx;
        for (idx = 0U; ((idx < data_size) && (retval != ATACTION_RSP_ERROR)); idx++)
        {
          /* convert received buffer from HEX to ASCII format
          * example: if we receive 48545450, take digits 2 by 2 and convert them
          *          to their hexa value
          *           => 48 = 0x48 = H
          *           => 54 = 0x54 = T
          *           => 54 = 0x54 = T
          *           => 50 = 0x50 = P
          */
          uint8_t conVal;
          if (convertHEXToChar((uint8_t)p_msg_in->buffer[element_infos->str_start_idx + 1U + (idx * 2U)],
                               (uint8_t)p_msg_in->buffer[element_infos->str_start_idx + 2U + (idx * 2U)],
                               &conVal) == ATSTATUS_OK)
          {
            /* recopy data to client buffer */
            p_modem_ctxt->socket_ctxt.socketReceivedata.p_buffer_addr_rcv[idx] = conVal;
          }
          else
          {
            retval = ATACTION_RSP_ERROR;
          }
        }

        /* finally, update buffer client size */
        if (retval != ATACTION_RSP_ERROR)
        {
          p_modem_ctxt->socket_ctxt.socketReceivedata.buffer_size = data_size;
          /* print buffer for DEBUG */
          PRINT_BUF((const uint8_t *)&p_modem_ctxt->socket_ctxt.socketReceivedata.p_buffer_addr_rcv[0], data_size)
        }
        else
        {
          PRINT_ERR("error occurred during string conversion")
          /* update buffer client size to zero */
          p_modem_ctxt->socket_ctxt.socketReceivedata.buffer_size = 0;
        }
      }
      else
      {
        /* error, size of received buffer exceed client size buffer */
        PRINT_ERR("Size of received buffer (%d) exceed client buffer size (%ld)", data_size,
                  p_modem_ctxt->socket_ctxt.socketReceivedata.max_buffer_size)
        retval = ATACTION_RSP_ERROR;
      }
    }
    else if (element_infos->param_rank == 6U)
    {
      /* <src_ip = remoteIP> */
      (void) memset((void *)&p_modem_ctxt->socket_ctxt.socketReceivedata.ip_addr_value[0],
                    0, MAX_IP_ADDR_SIZE);
      p_modem_ctxt->socket_ctxt.socketReceivedata.remote_port = 0U;
      atcm_extract_IP_address((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx],
                              (uint16_t) element_infos->str_size,
                              (uint8_t *)p_modem_ctxt->socket_ctxt.socketReceivedata.ip_addr_value);
      /* determine IP address type */
      p_modem_ctxt->socket_ctxt.socketReceivedata.ip_addr_type =
        atcm_get_ip_address_type((AT_CHAR_t *)&p_modem_ctxt->socket_ctxt.socketReceivedata.ip_addr_value);
    }
    else if (element_infos->param_rank == 7U)
    {
      /* <src_port = remotePort> */
      p_modem_ctxt->socket_ctxt.socketReceivedata.remote_port =
        (uint16_t) ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                             element_infos->str_size);
    }
    else
    {
      /* other parameters are ignored */
    }
  }
  else
  {
    PRINT_ERR("unexpected command")
  }

  END_PARAM_LOOP()

  return (retval);
}

at_action_rsp_t fRspAnalyze_SOCKETEV(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                     const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_SOCKETEV()")

  /* init param received info */
  uint32_t event_id = 0U; /* initialize to a invalid value */

  /* Receive %SOCKETEV
   * 1/ if it's an URC:
   *    %SOCKETEV:<event_id>,<socket_id>[,<connected_socket_id>]
   *
   * 2/ if it's the answer to AT%SOCKETEV=?
   *    %SOCKETEV:(list of supported <event_id>s),(list of supported <mode>s)
   *
   *  <event_id> = 1: Rx Buffer has more Bytes to read
   *  <event_id> = 3: Socket terminated by peer
   */
  if (p_atp_ctxt->current_atcmd.id != (CMD_ID_t) CMD_AT_SOCKETEV)
  {
    /* if this is an URC */

    START_PARAM_LOOP()
    if (element_infos->param_rank == 2U)
    {
      /* <event_id> */
      event_id = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      PRINT_DBG("SOCKET_EVENT <event_id> = %ld : %s", event_id,
                ((event_id == 1U) ? "RX buffer has more bytes to read" :
                 ((event_id == 2U) ? "Socket terminated by peer" : "Invalid event !")))
    }
    else if (element_infos->param_rank == 3U)
    {
      /* <socket_id> */
      uint32_t socket_id = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                     element_infos->str_size);
      socket_handle_t sockHandle = atcm_socket_get_socket_handle(p_modem_ctxt, socket_id);

      if (event_id == 1U)
      {
        /* RX data notification */
        (void) atcm_socket_set_urc_data_pending(p_modem_ctxt, sockHandle);
        PRINT_INFO("SOCKET_EVENT: RX data available on socket %ld (handle=%ld) ", socket_id, sockHandle)
        retval = ATACTION_RSP_URC_FORWARDED;
      }
      else if (event_id == 3U)
      {
        /* Closed socket notification */
        (void) atcm_socket_set_urc_closed_by_remote(p_modem_ctxt, sockHandle);
        PRINT_INFO("SOCKET_EVENT: socket %ld (handle=%ld) closed by remote", socket_id, sockHandle)
        retval = ATACTION_RSP_URC_FORWARDED;
      }
      else
      {
        /* ignored */
        PRINT_DBG("SOCKET_EVENT: on socket %ld (handle=%ld) ignored", socket_id, sockHandle)
      }
    }
    else if (element_infos->param_rank == 4U)
    {
      /* <connected_socket_id> */
      /* parameter not used for the moment */
      PRINT_INFO("SOCKET_EVENT <connected_socket_id> = %ld",
                 ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    else
    {
      /* other parameters are ignored */
    }
    END_PARAM_LOOP()
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_DNSRSLV(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                    const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_modem_ctxt);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_DNSRSLV()")

  uint32_t ip_type = 0U;

  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    /* <ip_type>
     * 0 for IPv4, 1 for IPv6
     */
    ip_type = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
    PRINT_INFO("<ip_type> = %ld", ip_type);
  }
  else if (element_infos->param_rank == 3U)
  {
    /* <ip_addr> */
    /* if IPv4 address received, we check that received address format is valid
     * note: this test should not be necessary but a bug in current modem FW reports an IPv6
     *       address and indicates that it is an IPv4 address.
     */
    if (ip_type == 0U)
    {
      csint_ip_addr_info_t  ip_addr_info;
      (void) memset((void *)&ip_addr_info, 0, sizeof(csint_ip_addr_info_t));
      /* retrieve IP address value */
      (void) memcpy((void *) & (ip_addr_info.ip_addr_value),
                    (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                    (size_t) element_infos->str_size);

      /* check that address format is valid */
      if (atcm_get_ip_address_type((AT_CHAR_t *)&ip_addr_info.ip_addr_value) == CS_IPAT_IPV4)
      {
        /* recopy address to user */
        (void) memcpy((void *)type1sc_shared.DNSRSLV_dns_info.hostIPaddr,
                      (const void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
                      (size_t) element_infos->str_size);
      }
      else
      {
        PRINT_ERR("error, invalid IPv4 address format")
      }
    }
    else
    {
      PRINT_ERR("error, IPv6 address format not supported yet")
    }
    /* IPv6 format not supported yet */
  }
  else
  {
    /* other parameters are ignored */
  }
  END_PARAM_LOOP()

  return (retval);
}

at_action_rsp_t fRspAnalyze_PINGCMD(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                    const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);
  at_action_rsp_t retval;
#if (TYPE1SC_ACTIVATE_PING_REPORT == 1)
  retval = ATACTION_RSP_URC_FORWARDED; /* received a valid intermediate answer */
#else
  retval = ATACTION_RSP_INTERMEDIATE; /* because report is not managed actually */
#endif /* (TYPE1SC_ACTIVATE_PING_REPORT == 1) */
  PRINT_API("enter fRspAnalyze_PINGCMD()")

  /* intermediate ping response format:
  * %PINGCMD:<id>,<dest_ip>,<rtt>,<ttl>
  *  with:
  *    <id> = decimal, identifier of each individual reply (this can be 1 to <count>)
  *    <dest_ip> = string, destination IPv4 or IPv6 address
  *    <rtt> = decimal, Round Trip PING Time
  *    <ttl> = decimal, Time To Leave within the PING reply
  */

  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    /* new ping response: clear ping response structure */
    clear_ping_resp_struct(p_modem_ctxt);
    p_modem_ctxt->persist.ping_resp_urc.ping_status = CELLULAR_ERROR; /* will be updated if all params are correct */

    /* <id> */
    uint32_t ping_id = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                 element_infos->str_size);
    PRINT_DBG("<id> = %ld", ping_id)
#if (TYPE1SC_ACTIVATE_PING_REPORT == 1)
    p_modem_ctxt->persist.urc_avail_ping_rsp = AT_TRUE;
#endif /* (TYPE1SC_ACTIVATE_PING_REPORT == 1) */
    p_modem_ctxt->persist.ping_resp_urc.index = (uint8_t)ping_id;
    PRINT_DBG("intermediate ping report")
    p_modem_ctxt->persist.ping_resp_urc.is_final_report = CELLULAR_FALSE;
    /* Type1SC modem does not provide length, it's an input parameter */
    p_modem_ctxt->persist.ping_resp_urc.ping_size = TYPE1SC_PING_LENGTH;
  }
  else if (element_infos->param_rank == 3U)
  {
    /* <dest_ip> */
    atcm_extract_IP_address((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx],
                            (uint16_t) element_infos->str_size,
                            (uint8_t *) p_modem_ctxt->persist.ping_resp_urc.ping_addr);
  }
  else if (element_infos->param_rank == 4U)
  {
    /* <rtt> */
    uint32_t rtt = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                             element_infos->str_size);
    PRINT_DBG("<rtt> = %ld", rtt);
    p_modem_ctxt->persist.ping_resp_urc.time = (uint32_t)rtt;
  }
  else if (element_infos->param_rank == 5U)
  {
    /* <ttl> */
    uint32_t ttl = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                             element_infos->str_size);
    PRINT_INFO("<ttl> = %ld", ttl);
    p_modem_ctxt->persist.ping_resp_urc.ttl = (uint8_t)ttl;

    /* finally, after having received all parameters, set the final status for this ping */
    p_modem_ctxt->persist.ping_resp_urc.ping_status = CELLULAR_OK;
  }
  else
  {
    /* other parameters are ignored */
  }
  END_PARAM_LOOP()

  return (retval);
}

void clear_ping_resp_struct(atcustom_modem_context_t *p_modem_ctxt)
{
  /* clear CS_Ping_response_t structure parameters EXCEPT ping_resp_urc.index */
  p_modem_ctxt->persist.ping_resp_urc.ping_status = CELLULAR_ERROR;
  p_modem_ctxt->persist.ping_resp_urc.is_final_report = CELLULAR_FALSE;
  (void) memset((void *)&p_modem_ctxt->persist.ping_resp_urc.ping_addr[0], 0, MAX_SIZE_IPADDR);
  p_modem_ctxt->persist.ping_resp_urc.ping_size = 0U;
  p_modem_ctxt->persist.ping_resp_urc.time = 0U;
  p_modem_ctxt->persist.ping_resp_urc.ttl = 0U;
  /* p_modem_ctxt->persist.ping_resp_urc.index is unchanged */
}

/* *********************** */

/**
  * @brief  Convert a number to its ASCII value.
  * @param  nbr Number to convert.
  * @retval corresponding ASCII value.
  */
static uint8_t convertToASCII(uint8_t nbr)
{
  uint8_t ascii;

  if (nbr <= 9U)
  {
    ascii = nbr + 48U;
  }
  else
  {
    ascii = nbr + 87U; /* 87 = 97 -10 (where 97 correspond to 'a') */
  }
  return (ascii);
}

/**
  * @brief  Convert a character to its HEX value
  *         for example 'A' is converted to '65' where msd='6' and lsd='5'
  * @param  val Character to convert.
  * @param  p_msb ptr to Most significant Digit of HEX value
  * @param  p_lsb ptr to Less significant Digit of HEX value
  * @retval none.
  */
static void convertCharToHEX(uint8_t val, uint8_t *p_msd, uint8_t *p_lsd)
{
  *p_msd = convertToASCII(val / 16U);
  *p_lsd = convertToASCII(val % 16U);
}

/**
  * @brief  Convert an hexa number (from 0 to F) to its ASCII value
  * @param  digit Value to convert.
  * @param  p_res ptr to converted value.
  * @retval at_status_t.
  */
static at_status_t convertDigitToValue(uint8_t digit, uint8_t *p_res)
{
  at_status_t retval = ATSTATUS_OK;

  if ((digit >= 48U) && (digit <= 57U))
  {
    /* 0 to 9 */
    *p_res = digit - 48U;
  }
  else if ((digit >= 97U) && (digit <= 102U))
  {
    /* a to f */
    *p_res = digit - 87U; /* 87 = -97+10 */
  }
  else if ((digit >= 65U) && (digit <= 70U))
  {
    /* A to F */
    *p_res = digit - 55U; /* 55 = -65+10*/
  }
  else
  {
    *p_res = 0;
    retval = ATSTATUS_ERROR;
  }
  return (retval);
}

/**
  * @brief  Convert a HEX to its Char value
  *         for example '65' (where msd='6' and lsd='5') is converted to 'A'
  * @param  msd Most significant Digit of HEX value .
  * @param  lsd Less significant Digit of HEX value.
  * @param  p_conv ptr to converted Character value.
  * @retval at_status_t.
  */
static at_status_t convertHEXToChar(uint8_t msd, uint8_t lsd, uint8_t *p_conv)
{
  at_status_t retval = ATSTATUS_OK;
  uint8_t convMSD;
  uint8_t convLSD;

  /* convert Most significant digit */
  if (convertDigitToValue(msd, &convMSD) == ATSTATUS_OK)
  {
    /* convert Less significant digit */
    if (convertDigitToValue(lsd, &convLSD) == ATSTATUS_OK)
    {
      /* compute converted char value */
      *p_conv = (convMSD << 4) + convLSD;
    }
    else
    {
      retval = ATSTATUS_ERROR;
    }
  }
  else
  {
    retval = ATSTATUS_ERROR;
  }
  return (retval);
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/


