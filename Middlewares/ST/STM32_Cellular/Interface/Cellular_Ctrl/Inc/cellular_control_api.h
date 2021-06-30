/**
  ******************************************************************************
  * @file    cellular_control_api.h
  * @author  MCD Application Team
  * @brief   Control api definition
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
#ifndef CELLULAR_CONTROL_API_H
#define CELLULAR_CONTROL_API_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

#include "plf_config.h"
#include "cellular_control_common.h"
#include "com_sockets.h"

/** @defgroup CELLULAR_CTRL CTRL: Control Module
  * @{
  */

/**
  * @}
  */


/** @addtogroup CELLULAR_CTRL
  * @{
  */

/* Exported constants --------------------------------------------------------*/

/** @defgroup CELLULAR_CTRL_Constants Constants
  * @{
  */

/** @brief Cellular Firmware Name value\n
  * When changing the value, please change CELLULAR_VERSION_FIRMWARE_NAME_LEN accordingly */
#define CELLULAR_VERSION_FIRMWARE_NAME         "X-CUBE-CELLULAR"
/** @brief Cellular Firmware Name lnegth\n
  * When changing the value, please change CELLULAR_VERSION_FIRMWARE_NAME_LEN accordingly */
#define CELLULAR_VERSION_FIRMWARE_NAME_LEN     (uint8_t)15

/** @brief Cellular firmware major version values */
#define CELLULAR_VERSION_MAJOR   6U
/** @brief Cellular firmware minor version values */
#define CELLULAR_VERSION_MINOR   0U
/** @brief Cellular firmware patch version values */
#define CELLULAR_VERSION_PATCH   0U
/** @brief Cellular firmware stage version Dev (1) or Release (0) */
#define CELLULAR_VERSION_IS_DEV  0U
/** @brief Cellular firmware name max length */
/* Version string is : <firmware-name>-<MMMMM>.<mmm>.<PPP>_DEV:                                                   */
/* 19 is the sum of max length of each part excluding the firmware name : 5 for Major version (16 bits number), */
/*     3 for minor and patch version (8 bits number), and 8 for '-', '.', '_DEV' and trailing '\0'              */
/* 15 is the actual name length                                                                                 */
#define CELLULAR_VERSION_STR_LEN     (uint8_t)((uint8_t)19 + CELLULAR_VERSION_FIRMWARE_NAME_LEN)

/* For each possible call back type, defines the maximum of it that will be used simultaneously */
/* by all applications using the API                                                           */
/* Examples :                                                                                  */
/* - App-1 registered to call back "sim info" and "cellular info"                              */
/*     2 call back types, and none used simultaneously                                         */
/*     => define to 1                                                                          */
/* - app-1 registered to "cellular info" and "signal info"                                     */
/*   app-2 registered to "signal info" and "IP info"                                           */
/*     3 call back types, and "signal info" used simultaneously by the 2 apps                  */
/*     => define to 2                                                                          */
#ifndef CELLULAR_CONCURENT_REGISTRATION_CB_MAX_NB
#define CELLULAR_CONCURENT_REGISTRATION_CB_MAX_NB (1U)
#endif /* CELLULAR_CONCURENT_REGISTRATION_CB_MAX_NB */

#define CA_ICCID_SIZE_MAX                   (uint8_t)(20U + 1U) /*!< ICCID size max: 22U + 1U for \0                  */
#define CA_IMSI_SIZE_MAX                    (uint8_t)(15U + 1U) /*!< IMSI size max: 16U + 1U for \0                   */
#define CA_MCC_SIZE_MAX                     (uint8_t)(3U + 1U)  /*!< MCC size max: 3U + 1U for \0                     */
#define CA_MNC_SIZE_MAX                     (uint8_t)(3U + 1U)  /*!< MNC size max: 3U + 1U for \0                     */
#define CA_MSIN_SIZE_MAX                    (uint8_t)(10U + 1U) /*!< MSIN size max: 10U + 1U for \0                   */
#define CA_IMEI_SIZE_MAX                    (uint8_t)(16U + 1U) /*!< IMEI size max: 16U + 1U for \0                   */
#define CA_MNO_NAME_SIZE_MAX                (uint8_t)(32U + 1U) /*!< MNO - MobileNetworkOperator name size max:
                                                                     32U + 1U for \0                                  */

#define CA_MANUFACTURER_ID_SIZE_MAX         (uint8_t)(32U + 1U) /*!< Manufacturer Identity size max: 32U + 1U for \0  */
#define CA_MODEL_ID_SIZE_MAX                (uint8_t)(32U + 1U) /*!< Model Identity size max: 32U + 1U for \0         */
#define CA_REVISION_ID_SIZE_MAX             (uint8_t)(32U + 1U) /*!< Revision size max: 32U + 1U for \0               */
#define CA_SERIAL_NUMBER_ID_SIZE_MAX        (uint8_t)(32U + 1U) /*!< Serial Number Identity size max: 32U + 1U for \0 */

#define CA_OPERATOR_NAME_SIZE_MAX           (uint8_t)(64U + 1U) /*!< Operator Name size max: 64U + 1U for \0          */
#define CA_APN_SIZE_MAX                     (uint8_t)(32U + 1U) /*!< APN size max: 32U + 1U for \0                    */
#define CA_USERNAME_SIZE_MAX                (uint8_t)(32U + 1U) /*!< Username size max: 32U + 1U for \0               */
#define CA_PASSWORD_SIZE_MAX                (uint8_t)(32U + 1U) /*!< Password size max: 32U + 1U for \0               */

#define CA_NFMC_VALUES_MAX_NB               (uint8_t)7U         /*!< Maximum NFMC values number                       */

/** @brief  Specific value of cellular signal level in raw */
#define CA_SIGNAL_STRENGTH_UNKNOWN          (uint8_t)99          /*!< Value when it is unknown or not detectable      */

/**
  * @}
  */

/* Exported types ------------------------------------------------------------*/

/** @defgroup CELLULAR_CTRL_Types Types
  * @{
  */

