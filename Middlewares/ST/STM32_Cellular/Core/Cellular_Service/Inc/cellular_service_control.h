/**
  ******************************************************************************
  * @file    cellular_service_control.h
  * @author  MCD Application Team
  * @brief   Header for cellular_service.c
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
#ifndef CELLULAR_SERVICE_CONTROL_H
#define CELLULAR_SERVICE_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include "at_core.h"
#include "plf_config.h"

/** @addtogroup CELLULAR_SERVICE CELLULAR_SERVICE
  * @{
  */

/** @addtogroup CELLULAR_SERVICE_API CELLULAR_SERVICE API
  * @{
  */

/** @defgroup CELLULAR_SERVICE_API_Exported_Macros CELLULAR_SERVICE API Exported Macros
  * @{
  */
#define MAX_SIZE_IMEI            ((uint16_t) 64U)  /* MAX = 32 characters */
#define MAX_SIZE_MANUFACT_NAME   ((uint16_t) 256U) /* theoretical MAX = 2048 characters !!! */
#define MAX_SIZE_MODEL           ((uint16_t) 256U) /* theoretical MAX = 2048 characters !!! */
#define MAX_SIZE_REV             ((uint16_t) 64U)  /* theoretical MAX = 2048 characters !!! */
#define MAX_SIZE_SN              ((uint16_t) 64U)  /* theoretical MAX = 2048 characters !!! */
#define MAX_SIZE_IMSI            ((uint16_t) 64U)  /* MAX = 32 characters */
#define MAX_SIZE_PHONE_NBR       ((uint16_t) 64U)  /* MAX = 32 characters */
#define MAX_SIZE_ICCID           ((uint16_t) 32U)  /* MAX = 32 characters */
#define MAX_SIZE_OPERATOR_NAME   ((uint16_t) 64U)  /* MAX = 64 characters */
#define MAX_SIZE_USERNAME        ((uint16_t) 32U)  /* MAX = 32 characters */
#define MAX_SIZE_PASSWORD        ((uint16_t) 32U)  /* MAX = 32 characters */
#define MAX_SIZE_IPADDR          ((uint16_t) 64U)  /* MAX = 64 characters */
#define MAX_DIRECT_CMD_SIZE      ((uint16_t)(ATCMD_MAX_BUF_SIZE - 10U))

/* Predefined T3412 values (E-UTRAN), periodic TAU
 * cf TS 27.007, table 10.5.163a
 */
#define PSM_T3412_DEACTIVATED (uint8_t)(0xE0) /* "111 00000" = 0xE0 */
#define PSM_T3412_1_MIN       (uint8_t)(0xA1) /* "101.00001" = 0xA1 - for test only */
#define PSM_T3412_4_MIN       (uint8_t)(0xA4) /* "101.00100" = 0xA4 */
#define PSM_T3412_6_MIN       (uint8_t)(0xA6) /* "101.00110" = 0xA6 */
#define PSM_T3412_15_MIN      (uint8_t)(0xAF) /* "101.01111" = 0xAF */
#define PSM_T3412_40_MIN      (uint8_t)(0x04) /* "000.00110" = 0x04 */
#define PSM_T3412_4_HOURS     (uint8_t)(0x24) /* "001.00100" = 0x24 */
#define PSM_T3412_24_HOURS    (uint8_t)(0x38) /* "001.11000" = 0x38 */
#define PSM_T3412_170_HOURS   (uint8_t)(0x51) /* "010.10001" = 0x51 */
#define PSM_T3412_40_DAYS     (uint8_t)(0xD3) /* "110.00011" = 0xD3 */

/* Predefined T3324 values (GERAN/UTRAN or E-UTRAN), active timer
 * cf TS 27.007, table 10.5.163 and table 10.5.172
 */
