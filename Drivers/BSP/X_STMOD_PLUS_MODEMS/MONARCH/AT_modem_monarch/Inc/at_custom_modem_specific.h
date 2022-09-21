/**
  ******************************************************************************
  * @file    at_custom_modem_specific.h
  * @author  MCD Application Team
  * @brief   Header for at_custom_modem_specific.c module
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
#ifndef AT_CUSTOM_MODEM_MONARCH_H
#define AT_CUSTOM_MODEM_MONARCH_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "at_core.h"
#include "at_parser.h"
#include "at_modem_api.h"
#include "at_modem_signalling.h"
#include "cellular_service.h"
#include "cellular_service_int.h"
#include "ipc_common.h"

/** @addtogroup AT_CUSTOM AT_CUSTOM
  * @{
  */

/** @addtogroup AT_CUSTOM_SEQUANS_MONARCH AT_CUSTOM SEQUANS_MONARCH
  * @{
  */

/** @addtogroup AT_CUSTOM_SEQUANS_MONARCH_SPECIFIC AT_CUSTOM SEQUANS_MONARCH SPECIFIC
  * @{
  */

/** @defgroup AT_CUSTOM_SEQUANS_MONARCH_SPECIFIC_Exported_defines AT_CUSTOM SEQUANS_MONARCH SPECIFIC Exported defines
  * @{
  */
/* device specific parameters */
#define MODEM_MAX_SOCKET_TX_DATA_SIZE  CONFIG_MODEM_MAX_SOCKET_TX_DATA_SIZE
#define MODEM_MAX_SOCKET_RX_DATA_SIZE  CONFIG_MODEM_MAX_SOCKET_RX_DATA_SIZE
#define SEQUANS_PING_LENGTH            ((uint16_t)32U)
#define GM01Q_ACTIVATE_PING_REPORT     (1) /* temporary solution, ping is synchrone, report not supported by com */
#define ENABLE_SEQUANS_LOW_POWER_MODE  (0) /* not applicable yet for this modem - Do not activate !!! */

/* commands timetout values */
#define SEQUANS_WAIT_SIM_TEMPO (8000U) /* time (in ms) allocated to wait SIM ready during modem init */
#define SEQMONARCH_DEFAULT_TIMEOUT  ((uint32_t)15000)
#define SEQMONARCH_AT_TIMEOUT       ((uint32_t)1000U)  /* timeout for AT */
#define SEQMONARCH_CMD_TIMEOUT      ((uint32_t)15000)
#define SEQMONARCH_SYSSTART_TIMEOUT ((uint32_t)60000) /* Maximum time allowed to receive sysstart (= boot event)*/
#define SEQMONARCH_SHUTDOWN_TIMEOUT ((uint32_t)30000) /* Maximum time allowed to receive Shutdown URC */

#define SEQMONARCH_DATA_SUSPEND_TIMEOUT  ((uint32_t)2000)   /* time before to send escape command */
#define SEQMONARCH_ESCAPE_TIMEOUT   ((uint32_t)2000)   /* maximum time allowed to receive a response to an Esc cmd */
#define SEQMONARCH_COPS_TIMEOUT     ((uint32_t)180000) /* 180 sec */
#define SEQMONARCH_CGATT_TIMEOUT    ((uint32_t)140000U) /* 140 sec */
#define SEQMONARCH_CGACT_TIMEOUT    ((uint32_t)150000U) /* 150 sec */
#define SEQMONARCH_CFUN_TIMEOUT     ((uint32_t)40000)  /* 40 sec: so long because in case of factory reset, modem
                                                       * answer can be very long, certainly because it takes time
                                                       * to save NV( Non Volatile) data.
                                                       */
#define SEQMONARCH_SOCKET_PROMPT_TIMEOUT ((uint32_t)10000)
#define SEQMONARCH_SQNSD_TIMEOUT         ((uint32_t)150000) /* 150s */
#define SEQMONARCH_SQNSH_TIMEOUT         ((uint32_t)150000) /* 150s */
#define SEQMONARCH_RESTART_RADIO_TIMEOUT ((uint32_t)5000)

/**
  * @}
  */

/** @defgroup AT_CUSTOM_SEQUANS_MONARCH_SPECIFIC_Exported_Types AT_CUSTOM SEQUANS_MONARCH SPECIFIC Exported Types
  * @{
  */
enum
{
  /* modem specific commands */
  CMD_AT_RESET  = (CMD_AT_LAST_GENERIC + 1U),       /* hardware reset */
  CMD_AT_SQNCTM,      /* Conformance Test mode (not described in Monarch ref. manual but in Calliope ref. manual */
  CMD_AT_AUTOATT,     /* Automatic Attach */
  CMD_AT_CGDCONT_REPROGRAM, /* special case, reprogram CGDCONT */
  CMD_AT_CLEAR_SCAN_CFG,    /* clear bands */
  CMD_AT_ADD_SCAN_BAND,     /* add a band */
  CMD_AT_ICCID,       /* ICCID request */
  CMD_AT_SMST,        /* SIM test */
  CMD_AT_CESQ,        /* Extended signal quality */
  CMD_AT_SQNSSHDN,    /* Power Down Modem */

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  /* MODEM SPECIFIC COMMANDS USED FOR SOCKET MODE */
  CMD_AT_SQNSRING,       /* socket ring */
  CMD_AT_SQNSCFG,     /* socket configuration */
  CMD_AT_SQNSCFGEXT,  /* socket extended configuration */
  CMD_AT_SQNSD,       /* socket dial */
  CMD_AT_SQNSH,       /* socket shutdown */
  CMD_AT_SQNSI,       /* socket information  */
  CMD_AT_SQNSS,       /* socket status */
  CMD_AT_SQNSRECV,    /* socket, receive data in command mode */
  CMD_AT_SQNSSENDEXT, /* socket, extended send data in command mode (waiting for prompt)  */
  CMD_AT_SQNSSEND_WRITE_DATA, /* socket, send data in command mode (send data) */
  CMD_AT_PING,        /* Ping request */
  CMD_AT_SQNDNSLKUP,  /* DNS request */
  CMD_AT_SOCKET_PROMPT,
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */

