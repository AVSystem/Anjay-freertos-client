/**
  ******************************************************************************
  * @file    cellular_service_task.h
  * @author  MCD Application Team
  * @brief   Header for cellular_service_task.c module
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
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
#include "plf_config.h"
#include "cellular_service.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported constants --------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/


/* Cellular service automaton states */
typedef uint16_t CST_autom_state_t ;

/* ================================= */
/* List of automation states - BEGIN */
/* ================================= */
#define CST_BOOT_STATE                                  (CST_autom_state_t)0U
#define CST_MODEM_INIT_STATE                            (CST_autom_state_t)1U
#define CST_MODEM_READY_STATE                           (CST_autom_state_t)2U
#define CST_WAITING_FOR_SIGNAL_QUALITY_OK_STATE         (CST_autom_state_t)3U
#define CST_WAITING_FOR_NETWORK_STATUS_STATE            (CST_autom_state_t)4U
#define CST_NETWORK_STATUS_OK_STATE                     (CST_autom_state_t)5U
#define CST_MODEM_REGISTERED_STATE                      (CST_autom_state_t)6U
#define CST_MODEM_PDN_ACTIVATING_STATE                  (CST_autom_state_t)7U
#define CST_MODEM_DATA_READY_STATE                      (CST_autom_state_t)8U
#define CST_MODEM_REPROG_STATE                          (CST_autom_state_t)9U
#define CST_MODEM_FAIL_STATE                            (CST_autom_state_t)10U
#define CST_MODEM_SIM_ONLY_STATE                        (CST_autom_state_t)11U
#define CST_MODEM_RESET_STATE                           (CST_autom_state_t)12U
#define CST_MODEM_OFF_STATE                             (CST_autom_state_t)13U
#define CST_MODEM_POWER_ON_ONLY_STATE                   (CST_autom_state_t)14U
#define CST_MODEM_POWER_DATA_IDLE_STATE                 (CST_autom_state_t)15U
#define CST_APN_CONFIG_STATE                            (CST_autom_state_t)16U

#define CST_MAX_STATE                                   17U
/* ================================= */
/* List of automation states - END   */
/* ================================= */

/* automaton event type */
typedef uint16_t CST_autom_event_t;

/* ================================= */
/* List of automation events - BEGIN */
/* ================================= */

#define CST_BOOT_EVENT                             (CST_autom_event_t)0U  /* init */
#define CST_MODEM_INIT_EVENT                       (CST_autom_event_t)1U  /* modem init */
#define CST_MODEM_READY_EVENT                      (CST_autom_event_t)2U  /* modem ready */
#define CST_SIGNAL_QUALITY_TO_CHECK_EVENT          (CST_autom_event_t)3U  /* signal quality updated */
#define CST_NETWORK_STATUS_TO_CHECK_EVENT          (CST_autom_event_t)4U  /* network status to check */
#define CST_NETWORK_STATUS_OK_EVENT                (CST_autom_event_t)5U  /* network status ok */
#define CST_MODEM_ATTACHED_EVENT                   (CST_autom_event_t)6U  /* modem attached */
#define CST_PDP_ACTIVATED_EVENT                    (CST_autom_event_t)7U  /* PDN activated */
#define CST_PDN_STATUS_TO_CHECK_EVENT              (CST_autom_event_t)8U  /* PDN status to check */
#define CST_PDN_ACTIVATE_RETRY_TIMER_EVENT         (CST_autom_event_t)9U  /* timer to retry PDN activation reached */
#define CST_CELLULAR_DATA_FAIL_EVENT               (CST_autom_event_t)10U  /* cellular data fail */
#define CST_FAIL_EVENT                             (CST_autom_event_t)11U  /* modem fail */
#define CST_POLLING_TIMER_EVENT                    (CST_autom_event_t)12U  /* time to poll modem */
#define CST_MODEM_URC_EVENT                        (CST_autom_event_t)13U  /* modem URC received */
#define CST_NO_EVENT                               (CST_autom_event_t)14U  /* no event to process */
#define CST_CMD_UNKWOWN_EVENT                      (CST_autom_event_t)15U  /* unknown cmd reveived */
#define CST_TARGET_STATE_CMD_EVENT                 (CST_autom_event_t)16U  /* modem target state request */
#define CST_APN_CONFIG_EVENT                       (CST_autom_event_t)17U  /* new apn config rerquest */
#define CST_NIFMAN_EVENT                           (CST_autom_event_t)18U  /* NIFMAN NETWORK INTERFACE event */
#define CST_REBOOT_MODEM_EVENT                     (CST_autom_event_t)19U  /* reboot modem */
#define CST_MODEM_POWER_ON_ONLY_EVENT              (CST_autom_event_t)20U  /* modem power on request */
#define CST_NETWORK_CALLBACK_EVENT                 (CST_autom_event_t)21U  /* cellular callback  */
#define CST_NW_REG_TIMEOUT_TIMER_EVENT             (CST_autom_event_t)22U  /* register timeout reached */
#define CST_FOTA_START_EVENT                       (CST_autom_event_t)23U  /* reboot modem */
#define CST_FOTA_END_EVENT                         (CST_autom_event_t)24U  /* FOTA end */
#define CST_FOTA_TIMEOUT_EVENT                     (CST_autom_event_t)25U  /* FOTA timeout */
#define CST_MODEM_RESET_EVENT                      (CST_autom_event_t)26U  /* modem reset requested */
#if (USE_LOW_POWER == 0)
#define CST_MAX_EVENT                              27U
#else /* (USE_LOW_POWER == 1) */
#define CST_POWER_SLEEP_TIMEOUT_EVENT              (CST_autom_event_t)27U  /* low power entry timeout */
#define CST_POWER_SLEEP_REQUEST_EVENT              (CST_autom_event_t)28U  /* low power request */
#define CST_POWER_SLEEP_COMPLETE_EVENT             (CST_autom_event_t)29U  /* low power completed */
#define CST_POWER_WAKEUP_EVENT                     (CST_autom_event_t)30U  /* exit from low power */
#define CST_POWER_SLEEP_ABORT_EVENT                (CST_autom_event_t)31U  /* low power request abort */
#define CST_MAX_EVENT                              32U
#endif /* (USE_LOW_POWER == 1) */

