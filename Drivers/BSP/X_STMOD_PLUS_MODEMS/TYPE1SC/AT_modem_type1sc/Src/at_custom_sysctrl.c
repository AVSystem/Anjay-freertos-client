/**
  ******************************************************************************
  * @file    at_custom_sysctrl.c
  * @author  MCD Application Team
  * @brief   This file provides all the specific code for System control of
  *          MURATA-TYPE1SC-EVK module (ALT1250 modem: LTE-cat-M1)
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2020-2021 STMicroelectronics.
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
#include "at_sysctrl.h"
#include "at_custom_sysctrl.h"
#include "ipc_common.h"
#include "plf_config.h"

#if !defined(USE_HAL_STUB)
/* call HAL functions */
#define GPIO_WRITE(gpio, pin, state) HAL_GPIO_WritePin(gpio, pin, state)
#define GPIO_READ(gpio, pin)         HAL_GPIO_ReadPin(gpio, pin)
#else
/* call stub functions (for testing purposes) */
#include "at_hw_abstraction.h"
#define GPIO_WRITE(gpio, pin, state) AT_HwAbs_GPIO_WritePin(gpio, pin, state)
#define GPIO_READ(gpio, pin)         AT_HwAbs_GPIO_ReadPin(gpio, pin)
#endif /* !defined(USE_HAL_STUB) */

/** @addtogroup AT_CUSTOM AT_CUSTOM
  * @{
  */

/** @addtogroup AT_CUSTOM_ALTAIR_T1SC AT_CUSTOM ALTAIR_T1SC
  * @{
  */

/** @addtogroup AT_CUSTOM_ALTAIR_T1SC_SYSCTRL AT_CUSTOM ALTAIR_T1SC SYSCTRL
  * @{
  */

/** @defgroup AT_CUSTOM_ALTAIR_T1SC_SYSCTRL_Exported_Macros AT_CUSTOM ALTAIR_T1SC SYSCTRL Exported Macros
  * @{
  */

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

#define MAX_COUNT_OPEN_CHANNEL_HOST_RX_HIGH (500U)
#define MAX_COUNT_CLOSE_CHANNEL_HOST_RING_LOW (500U)
#define MAX_COUNT_DISABLE_UART_HOST_RX_LOW (500U)
#define MAX_COUNT_HOST_RESUME_HOST_RX_HIGH (1000U)
#define MAX_COUNT_HOST_RESUME_RING_HIGH (1500U)
#define MAX_COUNT_MODEM_RESUME_HOST_RX_HIGH (500U)

/**
  * @}
  */

/** @defgroup AT_CUSTOM_ALTAIR_T1SC_SYSCTRL_Exported_Defines AT_CUSTOM ALTAIR_T1SC SYSCTRL Exported Defines
  * @{
  */
#define DEBUG_LOW_POWER (0)
#define TYPE1SC_BOOT_TIME (4000U) /* Type 1 SC has uboot timer fixed to 4s, it could be modified later */
/**
  * @}
  */

/** @defgroup AT_CUSTOM_ALTAIR_T1SC_SYSCTRL_Private_Variables_Prototypes
  * AT_CUSTOM ALTAIR_T1SC SYSCTRL Private Variables Prototypes
  * @{
  */
static bool modem_uart_initialized = false; /* used to manage UART init/deinit */
/**
  * @}
  */

/** @defgroup AT_CUSTOM_ALTAIR_T1SC_SYSCTRL_Private_Functions_Prototypes
  * AT_CUSTOM ALTAIR_T1SC SYSCTRL Private Functions Prototypes
  * @{
  */
static sysctrl_status_t TYPE1SC_setup(void);
static sysctrl_status_t enable_UART(IPC_Handle_t *ipc_handle, SysCtrl_TYPE1SC_HwFlowCtrl_t hwFC_status);
static void TYPE1SC_LP_disable_modem_uart(void);
static void disable_RING(void);
#if (ENABLE_T1SC_LOW_POWER_MODE != 0U)
static void enable_RING_wait_for_falling(void); /* waiting event to enter in sleep (before complete) */
static void enable_RING_wait_for_rising(void);  /* waiting event in sleep mode = modem wake up */
static sysctrl_status_t HIFC_A_host_resume(IPC_Handle_t *ipc_handle);
static sysctrl_status_t HIFC_A_modem_resume(IPC_Handle_t *ipc_handle);
#endif /* (ENABLE_T1SC_LOW_POWER_MODE != 0U) */

/**
  * @}
  */