#define PSM_T3324_DEACTIVATED (uint8_t)(0xE0) /* "111 00000" = 0xE0 */
#define PSM_T3324_10_SEC      (uint8_t)(0x05) /* "000.00101" = 0x05  - for test only */
#define PSM_T3324_16_SEC      (uint8_t)(0x08) /* "000.01000" = 0x08 */
#define PSM_T3324_2_MIN       (uint8_t)(0x22) /* "001.00010" = 0x22 */
#define PSM_T3324_4_MIN       (uint8_t)(0x24) /* "001.00100" = 0x24 */
#define PSM_T3324_12_MIN      (uint8_t)(0x2C) /* "001.01100" = 0x2C */
#define PSM_T3324_24_MIN      (uint8_t)(0x38) /* "001.11000" = 0x38 */
#define PSM_T3324_10_HOURS    (uint8_t)(0x41) /* "010.00001" = 0x41 */
#define PSM_T3324_30_HOURS    (uint8_t)(0x43) /* "010.00011" = 0x43 */
#define PSM_T3324_90_HOURS    (uint8_t)(0x49) /* "010.01001" = 0x49 */

/* Predefined PSM periodic_RAU T3312 values (GERAN/UTRAN)
 * timer not used
 */
#define PSM_T3312_DEACTIVATED (uint8_t)(0xE0) /* "111 00000" = 0xE0 */

/* Predefined PSM GPRS_READY_timer T3314 values (GERAN/UTRAN)
 * timer not used
 */
#define PSM_T3314_DEACTIVATED (uint8_t)(0xE0) /* "111 00000" = 0xE0 */

/* Predefined EDRX values */
#define EDRX_WB_S1_PTW_1S_DRX_40S  (uint8_t)(0x03) /* "0000.0011" = 0x49 : WB-S1 mode PTW=1.28 sec, EDRX=40.96 sec */

/**
  * @}
  */

/** @defgroup CELLULAR_SERVICE_API_Exported_Types CELLULAR_SERVICE API Exported Types
  * @{
  */
typedef uint8_t CS_CHAR_t;

/* To ensure backward compatibility */
typedef bool CS_Bool_t;
#define CS_FALSE false
#define CS_TRUE  true

/* enum */
typedef enum
{
  CS_OK = 0,                /* No error */
  CS_ERROR,                 /* Generic error */
  CS_NOT_IMPLEMENTED,       /* Function not implemented yet */
  /* - SIM errors - */
  CS_SIM_BUSY,               /* SIM error: SIM is busy */
  CS_SIM_NOT_INSERTED,       /* SIM error: SIM not inserted */
  CS_SIM_PIN_OR_PUK_LOCKED,  /* SIM error: SIM locked due to PIN, PIN2, PUK or PUK2 */
  CS_SIM_INCORRECT_PASSWORD, /* SIM error: SIM password is incorrect */
  CS_SIM_ERROR               /* SIM error: SIM other error */
} CS_Status_t;

typedef enum
{
  CS_CMI_MINI = 0,
  CS_CMI_FULL,
  CS_CMI_SIM_ONLY,
} CS_ModemInit_t;

typedef enum
{
  CS_MODEM_SIM_SOCKET_0 = 0,    /* to select SIM card placed in SIM socket */
  CS_MODEM_SIM_ESIM_1,          /* to select integrated SIM in modem module */
  CS_STM32_SIM_2,               /* to select SIM in STM32 side (various implementations) */
} CS_SimSlot_t;

typedef enum
{
  CS_PS_DETACHED = 0,
  CS_PS_ATTACHED = 1,
} CS_PSattach_t;

/* TS 27.007, <mode> parameter used in AT+COPS */
typedef uint16_t CS_NetworkRegMode_t;
#define CS_NRM_AUTO             (CS_NetworkRegMode_t)(0U)
#define CS_NRM_MANUAL           (CS_NetworkRegMode_t)(1U)
#define CS_NRM_DEREGISTER       (CS_NetworkRegMode_t)(2U)
#define CS_NRM_MANUAL_THEN_AUTO (CS_NetworkRegMode_t)(4U)

