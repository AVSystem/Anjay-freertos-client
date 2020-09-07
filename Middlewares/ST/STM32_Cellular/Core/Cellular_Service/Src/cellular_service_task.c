/**
  ******************************************************************************
  * @file    cellular_service_task.c
  * @author  MCD Application Team
  * @brief   This file defines functions for Cellular Service Task
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

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"
#include "cellular_service_task.h"
#include "cmsis_os_misrac2012.h"

#include "dc_common.h"
#include "error_handler.h"

#include "at_util.h"
#include "cellular_datacache.h"
#include "cellular_service.h"
#include "cellular_service_os.h"
#include "cellular_service_config.h"
#include "cellular_service_int.h"
#include "cellular_runtime_custom.h"

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

/* cellular register Test activation */

#define CST_MODEM_POLLING_PERIOD_DEFAULT 5000U

/* bad RSSI value (Signal Quality) */
#define CST_BAD_SIG_RSSI 99U

#define CST_COUNT_FAIL_MAX (5U)  /* Max of total restarts allowed after failure*/

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

/* delay for PND activation retry */
#define CST_PDN_ACTIVATE_RETRY_DELAY 30000U

/* FOTA Timeout since start programming */
#define CST_FOTA_TIMEOUT      (360000U) /* 6 min (calibrated for cat-M1 network, increase it for cat-NB1) */

/* SIM slot polling period */
#define CST_SIM_POLL_COUNT     200U    /* 20s */

/* Network registered status */
typedef uint16_t cst_network_status_t;
#define CST_NET_REGISTERED     (cst_network_status_t)0U   /* registered to network     */
#define CST_NOT_REGISTERED     (cst_network_status_t)1U   /* not registered to network */
#define CST_NET_STATUS_ERRROR  (cst_network_status_t)2U   /* network status error      */



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

/* Private typedef -----------------------------------------------------------*/

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
  CST_MODEM_SIM_FAIL
} cst_fail_cause_t;

/* type of messge to send to automaton task */
typedef struct
{
  CST_message_type_t  type;  /* message type */
  uint16_t            id;   /* message event or command ID */
} cst_message_t;


/* Cellular context */
typedef struct
{
  CST_autom_state_t    current_state;                    /* automaton current state                          */
  cst_fail_cause_t     fail_cause;                       /* cause of modem connection failure                */
  CS_PDN_event_t       pdn_status;                       /* PDN status when modem callback is called         */
  CS_SignalQuality_t   signal_quality;                   /* current signal quality                           */
  CS_NetworkRegState_t current_EPS_NetworkRegState;      /* current EPS network state                        */
  CS_NetworkRegState_t current_GPRS_NetworkRegState;     /* current GPRS network state                       */
  CS_NetworkRegState_t current_CS_NetworkRegState;       /* current CS network state                         */
  uint16_t             activate_pdn_nfmc_tempo_count;    /* current NFMC tempo count (if NFMC enabled)       */
  uint16_t             register_retry_tempo_count;       /* number of network register retry                 */
  uint8_t              sim_slot_index;                   /* index of sim slot used                           */

  /* failing counters BEGIN */
  uint8_t             power_on_reset_count;
  uint8_t             reset_count;
  uint8_t             csq_reset_count;
  uint8_t             attach_reset_count;
  uint8_t             activate_pdn_reset_count;
  uint8_t             register_reset_count;
  uint8_t             network_reset_count;
  uint8_t             sim_reset_count;
  uint8_t             cellular_data_retry_count;
  uint8_t             global_retry_count;
  uint8_t             csq_count_fail;
  /* failing counters END */
} cst_context_t;

/* NFMC context */
/* Note: NFMC Network-friendly Management Configuration       */
/*       Normalized network attachment temporisations         */
typedef struct
{
  bool      active;                      /* NFMC enable */
  bool      nfmc_timer_on_going;         /* NFMC timer on going  */
  uint32_t  tempo[CST_NFMC_TEMPO_NB];    /* NFMC tempo list          */
} cst_nfmc_context_t;

/* Private variables ---------------------------------------------------------*/

/* Event queue */
static osMessageQId      cst_queue_id;

/* Timer handlers */
static osTimerId         cst_pdn_activate_retry_timer_handle;  /* pdn activation timer                */
static osTimerId         cst_network_status_timer_handle;      /* waiting for network status OK timer */
static osTimerId         cst_register_retry_timer_handle;      /* registering to network timer        */
static osTimerId         cst_fota_timer_handle;                /* FOTA timer                          */

/* DC structures */
static dc_cellular_info_t         cst_cellular_info;           /* cellular infos               */
static dc_sim_info_t              cst_sim_info;                /* sim infos                    */
static dc_cellular_data_info_t    cst_cellular_data_info;      /* cellular data infos          */
static dc_cellular_params_t       cst_cellular_params;         /* cellular configuration param */
static dc_apn_config_t            cst_apn_config;              /* new apn config request       */

/* NFMC context */
static cst_nfmc_context_t cst_nfmc_context;                    /* NFMC context                 */

/* CST context */
static cst_context_t cst_context =
{
  CST_BOOT_STATE, CST_NO_FAIL, CS_PDN_EVENT_NW_DETACH,    /* Automaton State, FAIL Cause,  */
  { 0U, 0U},                               /* signal quality */
  CS_NRS_NOT_REGISTERED_NOT_SEARCHING, CS_NRS_NOT_REGISTERED_NOT_SEARCHING, CS_NRS_NOT_REGISTERED_NOT_SEARCHING,
  0U,                                     /* activate_pdn_nfmc_tempo_count */
  0U,                                     /* register_retry_tempo_count */
  0U,                                     /* sim slot index */
  0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U     /* fail counters */
};

/* operator context */
static CS_OperatorSelector_t    ctxt_operator =
{
  .mode = CS_NRM_AUTO,
  .format = CS_ONF_NOT_PRESENT,
  .operator_name = "00101",
  .AcT_present = CELLULAR_FALSE,
  .AcT = CS_ACT_E_UTRAN,
};


/* Private function prototypes -----------------------------------------------*/
static cst_network_status_t CST_get_network_status(void);
static void CST_pdn_event_callback(CS_PDN_conf_id_t cid, CS_PDN_event_t pdn_event);
static void CST_network_reg_callback(void);
static void CST_modem_event_callback(CS_ModemEvent_t event);
static void CST_location_info_callback(void);
static void CST_polling_timer_callback(void const *argument);
static void CST_location_info_callback(void);
static void CST_pdn_activate_retry_timer_callback(void const *argument);
static void CST_network_status_timer_callback(void const *argument);
static void CST_register_retry_timer_callback(void const *argument);
static void CST_fota_timer_callback(void const *argument);
static void CST_notif_callback(dc_com_event_id_t dc_event_id, const void *private_data);

static void CST_data_cache_cellular_info_set(dc_service_rt_state_t dc_service_state);
static void CST_config_fail(const uint8_t *msg_fail, cst_fail_cause_t fail_cause, uint8_t *fail_count,
                            uint8_t fail_max);
static void CST_modem_define_pdn(void);
static CS_Status_t CST_set_signal_quality(void);
static void CST_get_device_all_infos(dc_cs_target_state_t  target_state);
static void CST_subscribe_all_net_events(void);
static void CST_subscribe_modem_events(void);
static CST_autom_event_t CST_get_autom_event(osEvent event);
static void CST_fill_nfmc_tempo(uint32_t imsi_high, uint32_t imsi_low);
static CS_SimSlot_t cst_convert_sim_socket_type(dc_cs_sim_slot_type_t sim_slot_value);
static void CST_close_network_interface(void);
static void CST_fail_setting(void);

static void CST_boot_power_on_only_modem_mngt(void);
static void CST_init_power_on_only_modem_mngt(void);
static void CST_init_state_mngt(void);
static void CST_modem_reset_mngt(void);
static void CST_modem_sim_init(void);
static void CST_net_register_mngt(void);
static void CST_nw_reg_timeout_expiration_mngt(void);
static void CST_signal_quality_test_mngt(void);
static void CST_network_status_test_mngt(void);
static void CST_network_event_mngt(void);
static void CST_pdn_event_nw_detach_mngt(void);
static void CST_attach_modem_mngt(void);
static void CST_modem_activated_mngt(void);
static void CST_modem_activate_pdn_mngt(void);
static void CST_cellular_data_fail_mngt(void);
static void CST_pdn_event_mngt(void);
static void CST_polling_timer_mngt(void);
static void CST_apn_set_new_config_mngt(void);
static void CST_nifman_event_mngt(void);
static void CST_data_read_target_state_event_mngt(void);
static void CST_sim_only_target_state_event_mngt(void);
static void CST_modem_off_mngt(void);

static void CST_init_state(CST_autom_event_t autom_event);
static void CST_modem_reset_state(CST_autom_event_t autom_event);
static void CST_modem_ready_state(CST_autom_event_t autom_event);
static void CST_waiting_for_signal_quality_ok_state(CST_autom_event_t autom_event);
static void CST_waiting_for_network_status_state(CST_autom_event_t autom_event);
static void CST_network_status_ok_state(CST_autom_event_t autom_event);
static void CST_modem_registered_state(CST_autom_event_t autom_event);
static void CST_modem_pdn_activating_state(CST_autom_event_t autom_event);
static void CST_data_ready_state(CST_autom_event_t autom_event);
static void CST_fail_state(CST_autom_event_t autom_event);

#if (USE_LOW_POWER == 1)
static void CST_power_idle_state(CST_autom_event_t autom_event);
#endif /* (USE_LOW_POWER == 1) */

static void CST_cellular_service_task(void const *argument);

/* Global variables ----------------------------------------------------------*/
bool CST_polling_active;     /* modem polling activation flag */

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
  ((uint8_t *)"MODEM_OFF_STATE"),
  ((uint8_t *)"MODEM_ON_ONLY_STATE"),
  ((uint8_t *)"MODEM_POWER_DATA_IDLE_STATE"),
  ((uint8_t *)"APN_CONFIG_STATE")
};
#endif /* (( USE_TRACE_CELLULAR_SERVICE == 1) || ( USE_CMD_CONSOLE == 1 )) */

/* Private function Definition -----------------------------------------------*/

/* ===================================================================
   Tools functions  BEGIN
   =================================================================== */

/**
  * @brief  convert sim slot DC enum value to CS enum value
  * @param  sim_slot_value - sim slot DC enum value
  * @retval CS_SimSlot_t  - sim slot CS enum value
  */

static CS_SimSlot_t  cst_convert_sim_socket_type(dc_cs_sim_slot_type_t sim_slot_value)
{
  CS_SimSlot_t enum_value;
  switch (sim_slot_value)
  {
    case DC_SIM_SLOT_MODEM_SOCKET:
      enum_value = CS_MODEM_SIM_SOCKET_0;
      break;
    case DC_SIM_SLOT_MODEM_EMBEDDED_SIM:
      enum_value = CS_MODEM_SIM_ESIM_1;
      break;
    case DC_SIM_SLOT_STM32_EMBEDDED_SIM:
      enum_value = CS_STM32_SIM_2;
      break;
    default:
      enum_value = CS_MODEM_SIM_SOCKET_0;
      break;
  }
  return enum_value;
}


