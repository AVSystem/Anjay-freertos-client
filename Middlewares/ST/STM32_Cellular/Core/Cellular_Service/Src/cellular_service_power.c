/**
  ******************************************************************************
  * @file    cellular_service_power.c
  * @author  MCD Application Team
  * @brief   This file defines functions for Cellular Service Power
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
#include "plf_config.h"

#if (USE_LOW_POWER == 1)
#include <string.h>

#include "cellular_service.h"
#include "cellular_service_datacache.h"
#include "cellular_service_task.h"
#include "cellular_service_utils.h"
#include "cellular_service_power.h"
#include "cellular_service_os.h"

#include "rtosal.h"

#if (USE_CMD_CONSOLE == 1)
#include "cmd.h"
#endif  /* (USE_CMD_CONSOLE == 1) */

#include "dc_common.h"
#include "error_handler.h"


/* Private macros ------------------------------------------------------------*/

#if (USE_PRINTF == 0U)
/* Trace macro definition */
#include "trace_interface.h"
#define PRINT_FORCE(format, args...) \
  TRACE_PRINT_FORCE(DBG_CHAN_CELLULAR_SERVICE, DBL_LVL_P0, "" format "", ## args)

#else
#include <stdio.h>
#define PRINT_FORCE(format, args...)                (void)printf(format , ## args);
#endif  /* (USE_PRINTF == 1) */

/* Private defines -----------------------------------------------------------*/



#define CSP_CMD_PARAM_MAX        5U     /* number max of cmd param        */

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  CSP_PowerState_t power_state;            /* Actual activated power state                     */
  CSP_PowerState_t target_power_state;     /* Power state to reach, but not actually activated */
}  CSP_Context_t;

/* Private variables ---------------------------------------------------------*/
/* Timeout used for Modem low power activation. If this timer fire, CSP considered that the modem can not activate */
/*   low power, and CSP reverts to the previous low power state                                                    */
static osTimerId                  CSP_timeout_timer_handle;
static dc_cellular_power_config_t csp_dc_power_config;
static CSP_Context_t              CSP_Context;
/* string names associated with modem power states */
static const uint8_t              *CSP_power_state_name[] =
{
  ((uint8_t *)"CSP_LOW_POWER_DISABLED"),
  ((uint8_t *)"CSP_LOW_POWER_INACTIVE"),
  ((uint8_t *)"CSP_LOW_POWER_ON_GOING"),
  ((uint8_t *)"CSP_LOW_POWER_ACTIVE")
};
/*  mutual exclusion */
/* static osMutexId         CSP_mutex = NULL; */
#if (USE_CMD_CONSOLE == 1)
typedef struct
{
  dc_cellular_power_psm_config_t   psm;    /* !< psm config          */
  dc_cellular_power_edrx_config_t  edrx;   /* !< eDRX config         */
} csp_cmd_power_config_t;

/* initialize defaults values for PSM and eDRX config */
static csp_cmd_power_config_t csp_cmd_power_config =
{
  /* PSM config */
  {
    DC_POWER_PSM_REQ_PERIODIC_RAU_DEFAULT,
    DC_POWER_PSM_REQ_GPRS_READY_TIMER_DEFAULT,
    DC_POWER_PSM_REQ_PERIODIC_TAU_DEFAULT,
    DC_POWER_PSM_REQ_ACTIVE_TIMER_DEFAULT,
  },
  /* eDRX config */
  {
    DC_POWER_EDRX_ACT_TYPE_DEFAULT,
    DC_POWER_EDRX_REQ_VALUE_DEFAULT,
  }
};

/* CMD label to prefix the CSP commands */
static uint8_t *CSP_cmd_label = ((uint8_t *)"csp");

/* string names associated with eDRX modes */
static const uint8_t *CSP_edrx_act_type_name[] =
{
  ((uint8_t *)"EDRX_ACT_NOT_USED"),
  ((uint8_t *)"EDRX_ACT_EC_GSM_IOT"),
  ((uint8_t *)"EDRX_ACT_GSM"),
  ((uint8_t *)"EDRX_ACT_UTRAN"),
  ((uint8_t *)"EDRX_ACT_E_UTRAN_WB_S1"),
  ((uint8_t *)"EDRX_ACT_E_UTRAN_NB_S1")
};
#endif  /* (USE_CMD_CONSOLE == 1) */

/* Global variables ----------------------------------------------------------*/


/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  timer callback to check sleep activation
  * @param  argument - argument (not used)
  * @retval none
  */
static void CSP_TimeoutTimerCallback(void *argument);
/**
  * @brief  timer callback to check sleep activation
  * @param  argument - argument (not used)
  * @retval none
  */
static void CSP_ArmTimeout(uint32_t timeout);
/**
  * @brief  enter in low power mode
  * @note  called by cellular service task automaton
  * @param  none
  * @retval error code
  */
static void CSP_SleepRequest(uint32_t timeout);

#if (USE_CMD_CONSOLE == 1)
/**
  * @brief  Help command management. Display help for CSP commands
  * @param  none
  * @retval none
  */
static void CSP_HelpCmd(void);
/**
  * @brief  display the actual configuration of cellular service power
  * @param  none
  * @retval none
  */
static void CSP_get_config(void);
/**
  * @brief  Cellular Service Power command line management
  * @param  cmd_line_p  command line
  * @retval -
  */
static void CSP_cmd(uint8_t *cmd_line_p);
#endif  /* (USE_CMD_CONSOLE == 1) */

/* Private function Definition -----------------------------------------------*/


/* ============================================================ */
/* ==== STM32 STUB BEGIN ======= */
/* ============================================================ */
static void STM32_SleepRequest(void);
static void STM32_Wakeup(void);

static void STM32_SleepRequest(void)
{
  PRINT_CELLULAR_SERVICE("CST: STM32_SleepRequest\n\r")
}
static void STM32_Wakeup(void)
{
  PRINT_CELLULAR_SERVICE("CST: STM32_Wakeup\n\r")
}
/* ============================================================ */
/* ==== STM32 STUB END ======= */
/* ============================================================ */

#if (USE_CMD_CONSOLE == 1)
/**
  * @brief  Help command management. Display help for CSP commands
  * @param  none
  * @retval none
  */
