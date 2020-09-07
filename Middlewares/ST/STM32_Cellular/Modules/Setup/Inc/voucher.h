/**
  ******************************************************************************
  * @file    voucher.h
  * @author  MCD Application Team
  * @brief   Header for voucher.c module
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
#ifndef VOUCHER_H
#define VOUCHER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "menu_utils.h"

/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* type of application version */
/* Exported functions ------------------------------------------------------- */
/**
  * @brief  voucher component init
  * @param  none
  * @retval none
  */
void voucher_init(void);


#ifdef __cplusplus
}
#endif

#endif /* VOUCHER_H_ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

