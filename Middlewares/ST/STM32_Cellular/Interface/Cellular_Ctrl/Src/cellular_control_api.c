/**
  ******************************************************************************
  * @file    cellular_control_api.c
  * @author  MCD Application Team
  * @brief   This file defines functions for Cellular Control API
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
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
#include <string.h>
#include <stdbool.h>

#include "plf_config.h"
#include "cellular_control_api.h"
#include "cellular_runtime_standard.h"

#include "ipc_uart.h"
#include "com_core.h"
#if (USE_LOW_POWER == 1)
#include "cellular_service_power.h"
#endif  /* (USE_LOW_POWER == 1) */
#include "cellular_service_task.h"
#include "dc_common.h"

#if (USE_CMD_CONSOLE == 1)
#include "cmd.h"
#endif /* (USE_CMD_CONSOLE == 1) */

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
#include "ppposif.h"
#include "ppposif_client.h"
#endif  /* USE_SOCKETS_TYPE == USE_SOCKETS_LWIP */

#if (USE_PRINTF == 0U)
#include "trace_interface.h" /* needed to call traceIF_init() and traceIF_start() */
#define PRINT_FORCE(format, args...)   TRACE_PRINT_FORCE(DBG_CHAN_MAIN, DBL_LVL_P0, "" format "\n\r", ## args)
#else
#include <stdio.h>
#define PRINT_FORCE(format, args...)   (void)printf("" format "\n\r", ## args);
#endif  /* (USE_PRINTF == 0U) */


/* Private defines -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/**
  * @brief Type to store all the possible callback allowed by the API. These call back could be called later by the
  *        global call back which is the only one directly registered to Data Cache
  */
typedef struct
{
  cellular_info_cb_t        cellular_info_cb;
  void                     *p_cellular_info_cb_ctx;
  cellular_signal_info_cb_t cellular_signal_info_cb;
  void                     *p_cellular_signal_info_cb_ctx;
  cellular_sim_info_cb_t    cellular_sim_info_cb;
  void                     *p_cellular_sim_info_cb_ctx;
  cellular_nfmc_info_cb_t   cellular_nfmc_info_cb;
  void                     *p_cellular_nfmc_info_cb_ctx;
  cellular_ip_info_cb_t     cellular_ip_info_cb;
  void                     *p_cellular_ip_info_cb_ctx;
#if (USE_LOW_POWER == 1)
  cellular_power_info_cb_t  cellular_power_info_cb;
  void                     *p_cellular_power_info_cb_ctx;
#endif /* USE_LOW_POWER == 1 */
} cellular_api_cb_t;

/**
  * To store all the call backs registered by applications through cellular api.
  * Size of array is the number of application using cellular api.
  */
static cellular_api_cb_t cellular_api_registration_cb[CELLULAR_CONCURENT_REGISTRATION_CB_MAX_NB];

/* Private function prototypes -----------------------------------------------*/

/**
  * @brief         Copy string to a len + string structure.
  * @param[in]     p_in_str     - The string to be converted to len + string structure.
  * @param[in]     max_len      - The string length max that can be copied to the result string : value parameter.
  * @param[in,out] p_out_length - The output string length (0 if length "str" is greater than "max_len").
  * @param[in,out] p_out_value  - The output string content (empty string if length "str" is greater than "max_len").
  * @retval -
  */
static void cellular_api_fill_string(const uint8_t *p_in_str, uint8_t max_len, uint8_t *const p_out_length,
                                     uint8_t *const p_out_value);

/**
  * @brief      Data cache callback. Called when a Data Cache entry is updated
  * @param[in]  datacache_event_id  - data cache event
  * @param[in]  private_data - private Data Cache context (not used)
  * @retval -
  */
static void cellular_api_general_data_cache_callback(dc_com_event_id_t datacache_event_id, const void *p_private_data);

/* Global variables ----------------------------------------------------------*/
/* Private function Definition -----------------------------------------------*/
/**
  * @brief         Copy string to a len + string structure.
  * @param[in]     p_in_str     - The string to be converted to len + string structure.
  * @param[in]     max_len      - The string length max that can be copied to the result string : value parameter.
  * @param[in,out] p_out_length - The output string length (0 if length "str" is greater than "max_len").
  * @param[in,out] p_out_value  - The output string content (empty string if length "str" is greater than "max_len").
  * @retval -
  */
static void cellular_api_fill_string(const uint8_t *p_in_str, uint8_t max_len, uint8_t *const p_out_length,
                                     uint8_t *const p_out_value)
{
  uint32_t size;

  /* Get size of string */
  size = crs_strlen(p_in_str);
  /* Check if size can fit in destination string */
  if ((size + 1U) <= max_len)
  {
    /* If yes, fill output data */
    *p_out_length = (uint8_t)size;
    (void)memcpy((void *)p_out_value, p_in_str, size + 1U);
  }
  else
  {
    /* String size too big, return an empty string */
    *p_out_length = 0U;
    p_out_value[0] = (uint8_t)'\0';
  }
}

/**
  * @brief Register cellular api global call back to Data Cache if needed
  */
static cellular_result_t cellular_api_register_general_cb(void)
{
  /**
    * @brief to know if cellular_api general callback is already registered to Data Cacha or not.
    *        By default, no call back is used, so global call back is not registered to Data Cache
  */
  static bool cellular_api_global_cb_registered = false;

  cellular_result_t ret = CELLULAR_SUCCESS;

  /* if global call back not already registered to Data Cache */
  if (cellular_api_global_cb_registered == false)
  {
    /* register general call back to Data Cache  */
    if (dc_com_register_gen_event_cb(&dc_com_db, cellular_api_general_data_cache_callback, (const void *)NULL) ==
        DC_COM_INVALID_ENTRY)
    {
      /* Data Cache registration returned an error */
      ret = CELLULAR_ERR_INTERNAL;
    }
    else
    {
      /* Data Cache registration is OK, store the fact that cellular api global call back is now registered */
      cellular_api_global_cb_registered = true;
      /* Reset the content of the structure containing the call back information */
      (void)memset((void *)cellular_api_registration_cb, 0,
                   sizeof(cellular_api_cb_t) * CELLULAR_CONCURENT_REGISTRATION_CB_MAX_NB);
    }
  }

  return (ret);
}

/**
  * @brief      Cellular_api Data cache general callback. Called when a Data Cache entry is updated
  * @param[in]  datacache_event_id  - data cache event
  * @param[in]  private_data - private Data Cache context (not used)
  * @retval -
  */
static void cellular_api_general_data_cache_callback(dc_com_event_id_t datacache_event_id, const void *private_data)
{
  UNUSED(private_data);

  static cellular_info_t        cellular_info;
  static cellular_signal_info_t cellular_signal_info;
  static cellular_sim_info_t    cellular_sim_info;
  static cellular_nfmc_info_t   cellular_nfmc_info;
  static cellular_ip_info_t     cellular_ip_info;
#if (USE_LOW_POWER == 1)
  static cellular_power_info_t  cellular_power_info;
#endif /* (USE_LOW_POWER == 1) */

  uint8_t idx;

  for (idx = 0; idx < CELLULAR_CONCURENT_REGISTRATION_CB_MAX_NB; idx++)
  {
    if (datacache_event_id == DC_CELLULAR_INFO)
    {
      if (cellular_api_registration_cb[idx].cellular_info_cb != NULL)
      {
        cellular_get_cellular_info(&cellular_info);
        cellular_api_registration_cb[idx].cellular_info_cb(CA_CELLULAR_INFO_EVENT, &cellular_info,
                                                           cellular_api_registration_cb[idx].p_cellular_info_cb_ctx);
      }
    }
    else if (datacache_event_id == DC_CELLULAR_SIGNAL_INFO)
    {
      if (cellular_api_registration_cb[idx].cellular_signal_info_cb != NULL)
      {
        cellular_get_signal_info(&cellular_signal_info);
        cellular_api_registration_cb[idx].cellular_signal_info_cb \
        (CA_SIGNAL_INFO_EVENT, &cellular_signal_info, cellular_api_registration_cb[idx].p_cellular_signal_info_cb_ctx);

      }
    }
    else if (datacache_event_id == DC_CELLULAR_SIM_INFO)
    {
      if (cellular_api_registration_cb[idx].cellular_sim_info_cb != NULL)
      {
        cellular_get_sim_info(&cellular_sim_info);
        cellular_api_registration_cb[idx].cellular_sim_info_cb \
        (CA_SIM_INFO_EVENT, &cellular_sim_info, cellular_api_registration_cb[idx].p_cellular_sim_info_cb_ctx);
      }
    }
    else if (datacache_event_id == DC_CELLULAR_NFMC_INFO)
    {
      if (cellular_api_registration_cb[idx].cellular_nfmc_info_cb != NULL)
      {
        cellular_get_nfmc_info(&cellular_nfmc_info);
        cellular_api_registration_cb[idx].cellular_nfmc_info_cb \
        (CA_NFMC_INFO_EVENT, &cellular_nfmc_info, cellular_api_registration_cb[idx].p_cellular_nfmc_info_cb_ctx);
      }
    }
    else if (datacache_event_id == DC_CELLULAR_NIFMAN_INFO)
    {
      if (cellular_api_registration_cb[idx].cellular_ip_info_cb != NULL)
      {
        cellular_get_ip_info(&cellular_ip_info);
        cellular_api_registration_cb[idx].cellular_ip_info_cb \
        (CA_IP_INFO_EVENT, &cellular_ip_info, cellular_api_registration_cb[idx].p_cellular_ip_info_cb_ctx);
      }
    }
#if (USE_LOW_POWER == 1)
    else if (datacache_event_id == DC_CELLULAR_POWER_STATUS)
    {
      if (cellular_api_registration_cb[idx].cellular_power_info_cb != NULL)
      {
        cellular_get_power_info(&cellular_power_info);
        cellular_api_registration_cb[idx].cellular_power_info_cb \
        (CA_POWER_INFO_EVENT, &cellular_power_info, cellular_api_registration_cb[idx].p_cellular_power_info_cb_ctx);
      }
    }
#endif /* (USE_LOW_POWER == 1) */
    else
    {
      __NOP(); /* Nothing to do */
    }
  }
}

