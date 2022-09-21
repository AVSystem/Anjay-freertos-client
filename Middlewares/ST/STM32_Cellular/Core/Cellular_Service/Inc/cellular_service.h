/**
  ******************************************************************************
  * @file    cellular_service.h
  * @author  MCD Application Team
  * @brief   Header for cellular_service.c
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
#ifndef CELLULAR_SERVICE_H
#define CELLULAR_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

/* Cellular Service (control/signalling part) */
#include "cellular_service_control.h"
/* Cellular Service (data socket part) */
#include "cellular_service_socket.h"

#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_SERVICE_H */

