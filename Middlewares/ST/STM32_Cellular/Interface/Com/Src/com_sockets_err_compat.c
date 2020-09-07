/**
  ******************************************************************************
  * @file    com_sockets_err_compat.c
  * @author  MCD Application Team
  * @brief   This file implements Communication Socket Error Compatibility
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

/* Includes ------------------------------------------------------------------*/
#include "com_sockets_err_compat.h"
#include "plf_config.h"

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)

/* Private defines -----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/

/* Table to quickly map an com_sockets_err_t to to a socket error
 * by using -err as an index */
#if (COM_SOCKETS_ERRNO_COMPAT == 1)
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private function Definition -----------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/
/**
  * @brief  Err to Errno translation
  * @note   Translate a com_sockets_err_t err to an errno value
  *         To do the translation define COM_SOCKETS_ERRNO_COMPAT must be set to 1
  *         If COM_SOCKETS_ERRNO_COMPAT == 0 no translation is done
  * @param  err      - com error value
  * @retval int32_t  - errno value (see com_sockets_err_to_errno_table for the translation)
  */
int32_t com_sockets_err_to_errno(com_sockets_err_t err)
{
  static const int32_t com_sockets_err_to_errno_table[] =
  {
    0,             /* COM_ERR_OK            0 Everything OK              */
    -1,            /* COM_ERR_GENERAL      -1 General / Low level error  */
    EIO,           /* COM_ERR_DESCRIPTOR   -2 Invalid socket descriptor  */
    EINVAL,        /* COM_ERR_PARAMETER    -3 Invalid parameter          */
    EWOULDBLOCK,   /* COM_ERR_WOULDBLOCK   -4 Operation would block      */
    ENOMEM,        /* COM_ERR_NOMEMORY     -5 Out of memory error        */
    ENOTCONN,      /* COM_ERR_CLOSING      -6 Connection is closing      */
    EADDRINUSE,    /* COM_ERR_LOCKED       -7 Address in use             */
    EAGAIN,        /* COM_ERR_TIMEOUT      -8 Timeout                    */
    EINPROGRESS,   /* COM_ERR_INPROGRESS   -9 Operation in progress      */
    EHOSTUNREACH,  /* COM_ERR_NONAME      -10 Host Name not existing     */

    /* Added value */
    EHOSTUNREACH,  /* COM_ERR_NONETWORK   -11 No Network to proceed or ENONET ? */
    -1,            /* COM_ERR_UNSUPPORTED -12 Unsupported functionality  */
    -1,            /* COM_ERR_STATE       -13 Connect rqt but Socket is already connected
                                              Send/Recv rqt but Socket is not connected
                                              ... */
    EHOSTUNREACH   /* COM_ERR_NOICC       -14 No ICC to proceed          */
  };

  if ((err > 0)
      || (((uint32_t)(-err)) >= (sizeof(com_sockets_err_to_errno_table)
                                 / sizeof((com_sockets_err_to_errno_table[0])))))
  {
    return ((int32_t)EIO);
  }
  else
  {
    return ((int32_t)com_sockets_err_to_errno_table[(uint32_t)(-err)]);
  }
}
#else /* COM_SOCKETS_ERRNO_COMPAT == 0 */
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private function Definition -----------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/
/**
  * @brief  Err to Errno translation
  * @note   Translate a com_sockets_err_t err to an errno value
  *         To do the translation define COM_SOCKETS_ERRNO_COMPAT must be set to 1
  *         If COM_SOCKETS_ERRNO_COMPAT == 0 no translation is done
  * @param  err      - com error value
  * @retval int32_t  - errno value (see com_sockets_err_to_errno_table for the translation)
  */
int32_t com_sockets_err_to_errno(com_sockets_err_t err)
{
  return ((int32_t)err);
}
#endif /* COM_SOCKETS_ERRNO_COMPAT == 1 */

#else /* USE_SOCKETS_TYPE == USE_SOCKETS_LWIP */
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private function Definition -----------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/
/**
  * @brief  Err to Errno translation
  * @note   Translate a com_sockets_err_t err to an errno value
  *         To do the translation define COM_SOCKETS_ERRNO_COMPAT must be set to 1
  *         If COM_SOCKETS_ERRNO_COMPAT == 0 no translation is done
  * @param  err      - com error value
  * @retval int32_t  - errno value (see com_sockets_err_to_errno_table for the translation)
  */
int32_t com_sockets_err_to_errno(com_sockets_err_t err)
{
  return ((int32_t)err);
}

#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
