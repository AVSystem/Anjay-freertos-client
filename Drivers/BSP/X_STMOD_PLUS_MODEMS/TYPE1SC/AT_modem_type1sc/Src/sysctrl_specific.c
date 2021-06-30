/**
  ******************************************************************************
  * @file    sysctrl_specific.c
  * @author  MCD Application Team
  * @brief   This file provides all the specific code for System control of
  *          MURATA-TYPE1SC-EVK module (ALT1250 modem: LTE-cat-M1)
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "sysctrl.h"
#include "sysctrl_specific.h"
#include "ipc_common.h"
#include "plf_config.h"

/* Private typedef -----------------------------------------------------------*/
#define DEBUG_LOW_POWER (0)


/* Private macros ------------------------------------------------------------*/
#if (USE_TRACE_SYSCTRL == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_FORCE(format, args...) TRACE_PRINT_FORCE(DBG_CHAN_ATCMD, DBL_LVL_P0, format "\n\r", ## args)
#define PRINT_INFO(format, args...) TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P0, "SysCtrl_TYPE1SC:" format "\n\r", ## args)
#define PRINT_DBG(format, args...) TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P1, "SysCtrl_TYPE1SC:" format "\n\r", ## args)
#define PRINT_ERR(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_ERR,\
                                                "SysCtrl_TYPE1SC ERROR:" format "\n\r", ## args)