/** @brief  Modem State list */
typedef enum
{
  CA_MODEM_STATE_POWERED_ON       = (uint8_t)0x01,  /*!< Modem powered on                               */
  CA_MODEM_STATE_SIM_CONNECTED    = (uint8_t)0x02,  /*!< Modem started and SIM is connected             */
  CA_MODEM_NETWORK_SEARCHING      = (uint8_t)0x03,  /*!< Modem is searching network                     */
  CA_MODEM_NETWORK_REGISTERED     = (uint8_t)0x04,  /*!< Modem registered on network                    */
  CA_MODEM_STATE_DATAREADY        = (uint8_t)0x05,  /*!< Modem started and data is ready                */
  CA_MODEM_IN_FLIGHTMODE          = (uint8_t)0x06,  /*!< Modem in flight mode                           */
  CA_MODEM_REBOOTING              = (uint8_t)0x07,  /*!< Modem is rebooting                             */
  CA_MODEM_FOTA_INPROGRESS        = (uint8_t)0x08,  /*!< Modem is in FOTA update                        */
  CA_MODEM_POWER_OFF              = (uint8_t)0xFF   /*!< Modem not started / power off                  */
} ca_modem_state_t;

/** @brief  SIM Status list */
typedef enum
{
  CA_SIM_READY                    = (uint8_t)0x00,  /*!< SIM initialized and ready                      */
  CA_SIM_STATUS_UNKNOWN           = (uint8_t)0x01,  /*!< SIM status is not yet known                    */
  CA_SIM_CONNECTION_ONGOING       = (uint8_t)0x02,  /*!< SIM connection ongoing                         */
  CA_SIM_PIN_OR_PUK_LOCKED        = (uint8_t)0x03,  /*!< SIM locked with PIN or PUK                     */
  CA_SIM_INCORRECT_PIN            = (uint8_t)0x04,  /*!< SIM with an incorrect PIN password provided    */
  CA_SIM_BUSY                     = (uint8_t)0x05,  /*!< Sim busy - too many busy rsp during init       */
  CA_SIM_ERROR                    = (uint8_t)0x06,  /*!< SIM error - too many error rsp during init     */
  CA_SIM_NOT_INSERTED             = (uint8_t)0x07,  /*!< No SIM inserted in the SIM Slot                */
  CA_SIM_NOT_USED                 = (uint8_t)0x08,  /*!< Did not try any access to that sim slot         */
  CA_SIM_NOT_IMPLEMENTED          = (uint8_t)0xFF   /*!< Not implemented on this hardware               */
} ca_sim_status_t;

/** @brief Socket Type list */
typedef enum
{
  CA_SOCKETS_LWIP                 = (uint8_t)0x00,  /*!< Platform is using Sockets LwIP                 */
  CA_SOCKETS_MODEM                = (uint8_t)0x01   /*!< Platform is using Sockets Modem                */
} ca_sockets_type_t;

/** @brief access techno present */
typedef enum
{
  CA_ACT_NOT_PRESENT              = (bool)false,    /*!< No access techno set                            */
  CA_ACT_PRESENT                  = (bool)true      /*!< Access techno set                               */
} ca_access_techno_present_t;

/** @brief  Access techno list */
typedef enum
{
  CA_ACT_GSM                      = (uint8_t)0x00,  /*!< Value for GSM                                  */
  CA_ACT_E_UTRAN                  = (uint8_t)0x07,  /*!< Value for LTE Cat.M1                           */
  CA_ACT_E_UTRAN_NBS1             = (uint8_t)0x09   /*!< Value for LTE Cat.NB1                          */
} ca_access_techno_t;

/** @brief APN to be send to modem list */
typedef enum
{
  CA_APN_SEND_TO_MODEM            = (uint8_t)0x01,  /*!< APN is send to modem for network attachment    */
  CA_APN_NOT_SEND_TO_MODEM        = (uint8_t)0x02   /*!< APN is NOT send to modem.                      */
} cellular_apn_send_to_modem_t;

/** @brief  SIM slot type list*/
typedef enum
{
  CA_SIM_REMOVABLE_SLOT           = (uint8_t)0x00,  /*!< SIM inserted in a card reader                  */
  CA_SIM_EXTERNAL_MODEM_SLOT      = (uint8_t)0x01,  /*!< SIM soldered external to the Modem             */
  CA_SIM_INTERNAL_MODEM_SLOT      = (uint8_t)0x02,  /*!< SIM soldered internal to the Modem             */
  CA_SIM_NON_EXISTING_SLOT        = (uint8_t)99U    /*!< Not applicable / non existing sim slot         */
} ca_sim_slot_type_t;

/** @brief Operator Name Format List */
typedef enum
{
  CA_OPERATOR_NAME_FORMAT_LONG        = (uint8_t)0x00,  /*!< Up to 16 chars            */
  CA_OPERATOR_NAME_FORMAT_SHORT       = (uint8_t)0x01,  /*!< Up to 8 chars             */
  CA_OPERATOR_NAME_FORMAT_NUMERIC     = (uint8_t)0x02,  /*!< LAI                       */
  CA_OPERATOR_NAME_FORMAT_NOT_PRESENT = (uint8_t)0x09   /*!< Operator Name not present */
} ca_operator_name_format_t;

/** @brief Network Registration Mode list */
typedef enum
{
  /*!< operator name and techno will no be used for network attachment */
  CA_NTW_REGISTRATION_AUTO              = (uint8_t)0x00,
  /*!< provided operator name and techno will be used for network attachment */
  CA_NTW_REGISTRATION_MANUAL            = (uint8_t)0x01,
  /*!< unregistered from network */
  CA_NTW_REGISTRATION_DEREGISTER        = (uint8_t)0x02,
  /*!< provided operator name and techno will be first used for network attachment. But if attachment is not possible
       will nex try automatic mode without using provided operator name and techno */
  CA_NTW_REGISTRATION_MANUAL_THEN_AUTO  = (uint8_t)0x04
} ca_ntw_registration_mode_t;

/** @brief  Event type list */
typedef enum
{
  CA_CELLULAR_INFO_EVENT          = (uint8_t)0x01,  /*!< Cellular information event type value                    */
  CA_SIGNAL_INFO_EVENT            = (uint8_t)0x02,  /*!< Signal information event type                            */
  CA_SIM_INFO_EVENT               = (uint8_t)0x03,  /*!< SIM information event type value                         */
  CA_NFMC_INFO_EVENT              = (uint8_t)0x04,  /*!< NFMC information event type value                        */
  CA_IP_INFO_EVENT                = (uint8_t)0x05,  /*!< IP information event type value                          */
  CA_POWER_INFO_EVENT             = (uint8_t)0x06   /*!< Power information event type value                       */
} ca_event_type_t;

