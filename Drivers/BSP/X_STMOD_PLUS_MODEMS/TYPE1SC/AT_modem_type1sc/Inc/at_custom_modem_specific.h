/**
  ******************************************************************************
  * @file    at_custom_modem_specific.h
  * @author  MCD Application Team
  * @brief   Header for at_custom_modem_specific.c module for
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
#ifndef AT_CUSTOM_MODEM_TYPE1SC_H
#define AT_CUSTOM_MODEM_TYPE1SC_H

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

/** @addtogroup AT_CUSTOM_ALTAIR_T1SC AT_CUSTOM ALTAIR_T1SC
  * @{
  */

/** @addtogroup AT_CUSTOM_ALTAIR_T1SC_SPECIFIC AT_CUSTOM ALTAIR_T1SC SPECIFIC
  * @{
  */

/** @defgroup AT_CUSTOM_ALTAIR_T1SC_SPECIFIC_Exported_Defines AT_CUSTOM ALTAIR_T1SC SPECIFIC Exported Defines
  * @{
  */
#define MODEM_MAX_SOCKET_TX_DATA_SIZE   CONFIG_MODEM_MAX_SOCKET_TX_DATA_SIZE
#define MODEM_MAX_SOCKET_RX_DATA_SIZE   CONFIG_MODEM_MAX_SOCKET_RX_DATA_SIZE
#define TYPE1SC_PING_LENGTH             ((uint16_t)56U)
#define TYPE1SC_ACTIVATE_PING_REPORT    (1)

/* commands timetout values */
#define TYPE1SC_DEFAULT_TIMEOUT       ((uint32_t)15000)
#define TYPE1SC_CSQ_TIMEOUT           ((uint32_t)25000)
#define TYPE1SC_ESCAPE_TIMEOUT        ((uint32_t)3000)   /* max time allowed to receive a response to an Escape cmd */
#define TYPE1SC_COPS_TIMEOUT          ((uint32_t)180000) /* 180 sec */
#define TYPE1SC_CGATT_TIMEOUT         ((uint32_t)140000) /* 140 sec */
#define TYPE1SC_CGACT_TIMEOUT         ((uint32_t)150000) /* 150 sec */
#define TYPE1SC_ATH_TIMEOUT           ((uint32_t)90000)  /* 90 sec */
#define TYPE1SC_AT_TIMEOUT            ((uint32_t)1000)   /* timeout for AT */
#define TYPE1SC_SOCKET_PROMPT_TIMEOUT ((uint32_t)10000)
#define TYPE1SC_ATK_TIMEOUT           ((uint32_t)3000)   /* initially set to 1 sec but seems not enough sometimes... */
#define TYPE1SC_CPSMS_TIMEOUT         ((uint32_t)60000)
#define TYPE1SC_CEDRX_TIMEOUT         ((uint32_t)60000)
#define TYPE1SC_BOOTEV_TIMEOUT        ((uint32_t)5000) /* maxiboot time allowed to wait %BOOTEV*/
#define TYPE1SC_SIMREADY_TIMEOUT      ((uint32_t)3000U)

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
#define TYPE1SC_DNSRSLV_TIMEOUT       ((uint32_t)60000)  /* 60 sec */
#define TYPE1SC_PING_TIMEOUT          ((uint32_t)15000)  /* 15 sec */
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)*/

#define TYPE1SC_MODEM_SYNCHRO_AT_MAX_RETRIES ((uint8_t)30U)
#define TYPE1SC_MAX_SIM_STATUS_RETRIES  ((uint8_t)20U) /* maximum number of retries to wait SIM ready */

/**
  * @}
  */

/** @defgroup AT_CUSTOM_ALTAIR_T1SC_SPECIFIC_Exported_Types AT_CUSTOM ALTAIR_T1SC SPECIFIC Exported Types
  * @{
  */