#else
#include <stdio.h>
#define PRINT_FORCE(format, args...) (void) printf(format "\n\r", ## args);
#define PRINT_INFO(format, args...)  (void) printf("SysCtrl_TYPE1SC:" format "\n\r", ## args);
#define PRINT_DBG(format, args...)  (void) printf("SysCtrl_TYPE1SC:" format "\n\r", ## args);
#define PRINT_ERR(format, args...)   (void) printf("SysCtrl_TYPE1SC ERROR:" format "\n\r", ## args);
#endif /* USE_PRINTF */
#else
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_FORCE(format, args...) TRACE_PRINT_FORCE(DBG_CHAN_ATCMD, DBL_LVL_P0, format "\n\r", ## args)
#else
#include <stdio.h>
#define PRINT_FORCE(format, args...) (void) printf(format "\n\r", ## args);
#endif /* USE_PRINTF */
#define PRINT_INFO(...)   __NOP(); /* Nothing to do */
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(...)   __NOP(); /* Nothing to do */
#endif /* USE_TRACE_SYSCTRL */

/* Private defines -----------------------------------------------------------*/
#define TYPE1SC_BOOT_TIME (4000U) /* Type 1 SC has uboot timer fixed to 4s, it could be modified later */

/* Private variables ---------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static sysctrl_status_t TYPE1SC_setup(void);
static void TYPE1SC_LP_disable_modem_uart(void);
static void enable_RING_wait_for_falling(void); /* waiting event to enter in sleep (before complete) */
static void enable_RING_wait_for_rising(void);  /* waiting event in sleep mode = modem wake up */
static void disable_RING(void);
static sysctrl_status_t HIFC_A_host_resume(IPC_Handle_t *ipc_handle);
static sysctrl_status_t HIFC_A_modem_resume(IPC_Handle_t *ipc_handle);
sysctrl_status_t enable_UART(IPC_Handle_t *ipc_handle, SysCtrl_TYPE1SC_HwFlowCtrl_t hwFC_status);

/* Functions Definition ------------------------------------------------------*/
sysctrl_status_t SysCtrl_TYPE1SC_getDeviceDescriptor(sysctrl_device_type_t type, sysctrl_info_t *p_devices_list)
{
  sysctrl_status_t retval;

  if (p_devices_list == NULL)
  {
    retval = SCSTATUS_ERROR;
  }
  else
  {
    /* check type */
    if (type == DEVTYPE_MODEM_CELLULAR)
    {
#if defined(USE_MODEM_TYPE1SC)
      p_devices_list->type          = DEVTYPE_MODEM_CELLULAR;
      p_devices_list->ipc_device    = USER_DEFINED_IPC_DEVICE_MODEM;
      p_devices_list->ipc_interface = IPC_INTERFACE_UART;

      (void) IPC_init(p_devices_list->ipc_device, p_devices_list->ipc_interface, &MODEM_UART_HANDLE);
      retval = SCSTATUS_OK;
#else
      retval = SCSTATUS_ERROR;
#endif /* USE_MODEM_TYPE1SC */
    }
    else
    {
      retval = SCSTATUS_ERROR;
    }
  }

  if (retval != SCSTATUS_ERROR)
  {
    (void) TYPE1SC_setup();
  }

  return (retval);
}

sysctrl_status_t SysCtrl_TYPE1SC_open_channel(sysctrl_device_type_t type)
{
  UNUSED(type);
  sysctrl_status_t retval;

  PRINT_DBG("enter SysCtrl_TYPE1SC_open_channel")
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Re-configure HOST_RX pin to monitor it */
  GPIO_InitStruct.Pin = MODEM_RX_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(MODEM_RX_GPIO_PORT, &GPIO_InitStruct);

#if (DEBUG_LOW_POWER == 1)
  PRINT_INFO("set MODEM DTR pin to HIGH")
#endif /* (DEBUG_LOW_POWER == 1) */
  /* set DTR to HIGH */
  HAL_GPIO_WritePin(MODEM_DTR_GPIO_PORT, MODEM_DTR_PIN, GPIO_PIN_SET);

#if (DEBUG_LOW_POWER == 1)
  PRINT_INFO(">>> Waiting for HOST_RX pin set to HIGH by modem")
#endif /* (DEBUG_LOW_POWER == 1) */
  /* wait for HOST_RX set to HIGH by modem before to reconfigure UART */
  uint32_t count = 0U;
  uint32_t count_delay = 10U;
  uint32_t count_max = 500U;
  while ((HAL_GPIO_ReadPin(MODEM_RX_GPIO_PORT, MODEM_RX_PIN) == GPIO_PIN_RESET) &&
         (count < count_max))
  {
    SysCtrl_delay(count_delay);
    count++;
  }

#if (DEBUG_LOW_POWER == 1)
  if (count == count_max)
  {
    PRINT_INFO(">>> error, HOST_RX HIGH expected !!! (timeout=%ld)", count * count_delay)
  }
  else
  {
    PRINT_INFO(">>> HOST_RX HIGH after %ld ms", count * count_delay)
  }
#endif /* (DEBUG_LOW_POWER == 1) */

  /* enable UART */
  retval = enable_UART(NULL, SYSCTRL_HW_FLOW_CONTROL_USER_SETTING);

  return (retval);
}

sysctrl_status_t SysCtrl_TYPE1SC_close_channel(sysctrl_device_type_t type)
{
  UNUSED(type);
  sysctrl_status_t retval = SCSTATUS_OK;
  PRINT_DBG("SysCtrl_TYPE1SC_close_channel")

  /* mode HIFC A
  * - set HOST_TO_MODEM (=DTR) to low
  * - wait that MODEM_TO_HOST (=RING) switch to low
  * - disable UART (then TX and RX should switch to low)
  */

  /* Disable the UART IRQn */
  HAL_NVIC_DisableIRQ(MODEM_UART_IRQN);

#if (DEBUG_LOW_POWER == 1)
  PRINT_INFO("set MODEM HOST_TO_MODEM (=DTR) to LOW")
#endif /* (DEBUG_LOW_POWER == 1) */

  /* set HOST_TO_MODEM (=DTR) to LOW */
  HAL_GPIO_WritePin(MODEM_DTR_GPIO_PORT, MODEM_DTR_PIN, GPIO_PIN_RESET);

#if (DEBUG_LOW_POWER == 1)
  PRINT_INFO(">>> Waiting for RING state set to LOW by modem")
#endif /* (DEBUG_LOW_POWER == 1) */
  uint32_t count = 0U;
  uint32_t count_delay = 10U;
  uint32_t count_max = 500U;
  while ((HAL_GPIO_ReadPin(MODEM_RING_GPIO_PORT, MODEM_RING_PIN) == GPIO_PIN_SET) &&
         (count < count_max))
  {
    SysCtrl_delay(count_delay);
    count++;
  }

#if (DEBUG_LOW_POWER == 1)
  if (count == count_max)
  {
    PRINT_INFO(">>> error, RING LOW expected !!! (timeout=%ld)", count * count_delay)
  }
  else
  {
    PRINT_INFO(">>> RING LOW after %ld ms", count * count_delay)
  }
#endif /* (DEBUG_LOW_POWER == 1) */

  /* Deinit UART */
  if (HAL_UART_DeInit(&MODEM_UART_HANDLE) != HAL_OK)
  {
    PRINT_ERR("HAL_UART_DeInit error")
    retval = SCSTATUS_ERROR;
  }
  else
  {
    /* now, disable UART interface with modem */
    TYPE1SC_LP_disable_modem_uart();
    SysCtrl_delay(150U);

    PRINT_INFO("modem channel closed")
  }

  return (retval);
}

sysctrl_status_t SysCtrl_TYPE1SC_power_on(sysctrl_device_type_t type)
{
  UNUSED(type);
  sysctrl_status_t retval = SCSTATUS_OK;

  /* Power OFF */
  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_RESET);
  SysCtrl_delay(150U);

  PRINT_INFO("MODEM POWER ON")
  /* Power ON */
  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_SET);
  SysCtrl_delay(150U);

  /* enable RING pin (normal mode) */
  enable_RING_wait_for_falling();

  PRINT_DBG("MODEM POWER ON done")
  return (retval);
}

