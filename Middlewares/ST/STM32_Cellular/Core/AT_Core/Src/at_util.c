/**
  ******************************************************************************
  * @file    at_util.c
  * @author  MCD Application Team
  * @brief   This file provides code for atcore utilities
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

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdbool.h>
#include "at_util.h"
#include "plf_config.h"

/** @addtogroup AT_CORE AT_CORE
  * @{
  */

/** @addtogroup AT_CORE_UTIL AT_CORE UTIL
  * @{
  */

/** @defgroup AT_CORE_UTIL_Private_Defines AT_CORE UTIL Private Defines
  * @{
  */
#define MAX_32BITS_STRING_SIZE (8U)  /* = max string size for a 32bits value (FFFF.FFFF) */
#define MAX_64BITS_STRING_SIZE (16U) /* = max string size for a 64bits value (FFFF.FFFF.FFFF.FFFF) */
#define MAX_PARAM_SIZE ((uint16_t)32U) /* max size of string */
#define ASCII_VALUE_X_LOWERCASE (120U) /* ASCII value of x=120 */
/**
  * @}
  */

/** @defgroup AT_CORE_UTIL_Exported_Functions AT_CORE UTIL Exported Functions
  * @{
  */
uint32_t ATutil_ipow(uint32_t base, uint16_t exp)
{
  uint16_t local_exp = exp;
  uint32_t local_base = base;

  /* implementation of power function */
  uint32_t result = 1U;
  while (local_exp != 0U)
  {
    if ((local_exp & 1U) != 0U)
    {
      result *= local_base;
    }
    local_exp >>= 1;
    local_base *= local_base;
  }

  return result;
}

uint32_t ATutil_convertStringToInt(const uint8_t *p_string, uint16_t size)
{
  uint32_t conv_nbr = 0U;

  /* auto-detect if this is an hexa value (format: 0x....) */
  if ((size > 2U) && (p_string[1] == ASCII_VALUE_X_LOWERCASE))
  {
    conv_nbr = ATutil_convertHexaStringToInt32(p_string, size);
  }
  else
  {
    uint16_t idx;
    uint16_t nb_digit_ignored = 0U;
    uint16_t loop = 0U;

    /* decimal value */
    for (idx = 0U; idx < size; idx++)
    {
      /* consider only the numbers */
      if ((p_string[idx] >= 48U) && (p_string[idx] <= 57U))
      {
        loop++;
        conv_nbr = conv_nbr +
                   (((uint32_t) p_string[idx] - 48U) * ATutil_ipow(10U, (size - loop - nb_digit_ignored)));
      }
      else
      {
        nb_digit_ignored++;
      }
    }
  }

  return (conv_nbr);
}

uint32_t ATutil_convertHexaStringToInt32(const uint8_t *p_string, uint16_t size)
{
  uint32_t conv_nbr = 0U; /* returned value = converted numder (0 if an error occurs) */
  uint16_t idx;
  uint16_t nb_digit_ignored;
  uint16_t loop = 0U;
  uint16_t str_size_to_convert;

  /* This function assumes that the string value is an hexadecimal value with or without Ox prefix
   * It converts a string to its hexadecimal value (32 bits value)
   * example:
   * explicit input string format from "0xW" to "0xWWWWXXXX"
   * implicit input string format from "W" to "WWWWXXXX"
   * where X,Y,W and Z are characters from '0' to 'F'
   */

  /* auto-detect if 0x is present */
  if ((size > 2U) && (p_string[1] == ASCII_VALUE_X_LOWERCASE))
  {
    /* 0x is present */
    nb_digit_ignored = 2U;
  }
  else
  {
    /* 0x is not present */
    nb_digit_ignored = 0U;
  }

  /* if 0x is present, we can skip it */
  str_size_to_convert = size - nb_digit_ignored;

  /* check maximum string size */
  if (str_size_to_convert <= MAX_32BITS_STRING_SIZE)
  {
    /* convert string to hexa value */
    for (idx = nb_digit_ignored; idx < size; idx++)
    {
      if ((p_string[idx] >= 48U) && (p_string[idx] <= 57U))
      {
        /* 0 to 9 */
        loop++;
        conv_nbr = conv_nbr +
                   (((uint32_t)p_string[idx] - 48U) * ATutil_ipow(16U, (size - loop - nb_digit_ignored)));
      }
      else if ((p_string[idx] >= 97U) && (p_string[idx] <= 102U))
      {
        /* a to f */
        loop++;
        conv_nbr = conv_nbr +
                   (((uint32_t)p_string[idx] - 97U + 10U) * ATutil_ipow(16U, (size - loop - nb_digit_ignored)));
      }
      else if ((p_string[idx] >= 65U) && (p_string[idx] <= 70U))
      {
        /* A to F */
        loop++;
        conv_nbr = conv_nbr +
                   (((uint32_t)p_string[idx] - 65U + 10U) * ATutil_ipow(16U, (size - loop - nb_digit_ignored)));
      }
      else
      {
        nb_digit_ignored++;
      }
    }
  }

  return (conv_nbr);
}

