/**
  ******************************************************************************
  * @file    ppposif.h
  * @author  MCD Application Team
  * @brief   Header for ppposif.c module
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

#ifndef PPPOSIF_H
#define PPPOSIF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)

/* LwIP is a Third Party so MISRAC messages linked to it are ignored */
/*cstat -MISRAC2012-* */
#include "ppp.h"
#include "netif/ppp/pppos.h"
/*cstat +MISRAC2012-* */

#include "ipc_uart.h"

#if (USE_TRACE_PPPOSIF == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_PPPOSIF(format, args...)  TRACE_PRINT(DBG_CHAN_PPPOSIF, DBL_LVL_P0, "UTILS:" format "\n\r", ## args)
#else  /* USE_PRINTF == 1U */
#include <stdio.h>
#define PRINT_PPPOSIF(format, args...)  (void)printf("" format , ## args);
#endif /* USE_PRINTF == 0U */
#else  /* USE_TRACE_PPPOSIF == 0U */
#define PRINT_PPPOSIF(...)              __NOP(); /* Nothing to do */
#endif /* USE_TRACE_PPPOSIF == 1U */

#define PPPOSIF_CAUSE_POWER_OFF       0U
#define PPPOSIF_CAUSE_CONNECTION_LOST 1U

/* Exported types ------------------------------------------------------------*/
typedef uint16_t ppposif_status_t ;

#define  PPPOSIF_OK      (ppposif_status_t)(0)
#define  PPPOSIF_ERROR   (ppposif_status_t)(1)

/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
/**
  * @brief  read data from serial and send it to PPP
  * @param  ppp_netif      (in) netif reference
  * @param  p_ppp_pcb      (in) pcb reference
  * @param  pDevice        (in) serial device id
  * @retval none
  */
extern void  ppposif_input(const struct netif *ppp_netif, ppp_pcb  *p_ppp_pcb, IPC_Device_t pDevice);

/**
  * @brief      Closing PPP connection
  * @note       Initiate the end of the PPP session, without carrier lost signal
  *             (nocarrier=0), meaning a clean shutdown of PPP protocols.
  *             You can call this function at anytime.
  * @param  ppp_pcb_struct            (in) pcb reference
  * @retval ppposif_status_t    return status
  */
extern ppposif_status_t ppposif_close(ppp_pcb *ppp_pcb_struct);


/**
  * @brief  PPP status callback is called on PPP status change (up, down, ...) from lwIP
  * @param  pcb        pcb reference
  * @param  err_code   error
  * @param  ctx        user context
  * @retval ppposif_status_cb    return status
  */

extern void  ppposif_status_cb(ppp_pcb *pcb, int32_t err_code, void *ctx);

/**
  * @brief  PPPoS serial output callback
  * @param  pcb            (in) PPP control block
  * @param  data           (in) data, buffer to write to serial port
  * @param  len            (in) length of the data buffer
  * @param  ctx            (in) user context : contains serial device id
  * @retval number of char sent if write succeed - 0 otherwise
  */
extern u32_t ppposif_output_cb(ppp_pcb *pcb, u8_t *data, u32_t len, void *ctx);

/**
  * @brief  sys_now
  * @param  None
  * @retval GetTick
  */
#else
extern uint32_t sys_now(void);
extern uint32_t sys_jiffies(void);
#endif  /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */


#ifdef __cplusplus
}
#endif

#endif /* PPPOSIF_H */