#if (USE_LOW_POWER == 1)
/** @brief  Power State list */
typedef enum
{
  CA_POWER_LOWPOWER_INACTIVE      = (uint8_t)0x00,  /*!< Modem is not in low power mode                 */
  CA_POWER_LOWPOWER_ONGOING       = (uint8_t)0x01,  /*!< Modem is going to low power mode               */
  CA_POWER_IN_LOWPOWER            = (uint8_t)0x02,  /*!< Modem in low power mode                        */
} ca_power_state_t;

/** @brief Power Mode list */
typedef enum
{
  /*!< Real time power:   : STM32 RUN_LP      / eIDRX disabled / PSM disabled */
  CA_POWER_RUN_REAL_TIME          = (uint8_t)0x00,
  /*!< Interactive power 0: STM32 RUN_LP      / eIDRX disabled / PSM disabled */
  CA_POWER_RUN_INTERACTIVE_0      = (uint8_t)0x01,
  /*!< Interactive power 1: STM32 RUN_LP      / eIDRX disabled / PSM enabled  */
  CA_POWER_RUN_INTERACTIVE_1      = (uint8_t)0x02,
  /*!< Interactive power 2: STM32 RUN_LP      / eIDRX enabled  / PSM enabled  */
  CA_POWER_RUN_INTERACTIVE_2      = (uint8_t)0x03,
  /*!< Interactive power 3: STM32 RUN_LP      / eIDRX enabled  / PSM disabled */
  CA_POWER_RUN_INTERACTIVE_3      = (uint8_t)0x04,
  /*!< Idle power         : STM32 STOP2       / eIDRX disabled / PSM disabled */
  CA_POWER_IDLE                   = (uint8_t)0x05,
  /*!< Idle low power     : STM32 STOP2       / eIDRX allowed  / PSM allowed  */
  CA_POWER_IDLE_LP                = (uint8_t)0x06,
  /*!< low power          : STM32 STOP2       / eIDRX enabled  / PSM enabled  */
  CA_POWER_LP                     = (uint8_t)0x07,
  /*!< Ultra Low power    : STM32 STBY-shdown / eIDRX enabled  / PSM enabled  */
  CA_POWER_ULP                    = (uint8_t)0x08,
  /*!< STanby 1 Low power : STM32 STBY w RTC  / Modem Off     / Network Off  */
  CA_POWER_STANDBY1               = (uint8_t)0x09,
  /*!< STanby 2 Low power : STM32 STBY w RTC  / Modem Off     / Network Off  */
  CA_POWER_STANDBY2               = (uint8_t)0x0A,
  /*!< Modem OFF          : STM32 shutdown    / Modem Off     / Network Off  */
  CA_POWER_OFF                   = (uint8_t)0x0B
} ca_power_mode_t;

/** @brief Power command */
typedef enum
{
  CA_POWER_CMD_INIT               = (uint8_t)0x00,  /*!< Power parameters first init       */
  CA_POWER_CMD_SETTING            = (uint8_t)0x01   /*!< Power parameters settings         */
} ca_power_cmd_t;

/** @brief PSM mode */
typedef enum
{
  CA_PSM_MODE_DISABLE             = (bool)false,   /*!< Disable the use of PSM                                  */
  CA_PSM_MODE_ENABLE              = (bool)true    /*!< Enable the use of PSM                                   */
} ca_psm_mode_t;

/** @brief eIDRX settings act type */
typedef enum
{
  CA_EIDRX_ACT_NOT_USED           = (uint8_t)0x00,  /*!< Not used                  */
  CA_EIDRX_ACT_EC_GSM_IOT         = (uint8_t)0x01,  /*!< GSM IOT                   */
  CA_EIDRX_ACT_GSM                = (uint8_t)0x02,  /*!< GSM                       */
  CA_EIDRX_ACT_UTRAN              = (uint8_t)0x03,  /*!< UTRAN                     */
  CA_EIDRX_ACT_E_UTRAN_WBS1       = (uint8_t)0x04,  /*!< LTE                       */
  CA_EDRX_ACT_E_UTRAN_NBS1        = (uint8_t)0x05   /*!< LTE Cat.M1 or LTE Cat.NB1 */
} ca_eidrx_act_type_t;

/** @brief eIDRX mode */
typedef enum
{
  CA_EIDRX_MODE_DISABLE           = (uint8_t)0x00,  /*!< Disable the use of eIDRX                                 */
  CA_EIDRX_MODE_ENABLE            = (uint8_t)0x01,  /*!< Enable the use of eIDRX                                  */
  CA_EIDRX_MODE_ENABLE_WITH_URC   = (uint8_t)0x02,  /*!< Enable the use of eIDRX
                                                         and enable the unsolicited result code                   */
  CA_EIDRX_MODE_DISABLE_AND_RESET = (uint8_t)0x03   /*!< Disable the use of eIDRX and discard
                                                         all parameters for eIDRX or, if available,
                                                         reset to the manufacturer specific default values.       */
} ca_eidrx_mode_t;
#endif  /* (USE_LOW_POWER == 1) */

/* Exported Structures -------------------------------------------------------*/

/** @brief  Structure definition of cellular signal strength */
typedef struct
{
  uint8_t                         raw_value;  /*!< Raw signal strength: range 0-99
                                                   0 - 98: Signal strength
                                                   99    : Not known or not detectable see CA_SIGNAL_LEVEL_UNKNOWN */
  int32_t                         db_value;   /*!< Signal strength in dB                                           */
} cellular_signal_strength_t;

/** @brief  Structure definition of cellular network signal information */
typedef struct
{
  cellular_signal_strength_t      signal_strength;  /*!< Signal strength    */
  ca_access_techno_t              access_techno;    /*!< Access technology  */
} cellular_signal_info_t;

