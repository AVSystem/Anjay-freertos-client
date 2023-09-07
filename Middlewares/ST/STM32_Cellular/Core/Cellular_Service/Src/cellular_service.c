/**
  ******************************************************************************
  * @file    cellular_service.c
  * @author  MCD Application Team
  * @brief   This file defines functions for Cellular Service which
  *
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
#include <string.h>
#include "plf_config.h"
#include "cellular_service.h"
#include "cellular_service_int.h"
#include "at_core.h"
#include "at_datapack.h"
#include "at_util.h"
#include "at_sysctrl.h"
#include "cellular_runtime_custom.h"

/** @addtogroup CELLULAR_SERVICE CELLULAR_SERVICE
  * @{
  */

/** @addtogroup CELLULAR_SERVICE_API CELLULAR_SERVICE API
  * @{
  */

/** @defgroup CELLULAR_SERVICE_API_Private_Macros CELLULAR_SERVICE API Private Macros
  * @{
  */

#if (USE_TRACE_CELLULAR_SERVICE == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_INFO(format, args...) TRACE_PRINT(DBG_CHAN_MAIN, DBL_LVL_P0, "CS:" format "\n\r", ## args)
#define PRINT_DBG(format, args...)  TRACE_PRINT(DBG_CHAN_MAIN, DBL_LVL_P1, "CS:" format "\n\r", ## args)
#define PRINT_API(format, args...)  TRACE_PRINT(DBG_CHAN_MAIN, DBL_LVL_P2, "CS API:" format "\n\r", ## args)
#define PRINT_ERR(format, args...)  TRACE_PRINT(DBG_CHAN_MAIN, DBL_LVL_ERR, "CS ERROR:" format "\n\r", ## args)
#else
#include <stdio.h>
#define PRINT_INFO(format, args...)  (void) printf("CS:" format "\n\r", ## args);
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_API(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(format, args...)   (void) printf("CS ERROR:" format "\n\r", ## args);
#endif /* USE_PRINTF */
#else
#define PRINT_INFO(...)   __NOP(); /* Nothing to do */
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_API(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(...)   __NOP(); /* Nothing to do */
#endif /* USE_TRACE_CELLULAR_SERVICE */

/**
  * @}
  */

/** @defgroup CELLULAR_SERVICE_API_Private_Variables CELLULAR_SERVICE API Private Variables
  * @{
  */

/* URC callbacks declaration */
#define CS_MAX_NB_PDP_CTXT  ((uint8_t) CS_PDN_CONFIG_MAX + 1U)
/* network registration callbacks */
static cellular_urc_callback_t urc_eps_network_registration_callback = NULL;
static cellular_urc_callback_t urc_gprs_network_registration_callback = NULL;
static cellular_urc_callback_t urc_cs_network_registration_callback = NULL;
/* location info callbacks */
static cellular_urc_callback_t urc_eps_location_info_callback = NULL;
static cellular_urc_callback_t urc_gprs_location_info_callback = NULL;
static cellular_urc_callback_t urc_cs_location_info_callback = NULL;
/* signal quality callback */
static cellular_urc_callback_t urc_signal_quality_callback = NULL;
/* PDN event callback */
static cellular_pdn_event_callback_t urc_packet_domain_event_callback[CS_MAX_NB_PDP_CTXT] = {NULL};
/* Modem event callback */
static cellular_modem_event_callback_t urc_modem_event_callback = NULL;
/* Power status callback */
static cellular_power_status_callback_t urc_lp_status_callback = NULL;
/* SIM event callback */
static cellular_sim_event_callback_t urc_sim_event_callback = NULL;

#if defined(USE_COM_MDM)
static CS_comMdm_callback_t urc_commdm_event_callback = NULL;
#endif /* defined(USE_COM_MDM) */

/* Non-permanent variables */

/* Context for URC subscriptions */
static csint_urc_subscription_t cs_ctxt_urc_subscription =
{
  .eps_network_registration = CS_FALSE,
  .gprs_network_registration = CS_FALSE,
  .cs_network_registration = CS_FALSE,
  .eps_location_info = CS_FALSE,
  .gprs_location_info = CS_FALSE,
  .cs_location_info = CS_FALSE,
  .signal_quality = CS_FALSE,
  .packet_domain_event = CS_FALSE,
  .ping_rsp = CS_FALSE,
};
/* Context for EPS location information */
static csint_location_info_t cs_ctxt_eps_location_info =
{
  .ci = 0,
  .lac = 0,
  .ci_updated = CS_FALSE,
  .lac_updated = CS_FALSE,
};
/* Context for GPRS location information */
static csint_location_info_t cs_ctxt_gprs_location_info =
{
  .ci = 0,
  .lac = 0,
  .ci_updated = CS_FALSE,
  .lac_updated = CS_FALSE,
};
/* Context for CS location information */
static csint_location_info_t cs_ctxt_cs_location_info =
{
  .ci = 0,
  .lac = 0,
  .ci_updated = CS_FALSE,
  .lac_updated = CS_FALSE,
};
/* Context for EPS network registration state */
static CS_NetworkRegState_t cs_ctxt_eps_network_reg_state = CS_NRS_UNKNOWN;
/* Context for GPRS network registration state */
static CS_NetworkRegState_t cs_ctxt_gprs_network_reg_state = CS_NRS_UNKNOWN;
/* Context for CS network registration state */
static CS_NetworkRegState_t cs_ctxt_cs_network_reg_state = CS_NRS_UNKNOWN;
/**
  * @}
  */

/** @defgroup CELLULAR_SERVICE_API_Private_Functions_Prototypes CELLULAR_SERVICE API Private Functions Prototypes
  * @{
  */
static void CELLULAR_reset_context(void);
static void CELLULAR_reset_socket_context(void);
static CS_Status_t CELLULAR_init(void);
static void CELLULAR_urc_notif(at_buf_t *p_urc_buf);
static CS_Status_t CELLULAR_analyze_error_report(at_buf_t *p_rsp_buf);
static CS_Status_t convert_SIM_error(const csint_error_report_t *p_error_report);
static CS_Status_t perform_HW_reset(void);
static CS_Status_t perform_SW_reset(void);
static CS_Status_t perform_Factory_reset(void);
static CS_PDN_event_t convert_to_PDN_event(csint_PDN_event_desc_t event_desc);
static CS_PDN_conf_id_t convert_index_to_PDN_conf(uint8_t index);
/**
  * @}
  */

/** @defgroup CELLULAR_SERVICE_API_Exported_Functions CELLULAR_SERVICE API Exported Functions
  * @{
  */
/**
  * @brief  Initialization of Cellular Service in RTOS mode
  * @param  none
  * @retval CS_Status_t
  */
CS_Status_t CS_init(void)
{
  CS_Status_t retval;

  /* initialization of cellular context */
  retval = CELLULAR_init();

  /* check error code before to leave */
  if (retval == CS_OK)
  {
    /* request AT core to start */
    if (atcore_task_start(ATCORE_THREAD_STACK_PRIO, ATCORE_THREAD_STACK_SIZE) != ATSTATUS_OK)
    {
      /* at core start fails */
      retval = CS_ERROR;
    }
  }

  return (retval);
}

/**
  * @brief  Power ON the modem
  * @param  none
  * @retval CS_Status_t
  */
CS_Status_t CS_power_on(void)
{
  CS_Status_t retval = CS_ERROR;

  /* 1st step: power on modem by triggering GPIOs */
  if (SysCtrl_power_on(DEVTYPE_MODEM_CELLULAR) == SCSTATUS_OK)
  {
    /* 2nd step: open UART channel */
    if (SysCtrl_open_channel(DEVTYPE_MODEM_CELLULAR) == SCSTATUS_OK)
    {
      /* 3rd step: open AT channel (IPC) */
      if (AT_open_channel(get_Adapter_Handle()) == ATSTATUS_OK)
      {

        /* 4th step: send AT commands sequence to setup modem after power on */
        /* Prepare the SID message to send to ATCore  */
        if (DATAPACK_writeStruct(getCmdBufPtr(),
                                 (uint16_t) CSMT_NONE,
                                 (uint16_t) 0U,
                                 NULL) == DATAPACK_OK)
        {
          at_status_t err;

          /* Send the SID message to ATCore */
          err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_POWER_ON, getCmdBufPtr(), getRspBufPtr());
          if (err == ATSTATUS_OK)
          {
            PRINT_DBG("Cellular started and ready")
            retval = CS_OK;
          }
        }
      }
    }
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when power on process")
  }
  return (retval);
}

/**
  * @brief  Power OFF the modem
  * @param  none
  * @retval CS_Status_t
  */
CS_Status_t CS_power_off(void)
{
  CS_Status_t retval = CS_ERROR;

  /* reset Cellular Service context */
  CELLULAR_reset_context();

  /* update sockets states */
  csint_modem_reset_update_socket_state();

  /* Prepare the SID message to send to ATCore  */
  if (DATAPACK_writeStruct(getCmdBufPtr(),
                           (uint16_t) CSMT_NONE,
                           (uint16_t) 0U,
                           NULL) == DATAPACK_OK)
  {
    /* In case of Power Off request, ignore errors to be sure to finish by hardware power off */

    /* 1st step: send AT commands sequence for power off (if exist for this modem) */
    /* Send the SID message to ATCore */
    (void) AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_POWER_OFF, getCmdBufPtr(), getRspBufPtr());

    /* 2nd step: close AT channel (IPC) */
    (void) AT_close_channel(get_Adapter_Handle());

    /* 3rd step: close UART channel */
    (void) SysCtrl_close_channel(DEVTYPE_MODEM_CELLULAR);

    /* 4th step: power off modem by triggering GPIOs  */
    if (SysCtrl_power_off(DEVTYPE_MODEM_CELLULAR) == SCSTATUS_OK)
    {
      /* Cellular_Service> Stopped */
      retval = CS_OK;
    }
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error during power off process")
  }
  return (retval);
}

/**
  * @brief  Check that modem connection is successfully established
  * @note   Usually, the command AT is sent and OK is expected as response
  * @param  none
  * @retval CS_Status_t
  */
CS_Status_t CS_check_connection(void)
{
  CS_Status_t retval = CS_ERROR;

  /* Prepare the SID message to send to ATCore  */
  if (DATAPACK_writeStruct(getCmdBufPtr(),
                           (uint16_t) CSMT_NONE,
                           (uint16_t) 0U,
                           NULL) == DATAPACK_OK)
  {
    at_status_t err;

    /* Send the SID message to ATCore */
    err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_CHECK_CNX, getCmdBufPtr(), getRspBufPtr());
    if (err == ATSTATUS_OK)
    {
      /* Cellular_Service> Modem connection OK */
      retval = CS_OK;
    }
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error with modem connection")
  }
  return (retval);
}

/**
  * @brief  Select SIM slot to use.
  * @note   Only one SIM slot is active at a time.
  * @note   IMPORTANT !!! if the modem adaptation layer decides that a modem reboot
  * @note   is required (because the SIM slot has changed for example), this
  * @note   function will return an error to upper layer. This error means that
  * @note   a modem reboot is needed.
  * @param  simSelected Selected SIM slot
  * @retval CS_Status_t
  */
CS_Status_t CS_sim_select(CS_SimSlot_t simSelected)
{
  CS_Status_t retval = CS_ERROR;

  /* Prepare the SID message to send to ATCore  */
  if (DATAPACK_writeStruct(getCmdBufPtr(),
                           (uint16_t) CSMT_SIM_SELECT,
                           (uint16_t) sizeof(CS_SimSlot_t),
                           (void *)&simSelected) == DATAPACK_OK)
  {
    at_status_t err;

    /* Send the SID message to ATCore */
    err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_SIM_SELECT, getCmdBufPtr(), getRspBufPtr());
    if (err == ATSTATUS_OK)
    {
      PRINT_DBG("<Cellular_Service> SIM %d selected", simSelected)
      retval = CS_OK;
    }
    else
    {
      retval = CELLULAR_analyze_error_report(getRspBufPtr());
    }
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_INFO("<Cellular_Service> SIM selection need a modem restart to be applied")
  }
  return (retval);
}


/**
  * @brief  Send a SIM generic command to the modem
  * @param  sim_generic_access pointer on different buffers:
  *         command to send, response received
  *         size in bytes for all these buffers
  * @retval int32_t error or size in bytes of the response
  */
