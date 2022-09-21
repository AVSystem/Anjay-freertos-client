/**
  ******************************************************************************
  * @file    at_core.c
  * @author  MCD Application Team
  * @brief   This file provides code for AT Core
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
#include <string.h>
#include "ipc_common.h"
#include "at_core.h"
#include "at_parser.h"
#include "error_handler.h"
#include "cellular_runtime_standard.h"
#include "cellular_runtime_custom.h"
#include "plf_config.h"
/* following file added to check SID for DATA suspend/resume cases */
#include "cellular_service_int.h"

/** @addtogroup AT_CORE AT_CORE
  * @{
  */

/** @addtogroup AT_CORE_CORE AT_CORE CORE
  * @{
  */

/** @defgroup AT_CORE_CORE_Private_Macros AT_CORE CORE Private Macros
  * @{
  */
#if (USE_TRACE_ATCORE == 1U)
#if (USE_PRINTF  == 0U)
#include "trace_interface.h"
#define TRACE_INFO(format, args...) TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P0, "ATCore:" format "\n\r", ## args)
#define TRACE_DBG(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P1, "ATCore:" format "\n\r", ## args)
#define TRACE_ERR(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_ERR, "ATCore ERROR:" format "\n\r", ## args)
#else
#define TRACE_INFO(format, args...)  (void) printf("ATCore:" format "\n\r", ## args);
#define TRACE_DBG(...)   __NOP(); /* Nothing to do */
#define TRACE_ERR(format, args...)   (void) printf("ATCore ERROR:" format "\n\r", ## args);
#endif /* USE_PRINTF */
#else
#define TRACE_INFO(...)   __NOP(); /* Nothing to do */
#define TRACE_DBG(...)   __NOP(); /* Nothing to do */
#define TRACE_ERR(...)   __NOP(); /* Nothing to do */
#endif /* USE_TRACE_ATCORE */

#define LOG_ERROR(ErrId, gravity)   ERROR_Handler(DBG_CHAN_ATCMD, (ErrId), (gravity))
/**
  * @}
  */

/** @defgroup AT_CORE_CORE_Private_Defines AT_CORE CORE Private Defines
  * @{
  */
#define USE_PARSING_MUTEX    (1)
#define DBG_DUMP_IPC_RX_QUEUE (0) /* dump the IPC RX queue (advanced debug only) */
#define ATCORE_SEM_WAIT_ANSWER_COUNT     ((uint16_t) 1U)
#define ATCORE_SEM_SEND_COUNT            ((uint16_t) 1U)
#define MSG_IPC_RECEIVED_SIZE (uint32_t) ((uint16_t) 128U)
#define SIG_IPC_MSG                      (1U) /* signals definition for IPC message queue */
#define SIG_INTERNAL_EVENT_MODEM         (2U) /* signals definition for internal event from the cellular modem */
/**
  * @}
  */

/** @defgroup AT_CORE_CORE_Private_Variables AT_CORE CORE Private Variables
  * @{
  */
static uint8_t         AT_Core_initialized = 0U;
static IPC_Handle_t    ipcHandleTab;
static at_context_t    at_context;
static urc_callback_t  register_URC_callback;
static IPC_RxMessage_t msgFromIPC;       /* IPC msg */
static __IO uint8_t    MsgReceived = 0U; /* received IPC msg counter */
static IPC_CheckEndOfMsgCallbackTypeDef custom_checkEndOfMsgCallback = NULL;
/* this semaphore is used for waiting for an answer from Modem */
static osSemaphoreId s_WaitAnswer_SemaphoreId = NULL;
/* this queue is used by IPC to inform that messages are ready to be retrieved */
static osMessageQId q_msg_IPC_received_Id;

/* Mutex used to avoid crossing cases when preparing/parsing AT commands/responses/URC */
#if (USE_PARSING_MUTEX == 1)
static osMutexId ATCore_ParsingMutexHandle;
#endif /* USE_PARSING_MUTEX == 1 */
/**
  * @}
  */

/** @defgroup AT_CORE_CORE_Private_Functions_Prototypes AT_CORE CORE Private Functions Prototypes
  * @{
  */
static void ATCoreTaskBody(void *argument);
static void msgReceivedCallback(IPC_Handle_t *ipcHandle);
static void msgSentCallback(IPC_Handle_t *ipcHandle);
static at_status_t process_AT_transaction(at_msg_t msg_in_id, at_buf_t *p_rsp_buf);
static at_status_t waitOnMsgUntilTimeout(uint32_t Tickstart, uint32_t Timeout);
static at_status_t sendToIPC(uint8_t *cmdBuf, uint16_t cmdSize);
static at_status_t waitFromIPC(uint32_t tickstart, uint32_t cmdTimeout, IPC_RxMessage_t *p_msg);
static at_action_rsp_t process_answer(at_action_send_t action_send, uint32_t at_cmd_timeout);
static at_action_rsp_t analyze_action_result(at_action_rsp_t val);
static void IRQ_DISABLE(void);
static void IRQ_ENABLE(void);
/**
  * @}
  */

