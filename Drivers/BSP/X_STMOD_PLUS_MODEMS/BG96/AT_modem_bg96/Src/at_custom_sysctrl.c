/**
  ******************************************************************************
  * @file    at_custom_sysctrl.c
  * @author  MCD Application Team
  * @brief   This file provides all the specific code for System control of
  *          BG96 Quectel modem
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
#include "at_sysctrl.h"
#include "at_custom_sysctrl.h"
#include "ipc_common.h"
#include "plf_config.h"

/** @addtogroup AT_CUSTOM AT_CUSTOM
  * @{
  */

/** @addtogroup AT_CUSTOM_QUECTEL_BG96 AT_CUSTOM QUECTEL_BG96
  * @{
  */

/** @addtogroup AT_CUSTOM_QUECTEL_BG96_SYSCTRL AT_CUSTOM QUECTEL_BG96 SYSCTRL
  * @{
  */

/** @defgroup AT_CUSTOM_QUECTEL_BG96_SYSCTRL_Private_Macros AT_CUSTOM QUECTEL_BG96 SYSCTRL Private Macros
  * @{
  */
#if (USE_TRACE_SYSCTRL == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_FORCE(format, args...) TRACE_PRINT_FORCE(DBG_CHAN_ATCMD, DBL_LVL_P0, format "\n\r", ## args)
#define PRINT_INFO(format, args...) TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P0, "SysCtrl_BG96:" format "\n\r", ## args)
#define PRINT_ERR(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_ERR,\
                                                "SysCtrl_BG96 ERROR:" format "\n\r", ## args)
#else
#include <stdio.h>
#define PRINT_FORCE(format, args...) (void) printf(format "\n\r", ## args);
#define PRINT_INFO(format, args...)  (void) printf("SysCtrl_BG96:" format "\n\r", ## args);
#define PRINT_ERR(format, args...)   (void) printf("SysCtrl_BG96 ERROR:" format "\n\r", ## args);
#endif /* USE_PRINTF */

#else
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_FORCE(format, args...) TRACE_PRINT_FORCE(DBG_CHAN_ATCMD, DBL_LVL_P0, format "\n\r", ## args)
#else
#include <stdio.h>
#define PRINT_FORCE(format, args...) (void) printf(format "\n\r", ## args);
#endif /* USE_PRINTF */
#define PRINT_INFO(...) __NOP(); /* Nothing to do */
#define PRINT_ERR(...)  __NOP(); /* Nothing to do */
#endif /* USE_TRACE_SYSCTRL */
/**
  * @}
  */

/** @defgroup AT_CUSTOM_QUECTEL_BG96_SYSCTRL_Private_Defines AT_CUSTOM QUECTEL_BG96 SYSCTRL Private Defines
  * @{
  */
#define BG96_BOOT_TIME (5500U) /* Time in ms allowed to complete the modem boot procedure
                                *  according to spec, time = 13 sec
                                *  but practically, it seems that about 5 sec is acceptable
                                */
/**
  * @}
  */

/** @defgroup AT_CUSTOM_QUECTEL_BG96_SYSCTRL_Private_Functions_Prototypes
  *    AT_CUSTOM QUECTEL_BG96 SYSCTRL Private Functions Prototypes
  * @{
  */
static sysctrl_status_t SysCtrl_BG96_setup(void);
/**
  * @}
  */

/** @defgroup AT_CUSTOM_QUECTEL_BG96_SYSCTRL_Exported_Functions AT_CUSTOM QUECTEL_BG96 SYSCTRL Exported Functions
  * @{
  */

/**
  * @brief  Initialize System Control for this modem.
  * @param  type Type of device.
  * @param  p_devices_list Pointer to the structure describing the device and its interface.
  * @retval sysctrl_status_t
  */
sysctrl_status_t SysCtrl_BG96_getDeviceDescriptor(sysctrl_device_type_t type, sysctrl_info_t *p_devices_list)
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
#if defined(USE_MODEM_BG96)
      p_devices_list->type          = DEVTYPE_MODEM_CELLULAR;
      p_devices_list->ipc_device    = USER_DEFINED_IPC_DEVICE_MODEM;
      p_devices_list->ipc_interface = IPC_INTERFACE_UART;

      (void) IPC_init(p_devices_list->ipc_device, p_devices_list->ipc_interface, &MODEM_UART_HANDLE);
      retval = SCSTATUS_OK;
#else
      retval = SCSTATUS_ERROR
#endif /* USE_MODEM_BG96 */
    }
    else
    {
      retval = SCSTATUS_ERROR;
    }
  }

  if (retval != SCSTATUS_ERROR)
  {
    (void) SysCtrl_BG96_setup();
  }

  return (retval);
}

