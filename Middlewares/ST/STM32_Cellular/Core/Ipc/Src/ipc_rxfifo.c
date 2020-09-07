/**
  ******************************************************************************
  * @file    ipc_rxfifo.c
  * @author  MCD Application Team
  * @brief   This file provides common code for managing IPC RX FIFO
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
#include "ipc_rxfifo.h"
#include "ipc_common.h"
#include "plf_config.h"
#if (IPC_USE_UART == 1U)
#include "ipc_uart.h"
#endif /* (IPC_USE_UART == 1U) */

/* Private typedef -----------------------------------------------------------*/
typedef char IPC_TYPE_CHAR_t;

/* Private defines -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/
#if (USE_TRACE_IPC == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_INFO(format, args...) TRACE_PRINT(DBG_CHAN_IPC, DBL_LVL_P0, "IPC:" format "\n\r", ## args)
#define PRINT_DBG(format, args...)  TRACE_PRINT(DBG_CHAN_IPC, DBL_LVL_P1, "IPC:" format "\n\r", ## args)
#define PRINT_ERR(format, args...)  TRACE_PRINT(DBG_CHAN_IPC, DBL_LVL_ERR, "IPC ERROR:" format "\n\r", ## args)
#define PRINT_BUF(pbuf, size)       \
  TRACE_PRINT_BUF_CHAR(DBG_CHAN_ATCMD, DBL_LVL_P0, (const IPC_TYPE_CHAR_t *)pbuf, size);
#define PRINT_BUF_HEXA(pbuf, size)  \
  TRACE_PRINT_BUF_HEX(DBG_CHAN_ATCMD, DBL_LVL_P0, (const IPC_TYPE_CHAR_t *)pbuf, size);
#else
#include <stdio.h>
#define PRINT_INFO(format, args...)  (void) printf("IPC:" format "\n\r", ## args);
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(format, args...)   (void) printf("IPC ERROR:" format "\n\r", ## args);
#define PRINT_BUF(...)   __NOP(); /* Nothing to do */
#define PRINT_BUF_HEXA(...)   __NOP(); /* Nothing to do */
#endif /* USE_PRINTF */
#else
#define PRINT_INFO(...)  __NOP(); /* Nothing to do */
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(...)   __NOP(); /* Nothing to do */
#define PRINT_BUF(...)   __NOP(); /* Nothing to do */
#define PRINT_BUF_HEXA(...)   __NOP(); /* Nothing to do */
#endif /* USE_TRACE_IPC */

/* Private variables ---------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void RXFIFO_incrementTail(IPC_Handle_t *hipc, uint16_t inc_size);
static void RXFIFO_incrementHead(IPC_Handle_t *hipc);
static void RXFIFO_updateMsgHeader(IPC_Handle_t *hipc);
static void RXFIFO_prepareNextMsgHeader(IPC_Handle_t *hipc);
static void RXFIFO_rearm_RX_IT(IPC_Handle_t *hipc);

/* Functions Definition ------------------------------------------------------*/
/**
  * @brief  The function initialize the IPC RX FIFO.
  * @param  hipc IPC handle.
  * @retval none.
  */
void IPC_RXFIFO_init(IPC_Handle_t *hipc)
{
  (void) memset(hipc->RxQueue.data, 0, sizeof(uint8_t) * IPC_RXBUF_MAXSIZE);
  hipc->RxQueue.index_read = 0U;
  hipc->RxQueue.index_write = IPC_RXMSG_HEADER_SIZE;
  hipc->RxQueue.current_msg_index = 0U;
  hipc->RxQueue.current_msg_size = 0U;
  hipc->RxQueue.nb_unread_msg = 0U;

#if (DBG_IPC_RX_FIFO == 1U)
  /* init debug infos */
  hipc->dbgRxQueue.total_RXchar = 0U;
  hipc->dbgRxQueue.cpt_RXPause = 0U;
  hipc->dbgRxQueue.free_bytes = IPC_RXBUF_MAXSIZE;
  hipc->dbgRxQueue.queue_pos = 0U;
  hipc->dbgRxQueue.msg_info_queue[0].start_pos = hipc->RxQueue.index_read;
  hipc->dbgRxQueue.msg_info_queue[0].size = 0U;
  hipc->dbgRxQueue.msg_info_queue[0].complete = 0U;
#endif /* DBG_IPC_RX_FIFO */
}