/* TS 27.007, <AcT> parameter used in AT+COPS and AT+CEREG */
typedef uint16_t CS_AccessTechno_t;
#define  CS_ACT_GSM               (CS_AccessTechno_t)(0U)
#define  CS_ACT_GSM_COMPACT       (CS_AccessTechno_t)(1U)
#define  CS_ACT_UTRAN             (CS_AccessTechno_t)(2U)
#define  CS_ACT_GSM_EDGE          (CS_AccessTechno_t)(3U)
#define  CS_ACT_UTRAN_HSDPA       (CS_AccessTechno_t)(4U)
#define  CS_ACT_UTRAN_HSUPA       (CS_AccessTechno_t)(5U)
#define  CS_ACT_UTRAN_HSDPA_HSUPA (CS_AccessTechno_t)(6U)
#define  CS_ACT_E_UTRAN           (CS_AccessTechno_t)(7U) /* = LTE Cat.M1 */
#define  CS_ACT_EC_GSM_IOT        (CS_AccessTechno_t)(8U)
#define  CS_ACT_E_UTRAN_NBS1      (CS_AccessTechno_t)(9U) /* = LTE Cat.NB1 */

/* TS 27.007, <stat> parameter used in AT+CEREG */
typedef uint16_t CS_NetworkRegState_t;
#define CS_NRS_NOT_REGISTERED_NOT_SEARCHING     (CS_NetworkRegState_t)(0U)
#define CS_NRS_REGISTERED_HOME_NETWORK          (CS_NetworkRegState_t)(1U)
#define CS_NRS_NOT_REGISTERED_SEARCHING         (CS_NetworkRegState_t)(2U)
#define CS_NRS_REGISTRATION_DENIED              (CS_NetworkRegState_t)(3U)
#define CS_NRS_UNKNOWN                          (CS_NetworkRegState_t)(4U)
#define CS_NRS_REGISTERED_ROAMING               (CS_NetworkRegState_t)(5U)
#define CS_NRS_REGISTERED_SMS_ONLY_HOME_NETWORK (CS_NetworkRegState_t)(6U)
#define CS_NRS_REGISTERED_SMS_ONLY_ROAMING      (CS_NetworkRegState_t)(7U)
#define CS_NRS_EMERGENCY_ONLY                   (CS_NetworkRegState_t)(8U)
#define CS_NRS_REGISTERED_CFSB_NP_HOME_NETWORK  (CS_NetworkRegState_t)(9U)
#define CS_NRS_REGISTERED_CFSB_NP_ROAMING       (CS_NetworkRegState_t)(10U)

/* TS 27.007, <format> parameter used in AT+COPS */
typedef uint16_t CS_OperatorNameFormat_t;
#define CS_ONF_LONG        (CS_OperatorNameFormat_t)(0U) /* operator name is up to 16 chars */
#define CS_ONF_SHORT       (CS_OperatorNameFormat_t)(1U) /* operator name is up to 8 chars */
#define CS_ONF_NUMERIC     (CS_OperatorNameFormat_t)(2U) /* LAI */
#define CS_ONF_NOT_PRESENT (CS_OperatorNameFormat_t)(9U) /* not part of TS 27.00: Operator Name not present */
/* (custom value) */

typedef enum
{
  CS_URCEVENT_NONE                  = 0, /* none */
  CS_URCEVENT_EPS_NETWORK_REG_STAT  = 1, /* EPS registration status (CEREG) */
  CS_URCEVENT_EPS_LOCATION_INFO     = 2, /* EPS location status (CEREG) */
  CS_URCEVENT_GPRS_NETWORK_REG_STAT = 3, /* GPRS registration status (CGREG) */
  CS_URCEVENT_GPRS_LOCATION_INFO    = 4, /* GPRS registration status (CGREG) */
  CS_URCEVENT_CS_NETWORK_REG_STAT   = 5, /* Circuit-switched registration status (CREG) */
  CS_URCEVENT_CS_LOCATION_INFO      = 6, /* Circuit-switched status (CREG) */
  CS_URCEVENT_SIGNAL_QUALITY        = 7, /* signal quality registration (if supported) */
} CS_UrcEvent_t;

