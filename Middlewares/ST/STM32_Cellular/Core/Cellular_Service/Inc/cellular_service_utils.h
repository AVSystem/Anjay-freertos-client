/**
  ******************************************************************************
  * @file    cellular_service_task.h
  * @author  MCD Application Team
  * @brief   Header for cellular_service_util.c module
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
#ifndef CELLULAR_SERVICE_UTILS_H
#define CELLULAR_SERVICE_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif
/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"
#include "cellular_control_api.h"
#include "cellular_service.h"
#include "cellular_service_task.h"
#include "cellular_service_datacache.h"

/* Exported constants --------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* External variables --------------------------------------------------------*/
extern CS_LowPower_status_t cst_lp_status;

/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */

/**
  * @brief  convert access techno CS enum value to CA enum value
  * @param  access_techno       - access techno CS enum value
  * @retval ca_access_techno_t  - access techno CA enum value
  */
ca_access_techno_t cst_convert_access_techno(CS_AccessTechno_t access_techno);

/**
  * @brief  URC callback to get low power T3412 and T3324 timer values
  * @param  -
  * @retval -
  */
void CST_cellular_power_status_callback(CS_LowPower_status_t lp_status);

/**
  * @brief  init modem management
  * @param  -
  * @retval -
  */
void CST_modem_sim_init(void);

/**
  * @brief  update Cellular Info entry of Data Cache
  * @param  dc_service_state - new entry state to set
  * @param  ip_addr - new IP address (null if not defined)
  * @retval -
  */
void CST_data_cache_cellular_info_set(dc_service_rt_state_t dc_service_state, dc_network_addr_t *ip_addr);

/**
  * @brief  configuration failure management
  * @param  msg_fail   - failure message (only for trace)
  * @param  fail_cause - failure cause
  * @param  fail_count - count of failures
  * @param  fail_max   - max of allowed failures
  * @retval -
  */
void CST_config_fail(const uint8_t *msg_fail, cst_fail_cause_t fail_cause, uint8_t *fail_count, uint8_t fail_max);

/**
  * @brief  sets current signal quality values in DC
  * @param  -
  * @retval CS_Status_t - error code
  */
CS_Status_t CST_set_signal_quality(void);

/**
  * @brief  subscribes to modem events
  * @param  -
  * @retval -
  */

void CST_subscribe_modem_events(void);

/**
  * @brief  gets automaton event from message event
  * @param  event  - message event
  * @retval cst_autom_event_t - automaton event
  */
cst_autom_event_t CST_get_autom_event(cst_message_t event);

/**
  * @brief  gets network status
  * @param  -
  * @retval cst_network_status_t - network status
  */
cst_network_status_t  CST_get_network_status(void);

/**
  * @brief  modifies the content of modem state in datacache
  * @param  db    - datacache database
  * @param  state - state to be set in modem state entry
  * @param  chr   - text associated to "state" that will be displayed for debug
  * @retval -
  */
void CST_set_modem_state(dc_com_db_t *db, ca_modem_state_t state, const uint8_t *chr);

/**
  * @brief  Clean switch off of modem
  * @param  -
  * @retval -
  */
CS_Status_t CST_modem_power_off(void);

/**
  * @brief  returns the actual sim slot index in use
  * @param  -
  * @retval The actual sim slot index
  */
uint8_t CST_get_sim_slot_index(void);

#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_SERVICE_UTILS_H */
