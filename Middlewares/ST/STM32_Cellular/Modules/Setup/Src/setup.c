/**
  ******************************************************************************
  * @file    setup.c
  * @author  MCD Application Team
  * @brief   setup configuration management
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

#if (USE_DEFAULT_SETUP == 0)
#include "setup.h"
#include "cellular_runtime_standard.h"
#include "cellular_runtime_custom.h"
#include <stdlib.h>
#include <string.h>
#include "app_select.h"
#include "menu_utils.h"
#if (FEEPROM_UTILS_FLASH_USED == 1)
#include "feeprom_utils.h"
#endif  /* (FEEPROM_UTILS_FLASH_USED == 1) */
#include "time_date.h"

/* Private defines -----------------------------------------------------------*/
#define SETUP_NUMBER_MAX   ((uint16_t)SETUP_APPLI_MAX)
#define SETUP_APP_SELECT_LABEL "Setup configuration Menu"

/* Private macros ------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* list of actions required by setup menu */
typedef enum
{
  SETUP_ACTION_NONE = 0,
  SETUP_ACTION_UART,
  SETUP_ACTION_FEEPROM,
  SETUP_ACTION_ERASE,
  SETUP_ACTION_DUMP,
  SETUP_ACTION_HELP,
  SETUP_ACTION_QUIT
} setup_action_t;

/* list of configuration sources */
static int8_t *setup_source_string[MENU_SETUP_SOURCE_MAX] =
{
  (int8_t *)"NONE",
  (int8_t *)"UART",
  (int8_t *)"DEFAULT",
  (int8_t *)"FEEPROM"
};


/* setup application parameters */
typedef struct
{
  setup_appli_code_t code_appli;
  setup_appli_version_t version_appli;
  uint8_t         *label_appli;
  uint32_t (*setup_fnct)(void) ;
  void (*dump_fnct)(void) ;
  void (*help_fnct)(void) ;
  uint8_t        **default_config ;
  uint32_t           default_config_size ;
  uint8_t            config_done ;
} setup_params_t;


/* Private variables ---------------------------------------------------------*/

/* table of setup parameters for each application */
static setup_params_t setup_list[SETUP_NUMBER_MAX];

/* Number of application using setup */
static uint16_t setup_nb = 0U;

/* default configuration buffer */
static uint8_t setup_default_config[MENU_UTILS_SETUP_CONFIG_SIZE_MAX];

/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/


/* static function declarations */
static void setup_get_source(void);
static uint32_t setup_default_config_set(uint8_t **default_config, uint32_t default_config_size);
static uint32_t setup_feeprom_config_set(setup_appli_code_t code_appli,
                                         setup_appli_version_t version_appli,
                                         uint8_t **config_addr);
static setup_action_t setup_get_action(uint8_t *label);
#if (USE_RTC == 1)
static void setup_timedate_handle(uint32_t num_appli);
#endif /* USE_RTC == 1) */
static void setup_handle(uint32_t num_appli);
/* setup component start */
static bool setup_start(void);


/* Functions Definition ------------------------------------------------------*/

/**
  * @brief  display  the list of config and their associated source (DEFAULT or FEEPROM)
  * @param  none
  * @retval none
  */
static void setup_get_source(void)
{
  uint16_t i;
  menu_setup_source_t setup_source;

  PRINT_SETUP("\r\n")
  PRINT_SETUP("----------------------------\n\r")
  PRINT_SETUP("List of configuration status\n\r")
  PRINT_SETUP("----------------------------\n\r")
  PRINT_SETUP("Possible values: DEFAULT or FEEPROM\n\r")
  PRINT_SETUP("\r\n")

  /* find all confuration in feeprom */
  for (i = 0U; i < setup_nb; i++)
  {
#if (FEEPROM_UTILS_FLASH_USED == 1)
    uint32_t config_size = 0U;
    uint8_t *config_addr;
    if (feeprom_utils_read_config_flash(setup_list[i].code_appli,
                                        setup_list[i].version_appli,
                                        &config_addr, &config_size) == 0U)
    {
      /* configutation found in feeprom for this appli => source = feeprom */
      setup_source = MENU_SETUP_SOURCE_FEEPROM;
    }
    else
#endif  /*  (FEEPROM_UTILS_FLASH_USED == 1) */
    {
      /* configutation not found in feeprom for this appli => source = default */
      setup_source = MENU_SETUP_SOURCE_DEFAULT;
    }
    PRINT_SETUP(" %s from %s \n\r", setup_list[i].label_appli, setup_source_string[setup_source])
  }
}