/** @defgroup AT_CORE_CORE_Exported_Functions AT_CORE CORE Exported Functions
  * @{
  */

/**
  * @brief  Initialise AT core
  * @note   This function has to be called once.
  * @param  none
  * @retval at_status_t
  */
at_status_t  AT_init(void)
{
  at_status_t retval;

  /* should be called once */
  if (AT_Core_initialized == 1U)
  {
    LOG_ERROR(1, ERROR_WARNING);
    retval = ATSTATUS_ERROR;
  }
  else
  {
    MsgReceived = 0U;
    register_URC_callback = NULL;

    at_context.device_type = DEVTYPE_INVALID;
    at_context.in_data_mode = AT_FALSE;
    at_context.processing_cmd = 0U;
    at_context.dataSent = AT_FALSE;
    at_context.action_flags = ATACTION_RSP_NO_ACTION;
    at_context.p_rsp_buf = NULL;
    at_context.s_SendConfirm_SemaphoreId = NULL;

    (void) memset((void *)&at_context.parser, 0, sizeof(atparser_context_t));

#if (USE_PARSING_MUTEX == 1U)
    ATCore_ParsingMutexHandle = rtosalMutexNew((const rtosal_char_t *)"ATCORE_MUT_PARSING");
    if (ATCore_ParsingMutexHandle == NULL)
    {
      /* Platform is reset */
      ERROR_Handler(DBG_CHAN_ATCMD, 20, ERROR_FATAL);
    }
#endif /* ENABLE_BG96_LOW_POWER_MODE == 1U */

    AT_Core_initialized = 1U;
    retval = ATSTATUS_OK;
  }

  return (retval);
}

/**
  * @brief  Open (ie create) an AT context.
  * @param  p_device_infos Pointer to structure describing channel to open.
  * @param  event_callback Client callback for events
  * @param  urc_callback Client callback for URC
  * @retval at_handle_t Handle of the AT context created.
  */
at_handle_t  AT_open(sysctrl_info_t *p_device_infos, urc_callback_t urc_callback)
{
  at_handle_t affectedHandle;

  /* Initialize parser for required device type */
  if (ATParser_initParsers(p_device_infos->type) != ATSTATUS_OK)
  {
    affectedHandle = AT_HANDLE_INVALID;
  }
  /* allocate handle */
  else
  {
    affectedHandle = AT_HANDLE_MODEM;
  }

  if (affectedHandle != AT_HANDLE_INVALID)
  {
    /* set adapter name for this context */
    at_context.ipc_handle = &ipcHandleTab;
    at_context.device_type = p_device_infos->type;
    at_context.ipc_device  = p_device_infos->ipc_device;
    if (p_device_infos->ipc_interface == IPC_INTERFACE_UART)
    {
      at_context.ipc_mode = IPC_MODE_UART_CHARACTER;

      /* start in COMMAND MODE */
      at_context.in_data_mode = AT_FALSE;

      /* no command actually in progress */
      at_context.processing_cmd = 0U;

      /* init semaphore for data sent indication */
      at_context.dataSent = AT_FALSE;

      at_context.s_SendConfirm_SemaphoreId =
        rtosalSemaphoreNew((const rtosal_char_t *)"ATCORE_SEM_SEND",
                           ATCORE_SEM_SEND_COUNT);
      if (at_context.s_SendConfirm_SemaphoreId != NULL)
      {
        /* init semaphore */
        (void) rtosalSemaphoreAcquire(at_context.s_SendConfirm_SemaphoreId, 5000U);

        /* register client callback for URC */
        register_URC_callback = urc_callback;

        /* init the ATParser */
        ATParser_init(&at_context, &custom_checkEndOfMsgCallback);
      }
      else
      {
        TRACE_ERR("SendSemaphoreId creation error for handle = %d", affectedHandle)
        affectedHandle = AT_HANDLE_INVALID;
      }
    }
    else
    {
      /* Only UART is supported */
      affectedHandle = AT_HANDLE_INVALID;
    }
  }
  return ((at_handle_t) affectedHandle);
}

/**
  * @brief  Open an AT channel
  * @param  athandle Handle of the AT context where channel will be created.
  * @retval at_status_t
  */