sysctrl_status_t SysCtrl_TYPE1SC_power_off(sysctrl_device_type_t type)
{
  UNUSED(type);
  sysctrl_status_t retval = SCSTATUS_OK;

  PRINT_INFO("MODEM POWER OFF")
  /* Power OFF */
  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_RESET);
  SysCtrl_delay(150U);

  /* disable RING pin */
  disable_RING();

  PRINT_DBG("MODEM POWER OFF done")
  return (retval);
}

sysctrl_status_t SysCtrl_TYPE1SC_reset(sysctrl_device_type_t type)
{
  UNUSED(type);
  sysctrl_status_t retval = SCSTATUS_OK;

  /* Reference: TYPE1SC */
  PRINT_INFO("!!! Modem hardware reset triggered !!!")

  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_RESET);
  SysCtrl_delay(150U);
  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_SET);
  SysCtrl_delay(TYPE1SC_BOOT_TIME);

  return (retval);
}

sysctrl_status_t SysCtrl_TYPE1SC_sim_select(sysctrl_device_type_t type, sysctrl_sim_slot_t sim_slot)
{
  UNUSED(type);
  UNUSED(sim_slot);

  sysctrl_status_t retval = SCSTATUS_OK;
  /* SIM slot not selected via gpio for the moment */

  return (retval);
}

sysctrl_status_t SysCtrl_TYPE1SC_request_suspend_channel(IPC_Handle_t *ipc_handle, sysctrl_device_type_t type)
{
  UNUSED(ipc_handle);
  UNUSED(type);
  sysctrl_status_t retval = SCSTATUS_OK;
  PRINT_DBG("enter modem channel suspend request")

  /* mode HIFC A
  * - set HOST_TO_MODEM (=DTR) to LOW
  * - wait for MODEM_TO_HOST (=RING) switched to LOW (ie wait for COMPLETE)
  * - disable UART (then TX and RX should switched to LOW after complete)
  */

  /*
   *  we can receive up to 16 bytes from the modem before
   *  to receive sleep conf (ie RING from High to Low)
   */

  PRINT_INFO(">>> Request modem to enter Low Power: Set HOST_TO_MODEM (=DTR) to LOW")
  HAL_GPIO_WritePin(MODEM_DTR_GPIO_PORT, MODEM_DTR_PIN, GPIO_PIN_RESET);

  return (retval);
}

sysctrl_status_t SysCtrl_TYPE1SC_complete_suspend_channel(IPC_Handle_t *ipc_handle, sysctrl_device_type_t type)
{
  UNUSED(ipc_handle);
  UNUSED(type);
  sysctrl_status_t retval = SCSTATUS_OK;
  PRINT_DBG("enter modem channel suspend complete")

  /* mode HIFC A
  * - disable UART
  * - reconfigure UART to GPIO
  */

  /* Disable the UART IRQn */
  HAL_NVIC_DisableIRQ(MODEM_UART_IRQN);

  /* Deinit UART*/
  if (HAL_UART_DeInit(&MODEM_UART_HANDLE) != HAL_OK)
  {
    PRINT_ERR("HAL_UART_DeInit error")
    retval = SCSTATUS_ERROR;
  }
  else
  {
    PRINT_DBG("modem channel closed")
  }

  /* reconfigure UART to GPIOs */
  TYPE1SC_LP_disable_modem_uart();

  /* reconfigure RING to wait for wakeup envent */
  enable_RING_wait_for_rising();

  return (retval);
}