int32_t CS_sim_generic_access(CS_sim_generic_access_t *sim_generic_access)
{
  int32_t returned_data_size;
  CS_Status_t status = CS_ERROR;

  /* Initialize the local data structure used to create SID message */
  csint_sim_generic_access_t sim_generic_access_data;
  (void) memset((void *)&sim_generic_access_data, 0, sizeof(sim_generic_access_data));

  /* check buffer are not NULL and size does not exceed maximum/minimum buffers size */
  if ((sim_generic_access->p_cmd_str != NULL)
      && (sim_generic_access->p_rsp_str != NULL)
      && (sim_generic_access->cmd_str_size <= CONFIG_MODEM_MAX_SIM_GENERIC_ACCESS_CMD_SIZE)
      && (sim_generic_access->rsp_str_size >= CONFIG_MODEM_MIN_SIM_GENERIC_ACCESS_RSP_SIZE))
  {
    sim_generic_access_data.data = sim_generic_access;
    /* code commented to avoid Code Sonar error (variable already initialized with same value):
     *  sim_generic_access_data.bytes_received = 0U
     */

    /* Prepare the SID message to send to ATCore  */
    if (DATAPACK_writeStruct(getCmdBufPtr(),
                             (uint16_t) CSMT_SIM_GENERIC_ACCESS,
                             (uint16_t) sizeof(csint_sim_generic_access_t),
                             (void *)&sim_generic_access_data) == DATAPACK_OK)
    {
      at_status_t err;

      /* Send the SID message to ATCore */
      err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_SIM_GENERIC_ACCESS, getCmdBufPtr(), getRspBufPtr());
      if (err == ATSTATUS_OK)
      {
        /* Read response from ATCore  */
        if (DATAPACK_readStruct(getRspBufPtr(),
                                (uint16_t) CSMT_SIM_GENERIC_ACCESS,
                                (uint16_t) sizeof(csint_sim_generic_access_t),
                                &sim_generic_access_data) == DATAPACK_OK)
        {
          status = CS_OK;
        }
      }
    }
  }
  else
  {
    PRINT_ERR("SIM generic access Buffer NULL, CMD size too big=%ld/%ld or RSP size too small=%ld/%ld)",
              sim_generic_access->cmd_str_size,
              CONFIG_MODEM_MAX_SIM_GENERIC_ACCESS_CMD_SIZE,
              sim_generic_access->rsp_str_size,
              CONFIG_MODEM_MIN_SIM_GENERIC_ACCESS_RSP_SIZE)
  }

  /* check status returned from ATCore and set returned_data_size value */
  if (status == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when sending SIM generic access")
    returned_data_size = -1;
  }
  else
  {
    PRINT_INFO("Size of response received = %ld bytes", sim_generic_access_data.bytes_received)
    returned_data_size = ((int32_t)sim_generic_access_data.bytes_received);
  }

  return (returned_data_size);
}

/**
  * @brief  Initialize the service and configures the Modem FW functionalities
  * @note   Used to provide PIN code (if any) and modem function level.
  * @param  init Function level (MINI, FULL, SIM only).
  * @param  reset Indicates if modem reset will be applied or not.
  * @param  pin_code PIN code string.
  *
  * @retval CS_Status_t
  */
CS_Status_t CS_init_modem(CS_ModemInit_t init, CS_Bool_t reset, const CS_CHAR_t *pin_code)
{
  CS_Status_t retval = CS_ERROR;

  /* Initialize the local data structure used to create SID message */
  csint_modemInit_t modemInit_struct;
  (void) memset((void *)&modemInit_struct, 0, sizeof(modemInit_struct));
  modemInit_struct.init = init;
  modemInit_struct.reset = reset;
  (void) memcpy((CS_CHAR_t *)&modemInit_struct.pincode.pincode[0],
                (const CS_CHAR_t *)pin_code,
                strlen((const CRC_CHAR_t *)pin_code));

  /* Prepare the SID message to send to ATCore  */
  if (DATAPACK_writeStruct(getCmdBufPtr(),
                           (uint16_t) CSMT_INITMODEM,
                           (uint16_t) sizeof(csint_modemInit_t),
                           (void *)&modemInit_struct) == DATAPACK_OK)
  {
    at_status_t err;

    /* Send the SID message to ATCore */
    err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_INIT_MODEM, getCmdBufPtr(), getRspBufPtr());
    if (err == ATSTATUS_OK)
    {
      /* <Cellular_Service> Init done successfully */
      retval = CS_OK;
    }
    else
    {
      retval = CELLULAR_analyze_error_report(getRspBufPtr());
    }
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error during init")
  }
  return (retval);
}

/**
  * @brief  Return information related to modem status.
  * @param  p_devinfo Handle on modem information structure.
  * @retval CS_Status_t
  */
CS_Status_t CS_get_device_info(CS_DeviceInfo_t *p_devinfo)
{
  /* static structure used to send data to lower layer */
  static CS_DeviceInfo_t cs_ctxt_device_info = {0};

  CS_Status_t retval = CS_ERROR;

  /* reset our local copy */
  (void) memset((void *)&cs_ctxt_device_info, 0, sizeof(cs_ctxt_device_info));
  cs_ctxt_device_info.field_requested = p_devinfo->field_requested;

  /* Prepare the SID message to send to ATCore  */
  if (DATAPACK_writePtr(getCmdBufPtr(),
                        (uint16_t) CSMT_DEVICE_INFO,
                        (void *)&cs_ctxt_device_info) == DATAPACK_OK)
  {
    at_status_t err;

    /* Send the SID message to ATCore */
    err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_GET_DEVICE_INFO, getCmdBufPtr(), getRspBufPtr());
    if (err == ATSTATUS_OK)
    {
      /* <Cellular_Service> Device infos received */
      /* send info to user */
      (void) memcpy((void *)p_devinfo, (void *)&cs_ctxt_device_info, sizeof(CS_DeviceInfo_t));
      retval = CS_OK;
    }
    else
    {
      retval = CELLULAR_analyze_error_report(getRspBufPtr());
    }
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when getting device infos")
  }
  return (retval);
}

/**
  * @brief  Request the Modem to register to the Cellular Network.
  * @note   This function is used to select the operator. It returns a detailed
  *         network registration status.
  * @param  p_devinfo Handle on operator information structure.
  * @param  p_reg_status Handle on registration information structure.
  *         This information is valid only if return code is CS_OK
  * @retval CS_Status_t
  */
CS_Status_t CS_register_net(CS_OperatorSelector_t *p_operator,
                            CS_RegistrationStatus_t *p_reg_status)
{
  CS_Status_t retval = CS_ERROR;

  /* Initialize the local data structure used to create SID message */
  static CS_OperatorSelector_t cs_ctxt_operator;

  /* init returned fields */
  p_reg_status->optional_fields_presence = CS_RSF_NONE;
  p_reg_status->CS_NetworkRegState = CS_NRS_UNKNOWN;
  p_reg_status->GPRS_NetworkRegState = CS_NRS_UNKNOWN;
  p_reg_status->EPS_NetworkRegState = CS_NRS_UNKNOWN;

  (void) memcpy((void *)&cs_ctxt_operator, (void *)p_operator, sizeof(CS_OperatorSelector_t));
  /* Prepare the SID message to send to ATCore  */
  if (DATAPACK_writeStruct(getCmdBufPtr(),
                           (uint16_t) CSMT_OPERATORSELECT,
                           (uint16_t) sizeof(CS_OperatorSelector_t),
                           (void *)&cs_ctxt_operator) == DATAPACK_OK)
  {
    at_status_t err;

    /* Send the SID message to ATCore */
    err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_REGISTER_NET, getCmdBufPtr(), getRspBufPtr());
    if (err == ATSTATUS_OK)
    {
      /* Read response from ATCore  */
      if (DATAPACK_readStruct(getRspBufPtr(),
                              (uint16_t) CSMT_REGISTRATIONSTATUS,
                              (uint16_t) sizeof(CS_RegistrationStatus_t),
                              p_reg_status) == DATAPACK_OK)
      {
        /* <Cellular_Service> Network registration done */
        retval = CS_OK;
      }
    }
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error during network registration ")
  }
  return (retval);
}

/**
  * @brief  Request the Modem to deregister (manually) from the Cellular Network.
  * @retval CS_Status_t
  */
CS_Status_t CS_deregister_net(CS_RegistrationStatus_t *p_reg_status)
{
  CS_Status_t retval = CS_ERROR;

  /* initialize static structure used to send data to lower layer.
   *  Request to deregister from network : mode is set to CS_NRM_DEREGISTER.
   *  Other parameters are not needed by lower layer.
   */
  static CS_OperatorSelector_t cs_ctxt_operator =
  {
    .mode = CS_NRM_DEREGISTER,
    .format = CS_ONF_NOT_PRESENT,
    .AcT_present = CS_FALSE,
  };

  /* init returned fields */
  p_reg_status->optional_fields_presence = CS_RSF_NONE;
  p_reg_status->CS_NetworkRegState = CS_NRS_UNKNOWN;
  p_reg_status->GPRS_NetworkRegState = CS_NRS_UNKNOWN;
  p_reg_status->EPS_NetworkRegState = CS_NRS_UNKNOWN;

  /* Prepare the SID message to send to ATCore  */
  if (DATAPACK_writeStruct(getCmdBufPtr(),
                           (uint16_t) CSMT_OPERATORSELECT,
                           (uint16_t) sizeof(CS_OperatorSelector_t),
                           (void *)&cs_ctxt_operator) == DATAPACK_OK)
  {
    at_status_t err;

    /* Send the SID message to ATCore */
    err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_REGISTER_NET, getCmdBufPtr(), getRspBufPtr());
    if (err == ATSTATUS_OK)
    {
      /* Read response from ATCore  */
      if (DATAPACK_readStruct(getRspBufPtr(),
                              (uint16_t) CSMT_REGISTRATIONSTATUS,
                              (uint16_t) sizeof(CS_RegistrationStatus_t),
                              p_reg_status) == DATAPACK_OK)
      {
        /* <Cellular_Service> Network deregistration done */
        retval = CS_OK;
      }
    }
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error during network deregistration ")
  }
  return (retval);
}

/**
  * @brief  Register to an event change notification related to Network status.
  * @note   This function should be called for each event the user wants to start monitoring.
  * @param  event Event that will be registered to change notification.
  * @param  urc_callback Handle on user callback that will be used to notify a
  *                      change on requested event.
  * @retval CS_Status_t
  */
CS_Status_t CS_subscribe_net_event(CS_UrcEvent_t event, cellular_urc_callback_t urc_callback)
{
  CS_Status_t retval = CS_OK;

  /* URC registration */
  if (event == CS_URCEVENT_EPS_NETWORK_REG_STAT)
  {
    /* subscribe EPS network registration callback */
    urc_eps_network_registration_callback = urc_callback;
    cs_ctxt_urc_subscription.eps_network_registration = CS_TRUE;
  }
  else if (event == CS_URCEVENT_GPRS_NETWORK_REG_STAT)
  {
    /* subscribe GPRS network registration callback */
    urc_gprs_network_registration_callback = urc_callback;
    cs_ctxt_urc_subscription.gprs_network_registration = CS_TRUE;
  }
  else if (event == CS_URCEVENT_CS_NETWORK_REG_STAT)
  {
    /* subscribe CS network registration callback */
    urc_cs_network_registration_callback = urc_callback;
    cs_ctxt_urc_subscription.cs_network_registration = CS_TRUE;
  }
  else if (event == CS_URCEVENT_EPS_LOCATION_INFO)
  {
    /* subscribe EPS location information callback */
    urc_eps_location_info_callback = urc_callback;
    cs_ctxt_urc_subscription.eps_location_info = CS_TRUE;
  }
  else if (event == CS_URCEVENT_GPRS_LOCATION_INFO)
  {
    /* subscribe GPRS location information callback */
    urc_gprs_location_info_callback = urc_callback;
    cs_ctxt_urc_subscription.gprs_location_info = CS_TRUE;
  }
  else if (event == CS_URCEVENT_CS_LOCATION_INFO)
  {
    /* subscribe CS location information callback */
    urc_cs_location_info_callback = urc_callback;
    cs_ctxt_urc_subscription.cs_location_info = CS_TRUE;
  }
  else if (event == CS_URCEVENT_SIGNAL_QUALITY)
  {
    /* subscribe Signal quality callback */
    urc_signal_quality_callback = urc_callback;
    cs_ctxt_urc_subscription.signal_quality = CS_TRUE;
  }
  else
  {
    /* invalid event */
    PRINT_ERR("<Cellular_Service> invalid event")
    retval = CS_ERROR;
  }

  /* check error code before to leave */
  if (retval != CS_ERROR)
  {
    /* Prepare the SID message to send to ATCore  */
    if (DATAPACK_writeStruct(getCmdBufPtr(),
                             (uint16_t) CSMT_URC_EVENT,
                             (uint16_t) sizeof(CS_UrcEvent_t),
                             (void *)&event) == DATAPACK_OK)
    {
      at_status_t err;

      /* Send the SID message to ATCore */
      err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_SUSBCRIBE_NET_EVENT, getCmdBufPtr(), getRspBufPtr());
      if (err != ATSTATUS_OK)
      {
        retval = CS_ERROR;
      }
    }
    else
    {
      retval = CS_ERROR;
    }
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when subscribing event")
  }
  return (retval);
}