static void CSP_HelpCmd(void)
{
  /* Display help header */
  CMD_print_help(CSP_cmd_label);

  PRINT_FORCE("%s help\n\r", (CRC_CHAR_t *)CSP_cmd_label)
  /* General commands */
  PRINT_FORCE("%s state   (Displays the current power state)\n\r", CSP_cmd_label)
  PRINT_FORCE("%s mode [runrealtime|runinteractive|idle|ildllp|lp|ulp] (select power mode)\n\r", CSP_cmd_label)
  PRINT_FORCE("%s idle  (enter in low power)\n\r", CSP_cmd_label)
  PRINT_FORCE("%s wakeup  (leave low power)\n\r", CSP_cmd_label)
  /* Low power configuration commands */
  /* Introduction */
  PRINT_FORCE("\n\r")
  PRINT_FORCE("PSM and eDRX configuration can be modified using '%s config set'\n\r", CSP_cmd_label)
  PRINT_FORCE("and '%s config send' commands.\n\r", CSP_cmd_label)
  PRINT_FORCE("Update configuration is performed in two steps:\n\r")
  /* 1st step commands for Low power configuration */
  PRINT_FORCE("- 1st step: set the configuration parameters using the following command:\n\r");
  PRINT_FORCE("%s config set psmrau|psmgprstimer|psmtau|psmactivetimer|edrxacttype|edrxvalue ", CSP_cmd_label)
  PRINT_FORCE(" <hexa value prefixed by '0x'>  (set power config value)\n\r")
  PRINT_FORCE("  or\n\r")
  PRINT_FORCE("%s config set edrxacttype  NOT_USED|EC_GSM_IOT|GSM|UTRAN|UTRAN_WB_S1|UTRAN_NB_S1 ",
              CSP_cmd_label)
  PRINT_FORCE("(set edrx config value)\n\r")
  /* 2nd step commands for Low power configuration */
  PRINT_FORCE("- 2nd step: send the new configuration to the modem\n\r");
  PRINT_FORCE("%s config send\n\r", (CRC_CHAR_t *)CSP_cmd_label)
  PRINT_FORCE("\n\r");
  PRINT_FORCE("Note: 'psmrau psmgprstimer psmtau psmactivetimer edrxacttype edrxvalue' parameters\n\r")
  PRINT_FORCE("      have a default values.\n\r")
  PRINT_FORCE("      Current values of these parameters can be read using '%s config get' command\n\r", CSP_cmd_label)
  PRINT_FORCE("\n\r");

  PRINT_FORCE("\n\r");
}

/**
  * @brief  display the actual configuration of cellular service power
  * @param  none
  * @retval none
  */
static void CSP_get_config(void)
{
  /* Display low power config */
  /* PSM values */
  PRINT_FORCE("----------\n\r")
  PRINT_FORCE("PSM config\n\r")
  PRINT_FORCE("----------\n\r")
  PRINT_FORCE("periodic_RAU     0x%02x\n\r", csp_cmd_power_config.psm.req_periodic_RAU)
  PRINT_FORCE("GPRS ready timer 0x%02x\n\r", csp_cmd_power_config.psm.req_GPRS_READY_timer)
  PRINT_FORCE("periodic TAU     0x%02x\n\r", csp_cmd_power_config.psm.req_periodic_TAU)
  PRINT_FORCE("active time      0x%02x\n\r", csp_cmd_power_config.psm.req_active_time)
  PRINT_FORCE("\n\r")
  /* eDRX values */
  PRINT_FORCE("-----------\n\r")
  PRINT_FORCE("eDRX config\n\r")
  PRINT_FORCE("-----------\n\r")
  PRINT_FORCE("type %s\n\r", CSP_edrx_act_type_name[csp_cmd_power_config.edrx.act_type])
  PRINT_FORCE("value 0x%02x\n\r", csp_cmd_power_config.edrx.req_value)
}

/**
  * @brief  Cellular Service Power command line management
  * @param  cmd_line_p  command line
  * @retval -
  */
