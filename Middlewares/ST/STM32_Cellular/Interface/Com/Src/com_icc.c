/**
  ******************************************************************************
  * @file    com_icc.c
  * @author  MCD Application Team
  * @brief   This file implements communication with International Circuit Card
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
#include "com_icc.h"

#if (USE_COM_ICC == 1)

#include <string.h>
#include <stdlib.h>

#include "rtosal.h"

#include "com_err.h"
#include "com_sockets_net_compat.h"
#include "com_trace.h"

#if (USE_ST33 == 1)
#include "com_utils.h"
#include "ndlc_interface.h"
#endif /* USE_ST33 == 1 */

#include "cellular_service_os.h"

#include "dc_common.h"
#include "cellular_control_api.h"
#include "cellular_service_datacache.h"

/* Private defines -----------------------------------------------------------*/
#if (USE_ST33 == 1)
#define COM_ICC_SESSION_MAX_NB   2U /* 1: Communication with ICC using AT+CSIM
                                       2: Communication with ICC using NDLC    */
#else
#define COM_ICC_SESSION_MAX_NB   1U /* 1: Communication with ICC using AT+CSIM */
#endif /* USE_ST33 == 1 */

/* Private typedef -----------------------------------------------------------*/
/* Handle State */
typedef enum
{
  COM_ICC_INVALID = 0,
  COM_ICC_CREATED,
  COM_ICC_WAITING_RSP
} com_icc_state_t;

typedef struct
{
  com_icc_state_t state;
  uint8_t         protocol;
  int32_t         handle;
} com_icc_desc_t;

/* Private macros ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/* Mutex to protect concurrent access to :
   ICC descriptor
 */

static osMutexId ComIccMutexHandle;

static com_icc_desc_t com_icc_desc[COM_ICC_SESSION_MAX_NB];

static bool com_icc_is_available[COM_ICC_SESSION_MAX_NB];     /* Icc availability status is managed through Datacache */
static bool com_icc_is_power_on[COM_ICC_SESSION_MAX_NB];      /* Icc power on status is managed by Icc */
static bool com_icc_is_initialized[COM_ICC_SESSION_MAX_NB];   /* Icc initialized is managed by Icc */

#if (USE_ST33 == 1)
static ndlc_device_t com_icc_st33_device;
#endif /* USE_ST33 == 1 */

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Callback prototype */
/* Callback called by Datacache - used to know Icc status */
static void com_icc_datacache_cb(dc_com_event_id_t dc_event_id, const void *p_private_gui_data);

static void com_icc_init_icc_desc(uint8_t icc_index, uint8_t protocol);
static com_icc_desc_t *com_icc_provide_icc_desc(uint8_t icc_index);
static int32_t com_icc_use_icc_desc(int32_t handle, com_icc_state_t new_state, uint8_t *p_protocol);
static int32_t com_icc_delete_icc_desc(int32_t handle);
static bool com_icc_initialize_icc(uint8_t icc_index);

/* ICC status is managed through Datacache */
static bool com_icc_is_icc_available(uint8_t icc_index);

static int32_t com_icc_generic_access_csim(const com_char_t *p_buf_cmd, int32_t len_cmd,
                                           com_char_t *p_buf_rsp, int32_t len_rsp);
#if (USE_ST33 == 1)
static int32_t com_icc_generic_access_ndlc(const com_char_t *p_buf_cmd, int32_t len_cmd,
                                           com_char_t *p_buf_rsp, int32_t len_rsp);
#endif /* USE_ST33 == 1 */

/* Private function Definition -----------------------------------------------*/
/**
  * @brief  Callback called when a value in datacache changed
  * @note   Managed datacache value changed
  * @param  dc_event_id - value changed
  * @note   -
  * @param  p_private_gui_data - value provided at service subscription
  * @note   Unused
  * @retval -
  */
