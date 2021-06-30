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
#include <stdlib.h>

#include "plf_config.h"

#include "com_core.h"

#include "com_sockets.h"
#include "com_trace.h"

#if (USE_COM_ICC == 1)
#include "com_icc.h"
#endif /* USE_COM_ICC == 1 */

#include "rtosal.h"

#if (USE_CMD_CONSOLE == 1)
#include <string.h>
#include "cmd.h"
#include "com_sockets_statistic.h"
#endif /* USE_CMD_CONSOLE == 1 */

/* Private defines -----------------------------------------------------------*/
#define COM_CORE_CMD_ARG_MAX_NB                5U

/* Private typedef -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
#if (USE_CMD_CONSOLE == 1)
static void com_core_cmd_help(void);
static cmd_status_t com_core_cmd(uint8_t *p_cmd_line);
#endif /* USE_CMD_CONSOLE == 1 */

/* Private function Definition -----------------------------------------------*/
#if (USE_CMD_CONSOLE == 1)

/**
  * @brief  console cmd help
  * @param  -
  * @note   -
  * @retval -
  */
static void com_core_cmd_help(void)
{
  CMD_print_help((uint8_t *)"comlib");
  PRINT_FORCE("comlib help")
  PRINT_FORCE("comlib stat : display com statitistics\n\r")
}

/**
  * @brief  cmd management
  * @param  p_cmd_line - pointer on command parameters
  * @retval cmd_status_t - status of cmd management CMD_OK or CMD_SYNTAX_ERROR
  */
static cmd_status_t com_core_cmd(uint8_t *p_cmd_line)
{
  cmd_status_t result = CMD_SYNTAX_ERROR;
  uint32_t argc;
  uint32_t len;
  uint8_t  *p_argv[COM_CORE_CMD_ARG_MAX_NB];
  const uint8_t *p_cmd;

  PRINT_FORCE("")
  p_cmd = (uint8_t *)strtok((CRC_CHAR_t *)p_cmd_line, " \t");

  if (p_cmd != NULL)
  {
    if (memcmp((const CRC_CHAR_t *)p_cmd, "comlib", crs_strlen(p_cmd)) == 0)
    {
      /* parameters parsing */
      for (argc = 0U; argc < COM_CORE_CMD_ARG_MAX_NB; argc++)
      {
        p_argv[argc] = (uint8_t *)strtok(NULL, " \t");
        if (p_argv[argc] == NULL)
        {
          break;
        }
      }
      /* cmd 'comlib' [help|stat] ? */
      if (argc == 0U) /* No parameters */
      {
        /* cmd 'comlib' not authorized */
        /* Display help */
        com_core_cmd_help();
      }

      else /* At least one parameter provided */
      {
        /* cmd 'comlib' [help|stat] ? */
        len = crs_strlen(p_argv[0]);
        if (memcmp((CRC_CHAR_t *)p_argv[0], "help", len) == 0)
        {
          /* cmd 'comlib help': display help */
          result = CMD_OK;
          com_core_cmd_help();
        }
        else if (memcmp((CRC_CHAR_t *)p_argv[0], "stat", len) == 0)
        {
          /* cmd 'comlib stat': display statistics */
          result = CMD_OK;
          com_sockets_statistic_display();
        }
        else /* cmd 'comlib xxx ...': unknown */
        {
          PRINT_FORCE("comlib: Unrecognized command. Usage:")
          /* Display help */
          com_core_cmd_help();
        }
      }
    }
  }
  return (result);
}
#endif /* USE_CMD_CONSOLE == 1 */


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
  if ((init_socket == false) || (init_icc == false))
  {
    result = false;
  }

  /* RandomNumberGenerator - needed by com_icc and com_sockets as fallback solution */
  srand(rtosalGetSysTimerCount());

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

#if (USE_CMD_CONSOLE == 1)
  CMD_Declare((uint8_t *)"comlib", com_core_cmd, (uint8_t *)"com library commands");
#endif /* USE_CMD_CONSOLE == 1 */

  return (result);
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
