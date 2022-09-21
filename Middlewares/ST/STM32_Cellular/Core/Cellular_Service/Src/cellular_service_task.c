/**
  ******************************************************************************
  * @file    cellular_service_task.c
  * @author  MCD Application Team
  * @brief   This file defines functions for Cellular Service Task
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
#include "plf_config.h"

#include <string.h>

#include "cellular_service_task.h"
#include "rtosal.h"
#include "dc_common.h"
#include "error_handler.h"

#include "at_util.h"
#include "cellular_control_api.h"
#include "cellular_service.h"
#include "cellular_service_datacache.h"
#include "cellular_service_utils.h"
#include "cellular_service_os.h"
#include "cellular_service_config.h"
#include "cellular_service_int.h"
#include "cellular_runtime_custom.h"

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
#include "ppposif_client.h"
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */

#if (USE_CMD_CONSOLE == 1)
#include "cellular_service_cmd.h"
#include "cmd.h"
#endif  /* (USE_CMD_CONSOLE == 1) */

/*  Unitary tests of cellular service task */
#define USE_CELLULAR_SERVICE_TASK_TEST         (0) /* 0: not activated, 1: activated */

#if (USE_CELLULAR_SERVICE_TASK_TEST == 1)
#include "cellular_service_test.h"
#endif  /* (USE_CELLULAR_SERVICE_TASK_TEST == 1) */

#if (USE_LOW_POWER == 1)
#include "cellular_service_power.h"
#endif  /* (USE_LOW_POWER == 1) */

/* Private defines -----------------------------------------------------------*/

/* Maximum elements in Message queue */
#define CST_QUEUE_SIZE (10U)

/* cellular register Test activation */

#define CST_MODEM_POLLING_PERIOD_DEFAULT 5000U

/* delay for PND activation retry */
#define CST_PDN_ACTIVATE_RETRY_DELAY 30000U

/* FOTA Timeout since start programming */
#define CST_FOTA_TIMEOUT      (360000U) /* 6 min (calibrated for cat-M1 network, increase it for cat-NB1) */

/* Private macros ------------------------------------------------------------*/
#if (USE_PRINTF == 0U)
/* Trace macro definition */
#include "trace_interface.h"
#define PRINT_FORCE(format, args...) \
  TRACE_PRINT_FORCE(DBG_CHAN_CELLULAR_SERVICE, DBL_LVL_P0, "" format "", ## args)
#else
#include <stdio.h>
#define PRINT_FORCE(format, args...)                (void)printf(format, ## args);
#endif  /* (USE_PRINTF == 1) */

/* Private typedef -----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Event names to display */
#if (USE_TRACE_CELLULAR_SERVICE == 1U)
static const uint8_t *cst_event_name[CST_MAX_EVENT] =
{
  ((uint8_t *)"BOOT_EVENT"),
  ((uint8_t *)"MODEM_INIT_EVENT"),
  ((uint8_t *)"MODEM_READY_EVENT"),
  ((uint8_t *)"SIGNAL_QUALITY_TO_CHECK_EVENT"),
  ((uint8_t *)"NETWORK_STATUS_TO_CHECK_EVENT"),
  ((uint8_t *)"NETWORK_STATUS_OK_EVENT"),
  ((uint8_t *)"MODEM_ATTACHED_EVENT"),
  ((uint8_t *)"PDP_ACTIVATED_EVENT"),
  ((uint8_t *)"PDN_STATUS_TO_CHECK_EVENT"),
  ((uint8_t *)"MODEM_PDN_ACTIVATE_RETRY_TIMER_EVENT"),
  ((uint8_t *)"CELLULAR_DATA_FAIL_EVENT"),
  ((uint8_t *)"POLLING_TIMER_EVENT"),
  ((uint8_t *)"MODEM_URC_EVENT"),
  ((uint8_t *)"NO_EVENT"),
  ((uint8_t *)"CMD_UNKWOWN_EVENT"),
  ((uint8_t *)"TARGET_STATE_CMD_EVENT"),
  ((uint8_t *)"APN_CONFIG_EVENT"),
  ((uint8_t *)"REBOOT_MODEM_EVENT"),
  ((uint8_t *)"MODEM_POWER_ON_ONLY_EVENT"),
  ((uint8_t *)"NETWORK_CALLBACK_EVENT"),
  ((uint8_t *)"NW_REG_TIMEOUT_TIMER_EVENT"),
  ((uint8_t *)"FOTA_START_EVENT"),
  ((uint8_t *)"FOTA_END_EVENT"),
  ((uint8_t *)"FOTA_TIMEOUT_EVENT"),
  ((uint8_t *)"MODEM_RESET_EVENT"),
  ((uint8_t *)"MODEM_REBOOT_EVENT"),
  ((uint8_t *)"PPP_OPENED_EVENT"),
  ((uint8_t *)"PPP_CLOSED_EVENT"),
  ((uint8_t *)"MODEM_POWER_DOWN_EVENT"),
  ((uint8_t *)"SIM_RESET_EVENT"),
  ((uint8_t *)"POWER_STATUS_CALLBACK_EVENT"),
#if (USE_LOW_POWER == 1)
  ((uint8_t *)"POWER_SLEEP_TIMEOUT_EVENT"),
  ((uint8_t *)"POWER_SLEEP_REQUEST_EVENT"),
  ((uint8_t *)"POWER_SLEEP_COMPLETE_EVENT"),
  ((uint8_t *)"POWER_WAKEUP_EVENT"),
  ((uint8_t *)"POWER_MODEM_WAKEUP_EVENT"),
  ((uint8_t *)"POWER_SLEEP_ABORT_EVENT"),
  ((uint8_t *)"CST_LP_INACTIVITY_TIMER_EVENT"),
#endif /* (USE_LOW_POWER == 1) */
};
#endif /* (USE_TRACE_CELLULAR_SERVICE == 1) */

/* Event queue */
static osMessageQId      cst_queue_id;

/* Timer handlers */
static osTimerId         cst_pdn_activate_retry_timer_handle;  /* pdn activation timer                */
static osTimerId         cst_network_status_timer_handle;      /* waiting for network status OK timer */
static osTimerId         cst_register_retry_timer_handle;      /* registering to network timer        */
static osTimerId         cst_fota_timer_handle;                /* FOTA timer                          */
static osTimerId         cst_polling_timer_handle;             /* Pooling timer                       */

#if (USE_LOW_POWER == 1)
static osTimerId         cst_lp_inactivity_timer_handle;       /* Inactivity timer to check if there is data */
/* activity before to enter in low power mode */
#endif /* (USE_LOW_POWER == 1) */

/* DC structures */
dc_cellular_info_t         cst_cellular_info;           /* cellular infos               */
dc_sim_info_t              cst_sim_info;                /* sim infos                    */
dc_cellular_data_info_t    cst_cellular_data_info;      /* cellular data infos          */
dc_cellular_params_t       cst_cellular_params;         /* cellular configuration param */

/* NFMC context */
cst_nfmc_context_t cst_nfmc_context;                    /* NFMC context                 */

/* CST context */
cst_context_t cst_context =
{
  CST_BOOT_STATE, CST_NO_FAIL, CS_PDN_EVENT_NW_DETACH,    /* Automaton State, FAIL Cause,  */
  { 0U, 0U},                                         /* signal quality */
  CS_NRS_NOT_REGISTERED_NOT_SEARCHING, CS_NRS_NOT_REGISTERED_NOT_SEARCHING, CS_NRS_NOT_REGISTERED_NOT_SEARCHING,
  0U,                                                /* activate_pdn_nfmc_tempo_count */
  0U,                                                /* register_retry_tempo_count */
  0U,                                                /* sim slot index */
  false,                                             /* modem power status : power off */
  0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U     /* fail counters */
};

/* Private function prototypes -----------------------------------------------*/
static void CST_pdn_event_callback(CS_PDN_conf_id_t cid, CS_PDN_event_t pdn_event);
static void CST_polling_timer_callback(void *argument);
static void CST_pdn_activate_retry_timer_callback(void *argument);
static void CST_network_status_timer_callback(void *argument);
static void CST_register_retry_timer_callback(void *argument);
static void CST_fota_timer_callback(void *argument);
#if (USE_LOW_POWER == 1)
static void CST_lp_inactivity_timer_callback(void *argument);
#endif /* (USE_LOW_POWER == 1) */
static void CST_notif_callback(dc_com_event_id_t dc_event_id, const void *private_data);
static void CST_SimEventsCallback(CS_SimEvent_status_t sim_event);

static void CST_boot_power_on_only_modem_mngt(void);
static void CST_init_power_on_only_modem_mngt(void);
static void CST_init_state_mngt(void);
static void CST_init_state_sim_reset_mngt(void);
static void CST_modem_reset_mngt(void);
static void CST_modem_reboot_mngt(void);
static void CST_net_register_mngt(void);
static void CST_nw_reg_timeout_expiration_mngt(void);
static void CST_signal_quality_test_mngt(void);
static void CST_network_status_test_mngt(void);
static void CST_network_event_mngt(void);
static void CST_data_ready_state_network_event_mngt(void);
static void CST_pdn_event_nw_detach_mngt(void);
static void CST_attach_modem_mngt(void);
static void CST_modem_activated_mngt(void);
static void CST_modem_activate_pdn_mngt(void);
static void CST_cellular_data_fail_mngt(void);
static void CST_pdn_event_mngt(void);
static void CST_polling_timer_mngt(void);
static void CST_apn_set_new_config_mngt(void);
static void CST_data_mode_target_state_event_mngt(void);
static void CST_sim_only_target_state_event_mngt(void);
static void CST_modem_off_mngt(void);

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
static void CST_ppp_config_mngt(void);
static void CST_ppp_opened_event_mngt(void);
static void CST_ppp_closed_event_mngt(void);
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
static void CST_data_ready_mngt(void);

static void CST_init_state(cst_autom_event_t autom_event);
static void CST_modem_reset_state(cst_autom_event_t autom_event);
static void CST_modem_reboot_state(cst_autom_event_t autom_event);
static void CST_modem_ready_state(cst_autom_event_t autom_event);
static void CST_waiting_for_signal_quality_ok_state(cst_autom_event_t autom_event);
static void CST_waiting_for_network_status_state(cst_autom_event_t autom_event);
static void CST_network_status_ok_state(cst_autom_event_t autom_event);
static void CST_modem_registered_state(cst_autom_event_t autom_event);
static void CST_modem_pdn_activating_state(cst_autom_event_t autom_event);
static void CST_data_ready_state(cst_autom_event_t autom_event);
static void CST_fail_state(cst_autom_event_t autom_event);

#if (USE_LOW_POWER == 1)
static void CST_power_idle_state(cst_autom_event_t autom_event);
#endif /* (USE_LOW_POWER == 1) */

static void CST_cellular_service_task(void *argument);

/* Global variables ----------------------------------------------------------*/
bool CST_polling_active;            /* modem polling activation flag */
static bool CST_polling_on_going;   /* modem polling already asked, and on going */

#if (( USE_TRACE_CELLULAR_SERVICE == 1) || ( USE_CMD_CONSOLE == 1 ))
/* State names to display */
const uint8_t *CST_StateName[CST_MAX_STATE] =
{
  ((uint8_t *)"BOOT_STATE"),
  ((uint8_t *)"MODEM_INIT_STATE"),
  ((uint8_t *)"MODEM_READY_STATE"),
  ((uint8_t *)"WAITING_FOR_SIGNAL_QUALITY_OK_STATE"),
  ((uint8_t *)"WAITING_FOR_NETWORK_STATUS_STATE"),
  ((uint8_t *)"NETWORK_STATUS_OK_STATE"),
  ((uint8_t *)"MODEM_REGISTERED_STATE"),
  ((uint8_t *)"MODEM_PDN_ACTIVATING_STATE"),
  ((uint8_t *)"MODEM_DATA_READY_STATE"),
  ((uint8_t *)"MODEM_REPROG_STATE"),
  ((uint8_t *)"MODEM_FAIL_STATE"),
  ((uint8_t *)"MODEM_SIM_ONLY_STATE"),
  ((uint8_t *)"MODEM_RESET_STATE"),
  ((uint8_t *)"MODEM_REBOOT_STATE"),
  ((uint8_t *)"MODEM_OFF_STATE"),
  ((uint8_t *)"MODEM_ON_ONLY_STATE"),
  ((uint8_t *)"MODEM_POWER_DATA_IDLE_STATE"),
  ((uint8_t *)"APN_CONFIG_STATE"),
  ((uint8_t *)"CST_PPP_CONFIG_ON_GOING_STATE"),
  ((uint8_t *)"CST_PPP_CLOSE_ON_GOING_STATE")
};
#endif /* (( USE_TRACE_CELLULAR_SERVICE == 1) || ( USE_CMD_CONSOLE == 1 )) */

/* Private function Definition -----------------------------------------------*/

/**
  * @brief  sends message to cellular service task
  * @param  type   - message type
  * @param  event  - event/command
  * @retval -
  */
void  CST_send_message(CST_message_type_t type, cst_autom_event_t event)
{
  PRINT_CELLULAR_SERVICE("CST: CST_send_message: %s\n\r", cst_event_name[event])
  cst_message_t cmd_message;

  cmd_message = 0U;
  SET_AUTOMATON_MSG_TYPE(cmd_message, type);
  SET_AUTOMATON_MSG_ID(cmd_message, event);

  if (rtosalMessageQueuePut((osMessageQId)cst_queue_id, cmd_message, 0U) != osOK)
  {
    PRINT_CELLULAR_SERVICE_ERR("CST: CST queue msg %ld can NOT be added. (Queue full ?)\n\r", cmd_message)
  }
}

/* ===================================================================
   UTility functions  END
   =================================================================== */


/* ===================================================================
   Callback functions  BEGIN
   =================================================================== */

/**
  * @brief  PDN event callback
  * @param  cid  - current CID
  * @param  pdn_event - pdn event
  * @retval -
  */
static void CST_pdn_event_callback(CS_PDN_conf_id_t cid, CS_PDN_event_t pdn_event)
{
#if (USE_TRACE_CELLULAR_SERVICE == 0)
  UNUSED(cid);
#endif /* USE_TRACE_CELLULAR_SERVICE == 0 */

  PRINT_CELLULAR_SERVICE("CST: CST_pdn_event_callback (cid=%d / event=%d)\n\r",
                         cid, pdn_event)
  /* sends a message to automaton */
  cst_context.pdn_status = pdn_event;
  CST_send_message(CST_MESSAGE_CS_EVENT, CST_PDN_STATUS_TO_CHECK_EVENT);

}

/**
  * @brief  Data cache callback. Called when a DC entry is updated
  * @param  dc_event_id  - data cache event
  * @param  private_gui_data  - private Data Cache context (not used)
  * @retval -
  */
static void CST_notif_callback(dc_com_event_id_t dc_event_id, const void *private_data)
{
  UNUSED(private_data);

  PRINT_CELLULAR_SERVICE("CST: CST_notif_callback (Data Cache event=%d)\n\r", dc_event_id)
  if ((dc_event_id == DC_CELLULAR_DATA_INFO)
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
      || (dc_event_id == DC_CELLULAR_INFO)
#endif /*(USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
      || (dc_event_id == DC_CELLULAR_TARGET_STATE_CMD))
  {
    /* sends a message to automaton */
    CST_send_message(CST_MESSAGE_DC_EVENT, (cst_autom_event_t)dc_event_id);
  }
  else if (dc_event_id == DC_CELLULAR_CONFIG)
  {
    /* Update cst_cellular_params */
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cst_cellular_params,
                      sizeof(cst_cellular_params));
    /* new apn config required  */
    if (cst_cellular_params.sim_slot[cst_context.sim_slot_index].apnChanged == true)
    {
      /* reset value of apnChanged will be done when receiving the event CST_APN_CONFIG_EVENT */
      /* Can't write a new value in Data Cache as we actually treat the callback of Data Cache write */
      /* sends a message to automaton */
      CST_send_message(CST_MESSAGE_CS_EVENT, CST_APN_CONFIG_EVENT);
    }
  }
#if (USE_LOW_POWER == 1)
  else if (dc_event_id == DC_CELLULAR_POWER_CONFIG)
  {
    /* set power config  */
    CSP_SetPowerConfig();
  }
#endif /* (USE_LOW_POWER == 1) */
  else
  {
    __NOP(); /* Nothing to do */
  }
}


