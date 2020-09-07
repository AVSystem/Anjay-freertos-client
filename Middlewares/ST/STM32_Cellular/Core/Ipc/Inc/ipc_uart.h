/**
  ******************************************************************************
  * @file    ipc_uart.h
  * @author  MCD Application Team
  * @brief   Header for ipc_uart.c module
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef IPC_UART_H
#define IPC_UART_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "ipc_common.h"

/* Exported constants --------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
IPC_Status_t IPC_UART_init(IPC_Device_t  device, UART_HandleTypeDef  *huart);
IPC_Status_t IPC_UART_deinit(IPC_Device_t  device);
IPC_Status_t IPC_UART_open(IPC_Handle_t *hipc,
                           IPC_Device_t  device,
                           IPC_Mode_t    mode,
                           IPC_RxCallbackTypeDef pRxClientCallback,
                           IPC_TxCallbackTypeDef pTxClientCallback,
                           IPC_CheckEndOfMsgCallbackTypeDef pCheckEndOfMsg);
IPC_Status_t IPC_UART_close(IPC_Handle_t *hipc);
IPC_Status_t IPC_UART_select(IPC_Handle_t *hipc);
IPC_Status_t IPC_UART_reset(IPC_Handle_t *hipc);
IPC_Status_t IPC_UART_abort(IPC_Handle_t *hipc);
IPC_Handle_t *IPC_UART_get_other_channel(const IPC_Handle_t *hipc);
IPC_Status_t IPC_UART_send(IPC_Handle_t *hipc, uint8_t *p_TxBuffer, uint16_t bufsize);
IPC_Status_t IPC_UART_receive(IPC_Handle_t *hipc, IPC_RxMessage_t *p_msg);
IPC_Status_t IPC_UART_streamReceive(IPC_Handle_t *hipc,  uint8_t *p_buffer, int16_t *p_len);
void IPC_UART_rearm_RX_IT(IPC_Handle_t *hipc);

#if (DBG_IPC_RX_FIFO == 1U)
void IPC_UART_DumpRXQueue(const IPC_Handle_t *hipc, uint8_t readable);
#endif /* DBG_IPC_RX_FIFO */

void IPC_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle);
void IPC_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle);
void IPC_UART_ErrorCallback(UART_HandleTypeDef *UartHandle);

#ifdef __cplusplus
}
#endif

#endif /* IPC_UART_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

