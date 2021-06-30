/**
  ******************************************************************************
  * @file    cellular_service_power.c
  * @author  MCD Application Team
  * @brief   This file defines functions for Cellular Service Power
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
  CSP_PowerState_t power_state;
  CSP_PowerState_t target_power_state;
}  CSP_Context_t;

/* Private variables ---------------------------------------------------------*/
static osTimerId         CSP_timeout_timer_handle;
static dc_cellular_power_config_t csp_dc_power_config;
static CSP_Context_t     CSP_Context;
static const uint8_t *CSP_power_state_name[] =
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

static uint8_t *CSP_cmd_label = ((uint8_t *)"csp");


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
static void CSP_TimeoutTimerCallback(void *argument);
static void CSP_ArmTimeout(uint32_t timeout);
static void CSP_SleepRequest(uint32_t timeout);

#if (USE_CMD_CONSOLE == 1)
static void CSP_HelpCmd(void);
static void CSP_get_config(void);
static cmd_status_t CSP_cmd(uint8_t *cmd_line_p);
#endif  /* (USE_CMD_CONSOLE == 1) */

/* Private function Definition -----------------------------------------------*/


/* ============================================================ */
/* ==== STM32 STUB BEGIN ======= */
/* ============================================================ */
static void STM32_SleepRequest(void);
static void STM32_Wakeup(void);

static void STM32_SleepRequest(void)
{
  PRINT_CELLULAR_SERVICE("STM32_SleepRequest\n\r")
}
static void STM32_Wakeup(void)
{
  PRINT_CELLULAR_SERVICE("STM32_Wakeup\n\r")
}
/* ============================================================ */
/* ==== STM32 STUB END ======= */
/* ============================================================ */

#if (USE_CMD_CONSOLE == 1)
/**
  * @brief  Help command management
  * @param  none
  * @retval none
  */

static void CSP_HelpCmd(void)
{
  CMD_print_help(CSP_cmd_label);

  PRINT_FORCE("%s help\n\r", (CRC_CHAR_t *)CSP_cmd_label)
  PRINT_FORCE("%s state   (Displays the current power state)\n\r", CSP_cmd_label)
  PRINT_FORCE("%s mode [runrealtime|runinteractive|idle|ildllp|lp|ulp] (select power mode)\n\r", CSP_cmd_label)
  PRINT_FORCE("%s idle  (enter in low power)\n\r", CSP_cmd_label)
  PRINT_FORCE("%s wakeup  (leave low power)\n\r", CSP_cmd_label)

  PRINT_FORCE("\n\r")
  PRINT_FORCE("PSM and eDRX configuration can be modified using '%s config set'\n\r", CSP_cmd_label)
  PRINT_FORCE("and '%s config send' commands.\n\r", CSP_cmd_label)
  PRINT_FORCE("Update configuration is performed in two steps:\n\r")
  PRINT_FORCE("- 1st step: set the configuration parameters using the following command:\n\r");
  PRINT_FORCE("%s config set psmrau|psmgprstimer|psmtau|psmactivetimer|edrxacttype|edrxvalue ", CSP_cmd_label)
  PRINT_FORCE(" <hexa value prefixed by '0x'>  (set power config value)\n\r")
  PRINT_FORCE("  or\n\r")
  PRINT_FORCE("%s config set edrxacttype  NOT_USED|EC_GSM_IOT|GSM|UTRAN|UTRAN_WB_S1|UTRAN_NB_S1 ",
              CSP_cmd_label)
  PRINT_FORCE("(set edrx config value)\n\r")
  PRINT_FORCE("- 2nd step: send the new configuration to the modem\n\r");
  PRINT_FORCE("%s config send\n\r", (CRC_CHAR_t *)CSP_cmd_label)
  PRINT_FORCE("\n\r");
  PRINT_FORCE("Note: 'psmrau psmgprstimer psmtau psmactivetimer edrxacttype edrxvalue' parameters\n\r")
  PRINT_FORCE("      have a default values.\n\r")
  PRINT_FORCE("      Current values of these parameters can be read using '%s config get' command\n\r", CSP_cmd_label)
  PRINT_FORCE("\n\r");


  PRINT_FORCE("\n\r");
}

