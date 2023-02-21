/**
  ******************************************************************************
  * @file    sfu_low_level_security.c
  * @author  MCD Application Team
  * @brief   SFU Security Low Level Interface module
  *          This file provides set of firmware functions to manage SFU security
  *          low level interface.
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "sfu_low_level_security.h"
#include "sfu_low_level_flash_int.h"
#include "sfu_low_level.h"
#include "sfu_trace.h"
#include "sfu_boot.h"
#include "sfu_fsm_states.h" /* needed for sfu_error.h */
#include "sfu_error.h"
#include "sfu_standalone_loader.h"
#include "stm32l4xx_it.h"

#ifndef  SFU_WRP_PROTECT_ENABLE
#warning "SFU_WRP_PROTECT_DISABLED"
#endif /* SFU_WRP_PROTECT_ENABLE */

#ifndef  SFU_RDP_PROTECT_ENABLE
#warning "SFU_RDP_PROTECT_DISABLED"
#endif /* SFU_RDP_PROTECT_ENABLE */


#ifndef  SFU_MPU_PROTECT_ENABLE
#warning "SFU_MPU_PROTECT_DISABLED"
#endif /* SFU_MPU_PROTECT_ENABLE */

#ifndef  SFU_FWALL_PROTECT_ENABLE
#warning "SFU_FWALL_PROTECT_DISABLED"
#endif /* SFU_FWALL_PROTECT_ENABLE */


#ifndef  SFU_TAMPER_PROTECT_ENABLE
#warning "SFU_TAMPER_PROTECT_DISABLED"
#endif /* SFU_TAMPER_PROTECT_ENABLE */

#ifndef  SFU_DAP_PROTECT_ENABLE
#warning "SFU_DAP_PROTECT_DISABLED"
#endif /* SFU_DAP_PROTECT_ENABLE */

#ifndef  SFU_DMA_PROTECT_ENABLE
#warning "SFU_DMA_PROTECT_DISABLED"
#endif /* SFU_DMA_PROTECT_ENABLE */

#ifndef  SFU_IWDG_PROTECT_ENABLE
#warning "SFU_IWDG_PROTECT_DISABLED"
#endif /* SFU_IWDG_PROTECT_ENABLE */


/* Private typedef -----------------------------------------------------------*/
typedef enum
{
  SFU_FALSE = 0U,
  SFU_TRUE = !SFU_FALSE
} SFU_BoolTypeDef;

typedef struct
{
  uint8_t                Number;            /*!< Specifies the number of the region to protect. This parameter can be a
                                                 value of CORTEX_MPU_Region_Number */
  uint32_t               BaseAddress;       /*!< Specifies the base address of the region to protect. */
  uint8_t                Size;              /*!< Specifies the size of the region to protect. */
  uint8_t                AccessPermission;  /*!< Specifies the region access permission type. This parameter can be a
                                                 value of CORTEX_MPU_Region_Permission_Attributes */
  uint8_t                DisableExec;       /*!< Specifies the instruction access status. This parameter can be a value
                                                 of  CORTEX_MPU_Instruction_Access */
  uint8_t                SubRegionDisable;  /*!< Specifies the sub region field (region is divided in 8 slices) when bit
                                                 is 1 region sub region is disabled */
} SFU_MPU_InitTypeDef;

typedef uint32_t      SFU_ProtectionTypeDef;  /*!<   SFU HAL IF Protection Type Def*/

/* Private variables ---------------------------------------------------------*/
#ifdef  SFU_IWDG_PROTECT_ENABLE
static IWDG_HandleTypeDef   IwdgHandle;

#endif /* SFU_IWDG_PROTECT_ENABLE */
#ifdef  SFU_MPU_PROTECT_ENABLE
static SFU_MPU_InitTypeDef MpuAreas[] =

  {
    {
      MPU_REGION_NUMBER0, SFU_PROTECT_MPU_AREA_USER_START,     SFU_PROTECT_MPU_AREA_USER_SIZE,
      SFU_PROTECT_MPU_AREA_USER_PERM,     SFU_PROTECT_MPU_AREA_USER_EXEC,     SFU_PROTECT_MPU_AREA_USER_SREG
    },
{
  MPU_REGION_NUMBER1, SFU_PROTECT_MPU_AREA_SFUEN_START_0,  SFU_PROTECT_MPU_AREA_SFUEN_SIZE_0,
  SFU_PROTECT_MPU_AREA_SFUEN_PERM,    SFU_PROTECT_MPU_AREA_SFUEN_EXEC,    SFU_PROTECT_MPU_AREA_SFUEN_SREG_0
},
{
  MPU_REGION_NUMBER2, SFU_PROTECT_MPU_AREA_SFUEN_START_1,  SFU_PROTECT_MPU_AREA_SFUEN_SIZE_1,
  SFU_PROTECT_MPU_AREA_SFUEN_PERM,    SFU_PROTECT_MPU_AREA_SFUEN_EXEC,    SFU_PROTECT_MPU_AREA_SFUEN_SREG_1
},
{
  MPU_REGION_NUMBER3, SFU_PROTECT_MPU_AREA_VECT_START,     SFU_PROTECT_MPU_AREA_VECT_SIZE,
  SFU_PROTECT_MPU_AREA_VECT_PERM,     SFU_PROTECT_MPU_AREA_VECT_EXEC,     SFU_PROTECT_MPU_AREA_VECT_SREG
},
{
  MPU_REGION_NUMBER4, SFU_PROTECT_MPU_AREA_OB_BANK1_START, SFU_PROTECT_MPU_AREA_OB_BANK1_SIZE,
  SFU_PROTECT_MPU_AREA_OB_BANK1_PERM, SFU_PROTECT_MPU_AREA_OB_BANK1_EXEC, SFU_PROTECT_MPU_AREA_OB_BANK1_SREG
},
{
  MPU_REGION_NUMBER5, SFU_PROTECT_MPU_AREA_PERIPH_START,   SFU_PROTECT_MPU_AREA_PERIPH_SIZE,
  SFU_PROTECT_MPU_AREA_PERIPH_PERM,   SFU_PROTECT_MPU_AREA_PERIPH_EXEC,   SFU_PROTECT_MPU_AREA_PERIPH_SREG
},
  };
#endif /* SFU_MPU_PROTECT_ENABLE */

/* Private function prototypes -----------------------------------------------*/
static SFU_ErrorStatus SFU_LL_SECU_CheckFlashConfiguration(FLASH_OBProgramInitTypeDef *psFlashOptionBytes);
static SFU_ErrorStatus SFU_LL_SECU_SetFlashConfiguration(FLASH_OBProgramInitTypeDef *psFlashOptionBytes,
                                                         SFU_BoolTypeDef *pbIsProtectionToBeApplied);

#ifdef SFU_RDP_PROTECT_ENABLE
static SFU_ErrorStatus SFU_LL_SECU_SetProtectionRDP(FLASH_OBProgramInitTypeDef *psFlashOptionBytes,
                                                    SFU_BoolTypeDef *pbIsProtectionToBeApplied);
#endif /*SFU_RDP_PROTECT_ENABLE*/