/**
  * @brief  Structure definition of ICCID: Integrated Circuit Card IDentifier.
  *         According to the GSM Phase 1, the ICCID length is defined as an opaque data field, 10 octets (20 digits).
  * @note   If ICCID is unknown: len = 0U and value[0]='\0'\n
  *         If ICCID is known  : len > 0U and value[len]='\0'
  */
typedef struct
{
  uint8_t                         len;                       /*!< len of ICCID value          */
  uint8_t                         value[CA_ICCID_SIZE_MAX];  /*!< ICCID value. Format: octets */
} cellular_iccid_t;

/**
  * @brief  Structure definition of IMSI: International Mobile Subscriber Identity.
  *         IMSI is usually a 15-digit number but can be shorter.
  * @note   If IMSI is unknown: len = 0U and value[0]='\0'\n
  *         If IMSI is known  : len > 0U and value[len]='\0'
  */
typedef struct
{
  uint8_t                         len;                      /*!< len of IMSI value          */
  uint8_t                         value[CA_IMSI_SIZE_MAX];  /*!< IMSI value. Format: octets */
} cellular_imsi_t;

/**
  * @brief  Structure definition of IMSI:\n
  *         The first three digits represent the MCC: Mobile Country Code.\n
  *         The next two or three digits represent the MNC: Mobile Network Code;
  *         2-digit: European standard or 3-digit: North American standard.\n
  *         The remaining digits are the MSIN: Mobile Subscription Identification Number.
  */
typedef struct
{
  uint8_t                         mcc_len;                       /*!< len of MCC value           */
  uint8_t                         mcc_value[CA_MCC_SIZE_MAX];    /*!< MCC value. Format: octets  */
  uint8_t                         mnc_len;                       /*!< len of MNC value           */
  uint8_t                         mnc_value[CA_MNC_SIZE_MAX];    /*!< MNC value. Format: octets  */
  uint8_t                         msin_len;                      /*!< len of MSIN value          */
  uint8_t                         msin_value[CA_MSIN_SIZE_MAX];  /*!< MSIN value. Format: octets */
} cellular_imsi_def_t;

/**
  * @brief  Structure definition of IMEI: International Mobile Equipment Identity.\n
  *         IMEI (15 decimal digits: 14 digits plus a check digit)
  *         or IMEISV (16 decimal digits: 14 digits plus two software version digits)
  * @note   If IMEI is unknown: len = 0U and value[0]='\0'\n
  *         If IMEI is known  : len > 0U and value[len]='\0'
  */
typedef struct
{
  uint8_t                         len;                      /*!< len of IMEI value          */
  uint8_t                         value[CA_IMEI_SIZE_MAX];  /*!< IMEI value. Format: octets */
} cellular_imei_t;

/**
  * @brief  Structure definition of MNO Name: Mobile Network Operator Name (also known as Wireless Service Provider).
  * @note   If MNO name is unknown: len = 0U and value[0]='\0'\n
  *         If MNO name is known  : len > 0U and value[len]='\0'
  */
typedef struct
{
  uint8_t                         len;                          /*!< len of MNO name value                */
  uint8_t                         value[CA_MNO_NAME_SIZE_MAX];  /*!< MNO name value. Format: ASCII string */
} cellular_mno_name_t;

/**
  * @brief  Structure definition of Manufacturer Identity.
  * @note   If Manufacturer Identity is unknown: len = 0U and value[0]='\0'\n
  *         If Manufacturer Identity is known  : len > 0U and value[len]='\0'
  */
typedef struct
{
  uint8_t                         len;                                 /*!< len of Manufacturer Identity value       */
  uint8_t                         value[CA_MANUFACTURER_ID_SIZE_MAX];  /*!< Manufacturer Identity value.
                                                                            Format: ASCII string                     */
} cellular_manufacturer_id_t;

/**
  * @brief  Structure definition of Model Identity.
  * @note   If Model Identity is unknown: len = 0U and value[0]='\0'\n
  *         If Model Identity is known  : len > 0U and value[len]='\0'
  */
typedef struct
{
  uint8_t                         len;                          /*!< len of Model Identity value                     */
  uint8_t                         value[CA_MODEL_ID_SIZE_MAX];  /*!< Model Identity value.
                                                                     Format: ASCII string                            */
} cellular_model_id_t;

/**
  * @brief  Structure definition of Revision Identity.
  * @note   If Revision Identity is unknown: len = 0U and value[0]='\0'\n
  *         If Revision Identity is known  : len > 0U and value[len]='\0'
  */
typedef struct
{
  uint8_t                         len;                             /*!< len of Revision Identity value               */
  uint8_t                         value[CA_REVISION_ID_SIZE_MAX];  /*!< Revision value Identity.
                                                                        Format: ASCII string                         */
} cellular_revision_id_t;

/**
  * @brief  Structure definition of Serial Number Identity.
  * @note   If Serial Number Identity is unknown: len = 0U and value[0]='\0'\n
  *         If Serial Number Identity is known  : len > 0U and value[len]='\0'
  */
typedef struct
{
  uint8_t                         len;                                  /*!< len of Serial Number Identity value     */
  uint8_t                         value[CA_SERIAL_NUMBER_ID_SIZE_MAX];  /*!< Serial Number Identity value.
                                                                             Format: ASCII string                    */
} cellular_serial_number_id_t;

/**
  * @brief  Structure definition of Identification.
  * @note   Regroup manufacturer identity, model identity, revision identity and serial number identity information
  */
typedef struct
{
  cellular_manufacturer_id_t      manufacturer_id;   /*!< Manufacturer Identity  */
  cellular_model_id_t             model_id;          /*!< Model Identity         */
  cellular_revision_id_t          revision_id;       /*!< Revision Identity      */
  cellular_serial_number_id_t     serial_number_id;  /*!< Serial Number Identity */
} cellular_identity_t;

/**
  * @brief  Structure definition of Operator Name.
  * @note   If Operator Name is unknown: len = 0U and value[0]='\0'\n
  *         If Operator Name is known  : len > 0U and value[len]='\0'
  */
typedef struct
{
  uint8_t                         len;                               /*!< len of Operator Name value              */
  uint8_t                         value[CA_OPERATOR_NAME_SIZE_MAX];  /*!< Operator Name value.
                                                                          Format: ASCII string                    */
} cellular_operator_name_t;