/**
  * @brief  CST_polling_timer_callback function
  * @param  argument - argument (not used)
  * @retval -
  */
static void CST_polling_timer_callback(void *argument)
{
  UNUSED(argument);
  if (((cst_context.current_state == CST_WAITING_FOR_SIGNAL_QUALITY_OK_STATE)
       || (cst_context.current_state == CST_WAITING_FOR_NETWORK_STATUS_STATE)
       || (cst_context.current_state == CST_MODEM_DATA_READY_STATE))
      && (cst_nfmc_context.nfmc_timer_on_going == false)
      && (CST_polling_active == true)
     )
  {
    if (CST_polling_on_going == false)
    {
      /* something to do in this state: sends a message to automaton */
      CST_send_message(CST_MESSAGE_CS_EVENT, CST_POLLING_TIMER_EVENT);
    }
    /*
        else
        {
          PRINT_CELLULAR_SERVICE("CST: Discard pooling timer message, another pooling message is already on going\n\r")
        }
    */
  }
}

/**
  * @brief  timer callback to retry PDN activation
  * @param  argument - argument (not used)
  * @retval -
  */
static void CST_pdn_activate_retry_timer_callback(void *argument)
{
  UNUSED(argument);
  PRINT_CELLULAR_SERVICE("CST: CST_pdn_activate_retry_timer_callback\n\r")
  if (cst_context.current_state == CST_MODEM_PDN_ACTIVATING_STATE)
  {
    /* something to do in this state: sends a message to automaton */
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_PDN_ACTIVATE_RETRY_TIMER_EVENT);
  }
}

/**
  * @brief  timer callback to retry network register
  * @param  argument - argument (not used)
  * @retval -
  */
static void CST_register_retry_timer_callback(void *argument)
{
  UNUSED(argument);
  /* automaton to reinitialize : sends a message to automaton */
  cst_nfmc_context.nfmc_timer_on_going = false;

  CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_INIT_EVENT);
}

/**
  * @brief  timer callback to check network status
  * @param  argument - argument (not used)
  * @retval -
  */
static void CST_network_status_timer_callback(void *argument)
{
  UNUSED(argument);
  if (cst_context.current_state == CST_WAITING_FOR_NETWORK_STATUS_STATE)
  {
    /*  something to do in this state: sends a message to automaton */
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_NW_REG_TIMEOUT_TIMER_EVENT);
  }
}

/**
  * @brief  FOTA timeout: FOTA failure
  * @param  argument - argument (not used)
  * @retval -
  */
static void CST_fota_timer_callback(void *argument)
{
  UNUSED(argument);
  /* FOTA timeout has occurred: sends a message to automaton */
  CST_send_message(CST_MESSAGE_CS_EVENT, CST_FOTA_TIMEOUT_EVENT);
}

/**
  * @brief  Sim events treatment
  * @param  sim_event - Structure that defines the detected event (SIM inserted/removed/refresh)
  * @retval -
  */
static void CST_SimEventsCallback(CS_SimEvent_status_t sim_event)
{
  PRINT_CELLULAR_SERVICE("CST: CST_SimEventsCallback\n\r")
  /* Modem reset to be reconfigured with new SIM information */
  CST_send_message(CST_MESSAGE_CS_EVENT, CST_SIM_RESET_EVENT);


  if (sim_event.event == CS_SIMEVENT_SIM_DETECT)
  {
    if (sim_event.param1 == CS_SIMINFOS_CARD_INSERTED)
    {
      /* SIM CARD insertion detected */
      PRINT_CELLULAR_SERVICE("CST: --> SIM CARD insertion detected\n\r")
    }
    else if (sim_event.param1 == CS_SIMINFOS_CARD_REMOVED)
    {
      /* SIM CARD ejection detected */
      PRINT_CELLULAR_SERVICE("CST: --> SIM CARD ejection detected\n\r")
    }
    else
    {
      /* SIM detect parameter unknown */
      PRINT_CELLULAR_SERVICE("CST: --> SIM CARD detect parameter unknown\n\r")
    }
  }
  else if (sim_event.event == CS_SIMEVENT_SIM_REFRESH)
  {
    /* SIM REFRESH event detected */
    PRINT_CELLULAR_SERVICE("CST: --> SIM REFRESH event detected\n\r")
  }
  else
  {
    /* SIM event unknown */
    PRINT_CELLULAR_SERVICE("CST: --> SIM CARD event unknown\n\r")
  }
}

#if (USE_LOW_POWER == 1)
/**
  * @brief  LP inactivity timeout: Wait for no network event before to enter in low power mode
  * @param  argument - argument (not used)
  * @retval -
  */
static void CST_lp_inactivity_timer_callback(void *argument)
{
  UNUSED(argument);
  /* Netwok inactivity timeout has occurred: enter in low power mode */
  PRINT_CELLULAR_SERVICE("CST: CST_lp_inactivity_timer_callback\n\r")
  CST_send_message(CST_MESSAGE_CS_EVENT, CST_LP_INACTIVITY_TIMER_EVENT);

}
#endif /* (USE_LOW_POWER == 1) */
/* ===================================================================
   Callback functions  END
   =================================================================== */

/* ===================================================================
   Automaton processing functions  BEGIN
   =================================================================== */

/**
  * @brief  automation boot in the special case where the modem is powered on alone without network attachment
  * @param  -
  * @retval -
  */
static void CST_boot_power_on_only_modem_mngt(void)
{
  PRINT_CELLULAR_SERVICE("CST: CST_boot_modem_power_on_only_mngt\n\r")
  CST_set_modem_state(&dc_com_db, CA_MODEM_POWER_OFF, (uint8_t *)"CA_MODEM_POWER_OFF");

  /* Set new state and sends a message to automaton */
  CST_set_state(CST_MODEM_INIT_STATE);
  CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_POWER_ON_ONLY_EVENT);
}

/**
  * @brief  automation init in the special case where the modem is powered on alone without network attachment
  * @param  -
  * @retval -
  */
static void CST_init_power_on_only_modem_mngt(void)
{
  PRINT_CELLULAR_SERVICE("CST: CST_power_on_only_modem_mngt\n\r")
  /* Modem is powered on */
  PRINT_CELLULAR_SERVICE("CST: osCDS_power_on()\n\r")
  (void)osCDS_power_on();

  CST_set_modem_state(&dc_com_db, CA_MODEM_STATE_POWERED_ON, (uint8_t *)"CA_MODEM_STATE_POWERED_ON");

  CST_set_state(CST_MODEM_POWER_ON_ONLY_STATE);
}

/**
  * @brief  automaton boot in normal case
  * @param  -
  * @retval -
  */
static void CST_boot_event_mngt(void)
{
  PRINT_CELLULAR_SERVICE("CST: CST_boot_event_mngt\n\r")
  CST_set_modem_state(&dc_com_db, CA_MODEM_POWER_OFF, (uint8_t *)"CA_MODEM_POWER_OFF");

  CST_set_state(CST_MODEM_INIT_STATE);
  CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_INIT_EVENT);
}



static void CST_off_state_target_cmd_state_mngt(void)
{
  dc_cellular_target_state_t target_state;
  CS_Status_t                cs_status;

  PRINT_CELLULAR_SERVICE("CST: CST_off_state_target_cmd_state_mngt\n\r")

  /* Set modem target state to the actual target state for value coherency */
  if (dc_com_read(&dc_com_db, DC_CELLULAR_TARGET_STATE_CMD, (void *)&target_state, sizeof(target_state)) == DC_COM_OK)
  {
    target_state.rt_state     = DC_SERVICE_ON;
    target_state.target_state = cst_cellular_params.target_state;
    target_state.callback = false;
    (void)dc_com_write(&dc_com_db, DC_CELLULAR_TARGET_STATE_CMD, (void *)&target_state, sizeof(target_state));
  }

  if (cst_cellular_params.target_state == DC_TARGET_STATE_OFF)
  {
    (void)CST_modem_power_off();
  }
  else if (cst_cellular_params.target_state == DC_TARGET_STATE_MODEM_ONLY)
  {
    PRINT_CELLULAR_SERVICE("CST: osCDS_power_on()\n\r")
    (void)osCDS_power_on();
  }
  else
  {
    /* Target_state == SIM_ONLY or FULL */
    if (cst_nfmc_context.nfmc_timer_on_going == false)
    {
      /* modem target state required: SIM ONLY or FULL */

      /* Modem to power on */
      /* Before powering up the modem, subscribe to modem SIM events : Sim detect (inserted or remove) or Sim refresh */
      (void)CS_subscribe_sim_event(CST_SimEventsCallback);
      PRINT_CELLULAR_SERVICE("CST: osCDS_power_on()\n\r")
      cs_status = osCDS_power_on();

      if (cs_status != CELLULAR_OK)
      {
        /* Modem powered on failed */
        CST_config_fail(((uint8_t *)"CST_cmd"),
                        CST_MODEM_POWER_ON_FAIL,
                        &cst_context.power_on_reset_count,
                        CST_POWER_ON_RESET_MAX);

      }
      else
      {
        /* Modem powered on OK: update DC*/
        (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(dc_cellular_info_t));
        cst_cellular_info.rt_state    = DC_SERVICE_RUN;
        (void)dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(dc_cellular_info_t));
        CST_set_modem_state(&dc_com_db, CA_MODEM_STATE_POWERED_ON, (uint8_t *)"CA_MODEM_STATE_POWERED_ON");
        CST_modem_sim_init();
      }
    }
  }
}

/**
  * @brief  automation init in normal case
  * @param  -
  * @retval -
  */
static void CST_init_state_mngt(void)
{
  dc_cellular_target_state_t target_state;
  CS_Status_t                cs_status;

  PRINT_CELLULAR_SERVICE("CST: CST_init_state_mngt\n\r")

  /* Set modem target state to the actual target state for value coherency */
  if (dc_com_read(&dc_com_db, DC_CELLULAR_TARGET_STATE_CMD, (void *)&target_state, sizeof(target_state)) == DC_COM_OK)
  {
    target_state.rt_state     = DC_SERVICE_ON;
    target_state.target_state = cst_cellular_params.target_state;
    target_state.callback = false;
    (void)dc_com_write(&dc_com_db, DC_CELLULAR_TARGET_STATE_CMD, (void *)&target_state, sizeof(target_state));
  }

  if (cst_cellular_params.target_state == DC_TARGET_STATE_OFF)
  {
    /* modem target state required: OFF. */
    CST_set_state(CST_MODEM_OFF_STATE);
    /* Data Cache -> Radio OFF */
    CST_set_modem_state(&dc_com_db, CA_MODEM_POWER_OFF, (uint8_t *)"CA_MODEM_POWER_OFF");
  }
  else
  {
    if (cst_nfmc_context.nfmc_timer_on_going == false)
    {
      /* modem target state required: SIM ONLY or FULL */

      /* Modem to power on */
      /* Before powering up the modem, subscribe to modem SIM events : Sim detect (inserted or remove) or Sim refresh */
      (void)CS_subscribe_sim_event(CST_SimEventsCallback);
      PRINT_CELLULAR_SERVICE("CST: osCDS_power_on()\n\r")
      cs_status = osCDS_power_on();

      if (cs_status != CELLULAR_OK)
      {
        /* Modem powered on failed */
        CST_config_fail(((uint8_t *)"CST_cmd"),
                        CST_MODEM_POWER_ON_FAIL,
                        &cst_context.power_on_reset_count,
                        CST_POWER_ON_RESET_MAX);
      }
      else
      {
        /* Modem powered on OK: update DC*/
        (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(dc_cellular_info_t));
        cst_cellular_info.rt_state    = DC_SERVICE_RUN;
        (void)dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(dc_cellular_info_t));
        CST_set_modem_state(&dc_com_db, CA_MODEM_STATE_POWERED_ON, (uint8_t *)"CA_MODEM_STATE_POWERED_ON");
        CST_modem_sim_init();
      }
    }
  }
}