/** @defgroup AT_CUSTOM_ALTAIR_T1SC_SYSCTRL_Exported_Functions AT_CUSTOM ALTAIR_T1SC SYSCTRL Exported Functions
  * @{
  */

/**
  * @brief  Initialize System Control for this modem.
  * @param  type Type of device.
  * @param  p_devices_list Pointer to the structure describing the device and its interface.
  * @retval sysctrl_status_t
  */
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

/**
  * @brief  Open communication channel.
  * @param  type Type of device.
  * @retval sysctrl_status_t
  */
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
  GPIO_WRITE(MODEM_DTR_GPIO_PORT, MODEM_DTR_PIN, GPIO_PIN_SET);

#if (DEBUG_LOW_POWER == 1)
  PRINT_INFO(">>> Waiting for HOST_RX pin set to HIGH by modem")
#endif /* (DEBUG_LOW_POWER == 1) */
  /* wait for HOST_RX set to HIGH by modem before to reconfigure UART */
  uint32_t count = 0U;
  uint32_t count_delay = 10U;
  uint32_t count_max = MAX_COUNT_OPEN_CHANNEL_HOST_RX_HIGH;
  while ((GPIO_READ(MODEM_RX_GPIO_PORT, MODEM_RX_PIN) == GPIO_PIN_RESET) &&
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

/**
  * @brief  Close communication channel.
  * @param  type Type of device.
  * @retval sysctrl_status_t
  */
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
  GPIO_WRITE(MODEM_DTR_GPIO_PORT, MODEM_DTR_PIN, GPIO_PIN_RESET);

#if (DEBUG_LOW_POWER == 1)
  PRINT_INFO(">>> Waiting for RING state set to LOW by modem")
#endif /* (DEBUG_LOW_POWER == 1) */
  uint32_t count = 0U;
  uint32_t count_delay = 10U;
  uint32_t count_max = MAX_COUNT_CLOSE_CHANNEL_HOST_RING_LOW;
  while ((GPIO_READ(MODEM_RING_GPIO_PORT, MODEM_RING_PIN) == GPIO_PIN_SET) &&
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
    modem_uart_initialized = false;
    /* now, disable UART interface with modem */
    TYPE1SC_LP_disable_modem_uart();
    SysCtrl_delay(150U);

    PRINT_INFO("modem channel closed")
  }

  return (retval);
}

/**
  * @brief  Power ON the modem using hardware interface.
  * @param  type Type of device.
  * @retval sysctrl_status_t
  */
sysctrl_status_t SysCtrl_TYPE1SC_power_on(sysctrl_device_type_t type)
{
  UNUSED(type);
  sysctrl_status_t retval = SCSTATUS_OK;

  /* Power OFF */
  GPIO_WRITE(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_RESET);
  SysCtrl_delay(150U);

  PRINT_INFO("MODEM POWER ON")
  /* Power ON */
  GPIO_WRITE(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_SET);
  SysCtrl_delay(150U);

#if (ENABLE_T1SC_LOW_POWER_MODE != 0U)
  /* enable RING pin (default mode) */
  enable_RING_wait_for_falling();
#else
  /* disable RING pin (not used if modem low power not supported) */
  disable_RING();
#endif /* (ENABLE_T1SC_LOW_POWER_MODE != 0U) */

  PRINT_DBG("MODEM POWER ON done")
  return (retval);
}

/**
  * @brief  Power OFF the modem using hardware interface.
  * @param  type Type of device.
  * @retval sysctrl_status_t
  */
sysctrl_status_t SysCtrl_TYPE1SC_power_off(sysctrl_device_type_t type)
{
  UNUSED(type);
  sysctrl_status_t retval = SCSTATUS_OK;

  PRINT_INFO("MODEM POWER OFF")
  /* Power OFF */
  GPIO_WRITE(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_RESET);
  SysCtrl_delay(150U);

  /* disable RING pin */
  disable_RING();

  PRINT_DBG("MODEM POWER OFF done")
  return (retval);
}

/**
  * @brief  Reset the modem using hardware interface.
  * @param  type Type of device.
  * @retval sysctrl_status_t
  */
sysctrl_status_t SysCtrl_TYPE1SC_reset(sysctrl_device_type_t type)
{
  UNUSED(type);
  sysctrl_status_t retval = SCSTATUS_OK;

  /* Reference: TYPE1SC */
  PRINT_INFO("!!! Modem hardware reset triggered !!!")

  GPIO_WRITE(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_RESET);
  SysCtrl_delay(150U);
  GPIO_WRITE(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_SET);
  SysCtrl_delay(TYPE1SC_BOOT_TIME);

  return (retval);
}