/**
  * @brief  Open communication channel.
  * @param  type Type of device.
  * @retval sysctrl_status_t
  */
sysctrl_status_t SysCtrl_BG96_open_channel(sysctrl_device_type_t type)
{
  UNUSED(type);
  sysctrl_status_t retval = SCSTATUS_OK;

  /* UART configuration */
  MODEM_UART_HANDLE.Instance = MODEM_UART_INSTANCE;
  MODEM_UART_HANDLE.Init.BaudRate = MODEM_UART_BAUDRATE;
  MODEM_UART_HANDLE.Init.WordLength = MODEM_UART_WORDLENGTH;
  MODEM_UART_HANDLE.Init.StopBits = MODEM_UART_STOPBITS;
  MODEM_UART_HANDLE.Init.Parity = MODEM_UART_PARITY;
  MODEM_UART_HANDLE.Init.Mode = MODEM_UART_MODE;
  MODEM_UART_HANDLE.Init.HwFlowCtl = MODEM_UART_HWFLOWCTRL;
  MODEM_UART_HANDLE.Init.OverSampling = UART_OVERSAMPLING_16;
  MODEM_UART_HANDLE.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  /* do not activate autobaud (not compatible with current implementation) */
  MODEM_UART_HANDLE.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

  /* UART initialization */
  if (HAL_UART_Init(&MODEM_UART_HANDLE) != HAL_OK)
  {
    PRINT_ERR("HAL_UART_Init error")
    retval = SCSTATUS_ERROR;
  }

  /* Enable the UART IRQn */
  HAL_NVIC_EnableIRQ(MODEM_UART_IRQN);

  return (retval);
}

/**
  * @brief  Close communication channel.
  * @param  type Type of device.
  * @retval sysctrl_status_t
  */
sysctrl_status_t SysCtrl_BG96_close_channel(sysctrl_device_type_t type)
{
  UNUSED(type);
  sysctrl_status_t retval = SCSTATUS_OK;

  /* Disable the UART IRQn */
  HAL_NVIC_DisableIRQ(MODEM_UART_IRQN);

  /* UART deinitialization */
  if (HAL_UART_DeInit(&MODEM_UART_HANDLE) != HAL_OK)
  {
    PRINT_ERR("HAL_UART_DeInit error")
    retval = SCSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Power ON the modem using hardware interface.
  * @param  type Type of device.
  * @retval sysctrl_status_t
  */
sysctrl_status_t SysCtrl_BG96_power_on(sysctrl_device_type_t type)
{
  UNUSED(type);
  sysctrl_status_t retval = SCSTATUS_OK;

  /* Reference: Quectel BG96 Hardware Design V1.4
  *  PWRKEY   connected to MODEM_PWR_EN (inverse pulse)
  *  RESET_N  connected to MODEM_RST    (inverse)
  *
  * Turn ON module sequence
  *
  *                PWRKEY  PWR_EN  modem_state
  * init             0       1      OFF
  * T=0 ms           1       0      OFF
  * T1=30 ms         0       1      BOOTING
  * T2=T1+500 ms     1       0      BOOTING
  * T3=T1+4800 ms    1       0      RUNNING
  */

  /* First, turn OFF module in case it was not switched off correctly (can occur after
   *  a manual reset).
   * Set PWR_EN to 0 at least 650ms
   */
  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_SET);
  SysCtrl_delay(700U);
  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_RESET);
  SysCtrl_delay(1000U);

  /* Power ON sequence */
  /* Set PWR_EN to 1 (initial state) */
  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_SET);
  SysCtrl_delay(50U);

  /* Set PWR_EN to 0 during at least 30ms as defined by Quectel */
  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_RESET);
  SysCtrl_delay(30U);

  /* Set PWR_EN to 1 during at least 500ms */
  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_SET);
  SysCtrl_delay(510U);

  /* Set PWR_EN to 0 */
  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_RESET);

  /* wait for Modem to complete its booting procedure */
  PRINT_INFO("Waiting %d millisec for modem running...", BG96_BOOT_TIME)
  SysCtrl_delay(BG96_BOOT_TIME);
  PRINT_INFO("...done")

  return (retval);
}