static void com_icc_datacache_cb(dc_com_event_id_t dc_event_id, const void *p_private_gui_data)
{
  UNUSED(p_private_gui_data);

  /* Used to know Icc status */
  if (dc_event_id == DC_CELLULAR_SIM_INFO)
  {
    dc_sim_info_t dc_sim_rt_info;

    com_icc_is_available[0] = false;
    com_icc_is_power_on[0] = false;
    com_icc_is_initialized[0] = false;
    if (dc_com_read(&dc_com_db, DC_CELLULAR_SIM_INFO, (void *)&dc_sim_rt_info, sizeof(dc_sim_rt_info))
        == DC_COM_OK)
    {
      PRINT_DBG("sim state: rt_state:%d index_slot:%d sim_status:%d",
                dc_sim_rt_info.rt_state,
                dc_sim_rt_info.index_slot,
                dc_sim_rt_info.sim_status[dc_sim_rt_info.index_slot])

      if (dc_sim_rt_info.rt_state == DC_SERVICE_ON)
      {
        if (dc_sim_rt_info.sim_status[dc_sim_rt_info.index_slot] == CA_SIM_READY)
        {
          com_icc_is_available[0]   = true;
          com_icc_is_initialized[0] = true;
          com_icc_is_power_on[0]    = true;
        }
      }
    }
  }
  else if (dc_event_id == DC_CELLULAR_INFO)
  {
    dc_cellular_info_t dc_cellular_info;

    (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&dc_cellular_info, sizeof(dc_cellular_info));

    if (dc_cellular_info.modem_state == CA_MODEM_POWER_OFF)
    {
      for (uint8_t i = 0U; i < COM_ICC_SESSION_MAX_NB; i++)
      {
        com_icc_is_available[i]   = false;
        com_icc_is_initialized[i] = false;
        com_icc_is_power_on[i]    = false;
      }
    }
    else if (dc_cellular_info.modem_state == CA_MODEM_STATE_POWERED_ON)
    {
#if (USE_ST33 == 1)
      com_icc_is_available[1]   = true;
#else
      __NOP();
#endif /* USE_ST33 == 1 */
    }
    else
    {
      /* Nothing to do */
      __NOP();
    }
  }
  else
  {
    /* Nothing to do */
    __NOP();
  }

}

/**
  * @brief  Provide Icc status
  * @note   -
  * @param  icc_index - 0: SIM - 1: ESE
  * @retval true/false Icc is available/not available
  */
static bool com_icc_is_icc_available(uint8_t icc_index)
{
  bool result;
  result = com_icc_is_available[icc_index];
  return (result);
}

/**
  * @brief  Initialize Icc
  * @note   -
  * @param  icc_index - 0: SIM - 1: ESE
  * @retval true/false Icc is initialized/not initialized
  */
static bool com_icc_initialize_icc(uint8_t icc_index)
{
  bool result;

  if (com_icc_is_icc_available(icc_index) == true)
  {
    if (com_icc_is_power_on[icc_index] == false)
    {
#if (USE_ST33 == 1)
      if (icc_index == 1U)
      {
        if (NDLC_POWER(&com_icc_st33_device, true) == true)
        {
          com_icc_is_power_on[icc_index] = true;
          if (NDLC_INIT(NULL, &com_icc_st33_device) == true)
          {
            com_icc_is_initialized[icc_index] = true;
          }
        }
      }
#else
      /* Nothing can be done */
      __NOP();
#endif /* USE_ST33 == 1 */
    }
  }

  result = com_icc_is_initialized[icc_index];

  return (result);
}

/**
  * @brief  ICC descriptor initialization
  * @note   Initialize ICC descriptor
  * @param  icc_index - 0: SIM - 1: ESE
  * @param  protocol - protocol type
  * @note   Supported values: COM_PROTO_CSIM : communication with ICC using AT+CSIM command
  *                           COM_PROTO_NDLC : communication with ICC using NDLC command
  * @retval -
  */
static void com_icc_init_icc_desc(uint8_t icc_index, uint8_t protocol)
{
  /* Protect Icc descriptor access */
  (void)rtosalMutexAcquire(ComIccMutexHandle, RTOSAL_WAIT_FOREVER);

  com_icc_desc[icc_index].state     = COM_ICC_INVALID;
  com_icc_desc[icc_index].protocol  = protocol;
  com_icc_desc[icc_index].handle    = COM_HANDLE_INVALID_ID;

  /* Access to Icc descriptor finished */
  (void)rtosalMutexRelease(ComIccMutexHandle);
}

