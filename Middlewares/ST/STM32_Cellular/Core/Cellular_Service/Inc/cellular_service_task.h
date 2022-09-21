/**
  ******************************************************************************
  * @file    cellular_service_task.h
  * @author  MCD Application Team
  * @brief   Header for cellular_service_task.c module
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
#ifndef CELLULAR_SERVICE_TASK_H
#define CELLULAR_SERVICE_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

#include "plf_config.h"

#include "cellular_control_api.h"
#include "cellular_service.h"
#include "cellular_service_datacache.h"
#include "cellular_service_config.h"

/* Exported constants --------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* type of message to send to automaton task */
typedef uint32_t cst_message_t;

typedef uint16_t CST_message_type_t;

/* Cellular service automaton states */
typedef uint16_t CST_autom_state_t ;

/* NFMC context */
/* Note: NFMC Network-friendly Management Configuration       */
/*       Normalized network attachment temporisations         */
typedef struct
{
  bool      active;                      /* NFMC enable */
  bool      nfmc_timer_on_going;         /* NFMC timer on going  */
  uint32_t  tempo[CA_NFMC_VALUES_MAX_NB];    /* NFMC tempo list          */
} cst_nfmc_context_t;

/* List of modem failure causes   */
typedef enum
{
  CST_NO_FAIL,
  CST_MODEM_POWER_ON_FAIL,
  CST_MODEM_RESET_FAIL,
  CST_MODEM_CSQ_FAIL,
  CST_MODEM_REGISTER_FAIL,
  CST_MODEM_NETWORK_FAIL,
  CST_MODEM_ATTACH_FAIL,
  CST_MODEM_PDP_DEFINE_FAIL,
  CST_MODEM_SIM_FAIL,
  CST_PPP_FAIL
} cst_fail_cause_t;

/* Cellular context */
typedef struct
{
  CST_autom_state_t    current_state;                    /* automaton current state                                 */
  cst_fail_cause_t     fail_cause;                       /* cause of modem connection failure                       */
  CS_PDN_event_t       pdn_status;                       /* PDN status when modem callback is called                */
  CS_SignalQuality_t   signal_quality;                   /* current signal quality                                  */
  CS_NetworkRegState_t current_EPS_NetworkRegState;      /* current EPS network state                               */
  CS_NetworkRegState_t current_GPRS_NetworkRegState;     /* current GPRS network state                              */
  CS_NetworkRegState_t current_CS_NetworkRegState;       /* current CS network state                                */
  uint16_t             activate_pdn_nfmc_tempo_count;    /* current NFMC tempo count (if NFMC enabled)              */
  uint16_t             register_retry_tempo_count;       /* number of network register retry                        */
  uint8_t              sim_slot_index;                   /* index of sim slot used                                  */
  bool                 modem_on;                         /* Current power status of the modem false : off true : on */

  /* failing counters BEGIN */
  uint8_t             power_on_reset_count;
  uint8_t             reset_count;
  uint8_t             csq_reset_count;
  uint8_t             attach_reset_count;
  uint8_t             activate_pdn_reset_count;
  uint8_t             register_reset_count;
  uint8_t             network_reset_count;
  uint8_t             sim_reset_count;
  uint8_t             ppp_fail_count;
  uint8_t             cellular_data_retry_count;
  uint8_t             global_retry_count;
  uint8_t             csq_count_fail;
  /* failing counters END */
} cst_context_t;

