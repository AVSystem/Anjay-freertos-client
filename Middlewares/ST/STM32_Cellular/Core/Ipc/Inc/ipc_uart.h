/**
  ******************************************************************************
  * @file    ipc_uart.h
  * @author  MCD Application Team
  * @brief   Header for ipc_uart.c module
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
IPC_Status_t IPC_UART_init(IPC_Device_t  device, UART_HandleTypeDef   *const huart);
IPC_Status_t IPC_UART_deinit(IPC_Device_t  device);
IPC_Status_t IPC_UART_open(IPC_Handle_t *const hipc,
                           IPC_Device_t  device,
                           IPC_Mode_t    mode,
                           IPC_RxCallbackTypeDef pRxClientCallback,
                           IPC_TxCallbackTypeDef pTxClientCallback,
                           IPC_ErrCallbackTypeDef pErrorClientCallback,
                           IPC_CheckEndOfMsgCallbackTypeDef pCheckEndOfMsg);
IPC_Status_t IPC_UART_close(IPC_Handle_t *const hipc);
IPC_Status_t IPC_UART_select(IPC_Handle_t *const hipc);
IPC_Status_t IPC_UART_reset(IPC_Handle_t *const hipc);
IPC_Status_t IPC_UART_abort(IPC_Handle_t *const hipc);
IPC_Handle_t *IPC_UART_get_other_channel(const IPC_Handle_t *const hipc);
IPC_Status_t IPC_UART_send(IPC_Handle_t *const hipc, uint8_t *p_TxBuffer, uint16_t bufsize);
IPC_Status_t IPC_UART_receive(IPC_Handle_t *const hipc, IPC_RxMessage_t *const p_msg);
IPC_Status_t IPC_UART_streamReceive(IPC_Handle_t *const hipc,  uint8_t *const p_buffer, int16_t *const p_len);
void IPC_UART_rearm_RX_IT(IPC_Handle_t *const hipc);

#if (DBG_IPC_RX_FIFO == 1U)
void IPC_UART_DumpRXQueue(const IPC_Handle_t *const hipc, uint8_t readable);
#endif /* DBG_IPC_RX_FIFO == 1U */

void IPC_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle);
void IPC_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle);
void IPC_UART_ErrorCallback(UART_HandleTypeDef *UartHandle);

#ifdef __cplusplus
}
#endif

#endif /* IPC_UART_H */

