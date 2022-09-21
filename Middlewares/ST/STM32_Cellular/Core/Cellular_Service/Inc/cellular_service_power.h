/**
  ******************************************************************************
  * @file    cellular_service_power.h
  * @author  MCD Application Team
  * @brief   Header for cellular_service_power.c module
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CELLULAR_SERVICE_POWER_H
#define CELLULAR_SERVICE_POWER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "rtosal.h"
#include "cellular_service.h"
#include "dc_common.h"
#if (USE_LOW_POWER == 1)

/* Exported constants --------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

typedef enum
{
  CSP_LOW_POWER_DISABLED        = 0,     /*!< Low power not enabled          */
  CSP_LOW_POWER_INACTIVE        = 1,     /*!< Low power not active           */
  CSP_LOW_POWER_ON_GOING        = 2,     /*!< Low power activation requested */
  CSP_LOW_POWER_ACTIVE          = 3      /*!< Low power active                */
} CSP_PowerState_t;

/* External variables --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */


CS_Status_t CSP_DataIdle(void);
CS_Status_t CSP_DataWakeup(CS_wakeup_origin_t wakeup_origin);
CS_Status_t CSP_CSIdle(void);

void CSP_SleepCancel(void);
void CSP_SleepComplete(void);
void CSP_DataIdleManagment(void);
void CSP_SetPowerConfig(void);
void CSP_Init(void);
void CSP_Start(void);
void CSP_WakeupComplete(void);
void CSP_ResetPowerStatus(void);
void CSP_InitPowerConfig(void);
CSP_PowerState_t CSP_GetTargetPowerState(void);
void CSP_StopTimeout(void);

#endif  /* (USE_LOW_POWER == 1) */

#ifdef __cplusplus
}
#endif


#endif /* CELLULAR_SERVICE_POWER_H */