static void CSP_cmd(uint8_t *cmd_line_p)
{
  static dc_cellular_power_config_t csp_cmd_dc_power_config;
  static const uint8_t *CSP_power_mode_name[] =
  {
    ((uint8_t *)"POWER_RUN_REAL_TIME"),
    ((uint8_t *)"POWER_RUN_INTERACTIVE_0"),
    ((uint8_t *)"POWER_RUN_INTERACTIVE_1"),
    ((uint8_t *)"POWER_RUN_INTERACTIVE_2"),
    ((uint8_t *)"POWER_RUN_INTERACTIVE_3"),
    ((uint8_t *)"POWER_IDLE"),
    ((uint8_t *)"POWER_IDLE_LP"),
    ((uint8_t *)"POWER_LP"),
    ((uint8_t *)"POWER_ULP"),
    ((uint8_t *)"POWER_STBY1"),
    ((uint8_t *)"POWER_STBY2"),
    ((uint8_t *)"POWER_OFF")
  };

  uint8_t    *argv_p[CSP_CMD_PARAM_MAX];
  uint32_t   argc;
  uint32_t   value;
  uint8_t    *cmd_p;
  CS_Status_t status;

  PRINT_FORCE("\n\r")

  /* Get the first word of the command line */
  cmd_p = (uint8_t *)strtok((CRC_CHAR_t *)cmd_line_p, " \t");

  /* verify that it is a csp command : first word should be CSP_cmd_label */
  if (cmd_p != NULL)
  {
    if (memcmp((CRC_CHAR_t *)cmd_p,
               (CRC_CHAR_t *)CSP_cmd_label,
               crs_strlen(cmd_p))
        == 0)
    {
      /* parameters parsing. Fill the array argv_p with all the found parameters */
      for (argc = 0U ; argc < CSP_CMD_PARAM_MAX ; argc++)
      {
        argv_p[argc] = (uint8_t *)strtok(NULL, " \t");
        if (argv_p[argc] == NULL)
        {
          /* end of argument list reached */
          break;
        }
      }

      /* if no parameters */
      if (argc == 0U)
      {
        /* no argument: displays help */
        CSP_HelpCmd();
      }
      /* help command ------------------------------------------------------------------------------------------------*/
      else if (memcmp((CRC_CHAR_t *)argv_p[0],  "help",  crs_strlen(argv_p[0])) == 0)
      {
        /* help command: displays help */
        CSP_HelpCmd();
      }
      /* config command ----------------------------------------------------------------------------------------------*/
      else if (memcmp((CRC_CHAR_t *)argv_p[0], "config", crs_strlen(argv_p[0])) == 0)
      {
        /* 'csp config' command */
        if (argc == 1U)
        {
          CSP_get_config();
        }
        /* config set command ----------------------------------------------------------------------------------------*/
        else if (memcmp((CRC_CHAR_t *)argv_p[1], "set", crs_strlen(argv_p[1])) == 0)
        {
          /* 'csp config set ...' command */
          if (argc == 4U)
          {
            if (memcmp((CRC_CHAR_t *)argv_p[2], "psmrau", crs_strlen(argv_p[2])) == 0)
            {
              /* 'csp config set psmrau ...' command */
              (void)CMD_GetValue(argv_p[3], &value);
              csp_cmd_power_config.psm.req_periodic_RAU = (uint8_t)value & 0xffU;
            }
            else if (memcmp((CRC_CHAR_t *)argv_p[2], "psmgprstimer", crs_strlen(argv_p[2])) == 0)
            {
              /* 'csp config set psmgprstimer ...' command */
              (void)CMD_GetValue(argv_p[3], &value);
              csp_cmd_power_config.psm.req_GPRS_READY_timer = (uint8_t)value & 0xffU;
            }
            else if (memcmp((CRC_CHAR_t *)argv_p[2], "psmtau", crs_strlen(argv_p[2])) == 0)
            {
              /* 'csp config set psmtau ...' command */
              (void)CMD_GetValue(argv_p[3], &value);
              csp_cmd_power_config.psm.req_periodic_TAU = (uint8_t)value & 0xffU;
            }
            else if (memcmp((CRC_CHAR_t *)argv_p[2], "psmactivetimer", crs_strlen(argv_p[2])) == 0)
            {
              /* 'csp config set psmactivetimer ...' command */
              (void)CMD_GetValue(argv_p[3], &value);
              csp_cmd_power_config.psm.req_active_time = (uint8_t)value & 0xffU;
            }
            else if (memcmp((CRC_CHAR_t *)argv_p[2], "edrxacttype", crs_strlen(argv_p[2])) == 0)
            {
              /* 'csp config set edrxacttype ...' command */
              if (memcmp((CRC_CHAR_t *)argv_p[3], "NOT_USED", crs_strlen(argv_p[3])) == 0)
              {
                /* eDRX Not used */
                csp_cmd_power_config.edrx.act_type = CA_EIDRX_ACT_NOT_USED;
              }
              else if (memcmp((CRC_CHAR_t *)argv_p[3], "EC_GSM_IOT", crs_strlen(argv_p[3])) == 0)
              {
                /* eDRX GSM IOT */
                csp_cmd_power_config.edrx.act_type = CA_EIDRX_ACT_EC_GSM_IOT;
              }
              else if (memcmp((CRC_CHAR_t *)argv_p[3], "GSM", crs_strlen(argv_p[3])) == 0)
              {
                /* eDRX GSM */
                csp_cmd_power_config.edrx.act_type = CA_EIDRX_ACT_GSM;
              }
              else if (memcmp((CRC_CHAR_t *)argv_p[3], "UTRAN_WB_S1", crs_strlen(argv_p[3])) == 0)
              {
                /* eDRX WBS1 */
                csp_cmd_power_config.edrx.act_type = CA_EIDRX_ACT_E_UTRAN_WBS1;
              }
              else if (memcmp((CRC_CHAR_t *)argv_p[3], "UTRAN_NB_S1", crs_strlen(argv_p[3])) == 0)
              {
                /* eDRX NBS1 */
                csp_cmd_power_config.edrx.act_type = CA_EDRX_ACT_E_UTRAN_NBS1;
              }
              else if (memcmp((CRC_CHAR_t *)argv_p[3], "UTRAN", crs_strlen(argv_p[3])) == 0)
              {
                /* eDRX UTRAN */
                csp_cmd_power_config.edrx.act_type = CA_EIDRX_ACT_UTRAN;
              }
              else
              {
                /* 'csp config set edrxacttype ...' command : wrong value */
                PRINT_FORCE("csp config set edrxacttype wrong value %s\n\r", argv_p[3])
                PRINT_FORCE("Usage:\n\r")
                CSP_HelpCmd();
              }
            }
            else if (memcmp((CRC_CHAR_t *)argv_p[2], "edrxvalue", crs_strlen(argv_p[2])) == 0)
            {
              /* 'csp config set edrxvalue ...' command */
              (void)CMD_GetValue(argv_p[3], &value);
              csp_cmd_power_config.edrx.req_value = (uint8_t)value & 0xffU;
            }
            else
            {
              /* 'csp config set  ...' command : wrong parameters */
              PRINT_FORCE("Wrong syntax. Usage:\n\r")
              CSP_HelpCmd();
            }
          }
          else
          {
            /* 'csp config   ...' command : wrong parameters */
            PRINT_FORCE("Wrong syntax. Usage:\n\r")
            CSP_HelpCmd();
          }
        }
        /* config get command ----------------------------------------------------------------------------------------*/
        else if (memcmp((CRC_CHAR_t *)argv_p[1], "get", crs_strlen(argv_p[1])) == 0)
        {
          /* 'csp config get' command */
          CSP_get_config();
        }
        /* config send command ---------------------------------------------------------------------------------------*/
        else if (memcmp((CRC_CHAR_t *)argv_p[1], "send", crs_strlen(argv_p[1])) == 0)
        {
          /* 'csp config send' command */
          /* Read actual parameters from data Cache */
          (void)dc_com_read(&dc_com_db, DC_CELLULAR_POWER_CONFIG, (void *)&csp_cmd_dc_power_config,
                            sizeof(dc_cellular_power_config_t));
          /* PSM parameters */
          csp_cmd_dc_power_config.power_cmd                 = CA_POWER_CMD_SETTING;
          csp_cmd_dc_power_config.psm_present               = true;
          csp_cmd_dc_power_config.psm.req_periodic_RAU      = csp_cmd_power_config.psm.req_periodic_RAU;
          csp_cmd_dc_power_config.psm.req_GPRS_READY_timer  = csp_cmd_power_config.psm.req_GPRS_READY_timer;
          csp_cmd_dc_power_config.psm.req_periodic_TAU      = csp_cmd_power_config.psm.req_periodic_TAU;
          csp_cmd_dc_power_config.psm.req_active_time       = csp_cmd_power_config.psm.req_active_time;
          /* eDRX parameters */
          csp_cmd_dc_power_config.edrx_present              = true;
          csp_cmd_dc_power_config.edrx.act_type             = csp_cmd_power_config.edrx.act_type;
          csp_cmd_dc_power_config.edrx.req_value            = csp_cmd_power_config.edrx.req_value;
          /* Write new parameters to Data Cache */
          (void)dc_com_write(&dc_com_db, DC_CELLULAR_POWER_CONFIG, (void *)&csp_cmd_dc_power_config,
                             sizeof(dc_cellular_power_config_t));

          PRINT_FORCE("New config\n\r")
          CSP_get_config();
        }
        else
        {
          PRINT_FORCE("Wrong syntax. Usage:\n\r")
          CSP_HelpCmd();
        }
      }
      /* state command -----------------------------------------------------------------------------------------------*/
      else if (memcmp((CRC_CHAR_t *)argv_p[0], "state", crs_strlen(argv_p[0])) == 0)
      {
        /* 'csp state' command */
        /* Read data from data cache */
        (void)dc_com_read(&dc_com_db, DC_CELLULAR_POWER_CONFIG, (void *)&csp_cmd_dc_power_config,
                          sizeof(dc_cellular_power_config_t));
        PRINT_FORCE("Current power mode: %s\n\r", CSP_power_mode_name[csp_cmd_dc_power_config.power_mode])

        PRINT_FORCE("power state %s\n\r", CSP_power_state_name[CSP_Context.power_state])
        PRINT_FORCE("----------\n\r")
        PRINT_FORCE("PSM config\n\r")
        PRINT_FORCE("----------\n\r")
        if (csp_cmd_dc_power_config.psm_present == true)
        {
          /* PSM parameter used, display PSM parameters */
          PRINT_FORCE("PSM config present\n\r")
          PRINT_FORCE("periodic_RAU     0x%02x\n\r", csp_cmd_dc_power_config.psm.req_periodic_RAU)
          PRINT_FORCE("GPRS ready timer 0x%02x\n\r", csp_cmd_dc_power_config.psm.req_GPRS_READY_timer)
          PRINT_FORCE("periodic TAU     0x%02x\n\r", csp_cmd_dc_power_config.psm.req_periodic_TAU)
          PRINT_FORCE("active time      0x%02x\n\r", csp_cmd_dc_power_config.psm.req_active_time)
        }
        else
        {
          /* PSM Power config not present */
          PRINT_FORCE("PSM config not present\n\r")
        }
        PRINT_FORCE("\n\r")
        PRINT_FORCE("-----------\n\r")
        PRINT_FORCE("eDRX config\n\r")
        PRINT_FORCE("-----------\n\r")
        if (csp_cmd_dc_power_config.edrx_present == true)
        {
          /* eDRX parameter used, display eDRX parameters */
          PRINT_FORCE("periodic_RAU %s\n\r", CSP_edrx_act_type_name[csp_cmd_dc_power_config.edrx.act_type])
          PRINT_FORCE("periodic_RAU 0x%02x\n\r", csp_cmd_dc_power_config.edrx.req_value)
        }
        else
        {
          /* eDRX Power config not present */
          PRINT_FORCE("eDRX config not present\n\r")
        }
      }
      else if (memcmp((CRC_CHAR_t *)argv_p[0], "wakeup", crs_strlen(argv_p[0])) == 0)
      {
        /* 'csp wakeup' command */
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
        status = CSP_DataWakeup(HOST_WAKEUP);
        if (status == CS_OK)
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
        {
          PRINT_FORCE("Data wakeup OK\n\r")
        }
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
        else
        {
          PRINT_FORCE("Data wakeup FAIL\n\r")
        }
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
      }
      /* csp setmode command------------------------------------------------------------------------------------------*/
      else if (memcmp((CRC_CHAR_t *)argv_p[0], "setmode", crs_strlen(argv_p[0])) == 0)
      {
        /* 'csp setmode ...' command */
        if (argc == 2U)
        {
          (void)dc_com_read(&dc_com_db, DC_CELLULAR_POWER_CONFIG, (void *)&csp_cmd_dc_power_config,
                            sizeof(dc_cellular_power_config_t));
          if (memcmp((CRC_CHAR_t *)argv_p[1], "runrealtime", crs_strlen(argv_p[1])) == 0)
          {
            /* 'csp setmode runrealtime ...' command */
            csp_cmd_dc_power_config.power_mode = CA_POWER_RUN_REAL_TIME;
          }
          else if (memcmp((CRC_CHAR_t *)argv_p[1], "runinteractive0", crs_strlen(argv_p[1])) == 0)
          {
            /* 'csp setmode runinteractive0 ...' command */
            csp_cmd_dc_power_config.power_mode = CA_POWER_RUN_INTERACTIVE_0;
          }
          else if (memcmp((CRC_CHAR_t *)argv_p[1], "runinteractive1", crs_strlen(argv_p[1])) == 0)
          {
            /* 'csp setmode runinteractive1 ...' command */
            csp_cmd_dc_power_config.power_mode = CA_POWER_RUN_INTERACTIVE_1;
          }
          else if (memcmp((CRC_CHAR_t *)argv_p[1], "runinteractive2", crs_strlen(argv_p[1])) == 0)
          {
            /* 'csp setmode runinteractive2 ...' command */
            csp_cmd_dc_power_config.power_mode = CA_POWER_RUN_INTERACTIVE_2;
          }
          else if (memcmp((CRC_CHAR_t *)argv_p[1], "runinteractive3", crs_strlen(argv_p[1])) == 0)
          {
            /* 'csp setmode runinteractive3 ...' command */
            csp_cmd_dc_power_config.power_mode = CA_POWER_RUN_INTERACTIVE_3;
          }
          else if (memcmp((CRC_CHAR_t *)argv_p[1], "idle", crs_strlen(argv_p[1])) == 0)
          {
            /* 'csp setmode idle ...' command */
            csp_cmd_dc_power_config.power_mode = CA_POWER_IDLE;
          }
          else if (memcmp((CRC_CHAR_t *)argv_p[1], "ildllp", crs_strlen(argv_p[1])) == 0)
          {
            /* 'csp setmode ildllp ...' command */
            csp_cmd_dc_power_config.power_mode = CA_POWER_IDLE_LP;
          }
          else if (memcmp((CRC_CHAR_t *)argv_p[1], "lp", crs_strlen(argv_p[1])) == 0)
          {
            /* 'csp setmode lp ...' command */
            csp_cmd_dc_power_config.power_mode = CA_POWER_LP;
          }
          else if (memcmp((CRC_CHAR_t *)argv_p[1], "ulp", crs_strlen(argv_p[1])) == 0)
          {
            /* 'csp setmode ulp ...' command */
            csp_cmd_dc_power_config.power_mode = CA_POWER_ULP;
          }
          /*
            Futur power management mode
            else if (memcmp((CRC_CHAR_t *)argv_p[1], "stby1", crs_strlen(argv_p[1])) == 0)
            {
              csp_cmd_dc_power_config.power_mode = CA_POWER_STBY1;
            }
            else if (memcmp((CRC_CHAR_t *)argv_p[1], "stby1", crs_strlen(argv_p[1])) == 0)
            {
              csp_cmd_dc_power_config.power_mode = CA_POWER_STBY2;
            }
            else if (memcmp((CRC_CHAR_t *)argv_p[1], "off", crs_strlen(argv_p[1])) == 0)
            {
              csp_cmd_dc_power_config.power_mode = CA_POWER_OFF;
            }
          */
          else
          {
            /* 'csp setmode  ...' wrong parameter */
            PRINT_FORCE("csp setmode wrong value %s\n\r", argv_p[1])
            PRINT_FORCE("Usage:\n\r")
            CSP_HelpCmd();
          }
          (void)dc_com_write(&dc_com_db, DC_CELLULAR_POWER_CONFIG, (void *)&csp_cmd_dc_power_config,
                             sizeof(dc_cellular_power_config_t));

        }
        PRINT_FORCE("Current power mode: %s\n\r", CSP_power_mode_name[csp_cmd_dc_power_config.power_mode])

      }
      /* idle command ------------------------------------------------------------------------------------------------*/
      else if (memcmp((CRC_CHAR_t *)argv_p[0], "idle", crs_strlen(argv_p[0])) == 0)
      {
        /* 'csp idle' command */
        status = CSP_DataIdle();
        if (status == CS_OK)
        {
          /* DataIdle return OK */
          PRINT_FORCE("Data idle OK\n\r")
        }
        else
        {
          /* DataIdle return KO */
          PRINT_FORCE("Data idle FAIL\n\r")
        }
      }
      else
      {
        /* DataIdle bad parameter */
        PRINT_FORCE("Wrong syntax. Usage:\n\r")
        CSP_HelpCmd();
      }
    }
  }
}
#endif  /* (USE_CMD_CONSOLE == 1) */