sysctrl_status_t SysCtrl_TYPE1SC_resume_channel(IPC_Handle_t *ipc_handle,
                                                sysctrl_device_type_t type,
                                                uint8_t modem_originated)
{
  UNUSED(type);
  sysctrl_status_t retval;
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  PRINT_DBG("enter SysCtrl_TYPE1SC_resume_channel")

  /* Init GPIOs - common parameters */
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;

  /* Re-configure HOST_RX (=MODEM_RX) pin to monitor it */
  GPIO_InitStruct.Pin = MODEM_RX_PIN;
  HAL_GPIO_Init(MODEM_RX_GPIO_PORT, &GPIO_InitStruct);

  /* Re-configure MODEM_TO_HOST (=RING) pin to monitor it */
  GPIO_InitStruct.Pin = MODEM_RING_PIN;
  HAL_GPIO_Init(MODEM_RING_GPIO_PORT, &GPIO_InitStruct);
  HAL_NVIC_DisableIRQ(MODEM_RING_IRQN);

  if (modem_originated == 1U)
  {
    PRINT_FORCE("<< MODEM RESUME USECASE >> ")
    retval = HIFC_A_modem_resume(ipc_handle);
  }
  else /* HOST_WAKEUP or UNKNOWN_WAKEUP */
  {
    /**/
    PRINT_FORCE("<< HOST RESUME USECASE >> ")
    retval = HIFC_A_host_resume(ipc_handle);
  }

  /* reconfigure RING to default mode */
  enable_RING_wait_for_falling();
  SysCtrl_delay(100U);
  PRINT_DBG("resume channel OK ")

  return (retval);
}

static void enable_RING_wait_for_falling(void)
{
  /* activate MODEM_TO_HOST (=RING) interrupt to detect modem enters in Low Power mode
    * falling edge
  */
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = MODEM_RING_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL; /* GPIO_NOPULL or GPIO_PULLUP or GPIO_PULLDOWN ? */
  HAL_GPIO_Init(MODEM_RING_GPIO_PORT, &GPIO_InitStruct);

  HAL_NVIC_EnableIRQ(MODEM_RING_IRQN);
}

static void enable_RING_wait_for_rising(void)
{
  /* activate MODEM_TO_HOST (=RING) interrupt to detect modem enters
   * in Low Power mode
   */
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = MODEM_RING_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(MODEM_RING_GPIO_PORT, &GPIO_InitStruct);

  HAL_NVIC_EnableIRQ(MODEM_RING_IRQN);
}

static void disable_RING(void)
{
  /* deactivate MODEM_TO_HOST (=RING) interrupt
  * set pin in analog mode
  */
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = MODEM_RING_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(MODEM_RING_GPIO_PORT, &GPIO_InitStruct);

  HAL_NVIC_DisableIRQ(MODEM_RING_IRQN);
}

/* Note: following function is not part of standard sysctrl API
 * It is very specific to this modem and called by at_custom files of this modem.
 * It is used only when need to switch HwFlowControl mode of the Type1SC modem.
 * (after power_on or a reset)
 */
sysctrl_status_t SysCtrl_TYPE1SC_reinit_channel(IPC_Handle_t *ipc_handle, SysCtrl_TYPE1SC_HwFlowCtrl_t hwFC_status)
{
  sysctrl_status_t retval;
  PRINT_DBG("enter SysCtrl_TYPE1SC_reinit_channel")

  /* enable UART */
  retval = enable_UART(ipc_handle, hwFC_status);

  return (retval);
}

