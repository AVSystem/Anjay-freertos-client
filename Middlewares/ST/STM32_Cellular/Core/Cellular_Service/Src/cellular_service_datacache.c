/**
  ******************************************************************************
  * @file    cellular_service_datacache.c
  * @author  MCD Application Team
  * @brief   This file defines functions for Cellular Service Datacache
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <string.h>

#include "plf_config.h"
#include "cellular_service_datacache.h"

/* Private typedef -----------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/
/* Definition of Data Cache cellular entries  */
dc_com_res_id_t    DC_CELLULAR_INFO             = DC_COM_INVALID_ENTRY;
dc_com_res_id_t    DC_CELLULAR_DATA_INFO        = DC_COM_INVALID_ENTRY;
dc_com_res_id_t    DC_CELLULAR_NIFMAN_INFO      = DC_COM_INVALID_ENTRY;
dc_com_res_id_t    DC_CELLULAR_NFMC_INFO        = DC_COM_INVALID_ENTRY;
dc_com_res_id_t    DC_CELLULAR_SIM_INFO         = DC_COM_INVALID_ENTRY;
dc_com_res_id_t    DC_CELLULAR_SIGNAL_INFO      = DC_COM_INVALID_ENTRY;
dc_com_res_id_t    DC_CELLULAR_CONFIG           = DC_COM_INVALID_ENTRY;
dc_com_res_id_t    DC_CELLULAR_TARGET_STATE_CMD = DC_COM_INVALID_ENTRY;
#if (USE_LOW_POWER == 1)
dc_com_res_id_t    DC_CELLULAR_POWER_CONFIG     = DC_COM_INVALID_ENTRY;
dc_com_res_id_t    DC_CELLULAR_POWER_STATUS     = DC_COM_INVALID_ENTRY;
#endif /* (USE_LOW_POWER == 1) */

/* Private function prototypes -----------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/
/**
  * @brief  Initialize cellular service datacache entries.
  * @param  -
  * @retval -
  */
void cellular_service_datacache_init(void)
{
  static dc_cellular_info_t          dc_cellular_info;
  static dc_cellular_data_info_t     dc_cellular_data_info;
  static dc_nifman_info_t            dc_nifman_info;
  static dc_nfmc_info_t              dc_nfmc_info;
  static dc_sim_info_t               dc_sim_info;
  static dc_signal_info_t            dc_signal_info;
  static dc_cellular_params_t        dc_cellular_params;
  static dc_cellular_target_state_t  dc_cellular_target_state;
#if (USE_LOW_POWER == 1)
  static dc_cellular_power_config_t  dc_cellular_power_config;
  static dc_cellular_power_status_t  dc_cellular_power_status;
#endif  /* (USE_LOW_POWER == 1) */

  /* set all the data cache entries variables to 0 */
  /* Reset cellular info */
  (void)memset((void *)&dc_cellular_info,         0, sizeof(dc_cellular_info_t));
  /* Reset data info */
  (void)memset((void *)&dc_cellular_data_info,    0, sizeof(dc_cellular_data_info_t));
  /* Reset Nifman info */
  (void)memset((void *)&dc_nifman_info,           0, sizeof(dc_nifman_info_t));
  /* Reset NFMC info */
  (void)memset((void *)&dc_nfmc_info,             0, sizeof(dc_nfmc_info_t));
  /* Reset SIM info */
  (void)memset((void *)&dc_sim_info,              0, sizeof(dc_sim_info_t));
  /* Reset signal info */
  (void)memset((void *)&dc_signal_info,           0, sizeof(dc_signal_info_t));
  /* Register cellular config */
  (void)memset((void *)&dc_cellular_params,       0, sizeof(dc_cellular_params_t));
  /* Register target state */
  (void)memset((void *)&dc_cellular_target_state, 0, sizeof(dc_cellular_target_state_t));
#if (USE_LOW_POWER == 1)
  /* Register power config */
  (void)memset((void *)&dc_cellular_power_config, 0, sizeof(dc_cellular_power_config_t));
  /* Register power status */
  (void)memset((void *)&dc_cellular_power_status, 0, sizeof(dc_cellular_power_status_t));
#endif  /* (USE_LOW_POWER == 1) */

  /* register all all cellular entries of Data Cache */
  /* Register cellular info */
  DC_CELLULAR_INFO             = dc_com_register_serv(&dc_com_db, (void *)&dc_cellular_info,
                                                      (uint16_t)sizeof(dc_cellular_info_t));
  /* Register data info */
  DC_CELLULAR_DATA_INFO        = dc_com_register_serv(&dc_com_db, (void *)&dc_cellular_data_info,
                                                      (uint16_t)sizeof(dc_cellular_data_info_t));
  /* Register Nifman info */
  DC_CELLULAR_NIFMAN_INFO      = dc_com_register_serv(&dc_com_db, (void *)&dc_nifman_info,
                                                      (uint16_t)sizeof(dc_nifman_info_t));
  /* Register NFMC info */
  DC_CELLULAR_NFMC_INFO        = dc_com_register_serv(&dc_com_db, (void *)&dc_nfmc_info,
                                                      (uint16_t)sizeof(dc_nfmc_info_t));
  /* Register SIM info */
  DC_CELLULAR_SIM_INFO         = dc_com_register_serv(&dc_com_db, (void *)&dc_sim_info,
                                                      (uint16_t)sizeof(dc_sim_info_t));
  /* Register signal info */
  DC_CELLULAR_SIGNAL_INFO      = dc_com_register_serv(&dc_com_db, (void *)&dc_signal_info,
                                                      (uint16_t)sizeof(dc_signal_info_t));
  /* Register cellular config */
  DC_CELLULAR_CONFIG           = dc_com_register_serv(&dc_com_db, (void *)&dc_cellular_params,
                                                      (uint16_t)sizeof(dc_cellular_params_t));
  /* Register target state */
  DC_CELLULAR_TARGET_STATE_CMD = dc_com_register_serv(&dc_com_db, (void *)&dc_cellular_target_state,
                                                      (uint16_t)sizeof(dc_cellular_target_state_t));
#if (USE_LOW_POWER == 1)
  /* Register power config */
  DC_CELLULAR_POWER_CONFIG     = dc_com_register_serv(&dc_com_db, (void *)&dc_cellular_power_config,
                                                      (uint16_t)sizeof(dc_cellular_power_config));
  /* Register power status */
  DC_CELLULAR_POWER_STATUS     = dc_com_register_serv(&dc_com_db, (void *)&dc_cellular_power_status,
                                                      (uint16_t)sizeof(dc_cellular_power_status));
#endif  /* (USE_LOW_POWER == 1) */
}


