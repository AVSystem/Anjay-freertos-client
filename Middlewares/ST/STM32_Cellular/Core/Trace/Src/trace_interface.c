/**
  ******************************************************************************
  * @file    trace_interface.c
  * @author  MCD Application Team
  * @brief   This file contains trace define utilities
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
#include "trace_interface.h"
#include "cmsis_os_misrac2012.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>


#if (USE_CMD_CONSOLE == 1)
#include "cmd.h"
#endif  /* (USE_CMD_CONSOLE == 1) */


/* Private typedef -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
#define PRINT_FORCE(format, args...)  TRACE_PRINT_FORCE(DBG_CHAN_UTILITIES, DBL_LVL_P0, format, ## args)

/* Private defines -----------------------------------------------------------*/
#define MAX_HEX_PRINT_SIZE     210U

/* Private variables ---------------------------------------------------------*/
static bool traceIF_traceEnable = true; /* Trace enable per default */
static uint32_t traceIF_Level = TRACE_IF_MASK;
#if (RTOS_USED == 1)
/* Mutex to avoid trace mixing between components */
static osMutexId traceIF_uart_mutex = NULL;
#endif /* (RTOS_USED == 1) */

/* Array to know if trace is activated for a specific channel */
/* All debug channel  enabled per default */
static uint8_t traceIF_traceComponent[DBG_CHAN_MAX_VALUE] =
{
  1U,   /*  DBG_CHAN_GENERIC           */
  1U,   /*  DBG_CHAN_MAIN              */
  1U,   /*  DBG_CHAN_ATCMD             */
  1U,   /*  DBG_CHAN_COM               */
#if (USE_CUSTOM_CLIENT == 1)
  1U,   /*  DBG_CHAN_CUSTOM_CLIENT     */
#endif /* USE_CUSTOM_CLIENT == 1 */
#if (USE_ECHO_CLIENT == 1)
  1U,   /*  DBG_CHAN_ECHOCLIENT        */
#endif /* USE_ECHO_CLIENT == 1 */
#if (USE_HTTP_CLIENT == 1)
  1U,   /*  DBG_CHAN_HTTP              */
#endif /* USE_HTTP_CLIENT == 1 */
#if (USE_PING_CLIENT == 1)
  1U,   /*  DBG_CHAN_PING              */
#endif /* USE_PING_CLIENT == 1 */
#if (USE_COM_CLIENT == 1)
  1U,   /*  DBG_CHAN_COMCLIENT         */
#endif /* USE_COM_CLIENT == 1 */
#if (USE_MQTT_CLIENT == 1)
  1U,   /*  DBG_CHAN_MQTTCLIENT        */
#endif /* USE_MQTT_CLIENT == 1 */
  1U,   /*  DBG_CHAN_IPC               */
  1U,   /*  DBG_CHAN_PPPOSIF           */
  1U,   /*  DBG_CHAN_CELLULAR_SERVICE  */
  1U,   /*  DBG_CHAN_NIFMAN            */
  1U,   /*  DBG_CHAN_DATA_CACHE        */
  1U,   /*  DBG_CHAN_UTILITIES         */
  1U,   /*  DBG_CHAN_ERROR_LOGGER      */
  1U,   /*  DBG_CHAN_VALID             */
  1U    /*  DBG_CHAN_TEST              */
};

#if (USE_CMD_CONSOLE == 1)
#if (SW_DEBUG_VERSION == 1)
static uint8_t *trace_cmd_label = (uint8_t *)"trace";
#endif /* SW_DEBUG_VERSION == 1 */
#endif  /* (USE_CMD_CONSOLE == 1) */

/* Private function prototypes -----------------------------------------------*/
static void ITM_Out(uint32_t port, uint32_t ch);

#if (USE_CMD_CONSOLE == 1)
#if (SW_DEBUG_VERSION == 1)
static cmd_status_t traceIF_cmd(uint8_t *cmd_line_p);
static void traceIF_cmd_Help(void);
#endif /* SW_DEBUG_VERSION == 1 */
#endif  /* (USE_CMD_CONSOLE == 1) */

/* Global variables ----------------------------------------------------------*/
uint8_t dbgIF_buf[DBG_CHAN_MAX_VALUE][DBG_IF_MAX_BUFFER_SIZE];
uint8_t *traceIF_UartBusyFlag = NULL;

