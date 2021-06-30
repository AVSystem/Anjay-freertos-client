/**
  ******************************************************************************
  * @file    plf_features.h
  * @author  MCD Application Team
  * @brief   Includes feature list to include in firmware
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
#ifndef PLF_FEATURES_H
#define PLF_FEATURES_H

#ifdef __cplusplus
extern "C" {
#endif


/* Includes ------------------------------------------------------------------*/
#if (defined APPLICATION_CONFIG_FILE)
#include APPLICATION_CONFIG_FILE /* First include to overwrite Platform defines */
#endif /* defined APPLICATION_CONFIG_FILE */

/* Exported constants --------------------------------------------------------*/

/* ===================================== */
/* BEGIN - Cellular data mode            */
/* ===================================== */

/* Possible values for USE_SOCKETS_TYPE */
#define USE_SOCKETS_LWIP          (0)  /* define value affected to LwIP sockets type  */
#define USE_SOCKETS_MODEM         (1)  /* define value affected to Modem sockets type */

/* Sockets location */
#if !defined USE_SOCKETS_TYPE
#define USE_SOCKETS_TYPE          (USE_SOCKETS_MODEM) /* Possible values: USE_SOCKETS_LWIP or USE_SOCKETS_MODEM */
#endif /* !defined USE_SOCKETS_TYPE */

/* ===================================== */
/* END - Cellular data mode              */
/* ===================================== */

/* ======================================= */
/* BEGIN -  Miscellaneous functionalities  */
/* ======================================= */

/* To configure some parameters of the software */
#if !defined USE_CMD_CONSOLE
#define USE_CMD_CONSOLE           (0) /* 0: not activated, 1: activated */
#endif /* !defined USE_CMD_CONSOLE */

/* If included then com_ping interfaces are defined in com module */
#if !defined USE_COM_PING
#define USE_COM_PING              (1)  /* 0: not included, 1: included */
#endif /* !defined USE_COM_PING */

/* If included then com_icc interfaces are defined in com module */
#if !defined USE_COM_ICC
#define USE_COM_ICC               (1)  /* 0: not included, 1: included */
#endif /* !defined USE_COM_ICC */

/* ======================================= */
/* END   -  Miscellaneous functionalities  */
/* ======================================= */

/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* PLF_FEATURES_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