/**
  * @brief  Select the SIM used by the modem using hardware interface.
  * @param  type Type of device.
  * @param  sim_slot Requested SIM slot.
  * @retval sysctrl_status_t
  */
sysctrl_status_t SysCtrl_TYPE1SC_sim_select(sysctrl_device_type_t type, sysctrl_sim_slot_t sim_slot)
{
  UNUSED(type);
  UNUSED(sim_slot);

  sysctrl_status_t retval = SCSTATUS_OK;
  /* SIM slot not selected via gpio for the moment */

  return (retval);
}

#if (ENABLE_T1SC_LOW_POWER_MODE != 0U)
/**
  * @brief  Request to suspend the communication channel.
  * @param  ipc_handle Pointer to the IPC structure.
  * @param  type Type of device.
  * @retval sysctrl_status_t
  */
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
  GPIO_WRITE(MODEM_DTR_GPIO_PORT, MODEM_DTR_PIN, GPIO_PIN_RESET);

  return (retval);
}

/**
  * @brief  Complete suspension of the communication channel.
  * @param  ipc_handle Pointer to the IPC structure.
  * @param  type Type of device.
  * @retval sysctrl_status_t
  */
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
    modem_uart_initialized = false;
    PRINT_DBG("modem channel closed")
  }

  /* reconfigure UART to GPIOs */
  TYPE1SC_LP_disable_modem_uart();

  /* reconfigure RING to wait for wakeup envent */
  enable_RING_wait_for_rising();

  return (retval);
}

/**
  * @brief  Resume the communication channel.
  * @param  ipc_handle Pointer to the IPC structure.
  * @param  type Type of device.
  * @param  modem_originated Iindicates ifd request comes from the Modem (true) or the Host (false)
  * @retval sysctrl_status_t
  */
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
#endif /* (ENABLE_T1SC_LOW_POWER_MODE != 0U) */

/**
  * @brief  Reinitialize the communication channel after switching HW Flow Control mode.
  * @param  ipc_handle Pointer to the IPC structure.
  * @param  hwFC_status Hardware Flow Control mode.
  * @retval sysctrl_status_t
  */
sysctrl_status_t SysCtrl_TYPE1SC_reinit_channel(IPC_Handle_t *ipc_handle, SysCtrl_TYPE1SC_HwFlowCtrl_t hwFC_status)
{
  /* Note: this function is not part of standard sysctrl API
   * It is very specific to this modem and called by at_custom files of this modem.
   * It is used only when need to switch HwFlowControl mode of the Type1SC modem.
   * (after power_on or a reset)
   */
  sysctrl_status_t retval;
  PRINT_DBG("enter SysCtrl_TYPE1SC_reinit_channel")

  /* enable UART */
  retval = enable_UART(ipc_handle, hwFC_status);

  return (retval);
}

/**
  * @}
  */

/** @defgroup AT_CUSTOM_ALTAIR_T1SC_SYSCTRL_Private_Functions AT_CUSTOM ALTAIR_T1SC SYSCTRL Private Functions
  * @{
  */

/**
  * @brief  Setup modem hardware configuration.
  * @retval none
  */