/* Functions Definition ------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

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
    (uint8_t *)"generic",
    (uint8_t *)"main",
    (uint8_t *)"atcmd",
    (uint8_t *)"comlib",
#if (USE_CUSTOM_CLIENT == 1)
    (uint8_t *)"customclt",
#endif /* USE_CUSTOM_CLIENT == 1 */
#if (USE_ECHO_CLIENT == 1)
    (uint8_t *)"echoclt",
#endif /* USE_ECHO_CLIENT == 1 */
#if (USE_HTTP_CLIENT == 1)
    (uint8_t *)"http",
#endif /* USE_HTTP_CLIENT == 1 */
#if (USE_PING_CLIENT == 1)
    (uint8_t *)"ping",
#endif /* USE_PING_CLIENT == 1 */
#if (USE_COM_CLIENT == 1)
    (uint8_t *)"comclt",
#endif /* USE_COM_CLIENT == 1 */
#if (USE_MQTT_CLIENT == 1)
    (uint8_t *)"mqtt",
#endif /* USE_MQTT_CLIENT == 1 */
    (uint8_t *)"ipc",
    (uint8_t *)"ppposif",
    (uint8_t *)"cellular_service",
    (uint8_t *)"nifman",
    (uint8_t *)"data_cache",
    (uint8_t *)"utilities",
    (uint8_t *)"error",
    (uint8_t *)"valid",
    (uint8_t *)"test"
  };

  uint8_t i;

  /* Is request for all components ? */
  if (strncmp((CRC_CHAR_t *)component,
              "all",
              strlen((CRC_CHAR_t *)component))
      == 0)
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
      if (strncmp((CRC_CHAR_t *)component,
                  (CRC_CHAR_t *)traceIF_traceComponentName[i],
                  strlen((CRC_CHAR_t *)component))
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
  PRINT_FORCE("%s on (active traces)\r\n", trace_cmd_label);
  PRINT_FORCE("%s off (deactive traces)\r\n", trace_cmd_label);
  PRINT_FORCE("%s enable  all|generic|main|atcmd|comlib|echoclt|http|ping|ipc|ppposif|cellular_service|nifman",
              trace_cmd_label)
  PRINT_FORCE("           |data_cache|utilities|error\r\n")
  PRINT_FORCE(" -> enable traces of selected component\r\n")
  PRINT_FORCE("%s disable all|generic|main|atcmd|comlib|echoclt|http|ping|ipc|ppposif|cellular_service|nifman",
              trace_cmd_label)
  PRINT_FORCE("           |data_cache|utilities|error\r\n")
  PRINT_FORCE(" -> disable traces of selected component\r\n")
}

/**
  * @brief  console cmd management
  * @param  cmd_line_p - command parameters
  * @retval cmd_status - status of cmd management CMD_OK or CMD_SYNTAX_ERROR
  */
