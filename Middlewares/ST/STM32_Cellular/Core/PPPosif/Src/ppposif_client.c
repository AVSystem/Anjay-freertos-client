/**
  ******************************************************************************
  * @file    ppposif_client.c
  * @author  MCD Application Team
  * @brief   This file contains pppos client adatation layer
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

#include <stdbool.h>
#include "ppposif_client.h"
#include "plf_config.h"

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
/* LwIP is a Third Party so MISRAC messages linked to it are ignored */
/*cstat -MISRAC2012-* */
#include "ppp.h"
/*cstat +MISRAC2012-* */
#include "ppposif.h"
#include "ipc_common.h"
#include "ppposif_ipc.h"
#include "rtosal.h"

#include "main.h"
#include "error_handler.h"
#include "trace_interface.h"
#include "dc_common.h"
#include "cellular_service_datacache.h"

/* Private defines -----------------------------------------------------------*/
#define IPC_DEVICE IPC_DEVICE_0
#define PPPOSIF_CONFIG_TIMEOUT_VALUE 15000U
#define PPPOSIF_CONFIG_FAIL_MAX 3

/* Private typedef -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

static struct netif  gnetif_ppp_client;
static ppp_pcb      *ppp_pcb_client;
static osTimerId     ppposif_config_timeout_timer_handle;
static osSemaphoreId sem_ppp_init_client = NULL;
static uint32_t ppposif_create_done;


/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void ppp_notify_phase_client_cb(ppp_pcb *pcb, u8_t phase, void *ctx);
static void ppposif_client_running(ppp_pcb *pcb);
static void ppposif_client_network(void);
static void ppposif_client_dead(void);
static void ppposif_reconf(void);
static void ppposif_client_thread(void *argument);
static void ppposif_config_timeout_timer_callback(void const *argument);

/* Functions Definition ------------------------------------------------------*/

/* Private Functions Definition ------------------------------------------------------*/

/*
 Notify phase callback (PPP_NOTIFY_PHASE)
==========================================

Notify phase callback, enabled using the PPP_NOTIFY_PHASE config option, let
you configure a callback that is called on each PPP internal state change.
This is different from the status callback which only warns you about
up(running) and down(dead) events.

Notify phase callback can be used, for example, to set a LED pattern depending
on the current phase of the PPP session. Here is a callback example which
tries to mimic what we usually see on xDSL modems while they are negotiating
the link, which should be self-explanatory:
*/

static void ppp_notify_phase_client_cb(ppp_pcb *pcb, u8_t phase, void *ctx)
{
  UNUSED(ctx);
  (void)rtosalDelay(500U); /* hack, add a delay to avoid race condition with Modem which sends an LCP Request earlier.
                   To be improved by syncing the input reading and PPP state machine */
  switch (phase)
  {

    /* Session is down (either permanently or briefly) */
    case PPP_PHASE_DEAD:
      PRINT_PPPOSIF("client ppp_notify_phase_cb: PPP_PHASE_DEAD")
      ppposif_client_dead();
      break;

    /* We are between two sessions */
    case PPP_PHASE_HOLDOFF:
      PRINT_PPPOSIF("client ppp_notify_phase_cb: PPP_PHASE_HOLDOFF")
      ppposif_reconf();

      break;

    /* Session just started */
    case PPP_PHASE_INITIALIZE:
      PRINT_PPPOSIF("client ppp_notify_phase_cb: PPP_PHASE_INITIALIZE")
      break;

    /* Session is running */
    case PPP_PHASE_RUNNING:
      PRINT_PPPOSIF("client ppp_notify_phase_cb: PPP_PHASE_RUNNING")
      ppposif_client_running(pcb);
      break;

    /* Configure network layer protocol */
    case PPP_PHASE_NETWORK:
      PRINT_PPPOSIF("client ppp_notify_phase_cb: PPP_PHASE_NETWORK")
      ppposif_client_network();
      break;

    default:
      /* Unmanaged phases */
      PRINT_PPPOSIF("client ppp_notify_phase_cb: %d", phase)
      break;
  }
}

/* ppposif thread */
static void ppposif_client_thread(void *argument)
{
  UNUSED(argument);
  (void)rtosalSemaphoreAcquire(sem_ppp_init_client, RTOSAL_WAIT_FOREVER);
  while (true)
  {
    ppposif_input(&gnetif_ppp_client, ppp_pcb_client, IPC_DEVICE);
  }
}


static void ppposif_client_running(ppp_pcb *pcb)
{
  dc_cellular_info_t cellular_info;
  struct netif *pppif;
  pppif = ppp_netif((pcb));

  (void)rtosalTimerStop(ppposif_config_timeout_timer_handle);


  (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&cellular_info, sizeof(cellular_info));
  cellular_info.rt_state_ppp = DC_SERVICE_ON;
  cellular_info.ip_addr = pppif->ip_addr;

  (void)dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cellular_info, sizeof(cellular_info));
}

static void ppposif_client_network(void)
{
  dc_nifman_info_t nifman_info;
  /* Stop config timout */
  (void)rtosalTimerStop(ppposif_config_timeout_timer_handle);
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
  /* update of DC_CELLULAR_INFO */
  PRINT_PPPOSIF("ppposif_client_network \r\n")
  if (nifman_info.rt_state == DC_SERVICE_ON)
  {
    PRINT_PPPOSIF("ppposif_client_network: ppp_close \r\n")
    (void)ppp_close(ppp_pcb_client, 0U);
  }
}