/* ================================= */
/* List of automation events - END   */
/* ================================= */


/* list of event messages to send to cellular automaton task */
#define CST_MESSAGE_CS_EVENT              ((CST_message_type_t)0U)       /* CS: Cellular Service */
#define CST_MESSAGE_DC_EVENT              ((CST_message_type_t)1U)       /* DC: Data Cache       */
#define CST_MESSAGE_CMD                   ((CST_message_type_t)2U)       /* Command              */

typedef uint16_t CST_message_type_t;

/* External variables --------------------------------------------------------*/
extern uint8_t    *CST_SimSlotName_p[3];
extern bool    CST_polling_active;
#if (( USE_TRACE_CELLULAR_SERVICE == 1) || ( USE_CMD_CONSOLE == 1 ))
extern const uint8_t *CST_StateName[CST_MAX_STATE];
#endif /* (( USE_TRACE_CELLULAR_SERVICE == 1) || ( USE_CMD_CONSOLE == 1 )) */

/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */


/**
  * @brief  allows to set cellular service automaton current state
  * @param new_state - automaton stat
  * @retval -
  */
extern void CST_set_state(CST_autom_state_t new_state);

/**
  * @brief  allows to get cellular service automaton current state
  * @param  -
  * @retval CST_autom_state_t - automaton state
  */
extern CST_autom_state_t CST_get_state(void);

/**
  * @brief  allows to set radio on: start cellular automaton
  * @param  -
  * @retval CS_Status_t - return code
  */
extern CS_Status_t CST_radio_on(void);

/**
  * @brief  allows to boot modem only without network register
  * @note   the application is not started
  * @param  -
  * @retval CS_Status_t - return code
  */
extern CS_Status_t CST_modem_power_on(void);

/**
  * @brief  initializes cellular service component
  * @param  -
  * @retval CS_Status_t - return code
  */
extern CS_Status_t CST_cellular_service_init(void);

/**
  * @brief  starts cellular service component
  * @note   cellular service task automaton and tempos are started
  * @param  -
  * @retval CS_Status_t - return code
  */
extern CS_Status_t CST_cellular_service_start(void);

/**
  * @brief  allows to get the modem IP addr
  * @note   to be able to return an IP address the modem must be in data tranfer state
  * @note   else an error is returned
  * @param  ip_addr_type - type of IP address
  * @param  p_ip_addr_value - IP address value returned by the function
  * @retval CS_Status_t - return code
  */
extern CS_Status_t CST_get_dev_IP_address(CS_IPaddrType_t *ip_addr_type, CS_CHAR_t *p_ip_addr_value);


/**
  * @brief  sends a message to cellular service task automaton
  * @param  type - type of massage
  * @param  event - event
  * @retval -
  */
extern void  CST_send_message(CST_message_type_t  type, CST_autom_event_t event);

#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_SERVICE_TASK_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