/**
  * @brief  Deregister to an event change notification related to Network status.
  * @note   This function should be called for each event the user wants to stop monitoring.
  * @param  event Event that will be deregistered to change notification.
  * @retval CS_Status_t
  */
CS_Status_t CS_unsubscribe_net_event(CS_UrcEvent_t event)
{
  CS_Status_t retval = CS_OK;

  if (event == CS_URCEVENT_EPS_NETWORK_REG_STAT)
  {
    /* unsubscribe EPS network registration callback */
    urc_eps_network_registration_callback = NULL;
    cs_ctxt_urc_subscription.eps_network_registration = CS_FALSE;
  }
  else if (event == CS_URCEVENT_GPRS_NETWORK_REG_STAT)
  {
    /* unsubscribe GPRS network registration callback */
    urc_gprs_network_registration_callback = NULL;
    cs_ctxt_urc_subscription.gprs_network_registration = CS_FALSE;
  }
  else if (event == CS_URCEVENT_CS_NETWORK_REG_STAT)
  {
    /* unsubscribe CS network registration callback */
    urc_cs_network_registration_callback = NULL;
    cs_ctxt_urc_subscription.cs_network_registration = CS_FALSE;
  }
  else if (event == CS_URCEVENT_EPS_LOCATION_INFO)
  {
    /* unsubscribe EPS location information callback */
    urc_eps_location_info_callback = NULL;
    cs_ctxt_urc_subscription.eps_location_info = CS_FALSE;
  }
  else if (event == CS_URCEVENT_GPRS_LOCATION_INFO)
  {
    /* unsubscribe GPRS location information callback */
    urc_gprs_location_info_callback = NULL;
    cs_ctxt_urc_subscription.gprs_location_info = CS_FALSE;
  }
  else if (event == CS_URCEVENT_CS_LOCATION_INFO)
  {
    /* unsubscribe CS location information callback */
    urc_cs_location_info_callback = NULL;
    cs_ctxt_urc_subscription.cs_location_info = CS_FALSE;
  }
  else if (event == CS_URCEVENT_SIGNAL_QUALITY)
  {
    /* unsubscribe signal quality callback */
    urc_signal_quality_callback = NULL;
    cs_ctxt_urc_subscription.signal_quality = CS_FALSE;
  }
  else
  {
    /* invalid event */
    PRINT_ERR("<Cellular_Service> invalid event")
    retval = CS_ERROR;
  }

  /* check error code before to leave */
  if (retval != CS_ERROR)
  {
    /* Prepare the SID message to send to ATCore  */
    if (DATAPACK_writeStruct(getCmdBufPtr(),
                             (uint16_t) CSMT_URC_EVENT,
                             (uint16_t) sizeof(CS_UrcEvent_t),
                             (void *)&event) == DATAPACK_OK)
    {
      at_status_t err;

      /* Send the SID message to ATCore */
      err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_UNSUSBCRIBE_NET_EVENT, getCmdBufPtr(), getRspBufPtr());
      if (err != ATSTATUS_OK)
      {
        retval = CS_ERROR;
      }
    }
    else
    {
      retval = CS_ERROR;
    }
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when unsubscribing event")
  }
  return (retval);
}

/**
  * @brief  Register to event change notifications related to SIM status.
  * @note   When this function is called, SIM events will be reported through the callback.
  * @note   Events are modem dependent: some events may be not reported by all modems.
  * @param  sim_evt_callback Handle on user callback that will be used to notify a change on SIM event.
  * @retval CS_Status_t
  */
CS_Status_t CS_subscribe_sim_event(cellular_sim_event_callback_t sim_evt_callback)
{
  /* Save the callback pointer.
   * No message is sent to AT layer. URC will be reported by AT layer (from modem)
   * and will be forwarded to client if a valid callback pointer exists.
   */
  urc_sim_event_callback = sim_evt_callback;

  return (CS_OK);
}

/**
  * @brief  Request attach to packet domain.
  * @param  none.
  * @retval CS_Status_t
  */
CS_Status_t CS_attach_PS_domain(void)
{
  CS_Status_t retval = CS_ERROR;

  /* Prepare the SID message to send to ATCore  */
  if (DATAPACK_writeStruct(getCmdBufPtr(),
                           (uint16_t) CSMT_NONE,
                           (uint16_t) 0U,
                           NULL) == DATAPACK_OK)
  {
    at_status_t err;

    /* Send the SID message to ATCore */
    err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_ATTACH_PS_DOMAIN, getCmdBufPtr(), getRspBufPtr());
    if (err == ATSTATUS_OK)
    {
      /* <Cellular_Service> attach PS domain done */
      retval = CS_OK;
    }
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when attaching PS domain")
  }
  return (retval);
}

/**
  * @brief  Request detach from packet domain.
  * @param  none.
  * @retval CS_Status_t
  */
CS_Status_t CS_detach_PS_domain(void)
{
  CS_Status_t retval = CS_ERROR;

  /* Prepare the SID message to send to ATCore  */
  if (DATAPACK_writeStruct(getCmdBufPtr(),
                           (uint16_t) CSMT_NONE,
                           (uint16_t) 0U,
                           NULL) == DATAPACK_OK)
  {
    at_status_t err;

    /* Send the SID message to ATCore */
    err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_DETACH_PS_DOMAIN, getCmdBufPtr(), getRspBufPtr());
    if (err == ATSTATUS_OK)
    {
      /* <Cellular_Service> detach PS domain done */
      retval = CS_OK;
    }
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when detaching PS domain")
  }
  return (retval);
}

/**
  * @brief  Request for packet attach status.
  * @param  p_attach Handle to PS attach status.
  * @retval CS_Status_t
  */
CS_Status_t CS_get_attach_status(CS_PSattach_t *p_attach)
{
  CS_Status_t retval = CS_ERROR;

  /* Prepare the SID message to send to ATCore  */
  if (DATAPACK_writeStruct(getCmdBufPtr(),
                           (uint16_t) CSMT_NONE,
                           (uint16_t) 0U,
                           NULL) == DATAPACK_OK)
  {
    at_status_t err;

    /* Send the SID message to ATCore */
    err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_GET_ATTACHSTATUS, getCmdBufPtr(), getRspBufPtr());
    if (err == ATSTATUS_OK)
    {
      /* Read response from ATCore  */
      if (DATAPACK_readStruct(getRspBufPtr(),
                              (uint16_t) CSMT_ATTACHSTATUS,
                              (uint16_t) sizeof(CS_PSattach_t),
                              p_attach) == DATAPACK_OK)
      {
        /* Cellular_Service> Attachment status received */
        PRINT_DBG("<Cellular_Service> Attachment status received = %d", *p_attach)
        retval = CS_OK;
      }
    }
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when getting attachment status")
  }
  return (retval);
}

/**
  * @brief  Read the latest registration state to the Cellular Network.
  * @param  p_reg_status Handle to registration status structure.
  *         This information is valid only if return code is CS_OK
  * @retval CS_Status_t
  */
CS_Status_t CS_get_net_status(CS_RegistrationStatus_t *p_reg_status)
{
  CS_Status_t retval = CS_ERROR;

  /* init returned fields */
  p_reg_status->optional_fields_presence = CS_RSF_NONE;
  p_reg_status->CS_NetworkRegState = CS_NRS_UNKNOWN;
  p_reg_status->GPRS_NetworkRegState = CS_NRS_UNKNOWN;
  p_reg_status->EPS_NetworkRegState = CS_NRS_UNKNOWN;

  /* Prepare the SID message to send to ATCore  */
  if (DATAPACK_writeStruct(getCmdBufPtr(),
                           (uint16_t) CSMT_NONE,
                           (uint16_t) 0U,
                           NULL) == DATAPACK_OK)
  {
    at_status_t err;

    /* Send the SID message to ATCore */
    err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_GET_NETSTATUS, getCmdBufPtr(), getRspBufPtr());
    if (err == ATSTATUS_OK)
    {
      /* Read response from ATCore  */
      if (DATAPACK_readStruct(getRspBufPtr(),
                              (uint16_t) CSMT_REGISTRATIONSTATUS,
                              (uint16_t) sizeof(CS_RegistrationStatus_t),
                              p_reg_status) == DATAPACK_OK)
      {
        /* <Cellular_Service> Net status received */
        retval = CS_OK;
      }
    }
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when getting net status")
  }
  return (retval);
}

/**
  * @brief  Read the actual signal quality seen by Modem .
  * @param  p_sig_qual Handle to signal quality structure.
  * @retval CS_Status_t
  */
CS_Status_t CS_get_signal_quality(CS_SignalQuality_t *p_sig_qual)
{
  CS_Status_t retval = CS_ERROR;

  /* Initialize the local data structure used to create SID message */
  CS_SignalQuality_t local_sig_qual = {0};
  (void) memset((void *)&local_sig_qual, 0, sizeof(CS_SignalQuality_t));

  /* Prepare the SID message to send to ATCore  */
  if (DATAPACK_writePtr(getCmdBufPtr(),
                        (uint16_t) CSMT_SIGNAL_QUALITY,
                        (void *)&local_sig_qual) == DATAPACK_OK)
  {
    at_status_t err;

    /* Send the SID message to ATCore */
    err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_GET_SIGNAL_QUALITY, getCmdBufPtr(), getRspBufPtr());
    if (err == ATSTATUS_OK)
    {
      /* <Cellular_Service> Signal quality information received */
      /* recopy info to user */
      (void) memcpy((void *)p_sig_qual, (void *)&local_sig_qual, sizeof(CS_SignalQuality_t));
      retval = CS_OK;
    }
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when getting signal quality")
  }
  return (retval);
}

/**
  * @brief  Activates a PDN (Packet Data Network Gateway) allowing communication with internet.
  * @note   This function triggers the allocation of IP public WAN to the device.
  * @note   Only one PDN can be activated at a time.
  * @param  cid Configuration identifier
  *         This parameter can be one of the following values:
  *         CS_PDN_PREDEF_CONFIG To use default PDN configuration.
  *         CS_PDN_USER_CONFIG_1-5 To use a dedicated PDN configuration.
  *         CS_PDN_CONFIG_DEFAULT To use default PDN config set by CS_set_default_pdn().
  * @retval CS_Status_t
  */
CS_Status_t CS_activate_pdn(CS_PDN_conf_id_t cid)
{
  CS_Status_t retval = CS_ERROR;

  /* Prepare the SID message to send to ATCore  */
  if (DATAPACK_writeStruct(getCmdBufPtr(),
                           (uint16_t) CSMT_ACTIVATE_PDN,
                           (uint16_t) sizeof(CS_PDN_conf_id_t),
                           (void *)&cid) == DATAPACK_OK)
  {
    at_status_t err;

    /* Send the SID message to ATCore */
    err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_ACTIVATE_PDN, getCmdBufPtr(), getRspBufPtr());
    if (err == ATSTATUS_OK)
    {
      /* <Cellular_Service> PDN connected */
      PRINT_DBG("<Cellular_Service> PDN %d connected", cid)
      retval = CS_OK;
    }
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when PDN %d cid activation", cid)
  }
  return (retval);
}

/**
  * @brief  Deactivates a PDN.
  * @note   This function triggers the allocation of IP public WAN to the device.
  * @note  only one PDN can be activated at a time.
  * @param  cid Configuration identifier
  * @retval CS_Status_t
  */
CS_Status_t CS_deactivate_pdn(CS_PDN_conf_id_t cid)
{
  CS_Status_t retval = CS_ERROR;

  /* Prepare the SID message to send to ATCore  */
  if (DATAPACK_writeStruct(getCmdBufPtr(),
                           (uint16_t) CSMT_DEACTIVATE_PDN,
                           (uint16_t) sizeof(CS_PDN_conf_id_t),
                           (void *)&cid) == DATAPACK_OK)
  {
    at_status_t err;

    /* Send the SID message to ATCore */
    err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_DEACTIVATE_PDN, getCmdBufPtr(), getRspBufPtr());
    if (err == ATSTATUS_OK)
    {
      /* <Cellular_Service> PDN deactivated */
      PRINT_DBG("<Cellular_Service> PDN %d deactivated", cid)
      retval = CS_OK;
    }
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when PDN %d cid deactivation", cid)
  }
  return (retval);
}