/* Public function Definition -----------------------------------------------*/

/**
  * @brief  Initialize cellular software.
  * @param  -
  * @retval -
  */
void cellular_init(void)
{
  uint8_t cellular_version[CELLULAR_VERSION_STR_LEN];

#if (USE_PRINTF == 0U)
  /* Recall traceIF_init() in case StartDefaultTask is not used or is redefined */
  traceIF_init();
#endif /* (USE_PRINTF == 0U)  */

  if (cellular_get_string_version(&cellular_version[0], (uint8_t)sizeof(cellular_version)) == true)
  {
    PRINT_FORCE("\n\r===== X-Cube-Cellular version : %s =====\n\r", cellular_version);
  }
  else
  {
    PRINT_FORCE("!! X-Cube-Cellular version unreadable, array too short !!\n\r");
  }

#if (USE_CMD_CONSOLE == 1)
  /* CMD initialization */
  CMD_init();
#endif /* (USE_CMD_CONSOLE == 1) */

  /* Data Cache initialization */
  (void)dc_com_init(&dc_com_db);

  /* Communication interface initialization */
  (void)com_init();

  /* Cellular service initialization */
  (void)CST_cellular_service_init();

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  /* if LWIP is used, PPP component initialization */
  (void)ppposif_client_init();
#endif  /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
}

/**
  * @brief  Start cellular software with boot modem
  *         (and network registration if PLF_CELLULAR_TARGET_STATE = 2U see plf_cellular_config.h).
  * @param  -
  * @retval -
  */
void cellular_start(void)
{
#if (USE_PRINTF == 0U)
  /* Trace interface start */
  traceIF_start();
#endif /* (USE_PRINTF == 0U) */

#if (USE_CMD_CONSOLE == 1)
  /* CMD start */
  CMD_start();
#endif /* (USE_CMD_CONSOLE == 1) */

  /* Data Cache start */
  dc_com_start(&dc_com_db);

#if (USE_LOW_POWER == 1)
  CSP_Start();
#endif  /* (USE_LOW_POWER == 1) */

  /* Communication interface start */
  (void)com_start();

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  /* if LWIP is used, PPP component start */
  (void)ppposif_client_start();
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */

  /* Cellular service start */
  (void)CST_cellular_service_start();

  /* Cellular service radio on */
  (void)CST_radio_on();
}

/**
  * @brief  Start cellular with boot modem (NO network registration).\n
  *         Usage: Used to configure modem
  * @param  -
  * @retval -
  */
void cellular_modem_start(void)
{
  /* Cellular service start */
  (void)CST_cellular_service_start();
  /* Cellular service modem power on */
  (void)CST_modem_power_on();
}

/**
  * @brief  Stop the modem (stop data mode, and detach from the network).
  * @param  -
  * @retval cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                   indicating the cause of the error.\n
  *         CELLULAR_SUCCESS          The operation is successful.\n
  *         CELLULAR_ERR_INTERNAL     Error while executing X-Cube-Cellular internal function.
  */
cellular_result_t cellular_modem_stop(void)
{
  cellular_result_t ret = CELLULAR_SUCCESS;
  dc_cellular_target_state_t target_state;

#if (USE_LOW_POWER == 1)
  /* if low power is on, eventually need to wake up the modem if it was in PSM mode to allow
     clean detach toward the network. */
  (void)CSP_DataWakeup(HOST_WAKEUP);

  /* As modem will stop, need to stop the timer */
  CSP_StopTimeout();
#endif /* (USE_LOW_POWER == 1) */

  /* 'cst targetstate off' command: modem stops requested */
  /* New modem target state  request */
  if (dc_com_read(&dc_com_db, DC_CELLULAR_TARGET_STATE_CMD, (void *)&target_state, sizeof(target_state)) == DC_COM_OK)
  {
    target_state.rt_state     = DC_SERVICE_ON;
    target_state.target_state = DC_TARGET_STATE_OFF;
    target_state.callback     = true;
    (void)dc_com_write(&dc_com_db, DC_CELLULAR_TARGET_STATE_CMD, (void *)&target_state, sizeof(target_state));
  }
  else
  {
    ret = CELLULAR_ERR_INTERNAL;
  }

  return (ret);
}

/**
  * @brief  Modem power on and instruct the modem to perform network registration.
  * @param  -
  * @retval cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                   indicating the cause of the error.\n
  *         CELLULAR_SUCCESS          The operation is successful.\n
  *         CELLULAR_ERR_INTERNAL     Error while executing X-Cube-Cellular internal function.
  */
cellular_result_t cellular_connect(void)
{
  cellular_result_t ret = CELLULAR_SUCCESS;
  dc_cellular_target_state_t target_state;

  /* 'cst targetstate full' command: modem start with registration requested */
  /* New modem target state request */
  if (dc_com_read(&dc_com_db, DC_CELLULAR_TARGET_STATE_CMD, (void *)&target_state, sizeof(target_state)) == DC_COM_OK)
  {
    target_state.rt_state     = DC_SERVICE_ON;
    target_state.target_state = DC_TARGET_STATE_FULL;
    target_state.callback     = true;
    (void)dc_com_write(&dc_com_db, DC_CELLULAR_TARGET_STATE_CMD, (void *)&target_state, sizeof(target_state));
  }
  else
  {
    ret = CELLULAR_ERR_INTERNAL;
  }

  return (ret);
}

/**
  * @brief  Modem disconnect, instruct the modem to perform network unregistration.\n
  *         Modem stays on and sim is still accessible.
  * @param  -
  * @retval cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                   indicating the cause of the error.\n
  *         CELLULAR_SUCCESS          The operation is successful.\n
  *         CELLULAR_ERR_INTERNAL     Error while executing X-Cube-Cellular internal function.
  */
cellular_result_t cellular_disconnect(void)
{
  cellular_result_t ret = CELLULAR_SUCCESS;
  dc_cellular_target_state_t target_state;

  /* 'cst targetstate sim_only' command: modem deregistration requested */
  /* New modem target state request */
  if (dc_com_read(&dc_com_db, DC_CELLULAR_TARGET_STATE_CMD, (void *)&target_state, sizeof(target_state)) == DC_COM_OK)
  {
    target_state.rt_state     = DC_SERVICE_ON;
    target_state.target_state = DC_TARGET_STATE_SIM_ONLY;
    target_state.callback     = true;
    (void)dc_com_write(&dc_com_db, DC_CELLULAR_TARGET_STATE_CMD, (void *)&target_state, sizeof(target_state));
  }
  else
  {
    ret = CELLULAR_ERR_INTERNAL;
  }

  return (ret);
}

/**
  * @brief  Get X-Cube-Cellular major version encoded on 16 bits
  * @param  -
  * @retval X-Cube-Cellular major version on 16 bits
  */
uint16_t cellular_get_major_version(void)
{
  return ((uint16_t)CELLULAR_VERSION_MAJOR);
}

