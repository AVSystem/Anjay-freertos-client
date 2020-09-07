/**
  ******************************************************************************
  * @file    ipc_uart.c
  * @author  MCD Application Team
  * @brief   This file provides code for uart IPC
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
#include <string.h>
#include <stdbool.h>
#include "ipc_uart.h"
#include "ipc_rxfifo.h"
#include "plf_config.h"

#if (RTOS_USED == 1)
#include "cmsis_os_misrac2012.h"
#endif /* RTOS_USED */

/* Private typedef -----------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/

#if (USE_TRACE_IPC == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_INFO(format, args...) TRACE_PRINT(DBG_CHAN_IPC, DBL_LVL_P0, "IPC:" format "\n\r", ## args)
#define PRINT_DBG(format, args...)  TRACE_PRINT(DBG_CHAN_IPC, DBL_LVL_P1, "IPC:" format "\n\r", ## args)
#define PRINT_ERR(format, args...)  TRACE_PRINT(DBG_CHAN_IPC, DBL_LVL_ERR, "IPC ERROR:" format "\n\r", ## args)
#else
#include <stdio.h>
#define PRINT_INFO(format, args...)  (void) printf("IPC:" format "\n\r", ## args);
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(format, args...)   (void) printf("IPC ERROR:" format "\n\r", ## args);
#endif /* USE_PRINTF */
#else
#define PRINT_INFO(...)  __NOP(); /* Nothing to do */
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(...)   __NOP(); /* Nothing to do */
#endif /* USE_TRACE_IPC */

/* Private variables ---------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static uint8_t find_Device_Id(const UART_HandleTypeDef *huart);
static IPC_Status_t change_ipc_channel(IPC_Handle_t *hipc);

/* Functions Definition ------------------------------------------------------*/
/**
  * @brief  The function initialize the global variables of the IPC.
  * @param  device IPC device identifier.
  * @param  huart Handle to the HAL UART structure.
  * @retval status
  */
IPC_Status_t IPC_UART_init(IPC_Device_t device, UART_HandleTypeDef  *huart)
{
  IPC_Status_t retval;
  /* input parameters validity has been tested in calling function */

  /* check if this device has not been already initialized */
  if (IPC_DevicesList[device].state != IPC_STATE_NOT_INITIALIZED)
  {
    retval = IPC_ERROR;
  }
  else
  {
    IPC_DevicesList[device].state = IPC_STATE_INITIALIZED;
    IPC_DevicesList[device].phy_int.interface_type = IPC_INTERFACE_UART;
    IPC_DevicesList[device].phy_int.h_uart = huart;
    IPC_DevicesList[device].h_current_channel = NULL;
    IPC_DevicesList[device].h_inactive_channel = NULL;
    retval = IPC_OK;
  }

  return (retval);
}

/**
  * @brief  Reinitialize the IPC global variables.
  * @param  device IPC device identifier.
  * @retval status
  */
IPC_Status_t IPC_UART_deinit(IPC_Device_t device)
{
  /* input parameters validity has been tested in calling function */

  IPC_DevicesList[device].state = IPC_STATE_NOT_INITIALIZED;
  IPC_DevicesList[device].phy_int.interface_type = IPC_INTERFACE_UNINITIALIZED;
  IPC_DevicesList[device].phy_int.h_uart = NULL;
  IPC_DevicesList[device].h_current_channel = NULL;
  IPC_DevicesList[device].h_inactive_channel = NULL;

  return (IPC_OK);
}

/**
  * @brief  Open a specific channel.
  * @param  hipc IPC handle to open.
  * @param  device IPC device identifier.
  * @param  mode IPC mode (char or stream).
  * @param  pRxClientCallback Callback ptr called when a message has been received.
  * @param  pTxClientCallback Callback ptr called when a message has been send.
  * @param  pCheckEndOfMsg Callback ptr to the function used to analyze if char received is a termination char
  * @retval status
  */
