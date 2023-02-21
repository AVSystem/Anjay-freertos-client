/**
  ******************************************************************************
  * @file    sfu_def.h
  * @author  MCD Application Team
  * @brief   This file contains the general definitions for SBSFU application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file in
  * the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef SFU_DEF_H
#define SFU_DEF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#if defined(__CC_ARM) || defined (__ARMCC_VERSION)
#include "mapping_sbsfu.h"
#elif defined (__ICCARM__) || defined(__GNUC__)
#include "mapping_export.h"
#endif /* __ICCARM__ || __GNUC__ */
#include "app_sfu.h"

/* Exported types ------------------------------------------------------------*/
/**
  * @brief  SFU Error Typedef
  */
typedef enum
{
  SFU_ERROR = 0x00001FE1U,
  SFU_SUCCESS = 0x00122F11U
} SFU_ErrorStatus;

/* Exported constants --------------------------------------------------------*/
#if defined(__CC_ARM) || defined (__ARMCC_VERSION)
extern uint32_t Image$$vector_start$$Base;
#define  INTVECT_START ((uint32_t)& Image$$vector_start$$Base)
#endif /* __CC_ARM */

#define SFU_RAM_BASE             ((uint32_t) SE_REGION_RAM_START)
#define SFU_RAM_END              ((uint32_t) SB_REGION_RAM_END)
#define SFU_SB_RAM_BASE          ((uint32_t) SB_REGION_RAM_START)
#define SFU_SB_RAM_END           ((uint32_t) SB_REGION_RAM_END)

/* needed for sfu_test.c */
#define SFU_SRAM2_BASE           ((uint32_t)0x10000000)
#define SFU_SRAM2_END            ((uint32_t)0x10007FFF)

#define SFU_BOOT_BASE_ADDR       ((uint32_t) INTVECT_START)           /* SFU Boot Address */
#define SFU_ROM_ADDR_END         ((uint32_t) SB_REGION_ROM_END)       /* SBSFU end Address (covering all the SBSFU
                                                                         executable code) */

#define SFU_SENG_ROM_ADDR_START  ((uint32_t) SE_CODE_REGION_ROM_START)/* Secure Engine area address START */
#define SFU_SENG_ROM_ADDR_END    ((uint32_t) SE_CODE_REGION_ROM_END)  /* Secure Engine area address END - SE includes
                                                                         everything up to the License */
#define SFU_SENG_ROM_SIZE        ((uint32_t) (SFU_SENG_ROM_ADDR_END - \
                                              SFU_SENG_ROM_ADDR_START + 1U)) /* Secure Engine area size */

#define SFU_KEYS_ROM_ADDR_START  ((uint32_t) SE_KEY_REGION_ROM_START) /* Keys Area (Keys + Keys Retrieve function)
                                                                         START. This is the PCRoP Area */
#define SFU_KEYS_ROM_ADDR_END    ((uint32_t) SE_KEY_REGION_ROM_END)   /* Keys Area (Keys + Keys Retrieve function) END.
                                                                         This is the PCRoP Area */

#define SFU_SENG_RAM_ADDR_START  ((uint32_t) SE_REGION_RAM_START)   /* Secure Engine reserved RAM1 area START
                                                                         address */
#define SFU_SENG_RAM_ADDR_END    ((uint32_t) SE_REGION_RAM_END)     /* Secure Engine reserved RAM1 area END address */
#define SFU_SENG_RAM_SIZE        ((uint32_t) (SFU_SENG_RAM_ADDR_END - \
                                              SFU_SENG_RAM_ADDR_START + 1U)) /* Secure Engine reserved RAM area SIZE */

#ifdef __cplusplus
}
#endif

#endif /* SFU_DEF_H */
