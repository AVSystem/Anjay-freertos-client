/**
  ******************************************************************************
  * @file    ipc_rxfifo.h
  * @author  MCD Application Team
  * @brief   Header for ipc_rxfifo.c module
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
#ifndef IPC_RXFIFO_H
#define IPC_RXFIFO_H

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
void IPC_RXFIFO_init(IPC_Handle_t *const hipc);
void IPC_RXFIFO_writeCharacter(IPC_Handle_t *const hipc, uint8_t rxChar);
int16_t IPC_RXFIFO_read(IPC_Handle_t *const hipc, IPC_RxMessage_t *pMsg);
#if (IPC_USE_STREAM_MODE == 1U)
void IPC_RXFIFO_stream_init(IPC_Handle_t *const hipc);
void IPC_RXFIFO_writeStream(IPC_Handle_t *const hipc, uint8_t rxChar);
#endif /* IPC_USE_STREAM_MODE == 1U */
uint16_t IPC_RXFIFO_getFreeBytes(IPC_Handle_t *const hipc);
void IPC_RXFIFO_readMsgHeader_at_pos(const IPC_Handle_t *const hipc, IPC_RxHeader_t *pHeader, uint16_t pos);

#if (DBG_IPC_RX_FIFO == 1U)
/* Debug functions */
void IPC_RXFIFO_print_data(const IPC_Handle_t *const hipc, uint16_t index, uint16_t size, uint8_t readable);
void dump_RX_dbg_infos(IPC_Handle_t *const hipc, uint8_t databuf, uint8_t queue);
#endif /* DBG_IPC_RX_FIFO == 1U */

#ifdef __cplusplus
}
#endif

#endif /* IPC_RXFIFO_H */