/**
  * @brief  Write a char in the IPC RX FIFO.
  * @param  hipc IPC handle.
  * @param  rxChar character to write.
  * @retval none.
  */
void IPC_RXFIFO_writeCharacter(IPC_Handle_t *hipc, uint8_t rxChar)
{
  if (hipc != NULL)
  {
    hipc->RxQueue.data[hipc->RxQueue.index_write] = rxChar;

    hipc->RxQueue.current_msg_size++;

#if (DBG_IPC_RX_FIFO == 1U)
    hipc->dbgRxQueue.msg_info_queue[hipc->dbgRxQueue.queue_pos].size = hipc->RxQueue.current_msg_size;
#endif /* DBG_IPC_RX_FIFO */

    RXFIFO_incrementHead(hipc);

    if (hipc->State != IPC_STATE_PAUSED)
    {
      /* rearm RX Interrupt */
      RXFIFO_rearm_RX_IT(hipc);
    }

    /* check if the char received is an end of message */
    if ((*hipc->CheckEndOfMsgCallback)(rxChar) == 1U)
    {
      hipc->RxQueue.nb_unread_msg++;

      /* update header for message received */
      RXFIFO_updateMsgHeader(hipc);

      /* save start position of next message */
      hipc->RxQueue.current_msg_index = hipc->RxQueue.index_write;

      /* reset current msg size */
      hipc->RxQueue.current_msg_size = 0U;

      /* reserve place for next msg header */
      RXFIFO_prepareNextMsgHeader(hipc);

      /* msg received: call client callback */
      (* hipc->RxClientCallback)((IPC_Handle_t *)hipc);
    }
  }
}

/**
  * @brief  Read first unread message in the IPC RX FIFO.
  * @param  hipc IPC handle.
  * @param  pMsg ptr to the message read from IPC RX FIFO.
  * @retval message size (-1 if an error occured).
  */
int16_t IPC_RXFIFO_read(IPC_Handle_t *hipc, IPC_RxMessage_t *pMsg)
{
  int16_t retval;
  uint16_t oversize;
  IPC_RxHeader_t header;

  if (hipc != NULL)
  {
#if (DBG_IPC_RX_FIFO == 1U)
    PRINT_DBG(" *** start pos=%d ", hipc->RxQueue.index_read)
#endif /* DBG_IPC_RX_FIFO */

    /* read message header */
    IPC_RXFIFO_readMsgHeader_at_pos(hipc, &header, hipc->RxQueue.index_read);

    if (header.complete != 1U)
    {
      /* error: trying to read an uncomplete message */
      retval = -1;
    }
    else
    {
      /* jump header */
      RXFIFO_incrementTail(hipc, IPC_RXMSG_HEADER_SIZE);

#if (DBG_IPC_RX_FIFO == 1U)
      PRINT_DBG(" *** data pos=%d ", hipc->RxQueue.index_read)
      PRINT_DBG(" *** size=%d ", header.size)
      PRINT_DBG(" *** free bytes before read=%d ", hipc->dbgRxQueue.free_bytes)
#endif /* DBG_IPC_RX_FIFO */

      /* update size in output structure */
      pMsg->size = header.size;

      /* copy msg content to output structure */
      if ((hipc->RxQueue.index_read + header.size) > IPC_RXBUF_MAXSIZE)
      {
        /* message is split in 2 parts in the circular buffer */
        oversize = (hipc->RxQueue.index_read + header.size - IPC_RXBUF_MAXSIZE);
        uint16_t remaining_size = header.size - oversize;
        (void) memcpy((void *) & (pMsg->buffer[0]),
                      (void *) & (hipc->RxQueue.data[hipc->RxQueue.index_read]),
                      (size_t) remaining_size);
        (void) memcpy((void *) & (pMsg->buffer[header.size - oversize]),
                      (void *) & (hipc->RxQueue.data[0]),
                      (size_t) oversize);

#if (DBG_IPC_RX_FIFO == 1U)
        PRINT_DBG("override end of buffer")
#endif /* DBG_IPC_RX_FIFO */
      }
      else
      {
        /* message is contiguous in the circular buffer */
        (void) memcpy((void *)pMsg->buffer,
                      (void *) & (hipc->RxQueue.data[hipc->RxQueue.index_read]),
                      (size_t) header.size);
      }

      /* increment tail index to the next message */
      RXFIFO_incrementTail(hipc, header.size);

#if (DBG_IPC_RX_FIFO == 1U)
      /* update free_bytes infos */
      hipc->dbgRxQueue.free_bytes = IPC_RXFIFO_getFreeBytes(hipc);
      PRINT_DBG(" *** free after read bytes=%d ", hipc->dbgRxQueue.free_bytes)
#endif /* DBG_IPC_RX_FIFO */

      /* msg has been read */
      hipc->RxQueue.nb_unread_msg--;

      /* return number of unread messages */
      retval = (int16_t)hipc->RxQueue.nb_unread_msg;
    }
  }
  else
  {
    /* error: hipc is NULL */
    retval = -1;
  }

  return (retval);
}