/**
  * @brief  Get X-Cube-Cellular minor version encoded on 8 bits
  * @param  -
  * @retval X-Cube-Cellular minor version on 8 bits
  */
uint8_t cellular_get_minor_version(void)
{
  return ((uint8_t)CELLULAR_VERSION_MINOR);
}

/**
  * @brief  Get X-Cube-Cellular patch version encoded on 8 bits
  * @param  -
  * @retval X-Cube-Cellular patch version on 8 bits
  */
uint8_t cellular_get_patch_version(void)
{
  return ((uint8_t)CELLULAR_VERSION_PATCH);
}

/**
  * @brief  Check if X-Cube-Cellular version is a Release version
  * @param  -
  * @retval true if X-Cube-Cellular version is a release version, else return false
  */
bool cellular_version_is_release(void)
{
#if (CELLULAR_VERSION_IS_DEV == 0U)
  return (true);
#else
  return (false);
#endif /* (CELLULAR_VERSION_IS_DEV == 0U) */
}

/**
  * @brief  Get X-Cube-Cellular version encoded on 32 bits, following the pattern MMMMmmPP\n
  *         From Most Significant Bit to Least Significant Bit :
  *         MMMM Major version on 16 bits then mm minor version on 8 bits then PP Patch version on 8 bits
  * @param  -
  * @retval 32 bits value integer representing X-Cube-Cellular version
  */
uint32_t cellular_get_version(void)
{
  return ((((uint16_t)CELLULAR_VERSION_MAJOR << 16) +
           ((uint8_t)CELLULAR_VERSION_MINOR << 8) +
           ((uint8_t)CELLULAR_VERSION_PATCH)));
}

/**
  * @brief  Get version as string format representing the current X-Cube-Cellular version\n
  *         MM.mm.pp_Dev  for a dev version\n
  *         MM.mm.pp      for a release version
  * @param  p_ret_version pointer on an allocated area where will be stored the X-Cube-Cellular version as a string
  * @param  len           length of the structure pointed by p_ret_version - len must be >= CELLULAR_VERSION_STR_LEN
  * @retval bool - false/true - version is not returned (len too small) / version is returned
  */
bool cellular_get_string_version(uint8_t *p_ret_version, uint8_t len)
{
  bool ret;
  /* 19 is the sum of max length of each part : 5 for Major version (16 bits number),                */
  /*     3 for minor and patch version (8 bits number), and 8 for '-', '.', '_DEV' and trailing '\0' */
  /* Modify CELLULAR_VERSION_STR_LEN define if the pattern length change                             */
  if ((p_ret_version != NULL) && (len >= CELLULAR_VERSION_STR_LEN))
  {
#if (CELLULAR_VERSION_IS_DEV == 1)
    (void) sprintf((CRC_CHAR_t *)p_ret_version, "%.*s-%d.%d.%d_DEV",
                   (int16_t)CELLULAR_VERSION_FIRMWARE_NAME_LEN, CELLULAR_VERSION_FIRMWARE_NAME,
                   CELLULAR_VERSION_MAJOR, CELLULAR_VERSION_MINOR, CELLULAR_VERSION_PATCH);
#else
    (void) sprintf((CRC_CHAR_t *)p_ret_version, "%.*s-%d.%d.%d",
                   (int16_t)CELLULAR_VERSION_FIRMWARE_NAME_LEN, CELLULAR_VERSION_FIRMWARE_NAME,
                   CELLULAR_VERSION_MAJOR, CELLULAR_VERSION_MINOR, CELLULAR_VERSION_PATCH);
#endif /* (CELLULAR_VERSION_IS_DEV == 1) */
    ret = true;
  }
  else
  {
    ret = false;
  }

  return (ret);
}

/**
  * @brief         Get Cellular information.
  * @param[in,out] p_cellular_info - The cellular info structure to contain the response.
  * @retval -
  */
void cellular_get_cellular_info(cellular_info_t *const p_cellular_info)
{
  dc_cellular_info_t        datacache_cellular_info;       /* Cellular infos                                          */
  dc_cellular_params_t      datacache_cellular_param;      /* Cellular parameters used to configure the modem.        */
  dc_sim_info_t             datacache_sim_info;            /* Sim information */

  if (p_cellular_info != NULL)
  {
    /* Get needed data from Data Cache */
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&datacache_cellular_info, sizeof(dc_cellular_info_t));
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&datacache_cellular_param, sizeof(dc_cellular_params_t));
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_SIM_INFO, (void *)&datacache_sim_info, sizeof(dc_sim_info_t));

    /* Set sockets_type modem or LWIP */
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
    p_cellular_info->sockets_type    =  CA_SOCKETS_LWIP;
#else
    p_cellular_info->sockets_type    =  CA_SOCKETS_MODEM;
#endif /*  (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)*/
    /* Set modem state */
    p_cellular_info->modem_state = datacache_cellular_info.modem_state;

    /* Modem identity : manufacturer, model, revision and serial */
    cellular_api_fill_string(datacache_cellular_info.manufacturer_name, CA_MANUFACTURER_ID_SIZE_MAX,
                             &p_cellular_info->identity.manufacturer_id.len,
                             p_cellular_info->identity.manufacturer_id.value);
    cellular_api_fill_string(datacache_cellular_info.model, CA_MODEL_ID_SIZE_MAX,
                             &p_cellular_info->identity.model_id.len, p_cellular_info->identity.model_id.value);
    cellular_api_fill_string(datacache_cellular_info.revision, CA_REVISION_ID_SIZE_MAX,
                             &p_cellular_info->identity.revision_id.len, p_cellular_info->identity.revision_id.value);
    cellular_api_fill_string(datacache_cellular_info.serial_number, CA_SERIAL_NUMBER_ID_SIZE_MAX,
                             &p_cellular_info->identity.serial_number_id.len,
                             p_cellular_info->identity.serial_number_id.value);

    /* Imei */
    cellular_api_fill_string(datacache_cellular_info.imei, CA_IMEI_SIZE_MAX, &p_cellular_info->imei.len,
                             p_cellular_info->imei.value);

    /* Network attachment timeout */
    p_cellular_info->nwk_attachment_timeout = datacache_cellular_param.attachment_timeout;
    p_cellular_info->nwk_inactivity_timeout = datacache_cellular_param.lp_inactivity_timeout;

    /* Mobile Network Operator Name returned by network*/
    cellular_api_fill_string(datacache_cellular_info.mno_name, CA_MNO_NAME_SIZE_MAX,
                             &p_cellular_info->mno_name.len, p_cellular_info->mno_name.value);

    /* Socket modem IP address */
    p_cellular_info->ip_addr.addr = datacache_cellular_info.ip_addr.addr;
  }
}

/**
  * @brief     Set the PDN value to use for a specific SIM slot.
  * @param[in] sim_slot_type  - The SIM slot that as to be configured.
  * @param[in] p_cellular_pdn - The new PDN value to use.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_BADARGUMENT  Argument value not compliant.\n
  *                                      e.g sim_slot_type is unknown for this platform.\n
  *            CELLULAR_ERR_INTERNAL     Error while executing X-Cube-Cellular internal function.
  */
cellular_result_t cellular_set_pdn(ca_sim_slot_type_t sim_slot_type, const cellular_pdn_t *const p_cellular_pdn)
{
  cellular_result_t         ret;                           /* Return value of the function                            */
  dc_cellular_params_t      datacache_cellular_param;      /* Cellular parameters used to configure the modem.        */
  uint8_t                   idx;                           /* Loop index                                              */

  if (p_cellular_pdn != NULL)
  {
    ret = CELLULAR_SUCCESS;

    /* Check entry parameters */
    if ((p_cellular_pdn->apn_send_to_modem != CA_APN_SEND_TO_MODEM) &&
        (p_cellular_pdn->apn_send_to_modem != CA_APN_NOT_SEND_TO_MODEM))
    {
      ret = CELLULAR_ERR_BADARGUMENT;
    }

    if (p_cellular_pdn->cid > 9U)
    {
      ret = CELLULAR_ERR_BADARGUMENT;
    }

    /* If no error with entry parameter then continue, else return error */
    if (ret == CELLULAR_SUCCESS)
    {
      /* Get needed data from Data Cache */
      if (dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&datacache_cellular_param,
                      sizeof(dc_cellular_params_t)) == DC_COM_OK)
      {
        if (datacache_cellular_param.rt_state == DC_SERVICE_ON)
        {

          idx = 0;
          /* Search the Sim slot within the preference ordered table */
          while ((idx < DC_SIM_SLOT_NB) && (datacache_cellular_param.sim_slot[idx].sim_slot_type != sim_slot_type))
          {
            idx++;
          }
          /* If idx < DC_SIM_SLOT_NB, the requested sim slot type was found, else return error */
          if (idx < DC_SIM_SLOT_NB)
          {
            /* Sim slot to be modify found. Update data cache with new values */
            /* Apn_send_to_modem */
            datacache_cellular_param.sim_slot[idx].apnSendToModem = p_cellular_pdn->apn_send_to_modem;

            /* Cid */
            datacache_cellular_param.sim_slot[idx].cid = p_cellular_pdn->cid;

            /* APN */
            if (p_cellular_pdn->apn.len != 0U)
            {
              (void)memcpy(datacache_cellular_param.sim_slot[idx].apn, p_cellular_pdn->apn.value,
                           (uint32_t)p_cellular_pdn->apn.len + (uint32_t)1U);
            }
            else
            {
              datacache_cellular_param.sim_slot[idx].apn[0] = (uint8_t)'\0';
            }

            /* Username for APN */
            if (p_cellular_pdn->username.len != 0U)
            {
              (void)memcpy(datacache_cellular_param.sim_slot[idx].username, p_cellular_pdn->username.value,
                           (uint32_t)p_cellular_pdn->username.len + (uint32_t)1U);
            }
            else
            {
              datacache_cellular_param.sim_slot[idx].username[0] = (uint8_t)'\0';
            }

            /* Password for APN */
            if (p_cellular_pdn->username.len != 0U)
            {
              (void)memcpy(datacache_cellular_param.sim_slot[idx].password, p_cellular_pdn->password.value,
                           (uint32_t)p_cellular_pdn->password.len + (uint32_t)1U);
            }
            else
            {
              datacache_cellular_param.sim_slot[idx].password[0] = (uint8_t)'\0';
            }

            /* Check if error occurred */
            /* Write modified data to Data Cache */
            if (dc_com_write(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&datacache_cellular_param,
                             sizeof(dc_cellular_params_t)) != DC_COM_OK)
            {
              /* If something went wrong, return an error */
              ret = CELLULAR_ERR_INTERNAL;
            }
          }
          else /* Idx >= DC_SIM_SLOT_NB */
          {
            ret = CELLULAR_ERR_BADARGUMENT;
          }
        }
        else /* rt_state != DC_SERVICE_ON */
        {
          ret = CELLULAR_ERR_STATE;
        }
      }
      else /* Dc_com_read returned an error */
      {
        ret = CELLULAR_ERR_INTERNAL;
      }
    }
  }
  else
  {
    /* P_cellular_pdn is NULL */
    ret = CELLULAR_ERR_BADARGUMENT;
  }
  return (ret);
}

/**
  * @brief     Set the SIM slot order.
  * @param[in] sim_slot_nb     - The SIM slots number defined in p_sim_slot_type.
  * @param[in] p_sim_slot_type - The new SIM Slot type order to use.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_BADARGUMENT  Argument value not compliant.\n
  *                                      e.g sim_slot_nb > PLF_CELLULAR_SIM_SLOT_NB\n
  *                                          p_sim_slot_type contain a unknown SIM Slot type.\n
  *            CELLULAR_ERR_INTERNAL     Error while executing X-Cube-Cellular internal function.
  */
cellular_result_t cellular_set_sim_slot_order(uint8_t sim_slot_nb, const ca_sim_slot_type_t *const p_sim_slot_type)
{
  cellular_result_t         ret;                           /* Return value of the function                            */
  dc_cellular_params_t      datacache_cellular_param;      /* Cellular parameters used to configure the modem.        */
  uint8_t                   idx;                           /* Loop index                                              */

  if (p_sim_slot_type != NULL)
  {
    ret = CELLULAR_SUCCESS;

    /* Read data structure to be modified */
    if (dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&datacache_cellular_param, sizeof(dc_cellular_params_t)) ==
        DC_COM_OK)
    {
      if (datacache_cellular_param.rt_state == DC_SERVICE_ON)
      {
        /* Check if number of new sim slot do not exceeds the maximum allowed */
        if (sim_slot_nb <= DC_SIM_SLOT_NB)
        {
          /* Update the number of sim slots used */
          datacache_cellular_param.sim_slot_nb = sim_slot_nb;

          /* Update the table for the defined slots */
          for (idx = 0U; idx < sim_slot_nb; idx++)
          {
            datacache_cellular_param.sim_slot[idx].sim_slot_type = p_sim_slot_type[idx];
          }
          /* Reset the value of the unused slots to the default value */
          for (idx = sim_slot_nb; idx < DC_SIM_SLOT_NB; idx++)
          {
            datacache_cellular_param.sim_slot[idx].sim_slot_type = CA_SIM_NON_EXISTING_SLOT;
          }
          /* Write modified data to the Data Cache */
          if (dc_com_write(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&datacache_cellular_param,
                           sizeof(dc_cellular_params_t)) != DC_COM_OK)
          {
            /* Write to data cache return error */
            ret = CELLULAR_ERR_INTERNAL;
          }
        }
        else /* Number of new sim slot exceeds the maximum allowed */
        {
          ret = CELLULAR_ERR_BADARGUMENT;
        }
      }
      else /* rt_state != DC_SERVICE_ON */
      {
        ret = CELLULAR_ERR_STATE;
      }
    }
    else /* Dc_com_read returned an error */
    {
      ret = CELLULAR_ERR_INTERNAL;
    }
  }
  else
  {
    /* P_sim_slot_type is NULL */
    ret = CELLULAR_ERR_BADARGUMENT;
  }

  return (ret);
}

/**
  * @brief     Set Operator parameters.
  * @param[in] p_operator_selection    - The new operator configuration to use.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_BADARGUMENT  Argument value not compliant.\n
  *            CELLULAR_ERR_INTERNAL     Error while executing X-Cube-Cellular internal function.
  */
cellular_result_t cellular_set_operator(const cellular_operator_selection_t *const p_operator_selection)
{
  dc_cellular_params_t          datacache_cellular_params; /* Cellular parameters used to configure the modem.        */
  cellular_result_t             ret;                       /* Return value of the function                            */

  if (p_operator_selection != NULL)
  {
    ret = CELLULAR_SUCCESS;

    /* Get existing data from Data Cache */
    if (dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&datacache_cellular_params,
                    sizeof(dc_cellular_params_t)) == DC_COM_OK)
    {
      if (datacache_cellular_params.rt_state == DC_SERVICE_ON)
      {
        /* Registration mode */
        datacache_cellular_params.operator_selector.network_reg_mode = p_operator_selection->ntw_registration_mode;

        /* Operator name format */
        datacache_cellular_params.operator_selector.operator_name_format = p_operator_selection->operator_name_format;

        /* Operator name */
        if (p_operator_selection->operator_name.len != 0U)
        {
          (void)memcpy(datacache_cellular_params.operator_selector.operator_name,
                       p_operator_selection->operator_name.value,
                       (uint32_t)p_operator_selection->operator_name.len + (uint32_t)1U);
        }
        else
        {
          datacache_cellular_params.operator_selector.operator_name[0] = (uint8_t)'\0';
        }

        /* access techno present */
        datacache_cellular_params.operator_selector.access_techno_present = p_operator_selection->access_techno_present;

        /* access techno present */
        datacache_cellular_params.operator_selector.access_techno = p_operator_selection->access_techno;

        /* Write modified data to Data Cache if no error detected */
        if (dc_com_write(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&datacache_cellular_params,
                         sizeof(dc_cellular_params_t)) != DC_COM_OK)
        {
          /* If something went wrong, return an error */
          ret = CELLULAR_ERR_INTERNAL;
        }
      }
      else /* rt_state != DC_SERVICE_ON */
      {
        ret = CELLULAR_ERR_STATE;
      }
    }
    else
    {
      /* Dc_com_read return error */
      ret = CELLULAR_ERR_INTERNAL;
    }
  }
  else
  {
    /* P_power_config is NULL */
    ret = CELLULAR_ERR_BADARGUMENT;
  }
  return (ret);
}

/**
  * @brief     Set a new value for network attachment timeout.
  * @param[in] nwk_attachment_timeout  - The new network attachment timeout value to use. Unit: in milliseconds.
  *                                      0U : no timeout(means infinite wait).
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_BADARGUMENT  Argument value not compliant.\n
  *            CELLULAR_ERR_INTERNAL     Error while executing X-Cube-Cellular internal function.
  */
