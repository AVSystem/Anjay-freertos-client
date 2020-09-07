/**
  ******************************************************************************
  * @file    com_core.c
  * @author  MCD Application Team
  * @brief   This file implements Communication Core
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

/* Includes ------------------------------------------------------------------*/
#include "com_core.h"
#include "plf_config.h"

#include "com_sockets.h"

#if (USE_COM_ICC == 1)
#include "com_icc.h"
#endif /* USE_COM_ICC == 1 */

/* Private defines -----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private function Definition -----------------------------------------------*/
/* Functions Definition ------------------------------------------------------*/

/*** Component Initialization/Start *******************************************/
/*** Internal use only - Not an Application Interface *************************/

/**
  * @brief  Component initialization
  * @note   must be called only one time :
  *         - before using any other functions of com_*
  * @param  -
  * @retval bool      - true/false init ok/nok
  */
bool com_init(void)
{
  bool init_socket, init_icc, result;

  /* Init socket services */
  init_socket = com_sockets_init();

  /* Init icc services */
#if (USE_COM_ICC == 1)
  init_icc    = com_icc_init();
#else /* USE_COM_ICC == 0 */
  init_icc    = true;
#endif /* USE_COM_ICC == 1 */

  result = true;
  if ((init_socket == false)
      || (init_icc == false))
  {
    result = false;
  }

  return (result);
}


/**
  * @brief  Component start
  * @note   must be called only one time but
            after com_init and dc_start
            and before using any other functions of com_*
  * @param  -
  * @retval bool      - true/false start ok/nok
  */
bool com_start(void)
{
  bool result;

  result = true;

  /* Start socket services */
  com_sockets_start();

#if (USE_COM_ICC == 1)
  /* Start icc services */
  com_icc_start();
#endif /* USE_COM_ICC == 1 */

  return (result);
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