static void CSP_get_config(void)
{
  PRINT_FORCE("----------\n\r")
  PRINT_FORCE("PSM config\n\r")
  PRINT_FORCE("----------\n\r")
  PRINT_FORCE("periodic_RAU     0x%02x\n\r", csp_cmd_power_config.psm.req_periodic_RAU)
  PRINT_FORCE("GPRS ready timer 0x%02x\n\r", csp_cmd_power_config.psm.req_GPRS_READY_timer)
  PRINT_FORCE("periodic TAU     0x%02x\n\r", csp_cmd_power_config.psm.req_periodic_TAU)
  PRINT_FORCE("active time      0x%02x\n\r", csp_cmd_power_config.psm.req_active_time)
  PRINT_FORCE("\n\r")
  PRINT_FORCE("-----------\n\r")
  PRINT_FORCE("eDRX config\n\r")
  PRINT_FORCE("-----------\n\r")
  PRINT_FORCE("type %s\n\r", CSP_edrx_act_type_name[csp_cmd_power_config.edrx.act_type])
  PRINT_FORCE("value 0x%02x\n\r", csp_cmd_power_config.edrx.req_value)
}
/**
  * @brief  Cellular Sercice Power command line management
  * @param  cmd_line_p  command line
  * @retval cmd_status_t command result
  */