/**
  * @brief  Structure definition of APN: Access Point Name.
  * @note   If APN is unknown: len = 0U and value[0]='\0'\n
  *         If APN is known  : len > 0U and value[len]='\0'
  */
typedef struct
{
  uint8_t                         len;                     /*!< len of APN value                                  */
  uint8_t                         value[CA_APN_SIZE_MAX];  /*!< APN value.
                                                                Format: ASCII string                              */
} cellular_apn_t;

/**
  * @brief  Structure definition of Username.
  * @note   If Username is unknown: len = 0U and value[0]='\0'\n
  *         If Username is known  : len > 0U and value[len]='\0'
  */
typedef struct
{
  uint8_t                         len;                          /*!< len of Username value                            */
  uint8_t                         value[CA_USERNAME_SIZE_MAX];  /*!< Username value.
                                                                     Format: ASCII string                             */
} cellular_username_t;

/**
  * @brief  Structure definition of Password.
  * @note   If Password is unknown: len = 0U and value[0]='\0'\n
  *         If Password is known  : len > 0U and value[len]='\0'
  */
typedef struct
{
  uint8_t                         len;                          /*!< len of Password value                            */
  uint8_t                         value[CA_PASSWORD_SIZE_MAX];  /*!< Password value.
                                                                     Format: ASCII string                             */
} cellular_password_t;

/**
  * @brief  Structure definition of PDN: Public Data Network.
  */
typedef struct
{
  /**
    * @brief apn_send_to_modem: used for selecting the APN configuration
    *        - CA_APN_SEND_TO_MODEM : APN is managed by app and send to modem.
    *        - CA_APN_NOT_SEND_TO_MODEM : let modem manage APN using its own policy.
    */
  cellular_apn_send_to_modem_t    apn_send_to_modem;
  uint8_t                         cid;                /*!< CID Context IDentifier - Possible values [1-9].
                                                           Specifies a particular PDP context definition          */
  cellular_apn_t                  apn;                /*!< APN                                                    */
  cellular_username_t             username;           /*!< Username                                               */
  cellular_password_t             password;           /*!< Password (used only is Username is defined)            */
} cellular_pdn_t;

/**
  * @brief  Structure definition of Operator Selection.
  */
typedef struct
{
  ca_ntw_registration_mode_t      ntw_registration_mode;
  ca_operator_name_format_t       operator_name_format;
  cellular_operator_name_t        operator_name;
  ca_access_techno_present_t      access_techno_present;
  ca_access_techno_t              access_techno;
} cellular_operator_selection_t;

/**
  * @brief  Definition of IP address.
  */
typedef com_ip_addr_t             cellular_ip_addr_t;

/**
  * @brief  Structure definition of Cellular information.
  */
typedef struct
{
  ca_sockets_type_t               sockets_type;            /*!< Sockets Type                                      */
  ca_modem_state_t                modem_state;             /*!< Current modem state                               */
  cellular_identity_t             identity;                /*!< Identity                                          */
  cellular_imei_t                 imei;                    /*!< IMEI                                              */
  uint32_t                        nwk_attachment_timeout;  /*!< Network attachment timeout Unit: in milliseconds. */
  uint32_t                        nwk_inactivity_timeout;  /*!< Network inactivity timeout before to enable
                                                                LowPower mode. Unit: in milliseconds.             */
  cellular_mno_name_t             mno_name;                /*!< Mobile Network Operator returned by network
                                                                when attached                                     */
  cellular_ip_addr_t              ip_addr;                 /*!< IP address assigned by the network                */
} cellular_info_t;

/**
  * @brief  Structure definition of SIM information.
  */
typedef struct
{
  cellular_iccid_t                iccid;                              /*!< ICCID                                  */
  cellular_imsi_t                 imsi;                               /*!< IMSI                                   */
  uint8_t                         sim_index;                          /*!< SIM index in the configuration table;
  if (sim_status[sim_index] = CA_SIM_READY) then sim_index is the active sim and sim_slot_type[sim_index] its type.\n
  if ((sim_index = sim_slot_nb) and (sim_status[sim_index] != CA_SIM_READY)) then no sim can be activated.        */
  uint8_t                         sim_slot_nb;                             /*!< SIM slot nb defined in next fields   */
  ca_sim_slot_type_t              sim_slot_type[PLF_CELLULAR_SIM_SLOT_NB]; /*!< Type of available SIM slots          */
  ca_sim_status_t                 sim_status[PLF_CELLULAR_SIM_SLOT_NB];    /*!< Status of available SIM slots        */
  cellular_pdn_t                  pdn[PLF_CELLULAR_SIM_SLOT_NB];           /*!< PDN to use is specific to a SIM slot */
} cellular_sim_info_t;

/**
  * @brief  Structure definition of NFMC information.
  */
typedef struct
{
  bool                            enable;                               /*!< 0: disable - 1: enable               */
  uint8_t                         tempo_nb;                             /*!< NFMC tempo number                    */
  uint32_t                        tempo_values[CA_NFMC_VALUES_MAX_NB];  /*!< NFMC tempo values. This field is
                                                                             significant only if enable = true    */
} cellular_nfmc_info_t;

/**
  * @brief  Structure definition of IP information.
  */
typedef struct
{
  cellular_ip_addr_t              ip_addr;                 /*!< IP address assigned by the network                */
} cellular_ip_info_t;

#if (USE_LOW_POWER == 1)
/**
  * @brief  Structure definition of Power information.
  */
typedef struct
{
  ca_power_state_t                power_state;      /*!< Current power state                                      */
  uint32_t                        nwk_periodic_TAU; /*!< Negotiated value of T3412 - unit: seconds
                                                         0 means not available                                    */
  uint32_t                        nwk_active_time;  /*!< Negotiated value of T3324 - unit: seconds
                                                         0 means not available                                    */
} cellular_power_info_t;

/**
  * @brief Structure definition of
  */