/**
  * @brief  ICC descriptor reservation
  * @note   Provide a ICC descriptor
  * @param  icc_index - 0: SIM - 1: ESE
  * @retval com_icc_desc_t - ICC desc or NULL
  */
static com_icc_desc_t *com_icc_provide_icc_desc(uint8_t icc_index)
{
  com_icc_desc_t *icc_desc;

  /* Protect Icc descriptor access */
  (void)rtosalMutexAcquire(ComIccMutexHandle, RTOSAL_WAIT_FOREVER);

  icc_desc = &com_icc_desc[icc_index];

  /* Is Icc desc available ? */
  if (icc_desc->handle == COM_HANDLE_INVALID_ID)
  {
    /* Allocate Icc descriptor handle */
    uint32_t random;
    bool random_ok;
    do
    {
      bool exit = false;
      uint8_t i = 0U;
      random_ok = true;

      /* Set Icc descriptor handle to a random value */
#if defined(TFM_PSA_API)
      /* Decision to not call psa/crypto.h service */
      random = (uint32_t)rand();
#elif defined(RNG_HANDLE)
      if (HAL_RNG_GenerateRandomNumber(&RNG_HANDLE, &random) != HAL_OK)
      {
        random = (uint32_t)rand();
      }
#else /* !defined(TFM_PSA_API) && !defined(RNG_HANDLE) */
      random = (uint32_t)rand();
#endif /* TFM_PSA_API */
      /* Handle must be >= 0 when allocation is ok */
      random = random & 0x7FFFFFFFU;

      while ((i < COM_ICC_SESSION_MAX_NB) && (exit == false))
      {
        if (com_icc_desc[i].handle == (int32_t)random)
        {
          /* This random value is already in use - Must find another one */
          random_ok = false;
          /* Stop to check rest of com_icc_desc[i] */
          exit = true;
        }
        else
        {
          i++;
        }
      }
    } while (random_ok == false);

    icc_desc->handle = (int32_t)(random); /* no MISRA issue */
    icc_desc->state  = COM_ICC_CREATED;
  }
  else
  {
    /* Icc descriptor already in use */
    icc_desc = NULL;
  }

  /* Access to Icc descriptor finished */
  (void)rtosalMutexRelease(ComIccMutexHandle);

  return (icc_desc);
}

/**
  * @brief  ICC descriptor usage
  * @note   Set to use/unused icc descriptor
  * @param  handle    - ICC handle
  * @param  new_state - new state of com icc
  * @param  p_protocol  - protocol of Icc handle
  * @retval int32_t   - OK or error value
  */
static int32_t com_icc_use_icc_desc(int32_t handle, com_icc_state_t new_state, uint8_t *p_protocol)
{
  int32_t result;
  uint8_t i = 0U;
  bool found = false;

  /* Protect Icc descriptor access */
  (void)rtosalMutexAcquire(ComIccMutexHandle, RTOSAL_WAIT_FOREVER);

  /* Find handle */
  while ((i < COM_ICC_SESSION_MAX_NB) && (found == false))
  {
    if ((com_icc_desc[i].handle == handle) && (handle > COM_HANDLE_INVALID_ID))
    {
      found = true;
    }
    else
    {
      i++;
    }
  }

  if ((found == true) && (i < COM_ICC_SESSION_MAX_NB))
  {
    *p_protocol = com_icc_desc[i].protocol;
    if ((new_state == COM_ICC_WAITING_RSP) && (com_icc_desc[i].state == COM_ICC_CREATED))
    {
      com_icc_desc[i].state = COM_ICC_WAITING_RSP;
      result = COM_ERR_OK;
    }
    else if ((new_state == COM_ICC_CREATED) && (com_icc_desc[i].state == COM_ICC_WAITING_RSP))
    {
      com_icc_desc[i].state = COM_ICC_CREATED;
      result = COM_ERR_OK;
    }
    else
    {
      result = COM_ERR_STATE;
    }
  }
  else
  {
    /* Incorrect Icc descriptor handle */
    result = COM_ERR_DESCRIPTOR;
  }

  /* Access to Icc descriptor finished */
  (void)rtosalMutexRelease(ComIccMutexHandle);

  return (result);
}

/**
  * @brief  ICC descriptor delete
  * @note   Release a ICC descriptor
  * @param  handle - ICC handle
  * @retval int32_t - OK or error value
  */
static int32_t com_icc_delete_icc_desc(int32_t handle)
{
  int32_t result;
  uint8_t i = 0U;
  bool found = false;

  /* Protect Icc descriptor access */
  (void)rtosalMutexAcquire(ComIccMutexHandle, RTOSAL_WAIT_FOREVER);

  /* Find handle */
  while ((i < COM_ICC_SESSION_MAX_NB) && (found == false))
  {
    if ((com_icc_desc[i].handle == handle) && (handle > COM_HANDLE_INVALID_ID))
    {
      found = true;
    }
    else
    {
      i++;
    }
  }

  if ((found == true) && (i < COM_ICC_SESSION_MAX_NB))
  {
    if (com_icc_desc[i].state == COM_ICC_CREATED)
    {
      com_icc_desc[i].handle = COM_HANDLE_INVALID_ID;
      com_icc_desc[i].state  = COM_ICC_INVALID;
      result = COM_ERR_OK;
    }
    else
    {
      result = COM_ERR_STATE;
    }
  }
  else
  {
    /* Incorrect Icc descriptor handle */
    result = COM_ERR_DESCRIPTOR;
  }

  /* Access to Icc descriptor finished */
  (void)rtosalMutexRelease(ComIccMutexHandle);

  return (result);
}


static int32_t com_icc_generic_access_csim(const com_char_t *p_buf_cmd, int32_t len_cmd,
                                           com_char_t *p_buf_rsp, int32_t len_rsp)
{
  int32_t result;

  /* Specific test for CSIM command : None */

  /* Check availability of Icc */
  /* To do: Must add robustness at low-level for ICC availability */
  if (com_icc_initialize_icc(0U) == true)
  {
    /* To do: Must manage wake-up of Modem in case of Low-Power */
    CS_sim_generic_access_t sim_generic_access;

    sim_generic_access.p_cmd_str = p_buf_cmd;
    sim_generic_access.p_rsp_str = p_buf_rsp;
    sim_generic_access.cmd_str_size = (uint32_t)len_cmd; /* len_cmd > 0 */
    sim_generic_access.rsp_str_size = (uint32_t)len_rsp; /* len_rsp > 0 */

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
    CS_Status_t ret_suspend;
    /* Suspend data mode to interact with Modem using AT command */
    osCCS_get_wait_cs_resource();
    PRINT_INFO("Suspend data requested")
    ret_suspend = osCDS_suspend_data();
    /* Even if ret_suspend is NOK, maybe it is because Modem already in Command mode */
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */

    result = osCS_sim_generic_access(&sim_generic_access);

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
    /* Return to Data mode only if Suspend done by Com was Ok */
    if (ret_suspend == CS_OK)
    {
      PRINT_INFO("Resume data requested")
      (void)osCDS_resume_data();
    }
    osCCS_get_release_cs_resource();
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
  }
  else
  {
    result = COM_ERR_NOICC;
  }

  return (result);
}

#if (USE_ST33 == 1)
static int32_t com_icc_generic_access_ndlc(const com_char_t *p_buf_cmd, int32_t len_cmd,
                                           com_char_t *p_buf_rsp, int32_t len_rsp)
{
  int32_t result = COM_ERR_PARAMETER;;
  int32_t rsp_length;

  /* convert char ASCII apdu to NDLC format means 0x30 0x30 will be coded 00 */
  static uint8_t com_icc_ndlc_buf_tmp_apdu[256];
  /* in low layer response is memcpy without taking care size of p_buf_rsp
   * so to do quickly and to avoid memory erase a big buffer is provided
   * then a memcpy will be done in Samples buffer taking care of buffer response available size
   */
  static uint8_t com_icc_ndlc_buf_tmp_rsp[256];

  /* Specific test for eSE command */
  if (((uint32_t)len_cmd % 2U) == 0U)
  {
    uint8_t conVal;
    bool exit = false;
    uint32_t i = 0U;
    while ((i < ((uint32_t)len_cmd / 2U)) && (exit != true))
    {
      /* convert received buffer from HEX to ASCII format
       * example: if we receive 48545450, take digits 2 by 2 and convert them to their hexa value
       */
      if (com_utils_convertHEXToChar(p_buf_cmd[0U + (i * 2U)], p_buf_cmd[1U + (i * 2U)], &conVal) == true)
      {
        com_icc_ndlc_buf_tmp_apdu[i] = conVal;
        /* next character */
        i++;
      }
      else
      {
        /* Conversion NOK - stop the loop with result = COM_ERR_PARAMETER */
        exit = true;
      }
    }

    /* Is conversion OK ? */
    if (exit == false)
    {
      result = COM_ERR_OK;
    }
  }

  /* If rest of parameters are OK - Continue treatment */
  if (result == COM_ERR_OK)
  {
    /* Check availability of Icc */
    /* To do: Must add robustness at low-level for ICC availability */
    if (com_icc_initialize_icc(1U) == true)
    {
      /* To do: Must manage wake-up of Modem in case of Low-Power */
      /* Reset the hexa buffer response */
      (void)memset((void *)com_icc_ndlc_buf_tmp_rsp, (int32_t)'\0', sizeof(com_icc_ndlc_buf_tmp_rsp));
      /* Send the command to ESE and wait the response */
      /* Restriction: Interface NDLC is not OK to manage 256 bytes */
      rsp_length = NDLC_SEND_RECEIVE_APDU(&com_icc_st33_device, (uint8_t *)com_icc_ndlc_buf_tmp_apdu,
                                          ((uint16_t)len_cmd / 2U), com_icc_ndlc_buf_tmp_rsp);

      /* Analyze response */
      if (rsp_length > 0)
      {
        /* Copy buffer char response to buffer hexa response */
        uint32_t size_to_copy;
        /* -1U to reserve one byte to add end of string '\0' character */
        if ((uint32_t)rsp_length > (((uint32_t)len_rsp - 1U) / 2U))
        {
          size_to_copy = (((uint32_t)len_rsp - 1U) / 2U);
        }
        else
        {
          size_to_copy = (uint32_t)rsp_length;
        }
        for (uint32_t i = 0U; i < size_to_copy; i++)
        {
          (void)com_utils_convertCharToHEX(com_icc_ndlc_buf_tmp_rsp[i],
                                           &p_buf_rsp[(2U * i)], &p_buf_rsp[(1U + (2U * i))]);
        }
        /* Add to add end of string '\0' character */
        p_buf_rsp[(size_to_copy * 2U)] = (uint8_t)'\0';
        result = rsp_length * 2; /* to provide information how many bytes was the full result */
      }
      else
      {
        result = COM_ERR_GENERAL;
      }
    }
    else
    {
      result = COM_ERR_NOICC;
    }
  }

  return (result);
}
#endif /* USE_ST33 == 1 */

/* Functions Definition ------------------------------------------------------*/
/**
  * @brief  ICC handle creation
  * @note   Create a ICC session
  * @param  family   - address family
  * @note   Supported value: COM_AF_UNSPEC
  * @param  type     - connection type
  * @note   Supported value: COM_SOCK_SEQPACKET
  * @param  protocol - protocol type
  * @note   Supported values: COM_PROTO_CSIM : communication with ICC using AT+CSIM command
  *                           COM_PROTO_NDLC : communication with ICC using NDLC command
  * @retval int32_t  - icc handle or error value
  * @note   Possible returned value:\n
  *         int32_t > 0            : icc session created; the returned value is the icc handle session\n
  *         int32_t < 0            : an error occurred\n
  *         - COM_ERR_LOCKED       : a icc session is already in progress
  */
int32_t com_icc(int32_t family, int32_t type, int32_t protocol)
{
  int32_t result = COM_ERR_PARAMETER;
  com_icc_desc_t *icc_desc;
  uint8_t index = 0U;

  if ((family == COM_AF_UNSPEC) && (type == COM_SOCK_SEQPACKET))
  {
    if (protocol == COM_PROTO_CSIM)
    {
      result = COM_ERR_OK;
    }
#if (USE_ST33 == 1)
    else if (protocol == COM_PROTO_NDLC)
    {
      index = 1U;
      result = COM_ERR_OK;
    }
#endif /* USE_ST33 == 1 */
    else
    {
      __NOP();
    }
  }

  /* Continue the treatment only if arguments are OK */
  if (result == COM_ERR_OK)
  {
    /* Provide a Icc descriptor handle  */
    icc_desc = com_icc_provide_icc_desc(index);

    if (icc_desc != NULL)
    {
      result = icc_desc->handle;
    }
    else
    {
      result = COM_ERR_LOCKED; /* ICC already in use */
    }
  }

  return (result);
}

/**
  * @brief  ICC process a generic access
  * @note   Process a ICC request for generic access
  * @param  icc            - icc handle obtained with com_icc() call
  * @param  p_buf_cmd      - pointer to application data buffer containing the command to send\n
  *                        data is a command APDU in format as described in ETSI TS 102 221
  * @param  len_cmd        - length of the buffer command (in bytes)
  * @note   Max length of p_buf_cmd (and so len_cmd) is COM_ICC_xxxx_MAX_CMD_GENERIC_ACCESS_SZ bytes
  *         with xxxx = protocol\n
  *         p_buf_cmd[len_cmd] must be '\0' in order to check cohesion with len_cmd.\n
  *         if len_cmd > COM_ICC_xxxx_MAX_CMD_GENERIC_ACCESS_SZ or p_buf_cmd[len_cmd] != '\0'
  *         then COM_ERR_PARAMETER is returned\n
  *         e.g: to select Master File:\n
  *              p_buf_cmd[15]="00A4000C023F00", len_cmd=14, p_buf_cmd[0]='0x30'...p_buf_cmd[14]='\0'
  * @param  p_buf_rsp      - pointer to application data buffer to contain the response\n
  *                        data is a response APDU in format as described in ETSI TS 102 221
  * @param  len_rsp        - size max of the buffer response (in bytes)
  * @note   Min length of p_buf_rsp (and so len_rsp)is COM_ICC_xxxx_MIN_RSP_GENERIC_ACCESS_SZ bytes
  *         with xxxx = protocol\n
  *         if len_rsp < COM_ICC_xxxx_MIN_RSP_GENERIC_ACCESS_SZ, COM_ERR_PARAMETER is returned\n
  *         p_buf_rsp[MIN(len_rsp, retval)] is always set to '\0'\n
  *         e.g: if response to select Master File is SW1=90 and SW2=00:\n
  *              p_buf_rsp[]="9000", int32_t resuit=4, p_buf_rsp[0]='0x39'...p_buf_rsp[4]='\0'
  * @retval int32_t        - length of the ICC response or error value
  * @note if int32_t < 0 : an error occurred\n
  *       - COM_ERR_DESCRIPTOR   : icc handle parameter NOK
  *       - COM_ERR_PARAMETER    : at least one argument incorrect (e.g: buffer NULL, length min/max not respected ...)
  *       - COM_ERR_STATE        : a command is already in progress and its answer is not yet received
  *       - COM_ERR_NOICC        : no icc available to proceed the command
  *       - COM_ERR_GENERAL      : a low level error happened (e.g: no response)\n
  *       if int32_t > 0 :\n
  *       - if int32_t <= len_rsp, p_buf_rsp contains the complete ICC response\n
  *       - if int32_t > len_rsp, only the first bytes of the ICC response are available in p_buf_rsp\n
  *         means p_buf_rsp was too small to copy the complete ICC response\n
  *         (in this case int32_t contains the true length of the ICC response)
  */