static cmd_status_t CSP_cmd(uint8_t *cmd_line_p)
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

  cmd_status_t cmd_status ;
  cmd_status = CMD_OK;

  PRINT_FORCE("\n\r")

  cmd_p = (uint8_t *)strtok((CRC_CHAR_t *)cmd_line_p, " \t");

  /* verify that it is a cst command */
  if (cmd_p != NULL)
  {
    if (memcmp((CRC_CHAR_t *)cmd_p,
               (CRC_CHAR_t *)CSP_cmd_label,
               crs_strlen(cmd_p))
        == 0)
    {
      /* parameters parsing                     */
      for (argc = 0U ; argc < CSP_CMD_PARAM_MAX ; argc++)
      {
        argv_p[argc] = (uint8_t *)strtok(NULL, " \t");
        if (argv_p[argc] == NULL)
        {
          /* end of argument list reached */
          break;
        }
      }

      if (argc == 0U)
      {
        /* no argument: displays help */
        CSP_HelpCmd();
      }
      else if (memcmp((CRC_CHAR_t *)argv_p[0],  "help",  crs_strlen(argv_p[0])) == 0)
      {
        /* help command: displays help */
        CSP_HelpCmd();
      }
      else if (memcmp((CRC_CHAR_t *)argv_p[0], "config", crs_strlen(argv_p[0])) == 0)
      {
        /* 'csp config' command */
        if (argc == 1U)
        {
          CSP_get_config();
        }
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
                csp_cmd_power_config.edrx.act_type = CA_EIDRX_ACT_NOT_USED;
              }
              else if (memcmp((CRC_CHAR_t *)argv_p[3], "EC_GSM_IOT", crs_strlen(argv_p[3])) == 0)
              {
                csp_cmd_power_config.edrx.act_type = CA_EIDRX_ACT_EC_GSM_IOT;
              }
              else if (memcmp((CRC_CHAR_t *)argv_p[3], "GSM", crs_strlen(argv_p[3])) == 0)
              {
                csp_cmd_power_config.edrx.act_type = CA_EIDRX_ACT_GSM;
              }
              else if (memcmp((CRC_CHAR_t *)argv_p[3], "UTRAN_WB_S1", crs_strlen(argv_p[3])) == 0)
              {
                csp_cmd_power_config.edrx.act_type = CA_EIDRX_ACT_E_UTRAN_WBS1;
              }
              else if (memcmp((CRC_CHAR_t *)argv_p[3], "UTRAN_NB_S1", crs_strlen(argv_p[3])) == 0)
              {
                csp_cmd_power_config.edrx.act_type = CA_EDRX_ACT_E_UTRAN_NBS1;
              }
              else if (memcmp((CRC_CHAR_t *)argv_p[3], "UTRAN", crs_strlen(argv_p[3])) == 0)
              {
                csp_cmd_power_config.edrx.act_type = CA_EIDRX_ACT_UTRAN;
              }
              else
              {
                /* 'csp config set edrxacttype ...' command : wrong value */
                PRINT_FORCE("csp config set edrxacttype wrong value %s\n\r", argv_p[3])
                PRINT_FORCE("Usage:\n\r")
                CSP_HelpCmd();
                cmd_status = CMD_SYNTAX_ERROR;
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
              cmd_status = CMD_SYNTAX_ERROR;
            }
          }
          else
          {
            /* 'csp config   ...' command : wrong parameters */
            PRINT_FORCE("Wrong syntax. Usage:\n\r")
            CSP_HelpCmd();
            cmd_status = CMD_SYNTAX_ERROR;
          }
        }
        else if (memcmp((CRC_CHAR_t *)argv_p[1], "get", crs_strlen(argv_p[1])) == 0)
        {
          /* 'csp config get' command */
          CSP_get_config();
        }
        else if (memcmp((CRC_CHAR_t *)argv_p[1], "send", crs_strlen(argv_p[1])) == 0)
        {
          /* 'csp config send' command */
          (void)dc_com_read(&dc_com_db, DC_CELLULAR_POWER_CONFIG, (void *)&csp_cmd_dc_power_config,
                            sizeof(dc_cellular_power_config_t));
          csp_cmd_dc_power_config.power_cmd                 = CA_POWER_CMD_SETTING;
          csp_cmd_dc_power_config.psm_present               = true;
          csp_cmd_dc_power_config.psm.req_periodic_RAU      = csp_cmd_power_config.psm.req_periodic_RAU;
          csp_cmd_dc_power_config.psm.req_GPRS_READY_timer  = csp_cmd_power_config.psm.req_GPRS_READY_timer;
          csp_cmd_dc_power_config.psm.req_periodic_TAU      = csp_cmd_power_config.psm.req_periodic_TAU;
          csp_cmd_dc_power_config.psm.req_active_time       = csp_cmd_power_config.psm.req_active_time;

          csp_cmd_dc_power_config.edrx_present              = true;
          csp_cmd_dc_power_config.edrx.act_type             = csp_cmd_power_config.edrx.act_type;
          csp_cmd_dc_power_config.edrx.req_value            = csp_cmd_power_config.edrx.req_value;

          (void)dc_com_write(&dc_com_db, DC_CELLULAR_POWER_CONFIG, (void *)&csp_cmd_dc_power_config,
                             sizeof(dc_cellular_power_config_t));

          PRINT_FORCE("New config\n\r")
          CSP_get_config();
        }
        else
        {
          PRINT_FORCE("Wrong syntax. Usage:\n\r")
          CSP_HelpCmd();
          cmd_status = CMD_SYNTAX_ERROR;
        }
      }
      else if (memcmp((CRC_CHAR_t *)argv_p[0], "state", crs_strlen(argv_p[0])) == 0)
      {
        /* 'csp state' command */
        (void)dc_com_read(&dc_com_db, DC_CELLULAR_POWER_CONFIG, (void *)&csp_cmd_dc_power_config,
                          sizeof(dc_cellular_power_config_t));
        PRINT_FORCE("Current power mode: %s\n\r", CSP_power_mode_name[csp_cmd_dc_power_config.power_mode])

        PRINT_FORCE("power state %s\n\r", CSP_power_state_name[CSP_Context.power_state])
        PRINT_FORCE("----------\n\r")
        PRINT_FORCE("PSM config\n\r")
        PRINT_FORCE("----------\n\r")
        if (csp_cmd_dc_power_config.psm_present == true)
        {
          PRINT_FORCE("PSM config present\n\r")
          PRINT_FORCE("periodic_RAU     0x%02x\n\r", csp_cmd_dc_power_config.psm.req_periodic_RAU)
          PRINT_FORCE("GPRS ready timer 0x%02x\n\r", csp_cmd_dc_power_config.psm.req_GPRS_READY_timer)
          PRINT_FORCE("periodic TAU     0x%02x\n\r", csp_cmd_dc_power_config.psm.req_periodic_TAU)
          PRINT_FORCE("active time      0x%02x\n\r", csp_cmd_dc_power_config.psm.req_active_time)
        }
        else
        {
          PRINT_FORCE("PSM config not present\n\r")
        }
        PRINT_FORCE("\n\r")
        PRINT_FORCE("-----------\n\r")
        PRINT_FORCE("eDRX config\n\r")
        PRINT_FORCE("-----------\n\r")
        if (csp_cmd_dc_power_config.edrx_present == true)
        {
          PRINT_FORCE("periodic_RAU %s\n\r", CSP_edrx_act_type_name[csp_cmd_dc_power_config.edrx.act_type])
          PRINT_FORCE("periodic_RAU 0x%02x\n\r", csp_cmd_dc_power_config.edrx.req_value)
        }
        else
        {
          PRINT_FORCE("eDRX config not present\n\r")
        }
      }
      else if (memcmp((CRC_CHAR_t *)argv_p[0], "wakeup", crs_strlen(argv_p[0])) == 0)
      {
        /* 'csp wakeup' command */
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
        status = CSP_DataWakeup(HOST_WAKEUP);
        if (status == CELLULAR_OK)
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
            cmd_status = CMD_SYNTAX_ERROR;
          }
          (void)dc_com_write(&dc_com_db, DC_CELLULAR_POWER_CONFIG, (void *)&csp_cmd_dc_power_config,
                             sizeof(dc_cellular_power_config_t));

        }
        PRINT_FORCE("Current power mode: %s\n\r", CSP_power_mode_name[csp_cmd_dc_power_config.power_mode])

      }
      else if (memcmp((CRC_CHAR_t *)argv_p[0], "idle", crs_strlen(argv_p[0])) == 0)
      {
        /* 'csp idle' command */
        status = CSP_DataIdle();
        if (status == CELLULAR_OK)
        {
          PRINT_FORCE("Data idle OK\n\r")
        }
        else
        {
          PRINT_FORCE("Data idle FAIL\n\r")
        }
      }
      else
      {
        PRINT_FORCE("Wrong syntax. Usage:\n\r")
        CSP_HelpCmd();
        cmd_status = CMD_SYNTAX_ERROR;
      }
    }
  }
  return cmd_status;
}
#endif  /* (USE_CMD_CONSOLE == 1) */