uint8_t ATutil_convertHexaStringToInt64(const uint8_t *p_string, uint16_t size, uint32_t *high_part_value,
                                        uint32_t *low_part_value)
{
  uint8_t retval;
  uint16_t nb_digit_ignored;
  uint16_t str_size_to_convert;
  uint16_t high_part_size;
  uint16_t low_part_size;

  /* This function assumes that the string value is an hexadecimal value with or without Ox prefix
   * It converts a string to its hexadecimal value (64 bits made of two 32 bits values)
   * example:
   * explicit input string format from "0xW" to "0xWWWWXXXXYYYYZZZZ"
   * implicit input string format from "W" to "WWWWXXXXYYYYZZZZ"
   * where X,Y,W and Z are characters from '0' to 'F'
   */

  /* init decoded values */
  *high_part_value = 0U;
  *low_part_value = 0U;

  /* auto-detect if 0x is present */
  if ((size > 2U) && (p_string[1] == ASCII_VALUE_X_LOWERCASE))
  {
    /* 0x is present */
    nb_digit_ignored = 2U;
  }
  else
  {
    /* 0x is not present */
    nb_digit_ignored = 0U;
  }

  /* if 0x is present, we can skip it */
  str_size_to_convert = size - nb_digit_ignored;

  /* check maximum string size */
  if (str_size_to_convert > MAX_64BITS_STRING_SIZE)
  {
    /* conversion error */
    retval = 0U;
  }
  else
  {
    if (str_size_to_convert > 8U)
    {
      high_part_size = str_size_to_convert - 8U;
      /* convert upper part if exists */
      *high_part_value = ATutil_convertHexaStringToInt32((const uint8_t *)(p_string + nb_digit_ignored),
                                                         high_part_size);
    }
    else
    {
      high_part_size = 0U;
    }

    /* convert lower part */
    low_part_size = str_size_to_convert - high_part_size;
    *low_part_value =  ATutil_convertHexaStringToInt32((const uint8_t *)(p_string + nb_digit_ignored + high_part_size),
                                                       low_part_size);

    /* string successfully converted */
    retval = 1U;
  }

  return (retval);
}

uint32_t ATutil_convertBinStringToInt32(const uint8_t *p_string, uint16_t size)
{
  uint32_t conv_nbr = 0U; /* returned value = converted numder (returns 0 if an error occurs) */

  for (uint16_t i = 0; i < size; i++)
  {
    /* convert ASCII character to its value (0x31 for 1, 0x30 for 0) */
    uint32_t bit = (p_string[size - i - 1U] == 0x31U) ? 1U : 0U;
    /* bit weight */
    uint32_t weight = ATutil_ipow(2U, i);
    conv_nbr += bit * weight;
  }

  return (conv_nbr);
}

uint8_t ATutil_isNegative(const uint8_t *p_string, uint16_t size)
{
  /* returns 1 if number in p_string is negative */
  uint8_t isneg = 0U;
  uint16_t idx;

  /* search for "-" until to find a valid number */
  for (idx = 0U; idx < size; idx++)
  {
    /* search for "-" */
    if (p_string[idx] == 45U)
    {
      isneg = 1U;
    }
    /* check for leave-loop condition (negative or valid number found) */
    if ((isneg == 1U) ||
        ((p_string[idx] >= 48U) && (p_string[idx] <= 57U)))
    {
      break;
    }
  }
  return (isneg);
}

void ATutil_convertStringToUpperCase(uint8_t *p_string, uint16_t size)
{
  uint16_t idx = 0U;
  while ((p_string[idx] != 0U) && (idx < size))
  {
    /* if lower case character... */
    if ((p_string[idx] >= 97U) && (p_string[idx] <= 122U))
    {
      /* ...convert it to uppercase character */
      p_string[idx] -= 32U;
    }
    idx++;
  }
}

