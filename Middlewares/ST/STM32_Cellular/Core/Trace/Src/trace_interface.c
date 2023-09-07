/**
  ******************************************************************************
  * @file    trace_interface.c
  * @author  MCD Application Team
  * @brief   This file contains trace define utilities
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

/* Includes ------------------------------------------------------------------*/
#include "trace_interface.h"
#include "rtosal.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>


#if (USE_CMD_CONSOLE == 1)
#include "cmd.h"
#endif  /* (USE_CMD_CONSOLE == 1) */

/* Private typedef -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
#if (TRACE_IF_TRACES_UART == 1U)
#define PRINT_FORCE(format, args...)  TRACE_PRINT_FORCE(DBG_CHAN_UTILITIES, DBL_LVL_P0, format, ## args)
#endif /* TRACE_IF_TRACES_UART == 1U */

/* Private defines -----------------------------------------------------------*/
#if (TRACE_IF_TRACES_UART == 1U)
#define MAX_HEX_PRINT_SIZE     210U
#endif /* TRACE_IF_TRACES_UART == 1U */

/* Private variables ---------------------------------------------------------*/
#if (TRACE_IF_TRACES_UART == 1U)
static bool traceIF_traceEnable = true; /* Trace enable by default */
static uint32_t traceIF_Level = TRACE_IF_MASK;

/* Mutex to avoid trace mixing between components */
static osMutexId traceIF_uart_mutex = NULL;

/* Array to know if trace is activated for a specific channel */
/* All debug channel  enabled per default */
static uint8_t traceIF_traceComponent[DBG_CHAN_MAX_VALUE] =
{
  1U,   /*  DBG_CHAN_MAIN              */
  1U,   /*  DBG_CHAN_ATCMD             */
  1U,   /*  DBG_CHAN_CELLULAR_SERVICE  */
  1U,   /*  DBG_CHAN_COMLIB            */
  1U,   /*  DBG_CHAN_IPC               */
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  1U,   /*  DBG_CHAN_PPPOSIF           */
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_LWIP */
  1U,   /*  DBG_CHAN_UTILITIES         */
  1U,   /*  DBG_CHAN_ERROR_HANDLER     */
#if (USE_DBG_CHAN_APPLICATION == 1U)
  1U,   /*  DBG_CHAN_APPLICATION       */
#endif /* USE_DBG_CHAN_APPLICATION == 1U */
  1U    /*  DBG_CHAN_VALID             */
};

#if (USE_CMD_CONSOLE == 1)
#if (SW_DEBUG_VERSION == 1)
static uint8_t *trace_cmd_label = (uint8_t *)"trace";
#endif /* SW_DEBUG_VERSION == 1 */
#endif /* USE_CMD_CONSOLE == 1 */

#endif /* TRACE_IF_TRACES_UART == 1U */

/* Private function prototypes -----------------------------------------------*/

#if (TRACE_IF_TRACES_UART == 1U)
#if (USE_CMD_CONSOLE == 1)
#if (SW_DEBUG_VERSION == 1)
static void traceIF_cmd(uint8_t *cmd_line_p);
static void traceIF_cmd_Help(void);
#endif /* SW_DEBUG_VERSION == 1 */
#endif /* USE_CMD_CONSOLE == 1 */
#endif /* TRACE_IF_TRACES_UART == 1U */

/* Global variables ----------------------------------------------------------*/
#if (TRACE_IF_TRACES_UART == 1U)
uint8_t dbgIF_buf[DBG_CHAN_MAX_VALUE][DBG_IF_MAX_BUFFER_SIZE];
#endif /* TRACE_IF_TRACES_UART == 1U */

/* Functions Definition ------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
#if (TRACE_IF_TRACES_UART == 1U)
#if (USE_CMD_CONSOLE == 1)
#if (SW_DEBUG_VERSION == 1)
/**
  * @brief  Enable / Disable a trace component
  * @param  component - component name string
  * @param  enable - 0: disable 1: enable
  * @retval -
  */
