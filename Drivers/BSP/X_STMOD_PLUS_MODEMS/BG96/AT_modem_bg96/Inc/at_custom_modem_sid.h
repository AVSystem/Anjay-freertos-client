/**
  ******************************************************************************
  * @file    at_custom_modem_sid.h
  * @author  MCD Application Team
  * @brief   Header for at_custom_modem_sid.c module for BG96
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2020-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef AT_CUSTOM_MODEM_SID_H
#define AT_CUSTOM_MODEM_SID_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "at_core.h"
#include "at_modem_api.h"
#include "at_sysctrl.h"

/** @addtogroup AT_CUSTOM AT_CUSTOM
  * @{
  */

/** @addtogroup AT_CUSTOM_QUECTEL_BG96 AT_CUSTOM QUECTEL_BG96
  * @{
  */

/** @addtogroup AT_CUSTOM_QUECTEL_BG96_SID AT_CUSTOM QUECTEL_BG96 SID
  * @{
  */

/** @defgroup AT_CUSTOM_QUECTEL_BG96_SID_Exported_Types AT_CUSTOM QUECTEL_BG96 SID Exported Types
  * @{
  */
typedef at_status_t (*SIDFuncTypeDef)(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                      atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);

typedef struct
{
  uint32_t       sid;
  SIDFuncTypeDef sid_Function;
} sid_LUT_t;
/**
  * @}
  */

/** @defgroup AT_CUSTOM_QUECTEL_BG96_SID_Exported_Functions AT_CUSTOM QUECTEL_BG96 SID Exported Functions
  * @{
  */
at_status_t at_custom_SID_bg96(atcustom_modem_context_t *p_mdm_ctxt,
                               at_context_t *p_at_ctxt,
                               uint32_t *p_ATcmdTimeout);
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
#endif

#endif /* AT_CUSTOM_MODEM_SID_H */