/* Private function Definition -----------------------------------------------*/
static sysctrl_status_t TYPE1SC_setup(void)
{
  sysctrl_status_t retval = SCSTATUS_OK;

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO config
   * Initial pins state
   */
  HAL_GPIO_WritePin(MODEM_DTR_GPIO_PORT, MODEM_DTR_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_RESET);

  /* Init GPIOs - common parameters */
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  /* Init GPIOs - DTR pin */
  GPIO_InitStruct.Pin = MODEM_DTR_PIN;
  HAL_GPIO_Init(MODEM_DTR_GPIO_PORT, &GPIO_InitStruct);

  /* Init GPIOs - PWR_EN pin */
  GPIO_InitStruct.Pin = MODEM_PWR_EN_PIN;
  HAL_GPIO_Init(MODEM_PWR_EN_GPIO_PORT, &GPIO_InitStruct);

  /* NOTE: do not activate RING interrupt at this point, it's too early and can result
   *       in a crash in NVIC handler
   */

  /* Print current modem UART setup */
  PRINT_FORCE("TYPE1SC UART config: BaudRate=%d / HW flow ctrl=%d", MODEM_UART_BAUDRATE,
              ((MODEM_UART_HWFLOWCTRL == UART_HWCONTROL_NONE) ? 0 : 1))

  return (retval);
}