#ifdef SFU_WRP_PROTECT_ENABLE
static SFU_ErrorStatus SFU_LL_SECU_CheckProtectionWRP(FLASH_OBProgramInitTypeDef *psFlashOptionBytes);
static SFU_ErrorStatus SFU_LL_SECU_SetProtectionWRP(FLASH_OBProgramInitTypeDef *psFlashOptionBytes,
                                                    SFU_BoolTypeDef *pbIsProtectionToBeApplied);
#endif /*SFU_WRP_PROTECT_ENABLE*/


#ifndef SFU_DAP_PROTECT_ENABLE
#endif /*SFU_DAP_PROTECT_ENABLE*/

#ifdef SFU_FWALL_PROTECT_ENABLE
static SFU_ErrorStatus SFU_LL_SECU_CheckProtectionFWALL(void);
static SFU_ErrorStatus SFU_LL_SECU_SetProtectionFWALL(void);
#endif /* SFU_FWALL_PROTECT_ENABLE */

#ifdef SFU_IWDG_PROTECT_ENABLE
static SFU_ErrorStatus SFU_LL_SECU_SetProtectionIWDG(void);
#endif /*SFU_IWDG_PROTECT_ENABLE*/

#ifdef SFU_CLCK_MNTR_PROTECT_ENABLE
static SFU_ErrorStatus SFU_LL_SECU_SetProtectionCLOCK_MONITOR(void);
#endif /*SFU_CLCK_MNTR_PROTECT_ENABLE*/

#ifdef SFU_TEMP_MNTR_PROTECT_ENABLE
static SFU_ErrorStatus SFU_LL_SECU_SetProtectionTEMP_MONITOR(void);
#endif /*SFU_TEMP_MNTR_PROTECT_ENABLE*/



/* Functions Definition : helper ---------------------------------------------*/
#ifdef SFU_MPU_PROTECT_ENABLE
/**
  * @brief  Check MPU configuration
  * @param  MPU_InitStruct Configuration to be checked
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
static SFU_ErrorStatus MPU_CheckConfig(MPU_Region_InitTypeDef *pMPUInitStruct)
{
  uint32_t mpu_rasr = 0UL;

  /* Set the Region number */
  MPU->RNR = pMPUInitStruct->Number;

  mpu_rasr |= (((uint32_t)pMPUInitStruct->DisableExec        << MPU_RASR_XN_Pos) & MPU_RASR_XN_Msk);
  mpu_rasr |= (((uint32_t)pMPUInitStruct->AccessPermission   << MPU_RASR_AP_Pos) & MPU_RASR_AP_Msk);
  mpu_rasr |= (((uint32_t)pMPUInitStruct->TypeExtField       << MPU_RASR_TEX_Pos) & MPU_RASR_TEX_Msk);
  mpu_rasr |= (((uint32_t)pMPUInitStruct->IsShareable        << MPU_RASR_S_Pos) & MPU_RASR_S_Msk);
  mpu_rasr |= (((uint32_t)pMPUInitStruct->IsCacheable        << MPU_RASR_C_Pos) & MPU_RASR_C_Msk);
  mpu_rasr |= (((uint32_t)pMPUInitStruct->IsBufferable       << MPU_RASR_B_Pos) & MPU_RASR_B_Msk);
  mpu_rasr |= (((uint32_t)pMPUInitStruct->SubRegionDisable   << MPU_RASR_SRD_Pos) & MPU_RASR_SRD_Msk);
  mpu_rasr |= (((uint32_t)pMPUInitStruct->Size               << MPU_RASR_SIZE_Pos) & MPU_RASR_SIZE_Msk);
  mpu_rasr |= (((uint32_t)pMPUInitStruct->Enable             << MPU_RASR_ENABLE_Pos) & MPU_RASR_ENABLE_Msk);

  if (((MPU->RBAR & MPU_RBAR_ADDR_Msk) == pMPUInitStruct->BaseAddress) && (MPU->RASR == mpu_rasr))
  {
    return SFU_SUCCESS;
  }
  else
  {
    return SFU_ERROR;
  }
}
#endif /* SFU_MPU_PROTECT_ENABLE */

/* Functions Definition ------------------------------------------------------*/

/**
  * @brief  Check and if not applied apply the Static security  protections to
  *         all the SfuEn Sections in Flash: RDP, WRP, PCRoP. Static security protections
  *         those protections not impacted by a Reset. They are set using the Option Bytes
  *         When the device is locked (RDP Level2), these protections cannot be changed anymore
  * @param  None
  * @note   By default, the best security protections are applied to the different
  *         flash sections in order to maximize the security level for the specific MCU.
  * @retval uint32_t CRC (returned value is the combination of all the applied protections.
  *         If different from SFU_STD_PROTECTION_ALL, 1 or more protections cannot be applied)
  */
SFU_ErrorStatus SFU_LL_SECU_CheckApplyStaticProtections(void)
{
  FLASH_OBProgramInitTypeDef flash_option_bytes;
  SFU_BoolTypeDef is_protection_to_be_applied = SFU_FALSE;
  SFU_ErrorStatus e_ret_status = SFU_SUCCESS;

  /* Unlock the Flash to enable the flash control register access *************/
  (void) HAL_FLASH_Unlock();

  /* Clear OPTVERR bit set on virgin samples */
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);

  /* Unlock the Options Bytes *************************************************/
  (void) HAL_FLASH_OB_Unlock();

  /* Get Option Bytes status for FLASH_BANK_1: WRP AREA_A  and PCRoP **********/
  flash_option_bytes.WRPArea     = SFU_PROTECT_WRP_AREA_1;
  flash_option_bytes.PCROPConfig = FLASH_BANK_1;
  (void) HAL_FLASHEx_OBGetConfig(&flash_option_bytes);

  /* Check/Apply RDP_Level 1. This is the minimum protection allowed */
  /* if RDP_Level 2 is already applied it's not possible to modify the OptionBytes anymore */
  if (flash_option_bytes.RDPLevel == OB_RDP_LEVEL_2)
  {
    /* Sanity check of the (enabled) static protections */
    if (SFU_LL_SECU_CheckFlashConfiguration(&flash_option_bytes) != SFU_SUCCESS)
    {
      TRACE("\r\n= [SBOOT] Flash configuration failed! Product blocked.");
      /* Security issue : execution stopped ! */
      SFU_EXCPT_Security_Error();
    }

#ifdef SFU_WRP_PROTECT_ENABLE
    if (SFU_LL_SECU_CheckProtectionWRP(&flash_option_bytes) != SFU_SUCCESS)
    {
      TRACE("\r\n= [SBOOT] System Security Configuration failed! Product blocked.");
      /* Security issue : execution stopped ! */
      SFU_EXCPT_Security_Error();
    }
#endif /* SFU_WRP_PROTECT_ENABLE */



    /*RDP level 2 ==> Flow control by-passed */
    FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_RDP, FLOW_CTRL_RDP);
  }
  else
  {
    /* Check/Set Flash configuration *******************************************/
    e_ret_status = SFU_LL_SECU_SetFlashConfiguration(&flash_option_bytes, &is_protection_to_be_applied);

    /* Check/Apply WRP ********************************************************/
#ifdef SFU_WRP_PROTECT_ENABLE
    if (e_ret_status == SFU_SUCCESS)
    {
      e_ret_status = SFU_LL_SECU_SetProtectionWRP(&flash_option_bytes, &is_protection_to_be_applied);
    }
#endif /* SFU_WRP_PROTECT_ENABLE */

    /* Check/Apply PCRoP ******************************************************/


    /* Check/Apply RDP : RDP-L2 should be done as last option bytes configuration */
#ifdef SFU_RDP_PROTECT_ENABLE
    if (e_ret_status == SFU_SUCCESS)
    {
      e_ret_status = SFU_LL_SECU_SetProtectionRDP(&flash_option_bytes, &is_protection_to_be_applied);
    }
#endif  /* SFU_RDP_PROTECT_ENABLE */

    if (e_ret_status == SFU_SUCCESS)
    {
      if (is_protection_to_be_applied)
      {
        /* Generate System Reset to reload the new option byte values *************/
        /* WARNING: This means that if a protection can't be set, there will be a reset loop! */
        (void) HAL_FLASH_OB_Launch();
      }
    }
  }


  /* Lock the Options Bytes ***************************************************/
  (void) HAL_FLASH_OB_Lock();

  /* Lock the Flash to disable the flash control register access (recommended
  to protect the FLASH memory against possible unwanted operation) *********/
  (void) HAL_FLASH_Lock();

  /* If it was not possible to apply one of the above mandatory protections, the
  Option bytes have not been reloaded. Return the error status in order for the
  caller function to take the right actions */
  return e_ret_status;

}