/**
  * @brief  set setup configuration with default values
  * @param  default_config       table of string containing configuration parameters
  * @param  default_config_size  size on the config (number of parameters)
  * @retval lenght of default config in the buffer
  */
static uint32_t setup_default_config_set(uint8_t **default_config, uint32_t default_config_size)
{
  uint32_t config_index = 0U;
  uint32_t config_len;
  uint32_t  i;

  /* add all parameters sequentially un config buffer    */
  for (i = 0U ; i < default_config_size; i++)
  {
    config_len = crs_strlen(default_config[i]);
    if ((config_index + config_len + 1U) >= MENU_UTILS_SETUP_CONFIG_SIZE_MAX)
    {
      PRINT_SETUP("SETUP Warning : configuration too large\r\n")
    }
    else
    {
      /* copy parameter string in config buffer and add '\r' as separator */
      (void)memcpy((CRC_CHAR_t *)&setup_default_config[config_index], (CRC_CHAR_t *)default_config[i], config_len + 1U);
      config_index += crs_strlen(default_config[i]);
      if (config_index >= MENU_UTILS_SETUP_CONFIG_SIZE_MAX)
      {
        config_index = 0;
        break;
      }
      setup_default_config[config_index] = (uint8_t)'\r';
      config_index++;
    }
  }
  return config_index;
}

/**
  * @brief  select from menu the setup action to process
  * @param  application label  to display
  * @retval setup_action_t   return action to process
  */
static setup_action_t setup_get_action(uint8_t *label)
{
  setup_action_t sel = SETUP_ACTION_NONE;
  int32_t  ret;
  uint8_t  car = 0U;

  PRINT_SETUP("\n\r")
  PRINT_SETUP("------------------------------------\n\r")
  PRINT_SETUP(" %s configuration Menu \n\r", label)
  PRINT_SETUP("------------------------------------\n\r")

#if (FEEPROM_UTILS_FLASH_USED == 1)
  /*  PrintINFO(" f : get flash configuration (default choice)\n\r"); */
#endif  /* (FEEPROM_UTILS_FLASH_USED == 1) */
  PRINT_SETUP(" c : update configuration by console and store it in FEEPROM\n\r")
  /*  PrintINFO(" d : default configuration\n\r"); */
#if (FEEPROM_UTILS_FLASH_USED == 1)
  PRINT_SETUP(" e : erase the configuration stored in FEEPROM (restore to DEFAULT)\n\r")
#endif   /*   (FEEPROM_UTILS_FLASH_USED == 1) */
  PRINT_SETUP(" l : list current configuration \n\r")
  PRINT_SETUP(" h : help \n\r")
  PRINT_SETUP(" q : quit \n\r")

  /* get char action from menu */
  ret = menu_utils_get_uart_char(&car);
  if (ret != 0)
  {
    switch (car)
    {
      case 'c':
      case 'C':
      {
        /* update setup configuration from uart console */
        sel = SETUP_ACTION_UART;
        break;
      }
      case 'e':
      case 'E':
      {
        /* erase flash setup configuration */
        sel = SETUP_ACTION_ERASE;
        break;
      }
      case 'l':
      case 'L':
      {
        /* display active setup configuration help  */
        sel = SETUP_ACTION_DUMP;
        break;
      }
      case 'h':
      case 'H':
      {
        /* dump current active setup configuration */
        sel = SETUP_ACTION_HELP;
        break;
      }
      case 'q':
      case 'Q':
      {
        /* cancel : return from this menu */
        sel = SETUP_ACTION_QUIT;
        break;
      }
      default:
      {
        sel = SETUP_ACTION_QUIT;
        break;
      }
    }
  }
  return sel;
}