static void ppposif_client_dead(void)
{
  dc_cellular_info_t cellular_info;
  dc_nifman_info_t nifman_info;

  PRINT_PPPOSIF("ppposif_client_dead\r\n")
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));

  /* Consider PPP closing only if data connection was already established */
  if ((nifman_info.rt_state == DC_SERVICE_ON) ||
      (nifman_info.rt_state == DC_SERVICE_STARTING))
  {
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&cellular_info, sizeof(cellular_info));
    PRINT_PPPOSIF("ppposif_client_dead: DC_SERVICE_OFF\r\n")
    cellular_info.rt_state_ppp = DC_SERVICE_OFF;
    (void)dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cellular_info, sizeof(cellular_info));
  }
}

static void ppposif_reconf(void)
{
  dc_cellular_info_t cellular_info;
  (void)ppp_close(ppp_pcb_client, 0U);
  PRINT_PPPOSIF("ppposif_config_timeout_timer_callback")
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&cellular_info, sizeof(cellular_info));
  cellular_info.rt_state_ppp = DC_SERVICE_OFF;
  (void)dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cellular_info, sizeof(cellular_info));
}

static void ppposif_config_timeout_timer_callback(void const *argument)
{
  UNUSED(argument);
  ppposif_reconf();
}

/*  Exported functions Definition ------------------------------------------------------*/

/**
  * @brief  component init
  * @param  none
  * @retval ppposif_status_t    return status
  */
ppposif_status_t ppposif_client_init(void)
{
  /* Reset ppp creation status */
  ppposif_create_done = 0U;
  ppposif_ipc_init(IPC_DEVICE);
  return PPPOSIF_OK;
}

/**
  * @brief  component start
  * @param  none
  * @retval ppposif_status_t    return status
  */
ppposif_status_t ppposif_client_start(void)
{
  static osThreadId pppClientThreadId = NULL;

  ppposif_status_t ret = PPPOSIF_OK;

  PRINT_PPPOSIF("ppposif_client_start")

  sem_ppp_init_client = rtosalSemaphoreNew((const rtosal_char_t *)"SEM_PPP_CLIENT_INIT", (uint16_t) 1U);
  (void)rtosalSemaphoreAcquire(sem_ppp_init_client, RTOSAL_WAIT_FOREVER);

  ppposif_config_timeout_timer_handle = rtosalTimerNew((const rtosal_char_t *)"PPPOSIF_CONFIG_TIMEOUT_timer",
                                                       (os_ptimer)ppposif_config_timeout_timer_callback,
                                                       osTimerOnce,
                                                       NULL);

  pppClientThreadId = rtosalThreadNew((const rtosal_char_t *)"PPPosifClt",
                                      (os_pthread) ppposif_client_thread,
                                      PPPOSIF_CLIENT_THREAD_PRIO,
                                      (uint32_t)PPPOSIF_CLIENT_THREAD_STACK_SIZE,
                                      NULL);
  if (pppClientThreadId == NULL)
  {
    ERROR_Handler(DBG_CHAN_PPPOSIF, 1, ERROR_FATAL);
  }

  return ret;
}

/**
  * @brief  Create a new PPPoS client interface
  * @param  none
  * @retval ppposif_status_t    return status
  */
ppposif_status_t ppposif_client_config(void)
{
  err_t ppp_err;
  ppposif_status_t ret = PPPOSIF_OK;

  PRINT_PPPOSIF("ppposif_client_config")
  ppposif_ipc_select(IPC_DEVICE);

  if (ppposif_create_done == 0U)
  {
    /* ppp not yet created */
    ppp_pcb_client = pppos_create(&gnetif_ppp_client, ppposif_output_cb, ppposif_status_cb, (void *)IPC_DEVICE);
    if (ppp_pcb_client == NULL)
    {
      ret =  PPPOSIF_ERROR;
      ERROR_Handler(DBG_CHAN_PPPOSIF, 3, ERROR_FATAL);
    }
    else
    {
      netif_set_default(&gnetif_ppp_client);
      /* ppp is now created */
      ppposif_create_done = 1U;
    }
  }

  if (ret ==  PPPOSIF_OK)
  {
    ppp_set_default((ppp_pcb_client));
    ppp_set_notify_phase_callback(ppp_pcb_client,  ppp_notify_phase_client_cb);
    ppp_set_usepeerdns((ppp_pcb_client), (1));

    /*  ppp_set_auth(ppp_pcb_client, PPPAUTHTYPE_PAP, "USER", "PASS") */

    (void)rtosalTimerStart(ppposif_config_timeout_timer_handle, PPPOSIF_CONFIG_TIMEOUT_VALUE);
    ppp_err = ppp_connect(ppp_pcb_client, 0U);
    (void)rtosalSemaphoreRelease(sem_ppp_init_client);
    if (ppp_err != (err_t)ERR_OK)
    {
      ret = PPPOSIF_ERROR;
    }
  }
  return ret;
}


/**
  * @brief  close a PPPoS client interface
  * @param  none
  * @retval ppposif_status_t    return status
  */
ppposif_status_t ppposif_client_close(uint8_t cause)
{
  dc_cellular_info_t cellular_info;
  PRINT_PPPOSIF("ppposif_client_close")

  (void)rtosalTimerStop(ppposif_config_timeout_timer_handle);

  if (cause == PPPOSIF_CAUSE_POWER_OFF)
  {
    PRINT_PPPOSIF("ppposif_client_close : Closing PPP for POWER OFF")
    (void) ppposif_close(ppp_pcb_client);
    (void) ppposif_ipc_close(IPC_DEVICE);
    /* reset ppp creation status */
    ppposif_create_done = 0U;
  }
  else
  {
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&cellular_info, sizeof(cellular_info));
    cellular_info.rt_state_ppp = DC_SERVICE_FAIL;
    (void)dc_com_write(&dc_com_db, DC_CELLULAR_INFO, (void *)&cellular_info, sizeof(cellular_info));
  }



  return PPPOSIF_OK;
}


#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_LWIP */
