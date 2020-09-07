/**
  ******************************************************************************
  * @file    com_err.h
  * @author  MCD Application Team
  * @brief   COM module Error definition
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
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
#ifndef COM_ERR_H
#define COM_ERR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

/* Exported constants --------------------------------------------------------*/
/** @addtogroup COM_Constants
  * @{
  */

/**
  * @}
  */

/* Exported types ------------------------------------------------------------*/
/** @addtogroup COM_Types
  * @{
  */

/** @defgroup COM_Types_ERROR_CODES Error Codes
  * @{
  */
typedef int32_t com_err_t; /*!< Definitions for error constants */
#define COM_ERR_OK          (com_err_t)0    /*!< Everything OK              */
#define COM_ERR_GENERAL     (com_err_t)-1   /*!< General / Low level error  */
#define COM_ERR_DESCRIPTOR  (com_err_t)-2   /*!< Invalid handle descriptor  */
#define COM_ERR_PARAMETER   (com_err_t)-3   /*!< Invalid parameter          */
#define COM_ERR_WOULDBLOCK  (com_err_t)-4   /*!< Operation would block      */
#define COM_ERR_NOMEMORY    (com_err_t)-5   /*!< Out of memory error        */
#define COM_ERR_CLOSING     (com_err_t)-6   /*!< Connection is closing      */
#define COM_ERR_LOCKED      (com_err_t)-7   /*!< Address in use             */
#define COM_ERR_TIMEOUT     (com_err_t)-8   /*!< Timeout                    */
#define COM_ERR_INPROGRESS  (com_err_t)-9   /*!< Operation in progress      */
#define COM_ERR_NONAME      (com_err_t)-10  /*!< Host Name not existing     */
/* Added value */
#define COM_ERR_NONETWORK   (com_err_t)-11  /*!< No network to proceed      */
#define COM_ERR_UNSUPPORTED (com_err_t)-12  /*!< Unsupported functionnality */
#define COM_ERR_STATE       (com_err_t)-13  /*!< Connect rqt but Socket is already connected
                                                 Send/Recv rqt but Socket is not connected
                                                 ... */
#define COM_ERR_NOICC       (com_err_t)-14  /*!< No icc to proceed          */

/**
  * @}
  */

/**
  * @}
  */

/* External variables --------------------------------------------------------*/
/** @addtogroup COM_Variables
  * @{
  */

/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/
/** @addtogroup COM_Macros
  * @{
  */

/**
  * @}
  */

/* Exported functions ------------------------------------------------------- */

/** @addtogroup COM_Functions
  * @{
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* COM_ERR_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
