/**
  ******************************************************************************
  * @file    plf_power_config.h
  * @author  MCD Application Team
  * @brief   This file contains the power default configuration
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef PLF_POWER_CONFIG_H
#define PLF_POWER_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Default power mode */
#define DC_POWER_MODE_DEFAULT                           CA_POWER_IDLE

/* Power sleep request timeout default value */
#define DC_POWER_SLEEP_REQUEST_TIMEOUT_DEFAULT          (20000U) /* units: in milliseconds
                                                                    default: 20000ms = 20s */

/* eDRX values definition for Cat.M1 (WB-S1) */
#define DC_EDRX_WB_S1_PTW_1S_DRX_40S  (uint8_t)(0x03) /* "0000.0011" = 0x03 : WB-S1 mode PTW=1.28 sec, EDRX=40.96 sec */
#define DC_EDRX_WB_S1_PTW_1S_DRX_81S  (uint8_t)(0x05) /* "0000.0101" = 0x05 : WB-S1 mode PTW=1.28 sec, EDRX=81.92 sec */
/* eDRX values definition for Cat.NB1 (NB-S1) */
#define DC_EDRX_NB_S1_PTW_2S_DRX_40S  (uint8_t)(0x03) /* "0000.0011" = 0x03 : NB-S1 mode PTW=2.56 sec, EDRX=40.96 sec */
#define DC_EDRX_NB_S1_PTW_2S_DRX_81S  (uint8_t)(0x05) /* "0000.0101" = 0x05 : NB-S1 mode PTW=2.56 sec, EDRX=81.92 sec */

/* PSM values definition */
#define DC_PSM_T3312_DEACTIVATED (uint8_t)(0xE0) /* "111 00000" = 0xE0 */
#define DC_PSM_T3314_DEACTIVATED (uint8_t)(0xE0) /* "111 00000" = 0xE0 */

#define DC_PSM_T3412_1_MIN       (uint8_t)(0xA1) /* "101.00001" = 0xA1 */
#define DC_PSM_T3412_6_MIN       (uint8_t)(0xA6) /* "101.00110" = 0xA6 */
#define DC_PSM_T3412_4_HOURS     (uint8_t)(0x24) /* "001.00100" = 0x24 */

#define DC_PSM_T3324_10_SEC      (uint8_t)(0x05) /* "000.00101" = 0x05 */
#define DC_PSM_T3324_16_SEC      (uint8_t)(0x08) /* "000.01000" = 0x08 */
#define DC_PSM_T3324_4_MIN       (uint8_t)(0x24) /* "001.00100" = 0x24 */


/* PSM default values */
#define USE_TEST_VALUES (1)
#define USE_CATM1_NETWORK (0)

#if (USE_CATM1_NETWORK == 1)

/* cat.M1  ------------------------------------------------------------------- */
#if (USE_TEST_VALUES == 0)
/* default PSM values */
#define DC_POWER_PSM_REQ_PERIODIC_RAU_DEFAULT           DC_PSM_T3312_DEACTIVATED
#define DC_POWER_PSM_REQ_GPRS_READY_TIMER_DEFAULT       DC_PSM_T3314_DEACTIVATED
#define DC_POWER_PSM_REQ_PERIODIC_TAU_DEFAULT           DC_PSM_T3412_4_HOURS
#define DC_POWER_PSM_REQ_ACTIVE_TIMER_DEFAULT           DC_PSM_T3324_16_SEC
/* default cat.M1 EDRX values */
#define DC_POWER_EDRX_ACT_TYPE_DEFAULT                  CA_EIDRX_ACT_E_UTRAN_WBS1
#define DC_POWER_EDRX_REQ_VALUE_DEFAULT                 DC_EDRX_WB_S1_PTW_1S_DRX_40S

#else
/* test PSM values */
#define DC_POWER_PSM_REQ_PERIODIC_RAU_DEFAULT           DC_PSM_T3312_DEACTIVATED
#define DC_POWER_PSM_REQ_GPRS_READY_TIMER_DEFAULT       DC_PSM_T3314_DEACTIVATED
#define DC_POWER_PSM_REQ_PERIODIC_TAU_DEFAULT           DC_PSM_T3412_6_MIN
#define DC_POWER_PSM_REQ_ACTIVE_TIMER_DEFAULT           DC_PSM_T3324_10_SEC
/* test cat.M1 EDRX values */
#define DC_POWER_EDRX_ACT_TYPE_DEFAULT                  CA_EIDRX_ACT_E_UTRAN_WBS1
#define DC_POWER_EDRX_REQ_VALUE_DEFAULT                 DC_EDRX_WB_S1_PTW_1S_DRX_81S

#endif /* USE_TEST_VALUES == 0 */

#else
/* cat.NB1 ------------------------------------------------------------------- */
#if (USE_TEST_VALUES == 0)
/* default PSM values */
#define DC_POWER_PSM_REQ_PERIODIC_RAU_DEFAULT           DC_PSM_T3312_DEACTIVATED
#define DC_POWER_PSM_REQ_GPRS_READY_TIMER_DEFAULT       DC_PSM_T3314_DEACTIVATED
#define DC_POWER_PSM_REQ_PERIODIC_TAU_DEFAULT           DC_PSM_T3412_4_HOURS
#define DC_POWER_PSM_REQ_ACTIVE_TIMER_DEFAULT           DC_PSM_T3324_16_SEC
/* default cat.NB1 EDRX values */
#define DC_POWER_EDRX_ACT_TYPE_DEFAULT                  CA_EDRX_ACT_E_UTRAN_NBS1
#define DC_POWER_EDRX_REQ_VALUE_DEFAULT                 DC_EDRX_NB_S1_PTW_2S_DRX_40S

#else
/* test PSM values */
#define DC_POWER_PSM_REQ_PERIODIC_RAU_DEFAULT           DC_PSM_T3312_DEACTIVATED
#define DC_POWER_PSM_REQ_GPRS_READY_TIMER_DEFAULT       DC_PSM_T3314_DEACTIVATED
#define DC_POWER_PSM_REQ_PERIODIC_TAU_DEFAULT           DC_PSM_T3412_6_MIN
#define DC_POWER_PSM_REQ_ACTIVE_TIMER_DEFAULT           DC_PSM_T3324_4_MIN
/* test cat.NB1 EDRX values */
#define DC_POWER_EDRX_ACT_TYPE_DEFAULT                  CA_EDRX_ACT_E_UTRAN_NBS1
#define DC_POWER_EDRX_REQ_VALUE_DEFAULT                 DC_EDRX_NB_S1_PTW_2S_DRX_81S
#endif /* USE_TEST_VALUES == 0 */

#endif /* USE_CATM1_NETWORK == 1  --------------------------------------------*/


#ifdef __cplusplus
}
#endif

#endif /* PLF_POWER_CONFIG_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