typedef struct
{
  uint8_t                         req_periodic_RAU;     /*!< GERAN/UTRAN networks T3312
                                                             cf Table 10.5.163a from TS 24.008                    */
  uint8_t                         req_GPRS_READY_timer; /*!< GERAN/UTRAN networks T3314
                                                             cf Table 10.5.172 from TS 24.008                     */
  uint8_t                         req_periodic_TAU;     /*!< E-UTRAN networks T3412
                                                             cf Table 10.5.163a from TS 24.008                    */
  uint8_t                         req_active_time;      /*!< GERAN/UTRAN and E-UTRAN networks T3324
                                                             cf Table 10.5.163 from TS 24.008                     */
} cellular_power_psm_config_t;

/**
  * @brief Structure definition of
  */
typedef struct
{
  ca_eidrx_act_type_t             act_type;
  uint8_t                         req_value;
} cellular_power_eidrx_config_t;

/**
  * @brief  Structure definition of Power Configuration.
  */
typedef struct
{
  ca_power_cmd_t                  power_cmd;              /*!< power command to apply (init or setting)           */
  ca_power_mode_t                 power_mode;             /*!< target power mode                                  */
  bool                            psm_present;            /*!< indicates if PSM parameters below are present      */
  bool                            edrx_present;           /*!< indicates if eDRX parameters below are present     */
  ca_psm_mode_t                   psm_mode;               /*!< Requested PSM mode                                 */
  uint32_t                        sleep_request_timeout;  /*!< Sleep request timeout                              */
  cellular_power_psm_config_t     psm;                    /*!< PSM config                                         */
  ca_eidrx_mode_t                 eidrx_mode;             /*!< requested eDRX mode                                */
  cellular_power_eidrx_config_t   eidrx;                  /*!< eIDRX config                                       */
} cellular_power_config_t;
#endif  /* (USE_LOW_POWER == 1) */

/**
  * @brief     Callback definition used to inform about Cellular information changed.
  * @param[in] event           - Event that happened: CA_CELLULAR_INFO_EVENT.
  * @param[in] p_cellular_info - The new cellular information.
  * @param[in] p_callback_ctx  - The p_callback_ctx parameter in cellular_info_changed_registration function.
  */
typedef void (* cellular_info_cb_t)(ca_event_type_t event, const cellular_info_t *const p_cellular_info,
                                    void *const p_callback_ctx);

/**
  * @brief     Callback definition used to inform about Signal information changed.
  * @param[in] event          - Event that happened: CA_SIGNAL_INFO_EVENT.
  * @param[in] p_signal_info  - The new signal information.
  * @param[in] p_callback_ctx - The p_callback_ctx parameter in cellular_signal_info_changed_registration function.
  */
typedef void (* cellular_signal_info_cb_t)(ca_event_type_t event, const cellular_signal_info_t *const p_signal_info,
                                           void *const p_callback_ctx);

/**
  * @brief     Callback definition used to inform about SIM information changed.
  * @param[in] event          - Event that happened: CA_SIM_INFO_EVENT.
  * @param[in] p_sim_info     - The new sim information.
  * @param[in] p_callback_ctx - The p_callback_ctx parameter in cellular_sim_info_changed_registration function.
  */
typedef void (* cellular_sim_info_cb_t)(ca_event_type_t event, const cellular_sim_info_t *const p_sim_info,
                                        void *const p_callback_ctx);

/**
  * @brief     Callback definition used to inform about NFMC information changed.
  * @param[in] event          - Event that happened: CA_IP_INFO_EVENT.
  * @param[in] p_nfmc_info    - The new IP information.
  * @param[in] p_callback_ctx - The p_callback_ctx parameter in cellular_ip_info_changed_registration function.
  */
typedef void (* cellular_ip_info_cb_t)(ca_event_type_t event, const cellular_ip_info_t *const p_ip_info,
                                       void *const p_callback_ctx);

/**
  * @brief     Callback definition used to inform about IP information changed.
  * @param[in] event          - Event that happened: CA_NFMC_INFO_EVENT.
  * @param[in] p_nfmc_info    - The new NFMC information.
  * @param[in] p_callback_ctx - The p_callback_ctx parameter in cellular_nfmc_info_changed_registration function.
  */
typedef void (* cellular_nfmc_info_cb_t)(ca_event_type_t event, const cellular_nfmc_info_t *const p_nfmc_info,
                                         void *const p_callback_ctx);

#if (USE_LOW_POWER == 1)
/**
  * @brief     Callback definition used to inform about Power information changed.
  * @param[in] event          - Event that happened: CA_POWER_INFO_EVENT.
  * @param[in] p_power_info   - The new Power information.
  * @param[in] p_callback_ctx - The p_callback_ctx parameter in cellular_nfmc_info_changed_registration function.
  */
typedef void (* cellular_power_info_cb_t)(ca_event_type_t event, const cellular_power_info_t *const p_power_info,
                                          void *const p_callback_ctx);
#endif /* USE_LOW_POWER == 1 */

/**
  * @}
  */

/* External variables --------------------------------------------------------*/
/* None */

/* Exported macros -----------------------------------------------------------*/
/* None */

/* Exported functions ------------------------------------------------------- */

/** @defgroup CELLULAR_CTRL_Functions Functions
  * @{
  */

/**
  * @brief  Initialize cellular software.
  * @param  -
  * @retval -
  */
void cellular_init(void);

/**
  * @brief  Start cellular software with boot modem
  *         (and network registration if PLF_CELLULAR_TARGET_STATE = 2U see plf_cellular_config.h).
  * @param  -
  * @retval -
  */
void cellular_start(void);

/**
  * @brief  Start cellular with boot modem only (NO network registration).\n
  *         Usage: Used to configure modem
  * @param  -
  * @retval -
  */
void cellular_modem_start(void);

/**
  * @brief  Stop the modem (stop data mode, and detach from the network).
  * @param  -
  * @retval cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                   indicating the cause of the error.\n
  *         CELLULAR_SUCCESS          The operation is successful.\n
  *         CELLULAR_ERR_INTERNAL     Error while executing X-Cube-Cellular internal function.
  */
cellular_result_t cellular_modem_stop(void);

/**
  * @brief  Modem power on and instruct the modem to perform network registration.
  * @param  -
  * @retval cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                   indicating the cause of the error.\n
  *         CELLULAR_SUCCESS          The operation is successful.\n
  *         CELLULAR_ERR_INTERNAL     Error while executing X-Cube-Cellular internal function.
  */