/**
  * @brief  Define internet data profile for a configuration identifier
  * @param  cid Configuration identifier
  * @param  apn A string of the access point name. If NULL is passed, lower layers will use, if possible, a modem
  *             specific value in order to use APN value defined by the network side.
  * @param  pdn_conf Structure which contains additional configurations parameters (if non NULL)
  * @retval CS_Status_t
  */
CS_Status_t CS_define_pdn(CS_PDN_conf_id_t cid, const CS_CHAR_t *apn, CS_PDN_configuration_t *pdn_conf)
{
  CS_Status_t retval = CS_ERROR;

  /* Initialize the local data structure used to create SID message */
  csint_pdn_infos_t pdn_infos;

  /* check parameters validity */
  if ((cid < CS_PDN_USER_CONFIG_1) || (cid > CS_PDN_USER_CONFIG_5))
  {
    PRINT_ERR("<Cellular_Service> selected configuration id %d can not be set by user", cid)
  }
  else if (pdn_conf == NULL)
  {
    PRINT_ERR("<Cellular_Service> pdn_conf must be non NULL")
  }
  else if (apn == NULL)
  {
    /* prepare and send PDN infos, without apn */
    pdn_infos.conf_id = cid;
    pdn_infos.apn_present = CS_FALSE;
    (void) memset((void *)&pdn_infos.apn, 0, MAX_APN_SIZE);
    (void) memcpy((void *)&pdn_infos.pdn_conf, (void *)pdn_conf, sizeof(CS_PDN_configuration_t));

    /* Prepare the SID message to send to ATCore  */
    if (DATAPACK_writePtr(getCmdBufPtr(),
                          (uint16_t) CSMT_DEFINE_PDN,
                          (void *)&pdn_infos) == DATAPACK_OK)
    {
      at_status_t err;

      /* Send the SID message to ATCore */
      err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_DEFINE_PDN, getCmdBufPtr(), getRspBufPtr());
      if (err == ATSTATUS_OK)
      {
        retval = CS_OK;
      }
    }
  }
  else
  {
    /* prepare and send PDN infos, with apn */
    (void) memset((void *)&pdn_infos, 0, sizeof(csint_pdn_infos_t));
    pdn_infos.conf_id = cid;
    pdn_infos.apn_present = CS_TRUE;
    (void) memcpy((CS_CHAR_t *)&pdn_infos.apn[0],
                  (const CS_CHAR_t *)apn,
                  strlen((const CRC_CHAR_t *)apn));
    (void) memcpy((void *)&pdn_infos.pdn_conf, (void *)pdn_conf, sizeof(CS_PDN_configuration_t));

    /* Prepare the SID message to send to ATCore  */
    if (DATAPACK_writePtr(getCmdBufPtr(),
                          (uint16_t) CSMT_DEFINE_PDN,
                          (void *)&pdn_infos) == DATAPACK_OK)
    {
      at_status_t err;

      /* Send the SID message to ATCore */
      err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_DEFINE_PDN, getCmdBufPtr(), getRspBufPtr());
      if (err == ATSTATUS_OK)
      {
        retval = CS_OK;
      }
    }
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when defining PDN %d", cid)
  }
  return (retval);
}

/**
  * @brief  Select a PDN among of defined configuration identifier(s) as the default.
  * @note   By default, PDN_PREDEF_CONFIG is considered as the default PDN.
  * @param  cid Configuration identifier.
  * @retval CS_Status_t
  */
CS_Status_t CS_set_default_pdn(CS_PDN_conf_id_t cid)
{
  CS_Status_t retval = CS_ERROR;

  /* Prepare the SID message to send to ATCore  */
  if (DATAPACK_writeStruct(getCmdBufPtr(),
                           (uint16_t) CSMT_SET_DEFAULT_PDN,
                           (uint16_t) sizeof(CS_PDN_conf_id_t),
                           (void *)&cid) == DATAPACK_OK)
  {
    at_status_t err;

    /* Send the SID message to ATCore */
    err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_SET_DEFAULT_PDN, getCmdBufPtr(), getRspBufPtr());
    if (err == ATSTATUS_OK)
    {
      /* <Cellular_Service> set default PDN */
      PRINT_DBG("<Cellular_Service> PDN %d set as default", cid)
      retval = CS_OK;
    }
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when setting default PDN %d", cid)
  }
  return (retval);
}

/**
  * @brief  Get the IP address allocated to the device for a given PDN.
  * @param  cid Configuration identifier number.
  * @param  ip_addr_type IP address type and format.
  * @param  p_ip_addr_value Specifies the IP address of the given PDN (max size = MAX_SIZE_IPADDR), without quotes.
  * @retval CS_Status_t
  */
CS_Status_t CS_get_dev_IP_address(CS_PDN_conf_id_t cid, CS_IPaddrType_t *ip_addr_type, CS_CHAR_t *p_ip_addr_value)
{
  CS_Status_t retval = CS_ERROR;

  /* Prepare the SID message to send to ATCore  */
  if (DATAPACK_writeStruct(getCmdBufPtr(),
                           (uint16_t) CSMT_GET_IP_ADDRESS,
                           (uint16_t) sizeof(CS_PDN_conf_id_t),
                           (void *)&cid) == DATAPACK_OK)
  {
    at_status_t err;

    /* Send the SID message to ATCore */
    err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_GET_IP_ADDRESS, getCmdBufPtr(), getRspBufPtr());
    if (err == ATSTATUS_OK)
    {
      csint_ip_addr_info_t ip_addr_info;
      /* Read response from ATCore */
      if (DATAPACK_readStruct(getRspBufPtr(),
                              (uint16_t) CSMT_GET_IP_ADDRESS,
                              (uint16_t) sizeof(csint_ip_addr_info_t),
                              &ip_addr_info) == DATAPACK_OK)
      {
        /* <Cellular_Service> IP address information received */
        /* recopy info to user
         * try to remove quotes, if any, around IP address
         */
        csint_ip_addr_info_t tmp_ip_addr_info;
        uint16_t tmp_ip_addr_info_size;

        tmp_ip_addr_info_size =
          ATutil_extract_str_from_quotes(
            (const uint8_t *)ip_addr_info.ip_addr_value,
            (uint16_t) strlen((CRC_CHAR_t *)ip_addr_info.ip_addr_value),
            tmp_ip_addr_info.ip_addr_value,
            MAX_SIZE_IPADDR);

        /* retrieve IP address value */
        if (tmp_ip_addr_info_size != 0U)
        {
          /* quotes have been removed, recopy cleaned IP address */
          (void) memcpy((void *)p_ip_addr_value,
                        (const void *)&tmp_ip_addr_info.ip_addr_value,
                        (size_t) tmp_ip_addr_info_size);
        }
        else
        {
          /* no quotes detected, recopy received field without any modification */
          (void) memcpy((void *)p_ip_addr_value,
                        (const void *)&ip_addr_info.ip_addr_value,
                        (size_t) strlen((CRC_CHAR_t *)ip_addr_info.ip_addr_value));
        }

        *ip_addr_type = ip_addr_info.ip_addr_type;

        PRINT_DBG("<Cellular_Service> IP address = %s (type = %d)",
                  (CS_CHAR_t *)ip_addr_info.ip_addr_value, ip_addr_info.ip_addr_type)
        retval = CS_OK;
      }
    }
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when getting IP address information")
  }
  return (retval);
}

/**
  * @brief  Register to specified modem events.
  * @note   This function should be called once with all requested events.
  * @param  events_mask Events that will be registered (bitmask)
  * @param  urc_callback Handle on user callback that will be used to notify a
  *         change on requested event.
  * @retval CS_Status_t
  */
CS_Status_t CS_subscribe_modem_event(CS_ModemEvent_t events_mask, cellular_modem_event_callback_t modem_evt_cb)
{
  CS_Status_t retval = CS_ERROR;

  /* Prepare the SID message to send to ATCore  */
  if (DATAPACK_writeStruct(getCmdBufPtr(),
                           (uint16_t) CSMT_MODEM_EVENT,
                           (uint16_t) sizeof(CS_ModemEvent_t),
                           (void *)&events_mask) == DATAPACK_OK)
  {
    at_status_t err;

    /* Send the SID message to ATCore */
    err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_SUSBCRIBE_MODEM_EVENT, getCmdBufPtr(), getRspBufPtr());
    if (err == ATSTATUS_OK)
    {
      urc_modem_event_callback = modem_evt_cb;
      /* <Cellular_Service> modem events subscribed */
      retval = CS_OK;
    }
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when subscribing modem event")
  }
  return (retval);
}

/**
  * @brief  Register to event notifications related to internet connection.
  * @note   This function is used to register to an event related to a PDN
  *         Only explicit config id (CS_PDN_USER_CONFIG_1 to CS_PDN_USER_CONFIG_5) are
  *         supported and CS_PDN_PREDEF_CONFIG
  * @param  cid Configuration identifier number.
  * @param  pdn_event_callback client callback to call when an event occurred.
  * @retval CS_Status_t
  */
CS_Status_t  CS_register_pdn_event(CS_PDN_conf_id_t cid, cellular_pdn_event_callback_t pdn_event_callback)
{
  CS_Status_t retval = CS_ERROR;

  /* check parameters validity */
  if (cid > CS_PDN_USER_CONFIG_5)
  {
    PRINT_ERR("<Cellular_Service> only explicit PDN user config is supported (cid=%d)", cid)
  }
  else
  {
    /* Prepare the SID message to send to ATCore  */
    if (DATAPACK_writeStruct(getCmdBufPtr(),
                             (uint16_t) CSMT_NONE,
                             (uint16_t) 0U,
                             NULL) == DATAPACK_OK)
    {
      at_status_t err;

      /* Send the SID message to ATCore */
      err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_REGISTER_PDN_EVENT, getCmdBufPtr(), getRspBufPtr());
      if (err == ATSTATUS_OK)
      {
        /* <Cellular_Service> PDN events registered successfully */
        /* register callback */
        urc_packet_domain_event_callback[cid] = pdn_event_callback;
        cs_ctxt_urc_subscription.packet_domain_event = CS_TRUE;
        retval = CS_OK;
      }
    }
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service>error when registering PDN events")
  }
  return (retval);
}

/**
  * @brief  Deregister the internet event notifications.
  * @param  cid Configuration identifier number.
  * @retval CS_Status_t
  */
CS_Status_t CS_deregister_pdn_event(CS_PDN_conf_id_t cid)
{
  CS_Status_t retval = CS_ERROR;

  /* check parameters validity */
  if (cid > CS_PDN_USER_CONFIG_5)
  {
    PRINT_ERR("<Cellular_Service> only explicit PDN user config is supported (cid=%d)", cid)
  }
  else
  {
    /* register callback */
    urc_packet_domain_event_callback[cid] = NULL;
    cs_ctxt_urc_subscription.packet_domain_event = CS_FALSE;

    /* Prepare the SID message to send to ATCore  */
    if (DATAPACK_writeStruct(getCmdBufPtr(),
                             (uint16_t) CSMT_NONE,
                             (uint16_t) 0U,
                             NULL) == DATAPACK_OK)
    {
      at_status_t err;

      /* Send the SID message to ATCore */
      err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_DEREGISTER_PDN_EVENT, getCmdBufPtr(), getRspBufPtr());
      if (err == ATSTATUS_OK)
      {
        /* <Cellular_Service> PDN events deregistered successfully */
        retval = CS_OK;
      }
    }
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when deregistering PDN events")
  }
  return (retval);
}

/**
  * @brief  Request to suspend DATA mode.
  * @param  none
  * @retval CS_Status_t
  */
CS_Status_t CS_suspend_data(void)
{
  CS_Status_t retval = CS_ERROR;

  /* Prepare the SID message to send to ATCore  */
  if (DATAPACK_writeStruct(getCmdBufPtr(),
                           (uint16_t) CSMT_NONE,
                           (uint16_t) 0U,
                           NULL) == DATAPACK_OK)
  {
    at_status_t err;

    /* Send the SID message to ATCore */
    err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_DATA_SUSPEND, getCmdBufPtr(), getRspBufPtr());
    if (err == ATSTATUS_OK)
    {
      /* <Cellular_Service> DATA suspended */
      retval = CS_OK;
    }
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when suspending DATA")
  }
  return (retval);
}

/**
  * @brief  Request to resume DATA mode.
  * @param  none
  * @retval CS_Status_t
  */