#if (IPC_USE_STREAM_MODE == 1U)
/**
  * @brief Initialize IPC RX FIFO for the stream mode.
  * @param  hipc IPC handle.
  * @retval none.
  */
void IPC_RXFIFO_stream_init(IPC_Handle_t *hipc)
{
  if (hipc != NULL)
  {
    (void) memset(hipc->RxBuffer.data, 0,  sizeof(uint8_t) * IPC_RXBUF_STREAM_MAXSIZE);
    hipc->RxBuffer.index_read = 0U;
    hipc->RxBuffer.index_write = 0U;
    hipc->RxBuffer.available_char = 0U;
    hipc->RxBuffer.total_rcv_count = 0U;
  }
}

/**
  * @brief  Write a char in the IPC RX FIFO in stream mode.
  * @param  hipc IPC handle.
  * @param  rxChar character to write.
  * @retval none.
  */
void IPC_RXFIFO_writeStream(IPC_Handle_t *hipc, uint8_t rxChar)
{
  if (hipc != NULL)
  {
    hipc->RxBuffer.data[hipc->RxBuffer.index_write] = rxChar;

    /* rearm RX Interrupt */
    RXFIFO_rearm_RX_IT(hipc);

    hipc->RxBuffer.index_write++;
    hipc->RxBuffer.total_rcv_count++;

    if (hipc->RxBuffer.index_write >= IPC_RXBUF_STREAM_MAXSIZE)
    {
      hipc->RxBuffer.index_write = 0;
    }
    hipc->RxBuffer.available_char++;

    (* hipc->RxClientCallback)((void *)hipc);
  }
}
#endif /* IPC_USE_STREAM_MODE */

/**
  * @brief  Get number of free bytes in the IPC RX FIFO.
  * @param  hipc IPC handle.
  * @retval Number of free bytes in the IPC RX FIFO.
  */
uint16_t IPC_RXFIFO_getFreeBytes(IPC_Handle_t *hipc)
{
  uint16_t free_bytes;

  if (hipc != NULL)
  {
    if (hipc->RxQueue.index_write > hipc->RxQueue.index_read)
    {
      free_bytes = (IPC_RXBUF_MAXSIZE - hipc->RxQueue.index_write +  hipc->RxQueue.index_read);
    }
    else
    {
      free_bytes =  hipc->RxQueue.index_read - hipc->RxQueue.index_write;
    }
  }
  else
  {
    /* error: hipc is NULL */
    free_bytes = 0U;
  }

  return (free_bytes);
}


/**
  * @brief  Decode message header of a message in IPC RX FIFO.
  * @param  hipc IPC handle.
  * @param  pHeader Ptr to the header structure.
  * @param  pos Position of the message to decode.
  * @retval none.
  */