at_status_t  AT_open_channel(at_handle_t athandle)
{
  at_status_t retval;

  if (athandle != AT_HANDLE_INVALID)
  {
    at_context.in_data_mode = AT_FALSE;
    at_context.processing_cmd = 0U;
    at_context.dataSent = AT_FALSE;
    at_context.action_flags = ATACTION_RSP_NO_ACTION;

    /* Open the IPC channel */
    if (IPC_open(at_context.ipc_handle,
                 at_context.ipc_device,
                 at_context.ipc_mode,
                 msgReceivedCallback,
                 msgSentCallback,
                 NULL,
                 custom_checkEndOfMsgCallback) == IPC_OK)
    {

      /* Select the IPC opened channel as current channel */
      if (IPC_select(at_context.ipc_handle) == IPC_OK)
      {
        retval = ATSTATUS_OK;
      }
      else
      {
        TRACE_ERR("IPC selection error")
        retval = ATSTATUS_ERROR;
      }
    }
    else
    {
      TRACE_ERR("IPC open error")
      retval = ATSTATUS_ERROR;
    }
  }
  else
  {
    TRACE_ERR("IPC invalid handle")
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Close an AT channel
  * @param  athandle Handle of the AT context.
  * @retval at_status_t
  */
at_status_t  AT_close_channel(at_handle_t athandle)
{
  at_status_t retval;

  if (athandle != AT_HANDLE_INVALID)
  {
    if (IPC_close(at_context.ipc_handle) == IPC_OK)
    {
      retval = ATSTATUS_OK;
    }
    else
    {
      TRACE_ERR("IPC close error")
      retval = ATSTATUS_ERROR;
    }
  }
  else
  {
    TRACE_ERR("IPC invalid handle")
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Reset an AT context.
  * @param  athandle Handle of the AT context.
  * @retval at_status_t
  */
at_status_t AT_reset_context(at_handle_t athandle)
{
  at_status_t retval;

  if (athandle != AT_HANDLE_INVALID)
  {
    at_context.in_data_mode = AT_FALSE;
    at_context.processing_cmd = 0U;
    at_context.dataSent = AT_FALSE;
    at_context.action_flags = ATACTION_RSP_NO_ACTION;

    /* reinit IPC channel and select our channel */
    retval = ATSTATUS_ERROR;
    if (IPC_reset(at_context.ipc_handle) == IPC_OK)
    {
      if (IPC_select(at_context.ipc_handle) == IPC_OK)
      {
        retval = ATSTATUS_OK;
      }
    }
  }
  else
  {
    /* invalid handle */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Request to send an AT command
  * @note   The command will be sent to the current active channel.
  * @param  athandle Handle of the AT context.
  * @param  msg_in_id Message ID.
  * @param  p_cmd_in_buf Pointer to the buffer with the command to send.
  * @param  p_rsp_buf Pointer to the buffer to return the response.
  * @retval at_status_t
  */
at_status_t AT_sendcmd(at_handle_t athandle, at_msg_t msg_in_id, at_buf_t *p_cmd_in_buf, at_buf_t *p_rsp_buf)
{
  /* Sends a service request message to the ATCore.
  *  This is a blocking function.
  *  It returns when the command is fully processed or a timeout expires.
  */
  at_status_t retval = ATSTATUS_OK;

  if (athandle == AT_HANDLE_INVALID)
  {
    retval = ATSTATUS_ERROR;
  }
  /* Check if a command is already ongoing */
  else if (at_context.processing_cmd == 1U)
  {
    TRACE_ERR("!!!!!!!!!!!!!!!!!! WARNING COMMAND IS UNDER PROCESS !!!!!!!!!!!!!!!!!!")
    retval = ATSTATUS_ERROR;
  }
  else
  {
    /* initialize response buffer */
    (void) memset((void *)p_rsp_buf, 0, ATCMD_MAX_BUF_SIZE);

    /* start to process this command */
    at_context.processing_cmd = 1U;

    /* save ptr on response buffer */
    at_context.p_rsp_buf = p_rsp_buf;

    /* Check if current mode is DATA mode */
    if (at_context.in_data_mode == AT_TRUE)
    {
      /* Check if user command is DATA suspend */
      if (msg_in_id == (at_msg_t) SID_CS_DATA_SUSPEND)
      {
        /* restore IPC Command channel to send ESCAPE COMMAND */
        TRACE_DBG("<<< restore IPC COMMAND channel >>>")
        (void) IPC_select(at_context.ipc_handle);
      }
    }
    /* check if trying to suspend DATA while in command mode */
    else if (msg_in_id == (at_msg_t) SID_CS_DATA_SUSPEND)
    {
      TRACE_ERR("DATA not active")
      retval = ATSTATUS_ERROR;
    }
    else
    {
      /* nothing to do */
    }

    if (retval == ATSTATUS_OK)
    {
      /* Process the user request */
      ATParser_process_request(&at_context, msg_in_id, p_cmd_in_buf);

      /* Start an AT command transaction */
      retval = process_AT_transaction(msg_in_id, p_rsp_buf);
      if (retval == ATSTATUS_OK)
      {
        /* get command response buffer */
        (void) ATParser_get_rsp(&at_context, p_rsp_buf);
      }
      else
      {
        TRACE_DBG("AT_sendcmd error: process AT transaction")
        /* retrieve and send error report if exist */
        (void) ATParser_get_error(&at_context, p_rsp_buf);
        ATParser_abort_request(&at_context);
        if (msg_in_id == (at_msg_t) SID_CS_DATA_SUSPEND)
        {
          /* force to return to command mode */
          TRACE_ERR("force to return to COMMAND mode")
          at_context.in_data_mode = AT_FALSE ;
        }
      }
    }

    /* finished to process this command */
    at_context.processing_cmd = 0U;
  }

  return (retval);
}

/**
  * @brief  Notify that an internal event has been received.
  * @param  deviceType Device ID on which the event has been received.
  * @retval none.
  */
void AT_internalEvent(sysctrl_device_type_t deviceType)
{
  /* add internal event for the deviceType (supports only cellular modem actually)
   * NOTE: do not add trace in this function which can be called under interrupt !
   */
  if (deviceType == DEVTYPE_MODEM_CELLULAR)
  {
    (void) rtosalMessageQueuePut(q_msg_IPC_received_Id, (uint32_t) SIG_INTERNAL_EVENT_MODEM, (uint32_t)0U);
  }
}

/**
  * @brief  Start AT task.
  * @param  taskPrio Task priority.
  * @param  stackSize Stack size for this task.
  * @retval at_status_t.
  */
at_status_t atcore_task_start(osPriority taskPrio, uint16_t stackSize)
{
  at_status_t retval;

  /* ATCore task handler */
  static osThreadId atcoreTaskId = NULL;

  /* check if AT_init has been called before */
  if (AT_Core_initialized != 1U)
  {
    TRACE_ERR("error, ATCore is not initialized")
    LOG_ERROR(17, ERROR_WARNING);
    retval = ATSTATUS_ERROR;
  }
  else
  {
    /* semaphores creation */
    s_WaitAnswer_SemaphoreId = rtosalSemaphoreNew((const rtosal_char_t *) "ATCORE_SEM_WAIT_ANSWER",
                                                  ATCORE_SEM_WAIT_ANSWER_COUNT);
    if (s_WaitAnswer_SemaphoreId == NULL)
    {
      TRACE_ERR("s_WaitAnswer_SemaphoreId creation error")
      LOG_ERROR(18, ERROR_WARNING);
      retval = ATSTATUS_ERROR;
    }
    else
    {
      /* init semaphore */
      (void) rtosalSemaphoreAcquire(s_WaitAnswer_SemaphoreId, 15000U);

      /* queues creation */
      q_msg_IPC_received_Id = rtosalMessageQueueNew((const rtosal_char_t *) "IPC_MSG_RCV",
                                                    MSG_IPC_RECEIVED_SIZE); /* create message queue */

      /* start driver thread */
      atcoreTaskId = rtosalThreadNew((const rtosal_char_t *)"AtCore",
                                     (os_pthread) ATCoreTaskBody,
                                     taskPrio,
                                     (uint32_t)stackSize,
                                     NULL);
      if (atcoreTaskId == NULL)
      {
        TRACE_ERR("atcoreTaskId creation error")
        LOG_ERROR(19, ERROR_WARNING);
        retval = ATSTATUS_ERROR;
      }
      else
      {
        retval = ATSTATUS_OK;
      }
    }
  }

  return (retval);
}
/**
  * @}
  */

/** @defgroup AT_CORE_CORE_Private_Functions AT_CORE CORE Private Functions
  * @{
  */

/**
  * @brief  Callback used to inform that a complete AT message has been received.
  * @param  ipcHandle Pointer to IPC context.
  * @retval none.
  */
static void msgReceivedCallback(IPC_Handle_t *ipcHandle)
{
  UNUSED(ipcHandle);
  /* Warning ! this function is called under IT
   * disable irq not required, we are under IT */
  MsgReceived++;
  if (rtosalMessageQueuePut(q_msg_IPC_received_Id,
                            (uint32_t)SIG_IPC_MSG, (uint32_t)0U) != osOK)
  {
    TRACE_ERR("q_msg_IPC_received_Id error for SIG_IPC_MSG")
  }
}

/**
  * @brief  Callback used to inform that an AT message has been sent.
  * @param  ipcHandle Pointer to IPC context.
  * @retval none.
  */
static void msgSentCallback(IPC_Handle_t *ipcHandle)
{
  UNUSED(ipcHandle);
  /* Warning ! this function is called under IT */
  at_context.dataSent = AT_TRUE;
  (void) rtosalSemaphoreRelease(at_context.s_SendConfirm_SemaphoreId);
}

/**
  * @brief  Wait for message reception, limited by a Timeout.
  * @param  Tickstart Initial tickstart (unused).
  * @param  Timeout Timer value.
  * @retval at_status_t.
  */
static at_status_t waitOnMsgUntilTimeout(uint32_t Tickstart, uint32_t Timeout)
{
  at_status_t retval = ATSTATUS_OK;

  UNUSED(Tickstart);

  TRACE_DBG("**** Waiting Sema (to=%lu) *****", Timeout)
  if (Timeout != 0U)
  {
    rtosalStatus sem_status;
    sem_status = rtosalSemaphoreAcquire(s_WaitAnswer_SemaphoreId, Timeout);
    /* check if sema released because IPC msg received */
    if (sem_status != ((rtosalStatus)osOK))
    {
      TRACE_DBG("**** Sema Timeout (=%ld) !!! *****", Timeout)
      retval = ATSTATUS_TIMEOUT;
    }
    TRACE_DBG("**** Sema Freed *****")
  }
  else
  {
    /* simulate a timeout */
    retval = ATSTATUS_TIMEOUT;
  }

  return (retval);
}

/**
  * @brief  Process the answer to AT command.
  * @param  action_send Bitmap of actions requested for current AT command.
  * @param  at_cmd_timeout Timer value for current AT command.
  * @retval at_action_rsp_t Action finally applied for current command.
  */
static at_action_rsp_t process_answer(at_action_send_t action_send, uint32_t at_cmd_timeout)
{
  at_action_rsp_t  action_rsp;
  at_status_t  waitIPCstatus;

  /* init tickstart for a full AT transaction */
  uint32_t tickstart = HAL_GetTick();

  do
  {
    /* Wait for response from IPC */
    waitIPCstatus = waitFromIPC(tickstart, at_cmd_timeout, &msgFromIPC);
    if (waitIPCstatus != ATSTATUS_OK)
    {
      (void) IPC_abort(at_context.ipc_handle);

      /* No response received before timeout */
      if ((action_send & ATACTION_SEND_WAIT_MANDATORY_RSP) != 0U)
      {
        /* Waiting for a response (mandatory) */
        TRACE_ERR("AT_sendcmd error: wait from ipc")

#if (DBG_DUMP_IPC_RX_QUEUE == 1)
        /* in case of advanced debug of IPC RX queue only */
        if (waitIPCstatus == ATSTATUS_TIMEOUT)
        {
          IPC_DumpRXQueue(at_context.ipc_handle, 1);
        }
#endif /* (DBG_DUMP_IPC_RX_QUEUE == 1) */

        LOG_ERROR(10, ERROR_WARNING);
        action_rsp = ATACTION_RSP_ERROR;
      }
      else /* ATACTION_SEND_TEMPO */
      {
        /* Temporisation (was waiting for a non-mandatory event)
        *  now that timer has expired, proceed to next action if needed
        */
        if ((action_send & ATACTION_SEND_FLAG_LAST_CMD) != 0U)
        {
          action_rsp = ATACTION_RSP_FRC_END;
        }
        else
        {
          action_rsp = ATACTION_RSP_FRC_CONTINUE;
        }
      }
    }
    else
    {
      /* Treat the received response */
      /* Retrieve the action which has been set on IPC msg reception in ATCoreTaskBody
      *  More than one action could has been set
      */
      if ((at_context.action_flags & ATACTION_RSP_FRC_END) != 0U)
      {
        action_rsp = ATACTION_RSP_FRC_END;
        /* clean flag */
        at_context.action_flags &= ~((at_action_rsp_t) ATACTION_RSP_FRC_END);
      }
      else if ((at_context.action_flags & ATACTION_RSP_FRC_CONTINUE) != 0U)
      {
        action_rsp = ATACTION_RSP_FRC_CONTINUE;
        /* clean flag */
        at_context.action_flags &= ~((at_action_rsp_t) ATACTION_RSP_FRC_CONTINUE);
      }
      else if ((at_context.action_flags & ATACTION_RSP_ERROR) != 0U)
      {
        /* clean flag */
        at_context.action_flags &= ~((at_action_rsp_t) ATACTION_RSP_ERROR);
        TRACE_ERR("AT_sendcmd error: parse from rsp")
        LOG_ERROR(11, ERROR_WARNING);
        action_rsp = ATACTION_RSP_ERROR;
      }
      else
      {
        /* all other actions are ignored */
        action_rsp = ATACTION_RSP_IGNORED;
      }
    }

    /* exit loop if action_rsp = ATACTION_RSP_ERROR */
  } while (action_rsp == ATACTION_RSP_IGNORED);

  return (action_rsp);
}

/**
  * @brief  Process the answer to AT command.
  * @param  msg_in_id Service ID requested.
  * @param  p_rsp_buf Pointer to the buffer with the response to the Service ID.
  * @retval at_status_t.
  */
static at_status_t process_AT_transaction(at_msg_t msg_in_id, at_buf_t *p_rsp_buf)
{
  UNUSED(p_rsp_buf);

  /* static variables (do not use stack) */
  static AT_CHAR_t build_atcmd[ATCMD_MAX_CMD_SIZE] = {0};

  /* local variables */
  at_status_t retval = ATSTATUS_OK;
  uint32_t at_cmd_timeout = 0U;
  at_action_send_t action_send;
  uint16_t build_atcmd_size;
  uint8_t another_cmd_to_send;
  at_action_rsp_t action_rsp = ATACTION_RSP_NO_ACTION;

  /* reset at cmd buffer */
  (void) memset((void *) build_atcmd, 0, ATCMD_MAX_CMD_SIZE);

  /* clear all flags*/
  at_context.action_flags = ATACTION_RSP_NO_ACTION;

  do
  {
    another_cmd_to_send = 0U; /* default value: this is the last command (will be changed if this is not the case) */

    (void) memset((void *)&build_atcmd[0], 0, sizeof(AT_CHAR_t) * ATCMD_MAX_CMD_SIZE);
    build_atcmd_size = 0U;

    /* Get command to send */
#if (USE_PARSING_MUTEX == 1)
    (void)rtosalMutexAcquire(ATCore_ParsingMutexHandle, RTOSAL_WAIT_FOREVER);
#endif /* USE_PARSING_MUTEX == 1 */
    action_send = ATParser_get_ATcmd(&at_context,
                                     (uint8_t *)&build_atcmd[0],
                                     (uint16_t)(sizeof(AT_CHAR_t) * ATCMD_MAX_CMD_SIZE),
                                     &build_atcmd_size, &at_cmd_timeout);
#if (USE_PARSING_MUTEX == 1)
    (void)rtosalMutexRelease(ATCore_ParsingMutexHandle);
#endif /* USE_PARSING_MUTEX == 1 */

    if ((action_send & ATACTION_SEND_ERROR) != 0U)
    {
      TRACE_DBG("AT_sendcmd error: get at command")
      LOG_ERROR(7, ERROR_WARNING);
      retval = ATSTATUS_ERROR;
    }
    else
    {
      /* Send AT command through IPC if a valid command is available */
      if (build_atcmd_size > 0U)
      {
        /* Before to send a command, check if current mode is DATA mode
        *  (exception if request is to suspend data mode)
        */
        if ((at_context.in_data_mode == AT_TRUE) && (msg_in_id != (at_msg_t) SID_CS_DATA_SUSPEND))
        {
          /* impossible to send a CMD during data mode */
          TRACE_ERR("DATA ongoing, can not send a command")
          LOG_ERROR(8, ERROR_WARNING);
          retval = ATSTATUS_ERROR;
        }
        else
        {
          retval = sendToIPC((uint8_t *)&build_atcmd[0], build_atcmd_size);
          if (retval != ATSTATUS_OK)
          {
            TRACE_ERR("AT_sendcmd error: send to ipc")
            LOG_ERROR(9, ERROR_WARNING);
            retval = ATSTATUS_ERROR;
          }
        }
      }

      if (retval != ATSTATUS_ERROR)
      {
        /* Wait for a response or a delay (which could be = 0)*/
        if (((action_send & ATACTION_SEND_WAIT_MANDATORY_RSP) != 0U) ||
            ((action_send & ATACTION_SEND_TEMPO) != 0U))
        {
          action_rsp = process_answer(action_send, at_cmd_timeout);
          if (action_rsp == ATACTION_RSP_FRC_CONTINUE)
          {
            /* this is not the last command */
            another_cmd_to_send = 1U;
          }
        }
        else
        {
          TRACE_ERR("Invalid action code")
          LOG_ERROR(13, ERROR_WARNING);
          retval = ATSTATUS_ERROR;
        }
      }
    }

    /* check if an error occurred */
    if (retval == ATSTATUS_ERROR)
    {
      another_cmd_to_send = 0U;
    }

  } while (another_cmd_to_send == 1U);

  /* clear all flags*/
  at_context.action_flags = ATACTION_RSP_NO_ACTION;
  TRACE_DBG("action_rsp value = %d", action_rsp)

  /* check if an error occurred during the answer processing */
  if (action_rsp == ATACTION_RSP_ERROR)
  {
    LOG_ERROR(14, ERROR_WARNING);
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Send an AT command to IPC.
  * @param  cmdBuf Pointer to the buffer containing the command to send.
  * @param  cmdSize Size of the command.
  * @retval at_status_t.
  */
static at_status_t sendToIPC(uint8_t *cmdBuf, uint16_t cmdSize)
{
  at_status_t retval;

  /* Send AT command */
  if (IPC_send(at_context.ipc_handle, cmdBuf, cmdSize) == IPC_ERROR)
  {
    TRACE_ERR(" IPC send error")
    LOG_ERROR(15, ERROR_WARNING);
    retval = ATSTATUS_ERROR;
  }
  else
  {
    (void) rtosalSemaphoreAcquire(at_context.s_SendConfirm_SemaphoreId, 5000U);
    if (at_context.dataSent == AT_TRUE)
    {
      retval = ATSTATUS_OK;
    }
    else
    {
      retval = ATSTATUS_ERROR;
    }
  }

  return (retval);
}

/**
  * @brief  Wait reception of an AT command from IPC.
  * @param  tickstart Initial tick value.
  * @param  cmdTimeout Timeout value for the current command.
  * @param  p_msg Pointer to message received from IPC.
  * @retval at_status_t.
  */
static at_status_t waitFromIPC(uint32_t tickstart, uint32_t cmdTimeout, IPC_RxMessage_t *p_msg)
{
  UNUSED(p_msg);

  at_status_t retval;

  /* wait for complete message */
  retval = waitOnMsgUntilTimeout(tickstart, cmdTimeout);
  if (retval != ATSTATUS_OK)
  {
    if (cmdTimeout != 0U)
    {
      TRACE_INFO("TIMEOUT EVENT(%ld ms)", cmdTimeout)
    }
  }

  return (retval);
}

/**
  * @brief  Analyze action bitmap.
  * @param  val Action bitmap.
  * @retval at_action_rsp_t.
  */
static at_action_rsp_t analyze_action_result(at_action_rsp_t val)
{
  at_action_rsp_t action;

  /* retrieve DATA flag value */
  at_bool_t data_mode = ((val & ATACTION_RSP_FLAG_DATA_MODE) != 0U) ? AT_TRUE : AT_FALSE;

  /* clean DATA flag value */
  action = (at_action_rsp_t)(val & ~(at_action_rsp_t)ATACTION_RSP_FLAG_DATA_MODE);

  TRACE_DBG("RAW ACTION (analyze_action_result) = 0x%x", val)
  TRACE_DBG("CLEANED ACTION=%d (data mode=%d)", action, data_mode)

  if (data_mode == AT_TRUE)
  {
    /* DATA MODE has been activated */
    if (at_context.in_data_mode == AT_FALSE)
    {
      IPC_Handle_t *h_other_ipc = IPC_get_other_channel(at_context.ipc_handle);
      if (h_other_ipc != NULL)
      {
        (void) IPC_select(h_other_ipc);
        at_context.in_data_mode = AT_TRUE;
        TRACE_INFO("<<< DATA MODE SELECTED >>>")
      }
      else
      {
        TRACE_ERR("<<< ERROR WHEN SELECTING DATA MODE >>>")
        action = ATACTION_RSP_ERROR;
      }
    }
  }
  else
  {
    /* COMMAND MODE is active */
    if (at_context.in_data_mode == AT_TRUE)
    {
      at_context.in_data_mode = AT_FALSE;

      TRACE_INFO("<<< COMMAND MODE SELECTED >>>")
    }
  }

  /* return action cleaned from DATA flag value */
  return (action);
}

/**
  * @brief  Disable IRQ.
  * @retval at_action_rsp_t.
  */
static void IRQ_DISABLE(void)
{
  __disable_irq();
}

/**
  * @brief  Enable IRQ.
  * @retval at_action_rsp_t.
  */
static void IRQ_ENABLE(void)
{
  __enable_irq();
}

/**
  * @brief  Core of AT task.
  * @param  argument Pointer to task arguments.
  * @retval none.
  */
static void ATCoreTaskBody(void *argument)
{
  UNUSED(argument);

  at_status_t retUrc;
  at_action_rsp_t action;
  rtosalStatus status;
  uint32_t msg = 0;

  static at_buf_t urc_buf[ATCMD_MAX_BUF_SIZE]; /* buffer size not optimized yet */

  TRACE_DBG("<start ATCore TASK>")

  /* Infinite loop */
  for (;;)
  {
    /* waiting IPC message received event (message) */
    status = rtosalMessageQueueGet(q_msg_IPC_received_Id,
                                   (uint32_t *)&msg, (uint32_t) RTOSAL_WAIT_FOREVER);
    if ((status == osEventMessage) || (status == osOK))
    {
      if (msg == (SIG_IPC_MSG))
      {
        /* retrieve message from IPC */
        if (IPC_receive(&ipcHandleTab, &msgFromIPC) == IPC_ERROR)
        {
          TRACE_DBG("IPC receive error")
          ATParser_abort_request(&at_context);
          TRACE_DBG("**** Sema Released on error 1 *****")
          (void) rtosalSemaphoreRelease(s_WaitAnswer_SemaphoreId);
          /* skip this loop iteration */
          continue;
        }

        /* one message has been read */
        IRQ_DISABLE();
        MsgReceived--;
        IRQ_ENABLE();

        /* Parse the response */
#if (USE_PARSING_MUTEX == 1)
        (void)rtosalMutexAcquire(ATCore_ParsingMutexHandle, RTOSAL_WAIT_FOREVER);
#endif /* USE_PARSING_MUTEX == 1 */
        action = ATParser_parse_rsp(&at_context, &msgFromIPC);
#if (USE_PARSING_MUTEX == 1)
        (void)rtosalMutexRelease(ATCore_ParsingMutexHandle);
#endif /* USE_PARSING_MUTEX == 1 */

        /* analyze the response (check data mode flag) */
        action = analyze_action_result(action);

        /* add this action to action flags only if this kind of action will be treated later */
        if ((action == ATACTION_RSP_FRC_END)
            || (action == ATACTION_RSP_FRC_CONTINUE)
            || (action == ATACTION_RSP_ERROR))
        {
          at_context.action_flags |= action;
          TRACE_DBG("add action 0x%x (flags=0x%x)", action, at_context.action_flags)
        }
        if (action == ATACTION_RSP_ERROR)
        {
          TRACE_ERR("AT_sendcmd error")
          ATParser_abort_request(&at_context);
          TRACE_DBG("**** Sema Released on error 2 *****")
          (void) rtosalSemaphoreRelease(s_WaitAnswer_SemaphoreId);
          continue;
        }

        /* check if this is an URC to forward */
        if (action == ATACTION_RSP_URC_FORWARDED)
        {
          /* notify user with callback */
          if (register_URC_callback != NULL)
          {
            /* get URC response buffer */
            do
            {
              (void) memset((void *) urc_buf, 0, ATCMD_MAX_BUF_SIZE);
              retUrc = ATParser_get_urc(&at_context, urc_buf);
              if ((retUrc == ATSTATUS_OK) || (retUrc == ATSTATUS_OK_PENDING_URC))
              {
                /* call the URC callback */
                (* register_URC_callback)(urc_buf);
              }
            } while (retUrc == ATSTATUS_OK_PENDING_URC);
          }
        }
        else if ((action == ATACTION_RSP_FRC_CONTINUE) ||
                 (action == ATACTION_RSP_FRC_END) ||
                 (action == ATACTION_RSP_ERROR))
        {
          TRACE_DBG("**** Sema released *****")
          (void) rtosalSemaphoreRelease(s_WaitAnswer_SemaphoreId);
        }
        else
        {
          /* nothing to do */
        }
      }
      else if (msg == (SIG_INTERNAL_EVENT_MODEM))
      {
        /* An internal event has been received (ie not coming from IPC: could be an interrupt from modem,...)
         * Do not call IPC_receive in this case
         */
        TRACE_DBG("!!! an internal event has been received !!!")
        if (register_URC_callback != NULL)
        {
          do
          {
            (void) memset((void *) urc_buf, 0, ATCMD_MAX_BUF_SIZE);
            retUrc = ATParser_get_urc(&at_context, urc_buf);
            if ((retUrc == ATSTATUS_OK) || (retUrc == ATSTATUS_OK_PENDING_URC))
            {
              /* call the URC callback */
              (* register_URC_callback)(urc_buf);
            }
          } while (retUrc == ATSTATUS_OK_PENDING_URC);
        }
      }
      else
      {
        /* should not happen */
        __NOP();
      }
    }
  }
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