/**
  * @brief  timer callback to check sleep activation
  * @param  argument - argument (not used)
  * @retval none
  */
static void CSP_TimeoutTimerCallback(void *argument)
{
  UNUSED(argument);
  PRINT_CELLULAR_SERVICE("CST: CSP_TimeoutTimerCallback\n\r")

  /* Send message POWER_SLEEP_TIMEOUT_EVENT to automaton */
  CST_send_message(CST_MESSAGE_CS_EVENT, CST_POWER_SLEEP_TIMEOUT_EVENT);
}

/**
  * @brief  low power leaved. Wakeup is achieved, Update needed variables.
  * @param  none
  * @retval none
  */
void CSP_WakeupComplete(void)
{
  dc_cellular_power_status_t dc_power_status;

  PRINT_CELLULAR_SERVICE("CST: CSP_WakeupComplete\n\r")
  /* update CSP context power state */
  CSP_Context.power_state = CSP_LOW_POWER_INACTIVE;

  /* update data cache power state */
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_POWER_STATUS, (void *)&dc_power_status,
                    sizeof(dc_cellular_power_status_t));
  dc_power_status.power_state = DC_POWER_LOWPOWER_INACTIVE;
  (void)dc_com_write(&dc_com_db, DC_CELLULAR_POWER_STATUS, (void *)&dc_power_status,
                     sizeof(dc_cellular_power_status_t));

  PRINT_FORCE("++++++++++++++++ power state %s\n\r", CSP_power_state_name[CSP_Context.power_state])
}