/**
  * @brief  Apply Runtime security  protections.
  *         Runtime security protections have to be re-configured at each Reset.
  * @param  None
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LL_SECU_CheckApplyRuntimeProtections(uint8_t uStep)
{
  SFU_ErrorStatus e_ret_status = SFU_SUCCESS;
  SFU_ProtectionTypeDef runtime_protection = SFU_PROTECTIONS_NONE;

#ifdef SFU_MPU_PROTECT_ENABLE
  if (SFU_LL_SECU_SetProtectionMPU(uStep) == SFU_SUCCESS)
  {
    runtime_protection |= SFU_RUNTIME_PROTECTION_MPU;
  }
  else
  {
    /* When a protection cannot be set then SFU_ERROR is returned */
    e_ret_status = SFU_ERROR;
  }
#endif /* SFU_MPU_PROTECT_ENABLE */


#ifdef SFU_FWALL_PROTECT_ENABLE
  /* Check/Apply FWALL :
     - First Step : FWALL is configured
     - Second Step : Configuration is checked ==> FWALL cannot be configured twice */
  if (uStep == SFU_INITIAL_CONFIGURATION)
  {
    if (SFU_LL_SECU_SetProtectionFWALL() == SFU_SUCCESS)
    {
      runtime_protection |= SFU_RUNTIME_PROTECTION_FWALL;
    }
    else
    {
      /* When a protection cannot be set then SFU_ERROR is returned */
      e_ret_status = SFU_ERROR;
    }
  }
  else
  {
    if (SFU_LL_SECU_CheckProtectionFWALL() == SFU_SUCCESS)
    {
      runtime_protection |= SFU_RUNTIME_PROTECTION_FWALL;
    }
    else
    {
      /* When a protection cannot be set then SFU_ERROR is returned */
      e_ret_status = SFU_ERROR;
    }
  }
#endif /* SFU_FWALL_PROTECT_ENABLE */

  /* Check/Apply disable DMAs  ************************************************/
#ifdef SFU_DMA_PROTECT_ENABLE
  if (SFU_LL_SECU_SetProtectionDMA() == SFU_SUCCESS)
  {
    runtime_protection |= SFU_RUNTIME_PROTECTION_DMA;
  }
  else
  {
    /* When a protection cannot be set then SFU_ERROR is returned */
    e_ret_status = SFU_ERROR;
  }
#endif /* SFU_DMA_PROTECT_ENABLE */

  /* Check/Apply  IWDG **************************************************/
#ifdef SFU_IWDG_PROTECT_ENABLE
  if (SFU_LL_SECU_SetProtectionIWDG() == SFU_SUCCESS)
  {
    runtime_protection |= SFU_RUNTIME_PROTECTION_IWDG;
  }
  else
  {
    /* When a protection cannot be set then SFU_ERROR is returned */
    e_ret_status = SFU_ERROR;
  }
#endif /* SFU_IWDG_PROTECT_ENABLE */

  /* Check/Apply  DAP *********************************************************/
#ifdef SFU_DAP_PROTECT_ENABLE
  if (SFU_LL_SECU_SetProtectionDAP() == SFU_SUCCESS)
  {
    runtime_protection |= SFU_RUNTIME_PROTECTION_DAP;
  }
  else
  {
    /* When a protection cannot be set then SFU_ERROR is returned */
    e_ret_status = SFU_ERROR;
  }
#else
#endif /* SFU_DAP_PROTECT_ENABLE */

  /* Check/Apply  ANTI_TAMPER *************************************************/
#ifdef SFU_TAMPER_PROTECT_ENABLE
  if (SFU_LL_SECU_SetProtectionANTI_TAMPER() == SFU_SUCCESS)
  {
    runtime_protection |= SFU_RUNTIME_PROTECTION_ANTI_TAMPER;
  }
  else
  {
    /* When a protection cannot be set then SFU_ERROR is returned */
    e_ret_status = SFU_ERROR;
  }
#else
#ifdef SFU_TEST_PROTECTION
  if (SFU_LL_RTC_Init() != SFU_SUCCESS)
  {
    e_ret_status = SFU_ERROR;
  }
#endif /* SFU_TEST_PROTECTION */
#endif /* SFU_TAMPER_PROTECT_ENABLE */
  /* Check/Apply  CLOCK_MONITOR **********************************************/
#ifdef SFU_CLCK_MNTR_PROTECT_ENABLE
  if (SFU_LL_SECU_SetProtectionCLOCK_MONITOR() == SFU_SUCCESS)
  {
    runtime_protection |= SFU_RUNTIME_PROTECTION_CLOCK_MONITOR;
  }
  else
  {
    /* When a protection cannot be set then SFU_ERROR is returned */
    e_ret_status = SFU_ERROR;
  }
#endif /* SFU_CLCK_MNTR_PROTECT_ENABLE */

  /* Check/Apply  TEMP_MONITOR **********************************************/
#ifdef SFU_TEMP_MNTR_PROTECT_ENABLE
  if (SFU_LL_SECU_SetProtectionTEMP_MONITOR() == SFU_SUCCESS)
  {
    runtime_protection |= SFU_RUNTIME_PROTECTION_TEMP_MONITOR;
  }
  else
  {
    /* When a protection cannot be set then SFU_ERROR is returned */
    e_ret_status = SFU_ERROR;
  }
#endif /* SFU_TEMP_MNTR_PROTECT_ENABLE */

#if defined(SFU_VERBOSE_DEBUG_MODE)
  TRACE("\r\n= [SBOOT] RuntimeProtections: %x", runtime_protection);
#endif /* SFU_VERBOSE_DEBUG_MODE */

  return e_ret_status;
}

