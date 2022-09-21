/**
  ******************************************************************************
  * @file    trace_interface.h
  * @author  MCD Application Team
  * @brief   Header for trace_interface.c module
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef TRACE_INTERFACE_H
#define TRACE_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"
#include "cellular_runtime_standard.h"
#include "cellular_runtime_custom.h"

/* define debug levels (bitmap) */
typedef uint8_t dbg_levels_t;

#define DBL_LVL_P0     (dbg_levels_t)0x01U
#define DBL_LVL_P1     (dbg_levels_t)0x02U
#define DBL_LVL_P2     (dbg_levels_t)0x04U
#define DBL_LVL_WARN   (dbg_levels_t)0x08U
#define DBL_LVL_ERR    (dbg_levels_t)0x10U
#define DBL_LVL_VALID  (dbg_levels_t)0x20U

/* DEBUG MASK defines the allowed traces : to be defined in plf_sw_config.h */
/* Full traces */
/* #define TRACE_IF_MASK    (uint16_t)(DBL_LVL_P0 | DBL_LVL_P1 | DBL_LVL_P2 | DBL_LVL_WARN | DBL_LVL_ERR) */
/* Warn and Error traces only */
/* #define TRACE_IF_MASK    (uint16_t)(DBL_LVL_WARN | DBL_LVL_ERR) */


/* Maximum buffer size (per channel) */
#define DBG_IF_MAX_BUFFER_SIZE  (uint16_t)(256)

/* Exported types ------------------------------------------------------------*/

/* Define here the list of channels */
typedef enum
{
  DBG_CHAN_MAIN = 0,
  DBG_CHAN_ATCMD,
  DBG_CHAN_CELLULAR_SERVICE,
  DBG_CHAN_COMLIB,
  DBG_CHAN_IPC,
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  DBG_CHAN_PPPOSIF,
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_LWIP */
  DBG_CHAN_UTILITIES,
  DBG_CHAN_ERROR_HANDLER,
#if (USE_DBG_CHAN_APPLICATION == 1U)
  DBG_CHAN_APPLICATION,
#endif /* USE_DBG_CHAN_APPLICATION == 1U */
  DBG_CHAN_VALID,
  DBG_CHAN_MAX_VALUE        /* keep last */
} dbg_channels_t;


/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
extern uint8_t dbgIF_buf[DBG_CHAN_MAX_VALUE][DBG_IF_MAX_BUFFER_SIZE];

/* Exported functions ------------------------------------------------------- */
/**
  * @brief  Trace off - Set trace to disable
  * @param  -
  * @retval -
  */
void traceIF_trace_off(void);

/**
  * @brief  Trace on - Set trace to enable
  * @param  -
  * @retval -
  */
void traceIF_trace_on(void);

/**
  * @brief  Print a trace on UART
  * @param  chan - component channel
  * @param  lvl - trace level
  * @param  pptr - pointer on the trace
  * @param  len - length of the trace
  * @retval -
  */
void traceIF_uartPrint(uint8_t chan, uint8_t lvl, uint8_t *pptr, uint16_t len);

/**
  * @brief  Print a trace on UART even if global or component trace is disable
  * @param  chan - component channel (unused parameter)
  * @param  pptr - pointer on the trace
  * @param  len - length of the trace
  * @retval -
  */
void traceIF_uartPrintForce(uint8_t chan, uint8_t *pptr, uint16_t len);

/**
  * @brief  Print a trace in hexadecimal format
  * @note   Available for ITM or UART trace And NOT for printf
  * @param  chan - component channel
  * @param  lvl  - trace level
  * @param  buff - pointer on the trace
  * @param  len  - length of the trace
  * @retval -
  */
void traceIF_hexPrint(dbg_channels_t chan, dbg_levels_t level, uint8_t *buff, uint16_t len);

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
void traceIF_BufCharPrint(dbg_channels_t chan, dbg_levels_t level, const CRC_CHAR_t *buf, uint16_t size);

/**
  * @brief  Print a trace in hexadecimal format
  * @param  chan  - component channel
  * @param  level - trace level
  * @param  buf   - pointer on the trace
  * @param  size  - size of the trace
  * @retval -
  */
void traceIF_BufHexPrint(dbg_channels_t chan, dbg_levels_t level, const CRC_CHAR_t *buf, uint16_t size);

#if (TRACE_IF_TRACES_UART == 1U)
#define TRACE_PRINT(chan, lvl, format, args...) \
  (void)sprintf((CRC_CHAR_t *)dbgIF_buf[(chan)], format "", ## args);\
  traceIF_uartPrint( (uint8_t)(chan), (uint8_t)lvl, (uint8_t *)dbgIF_buf[(chan)],\
                     (uint16_t)crs_strlen(dbgIF_buf[(chan)]));
#else
#define TRACE_PRINT(...)      __NOP(); /* Nothing to do */
#endif /* TRACE_IF_TRACES_UART == 1U */

/* To force traces even if they are deactivated (used in Boot Menu for example) */
#define TRACE_PRINT_FORCE(chan, lvl, format, args...) \
  (void)sprintf((CRC_CHAR_t *)dbgIF_buf[(chan)], format "", ## args);\
  traceIF_uartPrintForce((uint8_t)(chan), (uint8_t *)dbgIF_buf[(chan)],\
                         (uint16_t)crs_strlen(dbgIF_buf[(chan)]));

#define TRACE_VALID(format, args...) \
  (void)sprintf((CRC_CHAR_t *)dbgIF_buf[(DBG_CHAN_VALID)], format "", ## args);\
  traceIF_uartPrintForce((uint8_t)(DBG_CHAN_VALID), (uint8_t *)dbgIF_buf[(DBG_CHAN_VALID)],\
                         (uint16_t)crs_strlen(dbgIF_buf[(DBG_CHAN_VALID)]));

#define TRACE_PRINT_BUF_CHAR(chan, lvl, pbuf, size) traceIF_BufCharPrint((chan), (lvl), (pbuf), (size))
#define TRACE_PRINT_BUF_HEX(chan, lvl, pbuf, size)  traceIF_BufHexPrint((chan), (lvl), (pbuf), (size))


/*** Component Initialization/Start *******************************************/
/*** Internal use only - Not an Application Interface *************************/

/**
  * @brief  Component initialization
  * @note   must be called only one time :
  *         - before using any other functions of traceIF_*
  * @param  -
  * @retval -
  */
void traceIF_init(void);

/**
  * @brief  Component start
  * @note   must be called only one time but after traceIF_init and before using any other functions of traceIF_*
  * @param  -
  * @retval -
  */
void traceIF_start(void);

#ifdef __cplusplus
}
#endif

#endif /* TRACE_INTERFACE_H */