/**
  * @brief  Reset low power status data
  * @param  none
  * @retval none
  */
void CSP_ResetPowerStatus(void)
{
  dc_cellular_power_status_t dc_power_status;

  PRINT_CELLULAR_SERVICE("CST: CSP_ResetPowerStatus\n\r")

  /* Reset low power status data in data cache */
  /* Read low power status from data cache */
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_POWER_STATUS, (void *)&dc_power_status,
                    sizeof(dc_cellular_power_status_t));
  /* reset status data */
  dc_power_status.power_state = DC_POWER_LOWPOWER_INACTIVE;
  dc_power_status.nwk_periodic_TAU = 0U;
  dc_power_status.nwk_active_time = 0U;
  /* write new status data to data cache */
  (void)dc_com_write(&dc_com_db, DC_CELLULAR_POWER_STATUS, (void *)&dc_power_status,
                     sizeof(dc_cellular_power_status_t));
}

/**
  * @brief  CSP call power mngt
  * @param  none
  * @retval error code
  */
static void CSP_ArmTimeout(uint32_t timeout)
{
  PRINT_CELLULAR_SERVICE("CST: CSP_ArmTimeout\n\r")
  /* On sleep request, arm timeout to be protected if modem do not answet to sleep request */
  (void)rtosalTimerStart(CSP_timeout_timer_handle, timeout);
}

/**
  * @brief  CSP stop timer
  * @param  none
  * @retval error code
  */
void CSP_StopTimeout(void)
{
  PRINT_CELLULAR_SERVICE("CST: CSP_StopTimeout\n\r")
  /* Stop timer if modem answered to sleep request */
  (void)rtosalTimerStop(CSP_timeout_timer_handle);
}

/**
  * @brief  enter in low power mode
  * @note  called by cellular service task automaton
  * @param  none
  * @retval error code
  */
void CSP_SleepRequest(uint32_t timeout)
{
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  CS_Status_t cs_status ;
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
  PRINT_CELLULAR_SERVICE("CST: CSP_SleepRequest\n\r")

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  /* stop data mode (no data can be transmit un low power mode) */
  PRINT_CELLULAR_SERVICE("CST: osCDS_suspend_data()\n\r")
  osCCS_get_wait_cs_resource();
  cs_status = osCDS_suspend_data();
  osCCS_get_release_cs_resource();
  if (cs_status == CS_OK)
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
  {
    /* arm timeout to protect from modem sleep request no response */
    CSP_ArmTimeout(timeout);
    /* Send sleep request to modem */
    PRINT_CELLULAR_SERVICE("CST: osCS_SleepRequest()\n\r")
    (void)osCS_SleepRequest();
    /* send sleep request to STM32 */
    STM32_SleepRequest();
  }
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  else
  {
    /* If error while stop data mode, send POWER_SLEEP_ABORT_EVENT to automaton */
    CST_send_message(CST_MESSAGE_CMD, CST_POWER_SLEEP_ABORT_EVENT);
  }
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
}

/**
  * @brief  Prepare to enter in low power mode
  * @note  called by cellular service task automaton
  * @param  none
  * @retval none
  */
void CSP_DataIdleManagment(void)
{
  PRINT_CELLULAR_SERVICE("CST: CSP_DataIdleManagment\n\r")

  if (CSP_Context.target_power_state == CSP_LOW_POWER_ACTIVE)
  {
    /* No low power activation is currently on going and low power is not established */
    /* Update CSP context power state */
    CSP_Context.power_state = CSP_LOW_POWER_ON_GOING;
    PRINT_FORCE("CST: power state %s\n\r", CSP_power_state_name[CSP_Context.power_state])

    switch (csp_dc_power_config.power_mode)
    {
      case CA_POWER_RUN_REAL_TIME:
        CST_set_state(CST_MODEM_POWER_DATA_IDLE_STATE);
        break;

      case CA_POWER_RUN_INTERACTIVE_0:
      case CA_POWER_RUN_INTERACTIVE_1:
      case CA_POWER_RUN_INTERACTIVE_2:
      case CA_POWER_RUN_INTERACTIVE_3:
        CST_set_state(CST_MODEM_POWER_DATA_IDLE_STATE);
        CSP_SleepRequest(csp_dc_power_config.sleep_request_timeout);
        break;

      case CA_POWER_IDLE:
        CST_set_state(CST_MODEM_POWER_DATA_IDLE_STATE);
        CSP_SleepRequest(csp_dc_power_config.sleep_request_timeout);
        break;

      case CA_POWER_IDLE_LP:
        CST_set_state(CST_MODEM_POWER_DATA_IDLE_STATE);
        CSP_SleepRequest(csp_dc_power_config.sleep_request_timeout);
        break;

      case CA_POWER_LP:
        CST_set_state(CST_MODEM_POWER_DATA_IDLE_STATE);
        CSP_SleepRequest(csp_dc_power_config.sleep_request_timeout);
        break;

      case CA_POWER_ULP:
        CST_set_state(CST_MODEM_POWER_DATA_IDLE_STATE);
        CSP_SleepRequest(csp_dc_power_config.sleep_request_timeout);
        break;

      default:
        /* Nothing to do */
        __NOP();
        break;

    }
  }
  else
  {
    PRINT_FORCE("++++++++++++++++ No need to go to low power. Target state changed")
  }
}