/**
  * @brief  set current configuration from flash
  * @param  code_appli      application code to match
  * @param  version_appli   application version to match
  * @param  version_appli   (out) addr of found config in flash
  * @retval config_size     returns size of found config (0 means: no configuration matching)
  */
static uint32_t setup_feeprom_config_set(setup_appli_code_t code_appli,
                                         setup_appli_version_t version_appli,
                                         uint8_t **config_addr)
{
  uint32_t config_size = 0U;
#if (FEEPROM_UTILS_FLASH_USED == 1)
  /* get current configuration address and size from feeprom */
  (void)feeprom_utils_read_config_flash(code_appli, version_appli, config_addr, &config_size);
#endif  /*  (FEEPROM_UTILS_FLASH_USED == 1) */
  return config_size;
}

/**
  * @brief  update date and time system
  * @param  num_appli   unused
  * @retval none
  */
#if (USE_RTC == 1)
static void setup_timedate_handle(uint32_t num_appli)
{
  UNUSED(num_appli);
  menu_utils_config_init(MENU_SETUP_SOURCE_UART, NULL, 0U);
  timedate_setup_handler();
}
#endif /* USE_RTC == 1) */

/**
  * @brief  processing of selected setup action
  * @param  num_appli   appli number
  * @retval none
  */