static void CMD_ComponentEnableDisable(uint8_t *component, uint8_t enable)
{
  /* Component name string */
  static uint8_t *traceIF_traceComponentName[DBG_CHAN_MAX_VALUE] =
  {
    (uint8_t *)"main",
    (uint8_t *)"atcmd",
    (uint8_t *)"cellular_service",
    (uint8_t *)"comlib",
    (uint8_t *)"ipc",
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
    (uint8_t *)"ppposif",
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_LWIP */
    (uint8_t *)"utilities",
    (uint8_t *)"error",
#if (USE_DBG_CHAN_APPLICATION == 1U)
    (uint8_t *)"app",
#endif /* USE_DBG_CHAN_APPLICATION == 1U */
    (uint8_t *)"valid"
  };

  uint8_t i;

  /* Is request for all components ? */
  if (strncmp((CRC_CHAR_t *)component, "all", strlen((CRC_CHAR_t *)component)) == 0)
  {
    /* Update trace enable/disable value for all components */
    for (i = 0U; i < (uint8_t)DBG_CHAN_MAX_VALUE ; i++)
    {
      traceIF_traceComponent[i] = enable;
    }
  }
  else
  {
    /* Check if value in component parameter is managed */
    for (i = 0U; i < (uint8_t)DBG_CHAN_MAX_VALUE ; i++)
    {
      if (strncmp((CRC_CHAR_t *)component, (CRC_CHAR_t *)traceIF_traceComponentName[i], strlen((CRC_CHAR_t *)component))
          == 0)
      {
        /* Component is found, stop the research */
        break;
      }
    }

    /* Component found ? */
    if (i >= (uint8_t)DBG_CHAN_MAX_VALUE)
    {
      /* Component not found, display an error */
      PRINT_FORCE("invalid canal name %s\r\n", component);
    }
    else
    {
      /* Component found, update its trace enable/disable value */
      traceIF_traceComponent[i] = enable;
    }
  }
}

/**
  * @brief  help cmd management
  * @param  -
  * @retval -
  */
static void traceIF_cmd_Help(void)
{
  /* Display information about trace interface cmd and its supported parameters */
  CMD_print_help(trace_cmd_label);

  PRINT_FORCE("%s help\r\n", trace_cmd_label);
  PRINT_FORCE("%s on (activate traces)\r\n", trace_cmd_label);
  PRINT_FORCE("%s off (deactivate traces)\r\n", trace_cmd_label);
#if (USE_DBG_CHAN_APPLICATION == 1U)
  PRINT_FORCE("%s enable  all|main|atcmd|cellular_service|comlib|ipc|ppposif|utilities|error|app\r\n",
              trace_cmd_label)
#else /* USE_DBG_CHAN_APPLICATION == 0U or undefined */
  PRINT_FORCE("%s enable  all|main|atcmd|cellular_service|comlib|ipc|ppposif|utilities|error\r\n",
              trace_cmd_label)
#endif /* USE_DBG_CHAN_APPLICATION == 1U */
  PRINT_FORCE(" -> enable traces of selected component\r\n")
#if (USE_DBG_CHAN_APPLICATION == 1U)
  PRINT_FORCE("%s disable all|main|atcmd|cellular_service|comlib|ipc|ppposif|utilities|error|app\r\n",
              trace_cmd_label)
#else /* USE_DBG_CHAN_APPLICATION == 0U or undefined */
  PRINT_FORCE("%s disable all|main|atcmd|cellular_service|comlib|ipc|ppposif|utilities|error\r\n",
              trace_cmd_label)
#endif /* USE_DBG_CHAN_APPLICATION == 1U */
  PRINT_FORCE(" -> disable traces of selected component\r\n")
}

/**
  * @brief  console cmd management
  * @param  cmd_line_p - command parameters
  * @retval -
  */