typedef uint16_t CS_ModemEvent_t;
#define CS_MDMEVENT_NONE          (CS_ModemEvent_t)(0x0000) /* none */
#define CS_MDMEVENT_BOOT          (CS_ModemEvent_t)(0x0001) /* Modem Boot indication received (if exists) */
#define CS_MDMEVENT_POWER_DOWN    (CS_ModemEvent_t)(0x0002) /* Modem Power Down indication received (if exists) */
#define CS_MDMEVENT_FOTA_START    (CS_ModemEvent_t)(0x0004) /* Modem FOTA Start indication received (if exists) */
#define CS_MDMEVENT_FOTA_END      (CS_ModemEvent_t)(0x0008) /* Modem FOTA End indication received (if exists) */
#define CS_MDMEVENT_LP_ENTER      (CS_ModemEvent_t)(0x0010) /* Modem Enter Low Power state */
#define CS_MDMEVENT_LP_LEAVE      (CS_ModemEvent_t)(0x0020) /* Modem Leave Low Power state */
#define CS_MDMEVENT_WAKEUP_REQ    (CS_ModemEvent_t)(0x0040) /* Modem WakeUp request during Low Power state */
#define CS_MDMEVENT_CLOUD_READY   (CS_ModemEvent_t)(0x0080) /* Modem Cloud is ready */

typedef enum
{
  CS_SIMEVENT_SIM_REFRESH  = 0, /* SIM refresh */
  CS_SIMEVENT_SIM_DETECT   = 1, /* runtime SIM hotswap detection */
  CS_SIMEVENT_SIM_STATE    = 2, /* SIM state changed */
} CS_Sim_Event_t;

typedef uint8_t CS_Sim_extended_infos_t;
#define CS_SIMINFOS_UNKNOWN          (CS_Sim_extended_infos_t)(0x00) /* unknown event or info not available */
/* for CS_SIMEVENT_SIM_REFRESH event, no parameters  */
/* for CS_SIMEVENT_SIM_DETECT event, possible parameter values are: */
#define CS_SIMINFOS_CARD_INSERTED    (CS_Sim_extended_infos_t)(0x01) /* SIM card has been inserted */
#define CS_SIMINFOS_CARD_REMOVED     (CS_Sim_extended_infos_t)(0x02) /* SIM card has been removed */
/* for CS_SIMEVENT_SIM_STATE event, possible parameter values are: */
#define CS_SIMINFOS_SIM_DEACTIVATED  (CS_Sim_extended_infos_t)(0x03) /* SIM is deactivated */
#define CS_SIMINFOS_SIM_INITIALIZED  (CS_Sim_extended_infos_t)(0x04) /* SIM is initialized (before PIN code) */
#define CS_SIMINFOS_SIM_ACTIVATED    (CS_Sim_extended_infos_t)(0x05) /* SIM is activated (ready) */

typedef struct
{
  CS_Sim_Event_t           event; /* indicates which Sim event is reported */
  CS_Sim_extended_infos_t  param1; /* param1 value depends of event type */
} CS_SimEvent_status_t;

typedef enum
{
  CS_IPAT_INVALID = 0,   /* invalid format */
  CS_IPAT_IPV4,          /* IPV4 */
  CS_IPAT_IPV6,          /* IPV6 */
} CS_IPaddrType_t;

typedef uint16_t CS_DeviceInfoFields_t;
#define  CS_DIF_IMEI_PRESENT               (CS_DeviceInfoFields_t)(0x01)
#define  CS_DIF_MANUF_NAME_PRESENT         (CS_DeviceInfoFields_t)(0x02)
#define  CS_DIF_MODEL_PRESENT              (CS_DeviceInfoFields_t)(0x04)
#define  CS_DIF_REV_PRESENT                (CS_DeviceInfoFields_t)(0x08)
#define  CS_DIF_SN_PRESENT                 (CS_DeviceInfoFields_t)(0x10)
#define  CS_DIF_IMSI_PRESENT               (CS_DeviceInfoFields_t)(0x20)
#define  CS_DIF_PHONE_NBR_PRESENT          (CS_DeviceInfoFields_t)(0x40)
#define  CS_DIF_ICCID_PRESENT              (CS_DeviceInfoFields_t)(0x80)

typedef enum
{
  CS_PDPTYPE_IP                  = 0x00,
  CS_PDPTYPE_IPV6                = 0x01,
  CS_PDPTYPE_IPV4V6              = 0x02,
  CS_PDPTYPE_PPP                 = 0x03,
  /*    CS_PDPTYPE_X25     */         /* NOT SUPPORTED - OBSOLETE */
  /*    CS_PDPTYPE_OSPIH   */         /* NOT SUPPORTED - OBSOLETE */
  /*    CS_PDPTYPE_NON_IP  */         /* NOT SUPPORTED */
} CS_PDPtype_t;