cellular_result_t cellular_set_nwk_attachment_timeout(uint32_t nwk_attachment_timeout)
{
  cellular_result_t         ret;                           /* Return value of the function                            */
  dc_cellular_params_t      datacache_cellular_param;      /* Cellular parameters used to configure the modem.        */

  ret = CELLULAR_SUCCESS;

  /* Read data structure to be modified */
  if (dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&datacache_cellular_param, sizeof(dc_cellular_params_t)) ==
      DC_COM_OK)
  {
    if (datacache_cellular_param.rt_state == DC_SERVICE_ON)
    {
      /* Update network attachment timeout in the data cache */
      datacache_cellular_param.attachment_timeout = nwk_attachment_timeout;

      /* Write modified data to the Data Cache */
      if (dc_com_write(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&datacache_cellular_param,
                       sizeof(dc_cellular_params_t)) != DC_COM_OK)
      {
        /* Write to data cache return error */
        ret = CELLULAR_ERR_INTERNAL;
      }
    }
    else /* rt_state != DC_SERVICE_ON */
    {
      ret = CELLULAR_ERR_STATE;
    }
  }
  else /* Dc_com_read returned an error */
  {
    ret = CELLULAR_ERR_INTERNAL;
  }

  return (ret);
}

/**
  * @brief         Get signal strength and access techno information.
  * @param[in,out] p_signal_info - The signal info structure to contain the response.
  * @retval -
  */
void cellular_get_signal_info(cellular_signal_info_t *const p_signal_info)
{
  dc_signal_info_t        datacache_signal_info;           /* Cellular infos                                          */

  if (p_signal_info != NULL)
  {
    /* Get needed data from Data Cache */
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_SIGNAL_INFO, (void *)&datacache_signal_info, sizeof(dc_signal_info_t));

    /* Retrieve signal strength raw and in db and copy it to response structure */
    p_signal_info->signal_strength.raw_value = datacache_signal_info.cs_signal_level;
    p_signal_info->signal_strength.db_value = datacache_signal_info.cs_signal_level_db;

    /* Get access techno from Data Cache and copy to response structure */
    p_signal_info->access_techno = datacache_signal_info.access_techno;
  }
}

/**
  * @brief         Get SIM information.
  * @param[in,out] p_sim_info - The sim info structure to contain the response.
  * @retval -
  */
void cellular_get_sim_info(cellular_sim_info_t *const p_sim_info)
{
  dc_sim_info_t             datacache_sim_info;            /* Cellular sim infos                                      */
  dc_cellular_params_t      datacache_cellular_param;      /* Cellular parameters used to configure the modem.        */
  dc_cellular_info_t        datacache_cellular_info;
  uint8_t                   idx;                           /* Loop index                                              */

  if (p_sim_info != NULL)
  {
    /* Get needed data from Data Cache */
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&datacache_cellular_param, sizeof(dc_cellular_params_t));
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&datacache_cellular_info, sizeof(dc_cellular_info_t));
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_SIM_INFO, (void *)&datacache_sim_info, sizeof(dc_sim_info_t));

    /* Fill response structure */
    /* ICCID of SIM card */
    cellular_api_fill_string(datacache_cellular_info.iccid, CA_ICCID_SIZE_MAX, &p_sim_info->iccid.len,
                             p_sim_info->iccid.value);

    /* Actual IMSI used */
    cellular_api_fill_string(datacache_sim_info.imsi, CA_IMSI_SIZE_MAX, &p_sim_info->imsi.len, p_sim_info->imsi.value);

    /* Current index of sim used */
    p_sim_info->sim_index = datacache_sim_info.index_slot;

    /* Number of sim slots */
    p_sim_info->sim_slot_nb = datacache_cellular_param.sim_slot_nb;

    /* Parse all sim slots to get info */
    for (idx = 0; idx < datacache_cellular_param.sim_slot_nb; idx++)
    {
      /* Sim slot type */
      p_sim_info->sim_slot_type[idx] = datacache_cellular_param.sim_slot[idx].sim_slot_type;

      /* Sim status */
      p_sim_info->sim_status[idx] = datacache_sim_info.sim_status[idx];

      /* Apn_send_to_modem */
      p_sim_info->pdn[idx].apn_send_to_modem = datacache_cellular_param.sim_slot[idx].apnSendToModem;

      /* Cid */
      p_sim_info->pdn[idx].cid = datacache_cellular_param.sim_slot[idx].cid;

      /* APN */
      cellular_api_fill_string(datacache_cellular_param.sim_slot[idx].apn, CA_APN_SIZE_MAX,
                               &p_sim_info->pdn[idx].apn.len, p_sim_info->pdn[idx].apn.value);

      /* Username for APN */
      cellular_api_fill_string(datacache_cellular_param.sim_slot[idx].username, CA_USERNAME_SIZE_MAX,
                               &p_sim_info->pdn[idx].username.len, p_sim_info->pdn[idx].username.value);

      /* Password for APN */
      cellular_api_fill_string(datacache_cellular_param.sim_slot[idx].password, CA_PASSWORD_SIZE_MAX,
                               &p_sim_info->pdn[idx].password.len, p_sim_info->pdn[idx].password.value);
    } /* For loop */
  }
}

/**
  * @brief         Get NFMC information.
  * @param[in,out] p_nfmc_info - The nfmc info structure to contain the response.
  * @retval -
  */
void cellular_get_nfmc_info(cellular_nfmc_info_t *const p_nfmc_info)
{
  dc_nfmc_info_t           datacache_nfmc_info;            /* Cellular parameters used to configure the modem.        */

  if (p_nfmc_info != NULL)
  {
    /* Get needed data from Data Cache */
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_NFMC_INFO, (void *)&datacache_nfmc_info, sizeof(dc_nfmc_info_t));

    /* NMFC parameters */
    /* NFMC enable or not */
    p_nfmc_info->enable = (datacache_nfmc_info.activate == 1U);
    /* Number of NFMC tempo */
    p_nfmc_info->tempo_nb = CA_NFMC_VALUES_MAX_NB;
    /* Get all NFMC tempo values */
    for (uint8_t i = 0U; i < CA_NFMC_VALUES_MAX_NB ; i++)
    {
      /* No need to checks if the cellular API NFMC value array index "i" exists */
      /*  CA_NFMC_VALUES_MAX_NB should be equal to CA_NFMC_VALUES_MAX_NB   */
      p_nfmc_info->tempo_values[i] = datacache_nfmc_info.tempo[i];
    }
  }
}

/**
  * @brief     Set NFMC feature.
  * @param[in] nfmc_enable - false: NFMC is disable - true: NFMC is enable.
  * @param[in] nfmc_value_nb - The NFMC values number defined in p_nfmc_value.
  * @param[in] p_nfmc_value  - The new NFMC values to use for the calculation of NFMC tempo values.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_BADARGUMENT  Argument value not compliant.\n
  *                                      e.g nfmc_value_nb > CA_NFMC_VALUES_MAX_NB\n
  *            CELLULAR_ERR_INTERNAL     Error while executing X-Cube-Cellular internal function.
  * @note      if (result == CELLULAR_SUCCESS) then call cellular_get_nfmc_info() to have the new NFMC tempo values.
  */
cellular_result_t cellular_set_nfmc(bool nfmc_enable, uint8_t nfmc_value_nb, const uint32_t *const p_nfmc_value)
{
  dc_cellular_params_t      datacache_cellular_param;      /* Cellular parameters used to configure the modem.        */
  cellular_result_t         ret;                           /* Return value of the function                            */

  if (p_nfmc_value != NULL)
  {
    ret = CELLULAR_SUCCESS;
    /* First checks */
    if (nfmc_value_nb >= CA_NFMC_VALUES_MAX_NB)
    {
      ret = CELLULAR_ERR_BADARGUMENT;
    }

    if (ret == CELLULAR_SUCCESS)
    {
      /* Get existing data from Data Cache */
      if (dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&datacache_cellular_param,
                      sizeof(dc_cellular_params_t)) == DC_COM_OK)
      {
        if (datacache_cellular_param.rt_state == DC_SERVICE_ON)
        {
          /* Fill the Data Cache with the input parameters */
          /* NMFC enable */
          if (nfmc_enable == true)
          {
            datacache_cellular_param.nfmc_active = 1U;
          }
          else
          {
            datacache_cellular_param.nfmc_active = 0U;
          }
          /* Set all NFMC tempo values */
          for (uint8_t i = 0U; i < nfmc_value_nb ; i++)
          {
            datacache_cellular_param.nfmc_value[i] = p_nfmc_value[i];
          }
          /* Write modified data to Data Cache */
          if (dc_com_write(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&datacache_cellular_param,
                           sizeof(dc_cellular_params_t)) != DC_COM_OK)
          {
            /* If something went wrong, return an error */
            ret = CELLULAR_ERR_INTERNAL;
          }
        }
        else /* rt_state != DC_SERVICE_ON */
        {
          ret = CELLULAR_ERR_STATE;
        }
      }
      else
      {
        /* Dc_com_read return error */
        ret = CELLULAR_ERR_INTERNAL;
      }
    }
  }
  else
  {
    /* P_nfmc_value is NULL */
    ret = CELLULAR_ERR_BADARGUMENT;
  }
  return (ret);
}

