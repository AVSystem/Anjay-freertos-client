/**
  ******************************************************************************
  * @file    com_core.h
  * @author  MCD Application Team
  * @brief   Header for com_core.c module
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
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

/**
  ******************************************************************************
  @verbatim
  ==============================================================================
                      ##### How to use COM Module #####
  ==============================================================================

  COM module allows :
  1) communication with a Remote host using IP protocol
  2) ping a Remote host
  3) communication with the International Circuit Card

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
/* None */

/* Exported macros -----------------------------------------------------------*/
/* None */

/* Exported functions ------------------------------------------------------- */
/* None */

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
