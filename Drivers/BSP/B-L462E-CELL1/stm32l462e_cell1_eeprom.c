/**
  ******************************************************************************
  * @file    stm32l462e_cell1_eeprom.c
  * @author  MCD Application Team
  * @brief   source file for the BSP EEPROM driver
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
#include "stm32l462e_cell1_eeprom.h"
#include "stm32l462e_cell1.h"
#include "i2c.h"
#include "stm32l462e_cell1_bus.h"

/** @addtogroup BSP BSP
  * @{
  */

/** @addtogroup STM32L462E_CELL1 STM32L462E_CELL1
  * @{
  */

/** @addtogroup STM32L462E_CELL1_EEPROM STM32L462E_CELL1 EEPROM
  * @{
  */

/** @defgroup STM32L462E_CELL1_EEPROM_Private_Functions_Prototypes STM32L462E_CELL1 EEPROM Private Functions Prototypes
  * @{
  */
static uint32_t EEPROM_WriteBufferAligned(uint8_t *pBuffer, uint16_t WriteAddr, uint16_t NumByteToWrite);
static uint32_t EEPROM_WriteBufferNotAligned(uint16_t addr, uint8_t *pBuffer, uint16_t WriteAddr,
                                             uint16_t NumByteToWrite);
static uint32_t EEPROM_WriteLessThanPage(uint16_t addr, uint8_t *pBuffer, uint16_t WriteAddr, uint16_t NumByteToWrite);
static uint32_t EEPROM_WriteMoreThanPage(uint16_t addr, uint8_t *pBuffer, uint16_t WriteAddr, uint16_t NumByteToWrite);

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_EEPROM_Exported_Functions STM32L462E_CELL1 EEPROM Exported Functions
  * @{
  */

/**
  * @brief  Initializes peripherals used by the I2C EEPROM driver.
  * @retval EEPROM_OK (0) if operation is correctly performed, else return value
  *         different from EEPROM_OK (0)
  */
uint32_t BSP_EEPROM_Init(void)
{
  uint32_t ret;

  /* I2C Initialization */
  (void) BSP_I2C1_Init();

  /* Select the EEPROM address check if OK */
  if (BSP_I2C1_IsDeviceReady(EEPROM_I2C_ADDRESS, EEPROM_MAX_TRIALS) != BSP_ERROR_NONE)
  {
    ret = EEPROM_FAIL;
  }
  else
  {
    ret = EEPROM_OK;
  }
  return (ret);
}

/**
  * @brief  DeInitializes the EEPROM.
  * @retval EEPROM state
  */
uint32_t BSP_EEPROM_DeInit(void)
{
  /* I2C is not disabled because common to other functionalities */
  return EEPROM_OK;
}

/**
  * @brief  Reads a block of data from the EEPROM.
  * @param  pBuffer: pointer to the buffer that receives the data read from
  *         the EEPROM.
  * @param  ReadAddr: EEPROM's internal address to start reading from.
  * @param  NumByteToRead: pointer to the variable holding number of bytes to
  *         be read from the EEPROM.
  *
  *        @note The variable pointed by NumByteToRead is reset to 0 when all the
  *              data are read from the EEPROM. Application should monitor this
  *              variable in order know when the transfer is complete.
  *
  * @retval EEPROM_OK (0) if operation is correctly performed, else return value
  *         different from EEPROM_OK (0) or the timeout user callback.
  */
uint32_t BSP_EEPROM_ReadBuffer(uint8_t *pBuffer, uint16_t ReadAddr, uint16_t *NumByteToRead)
{
  uint32_t status = EEPROM_OK;
  uint16_t buffersize = *NumByteToRead;

  /* Set the pointer to the Number of data to be read. This pointer will be used
  by the DMA Transfer Completer interrupt Handler in order to reset the
  variable to 0. User should check on this variable in order to know if the
  DMA transfer has been complete or not.
  EEPROMDataRead = *NumByteToRead; */

  if (BSP_I2C1_ReadReg_16b(EEPROM_I2C_ADDRESS, ReadAddr, pBuffer, buffersize) != BSP_ERROR_NONE)
  {
    BSP_EEPROM_TIMEOUT_UserCallback();
    status = EEPROM_FAIL;
  }

  return (status);
}

/**
  * @brief  Writes more than one byte to the EEPROM with a single WRITE cycle.
  *
  * @note   The number of bytes (combined to write start address) must not
  *         cross the EEPROM page boundary. This function can only write into
  *         the boundaries of an EEPROM page.
  *         This function doesn't check on boundaries condition (in this driver
  *         the function BSP_EEPROM_WriteBuffer() which calls BSP_EEPROM_WritePage() is
  *         responsible of checking on Page boundaries).
  *
  * @param  pBuffer: pointer to the buffer containing the data to be written to
  *         the EEPROM.
  * @param  WriteAddr: EEPROM's internal address to write to.
  * @param  NumByteToWrite: pointer to the variable holding number of bytes to
  *         be written into the EEPROM.
  *
  *        @note The variable pointed by NumByteToWrite is reset to 0 when all the
  *              data are written to the EEPROM. Application should monitor this
  *              variable in order know when the transfer is complete.
  *
  *        @note This function just configure the communication and enable the DMA
  *              channel to transfer data. Meanwhile, the user application may perform
  *              other tasks in parallel.
  *
  * @retval EEPROM_OK (0) if operation is correctly performed, else return value
  *         different from EEPROM_OK (0) or the timeout user callback.
  */
uint32_t BSP_EEPROM_WritePage(uint8_t *pBuffer, uint16_t WriteAddr, uint16_t *NumByteToWrite)
{
  uint16_t buffersize = *NumByteToWrite;
  uint32_t status = EEPROM_OK;

  /* Set the pointer to the Number of data to be written. This pointer will be used
  by the DMA Transfer Completer interrupt Handler in order to reset the
  variable to 0. User should check on this variable in order to know if the
  DMA transfer has been complete or not.
  EEPROMDataWrite = *NumByteToWrite; */

  if (BSP_I2C1_WriteReg_16b(EEPROM_I2C_ADDRESS, WriteAddr, pBuffer, buffersize) != BSP_ERROR_NONE)
  {
    BSP_EEPROM_TIMEOUT_UserCallback();
    status = EEPROM_FAIL;
  }

  if (BSP_EEPROM_WaitEepromStandbyState() != EEPROM_OK)
  {
    status = EEPROM_FAIL;
  }

  /* If all operations OK, return EEPROM_OK (0) */
  return status;
}

/**
  * @brief  Writes buffer of data to the I2C EEPROM.
  * @param  pBuffer: pointer to the buffer  containing the data to be written
  *         to the EEPROM.
  * @param  WriteAddr: EEPROM's internal address to write to.
  * @param  NumByteToWrite: number of bytes to write to the EEPROM.
  * @retval EEPROM_OK (0) if operation is correctly performed, else return value
  *         different from EEPROM_OK (0) or the timeout user callback.
  */
uint32_t BSP_EEPROM_WriteBuffer(uint8_t *pBuffer, uint16_t WriteAddr, uint16_t NumByteToWrite)
{
  uint32_t status;
  uint16_t addr;
  addr = WriteAddr % EEPROM_PAGESIZE;

  /* If WriteAddr is EEPROM_PAGESIZE aligned */
  if (addr == 0U)
  {
    status = EEPROM_WriteBufferAligned(pBuffer, WriteAddr, NumByteToWrite);
  }
  /* If WriteAddr is not EEPROM_PAGESIZE aligned */
  else
  {
    status = EEPROM_WriteBufferNotAligned(addr, pBuffer, WriteAddr, NumByteToWrite);
  }

  /* If all operations OK, return EEPROM_OK (0) */
  return (status);
}

/**
  * @brief  Wait for EEPROM Standby state.
  *
  * @note  This function allows to wait and check that EEPROM has finished the
  *        last operation. It is mostly used after Write operation: after receiving
  *        the buffer to be written, the EEPROM may need additional time to actually
  *        perform the write operation. During this time, it doesn't answer to
  *        I2C packets addressed to it. Once the write operation is complete
  *        the EEPROM responds to its address.
  *
  * @retval EEPROM_OK (0) if operation is correctly performed, else return value
  *         different from EEPROM_OK (0) or the timeout user callback.
  */
uint32_t BSP_EEPROM_WaitEepromStandbyState(void)
{
  uint32_t status = EEPROM_OK;

  /* Check if the maximum allowed number of trials has bee reached */
  if (BSP_I2C1_IsDeviceReady(EEPROM_I2C_ADDRESS, EEPROM_MAX_TRIALS) != BSP_ERROR_NONE)
  {
    /* If the maximum number of trials has been reached, exit the function */
    BSP_EEPROM_TIMEOUT_UserCallback();
    status = EEPROM_TIMEOUT;
  }
  return (status);
}

/**
  * @brief  Basic management of the timeout situation.
  * @retval None
  */
__weak void BSP_EEPROM_TIMEOUT_UserCallback(void)
{
}

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_EEPROM_Private_Functions STM32L462E_CELL1 EEPROM Private Functions
  * @{
  */

static uint32_t EEPROM_WriteLessThanPage(uint16_t addr, uint8_t *pBuffer, uint16_t WriteAddr, uint16_t NumByteToWrite)
{
  uint32_t status;

  uint16_t numofsingle;
  uint16_t count;
  uint16_t dataindex;

  count = EEPROM_PAGESIZE - addr;
  numofsingle = NumByteToWrite % EEPROM_PAGESIZE;

  /* If the number of data to be written is more than the remaining space
  in the current page: */
  if (NumByteToWrite > count)
  {
    /* Store the number of data to be written */
    dataindex = count;
    /* Write the data contained in same page */
    status = BSP_EEPROM_WritePage(pBuffer, WriteAddr, (uint16_t *)(&dataindex));
    if (status == EEPROM_OK)
    {
      /* Store the number of data to be written */
      dataindex = (NumByteToWrite - count);
      /* Write the remaining data in the following page */
      status = BSP_EEPROM_WritePage((uint8_t *)(pBuffer + count), (WriteAddr + count), (uint16_t *)(&dataindex));
    }
  }
  else
  {
    /* Store the number of data to be written */
    dataindex = numofsingle;
    status = BSP_EEPROM_WritePage(pBuffer, WriteAddr, (uint16_t *)(&dataindex));
  }

  return status;
}

static uint32_t EEPROM_WriteMoreThanPage(uint16_t addr, uint8_t *pBuffer, uint16_t WriteAddr, uint16_t NumByteToWrite)
{
  uint32_t status = EEPROM_OK;

  uint16_t numofpage;
  uint16_t numofsingle;
  uint16_t count;
  uint16_t dataindex;
  uint16_t writeAdd = WriteAddr;

  count = EEPROM_PAGESIZE - addr;
  uint16_t numByteToWrite = NumByteToWrite - count;
  numofpage =  numByteToWrite / EEPROM_PAGESIZE;
  numofsingle = numByteToWrite % EEPROM_PAGESIZE;

  if (count != 0U)
  {
    /* Store the number of data to be written */
    dataindex = count;
    status = BSP_EEPROM_WritePage(pBuffer, writeAdd, (uint16_t *)(&dataindex));
    if (status == EEPROM_OK)
    {
      writeAdd += count;
      pBuffer += count;
    }
  }

  if (status == EEPROM_OK)
  {
    while (numofpage != 0U)
    {
      numofpage--;
      /* Store the number of data to be written */
      dataindex = EEPROM_PAGESIZE;
      status = BSP_EEPROM_WritePage(pBuffer, writeAdd, (uint16_t *)(&dataindex));
      if (status == EEPROM_OK)
      {
        writeAdd +=  EEPROM_PAGESIZE;
        pBuffer += EEPROM_PAGESIZE;
      }
      else
      {
        /* leave the while loop */
        break;
      }
    }

    if (status == EEPROM_OK)
    {
      if (numofsingle != 0U)
      {
        /* Store the number of data to be written */
        dataindex = numofsingle;
        status = BSP_EEPROM_WritePage(pBuffer, writeAdd, (uint16_t *)(&dataindex));
      }
    }
  }
  return status;
}

static uint32_t EEPROM_WriteBufferAligned(uint8_t *pBuffer, uint16_t WriteAddr, uint16_t NumByteToWrite)
{
  uint32_t status = EEPROM_OK;

  uint16_t numofpage;
  uint16_t numofsingle;
  uint16_t dataindex;
  uint16_t writeAdd = WriteAddr;

  numofpage =  NumByteToWrite / EEPROM_PAGESIZE;
  numofsingle = NumByteToWrite % EEPROM_PAGESIZE;

  /* If NumByteToWrite < EEPROM_PAGESIZE */
  if (numofpage == 0U)
  {
    /* Store the number of data to be written */
    dataindex = numofsingle;
    /* Start writing data */
    status = BSP_EEPROM_WritePage(pBuffer, writeAdd, (uint16_t *)(&dataindex));
  }
  /* If NumByteToWrite > EEPROM_PAGESIZE */
  else
  {
    while (numofpage != 0U)
    {
      numofpage--;
      /* Store the number of data to be written */
      dataindex = EEPROM_PAGESIZE;
      status = BSP_EEPROM_WritePage(pBuffer, writeAdd, (uint16_t *)(&dataindex));
      if (status == EEPROM_OK)
      {
        writeAdd +=  EEPROM_PAGESIZE;
        pBuffer += EEPROM_PAGESIZE;
      }
      else
      {
        /* leave the while loop */
        break;
      }
    }

    if (status == EEPROM_OK)
    {
      if (numofsingle != 0U)
      {
        /* Store the number of data to be written */
        dataindex = numofsingle;
        status = BSP_EEPROM_WritePage(pBuffer, writeAdd, (uint16_t *)(&dataindex));
      }
    }
  }

  return status;
}

static uint32_t EEPROM_WriteBufferNotAligned(uint16_t addr, uint8_t *pBuffer, uint16_t WriteAddr,
                                             uint16_t NumByteToWrite)
{
  uint32_t status;

  uint16_t numofpage;
  numofpage =  NumByteToWrite / EEPROM_PAGESIZE;

  /* If NumByteToWrite < EEPROM_PAGESIZE */
  if (numofpage == 0U)
  {
    status = EEPROM_WriteLessThanPage(addr, pBuffer, WriteAddr, NumByteToWrite);
  }
  /* If NumByteToWrite > EEPROM_PAGESIZE */
  else
  {
    status = EEPROM_WriteMoreThanPage(addr, pBuffer, WriteAddr, NumByteToWrite);
  }

  return status;
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