typedef enum
{
  CS_RESET_SW             = 0,
  CS_RESET_HW             = 1,
  CS_RESET_AUTO           = 2,
  CS_RESET_FACTORY_RESET  = 3,
} CS_Reset_t;

typedef enum
{
  CS_PDN_PREDEF_CONFIG  = 0,     /* pre-defined configuration, cannot be modified by user */
  CS_PDN_USER_CONFIG_1  = 1,     /* user defined configuration */
  CS_PDN_USER_CONFIG_2  = 2,     /* user defined configuration */
  CS_PDN_USER_CONFIG_3  = 3,     /* user defined configuration */
  CS_PDN_USER_CONFIG_4  = 4,     /* user defined configuration */
  CS_PDN_USER_CONFIG_5  = 5,     /* user defined configuration */
  CS_PDN_CONFIG_MAX     = CS_PDN_USER_CONFIG_5,
  CS_PDN_CONFIG_DEFAULT = 11,  /* use one of previous config. (if not set, will use PDN_PREDEF_CONFIG by default) */
  CS_PDN_NOT_DEFINED    = 12,     /* for internal use only */
  CS_PDN_ALL            = 13,     /* for internal use only: all PDN are concerned */
} CS_PDN_conf_id_t;

typedef enum
{
  CS_PDN_EVENT_OTHER, /*none of the event described below */
  CS_PDN_EVENT_NW_DETACH,
  CS_PDN_EVENT_NW_DEACT,
  CS_PDN_EVENT_NW_PDN_DEACT,
} CS_PDN_event_t;

/* structures */
typedef struct
{
  /* Not implemented yet */
  /* this configuration structure will be used to configure modem at boot time
     also when reset triggered ? */
  CS_Bool_t test_variable;
} CS_ModemConfig_t;

typedef struct
{
  CS_DeviceInfoFields_t field_requested; /* device info field request below */
  union
  {
    CS_CHAR_t imei[MAX_SIZE_IMEI];
    CS_CHAR_t manufacturer_name[MAX_SIZE_MANUFACT_NAME];
    CS_CHAR_t model[MAX_SIZE_MODEL];
    CS_CHAR_t revision[MAX_SIZE_REV];
    CS_CHAR_t serial_number[MAX_SIZE_SN];
    CS_CHAR_t imsi[MAX_SIZE_IMSI];
    CS_CHAR_t phone_number[MAX_SIZE_PHONE_NBR];
    CS_CHAR_t iccid[MAX_SIZE_ICCID];
  } u;

} CS_DeviceInfo_t;

typedef struct
{
  CS_NetworkRegMode_t      mode;
  CS_OperatorNameFormat_t  format;
  CS_CHAR_t                operator_name[MAX_SIZE_OPERATOR_NAME];
  CS_Bool_t                AcT_present;
  CS_AccessTechno_t        AcT;

} CS_OperatorSelector_t;

typedef uint16_t CS_RegistrationStatusFields_t;
#define CS_RSF_NONE                    (CS_RegistrationStatusFields_t)(0x00U)
#define CS_RSF_FORMAT_PRESENT          (CS_RegistrationStatusFields_t)(0x01U)
#define CS_RSF_OPERATOR_NAME_PRESENT   (CS_RegistrationStatusFields_t)(0x02U)
#define CS_RSF_ACT_PRESENT             (CS_RegistrationStatusFields_t)(0x04U)

typedef struct
{
  CS_NetworkRegMode_t               mode;                                   /* mandatory field */
  CS_NetworkRegState_t              EPS_NetworkRegState;                    /* mandatory field */
  CS_NetworkRegState_t              GPRS_NetworkRegState;                   /* mandatory field */
  CS_NetworkRegState_t              CS_NetworkRegState;                     /* mandatory field */

  CS_RegistrationStatusFields_t     optional_fields_presence;               /* which fields below are present */
  CS_OperatorNameFormat_t           format;                                 /* optional field */
  CS_CHAR_t                         operator_name[MAX_SIZE_OPERATOR_NAME];  /* optional field */
  CS_AccessTechno_t                 AcT;                                    /* optional field */

} CS_RegistrationStatus_t;