void IPC_RXFIFO_readMsgHeader_at_pos(const IPC_Handle_t *hipc, IPC_RxHeader_t *pHeader, uint16_t pos)
{
  uint8_t header_byte1;
  uint8_t header_byte2;
  uint16_t index;

  if (hipc != NULL)
  {
    PRINT_DBG("DBG IPC_RXFIFO_readMsgHeader: index_read = %d", hipc->RxQueue.index_read)

    /* read header bytes */
    index =  pos;
    header_byte1 = hipc->RxQueue.data[index];
    index = (index + 1U) % IPC_RXBUF_MAXSIZE;
    header_byte2 = hipc->RxQueue.data[index];

    PRINT_DBG("header_byte1[0x%x] header_byte2[0x%x]", header_byte1, header_byte2)

    /* get msg complete bit */
    pHeader->complete = (IPC_RXMSG_HEADER_COMPLETE_MASK & header_byte1) >> 7;
    /* get msg size */
    pHeader->size = (((uint16_t)IPC_RXMSG_HEADER_SIZE_MASK & (uint16_t)header_byte1) << 8);
    pHeader->size = pHeader->size + header_byte2;

    PRINT_DBG("complete=%d size=%d", pHeader->complete, pHeader->size)
  }
}

#if (DBG_IPC_RX_FIFO == 1U)
/**
  * @brief  Print IPC RX FIFO content.
  * @param  hipc IPC handle.
  * @param  index Starting index in the RX FIFO.
  * @param  size Size of data to print.
  * @param  readable If equal 1, print special characters explicitly (<CR>, <LF>, <NULL>).
  * @retval none.
  */
void IPC_RXFIFO_print_data(const IPC_Handle_t *hipc, uint16_t index, uint16_t size, uint8_t readable)
{
  UNUSED(readable);
#if ((USE_TRACE_IPC == 1U) || (USE_PRINTF == 1U))
  if (hipc != NULL)
  {
    PRINT_INFO("DUMP RX QUEUE: (index=%d) (size=%d)", index, size)
    if ((index + size) > IPC_RXBUF_MAXSIZE)
    {
      /* in case buffer loops back to index 0 */
      /* print first buffer part (until end of queue) */
      PRINT_BUF((const uint8_t *)&hipc->RxQueue.data[index], (IPC_RXBUF_MAXSIZE - index))
      /* print second buffer part */
      PRINT_BUF((const uint8_t *)&hipc->RxQueue.data[0], (size - (IPC_RXBUF_MAXSIZE - index)))

      PRINT_INFO("dump same in hexa:")
      /* print first buffer part (until end of queue) */
      PRINT_BUF_HEXA((const uint8_t *)&hipc->RxQueue.data[index], (IPC_RXBUF_MAXSIZE - index))
      /* print second buffer part */
      PRINT_BUF_HEXA((const uint8_t *)&hipc->RxQueue.data[0], (size - (IPC_RXBUF_MAXSIZE - index)))
    }
    else
    {
      PRINT_BUF((const uint8_t *)&hipc->RxQueue.data[index], size)
      PRINT_INFO("dump same in hexa:")
      PRINT_BUF_HEXA((const uint8_t *)&hipc->RxQueue.data[index], size)
    }
    PRINT_INFO("\r\n")
  }
#else
  UNUSED(hipc);
  UNUSED(index);
  UNUSED(size);
#endif  /* (USE_TRACE_IPC == 1U) || (USE_PRINTF == 1U) */
}

/**
  * @brief  Dump content of IPC RX FIFO.
  * @param  hipc IPC handle.
  * @param  databuf Print buffer is equal to 1.
  * @param  queue Print queue infos is equal to 1.
  * @retval none.
  */
void dump_RX_dbg_infos(IPC_Handle_t *hipc, uint8_t databuf, uint8_t queue)
{
  uint32_t idx;

  if (hipc != NULL)
  {
    if (databuf == 1)
    {
      PRINT_BUF((const IPC_TYPE_CHAR_t *)&hipc->RxQueue.data[0], IPC_RXBUF_MAXSIZE)
      /* PRINT_BUF_HEXA((const IPC_TYPE_CHAR_t *)&hipc->RxQueue.data[0], IPC_RXBUF_MAXSIZE) */
    }

    PRINT_INFO("\r\n")

    if (queue == 1)
    {
      for (idx = 0; idx <= hipc->dbgRxQueue.queue_pos; idx++)
      {
        PRINT_INFO(" [index %d]:  start_pos=%d size=%d complete=%d",
                   idx,
                   hipc->dbgRxQueue.msg_info_queue[idx].start_pos,
                   hipc->dbgRxQueue.msg_info_queue[idx].size,
                   hipc->dbgRxQueue.msg_info_queue[idx].complete)
      }
    }
  }
}
#endif /* DBG_IPC_RX_FIFO */