/**
  * @brief  64bits modulo calculation
  * @param  div   - divisor
  * @param  val_m  - high 32bits value
  * @param  val_l  - low  32bits value
  * @retval uint32_t - result of modulo calculation
  */
static uint32_t cst_modulo64(uint32_t div, uint32_t val_m, uint32_t val_l)
{
  uint32_t div_m;
  uint32_t div_l;
  uint32_t tmp_m;
  uint32_t tmp_l;

  tmp_l = 0U;
  div_m = div;
  div_l = 0U;

  if (div_m != 0U)
  {
    tmp_m = val_m % div_m;

    tmp_l = val_l;

    while (tmp_m > 0U)
    {
      if ((div_m > tmp_m) || ((div_m == tmp_m) && (div_l > tmp_l)))
      {
        /* Nothing to do */
      }
      else if (div_l > tmp_l)
      {
        tmp_l = tmp_l - div_l;
        tmp_m--;
        tmp_m = tmp_m - div_m;
      }
      else
      {
        tmp_m = tmp_m - div_m;
        tmp_l = tmp_l - div_l;
      }

      div_l = div_l >> 1;
      if ((div_m & 1U) == 1U)
      {
        div_l = div_l | 0x80000000U;
      }
      div_m = div_m >> 1U;
    }
    tmp_l = tmp_l % div;
  }
  return tmp_l;
}

/* ===================================================================
   Tools functions  END
   =================================================================== */

/* ===================================================================
   UTility functions  BEGIN
   =================================================================== */

/**
  * @brief  init modem management
  * @param  -
  * @retval -
  */
static void  CST_modem_sim_init(void)
{
  CS_Status_t cs_status;
  cs_status = CELLULAR_OK;
  PRINT_CELLULAR_SERVICE("*********** CST_modem_sim_init ********\n\r")
  /* sim slot select */
  (void)osCS_sim_select(cst_convert_sim_socket_type(cst_sim_info.active_slot));

  (void)osDelay(10);  /* waiting for 10ms after sim selection */
  if (cst_cellular_params.set_pdn_mode != 0U)
  {
    /* we must first define Cellular context before activating the RF because
     * modem will immediately attach to network once RF is enabled
     */
    PRINT_CELLULAR_SERVICE("CST_modem_sim_init : CST_modem_define_pdn\n\r")
    CST_modem_define_pdn();
  }

  if (cst_cellular_params.target_state == DC_TARGET_STATE_SIM_ONLY)
  {
    /* Modem Target state : SIM ONLY */
    cs_status = osCDS_init_modem(CS_CMI_SIM_ONLY, CELLULAR_FALSE, CST_SIM_PINCODE);
  }
  else if (cst_cellular_params.target_state == DC_TARGET_STATE_FULL)
  {
    /* Modem Target state : FULL */
    cs_status = osCDS_init_modem(CS_CMI_FULL, CELLULAR_FALSE, CST_SIM_PINCODE);
  }
  else
  {
    __NOP(); /* Nothing to do */
  }

  if (cs_status == CELLULAR_SIM_INCORRECT_PASSWORD)
  {
    PRINT_FORCE("==================================\n\r")
    PRINT_FORCE(" WARNING: WRONG PIN CODE !!!\n\r")
    PRINT_FORCE(" DO NOT RESTART THE BOARD WITHOUT SETTING A CORRECT PIN CODE\n\r")
    PRINT_FORCE(" TO AVOID LOCKING THE SIM ! \n\r")
    PRINT_FORCE("==================================\n\r")
    for (;;)
    {
      /* Infinite loop to avoid to restart the board */
      __NOP(); /* Nothing to do */
    }
  }
  else if (cs_status == CELLULAR_SIM_PIN_OR_PUK_LOCKED)
  {
    PRINT_FORCE("==================================\n\r")
    PRINT_FORCE(" WARNING: PIN OK PUK LOCKED !!!  \n\r")
    PRINT_FORCE(" PROCESSING STOPPED\n\r")
    PRINT_FORCE("==================================\n\r")
    for (;;)
    {
      /* Infinite loop to avoid to restart the board */
      __NOP(); /* Nothing to do */
    }
  }
  else
  {
    __NOP(); /* Nothing to do */
  }

  if ((cs_status == CELLULAR_SIM_NOT_INSERTED) || (cs_status == CELLULAR_ERROR) || (cs_status == CELLULAR_SIM_ERROR))
  {
    /* SIM Error: FAIL */
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_SIM_INFO, (void *)&cst_sim_info, sizeof(cst_sim_info));
    cst_sim_info.sim_status[cst_context.sim_slot_index] = DC_SIM_ERROR;
    cst_sim_info.rt_state   = DC_SERVICE_ON;
    cst_context.sim_slot_index++;

    if (cst_context.sim_slot_index  >= cst_cellular_params.sim_slot_nb)
    {
      cst_context.sim_slot_index = 0U; /* No available SIM found: set default SIM socket (Modem socket) */
      PRINT_CELLULAR_SERVICE("CST_modem_powered_on_state : No SIM found\n\r")
    }

    cst_sim_info.active_slot = cst_cellular_params.sim_slot[cst_context.sim_slot_index].sim_slot_type;
    cst_sim_info.index_slot  = cst_context.sim_slot_index;
    (void)dc_com_write(&dc_com_db, DC_CELLULAR_SIM_INFO, (void *)&cst_sim_info, sizeof(cst_sim_info));

    CST_config_fail(((uint8_t *)"CST_modem_sim_init"),
                    CST_MODEM_SIM_FAIL,
                    &cst_context.sim_reset_count,
                    CST_SIM_RETRY_MAX);

  }
  else
  {
    /* SIM OK */
    /* Init Power config after Modem Power On and before subsribe modem evnets  */
#if (USE_LOW_POWER == 1)
    CSP_InitPowerConfig();
#endif  /* (USE_LOW_POWER == 1) */

    CST_subscribe_all_net_events();

    /* overwrite operator parameters */
    ctxt_operator.mode   = CS_NRM_AUTO;
    ctxt_operator.format = CS_ONF_NOT_PRESENT;
    CST_get_device_all_infos(cst_cellular_params.target_state);
    if (cst_cellular_params.target_state != DC_TARGET_STATE_SIM_ONLY)
    {
      CST_set_state(CST_MODEM_READY_STATE);
      CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_READY_EVENT);
    }
    else
    {
      /* Modem Target State == SIM ONLY : end of processing */
      /* DC Update */
      (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(dc_cellular_info_t));
      cst_cellular_info.rt_state    = DC_SERVICE_ON;
      cst_cellular_info.modem_state = DC_MODEM_STATE_SIM_CONNECTED;
      (void)dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(dc_cellular_info_t));

      CST_set_state(CST_MODEM_SIM_ONLY_STATE);
    }
  }
}

/**
  * @brief  update Cellular Info entry of Data Cache
  * @param  dc_service_state - new entry state to set
  * @retval -
  */
static void  CST_data_cache_cellular_info_set(dc_service_rt_state_t dc_service_state)
{
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_DATA_INFO, (void *)&cst_cellular_data_info,
                    sizeof(cst_cellular_data_info));
  if (cst_cellular_data_info.rt_state != dc_service_state)
  {
    cst_cellular_data_info.rt_state = dc_service_state;
    (void)dc_com_write(&dc_com_db, DC_CELLULAR_DATA_INFO, (void *)&cst_cellular_data_info,
                       sizeof(cst_cellular_data_info));
  }
}


/**
  * @brief  configuration failure management
  * @param  msg_fail   - failure message (only for trace)
  * @param  fail_cause - failure cause
  * @param  fail_count - count of failures
  * @param  fail_max   - max of allowed failures
  * @retval -
  */
static void CST_config_fail(const uint8_t *msg_fail, cst_fail_cause_t fail_cause, uint8_t *fail_count,
                            uint8_t fail_max)
{
#if (USE_TRACE_CELLULAR_SERVICE == 0)
  UNUSED(msg_fail);
#endif  /* (USE_TRACE_CELLULAR_SERVICE == 0) */

  PRINT_CELLULAR_SERVICE("=== %s Fail !!! === \r\n", msg_fail)
  ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 1, ERROR_WARNING);

  *fail_count = *fail_count + 1U;
  cst_context.global_retry_count++;
  cst_context.reset_count++;

  CST_data_cache_cellular_info_set(DC_SERVICE_OFF);
  if ((*fail_count <= fail_max) && (cst_context.global_retry_count <= CST_GLOBAL_RETRY_MAX))
  {
    /* maximal fail count not reached => restart automation */
    CST_set_state(CST_MODEM_RESET_STATE);
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_INIT_EVENT);
    cst_context.fail_cause    = fail_cause;
  }
  else
  {
    /* maximal fail count reached => stop cellular processing */
    CST_set_state(CST_MODEM_FAIL_STATE);
    CST_fail_setting();
    cst_context.fail_cause    = CST_MODEM_POWER_ON_FAIL;

    PRINT_CELLULAR_SERVICE_ERR("=== CST_set_fail_state %d - count %d/%d FATAL !!! ===\n\r",
                               fail_cause,
                               cst_context.global_retry_count,
                               *fail_count)
    ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 2, ERROR_FATAL);
  }
}


/**
  * @brief  sets current signal quality values in DC
  * @param  -
  * @retval CS_Status_t - error code
  */
static CS_Status_t CST_set_signal_quality(void)
{
  CS_Status_t cs_status;
  CS_SignalQuality_t sig_quality;
  cs_status = CELLULAR_OK;

  if (osCS_get_signal_quality(&sig_quality) == CELLULAR_OK)
  {
    /* signal quality service available */
    cst_context.csq_count_fail = 0U;
    if ((sig_quality.rssi != cst_context.signal_quality.rssi) || (sig_quality.ber != cst_context.signal_quality.ber))
    {
      /* signal quality value has changed => update DC values */
      cst_context.signal_quality.rssi = sig_quality.rssi;
      cst_context.signal_quality.ber  = sig_quality.ber;

      (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));

      if (sig_quality.rssi == CST_BAD_SIG_RSSI)
      {
        /* Wrong signal quality : not attached to network */
        cs_status = CELLULAR_ERROR;
        cst_cellular_info.cs_signal_level    = DC_NO_ATTACHED;
        cst_cellular_info.cs_signal_level_db = (int32_t)DC_NO_ATTACHED;
      }
      else
      {
        /* signal quality OK  */
        cs_status = CELLULAR_OK;
        cst_cellular_info.cs_signal_level     = sig_quality.rssi;                         /* range 0..99 */
        cst_cellular_info.cs_signal_level_db  = (-113 + (2 * (int32_t)sig_quality.rssi)); /* dBm value   */
      }
      (void)dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));
    }

    PRINT_CELLULAR_SERVICE(" -Sig quality rssi : %d\n\r", sig_quality.rssi)
    PRINT_CELLULAR_SERVICE(" -Sig quality ber  : %d\n\r", sig_quality.ber)
  }
  else
  {
    /* signal quality service not available */
    cs_status = CELLULAR_ERROR;
    cst_context.csq_count_fail++;
    PRINT_CELLULAR_SERVICE("Modem signal quality error\n\r")
    if (cst_context.csq_count_fail >= CST_COUNT_FAIL_MAX)
    {
      PRINT_CELLULAR_SERVICE("Modem signal quality error max\n\r")
      cst_context.csq_count_fail = 0U;
      CST_config_fail(((uint8_t *)"CS_get_signal_quality"),
                      CST_MODEM_CSQ_FAIL,
                      &cst_context.csq_reset_count,
                      CST_CSQ_MODEM_RESET_MAX);
    }
  }
  return cs_status;
}

