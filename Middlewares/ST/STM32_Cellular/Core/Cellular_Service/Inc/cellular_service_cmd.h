/**
  ******************************************************************************
  * @file    cellular_service_cmd.h
  * @author  MCD Application Team
  * @brief   Header for cellular_service_cmd.c module
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
#ifndef CELLULAR_SERVICE_CMD_H
#define CELLULAR_SERVICE_CMD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

/**
  * @brief  starts cellular service command
  * @param  -
  * @retval CS_Status_t - return code
  */

#if (USE_CMD_CONSOLE == 1)
extern CS_Status_t CST_cmd_cellular_service_start(void);
#endif  /*  (USE_CMD_CONSOLE == 1) */

#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_SERVICE_CMD_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