typedef struct
{
  uint8_t rssi;
  uint8_t ber;
} CS_SignalQuality_t;

typedef struct
{
  CS_PDPtype_t  pdp_type;  /* if NULL, applies default value */
  CS_CHAR_t     username[MAX_SIZE_USERNAME];
  CS_CHAR_t     password[MAX_SIZE_PASSWORD];
} CS_PDN_configuration_t;

typedef struct
{
  CS_CHAR_t cmd_str[MAX_DIRECT_CMD_SIZE]; /* the command string to send to the modem (without termination characters)*/
  uint16_t  cmd_size;                     /* size of cmd_str */
  uint32_t  cmd_timeout;                  /* timeout (in ms) to apply. If set to 0, will use default timer value */
} CS_direct_cmd_tx_t;

typedef struct
{
  CS_CHAR_t cmd_str[MAX_DIRECT_CMD_SIZE]; /* the command string to send to the modem (without termination characters)*/
  uint16_t  cmd_size;
  CS_Bool_t cmd_final;
} CS_direct_cmd_rx_t;

typedef struct
{
  const CS_CHAR_t  *p_cmd_str;    /* command buffer send to modem */
  CS_CHAR_t        *p_rsp_str;    /* response buffer received from modem */
  uint32_t         cmd_str_size;  /* size of the buffer pointed by p_cmd_str */
  uint32_t         rsp_str_size;  /* size of the buffer pointed by rsp_str_size */
} CS_sim_generic_access_t;

/* callbacks */
typedef void (* cellular_urc_callback_t)(void);
typedef void (* cellular_modem_event_callback_t)(CS_ModemEvent_t modem_event_received);
typedef void (* cellular_pdn_event_callback_t)(CS_PDN_conf_id_t cid, CS_PDN_event_t pdn_event);
typedef void (* cellular_direct_cmd_callback_t)(CS_direct_cmd_rx_t direct_cmd_rx);
typedef void (* cellular_sim_event_callback_t)(CS_SimEvent_status_t sim_event_infos);

/* LOW POWER API */

/* <<< LTE-M (cat.M1) deployment guide >>>
  *
  * PSM STANDALONE TIMERS
  * =====================
  * PSM turns off monitoring of Paging instances on the device and increases time periods of devices
  * sending Periodic Tracking Area Updates (pTAUs) to extended intervals to inform the network of its
  * current registration.
  * PSM has 2 timers:
  *   T3324 Active Timer: time the "IOT module" stays in ACTIVE/IDLE mode following a wake-up periodic TAU or
  *                       initiate a Mobile Origination event.
  *   T3412 Timer: this is the value the device informs the network (with periodic TAU) that it is still registered.
  *                T3412 resets after Mobile Origination events.
  *
  *   Recommended ratio of T3324/T3412 should be > 90%
  *   T3324: no recommended value but minimum value = 16 sec
  *   T3412: minimum recommended value = 4 hours, maximum value = 413 days
  *
  * eDRX STANDALONE
  * ===============
  * eDRX can be used without PSM or in conjunction with PSM.
  * During DRX, the device switch off network reception for a brief moment.
  * eDRX allows the time interval during which a device is not listening to the network to be greatly extended.
  * Mobile Origination events can be triggered at any time.
  *
  *  Recommended LTE-M values: minimum of 5.12 sec, maximum of 43.69 min
  *
  * PSM and eDRX COMBINED IMPLEMENTATION
  * ====================================
  * Combinent implementation is recommended.
  * Careful alignment is needed between different configuration parameters.
  *
  */
typedef uint8_t CS_PSM_mode_t;
#define PSM_MODE_DISABLE       (CS_PSM_mode_t)(0x0U)
#define PSM_MODE_ENABLE        (CS_PSM_mode_t)(0x1U)

typedef uint8_t CS_EDRX_mode_t;
#define EDRX_MODE_DISABLE            (CS_EDRX_mode_t)(0x0)
#define EDRX_MODE_ENABLE             (CS_EDRX_mode_t)(0x1)
#define EDRX_MODE_ENABLE_WITH_URC    (CS_EDRX_mode_t)(0x2)
#define EDRX_MODE_DISABLE_AND_RESET  (CS_EDRX_mode_t)(0x3)

