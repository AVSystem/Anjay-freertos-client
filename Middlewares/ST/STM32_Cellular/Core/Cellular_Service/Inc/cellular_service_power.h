/**
  ******************************************************************************
  * @file    cellular_service_power.h
  * @author  MCD Application Team
  * @brief   Header for cellular_service_power.c module
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