  /* modem specific events (URC, BOOT, ...) */
  CMD_AT_WAIT_EVENT,
  CMD_AT_SIMREADY_EVENT,
  CMD_AT_SYSSTART_TYPE1,
  CMD_AT_SYSSTART_TYPE2,
  CMD_AT_SYSSHDN,
  CMD_AT_SHUTDOWN,

};

typedef struct
{
  AT_CHAR_t hostIPaddr[MAX_SIZE_IPADDR]; /* = host_name parameter from CS_DnsReq_t */
} ATCustom_MONARCH_dns_t;

typedef enum
{
  /* Command syntax
   * +SQNSRECV:<connId>,<maxByte>[,<IPaddr>,<rPort>]<CR><LF>
   *  <data>
   *  OK
   */
  RX_SQNSRECV_header_not_started,
  RX_SQNSRECV_header_prefix,       /* receiving prefix, ie +SQNSRECV: */
  RX_SQNSRECV_header_connId,       /* receiving connId parameter */
  RX_SQNSRECV_header_maxByte,      /* receiving maxByte parameter */
  RX_SQNSRECV_header_other,        /* receiving other parameter */

} atcustom_RxSQNSRECV_header_t;

#define MAXBYTE_MAXIMUM_SIZE ((uint8_t)8U)

typedef struct
{
  /* structure used to manage answer to AT+SQNSRECV in case crossing-cases, especially when
   *  +SQNSRING URC is received instead of +SQNSRECV answer.
   */
  uint16_t counter_header;   /* count number of characters received in header */
  uint16_t counter_maxByte;  /* count number of characters received for maxByte parameter */
  atcustom_RxSQNSRECV_header_t analyze_state; /* state of header analyze */
  AT_CHAR_t buf_maxByte[MAXBYTE_MAXIMUM_SIZE]; /* buffer to stote header characters */

} ATCustom_MONARCH_RxSQNSRECV_header_t;

typedef struct
{
  ATCustom_MONARCH_dns_t  SQNDNSLKUP_dns_info;   /* memorize infos received for DNS in +SQNDNSLKUP */
  uint8_t                 SMST_sim_error_status; /* memorize infos received for DNS in +SMST */
  bool                    waiting_for_ring_irq;
  ATCustom_MONARCH_RxSQNSRECV_header_t RxSQNSRECV_header_info; /* structure used to manage SQNSRECV answer */
} monarch_shared_variables_t;

/**
  * @}
  */

/** @defgroup AT_CUSTOM_SEQUANS_MONARCH_SPECIFIC_Exported_Variables
  *    AT_CUSTOM SEQUANS_MONARCH SPECIFIC Exported Variables
  * @{
  */
extern monarch_shared_variables_t monarch_shared;
/**
  * @}
  */

/** @defgroup AT_CUSTOM_SEQUANS_MONARCH_SPECIFIC_Exported_Functions
  *    AT_CUSTOM SEQUANS_MONARCH SPECIFIC Exported Functions
  * @{
  */
void        ATCustom_MONARCH_init(atparser_context_t *p_atp_ctxt);
uint8_t     ATCustom_MONARCH_checkEndOfMsgCallback(uint8_t rxChar);
at_status_t ATCustom_MONARCH_getCmd(at_context_t *p_at_ctxt, uint32_t *p_ATcmdTimeout);
at_endmsg_t ATCustom_MONARCH_extractElement(atparser_context_t *p_atp_ctxt,
                                            const IPC_RxMessage_t *p_msg_in,
                                            at_element_info_t *element_infos);
at_action_rsp_t ATCustom_MONARCH_analyzeCmd(at_context_t *p_at_ctxt,
                                            const IPC_RxMessage_t *p_msg_in,
                                            at_element_info_t *element_infos);
at_action_rsp_t ATCustom_MONARCH_analyzeParam(at_context_t *p_at_ctxt,
                                              const IPC_RxMessage_t *p_msg_in,
                                              at_element_info_t *element_infos);
at_action_rsp_t ATCustom_MONARCH_terminateCmd(atparser_context_t *p_atp_ctxt, at_element_info_t *element_infos);

at_status_t ATCustom_MONARCH_get_rsp(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf);
at_status_t ATCustom_MONARCH_get_urc(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf);
at_status_t ATCustom_MONARCH_get_error(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf);
at_status_t ATCustom_MONARCH_hw_event(sysctrl_device_type_t deviceType, at_hw_event_t hwEvent, GPIO_PinState gstate);

void ATC_Monarch_modem_reset(atcustom_modem_context_t *p_modem_ctxt);
void ATC_Monarch_reset_variables(void);
void ATC_Monarch_reinitSyntaxAutomaton_monarch(void);

at_bool_t ATC_Monarch_init_monarch_low_power(atcustom_modem_context_t *p_modem_ctxt);
at_bool_t ATC_Monarch_set_monarch_low_power(atcustom_modem_context_t *p_modem_ctxt);

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

#ifdef __cplusplus
}
#endif

#endif /* AT_CUSTOM_MODEM_MONARCH_H */

