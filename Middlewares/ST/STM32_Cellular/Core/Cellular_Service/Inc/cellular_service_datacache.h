/**
  ******************************************************************************
  * @file    cellular_service_datacache.h
  * @author  MCD Application Team
  * @brief   Data Cache definitions for cellular service components
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
#ifndef CELLULAR_SERVICE_DATACACHE_H
#define CELLULAR_SERVICE_DATACACHE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "dc_common.h"
#include "com_sockets.h"
#include "cellular_service_int.h"
#include "cellular_control_api.h"

/**
  ******************************************************************************
  @verbatim

  ==============================================================================
          ##### Description of Cellular Service Data Cache Entries  #####
  ==============================================================================
  Cellular Service use Data Cache to share information with the other components
  See Data Cache module for details about Data Cache feature

  The list of Cellular Service Data Cache Entries is (summary):

    To get Cellular Service information:
    ------------------------------------
    DC_CELLULAR_INFO              contains information returned by the modem
    DC_CELLULAR_DATA_INFO         contains the state of data transfer feature
    DC_CELLULAR_NIFMAN_INFO       contains NIFMAN information
    DC_CELLULAR_SIM_INFO          contains SIM slot information
    DC_CELLULAR_SIGNAL_INFO       contains the signal strength and access techno information
    DC_CELLULAR_CONFIG            contains Cellular configuration parameters
    DC_CELLULAR_NFMC_INFO         contains NFMC information
    DC_CELLULAR_POWER_STATUS      contains PSM configuration returned by the modem

    To Make a Cellular Service request:
    -----------------------------------
    DC_CELLULAR_CONFIG            to set the Cellular configuration parameters
    DC_CELLULAR_TARGET_STATE_CMD  to make a new modem target state request
    DC_CELLULAR_POWER_CONFIG      to set the power configuration (only available if USE_LOW_POWER == 1)

  @endverbatim
  */


/* Exported constants --------------------------------------------------------*/

/** @brief Size max of Data Cache cellular information strings */
#define DC_MAX_IP_ADDR_SIZE        MAX_IP_ADDR_SIZE  /*!< Size max of ID ADDR              */

/** @brief Number max of SIM slot manage in DataCache structure */
#define DC_SIM_SLOT_NB             3U

/** @brief Value of signal level meaning 'no attached' */
#define DC_NO_ATTACHED             0U

/* Exported types ------------------------------------------------------------*/
/** @brief Raw signal level value */
typedef uint8_t dc_cs_signal_level_t; /*!< range 0..99  0: DC_NO_ATTACHED
                                                        99: Not known or not detectable */

/** @brief list of Network modes */
typedef enum
{
  DC_NETWORK_SOCKET_MODEM    = 1,     /*!< Modem Socket mode                  */
  DC_NETWORK_SOCKETS_LWIP    = 2      /*!< Socket LWIP mode                   */
} dc_cellular_network_t;

/** @brief list of Network modes */
typedef enum
{
  DC_NO_NETWORK            = 0,     /*!< No available Network               */
  DC_CELLULAR_SOCKET_MODEM = 1,     /*!< Modem Socket mode                  */
  DC_CELLULAR_SOCKETS_LWIP = 2      /*!< Socket LWIP mode                   */
} dc_nifman_network_t;

/** @brief list of modem target state */
typedef uint8_t dc_cs_target_state_t;

#define DC_TARGET_STATE_OFF        ((dc_cs_target_state_t)(0U))   /*!< Modem off                                     */
#define DC_TARGET_STATE_SIM_ONLY   ((dc_cs_target_state_t)(1U))   /*!< SIM connected but no network management
                                                                       (not implemented)                            */
#define DC_TARGET_STATE_FULL       ((dc_cs_target_state_t)(2U))   /*!< Full modem features available (data transfer) */
#define DC_TARGET_STATE_MODEM_ONLY ((dc_cs_target_state_t)(3U))   /*!< Modem only on with no network management      */
#define DC_TARGET_STATE_UNKNOWN    ((dc_cs_target_state_t)(4U))   /*!<                                               */

/** @brief IP address */
typedef com_ip_addr_t dc_network_addr_t;

#define CST_ACCESS_TECHNO_PRESENT_MAX 2

#if (USE_LOW_POWER == 1)
/** @brief list of power states */
typedef enum
{
  DC_POWER_LOWPOWER_INACTIVE      = 0U,    /*!< Modem is not in low power mode                 */
  DC_POWER_LOWPOWER_ONGOING       = 1U,    /*!< Modem is going to low power mode               */
  DC_POWER_IN_LOWPOWER            = 2U     /*!< Modem in low power mode                        */
} dc_cs_power_state_t;