/**
  * @brief  Reset only sim
  * @note   On sim refresh event, sim has to be reset to get correct information about the new sim.
  * @param  -
  * @retval -
  */
static void CST_init_state_sim_reset_mngt(void)
{
  dc_cellular_target_state_t target_state;

  PRINT_CELLULAR_SERVICE("CST: CST_init_state_sim_reset_mngt\n\r")

  /* Set modem target state to the actual target state for value coherency */
  if (dc_com_read(&dc_com_db, DC_CELLULAR_TARGET_STATE_CMD, (void *)&target_state, sizeof(target_state)) == DC_COM_OK)
  {
    target_state.rt_state     = DC_SERVICE_ON;
    target_state.target_state = cst_cellular_params.target_state;
    target_state.callback = false;
    (void)dc_com_write(&dc_com_db, DC_CELLULAR_TARGET_STATE_CMD, (void *)&target_state, sizeof(target_state));
  }

  if (cst_cellular_params.target_state == DC_TARGET_STATE_OFF)
  {
    /* modem target state required: OFF. */
    CST_set_state(CST_MODEM_OFF_STATE);
    /* Data Cache -> Radio OFF */
    CST_set_modem_state(&dc_com_db, CA_MODEM_POWER_OFF, (uint8_t *)"CA_MODEM_POWER_OFF");
  }
  else
  {
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(dc_cellular_info_t));
    cst_cellular_info.rt_state    = DC_SERVICE_RUN;
    (void)dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(dc_cellular_info_t));
    CST_data_cache_cellular_info_set(DC_SERVICE_OFF, NULL);
    CST_set_modem_state(&dc_com_db, CA_MODEM_STATE_POWERED_ON, (uint8_t *)"CA_MODEM_STATE_POWERED_ON");
    CST_modem_sim_init();
  }
}


/**
  * @brief  reboot modem management
  * @note   modem has signaling it is rebooting
  * @param  -
  * @retval -
  */
static void CST_reboot_modem_event_mngt(void)
{
  PRINT_CELLULAR_SERVICE("CST: CST_reboot_modem_event_mngt\n\r")
  /* if current_state == CST_MODEM_INIT_STATE then nothing to do (nominal case) */
  if (cst_context.current_state != CST_MODEM_INIT_STATE)
  {
#if (USE_LOW_POWER == 1)
    /* stop the modem sleep request protection timer (to be sure it won't fire after) */
    CSP_StopTimeout();
#endif /* (USE_LOW_POWER == 1) */

    /* modem power off */
    (void)CST_modem_power_off();

    /* something to do in this state: sends a message to automaton */
    PRINT_CELLULAR_SERVICE("CST: Modem event received: CS_MDMEVENT_BOOT\n\r")
    CST_set_state(CST_MODEM_INIT_STATE);
    CST_data_cache_cellular_info_set(DC_SERVICE_OFF, NULL);
    CST_set_modem_state(&dc_com_db, CA_MODEM_REBOOTING, (uint8_t *)"CA_MODEM_REBOOTING");

    CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_INIT_EVENT);
  }
}

/**
  * @brief  FOTA start
  * @param  -
  * @retval -
  */
static void CST_fota_start_event_mngt(void)
{
  PRINT_CELLULAR_SERVICE("CST: CST_fota_start_event_mngt\n\r")
  /* FOTA event: FOTA start */
  PRINT_CELLULAR_SERVICE("CST: Modem event received:  CS_MDMEVENT_FOTA_START\n\r")
  /* DC_CELLULAR_DATA_INFO, is used for call back management on IP or rt_state modification. */
  /* So, update this Data Cache structure */
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_DATA_INFO, (void *)&cst_cellular_data_info,
                    sizeof(cst_cellular_data_info));
  cst_cellular_data_info.rt_state = DC_SERVICE_SHUTTING_DOWN;
  CST_set_state(CST_MODEM_REPROG_STATE);
  (void)dc_com_write(&dc_com_db, DC_CELLULAR_DATA_INFO, (void *)&cst_cellular_data_info,
                     sizeof(cst_cellular_data_info));

  /* "Real" data to be used is in DC_CELLULAR_INFO, so, update also this Data Cache structure */
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info,
                    sizeof(cst_cellular_info));
  cst_cellular_info.rt_state = DC_SERVICE_SHUTTING_DOWN;
  (void)dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info,
                     sizeof(cst_cellular_info));

  CST_set_modem_state(&dc_com_db, CA_MODEM_FOTA_INPROGRESS, (uint8_t *)"CA_MODEM_FOTA_INPROGRESS");

  (void)rtosalTimerStart(cst_fota_timer_handle, CST_FOTA_TIMEOUT);
}

/**
  * @brief  FOTA end
  * @param  -
  * @retval -
  */
static void CST_fota_end_event_mngt(void)
{
  PRINT_CELLULAR_SERVICE("CST: CST_fota_end_event_mngt\n\r")
  /* FOTA event: FOTA end */
  /* stops FOTA timeout timer */
  (void)rtosalTimerStop(cst_fota_timer_handle);
  PRINT_CELLULAR_SERVICE("CST: Modem event received:  CS_MDMEVENT_FOTA_END\n\r")

  /* TRIGGER PLATFORM RESET after a delay  */
  PRINT_CELLULAR_SERVICE("CST: TRIGGER PLATFORM REBOOT AFTER FOTA UPDATE ...\n\r")
  CST_set_modem_state(&dc_com_db, CA_MODEM_REBOOTING, (uint8_t *)"CA_MODEM_REBOOTING");

  ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 3, ERROR_FATAL);
}

/**
  * @brief  FOTA timeout
  * @param  -
  * @retval -
  */
static void CST_fota_timeout_event_mngt(void)
{
  PRINT_CELLULAR_SERVICE("CST: CST_fota_timeout_event_mngt\n\r")
  /* FOTA timeout has occurred : FAIL */

  PRINT_CELLULAR_SERVICE("CST: FOTA FAIL : Timeout expired RESTART\n\r")

  (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(dc_cellular_info_t));
  cst_cellular_info.modem_state = CA_MODEM_REBOOTING;
  (void)dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(dc_cellular_info_t));

  ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 4, ERROR_FATAL);
}

/**
  * @brief  modem reset processing
  * @param  -
  * @retval -
  */
static void CST_modem_reset_mngt(void)
{
  PRINT_CELLULAR_SERVICE("CST: CST_modem_reset_mngt\n\r")

  /* Data Cache -> Data transfer off */
  CST_data_cache_cellular_info_set(DC_SERVICE_OFF, NULL);

  /* Data Cache -> Radio ON */
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(dc_cellular_info_t));
  cst_cellular_info.rt_state = DC_SERVICE_ON;
  (void)dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(dc_cellular_info_t));
  CST_set_modem_state(&dc_com_db, CA_MODEM_REBOOTING, (uint8_t *)"CA_MODEM_REBOOTING");

  /* current state changes */
  CST_set_state(CST_MODEM_INIT_STATE);
  CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_INIT_EVENT);
}

/**
  * @brief  modem reboot processing
  * @param  -
  * @retval -
  */
static void CST_modem_reboot_mngt(void)
{
  CS_Status_t cs_status;
  PRINT_CELLULAR_SERVICE("CST: CST_modem_reboot_mngt\n\r")

  /* modem power off */
  cs_status = CST_modem_power_off();

  if (cs_status != CELLULAR_OK)
  {
    CST_config_fail(((uint8_t *)"CST_modem_reboot_mngt"),
                    CST_MODEM_RESET_FAIL,
                    &cst_context.reset_count,
                    CST_RESET_MAX);
  }
  else
  {
    /* Data Cache -> Radio ON */
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(dc_cellular_info_t));
    cst_cellular_info.rt_state = DC_SERVICE_ON;
    (void)dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(dc_cellular_info_t));
    CST_set_modem_state(&dc_com_db, CA_MODEM_REBOOTING, (uint8_t *)"CA_MODEM_REBOOTING");
    /* current state changes */
    CST_set_state(CST_MODEM_INIT_STATE);
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_INIT_EVENT);
  }
}

/**
  * @brief  network registration management
  * @param  -
  * @retval -
  */
static void  CST_net_register_mngt(void)
{
  CS_Status_t cs_status;
  CS_RegistrationStatus_t  cst_ctxt_reg_status;
  /* operator context */
  static CS_OperatorSelector_t    ctxt_operator ;

  PRINT_CELLULAR_SERVICE("CST: CST_net_register_mngt\n\r")

  /* Read data from data cache to initialize ctxt_operator */
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cst_cellular_params, sizeof(cst_cellular_params));

  ctxt_operator.mode = (CS_NetworkRegMode_t)cst_cellular_params.operator_selector.network_reg_mode;
  ctxt_operator.format = (CS_OperatorNameFormat_t)cst_cellular_params.operator_selector.operator_name_format;
  (void)memcpy(ctxt_operator.operator_name, cst_cellular_params.operator_selector.operator_name,
               sizeof(ctxt_operator.operator_name));
  if (cst_cellular_params.operator_selector.access_techno_present == CA_ACT_PRESENT)
  {
    ctxt_operator.AcT_present = (CS_Bool_t)true;
  }
  else
  {
    ctxt_operator.AcT_present = (CS_Bool_t)false;
  }

  ctxt_operator.AcT = (CS_AccessTechno_t)cst_cellular_params.operator_selector.access_techno;

  /* find network and register when found */
  PRINT_CELLULAR_SERVICE("CST: osCDS_register_net()\n\r")
  cs_status = osCDS_register_net(&ctxt_operator, &cst_ctxt_reg_status);
  if (cs_status == CELLULAR_OK)
  {
    /* service OK: update current network status */
    cst_context.current_EPS_NetworkRegState  = cst_ctxt_reg_status.EPS_NetworkRegState;
    cst_context.current_GPRS_NetworkRegState = cst_ctxt_reg_status.GPRS_NetworkRegState;
    cst_context.current_CS_NetworkRegState   = cst_ctxt_reg_status.CS_NetworkRegState;
    /*   to force to attach to PS domain by default (in case the Modem does not perform automatic PS attach.) */
    /*   need to check target state in future. */
    PRINT_CELLULAR_SERVICE("CST: osCDS_attach_PS_domain()\n\r")
    (void)osCDS_attach_PS_domain();

    CST_set_modem_state(&dc_com_db, CA_MODEM_NETWORK_REGISTERED, (uint8_t *)"CA_MODEM_NETWORK_REGISTERED");

    /* current state changes */
    CST_set_state(CST_WAITING_FOR_SIGNAL_QUALITY_OK_STATE);
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_SIGNAL_QUALITY_TO_CHECK_EVENT);
  }
  else
  {
    /* unrecoverable error : fail */
    CST_set_modem_state(&dc_com_db, CA_MODEM_NETWORK_SEARCHING, (uint8_t *)"CA_MODEM_NETWORK_SEARCHING");

    CST_config_fail(((uint8_t *)"CST_net_register_mngt"),
                    CST_MODEM_REGISTER_FAIL,
                    &cst_context.register_reset_count,
                    CST_REGISTER_RESET_MAX);

    cst_context.current_EPS_NetworkRegState  = CS_NRS_NOT_REGISTERED_SEARCHING;
    cst_context.current_GPRS_NetworkRegState = CS_NRS_NOT_REGISTERED_SEARCHING;
    cst_context.current_CS_NetworkRegState   = CS_NRS_NOT_REGISTERED_SEARCHING;
  }
}

/**
  * @brief  test if modem catch right signal
  * @param  -
  * @retval -
  */
static void  CST_signal_quality_test_mngt(void)
{
  CS_Status_t cs_status;
  cs_status = CST_set_signal_quality();

  PRINT_CELLULAR_SERVICE("CST: CST_signal_quality_test_mngt\n\r")
  if (cs_status == CELLULAR_OK)
  {
    /* Signal Quality is OK */
    /* Start attachment timeout */
    (void)rtosalTimerStart(cst_network_status_timer_handle, cst_cellular_params.attachment_timeout);
    PRINT_CELLULAR_SERVICE("CST: --> Start NW REG TIMEOUT TIMER : %ld\n\r", cst_cellular_params.attachment_timeout)

    /* current state changes */
    CST_set_state(CST_WAITING_FOR_NETWORK_STATUS_STATE);
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_NETWORK_STATUS_TO_CHECK_EVENT);
  }

}

/**
  * @brief  test if network status is OK
  * @param  -
  * @retval -
  */
