/**
  ******************************************************************************
  * @file    cellular_mngt.c
  * @author  MCD Application Team
  * @brief   Management of cellular components
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
#include <stdbool.h>

#include "plf_config.h"
#include "plf_cellular_config.h"
#include "cellular_mngt.h"

#include "cmsis_os_misrac2012.h"

#include "ipc_uart.h"
#include "com_core.h"
#include "cellular_datacache.h"
#if (USE_LOW_POWER == 1)
#include "cellular_service_power.h"
#endif  /* (USE_LOW_POWER == 1) */
#include "cellular_service_task.h"
#include "app_select.h"
#include "setup.h"
#include "nifman.h"
#include "dc_common.h"
#include "cellular_runtime_standard.h"

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
#include "ppposif.h"
#include "ppposif_client.h"
#endif  /* USE_SOCKETS_TYPE == USE_SOCKETS_LWIP */

/* Private defines -----------------------------------------------------------*/
#if (USE_TRACE_CELLULAR_MNGT == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_INFO(format, args...) \
  TRACE_PRINT(DBG_CHAN_MAIN, DBL_LVL_P0, "CellMngt: " format "\n\r", ## args)

#else /* USE_PRINTF == 1U */
#include <stdio.h>
#define PRINT_INFO(format, args...)      (void)printf("CellMngt: " format "\n\r", ## args);

#endif /* USE_PRINTF == 0U */

#else /* USE_TRACE_CELLULAR_MNGT == 0U */
#define PRINT_INFO(...) __NOP(); /* Nothing to do */

#endif /* USE_TRACE_CELLULAR_MNGT == 1U */


/* Private macros ------------------------------------------------------------*/
#define CELLULAR_MNGT_FAQ_LABEL "FAQ display"

#if (USE_NETWORK_LIBRARY == 1)
#define STATE_TRANSITION_TIMEOUT        10000
#endif  /* (USE_NETWORK_LIBRARY == 1) */

/* Private variables ---------------------------------------------------------*/
#if (USE_NETWORK_LIBRARY == 1)
static net_if_handle_t netif;
static void hnet_notify(void *context, uint32_t event_class, uint32_t event_id, void  *event_data);
static net_if_notify_func cellular_notify_func = NULL;
#endif /* (USE_NETWORK_LIBRARY == 1) */

/* Private typedef -----------------------------------------------------------*/
#if (USE_NETWORK_LIBRARY == 1)
typedef uint8_t cellular_mngt_net_state_t ;
#define CELLULAR_MNGT_NET_STATE_DEINITIALIZED       ((cellular_mngt_net_state_t)NET_STATE_DEINITIALIZED)
#define CELLULAR_MNGT_NET_STATE_INITIALIZED         ((cellular_mngt_net_state_t)NET_STATE_INITIALIZED)
#define CELLULAR_MNGT_NET_STATE_STARTING            ((cellular_mngt_net_state_t)NET_STATE_STARTING)
#define CELLULAR_MNGT_NET_STATE_READY               ((cellular_mngt_net_state_t)NET_STATE_READY)
#define CELLULAR_MNGT_NET_STATE_CONNECTING          ((cellular_mngt_net_state_t)NET_STATE_CONNECTING)
#define CELLULAR_MNGT_NET_STATE_CONNECTED           ((cellular_mngt_net_state_t)NET_STATE_CONNECTED)
#define CELLULAR_MNGT_NET_STATE_STOPPING            ((cellular_mngt_net_state_t)NET_STATE_STOPPING)
#define CELLULAR_MNGT_NET_STATE_DISCONNECTING       ((cellular_mngt_net_state_t)NET_STATE_DISCONNECTING)
#define CELLULAR_MNGT_NET_STATE_CONNECTION_LOST     ((cellular_mngt_net_state_t)NET_STATE_CONNECTION_LOST)
#endif /* (USE_NETWORK_LIBRARY == 1) */

/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void cellular_datacache_init(void);
#if (!USE_DEFAULT_SETUP == 1)
static void  cellular_faq_init(void);
static bool cellular_faq_start(void);
#endif /* !USE_DEFAULT_SETUP == 1 */

/* Global variables ----------------------------------------------------------*/

/* definition of Data Cache cellular entries  */
dc_com_res_id_t    DC_CELLULAR_INFO             = DC_COM_INVALID_ENTRY;
dc_com_res_id_t    DC_CELLULAR_DATA_INFO        = DC_COM_INVALID_ENTRY;
dc_com_res_id_t    DC_CELLULAR_NIFMAN_INFO      = DC_COM_INVALID_ENTRY;
dc_com_res_id_t    DC_CELLULAR_NFMC_INFO        = DC_COM_INVALID_ENTRY;
dc_com_res_id_t    DC_CELLULAR_SIM_INFO         = DC_COM_INVALID_ENTRY;
dc_com_res_id_t    DC_CELLULAR_CONFIG           = DC_COM_INVALID_ENTRY;
dc_com_res_id_t    DC_CELLULAR_TARGET_STATE_CMD = DC_COM_INVALID_ENTRY;
dc_com_res_id_t    DC_CELLULAR_APN_CONFIG       = DC_COM_INVALID_ENTRY;
#if (USE_LOW_POWER == 1)
dc_com_res_id_t    DC_CELLULAR_POWER_CONFIG     = DC_COM_INVALID_ENTRY;
#endif /* (USE_LOW_POWER == 1) */
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
dc_com_res_id_t    DC_PPP_CLIENT_INFO           = DC_COM_INVALID_ENTRY;
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */

/* Functions Definition ------------------------------------------------------*/
#if (USE_NETWORK_LIBRARY == 1)

/**
  * @brief  network library callback - Used to know network status
  * @param  context - net_if_handle_t
  * @param  event_class - net_evt_t
  * @param  event_id - cellular_mngt_net_state_t
  * @param  event_data - event data
  * @retval -
  */

static void hnet_notify(void *context, uint32_t event_class, uint32_t event_id, void  *event_data)
{
  net_if_handle_t *netif_l = context;

  if (event_class == (uint32_t)NET_EVENT_STATE_CHANGE)
  {
    cellular_mngt_net_state_t new_state = (cellular_mngt_net_state_t) event_id;
    switch (new_state)
    {
      case CELLULAR_MNGT_NET_STATE_INITIALIZED:
        PRINT_INFO("- Network Interface initialized: \n");
        break;

      case CELLULAR_MNGT_NET_STATE_STARTING:
        PRINT_INFO("- Network Interface starting: \n");
        break;

      case CELLULAR_MNGT_NET_STATE_READY:
        PRINT_INFO("- Network Interface ready: \n");
        PRINT_INFO("   - Device Name : %s. \n", netif_l->DeviceName);
        PRINT_INFO("   - Device ID : %s. \n", netif_l->DeviceID);
        PRINT_INFO("   - Device Version : %s. \n", netif_l->DeviceVer);
#ifndef STM32L496xx
        PRINT_INFO("   - MAC address: %02X:%02X:%02X:%02X:%02X:%02X. \n", netif_l->macaddr.mac[0],
                   netif_l->macaddr.mac[1], netif_l->macaddr.mac[2], netif_l->macaddr.mac[3],
                   netif_l->macaddr.mac[4], netif_l->macaddr.mac[5]);
#endif /* #ifndef STM32L496xx */
        (void)net_if_connect(netif_l);
        break;

      case CELLULAR_MNGT_NET_STATE_CONNECTING:
        PRINT_INFO("- Network Interface connecting: \n");
        break;

      case CELLULAR_MNGT_NET_STATE_CONNECTED:
        PRINT_INFO("- Network Interface connected: \n");
        PRINT_INFO("   - IP address :  %s. \n", net_ntoa(&netif_l->ipaddr));
        break;

      case CELLULAR_MNGT_NET_STATE_DISCONNECTING:
        PRINT_INFO("- Network Interface disconnecting\n");
        break;

      case CELLULAR_MNGT_NET_STATE_CONNECTION_LOST:
        PRINT_INFO("- Network Interface disconnected\n");
        break;

      case CELLULAR_MNGT_NET_STATE_STOPPING:
        PRINT_INFO("- Network Interface stopping\n");
        break;

      case CELLULAR_MNGT_NET_STATE_DEINITIALIZED:
        PRINT_INFO("- Network Interface de-initialized\n");
        break;

      default:
        break;
    }
  }

  if (cellular_notify_func != NULL)
  {
    cellular_notify_func(context,  event_class, event_id, event_data);
  }
}
#endif /* (USE_NETWORK_LIBRARY == 1) */

#if (!USE_DEFAULT_SETUP == 1)
/**
  * @brief  Display Cellular FAQ
  * @param  -
  * @retval -
  */
static bool cellular_faq_start(void)
{
  uint8_t car;

  PRINT_SETUP("\n\r\n\r")
  PRINT_SETUP("---------------\n\r")
  PRINT_SETUP("  Cellular FAQ\n\r")
  PRINT_SETUP("---------------\n\r")
  PRINT_SETUP("Q:  Where is the complete FAQ?\n\r")
  PRINT_SETUP("R:  In the release note.\n\r")
  PRINT_SETUP("\n\r")
  PRINT_SETUP("Q:  It boots but there is an issue with the network?\n\r")
  PRINT_SETUP("R:  Check that the correct SIM is selected\n\r")
  PRINT_SETUP("    (Plastic SIM or soldered UICC; Plastic SIM bydefault)\n\r")
  PRINT_SETUP("\n\r")
  PRINT_SETUP("\n\r")
  PRINT_SETUP("Type any key to continue\n\r")

  /* Waiting for a key to reboot the board */
  (void)menu_utils_get_uart_char(&car);

  return false;
}
#endif   /* (!USE_DEFAULT_SETUP == 1) */

/**
  * @brief  Initialize all cellular entries of Data Cache
  * @param  -
  * @retval -
  */
static void cellular_datacache_init(void)
{
  static dc_cellular_info_t          dc_cellular_info;
  static dc_ppp_client_info_t        dc_ppp_client_info;
  static dc_cellular_data_info_t     dc_cellular_data_info;
  static dc_nifman_info_t            dc_nifman_info;
  static dc_nfmc_info_t              dc_nfmc_info;
  static dc_sim_info_t               dc_sim_info;
  static dc_cellular_params_t        dc_cellular_params;
  static dc_cellular_target_state_t  dc_cellular_target_state;
  static dc_apn_config_t             dc_apn_config;
#if (USE_LOW_POWER == 1)
  static dc_cellular_power_config_t  dc_cellular_power_config;
#endif  /* (USE_LOW_POWER == 1) */

  (void)memset((void *)&dc_cellular_info,         0, sizeof(dc_cellular_info_t));
  (void)memset((void *)&dc_ppp_client_info,       0, sizeof(dc_ppp_client_info_t));
  (void)memset((void *)&dc_cellular_data_info,    0, sizeof(dc_cellular_data_info_t));
  (void)memset((void *)&dc_nifman_info,           0, sizeof(dc_nifman_info_t));
  (void)memset((void *)&dc_nfmc_info,             0, sizeof(dc_nfmc_info_t));
  (void)memset((void *)&dc_sim_info,              0, sizeof(dc_sim_info_t));
  (void)memset((void *)&dc_cellular_params,       0, sizeof(dc_cellular_params_t));
  (void)memset((void *)&dc_cellular_target_state, 0, sizeof(dc_cellular_target_state_t));
  (void)memset((void *)&dc_apn_config,            0, sizeof(dc_apn_config_t));
#if (USE_LOW_POWER == 1)
  (void)memset((void *)&dc_cellular_power_config, 0, sizeof(dc_cellular_power_config_t));
#endif  /* (USE_LOW_POWER == 1) */

  /* register all all cellular entries of Data Cache */
  DC_CELLULAR_INFO             = dc_com_register_serv(&dc_com_db, (void *)&dc_cellular_info,
                                                      (uint16_t)sizeof(dc_cellular_info_t));
  DC_CELLULAR_DATA_INFO        = dc_com_register_serv(&dc_com_db, (void *)&dc_cellular_data_info,
                                                      (uint16_t)sizeof(dc_cellular_data_info_t));
  DC_CELLULAR_NIFMAN_INFO      = dc_com_register_serv(&dc_com_db, (void *)&dc_nifman_info,
                                                      (uint16_t)sizeof(dc_nifman_info_t));
  DC_CELLULAR_NFMC_INFO        = dc_com_register_serv(&dc_com_db, (void *)&dc_nfmc_info,
                                                      (uint16_t)sizeof(dc_nfmc_info_t));
  DC_CELLULAR_SIM_INFO         = dc_com_register_serv(&dc_com_db, (void *)&dc_sim_info,
                                                      (uint16_t)sizeof(dc_sim_info_t));
  DC_CELLULAR_CONFIG           = dc_com_register_serv(&dc_com_db, (void *)&dc_cellular_params,
                                                      (uint16_t)sizeof(dc_cellular_params_t));
  DC_CELLULAR_TARGET_STATE_CMD = dc_com_register_serv(&dc_com_db, (void *)&dc_cellular_target_state,
                                                      (uint16_t)sizeof(dc_cellular_target_state_t));
  DC_CELLULAR_APN_CONFIG       = dc_com_register_serv(&dc_com_db, (void *)&dc_apn_config,
                                                      (uint16_t)sizeof(dc_apn_config));
#if (USE_LOW_POWER == 1)
  DC_CELLULAR_POWER_CONFIG     = dc_com_register_serv(&dc_com_db, (void *)&dc_cellular_power_config,
                                                      (uint16_t)sizeof(dc_cellular_power_config));
#endif  /* (USE_LOW_POWER == 1) */
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  DC_PPP_CLIENT_INFO           = dc_com_register_serv(&dc_com_db, (void *)&dc_ppp_client_info,
                                                      (uint16_t)sizeof(dc_ppp_client_info_t));
#endif   /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
}

#if (!USE_DEFAULT_SETUP == 1)
/**
  * @brief  Record FAQ to boot menu
  * @param  -
  * @retval -
  */

static void cellular_faq_init(void)
{
  (void)app_select_record((uint8_t *)CELLULAR_MNGT_FAQ_LABEL, cellular_faq_start);
}
#endif /* (!USE_DEFAULT_SETUP == 1) */

/**
  * @brief  Initialize cellular features
  * @param  -
  * @retval -
  */
void cellular_init(void)
{
#if (USE_PRINTF == 0U)
  /* Error Handler in the modules below may use trace print */
  /* Recall traceIF_init() in case StartDefaultTask is not used or is redefined */
  traceIF_init();
#endif /* (USE_PRINTF == 0U)  */

#if (USE_STACK_ANALYSIS == 1)
  /* Recall stackAnalysis_init() in case StartDefaultTask is not used or is redefined */
  stackAnalysis_init();
#endif /* USE_STACK_ANALYSIS == 1 */

  /* Data Cache initialization */
  (void)dc_com_init();

  /* cellular entries of Data Cache initialization */
  cellular_datacache_init();

  /* communication interface initialization */
  (void)com_init();

  /* cellular service initialization */
  (void)CST_cellular_service_init();

#if (!USE_DEFAULT_SETUP == 1)
  /* FAQ menu record */
  cellular_faq_init();
#endif   /* (!USE_DEFAULT_SETUP == 1) */

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  /* if LWIP is used, PPP component initialization */
  (void)ppposif_client_init();
#endif  /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */

  /* Network InteFace MANager initialization */
  (void)nifman_init();
}

/**
  * @brief  Start cellular features with boot modem and network registration
  * @param  -
  * @retval -
  */
void cellular_start(void)
{
#if (USE_PRINTF == 0U)
  traceIF_start();
#endif /* (USE_PRINTF == 0U) */

#if (USE_STACK_ANALYSIS == 1)
  stackAnalysis_start();
#endif /* USE_STACK_ANALYSIS == 1 */

  dc_com_start();

#if (USE_LOW_POWER == 1)
  CSP_Start();
#endif  /* (USE_LOW_POWER == 1) */
  (void)com_start();

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  (void)ppposif_client_start();
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */

  (void)nifman_start();

  /* dynamical init of components */
#if defined(AT_TEST)
  at_modem_start();
#endif   /* defined(AT_TEST) */

#if !defined(AT_TEST)
  (void)CST_cellular_service_start();
#endif  /* !defined(AT_TEST) */

#if !defined(AT_TEST)
  (void)CST_radio_on();
#endif   /*  !defined(AT_TEST) */
}

/**
  * @brief  Start cellular feature with boot modem (NO network registration)
  *         Usage: configure the modem
  * @param  -
  * @retval -
  */
void cellular_modem_start(void)
{
  (void)CST_cellular_service_start();
  (void)CST_modem_power_on();
}

/**
  * @brief  Initialize dc_cellular_params_t structure with default value
  * @note   this function should be called by application before to store its own values
  * @param  cellular_params - cellular configuration
  * @retval -
  */
void cellular_set_default_setup_config(dc_cellular_params_t *cellular_params)
{
  dc_sim_slot_t *dc_sim_slot;

  dc_sim_slot = &cellular_params->sim_slot[0];

  /* Network Library Initialization */
  cellular_params->set_pdn_mode              = 1U; /* PDN to set */
  cellular_params->sim_slot_nb               = 1U; /* Number of SIM slot used */
  dc_sim_slot->sim_slot_type                 = DC_SIM_SLOT_MODEM_SOCKET;
  dc_sim_slot->cid                           = 1U;
  cellular_params->target_state              = DC_TARGET_STATE_FULL; /* Normal target */
  cellular_params->attachment_timeout        = (uint32_t)crs_atoi(PLF_CELLULAR_ATTACHMENT_TIMEOUT);
  cellular_params->nfmc_active               = 0U;  /* NFMC disabled */
  cellular_params->rt_state = DC_SERVICE_ON;  /* Modem Config valid */
}


#if (USE_NETWORK_LIBRARY == 1)

/**
  * @brief  Register a network notification function in case of Network Library use
  * @note   only one function can be registered
  * @param  notify_func - notification function to record
  * @retval 0: OK - 1: NOK
  */
uint32_t cellular_net_register(net_if_notify_func notify_func)
{
  uint32_t ret = 1;
  if (cellular_notify_func == NULL)
  {
    cellular_notify_func = notify_func;
    ret = 0;
  }
  return ret;
}

/**
  * @brief  Initialize cellular features in case of Network Library use
  * @param  -
  * @retval -
  */
void cellular_net_init(void)
{
  static const   net_event_handler_t  net_handler   = { hnet_notify, &netif };

  /* Network Library Initialization */
  (void)net_if_init(&netif, &cellular_net_driver, &net_handler);
}

/**
  * @brief  Start cellular features in case of Network Library use
  * @param  -
  * @retval -
  */
void cellular_net_start(void)
{
  static net_cellular_credentials_t  credentials;
  static dc_cellular_params_t        cellular_params;

  /* read current cellular config in Data Cache */
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cellular_params, sizeof(cellular_params));

  /* update new cellular config in Data Cache*/

  credentials.sim_socket_nb = cellular_params.sim_slot_nb;

  /* 1st SIM slot initialisation */
  credentials.sim_slot[0].sim_slot_type = (net_sim_slot_type_t)cellular_params.sim_slot[0].sim_slot_type;
  credentials.sim_slot[0].apn           = (char_t *)cellular_params.sim_slot[0].apn;
  credentials.sim_slot[0].cid           = (char_t)cellular_params.sim_slot[0].cid;
  credentials.sim_slot[0].username      = (char_t *)cellular_params.sim_slot[0].username;
  credentials.sim_slot[0].password      = (char_t *)cellular_params.sim_slot[0].password;

  /* 2nd SIM slot initialisation */
  credentials.sim_slot[1].sim_slot_type = (net_sim_slot_type_t)cellular_params.sim_slot[1].sim_slot_type;
  credentials.sim_slot[1].apn           = (char_t *)cellular_params.sim_slot[1].apn;
  credentials.sim_slot[1].cid           = (char_t)cellular_params.sim_slot[1].cid;
  credentials.sim_slot[1].username      = (char_t *)cellular_params.sim_slot[1].username;
  credentials.sim_slot[1].password      = (char_t *)cellular_params.sim_slot[1].password;

  /* set cellular credentials */
  (void)net_cellular_set_credentials(&netif, &credentials);

  /* Waiting for NET_STATE_INITIALIZED network notification (from Network Library) */
  (void)net_if_wait_state(&netif, NET_STATE_INITIALIZED, STATE_TRANSITION_TIMEOUT);
  (void)net_if_start(&netif);
}
#endif /* (USE_NETWORK_LIBRARY == 1) */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