/**
  * @brief  Return the reset source  detected after a reboot. The related flag is reset
  *         at the end of this function.
  * @param  peResetpSourceId: to be filled with the detected source of reset
  * @note   In case of multiple reset sources this function return only one of them.
  *         It can be improved returning and managing a combination of them.
  * @retval SFU_SUCCESS if successful, SFU_ERROR otherwise
  */
void SFU_LL_SECU_GetResetSources(SFU_RESET_IdTypeDef *peResetpSourceId)
{
  /* Check if the last reset has been generated from a Watchdog exception */
  if ((__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET) ||
      (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST) != RESET))
  {
    *peResetpSourceId = SFU_RESET_WDG_RESET;

  }

#ifdef SFU_FWALL_PROTECT_ENABLE
  /* Check if the last reset has been generated from a Firewall exception */
  else if (__HAL_RCC_GET_FLAG(RCC_FLAG_FWRST) != RESET)
  {
    *peResetpSourceId = SFU_RESET_FIREWALL;

  }
#endif /* SFU_FWALL_PROTECT_ENABLE */

  /* Check if the last reset has been generated from a Low Power reset */
  else if (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST) != RESET)
  {
    *peResetpSourceId = SFU_RESET_LOW_POWER;

  }

  /* Check if the last reset has been generated from a Software reset  */
  else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST) != RESET)
  {
    *peResetpSourceId = SFU_RESET_SW_RESET;

  }
  /* Check if the last reset has been generated from an Option Byte Loader reset  */
  else if (__HAL_RCC_GET_FLAG(RCC_FLAG_OBLRST) != RESET)
  {
    *peResetpSourceId = SFU_RESET_OB_LOADER;
  }
  /* Check if the last reset has been generated from a Hw pin reset  */
  else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST) != RESET)
  {
    *peResetpSourceId = SFU_RESET_HW_RESET;

  }
  /* Unknown */
  else
  {
    *peResetpSourceId = SFU_RESET_UNKNOWN;
  }
}

/**
  * @brief  Clear the reset sources. This function should be called after the actions
  *         on the reset sources has been already taken.
  * @param  none
  * @note   none
  * @retval none
  */
void SFU_LL_SECU_ClearResetSources()
{
  /* Clear reset flags  */
  __HAL_RCC_CLEAR_RESET_FLAGS();
}

/**
  * @brief  Refresh Watchdog : reload counter
  *         This function must be called just before jumping to the UserFirmware
  * @param  None
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LL_SECU_IWDG_Refresh(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;

#ifdef SFU_IWDG_PROTECT_ENABLE
  {
    /* Refresh IWDG: reload counter */
    if (HAL_IWDG_Refresh(&IwdgHandle) == HAL_OK)
    {
      e_ret_status = SFU_SUCCESS;
    }
  }
#else
  e_ret_status = SFU_SUCCESS;
#endif /*SFU_IWDG_PROTECT_ENABLE*/

  return e_ret_status;
}

/**
  * @brief  Check Flash configuration.
  * @param  psFlashOptionBytes: pointer to the Option Bytes structure
  * @retval SFU_ErrorStatus SFU_SUCCESS if Flash configuration is correct, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LL_SECU_CheckFlashConfiguration(FLASH_OBProgramInitTypeDef *psFlashOptionBytes)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;

    e_ret_status = SFU_SUCCESS; /*BFB2 already disabled */
  if (e_ret_status == SFU_SUCCESS)
  {
    /* Execution stopped if flow control failed */
    FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_UBE, FLOW_CTRL_UBE);
  }
  return e_ret_status;

}

/**
  * @brief  Set Flash configuration.
  * @param  psFlashOptionBytes: pointer to the Option Bytes structure
  * @param  pbIsProtectionToBeApplied: Output parameter to be set as "TRUE" if
  *         this OptByte has to be modified and immediately reloaded.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LL_SECU_SetFlashConfiguration(FLASH_OBProgramInitTypeDef *psFlashOptionBytes,
                                                  SFU_BoolTypeDef *pbIsProtectionToBeApplied)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;

  /* Check Flash configuration */
  if (SFU_LL_SECU_CheckFlashConfiguration(psFlashOptionBytes) == SFU_SUCCESS)
  {
    e_ret_status = SFU_SUCCESS;
  }
  else
  {
    /* Unset BFB2 to avoid Boot in bank2, BFB2 bit shall be disabled */
      /* Execution stopped if flow control failed */
    /* Security issue : execution stopped ! */
  }

  return e_ret_status;
}

#ifdef SFU_RDP_PROTECT_ENABLE
/**
  * @brief  Apply the RDP protection
  * @param  psFlashOptionBytes: pointer to the Option Bytes structure
  * @param  pbIsProtectionToBeApplied: Output parameter to be set as "TRUE" if
  *         this OptByte has to be modified and immediately reloaded.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LL_SECU_SetProtectionRDP(FLASH_OBProgramInitTypeDef *psFlashOptionBytes,
                                             SFU_BoolTypeDef *pbIsProtectionToBeApplied)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;

  /* Check/Apply RDP **********************************************************/
  /* Please consider that the suggested and most secure approach is to set the RDP_LEVEL_2 */
  if (psFlashOptionBytes->RDPLevel == SFU_PROTECT_RDP_LEVEL)
  {
    e_ret_status = SFU_SUCCESS; /*Protection already applied */
    /* Execution stopped if flow control failed */
    FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_RDP, FLOW_CTRL_RDP);
  }
  else
  {
#if defined(SECBOOT_OB_DEV_MODE)
#if defined(SFU_FINAL_SECURE_LOCK_ENABLE)
    TRACE("\r\n\t  Applying RDP-2 Level. Product locked! You might need to unplug/plug the USB cable!");
#else
    TRACE("\r\n\t  Applying RDP-1 Level. You might need to unplug/plug the USB cable!");
#endif /* SFU_FINAL_SECURE_LOCK_ENABLE */
    psFlashOptionBytes->OptionType      = OPTIONBYTE_RDP;
    psFlashOptionBytes->RDPLevel        = SFU_PROTECT_RDP_LEVEL;
    if (HAL_FLASHEx_OBProgram(psFlashOptionBytes) == HAL_OK)
    {
      *pbIsProtectionToBeApplied |= 1U;
      e_ret_status = SFU_SUCCESS;
      /* Execution stopped if flow control failed */
      FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_RDP, FLOW_CTRL_RDP);
    }
#else
    TRACE("\r\n= [SBOOT] System Security Configuration failed: RDP is incorrect. STOP!");
    /* Security issue : execution stopped ! */
    SFU_EXCPT_Security_Error();
#endif /* SECBOOT_OB_DEV_MODE */
  }
  return e_ret_status;
}
#endif /*SFU_RDP_PROTECT_ENABLE*/

