/**
  ******************************************************************************
  * @file    at_util.h
  * @author  MCD Application Team
  * @brief   Header for at_util.c module
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
#ifndef AT_UTIL_H
#define AT_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

/** @addtogroup AT_CORE AT_CORE
  * @{
  */

/** @addtogroup AT_CORE_UTIL AT_CORE UTIL
  * @{
  */

/** @defgroup AT_CORE_UTIL_Exported_Functions AT_CORE UTIL Exported Functions
  * @{
  */
uint32_t ATutil_ipow(uint32_t base, uint16_t exp);
uint32_t ATutil_convertStringToInt(const uint8_t *p_string, uint16_t size);
uint32_t ATutil_convertHexaStringToInt32(const uint8_t *p_string, uint16_t size);
uint8_t  ATutil_convertHexaStringToInt64(const uint8_t *p_string, uint16_t size, uint32_t *high_part_value,
                                         uint32_t *low_part_value);
uint32_t ATutil_convertBinStringToInt32(const uint8_t *p_string, uint16_t size);
void     ATutil_convertStringToUpperCase(uint8_t *p_string, uint16_t size);
uint8_t  ATutil_isNegative(const uint8_t *p_string, uint16_t size);
uint8_t  ATutil_convert_uint8_to_binary_string(uint32_t value, uint8_t nbBits, uint8_t sizeStr, uint8_t *binStr);
uint16_t ATutil_remove_quotes(const uint8_t *p_Src, uint16_t srcSize, uint8_t *p_Dst, uint16_t dstSize);
uint16_t ATutil_extract_str_from_quotes(const uint8_t *p_Src, uint16_t srcSize, uint8_t *p_Dst, uint16_t dstSize);

uint32_t ATutil_extract_hex_value_from_quotes(const uint8_t *p_str, uint16_t str_size, uint8_t param_size);
uint32_t ATutil_extract_bin_value_from_quotes(const uint8_t *p_str, uint16_t str_size, uint8_t param_size);
uint32_t ATutil_convert_T3412_to_seconds(uint32_t encoded_value);
uint32_t ATutil_convert_T3324_to_seconds(uint32_t encoded_value);
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
#endif

#endif /* AT_UTIL_H */