typedef uint8_t CS_EDRX_AcT_type_t;
#define EDRX_ACT_NOT_USED          (CS_EDRX_AcT_type_t)(0x0) /* */
#define EDRX_ACT_EC_GSM_IOT        (CS_EDRX_AcT_type_t)(0x1) /* */
#define EDRX_ACT_GSM               (CS_EDRX_AcT_type_t)(0x2) /* */
#define EDRX_ACT_UTRAN             (CS_EDRX_AcT_type_t)(0x3) /* */
#define EDRX_ACT_E_UTRAN_WB_S1     (CS_EDRX_AcT_type_t)(0x4) /* = LTE or LTE Cat.M1 */
#define EDRX_ACT_E_UTRAN_NB_S1     (CS_EDRX_AcT_type_t)(0x5) /* = LTE Cat.NB1 */

typedef struct
{
  uint8_t             req_periodic_RAU;     /* GERAN/UTRAN networks, (T3312), cf Table 10.5.163a from TS 24.008 */
  uint8_t             req_GPRS_READY_timer; /* GERAN/UTRAN networks, T3314), cf Table 10.5.172 from TS 24.008 */
  uint8_t             req_periodic_TAU;     /* E-UTRAN networks, (T3412), cf Table 10.5.163a from TS 24.008 */
  uint8_t             req_active_time;      /* GERAN/UTRAN and E-UTRAN networks, (T3324),
                                             *    cf Table 10.5.163 from TS 24.008 */
} CS_PSM_params_t;

typedef struct
{
  CS_EDRX_AcT_type_t  act_type;
  uint8_t             req_value;
} CS_EDRX_params_t;

typedef struct
{
  uint32_t  nwk_periodic_TAU; /* negotiated value of T3412, expressed in seconds (0 means not available) */
  uint32_t  nwk_active_time;  /* negotiated value of T3324, expressed in seconds (0 means not available) */
} CS_LowPower_status_t;

typedef struct
{
  CS_Bool_t  low_power_enable;    /* if false: ignore psm and edrx structures below
                                   * if true:  take into account psm and edrx structures
                                   */
  /* PSM and EDRX structures */
  CS_PSM_params_t  psm;
  CS_EDRX_params_t edrx;

} CS_init_power_config_t;

typedef struct
{
  CS_Bool_t  psm_present;              /* indicates if PSM parameters below are present */
  CS_Bool_t  edrx_present;             /* indicates if eDRX parameters below are present */

  /* PSM and EDRX structures */
  CS_PSM_mode_t    psm_mode;           /* requested PSM mode */
  CS_PSM_params_t  psm;
  CS_EDRX_mode_t   edrx_mode;          /* requested eDRX mode */
  CS_EDRX_params_t edrx;

} CS_set_power_config_t;

typedef enum
{
  UNKNOWN_WAKEUP = 0,
  HOST_WAKEUP  = 1,
  MODEM_WAKEUP = 2,
} CS_wakeup_origin_t;

/* PSM callback used to retrieve LowPower information
 *  - value of PSM timers negotiated between modem and network (T3412 & T3324)
 */
typedef void (* cellular_power_status_callback_t)(CS_LowPower_status_t lp_status);

/**
  * @}
  */

/** @defgroup CELLULAR_SERVICE_API_Exported_Functions CELLULAR_SERVICE API Exported Functions
  * @{
  */
CS_Status_t CS_init(void);
CS_Status_t CS_power_on(void);
CS_Status_t CS_power_off(void);
CS_Status_t CS_check_connection(void);
CS_Status_t CS_sim_select(CS_SimSlot_t simSelected);
int32_t     CS_sim_generic_access(CS_sim_generic_access_t *sim_generic_access);
CS_Status_t CS_init_modem(CS_ModemInit_t init, CS_Bool_t reset, const CS_CHAR_t *pin_code);
CS_Status_t CS_get_device_info(CS_DeviceInfo_t *p_devinfo);
CS_Status_t CS_register_net(CS_OperatorSelector_t *p_operator,
                            CS_RegistrationStatus_t *p_reg_status);