/**
  * @brief  Converts a value to its binary string
  *         example: the decimal value 54531 (=0xD503) will be converted to the
  *                  string 1101010100000011 with /0 has end character
  * @param  value the decimal value to convert.
  * @param  nbBits number of bits to compute.
  * @param  sizeStr size of result string (should be equal or greater than nbBits + 1).
  * @param  binStr ptr to the result string.
  * @retval 0 if no error, 1 if an error occurred.
  */
uint8_t ATutil_convert_uint8_to_binary_string(uint32_t value, uint8_t nbBits, uint8_t sizeStr, uint8_t *binStr)
{
  uint8_t retval;

  /* String need to be at least one character more than the number of bits */
  if (sizeStr > nbBits)
  {
    for (uint8_t i = 0U; i < nbBits; i++)
    {
      /* convert to binary string */
      binStr[nbBits - i - 1U] = (((value >> i) % 2U) == 0U) ? 0x30U : 0x31U;
    }
    /* set end string character */
    binStr[nbBits] = 0U;
    retval = 0U;
  }
  else
  {
    retval = 1U;
  }

  return (retval);
}

/**
  * @brief  Remove double quotes "" from a p_Src buffer,
  *         and recopy the result to p_Dst buffer
  * @param  p_Src ptr to source buffer (string with quotes)
  * @param  srcSize of p_Src buffer
  * @param  p_Dst ptr to Destination Buffer (string without quotes)
  * @param  dstSize of p_Dst buffer
  * @retval size of destination string (util part).
  */
uint16_t ATutil_remove_quotes(const uint8_t *p_Src, uint16_t srcSize, uint8_t *p_Dst, uint16_t dstSize)
{
  uint16_t src_idx;
  uint16_t dest_idx = 0U;

  /* reset p_Dst buffer */
  (void) memset((void *)p_Dst, 0, dstSize);

  /* parse p_Src */
  for (src_idx = 0; ((src_idx < srcSize) && (dest_idx < dstSize)); src_idx++)
  {
    /* remove quotes from the string */
    if (p_Src[src_idx] != 0x22U)
    {
      /* write to p_Dst*/
      p_Dst[dest_idx] = p_Src[src_idx];
      dest_idx++;
    }
  }

  return (dest_idx);
}

/**
  * @brief  Extract a string between 2 quotes (remove what is before and after the quotes)
  * @note   if no valid string is found (something between 2 quotes), size returned is null
  * @param  p_Src ptr to source buffer (string with quotes)
  * @param  srcSize of p_Src buffer
  * @param  p_Dst ptr to Destination Buffer (string without quotes)
  * @param  dstSize of p_Dst buffer
  * @retval size of destination string (util part).
  */
uint16_t ATutil_extract_str_from_quotes(const uint8_t *p_Src, uint16_t srcSize, uint8_t *p_Dst, uint16_t dstSize)
{
  uint16_t src_idx;
  uint16_t dest_idx = 0U;
  bool copy_char = false;
  uint8_t count_quotes = 0U;

  /* reset p_Dst buffer */
  (void) memset((void *)p_Dst, 0, dstSize);

  /* parse p_Src */
  for (src_idx = 0;
       ((src_idx < srcSize) && (dest_idx < dstSize) && (count_quotes <= 1U));
       src_idx++)
  {
    if (p_Src[src_idx] == 0x22U)
    {
      /* quote found */
      count_quotes++;
      if (count_quotes == 1U)
      {
        copy_char = true;
      }
      else
      {
        copy_char = false;
      }
    }
    /* check copy_char only is this is not a quote */
    else if (copy_char)
    {
      /* write to p_Dst*/
      p_Dst[dest_idx] = p_Src[src_idx];
      dest_idx++;
    }
    else
    {
      /* nothing to do (MISRA) */
      __NOP();
    }
  }

  /* if final quote was not found: returns string found length is null (not valid) */
  if (count_quotes != 2U)
  {

    dest_idx = 0U;
  }

  return (dest_idx);
}

/*
 * Extract the value of an hexadecimal parameter from a string
 */