#if (USE_TRACE_CELLULAR_SERVICE == 1U)
#if (USE_PRINTF == 0U)
#define PRINT_CELLULAR_SERVICE(format, args...)       \
  TRACE_PRINT(DBG_CHAN_CELLULAR_SERVICE, DBL_LVL_P0, format, ## args)
#define PRINT_CELLULAR_SERVICE_ERR(format, args...)   \
  TRACE_PRINT(DBG_CHAN_CELLULAR_SERVICE, DBL_LVL_ERR, "ERROR " format, ## args)
#else
#define PRINT_CELLULAR_SERVICE(format, args...)       (void)printf(format, ## args);
#define PRINT_CELLULAR_SERVICE_ERR(format, args...)   (void)printf(format, ## args);
#endif /* (USE_PRINTF == 0U) */
#else
#define PRINT_CELLULAR_SERVICE(...)        __NOP(); /* Nothing to do */
#define PRINT_CELLULAR_SERVICE_ERR(...)    __NOP(); /* Nothing to do */
#endif /* (USE_TRACE_CELLULAR_SERVICE == 1U) */

/* Set automaton message */
#define SET_AUTOMATON_MSG_TYPE(msg, type) ((msg) = ((msg)&0xFFFF0000U) | ((cst_message_t)(type)))
#define SET_AUTOMATON_MSG_ID(msg, id)     ((msg) = ((msg)&0x0000FFFFU) | (((cst_message_t)(id))<<16))
/* Get automaton message */
#define GET_AUTOMATON_MSG_TYPE(msg)       ((CST_message_type_t)((msg)&0x0000FFFFU))
#define GET_AUTOMATON_MSG_ID(msg)         ((cst_autom_event_t)(((msg)&0xFFFF0000U)>>16))

/* Max of restarts allowed  after failure for each cause */
#define CST_POWER_ON_RESET_MAX      5U
#define CST_RESET_MAX               5U
#define CST_CSQ_MODEM_RESET_MAX     5U
#define CST_ATTACH_RESET_MAX        5U
#define CST_DEFINE_PDN_RESET_MAX    5U
#define CST_REGISTER_RESET_MAX      5U
#define CST_NETWORK_RESET_MAX       5U
#define CST_SIM_RETRY_MAX           5U
#define CST_GLOBAL_RETRY_MAX        5U
#define CST_PPP_FAIL_MAX            5U

/* bad RSSI value (Signal Quality) */
#define CST_BAD_SIG_RSSI 99U

#define CST_COUNT_FAIL_MAX (5U)  /* Max of total restarts allowed after failure*/

/* ================================= */
/* List of automation states - BEGIN */
/* ================================= */
/* Warning : to be consistent with CST_StateName table in cellular_service_task.c */

/* wait platform init is done, then initialize Cellular service, and get boot modem information */
#define CST_BOOT_STATE                                  (CST_autom_state_t)0U

/* power on or off the modem according to modem target, and wait modem is initialized, and sim is initialized */
#define CST_MODEM_INIT_STATE                            (CST_autom_state_t)1U

/* Ask for network register and network attach, change to state WAITING_FOR_SIGNAL_QUALITY_OK */
#define CST_MODEM_READY_STATE                           (CST_autom_state_t)2U

/* Modem is ready, pool signal quality and continue when quality is OK */
#define CST_WAITING_FOR_SIGNAL_QUALITY_OK_STATE         (CST_autom_state_t)3U

/* Test if register to network. When registered, continue */
#define CST_WAITING_FOR_NETWORK_STATUS_STATE            (CST_autom_state_t)4U

/* Test if attached to network. When attached, continue */
#define CST_NETWORK_STATUS_OK_STATE                     (CST_autom_state_t)5U

/* Signal quality and registered and attached OK. Activate PDN */
#define CST_MODEM_REGISTERED_STATE                      (CST_autom_state_t)6U

/* Modem attached OK, wait for pdp activated */
#define CST_MODEM_PDN_ACTIVATING_STATE                  (CST_autom_state_t)7U

/* PDN Activated (eventually PPP Config done). Data is ready */
#define CST_MODEM_DATA_READY_STATE                      (CST_autom_state_t)8U

/* FOTA state : modem firmware update on going */
#define CST_MODEM_REPROG_STATE                          (CST_autom_state_t)9U

/* Maximum error number reached: Automaton remain in this state until CST_REBOOT_MODEM_EVENT event */
#define CST_MODEM_FAIL_STATE                            (CST_autom_state_t)10U

/* Sim Connected but Flight mode. Nwk attachment and DataReady will not be reached */
#define CST_MODEM_SIM_ONLY_STATE                        (CST_autom_state_t)11U

/* Reset CST, then change to state CST_MODEM_INIT_STATE to restart modem sim connection */
#define CST_MODEM_RESET_STATE                           (CST_autom_state_t)12U

/* Power off the modem, then change to state CST_MODEM_INIT_STATE to restart the modem */
#define CST_MODEM_REBOOT_STATE                          (CST_autom_state_t)13U

/* When target state of modem is "modem off", this state is reached after BOOT and MODEM_INIT states */
#define CST_MODEM_OFF_STATE                             (CST_autom_state_t)14U

/* State used in the special case where the modem is powered on alone without network attachment or register */
/* only a reset of the board may exit from this state */
#define CST_MODEM_POWER_ON_ONLY_STATE                   (CST_autom_state_t)15U

/* Modem il low power state */
#define CST_MODEM_POWER_DATA_IDLE_STATE                 (CST_autom_state_t)16U

/* when new apn config requested by application, change to this state, store new APN parameters in data cache, */
/* and change to CST_MODEM_RESET_STATE state to reset the modem to be reconfigured with new APN */
#define CST_APN_CONFIG_STATE                            (CST_autom_state_t)17U

/* PDN Activated - PPP config in Progress */
#define CST_PPP_CONFIG_ON_GOING_STATE                   (CST_autom_state_t)18U

/* PDN Activated - PPP close in Progress */
#define CST_PPP_CLOSE_ON_GOING_STATE                    (CST_autom_state_t)19U

/* Numder of states */
#define CST_MAX_STATE                                   20U

/* ================================= */
/* List of automation states - END   */
/* ================================= */

/* automaton event ID */
typedef uint16_t cst_autom_event_t;

/* ================================= */
/* List of automation events - BEGIN */
/* ================================= */
/* Warning : to be consistent with cst_event_name table in cellular_service_task.c */
#define CST_BOOT_EVENT                             (cst_autom_event_t)0U  /* init */
#define CST_MODEM_INIT_EVENT                       (cst_autom_event_t)1U  /* modem init */
#define CST_MODEM_READY_EVENT                      (cst_autom_event_t)2U  /* modem ready */
#define CST_SIGNAL_QUALITY_TO_CHECK_EVENT          (cst_autom_event_t)3U  /* signal quality updated */
#define CST_NETWORK_STATUS_TO_CHECK_EVENT          (cst_autom_event_t)4U  /* network status to check */
#define CST_NETWORK_STATUS_OK_EVENT                (cst_autom_event_t)5U  /* network status ok */
#define CST_MODEM_ATTACHED_EVENT                   (cst_autom_event_t)6U  /* modem attached */
#define CST_PDP_ACTIVATED_EVENT                    (cst_autom_event_t)7U  /* PDN activated */
#define CST_PDN_STATUS_TO_CHECK_EVENT              (cst_autom_event_t)8U  /* PDN status to check */
#define CST_PDN_ACTIVATE_RETRY_TIMER_EVENT         (cst_autom_event_t)9U  /* timer to retry PDN activation reached */
#define CST_CELLULAR_DATA_FAIL_EVENT               (cst_autom_event_t)10U  /* cellular data fail */
#define CST_POLLING_TIMER_EVENT                    (cst_autom_event_t)11U  /* time to poll modem */
#define CST_MODEM_URC_EVENT                        (cst_autom_event_t)12U  /* modem URC received */
#define CST_NO_EVENT                               (cst_autom_event_t)13U  /* no event to process */
#define CST_CMD_UNKWOWN_EVENT                      (cst_autom_event_t)14U  /* unknown cmd received */
#define CST_TARGET_STATE_CMD_EVENT                 (cst_autom_event_t)15U  /* modem target state request */
#define CST_APN_CONFIG_EVENT                       (cst_autom_event_t)16U  /* new apn config request */
#define CST_REBOOT_MODEM_EVENT                     (cst_autom_event_t)17U  /* reboot modem */
#define CST_MODEM_POWER_ON_ONLY_EVENT              (cst_autom_event_t)18U  /* modem power on request */
#define CST_NETWORK_CALLBACK_EVENT                 (cst_autom_event_t)19U  /* cellular callback  */
#define CST_NW_REG_TIMEOUT_TIMER_EVENT             (cst_autom_event_t)20U  /* register timeout reached */
#define CST_FOTA_START_EVENT                       (cst_autom_event_t)21U  /* reboot modem */
#define CST_FOTA_END_EVENT                         (cst_autom_event_t)22U  /* FOTA end */
#define CST_FOTA_TIMEOUT_EVENT                     (cst_autom_event_t)23U  /* FOTA timeout */
#define CST_MODEM_RESET_EVENT                      (cst_autom_event_t)24U  /* modem reset requested */
#define CST_MODEM_REBOOT_EVENT                     (cst_autom_event_t)25U  /* modem reboot requested */
#define CST_PPP_OPENED_EVENT                       (cst_autom_event_t)26U  /* ppp opened */
#define CST_PPP_CLOSED_EVENT                       (cst_autom_event_t)27U  /* ppp closed */
#define CST_MODEM_POWER_DOWN_EVENT                 (cst_autom_event_t)28U  /* modem power down event */
#define CST_SIM_RESET_EVENT                        (cst_autom_event_t)29U  /* SIM Change (refresh or reset, insertion,*/
/*                                                                                        removal)  event */
#define CST_POWER_STATUS_CALLBACK_EVENT            (cst_autom_event_t)30U  /* New low power parameter */
#if (USE_LOW_POWER == 0)
#define CST_MAX_EVENT                              31U
#else /* (USE_LOW_POWER == 1) */
#define CST_POWER_SLEEP_TIMEOUT_EVENT              (cst_autom_event_t)31U  /* low power entry timeout */
#define CST_POWER_SLEEP_REQUEST_EVENT              (cst_autom_event_t)32U  /* low power request */
#define CST_POWER_SLEEP_COMPLETE_EVENT             (cst_autom_event_t)33U  /* low power completed */
#define CST_POWER_WAKEUP_EVENT                     (cst_autom_event_t)34U  /* exit from low power Host wakeup */
#define CST_POWER_MODEM_WAKEUP_EVENT               (cst_autom_event_t)35U  /* exit from low power Modem wakeup */
#define CST_POWER_SLEEP_ABORT_EVENT                (cst_autom_event_t)36U  /* low power request abort */
#define CST_LP_INACTIVITY_TIMER_EVENT              (cst_autom_event_t)37U  /* low power request from network */
#define CST_MAX_EVENT                              38U


#endif /* (USE_LOW_POWER == 1) */

/* ================================= */
/* List of automation events - END   */
/* ================================= */

/* Maximum number of MCC/MNC - APN association */
#define MCCMNC_APN_MAX 5U

/* Maximum number of ICCID head - APN association */
#define ICCIDHEAD_APN_MAX 2U

/* list of event messages to send to cellular automaton task */
#define CST_MESSAGE_CS_EVENT              ((CST_message_type_t)0U)       /* CS: Cellular Service */
#define CST_MESSAGE_DC_EVENT              ((CST_message_type_t)1U)       /* DC: Data Cache       */
#define CST_MESSAGE_CMD                   ((CST_message_type_t)2U)       /* Command              */

/* Network registered status */
typedef uint16_t cst_network_status_t;
#define CST_NET_REGISTERED     (cst_network_status_t)0U   /* registered to network     */
#define CST_NOT_REGISTERED     (cst_network_status_t)1U   /* not registered to network */
#define CST_NET_STATUS_ERROR   (cst_network_status_t)2U   /* network status error      */
#define CST_NET_UNKNOWN        (cst_network_status_t)3U   /* network unknown just ignore*/

/* External variables --------------------------------------------------------*/
extern bool CST_polling_active;            /* modem polling activation flag */
#if (( USE_TRACE_CELLULAR_SERVICE == 1) || ( USE_CMD_CONSOLE == 1 ))
extern const uint8_t *CST_StateName[CST_MAX_STATE];
#endif /* (( USE_TRACE_CELLULAR_SERVICE == 1) || ( USE_CMD_CONSOLE == 1 )) */

/* Global variables ----------------------------------------------------------*/

/* DC structures */
extern dc_cellular_info_t         cst_cellular_info;           /* cellular infos               */
extern dc_sim_info_t              cst_sim_info;                /* sim infos                    */
extern dc_cellular_data_info_t    cst_cellular_data_info;      /* cellular data infos          */
extern dc_cellular_params_t       cst_cellular_params;         /* cellular configuration param */

extern cst_context_t cst_context;

/* NFMC context */
extern cst_nfmc_context_t cst_nfmc_context;                    /* NFMC context                 */

/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */

/**
  * @brief  allows to set cellular service automaton current state
  * @param new_state - automaton stat
  * @retval -
  */
void CST_set_state(CST_autom_state_t new_state);

/**
  * @brief  allows to get cellular service automaton current state
  * @param  -
  * @retval CST_autom_state_t - automaton state
  */
CST_autom_state_t CST_get_state(void);

/**
  * @brief  sends message to cellular service task
  * @param  type   - message type
  * @param  event  - event/command
  * @retval -
  */
void  CST_send_message(CST_message_type_t type, cst_autom_event_t event);

/**
  * @brief  allows to set radio on: start cellular automaton
  * @param  -
  * @retval CS_Status_t - return code
  */
CS_Status_t CST_radio_on(void);

/**
  * @brief  allows to boot modem only without network register
  * @note   the application is not started
  * @param  -
  * @retval CS_Status_t - return code
  */
CS_Status_t CST_modem_power_on(void);

/**
  * @brief  initializes cellular service component
  * @param  -
  * @retval CS_Status_t - return code
  */
CS_Status_t CST_cellular_service_init(void);

/**
  * @brief  starts cellular service component
  * @note   cellular service task automaton and tempos are started
  * @param  -
  * @retval CS_Status_t - return code
  */
CS_Status_t CST_cellular_service_start(void);

/**
  * @brief  allows to get the modem IP addr
  * @note   to be able to return an IP address the modem must be in data transfer state
  * @note   else an error is returned
  * @param  ip_addr_type - type of IP address
  * @param  p_ip_addr_value - IP address value returned by the function
  * @retval CS_Status_t - return code
  */
CS_Status_t CST_get_dev_IP_address(CS_IPaddrType_t *ip_addr_type, CS_CHAR_t *p_ip_addr_value);

#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_SERVICE_TASK_H */

