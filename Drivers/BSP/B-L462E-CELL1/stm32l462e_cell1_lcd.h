/**
  ******************************************************************************
  * @file    stm32l462e_cell1_lcd.h
  * @author  MCD Application Team
  * @brief   header file for the BSP LCD driver
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32L462E_CELL1_LCD_H
#define STM32L462E_CELL1_LCD_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32l462e_cell1.h"
#include "lcd.h"
#include "ssd1315.h"
#include "fonts.h"


/** @addtogroup BSP BSP
  * @{
  */

/** @addtogroup STM32L462E_CELL1 STM32L462E_CELL1
  * @{
  */

/** @addtogroup STM32L462E_CELL1_LCD STM32L462E_CELL1 LCD
  * @{
  */

/** @defgroup STM32L462E_CELL1_LCD_Exported_Constants STM32L462E_CELL1 LCD Exported Constants
  * @{
  */

#define LCD_INSTANCES_NBR                1U

/**
  * @brief  LCD status structure definition
  */
#define   LCD_OK         0x00
#define   LCD_ERROR      0x01
#define   LCD_TIMEOUT    0x02

/**
  * @brief  Line mode structures definition
  */

/**
  * @brief  LCD color
  */
#define LCD_COLOR_BLACK          0x00
#define LCD_COLOR_WHITE          0xFF

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_LCD_Exported_Types STM32L462E_CELL1 LCD Exported Types
  * @{
  */

typedef struct
{
  uint32_t Width;
  uint32_t Height;
  uint32_t IsMspCallbacksValid;
} BSP_LCD_Ctx_t;

extern BSP_LCD_Ctx_t LcdCtx[LCD_INSTANCES_NBR];
extern const LCD_UTILS_Drv_t LCD_Driver;

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_LCD_Exported_Functions STM32L462E_CELL1 LCD Exported Functions
  * @{
  */

int32_t  BSP_LCD_Init(uint32_t Instance);
int32_t  BSP_LCD_DeInit(uint32_t Instance);
int32_t  BSP_LCD_GetXSize(uint32_t Instance, uint32_t *pXSize);
int32_t  BSP_LCD_GetYSize(uint32_t Instance, uint32_t *pYSize);
int32_t  BSP_LCD_Clear(uint32_t Instance, uint32_t Color);
int32_t  BSP_LCD_Refresh(uint32_t Instance);

int32_t  BSP_LCD_DrawBitmap(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint8_t *pBmp);
int32_t  BSP_LCD_FillRGBRect(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint8_t *pData, uint32_t Width,
                             uint32_t Height);
int32_t  BSP_LCD_DrawHLine(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color);
int32_t  BSP_LCD_DrawVLine(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color);
int32_t  BSP_LCD_FillRect(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height,
                          uint32_t Color);
int32_t  BSP_LCD_ReadPixel(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t *Color);
int32_t  BSP_LCD_WritePixel(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Color);
int32_t  BSP_LCD_GetPixelFormat(uint32_t Instance, uint32_t *PixelFormat);

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* STM32L462E_CELL1_LCD_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
