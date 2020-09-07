/**
  ******************************************************************************
  * @file    com_sockets_err_compat.h
  * @author  MCD Application Team
  * @brief   Header for com_sockets_err_compat.c module
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
#ifndef COM_SOCKETS_ERR_COMPAT_H
#define COM_SOCKETS_ERR_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

#if ((COM_SOCKETS_ERRNO_COMPAT == 1) || (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP))
/* LwIP is a Third Party so MISRAC messages linked to it are ignored */
/*cstat -MISRAC2012-* */
#include "lwip/errno.h"
/*cstat +MISRAC2012-* */
#endif /* (COM_SOCKETS_ERRNO_COMPAT == 1) || (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */

#include "com_err.h"

/* Exported constants --------------------------------------------------------*/
/** @addtogroup COM_SOCKETS_Constants
  * @{
  */

/**
  * @}
  */

/* Exported types ------------------------------------------------------------*/
/** @addtogroup COM_SOCKETS_Types
  * @{
  */

/** @defgroup COM_SOCKETS_Types_ERROR_CODES Error Codes
  * @{
  */
/** @note Next define is to ensure compatibility with previous release */
typedef com_err_t com_sockets_err_t; /*!< Definitions for error constants */
#define COM_SOCKETS_ERR_OK             COM_ERR_OK           /*!< Everything OK              */
#define COM_SOCKETS_ERR_GENERAL        COM_ERR_GENERAL      /*!< General / Low level error  */
#define COM_SOCKETS_ERR_DESCRIPTOR     COM_ERR_DESCRIPTOR   /*!< Invalid socket descriptor  */
#define COM_SOCKETS_ERR_PARAMETER      COM_ERR_PARAMETER    /*!< Invalid parameter          */
#define COM_SOCKETS_ERR_WOULDBLOCK     COM_ERR_WOULDBLOCK   /*!< Operation would block      */
#define COM_SOCKETS_ERR_NOMEMORY       COM_ERR_NOMEMORY     /*!< Out of memory error        */
#define COM_SOCKETS_ERR_CLOSING        COM_ERR_CLOSING      /*!< Connection is closing      */
#define COM_SOCKETS_ERR_LOCKED         COM_ERR_LOCKED       /*!< Address in use             */
#define COM_SOCKETS_ERR_TIMEOUT        COM_ERR_TIMEOUT      /*!< Timeout                    */
#define COM_SOCKETS_ERR_INPROGRESS     COM_ERR_INPROGRESS   /*!< Operation in progress      */
#define COM_SOCKETS_ERR_NONAME         COM_ERR_NONAME       /*!< Host Name not existing     */
/* Added value */
#define COM_SOCKETS_ERR_NONETWORK      COM_ERR_NONETWORK    /*!< No network to proceed      */
#define COM_SOCKETS_ERR_UNSUPPORTED    COM_ERR_UNSUPPORTED  /*!< Unsupported functionnality */
#define COM_SOCKETS_ERR_STATE          COM_ERR_STATE        /*!< Connect rqt but Socket is already connected
                                                                 Send/Recv rqt but Socket is not connected
                                                                 ... */
/**
  * @}
  */

/**
  * @}
  */

/* External variables --------------------------------------------------------*/
/** @addtogroup COM_SOCKETS_Variables
  * @{
  */

/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/
/** @addtogroup COM_SOCKETS_Macros
  * @{
  */

/**
  * @}
  */

/* Exported functions ------------------------------------------------------- */
/** @addtogroup COM_SOCKETS_Functions_Other
  * @{
  */

/**
  * @brief  Err to Errno translation
  * @note   Translate a com_sockets_err_t err to an errno value
  *         To do the translation define COM_SOCKETS_ERRNO_COMPAT must be set to 1
  *         If COM_SOCKETS_ERRNO_COMPAT == 0 no translation is done
  * @param  err      - com error value
  * @retval int32_t  - errno value (see com_sockets_err_to_errno_table for the translation)
  */
int32_t com_sockets_err_to_errno(com_sockets_err_t err);

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* COM_SOCKETS_ERR_COMPAT_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