static cmd_status_t traceIF_cmd(uint8_t *cmd_line_p)
{
  uint8_t    *argv_p[10];
  uint32_t    argc;
  const uint8_t    *cmd_p;
  uint32_t    level;
  uint32_t    ret ;
  cmd_status_t cmd_status ;
  cmd_status = CMD_OK;

  cmd_p = (uint8_t *)strtok((CRC_CHAR_t *)cmd_line_p, " \t");

  if (cmd_p != NULL)
  {
    if (strncmp((const CRC_CHAR_t *)cmd_p,
                (const CRC_CHAR_t *)trace_cmd_label,
                strlen((const CRC_CHAR_t *)cmd_p))
        == 0)
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
      if ((argc == 0U)
          || (strncmp((CRC_CHAR_t *)argv_p[0],
                      "help",
                      strlen((CRC_CHAR_t *)argv_p[0]))
              == 0))
      {
        traceIF_cmd_Help();
      }
      /* 'on' : enable traces */
      else if (strncmp((CRC_CHAR_t *)argv_p[0],
                       "on",
                       strlen((CRC_CHAR_t *)argv_p[0]))
               == 0)
      {
        traceIF_traceEnable = true;
        PRINT_FORCE("\n\r <<< TRACE ACTIVE >>>\n\r")
      }
      /* 'enable' : set to 1U - means enable - traces for all components */
      else if (strncmp((CRC_CHAR_t *)argv_p[0],
                       "enable",
                       strlen((CRC_CHAR_t *)argv_p[0]))
               == 0)
      {
        PRINT_FORCE("\n\r <<< TRACE ENABLE >>>\n\r")
        CMD_ComponentEnableDisable(argv_p[1], 1);
      }
      /* 'disable' : set to 0U - means disable - traces for all components */
      else if (strncmp((CRC_CHAR_t *)argv_p[0],
                       "disable",
                       strlen((CRC_CHAR_t *)argv_p[0]))
               == 0)
      {
        PRINT_FORCE("\n\r <<< TRACE DISABLE >>>\n\r")
        CMD_ComponentEnableDisable(argv_p[1], 0);
      }
      /* 'level' : change the level e.g P0, P1, ... of the activated traces */
      else if (strncmp((CRC_CHAR_t *)argv_p[0],
                       "level",
                       strlen((CRC_CHAR_t *)argv_p[0]))
               == 0)
      {
        PRINT_FORCE("\n\r <<< TRACE LEVEL >>>\n\r")
        ret = CMD_GetValue(argv_p[1], (uint32_t *)&level);
        /* Parameter level not recognized ? */
        if (ret != 0U)
        {
          /* Parameter level not recognized - display an error */
          PRINT_FORCE("invalid level %s\r\n", argv_p[1]);
          cmd_status = CMD_SYNTAX_ERROR;
        }
        else
        {
          /* Parameter level recognized - update internal value */
          traceIF_Level = level;
        }
      }
      /* 'off' : disable traces */
      else if (strncmp((CRC_CHAR_t *)argv_p[0],
                       "off",
                       strlen((CRC_CHAR_t *)argv_p[0]))
               == 0)
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

  return cmd_status;
}
#endif /* SW_DEBUG_VERSION == 1 */
#endif  /* (USE_CMD_CONSOLE == 1) */


/**
  * @brief  Print a trace through ITM
  * @param  port - port value of ITM
  * @param  ch - data to print
  * @retval -
  */
static void ITM_Out(uint32_t port, uint32_t ch)
{
  /* Check port validity (0-31)*/
  if (port <= 31U)
  {
    uint32_t tmp_mask;
    tmp_mask = (ITM->TER & (1UL << port));
    if (((ITM->TCR & ITM_TCR_ITMENA_Msk) != 0UL) &&   /* ITM enabled ? */
        (tmp_mask != 0UL))   /* ITM selected Port enabled ? */
    {
      /* Wait until ITM port is ready */
      while (ITM->PORT[port].u32 == 0UL)
      {
        /* Nothing to do except continue to wait */
        __NOP();
      }

      /* ITM port is ready, send data, one byte at a time */
      ITM->PORT[port].u8 = (uint8_t) ch;
    }
  }
}


/**
  * @brief  Print a trace through UART
  * @param  ptr - pointer on the trace string
  * @param  len - length of the trace string
  * @retval -
  */
static void traceIF_uartTransmit(uint8_t *ptr, uint16_t len)
{
#if (RTOS_USED == 1)
  /* Mutex is used to avoid trace mixing between components */
  (void)osMutexWait(traceIF_uart_mutex, RTOS_WAIT_FOREVER);
#endif /* (RTOS_USED == 1) */

  /* Send the trace */
  (void)HAL_UART_Transmit(&TRACE_INTERFACE_UART_HANDLE, (uint8_t *)ptr, len, HAL_MAX_DELAY);

#if (RTOS_USED == 1)
  (void)osMutexRelease(traceIF_uart_mutex);
#endif /* (RTOS_USED == 1) */

  if (traceIF_UartBusyFlag != NULL)
  {
    while (HAL_UART_Receive_IT(&TRACE_INTERFACE_UART_HANDLE, traceIF_UartBusyFlag, 1U) != HAL_OK)
    {
      /* Nothing to do except continue to wait */
    }
    traceIF_UartBusyFlag = NULL;
  }
}

/* Functions Definition ------------------------------------------------------*/
/**
  * @brief  Trace off - Set trace to disable
  * @param  -
  * @retval -
  */
void traceIF_trace_off(void)
{
  traceIF_traceEnable = false;
}

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
  * @brief  Print a trace on ITM
  * @param  port - component channel
  * @param  lvl - trace level
  * @param  pptr - pointer on the trace
  * @param  len - length of the trace
  * @retval -
  */
void traceIF_itmPrint(uint8_t port, uint8_t lvl, uint8_t *pptr, uint16_t len)
{
  /* Is trace enable ? */
  if (traceIF_traceEnable == true)
  {
    /* Is this level of trace activated ? */
    if ((traceIF_Level & lvl) != 0U)
    {
      /* Is the trace for this component activated ? */
      if (traceIF_traceComponent[port] != 0U)
      {
        uint8_t *ptr;
        ptr = pptr;

        /* Print bytes of the trace one by one */
        for (uint16_t i = 0U; i < len; i++)
        {
          ITM_Out((uint32_t) port, (uint32_t) *ptr);
          ptr++;
        }
      }
    }
  }
}

/**
  * @brief  Print a trace on UART
  * @param  port - component channel
  * @param  lvl - trace level
  * @param  pptr - pointer on the trace
  * @param  len - length of the trace
  * @retval -
  */
void traceIF_uartPrint(uint8_t port, uint8_t lvl, uint8_t *pptr, uint16_t len)
{
  /* Is trace enable ? */
  if (traceIF_traceEnable == true)
  {
    /* Is this level of trace activated ? */
    if ((traceIF_Level & lvl) != 0U)
    {
      /* Is the trace for this component activated ? */
      if (traceIF_traceComponent[port] != 0U)
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
  * @brief  Print a trace on ITM even if global or component trace is disable
  * @param  port - component channel
  * @param  pptr - pointer on the trace
  * @param  len - length of the trace
  * @retval -
  */
void traceIF_itmPrintForce(uint8_t port, uint8_t *pptr, uint16_t len)
{
  uint8_t *ptr;
  ptr = pptr;

  /* Print bytes of the trace one by one */
  for (uint16_t i = 0U; i < len; i++)
  {
    ITM_Out((uint32_t) port, (uint32_t) *ptr);
    ptr++;
  }
}

/**
  * @brief  Print a trace on ITM even if global or component trace is disable
  * @param  port - component channel
  * @note   port is an unused parameter
  * @param  pptr - pointer on the trace
  * @param  len - length of the trace
  * @retval -
  */
void traceIF_uartPrintForce(uint8_t port, uint8_t *pptr, uint16_t len)
{
  UNUSED(port);

  uint8_t *ptr;
  ptr = pptr;

  /* Print bytes of the trace */
  traceIF_uartTransmit(ptr, len);
}

/**
  * @brief  Print a trace in hexadecimal format
  * @note   Available for ITM or UART trace And NOT for printf
  * @param  chan - component channel
  * @param  lvl  - trace level
  * @param  buff - pointer on the trace
  * @param  len  - length of the trace
  * @retval -
  */
void traceIF_hexPrint(dbg_channels_t chan, dbg_levels_t level, uint8_t *buff, uint16_t len)
{
#if ((TRACE_IF_TRACES_ITM == 1U) || (TRACE_IF_TRACES_UART == 1U))
  static  uint8_t car[MAX_HEX_PRINT_SIZE];

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
#endif  /* ((TRACE_IF_TRACES_ITM == 1U) || (TRACE_IF_TRACES_UART == 1U)) */
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
#if (RTOS_USED == 1)
  /* Multi call protection */
  if (traceIF_uart_mutex == NULL)
  {
    osMutexDef(osTraceUartMutex);
    traceIF_uart_mutex = osMutexCreate(osMutex(osTraceUartMutex));
  }
#else
  /* Nothing to do in no RTOS used */
  __NOP();
#endif /* (RTOS_USED == 1) */
}

/**
  * @brief  Component start
  * @note   must be called only one time but
            after traceIF_init
            and before using any other functions of traceIF_*
  * @param  -
  * @retval -
  */
void traceIF_start(void)
{
#if (USE_CMD_CONSOLE == 1)
#if (SW_DEBUG_VERSION == 1)
  /* Registration to cmd module to support cmd 'trace' */
  CMD_Declare((uint8_t *)"trace", traceIF_cmd, (uint8_t *)"trace management");
#endif /* SW_DEBUG_VERSION == 1 */
#endif /* USE_CMD_CONSOLE == 1 */
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