CS_Status_t CS_deregister_net(CS_RegistrationStatus_t *p_reg_status);
CS_Status_t CS_get_net_status(CS_RegistrationStatus_t *p_reg_status);
CS_Status_t CS_subscribe_net_event(CS_UrcEvent_t event, cellular_urc_callback_t urc_callback);
CS_Status_t CS_unsubscribe_net_event(CS_UrcEvent_t event);
CS_Status_t CS_subscribe_sim_event(cellular_sim_event_callback_t sim_evt_callback);
CS_Status_t CS_attach_PS_domain(void);
CS_Status_t CS_detach_PS_domain(void);
CS_Status_t CS_get_attach_status(CS_PSattach_t *p_attach);
CS_Status_t CS_get_signal_quality(CS_SignalQuality_t *p_sig_qual);
CS_Status_t CS_activate_pdn(CS_PDN_conf_id_t cid);
CS_Status_t CS_deactivate_pdn(CS_PDN_conf_id_t cid);
CS_Status_t CS_define_pdn(CS_PDN_conf_id_t cid, const CS_CHAR_t *apn, CS_PDN_configuration_t *pdn_conf);
CS_Status_t CS_set_default_pdn(CS_PDN_conf_id_t cid);
CS_Status_t CS_get_dev_IP_address(CS_PDN_conf_id_t cid, CS_IPaddrType_t *ip_addr_type, CS_CHAR_t *p_ip_addr_value);
CS_Status_t CS_suspend_data(void);
CS_Status_t CS_resume_data(void);
CS_Status_t CS_subscribe_modem_event(CS_ModemEvent_t events_mask, cellular_modem_event_callback_t modem_evt_cb);
CS_Status_t CS_register_pdn_event(CS_PDN_conf_id_t cid, cellular_pdn_event_callback_t pdn_event_callback);
CS_Status_t CS_deregister_pdn_event(CS_PDN_conf_id_t cid);
CS_Status_t CS_reset(CS_Reset_t rst_type);
CS_Status_t CS_direct_cmd(CS_direct_cmd_tx_t *direct_cmd_tx, cellular_direct_cmd_callback_t direct_cmd_callback);

/* LOW POWER API */
CS_Status_t CS_InitPowerConfig(CS_init_power_config_t *p_power_config,
                               cellular_power_status_callback_t lp_status_callback);
CS_Status_t CS_SetPowerConfig(CS_set_power_config_t *p_power_config);
CS_Status_t CS_SleepRequest(void);
CS_Status_t CS_SleepComplete(void);
CS_Status_t CS_SleepCancel(void);
CS_Status_t CS_PowerWakeup(CS_wakeup_origin_t wakeup_origin);

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#if defined(USE_COM_MDM)

typedef struct
{
  CS_CHAR_t  *p_buffer;    /* Input buffer pointer */
  uint32_t   buffer_size;  /* Input buffer size */
} CS_Tx_Buffer_t;

typedef struct
{
  uint32_t   max_buffer_size; /* maximum size allowed for Output buffer */
  CS_CHAR_t  *p_buffer;       /* Output buffer pointer */
  uint32_t   buffer_size;     /* Output buffer effective size */
} CS_Rx_Buffer_t;

typedef struct
{
  uint32_t  param1; /* this is an example, may evolve */
} CS_comMdm_status_t;

typedef void (* CS_comMdm_callback_t)(CS_comMdm_status_t comMdmd_event_infos);

CS_Status_t CS_ComMdm_subscribe_event(CS_comMdm_callback_t commdm_urc_cb);
CS_Status_t CS_ComMdm_send(CS_Tx_Buffer_t *txBuf, int32_t *errorCode);
CS_Status_t CS_ComMdm_transaction(CS_Tx_Buffer_t *txBuf, CS_Rx_Buffer_t *rxBuf, int32_t *errorCode);
CS_Status_t CS_ComMdm_receive(CS_Rx_Buffer_t *rxBuf, int32_t *errorCode);

#endif /* defined(USE_COM_MDM) */


#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_SERVICE_CONTROL_H */

