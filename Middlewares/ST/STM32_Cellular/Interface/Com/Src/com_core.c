/**
  ******************************************************************************
  * @file    com_core.c
  * @author  MCD Application Team
  * @brief   This file implements Communication Core
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

/* Includes ------------------------------------------------------------------*/
#include <stdlib.h>

#include "plf_config.h"

#include "com_core.h"

#include "com_trace.h"

#if (USE_COM_SOCKETS == 1)
#include "com_sockets.h"
#endif /* USE_COM_SOCKETS == 1 */

#if (USE_COM_ICC == 1)
#include "com_icc.h"
#endif /* USE_COM_ICC == 1 */

#include "rtosal.h"

#if (USE_CMD_CONSOLE == 1)
#include <string.h>
#include "cmd.h"
#if (USE_COM_SOCKETS == 1)
#include "com_sockets_statistic.h"
#endif /* USE_COM_SOCKETS == 1 */
#endif /* USE_CMD_CONSOLE == 1 */

#include "cellular_runtime_custom.h"   /* for CRC_CHAR_t definition   */
#include "cellular_runtime_standard.h" /* for crs_strlen() definition */

/* Private defines -----------------------------------------------------------*/
#define COM_CORE_CMD_ARG_MAX_NB                5U

/* Private typedef -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
#if (USE_CMD_CONSOLE == 1)
static void com_core_cmd_help(void);
static void com_core_cmd(uint8_t *p_cmd_line);
#endif /* USE_CMD_CONSOLE == 1 */

/* Private function Definition -----------------------------------------------*/
#if (USE_CMD_CONSOLE == 1)

/**
  * @brief  console cmd help
  * @param  -
  * @retval -
  */
static void com_core_cmd_help(void)
{
  CMD_print_help((uint8_t *)"comlib");
  PRINT_FORCE("comlib help")
#if (USE_COM_SOCKETS == 1)
  PRINT_FORCE("comlib stat : display com statitistics\n\r")
#endif /* USE_COM_SOCKETS == 1 */
}

/**
  * @brief  cmd management
  * @param  p_cmd_line - pointer on command parameters
  * @retval -
  */
static void com_core_cmd(uint8_t *p_cmd_line)
{
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
          com_core_cmd_help();
        }
        else if (memcmp((CRC_CHAR_t *)p_argv[0], "stat", len) == 0)
        {
#if (USE_COM_SOCKETS == 1)
          /* cmd 'comlib stat': display statistics */
          com_sockets_statistic_display();
#else  /* USE_COM_SOCKETS == 0 */
          PRINT_FORCE("comlib: option 'stat' not supported!")
#endif /* USE_COM_SOCKETS == 1 */
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
#if (USE_COM_SOCKETS == 1)
  init_socket = com_sockets_init();
#else  /* USE_COM_SOCKETS == 0 */
  init_socket = true;
#endif /* USE_COM_SOCKETS == 1 */

  /* Init icc services */
#if (USE_COM_ICC == 1)
  init_icc    = com_icc_init();
#else  /* USE_COM_ICC == 0 */
  init_icc    = true;
#endif /* USE_COM_ICC == 1 */

  result = true;
  if ((init_socket == false) || (init_icc == false))
  {
    result = false;
  }

  /* RandomNumberGenerator - needed by com_icc and com_sockets as fallback solution */
#if ((USE_COM_SOCKETS == 1) || (USE_COM_ICC == 1))
  srand(rtosalGetSysTimerCount());
#endif /* (USE_COM_SOCKETS == 1) || (USE_COM_ICC == 1) */

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

#if (USE_COM_SOCKETS == 1)
  /* Start socket services */
  com_sockets_start();
#endif /* USE_COM_SOCKETS == 1 */

#if (USE_COM_ICC == 1)
  /* Start icc services */
  com_icc_start();
#endif /* USE_COM_ICC == 1 */

#if (USE_CMD_CONSOLE == 1)
  CMD_Declare((uint8_t *)"comlib", com_core_cmd, (uint8_t *)"com library commands");
#endif /* USE_CMD_CONSOLE == 1 */

  return (result);
}