/**
  * @brief  Power OFF the modem using hardware interface.
  * @param  type Type of device.
  * @retval sysctrl_status_t
  */
sysctrl_status_t SysCtrl_BG96_power_off(sysctrl_device_type_t type)
{
  UNUSED(type);
  sysctrl_status_t retval = SCSTATUS_OK;

  /* Reference: Quectel BG96 Hardware Design V1.4
  *  PWRKEY   connected to MODEM_PWR_EN (inverse pulse)
  *  RESET_N  connected to MODEM_RST    (inverse)
  *
  * Turn OFF module sequence
  *
  *                PWRKEY  PWR_EN  modem_state
  * init             1       0      RUNNING
  * T=0 ms           0       1      RUNNING
  * T1=650 ms        1       0      POWER-DOWN PROCEDURE
  * T2=T1+2000 ms    1       0      OFF
  *
  * It is also safe to use AT+QPOWD command which is similar to turning off the modem
  * via PWRKEY pin.
  */

  /* wait at least 650ms */
  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_SET);
  SysCtrl_delay(700U);

  /* wait at least 2s but, in practice, it can exceed 4s or 5s */
  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_RESET);
  SysCtrl_delay(5000U);

  return (retval);
}

/**
  * @brief  Reset the modem using hardware interface.
  * @param  type Type of device.
  * @retval sysctrl_status_t
  */
sysctrl_status_t SysCtrl_BG96_reset(sysctrl_device_type_t type)
{
  UNUSED(type);
  sysctrl_status_t retval = SCSTATUS_OK;

  /* Reference: Quectel BG96 Hardware Design V1.4
  *  PWRKEY   connected to MODEM_PWR_EN (inverse pulse)
  *  RESET_N  connected to MODEM_RST    (inverse)
  *
  * Reset module sequence
  *
  * Can be done using RESET_N pin to low voltage for 100ms minimum
  *
  *          RESET_N  modem_state
  * init       1        RUNNING
  * T=0        0        OFF
  * T=150      1        BOOTING
  * T>=460     1        RUNNING
  */
  PRINT_INFO("!!! Hardware Reset triggered !!!")
  /* set RST to 1 for a time between 150ms and 460ms (200) */
  HAL_GPIO_WritePin(MODEM_RST_GPIO_PORT, MODEM_RST_PIN, GPIO_PIN_SET);
  SysCtrl_delay(200U);
  HAL_GPIO_WritePin(MODEM_RST_GPIO_PORT, MODEM_RST_PIN, GPIO_PIN_RESET);

  /* wait for Modem to complete its restart procedure */
  PRINT_INFO("Waiting %d millisec for modem running...", BG96_BOOT_TIME)
  SysCtrl_delay(BG96_BOOT_TIME);
  PRINT_INFO("...done")

  return (retval);
}

/**
  * @brief  Select the SIM used by the modem using hardware interface.
  * @param  type Type of device.
  * @param  sim_slot Requested SIM slot.
  * @retval sysctrl_status_t
  */