/**
  * @brief  sends message to cellular service task
  * @param  type   - message type
  * @param  event  - event/command
  * @retval -
  */
void  CST_send_message(CST_message_type_t  type, CST_autom_event_t event)
{
  cst_message_t cmd_message;
  cmd_message.type = type;
  cmd_message.id   = event;

  uint32_t *cmd_message_p = (uint32_t *)(&cmd_message);
  (void)osMessagePut((osMessageQId)cst_queue_id, *cmd_message_p, 0U);
}

/**
  * @brief  calculates NFMC tempos and sets them in DataCache
  * @param  imsi_high  - high IMSI 32bits value
  * @param  imsi_low   - low IMSI  32bits value
  * @retval -
  */
static void CST_fill_nfmc_tempo(uint32_t imsi_high, uint32_t imsi_low)
{
  uint32_t i;
  dc_nfmc_info_t nfmc_info;

  if (cst_cellular_params.nfmc_active != 0U)
  {
    /* NFMC active : NFMC tempos calculation */
    cst_nfmc_context.active = true;
    nfmc_info.activate = 1U;
    for (i = 0U; i < CST_NFMC_TEMPO_NB; i++)
    {
      uint32_t temp_value32;

      /* calculation of NFMC tempo */
      if (cst_cellular_params.nfmc_value[i] != 0U)
      {
        temp_value32 = cst_modulo64(cst_cellular_params.nfmc_value[i], imsi_high, imsi_low);
      }
      else
      {
        temp_value32 = imsi_low;  /* parameter value == 0 => value set to imsi_low */
      }
      temp_value32 = temp_value32 + cst_cellular_params.nfmc_value[i];
      cst_nfmc_context.tempo[i] = (0xffffffffU & temp_value32);

      nfmc_info.tempo[i] = cst_nfmc_context.tempo[i];
      PRINT_CELLULAR_SERVICE("VALUE/TEMPO %ld/%ld\n\r",  cst_cellular_params.nfmc_value[i], cst_nfmc_context.tempo[i])
    }
    nfmc_info.rt_state = DC_SERVICE_ON;
  }
  else
  {
    /* NFMC not active */
    nfmc_info.activate = 0U;
    nfmc_info.rt_state = DC_SERVICE_OFF;
    cst_nfmc_context.active = false;
  }
  (void)dc_com_write(&dc_com_db, DC_CELLULAR_NFMC_INFO, (void *)&nfmc_info, sizeof(nfmc_info));
}

/**
  * @brief  sets modem infos in data cache
  * @param  target_state  - modem target state
  * @retval -
  */
static void CST_get_device_all_infos(dc_cs_target_state_t  target_state)
{
  static CS_DeviceInfo_t cst_device_info;
  CS_Status_t            cs_status;
  uint16_t               sim_poll_count;
  bool                   end_of_loop;
  uint32_t               cst_imsi_high;
  uint32_t               cst_imsi_low;

  sim_poll_count = 0U;

  (void)memset((void *)&cst_device_info, 0, sizeof(CS_DeviceInfo_t));

  /* read current device info in Data Cache */
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));


  /* gets IMEI */
  cst_device_info.field_requested = CS_DIF_IMEI_PRESENT;
  if (osCDS_get_device_info(&cst_device_info) == CELLULAR_OK)
  {
    (void)memcpy(cst_cellular_info.imei, cst_device_info.u.imei, DC_MAX_SIZE_IMEI - 1U);
    cst_cellular_info.imei[DC_MAX_SIZE_IMEI - 1U] = 0U;     /* to avoid a non null terminated string */
    PRINT_CELLULAR_SERVICE(" -IMEI: %s\n\r", cst_device_info.u.imei)
  }
  else
  {
    cst_cellular_info.imei[0] = 0U;
    PRINT_CELLULAR_SERVICE("IMEI error\n\r")
  }


  /* gets Manufacturer Name  of modem*/
  cst_device_info.field_requested = CS_DIF_MANUF_NAME_PRESENT;
  if (osCDS_get_device_info(&cst_device_info) == CELLULAR_OK)
  {
    (void)memcpy((CRC_CHAR_t *)cst_cellular_info.manufacturer_name,
                 (CRC_CHAR_t *)cst_device_info.u.manufacturer_name,
                 DC_MAX_SIZE_MANUFACT_NAME - 1U);
    /* to avoid a non null terminated string */
    cst_cellular_info.manufacturer_name[DC_MAX_SIZE_MANUFACT_NAME - 1U] = 0U;
    PRINT_CELLULAR_SERVICE(" -MANUFACTURER: %s\n\r", cst_device_info.u.manufacturer_name)
  }
  else
  {
    cst_cellular_info.manufacturer_name[0] = 0U;
    PRINT_CELLULAR_SERVICE("Manufacturer Name error\n\r")
  }

  /* gets Model modem  */
  cst_device_info.field_requested = CS_DIF_MODEL_PRESENT;
  if (osCDS_get_device_info(&cst_device_info) == CELLULAR_OK)
  {
    (void)memcpy((CRC_CHAR_t *)cst_cellular_info.model,
                 (CRC_CHAR_t *)cst_device_info.u.model,
                 DC_MAX_SIZE_MODEL - 1U);
    cst_cellular_info.model[DC_MAX_SIZE_MODEL - 1U] = 0U; /* to avoid a non null terminated string */
    PRINT_CELLULAR_SERVICE(" -MODEL: %s\n\r", cst_device_info.u.model)
  }
  else
  {
    cst_cellular_info.model[0] = 0U;
    PRINT_CELLULAR_SERVICE("Model error\n\r")
  }

  /* gets revision of modem  */
  cst_device_info.field_requested = CS_DIF_REV_PRESENT;
  if (osCDS_get_device_info(&cst_device_info) == CELLULAR_OK)
  {
    (void)memcpy((CRC_CHAR_t *)cst_cellular_info.revision,
                 (CRC_CHAR_t *)cst_device_info.u.revision,
                 DC_MAX_SIZE_REV - 1U);
    cst_cellular_info.revision[DC_MAX_SIZE_REV - 1U] = 0U; /* to avoid a non null terminated string */
    PRINT_CELLULAR_SERVICE(" -REVISION: %s\n\r", cst_device_info.u.revision)
  }
  else
  {
    cst_cellular_info.revision[0] = 0U;
    PRINT_CELLULAR_SERVICE("Revision error\n\r")
  }

  /* gets serial number of modem  */
  cst_device_info.field_requested = CS_DIF_SN_PRESENT;
  if (osCDS_get_device_info(&cst_device_info) == CELLULAR_OK)
  {
    (void)memcpy((CRC_CHAR_t *)cst_cellular_info.serial_number,
                 (CRC_CHAR_t *)cst_device_info.u.serial_number,
                 DC_MAX_SIZE_SN - 1U);
    cst_cellular_info.serial_number[DC_MAX_SIZE_SN - 1U] = 0U; /* to avoid a non null terminated string */
    PRINT_CELLULAR_SERVICE(" -SERIAL NBR: %s\n\r", cst_device_info.u.serial_number)
  }
  else
  {
    cst_cellular_info.serial_number[0] = 0U;
    PRINT_CELLULAR_SERVICE("Serial Number error\n\r")
  }

  /* gets CCCID  */
  cst_device_info.field_requested = CS_DIF_ICCID_PRESENT;
  if (osCDS_get_device_info(&cst_device_info) == CELLULAR_OK)
  {
    (void)memcpy((CRC_CHAR_t *)cst_cellular_info.iccid,
                 (CRC_CHAR_t *)cst_device_info.u.iccid,
                 DC_MAX_SIZE_ICCID - 1U);
    cst_cellular_info.iccid[DC_MAX_SIZE_ICCID - 1U] = 0U; /* to avoid a non null terminated string */
    PRINT_CELLULAR_SERVICE(" -ICCID: %s\n\r", cst_device_info.u.iccid)
  }
  else
  {
    cst_cellular_info.serial_number[0] = 0U;
    PRINT_CELLULAR_SERVICE("Serial Number error\n\r")
  }

  /* writes updated cellular info in Data Cache */
  (void)dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));

  end_of_loop = true;
  if (target_state == DC_TARGET_STATE_FULL)
  {
    /* modem target state: FULL */

    /* SIM info set to 'on going' in Data Cache during SIM connection */
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_SIM_INFO, (void *)&cst_sim_info, sizeof(cst_sim_info));
    cst_sim_info.rt_state   = DC_SERVICE_ON;
    cst_sim_info.sim_status[cst_context.sim_slot_index] = DC_SIM_CONNECTION_ON_GOING;
    (void)dc_com_write(&dc_com_db, DC_CELLULAR_SIM_INFO, (void *)&cst_sim_info, sizeof(cst_sim_info));

    /* loop: waiting for SIM status */
    while (end_of_loop != false)
    {
      /* try to get IMSI to determine if SIM is present on this slot */
      cst_device_info.field_requested = CS_DIF_IMSI_PRESENT;
      cs_status = osCDS_get_device_info(&cst_device_info);
      if (cs_status == CELLULAR_OK)
      {
        /* IMSI available => SIM is present on this slot */

        /* NFMC tempos calculation (using IMSI)  (15 is size of imsi value) */
        (void)ATutil_convertHexaStringToInt64(cst_device_info.u.imsi, 15U, &cst_imsi_high, &cst_imsi_low);
        PRINT_CELLULAR_SERVICE(" -IMSI: %lx%lx\n\r", cst_imsi_high, cst_imsi_low)
        CST_fill_nfmc_tempo(cst_imsi_high, cst_imsi_low);

        (void)memcpy((CRC_CHAR_t *)cst_sim_info.imsi,
                     (CRC_CHAR_t *)cst_device_info.u.imsi,
                     DC_MAX_SIZE_IMSI - 1U);
        cst_sim_info.imsi[DC_MAX_SIZE_IMSI - 1U] = 0;  /* to avoid a non null terminated string */
        cst_sim_info.sim_status[cst_context.sim_slot_index] = DC_SIM_OK;
        end_of_loop = false;
      }
      else if ((cs_status == CELLULAR_SIM_BUSY)
               || (cs_status == CELLULAR_SIM_ERROR))
      {
        /* SIM presently not available: poll it untill available or polling time exceed */
        (void)osDelay(100U);
        sim_poll_count++;
        if (sim_poll_count > CST_SIM_POLL_COUNT)
        {
          /* polling time exceed: SIM not available on this slot */
          if (cs_status == CELLULAR_SIM_BUSY)
          {
            cst_sim_info.sim_status[cst_context.sim_slot_index] = DC_SIM_BUSY;
          }
          else
          {
            cst_sim_info.sim_status[cst_context.sim_slot_index] = DC_SIM_ERROR;
          }
          end_of_loop = false;
        }
      }
      else
      {
        /* error returned => SIM not available. Getting SIM error cause */
        if (cs_status == CELLULAR_SIM_NOT_INSERTED)
        {
          cst_sim_info.sim_status[cst_context.sim_slot_index] = DC_SIM_NOT_INSERTED;
        }
        else if (cs_status == CELLULAR_SIM_PIN_OR_PUK_LOCKED)
        {
          cst_sim_info.sim_status[cst_context.sim_slot_index] = DC_SIM_PIN_OR_PUK_LOCKED;
        }
        else if (cs_status == CELLULAR_SIM_INCORRECT_PASSWORD)
        {
          cst_sim_info.sim_status[cst_context.sim_slot_index] = DC_SIM_INCORRECT_PASSWORD;
        }
        else
        {
          cst_sim_info.sim_status[cst_context.sim_slot_index] = DC_SIM_ERROR;
        }
        end_of_loop = false;
      }
    }
    /* Set SIM state in Data Cache */
    (void)dc_com_write(&dc_com_db, DC_CELLULAR_SIM_INFO, (void *)&cst_sim_info, sizeof(cst_sim_info));
  }
}