static sysctrl_status_t TYPE1SC_setup(void)
{
  sysctrl_status_t retval = SCSTATUS_OK;

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO config
   * Initial pins state
   */
  GPIO_WRITE(MODEM_DTR_GPIO_PORT, MODEM_DTR_PIN, GPIO_PIN_RESET);
  GPIO_WRITE(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_RESET);

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

/**
  * @brief  Enable Modem UART.
  * @param  ipc_handle Pointer to the structure of IPC.
  * @param  hwFC_status Hardware Flow Control mode.
  * @retval sysctrl_status_t
  */
static sysctrl_status_t enable_UART(IPC_Handle_t *ipc_handle, SysCtrl_TYPE1SC_HwFlowCtrl_t hwFC_status)
{
  sysctrl_status_t retval = SCSTATUS_OK;
  PRINT_DBG("enter enable_UART")

  /* UART deinitialization if required */
  if (modem_uart_initialized)
  {
    if (HAL_UART_DeInit(&MODEM_UART_HANDLE) != HAL_OK)
    {
      PRINT_ERR("HAL_UART_DeInit error")
      retval = SCSTATUS_ERROR;
    }
    else
    {
      modem_uart_initialized = false;

    }
  }

  if (retval == SCSTATUS_OK)
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
      modem_uart_initialized = true;

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

/**
  * @brief  Disable Modem UART for Low Power mode.
  * @retval none
  */
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
  GPIO_WRITE(MODEM_RTS_GPIO_PORT, MODEM_RTS_PIN, GPIO_PIN_RESET);

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
  GPIO_WRITE(MODEM_TX_GPIO_PORT, MODEM_TX_PIN, GPIO_PIN_RESET);

#if (DEBUG_LOW_POWER == 1)
  PRINT_INFO(">>> Waiting for HOST-RX pin set to LOW by modem")
#endif /* (DEBUG_LOW_POWER == 1) */
  uint32_t count = 0U;
  uint32_t count_delay = 10U;
  uint32_t count_max = MAX_COUNT_DISABLE_UART_HOST_RX_LOW;
  while ((GPIO_READ(MODEM_RX_GPIO_PORT, MODEM_RX_PIN) == GPIO_PIN_SET) &&
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

/**
  * @brief  Disable RING GPIO Interrupt mode.
  * @retval none
  */
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

  /*
   * AVSystem changes: Remove disabling IRQ for Modem Ring as it also affects
   * the User Button.
   */
  // HAL_NVIC_DisableIRQ(MODEM_RING_IRQN);
}

#if (ENABLE_T1SC_LOW_POWER_MODE != 0U)
/**
  * @brief  Configure RING GPIO Interrupt mode to wait for falling signal.
  * @retval none
  */
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

  HAL_NVIC_SetPriority(MODEM_RING_IRQN, 5, 0);
  HAL_NVIC_EnableIRQ(MODEM_RING_IRQN);
}

/**
  * @brief  Configure RING GPIO Interrupt mode to wait for rising signal.
  * @retval none
  */
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

  HAL_NVIC_SetPriority(MODEM_RING_IRQN, 5, 0);
  HAL_NVIC_EnableIRQ(MODEM_RING_IRQN);
}

/**
  * @brief  Resume modem UART on Host request (HIFC mode A).
  * @param  ipc_handle Pointer to the structure of IPC.
  * @retval sysctrl_status_t
  */
static sysctrl_status_t HIFC_A_host_resume(IPC_Handle_t *ipc_handle)
{
  sysctrl_status_t retval;

#if (DEBUG_LOW_POWER == 1)
  PRINT_INFO("set HOST_TO_MODEM (=DTR) pin to 1 ")
#endif /* (DEBUG_LOW_POWER == 1) */
  GPIO_WRITE(MODEM_DTR_GPIO_PORT, MODEM_DTR_PIN, GPIO_PIN_SET);

#if (DEBUG_LOW_POWER == 1)
  PRINT_INFO(">>> Waiting for HOST_RX pin set to HIGH by modem")
#endif /* (DEBUG_LOW_POWER == 1) */
  uint32_t count = 0U;
  uint32_t count_delay = 10U;
  uint32_t count_max = MAX_COUNT_HOST_RESUME_HOST_RX_HIGH;
  while ((GPIO_READ(MODEM_RX_GPIO_PORT, MODEM_RX_PIN) == GPIO_PIN_RESET) &&
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
  count_max = MAX_COUNT_HOST_RESUME_RING_HIGH;
  while ((GPIO_READ(MODEM_RING_GPIO_PORT, MODEM_RING_PIN) == GPIO_PIN_RESET) &&
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

/**
  * @brief  Resume modem UART on Modem request (HIFC mode A).
  * @param  ipc_handle Pointer to the structure of IPC.
  * @retval sysctrl_status_t
  */
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
  GPIO_WRITE(MODEM_TX_GPIO_PORT, MODEM_TX_PIN, GPIO_PIN_SET);

#if (DEBUG_LOW_POWER == 1)
  PRINT_INFO(">>> Waiting for HOST_RX pin set to HIGH by modem")
#endif /* (DEBUG_LOW_POWER == 1) */

  uint32_t count = 0U;
  uint32_t count_delay = 10U;
  uint32_t count_max = MAX_COUNT_MODEM_RESUME_HOST_RX_HIGH;
  while ((GPIO_READ(MODEM_RX_GPIO_PORT, MODEM_RX_PIN) == GPIO_PIN_RESET) &&
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
  GPIO_WRITE(MODEM_DTR_GPIO_PORT, MODEM_DTR_PIN, GPIO_PIN_SET);

  return (retval);
}
#endif /* (ENABLE_T1SC_LOW_POWER_MODE != 0U) */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