/**
  * @brief         Get IP information.
  * @param[in,out] p_ip_info - The ip info structure to contain the response.
  * @retval -
  */
void cellular_get_ip_info(cellular_ip_info_t *const p_ip_info)
{
  dc_nifman_info_t         datacache_nifman_info;          /* Cellular parameters used to get IP address.             */

  if (p_ip_info != NULL)
  {
    /* Get needed data from Data Cache */
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_NIFMAN_INFO, (void *)&datacache_nifman_info, sizeof(dc_nifman_info_t));

    /* IP parameters */
    p_ip_info->ip_addr.addr = datacache_nifman_info.ip_addr.addr;
  }
}

#if (USE_LOW_POWER == 1)
/**
  * @brief         Get Power information.
  * @param[in,out] p_power_info - The power info structure to contain the response.
  * @retval -
  */
void cellular_get_power_info(cellular_power_info_t *const p_power_info)
{
  dc_cellular_power_status_t d_power_status;              /* Cellular parameters used to configure the modem.        */

  if (p_power_info != NULL)
  {
    /* Get existing data from Data Cache */
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_POWER_STATUS, (void *)&d_power_status,
                      sizeof(dc_cellular_power_status_t));

    /* Get needed information */
    /* Low power status */
    switch (d_power_status.power_state)
    {
      case DC_POWER_LOWPOWER_INACTIVE:
        p_power_info->power_state = CA_POWER_LOWPOWER_INACTIVE;
        break;
      case DC_POWER_LOWPOWER_ONGOING:
        p_power_info->power_state = CA_POWER_LOWPOWER_ONGOING;
        break;
      case DC_POWER_IN_LOWPOWER:
        p_power_info->power_state = CA_POWER_IN_LOWPOWER;
        break;
      default:
        /* Nothing to do */
        __NOP();
        break;
    }

    /* Periodic TAU T3412 */
    p_power_info->nwk_periodic_TAU = d_power_status.nwk_periodic_TAU;
    /* Active time T3324 */
    p_power_info->nwk_active_time = d_power_status.nwk_active_time;
  }
}

/**
  * @brief     Set Power feature.
  * @param[in] p_power_config - The new power configuration to use.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_BADARGUMENT  Argument value not compliant.\n
  *            CELLULAR_ERR_INTERNAL     Error while executing X-Cube-Cellular internal function.
  */
cellular_result_t cellular_set_power(const cellular_power_config_t *const p_power_config)
{
  dc_cellular_power_config_t     datacache_power_config;   /* Cellular parameters used to configure the modem.        */
  cellular_result_t              ret;                      /* Return value of the function                            */

  if (p_power_config != NULL)
  {
    ret = CELLULAR_SUCCESS;

    /* Get existing data from Data Cache */
    if (dc_com_read(&dc_com_db, DC_CELLULAR_POWER_CONFIG, (void *)&datacache_power_config,
                    sizeof(dc_cellular_power_config_t)) == DC_COM_OK)
    {
      if (datacache_power_config.rt_state == DC_SERVICE_ON)
      {
        /* Power command */
        datacache_power_config.power_cmd = p_power_config->power_cmd;

        /* Power mode */
        datacache_power_config.power_mode = p_power_config->power_mode;

        /* PSM present */
        datacache_power_config.psm_present = p_power_config->psm_present;

        /* EDRX present */
        datacache_power_config.edrx_present = p_power_config->edrx_present;

        /* PSM mode */
        datacache_power_config.psm_mode = p_power_config->psm_mode;

        /* Sleep timeout */
        datacache_power_config.sleep_request_timeout = p_power_config->sleep_request_timeout;

        /* PSM parameters */
        datacache_power_config.psm.req_periodic_RAU = p_power_config->psm.req_periodic_RAU;
        datacache_power_config.psm.req_GPRS_READY_timer = p_power_config->psm.req_GPRS_READY_timer;
        datacache_power_config.psm.req_periodic_TAU = p_power_config->psm.req_periodic_TAU;
        datacache_power_config.psm.req_active_time = p_power_config->psm.req_active_time;

        /* EIDRX mode */
        datacache_power_config.edrx_mode = p_power_config->eidrx_mode;

        /* EIDRX configuration : activation type, and value */
        datacache_power_config.edrx.act_type = p_power_config->eidrx.act_type;
        datacache_power_config.edrx.req_value = p_power_config->eidrx.req_value;

        /* Write modified data to Data Cache if no error detected */
        if (dc_com_write(&dc_com_db, DC_CELLULAR_POWER_CONFIG, (void *)&datacache_power_config,
                         sizeof(dc_cellular_power_config_t)) != DC_COM_OK)
        {
          /* If something went wrong, return an error */
          ret = CELLULAR_ERR_INTERNAL;
        }
      }
      else /* rt_state != DC_SERVICE_ON */
      {
        ret = CELLULAR_ERR_STATE;
      }
    }
    else
    {
      /* Dc_com_read return error */
      ret = CELLULAR_ERR_INTERNAL;
    }
  }
  else
  {
    /* P_power_config is NULL */
    ret = CELLULAR_ERR_BADARGUMENT;
  }
  return (ret);
}
#endif /* USE_LOW_POWER == 1 */

/**
  * @brief     Register a callback that will be called when Cellular information is updated.
  * @param[in] cellular_info_cb        - The callback to register.
  *                                      If set to NULL no data will be provided to the call back.
  * @param[in] p_callback_ctx          - The context to be passed when cellular_info_cb callback is called.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_NOMEMORY     No more memory to register the callback.\n
  *            CELLULAR_ERR_INTERNAL     Error while executing X-Cube-Cellular internal function.
  */
cellular_result_t cellular_info_cb_registration(cellular_info_cb_t cellular_info_cb, void *const p_callback_ctx)
{
  uint8_t idx;
  uint8_t free_idx = 0xFFU;
  cellular_result_t ret = CELLULAR_SUCCESS;

  /* Parse the call back information to find */
  /* - If actual new call back to be registered if already registered. */
  /* - the first empty place to register the new call back */
  for (idx = 0; idx < CELLULAR_CONCURENT_REGISTRATION_CB_MAX_NB; idx++)
  {
    if (cellular_api_registration_cb[idx].cellular_info_cb == cellular_info_cb)
    {
      /* Actual new call back already exists, do not re-register it */
      ret = CELLULAR_ERR_BADARGUMENT;
    }
    if ((cellular_api_registration_cb[idx].cellular_info_cb == NULL) && (free_idx == 0xFFU))
    {
      /* found the first free empty place to store new call back. store the index */
      free_idx = idx;
    }
  }

  if ((free_idx >= CELLULAR_CONCURENT_REGISTRATION_CB_MAX_NB))
  {
    /* did not found an empty place to store new call back */
    ret = CELLULAR_ERR_INTERNAL;
  }
  else if (ret == CELLULAR_SUCCESS)
  {
    /* if everything is OK, register the new call back */
    if (cellular_api_register_general_cb() == CELLULAR_SUCCESS)
    {
      cellular_api_registration_cb[free_idx].cellular_info_cb = cellular_info_cb;
      cellular_api_registration_cb[free_idx].p_cellular_info_cb_ctx = p_callback_ctx;
    }
    else
    {
      /* cellular_api_register_general_cb returned error, can't subscribe to Data Cache */
      ret = CELLULAR_ERR_INTERNAL;
    }
  }
  else
  {
    __NOP(); /* Nothing to do */
  }

  return (ret);
}

/**
  * @brief     Deregister a Cellular information callback.
  * @param[in] cellular_info_cb        - The callback to deregister.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_BADARGUMENT  The callback to deregister is unknown.
 */