/**
  * @brief  timer callback to check sleep activation
  * @param  argument - argument (not used)
  * @retval none
  */
static void CSP_TimeoutTimerCallback(void *argument)
{
  dc_cellular_power_status_t dc_power_status;

  UNUSED(argument);
  PRINT_CELLULAR_SERVICE("++++++++++++++++ CSP_TimeoutTimerCallback\n\r")

  (void)osCS_SleepCancel();

  CSP_Context.power_state = CSP_LOW_POWER_INACTIVE;
  PRINT_FORCE("++++++++++++++++ Call back - power state %s\n\r", CSP_power_state_name[CSP_Context.power_state])

  (void)dc_com_read(&dc_com_db, DC_CELLULAR_POWER_STATUS, (void *)&dc_power_status,
                    sizeof(dc_cellular_power_status_t));
  dc_power_status.power_state = DC_POWER_LOWPOWER_INACTIVE;
  (void)dc_com_write(&dc_com_db, DC_CELLULAR_POWER_STATUS, (void *)&dc_power_status,
                     sizeof(dc_cellular_power_status_t));

  CST_send_message(CST_MESSAGE_CS_EVENT, CST_POWER_SLEEP_TIMEOUT_EVENT);
}

/**
  * @brief  low power leaved
  * @param  none
  * @retval none
  */
void CSP_WakeupComplete(void)
{
  dc_cellular_power_status_t dc_power_status;

  PRINT_CELLULAR_SERVICE("++++++++++++++++ CSP_WakeupComplete\n\r")
  CSP_Context.power_state = CSP_LOW_POWER_INACTIVE;

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

  PRINT_CELLULAR_SERVICE("++++++++++++++++ CSP_ResetPowerStatus\n\r")

  (void)dc_com_read(&dc_com_db, DC_CELLULAR_POWER_STATUS, (void *)&dc_power_status,
                    sizeof(dc_cellular_power_status_t));

  dc_power_status.power_state = DC_POWER_LOWPOWER_INACTIVE;
  dc_power_status.nwk_periodic_TAU = 0U;
  dc_power_status.nwk_active_time = 0U;

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
  PRINT_CELLULAR_SERVICE("++++++++++++++++ CSP_ArmTimeout\n\r")
  (void)rtosalTimerStart(CSP_timeout_timer_handle, timeout);
}

/**
  * @brief  CSP stop timer
  * @param  none
  * @retval error code
  */