#ifdef SFU_WRP_PROTECT_ENABLE
/**
  * @brief  Check the WRP protection to the specified Area. It includes the SFU Vector Table
  * @param  psFlashOptionBytes: pointer to the Option Bytes structure
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LL_SECU_CheckProtectionWRP(FLASH_OBProgramInitTypeDef *psFlashOptionBytes)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;

  /* Check WRP ****************************************************************/
  if ((psFlashOptionBytes->WRPStartOffset == SFU_PROTECT_WRP_PAGE_START_1) &&
      (psFlashOptionBytes->WRPEndOffset   == SFU_PROTECT_WRP_PAGE_END_1))
  {
    e_ret_status = SFU_SUCCESS; /*Protection applied */
  }
  if (e_ret_status == SFU_SUCCESS)
  {
    /* Execution stopped if flow control failed */
    FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_WRP, FLOW_CTRL_WRP);
  }
  return e_ret_status;
}

/**
  * @brief  Apply the WRP protection to the specified Area. It includes the SFU Vector Table
  * @param  psFlashOptionBytes: pointer to the Option Bytes structure
  * @param  pbIsProtectionToBeApplied: Output parameter to be set as "TRUE" if
  *         this OptByte has to be modified and immediately reloaded.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LL_SECU_SetProtectionWRP(FLASH_OBProgramInitTypeDef *psFlashOptionBytes,
                                             SFU_BoolTypeDef *pbIsProtectionToBeApplied)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;

  /* Check/Apply WRP **********************************************************/
  if (SFU_LL_SECU_CheckProtectionWRP(psFlashOptionBytes) == SFU_SUCCESS)
  {
    e_ret_status = SFU_SUCCESS; /*Protection already applied */
  }
  else
  {
#if defined(SECBOOT_OB_DEV_MODE)
    psFlashOptionBytes->OptionType     = OPTIONBYTE_WRP;
    psFlashOptionBytes->WRPArea        = SFU_PROTECT_WRP_AREA_1;
    psFlashOptionBytes->WRPStartOffset = SFU_PROTECT_WRP_PAGE_START_1;
    psFlashOptionBytes->WRPEndOffset   = SFU_PROTECT_WRP_PAGE_END_1;

    if (HAL_FLASHEx_OBProgram(psFlashOptionBytes) == HAL_OK)
    {
      *pbIsProtectionToBeApplied |= 1U;
      e_ret_status = SFU_SUCCESS;
      /* Execution stopped if flow control failed */
      FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_WRP, FLOW_CTRL_WRP);
    }
#else
    TRACE("\r\n= [SBOOT] System Security Configuration failed: WRP is incorrect. STOP!");
    /* Security issue : execution stopped ! */
    SFU_EXCPT_Security_Error();
#endif /* SECBOOT_OB_DEV_MODE */
  }

  return e_ret_status;
}
#endif /*SFU_WRP_PROTECT_ENABLE*/

#ifdef SFU_MPU_PROTECT_ENABLE
/**
  * @brief  Apply MPU protection before executing UserApp
  * @param  None
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LL_SECU_SetProtectionMPU_UserApp(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;

  MPU_InitStruct.Enable               = MPU_REGION_ENABLE;
  MPU_InitStruct.Number               = MPU_REGION_NUMBER6;
  MPU_InitStruct.BaseAddress          = APP_PROTECT_MPU_AREA_ACTIVE_SLOT_START;
  MPU_InitStruct.Size                 = APP_PROTECT_MPU_AREA_ACTIVE_SLOT_SIZE;
  MPU_InitStruct.SubRegionDisable     = APP_PROTECT_MPU_AREA_ACTIVE_SLOT_SREG;
  MPU_InitStruct.AccessPermission     = APP_PROTECT_MPU_AREA_ACTIVE_SLOT_PERM;
  MPU_InitStruct.DisableExec          = APP_PROTECT_MPU_AREA_ACTIVE_SLOT_EXEC;
  MPU_InitStruct.IsShareable          = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsBufferable         = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable          = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.TypeExtField         = MPU_TEX_LEVEL0;
  HAL_MPU_ConfigRegion(&MPU_InitStruct);



  return SFU_SUCCESS;
}
#endif /* SFU_MPU_PROTECT_ENABLE */

#if  (SECBOOT_LOADER == SECBOOT_USE_STANDALONE_LOADER)
/**
  * @brief  Apply MPU protection before executing Standalone loader
  * @param  None
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LL_SECU_SetProtectionMPU_StandaloneLoader(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;

  MPU_InitStruct.Enable               = MPU_REGION_ENABLE;
  MPU_InitStruct.Number               = MPU_REGION_NUMBER6;
  MPU_InitStruct.BaseAddress          = LOADER_PROTECT_MPU_AREA_START;
  MPU_InitStruct.Size                 = LOADER_PROTECT_MPU_AREA_SIZE;
  MPU_InitStruct.SubRegionDisable     = LOADER_PROTECT_MPU_AREA_SREG;
  MPU_InitStruct.AccessPermission     = LOADER_PROTECT_MPU_AREA_PERM;
  MPU_InitStruct.DisableExec          = LOADER_PROTECT_MPU_AREA_EXEC;
  MPU_InitStruct.IsShareable          = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsBufferable         = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable          = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.TypeExtField         = MPU_TEX_LEVEL0;
  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  return SFU_SUCCESS;
}
#endif /* (SECBOOT_LOADER == SECBOOT_USE_STANDALONE_LOADER) */

#ifdef SFU_MPU_PROTECT_ENABLE