static void traceIF_cmd(uint8_t *cmd_line_p)
{
  uint8_t    *argv_p[10];
  uint32_t    argc;
  const uint8_t    *cmd_p;
  uint32_t    level;
  uint32_t    ret ;

  cmd_p = (uint8_t *)strtok((CRC_CHAR_t *)cmd_line_p, " \t");

  if (cmd_p != NULL)
  {
    if (strncmp((const CRC_CHAR_t *)cmd_p, (const CRC_CHAR_t *)trace_cmd_label, strlen((const CRC_CHAR_t *)cmd_p)) == 0)
    {
      /* Parameters parsing */
      for (argc = 0U ; argc < 10U ; argc++)
      {
        argv_p[argc] = (uint8_t *)strtok(NULL, " \t");
        if (argv_p[argc] == NULL)
        {
          break;
        }
      }

      /* If no parameter or 'help' - display help */
      if ((argc == 0U) || (strncmp((CRC_CHAR_t *)argv_p[0], "help", strlen((CRC_CHAR_t *)argv_p[0])) == 0))
      {
        traceIF_cmd_Help();
      }
      /* 'on' : enable traces */
      else if (strncmp((CRC_CHAR_t *)argv_p[0], "on", strlen((CRC_CHAR_t *)argv_p[0])) == 0)
      {
        traceIF_traceEnable = true;
        PRINT_FORCE("\n\r <<< TRACE ACTIVE >>>\n\r")
      }
      /* 'enable' : set to 1U - means enable - traces for all components */
      else if (strncmp((CRC_CHAR_t *)argv_p[0], "enable", strlen((CRC_CHAR_t *)argv_p[0])) == 0)
      {
        PRINT_FORCE("\n\r <<< TRACE ENABLE >>>\n\r")
        CMD_ComponentEnableDisable(argv_p[1], 1);
      }
      /* 'disable' : set to 0U - means disable - traces for all components */
      else if (strncmp((CRC_CHAR_t *)argv_p[0], "disable", strlen((CRC_CHAR_t *)argv_p[0])) == 0)
      {
        PRINT_FORCE("\n\r <<< TRACE DISABLE >>>\n\r")
        CMD_ComponentEnableDisable(argv_p[1], 0);
      }
      /* 'level' : change the level e.g P0, P1, ... of the activated traces */
      else if (strncmp((CRC_CHAR_t *)argv_p[0], "level", strlen((CRC_CHAR_t *)argv_p[0])) == 0)
      {
        PRINT_FORCE("\n\r <<< TRACE LEVEL >>>\n\r")
        ret = CMD_GetValue(argv_p[1], (uint32_t *)&level);
        /* Parameter level not recognized ? */
        if (ret != 0U)
        {
          /* Parameter level not recognized - display an error */
          PRINT_FORCE("invalid level %s\r\n", argv_p[1]);
        }
        else
        {
          /* Parameter level recognized - update internal value */
          traceIF_Level = level;
        }
      }
      /* 'off' : disable traces */
      else if (strncmp((CRC_CHAR_t *)argv_p[0], "off", strlen((CRC_CHAR_t *)argv_p[0])) == 0)
      {
        traceIF_traceEnable = false;
        PRINT_FORCE("\n\r <<< TRACE INACTIVE >>>\n\r")
      }
      else
      {
        /* Parameter not recognized - display help */
        PRINT_FORCE("\n\rTRACE usage\n\r")
        traceIF_cmd_Help();
      }
    }
  }
}
#endif /* SW_DEBUG_VERSION == 1 */
#endif /* USE_CMD_CONSOLE == 1 */
#endif /* TRACE_IF_TRACES_UART == 1U */

#if (TRACE_IF_TRACES_UART == 1U)
/**
  * @brief  Print a trace through UART
  * @param  ptr - pointer on the trace string
  * @param  len - length of the trace string
  * @retval -
  */
static void traceIF_uartTransmit(uint8_t *ptr, uint16_t len)
{
  /* Mutex is used to avoid trace mixing between components */
  (void)rtosalMutexAcquire(traceIF_uart_mutex, RTOSAL_WAIT_FOREVER);

  /* Send the trace */
  (void)HAL_UART_Transmit(&TRACE_INTERFACE_UART_HANDLE, (uint8_t *)ptr, len, HAL_MAX_DELAY);

  (void)rtosalMutexRelease(traceIF_uart_mutex);
}
#endif /* TRACE_IF_TRACES_UART == 1U */

/* Functions Definition ------------------------------------------------------*/
#if (TRACE_IF_TRACES_UART == 1U)
/**
  * @brief  Print a trace on UART
  * @param  chan - component channel
  * @param  lvl - trace level
  * @param  pptr - pointer on the trace
  * @param  len - length of the trace
  * @retval -
  */