cellular_result_t cellular_info_cb_deregistration(cellular_info_cb_t cellular_info_cb)
{
  uint8_t idx;
  cellular_result_t ret = CELLULAR_ERR_BADARGUMENT;

  /* Parse the call back information to find the call back to be deregistered. */
  for (idx = 0; idx < CELLULAR_CONCURENT_REGISTRATION_CB_MAX_NB; idx++)
  {
    if (cellular_api_registration_cb[idx].cellular_info_cb == cellular_info_cb)
    {
      /* Call back to be deregistered found. Remove if from call back information */
      cellular_api_registration_cb[idx].cellular_info_cb = NULL;
      cellular_api_registration_cb[idx].p_cellular_info_cb_ctx = NULL;
      ret = CELLULAR_SUCCESS;
    }
  }

  return (ret);
}

/**
  * @brief     Register a callback that will be called when Network Signal information is updated.
  * @param[in] cellular_signal_info_cb - The callback to register.
  * @param[in] p_callback_ctx          - The context to be passed when cellular_signal_info_cb callback is called.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_NOMEMORY     No more memory to register the callback.
  */
cellular_result_t cellular_signal_info_cb_registration(cellular_signal_info_cb_t cellular_signal_info_cb,
                                                       void *const p_callback_ctx)
{
  uint8_t idx;
  uint8_t free_idx = 0xFFU;
  cellular_result_t ret = CELLULAR_SUCCESS;

  /* Parse the call back information to find */
  /* - If actual new call back to be registered if already registered. */
  /* - the first empty place to register the new call back */
  for (idx = 0; idx < CELLULAR_CONCURENT_REGISTRATION_CB_MAX_NB; idx++)
  {
    if (cellular_api_registration_cb[idx].cellular_signal_info_cb == cellular_signal_info_cb)
    {
      /* Actual new call back already exists, do not re-register it */
      ret = CELLULAR_ERR_BADARGUMENT;
    }
    if ((cellular_api_registration_cb[idx].cellular_signal_info_cb == NULL) && (free_idx == 0xFFU))
    {
      /* found the first free empty place to store new call back. store the index */
      free_idx = idx;
    }
  }

  if ((free_idx >= CELLULAR_CONCURENT_REGISTRATION_CB_MAX_NB))
  {
    /* did not found an empty place to store new call back */
    ret = CELLULAR_ERR_INTERNAL;
  }
  else if (ret == CELLULAR_SUCCESS)
  {
    /* if everything is OK, register the new call back */
    if (cellular_api_register_general_cb() == CELLULAR_SUCCESS)
    {
      cellular_api_registration_cb[free_idx].cellular_signal_info_cb = cellular_signal_info_cb;
      cellular_api_registration_cb[free_idx].p_cellular_signal_info_cb_ctx = p_callback_ctx;
    }
    else
    {
      /* cellular_api_register_general_cb returned error, can't subscribe to Data Cache */
      ret = CELLULAR_ERR_INTERNAL;
    }
  }
  else
  {
    __NOP(); /* Nothing to do */
  }

  return (ret);
}

/**
  * @brief     Deregister a Network Signal information callback.
  * @param[in] cellular_signal_info_cb - The callback to deregister.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_BADARGUMENT  The callback to deregister is unknown.
 */
cellular_result_t cellular_signal_info_cb_deregistration(cellular_signal_info_cb_t cellular_signal_info_cb)
{
  uint8_t idx;
  cellular_result_t ret = CELLULAR_ERR_BADARGUMENT;

  /* Parse the call back information to find the call back to be deregistered. */
  for (idx = 0; idx < CELLULAR_CONCURENT_REGISTRATION_CB_MAX_NB; idx++)
  {
    if (cellular_api_registration_cb[idx].cellular_signal_info_cb == cellular_signal_info_cb)
    {
      /* Call back to be deregistered found. Remove if from call back information */
      cellular_api_registration_cb[idx].cellular_signal_info_cb = NULL;
      cellular_api_registration_cb[idx].p_cellular_signal_info_cb_ctx = NULL;
      ret = CELLULAR_SUCCESS;
    }
  }

  return (ret);
}

/**
  * @brief     Register a callback that will be called when SIM information is updated.
  * @param[in] cellular_sim_info_cb    - The callback to register.
  * @param[in] p_callback_ctx          - The context to be passed when cellular_sim_info_cb callback is called.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_NOMEMORY     No more memory to register the callback.
  */
cellular_result_t cellular_sim_info_cb_registration(cellular_sim_info_cb_t cellular_sim_info_cb,
                                                    void *const p_callback_ctx)
{
  uint8_t idx;
  uint8_t free_idx = 0xFFU;
  cellular_result_t ret = CELLULAR_SUCCESS;

  /* Parse the call back information to find */
  /* - If actual new call back to be registered if already registered. */
  /* - the first empty place to register the new call back */
  for (idx = 0; idx < CELLULAR_CONCURENT_REGISTRATION_CB_MAX_NB; idx++)
  {
    if (cellular_api_registration_cb[idx].cellular_sim_info_cb == cellular_sim_info_cb)
    {
      /* Actual new call back already exists, do not re-register it */
      ret = CELLULAR_ERR_BADARGUMENT;
    }
    if ((cellular_api_registration_cb[idx].cellular_sim_info_cb == NULL) && (free_idx == 0xFFU))
    {
      /* found the first free empty place to store new call back. store the index */
      free_idx = idx;
    }
  }

  if ((free_idx >= CELLULAR_CONCURENT_REGISTRATION_CB_MAX_NB))
  {
    /* did not found an empty place to store new call back */
    ret = CELLULAR_ERR_INTERNAL;
  }
  else if (ret == CELLULAR_SUCCESS)
  {
    /* if everything is OK, register the new call back */
    if (cellular_api_register_general_cb() == CELLULAR_SUCCESS)
    {
      cellular_api_registration_cb[free_idx].cellular_sim_info_cb = cellular_sim_info_cb;
      cellular_api_registration_cb[free_idx].p_cellular_sim_info_cb_ctx = p_callback_ctx;
    }
    else
    {
      /* cellular_api_register_general_cb returned error, can't subscribe to Data Cache */
      ret = CELLULAR_ERR_INTERNAL;
    }
  }
  else
  {
    __NOP(); /* Nothing to do */
  }

  return (ret);
}

/**
  * @brief     Deregister a SIM information callback.
  * @param[in] cellular_sim_info_cb    - The callback to deregister.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_BADARGUMENT  The callback to deregister is unknown.
 */
cellular_result_t cellular_sim_info_cb_deregistration(cellular_sim_info_cb_t cellular_sim_info_cb)
{
  uint8_t idx;
  cellular_result_t ret = CELLULAR_ERR_BADARGUMENT;

  /* Parse the call back information to find the call back to be deregistered. */
  for (idx = 0; idx < CELLULAR_CONCURENT_REGISTRATION_CB_MAX_NB; idx++)
  {
    if (cellular_api_registration_cb[idx].cellular_sim_info_cb == cellular_sim_info_cb)
    {
      /* Call back to be deregistered found. Remove if from call back information */
      cellular_api_registration_cb[idx].cellular_sim_info_cb = NULL;
      cellular_api_registration_cb[idx].p_cellular_sim_info_cb_ctx = NULL;
      ret = CELLULAR_SUCCESS;
    }
  }

  return (ret);
}

/**
  * @brief     Register a callback that will be called when NFMC information is updated.
  * @param[in] cellular_nfmc_info_cb   - The callback to register.
  * @param[in] p_callback_ctx          - The context to be passed when cellular_nfmc_info_cb callback is called.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_NOMEMORY     No more memory to register the callback.
  */
cellular_result_t cellular_nfmc_info_cb_registration(cellular_nfmc_info_cb_t cellular_nfmc_info_cb,
                                                     void *const p_callback_ctx)
{
  uint8_t idx;
  uint8_t free_idx = 0xFFU;
  cellular_result_t ret = CELLULAR_SUCCESS;

  /* Parse the call back information to find */
  /* - If actual new call back to be registered if already registered. */
  /* - the first empty place to register the new call back */
  for (idx = 0; idx < CELLULAR_CONCURENT_REGISTRATION_CB_MAX_NB; idx++)
  {
    if (cellular_api_registration_cb[idx].cellular_nfmc_info_cb == cellular_nfmc_info_cb)
    {
      /* Actual new call back already exists, do not re-register it */
      ret = CELLULAR_ERR_BADARGUMENT;
    }
    if ((cellular_api_registration_cb[idx].cellular_nfmc_info_cb == NULL) && (free_idx == 0xFFU))
    {
      /* found the first free empty place to store new call back. store the index */
      free_idx = idx;
    }
  }

  if ((free_idx >= CELLULAR_CONCURENT_REGISTRATION_CB_MAX_NB))
  {
    /* did not found an empty place to store new call back */
    ret = CELLULAR_ERR_INTERNAL;
  }
  else if (ret == CELLULAR_SUCCESS)
  {
    /* if everything is OK, register the new call back */
    if (cellular_api_register_general_cb() == CELLULAR_SUCCESS)
    {
      cellular_api_registration_cb[free_idx].cellular_nfmc_info_cb = cellular_nfmc_info_cb;
      cellular_api_registration_cb[free_idx].p_cellular_nfmc_info_cb_ctx = p_callback_ctx;
    }
    else
    {
      /* cellular_api_register_general_cb returned error, can't subscribe to Data Cache */
      ret = CELLULAR_ERR_INTERNAL;
    }
  }
  else
  {
    __NOP(); /* Nothing to do */
  }

  return (ret);
}