static void  CST_network_status_test_mngt(void)
{
  CS_Status_t cs_status;
  cst_network_status_t ret;

  PRINT_CELLULAR_SERVICE("CST: CST_network_status_test_mngt\n\r")

  ret = CST_get_network_status();

  if (ret == CST_NET_REGISTERED)
  {
    /* When registered then stop the NW REG TIMER */
    (void)rtosalTimerStop(cst_network_status_timer_handle);
    PRINT_CELLULAR_SERVICE("CST: --> Stop NW REG TIMEOUT TIMER\n\r")
    cst_context.register_retry_tempo_count = 0U;

    CST_set_modem_state(&dc_com_db, CA_MODEM_NETWORK_REGISTERED, (uint8_t *)"CA_MODEM_NETWORK_REGISTERED");

    CST_set_state(CST_NETWORK_STATUS_OK_STATE);
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_NETWORK_STATUS_OK_EVENT);
  }
  else if (ret == CST_NOT_REGISTERED)
  {
    /* check signal quality if OK then does nothing, wait that Modem completes the registration to the network */
    /* if signal level is KO we lost the signal then enter to the wait for signal state*/
    cs_status = CST_set_signal_quality();

    if (cs_status != CELLULAR_OK)
    {
      /* When registered then stop the NW REG TIMER */
      (void)rtosalTimerStop(cst_network_status_timer_handle);
      PRINT_CELLULAR_SERVICE("CST: --> BAD SIGNAL : Stop NW REG TIMEOUT TIMER\n\r")
      cst_context.register_retry_tempo_count = 0U;

      CST_set_modem_state(&dc_com_db, CA_MODEM_NETWORK_SEARCHING, (uint8_t *)"CA_MODEM_NETWORK_SEARCHING");

      CST_set_state(CST_WAITING_FOR_SIGNAL_QUALITY_OK_STATE);
      CST_send_message(CST_MESSAGE_CS_EVENT, CST_SIGNAL_QUALITY_TO_CHECK_EVENT);
    }
  }
  else if (ret == CST_NET_UNKNOWN)
  {
    /* does nothing just ignore*/
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_NO_EVENT);
  }
  else
  {
    /* unrecoverable error : fail */
    CST_config_fail(((uint8_t *)"CST_network_status_test_mngt"),
                    CST_MODEM_NETWORK_FAIL,
                    &cst_context.network_reset_count,
                    CST_NETWORK_RESET_MAX);
  }
}



/**
  * @brief  management of pdn network detach event
  * @param  -
  * @retval -
  */
static void  CST_pdn_event_nw_detach_mngt(void)
{
  cst_network_status_t ret;

  PRINT_CELLULAR_SERVICE("CST: CST_pdn_event_nw_detach_mngt\n\r")
  if (cst_context.current_state == CST_MODEM_DATA_READY_STATE)
  {
    /* if current state is data ready then close data transfer service */
    CST_data_cache_cellular_info_set(DC_SERVICE_OFF, NULL);

    CST_set_modem_state(&dc_com_db, CA_MODEM_STATE_POWERED_ON, (uint8_t *)"CA_MODEM_STATE_POWERED_ON");
  }

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  osCCS_get_wait_cs_resource();
  PRINT_CELLULAR_SERVICE("CST: osCDS_suspend_data()\n\r")
  (void)osCDS_suspend_data();
#endif  /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */

  /* get new network status */
  ret = CST_get_network_status();

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  osCCS_get_release_cs_resource();
#endif      /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */

  if (ret == CST_NET_REGISTERED)
  {
    /* Modem registered to network : Nothing to do */
  }
  else if (ret == CST_NOT_REGISTERED)
  {
    /* Modem no more registered to network : set data transfer service off */
    CST_data_cache_cellular_info_set(DC_SERVICE_OFF, NULL);

    CST_set_modem_state(&dc_com_db, CA_MODEM_STATE_POWERED_ON, (uint8_t *)"CA_MODEM_STATE_POWERED_ON");
  }
  else
  {
    /* network status not available : set data transfer service off */
    CST_data_cache_cellular_info_set(DC_SERVICE_OFF, NULL);

    CST_set_modem_state(&dc_com_db, CA_MODEM_STATE_POWERED_ON, (uint8_t *)"CA_MODEM_STATE_POWERED_ON");
    PRINT_CELLULAR_SERVICE("CST: osCDS_get_net_status FAIL\n\r")
  }

  /* current state changes */
  CST_set_state(CST_WAITING_FOR_NETWORK_STATUS_STATE);
}

/**
  * @brief  network event processing
  * @param  -
  * @retval -
  */
static void  CST_network_event_mngt(void)
{
  cst_network_status_t ret;

  /* get network status */
  ret = CST_get_network_status();

  if (ret == CST_NET_REGISTERED)
  {
    /* Modem registered to network */
    CST_set_state(CST_NETWORK_STATUS_OK_STATE);
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_NETWORK_STATUS_OK_EVENT);
  }
  else if (ret == CST_NOT_REGISTERED)
  {
    /* Modem no more registered to network : set data transfer service off */
    CST_data_cache_cellular_info_set(DC_SERVICE_OFF, NULL);
    CST_set_state(CST_WAITING_FOR_NETWORK_STATUS_STATE);
#if (USE_LOW_POWER == 1)
    /* need to exit the low power state. COM is in charge of configuration to enter in low power when registered */
    /* again to the network should be double check if still needed. */
    CSP_WakeupComplete();
#endif /* (USE_LOW_POWER == 1) */
    CST_set_modem_state(&dc_com_db, CA_MODEM_NETWORK_SEARCHING, (uint8_t *)"CA_MODEM_NETWORK_SEARCHING");

    /* Not event sent because waiting for network status modification (timer polling or modem urc */
  }
  else if (ret == CST_NET_UNKNOWN)
  {
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_NO_EVENT);
  }
  else
  {
    /* network status not available : set data transfer service off */
    CST_data_cache_cellular_info_set(DC_SERVICE_OFF, NULL);
    PRINT_CELLULAR_SERVICE("CST: osCDS_get_net_status FAIL\n\r")
    /* unrecoverable error : fail */
    CST_config_fail(((uint8_t *)"CST_network_status_test_mngt"),
                    CST_MODEM_NETWORK_FAIL,
                    &cst_context.network_reset_count,
                    CST_NETWORK_RESET_MAX);
  }
}


/**
  * @brief  network event processing
  * @param  -
  * @retval -
  */
static void  CST_data_ready_state_network_event_mngt(void)
{
  cst_network_status_t ret;

  /* get network status */
  ret = CST_get_network_status();

  if (ret == CST_NET_REGISTERED)
  {
    /* Modem registered to network */
    CST_set_modem_state(&dc_com_db, CA_MODEM_STATE_DATAREADY, (uint8_t *)"CA_MODEM_STATE_DATAREADY");

    CST_send_message(CST_MESSAGE_CS_EVENT, CST_NO_EVENT);
  }
  else if (ret == CST_NOT_REGISTERED)
  {
    /* Modem no more registered to network : set data transfer service off */
    CST_data_cache_cellular_info_set(DC_SERVICE_OFF, NULL);
    CST_set_state(CST_WAITING_FOR_NETWORK_STATUS_STATE);
#if (USE_LOW_POWER == 1)
    /* need to exit the low power state. COM is in charge of configuration to enter in low power when registered */
    /* again to the network should be double check if still needed. */
    CSP_WakeupComplete();
#endif /* (USE_LOW_POWER == 1) */
    CST_set_modem_state(&dc_com_db, CA_MODEM_NETWORK_SEARCHING, (uint8_t *)"CA_MODEM_NETWORK_SEARCHING");
    /* Not event sent because waiting for network status modification (timer polling or modem urc */
  }
  else if (ret == CST_NET_UNKNOWN)
  {
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_NO_EVENT);
  }
  else
  {
    /* network status not available : set data transfer service off */
    CST_data_cache_cellular_info_set(DC_SERVICE_OFF, NULL);
    PRINT_CELLULAR_SERVICE("CST: osCDS_get_net_status FAIL\n\r")
    /* unrecoverable error : fail */
    CST_config_fail(((uint8_t *)"CST_network_status_test_mngt"),
                    CST_MODEM_NETWORK_FAIL,
                    &cst_context.network_reset_count,
                    CST_NETWORK_RESET_MAX);
  }
}

/**
  * @brief  NW REG timeout expiration
  * @param  -
  * @retval -
  */

static void CST_nw_reg_timeout_expiration_mngt(void)
{
  PRINT_CELLULAR_SERVICE("CST: CST_nw_reg_timeout_expiration_mngt\n\r")
  PRINT_CELLULAR_SERVICE("CST: --> NW REG TIMEOUT TIMER EXPIRY WE PWDN THE MODEM \n\r")
  if (cst_nfmc_context.active == true)
  {
    cst_nfmc_context.nfmc_timer_on_going = true;

    /* modem power off */
    (void)CST_modem_power_off();

    CST_set_modem_state(&dc_com_db, CA_MODEM_POWER_OFF, (uint8_t *)"CA_MODEM_POWER_OFF");


    /* start retry NFMC tempo */
    (void)rtosalTimerStart(cst_register_retry_timer_handle,
                           cst_nfmc_context.tempo[cst_context.register_retry_tempo_count]);
    PRINT_CELLULAR_SERVICE("CST: --> CST_waiting_for_network_status NOK - retry tempo %d : %ld\n\r",
                           cst_context.register_retry_tempo_count + 1U,
                           cst_nfmc_context.tempo[cst_context.register_retry_tempo_count])

    cst_context.register_retry_tempo_count++;
    if (cst_context.register_retry_tempo_count >= CA_NFMC_VALUES_MAX_NB)
    {
      /* last NFMC tempo reached: restart with the first NFMC tempo */
      cst_context.register_retry_tempo_count = 0U;
    }
  }
}

/**
  * @brief  network attachment management
  * @param  -
  * @retval -
  */
static void  CST_attach_modem_mngt(void)
{
  CS_Status_t              cs_status;
  CS_PSattach_t            cst_ctxt_attach_status;
  CS_RegistrationStatus_t  reg_status;

  PRINT_CELLULAR_SERVICE("CST: CST_attach_modem_mngt\n\r")

  (void)memset((void *)&reg_status, 0, sizeof(CS_RegistrationStatus_t));

  /* gets net status */
  PRINT_CELLULAR_SERVICE("CST: osCDS_get_net_status()\n\r")
  cs_status = osCDS_get_net_status(&reg_status);
  if (cs_status == CELLULAR_OK)
  {
    /* service available */
    if (((uint16_t)reg_status.optional_fields_presence & (uint16_t)CS_RSF_FORMAT_PRESENT) != 0U)
    {
      /* MNO present: store it in DC */
      (void)dc_com_read(&dc_com_db,  DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));
      (void)memcpy(cst_cellular_info.mno_name, reg_status.operator_name, CA_MNO_NAME_SIZE_MAX - 1U);
      cst_cellular_info.mno_name[CA_MNO_NAME_SIZE_MAX - 1U] = 0U;  /* to avoid a non null terminated string */
      cst_cellular_info.rt_state              = DC_SERVICE_ON;
      (void)dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));

      PRINT_CELLULAR_SERVICE("CST: --> operator_name = %s\n\r", reg_status.operator_name)
    }
  }

  PRINT_CELLULAR_SERVICE("CST: osCDS_get_attach_status()\n\r")
  cs_status = osCDS_get_attach_status(&cst_ctxt_attach_status);
  if (cs_status != CELLULAR_OK)
  {
    /* service not available : FAIL */
    PRINT_CELLULAR_SERVICE("CST: --> attach modem fail\n\r")

    CST_set_modem_state(&dc_com_db, CA_MODEM_NETWORK_SEARCHING, (uint8_t *)"CA_MODEM_NETWORK_SEARCHING");

    CST_config_fail(((uint8_t *)"CS_get_attach_status FAIL"),
                    CST_MODEM_ATTACH_FAIL,
                    &cst_context.attach_reset_count,
                    CST_ATTACH_RESET_MAX);
  }
  else
  {
    /* service available */
    if (cst_ctxt_attach_status == CS_PS_ATTACHED)
    {
      /* modem attached */
      PRINT_CELLULAR_SERVICE("CST: --> attach modem OK\n\r")

      CST_set_modem_state(&dc_com_db, CA_MODEM_NETWORK_REGISTERED, (uint8_t *)"CA_MODEM_NETWORK_REGISTERED");

      CST_set_state(CST_MODEM_REGISTERED_STATE);
      CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_ATTACHED_EVENT);
    }

    else
    {
      /* modem not attached */
      PRINT_CELLULAR_SERVICE("CST: --> NOT ATTACHED !!!\n\r")

      CST_set_modem_state(&dc_com_db, CA_MODEM_NETWORK_SEARCHING, (uint8_t *)"CA_MODEM_NETWORK_SEARCHING");

      /* We propose to simply wait network event at "waiting for signal quality OK" state */
      PRINT_CELLULAR_SERVICE("CST: --> wait for quality state OK\n\r")
      CST_set_state(CST_WAITING_FOR_SIGNAL_QUALITY_OK_STATE);
    }
  }
}

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
static void CST_ppp_config_mngt(void)
{
  ppposif_status_t err;

  err = ppposif_client_config();
  if (err != 0U)
  {
    CST_config_fail(((uint8_t *)"CST_ppp_config_mngt"),
                    CST_PPP_FAIL,
                    &cst_context.ppp_fail_count,
                    CST_PPP_FAIL_MAX);



  }
  else
  {
    CST_data_cache_cellular_info_set(DC_SERVICE_STARTING, NULL);
    CST_set_state(CST_PPP_CONFIG_ON_GOING_STATE);
  }
}
#endif  /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */

static void CST_data_ready_mngt(void)
{
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
#else
  CS_IPaddrType_t  ip_addr_type;
  dc_network_addr_t ip_addr;
  uint32_t err;
  static uint8_t cs_ip_addr[DC_MAX_IP_ADDR_SIZE];
#endif  /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
  /* resets failure couters  */
  cst_context.power_on_reset_count       = 0U;
  cst_context.reset_count                = 0U;
  cst_context.csq_reset_count            = 0U;
  cst_context.attach_reset_count         = 0U;
  cst_context.activate_pdn_reset_count   = 0U;
  cst_context.cellular_data_retry_count  = 0U;
  cst_context.ppp_fail_count             = 0U;
  cst_context.global_retry_count         = 0U;

  /* set state to data trasfert ready */
  CST_set_state(CST_MODEM_DATA_READY_STATE);

  /* Data Cache -> Data transfer available */
  CST_set_modem_state(&dc_com_db, CA_MODEM_STATE_DATAREADY, (uint8_t *)"CA_MODEM_STATE_DATAREADY");

  /* just to trigger the end of transition to check if the next step need to handle low power */
  CST_send_message(CST_MESSAGE_CS_EVENT, CST_NO_EVENT);

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));
  switch (cst_cellular_info.rt_state_ppp)
  {
    case DC_SERVICE_ON:
    {
      /* PPP client ON => data transfer feature available */
      /* update nifman DC entry (service on) */
      CST_data_cache_cellular_info_set(DC_SERVICE_ON, &cst_cellular_info.ip_addr);
      break;
    }
    case DC_SERVICE_OFF:
    {
      CST_data_cache_cellular_info_set(DC_SERVICE_OFF, NULL);
      break;
    }
    default:
    {
      /* all other event => data transfer feature no more available */
      /* update nifman DC entry (service off)                        */
      CST_data_cache_cellular_info_set(DC_SERVICE_FAIL, NULL);
      break;
    }
  }