/**
  * @brief  subscribes to network events
  * @param  -
  * @retval -
  */
static void CST_subscribe_all_net_events(void)
{
  PRINT_CELLULAR_SERVICE("Subscribe URC events: Network registration\n\r")
  (void)osCDS_subscribe_net_event(CS_URCEVENT_CS_NETWORK_REG_STAT, CST_network_reg_callback);
  (void)osCDS_subscribe_net_event(CS_URCEVENT_GPRS_NETWORK_REG_STAT, CST_network_reg_callback);
  (void)osCDS_subscribe_net_event(CS_URCEVENT_EPS_NETWORK_REG_STAT, CST_network_reg_callback);
  PRINT_CELLULAR_SERVICE("Subscribe URC events: Location info\n\r")
  (void)osCDS_subscribe_net_event(CS_URCEVENT_EPS_LOCATION_INFO, CST_location_info_callback);
  (void)osCDS_subscribe_net_event(CS_URCEVENT_GPRS_LOCATION_INFO, CST_location_info_callback);
  (void)osCDS_subscribe_net_event(CS_URCEVENT_CS_LOCATION_INFO, CST_location_info_callback);
}

/**
  * @brief  subscribes to modem events
  * @param  -
  * @retval -
  */
static void CST_subscribe_modem_events(void)
{
  PRINT_CELLULAR_SERVICE("Subscribe modems events\n\r")
  CS_ModemEvent_t events_mask = (CS_ModemEvent_t)((uint16_t)CS_MDMEVENT_BOOT       |
                                                  (uint16_t)CS_MDMEVENT_POWER_DOWN |
                                                  (uint16_t)CS_MDMEVENT_FOTA_START |
#if (USE_LOW_POWER == 1)
                                                  (uint16_t)CS_MDMEVENT_LP_ENTER   |
                                                  (uint16_t)CS_MDMEVENT_WAKEUP_REQ |
#endif  /* (USE_LOW_POWER == 1) */
                                                  (uint16_t)CS_MDMEVENT_FOTA_END);
  (void)osCDS_subscribe_modem_event(events_mask, CST_modem_event_callback);
}


/**
  * @brief  gets automaton event from message event
  * @param  event  - message event
  * @retval CST_autom_event_t - automaton event
  */
static CST_autom_event_t CST_get_autom_event(osEvent event)
{
  static dc_cellular_target_state_t cst_target_state;            /* new target state requested   */

  CST_autom_event_t autom_event;
  cst_message_t  message;
  cst_message_t *message_p;
  autom_event = CST_NO_EVENT;
  message_p = (cst_message_t *) & (event.value.v);
  message   = *message_p;

  /*  types of messages:
       -> CS automaton event
       -> CS CMD
       -> DC EVENT  (DC_CELLULAR_DATA_INFO: / FAIL)
  */
  if (message.type == CST_MESSAGE_CS_EVENT)
  {
    /* Cellular Event */
    autom_event = (CST_autom_event_t)message.id;
  }
  else if (message.type == CST_MESSAGE_CMD)
  {
    /* Command Event */
    autom_event = (CST_autom_event_t)message.id;
  }
  else if (message.type == CST_MESSAGE_DC_EVENT)
  {
    /* Data Cache Event */
    if (message.id == (uint16_t)DC_CELLULAR_DATA_INFO)
    {
      /* DC_CELLULAR_DATA_INFO entry event */
      (void)dc_com_read(&dc_com_db, DC_CELLULAR_DATA_INFO, (void *)&cst_cellular_data_info,
                        sizeof(cst_cellular_data_info));
      if (cst_cellular_data_info.rt_state == DC_SERVICE_FAIL)
      {
        autom_event = CST_CELLULAR_DATA_FAIL_EVENT;
      }
    }
    if (message.id == (uint16_t)DC_CELLULAR_TARGET_STATE_CMD)
    {
      /* New modem target state  request */
      (void)dc_com_read(&dc_com_db, DC_CELLULAR_TARGET_STATE_CMD, (void *)&cst_target_state, sizeof(cst_target_state));
      if (cst_target_state.rt_state == DC_SERVICE_ON)
      {
        cst_cellular_params.target_state = cst_target_state.target_state;
        autom_event = CST_TARGET_STATE_CMD_EVENT;
      }
    }
    if (message.id == (uint16_t) DC_CELLULAR_NIFMAN_INFO)
    {
      /* NIFMAN Event */
      autom_event = CST_NIFMAN_EVENT;
    }
  }
  else
  {
    __NOP(); /* Nothing to do */
  }
  return autom_event;
}

/**
  * @brief  gets network status
  * @param  -
  * @retval cst_network_status_t - network status
  */
static cst_network_status_t  CST_get_network_status(void)
{
  CS_Status_t cs_status;
  CS_RegistrationStatus_t reg_status;
  cst_network_status_t ret;

  (void)memset((void *)&reg_status, 0, sizeof(reg_status));

  cs_status = osCDS_get_net_status(&reg_status);
  if (cs_status == CELLULAR_OK)
  {
    /* service available */
    cst_context.current_EPS_NetworkRegState  = reg_status.EPS_NetworkRegState;
    cst_context.current_GPRS_NetworkRegState = reg_status.GPRS_NetworkRegState;
    cst_context.current_CS_NetworkRegState   = reg_status.CS_NetworkRegState;

    if ((cst_context.current_EPS_NetworkRegState  != CS_NRS_REGISTERED_HOME_NETWORK)
        && (cst_context.current_EPS_NetworkRegState  != CS_NRS_REGISTERED_ROAMING)
        && (cst_context.current_GPRS_NetworkRegState != CS_NRS_REGISTERED_HOME_NETWORK)
        && (cst_context.current_GPRS_NetworkRegState != CS_NRS_REGISTERED_ROAMING))
    {
      /* network not registered */
      ret = CST_NOT_REGISTERED;
    }
    else /* device registered to network */
    {
      /* network registered */
      ret = CST_NET_REGISTERED;
      if (((uint16_t)reg_status.optional_fields_presence & (uint16_t)CS_RSF_FORMAT_PRESENT) != 0U)
      {
        /* update DC */
        (void)dc_com_read(&dc_com_db,  DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));
        (void)memcpy(cst_cellular_info.mno_name, reg_status.operator_name, DC_MAX_SIZE_MNO_NAME - 1U);
        cst_cellular_info.mno_name[DC_MAX_SIZE_MNO_NAME - 1U] = 0;  /* to avoid a non null terminated string */
        cst_cellular_info.rt_state = DC_SERVICE_ON;
        (void)dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));
        PRINT_CELLULAR_SERVICE(" ->operator_name = %s", reg_status.operator_name)
      }
    }
  }
  else
  {
    /* service not available */
    cst_context.current_EPS_NetworkRegState  = CS_NRS_NOT_REGISTERED_SEARCHING;
    cst_context.current_GPRS_NetworkRegState = CS_NRS_NOT_REGISTERED_SEARCHING;
    cst_context.current_CS_NetworkRegState   = CS_NRS_NOT_REGISTERED_SEARCHING;
    ret = CST_NET_STATUS_ERRROR;
  }
  return ret;
}

/**
  * @brief  close network interface
  * @param  -
  * @retval -
  */
static void CST_close_network_interface(void)
{
  dc_nifman_info_t nifman_info;

  PRINT_CELLULAR_SERVICE("*********** CST_close_network_interface ********\n\r")
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
  CST_data_cache_cellular_info_set(DC_SERVICE_OFF);
  if (nifman_info.rt_state   ==  DC_SERVICE_ON)
  {
    /* Loop allowing to wait for the effective closing of the network interface with a max of 10s */
    for (uint8_t i = 0U; i < 10U; i++)
    {
      (void)dc_com_read(&dc_com_db, DC_CELLULAR_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
      if (nifman_info.rt_state !=  DC_SERVICE_OFF)
      {
        PRINT_CELLULAR_SERVICE("*********** wait for closing Network Interface ********\n\r")
        (void)osDelay(1000);
      }
      else
      {
        PRINT_CELLULAR_SERVICE("nifman_info.rt_state: DC_SERVICE_OFF: NW IF CLOSED DOWN\n\r")
        break;
      }
    }
  }
}


/**
  * @brief  setting FAIL mode in DC
  * @param  -
  * @retval -
  */
static void CST_fail_setting(void)
{
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(dc_cellular_info_t));
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_DATA_INFO, (void *)&cst_cellular_data_info,
                    sizeof(dc_cellular_data_info_t));
  cst_cellular_info.rt_state      = DC_SERVICE_FAIL;
  cst_cellular_data_info.rt_state = DC_SERVICE_FAIL;
  (void)dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(dc_cellular_info_t));
  (void)dc_com_write(&dc_com_db, DC_CELLULAR_DATA_INFO, (void *)&cst_cellular_data_info,
                     sizeof(dc_cellular_data_info_t));
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

  PRINT_CELLULAR_SERVICE("====================================CST_pdn_event_callback (cid=%d / event=%d)\n\r",
                         cid, pdn_event)
  /* sends a message to automaton */
  cst_context.pdn_status = pdn_event;
  CST_send_message(CST_MESSAGE_CS_EVENT, CST_PDN_STATUS_TO_CHECK_EVENT);

}

/**
  * @brief  URC callback (Unsollicited Result Code from modem)
  * @param  -
  * @retval -
  */
static void CST_network_reg_callback(void)
{
  PRINT_CELLULAR_SERVICE("==================================CST_network_reg_callback\n\r")
  /* sends a message to automaton */
  CST_send_message(CST_MESSAGE_CS_EVENT, CST_NETWORK_CALLBACK_EVENT);
}

/**
  * @brief  location info callback callback
  * @param  -
  * @retval -
  */
