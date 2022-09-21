/**
  ******************************************************************************
  * @file    at_custom_modem_signalling.h
  * @author  MCD Application Team
  * @brief   Header for at_custom_modem_signalling.c module
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
#ifndef AT_CUSTOM_SIGNALLING_MONARCH_H
#define AT_CUSTOM_SIGNALLING_MONARCH_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "at_core.h"
#include "at_parser.h"
#include "at_modem_api.h"
#include "at_modem_signalling.h"
#include "at_custom_modem_specific.h"

/** @addtogroup AT_CUSTOM AT_CUSTOM
  * @{
  */

/** @addtogroup AT_CUSTOM_SEQUANS_MONARCH AT_CUSTOM SEQUANS_MONARCH
  * @{
  */

/** @addtogroup AT_CUSTOM_SEQUANS_MONARCH_SIGNALLING AT_CUSTOM SEQUANS_MONARCH SIGNALLING
  * @{
  */

/** @defgroup AT_CUSTOM_SEQUANS_MONARCH_SIGNALLING_Exported_Functions
  * AT_CUSTOM SEQUANS_MONARCH SIGNALLING Exported Functions
  * @{
  */

/* MONARCH specific build commands */
at_status_t fCmdBuild_ATD_MONARCH(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_SQNCTM_MONARCH(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_AUTOATT_MONARCH(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_CGDCONT_REPROGRAM_MONARCH(atparser_context_t *p_atp_ctxt,
                                                atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_CLEAR_SCAN_CFG_MONARCH(atparser_context_t *p_atp_ctxt,
                                             atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_ADD_SCAN_BAND_MONARCH(atparser_context_t *p_atp_ctxt,
                                            atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_SQNDNSLKUP_MONARCH(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_SMST_MONARCH(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);

/* MONARCH specific analyze commands */
at_action_rsp_t fRspAnalyze_Error_MONARCH(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                          const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_SQNCCID_MONARCH(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                            const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_SQNDNSLKUP_MONARCH(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                               const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_SMST_MONARCH(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                         const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_CESQ_MONARCH(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                         const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);

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

#endif /* AT_CUSTOM_SIGNALLING_MONARCH_H */

