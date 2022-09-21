/**
  ******************************************************************************
  * @file    cellular_service_cmd.h
  * @author  MCD Application Team
  * @brief   Header for cellular_service_cmd.c module
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