static void setup_handle(uint32_t num_appli)
{
  uint32_t setup_config_size;
  uint8_t *setup_config_addr = NULL;
  setup_action_t setup_action;
  menu_setup_source_t setup_source;
  uint8_t string_version[5];
  uint32_t bad_version ;
  uint32_t size ;
  uint32_t ret ;
  uint32_t config_fail ;
  uint8_t  setup_quit;

  /* get action to process from setup menu */
  setup_quit = 0;
  while (setup_quit == 0U)
  {
    config_fail = 0;
    setup_action = setup_get_action(setup_list[num_appli].label_appli);

    if (setup_action == SETUP_ACTION_QUIT)
    {
      /* quit setup menu */
      setup_quit = 1;
    }
    else
    {
      if (SETUP_ACTION_ERASE == setup_action)
      {
#if (FEEPROM_UTILS_FLASH_USED == 1)
        /* action requested: erase feeprom configuration of the current application */
        ret = feeprom_utils_setup_erase(setup_list[num_appli].code_appli,
                                        setup_list[num_appli].version_appli);
        if (ret == 0U)
        {
          PRINT_SETUP("\r\nFEEPROM Configuration erased\r\n")
        }
        else
        {
          PRINT_SETUP("\r\nError: FEEPROM Configuration NOT Erased\r\n")
        }
#endif /* (FEEPROM_UTILS_FLASH_USED == 1) */
      }

      if (SETUP_ACTION_HELP == setup_action)
      {
        /* call  application dump calback */
        if (setup_list[num_appli].help_fnct != NULL)
        {
          setup_list[num_appli].help_fnct();
        }
      }

      if ((setup_action == SETUP_ACTION_FEEPROM)
          || (setup_action == SETUP_ACTION_DUMP)
          || (setup_action == SETUP_ACTION_UART))
      {
        /* Select the active configuration and set it */
        /* flash configuration preempts default configuration if exists*/
        setup_config_size = setup_feeprom_config_set(setup_list[num_appli].code_appli,
                                                     setup_list[num_appli].version_appli,
                                                     &setup_config_addr);
        if (setup_config_size != 0U)
        {
          /* flash configuration exists => flash configuration set as active */
          setup_source = MENU_SETUP_SOURCE_FEEPROM;
        }
        else
        {
          /* No configuration in flash => default configuration set as active */
          setup_config_size = setup_default_config_set(setup_list[num_appli].default_config,
                                                       setup_list[num_appli].default_config_size);
          setup_config_addr = setup_default_config;
          setup_source      = MENU_SETUP_SOURCE_DEFAULT;
        }

        if (setup_action == SETUP_ACTION_UART)
        {
          /* configuration get from console uart */
          setup_source = MENU_SETUP_SOURCE_UART;
        }

        PRINT_SETUP("\n\r--------------------------------\n\r")
        PRINT_SETUP(" %s from %s \n\r", setup_list[num_appli].label_appli,
                    setup_source_string[setup_source])
        PRINT_SETUP("--------------------------------\n\r")

        menu_utils_config_init(setup_source, setup_config_addr, setup_config_size);


        /* Active configuration selected : set it */
        bad_version = 0U;
        if ((setup_list[num_appli].setup_fnct != NULL) && (setup_config_size != 0U))
        {
          if (setup_action == SETUP_ACTION_UART)
          {
            /* get and check configuration format version */
            int32_t string_version_int;
            PRINT_SETUP("\n\rVersion (%d): ", setup_list[num_appli].version_appli)
            size = menu_utils_get_line(string_version, sizeof(string_version));
            PRINT_SETUP("\n\r")
            string_version_int = crs_atoi(string_version);
            if ((size == 0U) || (string_version_int != (int32_t)setup_list[num_appli].version_appli))
            {
              /* configuration format version not match: exit from configuration */
              PRINT_SETUP("\n\rBad Appli version \"%s\" : expected \"%d\"\n\r", string_version,
                          setup_list[num_appli].version_appli)
              bad_version = 1U;
            }
          }

          if (bad_version == 0U)
          {
            /* right version */
            if (SETUP_ACTION_DUMP == setup_action)
            {
              /* action == dump: setting configuration procession must no be displayed */
              menu_utils_dump_mode = 1;
            }

            /* Call setup callback function to set configuration */
            ret = setup_list[num_appli].setup_fnct();
            menu_utils_dump_mode = 0;
            if (ret == 0U)
            {
              /* configuration setting OK */
              setup_list[num_appli].config_done = 1U;
#if (FEEPROM_UTILS_FLASH_USED == 1)
              if (setup_action == SETUP_ACTION_UART)
              {
                /* set configuration to update from uart console */
                menu_utils_get_new_config(&setup_config_addr, &setup_config_size);

                /* save updated configuration in feeprom */
                ret = feeprom_utils_save_config_flash(setup_list[num_appli].code_appli,
                                                      setup_list[num_appli].version_appli,
                                                      setup_config_addr,
                                                      setup_config_size);
                if (ret != 0U)
                {
                  PRINT_SETUP("New config is written in feeprom (%ld bytes)\n\r", ret)
                }
                else
                {
                  config_fail = 1;
                  PRINT_SETUP("FEEPROM write error\n\r")
                }
              }
#endif   /* (FEEPROM_UTILS_FLASH_USED == 1) */
            }
            else
            {
              /* configuration setting KO */
              config_fail = 1;

              if (SETUP_ACTION_DUMP != setup_action)
              {
                PRINT_SETUP("---> Setup config error <---\n\r")
              }
            }
          }
        }

        if (SETUP_ACTION_DUMP == setup_action)
        {
          /* call  application dump calback */
          if ((setup_list[num_appli].dump_fnct != NULL) && (config_fail == 0U))
          {
            /* configuration setting OK => dump callback of application called */
            setup_list[num_appli].dump_fnct();
          }
          else
          {
            /* configuration setting KO */
            PRINT_SETUP("------------------------------------------\n\r")
            PRINT_SETUP("----> SETUP ERROR : Check the config <----\n\r")
            PRINT_SETUP("------------------------------------------\n\r")
          }
        }
      }
    }
  }
}

