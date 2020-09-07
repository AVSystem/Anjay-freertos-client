/**
  ******************************************************************************
  * @file    com_icc.c
  * @author  MCD Application Team
  * @brief   This file implements communication with International Circuit Card
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
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
#include "com_icc.h"

#if (USE_COM_ICC == 1)

#include <stdlib.h>

#include "cmsis_os_misrac2012.h"

#include "com_err.h"
#include "com_sockets_net_compat.h"

#include "cellular_service_os.h"

#include "rng.h" /* Random functions used for icc handle */

#if (USE_DATACACHE == 1)
#include "dc_common.h"
#include "cellular_datacache.h"
#endif /* USE_DATACACHE == 1 */

/* Private defines -----------------------------------------------------------*/
#if (USE_TRACE_COM_SOCKETS == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_INFO(format, args...) TRACE_PRINT(DBG_CHAN_COMLIB, DBL_LVL_P0, "ComLib: " format "\n\r", ## args)
#define PRINT_DBG(format, args...)  TRACE_PRINT(DBG_CHAN_COMLIB, DBL_LVL_P1, "ComLib: " format "\n\r", ## args)
#define PRINT_ERR(format, args...)  TRACE_PRINT(DBG_CHAN_COMLIB, DBL_LVL_ERR, "ComLib ERROR: " format "\n\r", ## args)
#else /* USE_PRINTF == 1 */
#define PRINT_INFO(format, args...)  (void)printf("ComLib: " format "\n\r", ## args);
/* To reduce trace PRINT_DBG is deactivated when using printf */
#define PRINT_DBG(...)               __NOP(); /* Nothing to do */
#define PRINT_ERR(format, args...)   (void)printf("ComLib ERROR: " format "\n\r", ## args);
#endif /* USE_PRINTF == 0U */

#else /* USE_TRACE_COM_SOCKETS == 0U */
#define PRINT_INFO(...)  __NOP(); /* Nothing to do */
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(...)   __NOP(); /* Nothing to do */
#endif /* USE_TRACE_COM_SOCKETS == 1U */

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
  int32_t         handle;
} com_icc_desc_t;

/* Private macros ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/* Mutex to protect concurrent access to :
   ICC descriptor
   */
static osMutexId ComIccMutexHandle;

static com_icc_desc_t com_icc_desc; /* Only one Icc descriptor */

static bool com_icc_is_available; /* Icc status is managed through Datacache */

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Callback prototype */
#if (USE_DATACACHE == 1)
/* Callback called by Datacache - used to know Icc status */
static void com_icc_datacache_cb(dc_com_event_id_t dc_event_id,
                                 const void *private_gui_data);
#endif /* USE_DATACACHE == 1 */

static void com_icc_init_icc_desc(void);
static com_icc_desc_t *com_icc_provide_icc_desc(void);
static int32_t com_icc_use_icc_desc(int32_t         handle,
                                    com_icc_state_t new_state);
static int32_t com_icc_delete_icc_desc(int32_t handle);

/* ICC status is managed through Datacache */
static bool com_icc_is_icc_available(void);

/* Private function Definition -----------------------------------------------*/
#if (USE_DATACACHE == 1)
/**
  * @brief  Callback called when a value in datacache changed
  * @note   Managed datacache value changed
  * @param  dc_event_id - value changed
  * @note   -
  * @param  private_gui_data - value provided at service subscription
  * @note   Unused
  * @retval -
  */
