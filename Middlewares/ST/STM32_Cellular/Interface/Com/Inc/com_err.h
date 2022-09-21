/**
  ******************************************************************************
  * @file    com_err.h
  * @author  MCD Application Team
  * @brief   COM module Error definition
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
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
#include <stdint.h>

/** @addtogroup COM_Common
  * @{
  */

/* Exported constants --------------------------------------------------------*/
/* None */

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
#define COM_ERR_UNSUPPORTED (com_err_t)-12  /*!< Unsupported functionality  */
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
/* None */

/* Exported macros -----------------------------------------------------------*/
/* None */

/* Exported functions ------------------------------------------------------- */
/* None */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* COM_ERR_H */