void CSP_StopTimeout(void)
{
  PRINT_CELLULAR_SERVICE("++++++++++++++++ CSP_StopTimeout\n\r")
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
  PRINT_CELLULAR_SERVICE("++++++++++++++++ CSP_SleepRequest\n\r")

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  cs_status = osCDS_suspend_data();
  if (cs_status == CELLULAR_OK)
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
  {
    CSP_ArmTimeout(timeout);
    (void)osCS_SleepRequest();
    STM32_SleepRequest();
  }
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  else
  {
    CST_send_message(CST_MESSAGE_CMD, CST_POWER_SLEEP_ABORT_EVENT);
  }
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
}

/**
  * @brief  enter in low power mode
  * @note  called by cellular service task automaton
  * @param  none
  * @retval error code
  */
void CSP_DataIdleManagment(void)
{
  PRINT_CELLULAR_SERVICE("++++++++++++++++ CSP_DataIdleManagment\n\r")

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


/**
  * @brief  enter in low power mode request
  * @param  none
  * @retval error code
  */
CS_Status_t CSP_DataIdle(void)
{
  dc_cellular_power_status_t dc_power_status;

  PRINT_CELLULAR_SERVICE("++++++++++++++++ CSP_DataIdle\n\r")
  CS_Status_t status;
  status = CELLULAR_OK;
  CSP_Context.target_power_state = CSP_LOW_POWER_ACTIVE;
  PRINT_CELLULAR_SERVICE("++++++++++++++++ power state %s\n\r", CSP_power_state_name[CSP_Context.power_state])

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
        CSP_Context.power_state = CSP_LOW_POWER_ON_GOING;
        PRINT_FORCE("++++++++++++++++ power state %s\n\r", CSP_power_state_name[CSP_Context.power_state])

        (void)dc_com_read(&dc_com_db, DC_CELLULAR_POWER_STATUS, (void *)&dc_power_status,
                          sizeof(dc_cellular_power_status_t));
        dc_power_status.power_state = DC_POWER_LOWPOWER_ONGOING;
        (void)dc_com_write(&dc_com_db, DC_CELLULAR_POWER_STATUS, (void *)&dc_power_status,
                           sizeof(dc_cellular_power_status_t));

        CST_send_message(CST_MESSAGE_CMD, CST_POWER_SLEEP_REQUEST_EVENT);
      }
      else
      {
        status  = CELLULAR_ERROR;
      }
    }
  }
  else
  {
    status  = CELLULAR_ERROR;
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
  PRINT_CELLULAR_SERVICE("++++++++++++++++ CSP_CSIdle\n\r")
  CS_Status_t status;
  status = CELLULAR_OK;

  if (CSP_Context.target_power_state == CSP_LOW_POWER_ACTIVE)
  {
    status = CSP_DataIdle();
  }
  return status;
}

/**
  * @brief  enter in low power mode request
  * @param  none
  * @retval error code
  */
