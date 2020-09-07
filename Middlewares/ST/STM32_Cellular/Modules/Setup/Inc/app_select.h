/**
  ******************************************************************************
  * @file    app_select.h
  * @author  MCD Application Team
  * @brief   Header for app_select.c module
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
#ifndef APP_SELECT_H
#define APP_SELECT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
typedef uint8_t AS_CHART_t ;

/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/* init function of app_select component */
void app_select_init(void);

typedef bool (*AS_HandlerCallback)(void);

/* allows to record an application */
int32_t app_select_record(AS_CHART_t *app_label, AS_HandlerCallback handler_callback);

/* start function of app_select component - display menu */
void app_select_start(void);


#ifdef __cplusplus
}
#endif

#endif /* APP_SELECT_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