#endif  /* (USE_LOW_POWER == 1) */

/* Structures types ------------------------------------------------------------*/

/* =================================================== */
/* Structures definition of Data Cache entries - BEGIN */
/* =================================================== */

/**
  * @brief  Structure definition of DC_CELLULAR_INFO DC entry.
  * This DC entry contains the main cellular information received
  * from the modem after its initialization.
  */
typedef struct
{
  dc_service_rt_header_t header;     /*!< Internal use */

  /** @brief rt_state: entry state.
    *!<
    * - DC_SERVICE_UNAVAIL: service not initialized,
    *                       the field values of structure are not significant.
    * - DC_SERVICE_RUN: modem powered on and initialized,
    *                   the field values of structure are significant except MNO name.
    * - DC_SERVICE_ON: modem attached,
    *                  all the field values of structure are significant.
    * - Other state values not used.
    */
  dc_service_rt_state_t  rt_state;
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  dc_service_rt_state_t  rt_state_ppp;
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
  ca_modem_state_t       modem_state;     /*!< current modem state                  */
  /* modem information */
  uint8_t                imei[CA_IMEI_SIZE_MAX];
  uint8_t                mno_name[CA_MNO_NAME_SIZE_MAX];
  uint8_t                manufacturer_name[CA_MANUFACTURER_ID_SIZE_MAX];
  uint8_t                model[CA_MODEL_ID_SIZE_MAX];
  uint8_t                revision[CA_REVISION_ID_SIZE_MAX];
  uint8_t                serial_number[CA_SERIAL_NUMBER_ID_SIZE_MAX];
  uint8_t                iccid[CA_ICCID_SIZE_MAX];
  dc_network_addr_t      ip_addr;   /*!< IP address */
} dc_cellular_info_t;


/**
  * @brief  Structure definition of DC_CELLULAR_SIM_INFO entry.
  * This DC entry contains information of the available SIMs.
  */
typedef struct
{
  dc_service_rt_header_t header;     /*!< Internal use */

  /** @brief rt_state: entry state.
    *!<
    * - DC_SERVICE_UNAVAIL: service not initialized,
    *                       the field values of structure are not significant.
    * - DC_SERVICE_ON: modem attached,
    *                  all the field values of structure are significant.
    * - Other state values not used.
    */
  dc_service_rt_state_t  rt_state;
  uint8_t                imsi[CA_IMSI_SIZE_MAX];      /*!< IMSI value                                */
  uint8_t                index_slot;                  /*!< SIM Slot index in the configuration table
                                                           of Active SIM Slot                        */
  ca_sim_slot_type_t     active_slot;                 /*!< Type of active SIM Slot                   */
  ca_sim_status_t        sim_status[DC_SIM_SLOT_NB];  /*!< Status of available SIM slots             */
} dc_sim_info_t;

/**
  * @brief  Structure definition of DC_CELLULAR_SIGNAL_INFO entry.
  * This DC entry contains information of signal strength and the access techno used.
  */
typedef struct
{
  dc_service_rt_header_t header;     /*!< Internal use */

  /** @brief rt_state: entry state.
    *!<
    * - DC_SERVICE_UNAVAIL: service not initialized,
    *                       the field values of structure are not significant.
    * - DC_SERVICE_ON: modem attached,
    *                  all the field values of structure are significant.
    * - Other state values not used.
    */
  dc_service_rt_state_t  rt_state;
  dc_cs_signal_level_t   cs_signal_level;          /*!< raw cellular signal level range 0-99
                                                      0  : Not attached
                                                      99 : Not known or not detectable       */
  int32_t                cs_signal_level_db;       /*!< cellular signal level in dB          */
  ca_access_techno_t     access_techno;
} dc_signal_info_t;


/**
  * @brief  Structure definition of DC_CELLULAR_DATA_INFO entry.
  * This DC entry contains the state of cellular data transfer feature.
  */
typedef struct
{
  dc_service_rt_header_t header;     /*!< Internal use */

  /** @brief rt_state: entry state.
    *!<
    * - DC_SERVICE_UNAVAIL      : service not initialized,
    *                             the field values of structure are not significant
    * - DC_SERVICE_OFF          : network not available
    * - DC_SERVICE_ON           : network available,
    *                             the other field values of structure are significant
    * - DC_SERVICE_FAIL         : network stack returns error. Network is not available
    * - DC_SERVICE_SHUTTING_DOWN: network shut down. Network is not available
    * - Other state values not used.
    */
  dc_service_rt_state_t  rt_state;
  dc_cellular_network_t  network;   /*!< network type used */
  dc_network_addr_t      ip_addr;   /*!< IP address */
} dc_cellular_data_info_t;