/* External functions BEGIN */

/**
  * @brief  save the new configuration in flash
  * @param  code_appli      code appli
  * @param  version_appli   version appli
  * @retval returns the number of saved bytes
  */
uint32_t setup_save_config_flash(setup_appli_code_t code_appli, setup_appli_version_t version_appli)

{
  uint32_t ret;
  uint32_t setup_config_size;
  uint8_t *setup_config_addr;

  /* get address and size of new configuration to save in feeprom */
  menu_utils_get_new_config(&setup_config_addr, &setup_config_size);
#if (FEEPROM_UTILS_FLASH_USED == 1)

  ret = feeprom_utils_save_config_flash(code_appli,
                                        version_appli,
                                        setup_config_addr,
                                        setup_config_size);

#else
  ret = 0;
#endif /* (FEEPROM_UTILS_FLASH_USED == 1) */
  return ret;
}

/**
  * @brief  allows an application to setup its own setup configuration
  * @param  code_appli      code appli
  * @param  version_appli   version appli
  * @param  setup_fnct      application function called to get parse the configuration
  * @param  dump_fnct       application function called to dump the configuration
  * @param  help_fnct       application function called to display the help
  * @retval returns the number of saved bytes
  */
int32_t setup_record(setup_appli_code_t code_appli, setup_appli_version_t version_appli,
                     uint8_t *label_appli,
                     uint32_t (*setup_fnct)(void),
                     void (*dump_fnct)(void),
                     void (*help_fnct)(void),
                     uint8_t **default_config, uint32_t default_config_size)
{
  int32_t ret;
  if (setup_nb >= SETUP_NUMBER_MAX)
  {
    ret = -1;
  }
  else
  {
    /* initializes parameters of application */
    setup_list[setup_nb].code_appli            = code_appli;
    setup_list[setup_nb].version_appli         = version_appli;
    setup_list[setup_nb].label_appli           = label_appli;
    setup_list[setup_nb].setup_fnct            = setup_fnct;
    setup_list[setup_nb].dump_fnct             = dump_fnct;
    setup_list[setup_nb].help_fnct             = help_fnct;
    setup_list[setup_nb].default_config        = default_config;
    setup_list[setup_nb].default_config_size   = default_config_size;
    setup_list[setup_nb].config_done           = 0U;

    setup_nb++;
    ret = 0;
  }
  return ret;
}

/**
  * @brief  get the label of an appli
  * @param  code_appli      code appli
  * @retval returns label of the application
  */
const uint8_t *setup_get_label_appli(setup_appli_code_t code_appli)
{
  const uint8_t *label;
  uint16_t i;

  label = NULL;
  for (i = 0U; i < setup_nb; i++)
  {
    if (setup_list[i].code_appli == code_appli)
    {
      label = setup_list[i].label_appli;
      break;
    }
  }
  return label;
}

/**
  * @brief  display general help
  * @param  none
  * @retval none
  */
