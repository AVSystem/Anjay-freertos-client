/**
  ******************************************************************************
  * @file    com_icc.h
  * @author  MCD Application Team
  * @brief   Header for com_icc.c module
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
#ifndef COM_ICC_H
#define COM_ICC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

#if (USE_COM_ICC == 1)

#include "com_common.h"

/** @addtogroup COM
  * @{
  */

/**
  ******************************************************************************
  @verbatim
  ==============================================================================
                    ##### How to use Com ICC interface #####
  ==============================================================================
  Allows to communicate with the International Circuit Card

  in Application, call:
  1) com_icc to obtain a icc session handle
  2) com_icc_generic_access for each command to send to the ICC
  3) com_closeicc to close the icc session

  At the moment only one session at a time is possible.
  This mechanism prevent concurrent access to the ICC
  e.g:
  Application1 request a Select on Master File, then a Select on a 1st level Dedicated File
  but before to have time to interact on a Elementary File in this Dedicated File,
  Application2 request a Select on a different Dedicated File

  To use this module define USE_COM_ICC must be set to 1

  @endverbatim

  */

/** @defgroup COM_ICC ICC: International Circuit Card
  * @{
  */


/* Exported constants --------------------------------------------------------*/
/** @defgroup COM_ICC_Constants Constants
  * @{
  */
/** @note COM_ICC_CSIM_MAX_CMD_GENERIC_ACCESS_SZ :
  * Protocol CSIM is supporting 256 characters
  * (*1) : apdu send through AT+CSIM is encoded char ASCII
  */
#define COM_ICC_CSIM_MAX_CMD_GENERIC_ACCESS_SZ ((uint32_t)(256U * 1U))

/** @note COM_ICC_CSIM_MIN_RSP_GENERIC_ACCESS_SZ :
  4U : to write SW1 and SW2\n
+ 1U : to always write \0 at the end of the string
  */
#define COM_ICC_CSIM_MIN_RSP_GENERIC_ACCESS_SZ ((uint32_t)(4U + 1U))

#if (USE_ST33 == 1)
/** @note COM_ICC_NDLC_MAX_CMD_GENERIC_ACCESS_SZ :
  * Protocol NDLC is supporting 256 characters but not NDLC stack yet
  * (*2U) : apdu send though NDLC is encoded 2 characters on one byte
  */
#define COM_ICC_NDLC_MAX_CMD_GENERIC_ACCESS_SZ ((uint32_t)(253U * 2U))

/** @note COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ :
  *   4U : to write SW1 and SW2\n
  * + 1U : to always write \0 at the end of the string
  */
#define COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ ((uint32_t)(4U + 1U))
#endif /* USE_ST33 == 1 */

/**
  * @}
  */

/* Exported types ------------------------------------------------------------*/
/* None */

/* External variables --------------------------------------------------------*/
/* None */

/* Exported macros -----------------------------------------------------------*/
/* None */

/* Exported functions ------------------------------------------------------- */
/**
  ******************************************************************************
  @verbatim
  ==============================================================================
                    ##### How to use Com ICC interface #####
  ==============================================================================
  Allows to communicate with the ICC

  in Application, call:
  1) com_icc to obtain a icc session handle
  2) com_icc_generic_access for each command to send to the ICC
  3) com_closeicc to close the icc session

  At the moment only one session at a time is possible.
  This mechanism prevent concurrent access to the ICC
  e.g:
  Application1 request a Select on Master File, then a Select on a 1st level Dedicated File
  but before to have time to interact on a Elementary File in this Dedicated File,
  Application2 request a Select on a different Dedicated File

  To use this module define USE_COM_ICC must be set to 1

  @endverbatim

  */

/** @defgroup COM_ICC_Functions Functions
  * @{
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

/** @addtogroup COM_ICC_Functions
  * @{
  */


/*** ICC functionalities ****************************************************/

/**
  * @brief  ICC handle creation
  * @note   Create a ICC session
  * @param  family   - address family
  * @note   Supported value: COM_AF_UNSPEC
  * @param  type     - connection type
  * @note   Supported value: COM_SOCK_SEQPACKET
  * @param  protocol - protocol type
  * @note   Supported values: COM_PROTO_CSIM : communication with ICC using AT+CSIM command
  *                           COM_PROTO_NDLC : communication with ICC using NDLC command
  * @retval int32_t  - icc handle or error value
  * @note   Possible returned value:\n
  *         int32_t > 0            : icc session created; the returned value is the icc handle session\n
  *         int32_t < 0            : an error occurred\n
  *         - COM_ERR_LOCKED       : a icc session is already in progress
  */