/**
  * @brief  Apply MPU protection
  * @param  uStep Configuration step : SFU_INITIAL_CONFIGURATION, SFU_SECOND_CONFIGURATION, SFU_THIRD_CONFIGURATION
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LL_SECU_SetProtectionMPU(uint8_t uStep)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;
  /**
    * ===== MPU regions =====
    * The executable code is made of: {SB_SFU vectors, SE_CoreBin, SB_SFU}
    * This executable code is mapped with 2 MPU regions:
    *   - Region 1: size up to 128kB with up to 8*16kB sub-regions
    *   - Region 2: size up to 16kB  with up to 8*2kB sub-regions
    * Region 2 is placed right after Region 1, even if Region 1 is smaller than 128kB.
    *
    *
    * IMPORTANT: the maximum executable size is NOT 128kB + 16kB.
    *            The algorithm accepts a size up to 128kB + 16kB - 1 byte.
    *
    * Examples:
    *
    * 1. Region 1 is 128kB long with Region 2 being 4kB long
    * <--------------128kB---------------><-------16kB--------->
    * [Reg.1.1][Reg.1.2].........[Reg.1.8][Reg.2.1][Reg.2.2].FF.   (.FF. means deactivated)
    *
    * 2. Region 1 is 64kB long with Region 2 being 6kB long
    * <---------------64kB---------------><------------16kB------------->
    * [Reg.1.1][Reg.1.2].........[Reg.1.4][Reg.2.1][Reg.2.2][Reg.2.3].FF.   (.FF. means deactivated)
    *
    * The size covered by region 1 corresponds to the quotient of the sbsfu code size divided by 16kB.
    * The size covered by region 2 corresponds to the remainder of the sbsfu code size divided by 16kB, rounded-up to
    * the next 2kB.
    */
  uint32_t region1_first_16kb_sub_region_disabled; /* index of the first sub-region to be disabled in region 1 */
  uint32_t region2_first_2kb_sub_region_disabled;  /* index of the first sub-region to be disabled in region 2 */
  /* The total size of the executable code is: SFU_ROM_ADDR_END - FLASH_BASE + 1 */
  uint32_t sbsfu_code_size = (SFU_ROM_ADDR_END - FLASH_BASE + 1U);  /* sbsfu, secure engine, callgate, se_interface,
                                                                      PCROPed keys... */
  uint8_t mpu_region_num; /* id of the MPU region being configured */

  /* Check/Apply MPU **********************************************************/
  MPU_Region_InitTypeDef MPU_InitStruct;

  /* Compute the number of 16Kbytes sub-regions required to cover the executable size in Region 1 (up to 8 maximum).
    * This number of sub-regions is the left-SHIFT value we apply to decide which regions are disabled.
    * The total size of the executable code is: SFU_ROM_ADDR_END - FLASH_BASE + 1 (sbsfu, secure engine, callgate,
    * se_interface, PCROPed keys...)
    *
    * WATCH OUT: This value cannot exceed SFU_PROTECT_MPU_MAX_NB_SUBREG (8 sub-regions).
    */
  region1_first_16kb_sub_region_disabled = sbsfu_code_size >> 14U ; /* shifting by 14 to divide by 16k */

  /* Compute the remainder of the code size divided by 16k */
  region2_first_2kb_sub_region_disabled = sbsfu_code_size & 16383U;

  /* Compute the number of 2Kbytes sub-regions required to cover the end of the executable in Region 2 (up to 8 maximum)
    * This number of sub-regions is the left-SHIFT value we apply to decide which regions are disabled.
    */
  if (region2_first_2kb_sub_region_disabled != 0U)
  {
    /*
      * The code size is not a multiple of 16k.
      * We compute the number of 2kB sub-regions needed in Region 2.
      *
      * +2047 to round-up to next 2k and shifting by 11 to divide by 2k
      */
    region2_first_2kb_sub_region_disabled = ((region2_first_2kb_sub_region_disabled + 2047U) >> 11U);
  }
  else
  {
    /*
      * No remainder: the code size is a multiple of 16k.
      * 2 possibilities but only 1 is supported:
      *  - All 2kB sub-regions are needed in Region 2 if the executable size is 128kB + 16kB.
      *    In this case, region1_first_16kb_sub_region_disabled reaches 9 so exceeds 8 and we return an error.
      *    NOT SUPPORTED
      *
      *  - Or no 2Kb sub-region is needed in Region 2 because the executable is contained fully in Region 1.
      *    SUPPORTED
      */
    region2_first_2kb_sub_region_disabled = 0; /* Will result in 0xFF << 0 : disable all sub-regions */
  }
  /*
    * If the executable size is more than 128kB + 16kB - 1 byte then we return an error,
    * because the algorithm would try configuring 9 sub-regions of 16kB.
    * Restriction: the current algorithm cannot compute that we need 8*16kB + 8*2kB.
    */
  if (region1_first_16kb_sub_region_disabled <= SFU_PROTECT_MPU_MAX_NB_SUBREG)
  {
    e_ret_status = SFU_SUCCESS;

#if defined(SFU_VERBOSE_DEBUG_MODE)
    TRACE("\r\n\t  [MPU] Region 1 with %d sub-region(s).", region1_first_16kb_sub_region_disabled);
    TRACE("\r\n\t  [MPU] Region 2 with %d sub-region(s).", region2_first_2kb_sub_region_disabled);
#endif /* SFU_VERBOSE_DEBUG_MODE */

    /* Sanity check: let's make sure the MPU settings cover the executable code */
    if (((region1_first_16kb_sub_region_disabled * 16384U) + (region2_first_2kb_sub_region_disabled * 2048U)) <
        sbsfu_code_size)
    {
      /* The MPU settings do not cover properly the executable code size */
      TRACE("\r\n [MPU] Executable code (%lu bytes) not fully covered !", sbsfu_code_size);
      return SFU_ERROR;
    }
    /* else the MPU settings cover at least the executable code size */

#if defined(SFU_VERBOSE_DEBUG_MODE)
    TRACE("\r\n= [SBOOT] MPU REGIONS CONFIGURATION");
#endif /* SFU_VERBOSE_DEBUG_MODE */
    /* The UserApp needs to know this configuration in case it wants to use the same WRP Areas */
    /* Initializes and configures the Region and the memory to be protected */
    for (mpu_region_num = 0U; mpu_region_num < (sizeof(MpuAreas) / sizeof(SFU_MPU_InitTypeDef)); mpu_region_num++)
    {
      MPU_InitStruct.Enable               = MPU_REGION_ENABLE;
      MPU_InitStruct.Number               = MpuAreas[mpu_region_num].Number;
      MPU_InitStruct.BaseAddress          = MpuAreas[mpu_region_num].BaseAddress;
      MPU_InitStruct.Size                 = MpuAreas[mpu_region_num].Size;
      MPU_InitStruct.SubRegionDisable     = MpuAreas[mpu_region_num].SubRegionDisable;
      MPU_InitStruct.TypeExtField         = MPU_TEX_LEVEL0;
      MPU_InitStruct.AccessPermission     = MpuAreas[mpu_region_num].AccessPermission;
      MPU_InitStruct.DisableExec          = MpuAreas[mpu_region_num].DisableExec;
      MPU_InitStruct.IsShareable          = MPU_ACCESS_NOT_SHAREABLE;
      MPU_InitStruct.IsCacheable          = MPU_ACCESS_CACHEABLE;
      MPU_InitStruct.IsBufferable         = MPU_ACCESS_NOT_BUFFERABLE;

#if defined(SFU_VERBOSE_DEBUG_MODE)
      TRACE("\r\n\t  Region %d", mpu_region_num);
#endif /* SFU_VERBOSE_DEBUG_MODE */

      if (mpu_region_num == 1U)
      {
        /* Region 1 (executable, up to 128kB with up to 8*16kB sub-regions) */
        MPU_InitStruct.SubRegionDisable = (uint8_t)(0xffU << region1_first_16kb_sub_region_disabled);
      }
      else if (mpu_region_num == 2U)
      {
        /* Region 2 (executable, up to 16kB with up to 8*2kB sub-regions) */
        MPU_InitStruct.SubRegionDisable = (uint8_t)(0xffU << region2_first_2kb_sub_region_disabled);
        /* The region start address is the 16k-aligned address before the end address of the executable area */
        MPU_InitStruct.BaseAddress = (SFU_ROM_ADDR_END  & ~16383U); /* 16k LSBs set to 0 */
      }
      else
      {
        /*  keep the settings from the constant */
      }

#if defined(SFU_VERBOSE_DEBUG_MODE)
      TRACE(" @:%x size:%x sub:%x perm:%x exec:%x",
            MPU_InitStruct.BaseAddress, MPU_InitStruct.Size, MPU_InitStruct.SubRegionDisable,
            MPU_InitStruct.AccessPermission,  MPU_InitStruct.DisableExec);
#endif /* SFU_VERBOSE_DEBUG_MODE */

      if (uStep == SFU_INITIAL_CONFIGURATION)
      {
        HAL_MPU_ConfigRegion(&MPU_InitStruct);
      }
      else
      {
        if (MPU_CheckConfig(&MPU_InitStruct) == SFU_ERROR)
        {
          return SFU_ERROR;
        }
      }
    }

    if (uStep == SFU_INITIAL_CONFIGURATION)
    {
#if defined(SFU_VERBOSE_DEBUG_MODE)
      TRACE("\r\n");
#endif /* SFU_VERBOSE_DEBUG_MODE */
      /* Enables the MPU */
      HAL_MPU_Enable(MPU_HARDFAULT_NMI);

#if defined(SCB_SHCSR_MEMFAULTENA_Msk)
      /* Enables memory fault exception */
      SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk;
#endif /* SCB_SHCSR_MEMFAULTENA_Msk */
    }
    else
    {
      if (MPU->CTRL != (MPU_HARDFAULT_NMI | MPU_CTRL_ENABLE_Msk))
      {
        return SFU_ERROR;
      }
#if defined(SCB_SHCSR_MEMFAULTENA_Msk)
      if ((SCB->SHCSR & SCB_SHCSR_MEMFAULTENA_Msk) != SCB_SHCSR_MEMFAULTENA_Msk)
      {
        return SFU_ERROR;
      }
#endif /* SCB_SHCSR_MEMFAULTENA_Msk */
    }

  } /* Else: too many sub-regions in Region 1, return SFU_ERROR */