CS_Status_t CS_resume_data(void)
{
  CS_Status_t retval = CS_ERROR;

  /* Prepare the SID message to send to ATCore  */
  if (DATAPACK_writeStruct(getCmdBufPtr(),
                           (uint16_t) CSMT_NONE,
                           (uint16_t) 0U,
                           NULL) == DATAPACK_OK)
  {
    at_status_t err;

    /* Send the SID message to ATCore */
    err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_DATA_RESUME, getCmdBufPtr(), getRspBufPtr());
    if (err == ATSTATUS_OK)
    {
      /* <Cellular_Service> DATA resumed */
      retval = CS_OK;
    }
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when resuming DATA")
  }
  return (retval);
}

/**
  * @brief  Request to reset the device.
  * @param  rst_type Type of reset requested (SW, HW, ...)
  * @retval CS_Status_t
  */
CS_Status_t CS_reset(CS_Reset_t rst_type)
{
  CS_Status_t retval = CS_OK;

  /* reset Cellular Service context */
  CELLULAR_reset_context();

  /* update sockets states */
  csint_modem_reset_update_socket_state();

  /* reset AT context */
  if (AT_reset_context(get_Adapter_Handle()) != ATSTATUS_OK)
  {
    PRINT_ERR("<Cellular_Service> Reset context error")
    retval = CS_ERROR;
  }

  /* treament depends of reset type */
  switch (rst_type)
  {
    case CS_RESET_HW:
      /* perform hardware reset */
      if (perform_HW_reset() != CS_OK)
      {
        retval = CS_ERROR;
      }
      break;

    case CS_RESET_SW:
      /* perform software reset */
      if (perform_SW_reset() != CS_OK)
      {
        retval = CS_ERROR;
      }
      break;

    case CS_RESET_AUTO:
      /* perform software reset first */
      if (perform_SW_reset() != CS_OK)
      {
        /* if software reset failed, perform hardware reset */
        if (perform_HW_reset() != CS_OK)
        {
          retval = CS_ERROR;
        }
      }
      break;

    case CS_RESET_FACTORY_RESET:
      /* perform factory reset  */
      if (perform_Factory_reset() != CS_OK)
      {
        retval = CS_ERROR;
      }
      break;

    default:
      PRINT_ERR("Invalid reset type")
      retval = CS_ERROR;
      break;
  }

  return (retval);
}

/**
  * @brief  Send a string will which be sended as it is to the modem (termination char will be added automatically)
  * @note   The termination char will be automatically added by the lower layer
  * @param  direct_cmd_tx The structure describing the command to send to the modem
  * @param  direct_cmd_callback Callback to send back to user the content of response received from the modem
  *                             parameter NOT IMPLEMENTED YET
  * @retval CS_Status_t
  */
CS_Status_t CS_direct_cmd(CS_direct_cmd_tx_t *direct_cmd_tx, cellular_direct_cmd_callback_t direct_cmd_callback)
{
  UNUSED(direct_cmd_callback); /* direct_cmd_callback not used for the moment */

  CS_Status_t retval = CS_ERROR;

  if (direct_cmd_tx->cmd_size <= MAX_DIRECT_CMD_SIZE)
  {
    /* Prepare the SID message to send to ATCore  */
    if (DATAPACK_writePtr(getCmdBufPtr(),
                          (uint16_t) CSMT_DIRECT_CMD,
                          (void *)direct_cmd_tx) == DATAPACK_OK)
    {
      at_status_t err;

      /* Send the SID message to ATCore */
      err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_DIRECT_CMD, getCmdBufPtr(), getRspBufPtr());
      if (err == ATSTATUS_OK)
      {
        /* <Cellular_Service> Direct command infos received */
        retval = CS_OK;
      }
      else
      {
        retval = CELLULAR_analyze_error_report(getRspBufPtr());
      }
    }
  }
  else
  {
    PRINT_INFO("<Cellular_Service> Direct command command size to big (limit=%d)", MAX_DIRECT_CMD_SIZE)
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when sending direct cmd")
  }
  return (retval);
}

/* Low Power API ----------------------------------------------------------------------------------------------- */

/**
  * @brief  Initialise power configuration. Called systematically first.
  * @note
  * @param  power_config Pointer to the structure describing the power parameters
  * @param  lp_status_callback callback function
  * @retval CS_Status_t
  */
CS_Status_t CS_InitPowerConfig(CS_init_power_config_t *p_power_config,
                               cellular_power_status_callback_t lp_status_callback)
{
  CS_Status_t retval = CS_ERROR;
  PRINT_INFO("CS_InitPowerConfig")

  /* save the callback */
  urc_lp_status_callback = lp_status_callback;

  /* Initialize the local data structure used to create SID message */
  static CS_init_power_config_t cs_ctxt_init_power_config;
  (void) memset((void *)&cs_ctxt_init_power_config, 0, sizeof(CS_init_power_config_t));
  (void) memcpy((void *)&cs_ctxt_init_power_config, (void *)p_power_config, sizeof(CS_init_power_config_t));

  /* Prepare the SID message to send to ATCore  */
  if (DATAPACK_writeStruct(getCmdBufPtr(),
                           (uint16_t) CSMT_INIT_POWER_CONFIG,
                           (uint16_t) sizeof(CS_init_power_config_t),
                           (void *)&cs_ctxt_init_power_config) == DATAPACK_OK)
  {
    at_status_t err;

    /* Send the SID message to ATCore */
    err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_INIT_POWER_CONFIG, getCmdBufPtr(), getRspBufPtr());
    if (err == ATSTATUS_OK)
    {
      /* <Cellular_Service> Power configuration set */
      retval = CS_OK;
    }
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when setting power configuration")
  }

  return (retval);
}

/**
  * @brief  Send power configuration (PSM & DRX) to apply to the modem
  * @note
  * @param  power_config Pointer to the structure describing the power parameters
  * @retval CS_Status_t
  */
CS_Status_t CS_SetPowerConfig(CS_set_power_config_t *p_power_config)
{
  CS_Status_t retval = CS_ERROR;
  PRINT_INFO("CS_SetPowerConfig")

  /* Initialize the local data structure used to create SID message */
  static CS_set_power_config_t cs_ctxt_set_power_config;
  (void) memset((void *)&cs_ctxt_set_power_config, 0, sizeof(CS_set_power_config_t));
  (void) memcpy((void *)&cs_ctxt_set_power_config, (void *)p_power_config, sizeof(CS_set_power_config_t));

  /* Prepare the SID message to send to ATCore  */
  if (DATAPACK_writeStruct(getCmdBufPtr(),
                           (uint16_t) CSMT_SET_POWER_CONFIG,
                           (uint16_t) sizeof(CS_set_power_config_t),
                           (void *)&cs_ctxt_set_power_config) == DATAPACK_OK)
  {
    at_status_t err;

    /* Send the SID message to ATCore */
    err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_SET_POWER_CONFIG, getCmdBufPtr(), getRspBufPtr());
    if (err == ATSTATUS_OK)
    {
      /* <Cellular_Service> Power configuration set */
      retval = CS_OK;
    }
  }

  /* check error code before to leave */
  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error when setting power configuration")
  }

  return (retval);
}

/**
  * @brief  Request sleep procedure
  * @note
  * @param  none
  * @retval CS_Status_t
  */
CS_Status_t CS_SleepRequest(void)
{
  CS_Status_t retval = CS_ERROR;
  PRINT_INFO("CS_SleepRequest")

  /* Prepare the SID message to send to ATCore  */
  if (DATAPACK_writeStruct(getCmdBufPtr(),
                           (uint16_t) CSMT_NONE,
                           (uint16_t) 0U,
                           NULL) == DATAPACK_OK)
  {
    at_status_t err;

    /* Send the SID message to ATCore */
    err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_SLEEP_REQUEST, getCmdBufPtr(), getRspBufPtr());
    if (err == ATSTATUS_OK)
    {
      /* <Cellular_Service> Sleep Request done */
      retval = CS_OK;
    }
  }

  return (retval);
}

/**
  * @brief  Complete sleep procedure
  * @note
  * @param  none
  * @retval CS_Status_t
  */
CS_Status_t CS_SleepComplete(void)
{
  CS_Status_t retval = CS_ERROR;
  PRINT_INFO("CS_SleepComplete")

  /* Prepare the SID message to send to ATCore  */
  if (DATAPACK_writeStruct(getCmdBufPtr(),
                           (uint16_t) CSMT_NONE,
                           (uint16_t) 0U,
                           NULL) == DATAPACK_OK)
  {
    at_status_t err;

    /* Send the SID message to ATCore */
    err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_SLEEP_COMPLETE, getCmdBufPtr(), getRspBufPtr());
    if (err == ATSTATUS_OK)
    {
      /* <Cellular_Service> Sleep Complete done */
      retval = CS_OK;
    }
  }

  return (retval);
}

/**
  * @brief  Cancel sleep procedure
  * @note   if something goes wrong: for example, the URC indicating that modem entered
  *         in Low Power has not been received.
  * @param  none
  * @retval CS_Status_t
  */
CS_Status_t CS_SleepCancel(void)
{
  CS_Status_t retval = CS_ERROR;
  PRINT_INFO("CS_SleepCancel")

  /* Prepare the SID message to send to ATCore  */
  if (DATAPACK_writeStruct(getCmdBufPtr(),
                           (uint16_t) CSMT_NONE,
                           (uint16_t) 0U,
                           NULL) == DATAPACK_OK)
  {
    at_status_t err;

    /* Send the SID message to ATCore */
    err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_SLEEP_CANCEL, getCmdBufPtr(), getRspBufPtr());
    if (err == ATSTATUS_OK)
    {
      /* <Cellular_Service> Sleep Cancel done */
      retval = CS_OK;
    }
  }

  return (retval);
}

/**
  * @brief  Request Power WakeUp
  * @note
  * @param  wakeup_origin indicates if wake-up comes from HOST or from MODEM
  * @retval CS_Status_t
  */
CS_Status_t CS_PowerWakeup(CS_wakeup_origin_t wakeup_origin)
{
  CS_Status_t retval = CS_ERROR;
  PRINT_INFO("CS_PowerWakeup")

  /* Prepare the SID message to send to ATCore  */
  if (DATAPACK_writeStruct(getCmdBufPtr(),
                           (uint16_t) CSMT_WAKEUP_ORIGIN,
                           (uint16_t) sizeof(CS_wakeup_origin_t),
                           (void *)&wakeup_origin) == DATAPACK_OK)
  {
    at_status_t err;

    /* Send the SID message to ATCore */
    err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_WAKEUP, getCmdBufPtr(), getRspBufPtr());
    if (err == ATSTATUS_OK)
    {
      /* <Cellular_Service> Power WakeUp done */
      retval = CS_OK;
    }
  }

  return (retval);
}

/**
  * @}
  */

/** @defgroup CELLULAR_SERVICE_API_Private_Functions CELLULAR_SERVICE API Private Functions
  * @{
  */

/**
  * @brief  Perform an hardware reset.
  * @param  none
  * @retval CS_Status_t
  */
static CS_Status_t perform_HW_reset(void)
{
  CS_Status_t retval = CS_ERROR;

  /* Initialize the local data structure used to create SID message */
  CS_Reset_t rst_type = CS_RESET_HW;

  if (SysCtrl_reset_device(DEVTYPE_MODEM_CELLULAR) == SCSTATUS_OK)
  {
    /* Prepare the SID message to send to ATCore  */
    if (DATAPACK_writeStruct(getCmdBufPtr(),
                             (uint16_t) CSMT_RESET,
                             (uint16_t) sizeof(CS_Reset_t),
                             (void *)&rst_type) == DATAPACK_OK)
    {
      at_status_t err;

      /* Send the SID message to ATCore */
      err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_RESET, getCmdBufPtr(), getRspBufPtr());
      if (err == ATSTATUS_OK)
      {
        /* <Cellular_Service> HW device reset done */
        retval = CS_OK;
      }
    }
  }

  return (retval);
}

/**
  * @brief  Perform a software reset.
  * @param  none
  * @retval CS_Status_t
  */
static CS_Status_t perform_SW_reset(void)
{
  CS_Status_t retval = CS_ERROR;

  /* Initialize the local data structure used to create SID message */
  CS_Reset_t rst_type = CS_RESET_SW;

  /* Prepare the SID message to send to ATCore  */
  if (DATAPACK_writeStruct(getCmdBufPtr(),
                           (uint16_t) CSMT_RESET,
                           (uint16_t) sizeof(CS_Reset_t),
                           (void *)&rst_type) == DATAPACK_OK)
  {
    at_status_t err;

    /* Send the SID message to ATCore */
    err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_RESET, getCmdBufPtr(), getRspBufPtr());
    if (err == ATSTATUS_OK)
    {
      /* <Cellular_Service> SW device reset done */
      retval = CS_OK;
    }
  }

  return (retval);
}

/**
  * @brief  Perform a facrory reset (if available for this modem).
  * @param  none
  * @retval CS_Status_t
  */