IPC_Status_t IPC_UART_open(IPC_Handle_t *hipc,
                           IPC_Device_t  device,
                           IPC_Mode_t    mode,
                           IPC_RxCallbackTypeDef pRxClientCallback,
                           IPC_TxCallbackTypeDef pTxClientCallback,
                           IPC_CheckEndOfMsgCallbackTypeDef pCheckEndOfMsg)
{
  IPC_Status_t retval;
  HAL_StatusTypeDef uart_status;

  /* some input parameters have been already tested in calling function */
  if ((mode != IPC_MODE_UART_CHARACTER) && (mode != IPC_MODE_UART_STREAM))
  {
    retval = IPC_ERROR;
  }
  else if ((mode == IPC_MODE_UART_CHARACTER) && (pCheckEndOfMsg == NULL))
  {
    retval = IPC_ERROR;
  }
  else
  {
    /* Register this channel into the IPC devices list */
    if (IPC_DevicesList[device].h_current_channel == NULL)
    {
      /* register this channel as default channel (first call) */
      IPC_DevicesList[device].h_current_channel = hipc;
      retval = IPC_OK;
    }
    else if (IPC_DevicesList[device].h_inactive_channel == NULL)
    {
      /* another channel already registered and active, register this channel as inactive */
      IPC_DevicesList[device].h_inactive_channel = hipc;
      retval = IPC_OK;
    }
    else
    {
      /* supports only 2 channels for same IPC device */
      retval = IPC_ERROR;
    }
  }

  if (retval != IPC_ERROR)
  {
    /* initialize common RX buffer */
    IPC_DevicesList[device].RxChar[0] = (IPC_CHAR_t)('\0');

    PRINT_DBG("IPC channel %p registered", IPC_DevicesList[device].h_current_channel)
    PRINT_DBG("state 0x%x", IPC_DevicesList[device].state)
    PRINT_DBG("active channel handle: %p", IPC_DevicesList[device].h_current_channel)
    PRINT_DBG("inactive channel handle: %p", IPC_DevicesList[device].h_inactive_channel)

    /* select default queue (character or stream) */
    if (mode == IPC_MODE_UART_CHARACTER)
    {
      hipc->RxFifoWrite = IPC_RXFIFO_writeCharacter;
    }
#if (IPC_USE_STREAM_MODE == 1U)
    else
    {
      hipc->RxFifoWrite = IPC_RXFIFO_writeStream;
    }
#endif /* IPC_USE_STREAM_MODE */

    /* initialize IPC channel parameters */
    hipc->Device_ID = device;
    hipc->Interface.interface_type = IPC_DevicesList[device].phy_int.interface_type;
    hipc->Interface.h_uart = IPC_DevicesList[device].phy_int.h_uart;
    hipc->State = IPC_STATE_INITIALIZED;

    /* register client callback */
    hipc->RxClientCallback = pRxClientCallback;
    hipc->TxClientCallback = pTxClientCallback;
    hipc->CheckEndOfMsgCallback = pCheckEndOfMsg;
    hipc->Mode = mode;
    hipc->UartBusyFlag = 0U;

    /* init RXFIFO */
    IPC_RXFIFO_init(hipc);
#if (IPC_USE_STREAM_MODE == 1U)
    IPC_RXFIFO_stream_init(hipc);
#endif /* IPC_USE_STREAM_MODE */

    /* start RX IT */
    uart_status = HAL_UART_Receive_IT(hipc->Interface.h_uart, (uint8_t *)IPC_DevicesList[device].RxChar, 1U);
    if (uart_status != HAL_OK)
    {
      PRINT_ERR("HAL_UART_Receive_IT error")
      retval = IPC_ERROR;
    }
    else
    {
      hipc->State = IPC_STATE_ACTIVE;
      retval = IPC_OK;
    }
  }
  return (retval);
}

/**
  * @brief  Close a specific channel.
  * @param  hipc IPC handle to close.
  * @retval status
  */