/**
  * @brief  enter in low power mode request
  * @param  none
  * @retval error code
  */
CS_Status_t CSP_DataIdle(void)
{
  dc_cellular_power_status_t dc_power_status;

  PRINT_CELLULAR_SERVICE("CST: CSP_DataIdle\n\r")
  CS_Status_t status;
  status = CS_OK;
  /* update target state. It is not the actual state but, the target to reach */
  PRINT_CELLULAR_SERVICE("CST: target power state CSP_LOW_POWER_ACTIVE\n\r")
  CSP_Context.target_power_state = CSP_LOW_POWER_ACTIVE;
  /* display actual power state */
  PRINT_CELLULAR_SERVICE("CST: power state %s\n\r", CSP_power_state_name[CSP_Context.power_state])

  (void)dc_com_read(&dc_com_db, DC_CELLULAR_POWER_CONFIG, (void *)&csp_dc_power_config,
                    sizeof(dc_cellular_power_config_t));
  if (csp_dc_power_config.rt_state == DC_SERVICE_ON)
  {
    if ((csp_dc_power_config.power_mode == CA_POWER_RUN_INTERACTIVE_0)
        || (csp_dc_power_config.power_mode == CA_POWER_RUN_INTERACTIVE_1)
        || (csp_dc_power_config.power_mode == CA_POWER_RUN_INTERACTIVE_2)
        || (csp_dc_power_config.power_mode == CA_POWER_RUN_INTERACTIVE_3)
        || (csp_dc_power_config.power_mode == CA_POWER_IDLE)
        || (csp_dc_power_config.power_mode == CA_POWER_IDLE_LP)
        || (csp_dc_power_config.power_mode == CA_POWER_LP)
        || (csp_dc_power_config.power_mode == CA_POWER_ULP))
    {
      /*      mutual exclusion  */
      /*      (void)rtosalMutexAcquire(CSP_mutex, RTOSAL_WAIT_FOREVER);  */
      if (CSP_Context.power_state == CSP_LOW_POWER_INACTIVE)
      {
        /* update power state in data cache */
        (void)dc_com_read(&dc_com_db, DC_CELLULAR_POWER_STATUS, (void *)&dc_power_status,
                          sizeof(dc_cellular_power_status_t));
        dc_power_status.power_state = DC_POWER_LOWPOWER_ONGOING;
        (void)dc_com_write(&dc_com_db, DC_CELLULAR_POWER_STATUS, (void *)&dc_power_status,
                           sizeof(dc_cellular_power_status_t));

        /* send message POWER_SLEEP_REQUEST_EVENT to automaton */
        CST_send_message(CST_MESSAGE_CMD, CST_POWER_SLEEP_REQUEST_EVENT);
      }
      else
      {
        /* A low power activation or low power is already active. Raise an error. This case should not occur */
        status  = CS_ERROR;
      }
    }
  }
  else
  {
    /* Data in data cache is not relevant, raise an error. This case should never occur */
    status  = CS_ERROR;
  }

  return status;
}

/**
  * @brief  enter in low power mode request
  * @param  none
  * @retval error code
  */
CS_Status_t CSP_CSIdle(void)
{
  PRINT_CELLULAR_SERVICE("CST: CSP_CSIdle\n\r")
  CS_Status_t status;
  status = CS_OK;

  if (CSP_Context.target_power_state == CSP_LOW_POWER_ACTIVE)
  {
    status = CSP_DataIdle();
  }
  return status;
}

/**
  * @brief  Cancel ongoing sleep request. Action to be done between CS_sleep_request and CS_sleepComplete
  * @param  none
  * @retval none
  */
void CSP_SleepCancel(void)
{
  PRINT_CELLULAR_SERVICE("CST: osCS_SleepCancel()\n\r")
  (void)osCS_SleepCancel();
  CSP_WakeupComplete();
  CST_set_state(CST_MODEM_DATA_READY_STATE);
}

/**
  * @brief  Update all needed data once sleep mode is enable
  * @param  none
  * @retval error code
  */
void CSP_SleepComplete(void)
{
  dc_cellular_power_status_t dc_power_status;

  PRINT_CELLULAR_SERVICE("CST: CSP_SleepComplete\n\r")
  if (CSP_Context.power_state == CSP_LOW_POWER_ON_GOING)
  {
    /* update data only if actual state is LOW_POWER_ON_GOING. Any other state should be impossible */
    /* Stop timeout timer that is armed to protect against modem not going to low power */
    (void)rtosalTimerStop(CSP_timeout_timer_handle);
    /* Complete sleep procedure at modem side*/
    PRINT_CELLULAR_SERVICE("CST: osCS_SleepComplete()\n\r")
    (void)osCS_SleepComplete();
    /* Update CSP context data */
    CSP_Context.power_state = CSP_LOW_POWER_ACTIVE;
    /* update data cache */
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_POWER_STATUS, (void *)&dc_power_status,
                      sizeof(dc_cellular_power_status_t));
    dc_power_status.power_state = DC_POWER_IN_LOWPOWER;
    (void)dc_com_write(&dc_com_db, DC_CELLULAR_POWER_STATUS, (void *)&dc_power_status,
                       sizeof(dc_cellular_power_status_t));

    PRINT_FORCE("++++++++++++++++ power state %s\n\r", CSP_power_state_name[CSP_Context.power_state])
  }
}

/**
  * @brief  exit from low power mode request
  * @param  none
  * @retval error code
  */