enum
{
  /* modem specific commands */
  CMD_AT_PDNSET = (CMD_AT_LAST_GENERIC + 1U), /* set run-time PDN parameters for data PDNs */
  CMD_AT_CCID,                                /* read the ICCID from SIM */
  CMD_AT_AND_K3,                              /* flow control activated (FAST UART) */
  CMD_AT_AND_K0,                              /* flow control deactivated */
  CMD_AT_SETCFG,                              /* set Modem Normal configuration parameters */
  CMD_AT_GETCFG,                              /* get Modem Normal configuration parameters */
  CMD_AT_SETACFG,                             /* set Modem Advanced configuration parameters */
  CMD_AT_GETACFG,                             /* get Modem Advanced configuration parameters */
  CMD_AT_SETBDELAY,                           /* set modem boot delay */
  CMD_AT_BOOTEV,                              /* notify about modem boot or reset events */
  CMD_AT_PDNRDP,                              /* get info for active PDN */
  CMD_AT_SETSYSCFG,                           /* set Modem system configuration parameters */
  CMD_AT_GETSYSCFG,                           /* get Modem system configuration parameters */
  CMD_AT_NOTIFYEV,                            /* notify host about important events */

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  CMD_AT_PDNACT,                              /* activate or deactivate a specific PDN */
  CMD_AT_SOCKETCMD_ALLOCATE,                  /* allocate socket session */
  CMD_AT_SOCKETCMD_ACTIVATE,                  /* activate the predefined socket ID */
  CMD_AT_SOCKETCMD_INFO,                      /* returns the details of the specific socket ID */
  CMD_AT_SOCKETCMD_DEACTIVATE,                /* req to deactivate the specific socket ID and release its resources */
  CMD_AT_SOCKETCMD_FASTEND,                   /* activates the predefined socket ID,
                                               * writes to the socket and deactivates it */
  CMD_AT_SOCKETCMD_DELETE,                    /* delete the specific socket ID */
  CMD_AT_SOCKETCMD_LASTERROR,                 /* get last socket error code */
  CMD_AT_SOCKETCMD_CONFSEND,                  /* similar to FASTSEND with additions */
  CMD_AT_SOCKETCMD_SSLINFO,                   /* */
  CMD_AT_SOCKETDATA_SEND,                     /* send DATA */
  CMD_AT_SOCKETDATA_RECEIVE,                  /* received DATA */
  CMD_AT_SOCKETEV,                            /* notify about socket events */
  CMD_AT_DNSRSLV,                             /* resolve a specific domain name */
  CMD_AT_PINGCMD,                             /* execute PING services */
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)*/

};

typedef enum
{
  SETGETCFG_UNDEFINED,
  SETGETCFG_SIM_POLICY,  /* Normal command (setcfg/getcfg) */
  SETGETCFG_BAND,        /* Normal command (setcfg/getcfg) */
  SETGETCFG_OPER,        /* Normal command (setcfg/getcfg) */
  SETGETCFG_HIFC_MODE,   /* Advanced command (setacfg/getacfg) */
  SETGETCFG_PMCONF_SLEEP_MODE,   /* Advanced command (setacfg/getacfg) */
  SETGETCFG_PMCONF_MAX_ALLOWED,  /* Advanced command (setacfg/getacfg) */
  SETGETCFG_BOOT_EVENT_TRUE,          /* Advanced command (setacfg/getacfg) */
  SETGETCFG_BOOT_EVENT_FALSE,         /* Advanced command (setacfg/getacfg) */
  SETGETCFG_UART_FLOW_CONTROL,        /* Advanced command (setacfg/getacfg) */
} ATCustom_TYPE1SC_SETGETCFG_t;

typedef enum
{
  SETGETSYSCFG_UNDEFINED,
  SETGETSYSCFG_SIM_POLICY,
  SETGETSYSCFG_BAND,        /* not implemented yet */
} ATCustom_T1SC_SETGETSYSCFG_t;

typedef struct
{
  AT_CHAR_t hostIPaddr[MAX_SIZE_IPADDR]; /* = host_name parameter from CS_DnsReq_t */
} ATCustom_TYPE1SC_dns_t;

#if (ENABLE_T1SC_LOW_POWER_MODE != 0U)
typedef enum
{
  HOST_LP_STATE_IDLE,               /* state 0: */
  HOST_LP_STATE_LP_REQUIRED,        /* state 1: host required modem sleep mode */
  HOST_LP_STATE_LP_CONFIRMED,       /* state 2: modem confirmed and entered in sleep mode */
  HOST_LP_STATE_LP_ONGOING,         /* state 3: host entered in sleep mode */
  HOST_LP_STATE_WAKEUP_REQUIRED,    /* state 4: host waiting wakeup confirmation from modem */
  HOST_LP_STATE_ERROR,              /* state 3 */
} ATCustom_T1SC_Host_LP_state_t;

typedef enum
{
  EVENT_LP_HOST_SLEEP_REQ,        /* event 0 */
  EVENT_LP_HOST_SLEEP_CANCEL,     /* event 1 */
  EVENT_LP_HOST_SLEEP_COMPLETE,   /* event 2 */
  EVENT_LP_HOST_WAKEUP_REQ,       /* event 3 */
  EVENT_LP_MDM_RING,              /* event 4: used by modem to confirm sleep mode or to request wakeup */
} ATCustom_T1SC_LP_event_t;
#endif /* (ENABLE_T1SC_LOW_POWER_MODE != 0U) */

