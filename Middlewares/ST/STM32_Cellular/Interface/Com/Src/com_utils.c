/**
  ******************************************************************************
  * @file    com_utils.c
  * @author  MCD Application Team
  * @brief   This file implements ComLib Utilities
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "com_utils.h"

#include <stddef.h>

/* Private function Definition -----------------------------------------------*/
/**
  * @brief  Convert a Hexadecimal byte to its value [0,9],[a,f],[A,F]
  * @param  digit - hexadecimal value of [0,9],[a,f],[A,F]
  * @param  p_res - conversion result pointer
  * @retval true/false - conversion OK/NOK
  */
static bool com_utils_convertDigitToValue(uint8_t digit, uint8_t *p_res)
{
  bool result = true;

  if ((digit >= 48U) && (digit <= 57U))
  {
    /* 0 to 9 */
    *p_res = digit - 48U;
  }
  else if ((digit >= 97U) && (digit <= 102U))
  {
    /* a to f */
    *p_res = digit - 87U; /* 87 = -97+10 */
  }
  else if ((digit >= 65U) && (digit <= 70U))
  {
    /* A to F */
    *p_res = digit - 55U; /* 55 = -65+10*/
  }
  else
  {
    *p_res = 0U;
    result = false;
  }

  return (result);
}

/**
  * @brief  Convert a number to its hexadecimal value
  * @param  nbr - value to convert
  * @retval result of the conversion
  */
static uint8_t com_utils_convertToASCII(uint8_t nbr)
{
  uint8_t ascii;

  if (nbr <= 9U)
  {
    ascii = nbr + 48U;
  }
  else
  {
    ascii = nbr + 87U; /* 87 = 97 -10 (where 97 corresponds to 'a') */
  }
  return (ascii);
}

/* Functions Definition ------------------------------------------------------*/
/**
  * @brief  Convert two Hexadecimal byte to one Char byte
  * @note   example: msd:0x31 lsd:0x32 result = true *p_conv=0x12
  * @param  msd    - msd part possible value [0,9],[a,f],[A,F]
  * @param  lsd    - lsd part possible value [0,9],[a,f],[A,F]
  * @param  p_conv - conversion result pointer
  * @retval true/false - conversion OK/NOK
  */
bool com_utils_convertHEXToChar(uint8_t msd, uint8_t lsd, uint8_t *p_conv)
{
  bool result = false;
  uint8_t convMSD;
  uint8_t convLSD;

  if (p_conv != NULL)
  {
    /* convert Most significant digit */
    if (com_utils_convertDigitToValue(msd, &convMSD) == true)
    {
      /* convert Less significant digit */
      if (com_utils_convertDigitToValue(lsd, &convLSD) == true)
      {
        /* compute converted char value */
        *p_conv = (convMSD << 4) + convLSD;
        result = true;
      }
      /* else result = false */
    }
    /* else result = false */
  }
  /* else result = false */

  return (result);
}

/**
  * @brief  Convert one Char byte to two Hexadecimal
  * @note   example: val:0x12 result = true, *p_msd:0x31 *p_lsd:0x32
  * @param  val    - value to convert
  * @param  p_msd  - msd part possible value [0,9],[a,f]
  * @param  p_lsd  - lsd part possible value [0,9],[a,f]
  * @retval true/false - conversion OK/NOK
  */
bool com_utils_convertCharToHEX(uint8_t val, uint8_t *p_msd, uint8_t *p_lsd)
{
  bool result = false;

  if ((p_msd != NULL) && (p_lsd != NULL))
  {
    *p_msd = com_utils_convertToASCII(val / 16U);
    *p_lsd = com_utils_convertToASCII(val % 16U);
    result = true;
  }

  return (result);
}


/*** Component Initialization/Start *******************************************/
/*** Used by com_core module - Not an User Interface **************************/



/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