cellular_result_t cellular_connect(void);

/**
  * @brief  Modem disconnect, instruct the modem to perform network unregistration.\n
  *         Modem stays on and sim is still accessible.
  * @param  -
  * @retval cellular_result_t        The code indicating if the operation is successful otherwise an error code
  *                                  indicating the cause of the error.\n
  *         CELLULAR_SUCCESS         The operation is successful.\n
  *         CELLULAR_ERR_INTERNAL    Error while executing X-Cube-Cellular internal function.
  */
cellular_result_t cellular_disconnect(void);

/**
  * @brief  Get X-Cube-Cellular major version encoded on 16 bits
  * @param  -
  * @retval X-Cube-Cellular major version on 16 bits
  */
uint16_t cellular_get_major_version(void);

/**
  * @brief  Get X-Cube-Cellular minor version encoded on 8 bits
  * @param  -
  * @retval X-Cube-Cellular minor version on 8 bits
  */
uint8_t cellular_get_minor_version(void);

/**
  * @brief  Get X-Cube-Cellular patch version encoded on 8 bits
  * @param  -
  * @retval X-Cube-Cellular patch version on 8 bits
  */
uint8_t cellular_get_patch_version(void);

/**
  * @brief  Check if X-Cube-Cellular version is a Release version
  * @param  -
  * @retval true if X-Cube-Cellular version is a release version, else return false
  */
bool cellular_version_is_release(void);

/**
  * @brief  Get X-Cube-Cellular version encoded on 32 bits, following the pattern MMMMmmPP\n
  *         From Most Significant Bit to Least Significant Bit :
  *         MMMM Major version on 16 bits then mm minor version on 8 bits then PP Patch version on 8 bits
  * @param  -
  * @retval 32 bits value integer representing X-Cube-Cellular version
  */
uint32_t cellular_get_version(void);

/**
  * @brief  Get version as string format representing the current X-Cube-Cellular version\n
  *         MM.mm.pp_Dev  for a dev version\n
  *         MM.mm.pp      for a release version
  * @param  p_ret_version pointer on an allocated area where will be stored the X-Cube-Cellular version as a string
  * @param  len           length of the structure pointed by p_ret_version - len must be >= CELLULAR_VERSION_STR_LEN
  * @retval bool - false/true - version is not returned (len too small) / version is returned
  */
bool cellular_get_string_version(uint8_t *p_ret_version, uint8_t len);

/**
  * @brief         Get Cellular information.
  * @param[in,out] p_cellular_info - The cellular info structure to contain the response.
  * @retval -
  */
void cellular_get_cellular_info(cellular_info_t *const p_cellular_info);

/**
  * @brief     Set the PDN value to use for a specific SIM slot.
  * @param[in] sim_slot_type  - The SIM slot that as to be configured.
  * @param[in] p_cellular_pdn - The new PDN value to use.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_BADARGUMENT  Argument value not compliant.\n
  *                                      e.g sim_slot_type is unknown for this platform.\n
  *            CELLULAR_ERR_INTERNAL     Error while executing X-Cube-Cellular internal function.
  */
cellular_result_t cellular_set_pdn(ca_sim_slot_type_t sim_slot_type, const cellular_pdn_t *const p_cellular_pdn);

/**
  * @brief     Set the SIM slot order.
  * @param[in] sim_slot_nb     - The SIM slots number defined in p_sim_slot_type.
  * @param[in] p_sim_slot_type - The new SIM Slot type order to use.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_BADARGUMENT  Argument value not compliant.\n
  *                                      e.g sim_slot_nb > PLF_CELLULAR_SIM_SLOT_NB\n
  *                                          p_sim_slot_type contain a unknown SIM Slot type.\n
  *            CELLULAR_ERR_INTERNAL     Error while executing X-Cube-Cellular internal function.
  */
cellular_result_t cellular_set_sim_slot_order(uint8_t sim_slot_nb, const ca_sim_slot_type_t *const p_sim_slot_type);

/**
  * @brief     Set Operator parameters.
  * @param[in] p_operator_selection    - The new operator configuration to use.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_BADARGUMENT  Argument value not compliant.\n
  *            CELLULAR_ERR_INTERNAL     Error while executing X-Cube-Cellular internal function.
  */
cellular_result_t cellular_set_operator(const cellular_operator_selection_t *const p_operator_selection);

/**
  * @brief     Set a new value for network attachment timeout.
  * @param[in] nwk_attachment_timeout  - The new network attachment timeout value to use. Unit: in milliseconds.
  *                                      0U : no timeout(means infinite wait).
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_BADARGUMENT  Argument value not compliant.\n
  *            CELLULAR_ERR_INTERNAL     Error while executing X-Cube-Cellular internal function.
  */
cellular_result_t cellular_set_nwk_attachment_timeout(uint32_t nwk_attachment_timeout);

/**
  * @brief         Get signal strength and access techno information.
  * @param[in,out] p_signal_info - The signal info structure to contain the response.
  * @retval -
  */
void cellular_get_signal_info(cellular_signal_info_t *const p_signal_info);

/**
  * @brief         Get SIM information.
  * @param[in,out] p_sim_info - The sim info structure to contain the response.
  * @retval -
  */
void cellular_get_sim_info(cellular_sim_info_t *const p_sim_info);

/**
  * @brief         Get NFMC information.
  * @param[in,out] p_nfmc_info - The nfmc info structure to contain the response.
  * @retval -
  */
void cellular_get_nfmc_info(cellular_nfmc_info_t *const p_nfmc_info);

/**
  * @brief     Set NFMC feature.
  * @param[in] nfmc_enable - false: NFMC is disable - true: NFMC is enable.
  * @param[in] nfmc_value_nb - The NFMC values number defined in p_nfmc_value.
  * @param[in] p_nfmc_value  - The new NFMC values to use for the calculation of NFMC tempo values.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_BADARGUMENT  Argument value not compliant.\n
  *                                      e.g nfmc_value_nb > CA_NFMC_VALUES_MAX_NB\n
  *            CELLULAR_ERR_INTERNAL     Error while executing X-Cube-Cellular internal function.
  * @note      if (result == CELLULAR_SUCCESS) then call cellular_get_nfmc_info() to have the new NFMC tempo values.
  */
