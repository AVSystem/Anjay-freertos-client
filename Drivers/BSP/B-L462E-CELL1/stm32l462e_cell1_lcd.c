/**
  ******************************************************************************
  * @file    stm32l462e_cell1_lcd.c
  * @author  MCD Application Team
  * @brief   source file for the BSP LCD driver
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

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include "stm32l462e_cell1_lcd.h"

/** @addtogroup BSP BSP
  * @{
  */

/** @addtogroup STM32L462E_CELL1 STM32L462E_CELL1
  * @{
  */

/** @addtogroup STM32L462E_CELL1_LCD STM32L462E_CELL1 LCD
  * @{
  */

/** @defgroup STM32L462E_CELL1_LCD_Exported_Variables STM32L462E_CELL1 LCD Exported Variables
  * @{
  */

/* LINK UTIL LCD */
const LCD_UTILS_Drv_t LCD_Driver =
{
  BSP_LCD_DrawBitmap,
  BSP_LCD_FillRGBRect,
  BSP_LCD_DrawHLine,
  BSP_LCD_DrawVLine,
  BSP_LCD_FillRect,
  BSP_LCD_ReadPixel,
  BSP_LCD_WritePixel,
  BSP_LCD_GetXSize,
  BSP_LCD_GetYSize,
  NULL,
  BSP_LCD_GetPixelFormat
};
BSP_LCD_Ctx_t LcdCtx[LCD_INSTANCES_NBR];

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_LCD_Private_Defines STM32L462E_CELL1 LCD Private Defines
  * @{
  */

#define POLY_X(Z)               ((int32_t)((pPoints + (Z))->X))
#define POLY_Y(Z)               ((int32_t)((pPoints + (Z))->Y))

#define MAX_HEIGHT_FONT         11U
#define MAX_WIDTH_FONT          16U
#define OFFSET_BITMAP           54U

#define ABS(X)  ((X) > 0 ? (X) : -(X))

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_LCD_Private_Variables Private Variables
  * @{
  */

static LCD_DrvTypeDef  *lcd_drv;
static bool bsp_lcd_initialized = false;

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_LCD_Private_Functions_Prototypes STM32L462E_CELL1 LCD Private Functions Prototypes
  * @{
  */

static uint16_t convertColor(uint32_t Color);

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_LCD_Exported_Functions STM32L462E_CELL1 LCD Exported Functions
  * @{
  */

/**
  * @brief  Initializes the LCD.
  * @retval LCD state
  */
int32_t BSP_LCD_Init(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if (Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {

    if (bsp_lcd_initialized == false)
    {
      lcd_drv = &ssd1315_drv;

      /* LCD Init */
      lcd_drv->Init();

      /* Update BSP LCD initialization status */
      bsp_lcd_initialized = true;
    }
  }

  return ret;
}

/**
  * @brief  Deinitializes the LCD.
  * @retval LCD state
  */
int32_t BSP_LCD_DeInit(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if (Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if (bsp_lcd_initialized == true)
    {
      LCD_IO_DeInit();
      bsp_lcd_initialized = false;
    }
  }
  return ret;
}

/**
  * @brief  Gets the LCD X size.
  * @param  Instance LCD Instance
  * @param  pXSize pointer to Used LCD X size
  * @retval BSP status
  */
int32_t BSP_LCD_GetXSize(uint32_t Instance, uint32_t *pXSize)
{

  int32_t ret = BSP_ERROR_NONE;

  if (Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    *pXSize = lcd_drv->GetLcdPixelWidth();
  }
  return ret;
}

/**
  * @brief  Gets the LCD Y size.
  * @param  Instance LCD Instance
  * @param  pYSize pointer to Used LCD Y size
  * @retval BSP status
  */
int32_t BSP_LCD_GetYSize(uint32_t Instance, uint32_t *pYSize)
{
  int32_t ret = BSP_ERROR_NONE;

  if (Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    *pYSize = lcd_drv->GetLcdPixelHeight();
  }
  return ret;
}

/**
  * @brief  clear the LCD in currently active layer.
  * @param  Instance LCD Instance
  * @param  Color to set
  * @retval BSP status
  */
int32_t BSP_LCD_Clear(uint32_t Instance, uint32_t Color)
{
  int32_t ret = BSP_ERROR_NONE;

  if (Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    ssd1315_Clear(convertColor(Color));
  }

  return ret;
}

/**
  * @brief  Refresh the display.
  * @param  Instance LCD Instance
  * @retval BSP status
  */
int32_t BSP_LCD_Refresh(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if (Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    ssd1315_Refresh();
  }
  return ret;
}

/**
  * @brief  Draws a bitmap picture loaded in the internal Flash in currently active layer.
  * @param  Instance LCD Instance
  * @param  Xpos Bmp X position in the LCD
  * @param  Ypos Bmp Y position in the LCD
  * @param  pBmp Pointer to Bmp picture address in the internal Flash
  * @retval BSP status
  */
int32_t BSP_LCD_DrawBitmap(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint8_t *pBmp)
{
  int32_t ret = BSP_ERROR_NONE;

  if (Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (lcd_drv->DrawBitmap != NULL)
  {
    lcd_drv->DrawBitmap(Xpos, Ypos, pBmp);
  }
  else
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }

  return ret;
}

/**
  * @brief  Fill a rectangle with a BitMap on LCD.
  * @param  Instance LCD Instance.
  * @param  pData Pointer to RGB line data
  * @param  Xpos X position.
  * @param  Ypos Y position.
  * @param  Width width of the rectangle to fill.
  * @param  Height height of the rectangle to fill.
  * @retval BSP status.
  */
int32_t BSP_LCD_FillRGBRect(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint8_t *pData, uint32_t Width,
                            uint32_t Height)
{
  int32_t ret = BSP_ERROR_NONE;

  if (Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    uint32_t i;
    uint32_t j;
    uint32_t color;
    for (i = 0; i < Height; i++)
    {
      for (j = 0; j < Width; j++)
      {
        color = (uint32_t)(*pData | (*(pData + 1) << 8) | (*(pData + 2) << 16) | (*(pData + 3) << 24));

        (void) BSP_LCD_WritePixel(Instance, Xpos + j, Ypos + i, color);
        pData += 4;
      }
    }
  }
  return ret;
}

/**
  * @brief  Draws an horizontal line
  * @param  Instance LCD instance
  * @param  Xpos X position
  * @param  Ypos Y position
  * @param  Length Line length
  * @param  Color Line color
  * @retval BSP status
  */
int32_t BSP_LCD_DrawHLine(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  int32_t ret = BSP_ERROR_NONE;

  if (Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (lcd_drv->DrawHLine != NULL)
  {
    lcd_drv->DrawHLine(convertColor(Color),  Xpos, Ypos, Length);
  }
  else
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }

  return ret;
}

/**
  * @brief  Draws a vertical line
  * @param  Instance LCD instance
  * @param  Xpos X position
  * @param  Ypos Y position
  * @param  Length Line length
  * @param  Color Line color
  * @retval BSP status
  */
int32_t BSP_LCD_DrawVLine(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  int32_t ret = BSP_ERROR_NONE;
  if (Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (lcd_drv->DrawVLine != NULL)
  {
    lcd_drv->DrawVLine(convertColor(Color), Xpos, Ypos, Length);
  }
  else
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }

  return ret;
}

/**
  * @brief  Draws a full rectangle in currently active layer.
  * @param  Instance LCD Instance
  * @param  Xpos X position
  * @param  Ypos Y position
  * @param  Width Rectangle width
  * @param  Height Rectangle height
  * @param  Color Color of rectangle
  * @retval BSP status
  */
int32_t BSP_LCD_FillRect(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height,
                         uint32_t Color)
{
  int32_t ret = BSP_ERROR_NONE;

  if (Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    uint32_t localYpos = Ypos;
    uint32_t localHeight = Height;
    while (localHeight > 0U)
    {
      localHeight--;
      (void) BSP_LCD_DrawHLine(0, Xpos, localYpos, Width, Color);
      localYpos++;
    }
  }

  return ret;
}

/**
  * @brief  Reads a LCD pixel color.
  * @param  Instance LCD Instance
  * @param  Xpos X position
  * @param  Ypos Y position
  * @param  Color pointer to RGB pixel color
  * @retval BSP status
  */
int32_t  BSP_LCD_ReadPixel(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t *Color)
{
  int32_t ret = BSP_ERROR_NONE;

  if (Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (lcd_drv->ReadPixel != NULL)
  {
    *Color = lcd_drv->ReadPixel(Xpos, Ypos);
  }
  else
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }

  return ret;
}

/**
  * @brief  Writes a LCD pixel.
  * @param  Instance LCD Instance
  * @param  Xpos X position
  * @param  Ypos Y position
  * @param  Color RGB pixel color
  * @retval BSP status
  */
int32_t  BSP_LCD_WritePixel(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Color)
{
  int32_t ret = BSP_ERROR_NONE;

  if (Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else  if (lcd_drv->WritePixel != NULL)
  {
    lcd_drv->WritePixel(Xpos, Ypos, convertColor(Color));
  }
  else
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }

  return ret;
}

/**
  * @brief  Gets the LCD Active LCD Pixel Format.
  * @param  Instance LCD Instance
  * @param  PixelFormat Active LCD Pixel Format
  * @retval BSP status
  */
int32_t BSP_LCD_GetPixelFormat(uint32_t Instance, uint32_t *PixelFormat)
{
  UNUSED(PixelFormat);

  int32_t ret;

  if (Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }

  return ret;
}

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_LCD_Private_Functions STM32L462E_CELL1 LCD Private Functions
  * @{
  */

static uint16_t convertColor(uint32_t Color)
{
  uint16_t convertedColor;
  if (Color == 0U)
  {
    convertedColor = LCD_COLOR_BLACK;
  }
  else
  {
    convertedColor = LCD_COLOR_WHITE;
  }
  return convertedColor;
}

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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