IPC_Status_t IPC_UART_close(IPC_Handle_t *hipc)
{
  IPC_Status_t retval;

  /* input parameters validity has been tested in calling function */
  if (hipc->State != IPC_STATE_NOT_INITIALIZED)
  {
    /* reset IPC status */
    hipc->State = IPC_STATE_NOT_INITIALIZED;
    hipc->RxClientCallback = NULL;
    hipc->CheckEndOfMsgCallback = NULL;

    /* init RXFIFO */
    IPC_RXFIFO_init(hipc);
#if (IPC_USE_STREAM_MODE == 1U)
    IPC_RXFIFO_stream_init(hipc);
#endif /* IPC_USE_STREAM_MODE */

    /* update IPC_DevicesList state */
    uint8_t device_id = hipc->Device_ID;
    if (device_id != IPC_DEVICE_NOT_FOUND)
    {
      if (IPC_DevicesList[device_id].h_current_channel == hipc)
      {
        /* if closed IPC was active, remove it and inactive IPC becomes active (or NULL) */
        IPC_DevicesList[device_id].h_current_channel = IPC_DevicesList[device_id].h_inactive_channel;
        IPC_DevicesList[device_id].h_inactive_channel = NULL;
      }
      else if (IPC_DevicesList[device_id].h_inactive_channel == hipc)
      {
        /* if closed IPC was inactive, just remove it */
        IPC_DevicesList[device_id].h_inactive_channel = NULL;
      }
      else
      {
        /* should not happen */
      }

      retval = IPC_OK;

      /* if no more registered channel, abort RX transfer */
      if ((IPC_DevicesList[device_id].h_current_channel == NULL) &&
          (IPC_DevicesList[device_id].h_inactive_channel == NULL))
      {
        if (hipc->Interface.h_uart != NULL)
        {
          (void)HAL_UART_AbortTransmit_IT(hipc->Interface.h_uart);
        }
      }

      PRINT_DBG("IPC channel %p closed", hipc)
      PRINT_DBG("state 0x%x", IPC_DevicesList[hipc->Device_ID].state)
      PRINT_DBG("active channel handle: %p", IPC_DevicesList[hipc->Device_ID].h_current_channel)
      PRINT_DBG("inactive channel handle: %p", IPC_DevicesList[hipc->Device_ID].h_inactive_channel)
    }
    else
    {
      /* no device found */
      retval = IPC_ERROR;
    }
  }
  else
  {
    /* ipc not initialized */
    retval = IPC_ERROR;
  }

  return (retval);
}

/**
  * @brief  Reset IPC handle to select.
  * @retval status
  */
IPC_Status_t IPC_UART_reset(IPC_Handle_t *hipc)
{
  IPC_Status_t retval = IPC_ERROR;
  PRINT_DBG("IPC reset %p", hipc)
  /* input parameters validity has been tested in calling function */

  /* initialize common RX buffer */
  uint8_t device_id = hipc->Device_ID;
  if (device_id != IPC_DEVICE_NOT_FOUND)
  {
    IPC_DevicesList[device_id].RxChar[0] = (IPC_CHAR_t)('\0');

    /* init RXFIFO */
    IPC_RXFIFO_init(hipc);
#if (IPC_USE_STREAM_MODE == 1U)
    IPC_RXFIFO_stream_init(hipc);
#endif /* IPC_USE_STREAM_MODE */

    /* rearm IT */
    (void) HAL_UART_Receive_IT(hipc->Interface.h_uart, (uint8_t *)IPC_DevicesList[device_id].RxChar, 1U);
    hipc->State = IPC_STATE_ACTIVE;
    retval = IPC_OK;
  }

  return (retval);
}

/**
  * @brief  Abort IPC transaction.
  * @retval status
  */
IPC_Status_t IPC_UART_abort(IPC_Handle_t *hipc)
{
  PRINT_DBG("IPC abort %p", hipc)
  /* input parameters validity has been tested in calling function */

  if (hipc->Interface.h_uart != NULL)
  {
    if (hipc->Interface.h_uart->gState != HAL_UART_STATE_RESET)
    {
      (void)HAL_UART_AbortTransmit_IT(hipc->Interface.h_uart);
    }
  }

  return (IPC_OK);
}

/**
  * @brief  Select current channel.
  * @param  hipc IPC handle to select.
  * @retval status
  */
IPC_Status_t IPC_UART_select(IPC_Handle_t *hipc)
{
  PRINT_DBG("IPC select %p", hipc)
  if (hipc != IPC_DevicesList[hipc->Device_ID].h_current_channel)
  {
    (void) change_ipc_channel(hipc);
  }

  return (IPC_OK);
}

/**
  * @brief  Get other channel handle if exists.
  * @param  hipc IPC handle.
  * @retval IPC_Handle_t*
  */
IPC_Handle_t *IPC_UART_get_other_channel(const IPC_Handle_t *hipc)
{
  IPC_Handle_t *handle = NULL;

  if ((IPC_DevicesList[hipc->Device_ID].h_current_channel == hipc) &&
      (IPC_DevicesList[hipc->Device_ID].h_inactive_channel != NULL))
  {
    handle = IPC_DevicesList[hipc->Device_ID].h_inactive_channel;
  }

  if ((IPC_DevicesList[hipc->Device_ID].h_inactive_channel == hipc) &&
      (IPC_DevicesList[hipc->Device_ID].h_current_channel != NULL))
  {
    handle = IPC_DevicesList[hipc->Device_ID].h_current_channel;
  }

  /* no other valid channel available */
  return (handle);
}