cellular_result_t cellular_set_nfmc(bool nfmc_enable, uint8_t nfmc_value_nb, const uint32_t *const p_nfmc_value);

/**
  * @brief         Get IP information.
  * @param[in,out] p_ip_info - The ip info structure to contain the response.
  * @retval -
  */
void cellular_get_ip_info(cellular_ip_info_t *const p_ip_info);

#if (USE_LOW_POWER == 1)
/**
  * @brief         Get Power information.
  * @param[in,out] p_power_info - The power info structure to contain the response.
  * @retval -
  */
void cellular_get_power_info(cellular_power_info_t *const p_power_info);

/**
  * @brief     Set Power feature.
  * @param[in] p_power_config - The new power configuration to use.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_BADARGUMENT  Argument value not compliant.\n
  *            CELLULAR_ERR_INTERNAL     Error while executing X-Cube-Cellular internal function.
  */
cellular_result_t cellular_set_power(const cellular_power_config_t *const p_power_config);
#endif /* USE_LOW_POWER == 1 */

/**
  * @brief     Register a callback that will be called when Cellular information is updated.
  * @param[in] cellular_info_cb        - The callback to register.
  *                                      If set to NULL no data will be provided to the call back.
  * @param[in] p_callback_ctx          - The context to be passed when cellular_info_cb callback is called.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_NOMEMORY     No more memory to register the callback.\n
  *            CELLULAR_ERR_INTERNAL     Error while executing X-Cube-Cellular internal function.
  */
cellular_result_t cellular_info_cb_registration(cellular_info_cb_t cellular_info_cb, void *const p_callback_ctx);
/**
  * @brief     Deregister a Cellular information callback.
  * @param[in] cellular_info_cb        - The callback to deregister.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_BADARGUMENT  The callback to deregister is unknown.
 */
cellular_result_t cellular_info_cb_deregistration(cellular_info_cb_t cellular_info_cb);

/**
  * @brief     Register a callback that will be called when Network Signal information is updated.
  * @param[in] cellular_signal_info_cb - The callback to register.
  * @param[in] p_callback_ctx          - The context to be passed when cellular_signal_info_cb callback is called.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_NOMEMORY     No more memory to register the callback.
  */
cellular_result_t cellular_signal_info_cb_registration(cellular_signal_info_cb_t cellular_signal_info_cb,
                                                       void *const p_callback_ctx);
/**
  * @brief     Deregister a Network Signal information callback.
  * @param[in] cellular_signal_info_cb - The callback to deregister.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_BADARGUMENT  The callback to deregister is unknown.
 */
cellular_result_t cellular_signal_info_cb_deregistration(cellular_signal_info_cb_t cellular_signal_info_cb);

/**
  * @brief     Register a callback that will be called when SIM information is updated.
  * @param[in] cellular_sim_info_cb    - The callback to register.
  * @param[in] p_callback_ctx          - The context to be passed when cellular_sim_info_cb callback is called.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_NOMEMORY     No more memory to register the callback.
  */
cellular_result_t cellular_sim_info_cb_registration(cellular_sim_info_cb_t cellular_sim_info_cb,
                                                    void *const p_callback_ctx);
/**
  * @brief     Deregister a SIM information callback.
  * @param[in] cellular_sim_info_cb    - The callback to deregister.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_BADARGUMENT  The callback to deregister is unknown.
 */
cellular_result_t cellular_sim_info_cb_deregistration(cellular_sim_info_cb_t cellular_sim_info_cb);

/**
  * @brief     Register a callback that will be called when NFMC information is updated.
  * @param[in] cellular_nfmc_info_cb   - The callback to register.
  * @param[in] p_callback_ctx          - The context to be passed when cellular_nfmc_info_cb callback is called.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_NOMEMORY     No more memory to register the callback.
  */
cellular_result_t cellular_nfmc_info_cb_registration(cellular_nfmc_info_cb_t cellular_nfmc_info_cb,
                                                     void *const p_callback_ctx);

/**
  * @brief     Deregister a NFMC information callback.
  * @param[in] cellular_nfmc_info_cb   - The callback to deregister.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_BADARGUMENT  The callback to deregister is unknown.
  */
cellular_result_t cellular_nfmc_info_cb_deregistration(cellular_nfmc_info_cb_t cellular_nfmc_info_cb);

/**
  * @brief     Register a callback that will be called when IP information is updated.
  * @param[in] cellular_ip_info_cb     - The callback to register.
  * @param[in] p_callback_ctx          - The context to be passed when cellular_ip_info_cb callback is called.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_NOMEMORY     No more memory to register the callback.
  */
cellular_result_t cellular_ip_info_cb_registration(cellular_ip_info_cb_t cellular_ip_info_cb,
                                                   void *const p_callback_ctx);

/**
  * @brief     Deregister a IP information callback.
  * @param[in] cellular_ip_info_cb     - The callback to deregister.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_BADARGUMENT  The callback to deregister is unknown.
 */
cellular_result_t cellular_ip_info_cb_deregistration(cellular_ip_info_cb_t cellular_ip_info_cb);

#if (USE_LOW_POWER == 1)
/**
  * @brief     Register a callback that will be called when Power information is updated.
  * @param[in] cellular_power_info_cb  - The callback to register.
  * @param[in] p_callback_ctx          - The context to be passed when cellular_power_info_cb callback is called.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_NOMEMORY     No more memory to register the callback.
  */
cellular_result_t cellular_power_info_cb_registration(cellular_power_info_cb_t cellular_power_info_cb,
                                                      void *constp_callback_ctx);
/**
  * @brief     Deregister a Power information callback.
  * @param[in] cellular_power_info_cb  - The callback to deregister.
  * @retval    cellular_result_t         The code indicating if the operation is successful otherwise an error code
  *                                      indicating the cause of the error.\n
  *            CELLULAR_SUCCESS          The operation is successful.\n
  *            CELLULAR_ERR_BADARGUMENT  The callback to deregister is unknown.
 */
cellular_result_t cellular_power_info_cb_deregistration(cellular_power_info_cb_t cellular_power_info_cb);

#endif /* USE_LOW_POWER == 1 */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_CONTROL_API_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