#if defined(SFU_VERBOSE_DEBUG_MODE)
  else
  {
    TRACE("\r\n\t  [MPU] Too many MPU sub-regions in Region 1 : %d > %d", region1_first_16kb_sub_region_disabled,
          SFU_PROTECT_MPU_MAX_NB_SUBREG);
  }
#endif /* SFU_VERBOSE_DEBUG_MODE */

  if (e_ret_status == SFU_SUCCESS)
  {
    /* Execution stopped if flow control failed */
    FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_MPU, FLOW_CTRL_MPU);
  }
  return e_ret_status;
}
#endif /*SFU_MPU_PROTECT_ENABLE*/


#ifdef SFU_FWALL_PROTECT_ENABLE
/**
  * @brief  Check if FWALL protection is enabled.
  * @param  None
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LL_SECU_CheckProtectionFWALL(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;

  if (__HAL_FIREWALL_IS_ENABLED() != RESET)
  {
    /* Firewall enabled: from this point, the Firewall is closed */
    e_ret_status = SFU_SUCCESS;
    /* Execution stopped if flow control failed */
    FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_FWALL, FLOW_CTRL_FWALL);
  }
  return e_ret_status;
}

/**
  * @brief  Apply FWALL protection: CODE, NVDATA and VDATA.
  * @param  None
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LL_SECU_SetProtectionFWALL(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;
  FIREWALL_InitTypeDef FWALL_InitStruct;
  FIREWALL_InitTypeDef FWALL_VerifStruct;

  /* Protected code segment start address and length (it has to be multiple of 256 bytes). */
  FWALL_InitStruct.CodeSegmentStartAddress      = SFU_PROTECT_FWALL_CODE_ADDR_START;
  FWALL_InitStruct.CodeSegmentLength            = SFU_PROTECT_FWALL_CODE_SIZE;

  /* Protected Non volatile data segment (in Flash memory) start address and length */
  FWALL_InitStruct.NonVDataSegmentStartAddress    = SFU_PROTECT_FWALL_NVDATA_ADDR_START;
  FWALL_InitStruct.NonVDataSegmentLength          = SFU_PROTECT_FWALL_NVDATA_SIZE;

  /* Protected volatile data segment (in SRAM1 memory) start address and length */
  FWALL_InitStruct.VDataSegmentStartAddress    = SFU_PROTECT_FWALL_VDATA_ADDR_START;
  FWALL_InitStruct.VDataSegmentLength          = SFU_PROTECT_FWALL_VDATA_SIZE;

  /* The protected volatile data segment can't be executed */
  FWALL_InitStruct.VolatileDataExecution       = FIREWALL_VOLATILEDATA_NOT_EXECUTABLE;

  /* The protected volatile data segment is shared with non-protected
  application code */
  FWALL_InitStruct.VolatileDataShared          = FIREWALL_VOLATILEDATA_NOT_SHARED;

  /* Firewall configuration */
  if (HAL_FIREWALL_Config(&FWALL_InitStruct) == HAL_OK)
  {
    /* Verify firewall configuration */
    HAL_FIREWALL_GetConfig(&FWALL_VerifStruct);
    if ((FWALL_VerifStruct.CodeSegmentStartAddress == (SFU_PROTECT_FWALL_CODE_ADDR_START & FW_CSSA_ADD)) &&
        (FWALL_VerifStruct.CodeSegmentLength == (SFU_PROTECT_FWALL_CODE_SIZE & FW_CSL_LENG)) &&
        (FWALL_VerifStruct.NonVDataSegmentStartAddress == (SFU_PROTECT_FWALL_NVDATA_ADDR_START & FW_NVDSSA_ADD)) &&
        (FWALL_VerifStruct.NonVDataSegmentLength == (SFU_PROTECT_FWALL_NVDATA_SIZE & FW_NVDSL_LENG)) &&
        (FWALL_VerifStruct.VDataSegmentStartAddress == (SFU_PROTECT_FWALL_VDATA_ADDR_START & FW_VDSSA_ADD)) &&
        (FWALL_VerifStruct.VDataSegmentLength == (SFU_PROTECT_FWALL_VDATA_SIZE & FW_VDSL_LENG)) &&
        (FWALL_VerifStruct.VolatileDataExecution == (FIREWALL_VOLATILEDATA_NOT_EXECUTABLE & FW_CR_VDE)) &&
        (FWALL_VerifStruct.VolatileDataShared == (FIREWALL_VOLATILEDATA_NOT_SHARED & FW_CR_VDS)))
    {
      /* Enable Firewall */
      HAL_FIREWALL_EnableFirewall();
      /* Check the outcome of the enable procedure */
      if (__HAL_FIREWALL_IS_ENABLED() != RESET)
      {
        /* Firewall enabled: from this point, the Firewall is closed */
        e_ret_status = SFU_SUCCESS;
        /* Execution stopped if flow control failed */
        FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_FWALL, FLOW_CTRL_FWALL);
      }
    }
  }
  return e_ret_status;
}
#endif /* SFU_FWALL_PROTECT_ENABLE */

#ifdef SFU_DMA_PROTECT_ENABLE
/**
  * @brief  Apply DMA protection
  * @param  None
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LL_SECU_SetProtectionDMA(void)
{
  /*
   * In this function we disable the DMA buses in order to avoid that while the SB/SFU is running
   * some DMA has been already enabled (e.g. through debugger in RDP-1 after reset) in order to access sensitive
   * information in SRAM, FLASH
   */
  /* Disable  DMA1, DMA2 */
  __HAL_RCC_DMA1_CLK_DISABLE();

  __HAL_RCC_DMA2_CLK_DISABLE();





  /* Execution stopped if flow control failed */
  FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_DMA, FLOW_CTRL_DMA);
  return SFU_SUCCESS;
}
#endif /*SFU_DMA_PROTECT_ENABLE*/