static void CST_location_info_callback(void)
{
  /* Not used yet: reserved for future usage */
  PRINT_CELLULAR_SERVICE("CST_location_info_callback\n\r")
}


/**
  * @brief  modem event calback
  * @param  event - modem event
  * @retval -
  */
static void CST_modem_event_callback(CS_ModemEvent_t event)
{
  /* event is a bitmask, we can have more than one evt reported at the same time */
  if (((uint16_t)event & (uint16_t)CS_MDMEVENT_BOOT) != 0U)
  {
    /* reboot modem has occurred: sends a message to automaton */
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_REBOOT_MODEM_EVENT);
  }
  if (((uint16_t)event & (uint16_t)CS_MDMEVENT_POWER_DOWN) != 0U)
  {
    /* Modem power down event: sends a message to automaton */
    PRINT_CELLULAR_SERVICE("Modem event received:  CS_MDMEVENT_POWER_DOWN\n\r")
    /* Nothing to do */
  }
  if (((uint16_t)event & (uint16_t)CS_MDMEVENT_FOTA_START) != 0U)
  {
    /* FOTA programmation start : send a message to automaton */
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_FOTA_START_EVENT);
  }
  if (((uint16_t)event & (uint16_t)CS_MDMEVENT_FOTA_END) != 0U)
  {
    /* FOTA programmation end : sends a message to automaton */
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_FOTA_END_EVENT);
  }

#if (USE_LOW_POWER == 1)
  /* ---------------- */
  /* Low Power events */
  /* ---------------- */

  if (((uint16_t)event & (uint16_t)CS_MDMEVENT_LP_ENTER) != 0U)
  {
    /* Enter Low power: sends a message to automaton  */
    PRINT_CELLULAR_SERVICE("Modem event received:  CS_MDMEVENT_LP_ENTER\n\r")
    CST_send_message(CST_MESSAGE_CMD, CST_POWER_SLEEP_COMPLETE_EVENT);
  }

  /* Modem requests to leave Low Power  */
  if (((uint16_t)event & (uint16_t)CS_MDMEVENT_WAKEUP_REQ) != 0U)
  {
    /* Modem requests to leave Low Power  */
    (void)CSP_DataWakeup(MODEM_WAKEUP);
    PRINT_CELLULAR_SERVICE("Modem event received:  CS_MDMEVENT_WAKEUP_REQ\n\r")
  }

  if (((uint16_t)event & (uint16_t)CS_MDMEVENT_LP_LEAVE) != 0U)
  {
    /* Modem Leave Low Power state  */
    PRINT_CELLULAR_SERVICE("Modem event received:  CS_MDMEVENT_LP_LEAVE\n\r")
    /* Nothing to do */
  }
#endif /* (USE_LOW_POWER == 1) */
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
  uint32_t old_apn_len;
  uint32_t new_apn_len;

  if ((dc_event_id == DC_CELLULAR_DATA_INFO)
      || (dc_event_id == DC_CELLULAR_NIFMAN_INFO)
      || (dc_event_id == DC_CELLULAR_TARGET_STATE_CMD))
  {
    /* sends a message to automaton */
    CST_send_message(CST_MESSAGE_DC_EVENT, (CST_autom_event_t)dc_event_id);
  }
  else if (dc_event_id == DC_CELLULAR_APN_CONFIG)
  {
    /* new apn config required  */
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_APN_CONFIG, (void *)&cst_apn_config,
                      sizeof(dc_apn_config_t));

    if (cst_apn_config.rt_state == DC_SERVICE_ON)
    {
      old_apn_len = crs_strlen(cst_cellular_params.sim_slot[cst_context.sim_slot_index].apn);
      new_apn_len = crs_strlen(cst_apn_config.apn);
      if ((old_apn_len != new_apn_len)
          ||
          (memcmp(cst_apn_config.apn, cst_cellular_params.sim_slot[cst_context.sim_slot_index].apn, new_apn_len) != 0))
      {
#if (!USE_DEFAULT_SETUP == 1)
        uint32_t ret;
        /* stores new APN config in FEEPROM setup params */
        ret = CST_update_config_setup_handler(&cst_apn_config, cst_sim_info.active_slot);
        if (ret != 0U)
#endif /* (!USE_DEFAULT_SETUP == 1) */
        {
          CST_set_state(CST_APN_CONFIG_STATE);
          /* sends a message to automaton */
          CST_send_message(CST_MESSAGE_CS_EVENT, CST_APN_CONFIG_EVENT);
        }
      }
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
static void CST_polling_timer_callback(void const *argument)
{
  UNUSED(argument);
  if (((cst_context.current_state == CST_WAITING_FOR_SIGNAL_QUALITY_OK_STATE)
       || (cst_context.current_state == CST_WAITING_FOR_NETWORK_STATUS_STATE)
       || (cst_context.current_state == CST_MODEM_DATA_READY_STATE))
      && (cst_nfmc_context.nfmc_timer_on_going == false)
      && (CST_polling_active == true)
     )
  {
    /* something to do in this state: sends a message to automaton */
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_POLLING_TIMER_EVENT);
  }
}

/**
  * @brief  timer callback to retry PDN activation
  * @param  argument - argument (not used)
  * @retval -
  */
static void CST_pdn_activate_retry_timer_callback(void const *argument)
{
  UNUSED(argument);
  PRINT_CELLULAR_SERVICE("*********** CST_pdn_activate_retry_timer_callback ********\n\r")
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
static void CST_register_retry_timer_callback(void const *argument)
{
  UNUSED(argument);
  /* automaton to reinitialize : sends a message to automaton */
  cst_nfmc_context.nfmc_timer_on_going = false;

  CST_set_state(CST_MODEM_INIT_STATE);
  CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_INIT_EVENT);
}

/**
  * @brief  timer callback to check network status
  * @param  argument - argument (not used)
  * @retval -
  */
static void CST_network_status_timer_callback(void const *argument)
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
static void CST_fota_timer_callback(void const *argument)
{
  UNUSED(argument);
  /* FOTA timeout has occured: sends a message to automaton */
  CST_send_message(CST_MESSAGE_CS_EVENT, CST_FOTA_TIMEOUT_EVENT);
}


/* ===================================================================
   Callback functions  END
   =================================================================== */

/* ===================================================================
   Automaton processing functions  BEGIN
   =================================================================== */

/**
  * @brief  automation boot in the special case where the modem is powered on alone without network attachement
  * @param  -
  * @retval -
  */
static void CST_boot_power_on_only_modem_mngt(void)
{
  PRINT_CELLULAR_SERVICE("*********** CST_boot_modem_power_on_only_mngt ********\n\r")
  /* Set new state and sends a message to automaton */
  CST_set_state(CST_MODEM_INIT_STATE);
  CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_POWER_ON_ONLY_EVENT);
}

/**
  * @brief  automation init in the special case where the modem is powered on alone without network attachement
  * @param  -
  * @retval -
  */
static void CST_init_power_on_only_modem_mngt(void)
{
  PRINT_CELLULAR_SERVICE("*********** CST_power_on_only_modem_mngt ********\n\r")
  /* Modem is powered on */
  (void)osCDS_power_on();
  CST_set_state(CST_MODEM_POWER_ON_ONLY_STATE);
}

/**
  * @brief  automaton boot in normal case
  * @param  -
  * @retval -
  */
static void CST_boot_event_mngt(void)
{
  PRINT_CELLULAR_SERVICE("*********** CST_boot_event_mngt ********\n\r")
  CST_set_state(CST_MODEM_INIT_STATE);
  CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_INIT_EVENT);
}
/**
  * @brief  automation init in normal case
  * @param  -
  * @retval -
  */
static void CST_init_state_mngt(void)
{
  CS_Status_t cs_status;

  PRINT_CELLULAR_SERVICE("*********** CST_init_state_mngt ********\n\r")

  if (cst_cellular_params.target_state == DC_TARGET_STATE_OFF)
  {
    /* modem target state required: OFF. */
    CST_set_state(CST_MODEM_OFF_STATE);
    /* Data Cache -> Radio OFF */
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(dc_cellular_info_t));
    cst_cellular_info.modem_state = DC_MODEM_STATE_OFF;
    (void)dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(dc_cellular_info_t));
  }
  else
  {
    if (cst_nfmc_context.nfmc_timer_on_going == false)
    {
      /* modem target state required: SIM ONLY or FULL */

      /* Modem to power on */
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
        cst_cellular_info.modem_state = DC_MODEM_STATE_POWERED_ON;
        (void)dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(dc_cellular_info_t));
        CST_modem_sim_init();
      }
    }
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
  /* if current_state == CST_MODEM_INIT_STATE then nothing todo (nominal case) */
  if (cst_context.current_state != CST_MODEM_INIT_STATE)
  {
    /* modem power off */
    (void)osCDS_power_off();

    /* something to do in this state: sends a message to automaton */
    PRINT_CELLULAR_SERVICE("Modem event received: CS_MDMEVENT_BOOT\n\r")
    CST_set_state(CST_MODEM_INIT_STATE);
    CST_data_cache_cellular_info_set(DC_SERVICE_OFF);
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
  /* FOTA event: FOTA start */
  PRINT_CELLULAR_SERVICE("Modem event received:  CS_MDMEVENT_FOTA_START\n\r")
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_DATA_INFO, (void *)&cst_cellular_data_info,
                    sizeof(cst_cellular_data_info));
  cst_cellular_data_info.rt_state = DC_SERVICE_SHUTTING_DOWN;
  CST_set_state(CST_MODEM_REPROG_STATE);
  (void)dc_com_write(&dc_com_db, DC_CELLULAR_DATA_INFO, (void *)&cst_cellular_data_info,
                     sizeof(cst_cellular_data_info));
  (void)osTimerStart(cst_fota_timer_handle, CST_FOTA_TIMEOUT);
}

/**
  * @brief  FOTA end
  * @param  -
  * @retval -
  */
static void CST_fota_end_event_mngt(void)
{
  /* FOTA event: FOTA end */
  /* stops FOTA timeout timer */
  (void)osTimerStop(cst_fota_timer_handle);
  PRINT_CELLULAR_SERVICE("Modem event received:  CS_MDMEVENT_FOTA_END\n\r")

  /* TRIGGER PLATFORM RESET after a delay  */
  PRINT_CELLULAR_SERVICE("TRIGGER PLATFORM REBOOT AFTER FOTA UPDATE ...\n\r")
  ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 3, ERROR_FATAL);
}

/**
  * @brief  FOTA timeout
  * @param  -
  * @retval -
  */
static void CST_fota_timeout_event_mngt(void)
{
  /* FOTA timeout has occured : FAIL */

  PRINT_CELLULAR_SERVICE("CST FOTA FAIL : Timeout expired RESTART\n\r")

  ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 4, ERROR_FATAL);
}

/**
  * @brief  modem reset processing
  * @param  -
  * @retval -
  */