/**
  * @brief     Deregister a NFMC information callback.
  * @param[in] cellular_nfmc_info_cb   - The callback to deregister.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_BADARGUMENT  The callback to deregister is unknown.
  */
cellular_result_t cellular_nfmc_info_cb_deregistration(cellular_nfmc_info_cb_t cellular_nfmc_info_cb)
{
  uint8_t idx;
  cellular_result_t ret = CELLULAR_ERR_BADARGUMENT;

  /* Parse the call back information to find the call back to be deregistered. */
  for (idx = 0; idx < CELLULAR_CONCURENT_REGISTRATION_CB_MAX_NB; idx++)
  {
    if (cellular_api_registration_cb[idx].cellular_nfmc_info_cb == cellular_nfmc_info_cb)
    {
      /* Call back to be deregistered found. Remove if from call back information */
      cellular_api_registration_cb[idx].cellular_nfmc_info_cb = NULL;
      cellular_api_registration_cb[idx].p_cellular_nfmc_info_cb_ctx = NULL;
      ret = CELLULAR_SUCCESS;
    }
  }

  return (ret);
}

/**
  * @brief     Register a callback that will be called when IP information is updated.
  * @param[in] cellular_ip_info_cb     - The callback to register.
  * @param[in] p_callback_ctx          - The context to be passed when cellular_ip_info_cb callback is called.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_NOMEMORY     No more memory to register the callback.
  */
cellular_result_t cellular_ip_info_cb_registration(cellular_ip_info_cb_t cellular_ip_info_cb,
                                                   void *const p_callback_ctx)
{
  uint8_t idx;
  uint8_t free_idx = 0xFFU;
  cellular_result_t ret = CELLULAR_SUCCESS;

  /* Parse the call back information to find */
  /* - If actual new call back to be registered if already registered. */
  /* - the first empty place to register the new call back */
  for (idx = 0; idx < CELLULAR_CONCURENT_REGISTRATION_CB_MAX_NB; idx++)
  {
    if (cellular_api_registration_cb[idx].cellular_ip_info_cb == cellular_ip_info_cb)
    {
      /* Actual new call back already exists, do not re-register it */
      ret = CELLULAR_ERR_BADARGUMENT;
    }
    if ((cellular_api_registration_cb[idx].cellular_ip_info_cb == NULL) && (free_idx == 0xFFU))
    {
      /* found the first free empty place to store new call back. store the index */
      free_idx = idx;
    }
  }

  if ((free_idx >= CELLULAR_CONCURENT_REGISTRATION_CB_MAX_NB))
  {
    /* did not found an empty place to store new call back */
    ret = CELLULAR_ERR_INTERNAL;
  }
  else if (ret == CELLULAR_SUCCESS)
  {
    /* if everything is OK, register the new call back */
    if (cellular_api_register_general_cb() == CELLULAR_SUCCESS)
    {
      cellular_api_registration_cb[free_idx].cellular_ip_info_cb = cellular_ip_info_cb;
      cellular_api_registration_cb[free_idx].p_cellular_ip_info_cb_ctx = p_callback_ctx;
    }
    else
    {
      /* cellular_api_register_general_cb returned error, can't subscribe to Data Cache */
      ret = CELLULAR_ERR_INTERNAL;
    }
  }
  else
  {
    __NOP(); /* Nothing to do */
  }

  return (ret);
}

/**
  * @brief     Deregister a IP information callback.
  * @param[in] cellular_ip_info_cb     - The callback to deregister.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_BADARGUMENT  The callback to deregister is unknown.
 */
cellular_result_t cellular_ip_info_cb_deregistration(cellular_ip_info_cb_t cellular_ip_info_cb)
{
  uint8_t idx;
  cellular_result_t ret = CELLULAR_ERR_BADARGUMENT;

  /* Parse the call back information to find the call back to be deregistered. */
  for (idx = 0; idx < CELLULAR_CONCURENT_REGISTRATION_CB_MAX_NB; idx++)
  {
    if (cellular_api_registration_cb[idx].cellular_ip_info_cb == cellular_ip_info_cb)
    {
      /* Call back to be deregistered found. Remove if from call back information */
      cellular_api_registration_cb[idx].cellular_ip_info_cb = NULL;
      cellular_api_registration_cb[idx].p_cellular_ip_info_cb_ctx = NULL;
      ret = CELLULAR_SUCCESS;
    }
  }

  return (ret);
}

#if (USE_LOW_POWER == 1)
/**
  * @brief     Register a callback that will be called when Power information is updated.
  * @param[in] cellular_power_info_cb  - The callback to register.
  * @param[in] p_callback_ctx          - The context to be passed when cellular_power_info_cb callback is called.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_NOMEMORY     No more memory to register the callback.
  */
cellular_result_t cellular_power_info_cb_registration(cellular_power_info_cb_t cellular_power_info_cb,
                                                      void *const p_callback_ctx)
{
  uint8_t idx;
  uint8_t free_idx = 0xFFU;
  cellular_result_t ret = CELLULAR_SUCCESS;

  /* Parse the call back information to find */
  /* - If actual new call back to be registered if already registered. */
  /* - the first empty place to register the new call back */
  for (idx = 0; idx < CELLULAR_CONCURENT_REGISTRATION_CB_MAX_NB; idx++)
  {
    if (cellular_api_registration_cb[idx].cellular_power_info_cb == cellular_power_info_cb)
    {
      /* Actual new call back already exists, do not re-register it */
      ret = CELLULAR_ERR_BADARGUMENT;
    }
    if ((cellular_api_registration_cb[idx].cellular_power_info_cb == NULL) && (free_idx == 0xFFU))
    {
      /* found the first free empty place to store new call back. store the index */
      free_idx = idx;
    }
  }

  if ((free_idx >= CELLULAR_CONCURENT_REGISTRATION_CB_MAX_NB))
  {
    /* did not found an empty place to store new call back */
    ret = CELLULAR_ERR_INTERNAL;
  }
  else if (ret == CELLULAR_SUCCESS)
  {
    /* if everything is OK, register the new call back */
    if (cellular_api_register_general_cb() == CELLULAR_SUCCESS)
    {
      cellular_api_registration_cb[free_idx].cellular_power_info_cb = cellular_power_info_cb;
      cellular_api_registration_cb[free_idx].p_cellular_power_info_cb_ctx = p_callback_ctx;
    }
    else
    {
      /* cellular_api_register_general_cb returned error, can't subscribe to Data Cache */
      ret = CELLULAR_ERR_INTERNAL;
    }
  }
  else
  {
    __NOP(); /* Nothing to do */
  }

  return (ret);
}

/**
  * @brief     Deregister a Power information callback.
  * @param[in] cellular_power_info_cb  - The callback to deregister.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_BADARGUMENT  The callback to deregister is unknown.
 */
cellular_result_t cellular_power_info_cb_deregistration(cellular_power_info_cb_t cellular_power_info_cb)
{
  uint8_t idx;
  cellular_result_t ret = CELLULAR_ERR_BADARGUMENT;

  /* Parse the call back information to find the call back to be deregistered. */
  for (idx = 0; idx < CELLULAR_CONCURENT_REGISTRATION_CB_MAX_NB; idx++)
  {
    if (cellular_api_registration_cb[idx].cellular_power_info_cb == cellular_power_info_cb)
    {
      /* Call back to be deregistered found. Remove if from call back information */
      cellular_api_registration_cb[idx].cellular_power_info_cb = NULL;
      cellular_api_registration_cb[idx].p_cellular_power_info_cb_ctx = NULL;
      ret = CELLULAR_SUCCESS;
    }
  }

  return (ret);
}

#endif /* USE_LOW_POWER == 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