typedef struct
{
  uint16_t                      sim_status_retries;
  ATCustom_TYPE1SC_dns_t        DNSRSLV_dns_info;      /* memorize infos received for DNS in %DNSRSLV */
  at_bool_t                     SocketCmd_Allocated_SocketID;     /* allocated socket ID received */
  at_bool_t                     SocketCmd_Allocated_SocketID_OK;  /* allocated socket ID confirmed (OK) */
  at_bool_t                     SocketCmd_Activated;              /* socket activated successfully */
  at_bool_t                     SocketCmd_Delete_success;         /* socket deleted successfully */
  ATCustom_TYPE1SC_SETGETCFG_t  getcfg_function;
  ATCustom_TYPE1SC_SETGETCFG_t  setcfg_function;
  ATCustom_T1SC_SETGETSYSCFG_t  syscfg_function;      /* used for GETSYSCG and SETSYSCFG */
  bool                          modem_waiting_for_bootev;
  bool                          modem_bootev_received;
  uint8_t                       notifyev_mode; /* define which NOTIFYEV are requested */
  bool                          modem_sim_same_as_selected;

#if (ENABLE_T1SC_LOW_POWER_MODE != 0U)
  /* low-power variables */
  ATCustom_T1SC_Host_LP_state_t   host_lp_state;         /* to manage automaton Host Low Power state */
#endif /* (ENABLE_T1SC_LOW_POWER_MODE != 0U) */

} type1sc_shared_variables_t;

/**
  * @}
  */

/** @defgroup AT_CUSTOM_ALTAIR_T1SC_SPECIFIC_Exported_Variables AT_CUSTOM ALTAIR_T1SC SPECIFIC Exported Variables
  * @{
  */
extern type1sc_shared_variables_t type1sc_shared;
/**
  * @}
  */

/** @defgroup AT_CUSTOM_ALTAIR_T1SC_SPECIFIC_Exported_Functions AT_CUSTOM ALTAIR_T1SC SPECIFIC Exported Functions
  * @{
  */
/* generic functions exported */
void        ATCustom_TYPE1SC_init(atparser_context_t *p_atp_ctxt);
uint8_t     ATCustom_TYPE1SC_checkEndOfMsgCallback(uint8_t rxChar);
at_status_t ATCustom_TYPE1SC_getCmd(at_context_t *p_at_ctxt, uint32_t *p_ATcmdTimeout);
at_endmsg_t ATCustom_TYPE1SC_extractElement(atparser_context_t *p_atp_ctxt,
                                            const IPC_RxMessage_t *p_msg_in,
                                            at_element_info_t *element_infos);
at_action_rsp_t ATCustom_TYPE1SC_analyzeCmd(at_context_t *p_at_ctxt,
                                            const IPC_RxMessage_t *p_msg_in,
                                            at_element_info_t *element_infos);
at_action_rsp_t ATCustom_TYPE1SC_analyzeParam(at_context_t *p_at_ctxt,
                                              const IPC_RxMessage_t *p_msg_in,
                                              at_element_info_t *element_infos);
at_action_rsp_t ATCustom_TYPE1SC_terminateCmd(atparser_context_t *p_atp_ctxt, at_element_info_t *element_infos);
at_status_t ATCustom_TYPE1SC_get_rsp(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf);
at_status_t ATCustom_TYPE1SC_get_urc(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf);
at_status_t ATCustom_TYPE1SC_get_error(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf);
at_status_t ATCustom_TYPE1SC_hw_event(sysctrl_device_type_t deviceType, at_hw_event_t hwEvent, GPIO_PinState gstate);

/* functions exported for internal purpose */
void ATC_TYPE1SC_modem_reset(atcustom_modem_context_t *p_modem_ctxt);
void ATC_TYPE1SC_reset_variables(void);
void ATC_TYPE1SC_reinitSyntaxAutomaton(void);
void ATC_TYPE1SC_modem_init(atcustom_modem_context_t *p_modem_ctxt);

#if (ENABLE_T1SC_LOW_POWER_MODE != 0U)
at_bool_t ATC_TYPE1SC_init_low_power(atcustom_modem_context_t *p_modem_ctxt);
at_bool_t ATC_TYPE1SC_set_low_power(atcustom_modem_context_t *p_modem_ctxt);
void ATC_TYPE1SC_low_power_event(ATCustom_T1SC_LP_event_t event, bool called_under_it);
#endif /* (ENABLE_T1SC_LOW_POWER_MODE != 0U) */

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

#endif /* AT_CUSTOM_MODEM_TYPE1SC_H */