static void CST_modem_reset_mngt(void)
{
  CS_Status_t cs_status;

  PRINT_CELLULAR_SERVICE("*********** CST_modem_reset_mngt ********\n\r")

  /* modem power off */
  cs_status = osCDS_power_off();

  if (cs_status != CELLULAR_OK)
  {
    CST_config_fail(((uint8_t *)"CST_modem_reset_mngt"),
                    CST_MODEM_RESET_FAIL,
                    &cst_context.reset_count,
                    CST_RESET_MAX);

  }
  else
  {
    /* Data Cache -> Data transfer off */
    CST_data_cache_cellular_info_set(DC_SERVICE_OFF);

    /* Data Cache -> Radio ON */
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(dc_cellular_info_t));
    cst_cellular_info.rt_state = DC_SERVICE_ON;
    (void)dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(dc_cellular_info_t));

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

  PRINT_CELLULAR_SERVICE("=== CST_net_register_mngt ===\n\r")

  /* find network and register when found */
  cs_status = osCDS_register_net(&ctxt_operator, &cst_ctxt_reg_status);
  if (cs_status == CELLULAR_OK)
  {
    /* service OK: update current network status */
    cst_context.current_EPS_NetworkRegState  = cst_ctxt_reg_status.EPS_NetworkRegState;
    cst_context.current_GPRS_NetworkRegState = cst_ctxt_reg_status.GPRS_NetworkRegState;
    cst_context.current_CS_NetworkRegState   = cst_ctxt_reg_status.CS_NetworkRegState;
    /*   to force to attach to PS domain by default (in case the Modem does not perform automatic PS attach.) */
    /*   need to check target state in future. */
    (void)osCDS_attach_PS_domain();

    /* current state changes */
    CST_set_state(CST_WAITING_FOR_SIGNAL_QUALITY_OK_STATE);
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_SIGNAL_QUALITY_TO_CHECK_EVENT);
  }
  else
  {
    /* unrecoverable error : fail */
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

  if (cs_status == CELLULAR_OK)
  {
    /* Signal Quality is OK */
    /* Start attachement timeout */
    (void)osTimerStart(cst_network_status_timer_handle, cst_cellular_params.attachment_timeout);
    PRINT_CELLULAR_SERVICE("-----> Start NW REG TIMEOUT TIMER   : %ld\n\r", cst_cellular_params.attachment_timeout)

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

  PRINT_CELLULAR_SERVICE("*********** CST_network_status_test_mngt ********\n\r")


  ret = CST_get_network_status();

  if (ret == CST_NET_REGISTERED)
  {
    /* When registered then stop the NW REG TIMER */
    (void)osTimerStop(cst_network_status_timer_handle);
    PRINT_CELLULAR_SERVICE("-----> Stop NW REG TIMEOUT TIMER\n\r")
    cst_context.register_retry_tempo_count = 0U;
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
      (void)osTimerStop(cst_network_status_timer_handle);
      PRINT_CELLULAR_SERVICE("-----> BAD SIGNAL : Stop NW REG TIMEOUT TIMER\n\r")
      cst_context.register_retry_tempo_count = 0U;
      CST_set_state(CST_WAITING_FOR_SIGNAL_QUALITY_OK_STATE);
      CST_send_message(CST_MESSAGE_CS_EVENT, CST_SIGNAL_QUALITY_TO_CHECK_EVENT);
    }
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

  PRINT_CELLULAR_SERVICE("*********** CST_pdn_event_nw_detach_mngt ********\n\r")
  if (cst_context.current_state == CST_MODEM_DATA_READY_STATE)
  {
    /* if current state is data ready then close data transfer service */
    CST_close_network_interface();
  }

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  osCCS_get_wait_cs_resource();
  (void)osCDS_suspend_data();
#endif  /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */

  /* get new network status */
  ret = CST_get_network_status();

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  osCCS_get_release_cs_resource();
#endif      /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
  /* we informs NIFMAN to release the PPP link (we should keep the same for socket mode as well... */

  if (ret == CST_NET_REGISTERED)
  {
    /* Modem registered to network : Nothing to do */
  }
  else if (ret == CST_NOT_REGISTERED)
  {
    /* Modem no more registered to network : set data tranfer service off */
    CST_data_cache_cellular_info_set(DC_SERVICE_OFF);
  }
  else
  {
    /* network status not available : set data tranfer service off */
    CST_data_cache_cellular_info_set(DC_SERVICE_OFF);
    PRINT_CELLULAR_SERVICE("******** CST_pdn_event_nw_detach_mngt: osCDS_get_net_status FAIL ****\n\r")
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
    /* Modem no more registered to network : set data tranfer service off */
    CST_data_cache_cellular_info_set(DC_SERVICE_OFF);
    CST_set_state(CST_WAITING_FOR_NETWORK_STATUS_STATE);
    /* Not event sent because waiting for network status modification (timer polling or modem urc */
  }
  else
  {
    /* network status not available : set data tranfer service off */
    CST_data_cache_cellular_info_set(DC_SERVICE_OFF);
    PRINT_CELLULAR_SERVICE("******** CST_network_event_mngt: osCDS_get_net_status FAIL ****\n\r")
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
  PRINT_CELLULAR_SERVICE("-----> NW REG TIMEOUT TIMER EXPIRY WE PWDN THE MODEM \n\r")
  if (cst_nfmc_context.active == true)
  {
    cst_nfmc_context.nfmc_timer_on_going = true;

    /* modem power off */
    (void)osCDS_power_off();

    /* start retry NFMC tempo */
    (void)osTimerStart(cst_register_retry_timer_handle,
                       cst_nfmc_context.tempo[cst_context.register_retry_tempo_count]);
    PRINT_CELLULAR_SERVICE("-----> CST_waiting_for_network_status NOK - retry tempo %d : %ld\n\r",
                           cst_context.register_retry_tempo_count + 1U,
                           cst_nfmc_context.tempo[cst_context.register_retry_tempo_count])

    cst_context.register_retry_tempo_count++;
    if (cst_context.register_retry_tempo_count >= CST_NFMC_TEMPO_NB)
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

  PRINT_CELLULAR_SERVICE("*********** CST_attach_modem_mngt ********\n\r")

  (void)memset((void *)&reg_status, 0, sizeof(CS_RegistrationStatus_t));

  /* gets net status */
  cs_status = osCDS_get_net_status(&reg_status);
  if (cs_status == CELLULAR_OK)
  {
    /* service available */
    if (((uint16_t)reg_status.optional_fields_presence & (uint16_t)CS_RSF_FORMAT_PRESENT) != 0U)
    {
      /* MNO present: store it in DC */
      (void)dc_com_read(&dc_com_db,  DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));
      (void)memcpy(cst_cellular_info.mno_name, reg_status.operator_name, DC_MAX_SIZE_MNO_NAME - 1U);
      cst_cellular_info.mno_name[DC_MAX_SIZE_MNO_NAME - 1U] = 0U;  /* to avoid a non null terminated string */
      cst_cellular_info.rt_state              = DC_SERVICE_ON;
      (void)dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));

      PRINT_CELLULAR_SERVICE(" ->operator_name = %s\n\r", reg_status.operator_name)
    }
  }

  cs_status = osCDS_get_attach_status(&cst_ctxt_attach_status);
  if (cs_status != CELLULAR_OK)
  {
    /* service not available : FAIL */
    PRINT_CELLULAR_SERVICE("*********** CST_attach_modem_mngt fail ********\n\r")
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
      PRINT_CELLULAR_SERVICE("*********** CST_attach_modem_mngt OK ********\n\r")
      CST_set_state(CST_MODEM_REGISTERED_STATE);
      CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_ATTACHED_EVENT);
    }

    else
    {
      /* modem not attached */
      PRINT_CELLULAR_SERVICE("===CST_attach_modem_mngt - NOT ATTACHED !!! ===\n\r")
      /* We propose to simply wait network event at "waiting for signal quality OK" state */
      PRINT_CELLULAR_SERVICE("===>CST_WAITING_FOR_SIGNAL_QUALITY_OK_STATE <===\n\r")
      CST_set_state(CST_WAITING_FOR_SIGNAL_QUALITY_OK_STATE);
    }
  }
}

/**
  * @brief  modem activated management
  * @param  -
  * @retval -
  */
static void  CST_modem_activated_mngt(void)
{
  /* resets failure couters  */
  cst_context.power_on_reset_count       = 0U;
  cst_context.reset_count                = 0U;
  cst_context.csq_reset_count            = 0U;
  cst_context.attach_reset_count         = 0U;
  cst_context.activate_pdn_reset_count   = 0U;
  cst_context.cellular_data_retry_count  = 0U;
  cst_context.global_retry_count         = 0U;

  /* set state to data trasfert ready */
  CST_set_state(CST_MODEM_DATA_READY_STATE);
  CST_data_cache_cellular_info_set(DC_SERVICE_ON);

  /* Data Cache -> Data transfer available */
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(dc_cellular_info_t));
  cst_cellular_info.modem_state = DC_MODEM_STATE_DATA_OK;
  (void)dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(dc_cellular_info_t));
}

/**
  * @brief  PDN definition management
  * @param  -
  * @retval -
  */
static void CST_modem_define_pdn(void)
{
  CS_PDN_configuration_t pdn_conf;
  uint32_t size;

  CS_Status_t cs_status;
  /* define user PDN configurations */

  /* common user name and password */
  (void)memset((void *)&pdn_conf, 0, sizeof(CS_PDN_configuration_t));
  size =  crs_strlen(cst_cellular_params.sim_slot[cst_context.sim_slot_index].username) + 1U;
  if (size <= MAX_SIZE_USERNAME)
  {
    (void)memcpy((CRC_CHAR_t *)pdn_conf.username,
                 (CRC_CHAR_t *)cst_cellular_params.sim_slot[cst_context.sim_slot_index].username,
                 size);

    size =  crs_strlen(cst_cellular_params.sim_slot[cst_context.sim_slot_index].password) + 1U;
    if (size <= MAX_SIZE_USERNAME)
    {
      (void)memcpy((CRC_CHAR_t *)pdn_conf.password,
                   (CRC_CHAR_t *)cst_cellular_params.sim_slot[cst_context.sim_slot_index].password,
                   size);
    }
  }

  /* example for CS_PDN_USER_CONFIG_1 with access point name =  "PDN CONFIG 1" */
  cs_status = osCDS_define_pdn(cst_get_cid_value(cst_cellular_params.sim_slot[cst_context.sim_slot_index].cid),
                               (const uint8_t *)cst_cellular_params.sim_slot[cst_context.sim_slot_index].apn,
                               &pdn_conf);


  if (cs_status != CELLULAR_OK)
  {
    CST_config_fail(((uint8_t *)"CST_modem_define_pdn"),
                    CST_MODEM_PDP_DEFINE_FAIL,
                    &cst_context.activate_pdn_reset_count,
                    CST_DEFINE_PDN_RESET_MAX);
  }
}

/**
  * @brief  PDN activation management
  * @param  -
  * @retval -
  */