void CSP_SleepComplete(void)
{
  dc_cellular_power_status_t dc_power_status;

  PRINT_CELLULAR_SERVICE("++++++++++++++++ CSP_SleepComplete\n\r")
  if (CSP_Context.power_state == CSP_LOW_POWER_ON_GOING)
  {
    (void)rtosalTimerStop(CSP_timeout_timer_handle);
    (void)osCS_SleepComplete();
    CSP_Context.power_state = CSP_LOW_POWER_ACTIVE;

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
  /*      mutual exclusion  */
  /*  (void)rtosalMutexRelease(CSP_mutex); */

  if ((CSP_Context.power_state == CSP_LOW_POWER_ACTIVE) ||
      (CSP_Context.power_state == CSP_LOW_POWER_ON_GOING))
  {
    STM32_Wakeup();

    if (wakeup_origin == HOST_WAKEUP)
    {
      PRINT_CELLULAR_SERVICE("++++++++++++++++ CSP_DataWakeup host wakeup\n\r")
      CST_send_message(CST_MESSAGE_CMD, CST_POWER_WAKEUP_EVENT);
      CSP_Context.target_power_state = CSP_LOW_POWER_INACTIVE;
      PRINT_CELLULAR_SERVICE("++++++++++++++++ power state %s\n\r", CSP_power_state_name[CSP_Context.power_state])
      while (CSP_Context.power_state == CSP_LOW_POWER_ON_GOING)
      {
        PRINT_CELLULAR_SERVICE("++++++++++++++++ wait for wakeup completion: CSP_LOW_POWER_ON_GOING  \n\r")
        (void)rtosalDelay(100) ;
      }
      CSP_StopTimeout();
      while (CSP_Context.power_state == CSP_LOW_POWER_ACTIVE)
      {
        PRINT_CELLULAR_SERVICE("++++++++++++++++ wait for wakeup completion LOW POWER \n\r")
        (void)rtosalDelay(100) ;
      }
    }
    else
    {
      PRINT_CELLULAR_SERVICE("++++++++++++++++ CSP_DataWakeup modem wakeup\n\r")
      CSP_StopTimeout();
      CST_send_message(CST_MESSAGE_CMD, CST_POWER_MODEM_WAKEUP_EVENT);
    }
  }

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  status = osCDS_resume_data();
#else
  status = CELLULAR_OK;
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
  return status;
}

/**
  * @brief  set power config
  * @param  none
  * @retval error code
  */

void CSP_SetPowerConfig(void)
{
  CS_set_power_config_t cs_power_config;
  PRINT_CELLULAR_SERVICE("++++++++++++++++ CSP_SetPowerConfig\n\r")
  if (CSP_Context.power_state != CSP_LOW_POWER_DISABLED)
  {
    (void)dc_com_read(&dc_com_db, DC_CELLULAR_POWER_CONFIG, (void *)&csp_dc_power_config,
                      sizeof(dc_cellular_power_config_t));
    if ((csp_dc_power_config.rt_state == DC_SERVICE_ON) && (csp_dc_power_config.power_cmd == CA_POWER_CMD_SETTING))
    {
      if (csp_dc_power_config.psm_present == true)
      {
        cs_power_config.psm_present              = CELLULAR_TRUE;
        cs_power_config.psm.req_periodic_RAU     = csp_dc_power_config.psm.req_periodic_RAU;
        cs_power_config.psm.req_GPRS_READY_timer = csp_dc_power_config.psm.req_GPRS_READY_timer;
        cs_power_config.psm.req_periodic_TAU     = csp_dc_power_config.psm.req_periodic_TAU;
        cs_power_config.psm.req_active_time      = csp_dc_power_config.psm.req_active_time;
      }
      else
      {
        cs_power_config.psm_present = CELLULAR_FALSE;
      }

      if (csp_dc_power_config.edrx_present == true)
      {
        cs_power_config.edrx_present      = CELLULAR_TRUE;
        cs_power_config.edrx.act_type     = (uint8_t)csp_dc_power_config.edrx.act_type;
        cs_power_config.edrx.req_value    = csp_dc_power_config.edrx.req_value;
      }
      else
      {
        cs_power_config.edrx_present = CELLULAR_FALSE;
      }

      switch (csp_dc_power_config.power_mode)
      {
        case CA_POWER_RUN_REAL_TIME:
          /*  eDRX disable */
          cs_power_config.edrx_mode = EDRX_MODE_DISABLE;
          /*  PSM disable */
          cs_power_config.psm_mode  = PSM_MODE_DISABLE;
          CSP_ResetPowerStatus();
          (void)osCS_SetPowerConfig(&cs_power_config);
          break;

        case CA_POWER_RUN_INTERACTIVE_0:
          /*  eDRX disable */
          cs_power_config.edrx_mode = EDRX_MODE_DISABLE;
          /*  PSM disable */
          cs_power_config.psm_mode  = PSM_MODE_DISABLE;
          CSP_ResetPowerStatus();
          (void)osCS_SetPowerConfig(&cs_power_config);
          break;

        case CA_POWER_RUN_INTERACTIVE_1:
          /*  eDRX disable */
          cs_power_config.edrx_mode = EDRX_MODE_DISABLE;
          /*  PSM enable */
          cs_power_config.psm_mode  = PSM_MODE_ENABLE;
          (void)osCS_SetPowerConfig(&cs_power_config);
          break;

        case CA_POWER_RUN_INTERACTIVE_2:
          /*  eDRX enable */
          cs_power_config.edrx_mode = PSM_MODE_ENABLE;
          /*  PSM enable */
          cs_power_config.psm_mode  = PSM_MODE_ENABLE;
          (void)osCS_SetPowerConfig(&cs_power_config);
          break;

        case CA_POWER_RUN_INTERACTIVE_3:
          /*  eDRX enable */
          cs_power_config.edrx_mode = PSM_MODE_ENABLE;
          /*  PSM disable */
          cs_power_config.psm_mode  = EDRX_MODE_DISABLE;
          CSP_ResetPowerStatus();
          (void)osCS_SetPowerConfig(&cs_power_config);
          break;

        case CA_POWER_IDLE:
          /*  eDRX disable */
          cs_power_config.edrx_mode = EDRX_MODE_DISABLE;
          /*  PSM disable */
          cs_power_config.psm_mode  = PSM_MODE_DISABLE;
          CSP_ResetPowerStatus();
          (void)osCS_SetPowerConfig(&cs_power_config);
          break;

        case CA_POWER_IDLE_LP:
          /*  eDRX enable */
          cs_power_config.edrx_mode = EDRX_MODE_ENABLE;
          /* set eDRX parameters*/

          /*  PSM disable */
          cs_power_config.psm_mode  = PSM_MODE_DISABLE;
          CSP_ResetPowerStatus();
          (void)osCS_SetPowerConfig(&cs_power_config);
          break;

        case CA_POWER_LP:
          /*  eDRX enable */
          cs_power_config.edrx_mode = EDRX_MODE_ENABLE;
          /* set eDRX parameters*/

          /*  PSM enable */
          cs_power_config.psm_mode  = PSM_MODE_ENABLE;
          /* set PSM parameters */

          (void)osCS_SetPowerConfig(&cs_power_config);
          break;

        case CA_POWER_ULP:
          /*  eDRX enable */
          cs_power_config.edrx_mode = EDRX_MODE_ENABLE;
          /* set eDRX parameters*/

          /*  PSM enable */
          cs_power_config.psm_mode  = PSM_MODE_ENABLE;
          /* set PSM parameters */

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

  PRINT_CELLULAR_SERVICE("++++++++++++++++ CSP_Init\n\r")

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

  PRINT_CELLULAR_SERVICE("++++++++++++++++ CSP_InitPowerConfig\n\r")

  (void)dc_com_read(&dc_com_db, DC_CELLULAR_POWER_CONFIG, (void *)&csp_dc_power_config,
                    sizeof(dc_cellular_power_config_t));
  if ((csp_dc_power_config.rt_state != DC_SERVICE_ON)
      || (csp_dc_power_config.psm_present  != true)
      || (csp_dc_power_config.edrx_present != true))
  {
    CSP_Context.power_state = CSP_LOW_POWER_DISABLED;
    CSP_ResetPowerStatus();
  }
  else
  {
    cs_power_config.low_power_enable         = CELLULAR_TRUE;
    cs_power_config.psm.req_periodic_RAU     = csp_dc_power_config.psm.req_periodic_RAU;
    cs_power_config.psm.req_GPRS_READY_timer = csp_dc_power_config.psm.req_GPRS_READY_timer;
    cs_power_config.psm.req_periodic_TAU     = csp_dc_power_config.psm.req_periodic_TAU;
    cs_power_config.psm.req_active_time      = csp_dc_power_config.psm.req_active_time;
    cs_power_config.edrx.act_type            = (uint8_t)csp_dc_power_config.edrx.act_type;
    cs_power_config.edrx.req_value           = csp_dc_power_config.edrx.req_value;
    status = osCS_InitPowerConfig(&cs_power_config, CST_cellular_power_status_callback);
    if (status == CELLULAR_OK)
    {
      CSP_Context.power_state = CSP_LOW_POWER_INACTIVE;
      CSP_Context.target_power_state = CSP_LOW_POWER_INACTIVE;
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
  PRINT_CELLULAR_SERVICE("++++++++++++++++ CSP_Start\n\r")
#if (USE_CMD_CONSOLE == 1)
  CMD_Declare(CSP_cmd_label, CSP_cmd, (uint8_t *)"power management");
#endif  /*  (USE_CMD_CONSOLE == 1) */
  /* init timer for timeout management */
  /* creates timer */
  CSP_timeout_timer_handle = rtosalTimerNew(NULL, (os_ptimer)CSP_TimeoutTimerCallback, osTimerOnce, NULL);
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