#else

  (void)CST_get_dev_IP_address(&ip_addr_type, cs_ip_addr);
  err = crc_get_ip_addr(&cs_ip_addr[1], cs_ip_addr, NULL);
  if (err == 0U)
  {
    ip_addr.addr = (uint32_t)cs_ip_addr[0] +
                   ((uint32_t)cs_ip_addr[1] <<  8) +
                   ((uint32_t)cs_ip_addr[2] << 16) +
                   ((uint32_t)cs_ip_addr[3] << 24);
    /* Display trace even in release binary. Used for tests */
    PRINT_FORCE("Network is up with IP %d.%d.%d.%d\r\n",
                cs_ip_addr[0], cs_ip_addr[1], cs_ip_addr[2], cs_ip_addr[3])
  }
  else
  {
    ip_addr.addr = 0;
    /* Display trace even in release binary. Used for tests */
    PRINT_FORCE("Network is up with no IP\r\n")
  }

  CST_data_cache_cellular_info_set(DC_SERVICE_ON, &ip_addr);
#endif  /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
}

/**
  * @brief  modem activated management
  * @param  -
  * @retval -
  */
static void  CST_modem_activated_mngt(void)
{
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  CST_ppp_config_mngt();
#else
  CST_data_ready_mngt();
#endif  /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
}

/**
  * @brief  PDN activation management
  * @param  -
  * @retval -
  */
static void CST_modem_activate_pdn_mngt(void)
{
  CS_Status_t cs_status;

  PRINT_CELLULAR_SERVICE("CST: CST_modem_activate_pdn_mngt\n\r")
  CST_set_state(CST_MODEM_PDN_ACTIVATING_STATE);

  PRINT_CELLULAR_SERVICE("CST: osCDS_set_default_pdn()\n\r")
  (void)osCDS_set_default_pdn(cst_get_cid_value(cst_cellular_params.sim_slot[cst_context.sim_slot_index].cid));

  /* register to PDN events for this CID*/
  PRINT_CELLULAR_SERVICE("CST: osCDS_register_pdn_event()\n\r")
  (void)osCDS_register_pdn_event(cst_get_cid_value(cst_cellular_params.sim_slot[cst_context.sim_slot_index].cid),
                                 CST_pdn_event_callback);

  PRINT_CELLULAR_SERVICE("CST: osCDS_activate_pdn()\n\r")
  cs_status = osCDS_activate_pdn(CS_PDN_CONFIG_DEFAULT);

  if (cs_status != CELLULAR_OK)
  {
    if (cst_nfmc_context.active == false)
    {
      (void)rtosalTimerStart(cst_pdn_activate_retry_timer_handle, CST_PDN_ACTIVATE_RETRY_DELAY);
      PRINT_CELLULAR_SERVICE("CST: --> NOK - retry tempo  : %d\n\r",
                             CST_PDN_ACTIVATE_RETRY_DELAY)
    }
    else
    {
      (void)rtosalTimerStart(cst_pdn_activate_retry_timer_handle,
                             cst_nfmc_context.tempo[cst_context.activate_pdn_nfmc_tempo_count]);
      PRINT_CELLULAR_SERVICE("CST: --> NOK - retry tempo %d : %ld\n\r",
                             cst_context.activate_pdn_nfmc_tempo_count + 1U,
                             cst_nfmc_context.tempo[cst_context.activate_pdn_nfmc_tempo_count])
    }

    cst_context.activate_pdn_nfmc_tempo_count++;
    if (cst_context.activate_pdn_nfmc_tempo_count >= CA_NFMC_VALUES_MAX_NB)
    {
      /* tempo count has incoherent value so it is assumed that the tempo values are not consistent */
      cst_context.activate_pdn_nfmc_tempo_count = 0U;
    }
  }
  else
  {
    cst_context.activate_pdn_nfmc_tempo_count = 0U;
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_PDP_ACTIVATED_EVENT);
  }
}

/**
  * @brief  cellular data failure management
  * @param  -
  * @retval -
  */
static void CST_cellular_data_fail_mngt(void)
{
  if (cst_context.current_state == CST_MODEM_DATA_READY_STATE)
  {
    CST_data_cache_cellular_info_set(DC_SERVICE_OFF, NULL);
    CST_set_modem_state(&dc_com_db, CA_MODEM_NETWORK_SEARCHING, (uint8_t *)"CA_MODEM_NETWORK_SEARCHING");


#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
    osCCS_get_wait_cs_resource();
    PRINT_CELLULAR_SERVICE("CST: osCDS_suspend_data()\n\r")
    (void)osCDS_suspend_data(); /* osCDS_stop_data should be the called */
    osCCS_get_release_cs_resource();
#endif  /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
  }
  /* when DATA plane fails then we just wait event in signal state */
  CST_set_state(CST_WAITING_FOR_SIGNAL_QUALITY_OK_STATE);
}

/**
  * @brief  PDN event management
  * @param  -
  * @retval -
  */
static void CST_pdn_event_mngt(void)
{
  PRINT_CELLULAR_SERVICE("CST: CST_pdn_event_mngt\n\r")
  CST_data_cache_cellular_info_set(DC_SERVICE_OFF, NULL);
#if (USE_LOW_POWER == 1)
  /* need to exit the low power state. COM is in charge of configuration to enter in low power when registered */
  /* again to the network should be double check if still needed. */
  CSP_WakeupComplete();
#endif /* (USE_LOW_POWER == 1) */
  if (cst_context.pdn_status == CS_PDN_EVENT_NW_DETACH)
  {
    /* Workaround waiting for Modem behaviour clarification */
    CST_pdn_event_nw_detach_mngt();
  }
  else if (
    (cst_context.pdn_status == CS_PDN_EVENT_NW_DEACT)
    || (cst_context.pdn_status == CS_PDN_EVENT_NW_PDN_DEACT))
  {
    PRINT_CELLULAR_SERVICE("CST: CS_PDN_EVENT_NW_DEACT\n\r")
    CST_set_state(CST_MODEM_REGISTERED_STATE);
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_ATTACHED_EVENT);
  }
  else
  {
    CST_set_state(CST_WAITING_FOR_NETWORK_STATUS_STATE);
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_NETWORK_CALLBACK_EVENT);
  }
}

/**
  * @brief  cellular target state event management
  * @param  -
  * @retval -
  */
static void CST_target_state_cmd_event_mngt(void)
{
  if (cst_cellular_params.target_state == DC_TARGET_STATE_SIM_ONLY)
  {
    /* new Modem target state 'SIM ONLY' requested*/
    PRINT_CELLULAR_SERVICE("CST: Transition to CST_MODEM_SIM_ONLY_STATE Ongoing\n\r")
    /* reinit the mode with SIM_ONLY mode */
    PRINT_CELLULAR_SERVICE("CST: osCDS_init_modem()\n\r")
    (void) osCDS_init_modem(CS_CMI_SIM_ONLY, CELLULAR_FALSE, PLF_CELLULAR_SIM_PINCODE);

    /* set 'SIM_CONNECTED in Data Cache to inform application */
    CST_set_modem_state(&dc_com_db, CA_MODEM_STATE_SIM_CONNECTED, (uint8_t *)"CA_MODEM_STATE_SIM_CONNECTED");

    /* set the automaton to SIM_ONLY_STATE (no event to send because the automation remain in this state
       until a new modem target state will be required  */
    CST_set_state(CST_MODEM_SIM_ONLY_STATE);
    PRINT_CELLULAR_SERVICE("CST: Transition to CST_MODEM_SIM_ONLY_STATE\n\r")
  }
  else if (cst_cellular_params.target_state == DC_TARGET_STATE_OFF)
  {
    /* New target state to reach: modem off */
    /* SIM info not available */
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_SIM_INFO, (void *)&cst_sim_info, sizeof(cst_sim_info));
    cst_sim_info.rt_state = DC_SERVICE_OFF;
    (void)dc_com_write(&dc_com_db, DC_CELLULAR_SIM_INFO, (void *)&cst_sim_info, sizeof(cst_sim_info));

    /* modem power off */
    (void)CST_modem_power_off();
    CST_set_modem_state(&dc_com_db, CA_MODEM_POWER_OFF, (uint8_t *)"CA_MODEM_POWER_OFF");

    /* set the automaton to MODEM_OFF_STATE (cellular_info Data Cache entry
       will be updated when the state will be reached) */
    CST_set_state(CST_MODEM_OFF_STATE);
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_INIT_EVENT);
  }
  else
  {
    /* DC_TARGET_STATE_FULL : Nothing to do. Modem configuration will continue */
    __NOP();
  }
}


/**
  * @brief  polling management to monitor Signal Quality in 'DATA READY mode'
  * @param  -
  * @retval -
  */
static void CST_polling_timer_mngt(void)
{

#if (CST_MODEM_POLLING_PERIOD != 0)
  if (CST_polling_active == true)
  {
    if (CST_polling_on_going == false)
    {
      CST_polling_on_going = true;
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
      osCCS_get_wait_cs_resource();
      /* we should read the status if connection lost while changing to at command mode */
      PRINT_CELLULAR_SERVICE("CST: osCDS_suspend_data()\n\r")
      CS_Status_t cs_status = osCDS_suspend_data();

      /* For instance disable the signal polling to test suspend resume  */
      (void)CST_set_signal_quality();
      /* we should read the status if connection lost while resuming data */
      PRINT_CELLULAR_SERVICE("CST: osCDS_resume_data()\n\r")
      cs_status = osCDS_resume_data();
      osCCS_get_release_cs_resource();
      if (cs_status != CELLULAR_OK)
      {
        /* to add resume_data failure */
        CST_cellular_data_fail_mngt();
      }
#else
      /* For instance disable the signal polling to test suspend resume  */
      (void)CST_set_signal_quality();
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
      CST_polling_on_going = false;
    }
    else
    {
      PRINT_CELLULAR_SERVICE("CST: Discard pooling timer, another one is already on going\n\r")
    }
  }
#endif /* CST_MODEM_POLLING_PERIOD != 0) */
}

/**
  * @brief  new apn config requested by application : set it in Data Cache
  * @param  -
  * @retval -
  */
static void CST_apn_set_new_config_mngt(void)
{
  /* read current cellular config in Data Cache */
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cst_cellular_params, sizeof(cst_cellular_params));

  /* set Data Cache Entry valid */
  cst_cellular_params.rt_state = DC_SERVICE_ON;

  /* reset value of apnChanged */
  cst_cellular_params.sim_slot[cst_context.sim_slot_index].apnChanged = false;

  (void)dc_com_write(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cst_cellular_params, sizeof(cst_cellular_params));

  /* Modem reset to be reconfigured with new APN */
  CST_set_state(CST_MODEM_RESET_STATE);
  CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_RESET_EVENT);
}

static void CST_data_mode_target_state_event_mngt(void)
{
  /*  we check target state to process any target state change while Network IF is being established. */
  if (cst_cellular_params.target_state != DC_TARGET_STATE_FULL)
  {
    /* the target state has changed: not more FULL */
    /* Close network Data transfer service */
    CST_data_cache_cellular_info_set(DC_SERVICE_OFF, NULL);

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
    /* lwip mode : must close ppp before to set lodem off */
    (void)ppposif_client_close(PPPOSIF_CAUSE_POWER_OFF);
    CST_set_state(CST_PPP_CLOSE_ON_GOING_STATE);
#else
    /* apply target state command */
    CST_target_state_cmd_event_mngt();
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_LWIP */
  }
}

/**
  * @brief  target state event management in case of current state == SIM_ ONLY
  * @param  -
  * @retval -
  */
static void CST_sim_only_target_state_event_mngt(void)
{
  if (cst_cellular_params.target_state == DC_TARGET_STATE_FULL)
  {
    CST_modem_sim_init();
  }
  else if (cst_cellular_params.target_state == DC_TARGET_STATE_OFF)
  {
    /* new Modem target state 'MODEM OFF' requested */
    /* modem power off */
    (void)CST_modem_power_off();

    CST_set_modem_state(&dc_com_db, CA_MODEM_POWER_OFF, (uint8_t *)"CA_MODEM_POWER_OFF");

    CST_set_state(CST_MODEM_OFF_STATE);
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_INIT_EVENT);
  }
  else
  {
    /* Nothing to do */
    __NOP();
  }
}

/**
  * @brief  set modem off
  * @param  -
  * @retval -
  */
static void CST_modem_off_mngt(void)
{
  CST_set_modem_state(&dc_com_db, CA_MODEM_POWER_OFF, (uint8_t *)"CA_MODEM_POWER_OFF");

}

/* ===================================================================
   Automaton processing functions  END
   =================================================================== */

/* =================================================================
   automaton state functions BEGIN
   ================================================================= */
/**
  * @brief  boot state processing
  * @param  autom_event - automaton event
  * @retval -
  */
static void CST_boot_state(cst_autom_event_t autom_event)
{
#if (USE_LOW_POWER == 1)
  (void)rtosalTimerStop(cst_lp_inactivity_timer_handle);
#endif /* (USE_LOW_POWER == 1) */
  switch (autom_event)
  {
    case CST_BOOT_EVENT:
      CST_boot_event_mngt();
      break;

    case CST_REBOOT_MODEM_EVENT:
      CST_reboot_modem_event_mngt();
      break;

    case CST_MODEM_POWER_ON_ONLY_EVENT:
      CST_boot_power_on_only_modem_mngt();
      break;

    default:
      /* Nothing to do */
      break;
  }
}

