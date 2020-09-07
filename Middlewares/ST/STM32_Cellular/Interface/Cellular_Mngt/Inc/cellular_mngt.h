/**
  ******************************************************************************
  * @file    cellular_mngt.h
  * @author  MCD Application Team
  * @brief   Header for cellular_mngt.c module
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
#ifndef CELLULAR_MNGT_H
#define CELLULAR_MNGT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"
#if (USE_NETWORK_LIBRARY == 1)
#include "net_connect.h"
#endif /* (USE_NETWORK_LIBRARY == 1) */
#include "cellular_datacache.h"


/**
  ******************************************************************************
  @verbatim
  ==============================================================================
                  ##### How to use Cellular Management module #####
  ==============================================================================

 Cellular Management allows Initialization and Start of X-Cube-Cellular components.

 Before to access any other services of X-Cube-Cellular component, initialization must be done.
 Then three start options are available:

 1) boot the modem with network registration in order to make data transfert
    cellular_init() --> cellular_start()

 2) boot the modem with network registration with network library use, in order to make data transfert
    cellular_net_init() --> cellular_net_start()
     Note: the use of Network Library component is usefull in 2 main cases:
        - use of mbedtls: Network Library integrates natively the use of mbedtls.
        - multi network: Network Library unifies the interface of cellular service, Wifi and Ethernet networks access.

 3) boot modem with no network registration  (used to configure modem)
   cellular_init() --> cellular_modem_start()

 -----
 Note:
 -----
 There are 3 ways to configure cellular service (APN,...) :
 1) Update cellular parameters in plf_cellular_config.h configuration file

 2) Use the setup menu. In this case, cellular parameters can be entered at boot time using setup menu
    and stored in FEEPRON

 3) Set cellular parameters using DC_CELLULAR_CONFIG Data Cache Entry.
    In this case the DC_CELLULAR_CONFIG Data Cache Entry update must be done
    between cellular_init and cellular_start calls.
    See Cellular Data Cache Entries.

 @endverbatim
  */

/** @defgroup CELLULAR_MANAGEMENT CellularMngt: Cellular Management module
  * @{
  */

/**
  * @}
  */

/** @addtogroup CELLULAR_MANAGEMENT
  * @{
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup CELLULAR_MANAGEMENT_Constants Constants
  * @{
  */

/**
  * @}
  */

/* Exported types ------------------------------------------------------------*/
/** @defgroup CELLULAR_MANAGEMENT_Types Types
  * @{
  */

/**
  * @}
  */

/* External variables --------------------------------------------------------*/
/** @defgroup CELLULAR_MANAGEMENT_Variables Variables
  * @{
  */

/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/
/** @defgroup CELLULAR_MANAGEMENT_Macros Macros
  * @{
  */

/**
  * @}
  */

/* Exported functions ------------------------------------------------------- */
/** @defgroup CELLULAR_MANAGEMENT_Functions Functions
  * @{
  */

/** @defgroup CELLULAR_MANAGEMENT_Functions_Group1  Cellular Initialization and Start services
  * @brief
  * @{
  */

/**
  * @brief  Initialize cellular features
  * @param  -
  * @retval -
  */
void cellular_init(void);

/**
  * @brief  Start cellular features with boot modem and network registration
  * @param  -
  * @retval -
  */
void cellular_start(void);

/**
  * @brief  Start cellular feature with boot modem (NO network registration)
  *         Usage: configure the modem
  * @param  -
  * @retval -
  */
void cellular_modem_start(void);

/**
  * @brief  Initialize dc_cellular_params_t structure with default value
  * @note   this function should be called by application before to store its own values
  * @param  cellular_params - cellular configuration
  * @retval -
  */
void cellular_set_default_setup_config(dc_cellular_params_t *cellular_params);

/**
  * @}
  */

#if (USE_NETWORK_LIBRARY == 1)

/** @defgroup CELLULAR_MANAGEMENT_Functions_Group3 Cellular Initialization and Start services Network Library
  * @brief    Initialization and Start services in case of Network Library use
  * @{
  */

/**
  * @brief  Register a network notification function in case of Network Library use
  * @note   only one function can be registered
  * @param  notify_func - notification function to record
  * @retval 0: OK - 1: NOK
  */
uint32_t cellular_net_register(net_if_notify_func notify_func);

/**
  * @brief  Initialize cellular features in case of Network Library use
  * @param  -
  * @retval -
  */
void cellular_net_init(void);

/**
  * @brief  Start cellular features in case of Network Library use
  * @param  -
  * @retval -
  */
void cellular_net_start(void);

/**
  * @}
 */

#endif  /* (USE_NETWORK_LIBRARY == 1) */

/**
  * @}
  */

/**
  * @}
  */


#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_MNGT_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