sysctrl_status_t SysCtrl_BG96_sim_select(sysctrl_device_type_t type, sysctrl_sim_slot_t sim_slot)
{
  UNUSED(type);
  sysctrl_status_t retval = SCSTATUS_OK;

  switch (sim_slot)
  {
    case SC_MODEM_SIM_SOCKET_0:
      HAL_GPIO_WritePin(MODEM_SIM_SELECT_0_GPIO_PORT, MODEM_SIM_SELECT_0_PIN, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(MODEM_SIM_SELECT_1_GPIO_PORT, MODEM_SIM_SELECT_1_PIN, GPIO_PIN_RESET);
      PRINT_INFO("MODEM SIM SOCKET SELECTED")
      break;

    case SC_MODEM_SIM_ESIM_1:
      HAL_GPIO_WritePin(MODEM_SIM_SELECT_0_GPIO_PORT, MODEM_SIM_SELECT_0_PIN, GPIO_PIN_SET);
      HAL_GPIO_WritePin(MODEM_SIM_SELECT_1_GPIO_PORT, MODEM_SIM_SELECT_1_PIN, GPIO_PIN_RESET);
      PRINT_INFO("MODEM SIM ESIM SELECTED")
      break;

    case SC_STM32_SIM_2:
      HAL_GPIO_WritePin(MODEM_SIM_SELECT_0_GPIO_PORT, MODEM_SIM_SELECT_0_PIN, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(MODEM_SIM_SELECT_1_GPIO_PORT, MODEM_SIM_SELECT_1_PIN, GPIO_PIN_SET);
      PRINT_INFO("STM32 SIM SELECTED")
      break;

    default:
      PRINT_ERR("Invalid SIM %d selected", sim_slot)
      retval = SCSTATUS_ERROR;
      break;
  }

  return (retval);
}

/**
  * @brief  Request Modem to wake up from PSM.
  * @param  delay Delay (in ms) to apply before to request wake up (for some usecases).
  * @retval sysctrl_status_t
  */
sysctrl_status_t SysCtrl_BG96_wakeup_from_PSM(uint32_t delay)
{
  sysctrl_status_t retval = SCSTATUS_OK;

  /* Reference: Quectel BG96 Hardware Design V1.4
  *  PWRKEY   connected to MODEM_PWR_EN (inverse pulse)
  *  RESET_N  connected to MODEM_RST    (inverse)
  *
  * wake up from PSM sequence : Drive PWRKEY pin to low level will wake up the module.
  *                             ie drive MODEM_PWR_EN to high level
  */

  SysCtrl_delay(delay);
  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_SET);
  SysCtrl_delay(200U);
  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_RESET);

  return (retval);
}
/**
  * @}
  */

/** @defgroup AT_CUSTOM_QUECTEL_BG96_SYSCTRL_Private_Functions AT_CUSTOM QUECTEL_BG96 SYSCTRL Private Functions
  * @{
  */

/**
  * @brief  Setup modem hardware configuration.
  * @retval none
  */
static sysctrl_status_t SysCtrl_BG96_setup(void)
{
  sysctrl_status_t retval = SCSTATUS_OK;

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO config
   * Initial pins state:
   *  PWR_EN initial state = 1 : used to power-on/power-off the board
   *  RST initial state = 0 : used to reset the board
   *  DTR initial state = 0 ; not used
   */
  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(MODEM_RST_GPIO_PORT, MODEM_RST_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(MODEM_DTR_GPIO_PORT, MODEM_DTR_PIN, GPIO_PIN_RESET);

  /* Init GPIOs - common parameters */
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  /* Init GPIOs - RESET pin */
  GPIO_InitStruct.Pin = MODEM_RST_PIN;
  HAL_GPIO_Init(MODEM_RST_GPIO_PORT, &GPIO_InitStruct);

  /* Init GPIOs - DTR pin */
  GPIO_InitStruct.Pin = MODEM_DTR_PIN;
  HAL_GPIO_Init(MODEM_DTR_GPIO_PORT, &GPIO_InitStruct);

  /* Init GPIOs - PWR_EN pin */
  GPIO_InitStruct.Pin = MODEM_PWR_EN_PIN;
  HAL_GPIO_Init(MODEM_PWR_EN_GPIO_PORT, &GPIO_InitStruct);

  PRINT_FORCE("BG96 UART config: BaudRate=%d / HW flow ctrl=%d", MODEM_UART_BAUDRATE,
              ((MODEM_UART_HWFLOWCTRL == UART_HWCONTROL_NONE) ? 0 : 1))

  return (retval);
}

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