void setup_version_help(void)
{
  PRINT_SETUP("The default configuration of the component is defined at compilation time\n\r")
  PRINT_SETUP("When the 'update configuration by console' option is selected (option 'c'),\n\r")
  PRINT_SETUP("the parameter values must be entered on the console\n\r")
  PRINT_SETUP("\n\r")
  PRINT_SETUP("There are three ways to enter parameter values:\n\r")
  PRINT_SETUP("- Enter the configuration manually from the keyboard\n\r")
  PRINT_SETUP("- Copy and paste the configuration from a configuration file\n\r")
  PRINT_SETUP("- Send a configuration file to the console by using the Teraterm send menu\n\r")
  PRINT_SETUP("For each parameter (except Version), the current value of the parameter is displayed\n\r")
  PRINT_SETUP("If no value is entered (return key pressed), the current value is kept\n\r")
  PRINT_SETUP("At the end, the new configuration is stored in FEEPROM\n\r")
  PRINT_SETUP("\n\r")
  PRINT_SETUP("The option 'erase the configuration' (e) allows to erase configuration in FEEPROM\n\r")
  PRINT_SETUP("and to restore the default configuration\n\r")
  PRINT_SETUP("\n\r")
  PRINT_SETUP("----------\n\r")
  PRINT_SETUP("Version(n)\n\r")
  PRINT_SETUP("----------\n\r")
  PRINT_SETUP("Version is the first configuration field.\n\r")
  PRINT_SETUP("It allows checking the configuration version when a configuration file is used.\n\r")
  PRINT_SETUP("If the entered version does not match with 'n', the configuration is aborted.\n\r")
  PRINT_SETUP("- If the configuration is entered manually,\n\r")
  PRINT_SETUP("  the version must be typed as displayed on the first configuration line.\n\r")
  PRINT_SETUP("  Example: In case of 'Version(2)' type '2' and 'Return'\n\r")
  PRINT_SETUP("- If the configuration is entered from a text file, \n\r")
  PRINT_SETUP("  check that the configuration of the file (first line)\n\r")
  PRINT_SETUP("  match with version of the configuration displayed by the console\n\r")
  PRINT_SETUP("\n\r")
  PRINT_SETUP("Caution: The format of the configurations can change from one FW version to the other.\n\r")
  PRINT_SETUP("         In such a case, the configuration stored in FEEPROM is erased at first boot\n\r")
  PRINT_SETUP("         and the default configuration is restored.\n\r")
  PRINT_SETUP("\n\r")
  PRINT_SETUP("\n\r")

}

/**
  * @brief  setup component init
  * @param  none
  * @retval none
  */
void setup_init(void)
{
  (void)app_select_record((AS_CHART_t *)SETUP_APP_SELECT_LABEL, setup_start);
}

/**
  * @brief  setup component start
  * @param  none
  * @retval none
  */
static bool setup_start(void)
{
  uint32_t i;
  uint8_t  car;
  uint32_t num_appli;
  int32_t  ret;
  uint8_t end_of_line;

  end_of_line = 0U;
  car = 0U;

  while (end_of_line == 0U)
  {
    PRINT_SETUP("\n\r--------------------------------\n\r")
#if (USE_RTC == 1)
    timedate_setup_dump();
#endif /* USE_RTC == 1) */
    PRINT_SETUP("--------------------------------\n\r")
    PRINT_SETUP("   %s   \n\r", SETUP_APP_SELECT_LABEL)
    PRINT_SETUP("--------------------------------\n\r")
    PRINT_SETUP("Select the action to process:\n\r")
    PRINT_SETUP("\n\r")
    PRINT_SETUP("0: Quit\n\r")
    PRINT_SETUP("1: Date/Time setting (RTC)\n\r")

    for (i = 0U; i < setup_nb; i++)
    {
      PRINT_SETUP("%ld: Configuration: %s\n\r", i + 2U, setup_list[i].label_appli)
    }
    PRINT_SETUP("8: Status of above configurations\n\r")

#if (FEEPROM_UTILS_FLASH_USED == 1)
    PRINT_SETUP("9: Erase all FEEPROM configurations (restore to DEFAULT)\n\r")
#endif  /* (FEEPROM_UTILS_FLASH_USED == 1) */
    ret = menu_utils_get_uart_char(&car);
    if (ret != 0)
    {
      if (car >= 0x30U)
      {
        num_appli = (uint32_t)car - 0x30U;
      }
      else
      {
        num_appli = 0U;
      }
      if (num_appli == 0U)
      {
        end_of_line = 1U;
      }

#if (FEEPROM_UTILS_FLASH_USED == 1)
      else if (num_appli == 9U)
      {
        feeprom_utils_flash_erase_all();
        PRINT_SETUP("\n\r-> All FEEPROM configurations erased\n\r\n\r")
      }
#endif  /* (FEEPROM_UTILS_FLASH_USED == 1) */
      else if (num_appli == 8U)
      {
        setup_get_source();
      }
      else if (num_appli == 1U)
      {
#if (USE_RTC == 1)
        setup_timedate_handle(num_appli);
#endif /* USE_RTC == 1) */
      }
      else
      {
        num_appli -= 2U;
        if (num_appli < setup_nb)
        {
          setup_handle(num_appli);
        }
      }
    }
  }

  return false;
}

