/**
  ******************************************************************************
  * @file    plf_rtosal_config.h
  * @author  MCD Application Team
  * @brief   This file contains RTOS abstarction layer configuration.
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
#ifndef PLF_RTOSAL_CONFIG_H
#define PLF_RTOSAL_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
/*cstat -MISRAC2012-* */
#include "cmsis_os.h"
/*cstat +MISRAC2012-* */

/* Exported constants --------------------------------------------------------*/

/* Configure RTOSAL memory allocation service */
#if !defined RTOSAL_MALLOC
#define RTOSAL_MALLOC                  pvPortMalloc
#endif /* !defined RTOSAL_MALLOC */

/* Configure RTOSAL memory deallocation service */
#if !defined RTOSAL_FREE
#define RTOSAL_FREE                    vPortFree
#endif /* !defined RTOSAL_FREE */

/* Configure RTOSAL stack type size value */
/* Cellular thread stack sizes in plf_thread_config.h are expressed in dwords.
 * According to RTOS thread stack allocation implementation,
 * when osThreadNew() / osThreadCreate() are called, the stack unit to allocate is in bytes or in dwords.
 * So to always have the same thread stack size definition in plf_thread_config.h,
 * an adaptation should sometimes be done to have the correct thread stack size allocation.
 * This adaptation is done in rtosalThreadNew() using RTOSAL_STACK_TYPE_SIZE define.
 */
/* Some RTOS allocation service have stack_size parameter expressed in dwords or convert it from bytes to dwords;
 * in such case RTOSAL_STACK_TYPE_SIZE must be defined to 1.
 * Some RTOS allocation service have stack_size parameter expressed in bytes or don't convert it from bytes to dwords;
 * in such case RTOSAL_STACK_TYPE_SIZE must be defined to 4.
 */
#if !defined RTOSAL_STACK_TYPE_SIZE
#if (osCMSIS < 0x20000U)
#define RTOSAL_STACK_TYPE_SIZE         1
#else
#define RTOSAL_STACK_TYPE_SIZE         sizeof(StackType_t)
#endif /* osCMSIS < 0x20000U */
#endif /* !defined RTOSAL_STACK_TYPE_SIZE */

/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */


#ifdef __cplusplus
}
#endif

#endif /* PLF_RTOSAL_CONFIG_H */