/**
  * @brief  init state processing
  * @param  autom_event - automaton event
  * @retval -
  */
static void CST_init_state(cst_autom_event_t autom_event)
{
#if (USE_LOW_POWER == 1)
  (void)rtosalTimerStop(cst_lp_inactivity_timer_handle);
#endif /* (USE_LOW_POWER == 1) */
  switch (autom_event)
  {
    case CST_MODEM_INIT_EVENT:
      CST_init_state_mngt();
      break;

    case CST_SIM_RESET_EVENT:
      CST_init_state_sim_reset_mngt();
      break;

    case CST_MODEM_POWER_ON_ONLY_EVENT:
      CST_init_power_on_only_modem_mngt();
      break;

    case CST_REBOOT_MODEM_EVENT:
      CST_reboot_modem_event_mngt();
      break;

    case CST_TARGET_STATE_CMD_EVENT:
      CST_target_state_cmd_event_mngt();
      break;

    case CST_FOTA_START_EVENT:
      CST_fota_start_event_mngt();
      break;

    default:
      /* Nothing to do */
      break;
  }
  /* subscribe modem events after power ON */
  CST_subscribe_modem_events();
}


/**
  * @brief  modem ready state processing
  * @param  autom_event - automaton event
  * @retval -
  */
static void CST_modem_ready_state(cst_autom_event_t autom_event)
{
#if (USE_LOW_POWER == 1)
  (void)rtosalTimerStop(cst_lp_inactivity_timer_handle);
#endif /* (USE_LOW_POWER == 1) */
  switch (autom_event)
  {
    case CST_MODEM_READY_EVENT:
      CST_net_register_mngt();
      break;

    case CST_TARGET_STATE_CMD_EVENT:
      CST_target_state_cmd_event_mngt();
      break;

    case CST_REBOOT_MODEM_EVENT:
      CST_reboot_modem_event_mngt();
      break;

    case CST_FOTA_START_EVENT:
      CST_fota_start_event_mngt();
      break;

    default:
      /* Nothing to do */
      break;
  }
}

/**
  * @brief  waiting for signal quality state processing
  * @param  autom_event - automaton event
  * @retval -
  */
static void CST_waiting_for_signal_quality_ok_state(cst_autom_event_t autom_event)
{
  PRINT_CELLULAR_SERVICE("CST: CST_waiting_for_signal_quality_ok_state\n\r")
#if (USE_LOW_POWER == 1)
  (void)rtosalTimerStop(cst_lp_inactivity_timer_handle);
#endif /* (USE_LOW_POWER == 1) */
  switch (autom_event)
  {
    case CST_POLLING_TIMER_EVENT:
    case CST_SIGNAL_QUALITY_TO_CHECK_EVENT:
      CST_signal_quality_test_mngt();
      break;

    case CST_NETWORK_CALLBACK_EVENT:
      CST_network_event_mngt();
      break;

    case CST_TARGET_STATE_CMD_EVENT:
      CST_target_state_cmd_event_mngt();
      break;

    case CST_REBOOT_MODEM_EVENT:
      CST_reboot_modem_event_mngt();
      break;

    case CST_FOTA_START_EVENT:
      CST_fota_start_event_mngt();
      break;

    default:
      /* Nothing to do */
      break;
  }
}

/**
  * @brief  waiting for network status state processing
  * @param  autom_event - automaton event
  * @retval -
  */
static void CST_waiting_for_network_status_state(cst_autom_event_t autom_event)
{
#if (USE_LOW_POWER == 1)
  (void)rtosalTimerStop(cst_lp_inactivity_timer_handle);
#endif /* (USE_LOW_POWER == 1) */
  switch (autom_event)
  {
    case CST_POLLING_TIMER_EVENT:
    case CST_NETWORK_CALLBACK_EVENT:
    case CST_SIGNAL_QUALITY_TO_CHECK_EVENT:
    case CST_NETWORK_STATUS_TO_CHECK_EVENT:
      CST_network_status_test_mngt();
      break;

    case CST_NW_REG_TIMEOUT_TIMER_EVENT:
      CST_nw_reg_timeout_expiration_mngt();
      break;

    case CST_TARGET_STATE_CMD_EVENT:
      CST_target_state_cmd_event_mngt();
      break;

    case CST_REBOOT_MODEM_EVENT:
      CST_reboot_modem_event_mngt();
      break;

    case CST_FOTA_START_EVENT:
      CST_fota_start_event_mngt();
      break;

    default:
      /* Nothing to do */
      break;
  }
}

/**
  * @brief  network status ok state processing
  * @param  autom_event - automaton event
  * @retval -
  */
static void CST_network_status_ok_state(cst_autom_event_t autom_event)
{
#if (USE_LOW_POWER == 1)
  (void)rtosalTimerStop(cst_lp_inactivity_timer_handle);
#endif /* (USE_LOW_POWER == 1) */
  switch (autom_event)
  {
    case CST_NETWORK_STATUS_OK_EVENT:
      CST_attach_modem_mngt();
      break;

    case CST_TARGET_STATE_CMD_EVENT:
      CST_target_state_cmd_event_mngt();
      break;

    case CST_REBOOT_MODEM_EVENT:
      CST_reboot_modem_event_mngt();
      break;

    case CST_FOTA_START_EVENT:
      CST_fota_start_event_mngt();
      break;

    default:
      /* Nothing to do */
      break;
  }
}

/**
  * @brief  modem registered state processing
  * @param  autom_event - automaton event
  * @retval -
  */
static void CST_modem_registered_state(cst_autom_event_t autom_event)
{
#if (USE_LOW_POWER == 1)
  (void)rtosalTimerStop(cst_lp_inactivity_timer_handle);
#endif /* (USE_LOW_POWER == 1) */
  switch (autom_event)
  {
    case CST_MODEM_ATTACHED_EVENT:
      CST_modem_activate_pdn_mngt();
      break;

    case CST_NETWORK_CALLBACK_EVENT:
      CST_network_event_mngt();
      break;

    case CST_TARGET_STATE_CMD_EVENT:
      CST_target_state_cmd_event_mngt();
      break;

    case CST_REBOOT_MODEM_EVENT:
      CST_reboot_modem_event_mngt();
      break;

    case CST_FOTA_START_EVENT:
      CST_fota_start_event_mngt();
      break;

    default:
      /* Nothing to do */
      break;

  }
}

/**
  * @brief  modem PDN activate state processing
  * @param  autom_event - automaton event
  * @retval -
  */
static void CST_modem_pdn_activating_state(cst_autom_event_t autom_event)
{
#if (USE_LOW_POWER == 1)
  (void)rtosalTimerStop(cst_lp_inactivity_timer_handle);
#endif /* (USE_LOW_POWER == 1) */
  switch (autom_event)
  {
    case CST_PDP_ACTIVATED_EVENT:
      CST_modem_activated_mngt();
      break;

    case CST_PDN_ACTIVATE_RETRY_TIMER_EVENT:
      CST_modem_activate_pdn_mngt();
      break;

    case CST_NETWORK_CALLBACK_EVENT:
      CST_network_event_mngt();
      break;

    case CST_PDN_STATUS_TO_CHECK_EVENT:
      CST_pdn_event_mngt();
      break;

    case CST_TARGET_STATE_CMD_EVENT:
      CST_target_state_cmd_event_mngt();
      break;

    case CST_REBOOT_MODEM_EVENT:
      CST_reboot_modem_event_mngt();
      break;

    case CST_FOTA_START_EVENT:
      CST_fota_start_event_mngt();
      break;

    default:
      /* Nothing to do */
      break;
  }
}

/**
  * @brief  data ready state processing
  * @param  autom_event - automaton event
  * @retval -
  */
static void CST_data_ready_state(cst_autom_event_t autom_event)
{
#if (USE_LOW_POWER == 1)
  if ((CSP_GetTargetPowerState() == CSP_LOW_POWER_ACTIVE) &&
      (autom_event != CST_POWER_SLEEP_REQUEST_EVENT))
  {
    (void)rtosalTimerStart(cst_lp_inactivity_timer_handle, cst_cellular_params.lp_inactivity_timeout);
    PRINT_CELLULAR_SERVICE("CST: --> Start LP INACTIVITY TIMEOUT TIMER : %ld\n\r",
                           cst_cellular_params.lp_inactivity_timeout)
  }
#endif /* (USE_LOW_POWER == 1) */
  switch (autom_event)
  {
    case CST_NETWORK_CALLBACK_EVENT:
#if (USE_LOW_POWER == 1)
      if (CSP_GetTargetPowerState() == CSP_LOW_POWER_ACTIVE)
      {
        (void)rtosalTimerStart(cst_lp_inactivity_timer_handle, cst_cellular_params.lp_inactivity_timeout);
        PRINT_CELLULAR_SERVICE("CST: --> Receive network data, RE-Start LP INACTIVITY TIMEOUT TIMER   : %ld\n\r",
                               cst_cellular_params.lp_inactivity_timeout)
      }
#endif /* (USE_LOW_POWER == 1) */
      CST_data_ready_state_network_event_mngt();

      break;

    case CST_POLLING_TIMER_EVENT:
      CST_polling_timer_mngt();
      break;

    case CST_CELLULAR_DATA_FAIL_EVENT:
      CST_cellular_data_fail_mngt();
      break;

    case CST_PDN_STATUS_TO_CHECK_EVENT:
      CST_pdn_event_mngt();
      break;

    case CST_TARGET_STATE_CMD_EVENT:
      CST_data_mode_target_state_event_mngt();
      break;

    case CST_REBOOT_MODEM_EVENT:
    case CST_MODEM_POWER_DOWN_EVENT: /* for instance we reboot it immediately, in future we should start a timer */
      /* before rebooting the Modem */
      CST_reboot_modem_event_mngt();
      break;

    case CST_FOTA_START_EVENT:
      CST_fota_start_event_mngt();
      break;

#if (USE_LOW_POWER == 1)
    case CST_POWER_SLEEP_REQUEST_EVENT:
      CSP_DataIdleManagment();
      break;
#endif  /* (USE_LOW_POWER == 1) */

    default:
      /* Nothing to do */
      break;
  }
}


/**
  * @brief  modem off state processing
  * @param  autom_event - automaton event
  * @retval -
  */
static void CST_modem_off_state(cst_autom_event_t autom_event)
{
#if (USE_LOW_POWER == 1)
  (void)rtosalTimerStop(cst_lp_inactivity_timer_handle);
#endif /* (USE_LOW_POWER == 1) */
  switch (autom_event)
  {
    case CST_TARGET_STATE_CMD_EVENT:
      CST_off_state_target_cmd_state_mngt();
      break;

    case CST_MODEM_INIT_EVENT:
      CST_modem_off_mngt();
      break;

    case CST_REBOOT_MODEM_EVENT:
      CST_reboot_modem_event_mngt();
      break;

    default:
      /* Nothing to do */
      break;
  }
}

/**
  * @brief  modem sim only state processing
  * @param  autom_event - automaton event
  * @retval -
  */
static void CST_modem_sim_only_state(cst_autom_event_t autom_event)
{
#if (USE_LOW_POWER == 1)
  (void)rtosalTimerStop(cst_lp_inactivity_timer_handle);
#endif /* (USE_LOW_POWER == 1) */
  switch (autom_event)
  {
    case CST_TARGET_STATE_CMD_EVENT:
      CST_sim_only_target_state_event_mngt();
      break;

    case CST_REBOOT_MODEM_EVENT:
      CST_reboot_modem_event_mngt();
      break;

    default:
      /* Nothing to do */
      break;
  }
}

/**
  * @brief  reset state processing
  * @param  autom_event - automaton event
  * @retval -
  */
static void CST_modem_reset_state(cst_autom_event_t autom_event)
{
  /* reset is applied  whatever the event */
#if (USE_LOW_POWER == 1)
  (void)rtosalTimerStop(cst_lp_inactivity_timer_handle);
#endif /* (USE_LOW_POWER == 1) */
  UNUSED(autom_event);
  CST_modem_reset_mngt();
}

/**
  * @brief  reboot modem state processing
  * @param  autom_event - automaton event
  * @retval -
  */
static void CST_modem_reboot_state(cst_autom_event_t autom_event)
{
  /* reset is applied  whatever the event */
#if (USE_LOW_POWER == 1)
  (void)rtosalTimerStop(cst_lp_inactivity_timer_handle);
#endif /* (USE_LOW_POWER == 1) */
  UNUSED(autom_event);
  CST_modem_reboot_mngt();
}

/**
  * @brief  FOTA state : modem formware update on going
  * @param  autom_event - automaton event
  * @retval -
  */
static void CST_modem_reprog_state(cst_autom_event_t autom_event)
{
#if (USE_LOW_POWER == 1)
  (void)rtosalTimerStop(cst_lp_inactivity_timer_handle);
#endif /* (USE_LOW_POWER == 1) */
  switch (autom_event)
  {
    case CST_FOTA_END_EVENT:
      CST_fota_end_event_mngt();
      break;

    case CST_FOTA_TIMEOUT_EVENT:
      CST_fota_timeout_event_mngt();
      break;

    case CST_REBOOT_MODEM_EVENT:
      /* Nothing to do because fota timer => board reboot */
      __NOP();
      break;

    default:
      /* Nothing to do */
      break;
  }
}

/**
  * @brief  APN config state : new apn to configure
  * @param  autom_event - automaton event
  * @retval -
  */
static void CST_apn_config_state(cst_autom_event_t autom_event)
{
#if (USE_LOW_POWER == 1)
  (void)rtosalTimerStop(cst_lp_inactivity_timer_handle);
#endif /* (USE_LOW_POWER == 1) */
  switch (autom_event)
  {
    case CST_APN_CONFIG_EVENT:
      CST_apn_set_new_config_mngt();
      break;

    default:
      /* Nothing to do */
      break;
  }
}


/**
  * @brief  power idle state processing
  * @param  autom_event - automaton event
  * @retval -
  */
