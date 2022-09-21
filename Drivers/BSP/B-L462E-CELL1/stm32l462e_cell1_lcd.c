/**
  ******************************************************************************
  * @file    stm32l462e_cell1_lcd.c
  * @author  MCD Application Team
  * @brief   source file for the BSP LCD driver
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include "stm32l462e_cell1.h"
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

/**
  * @brief  LINK UTIL LCD (used in stm32_lcd)
  */
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

/**
  * @brief  BSP LCD context.
  */
BSP_LCD_Ctx_t LcdCtx[LCD_INSTANCES_NBR];

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_LCD_Private_Defines STM32L462E_CELL1 LCD Private Defines
  * @{
  */

#define POLY_X(Z)               ((int32_t)((pPoints + (Z))->X))
#define POLY_Y(Z)               ((int32_t)((pPoints + (Z))->Y))
#define ABS(X)  ((X) > 0 ? (X) : -(X))

#define MAX_HEIGHT_FONT         11U
#define MAX_WIDTH_FONT          16U
#define OFFSET_BITMAP           54U

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_LCD_Private_Variables Private Variables
  * @{
  */
static SSD1315_Drv_t *LcdDrv = NULL;
static SSD1315_Object_t *LcdCompObj = NULL;

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_LCD_Private_Functions_Prototypes STM32L462E_CELL1 LCD Private Functions Prototypes
  * @{
  */
static int32_t SSD1315_Probe(uint32_t Orientation);
static int32_t LCD_IO_Init(void);
static int32_t LCD_IO_DeInit(void);
static int32_t BSP_LCD_ReadReg(uint16_t Reg, uint8_t *pData, uint16_t Length);
static int32_t BSP_LCD_WriteReg(uint16_t Reg, uint8_t *pData, uint16_t Length);
static int32_t BSP_LCD_SendData(uint8_t *pData, uint16_t Length);
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
  static bool bsp_lcd_initialized = false;

  if (Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if (bsp_lcd_initialized == false)
    {
      /* Orientation: Landscape */
      LcdCtx[Instance].Width  = LCD_DEFAULT_WIDTH;
      LcdCtx[Instance].Height = LCD_DEFAULT_HEIGHT;

      /* registers the function and initialize the controller */
      if (SSD1315_Probe(SSD1315_ORIENTATION_LANDSCAPE) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_UNKNOWN_COMPONENT;
      }

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
    if (LcdDrv->DeInit(LcdCompObj) < 0)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }

    /* turn LCD off = drive pin high (active low) */
    LCD_CS_HIGH();
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
  else if (LcdDrv->GetXSize != NULL)
  {
    if (LcdDrv->GetXSize(LcdCompObj, pXSize) < 0)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  else
  {
    *pXSize = LcdCtx[Instance].Width;
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
  else if (LcdDrv->GetYSize != NULL)
  {
    if (LcdDrv->GetYSize(LcdCompObj, pYSize) < 0)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  else
  {
    *pYSize = LcdCtx[Instance].Height;
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
  else if (LcdDrv->FillRect != NULL)
  {
    if (LcdDrv->FillRect(LcdCompObj, 0, 0, LcdCtx[Instance].Width, LcdCtx[Instance].Height, Color) < 0)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  else
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
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
  else if (LcdDrv->Refresh != NULL)
  {
    if (LcdDrv->Refresh(LcdCompObj) < 0)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  else
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
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
  else if (LcdDrv->DrawBitmap != NULL)
  {
    /* Draw the bitmap on LCD */
    if (LcdDrv->DrawBitmap(LcdCompObj, Xpos, Ypos, pBmp) < 0)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
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
  else if (LcdDrv->FillRGBRect != NULL)
  {
    /* shift bitmap on LCD */
    if (LcdDrv->FillRGBRect(LcdCompObj, Xpos, Ypos, pData, Width, Height) < 0)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  else
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
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
  else if (LcdDrv->DrawHLine != NULL)
  {
    /* Draw the horizontal line on LCD */
    if (LcdDrv->DrawHLine(LcdCompObj, Xpos, Ypos, Length, Color) < 0)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
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
  else if (LcdDrv->DrawVLine != NULL)
  {
    /* Draw the vertical line on LCD */
    if (LcdDrv->DrawVLine(LcdCompObj, Xpos, Ypos, Length, Color) < 0)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
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
  else if (LcdDrv->FillRect != NULL)
  {
    if (LcdDrv->FillRect(LcdCompObj, Xpos, Ypos, Width, Height, Color) < 0)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  else
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
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
  else if (LcdDrv->GetPixel != NULL)
  {
    if (LcdDrv->GetPixel(LcdCompObj, Xpos, Ypos, Color) < 0)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
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
  else if (LcdDrv->SetPixel != NULL)
  {
    if (LcdDrv->SetPixel(LcdCompObj, Xpos, Ypos, Color) < 0)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
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

/**
  * @brief  Register Bus IOs for instance 0 if SSD1315 ID is OK
  * @param  Orientation
  * @retval BSP status
  */
static int32_t SSD1315_Probe(uint32_t Orientation)
{
  int32_t                 ret = BSP_ERROR_NONE;
  SSD1315_IO_t            IOCtx;
  static SSD1315_Object_t SSD1315Obj;

  /* Configure the lcd driver : map to LCD_IO function*/
  IOCtx.Init             = LCD_IO_Init;
  IOCtx.DeInit           = LCD_IO_DeInit;
  IOCtx.ReadReg          = BSP_LCD_ReadReg;
  IOCtx.WriteReg         = BSP_LCD_WriteReg;
  IOCtx.GetTick          = BSP_GetTick;

  if (SSD1315_RegisterBusIO(&SSD1315Obj, &IOCtx) != SSD1315_OK)
  {
    ret = BSP_ERROR_UNKNOWN_COMPONENT;
  }
  else
  {
    LcdCompObj = &SSD1315Obj;

    /* turn LCD on = drive pin low (active low) */
    LCD_CS_LOW();

    /* LCD Initialization */
    LcdDrv = (SSD1315_Drv_t *)&SSD1315_Driver;
    if (LcdDrv->Init(LcdCompObj, SSD1315_FORMAT_DEFAULT, Orientation) != SSD1315_OK)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }

  return ret;
}

/**
  * @brief  Initializes lcd low level.
  * @retval int32_t
  */
int32_t LCD_IO_Init(void)
{
  int32_t ret;

  GPIO_InitTypeDef GPIO_InitStruct;
  HAL_GPIO_WritePin(LCD_D_C_DISP_GPIO_PORT, LCD_D_C_DISP_PIN, GPIO_PIN_SET);

  /* Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_RST_DISP_GPIO_PORT, LCD_RST_DISP_PIN, GPIO_PIN_SET);

  /* Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_CS_DISP_GPIO_PORT, LCD_CS_DISP_PIN, GPIO_PIN_SET);

  /* Configure GPIO pin : LCD_D_C_DISP_PIN */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  GPIO_InitStruct.Pin = LCD_D_C_DISP_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LCD_D_C_DISP_GPIO_PORT, &GPIO_InitStruct);
  HAL_GPIO_WritePin(LCD_D_C_DISP_GPIO_PORT, LCD_D_C_DISP_PIN, GPIO_PIN_RESET);

  /* Configure GPIO pin : LCD_RST_DISP_PIN */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  GPIO_InitStruct.Pin = LCD_RST_DISP_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  /* GPIO_InitStruct.Pull = GPIO_NOPULL; */          /* Already done in previous line */
  /* GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; */ /* Already done in previous line */
  HAL_GPIO_Init(LCD_RST_DISP_GPIO_PORT, &GPIO_InitStruct);

  /* Configure GPIO pin : LCD_CS_DISP_PIN */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  GPIO_InitStruct.Pin = LCD_CS_DISP_PIN;
  /* GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD; */  /* Already done in previous line */
  /* GPIO_InitStruct.Pull = GPIO_NOPULL; */          /* Already done in previous line */
  /* GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; */ /* Already done in previous line */
  HAL_GPIO_Init(LCD_CS_DISP_GPIO_PORT, &GPIO_InitStruct);
  HAL_GPIO_WritePin(LCD_CS_DISP_GPIO_PORT, LCD_CS_DISP_PIN, GPIO_PIN_RESET);

  /* Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LCD_RST_DISP_GPIO_PORT, LCD_RST_DISP_PIN, GPIO_PIN_RESET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(LCD_RST_DISP_GPIO_PORT, LCD_RST_DISP_PIN, GPIO_PIN_SET);

  ret = BSP_SPI3_Init();

  GPIO_InitStruct.Pin = LCD_D_C_DISP_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  /* GPIO_InitStruct.Pull = GPIO_NOPULL; */  /* Already done in previous line */
  /* GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; */  /* Already done in previous line */
  HAL_GPIO_Init(LCD_D_C_DISP_GPIO_PORT, &GPIO_InitStruct);
  HAL_GPIO_WritePin(LCD_D_C_DISP_GPIO_PORT, LCD_D_C_DISP_PIN, GPIO_PIN_RESET);

  return (ret);
}

/**
  * @brief  Deinitializes lcd low level.
  * @retval int32_t
  */
int32_t LCD_IO_DeInit(void)
{
  int32_t ret;

  /* SPI Deinit */
  ret = BSP_SPI3_DeInit();

  HAL_GPIO_DeInit(GPIOC, (GPIO_PIN_12 | GPIO_PIN_11 | GPIO_PIN_10));
  __HAL_RCC_SPI3_FORCE_RESET();
  __HAL_RCC_SPI3_RELEASE_RESET();

  HAL_GPIO_WritePin(LCD_CS_DISP_GPIO_PORT, LCD_CS_DISP_PIN, GPIO_PIN_SET);

  /* Disable SPIx clock  */
  __HAL_RCC_GPIOC_CLK_DISABLE();

  return (ret);
}

/**
  * @brief  Read data from LCD data register.
  * @param  Reg Register to be read
  * @param  pData pointer to the read data from LCD SRAM.
  * @param  Length length of data be read from the LCD SRAM
  * @retval BSP status
  */
static int32_t BSP_LCD_ReadReg(uint16_t Reg, uint8_t *pData, uint16_t Length)
{
  int32_t ret = BSP_ERROR_NONE;
  UNUSED(Length);

  /* Send Reg value to Read */
  if (BSP_LCD_WriteReg(Reg, pData, 0) != BSP_ERROR_NONE)
  {
    ret = BSP_ERROR_BUS_FAILURE;
  }
  /* Reset LCD control line(/CS) and Send command */
  LCD_CS_LOW();

  if (ret == BSP_ERROR_NONE)
  {
    if (BSP_SPI3_Recv(pData, 2) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_BUS_FAILURE;
    }
  }
  /* Deselect : Chip Select high */
  LCD_CS_HIGH();

  return ret;
}

/**
  * @brief  Writes register on LCD register.
  * @param  Reg Register to be written
  * @param  pData pointer to the read data from LCD SRAM.
  * @param  Length length of data be read from the LCD SRAM
  * @retval BSP status
  */
static int32_t BSP_LCD_WriteReg(uint16_t Reg, uint8_t *pData, uint16_t Length)
{
  UNUSED(Reg);
  int32_t ret = BSP_ERROR_NONE;

  /* Send Data */
  if (BSP_LCD_SendData(pData, Length) != BSP_ERROR_NONE)
  {
    ret = BSP_ERROR_BUS_FAILURE;
  }

  return ret;
}

/**
  * @brief  Send data to select the LCD SRAM.
  * @param  pData pointer to data to write to LCD SRAM.
  * @param  Length length of data to write to LCD SRAM
  * @retval Error status
  */
static int32_t BSP_LCD_SendData(uint8_t *pData, uint16_t Length)
{
  int32_t ret = BSP_ERROR_NONE;
  if (Length == 1U) /* [MISRAC2012] Rule-14.3_b Conditional expression is always false analyzed as false positive */
  {
    /* Reset LCD control line CS */
    LCD_CS_LOW();
    LCD_DC_LOW();
    /* Send Data */
    if (BSP_SPI3_Send(pData, Length) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_BUS_FAILURE;
    }
    /* Deselect : Chip Select high */
    LCD_CS_HIGH();
  }
  else
  {
    LCD_CS_LOW();
    LCD_DC_HIGH();
    /* Send Data */
    if (BSP_SPI3_Send(pData, Length) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_BUS_FAILURE;
    }
    LCD_DC_LOW() ;
    /* Deselect : Chip Select high */
    LCD_CS_HIGH();
  }

  return ret;
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