static void CST_modem_activate_pdn_mngt(void)
{
  CS_Status_t cs_status;

  CST_set_state(CST_MODEM_PDN_ACTIVATING_STATE);

  (void)osCDS_set_default_pdn(cst_get_cid_value(cst_cellular_params.sim_slot[cst_context.sim_slot_index].cid));

  /* register to PDN events for this CID*/
  (void)osCDS_register_pdn_event(cst_get_cid_value(cst_cellular_params.sim_slot[cst_context.sim_slot_index].cid),
                                 CST_pdn_event_callback);

  cs_status = osCDS_activate_pdn(CS_PDN_CONFIG_DEFAULT);

  if (cs_status != CELLULAR_OK)
  {
    if (cst_nfmc_context.active == false)
    {
      (void)osTimerStart(cst_pdn_activate_retry_timer_handle, CST_PDN_ACTIVATE_RETRY_DELAY);
      PRINT_CELLULAR_SERVICE("-----> CST_modem_activate_pdn_mngt NOK - retry tempo  : %d\n\r",
                             CST_PDN_ACTIVATE_RETRY_DELAY)
    }
    else
    {
      (void)osTimerStart(cst_pdn_activate_retry_timer_handle,
                         cst_nfmc_context.tempo[cst_context.activate_pdn_nfmc_tempo_count]);
      PRINT_CELLULAR_SERVICE("-----> CST_modem_activate_pdn_mngt NOK - retry tempo %d : %ld\n\r",
                             cst_context.activate_pdn_nfmc_tempo_count + 1U,
                             cst_nfmc_context.tempo[cst_context.activate_pdn_nfmc_tempo_count])
    }

    cst_context.activate_pdn_nfmc_tempo_count++;
    if (cst_context.activate_pdn_nfmc_tempo_count >= CST_NFMC_TEMPO_NB)
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
    CST_close_network_interface();
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
    osCCS_get_wait_cs_resource();
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
  if (cst_context.pdn_status == CS_PDN_EVENT_NW_DETACH)
  {
    /* Workaround waiting for Modem behaviour clarification */
    CST_pdn_event_nw_detach_mngt();
  }
  else if (
    (cst_context.pdn_status == CS_PDN_EVENT_NW_DEACT)
    || (cst_context.pdn_status == CS_PDN_EVENT_NW_PDN_DEACT))
  {
    PRINT_CELLULAR_SERVICE("=========CST_pdn_event_mngt CS_PDN_EVENT_NW_DEACT\n\r")
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
  * @brief  cellular state event management
  * @param  -
  * @retval -
  */
static void CST_target_state_cmd_event_mngt(void)
{
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));
  cst_cellular_info.rt_state = DC_SERVICE_UNAVAIL;
  (void)dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));

  (void)dc_com_read(&dc_com_db, DC_CELLULAR_SIM_INFO, (void *)&cst_sim_info, sizeof(cst_sim_info));
  cst_sim_info.rt_state = DC_SERVICE_UNAVAIL;
  (void)dc_com_write(&dc_com_db, DC_CELLULAR_SIM_INFO, (void *)&cst_sim_info, sizeof(cst_sim_info));

  (void)dc_com_read(&dc_com_db, DC_CELLULAR_DATA_INFO, (void *)&cst_cellular_data_info,
                    sizeof(cst_cellular_data_info));
  /* informs NIFMAN that data plane is shutting down  */
  cst_cellular_data_info.rt_state = DC_SERVICE_SHUTTING_DOWN;
  (void)dc_com_write(&dc_com_db, DC_CELLULAR_DATA_INFO, (void *)&cst_cellular_data_info,
                     sizeof(cst_cellular_data_info));

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  if (cst_context.current_state == CST_MODEM_DATA_READY_STATE)
  {
    osCCS_get_wait_cs_resource();
    (void)osCDS_suspend_data();
    osCCS_get_release_cs_resource();
  }
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_LWIP */

  if (cst_cellular_params.target_state == DC_TARGET_STATE_SIM_ONLY)
  {
    /* new Modem target state 'SIM ONLY' requested*/
    PRINT_CELLULAR_SERVICE("****** Transition to CST_MODEM_SIM_ONLY_STATE Ongoing *****\n\r")
    (void) osCDS_init_modem(CS_CMI_SIM_ONLY, CELLULAR_FALSE, CST_SIM_PINCODE);
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(dc_cellular_info_t));
    cst_cellular_info.modem_state = DC_MODEM_STATE_SIM_CONNECTED;
    (void)dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(dc_cellular_info_t));
    CST_set_state(CST_MODEM_SIM_ONLY_STATE);
    PRINT_CELLULAR_SERVICE("****** CST_MODEM_SIM_ONLY_STATE *****\n\r")
  }
  else
  {
    /* new Modem target state 'MODEM OFF' requested */
    (void) osCDS_init_modem(CS_CMI_MINI, CELLULAR_FALSE, CST_SIM_PINCODE);
    /* modem power off */
    (void)osCDS_power_off();
    CST_set_state(CST_MODEM_OFF_STATE);
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_INIT_EVENT);
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
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
    osCCS_get_wait_cs_resource();
    dc_nifman_info_t nifman_info;
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
    if (nifman_info.rt_state   ==  DC_SERVICE_ON)
    {
      /* we should read the status if connection lost while changing to at command mode */
      cs_status = osCDS_suspend_data();

      /* For instance disable the signal polling to test suspend resume  */
      (void)CST_set_signal_quality();
      /* we should read the status if connection lost while resuming data */
      cs_status = osCDS_resume_data();
    }
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
  uint32_t size;
  /* read new requested apn config in Data Cache */
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_APN_CONFIG, (void *)&cst_apn_config, sizeof(dc_apn_config_t));

  /* read current cellular config in Data Cache */
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cst_cellular_params, sizeof(cst_cellular_params));

  /* update new cellular config in Data Cache*/
  cst_cellular_params.sim_slot[cst_sim_info.index_slot].cid  = cst_apn_config.cid;

  /* APN parameter */
  size = crs_strlen(cst_apn_config.apn) + 1U;
  /* to avoid string overflow */
  if (size <= DC_MAX_SIZE_APN)
  {
    (void)memcpy(cst_cellular_params.sim_slot[cst_sim_info.index_slot].apn,
                 cst_apn_config.apn,
                 size);
  }

  /* username parameter */
  size =  crs_strlen(cst_apn_config.username) + 1U;
  /* to avoid string overflow */
  if (size <= DC_CST_USERNAME_SIZE)
  {
    (void)memcpy(cst_cellular_params.sim_slot[cst_sim_info.index_slot].username,
                 cst_apn_config.username,
                 size);
  }

  /* password parameter */
  size =  crs_strlen(cst_apn_config.password) + 1U;
  /* to avoid string overflow */
  if (size <= DC_CST_PASSWORD_SIZE)
  {
    (void)memcpy(cst_cellular_params.sim_slot[cst_sim_info.index_slot].password,
                 cst_apn_config.password,
                 size);
  }

  /* set Data Cache Entry valid */
  cst_cellular_params.rt_state = DC_SERVICE_ON;
  (void)dc_com_write(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cst_cellular_params, sizeof(cst_cellular_params));

  /* Modem reset to be reconfigured with new APN */
  CST_set_state(CST_MODEM_RESET_STATE);
  CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_RESET_EVENT);
}

/**
  * @brief  nifman event managment
  * @param  -
  * @retval -
  */
static void CST_nifman_event_mngt(void)
{
  dc_nifman_info_t nifman_info;
  dc_cellular_target_state_t cst_target_state;

  (void)dc_com_read(&dc_com_db, DC_CELLULAR_TARGET_STATE_CMD, (void *)&cst_target_state, sizeof(cst_target_state));
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));

  /* check if a new modem target state command has occured */
  if (cst_target_state.rt_state == DC_SERVICE_ON)
  {
    /*  we check target state to process any target state change while Network IF is being estabhlished. */
    if (cst_target_state.target_state != DC_TARGET_STATE_FULL)
    {
      /* the target state has changed: not more FULL */
      if (nifman_info.rt_state == DC_SERVICE_ON)
      {
        CST_close_network_interface();
        CST_target_state_cmd_event_mngt();
      }
    }
  }
}

/**
  * @brief  target state event managment in case of current state == data ready
  * @param  -
  * @retval -
  */
static void CST_data_read_target_state_event_mngt(void)
{
  dc_nifman_info_t nifman_info;

  /* check if NIFMAN service is ON */
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
  if (nifman_info.rt_state == DC_SERVICE_ON)
  {
    CST_close_network_interface();
    CST_target_state_cmd_event_mngt();
  }
}

/**
  * @brief  target state event managment in case of current state == SIM_ ONLY
  * @param  -
  * @retval -
  */