static void TYPE1SC_LP_disable_modem_uart(void)
{
  /* Note:
  *  Not needed for TYPE1SC_LP_enable_modem_uart because it is
  *  already done in HAL_UART_Init() which call MSP_Init
  */
  PRINT_DBG("enter TYPE1SC_LP_disable_modem_uart")
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Configure CTS and RTS pins
   *   Do this even if HwFlowControl is not activated because
   *   pins are reserved in the current project
   */
  GPIO_InitStruct.Pin = MODEM_CTS_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(MODEM_CTS_GPIO_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = MODEM_RTS_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(MODEM_RTS_GPIO_PORT, &GPIO_InitStruct);

#if (DEBUG_LOW_POWER == 1)
  PRINT_INFO(">>> set RTS pin to LOW)")
#endif /* (DEBUG_LOW_POWER == 1) */
  HAL_GPIO_WritePin(MODEM_RTS_GPIO_PORT, MODEM_RTS_PIN, GPIO_PIN_RESET);

  /* Re-configure HOST_RX pin to monitor it */
  GPIO_InitStruct.Pin = MODEM_RX_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(MODEM_RX_GPIO_PORT, &GPIO_InitStruct);

  /* Re-configure HOST_TX pin to allow to trigger Low Power */
  GPIO_InitStruct.Pin = MODEM_TX_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(MODEM_TX_GPIO_PORT, &GPIO_InitStruct);

#if (DEBUG_LOW_POWER == 1)
  PRINT_INFO(">>> Request modem to enter Low Power (set HOST-TX pin to LOW)")
#endif /* (DEBUG_LOW_POWER == 1) */
  HAL_GPIO_WritePin(MODEM_TX_GPIO_PORT, MODEM_TX_PIN, GPIO_PIN_RESET);

#if (DEBUG_LOW_POWER == 1)
  PRINT_INFO(">>> Waiting for HOST-RX pin set to LOW by modem")
#endif /* (DEBUG_LOW_POWER == 1) */
  uint32_t count = 0U;
  uint32_t count_delay = 10U;
  uint32_t count_max = 500U;
  while ((HAL_GPIO_ReadPin(MODEM_RX_GPIO_PORT, MODEM_RX_PIN) == GPIO_PIN_SET) &&
         (count < count_max))
  {
    SysCtrl_delay(count_delay);
    count++;
  }

#if (DEBUG_LOW_POWER == 1)
  if (count == count_max)
  {
    PRINT_INFO(">>> error, HOST-RX LOW expected !!! (timeout=%ld)", count * count_delay)
  }
  else
  {
    PRINT_INFO(">>> HOST-RX LOW after %ld ms", count * count_delay)
  }
#endif /* (DEBUG_LOW_POWER == 1) */

  /* Set modem UART GPIO pins to ANALOG to optimize power consumption */
  GPIO_InitStruct.Pin = MODEM_TX_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(MODEM_TX_GPIO_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = MODEM_RX_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(MODEM_RX_GPIO_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = MODEM_CTS_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(MODEM_CTS_GPIO_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = MODEM_RTS_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(MODEM_RTS_GPIO_PORT, &GPIO_InitStruct);
}

static sysctrl_status_t HIFC_A_host_resume(IPC_Handle_t *ipc_handle)
{
  sysctrl_status_t retval;

#if (DEBUG_LOW_POWER == 1)
  PRINT_INFO("set HOST_TO_MODEM (=DTR) pin to 1 ")
#endif /* (DEBUG_LOW_POWER == 1) */
  HAL_GPIO_WritePin(MODEM_DTR_GPIO_PORT, MODEM_DTR_PIN, GPIO_PIN_SET);

#if (DEBUG_LOW_POWER == 1)
  PRINT_INFO(">>> Waiting for HOST_RX pin set to HIGH by modem")
#endif /* (DEBUG_LOW_POWER == 1) */
  uint32_t count = 0U;
  uint32_t count_delay = 10U;
  uint32_t count_max = 1000U;
  while ((HAL_GPIO_ReadPin(MODEM_RX_GPIO_PORT, MODEM_RX_PIN) == GPIO_PIN_RESET) &&
         (count < count_max))
  {
    SysCtrl_delay(count_delay);
    count++;
  }

#if (DEBUG_LOW_POWER == 1)
  if (count == count_max)
  {
    PRINT_INFO(">>> error, HOST_RX HIGH expected !!! (timeout=%ld)", count * count_delay)
  }
  else
  {
    PRINT_INFO(">>> HOST_RX HIGH after %ld ms", count * count_delay)
  }
#endif /* (DEBUG_LOW_POWER == 1) */

  /* enable UART */
  retval = enable_UART(ipc_handle, SYSCTRL_HW_FLOW_CONTROL_USER_SETTING);

#if (DEBUG_LOW_POWER == 1)
  PRINT_INFO(">>> Waiting for MODEM_TO_HOST (=RING) set to HIGH by modem")
#endif /* (DEBUG_LOW_POWER == 1) */
  count = 0U;
  count_delay = 10U;
  count_max = 1500U;
  while ((HAL_GPIO_ReadPin(MODEM_RING_GPIO_PORT, MODEM_RING_PIN) == GPIO_PIN_RESET) &&
         (count < count_max))
  {
    SysCtrl_delay(count_delay);
    count++;
  }

#if (DEBUG_LOW_POWER == 1)
  if (count == count_max)
  {
    PRINT_INFO(">>> error, HOST_TO_MODEM (=RING) HIGH expected !!! (timeout=%ld)", count * count_delay)
  }
  else
  {
    PRINT_INFO(">>> HOST_TO_MODEM (=RING) HIGH after %ld ms", count * count_delay)
  }
#endif /* (DEBUG_LOW_POWER == 1) */

  return (retval);
}

static sysctrl_status_t HIFC_A_modem_resume(IPC_Handle_t *ipc_handle)
{
  sysctrl_status_t retval;

#if (DEBUG_LOW_POWER == 1)
  PRINT_INFO(">>> FORCE UART HOST_TX TO HIGH <<<")
#endif /* (DEBUG_LOW_POWER == 1) */

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = MODEM_TX_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(MODEM_TX_GPIO_PORT, &GPIO_InitStruct);
  HAL_GPIO_WritePin(MODEM_TX_GPIO_PORT, MODEM_TX_PIN, GPIO_PIN_SET);

#if (DEBUG_LOW_POWER == 1)
  PRINT_INFO(">>> Waiting for HOST_RX pin set to HIGH by modem")
#endif /* (DEBUG_LOW_POWER == 1) */

  uint32_t count = 0U;
  uint32_t count_delay = 10U;
  uint32_t count_max = 500U;
  while ((HAL_GPIO_ReadPin(MODEM_RX_GPIO_PORT, MODEM_RX_PIN) == GPIO_PIN_RESET) &&
         (count < count_max))
  {
    SysCtrl_delay(count_delay);
    count++;
  }

#if (DEBUG_LOW_POWER == 1)
  if (count == count_max)
  {
    PRINT_INFO(">>> error, HOST_RX HIGH expected !!! (timeout=%ld)", count * count_delay)
  }
  else
  {
    PRINT_INFO(">>> HOST_RX HIGH after %ld ms", count * count_delay)
  }
#endif /* (DEBUG_LOW_POWER == 1) */

  /* enable UART */
  retval = enable_UART(ipc_handle, SYSCTRL_HW_FLOW_CONTROL_USER_SETTING);

  SysCtrl_delay(1000U);

#if (DEBUG_LOW_POWER == 1)
  PRINT_INFO("set MODEM DTR (HOST_TO_MODEM pin to 1 ")
#endif /* (DEBUG_LOW_POWER == 1) */
  HAL_GPIO_WritePin(MODEM_DTR_GPIO_PORT, MODEM_DTR_PIN, GPIO_PIN_SET);

  return (retval);
}

sysctrl_status_t enable_UART(IPC_Handle_t *ipc_handle, SysCtrl_TYPE1SC_HwFlowCtrl_t hwFC_status)
{
  sysctrl_status_t retval = SCSTATUS_OK;
  PRINT_DBG("enter enable_UART")

  /* UART deinitialization */
  if (HAL_UART_DeInit(&MODEM_UART_HANDLE) != HAL_OK)
  {
    PRINT_ERR("HAL_UART_DeInit error")
    retval = SCSTATUS_ERROR;
  }
  else
  {
    /* apply a delay before to reconfigure UART */
    SysCtrl_delay(50U);

    /* UART configuration */
    MODEM_UART_HANDLE.Instance = MODEM_UART_INSTANCE;
    MODEM_UART_HANDLE.Init.BaudRate = MODEM_UART_BAUDRATE;
    MODEM_UART_HANDLE.Init.WordLength = MODEM_UART_WORDLENGTH;
    MODEM_UART_HANDLE.Init.StopBits = MODEM_UART_STOPBITS;
    MODEM_UART_HANDLE.Init.Parity = MODEM_UART_PARITY;
    MODEM_UART_HANDLE.Init.Mode = MODEM_UART_MODE;
    if (hwFC_status == SYSCTRL_HW_FLOW_CONTROL_NONE)
    {
      /* force to No Hardware Flow Control */
      MODEM_UART_HANDLE.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    }
    else if (hwFC_status == SYSCTRL_HW_FLOW_CONTROL_RTS_CTS)
    {
      /* force to RTS/CTE Hardware Flow Control */
      MODEM_UART_HANDLE.Init.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
    }
    else /* if (hwFC_status == SYSCTRL_HW_FLOW_CONTROL_USER_SETTING) */
    {
      /* set the user setting for Hardware Flow Control */
      MODEM_UART_HANDLE.Init.HwFlowCtl = MODEM_UART_HWFLOWCTRL;
    }
    PRINT_INFO("UART config: setting HW Flow Control to %d",
               (MODEM_UART_HANDLE.Init.HwFlowCtl == UART_HWCONTROL_NONE) ? 0 : 1);
    MODEM_UART_HANDLE.Init.OverSampling = UART_OVERSAMPLING_16;
    MODEM_UART_HANDLE.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    /* do not activate autobaud (not compatible with current implementation) */
    MODEM_UART_HANDLE.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    /* UART initialization */
    if (HAL_UART_Init(&MODEM_UART_HANDLE) == HAL_OK)
    {
      /* In the code generated by CubeMX for HAL_UART_MspInit() (which is called by HAL_UART_Init()),
       * CTS and RTS are configured because default project uses Hardware Flow Control.
       * If HwFC has been deactivated by user, we need to deactivate corresponding PINS.
      */
      if (MODEM_UART_HANDLE.Init.HwFlowCtl == UART_HWCONTROL_NONE)
      {
        GPIO_InitTypeDef GPIO_InitStruct = {0};

        GPIO_InitStruct.Pin = MODEM_CTS_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(MODEM_CTS_GPIO_PORT, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = MODEM_RTS_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(MODEM_RTS_GPIO_PORT, &GPIO_InitStruct);
      }

      if (ipc_handle != NULL)
      {
        PRINT_DBG("call IPC_reset")
        (void) IPC_reset(ipc_handle);
      }
      HAL_NVIC_EnableIRQ(MODEM_UART_IRQN);
    }
    else
    {
      PRINT_ERR("error in HAL_UART_Init")
      retval = SCSTATUS_ERROR;
    }
  }

  return (retval);
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