/**
  * @brief  Send data over an UART channel.
  * @param  hipc IPC handle.
  * @param  p_TxBuffer Pointer to the data buffer to transfer.
  * @param  bufsize Length of the data buffer.
  * @retval status
  */
IPC_Status_t IPC_UART_send(IPC_Handle_t *hipc, uint8_t *p_TxBuffer, uint16_t bufsize)
{
  IPC_Status_t retval;

#if (RTOS_USED == 1)
  /* Test if current hipc */
  if (hipc != IPC_DevicesList[hipc->Device_ID].h_current_channel)
  {
    retval = IPC_ERROR;
  }
  else
#endif /* RTOS_USED */
  {
    /* send string in one block */
    while (true)
    {
      HAL_StatusTypeDef err;
      err = HAL_UART_Transmit_IT(hipc->Interface.h_uart, (uint8_t *)p_TxBuffer, bufsize);
      if (err !=  HAL_BUSY)
      {
        if ((hipc->UartBusyFlag == 1U) && (hipc->Interface.interface_type == IPC_INTERFACE_UART))
        {
          hipc->UartBusyFlag = 0U;
          while (HAL_UART_Receive_IT(hipc->Interface.h_uart, (uint8_t *)IPC_DevicesList[hipc->Device_ID].RxChar, 1U)
                 != HAL_OK)
          {
          }
          PRINT_INFO("Receive rearmed...")
        }
        break;
      }

#if (RTOS_USED == 1)
      (void) osDelay(10U);
#endif /* RTOS_USED */
    }
    retval = IPC_OK;
  }
  return (retval);
}

/**
  * @brief  Receive a message from an UART channel.
  * @param  hipc IPC handle.
  * @param  p_msg Pointer to the IPC message structure to fill with received message.
  * @retval status
  */
IPC_Status_t IPC_UART_receive(IPC_Handle_t *hipc, IPC_RxMessage_t *p_msg)
{
  IPC_Status_t retval;
  int16_t unread_msg;
#if (DBG_IPC_RX_FIFO == 1U)
  int16_t free_bytes;
#endif /* DBG_IPC_RX_FIFO */

  /* check the handle */
  if (hipc->Mode == IPC_MODE_UART_CHARACTER)
  {
    if (p_msg == NULL)
    {
      PRINT_ERR("IPC_receive err - p_msg NULL")
      retval = IPC_ERROR;
    }
    else
    {
#if (DBG_IPC_RX_FIFO == 1U)
      free_bytes = IPC_RXFIFO_getFreeBytes(hipc);
      PRINT_DBG("free_bytes before msg read=%d", free_bytes)
#endif /* DBG_IPC_RX_FIFO */

      /* read the first unread message */
      unread_msg = IPC_RXFIFO_read(hipc, p_msg);
      if (unread_msg == -1)
      {
        PRINT_ERR("IPC_receive err - no unread msg")
        retval = IPC_ERROR;
      }
      else
      {
#if (DBG_IPC_RX_FIFO == 1U)
        free_bytes = IPC_RXFIFO_getFreeBytes(hipc);
        PRINT_DBG("free bytes after msg read=%d", free_bytes)
#endif /* DBG_IPC_RX_FIFO */

        if (hipc->State == IPC_STATE_PAUSED)
        {
#if (DBG_IPC_RX_FIFO == 1U)
          /* dump_RX_dbg_infos(hipc, 1, 1); */
          PRINT_INFO("Resume IPC (paused %d times) %d unread msg", hipc->dbgRxQueue.cpt_RXPause, unread_msg)
#endif /* DBG_IPC_RX_FIFO */

          hipc->State = IPC_STATE_ACTIVE;
          (void) HAL_UART_Receive_IT(hipc->Interface.h_uart, (uint8_t *)IPC_DevicesList[hipc->Device_ID].RxChar, 1U);
        }

        if (unread_msg == 0)
        {
          retval = IPC_RXQUEUE_EMPTY;
        }
        else
        {
          retval = IPC_RXQUEUE_MSG_AVAIL;
        }
      }
    }
  }
  else
  {
    PRINT_ERR("IPC_receive err - IPC mode not matching")
    retval = IPC_ERROR;
  }

  return (retval);
}

#if (IPC_USE_STREAM_MODE == 1U)
/**
  * @brief  Receive a data buffer from an UART channel.
  * @param  hipc IPC handle.
  * @param  p_buffer Pointer to the data buffer to transfer.
  * @param  p_len Length of the data buffer.
  * @note   p_len value received from user indicates the maximum user buffer length
  *         and p_len is updated with received buffer size.
  * @retval status
  */