#if (USE_LOW_POWER == 1)
static void CST_power_idle_state(cst_autom_event_t autom_event)
{
  (void)rtosalTimerStop(cst_lp_inactivity_timer_handle);
  switch (autom_event)
  {
    case CST_POWER_SLEEP_TIMEOUT_EVENT:
      CSP_SleepCancel();
      break;
    case CST_POWER_SLEEP_ABORT_EVENT:
      CSP_WakeupComplete();
      CST_set_state(CST_MODEM_DATA_READY_STATE);
      break;

    case CST_POWER_SLEEP_COMPLETE_EVENT:
      CSP_SleepComplete();
      break;

    case CST_POWER_WAKEUP_EVENT:
      (void)rtosalTimerStop(cst_lp_inactivity_timer_handle);
      PRINT_CELLULAR_SERVICE("CST: osCS_PowerWakeup()\n\r")
      (void)osCS_PowerWakeup(HOST_WAKEUP);
      CSP_WakeupComplete();
      CST_set_state(CST_MODEM_DATA_READY_STATE);
      break;

    case CST_POWER_MODEM_WAKEUP_EVENT:
      (void)rtosalTimerStop(cst_lp_inactivity_timer_handle);
      /* stop the modem sleep request protection timer (to be sure it won't fire after) */
      CSP_StopTimeout();
      PRINT_CELLULAR_SERVICE("CST: osCS_PowerWakeup()\n\r")
      (void)osCS_PowerWakeup(MODEM_WAKEUP);
      CSP_WakeupComplete();
      CST_set_state(CST_MODEM_DATA_READY_STATE);
      break;

    case CST_REBOOT_MODEM_EVENT:
      /* If low power is in use, modem has been wake up before CST_REBOOT_MODEM_EVENT event */
      CST_reboot_modem_event_mngt();
      break;

    case CST_MODEM_POWER_DOWN_EVENT:
      CSP_InitPowerConfig(); /* We consider the Modem as in Power down, we force init. */
      CST_reboot_modem_event_mngt(); /* We reboot it for instance but we should only inform app */

      break;

    case CST_FOTA_START_EVENT:
      CST_fota_start_event_mngt();
      break;

    case CST_TARGET_STATE_CMD_EVENT:
      CST_target_state_cmd_event_mngt();
      break;

    default:
      /* Nothing to do */
      break;
  }
}
#endif /* (USE_LOW_POWER == 1) */

/**
  * @brief  Maximum error number reached: Automaton remain in this state until CST_REBOOT_MODEM_EVENT event
  * @param  autom_event - automaton event
  * @retval -
  */
static void CST_fail_state(cst_autom_event_t autom_event)
{
#if (USE_LOW_POWER == 1)
  (void)rtosalTimerStop(cst_lp_inactivity_timer_handle);
#endif /* (USE_LOW_POWER == 1) */
  switch (autom_event)
  {
    case CST_REBOOT_MODEM_EVENT:
      CST_reboot_modem_event_mngt();
      break;

    default:
      /* Nothing to do */
      break;
  }
}

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
/**
  * @brief  ppp closed state management
  * @param  autom_event - automaton event
  * @retval -
  */
static void CST_ppp_closed_event_mngt(void)
{
#if (USE_LOW_POWER == 1)
  (void)rtosalTimerStop(cst_lp_inactivity_timer_handle);
#endif /* (USE_LOW_POWER == 1) */
  /* back to modem command mode */
  osCCS_get_wait_cs_resource();
  PRINT_CELLULAR_SERVICE("CST: osCDS_suspend_data()\n\r")
  (void)osCDS_suspend_data();
  osCCS_get_release_cs_resource();

  /* configure modem off state */
  CST_target_state_cmd_event_mngt();
}

/**
  * @brief  ppp open state management
  * @param  autom_event - automaton event
  * @retval -
  */
static void CST_ppp_opened_event_mngt(void)
{
#if (USE_LOW_POWER == 1)
  (void)rtosalTimerStop(cst_lp_inactivity_timer_handle);
#endif /* (USE_LOW_POWER == 1) */
  dc_nifman_info_t nifman_info;

  /* PPP client event reveived */
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));
  switch (cst_cellular_info.rt_state_ppp)
  {
    case DC_SERVICE_ON:
    {
      CST_data_ready_mngt();
      break;
    }
    case DC_SERVICE_OFF:
    {
      (void)dc_com_read(&dc_com_db, DC_CELLULAR_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
      nifman_info.rt_state   =  DC_SERVICE_OFF;
      nifman_info.network    =  DC_CELLULAR_SOCKETS_LWIP;
      (void)dc_com_write(&dc_com_db, DC_CELLULAR_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));

      cst_cellular_info.rt_state   =  DC_SERVICE_OFF;
      (void)dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));
      break;
    }
    default:
    {
      /* all other event => data transfer feature no more available */
      /* update nifman DC entry (service off)                        */
      (void)dc_com_read(&dc_com_db, DC_CELLULAR_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
      nifman_info.rt_state   =  DC_SERVICE_OFF;
      (void)dc_com_write(&dc_com_db, DC_CELLULAR_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
      cst_cellular_info.rt_state   =  DC_SERVICE_OFF;
      (void)dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));

      CST_config_fail(((uint8_t *)"CST_ppp_opened_event_mngt"),
                      CST_PPP_FAIL,
                      &cst_context.ppp_fail_count,
                      CST_PPP_FAIL_MAX);

      break;
    }
  }
}
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
/**
  * @brief  ppp config on going state management
  * @param  autom_event - automaton event
  * @retval -
  */
static void CST_ppp_config_on_going_state(cst_autom_event_t autom_event)
{
#if (USE_LOW_POWER == 1)
  (void)rtosalTimerStop(cst_lp_inactivity_timer_handle);
#endif /* (USE_LOW_POWER == 1) */
  switch (autom_event)
  {
    case CST_PPP_OPENED_EVENT:
      CST_ppp_opened_event_mngt();
      break;

    case CST_TARGET_STATE_CMD_EVENT:
      CST_data_mode_target_state_event_mngt();
      break;

    default:
      /* Nothing to do */
      __NOP();
      break;
  }
}


/**
  * @brief  ppp close on going state management
  * @param  autom_event - automaton event
  * @retval -
  */
static void CST_ppp_close_on_going_state(cst_autom_event_t autom_event)
{
#if (USE_LOW_POWER == 1)
  (void)rtosalTimerStop(cst_lp_inactivity_timer_handle);
#endif /* (USE_LOW_POWER == 1) */
  switch (autom_event)
  {
    case CST_PPP_CLOSED_EVENT:
      CST_ppp_closed_event_mngt();
      break;

    default:
      /* Nothing to do */
      __NOP();
      break;
  }
}
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */

/* =================================================================
   automaton state functions END
   ================================================================= */


/* ============================================================== */
/*   Automaton : descriton of nominal way                         */
/* -------------------------------------------------------------- */
/*    CST_BOOT_STATE                                              */
/*    -> CST_BOOT_EVENT                                           */
/*      - cellular service init                                   */
/*    CST_MODEM_INIT_STATE                                        */
/*    -> CST_MODEM_INIT_EVENT                                     */
/*      - modem power on                                          */
/*      - modem init                                              */
/*      - sim init                                                */
/*      - get modem info                                          */
/*    CST_MODEM_READY_STATE                                       */
/*    -> CST_MODEM_READY_EVENT                                    */
/*      - network register                                        */
/*      - network attach                                          */
/*    CST_WAITING_FOR_SIGNAL_QUALITY_OK_STATE                     */
/*    -> CST_SIGNAL_QUALITY_TO_CHECK_EVENT                        */
/*      - Signal Quality test                                     */
/*    CST_WAITING_FOR_NETWORK_STATUS_STATE                        */
/*    -> CST_NETWORK_STATUS_TO_CHECK_EVENT                        */
/*      - tests if registered to Network                          */
/*    CST_NETWORK_STATUS_OK_STATE                                 */
/*    -> CST_NETWORK_STATUS_OK_EVENT                              */
/*      - tests if attached to Network                            */
/*    CST_MODEM_REGISTERED_STATE                                  */
/*    -> CST_MODEM_ATTACHED_EVENT                                 */
/*      - set default pdn                                         */
/*      - register pdn                                            */
/*      - activate pdn                                            */
/*    CST_MODEM_PDN_ACTIVATING_STATE                              */
/*    -> CST_PDP_ACTIVATED_EVENT                                  */
/*    CST_MODEM_DATA_READY_STATE                                  */
/* ============================================================== */

/* ============================================================== */
/* Automaton : description of modem 'SIM ONLY' target state way   */
/* -------------------------------------------------------------- */
/*    CST_BOOT_STATE                                              */
/*    -> CST_BOOT_EVENT                                           */
/*    CST_MODEM_INIT_STATE                                        */
/*    (target state requested: SIM_ONLY                           */
/*    CST_MODEM_SIM_ONLY_STATE                                    */
/* ============================================================== */

/* ============================================================== */
/*   Automaton : description of modem 'MODEM OFF' target state     */
/* -------------------------------------------------------------- */
/*    CST_BOOT_STATE                                              */
/*    -> CST_BOOT_EVENT                                           */
/*    CST_MODEM_INIT_STATE                                        */
/*    (target state requested: MODEM OFF)                         */
/*    CST_MODEM_OFF_STATE                                         */
/* ============================================================== */

/**
  * @brief  Cellular Service Task : automaton management
  * @param  argument - Task argument (not used)
  * @retval -
  */
static void CST_cellular_service_task(void *argument)
{
  UNUSED(argument);

  uint32_t                   event;
  cst_autom_event_t          autom_event;
#if (USE_LOW_POWER == 1)
  dc_cellular_power_status_t dc_power_status;
#endif /* (USE_LOW_POWER == 1) */

  /* Automation infinite loop */
  for (;;)
  {
    event = 0xffffffffU;
    /* waiting for an automaton event */
    (void)rtosalMessageQueueGet((osMessageQId)cst_queue_id, &event, RTOSAL_WAIT_FOREVER);
    autom_event = CST_get_autom_event(event);

    if (autom_event != CST_NO_EVENT)
    {
      /* Generic events, to manage whatever the current state and results in a state change */
      switch (autom_event)
      {
        /* New APN to be taken in account */
        case CST_APN_CONFIG_EVENT:
          CST_set_state(CST_APN_CONFIG_STATE);
          break;

        /* Ask for Modem int */
        case CST_MODEM_INIT_EVENT:
          if ((cst_context.current_state != CST_MODEM_INIT_STATE) &&
              (cst_context.current_state != CST_MODEM_OFF_STATE))
          {
            /* If current state is not a state to manage modem init, change current state */
            CST_set_state(CST_MODEM_INIT_STATE);
          }
          break;

        /* Ask for a sim reset (insert SIM - eject SIM - SIM refersh - ...) */
        case CST_SIM_RESET_EVENT:
          CST_set_state(CST_MODEM_INIT_STATE);
          break;

#if (USE_LOW_POWER == 1)
        /* Enter in low power mode on network network inactivity timeout */
        case CST_LP_INACTIVITY_TIMER_EVENT:
          (void)CSP_CSIdle();
          break;

        case CST_POWER_STATUS_CALLBACK_EVENT:
          /* Read power status from data cache */
          (void)dc_com_read(&dc_com_db, DC_CELLULAR_POWER_STATUS, (void *)&dc_power_status,
                            sizeof(dc_cellular_power_status_t));
          /* update data cache values with the one coming from network */
          dc_power_status.nwk_periodic_TAU = cst_lp_status.nwk_periodic_TAU;
          dc_power_status.nwk_active_time = cst_lp_status.nwk_active_time;
          /* Write back new values to data cache */
          (void)dc_com_write(&dc_com_db, DC_CELLULAR_POWER_STATUS, (void *)&dc_power_status,
                             sizeof(dc_cellular_power_status_t));
          break;
#endif /* (USE_LOW_POWER == 1) */

        default:
          /* Nothing to do */
          break;
      }

      /* Display the actual state and the received event to manage */
      if ((cst_context.current_state < CST_MAX_STATE) && (autom_event < CST_MAX_EVENT))
      {
        PRINT_CELLULAR_SERVICE("CST: AUTOM TASK: %s - %s\n\r", CST_StateName[cst_context.current_state],
                               cst_event_name[autom_event])
      }

      /* Call the right function according to the automaton current state */
      switch (cst_context.current_state)
      {
        /* ===================================================== */
        /* =======            Nominal States            ======== */
        /* ===================================================== */
        case CST_BOOT_STATE:
          CST_boot_state(autom_event);
          break;

        case CST_MODEM_INIT_STATE:
          CST_init_state(autom_event);
          break;

        case CST_MODEM_READY_STATE:
          CST_modem_ready_state(autom_event);
          break;

        case CST_WAITING_FOR_SIGNAL_QUALITY_OK_STATE:
          CST_waiting_for_signal_quality_ok_state(autom_event);
          break;

        case CST_WAITING_FOR_NETWORK_STATUS_STATE:
          CST_waiting_for_network_status_state(autom_event);
          break;

        case CST_NETWORK_STATUS_OK_STATE:
          CST_network_status_ok_state(autom_event);
          break;

        case CST_MODEM_REGISTERED_STATE:
          CST_modem_registered_state(autom_event);
          break;

        case CST_MODEM_PDN_ACTIVATING_STATE:
          CST_modem_pdn_activating_state(autom_event);
          break;

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
        case CST_PPP_CONFIG_ON_GOING_STATE:
          CST_ppp_config_on_going_state(autom_event);
          break;

        case CST_PPP_CLOSE_ON_GOING_STATE:
          CST_ppp_close_on_going_state(autom_event);
          break;
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */

        case CST_MODEM_DATA_READY_STATE:
          CST_data_ready_state(autom_event);
          break;


        /* ========================================================== */
        /* =======            Specific states                ======== */
        /* ========================================================== */
        case CST_MODEM_OFF_STATE:
          CST_modem_off_state(autom_event);
          break;

        case CST_MODEM_SIM_ONLY_STATE:
          CST_modem_sim_only_state(autom_event);
          break;

        case CST_MODEM_POWER_ON_ONLY_STATE:
          /* Nothing to do */
          break;

        case CST_MODEM_REPROG_STATE:
          CST_modem_reprog_state(autom_event);
          break;

        case CST_APN_CONFIG_STATE:
          CST_apn_config_state(autom_event);
          break;

        /* ========================================================== */
        /* =======            Error states                   ======== */
        /* ========================================================== */
        case CST_MODEM_RESET_STATE:
          CST_modem_reset_state(autom_event);
          break;

        case CST_MODEM_REBOOT_STATE:
          CST_modem_reboot_state(autom_event);
          break;

        case CST_MODEM_FAIL_STATE:
          CST_fail_state(autom_event);
          break;

          /* ========================================================== */
          /* =======            Low power states               ======== */
          /* ========================================================== */
#if (USE_LOW_POWER == 1)
        case CST_MODEM_POWER_DATA_IDLE_STATE:
          CST_power_idle_state(autom_event);
          break;

#endif /* (USE_LOW_POWER == 1) */

        default:
          /* Nothing to do */
          break;

      }
    }
    else
    {
      PRINT_CELLULAR_SERVICE("CST: autom_event = no event \n\r")
    }
  }
}


