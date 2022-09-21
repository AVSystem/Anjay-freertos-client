/**
  ******************************************************************************
  * @file    ppposif_client.h
  * @author  MCD Application Team
  * @brief   Header for ppposif_client.c module
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
#ifndef PPPOSIF_CLIENT_H
#define PPPOSIF_CLIENT_H


#ifdef __cplusplus
extern "C" {
#endif


/* Includes ------------------------------------------------------------------*/
#include "ppposif.h"

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/**
  * @brief  component init
  * @param  none
  * @retval ppposif_status_t    return status
  */
extern ppposif_status_t ppposif_client_init(void);

/**
  * @brief  component start
  * @param  none
  * @retval ppposif_status_t    return status
  */
extern ppposif_status_t ppposif_client_start(void);

/**
  * @brief  Create a new PPPoS client interface
  * @param  none
  * @retval ppposif_status_t    return status
  */
extern ppposif_status_t ppposif_client_config(void);

/**
  * @brief  close PPPoS client interface
  * @param  none
  * @retval ppposif_status_t    return status
  */
extern ppposif_status_t ppposif_client_close(uint8_t cause);

#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */

#ifdef __cplusplus
}
#endif

#endif /* PPPOSIF_CLIENT_H */