static CS_Status_t perform_Factory_reset(void)
{
  CS_Status_t retval = CS_ERROR;

  /* Initialize the local data structure used to create SID message */
  CS_Reset_t rst_type = CS_RESET_FACTORY_RESET;

  /* .e to send to ATCore  */
  if (DATAPACK_writeStruct(getCmdBufPtr(),
                           (uint16_t) CSMT_RESET,
                           (uint16_t) sizeof(CS_Reset_t),
                           (void *)&rst_type) == DATAPACK_OK)
  {
    at_status_t err;

    /* Send the SID message to ATCore */
    err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_RESET, getCmdBufPtr(), getRspBufPtr());
    if (err == ATSTATUS_OK)
    {
      /* <Cellular_Service> Factory device reset done */
      retval = CS_OK;
    }
  }

  return (retval);
}

/**
  * @brief  Reset cellular context.
  * @param  none
  * @retval none
  */
static void CELLULAR_reset_context(void)
{
  /* init cs_ctxt_urc_subscription */
  cs_ctxt_urc_subscription.eps_network_registration = CS_FALSE;
  cs_ctxt_urc_subscription.gprs_network_registration = CS_FALSE;
  cs_ctxt_urc_subscription.cs_network_registration = CS_FALSE;
  cs_ctxt_urc_subscription.eps_location_info = CS_FALSE;
  cs_ctxt_urc_subscription.gprs_location_info = CS_FALSE;
  cs_ctxt_urc_subscription.cs_location_info = CS_FALSE;
  cs_ctxt_urc_subscription.signal_quality = CS_FALSE;
  cs_ctxt_urc_subscription.packet_domain_event = CS_FALSE;
  cs_ctxt_urc_subscription.ping_rsp = CS_FALSE;

  /* init cs_ctxt_eps_location_info */
  cs_ctxt_eps_location_info.ci = 0U;
  cs_ctxt_eps_location_info.lac = 0U;
  cs_ctxt_eps_location_info.ci_updated = CS_FALSE;
  cs_ctxt_eps_location_info.lac_updated = CS_FALSE;

  /* init cs_ctxt_gprs_location_info */
  cs_ctxt_gprs_location_info.ci = 0U;
  cs_ctxt_gprs_location_info.lac = 0U;
  cs_ctxt_gprs_location_info.ci_updated = CS_FALSE;
  cs_ctxt_gprs_location_info.lac_updated = CS_FALSE;

  /* init cs_ctxt_cs_location_info */
  cs_ctxt_cs_location_info.ci = 0U;
  cs_ctxt_cs_location_info.lac = 0U;
  cs_ctxt_cs_location_info.ci_updated = CS_FALSE;
  cs_ctxt_cs_location_info.lac_updated = CS_FALSE;

  /* init network states */
  cs_ctxt_eps_network_reg_state = CS_NRS_UNKNOWN;
  cs_ctxt_gprs_network_reg_state = CS_NRS_UNKNOWN;
  cs_ctxt_cs_network_reg_state = CS_NRS_UNKNOWN;
}

/**
  * @brief  Reset socket context.
  * @param  none
  * @retval none
  */
static void CELLULAR_reset_socket_context(void)
{
  PRINT_DBG("CELLULAR_reset_socket_context")
  uint8_t cpt;
  for (cpt = 0U; cpt < CELLULAR_MAX_SOCKETS; cpt ++)
  {
    csint_socket_init((socket_handle_t)cpt);
  }
}

/**
  * @brief  Initialization of cellulat context.
  * @param  none
  * @retval none
  */
static CS_Status_t CELLULAR_init(void)
{
  CS_Status_t retval = CS_ERROR;

  /* static variables */
  static sysctrl_info_t modem_device_infos;  /* LTE Modem information */

  if (SysCtrl_getDeviceDescriptor(DEVTYPE_MODEM_CELLULAR, &modem_device_infos) == SCSTATUS_OK)
  {
    /* init ATCore & IPC layers*/
    (void) AT_init();

    at_handle_t at_handle = AT_open(&modem_device_infos, CELLULAR_urc_notif);
    set_Adapter_Handle(at_handle);
    if (at_handle != AT_HANDLE_INVALID)
    {
      /* init local context variables */
      CELLULAR_reset_context();

      /* init socket context */
      CELLULAR_reset_socket_context();

      retval = CS_OK;
    }
  }

  return (retval);
}

/**
  * @brief  Callback function for URC notification.
  * @param  p_urc_buf Pointer to buffer with URC content.
  * @retval none
  */