CS_Status_t CSP_DataWakeup(CS_wakeup_origin_t wakeup_origin)
{
  CS_Status_t status;

  /* Change CSP contest target state */
  PRINT_CELLULAR_SERVICE("CST: CSP_DataWakeup\n\r")

  PRINT_CELLULAR_SERVICE("CST: target power state CSP_LOW_POWER_INACTIVE\n\r")
  CSP_Context.target_power_state = CSP_LOW_POWER_INACTIVE;

  /*      mutual exclusion  */
  /*  (void)rtosalMutexRelease(CSP_mutex); */
  if ((CSP_Context.power_state == CSP_LOW_POWER_ACTIVE) ||
      (CSP_Context.power_state == CSP_LOW_POWER_ON_GOING))
  {
    /* Wake up process only if low power is on going or active */

    /* Wakeup only if LOW POWER is on going or active */
    STM32_Wakeup();

    if (wakeup_origin == HOST_WAKEUP)
    {
      /* wakeup request from host, that is, no need to wake up the host */
      PRINT_CELLULAR_SERVICE("CST: --> host wakeup\n\r")
      if (CSP_Context.power_state == CSP_LOW_POWER_ON_GOING)
      {
        /* If low power is ongoing, just cancel it */
        PRINT_CELLULAR_SERVICE("CST: --> cancel on going low power\n\r")
        /* stop the modem sleep request protection timer (to be sure it won't fire after) */
        CSP_StopTimeout();
        CSP_SleepCancel();
      }

      if (CSP_Context.power_state == CSP_LOW_POWER_ACTIVE)
      {
        /* If low power is active, wake up the modem */
        PRINT_CELLULAR_SERVICE("CST: --> Wake up established low power\n\r")
        CSP_StopTimeout();
        PRINT_CELLULAR_SERVICE("CST: osCS_PowerWakeup()\n\r")
        (void)osCS_PowerWakeup(HOST_WAKEUP);
      }
    }
    else
    {
      /* wakeup request from modem, that is, no need to wake up the modem */
      PRINT_CELLULAR_SERVICE("CST: --> modem wakeup\n\r")
      CSP_StopTimeout();
      /* stop the modem sleep request protection timer (to be sure it won't fire after) */
      CSP_StopTimeout();
      PRINT_CELLULAR_SERVICE("CST: osCS_PowerWakeup()\n\r")
      (void)osCS_PowerWakeup(MODEM_WAKEUP);
    }
  }
  else
  {
    PRINT_CELLULAR_SERVICE("CST: --> nothing to do, already wake up\n\r")
  }

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  /* resume data mode */
  PRINT_CELLULAR_SERVICE("CST: osCDS_resume_data()\n\r")
  osCCS_get_wait_cs_resource();
  status = osCDS_resume_data();
  osCCS_get_release_cs_resource();
#else
  status = CS_OK;
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */

  if ((CSP_Context.power_state == CSP_LOW_POWER_ACTIVE) ||
      (CSP_Context.power_state == CSP_LOW_POWER_ON_GOING))
  {
    /* Set needed variables to the values corresponding to low power inactive */
    CSP_WakeupComplete();
    CST_set_state(CST_MODEM_DATA_READY_STATE);
  }

  return status;
}

/**
  * @brief  set power config in cellular service structure, and send to the modem
  * @param  none
  * @retval error code
  */

void CSP_SetPowerConfig(void)
{
  CS_set_power_config_t cs_power_config;
  PRINT_CELLULAR_SERVICE("CST: CSP_SetPowerConfig\n\r")
  if (CSP_Context.power_state != CSP_LOW_POWER_DISABLED)
  {
    /* Setup only if low power is enabled or ongoing */
    /* Read dada cache data to get the low power values to send to the modem */
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_POWER_CONFIG, (void *)&csp_dc_power_config,
                      sizeof(dc_cellular_power_config_t));
    if ((csp_dc_power_config.rt_state == DC_SERVICE_ON) && (csp_dc_power_config.power_cmd == CA_POWER_CMD_SETTING))
    {
      /* Continue if data cache data are relevant */
      if (csp_dc_power_config.psm_present == true)
      {
        /* Copy PSM values */
        cs_power_config.psm_present              = CS_TRUE;
        cs_power_config.psm.req_periodic_RAU     = csp_dc_power_config.psm.req_periodic_RAU;
        cs_power_config.psm.req_GPRS_READY_timer = csp_dc_power_config.psm.req_GPRS_READY_timer;
        cs_power_config.psm.req_periodic_TAU     = csp_dc_power_config.psm.req_periodic_TAU;
        cs_power_config.psm.req_active_time      = csp_dc_power_config.psm.req_active_time;
      }
      else
      {
        /* If PSM values are not present, inform the modem */
        cs_power_config.psm_present = CS_FALSE;
      }

      if (csp_dc_power_config.edrx_present == true)
      {
        /* copy eDRX values */
        cs_power_config.edrx_present      = CS_TRUE;
        cs_power_config.edrx.act_type     = (uint8_t)csp_dc_power_config.edrx.act_type;
        cs_power_config.edrx.req_value    = csp_dc_power_config.edrx.req_value;
      }
      else
      {
        /* If eDRX values are not preesent, inform the modem */
        cs_power_config.edrx_present = CS_FALSE;
      }

      switch (csp_dc_power_config.power_mode)
      {
        /* depending on the power mode value, set or unset PSM or eDRX */
        case CA_POWER_RUN_REAL_TIME:
          /*  eDRX disable */
          cs_power_config.edrx_mode = EDRX_MODE_DISABLE;
          /*  PSM disable */
          cs_power_config.psm_mode  = PSM_MODE_DISABLE;
          CSP_ResetPowerStatus();
          /* Send power config data to modem */
          PRINT_CELLULAR_SERVICE("CST: osCS_SetPowerConfig()\n\r")
          (void)osCS_SetPowerConfig(&cs_power_config);
          break;

        case CA_POWER_RUN_INTERACTIVE_0:
          /*  eDRX disable */
          cs_power_config.edrx_mode = EDRX_MODE_DISABLE;
          /*  PSM disable */
          cs_power_config.psm_mode  = PSM_MODE_DISABLE;
          CSP_ResetPowerStatus();
          /* Send power config data to modem */
          PRINT_CELLULAR_SERVICE("CST: osCS_SetPowerConfig()\n\r")
          (void)osCS_SetPowerConfig(&cs_power_config);
          break;

        case CA_POWER_RUN_INTERACTIVE_1:
          /*  eDRX disable */
          cs_power_config.edrx_mode = EDRX_MODE_DISABLE;
          /*  PSM enable */
          cs_power_config.psm_mode  = PSM_MODE_ENABLE;
          /* Send power config data to modem */
          PRINT_CELLULAR_SERVICE("CST: osCS_SetPowerConfig()\n\r")
          (void)osCS_SetPowerConfig(&cs_power_config);
          break;

        case CA_POWER_RUN_INTERACTIVE_2:
          /*  eDRX enable */
          cs_power_config.edrx_mode = PSM_MODE_ENABLE;
          /*  PSM enable */
          cs_power_config.psm_mode  = PSM_MODE_ENABLE;
          /* Send power config data to modem */
          PRINT_CELLULAR_SERVICE("CST: osCS_SetPowerConfig()\n\r")
          (void)osCS_SetPowerConfig(&cs_power_config);
          break;

        case CA_POWER_RUN_INTERACTIVE_3:
          /*  eDRX enable */
          cs_power_config.edrx_mode = PSM_MODE_ENABLE;
          /*  PSM disable */
          cs_power_config.psm_mode  = EDRX_MODE_DISABLE;
          CSP_ResetPowerStatus();
          /* Send power config data to modem */
          PRINT_CELLULAR_SERVICE("CST: osCS_SetPowerConfig()\n\r")
          (void)osCS_SetPowerConfig(&cs_power_config);
          break;

        case CA_POWER_IDLE:
          /*  eDRX disable */
          cs_power_config.edrx_mode = EDRX_MODE_DISABLE;
          /*  PSM disable */
          cs_power_config.psm_mode  = PSM_MODE_DISABLE;
          CSP_ResetPowerStatus();
          /* Send power config data to modem */
          PRINT_CELLULAR_SERVICE("CST: osCS_SetPowerConfig()\n\r")
          (void)osCS_SetPowerConfig(&cs_power_config);
          break;

        case CA_POWER_IDLE_LP:
          /*  eDRX enable */
          cs_power_config.edrx_mode = EDRX_MODE_ENABLE;
          /*  PSM disable */
          cs_power_config.psm_mode  = PSM_MODE_DISABLE;
          CSP_ResetPowerStatus();
          /* Send power config data to modem */
          PRINT_CELLULAR_SERVICE("CST: osCS_SetPowerConfig()\n\r")
          (void)osCS_SetPowerConfig(&cs_power_config);
          break;

        case CA_POWER_LP:
          /*  eDRX enable */
          cs_power_config.edrx_mode = EDRX_MODE_ENABLE;
          /*  PSM enable */
          cs_power_config.psm_mode  = PSM_MODE_ENABLE;
          /* Send powedr config data to modem */
          PRINT_CELLULAR_SERVICE("CST: osCS_SetPowerConfig()\n\r")
          (void)osCS_SetPowerConfig(&cs_power_config);
          break;

        case CA_POWER_ULP:
          /*  eDRX enable */
          cs_power_config.edrx_mode = EDRX_MODE_ENABLE;
          /*  PSM enable */
          cs_power_config.psm_mode  = PSM_MODE_ENABLE;
          /* Send power config data to modem */
          PRINT_CELLULAR_SERVICE("CST: osCS_SetPowerConfig()\n\r")
          (void)osCS_SetPowerConfig(&cs_power_config);
          break;

        default:
          /* Nothing to do */
          __NOP();
          break;
      }
    }
  }
}