/* Public Functions Definition ------------------------------------------------------*/
/**
  * @brief  sets new automaton state
  * @param  new_state - new current automaton state to set
  * @retval -
  */
void CST_set_state(CST_autom_state_t new_state)
{
  if (cst_context.current_state == CST_MODEM_PDN_ACTIVATING_STATE)
  {
    /* Exit from CST_MODEM_PDN_ACTIVATING_STATE state : stop possibly armed and no more used timers */
    /* Stop timer pdn_activate_retry */
    (void)rtosalTimerStop(cst_pdn_activate_retry_timer_handle);
  }
  else if (cst_context.current_state == CST_WAITING_FOR_NETWORK_STATUS_STATE)
  {
    /* Exit from CST_WAITING_FOR_NETWORK_STATUS_STATE state : stop possibly armed and no more used timers */
    /* stop retry NFMC tempo */
    (void)rtosalTimerStop(cst_register_retry_timer_handle);
  }
  else
  {
    __NOP(); /* Nothing to do */
  }

  /* set new state */
  cst_context.current_state = new_state;
  PRINT_CELLULAR_SERVICE("CST: New State: %s\n\r", CST_StateName[new_state])

#if (USE_CELLULAR_SERVICE_TASK_TEST == 1)
  /* instrumentation code to test automaton */
  CSTE_cellular_service_task_test(cst_context.current_state);
#endif  /* (USE_CELLULAR_SERVICE_TASK_TEST == 1) */
}


/**
  * @brief  allows to get cellular service automaton current state
  * @param  -
  * @retval CST_autom_state_t - automaton state
  */
CST_autom_state_t CST_get_state(void)
{
  return cst_context.current_state;
}


/**
  * @brief  allows to set radio on: start cellular automaton
  * @param  -
  * @retval CS_Status_t - return code
  */
CS_Status_t  CST_radio_on(void)
{
  /* Sends a message to start automaton */
  CST_send_message(CST_MESSAGE_CMD, CST_BOOT_EVENT);
  return CELLULAR_OK;
}

/**
  * @brief  allows to boot modem only without network register
  * @note   the application is not started
  * @param  -
  * @retval CS_Status_t - return code
  */
CS_Status_t  CST_modem_power_on(void)
{
  CST_send_message(CST_MESSAGE_CMD, CST_MODEM_POWER_ON_ONLY_EVENT);
  return CELLULAR_OK;
}

/**
  * @brief  allows to get the modem IP addr
  * @note   to be able to return an IP address the modem must be in data transfer state
  * @note   else an error is returned
  * @param  ip_addr_type - type of IP address
  * @param  p_ip_addr_value - IP address value returned by the function
  * @retval CS_Status_t - return code
  */
CS_Status_t CST_get_dev_IP_address(CS_IPaddrType_t *ip_addr_type, CS_CHAR_t *p_ip_addr_value)
{
  PRINT_CELLULAR_SERVICE("CST: osCDS_get_dev_IP_address()\n\r")
  return osCDS_get_dev_IP_address(cst_get_cid_value(cst_cellular_params.sim_slot[cst_context.sim_slot_index].cid),
                                  ip_addr_type,
                                  p_ip_addr_value);
}

/**
  * @brief  initializes cellular service component
  * @param  -
  * @retval CS_Status_t - return code
  */
CS_Status_t CST_cellular_service_init(void)
{
  dc_cellular_target_state_t target_state;
  CS_Status_t                ret;

  CST_set_state(CST_BOOT_STATE);

  /* Init cellular service datacache entries */
  cellular_service_datacache_init();

  /* request modem init to Cellular Service */
  ret = CS_init();
  if (ret != CELLULAR_OK)
  {
    ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 6, ERROR_WARNING);
  }

  if (ret == CELLULAR_OK)
  {
#if (USE_CELLULAR_SERVICE_TASK_TEST == 1)
    /* instrumentation code to test automaton */
    CSTE_cellular_test_init();
#endif  /* (USE_CELLULAR_SERVICE_TASK_TEST == 1) */

    PRINT_CELLULAR_SERVICE("CST: osCDS_cellular_service_init()\n\r")
    (void)osCDS_cellular_service_init();
    cst_context.csq_count_fail = 0U;

    /* polling activated by default */
    CST_polling_active = true;

    /* No pooling is currently on going */
    CST_polling_on_going = false;

    /* Initialize modem target state with the one defined in plateform config */
    if (dc_com_read(&dc_com_db, DC_CELLULAR_TARGET_STATE_CMD, (void *)&target_state, sizeof(target_state)) == DC_COM_OK)
    {
      target_state.rt_state     = DC_SERVICE_ON;
      target_state.target_state = (dc_cs_target_state_t)PLF_CELLULAR_TARGET_STATE;
      target_state.callback = false;
      (void)dc_com_write(&dc_com_db, DC_CELLULAR_TARGET_STATE_CMD, (void *)&target_state, sizeof(target_state));
    }

    /* initialize modem state */
    CST_set_modem_state(&dc_com_db, CA_MODEM_POWER_OFF, (uint8_t *)"CA_MODEM_POWER_OFF");

    (void)CST_config_init();
#if (USE_LOW_POWER == 1)
    CSP_Init();
#endif /* (USE_LOW_POWER == 1) */

    cst_queue_id = rtosalMessageQueueNew((const rtosal_char_t *)"CST_QUE_MAIN", CST_QUEUE_SIZE);
    if (cst_queue_id == NULL)
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 5, ERROR_FATAL);
      ret = CELLULAR_ERROR;
    }

    /* creates timers */
    /* initializes modem polling timer */
    cst_polling_timer_handle = rtosalTimerNew((const rtosal_char_t *)"CST_TIM_POOLING",
                                              (os_ptimer)CST_polling_timer_callback, osTimerPeriodic, NULL);

    /* initializes pdn activation timer */
    cst_pdn_activate_retry_timer_handle = rtosalTimerNew((const rtosal_char_t *)"CST_TIM_PDN_ACTIVATE_RETRY",
                                                         (os_ptimer)CST_pdn_activate_retry_timer_callback,
                                                         osTimerOnce, NULL);

    /* initializes network monitoring state timer */
    cst_network_status_timer_handle = rtosalTimerNew((const rtosal_char_t *)"CST_TIM_NET_STATUS",
                                                     (os_ptimer)CST_network_status_timer_callback, osTimerOnce,
                                                     NULL);

    /* initializes register timer */
    cst_register_retry_timer_handle = rtosalTimerNew((const rtosal_char_t *)"CST_TIM_REGISTER_RETRY",
                                                     (os_ptimer)CST_register_retry_timer_callback, osTimerOnce,
                                                     NULL);

    /* initializes FOTA timer */
    cst_fota_timer_handle = rtosalTimerNew((const rtosal_char_t *)"CST_TIM_FOTA",
                                           (os_ptimer)CST_fota_timer_callback, osTimerOnce, NULL);

#if (USE_LOW_POWER == 1)
    /* initializes low power inactivity timer */
    cst_lp_inactivity_timer_handle = rtosalTimerNew((const rtosal_char_t *)"CST_TIM_LP_INACTIVITY",
                                                    (os_ptimer)CST_lp_inactivity_timer_callback, osTimerOnce,
                                                    NULL);
#endif /* (USE_LOW_POWER == 1) */

  }
  return ret;
}

/**
  * @brief  starts cellular service component
  * @note   cellular service task automaton and tempos are started
  * @param  -
  * @retval CS_Status_t - return code
  */
CS_Status_t CST_cellular_service_start(void)
{
  static osThreadId cst_cellular_service_thread_id = NULL;
  dc_nfmc_info_t  nfmc_info;
  uint32_t        cst_polling_period;
  dc_com_status_t dc_ret;
  uint32_t        cs_ret;
  CS_Status_t     cst_ret;
  rtosalStatus    os_ret;

  dc_ret  = DC_COM_OK;
  cst_ret = CELLULAR_OK;
  cs_ret  = 0U;

#if (USE_CMD_CONSOLE == 1)
  (void)CST_cmd_cellular_service_start();
#endif /*  (USE_CMD_CONSOLE == 1) */

  /* reads cellular configuration in Data Cache */
  if (dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cst_cellular_params, sizeof(cst_cellular_params)) ==
      DC_COM_ERROR)
  {
    dc_ret = DC_COM_ERROR;
  }

  /* read Data Cache SIM slot entry  */
  if (dc_com_read(&dc_com_db, DC_CELLULAR_SIM_INFO, (void *)&cst_sim_info, sizeof(cst_sim_info)) == DC_COM_ERROR)
  {
    dc_ret = DC_COM_ERROR;
  }

  /* read Data Cache NFMC entry  */
  if (dc_com_read(&dc_com_db, DC_CELLULAR_NFMC_INFO, (void *)&nfmc_info, sizeof(nfmc_info)) == DC_COM_ERROR)
  {
    dc_ret = DC_COM_ERROR;
  }

  /* read Data Cache cellular info entry  */
  if (dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info)) == DC_COM_ERROR)
  {
    dc_ret = DC_COM_ERROR;
  }

  cst_sim_info.sim_status[CA_SIM_REMOVABLE_SLOT] = CA_SIM_NOT_USED;
  cst_sim_info.sim_status[CA_SIM_EXTERNAL_MODEM_SLOT]  = CA_SIM_NOT_USED;
  cst_sim_info.sim_status[CA_SIM_INTERNAL_MODEM_SLOT] = CA_SIM_NOT_USED;
  cst_context.sim_slot_index = 0U;
  cst_sim_info.active_slot = cst_cellular_params.sim_slot[cst_context.sim_slot_index].sim_slot_type;
  cst_sim_info.index_slot  = cst_context.sim_slot_index;
  if (dc_com_write(&dc_com_db, DC_CELLULAR_SIM_INFO, (void *)&cst_sim_info, sizeof(cst_sim_info)) == DC_COM_ERROR)
  {
    dc_ret = DC_COM_ERROR;
  }

#if (USE_LOW_POWER == 1)
  CSP_Start();
#endif  /* (USE_LOW_POWER == 1) */

  /* register component to Data Cache  */
  if (dc_com_core_register_gen_event_cb(&dc_com_db, CST_notif_callback, (const void *)NULL) == DC_COM_INVALID_ENTRY)
  {
    dc_ret = DC_COM_ERROR;
  }

  cst_cellular_info.mno_name[0]           = 0U;
  cst_cellular_info.rt_state              = DC_SERVICE_UNAVAIL;

  /* write Data Cache cellular info entry  */
  if (dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info)) == DC_COM_ERROR)
  {
    dc_ret = DC_COM_ERROR;
  }

  /* initializes Data Cache NFMC entry  */
  nfmc_info.rt_state = DC_SERVICE_UNAVAIL;
  if (dc_com_write(&dc_com_db, DC_CELLULAR_NFMC_INFO, (void *)&nfmc_info, sizeof(nfmc_info)) == DC_COM_ERROR)
  {
    dc_ret = DC_COM_ERROR;
  }

  /* creates and starts cellar service task automaton */
  cst_cellular_service_thread_id = rtosalThreadNew((const rtosal_char_t *)"CellularService",
                                                   (os_pthread)CST_cellular_service_task, CELLULAR_SERVICE_THREAD_PRIO,
                                                   CELLULAR_SERVICE_THREAD_STACK_SIZE, NULL);

  if (cst_cellular_service_thread_id == NULL)
  {
    /* thread creation fails */
    cs_ret |= (uint32_t)CELLULAR_ERROR;
    ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 7, ERROR_FATAL);
  }

  /* start modem polling timer */
#if (CST_MODEM_POLLING_PERIOD == 0)
  cst_polling_period = CST_MODEM_POLLING_PERIOD_DEFAULT;
#else
  cst_polling_period = CST_MODEM_POLLING_PERIOD;
#endif /* (CST_MODEM_POLLING_PERIOD == 1) */
  os_ret = rtosalTimerStart(cst_polling_timer_handle, cst_polling_period);
  if (os_ret != osOK)
  {
    /* polling timer start fails */
    cs_ret |= (uint32_t)CELLULAR_ERROR;
  }

#if (USE_LOW_POWER == 1)
  /* initializes low power inactivity timer */
  cst_lp_inactivity_timer_handle = rtosalTimerNew((const rtosal_char_t *)"CST_TIM_LP_INACTIVITY",
                                                  (os_ptimer)CST_lp_inactivity_timer_callback, osTimerOnce,
                                                  NULL);
#endif /* (USE_LOW_POWER == 1) */

  if ((dc_ret != DC_COM_OK) || (cs_ret != 0U))
  {
    /* At least one error occurs during start function */
    cst_ret = CELLULAR_ERROR;
  }

  return cst_ret;
}