static void com_icc_datacache_cb(dc_com_event_id_t dc_event_id,
                                 const void *private_gui_data)
{
  UNUSED(private_gui_data);

  /* Used to know Icc status */
  if (dc_event_id == DC_CELLULAR_SIM_INFO)
  {
    dc_sim_info_t dc_sim_rt_info;

    com_icc_is_available = false;
    if (dc_com_read(&dc_com_db, DC_CELLULAR_SIM_INFO,
                    (void *)&dc_sim_rt_info,
                    sizeof(dc_sim_rt_info))
        == DC_COM_OK)
    {
      PRINT_DBG("sim state: rt_state:%d index_slot:%d sim_status:%d",
                dc_sim_rt_info.rt_state,
                dc_sim_rt_info.index_slot,
                dc_sim_rt_info.sim_status[dc_sim_rt_info.index_slot])

      if (dc_sim_rt_info.rt_state == DC_SERVICE_ON)
      {
        /* Icc under test ? */
        /*
        if (dc_sim_rt_info.sim_status[dc_sim_rt_info.index_slot] == DC_SIM_CONNECTION_ON_GOING)
        {
          sim_under_test = true;
        }
        else
        {
          sim_under_test = false;
          if (dc_sim_rt_info.sim_status[dc_sim_rt_info.index_slot] == DC_SIM_OK)
          {
            sim_available = true;
          }
        }
        */
        if (dc_sim_rt_info.sim_status[dc_sim_rt_info.index_slot] == DC_SIM_OK)
        {
          com_icc_is_available = true;
        }
      }
    }
  }
}
#endif /* USE_DATACACHE == 1 */

/**
  * @brief  Provide Icc status
  * @note   -
  * @param  -
  * @retval true/false Icc is available/not available
  */
static bool com_icc_is_icc_available(void)
{
  return (com_icc_is_available);
}

/**
  * @brief  ICC descriptor initialization
  * @note   Initialize ICC descriptor
  * @param  -
  * @retval -
  */
static void com_icc_init_icc_desc(void)
{
  /* Protect Icc descriptor access */
  (void)osMutexWait(ComIccMutexHandle, RTOS_WAIT_FOREVER);

  com_icc_desc.state  = COM_ICC_INVALID;
  com_icc_desc.handle = COM_HANDLE_INVALID_ID;

  /* Access to Icc descriptor finished */
  (void)osMutexRelease(ComIccMutexHandle);
}

/**
  * @brief  ICC descriptor reservation
  * @note   Provide a ICC descriptor
  * @param  -
  * @retval com_icc_desc_t - ICC desc or NULL
  */
static com_icc_desc_t *com_icc_provide_icc_desc(void)
{
  com_icc_desc_t *icc_desc;

  /* Protect Icc descriptor access */
  (void)osMutexWait(ComIccMutexHandle, RTOS_WAIT_FOREVER);

  icc_desc = &com_icc_desc;

  /* Is Icc desc available ? */
  if (icc_desc->handle == COM_HANDLE_INVALID_ID)
  {
    /* Allocate Icc descriptor handle */
    uint32_t random;

    /* Set Icc descriptor handle to a random value */
    if (HAL_OK != HAL_RNG_GenerateRandomNumber(&hrng, &random))
    {
      random = (uint32_t)rand();
    }

    /* Handle must be >= 0 when allocation is ok */
    /* Next code line raise a MISRA issue: explicit conversion */
    /* icc_desc->handle = (int32_t)(random & 0x7FFFFFFFU); */
    /* Next code lines don't raise a MISRA issue */
    random = random & 0x7FFFFFFFU;
    icc_desc->handle = (int32_t)(random); /* no MISRA issue */

    icc_desc->state  = COM_ICC_CREATED;
  }
  else
  {
    /* Icc descriptor already in use */
    icc_desc = NULL;
  }

  /* Access to Icc descriptor finished */
  (void)osMutexRelease(ComIccMutexHandle);

  return (icc_desc);
}

/**
  * @brief  ICC descriptor usage
  * @note   Set to use/unused icc descriptor
  * @param  handle    - ICC handle
  * @param  new_state - new state of com icc
  * @retval int32_t   - OK or error value
  */