IPC_Status_t IPC_UART_streamReceive(IPC_Handle_t *hipc,  uint8_t *p_buffer, int16_t *p_len)
{
  IPC_Status_t retval;
  uint16_t rx_size = 0;

  if (*p_len > 0)
  {
    uint16_t maximum_buffer_size = (uint16_t) * p_len;

    if (hipc->Mode == IPC_MODE_UART_STREAM)
    {
      /* receive */
      while ((hipc->RxBuffer.available_char != 0U) &&
             (rx_size < maximum_buffer_size))
      {
        p_buffer[rx_size] = hipc->RxBuffer.data[hipc->RxBuffer.index_read];
        hipc->RxBuffer.index_read++;
        if (hipc->RxBuffer.index_read >= IPC_RXBUF_STREAM_MAXSIZE)
        {
          hipc->RxBuffer.index_read = 0;
        }
        rx_size++;
        hipc->RxBuffer.available_char--;
      }

      /* update buffer size */
      *p_len = (int16_t) rx_size;
      retval = IPC_OK;
    }
    else
    {
      PRINT_ERR("IPC_receive err - IPC mode not matching")
      retval = IPC_ERROR;
    }
  }
  else
  {
    PRINT_ERR("IPC_receive err - Invalid buffer size")
    retval = IPC_ERROR;
  }

  return (retval);
}
#endif  /* IPC_USE_STREAM_MODE */

/**
  * @brief  Rearm RX interrupt.
  * @param  hipc IPC handle to select.
  * @retval none.
  */
void IPC_UART_rearm_RX_IT(IPC_Handle_t *hipc)
{
  HAL_StatusTypeDef err ;

  if (hipc != NULL)
  {
    /* Comment: specific to UART, should be in ipc_uart.c */
    /* rearm uart TX interrupt */
    if (hipc->Interface.interface_type == IPC_INTERFACE_UART)
    {
      err = HAL_UART_Receive_IT(hipc->Interface.h_uart, (uint8_t *)IPC_DevicesList[hipc->Device_ID].RxChar, 1U);
      if (err != HAL_OK)
      {
        hipc->UartBusyFlag = 1U;
      }
    }
  }
}

#if (DBG_IPC_RX_FIFO == 1U)
/**
  * @brief  Dump content of IPC Rx queue (for debug purpose).
  * @param  hipc IPC handle.
  * @param  readable If equal 1, print special characters explicitly (<CR>, <LF>, <NULL>).
  * @retval none
  */
void IPC_UART_DumpRXQueue(const IPC_Handle_t *hipc, uint8_t readable)
{
#if (USE_TRACE_IPC == 1U)
  IPC_RxHeader_t header;
  uint16_t dump_index;
  uint16_t first_uncomplete_header;
  uint16_t first_uncomplete_size;
  uint16_t last_index;

  /* Take a picture of the RX queue at the time of entry in this function
    *  new char could be received but we do not take them into account
    * => set variables now
  */
  first_uncomplete_header = hipc->RxQueue.current_msg_index;
  first_uncomplete_size = hipc->RxQueue.current_msg_size;
  dump_index = hipc->RxQueue.index_read;
  last_index = hipc->RxQueue.index_write;

  PRINT_INFO(" *** IPC state =%d ", hipc->State)
  PRINT_INFO(" *** First header position =%d ", dump_index)
  PRINT_INFO(" *** Last incomplete header position=%d ", first_uncomplete_header)
  PRINT_INFO(" *** Current write pos=%d ", last_index)

  /* read message header */
  IPC_RXFIFO_readMsgHeader_at_pos(hipc, &header, dump_index);

  while ((header.complete == 1U) && (dump_index != first_uncomplete_header))
  {
    uint16_t core_msg_index = (dump_index + IPC_RXMSG_HEADER_SIZE) % IPC_RXBUF_MAXSIZE;
    PRINT_INFO(" ### Complete msg, size=%d, data pos=%d:", header.size, core_msg_index)
    IPC_RXFIFO_print_data(hipc, core_msg_index, header.size, readable);
    /* read next message header */
    dump_index = (dump_index + IPC_RXMSG_HEADER_SIZE + header.size) % IPC_RXBUF_MAXSIZE;
    IPC_RXFIFO_readMsgHeader_at_pos(hipc, &header, dump_index);
  }

  /* debug info: last queue message */
  if (header.complete == 1U)
  {
    /* should not happen... */
    uint16_t core_msg_index = (dump_index + IPC_RXMSG_HEADER_SIZE) % IPC_RXBUF_MAXSIZE;
    PRINT_INFO(" ### Last msg is complete, size=%d, data pos=%d: ", header.size, core_msg_index)
    IPC_RXFIFO_print_data(hipc, (dump_index + IPC_RXMSG_HEADER_SIZE), first_uncomplete_size, readable);
  }
  else
  {
    uint16_t core_msg_index = (dump_index + IPC_RXMSG_HEADER_SIZE) % IPC_RXBUF_MAXSIZE;
    PRINT_INFO(" ### Last msg is not complete, size=%d, pos=%d: ", first_uncomplete_size, core_msg_index)
    IPC_RXFIFO_print_data(hipc, core_msg_index, first_uncomplete_size, readable);
  }
#else
  UNUSED(hipc);
  UNUSED(readable);
#endif /* USE_TRACE_IPC */
}
#endif /* DBG_IPC_RX_FIFO */

