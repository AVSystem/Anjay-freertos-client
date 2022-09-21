/**
  ******************************************************************************
  * @file    ppposif_ipc.c
  * @author  MCD Application Team
  * @brief   Interface between ppposif and IPC
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
#include "ppposif_ipc.h"
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)

#include "rtosal.h"
#include "ipc_uart.h"
#include "main.h"
#include "error_handler.h"
#include "plf_config.h"

/* Private defines -----------------------------------------------------------*/
#define SND_MAX 800


/* Private typedef -----------------------------------------------------------*/

typedef struct
{
  IPC_Handle_t *ipcHandle;
  osSemaphoreId     rcvSemaphore;
  osSemaphoreId     sndSemaphore;
  __IO uint32_t TransmitChar;
  __IO uint32_t rcvSemaphoreFlag;
  __IO uint32_t sndSemaphoreFlag;
  __IO uint32_t TransmitOnGoing;
  u8_t              snd_buff[SND_MAX];
} ppposif_ipc_ctx_t;

/* Private macros ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static ppposif_ipc_ctx_t ppposif_ipc_ctx[2];
static IPC_Handle_t   IPC_Handle[2] ;

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void IPC_MessageSentCallback(IPC_Handle_t *ipcHandle);
static void IPC_MessageReceivedCallback(IPC_Handle_t *ipcHandle);


/* Functions Definition ------------------------------------------------------*/
/* Private function Definition -----------------------------------------------*/
/**
  * @brief  Tx Transfer completed callback
  * @param  UartHandle: UART handle.
  * @note   This example shows a simple way to report end of IT Tx transfer, and
  *         you can add your own implementation.
  * @retval None
  */
static void IPC_MessageSentCallback(IPC_Handle_t *ipcHandle)
{
  ppposif_ipc_ctx[ipcHandle->Device_ID].TransmitOnGoing = 0U;
  (void)rtosalSemaphoreRelease(ppposif_ipc_ctx[ipcHandle->Device_ID].sndSemaphore);
}

/**
  * @brief  Rx  IPC completed callback
  * @param  clienthandle: IPC handle
  * @retval None
  */
static void IPC_MessageReceivedCallback(IPC_Handle_t *ipcHandle)
{
  /* Warning ! this function is called under IT */
  (void)rtosalSemaphoreRelease(ppposif_ipc_ctx[ipcHandle->Device_ID].rcvSemaphore);
  ppposif_ipc_ctx[ipcHandle->Device_ID].rcvSemaphoreFlag = 1U;
}

/**
  * @brief  component init
  * @param  pDevice: device to init.
  * @retval return code
  */

/* Exported function Definition -----------------------------------------------*/
void ppposif_ipc_init(IPC_Device_t pDevice)
{
  ppposif_ipc_ctx[pDevice].ipcHandle         = &IPC_Handle[pDevice];
  ppposif_ipc_ctx[pDevice].TransmitChar      = 0U;
  ppposif_ipc_ctx[pDevice].rcvSemaphoreFlag  = 0U;
  ppposif_ipc_ctx[pDevice].sndSemaphoreFlag  = 0U;
  ppposif_ipc_ctx[pDevice].TransmitOnGoing   = 0U;

  ppposif_ipc_ctx[pDevice].rcvSemaphore = rtosalSemaphoreNew((const rtosal_char_t *) "SEM_UART_RCV",
                                                             (uint16_t) 10000U);
  if (ppposif_ipc_ctx[pDevice].rcvSemaphore == NULL)
  {
    ERROR_Handler(DBG_CHAN_PPPOSIF, 11, ERROR_FATAL);
  }

  ppposif_ipc_ctx[pDevice].sndSemaphore = rtosalSemaphoreNew((const rtosal_char_t *) "SEM_UART_SND",
                                                             (uint16_t) 1U);
  (void)rtosalSemaphoreAcquire(ppposif_ipc_ctx[pDevice].sndSemaphore, RTOSAL_WAIT_FOREVER);

  (void)IPC_open(&IPC_Handle[pDevice],  pDevice, IPC_MODE_UART_STREAM, IPC_MessageReceivedCallback,
                 IPC_MessageSentCallback, NULL, NULL);
}

/**
  * @brief  data IPC channel select
  * @param  pDevice: device to init.
  * @retval return code
  */

void ppposif_ipc_select(IPC_Device_t pDevice)
{
  (void)IPC_select(&IPC_Handle[pDevice]);
}

/**
  * @brief  component de init
  * @param  pDevice: device to de init.
  * @retval return code
  */

void ppposif_ipc_deinit(IPC_Device_t pDevice)
{
  UNUSED(pDevice);
}

/**
  * @brief  Rcv data
  * @param  pDevice: serial device.
  * @param  data: buffer data to read.
  * @param  len: data size to read.
  * @retval data rcv byte number
  */

int16_t ppposif_ipc_read(IPC_Device_t pDevice, u8_t *buff, int16_t size)
{

  ppposif_ipc_ctx[pDevice].rcvSemaphoreFlag = 2U;
  (void)rtosalSemaphoreAcquire(ppposif_ipc_ctx[pDevice].rcvSemaphore, RTOSAL_WAIT_FOREVER);
  ppposif_ipc_ctx[pDevice].rcvSemaphoreFlag = 0U;
  __disable_irq();
  (void)IPC_streamReceive(ppposif_ipc_ctx[pDevice].ipcHandle, buff, &size);
  __enable_irq();

  return size;
}

/**
  * @brief  Tx Send data
  * @param  pDevice: device .
  * @param  data: buffer data to send.
  * @param  len: data size to send.
  * @retval data sent byte number
  */
int16_t ppposif_ipc_write(IPC_Device_t pDevice, u8_t *data, int16_t len)
{
  int16_t temp_len;
  temp_len = len;
  IPC_Status_t status;
  ppposif_ipc_ctx[pDevice].TransmitOnGoing = 1U;

  /*   for(int i=0; i<temp_len; i++) ppposif_ipc_ctx[pDevice].snd_buff[i] = data[i];*/

  status = IPC_send(ppposif_ipc_ctx[pDevice].ipcHandle, (uint8_t *) data, (uint16_t)temp_len);
  if (status != IPC_OK)
  {
    temp_len = 0;
  }
  else
  {
    ppposif_ipc_ctx[pDevice].sndSemaphoreFlag = 1U;
    (void)rtosalSemaphoreAcquire(ppposif_ipc_ctx[pDevice].sndSemaphore, RTOSAL_WAIT_FOREVER);

    ppposif_ipc_ctx[pDevice].TransmitChar += (uint16_t)temp_len;
    ppposif_ipc_ctx[pDevice].sndSemaphoreFlag = 0U;
  }
  return temp_len;
}
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */



