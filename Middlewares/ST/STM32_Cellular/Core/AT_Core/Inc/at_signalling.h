/**
  ******************************************************************************
  * @file    at_signalling.h
  * @author  MCD Application Team
  * @brief   Header which contains commons information about AT commands.
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
#ifndef AT_SIGNALLING_H
#define AT_SIGNALLING_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "at_core.h"
#include "at_parser.h"
#include "at_modem_api.h"
#include "at_modem_common.h"
#include "cellular_service.h"
#include "cellular_service_int.h"
#include "ipc_common.h"

/** @addtogroup AT_CORE AT_CORE
  * @{
  */

/** @addtogroup AT_CORE_SIGNALLING AT_CORE SIGNALLING
  * @{
  */

/** @defgroup AT_CORE_SIGNALLING_Exported_Types AT_CORE SIGNALLING Exported Types
  * @{
  */
enum
{
  /* standard commands */
  CMD_AT = 0, /* empty command or empty answer */
  CMD_AT_OK,
  CMD_AT_CONNECT,
  CMD_AT_RING,
  CMD_AT_NO_CARRIER,
  CMD_AT_ERROR,
  CMD_AT_NO_DIALTONE,
  CMD_AT_BUSY,
  CMD_AT_NO_ANSWER,
  CMD_AT_CME_ERROR,
  CMD_AT_CMS_ERROR,

  /* 3GPP TS 27.007 and GSM 07.07 commands */
  CMD_AT_CGMI, /* Request manufacturer identification */
  CMD_AT_CGMM, /* Model identification */
  CMD_AT_CGMR, /* Revision identification */
  CMD_AT_CGSN, /* Product serial number identification */
  CMD_AT_CIMI, /* IMSI */
  CMD_AT_CEER, /* Extended error report */
  CMD_AT_CMEE, /* Report mobile equipment error */
  CMD_AT_CPIN, /* Enter PIN */
  CMD_AT_CFUN, /* Set phone functionality */
  CMD_AT_COPS, /* Operator selection */
  CMD_AT_CNUM, /* Subscriber number */
  CMD_AT_CGATT,   /* PS attach or detach */
  CMD_AT_CREG,    /* Network registration: enable or disable +CREG urc */
  CMD_AT_CGREG,   /* GPRS network registation status: enable or disable +CGREG urc */
  CMD_AT_CEREG,   /* EPS network registration status: enable or disable +CEREG urc */
  CMD_AT_CGEREP,  /* Packet domain event reporting: enable or disable +CGEV urc */
  CMD_AT_CGEV,    /* EPS bearer indication status */
  CMD_AT_CSQ,     /* Signal quality */
  CMD_AT_CGDCONT, /* Define PDP context */
  CMD_AT_CGACT,   /* PDP context activate or deactivate */
  CMD_AT_CGDATA,  /* Enter Data state */
  CMD_AT_CGAUTH,  /* Define PDP context authentication parameters */
  CMD_AT_CGPADDR, /* Show PDP Address */
  CMD_AT_CPSMS,    /* Power Saving Mode setting */
  CMD_AT_CEDRXS,   /* eDRX settings */
  CMD_AT_CEDRXP,   /* eDRX URC */
  CMD_AT_CEDRXRDP, /* eDRX Read Dynamic Parameters */
  CMD_AT_CSIM,     /* Sim Generic Access */

  /* V.25TER commands */
  CMD_ATD,       /* Dial */
  CMD_ATE,       /* Command Echo */
  CMD_ATH,       /* Hook control (disconnect existing connection) */
  CMD_ATO,       /* Return to online data state (switch from COMMAND to DATA mode) */
  CMD_ATV,       /* DCE response format */
  CMD_AT_AND_W,  /* Store current Parameters to User defined profile */
  CMD_AT_AND_D,  /* Set DTR function mode */
  CMD_ATX,       /* CONNECT Result code and monitor call progress */
  CMD_ATZ,       /* Set all current parameters to user defined profile */
  CMD_AT_GSN,    /* Request product serial number identification */
  CMD_AT_IPR,    /* Fixed DTE rate */
  CMD_AT_IFC,    /* set DTE-DCE local flow control */

  /* other */
  CMD_AT_ESC_CMD,     /* escape command for switching from DATA to COMMAND mode */
  CMD_AT_DIRECT_CMD,  /* allow user to send command directly to the modem */
  CMD_AT_LAST_GENERIC, /* keep it at last position */

};
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* AT_SIGNALLING_H */