static void CELLULAR_urc_notif(at_buf_t *p_urc_buf)
{
  uint16_t msgtype;

  msgtype = DATAPACK_readMsgType(p_urc_buf);

  /* --- EPS NETWORK REGISTRATION URC --- */
  if ((msgtype == (uint16_t) CSMT_URC_EPS_NETWORK_REGISTRATION_STATUS) &&
      (cs_ctxt_urc_subscription.eps_network_registration == CS_TRUE))
  {
    CS_NetworkRegState_t rx_state;
    /* Read response from ATCore  */
    if (DATAPACK_readStruct(p_urc_buf,
                            (uint16_t) CSMT_URC_EPS_NETWORK_REGISTRATION_STATUS,
                            (uint16_t) sizeof(CS_NetworkRegState_t),
                            (void *)&rx_state) == DATAPACK_OK)
    {
      /* if network registration status has changed, notify client */
      if (rx_state != cs_ctxt_eps_network_reg_state)
      {
        PRINT_DBG("<Cellular_Service> EPS network registration updated: %d", rx_state)
        cs_ctxt_eps_network_reg_state = rx_state;

        /* if a valid callback is registered, call it */
        if (urc_eps_network_registration_callback != NULL)
        {
          /* possible evolution: pack data to client */
          (* urc_eps_network_registration_callback)();
        }
      }
    }
  }
  /* --- GPRS NETWORK REGISTRATION URC --- */
  else if ((msgtype == (uint16_t) CSMT_URC_GPRS_NETWORK_REGISTRATION_STATUS) &&
           (cs_ctxt_urc_subscription.gprs_network_registration == CS_TRUE))
  {
    CS_NetworkRegState_t rx_state;
    /* Read response from ATCore  */
    if (DATAPACK_readStruct(p_urc_buf,
                            (uint16_t) CSMT_URC_GPRS_NETWORK_REGISTRATION_STATUS,
                            (uint16_t) sizeof(CS_NetworkRegState_t),
                            (void *)&rx_state) == DATAPACK_OK)
    {
      /* if network registration status has changed, notify client */
      if (rx_state != cs_ctxt_gprs_network_reg_state)
      {
        PRINT_DBG("<Cellular_Service> GPRS network registration updated: %d", rx_state)
        cs_ctxt_gprs_network_reg_state = rx_state;

        /* if a valid callback is registered, call it */
        if (urc_gprs_network_registration_callback != NULL)
        {
          (* urc_gprs_network_registration_callback)();
        }
      }
    }
  }
  /* --- CS NETWORK REGISTRATION URC --- */
  else if ((msgtype == (uint16_t) CSMT_URC_CS_NETWORK_REGISTRATION_STATUS) &&
           (cs_ctxt_urc_subscription.cs_network_registration == CS_TRUE))
  {
    CS_NetworkRegState_t rx_state;
    /* Read response from ATCore  */
    if (DATAPACK_readStruct(p_urc_buf,
                            (uint16_t) CSMT_URC_CS_NETWORK_REGISTRATION_STATUS,
                            (uint16_t) sizeof(CS_NetworkRegState_t),
                            (void *)&rx_state) == DATAPACK_OK)
    {
      /* if network registration status has changed, notify client */
      if (rx_state != cs_ctxt_cs_network_reg_state)
      {
        PRINT_DBG("<Cellular_Service> CS network registration updated: %d", rx_state)
        cs_ctxt_cs_network_reg_state = rx_state;

        /* if a valid callback is registered, call it */
        if (urc_cs_network_registration_callback != NULL)
        {
          (* urc_cs_network_registration_callback)();
        }
      }
    }
  }
  /* --- EPS LOCATION INFORMATION URC --- */
  else if ((msgtype == (uint16_t) CSMT_URC_EPS_LOCATION_INFO) &&
           (cs_ctxt_urc_subscription.eps_location_info == CS_TRUE))
  {
    CS_Bool_t loc_update = CS_FALSE;
    csint_location_info_t rx_loc;
    /* Read response from ATCore  */
    if (DATAPACK_readStruct(p_urc_buf,
                            (uint16_t) CSMT_URC_EPS_LOCATION_INFO,
                            (uint16_t) sizeof(csint_location_info_t),
                            (void *)&rx_loc) == DATAPACK_OK)
    {
      /* ci received and changed since last time ? */
      if (rx_loc.ci_updated == CS_TRUE)
      {
        if (rx_loc.ci != cs_ctxt_eps_location_info.ci)
        {
          /* ci has change */
          loc_update = CS_TRUE;
          cs_ctxt_eps_location_info.ci = rx_loc.ci;
        }

        /* if local ci info was not updated */
        if (cs_ctxt_eps_location_info.ci_updated == CS_FALSE)
        {
          loc_update = CS_TRUE;
          cs_ctxt_eps_location_info.ci_updated = CS_TRUE;
        }
      }

      /* lac received and changed since last time ? */
      if (rx_loc.lac_updated == CS_TRUE)
      {
        if (rx_loc.lac != cs_ctxt_eps_location_info.lac)
        {
          /* lac has change */
          loc_update = CS_TRUE;
          cs_ctxt_eps_location_info.lac = rx_loc.lac;
        }

        /* if local lac info was not updated */
        if (cs_ctxt_eps_location_info.lac_updated == CS_FALSE)
        {
          loc_update = CS_TRUE;
          cs_ctxt_eps_location_info.lac_updated = CS_TRUE;
        }
      }

      /* if location has changed, notify client */
      if (loc_update == CS_TRUE)
      {
        /* if a valid callback is registered, call it */
        if (urc_eps_location_info_callback != NULL)
        {
          PRINT_DBG("<Cellular_Service> EPS location information info updated: lac=%d, ci=%ld", rx_loc.lac, rx_loc.ci)
          (* urc_eps_location_info_callback)();
        }
      }
    }
  }
  /* --- GPRS LOCATION INFORMATION URC --- */
  else if ((msgtype == (uint16_t) CSMT_URC_GPRS_LOCATION_INFO) &&
           (cs_ctxt_urc_subscription.gprs_location_info == CS_TRUE))
  {
    CS_Bool_t loc_update = CS_FALSE;
    csint_location_info_t rx_loc;
    /* Read response from ATCore  */
    if (DATAPACK_readStruct(p_urc_buf,
                            (uint16_t) CSMT_URC_GPRS_LOCATION_INFO,
                            (uint16_t) sizeof(csint_location_info_t),
                            (void *)&rx_loc) == DATAPACK_OK)
    {
      /* ci received and changed since last time ? */
      if (rx_loc.ci_updated == CS_TRUE)
      {
        if (rx_loc.ci != cs_ctxt_gprs_location_info.ci)
        {
          /* ci has change */
          loc_update = CS_TRUE;
          cs_ctxt_gprs_location_info.ci = rx_loc.ci;
        }

        /* if local ci info was not updated */
        if (cs_ctxt_gprs_location_info.ci_updated == CS_FALSE)
        {
          loc_update = CS_TRUE;
          cs_ctxt_gprs_location_info.ci_updated = CS_TRUE;
        }
      }

      /* lac received and changed since last time ? */
      if (rx_loc.lac_updated == CS_TRUE)
      {
        if (rx_loc.lac != cs_ctxt_gprs_location_info.lac)
        {
          /* lac has change */
          loc_update = CS_TRUE;
          cs_ctxt_gprs_location_info.lac = rx_loc.lac;
        }

        /* if local lac info was not updated */
        if (cs_ctxt_gprs_location_info.lac_updated == CS_FALSE)
        {
          loc_update = CS_TRUE;
          cs_ctxt_gprs_location_info.lac_updated = CS_TRUE;
        }
      }

      /* if location has changed, notify client */
      if (loc_update == CS_TRUE)
      {
        /* if a valid callback is registered, call it */
        if (urc_gprs_location_info_callback != NULL)
        {
          PRINT_DBG("<Cellular_Service> GPRS location information info updated: lac=%d, ci=%ld", rx_loc.lac, rx_loc.ci)
          (* urc_gprs_location_info_callback)();
        }
      }
    }
  }
  /* --- CS LOCATION INFORMATION URC --- */
  else if ((msgtype == (uint16_t) CSMT_URC_CS_LOCATION_INFO) &&
           (cs_ctxt_urc_subscription.cs_location_info == CS_TRUE))
  {
    CS_Bool_t loc_update = CS_FALSE;
    csint_location_info_t rx_loc;
    /* Read response from ATCore  */
    if (DATAPACK_readStruct(p_urc_buf,
                            (uint16_t) CSMT_URC_CS_LOCATION_INFO,
                            (uint16_t) sizeof(csint_location_info_t),
                            (void *)&rx_loc) == DATAPACK_OK)
    {
      /* ci received and changed since last time ? */
      if (rx_loc.ci_updated == CS_TRUE)
      {
        if (rx_loc.ci != cs_ctxt_cs_location_info.ci)
        {
          /* ci has change */
          loc_update = CS_TRUE;
          cs_ctxt_cs_location_info.ci = rx_loc.ci;
        }

        /* if local ci info was not updated */
        if (cs_ctxt_cs_location_info.ci_updated == CS_FALSE)
        {
          loc_update = CS_TRUE;
          cs_ctxt_cs_location_info.ci_updated = CS_TRUE;
        }
      }

      /* lac received and changed since last time ? */
      if (rx_loc.lac_updated == CS_TRUE)
      {
        if (rx_loc.lac != cs_ctxt_cs_location_info.lac)
        {
          /* lac has change */
          loc_update = CS_TRUE;
          cs_ctxt_cs_location_info.lac = rx_loc.lac;
        }

        /* if local lac info was not updated */
        if (cs_ctxt_cs_location_info.lac_updated == CS_FALSE)
        {
          loc_update = CS_TRUE;
          cs_ctxt_cs_location_info.lac_updated = CS_TRUE;
        }
      }

      /* if location has changed, notify client */
      if (loc_update == CS_TRUE)
      {
        /* if a valid callback is registered, call it */
        if (urc_cs_location_info_callback != NULL)
        {
          PRINT_DBG("<Cellular_Service> CS location information info updated: lac=%d, ci=%ld", rx_loc.lac, rx_loc.ci)
          (* urc_cs_location_info_callback)();
        }
      }
    }
  }
  /* --- SIGNAL QUALITY URC --- */
  else if ((msgtype == (uint16_t) CSMT_URC_SIGNAL_QUALITY) &&
           (cs_ctxt_urc_subscription.signal_quality == CS_TRUE))
  {
    CS_SignalQuality_t local_sig_qual;
    /* Read response from ATCore  */
    if (DATAPACK_readStruct(p_urc_buf,
                            (uint16_t) CSMT_URC_SIGNAL_QUALITY,
                            (uint16_t) sizeof(CS_SignalQuality_t),
                            (void *)&local_sig_qual) == DATAPACK_OK)
    {
      /* if a valid callback is registered, call it */
      if (urc_signal_quality_callback != NULL)
      {
        PRINT_INFO("<Cellular_Service> CS signal quality info updated: rssi=%d, ber=%d", local_sig_qual.rssi,
                   local_sig_qual.ber)
        (* urc_signal_quality_callback)();
      }
    }
  }
  /* --- SOCKET DATA PENDING URC --- */
  else if (msgtype == (uint16_t) CSMT_URC_SOCKET_DATA_PENDING)
  {
    /* unpack data received */
    socket_handle_t sockHandle;
    /* Read response from ATCore  */
    if (DATAPACK_readStruct(p_urc_buf,
                            (uint16_t) CSMT_URC_SOCKET_DATA_PENDING,
                            (uint16_t) sizeof(socket_handle_t),
                            (void *)&sockHandle) == DATAPACK_OK)
    {
      if (sockHandle != CS_INVALID_SOCKET_HANDLE)
      {
        /* inform client that data are pending */
        if (cs_ctxt_sockets_info[sockHandle].socket_data_ready_callback != NULL)
        {
          (* cs_ctxt_sockets_info[sockHandle].socket_data_ready_callback)(sockHandle);
        }
      }
    }
  }
  /* --- SOCKET DATA CLOSED BY REMOTE URC --- */
  else if (msgtype == (uint16_t) CSMT_URC_SOCKET_CLOSED)
  {
    /* unpack data received */
    socket_handle_t sockHandle;
    /* Read response from ATCore  */
    if (DATAPACK_readStruct(p_urc_buf,
                            (uint16_t) CSMT_URC_SOCKET_CLOSED,
                            (uint16_t) sizeof(socket_handle_t),
                            (void *)&sockHandle) == DATAPACK_OK)
    {
      if (sockHandle != CS_INVALID_SOCKET_HANDLE)
      {
        /* inform client that socket has been closed by remote  */
        if (cs_ctxt_sockets_info[sockHandle].socket_remote_close_callback != NULL)
        {
          (* cs_ctxt_sockets_info[sockHandle].socket_remote_close_callback)(sockHandle);
        }

        /* do not deallocate socket handle and reinit socket parameters
          *   socket_deallocateHandle(sockHandle);
          *   client has to confirm with a call to CDS_socket_close()
          */
      }
    }
  }
  /* --- PACKET DOMAIN EVENT URC --- */
  else if ((msgtype == (uint16_t) CSMT_URC_PACKET_DOMAIN_EVENT) &&
           (cs_ctxt_urc_subscription.packet_domain_event == CS_TRUE))
  {
    /* unpack data received */
    csint_PDN_event_desc_t pdn_event;
    /* Read response from ATCore  */
    if (DATAPACK_readStruct(p_urc_buf,
                            (uint16_t) CSMT_URC_PACKET_DOMAIN_EVENT,
                            (uint16_t) sizeof(csint_PDN_event_desc_t),
                            (void *)&pdn_event) == DATAPACK_OK)
    {
      PRINT_DBG("PDN event: origine=%d scope=%d type=%d (user cid=%d) ",
                pdn_event.event_origine, pdn_event.event_scope,
                pdn_event.event_type, pdn_event.conf_id)

      /* is it a valid PDN ? */
      if ((pdn_event.conf_id == CS_PDN_USER_CONFIG_1) ||
          (pdn_event.conf_id == CS_PDN_USER_CONFIG_2) ||
          (pdn_event.conf_id == CS_PDN_USER_CONFIG_3) ||
          (pdn_event.conf_id == CS_PDN_USER_CONFIG_4) ||
          (pdn_event.conf_id == CS_PDN_USER_CONFIG_5) ||
          (pdn_event.conf_id == CS_PDN_PREDEF_CONFIG))
      {
        /* if a valid callback is registered, call it */
        if (urc_packet_domain_event_callback[pdn_event.conf_id] != NULL)
        {
          CS_PDN_event_t conv_pdn_event = convert_to_PDN_event(pdn_event);
          (* urc_packet_domain_event_callback[pdn_event.conf_id])(pdn_event.conf_id, conv_pdn_event);
        }
      }
      else if (pdn_event.conf_id == CS_PDN_ALL)
      {
        CS_PDN_event_t conv_pdn_event = convert_to_PDN_event(pdn_event);
        /* event reported to all PDN registered */
        for (uint8_t loop = 0U; loop < CS_MAX_NB_PDP_CTXT; loop++)
        {
          /* if a valid callback is registered, call it */
          if (urc_packet_domain_event_callback[loop] != NULL)
          {
            CS_PDN_conf_id_t pdn_cid = convert_index_to_PDN_conf(loop);
            (* urc_packet_domain_event_callback[loop])(pdn_cid, conv_pdn_event);
          }
        }
      }
      else
      {
        PRINT_INFO("PDN not identified")
      }
    }
  }
  /* --- PING URC --- */
  else if (msgtype == (uint16_t) CSMT_URC_PING_RSP)
  {
    /* unpack data received */
    CS_Ping_response_t ping_rsp;
    /* Read response from ATCore  */
    if (DATAPACK_readStruct(p_urc_buf,
                            (uint16_t) CSMT_URC_PING_RSP,
                            (uint16_t) sizeof(CS_Ping_response_t),
                            (void *)&ping_rsp) == DATAPACK_OK)
    {
      PRINT_INFO("ping URC received at CS level")
      /* if a valid callback is registered, call it */
      if (urc_ping_rsp_callback != NULL)
      {
        (* urc_ping_rsp_callback)(ping_rsp);
      }
    }
  }
  /* --- MODEM EVENT URC --- */
  else if (msgtype == (uint16_t) CSMT_URC_MODEM_EVENT)
  {
    /* unpack data received */
    CS_ModemEvent_t modem_events;
    /* Read response from ATCore  */
    if (DATAPACK_readStruct(p_urc_buf,
                            (uint16_t) CSMT_URC_MODEM_EVENT,
                            (uint16_t) sizeof(CS_ModemEvent_t),
                            (void *)&modem_events) == DATAPACK_OK)
    {
      PRINT_DBG("MODEM events received= 0x%x", modem_events)
      /* if a valid callback is registered, call it */
      if (urc_modem_event_callback != NULL)
      {
        (* urc_modem_event_callback)(modem_events);
      }
    }
  }
  /* --- SIM STATUS EVENT URC --- */
  else if (msgtype == (uint16_t) CSMT_URC_SIM_EVENT)
  {
    /* unpack data received */
    CS_SimEvent_status_t sim_status;
    /* Read response from ATCore  */
    if (DATAPACK_readStruct(p_urc_buf,
                            (uint16_t) CSMT_URC_SIM_EVENT,
                            (uint16_t) sizeof(CS_SimEvent_status_t),
                            (void *)&sim_status) == DATAPACK_OK)
    {
      PRINT_INFO("MODEM SIM event received= %d", sim_status.event)
      /* if a valid callback is registered, call it */
      if (urc_sim_event_callback != NULL)
      {
        (* urc_sim_event_callback)(sim_status);
      }
    }
  }
  /* --- LOW POWER STATUS EVENT URC --- */
  else if (msgtype == (uint16_t) CSMT_URC_LP_STATUS_EVENT)
  {
    /* unpack data received */
    CS_LowPower_status_t lp_status;
    /* Read response from ATCore  */
    if (DATAPACK_readStruct(p_urc_buf,
                            (uint16_t) CSMT_URC_LP_STATUS_EVENT,
                            (uint16_t) sizeof(CS_LowPower_status_t),
                            (void *)&lp_status) == DATAPACK_OK)
    {
      PRINT_DBG("negotiated value of T3324 = %ld", lp_status.nwk_active_time)
      PRINT_DBG("negotiated value of T3412 = %ld", lp_status.nwk_periodic_TAU)
      /* if a valid callback is registered, call it */
      if (urc_lp_status_callback != NULL)
      {
        (* urc_lp_status_callback)(lp_status);
      }
    }
  }
#if defined(USE_COM_MDM)
  /* --- COMMDM EVENT URC --- */
  else if (msgtype == (uint16_t) CSMT_URC_COMMDM_EVENT)
  {
    /* unpack data received */
    CS_comMdm_status_t comMdmd_event_infos;
    comMdmd_event_infos.param1 = 0U;
    if (DATAPACK_readStruct(p_urc_buf,
                            (uint16_t) CSMT_URC_COMMDM_EVENT,
                            (uint16_t) sizeof(CS_comMdm_status_t),
                            (void *)&comMdmd_event_infos) == DATAPACK_OK)
    {
      PRINT_INFO("COMMDM event received")
      if (urc_commdm_event_callback != NULL)
      {
        (* urc_commdm_event_callback)(comMdmd_event_infos);
      }
    }
  }
#endif /* defined(USE_COM_MDM) */
  else
  {
    PRINT_DBG("ignore received URC (type=%d)", msgtype)
  }
}

/**
  * @brief  Function used to prepare and report ERROR report if available.
  * @param  p_rsp_buf Pointer to buffer with error report content.
  * @retval CS_Status_t
  */
static CS_Status_t CELLULAR_analyze_error_report(at_buf_t *p_rsp_buf)
{
  CS_Status_t retval;
  uint16_t msgtype;

  msgtype = DATAPACK_readMsgType(p_rsp_buf);

  /* default return value */
  retval = CS_ERROR;

  /* check if we have received an error report */
  if (msgtype == (uint16_t) CSMT_ERROR_REPORT)
  {
    csint_error_report_t error_report;
    /* Read response from ATCore  */
    if (DATAPACK_readStruct(p_rsp_buf,
                            (uint16_t) CSMT_ERROR_REPORT,
                            (uint16_t) sizeof(csint_error_report_t),
                            (void *)&error_report) == DATAPACK_OK)
    {
      if (error_report.error_type == CSERR_SIM)
      {
        /* SIM error */
        retval = convert_SIM_error(&error_report);
      }
      /* other errors returns generic error value */
    }
  }

  PRINT_DBG("CS returned modified value after error report analysis = %d", retval)
  return (retval);
}