int32_t com_icc(int32_t family, int32_t type, int32_t protocol);

/**
  * @brief  ICC process a generic access
  * @note   Process a ICC request for generic access
  * @param  icc            - icc handle obtained with com_icc() call
  * @param  p_buf_cmd      - pointer to application data buffer containing the command to send\n
  *                        data is a command APDU in format as described in ETSI TS 102 221
  * @param  len_cmd        - length of the buffer command (in bytes)
  * @note   Max length of p_buf_cmd (and so len_cmd) is COM_ICC_xxxx_MAX_CMD_GENERIC_ACCESS_SZ bytes
  *         with xxxx = protocol\n
  *         p_buf_cmd[len_cmd] must be '\0' in order to check cohesion with len_cmd.\n
  *         if len_cmd > COM_ICC_xxxx_MAX_CMD_GENERIC_ACCESS_SZ or p_buf_cmd[len_cmd] != '\0'
  *         then COM_ERR_PARAMETER is returned\n
  *         e.g: to select Master File:\n
  *              p_buf_cmd[15]="00A4000C023F00", len_cmd=14, p_buf_cmd[0]='0x30'...p_buf_cmd[14]='\0'
  * @param  p_buf_rsp      - pointer to application data buffer to contain the response\n
  *                        data is a response APDU in format as described in ETSI TS 102 221
  * @param  len_rsp        - size max of the buffer response (in bytes)
  * @note   Min length of p_buf_rsp (and so len_rsp)is COM_ICC_xxxx_MIN_RSP_GENERIC_ACCESS_SZ bytes
  *         with xxxx = protocol\n
  *         if len_rsp < COM_ICC_xxxx_MIN_RSP_GENERIC_ACCESS_SZ, COM_ERR_PARAMETER is returned\n
  *         p_buf_rsp[MIN(len_rsp, retval)] is always set to '\0'\n
  *         e.g: if response to select Master File is SW1=90 and SW2=00:\n
  *              p_buf_rsp[]="9000", int32_t resuit=4, p_buf_rsp[0]='0x39'...p_buf_rsp[4]='\0'
  * @retval int32_t        - length of the ICC response or error value
  * @note if int32_t < 0 : an error occurred\n
  *       - COM_ERR_DESCRIPTOR   : icc handle parameter NOK
  *       - COM_ERR_PARAMETER    : at least one argument incorrect (e.g: buffer NULL, length min/max not respected ...)
  *       - COM_ERR_STATE        : a command is already in progress and its answer is not yet received
  *       - COM_ERR_NOICC        : no icc available to proceed the command
  *       - COM_ERR_GENERAL      : a low level error happened (e.g: no response)\n
  *       if int32_t > 0 :\n
  *       - if int32_t <= len_rsp, p_buf_rsp contains the complete ICC response\n
  *       - if int32_t > len_rsp, only the first bytes of the ICC response are available in p_buf_rsp\n
  *         means p_buf_rsp was too small to copy the complete ICC response\n
  *         (in this case int32_t contains the true length of the ICC response)
  */
int32_t com_icc_generic_access(int32_t icc,
                               const com_char_t *p_buf_cmd, int32_t len_cmd,
                               com_char_t *p_buf_rsp, int32_t len_rsp);

/**
  * @brief  ICC session close
  * @note   Close a ICC session and release icc handle
  * @param  icc     - icc handle obtained with com_icc() call
  * @retval int32_t - ok or error value
  * @note   Possible returned value:\n
  *         - COM_ERR_OK           : icc handle release OK
  *         - COM_ERR_DESCRIPTOR   : icc handle parameter NOK
  *         - COM_ERR_STATE        : a command is already in progress and its answer is not yet received
  */
int32_t com_closeicc(int32_t icc);

/**
  * @}
  */

/*** Component Initialization/Start *******************************************/
/*** Used by com_core module - Not an User Interface **************************/

/**
  * @brief  Component initialization
  * @note   must be called only one time :
  *         - before using any other functions of com_icc*
  * @param  -
  * @retval bool      - true/false init ok/nok
  */
bool com_icc_init(void);

/**
  * @brief  Component start
  * @note   must be called only one time:
            - after com_icc_init
            - and before using any other functions of com_icc_*
  * @param  -
  * @retval -
  */
void com_icc_start(void);

#endif /* USE_COM_ICC == 1 */

#ifdef __cplusplus
}
#endif

#endif /* COM_ICC_H */