/**
  * @brief  Structure definition of DC_CELLULAR_TARGET_STATE_CMD entry.
  * This DC entry allows to request a new modem target state.
  */
typedef struct
{
  dc_service_rt_header_t header;       /*!< Internal use */
  dc_service_rt_state_t  rt_state;     /*!< must be set to DC_SERVICE_ON to make
                                            all the field values of structure significant. */
  bool                   callback;     /*!< If true, associated callback will be called. If false, it will not be */
  dc_cs_target_state_t   target_state; /*!< modem target state to reach */
} dc_cellular_target_state_t;

/**
  * @brief  Structure definition of DC_CELLULAR_NIFMAN_INFO entry.
  * This DC entry contains the Nifman information.
  */
typedef struct
{
  dc_service_rt_header_t header;     /*!< Internal use */
  /** @brief rt_state: entry state.
    *!<
    * - DC_SERVICE_UNAVAIL      : service not initialized,
    *                             the field values of structure are not significant
    * - DC_SERVICE_OFF          : network not available
    * - DC_SERVICE_ON           : network available,
    *                             the other field values of structure are significant
    * - DC_SERVICE_FAIL         : network stack returns error. Network is not available
    * - DC_SERVICE_SHUTTING_DOWN: network shut down. Network is not available
    * - Other state values not used.
    */
  dc_service_rt_state_t  rt_state;

  dc_nifman_network_t    network;   /*!< network type used */
  dc_network_addr_t      ip_addr;   /*!< IP address */
} dc_nifman_info_t;

/**
  * @brief  Structure definition of DC_CELLULAR_NFMC_INFO entry.
  * This DC entry contains the NFMC information.
  */
typedef struct
{
  dc_service_rt_header_t header;     /*!< Internal use */
  /** @brief rt_state: entry state.
    *!<
    * - DC_SERVICE_UNAVAIL      : NFMC service not initialized,
    *                             the field values of structure are not significant
    * - DC_SERVICE_OFF          : NFMC feature not activated
    * - DC_SERVICE_ON           : NFMC feature activated,
    *                             the other field values of structure are significant
    * - Other state values not used.
    */
  dc_service_rt_state_t rt_state;

  uint32_t activate;                /*!< 0: feature not activated - 1: feature activated */
  uint32_t tempo[CA_NFMC_VALUES_MAX_NB]; /*!< NFMC tempo values (significant only if activate==1)
                                         and (rt_state == DC_SERVICE_ON) */
} dc_nfmc_info_t;


/**
  * @brief dc_sim_slot_t - SIM configuration structure.
  */
typedef struct
{
  ca_sim_slot_type_t        sim_slot_type;               /*!< type of SIM slot                                   */
  cellular_apn_send_to_modem_t apnSendToModem;           /*!< CA_APN_SEND_TO_MODEM :
                                                          APN is managed by app and send to modem,
                                                          CA_APN_NOT_SEND_TO_MODEM :
                                                          let modem manage APN using its own policy              */
  bool                   apnPresent;                     /*!< true if an APN parameter contains a valid
                                                              value                                              */
  uint8_t                apn[CA_APN_SIZE_MAX];           /*!< APN Value (string)                                 */
  uint8_t                cid;                            /*!< CID value (1-9)                                    */
  uint8_t                username[CA_USERNAME_SIZE_MAX]; /*!< username: empty string => no username              */
  uint8_t                password[CA_PASSWORD_SIZE_MAX]; /*!< password (used only is username is defined)        */
  bool                   apnChanged;                     /*!< true if an APN parameter has changed               */
} dc_sim_slot_t;

/**
  * @brief dc_sim_slot_t - SIM configuration structure.
  */
typedef struct
{
  ca_ntw_registration_mode_t network_reg_mode;
  ca_operator_name_format_t  operator_name_format;
  uint8_t                    operator_name[CA_OPERATOR_NAME_SIZE_MAX];
  ca_access_techno_present_t access_techno_present;
  ca_access_techno_t         access_techno;
} dc_operator_selector_t;

/**
  * @brief  Structure definition of DC_CELLULAR_CONFIG entry.
  * This DC entry contains the cellular parameters used to configure the modem.
  * At boot time this entry is set by default value.
  * If the application needs to set its own modem configuration,
  * it must set the DC_CELLULAR_CONFIG Data Cache entry at boot time between the calls
  * to cellular_init() and cellular_start().
  */