#ifdef SFU_IWDG_PROTECT_ENABLE
/**
  *  @brief Apply IWDG protection
  *         The IWDG timeout is set to 4 second.
  *         Then, the IWDG reload counter is configured as below to obtain 4 second according
  *         to the measured LSI frequency after setting the prescaler value:
  *         IWDG counter clock Frequency = LSI Frequency / Prescaler value
  * @param  None
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LL_SECU_SetProtectionIWDG(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;

  /* TIMER could be used to get the LSI frequency in order to have a more precise IWDG.
  This is not used in this implementation because not necessary and in order
  to optimize code-size. If you are interested, please have a look at the IWDG Cube example. */

  /* Configure & Start the IWDG peripheral */
  /* Set counter reload value to obtain 6 sec. IWDG TimeOut.
  IWDG counter clock Frequency = uwLsiFreq
  Set Prescaler to 64 (IWDG_PRESCALER_64)
  Timeout Period = (Reload Counter Value * 64) / uwLsiFreq
  So Set Reload Counter Value = (6 * uwLsiFreq) / 64 */
  IwdgHandle.Instance = IWDG;
  IwdgHandle.Init.Prescaler = IWDG_PRESCALER_64;
  IwdgHandle.Init.Reload = (SFU_IWDG_TIMEOUT * LSI_VALUE / 64U);
  IwdgHandle.Init.Window = IWDG_WINDOW_DISABLE;

  {
    if (HAL_IWDG_Init(&IwdgHandle) == HAL_OK)
    {
      e_ret_status = SFU_SUCCESS;
      /* Execution stopped if flow control failed */
      FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_IWDG, FLOW_CTRL_IWDG);
    }
  }

  return e_ret_status;
}
#endif /*SFU_IWDG_PROTECT_ENABLE*/

#ifdef SFU_DAP_PROTECT_ENABLE
/**
  * @brief  Set DAP protection status, configuring SWCLK and SWDIO GPIO pins.
  * @param  None
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LL_SECU_SetProtectionDAP(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;

  GPIO_InitTypeDef GPIO_InitStruct;

  /* Enable clock of DBG GPIO port */
  SFU_DBG_CLK_ENABLE();

  /* Enable the DAP protections, so disable the DAP re-configuring SWCLK and SWDIO GPIO pins */
  GPIO_InitStruct.Pin = SFU_DBG_SWDIO_PIN | SFU_DBG_SWCLK_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(SFU_DBG_PORT, &GPIO_InitStruct);
  e_ret_status = SFU_SUCCESS;
  /* Execution stopped if flow control failed */
  FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_DAP, FLOW_CTRL_DAP);

  return e_ret_status;
}
#else
#endif /*SFU_DAP_PROTECT_ENABLE*/

#ifdef SFU_TAMPER_PROTECT_ENABLE
/**
  * @brief  Apply ANTI TAMPER protection
  * @param None
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LL_SECU_SetProtectionANTI_TAMPER(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;
  RTC_TamperTypeDef  stamperstructure;

  /* RTC_TAMPER_2 (PA0) selected. PC13 connected to RTC_TAMPER_1 is also connected to the USER button */
  TAMPER_GPIO_CLK_ENABLE();

  /* Configure Tamper Pin */
  /* tamper is an additional function */
  /* not an alternate Function : config not needed */
  /* Configure the RTC peripheral */
  /* Configure RTC prescaler and RTC data registers */
  /* RTC configured as follows:
  - Hour Format    = Format 24
  - Asynch Prediv  = Value according to source clock
  - Synch Prediv   = Value according to source clock
  - OutPut         = Output Disable
  - OutPutPolarity = High Polarity
  - OutPutType     = Open Drain */
  RtcHandle.Instance            = RTC;
  RtcHandle.Init.HourFormat     = RTC_HOURFORMAT_24;
  RtcHandle.Init.AsynchPrediv   = RTC_ASYNCH_PREDIV;
  RtcHandle.Init.SynchPrediv    = RTC_SYNCH_PREDIV;
  RtcHandle.Init.OutPut         = RTC_OUTPUT_DISABLE;
  RtcHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  RtcHandle.Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;

  if (HAL_RTC_Init(&RtcHandle) == HAL_OK)
  {
    /* Configure RTC Tamper */
    stamperstructure.Tamper                       = RTC_TAMPER_ID;
    stamperstructure.Trigger                      = RTC_TAMPERTRIGGER_FALLINGEDGE;
    stamperstructure.Filter                       = RTC_TAMPERFILTER_DISABLE;
    stamperstructure.SamplingFrequency            = RTC_TAMPERSAMPLINGFREQ_RTCCLK_DIV32768;
    stamperstructure.PrechargeDuration            = RTC_TAMPERPRECHARGEDURATION_1RTCCLK;
    stamperstructure.TamperPullUp                 = RTC_TAMPER_PULLUP_ENABLE;
    stamperstructure.TimeStampOnTamperDetection   = RTC_TIMESTAMPONTAMPERDETECTION_DISABLE;
    stamperstructure.NoErase                      = RTC_TAMPER_ERASE_BACKUP_ENABLE;
    stamperstructure.MaskFlag                     = RTC_TAMPERMASK_FLAG_DISABLE;
    stamperstructure.Interrupt                    = RTC_TAMPER_ID_INTERRUPT;

    if (HAL_RTCEx_SetTamper_IT(&RtcHandle, &stamperstructure) == HAL_OK)
    {

      /* Clear the Tamper interrupt pending bit */
      __HAL_RTC_TAMPER_CLEAR_FLAG(&RtcHandle, RTC_FLAG_TAMP2F);
      e_ret_status = SFU_SUCCESS;
      /* Execution stopped if flow control failed */
      FLOW_CONTROL_STEP(uFlowProtectValue, FLOW_STEP_TAMPER, FLOW_CTRL_TAMPER);
    }
  }

  return e_ret_status;
}
#endif /*SFU_TAMPER_PROTECT_ENABLE*/

#ifdef SFU_CLCK_MNTR_PROTECT_ENABLE
/**
  * @brief  Apply CLOCK MONITOR protection
  * @note   This function has been added just as template to be used/customized
  *         if a clock monitor is requested.
  * @param  None
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LL_SECU_SetProtectionCLOCK_MONITOR(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;

  /* Apply the Clock Monitoring */
  /* Add your code here for customization
     e.g. if HSE or LSE is used enable the CSS!
   ...
   ...
  */

  e_ret_status = SFU_SUCCESS;

  return e_ret_status;
}
#endif /*SFU_CLCK_MNTR_PROTECT_ENABLE*/

#ifdef SFU_TEMP_MNTR_PROTECT_ENABLE
/**
  * @brief  Apply TEMP MONITOR protection
  * @note   This function has been added just as template to be used/customized
  *         if a temperature monitor is requested.
  * @param  None
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LL_SECU_SetProtectionTEMP_MONITOR(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;

  /* Apply the Temperature Monitoring */
  /* Add your code here for customization
     ...
     ...
  */

  e_ret_status = SFU_SUCCESS;

  return e_ret_status;
}
#endif /*SFU_TEMP_MNTR_PROTECT_ENABLE*/


