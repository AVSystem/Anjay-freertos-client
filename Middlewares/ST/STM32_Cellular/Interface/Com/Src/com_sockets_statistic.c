/**
  ******************************************************************************
  * @file    com_sockets_statistic.c
  * @author  MCD Application Team
  * @brief   This file implements Socket statistic
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
#include "com_sockets_statistic.h"

#if (USE_COM_SOCKETS == 1)

#if (COM_SOCKETS_STATISTIC == 1U)

#include <string.h>

#include "rtosal.h"

#include "com_trace.h"

#include "dc_common.h"

/* Private defines -----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/
/* Socket statistics counter definition */
typedef struct
{
  uint16_t ok;
  uint16_t nok;
} com_sockets_stat_counter_t;

/* Socket statistics definition */
typedef struct
{
  com_sockets_stat_counter_t create;
  com_sockets_stat_counter_t connect;
  com_sockets_stat_counter_t send;
  com_sockets_stat_counter_t receive;
  com_sockets_stat_counter_t close;
  com_sockets_stat_counter_t network;
} com_socket_statistic_t;

/* Private macros ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

#if 0
#if ((USE_TRACE_COM_SOCKETS == 1U) || (USE_CMD_CONSOLE == 1U))
/* Statistic socket state readable print */
static const uint8_t *com_socket_state_string[] =
{
  "Invalid",
  "Creating",
  "Created",
  "Connected",
  "Sending",
  "Receiving",
  "Closing"
};
#endif /* (USE_TRACE_COM_SOCKETS == 1U) || (USE_CMD_CONSOLE == 1U) */
#endif /* not yet supported */

/* Statistic socket variable */
static com_socket_statistic_t com_socket_statistic;

/* Private typedef -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
/* Callback prototype */
/* Callback called when timer expires */
#if (COM_SOCKETS_STATISTIC_PERIOD != 0U)
static void com_socket_statistic_timer_cb(void *argument);
#endif /* COM_SOCKETS_STATISTIC_PERIOD != 0U */

/* Private function Definition -----------------------------------------------*/

#if (COM_SOCKETS_STATISTIC_PERIOD != 0U)
/**
  * @brief  Called when statistic timer raised
  * @note   Request com socket statistic print
  * @param  argument - parameter passed at creation of osTimer
  * @note   Unused
  * @retval -
  */
static void com_socket_statistic_timer_cb(void *argument)
{
  UNUSED(argument);
  com_sockets_statistic_display();
}
#endif /* COM_SOCKETS_STATISTIC_PERIOD != 0U */

/* Functions Definition ------------------------------------------------------*/

/*** Used by com_sockets module - Not an User Interface ***********************/
/**
  * @brief  Component initialization
  * @note   must be called only one time and
  *         before using any other functions of com_*
  * @param  -
  * @retval -
  */
void com_sockets_statistic_init(void)
{
#if (COM_SOCKETS_STATISTIC_PERIOD != 0U)
  /* Statistic display timer */
  static osTimerId com_socket_statistic_timer_handle;
#endif /* COM_SOCKETS_STATISTIC_PERIOD != 0U */

  /* Initialize socket statistics structure to 0U */
  (void)memset(&com_socket_statistic, 0, sizeof(com_socket_statistic_t));

#if (COM_SOCKETS_STATISTIC_PERIOD != 0U)
  /* Timer creation */
  com_socket_statistic_timer_handle = rtosalTimerNew((const rtosal_char_t *)"COMSOCKSTAT_TIM_STAT",
                                                     (os_ptimer)com_socket_statistic_timer_cb, osTimerPeriodic,
                                                     NULL);
  if (com_socket_statistic_timer_handle != NULL)
  {
    (void)rtosalTimerStart(com_socket_statistic_timer_handle, (uint32_t)(COM_SOCKETS_STATISTIC_PERIOD * 60000U));
  }
  else
  {
    PRINT_FORCE("ComLibStat: Timer creation NOK")
  }

#endif /* COM_SOCKETS_STATISTIC_PERIOD != 0U */
}

/*** Used by com_sockets_* modules - Not an User Interface ********************/
/**
  * @brief  Managed com sockets statistic update
  * @note   -
  * @param  stat - to know what the function has to do
  * @note   statistic update
  * @retval -
  */