/* Callback functions ----------------------------------------------------------*/
/**
  * @brief  IPC uart RX callback (called under IT !).
  * @param  UartHandle Ptr to the HAL UART handle.
  * @retval none
  */
void IPC_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
  /* Warning ! this function is called under IT */
  uint8_t device_id = find_Device_Id(UartHandle);
  if (device_id < IPC_MAX_DEVICES)
  {
    if (IPC_DevicesList[device_id].h_current_channel != NULL)
    {
      IPC_DevicesList[device_id].h_current_channel->RxFifoWrite(IPC_DevicesList[device_id].h_current_channel,
                                                                IPC_DevicesList[device_id].RxChar[0]);
    }
  }
}

/**
  * @brief  IPC uart TX callback (called under IT !).
  * @param  UartHandle Ptr to the HAL UART handle.
  * @retval none
  */
void IPC_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle)
{
  /* Warning ! this function is called under IT */
  uint8_t device_id = find_Device_Id(UartHandle);

  if (device_id < IPC_MAX_DEVICES)
  {
    if (IPC_DevicesList[device_id].h_current_channel != NULL)
    {
      /* Set transmission flag: transfer complete */
      IPC_DevicesList[device_id].h_current_channel->TxClientCallback(
        IPC_DevicesList[device_id].h_current_channel
      );
    }
  }
}

/**
  * @brief  IPC uart error callback (called under IT !).
  * @param  UartHandle Ptr to the HAL UART handle.
  * @retval none
  */
void IPC_UART_ErrorCallback(UART_HandleTypeDef *UartHandle)
{
  UNUSED(UartHandle);
  /* Warning ! this function is called under IT */
}

/* Private function Definition -----------------------------------------------*/
/**
  * brief  Find an IPC device corresponding to an UART handle.
  * param  huart Handle to the HAL UART structure.
  * retval device_id if found, otherwise returns IPC_DEVICE_NOT_FOUND.
  */
static uint8_t find_Device_Id(const UART_HandleTypeDef *huart)
{
  /* search device corresponding to this uart in the devices list */
  uint8_t device_id = IPC_DEVICE_NOT_FOUND;
  uint8_t idx = 0U;
  bool leave_loop = false;

  do
  {
    /* search the device corresponding to this instance */
    if (huart->Instance == IPC_DevicesList[idx].phy_int.h_uart->Instance)
    {
      /* force loop exit */
      device_id = idx;
      leave_loop = true;
    }
    idx++;
  } while ((leave_loop == false) && (idx < IPC_MAX_DEVICES));

  return (device_id);
}

/**
  * brief  Change the IPC channel.
  * param  hipc IPC handle.
  * retval status
  */
static IPC_Status_t change_ipc_channel(IPC_Handle_t *hipc)
{
  IPC_Handle_t *tmp_handle;
  tmp_handle = IPC_DevicesList[hipc->Device_ID].h_current_channel;

  /* swap channels */
  IPC_DevicesList[hipc->Device_ID].h_current_channel = hipc;
  IPC_DevicesList[hipc->Device_ID].h_inactive_channel = tmp_handle;

  PRINT_DBG("Change IPC channels")
  PRINT_DBG("state 0x%x", IPC_DevicesList[hipc->Device_ID].state)
  PRINT_DBG("active channel handle: %p", IPC_DevicesList[hipc->Device_ID].h_current_channel)
  PRINT_DBG("inactive channel handle: %p", IPC_DevicesList[hipc->Device_ID].h_inactive_channel)

  return (IPC_OK);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