uint32_t ATutil_extract_hex_value_from_quotes(const uint8_t *p_str, uint16_t str_size, uint8_t param_size)
{
  uint32_t converted_value;
  if (str_size <= MAX_PARAM_SIZE)
  {
    uint8_t tmp_array[MAX_PARAM_SIZE] = {0};
    uint16_t real_size;
    real_size = ATutil_remove_quotes(p_str, str_size, &tmp_array[0], param_size);
    converted_value = ATutil_convertHexaStringToInt32(&tmp_array[0], real_size);
  }
  else
  {
    converted_value = 0U;
  }
  return (converted_value);
}

/*
 * Extract the value of an binary parameter from a string
 */
uint32_t ATutil_extract_bin_value_from_quotes(const uint8_t *p_str, uint16_t str_size, uint8_t param_size)
{
  uint32_t converted_value;
  if (str_size <= MAX_PARAM_SIZE)
  {
    uint8_t tmp_array[MAX_PARAM_SIZE] = {0};
    uint16_t real_size;
    real_size = ATutil_remove_quotes(p_str, str_size, &tmp_array[0], param_size);
    converted_value = ATutil_convertBinStringToInt32(&tmp_array[0], real_size);
  }
  else
  {
    converted_value = 0U;
  }
  return (converted_value);
}

/**
  * @brief  Convert PSM timer T3412 (periodic TAU) to seconds
  * @note   value received in CEREG
  * @param  encoded_value Timer value encoded as per 3GPP TS 27.007 and 24.008
  * @retval decoded value in seconds (returns 0 if no valid value decoded)
  */
uint32_t ATutil_convert_T3412_to_seconds(uint32_t encoded_value)
{
  /* cf Fig. 10.5.147a of TS 24.008 / Table 10.5.163a of TS 24.008 */
  static const uint32_t AT_T3412_UNIT_COEFF[8] =
  {
    ((uint32_t)600U),     /* 0 : 10 min = 600 sec */
    ((uint32_t)3600U),    /* 1 : 1 hr = 3600 sec */
    ((uint32_t)36000U),   /* 2 : 10 hrs = 36000 sec */
    ((uint32_t)2U),       /* 3 : 2 sec */
    ((uint32_t)30U),      /* 4 : 30 sec */
    ((uint32_t)60U),      /* 5 : 1 min = 60 sec */
    ((uint32_t)1152000U), /* 6 : 320 hrs = sec */
    ((uint32_t)0U)        /* 7 : timer deactivated */
  };

  uint32_t decode_value;

  /* Bits 1 to 5 represent timer value
   * Bits 6 to 8 represent timer unit
   *
   * unit part is encoded on 3 bits and will always be < 8 (ie the size of AT_T3412_UNIT_COEFF)
   */
  uint32_t unit_part = (0x000000E0U & encoded_value) >> 5;
  uint32_t value_part = (0x0000001FU & encoded_value);

  decode_value = value_part * AT_T3412_UNIT_COEFF[unit_part];

  return (decode_value);
}

/**
  * @brief  Convert PSM timer T3324 (active time) to seconds
  * @note   value received in CEREG
  * @param  encoded_value Timer value encoded as per 3GPP TS 27.007 and 24.008
  * @retval decoded value in seconds (returns 0 if no valid value decoded)
  */
uint32_t ATutil_convert_T3324_to_seconds(uint32_t encoded_value)
{
  /* cf  Fig. 10.5.147 of TS 24.008 / Table 10.5.163 of TS 24.008
   * and Fig. 10.5.146 of TS 24.008 / Table 10.5.172 of TS 24.008
   */
  static const uint32_t AT_T3324_UNIT_COEFF[8] =
  {
    ((uint32_t)2U),       /* 0 : 2 sec */
    ((uint32_t)60U),      /* 1 : 1 min = 60 sec */
    ((uint32_t)36000U),   /* 2 : 10 hrs = 36000 sec */
    ((uint32_t)0U),       /* invalid value */
    ((uint32_t)0U),       /* invalid value */
    ((uint32_t)0U),       /* invalid value */
    ((uint32_t)0U),       /* invalid value */
    ((uint32_t)0U)        /* 7 : timer deactivated */
  };

  uint32_t decode_value;
  /* Bits 1 to 5 represent timer value
   * Bits 6 to 8 represent timer unit
   *
   * unit part is encoded on 3 bits and will always be < 8 (ie the size of AT_T3412_UNIT_COEFF)
   */
  uint32_t unit_part = (0x000000E0U & encoded_value) >> 5;
  uint32_t value_part = (0x0000001FU & encoded_value);
  decode_value = value_part * AT_T3324_UNIT_COEFF[unit_part];

  return (decode_value);
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