int32_t com_icc_generic_access(int32_t icc,
                               const com_char_t *p_buf_cmd, int32_t len_cmd,
                               com_char_t *p_buf_rsp, int32_t len_rsp)
{
  int32_t result;
  uint8_t icc_protocol;
  uint32_t apdu_min_length;
  uint32_t apdu_max_length;

  /* Test Icc descriptor/state and change its state */
  result = com_icc_use_icc_desc(icc, COM_ICC_WAITING_RSP, &icc_protocol);

  if (result == COM_ERR_OK)
  {
    result = COM_ERR_PARAMETER;
    if (icc_protocol == (uint8_t)COM_PROTO_CSIM)
    {
      apdu_min_length = COM_ICC_CSIM_MIN_RSP_GENERIC_ACCESS_SZ;
      apdu_max_length = COM_ICC_CSIM_MAX_CMD_GENERIC_ACCESS_SZ;
    }
#if (USE_ST33 == 1)
    else if (icc_protocol == (uint8_t)COM_PROTO_NDLC)
    {
      apdu_min_length = COM_ICC_NDLC_MIN_RSP_GENERIC_ACCESS_SZ;
      apdu_max_length = COM_ICC_NDLC_MAX_CMD_GENERIC_ACCESS_SZ;
    }
#endif /* USE_ST33 == 1 */
    else
    {
      apdu_min_length = 0U;
      apdu_max_length = 0U;
    }
    /* Test the other arguments */
    if ((p_buf_cmd != NULL) && (p_buf_rsp != NULL) /* Check pointers not NULL */
        && (len_cmd > 0) && (len_rsp > 0)          /* Check length > 0 */
        && ((uint32_t)len_cmd <= apdu_max_length)  /* Check length cmd <= length max */
        && ((uint32_t)len_rsp >= apdu_min_length)  /* Check length rsp >= length min */
        && (p_buf_cmd[len_cmd] == (uint8_t)'\0'))  /* Check length cmd is correct regarding the buffer cmd */
    {
      if (icc_protocol == (uint8_t)COM_PROTO_CSIM)
      {
        result = com_icc_generic_access_csim(p_buf_cmd, len_cmd, p_buf_rsp, len_rsp);
      }
#if (USE_ST33 == 1)
      else
      {
        /* Specific test for NDLC */
        result = com_icc_generic_access_ndlc(p_buf_cmd, len_cmd, p_buf_rsp, len_rsp);
      }
#endif /* USE_ST33 ==1 */
    }

    /* Change Icc descriptor state */
    /* Next function call can't return a result NOK
       and DO NOT change previous result value */
    (void)com_icc_use_icc_desc(icc, COM_ICC_CREATED, &icc_protocol);
  }
  /* com_icc_use_icc_desc is charge to test handle and state */

  return (result);
}

/**
  * @brief  ICC session close
  * @note   Close a ICC session and release icc handle
  * @param  icc     - icc handle obtained with com_icc() call
  * @retval int32_t - ok or error value
  * @note   Possible returned value:\n
  *         - COM_ERR_OK           : icc handle release OK
  *         - COM_ERR_DESCRIPTOR   : icc handle parameter NOK
  *         - COM_ERR_STATE        : a command is already in progress and its answer is not yet received
  */
int32_t com_closeicc(int32_t icc)
{
  int32_t result;

  result = com_icc_delete_icc_desc(icc);

  return (result);
}


/*** Component Initialization/Start *******************************************/
/*** Used by com_core module - Not an User Interface **************************/

/**
  * @brief  Component initialization
  * @note   must be called only one time :
  *         - before using any other functions of com_icc*
  * @param  -
  * @retval bool      - true/false init ok/nok
  */
bool com_icc_init(void)
{
  bool result = true;

  /* Initialize ICC availability */
  for (uint8_t i = 0U; i < COM_ICC_SESSION_MAX_NB; i++)
  {
    com_icc_is_available[i]   = false;
    com_icc_is_initialized[i] = false;
    com_icc_is_power_on[i]    = false;
  }

  /* Initialize Mutex to protect ICC handle descriptor access */
  ComIccMutexHandle = rtosalMutexNew((const rtosal_char_t *)"COMICC_MUT_ICC_DESC");
  if (ComIccMutexHandle != NULL)
  {
    for (uint8_t i = 0U; i < COM_ICC_SESSION_MAX_NB; i++)
    {
      com_icc_init_icc_desc(i, ((uint8_t)COM_PROTO_CSIM + i));
    }
  }
  else
  {
    result = false;
  }

  return (result);
}


/**
  * @brief  Component start
  * @note   must be called only one time:
            - after com_icc_init
            - and before using any other functions of com_icc_*
  * @param  -
  * @retval -
  */
void com_icc_start(void)
{
  /* Datacache registration for icc status */
  (void)dc_com_core_register_gen_event_cb(&dc_com_db, com_icc_datacache_cb, (void *)NULL);
}

#endif /* USE_COM_ICC == 1 */