void com_sockets_statistic_update(com_sockets_stat_update_t stat)
{
  switch (stat)
  {
    case COM_SOCKET_STAT_NWK_UP:
      com_socket_statistic.network.ok++;
      break;
    case COM_SOCKET_STAT_NWK_DWN:
      com_socket_statistic.network.nok++;
      break;
    case COM_SOCKET_STAT_CRE_OK:
      com_socket_statistic.create.ok++;
      break;
    case COM_SOCKET_STAT_CRE_NOK:
      com_socket_statistic.create.nok++;
      break;
    case COM_SOCKET_STAT_CNT_OK:
      com_socket_statistic.connect.ok++;
      break;
    case COM_SOCKET_STAT_CNT_NOK:
      com_socket_statistic.connect.nok++;
      break;
    case COM_SOCKET_STAT_SND_OK:
      com_socket_statistic.send.ok++;
      break;
    case COM_SOCKET_STAT_SND_NOK:
      com_socket_statistic.send.nok++;
      break;
    case COM_SOCKET_STAT_RCV_OK:
      com_socket_statistic.receive.ok++;
      break;
    case COM_SOCKET_STAT_RCV_NOK:
      com_socket_statistic.receive.nok++;
      break;
    case COM_SOCKET_STAT_CLS_OK:
      com_socket_statistic.close.ok++;
      break;
    case COM_SOCKET_STAT_CLS_NOK:
      com_socket_statistic.close.nok++;
      break;
    default:
      __NOP(); /* Nothing to do */
      break;
  }
}

/**
  * @brief  Display com sockets statistics
  * @note   COM_SOCKETS_STATISTIC and USE_TRACE_COM_SOCKETS must be set to 1
  * @param  -
  * @retval -
  */
void com_sockets_statistic_display(void)
{
#if 0
  socket_desc_t *socket_desc;
  socket_desc = socket_desc_list;
#endif /* not yet supported */

  /* Check that at least one socket has run */
  if (com_socket_statistic.create.ok != 0U)
  {
    PRINT_FORCE("*** Socket Stat Begin ***")

    PRINT_FORCE("ComLibStat: Nwk: up:%5d - dwn:%5d - tot:%6d",
                com_socket_statistic.network.ok, com_socket_statistic.network.nok,
                (com_socket_statistic.network.ok + com_socket_statistic.network.nok))
    PRINT_FORCE("ComLibStat: Cre: ok:%5d - nok:%5d - tot:%6d",
                com_socket_statistic.create.ok, com_socket_statistic.create.nok,
                (com_socket_statistic.create.ok + com_socket_statistic.create.nok))
    PRINT_FORCE("ComLibStat: Con: ok:%5d - nok:%5d - tot:%6d",
                com_socket_statistic.connect.ok, com_socket_statistic.connect.nok,
                (com_socket_statistic.connect.ok + com_socket_statistic.connect.nok))
    PRINT_FORCE("ComLibStat: Snd: ok:%5d - nok:%5d - tot:%6d",
                com_socket_statistic.send.ok, com_socket_statistic.send.nok,
                (com_socket_statistic.send.ok + com_socket_statistic.send.nok))
    PRINT_FORCE("ComLibStat: Rcv: ok:%5d - nok:%5d - tot:%6d",
                com_socket_statistic.receive.ok, com_socket_statistic.receive.nok,
                (com_socket_statistic.receive.ok + com_socket_statistic.receive.nok))
    PRINT_FORCE("ComLibStat: Cls: ok:%5d - nok:%5d - tot:%6d",
                com_socket_statistic.close.ok, com_socket_statistic.close.nok,
                (com_socket_statistic.close.ok + com_socket_statistic.close.nok))
#if 0
    /* Socket status displayed */
    while (socket_desc != NULL)
    {
      if (socket_desc->local == COM_SOCKETS_FALSE)
      {
        PRINT_FORCE("ComLibStat: Sock: id:%3d-State:%s-Err:%d",
                    socket_desc->id, com_socket_state_string[socket_desc->state], socket_desc->error)
      }
      else
      {
        PRINT_FORCE("ComLibStat: Ping: id:%3d-State:%s-Err:%d",
                    socket_desc->id, com_socket_state_string[socket_desc->state], socket_desc->error)
      }
      socket_desc = socket_desc->next;
    }
#endif /* not yet supported */
    PRINT_FORCE("*** Socket Stat End ***")
  }
  else
  {
    PRINT_FORCE("*** Socket Stat Begin ***")
    PRINT_FORCE("ComLibStat: No connection or exchange done !")
    PRINT_FORCE("*** Socket Stat End ***")
  }
}

#else /* COM_SOCKETS_STATISTIC == 0U */
/**
  * @brief  Component initialization
  * @note   must be called only one time and
  *         before using any other functions of com_*
  * @param  -
  * @retval -
  */
void com_sockets_statistic_init(void)
{
  __NOP(); /* Nothing to do */
}

/**
  * @brief  Managed com sockets statistic update and print
  * @note   -
  * @param  stat - to know what the function has to do
  * @note   statistic init, update or print
  * @retval -
  */
void com_sockets_statistic_update(com_sockets_stat_update_t stat)
{
  UNUSED(stat); /* Nothing to do */
  __NOP();
}

/**
  * @brief  Display com sockets statistics
  * @note   COM_SOCKETS_STATISTIC and USE_TRACE_COM_SOCKETS must be set to 1
  * @param  -
  * @retval -
  */
void com_sockets_statistic_display(void)
{
  __NOP(); /* Nothing to do */
}

#endif /* COM_SOCKET_STATISTIC == 1U */

#endif /* USE_COM_SOCKETS == 1 */