/**
  * @brief  CS power feature init
  * @param  none
  * @retval error code
  */

void CSP_Init(void)
{
  /* register all cellular entries of Data Cache */
  CSP_Context.power_state = CSP_LOW_POWER_DISABLED;

  PRINT_CELLULAR_SERVICE("CST: CSP_Init\n\r")

  /* register default values in data cache */
  /* Note: these values can be overloaded by application between cellular_init()
        and cellula_start() calls */
  csp_dc_power_config.rt_state                  = DC_SERVICE_ON;
  csp_dc_power_config.power_cmd                 = CA_POWER_CMD_INIT;
  csp_dc_power_config.power_mode                = DC_POWER_MODE_DEFAULT;
  csp_dc_power_config.sleep_request_timeout     = DC_POWER_SLEEP_REQUEST_TIMEOUT_DEFAULT;

  csp_dc_power_config.psm_present               = true;
  csp_dc_power_config.psm.req_periodic_RAU      = DC_POWER_PSM_REQ_PERIODIC_RAU_DEFAULT;
  csp_dc_power_config.psm.req_GPRS_READY_timer  = DC_POWER_PSM_REQ_GPRS_READY_TIMER_DEFAULT;
  csp_dc_power_config.psm.req_periodic_TAU      = DC_POWER_PSM_REQ_PERIODIC_TAU_DEFAULT ;
  csp_dc_power_config.psm.req_active_time       = DC_POWER_PSM_REQ_ACTIVE_TIMER_DEFAULT ;

  csp_dc_power_config.edrx_present              = true;
  csp_dc_power_config.edrx.act_type             = DC_POWER_EDRX_ACT_TYPE_DEFAULT;
  csp_dc_power_config.edrx.req_value            = DC_POWER_EDRX_REQ_VALUE_DEFAULT;

  (void)dc_com_write(&dc_com_db, DC_CELLULAR_POWER_CONFIG, (void *)&csp_dc_power_config,
                     sizeof(dc_cellular_power_config_t));
}


/**
  * @brief  initializes power config (set default values)
  * @param  none
  * @retval error code
  */

void CSP_InitPowerConfig(void)
{
  CS_init_power_config_t cs_power_config;
  CS_Status_t status;

  PRINT_CELLULAR_SERVICE("CST: CSP_InitPowerConfig\n\r")

  /* Read power config data from data cache */
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_POWER_CONFIG, (void *)&csp_dc_power_config,
                    sizeof(dc_cellular_power_config_t));
  if ((csp_dc_power_config.rt_state != DC_SERVICE_ON)
      || (csp_dc_power_config.psm_present  != true)
      || (csp_dc_power_config.edrx_present != true))
  {
    /* Neither PSM or eRDX is present. Reset the low power status values in data cache */
    CSP_Context.power_state = CSP_LOW_POWER_DISABLED;
    CSP_ResetPowerStatus();
  }
  else
  {
    /* PSM or eDRX is present, initialize data for the modem */
    cs_power_config.low_power_enable         = CS_TRUE;
    cs_power_config.psm.req_periodic_RAU     = csp_dc_power_config.psm.req_periodic_RAU;
    cs_power_config.psm.req_GPRS_READY_timer = csp_dc_power_config.psm.req_GPRS_READY_timer;
    cs_power_config.psm.req_periodic_TAU     = csp_dc_power_config.psm.req_periodic_TAU;
    cs_power_config.psm.req_active_time      = csp_dc_power_config.psm.req_active_time;
    cs_power_config.edrx.act_type            = (uint8_t)csp_dc_power_config.edrx.act_type;
    cs_power_config.edrx.req_value           = csp_dc_power_config.edrx.req_value;
    /* Send init power config data to the modem */
    PRINT_CELLULAR_SERVICE("CST: osCS_InitPowerConfig()\n\r")
    status = osCS_InitPowerConfig(&cs_power_config, CST_cellular_power_status_callback);
    if (status == CS_OK)
    {
      /* reset context data*/
      CSP_Context.power_state = CSP_LOW_POWER_INACTIVE;
      CSP_Context.target_power_state = CSP_LOW_POWER_INACTIVE;
      /* reset low power status values in data cache */
      CSP_ResetPowerStatus();
    }
  }
}

/**
  * @brief  CS power feature start
  * @param  none
  * @retval error code
  */

void CSP_Start(void)
{
  PRINT_CELLULAR_SERVICE("CST: CSP_Start\n\r")
#if (USE_CMD_CONSOLE == 1)
  CMD_Declare(CSP_cmd_label, CSP_cmd, (uint8_t *)"power management");
#endif  /*  (USE_CMD_CONSOLE == 1) */
  /* init timer for timeout management */
  /* creates timer */
  CSP_timeout_timer_handle = rtosalTimerNew((const rtosal_char_t *)"CSP_TIM_TIMEOUT",
                                            (os_ptimer)CSP_TimeoutTimerCallback, osTimerOnce, NULL);
}
#endif  /* (USE_LOW_POWER == 1) */

/**
  * @brief  CS power get target power state
  * @param  none
  * @retval The actual value of the targeted power state
  */
#if (USE_LOW_POWER == 1)
CSP_PowerState_t CSP_GetTargetPowerState(void)
{
  return CSP_Context.target_power_state;
}
#endif /* (USE_LOW_POWER == 1) */

