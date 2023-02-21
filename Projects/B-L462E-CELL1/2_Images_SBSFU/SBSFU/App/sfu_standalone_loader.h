/**
  ******************************************************************************
  * @file    sfu_standalone_loader.h
  * @author  MCD Application Team
  * @brief   This file contains definitions for Secure Firmware Update standalone
  *          loader.
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
#ifndef SFU_STANDALONE_LOADER_H
#define SFU_STANDALONE_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Exported types ------------------------------------------------------------*/
#if (SECBOOT_LOADER == SECBOOT_USE_STANDALONE_LOADER)
/**
  * Standalone loader status definitions : information saved in non-initialized RAM shared memory
  * ==> LOADER_COM_REGION_RAM_START
  */
#define STANDALONE_LOADER_NO_REQ         0x00000000U /*!< No request */
#define STANDALONE_LOADER_DWL_REQ        0x375AA32CU /*!< Download function execution requested */
#define STANDALONE_LOADER_BYPASS_REQ     0xABCDEF01U /*!< By pass mode request */
#define STANDALONE_LOADER_INSTALL_REQ    0x01234567U /*!< User App installation request */

#define STANDALONE_LOADER_STATE          (*(uint32_t *)LOADER_COM_REGION_RAM_START)
#endif

#ifdef __cplusplus
}
#endif

#endif /* SFU_STANDALONE_LOADER_H */