void traceIF_uartPrint(uint8_t chan, uint8_t lvl, uint8_t *pptr, uint16_t len)
{
  /* Is trace enable ? */
  if (traceIF_traceEnable == true)
  {
    /* Is this level of trace activated ? */
    if ((traceIF_Level & lvl) != 0U)
    {
      /* Is the trace for this component activated ? */
      if (traceIF_traceComponent[chan] != 0U)
      {
        uint8_t *ptr;
        ptr = pptr;

        /* Print bytes of the trace  */
        traceIF_uartTransmit(ptr, len);
      }
    }
  }
}

/**
  * @brief  Print a trace on UART even if global or component trace is disable
  * @param  chan - component channel (unused parameter)
  * @param  pptr - pointer on the trace
  * @param  len - length of the trace
  * @retval -
  */
void traceIF_uartPrintForce(uint8_t chan, uint8_t *pptr, uint16_t len)
{
  UNUSED(chan);

  uint8_t *ptr;
  ptr = pptr;

  /* Print bytes of the trace */
  traceIF_uartTransmit(ptr, len);
}

/**
  * @brief  Print a trace in hexadecimal format
  * @note   Available for UART trace And NOT for printf
  * @param  chan - component channel
  * @param  lvl  - trace level
  * @param  buff - pointer on the trace
  * @param  len  - length of the trace
  * @retval -
  */
void traceIF_hexPrint(dbg_channels_t chan, dbg_levels_t level, uint8_t *buff, uint16_t len)
{
  static uint8_t car[MAX_HEX_PRINT_SIZE];

  uint32_t i;
  uint16_t tmp_len;
  tmp_len = len;

  /* Check maximum length value */
  if (((tmp_len * 2U) + 1U) > MAX_HEX_PRINT_SIZE) /* +1U to ensure '\0' at the end */
  {
    /* Trace too big for the buffer conversion */
    TRACE_PRINT(chan,  level, "TR (%d/%d)\n\r", (MAX_HEX_PRINT_SIZE >> 1) - 2U, tmp_len)
    tmp_len = (MAX_HEX_PRINT_SIZE >> 1) - 2U; /* cut the buffer */
  }

  /* Character conversion */
  for (i = 0U; i < tmp_len; i++)
  {
    uint8_t digit = ((buff[i] >> 4U) & 0xfU);
    if (digit <= 9U)
    {
      car[2U * i] =  digit + 0x30U;
    }
    else
    {
      car[2U * i] =  digit + 0x41U - 10U;
    }

    digit = (0xfU & buff[i]);
    if (digit <= 9U)
    {
      car[(2U * i) + 1U] =  digit + 0x30U;
    }
    else
    {
      car[(2U * i) + 1U] =  digit + 0x41U - 10U;
    }
  }
  car[2U * i] =  0U;

  /* Trace the converted buffer */
  TRACE_PRINT(chan,  level, "%s ", (CRC_CHAR_t *)car)
}

/**
  * @brief  Print a trace in character format - treat special character e.g
  *  '\0' replace by string <NULL CHAR>
  *  '\r' replace by string <CR>
  *  '\n' replace by string <LF>
  * @param  chan  - component channel
  * @param  level - trace level
  * @param  buf   - pointer on the trace
  * @param  size  - size of the trace
  * @retval -
  */