static void CST_sim_only_target_state_event_mngt(void)
{
  if ((cst_cellular_params.target_state != DC_TARGET_STATE_OFF)
      && (cst_cellular_params.target_state != DC_TARGET_STATE_UNKNOWN))
  {
    CST_modem_sim_init();
  }
  else if (cst_cellular_params.target_state == DC_TARGET_STATE_OFF)
  {
    CST_target_state_cmd_event_mngt();
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
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(dc_cellular_info_t));
  cst_cellular_info.modem_state = DC_MODEM_STATE_OFF;
  (void)dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(dc_cellular_info_t));
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
static void CST_boot_state(CST_autom_event_t autom_event)
{
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
static void CST_init_state(CST_autom_event_t autom_event)
{
  switch (autom_event)
  {
    case CST_MODEM_INIT_EVENT:
      CST_init_state_mngt();
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
static void CST_modem_ready_state(CST_autom_event_t autom_event)
{
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
static void CST_waiting_for_signal_quality_ok_state(CST_autom_event_t autom_event)
{
  PRINT_CELLULAR_SERVICE("\n\r ====> CST_waiting_for_signal_quality_ok_state <===== \n\r")

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
static void CST_waiting_for_network_status_state(CST_autom_event_t autom_event)
{
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
static void CST_network_status_ok_state(CST_autom_event_t autom_event)
{
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
static void CST_modem_registered_state(CST_autom_event_t autom_event)
{
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
static void CST_modem_pdn_activating_state(CST_autom_event_t autom_event)
{
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
static void CST_data_ready_state(CST_autom_event_t autom_event)
{
  switch (autom_event)
  {
    case CST_NETWORK_CALLBACK_EVENT:
      CST_network_event_mngt();
      break;

    case CST_NIFMAN_EVENT:
      CST_nifman_event_mngt();
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
      CST_data_read_target_state_event_mngt();
      break;

    case CST_REBOOT_MODEM_EVENT:
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
static void CST_modem_off_state(CST_autom_event_t autom_event)
{
  switch (autom_event)
  {
    case CST_TARGET_STATE_CMD_EVENT:
      CST_init_state_mngt();
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
static void CST_modem_sim_only_state(CST_autom_event_t autom_event)
{
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
static void CST_modem_reset_state(CST_autom_event_t autom_event)
{
  /* reset is applied  whatever the event */
  UNUSED(autom_event);
  CST_modem_reset_mngt();
}

/**
  * @brief  FOTA state : modem formware update on going
  * @param  autom_event - automaton event
  * @retval -
  */
static void CST_modem_reprog_state(CST_autom_event_t autom_event)
{
  switch (autom_event)
  {
    case CST_FOTA_END_EVENT:
      CST_fota_end_event_mngt();
      break;

    case CST_FOTA_TIMEOUT_EVENT:
      CST_fota_timeout_event_mngt();
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
static void CST_apn_config_state(CST_autom_event_t autom_event)
{
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
static void CST_power_idle_state(CST_autom_event_t autom_event)
{
  switch (autom_event)
  {
    case CST_POWER_SLEEP_TIMEOUT_EVENT:
      CSP_WakeupComplete();
      CST_set_state(CST_MODEM_DATA_READY_STATE);
      break;

    case CST_POWER_SLEEP_ABORT_EVENT:
      CSP_WakeupComplete();
      CST_set_state(CST_MODEM_DATA_READY_STATE);
      break;

    case CST_POWER_SLEEP_COMPLETE_EVENT:
      CSP_SleepComplete();
      break;

    case CST_POWER_WAKEUP_EVENT:
      CSP_WakeupComplete();
      CST_set_state(CST_MODEM_DATA_READY_STATE);
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
#endif /* (USE_LOW_POWER == 1) */

/**
  * @brief  Maximum error number reached: Automaton remain in this state untill CST_REBOOT_MODEM_EVENT event
  * @param  autom_event - automaton event
  * @retval -
  */
static void CST_fail_state(CST_autom_event_t autom_event)
{
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
static void CST_cellular_service_task(void const *argument)
{
  UNUSED(argument);
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
    ((uint8_t *)"FAIL_EVENT"),
    ((uint8_t *)"POLLING_TIMER_EVENT"),
    ((uint8_t *)"MODEM_URC_EVENT"),
    ((uint8_t *)"NO_EVENT"),
    ((uint8_t *)"CMD_UNKWOWN_EVENT"),
    ((uint8_t *)"TARGET_STATE_CMD_EVENT"),
    ((uint8_t *)"APN_CONFIG_EVENT"),
    ((uint8_t *)"NIFMAN_EVENT"),
    ((uint8_t *)"REBOOT_MODEM_EVENT"),
    ((uint8_t *)"MODEM_POWER_ON_ONLY_EVENT"),
    ((uint8_t *)"NETWORK_CALLBACK_EVENT"),
    ((uint8_t *)"NW_REG_TIMEOUT_TIMER_EVENT"),
    ((uint8_t *)"FOTA_START_EVENT"),
    ((uint8_t *)"FOTA_END_EVENT"),
    ((uint8_t *)"FOTA_TIMEOUT_EVENT"),
    ((uint8_t *)"MODEM_RESET_EVENT"),
#if (USE_LOW_POWER == 1)
    ((uint8_t *)"POWER_SLEEP_TIMEOUT_EVENT"),
    ((uint8_t *)"POWER_SLEEP_REQUEST_EVENT"),
    ((uint8_t *)"POWER_SLEEP_COMPLETE_EVENT"),
    ((uint8_t *)"POWER_WAKEUP_EVENT"),
    ((uint8_t *)"POWER_SLEEP_ABORT_EVENT"),
#endif /* (USE_LOW_POWER == 1) */
  };
#endif /* (USE_TRACE_CELLULAR_SERVICE == 1) */
  osEvent event;
  CST_autom_event_t autom_event;

  /* Automation infinite loop */
  for (;;)
  {
    /* waiting for an automaton event */
    event = osMessageGet(cst_queue_id, RTOS_WAIT_FOREVER);
    autom_event = CST_get_autom_event(event);
    PRINT_CELLULAR_SERVICE("AUTOM TASK:  %s - %s\n\r", CST_StateName[cst_context.current_state],
                           cst_event_name[autom_event])

    if (autom_event != CST_NO_EVENT)
    {
      /* Call the right function according to the  automaton current state */
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
          /* Nothing todo */
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
      PRINT_CELLULAR_SERVICE("============ CST_cellular_service_task : autom_event = no event \n\r")
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
  cst_context.current_state = new_state;
  PRINT_CELLULAR_SERVICE("-----> New State: %s <-----\n\r", CST_StateName[new_state])

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
  * @note   to be able to return an IP address the modem must be in data tranfer state
  * @note   else an error is returned
  * @param  ip_addr_type - type of IP address
  * @param  p_ip_addr_value - IP address value returned by the function
  * @retval CS_Status_t - return code
  */
CS_Status_t CST_get_dev_IP_address(CS_IPaddrType_t *ip_addr_type, CS_CHAR_t *p_ip_addr_value)
{
  return CS_get_dev_IP_address(cst_get_cid_value(cst_cellular_params.sim_slot[cst_context.sim_slot_index].cid),
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
  CS_Status_t ret;

  CST_set_state(CST_BOOT_STATE);

  /* request modem init to Cellular Service */
  ret = CS_init();

  if (ret == CELLULAR_OK)
  {
#if (USE_CELLULAR_SERVICE_TASK_TEST == 1)
    /* instrumentation code to test automaton */
    CSTE_cellular_test_init();
#endif  /* (USE_CELLULAR_SERVICE_TASK_TEST == 1) */

    (void)osCDS_cellular_service_init();
    cst_context.csq_count_fail = 0U;

    /* polling activated by default */
    CST_polling_active = true;

    ret = CST_config_init();
#if (USE_LOW_POWER == 1)
    CSP_Init();
#endif /* (USE_LOW_POWER == 1) */

    osMessageQDef(cst_queue_id, 10, sizeof(cst_message_t));
    cst_queue_id = osMessageCreate(osMessageQ(cst_queue_id), NULL);
    if (cst_queue_id == NULL)
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 5, ERROR_FATAL);
      ret = CELLULAR_ERROR;
    }
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
  dc_nfmc_info_t nfmc_info;
  uint32_t cst_polling_period;
  dc_com_status_t dc_ret;
  uint32_t       cs_ret;
  CS_Status_t    cst_ret;
  osStatus       os_ret;

  dc_ret  = DC_COM_OK;
  cst_ret = CELLULAR_OK;
  cs_ret  = 0U;

  osTimerId         cst_polling_timer_handle;

#if (USE_CMD_CONSOLE == 1)
  (void)CST_cmd_cellular_service_start();
#endif /*  (USE_CMD_CONSOLE == 1) */

  /* reads cellular configuration in Data Cache */
  dc_ret |= dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cst_cellular_params, sizeof(cst_cellular_params));

  /* initializes Data Cache SIM slot entry  */
  dc_ret |= dc_com_read(&dc_com_db, DC_CELLULAR_SIM_INFO, (void *)&cst_sim_info, sizeof(cst_sim_info));
  cst_sim_info.sim_status[DC_SIM_SLOT_MODEM_SOCKET]       = DC_SIM_NOT_USED;
  cst_sim_info.sim_status[DC_SIM_SLOT_MODEM_EMBEDDED_SIM] = DC_SIM_NOT_USED;
  cst_sim_info.sim_status[DC_SIM_SLOT_STM32_EMBEDDED_SIM] = DC_SIM_NOT_USED;
  cst_context.sim_slot_index = 0U;
  cst_sim_info.active_slot = cst_cellular_params.sim_slot[cst_context.sim_slot_index].sim_slot_type;
  cst_sim_info.index_slot  = cst_context.sim_slot_index;
  dc_ret |= dc_com_write(&dc_com_db, DC_CELLULAR_SIM_INFO, (void *)&cst_sim_info, sizeof(cst_sim_info));

  /* request AT core to start */
  if (atcore_task_start(ATCORE_THREAD_STACK_PRIO, ATCORE_THREAD_STACK_SIZE) != ATSTATUS_OK)
  {
    /* at core start fails */
    cs_ret |= (uint32_t)CELLULAR_ERROR;
    ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 6, ERROR_WARNING);
  }

  /* register component to Data Cache  */
  dc_ret |= dc_com_register_gen_event_cb(&dc_com_db, CST_notif_callback, (const void *)NULL);
  cst_cellular_info.mno_name[0]           = 0U;
  cst_cellular_info.rt_state              = DC_SERVICE_UNAVAIL;

  /* registers component callback to Data Cache  */
  dc_ret |= dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));

  /* initializes Data Cache NFMC entry  */
  nfmc_info.rt_state = DC_SERVICE_UNAVAIL;
  dc_ret |= dc_com_write(&dc_com_db, DC_CELLULAR_NFMC_INFO, (void *)&nfmc_info, sizeof(nfmc_info));

  /* creates and starts cellar service task automaton */
  osThreadDef(cellularServiceTask, CST_cellular_service_task, CELLULAR_SERVICE_THREAD_PRIO, 0,
              USED_CELLULAR_SERVICE_THREAD_STACK_SIZE);
  cst_cellular_service_thread_id = osThreadCreate(osThread(cellularServiceTask), NULL);

  if (cst_cellular_service_thread_id == NULL)
  {
    /* thread creation fails */
    cs_ret |= (uint32_t)CELLULAR_ERROR;
    ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 7, ERROR_FATAL);
  }
  else
  {
#if (USE_STACK_ANALYSIS == 1)
    (void)stackAnalysis_addStackSizeByHandle(cst_cellular_service_thread_id, USED_CELLULAR_SERVICE_THREAD_STACK_SIZE);
#endif /* USE_STACK_ANALYSIS == 1 */
  }

  /* creates and start modem polling timer */
  osTimerDef(cs_polling_timer, CST_polling_timer_callback);
  cst_polling_timer_handle = osTimerCreate(osTimer(cs_polling_timer), osTimerPeriodic, NULL);
#if (CST_MODEM_POLLING_PERIOD == 0)
  cst_polling_period = CST_MODEM_POLLING_PERIOD_DEFAULT;
#else
  cst_polling_period = CST_MODEM_POLLING_PERIOD;
#endif /* (CST_MODEM_POLLING_PERIOD == 1) */
  os_ret = osTimerStart(cst_polling_timer_handle, cst_polling_period);
  if (os_ret != osOK)
  {
    /* polling timer start fails */
    cs_ret |= (uint32_t)CELLULAR_ERROR;
  }

  /* creates timers */

  /* initializes pdn activation timer */
  osTimerDef(CST_pdn_activate_retry_timer, CST_pdn_activate_retry_timer_callback);
  cst_pdn_activate_retry_timer_handle = osTimerCreate(osTimer(CST_pdn_activate_retry_timer), osTimerOnce, NULL);

  /* initializes network monitoring state timer */
  osTimerDef(CST_waiting_for_network_status_timer, CST_network_status_timer_callback);
  cst_network_status_timer_handle = osTimerCreate(osTimer(CST_waiting_for_network_status_timer), osTimerOnce, NULL);

  /* initializes register timer */
  osTimerDef(CST_register_retry_timer, CST_register_retry_timer_callback);
  cst_register_retry_timer_handle = osTimerCreate(osTimer(CST_register_retry_timer), osTimerOnce, NULL);

  /* initializes FOTA timer */
  osTimerDef(CST_fota_timer, CST_fota_timer_callback);
  cst_fota_timer_handle = osTimerCreate(osTimer(CST_fota_timer), osTimerOnce, NULL);

  if ((dc_ret != DC_COM_OK) || (cs_ret != 0U))
  {
    /* At least one error occurs during start function */
    cst_ret = CELLULAR_ERROR;
  }

  return cst_ret;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

