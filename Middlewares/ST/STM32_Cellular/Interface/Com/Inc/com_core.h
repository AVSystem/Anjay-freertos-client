/**
  ******************************************************************************
  * @file    com_core.h
  * @author  MCD Application Team
  * @brief   Header for com_core.c module
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
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
#ifndef COM_CORE_H
#define COM_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>

#include "plf_config.h"

/**
  ******************************************************************************
  @verbatim
  ==============================================================================
                      ##### How to use COM Module #####
  ==============================================================================

  COM module allows :
  1) communication with a Remote host using IP protocol
  2) ping a Remote host
  3) communication with the SIM

  @endverbatim
  */

/** @defgroup COM COM: Communication Module
  * @{
  */

/**
  * @}
  */


/** @addtogroup COM
  * @{
  */

/** @defgroup COM_Common Common
  * @{
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup COM_Constants Constants
  * @{
  */

/**
  * @}
  */

/* Exported types ------------------------------------------------------------*/
/** @defgroup COM_Types Types
  * @{
  */

/**
  * @}
  */

/* External variables --------------------------------------------------------*/
/** @defgroup COM_Variables Variables
  * @{
  */

/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/
/** @defgroup COM_Macros Macros
  * @{
  */

/**
  * @}
  */

/* Exported functions ------------------------------------------------------- */
/** @defgroup COM_Functions Functions
  * @{
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

/*** Component Initialization/Start *******************************************/
/*** Internal use only - Not an Application Interface *************************/

/**
  * @brief  Component initialization
  * @note   must be called only one time :
  *         - before using any other functions of com_*
  * @param  -
  * @retval bool      - true/false init ok/nok
  */
bool com_init(void);

/**
  * @brief  Component start
  * @note   must be called only one time but
            after com_init and dc_start
            and before using any other functions of com_*
  * @param  -
  * @retval bool      - true/false init ok/nok
  */
bool com_start(void);



#ifdef __cplusplus
}
#endif

#endif /* COM_CORE_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