static int32_t com_icc_use_icc_desc(int32_t         handle,
                                    com_icc_state_t new_state)
{
  int32_t result;

  /* Protect Icc descriptor access */
  (void)osMutexWait(ComIccMutexHandle, RTOS_WAIT_FOREVER);

  if ((com_icc_desc.handle == handle)
      && (handle > COM_HANDLE_INVALID_ID))
  {
    if ((new_state == COM_ICC_WAITING_RSP)
        && (com_icc_desc.state == COM_ICC_CREATED))
    {
      com_icc_desc.state = COM_ICC_WAITING_RSP;
      result = COM_ERR_OK;
    }
    else if ((new_state == COM_ICC_CREATED)
             && (com_icc_desc.state == COM_ICC_WAITING_RSP))
    {
      com_icc_desc.state = COM_ICC_CREATED;
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
  (void)osMutexRelease(ComIccMutexHandle);

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

  /* Protect Icc descriptor access */
  (void)osMutexWait(ComIccMutexHandle, RTOS_WAIT_FOREVER);

  if ((com_icc_desc.handle == handle)
      && (handle > COM_HANDLE_INVALID_ID))

  {
    if (com_icc_desc.state == COM_ICC_CREATED)
    {
      com_icc_desc.handle = COM_HANDLE_INVALID_ID;
      com_icc_desc.state  = COM_ICC_INVALID;
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
  (void)osMutexRelease(ComIccMutexHandle);

  return (result);
}


/* Functions Definition ------------------------------------------------------*/
/**
  * @brief  ICC handle creation
  * @note   Create a ICC session
  * @param  family   - address family
  * @note   Supported value: COM_AF_UNSPEC
  * @param  type     - connection type
  * @note   Supported value: COM_SOCK_SEQPACKET
  * @param  protocol - protocol type
  * @note   Supported value: COM_PROTO_CSIM : communcation to SIM using AT+CSIM command
  * @retval int32_t  - icc handle or error value
  * @note   Possible returned value:\n
  *         int32_t > 0            : icc session created; the returned value is the icc handle session\n
  *         int32_t < 0            : an error occured\n
  *         - COM_ERR_LOCKED       : a icc session is already in progress
  */
int32_t com_icc(int32_t family, int32_t type, int32_t protocol)
{
  int32_t result;
  com_icc_desc_t *icc_desc;

  if ((family == COM_AF_UNSPEC)
      && (type == COM_SOCK_SEQPACKET)
      && (protocol == COM_PROTO_CSIM))
  {
    result = COM_ERR_OK;
  }
  else
  {
    result = COM_ERR_PARAMETER;
  }

  /* Continue the treatment only if arguments are OK */
  if (result == COM_ERR_OK)
  {
    /* Provide a Icc descriptor handle  */
    icc_desc = com_icc_provide_icc_desc();

    if (icc_desc != NULL)
    {
      result = icc_desc->handle;
    }
    else
    {
      result = COM_ERR_LOCKED; /* No Icc descriptor available */
    }
  }

  return (result);
}

/**
  * @brief  ICC process a generic access
  * @note   Process a ICC request for generic access
  * @param  icc            - icc handle obtained with com_icc() call
  * @param  buf_cmd        - pointer to application data buffer containg the command to send\n
  *                        data is a command APDU in format as described in ETSI TS 102 221
  * @param  len_cmd        - length of the buffer command (in bytes)
  * @note   buf_cmd (and so len_cmd) maximum length accepted is COM_ICC_MAX_GENERIC_ACCESS_CMD_SIZE bytes\n
  *         buf_cmd[len_cmd] must be '\0' in order to check cohesion with len_cmd.\n
  *         if len_cmd > COM_ICC_MAX_GENERIC_ACCESS_CMD_SIZE or buf_cmd[len_cmd] != '\0'
  *         then COM_ERR_PARAMETER is returned\n
  *         e.g: to select Master File:\n
  *              buf_cmd[15]="00A4000C023F00", sizeof(buf_cmd)=15, len_cmd=14, buf_cmd[0]='0x30'...buf_cmd[14]='\0'
  * @param  buf_rsp        - pointer to application data buffer to contain the response\n
  *                        data is a response APDU in format as described in ETSI TS 102 221
  * @param  len_rsp        - size max of the buffer response (in bytes)
  * @note   buf_rsp (and so len_rsp) minimum length accepted is COM_ICC_MIN_GENERIC_ACCESS_RSP_SIZE bytes\n
  *         if len_rsp < COM_ICC_MIN_GENERIC_ACCESS_RSP_SIZE, COM_ERR_PARAMETER is returned\n
  *         buf_rsp[MIN(len_rsp, retval)] is always set to '\0'\n
  *         e.g: if response to select Master File is SW1=90 and SW2=00:\n
  *              sizeof(buf_rsp)>=5, buf_rsp[]="9000", int32_t=4, buf_rsp[0]='0x39'...buf_rsp[4]='\0'
  * @retval int32_t        - length of the ICC response or error value
  * @note if int32_t < 0 : an error occurred\n
  *       - COM_ERR_DESCRIPTOR   : icc handle parameter NOK
  *       - COM_ERR_PARAMETER    : at least one argument incorrect (e.g: buffer NULL, length min/max not respected ...)
  *       - COM_ERR_STATE        : a command is already in progress and its answer is not yet received
  *       - COM_ERR_NOICC        : no icc available to proceed the command
  *       - COM_ERR_GENERAL      : a low level error happened (e.g: no response)\n
  *       if int32_t > 0 :\n
  *       - if int32_t <= len_rsp, buf_rsp contains the complete ICC response\n
  *       - if int32_t > len_rsp, only the first bytes of the ICC response are available in buf_rsp\n
  *         means buf_rsp was too small to copy the complete ICC response\n
  *         (in this case int32_t contains the true length of the ICC response)
  */
int32_t com_icc_generic_access(int32_t icc,
                               const com_char_t *buf_cmd, int32_t len_cmd,
                               com_char_t *buf_rsp, int32_t len_rsp)
{
  int32_t result;


  /* Test Icc descriptor/state and change its state */
  result = com_icc_use_icc_desc(icc,
                                COM_ICC_WAITING_RSP);

  if (result == COM_ERR_OK)
  {
    /* Test the other arguments */
    if (/* Check pointer not NULL */
      (buf_cmd != NULL) && (buf_rsp != NULL)
      /* Check length > 0 */
      && (len_cmd > 0) && (len_rsp > 0)
      /* Check length cmd <= length max */
      && ((uint32_t)len_cmd <= COM_ICC_MAX_GENERIC_ACCESS_CMD_SIZE)
      /* Check length rsp >= length min */
      && ((uint32_t)len_rsp >= COM_ICC_MIN_GENERIC_ACCESS_RSP_SIZE)
      /* Check length cmd is correct regarding the buffer cmd */
      && (buf_cmd[len_cmd] == (uint8_t)'\0'))
    {
      /* Check availability of the Icc */
      /* To do: Must add robustness at low-level for ICC availability */
      if (com_icc_is_icc_available() == true)
      {
        /* To do: Must manage wake-up of Modem in case of Low-Power */
        CS_sim_generic_access_t sim_generic_access;

        sim_generic_access.p_cmd_str = buf_cmd;
        sim_generic_access.p_rsp_str = buf_rsp;
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
        if (ret_suspend == CELLULAR_OK)
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
    }
    else
    {
      result = COM_ERR_PARAMETER;
    }

    /* Change Icc descriptor state */
    /* Next function call can't return a result NOK
       and DO NOT change previous result value */
    (void)com_icc_use_icc_desc(icc,
                               COM_ICC_CREATED);
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
  bool result;

  /* Initialize ICC availability */
  com_icc_is_available = false; /* ICC status updated by Datacache see com_icc_datacache_cb() */

  /* Initialize Mutex to protect ICC handle descriptor access */
  osMutexDef(ComIccMutex);
  ComIccMutexHandle = osMutexCreate(osMutex(ComIccMutex));
  if (ComIccMutexHandle != NULL)
  {
    com_icc_init_icc_desc();
    result = true;
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
#if (USE_DATACACHE == 1)
  /* Datacache registration for icc status */
  (void)dc_com_register_gen_event_cb(&dc_com_db, com_icc_datacache_cb, (void *)NULL);
#endif /* USE_DATACACHE == 1 */

}

#endif /* USE_COM_ICC == 1 */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
