/**
  ******************************************************************************
  * @file    error_handler.c
  * @author  MCD Application Team
  * @brief   This file contains error utilities
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
#include "error_handler.h"
#include "plf_config.h"

/* Private macros ------------------------------------------------------------*/
#if (USE_TRACE_ERROR_HANDLER == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_INFO(format, args...) TRACE_PRINT(DBG_CHAN_ERROR_HANDLER, DBL_LVL_P0, "" format "\n\r", ## args)
#else
#include <stdio.h>
#define PRINT_INFO(format, args...)  (void)printf("Error Handler:" format "\n\r", ## args);
#endif /* USE_PRINTF */
#else
#define PRINT_INFO(...)   __NOP(); /* Nothing to do */
#endif /* USE_TRACE_ERROR_HANDLER */

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  dbg_channels_t  channel; /* channel where error occurred */
  int32_t         errorId; /* number identifying the error in the channel */
  error_gravity_t gravity; /* error gravity */
  uint32_t        count;   /* count how many errors have been logged since the beginning */
} error_handler_desc_t;

/* Private defines -----------------------------------------------------------*/
#define MAX_ERROR_ENTRIES (32U)     /* log only last MAX_ERROR_ENTRIES errors */
#define MAX_ERROR_COUNTER (0xFFFFU) /* count how many errors have been logged since the beginning */

/* Private variables ---------------------------------------------------------*/
static error_handler_desc_t errors_table[MAX_ERROR_ENTRIES]; /* errors table */
static uint16_t error_counter = 0U; /* total number of errors */
static uint16_t error_index;        /* current error index */

/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/
/**
  * @brief  Initialize error handler module
  * @param  -
  * @retval -
  */
void ERROR_Handler_Init(void)
{
  uint8_t i;

  /* Initialize error array */
  for (i = 0U; i < MAX_ERROR_ENTRIES; i++)
  {
    errors_table[i].channel = DBG_CHAN_ERROR_HANDLER; /* default value = self (ie no error) */
    errors_table[i].errorId = 0;
    errors_table[i].gravity = ERROR_NO;
  }
  error_index = 0U; /* current error index */
}

/**
  * @brief  Log an error
  * @param  chan    - channel/component in error
  * @param  errorId - error value to log
  * @param  gravity - error gravity to log - if equal to ERROR_FATAL then a SystemReset() is done
  * @retval -
  */
void ERROR_Handler(dbg_channels_t chan, int32_t errorId, error_gravity_t gravity)
{
  /* if this is the very first error, init error array */
  if (error_counter == 0U)
  {
    ERROR_Handler_Init();
#if (USE_PRINTF == 0U)
    /* Error Handler may use trace print */
    traceIF_init();
#endif /* (USE_PRINTF == 0U)  */
  }

  /* log the error */
  error_counter = (error_counter + 1U) % MAX_ERROR_COUNTER;
  errors_table[error_index].count = error_counter;
  errors_table[error_index].channel = chan;
  errors_table[error_index].errorId = errorId;
  errors_table[error_index].gravity = gravity;

  PRINT_INFO("LOG ERROR #%d: channel=%d / errorId=%ld / gravity=%d", error_counter, chan, errorId, gravity)

  /* endless loop if error is fatal */
  if (gravity == ERROR_FATAL)
  {
    HAL_Delay(1000U);
    NVIC_SystemReset(); /* Infinite loop done in NVIC_SystemReset() */
  }

  /* increment error index */
  error_index = (error_index + 1U) %  MAX_ERROR_ENTRIES;
}

/**
  * @brief  Dump all errors logged with a trace
  * @param  -
  * @retval -
  */
void ERROR_Dump_All(void)
{
  uint8_t i;
  if (error_counter > 0U)
  {
    /* Dump errors array */
    for (i = 0U; i < MAX_ERROR_ENTRIES; i++)
    {
      if (errors_table[i].gravity != ERROR_NO)
      {
        PRINT_INFO("DUMP ERROR[%d] (#%ld): channel=%d / errorId=%ld / gravity=%d",
                   i, errors_table[i].count, errors_table[i].channel, errors_table[i].errorId, errors_table[i].gravity)
      }
    }
  }
}

/**
  * @brief  Dump the last error logged
  * @param  -
  * @retval -
  */
void ERROR_Dump_Last(void)
{
#if (USE_TRACE_ERROR_HANDLER == 1U)
  if (error_counter != 0U)
  {
    /* get last error index */
    uint16_t previous_index; /* Last error index */

    if (error_index == 0U)
    {
      previous_index = MAX_ERROR_ENTRIES;
    }
    else
    {
      previous_index = error_index - 1U;
    }

    PRINT_INFO("DUMP LAST ERROR[%d] (#%ld): channel=%d / errorId=%ld / gravity=%d",
               previous_index, errors_table[previous_index].count, errors_table[previous_index].channel,
               errors_table[previous_index].errorId, errors_table[previous_index].gravity)
  }
#endif  /* (USE_TRACE_ERROR_HANDLER == 1U) */
}