/**
  * @brief  Convert detailed SIM error to CS_Status_t error.
  * @param  p_error_report Pointer to error report buffer.
  * @retval CS_Status_t
  */
static CS_Status_t convert_SIM_error(const csint_error_report_t *p_error_report)
{
  CS_Status_t retval;

  /* convert SIM state to Cellular Service error returned to the client */
  switch (p_error_report->sim_state)
  {
    case CS_SIMSTATE_SIM_NOT_INSERTED:
      retval = CS_SIM_NOT_INSERTED;
      break;
    case CS_SIMSTATE_SIM_BUSY:
      retval = CS_SIM_BUSY;
      break;
    case CS_SIMSTATE_SIM_WRONG:
    case CS_SIMSTATE_SIM_FAILURE:
      retval = CS_SIM_ERROR;
      break;
    case CS_SIMSTATE_SIM_PIN_REQUIRED:
    case CS_SIMSTATE_SIM_PIN2_REQUIRED:
    case CS_SIMSTATE_SIM_PUK_REQUIRED:
    case CS_SIMSTATE_SIM_PUK2_REQUIRED:
      retval = CS_SIM_PIN_OR_PUK_LOCKED;
      break;
    case CS_SIMSTATE_INCORRECT_PASSWORD:
      retval = CS_SIM_INCORRECT_PASSWORD;
      break;
    default:
      retval = CS_SIM_ERROR;
      break;
  }
  return (retval);
}

/**
  * @brief  Convert PDN event description to PDN event.
  * @param  event_desc Event description structure.
  * @retval CS_PDN_event_t
  */
static CS_PDN_event_t convert_to_PDN_event(csint_PDN_event_desc_t event_desc)
{
  CS_PDN_event_t ret = CS_PDN_EVENT_OTHER;

  /* Network detach events */
  if ((event_desc.event_origine == CGEV_EVENT_ORIGINE_NW) &&
      (event_desc.event_scope == CGEV_EVENT_SCOPE_GLOBAL) &&
      (event_desc.event_type == CGEV_EVENT_TYPE_DETACH))
  {
    ret = CS_PDN_EVENT_NW_DETACH;
  }
  /* Network global deactivation events */
  else if ((event_desc.event_origine == CGEV_EVENT_ORIGINE_NW) &&
           (event_desc.event_scope == CGEV_EVENT_SCOPE_GLOBAL) &&
           (event_desc.event_type == CGEV_EVENT_TYPE_DEACTIVATION))
  {
    ret = CS_PDN_EVENT_NW_DEACT;
  }
  /* Network PDN deactivation events */
  else if ((event_desc.event_origine == CGEV_EVENT_ORIGINE_NW) &&
           (event_desc.event_scope == CGEV_EVENT_SCOPE_PDN) &&
           (event_desc.event_type == CGEV_EVENT_TYPE_DEACTIVATION))
  {
    ret = CS_PDN_EVENT_NW_PDN_DEACT;
  }
  else
  {
    /* ignored */
  }

  return (ret);
}

/**
  * @brief  Convert index value to PDN configuration ID.
  * @param  index Index to convert.
  * @retval CS_PDN_conf_id_t
  */
static CS_PDN_conf_id_t convert_index_to_PDN_conf(uint8_t index)
{
  CS_PDN_conf_id_t PDNconf;
  switch (index)
  {
    case 0:
      PDNconf = CS_PDN_PREDEF_CONFIG;
      break;
    case 1:
      PDNconf = CS_PDN_USER_CONFIG_1;
      break;
    case 2:
      PDNconf = CS_PDN_USER_CONFIG_2;
      break;
    case 3:
      PDNconf = CS_PDN_USER_CONFIG_3;
      break;
    case 4:
      PDNconf = CS_PDN_USER_CONFIG_4;
      break;
    case 5:
      PDNconf = CS_PDN_USER_CONFIG_5;
      break;
    default:
      PDNconf = CS_PDN_NOT_DEFINED;
      break;
  }
  return (PDNconf);
}

#if defined(USE_COM_MDM)
/**
  * @brief  register callback for mdm urc
  * @note   register a function as callback for mdm urc receive from modem
  * @param  commdm_urc_cb - The call back to be registered. May be NULL to unregister callback.
  * @retval CS_Status_t
  * @note   the provided call back function should execute a minimum of code.
  *         Application should create an event or message to trigger a receive of a message to be treated later
  */
CS_Status_t CS_ComMdm_subscribe_event(CS_comMdm_callback_t commdm_urc_cb)
{
  if (commdm_urc_cb == NULL)
  {
    /* unregister callback */
    urc_commdm_event_callback = NULL;
  }
  else
  {
    /* register callback */
    urc_commdm_event_callback = commdm_urc_cb;
  }

  return (CS_OK);
}

/**
  * @brief  sends a MDM command to the Modem.
  * @param[in]  txBuf Pointer to the transmitted buffer descriptor
  * @param[out]  errorCode Pointer to the error code returned for the send operation.
  * @retval CS_Status_t
  */
CS_Status_t CS_ComMdm_send(CS_Tx_Buffer_t *txBuf, int32_t *errorCode)
{
  CS_Status_t retval = CS_ERROR;
  PRINT_API("CS_ComMdm_send")

  /* check buffer are not NULL and size is not 0 */
  if (txBuf != NULL)
  {
    if ((txBuf->p_buffer != NULL)
        && (txBuf->buffer_size != 0U))
    {
      csint_ComMdm_t com_mdm_data;

      com_mdm_data.transaction_type = CS_COMMDM_SEND;
      (void) memcpy((void *)&com_mdm_data.txBuffer, (void *)txBuf, sizeof(CS_Tx_Buffer_t));
      (void) memset((void *)&com_mdm_data.rxBuffer, 0, sizeof(CS_Rx_Buffer_t));
      com_mdm_data.errorCode = 0;

      if (DATAPACK_writeStruct(getCmdBufPtr(),
                               (uint16_t) CSMT_COM_MDM,
                               (uint16_t) sizeof(csint_ComMdm_t),
                               (void *)&com_mdm_data) == DATAPACK_OK)
      {
        at_status_t err;
        err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_COM_MDM_TRANSACTION, getCmdBufPtr(), getRspBufPtr());
        if (err == ATSTATUS_OK)
        {
          retval = CS_OK;
          if (DATAPACK_readStruct(getRspBufPtr(),
                                  (uint16_t) CSMT_COM_MDM,
                                  (uint16_t) sizeof(csint_ComMdm_t),
                                  &com_mdm_data) == DATAPACK_OK)
          {
            /* PRINT_INFO("returned value: error code=%d", com_mdm_data.errorCode) */

            /* recopy  error code */
            *errorCode = com_mdm_data.errorCode;
            retval = CS_OK;
          }
        }
      }
    }
  }

  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error during COM-MDM send")
  }
  return (retval);
}

/**
  * @brief  initiate a full transaction (send + answer receive) for a MDM command to the Modem.
  * @param[in]  txBuf Pointer to the transmitted buffer descriptor
  * @param[in]  rxBuf Pointer to the received buffer descriptor
  * @param[out]  errorCode Pointer to the error code returned for the transaction operation.
  * @retval CS_Status_t
  */
CS_Status_t CS_ComMdm_transaction(CS_Tx_Buffer_t *txBuf, CS_Rx_Buffer_t *rxBuf, int32_t *errorCode)
{
  CS_Status_t retval = CS_ERROR;
  PRINT_API("CS_ComMdm_transaction")

  /* check buffer are not NULL and size is not 0 */
  if ((txBuf != NULL) && (rxBuf != NULL))
  {
    if ((txBuf->p_buffer != NULL)
        && (txBuf->buffer_size != 0U)
        && (rxBuf->p_buffer != NULL)
        && (rxBuf->max_buffer_size != 0U))
    {
      csint_ComMdm_t com_mdm_data;

      com_mdm_data.transaction_type = CS_COMMDM_TRANSACTION;
      (void) memcpy((void *)&com_mdm_data.txBuffer, (void *)txBuf, sizeof(CS_Tx_Buffer_t));
      (void) memcpy((void *)&com_mdm_data.rxBuffer, (void *)rxBuf, sizeof(CS_Rx_Buffer_t));
      com_mdm_data.errorCode = 0;

      if (DATAPACK_writeStruct(getCmdBufPtr(),
                               (uint16_t) CSMT_COM_MDM,
                               (uint16_t) sizeof(csint_ComMdm_t),
                               (void *)&com_mdm_data) == DATAPACK_OK)
      {
        at_status_t err;
        err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_COM_MDM_TRANSACTION, getCmdBufPtr(), getRspBufPtr());
        if (err == ATSTATUS_OK)
        {
          retval = CS_OK;
          if (DATAPACK_readStruct(getRspBufPtr(),
                                  (uint16_t) CSMT_COM_MDM,
                                  (uint16_t) sizeof(csint_ComMdm_t),
                                  &com_mdm_data) == DATAPACK_OK)
          {
            /*
            PRINT_INFO("returned value: TX ptr=%p size=%d", com_mdm_data.txBuffer.p_buffer,
                                                            com_mdm_data.txBuffer.buffer_size)
            PRINT_INFO("returned value: RX ptr=%p size=%d", com_mdm_data.rxBuffer.p_buffer,
                                                            com_mdm_data.rxBuffer.buffer_size)
            PRINT_INFO("returned value: RX = %s", com_mdm_data.rxBuffer.p_buffer)
            PRINT_INFO("returned value: error code=%d", com_mdm_data.errorCode)
            */

            /* recopy size of received buffer + error code */
            rxBuf->buffer_size = com_mdm_data.rxBuffer.buffer_size;
            *errorCode = com_mdm_data.errorCode;
            retval = CS_OK;
          }
        }
      }
    }
  }

  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error during COM-MDM transaction")
  }
  return (retval);
}

/**
  * @brief  read message from modem to the rsp buffer provided by the application.
  * @param[in]  rxBuf Pointer to the received buffer descriptor
  * @param[out]  errorCode Pointer to the error code returned for the receive operation.
  * @retval CS_Status_t
  */
CS_Status_t CS_ComMdm_receive(CS_Rx_Buffer_t *rxBuf, int32_t *errorCode)
{
  CS_Status_t retval = CS_ERROR;
  PRINT_API("CS_ComMdm_receive")

  /* check buffer are not NULL and size is not 0 */
  if (rxBuf != NULL)
  {
    if ((rxBuf->p_buffer != NULL)
        && (rxBuf->max_buffer_size != 0U))
    {
      csint_ComMdm_t com_mdm_data;

      com_mdm_data.transaction_type = CS_COMMDM_RECEIVE;
      (void) memset((void *)&com_mdm_data.txBuffer, 0, sizeof(CS_Tx_Buffer_t));
      (void) memcpy((void *)&com_mdm_data.rxBuffer, (void *)rxBuf, sizeof(CS_Rx_Buffer_t));
      com_mdm_data.errorCode = 0;

      if (DATAPACK_writeStruct(getCmdBufPtr(),
                               (uint16_t) CSMT_COM_MDM,
                               (uint16_t) sizeof(csint_ComMdm_t),
                               (void *)&com_mdm_data) == DATAPACK_OK)
      {
        at_status_t err;
        err = AT_sendcmd(get_Adapter_Handle(), (at_msg_t) SID_CS_COM_MDM_TRANSACTION, getCmdBufPtr(), getRspBufPtr());
        if (err == ATSTATUS_OK)
        {
          retval = CS_OK;
          if (DATAPACK_readStruct(getRspBufPtr(),
                                  (uint16_t) CSMT_COM_MDM,
                                  (uint16_t) sizeof(csint_ComMdm_t),
                                  &com_mdm_data) == DATAPACK_OK)
          {
            /*
            PRINT_INFO("returned value: TX ptr=%p size=%d", com_mdm_data.txBuffer.p_buffer,
                                                            com_mdm_data.txBuffer.buffer_size)
            PRINT_INFO("returned value: RX ptr=%p size=%d", com_mdm_data.rxBuffer.p_buffer,
                                                            com_mdm_data.rxBuffer.buffer_size)
            PRINT_INFO("returned value: RX = %s", com_mdm_data.rxBuffer.p_buffer)
            PRINT_INFO("returned value: error code=%d", com_mdm_data.errorCode)
            */

            /* recopy size of received buffer + error code */
            rxBuf->buffer_size = com_mdm_data.rxBuffer.buffer_size;
            *errorCode = com_mdm_data.errorCode;
            retval = CS_OK;
          }
        }
      }
    }
  }

  if (retval == CS_ERROR)
  {
    PRINT_ERR("<Cellular_Service> error during COM-MDM receive")
  }
  return (retval);
}
#endif /* defined(USE_COM_MDM) */