typedef struct
{
  dc_service_rt_header_t header;     /*!< Internal use */

  /** @brief rt_state: entry state.
    *!<
    * - DC_SERVICE_UNAVAIL      : service not initialized,
    *                             the field values of structure are not significant
    * - DC_SERVICE_ON           : service started,
    *                             the other field values of structure are significant
    * - Other state values not used.
    */
  dc_service_rt_state_t  rt_state;

  /*!<
    * sim_slot_nb: number of SIM slots used (max 2)
    * For each SIM slot used, set the associated parameters in the sim_slot table.
    */
  uint8_t                sim_slot_nb;

  /*!< sim_slot: table of SIM slot parameters */
  dc_sim_slot_t          sim_slot[DC_SIM_SLOT_NB];

  /*!<  target_state: actual target stateused. */
  dc_cs_target_state_t   target_state;

  /*!< maximum network attachment duration */
  uint32_t               attachment_timeout;

  dc_operator_selector_t operator_selector;

  /*!<
    *  nfmc_active: this flag (0:inactive - 1: active)
    *  specifies if NFMC feature must be activated.
    *  If yes, the nfmc_value table is used.
    */
  uint8_t                nfmc_active;

  /*!<
    *   nfmc_value: table of NFMC values allowing the calculation of NFMC tempos.
    *   This field is used only if nfmc_active==1.
    */
  uint32_t               nfmc_value[CA_NFMC_VALUES_MAX_NB];

  /*!< network inactivity duration before to enable low power mode */
  uint32_t               lp_inactivity_timeout;

} dc_cellular_params_t;


#if (USE_LOW_POWER == 1)
/**
  * @brief dc_cellular_power_psm_config_t - psm lower power configuration .
  */
typedef struct
{
  uint8_t             req_periodic_RAU;     /*!<  GERAN/UTRAN networks, (T3312), cf Table 10.5.163a from TS 24.008 */
  uint8_t             req_GPRS_READY_timer; /*!<  GERAN/UTRAN networks, T3314), cf Table 10.5.172 from TS 24.008   */
  uint8_t             req_periodic_TAU;     /*!<  E-UTRAN networks, (T3412), cf Table 10.5.163a from TS 24.008     */
  uint8_t             req_active_time;      /*!<  GERAN/UTRAN and E-UTRAN networks, (T3324),
                                                  cf Table 10.5.163 from TS 24.008                                 */
} dc_cellular_power_psm_config_t;

/**
  * @brief dc_cellular_power_edrx_config_t - edrx lower power configuration .
  */
typedef struct
{
  ca_eidrx_act_type_t act_type;
  uint8_t             req_value;
} dc_cellular_power_edrx_config_t;

/**
  * @brief  Structure definition of DC_CELLULAR_POWER_CONFIG entry.
  * This DC entry allows to set Power configuration
  */
typedef struct
{
  dc_service_rt_header_t header;     /*!< Internal use */

  /** @brief rt_state: entry state.
    *!<
    * - DC_SERVICE_UNAVAIL: service not initialized,
    *                       the field values of structure are not significant.
    * - DC_SERVICE_ON: all the field values of structure are significant.
    */
  dc_service_rt_state_t  rt_state;

  ca_power_cmd_t      power_cmd;  /*!< power command to apply (init or setting)       */
  ca_power_mode_t     power_mode; /*!< target power mode */
  bool  psm_present;              /*!< indicates if PSM parameters below are present  */
  bool  edrx_present;             /*!< indicates if eDRX parameters below are present */

  ca_psm_mode_t      psm_mode;               /* !< requested PSM mode     */
  uint32_t           sleep_request_timeout;  /* !< sleep request timeout  */
  dc_cellular_power_psm_config_t   psm;      /* !< psm config             */
  ca_eidrx_mode_t    edrx_mode;              /* !< requested eDRX mode    */
  dc_cellular_power_edrx_config_t  edrx;     /* !< eDRX config            */
} dc_cellular_power_config_t;

/**
  * @brief  Structure definition of DC_CELLULAR_POWER_STATUS entry.
  * This DC entry allows to set Power configuration
  */
typedef struct
{
  dc_service_rt_header_t header;     /*!< Internal use */

  dc_cs_power_state_t power_state;       /*!< current modem state                  */
  uint32_t            nwk_periodic_TAU;  /*!< negotiated value of T3412, expressed in seconds (0 means not available) */
  uint32_t            nwk_active_time;   /*!< negotiated value of T3324, expressed in seconds (0 means not available) */
} dc_cellular_power_status_t;
#endif  /* (USE_LOW_POWER == 1) */


/* ===================================================== */
/* Structures definition of Data Cache entries - END     */
/* ===================================================== */