void traceIF_BufCharPrint(dbg_channels_t chan, dbg_levels_t level, const CRC_CHAR_t *buf, uint16_t size)
{
  for (uint16_t cpt = 0U; cpt < size; cpt++)
  {
    if (buf[cpt] == (CRC_CHAR_t)0)
    {
#if defined(USER_FLAG_TRACE_NO_SPECIAL_CHAR)
#else
      TRACE_PRINT(chan, level, "<NULL CHAR>")
#endif /* defined(USER_FLAG_TRACE_NO_SPECIAL_CHAR) */
    }
    else if (buf[cpt] == '\r')
    {
#if defined(USER_FLAG_TRACE_NO_SPECIAL_CHAR)
#else
      TRACE_PRINT(chan, level, "<CR>")
#endif /* defined(USER_FLAG_TRACE_NO_SPECIAL_CHAR) */
    }
    else if (buf[cpt] == '\n')
    {
#if defined(USER_FLAG_TRACE_NO_SPECIAL_CHAR)
#else
      TRACE_PRINT(chan, level, "<LF>")
#endif /* defined(USER_FLAG_TRACE_NO_SPECIAL_CHAR) */
    }
    else if (buf[cpt] == (CRC_CHAR_t)0x1A)
    {
#if defined(USER_FLAG_TRACE_NO_SPECIAL_CHAR)
#else
      TRACE_PRINT(chan, level, "<CTRL-Z>")
#endif /* defined(USER_FLAG_TRACE_NO_SPECIAL_CHAR) */
    }
    else if ((buf[cpt] >= (CRC_CHAR_t)0x20) && (buf[cpt] <= (CRC_CHAR_t)0x7E))
    {
      /* printable CRC_CHAR_t */
      TRACE_PRINT(chan, level, "%c", buf[cpt])
    }
    else
    {
#if defined(USER_FLAG_TRACE_NO_SPECIAL_CHAR)
#else
      /* Special Character - not printable */
      TRACE_PRINT(chan, level, "<SC>")
#endif /* defined(USER_FLAG_TRACE_NO_SPECIAL_CHAR) */
    }
  }

  /* Force to go to next line to prepare next trace */
  TRACE_PRINT(chan, level, "\n\r")
}

/**
  * @brief  Print a trace in hexadecimal format
  * @param  chan  - component channel
  * @param  level - trace level
  * @param  buf   - pointer on the trace
  * @param  size  - size of the trace
  * @retval -
  */
void traceIF_BufHexPrint(dbg_channels_t chan, dbg_levels_t level, const CRC_CHAR_t *buf, uint16_t size)
{
  for (uint16_t cpt = 0U; cpt < size; cpt++)
  {
    /* Print bytes one per one */
    TRACE_PRINT(chan, level, "0x%02x ", (uint8_t) buf[cpt])

    /* Print only 16 bytes on same line */
    if ((cpt != 0U) && (((cpt + 1U) % 16U) == 0U))
    {
      /* Force to go to next line to prepare next trace */
      TRACE_PRINT(chan, level, "\n\r")
    }
  }

  /* Force to go to next line to prepare next trace */
  TRACE_PRINT(chan, level, "\n\r")
}
#endif /* TRACE_IF_TRACES_UART == 1U */

#if (TRACE_IF_TRACES_UART == 1U)
/**
  * @brief  Trace on - Set trace to enable
  * @param  -
  * @retval -
  */
void traceIF_trace_on(void)
{
  traceIF_traceEnable = true;
}

/**
  * @brief  Trace off - Set trace to disable
  * @param  -
  * @retval -
  */
void traceIF_trace_off(void)
{
  traceIF_traceEnable = false;
}
#endif /* TRACE_IF_TRACES_UART == 1U */

/*** Component Initialization/Start *******************************************/
/*** Internal use only - Not an Application Interface *************************/

/**
  * @brief  Component initialization
  * @note   must be called only one time :
  *         - before using any other functions of traceIF_*
  * @param  -
  * @retval -
  */
void traceIF_init(void)
{
#if (TRACE_IF_TRACES_UART == 1U)
  /* Multi call protection */
  if (traceIF_uart_mutex == NULL)
  {
    traceIF_uart_mutex = rtosalMutexNew((const rtosal_char_t *)"TRC_MUT_UART");
  }
#endif /* TRACE_IF_TRACES_UART == 1U */
}

/**
  * @brief  Component start
  * @note   must be called only one time but after traceIF_init and before using any other functions of traceIF_*
  * @param  -
  * @retval -
  */
void traceIF_start(void)
{
#if (TRACE_IF_TRACES_UART == 1U)
#if (USE_CMD_CONSOLE == 1)
#if (SW_DEBUG_VERSION == 1)
  /* Registration to cmd module to support cmd 'trace' */
  CMD_Declare((uint8_t *)"trace", traceIF_cmd, (uint8_t *)"trace management");
#endif /* SW_DEBUG_VERSION == 1 */
#endif /* USE_CMD_CONSOLE == 1 */
#endif /* TRACE_IF_TRACES_UART == 1U */
}