/* Private function Definition -----------------------------------------------*/
/**
  * @brief  Increment IPC RX FIFO tail.
  * @param  hipc IPC handle.
  * @param  inc_size Size to increment.
  * @retval none.
  */
static void RXFIFO_incrementTail(IPC_Handle_t *hipc, uint16_t inc_size)
{
  hipc->RxQueue.index_read = (hipc->RxQueue.index_read + inc_size) % IPC_RXBUF_MAXSIZE;
}

/**
  * @brief  Increment IPC RX FIFO Head for next message Header.
  * @param  hipc IPC handle.
  * @retval none.
  */
static void RXFIFO_incrementHead(IPC_Handle_t *hipc)
{
  uint16_t free_bytes;

  hipc->RxQueue.index_write = (hipc->RxQueue.index_write + 1U) % IPC_RXBUF_MAXSIZE;
  free_bytes = IPC_RXFIFO_getFreeBytes(hipc);

#if (DBG_IPC_RX_FIFO == 1U)
  hipc->dbgRxQueue.free_bytes = free_bytes;
#endif /* DBG_IPC_RX_FIFO */

  if (free_bytes <= IPC_RXBUF_THRESHOLD)
  {
    hipc->State = IPC_STATE_PAUSED;

#if (DBG_IPC_RX_FIFO == 1U)
    hipc->dbgRxQueue.cpt_RXPause++;
#endif /* DBG_IPC_RX_FIFO */
  }
}

/**
  * @brief  Update current message Header.
  * @param  hipc IPC handle.
  * @retval none.
  */
static void RXFIFO_updateMsgHeader(IPC_Handle_t *hipc)
{
  /* update header with the size of last complete msg received */
  uint8_t header_byte1;
  uint8_t header_byte2;
  uint16_t index;

  /* set header byte 1:  complete bit + size (upper part)*/
  header_byte1 = (uint8_t)(IPC_RXMSG_HEADER_COMPLETE_MASK | ((hipc->RxQueue.current_msg_size >> 8) & 0x9FU));
  /* set header byte 2:  size (lower part)*/
  header_byte2 = (uint8_t)(hipc->RxQueue.current_msg_size & 0x00FFU);

  /* write header bytes */
  index = hipc->RxQueue.current_msg_index;
  hipc->RxQueue.data[index] = header_byte1;
  index = (index + 1U) % IPC_RXBUF_MAXSIZE;
  hipc->RxQueue.data[index] = header_byte2;

#if (DBG_IPC_RX_FIFO == 1U)
  hipc->dbgRxQueue.msg_info_queue[hipc->dbgRxQueue.queue_pos].complete = 1;
  hipc->dbgRxQueue.queue_pos = (hipc->dbgRxQueue.queue_pos + 1) % DBG_QUEUE_SIZE;
  hipc->dbgRxQueue.msg_info_queue[hipc->dbgRxQueue.queue_pos].start_pos = hipc->RxQueue.current_msg_index;
  hipc->dbgRxQueue.msg_info_queue[hipc->dbgRxQueue.queue_pos].complete = 0;
#endif /* DBG_IPC_RX_FIFO */
}

/**
  * @brief  Prepare next message Header.
  * @param  hipc IPC handle.
  * @retval none.
  */
static void RXFIFO_prepareNextMsgHeader(IPC_Handle_t *hipc)
{
  uint8_t idx;
  for (idx = 0U; idx < IPC_RXMSG_HEADER_SIZE; idx++)
  {
    /* clean data and increment head */
    hipc->RxQueue.data[hipc->RxQueue.index_write] = 0U;
    RXFIFO_incrementHead(hipc);
  }
}

static void RXFIFO_rearm_RX_IT(IPC_Handle_t *hipc)
{
#if (IPC_USE_UART == 1U)
  IPC_UART_rearm_RX_IT(hipc);
#else
  __NOP();
#endif /* IPC_USE_UART == 1U */
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