/**
  * @brief  allows to update a setup configuration
  * @note   for example, it is used by Cellular Service when an application wants to update the cellular configuration
  * @param  code_appli       code appli
  * @param  version_appli    version appli
  * @param  config           table of string containing configuration parameters
  * @param  config_size      size on the config (number of parameters)
  * @retval none
  */
void setup_set_config(setup_appli_code_t code_appli, setup_appli_version_t version_appli,
                      uint8_t **config, uint32_t config_size)
{
  uint32_t setup_config_size;
  uint8_t *setup_config_addr;
  menu_setup_source_t setup_source;

  /* find configuration en feeprom */
  setup_config_size = setup_feeprom_config_set(code_appli,
                                               version_appli,
                                               &setup_config_addr);
  if (setup_config_size == 0U)
  {
    /* configuration is not in feeprom => set default configuration */
    setup_config_size   = setup_default_config_set(config, config_size);
    setup_source        = MENU_SETUP_SOURCE_DEFAULT;
    setup_config_addr   = setup_default_config;
  }
  else
  {
    /* configuration is not in feeprom => set feeprom configuration */
    setup_source      = MENU_SETUP_SOURCE_FEEPROM;
  }

  menu_utils_config_init(setup_source, setup_config_addr, setup_config_size);
}

/**
  * @brief  apply all active setup configurations
  * @param  none
  * @retval none
  */
void setup_apply(void)
{
  uint32_t i;
  uint32_t ret ;
  uint32_t setup_config_size;
  uint8_t *setup_config_addr;
  menu_setup_source_t setup_source;

  PRINT_SETUP("\n\r--------SETUP Configuration ----------\n\r")

  /* setting all recorded configuration */
  for (i = 0U; i < setup_nb; i++)
  {
    if (setup_list[i].config_done == 0U)
    {
      setup_config_size = setup_feeprom_config_set(setup_list[i].code_appli,
                                                   setup_list[i].version_appli,
                                                   &setup_config_addr);
      if (setup_config_size == 0U)
      {
        setup_config_size = setup_default_config_set(setup_list[i].default_config,
                                                     setup_list[i].default_config_size);
        setup_source        = MENU_SETUP_SOURCE_DEFAULT;
        setup_config_addr   = setup_default_config;
      }
      else
      {
        setup_source      = MENU_SETUP_SOURCE_FEEPROM;
      }

      menu_utils_config_init(setup_source, setup_config_addr, setup_config_size);

      PRINT_SETUP(" %s from %s \n\r", setup_list[i].label_appli, setup_source_string[setup_source])
      menu_utils_dump_mode = 1;
      /* setting callback of application is called to process configuration */
      ret = setup_list[i].setup_fnct();
      menu_utils_dump_mode = 0;
      if (ret != 0U)
      {
        PRINT_SETUP("------------------------------------------\n\r")
        PRINT_SETUP("----> SETUP ERROR : Check the config <----\n\r")
        PRINT_SETUP("------------------------------------------\n\r")
        for (;;)
        {
          menu_utils_dump_mode = 0;
        }
      }

      setup_list[i].config_done = 1U;
    }
  }
  PRINT_SETUP("------------------------------------------\n\r")
}
#endif /* (!USE_DEFAULT_SETUP == 1) */

/* External functions BEGIN */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