/* External variables --------------------------------------------------------*/

/* =============================================== */
/* List of Cellular Data Cache entries - BEGIN     */
/* =============================================== */

/**
  * @brief  contains information returned by the modem.
  * Associated with dc_cellular_info_t
  */
extern dc_com_res_id_t    DC_CELLULAR_INFO;    /*<! see dc_cellular_info_t */

/**
  * @brief  contains the state of data transfer feature.
  *         allows to know if the cellular data transfer service is available or not
  *         This Data Cache Entry is associated with dc_cellular_data_info_t data structure
  */
extern dc_com_res_id_t    DC_CELLULAR_DATA_INFO;    /*<! see dc_cellular_data_info_t */

/**
  * @brief  contains NIFMAN information.
  *         allows to know if the network data transfer service is available or not
  *         This Data Cache Entry is associated with dc_nifman_info_t data structure
  */
extern dc_com_res_id_t    DC_CELLULAR_NIFMAN_INFO;    /*<! see dc_nifman_info_t */

/**
  * @brief  contains NFMC information.
  *         This Data Cache Entry is associated with dc_nfmc_info_t data structure
  */
extern dc_com_res_id_t    DC_CELLULAR_NFMC_INFO;    /*<! see dc_nfmc_info_t */

/**
  * @brief  contains SIM slot information.
  *         This Data Cache Entry is associated with dc_sim_info_t data structure
  */
extern dc_com_res_id_t    DC_CELLULAR_SIM_INFO;    /*<! see dc_sim_info_t */

/**
  * @brief  contains signal strength and access techno information.
  *         This Data Cache Entry is associated with dc_signal_info_t data structure
  */
extern dc_com_res_id_t    DC_CELLULAR_SIGNAL_INFO;    /*<! see dc_signal_info_t */

/**
  * @brief  contains Cellular configuration parameters.
  *         This Data Cache entry can be called to know the current modem configuration
  *         It can also be called to set the modem configuration to apply at boot:
  *           During cellule_init, the DC_CELLULAR_CONFIG is initialized
  *           with the default parameter included in plf_cellular_config.h configuration file
  *           This entry must be updated after cellule_init and berfore cellular_start calls
  *           Example of modem configuration sequence:
  *
  *                cellular_init();
  *                ...
  *                cellular_set_config();
  *                ...
  *                cellular_start();
  *
  *              static void cellular_set_config(void)
  *              {
  *                dc_cellular_params_t cellular_params;
  *                uint8_t i;
  *
  *                (void) memset((void *)&cellular_params,     0, sizeof(dc_cellular_params_t));
  *                (void) dc_com_read(&dc_com_db, DC_COM_CELLULAR_PARAM, (void *)&cellular_params,
  *                                    sizeof(cellular_params));
  *
  *                * update cellular_params with your parameters *
  *
  *                (void) dc_com_write(&dc_com_db, DC_COM_CELLULAR_PARAM, (void *)&cellular_params,
  *                                     sizeof(cellular_params));
  *              }
  *
  *         This Data Cache Entry is associated with dc_cellular_params_t data structure
  */
extern dc_com_res_id_t    DC_CELLULAR_CONFIG; /*<! see dc_cellular_params_t */

/**
  * @brief  allows to make a request to modify modem target state.
  *         After dc_com_write function has been called with this entry,
  *         the modem state is modified to reach the target state requested
  *         The  target state available are: power off, sim only and full feature.
  *         This Data Cache Entry is associated with dc_cellular_target_state_t data structure
  */
extern dc_com_res_id_t    DC_CELLULAR_TARGET_STATE_CMD;  /*<! see dc_cellular_target_state_t */

#if (USE_LOW_POWER == 1)
/**
  * @brief  set power configuration.
  *         The power is confired with the parameters included in the dc_cellular_power_config_t data structure
  */
extern dc_com_res_id_t    DC_CELLULAR_POWER_CONFIG;    /*<! see dc_cellular_power_config_t */

/**
  * @brief  get power configuration.
  *         The PSM configuration data returned by the network (timers T3412 and T3324) are included in the
  *         dc_cellular_power_status_t data structure
  */
extern dc_com_res_id_t    DC_CELLULAR_POWER_STATUS;    /*<! see dc_cellular_power_status_t */
#endif /* USE_LOW_POWER == 1 */

/* =============================================== */
/* List of Cellular Data Cache entries - END       */
/* =============================================== */

/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
/**
  * @brief  Initialize cellular service datacache entries.
  * @param  -
  * @retval -
  */
void cellular_service_datacache_init(void);

#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_SERVICE_DATACACHE_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
