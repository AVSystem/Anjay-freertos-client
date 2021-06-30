/**
  ******************************************************************************
  * @file    dc_common.c
  * @author  MCD Application Team
  * @brief   Code of Data Cache common services.
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
#include <string.h>
#include <stdbool.h>

#include "dc_common.h"

#include "rtosal.h"
#include "error_handler.h"

/* Private typedef -----------------------------------------------------------*/
/** @brief Mandatory type header for data cache consumers structures */
typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t rt_state;
} dc_base_rt_info_t;

/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/

#if (USE_TRACE_CELLULAR_SERVICE == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_INFO(format, args...) TRACE_PRINT(DBG_CHAN_MAIN, DBL_LVL_P0, "DataCache:" format "\n\r", ## args)
#define PRINT_DBG(format, args...)  TRACE_PRINT(DBG_CHAN_MAIN, DBL_LVL_P1, "DataCache:" format "\n\r", ## args)
#define PRINT_API(format, args...)  TRACE_PRINT(DBG_CHAN_MAIN, DBL_LVL_P2, "DataCache API:" format "\n\r", ## args)
#define PRINT_ERR(format, args...)  TRACE_PRINT(DBG_CHAN_MAIN, DBL_LVL_ERR, "DataCache ERROR:" format "\n\r", ## args)
#else
#include <stdio.h>
#define PRINT_INFO(format, args...)  (void) printf("DataCache:" format "\n\r", ## args);
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_API(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(format, args...)   (void) printf("DataCache ERROR:" format "\n\r", ## args);
#endif /* USE_PRINTF */
#else
#define PRINT_INFO(...)   __NOP(); /* Nothing to do */
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_API(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(...)   __NOP(); /* Nothing to do */
#endif /* USE_TRACE_CELLULAR_SERVICE */

/* Global variables ----------------------------------------------------------*/

/* Global Data cache data base */
dc_com_db_t dc_com_db;

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  Common registration process to allow a consumer to register to the Data Cache notifications.
  * @param  p_dc_db         - data base reference (Must be set to &dc_com_db)
  * @param  notif_cb        - address of callback.
  * @param  id              - index in the datacache consumer info where to store data.
  * @note                     This callback is called when a Data Cache event
  *                           is sent by a call to dc_com_write_event.
  *                           The callback is executed in the writing thread context.
  * @param  p_private_data  - address of consumer private context (optional).
  * @note                     This address is passed as a parameter of the callback
  * @retval dc_com_reg_id_t - return the identifier of the registered consumer or
  *                           DC_COM_INVALID_ENTRY in case of error
  */
void dc_com_register_gen_event_cb_common(dc_com_db_t *p_dc_db,
                                         uint8_t id,
                                         dc_com_gen_event_callback_t notif_cb,
                                         const void *p_private_data);

/* Private variables ---------------------------------------------------------*/

/* Mutex to avoid  Data Cache concurrent access */
static osMutexId dc_common_mutex;

/* Functions Definition ------------------------------------------------------*/

/**
  * @brief  Allow a Data Cache producer to register to a new entry/service.
  * @param  p_dc_db         - reference to the Data Cache used. Must be set to &dc_com_db
  * @param  p_data          - address of the structure associated with Data Cache Entry.
  * @note                     this structure must be a persistent variable.
  * @param size             - size of p_data structure.
  * @retval dc_com_res_id_t - returns the identifier of the registered Data Cache entry
  */
dc_com_res_id_t dc_com_register_serv(dc_com_db_t *p_dc_db, void *p_data, uint16_t size)
{
  dc_com_res_id_t res_id;
  dc_base_rt_info_t *p_base_rt;

  if ((p_dc_db != NULL) &&
      (p_dc_db->serv_number < DC_COM_ENTRY_MAX_NB) &&
      (size >= sizeof(dc_base_rt_info_t)))
  {
    (void)rtosalMutexAcquire(dc_common_mutex, RTOSAL_WAIT_FOREVER);
    res_id = p_dc_db->serv_number;
    p_dc_db->p_dc_db[res_id]   = p_data;
    p_dc_db->dc_db_len[res_id] = size;
    p_base_rt = (dc_base_rt_info_t *)p_data;

    p_base_rt->header.res_id = res_id;
    p_base_rt->header.size   = size;
    p_base_rt->rt_state      = DC_SERVICE_OFF;

    p_dc_db->serv_number++;
    (void)rtosalMutexRelease(dc_common_mutex);
  }
  else
  {
    res_id = DC_COM_INVALID_ENTRY;
    PRINT_ERR("dc_com_register_serv : Impossible to register")
  }

  return res_id;
}

/**
  * @brief  Allow a core consumer to register to the Data Cache notifications.
  * @param  p_dc_db         - data base reference (Must be set to &dc_com_db)
  * @param  notif_cb        - address of callback.
  * @note                     This callback is called when a Data Cache event
  *                           is sent by a call to dc_com_write_event.
  *                           The callback is executed in the writing thread context.
  * @param  p_private_data  - address of consumer private context (optional).
  * @note                     This address is passed as a parameter of the callback
  * @retval dc_com_reg_id_t - return the identifier of the registered consumer or
  *                           DC_COM_INVALID_ENTRY in case of error
  */
dc_com_reg_id_t dc_com_core_register_gen_event_cb(dc_com_db_t *p_dc_db,
                                                  dc_com_gen_event_callback_t notif_cb,
                                                  const void *p_private_data)
{
  dc_com_reg_id_t consumer_id;

  if ((p_dc_db != NULL) && (notif_cb != NULL)  && (p_dc_db->consumer_core_number < DC_COM_MAX_NB_CORE_SUBSCRIBER))
  {
    consumer_id = p_dc_db->consumer_core_number;
    (void)rtosalMutexAcquire(dc_common_mutex, RTOSAL_WAIT_FOREVER);
    dc_com_register_gen_event_cb_common(p_dc_db, consumer_id, notif_cb, p_private_data);
    p_dc_db->consumer_core_number++;
    (void)rtosalMutexRelease(dc_common_mutex);
  }
  else
  {
    consumer_id = DC_COM_INVALID_ENTRY;
    PRINT_ERR("dc_com_core_register_gen_event_cb : Impossible to register to events")
  }

  return consumer_id;
}

/**
  * @brief  Allow a application consumer to register to the Data Cache notifications.
  * @param  p_dc_db         - data base reference (Must be set to &dc_com_db)
  * @param  notif_cb        - address of callback.
  * @note                     This callback is called when a Data Cache event
  *                           is sent by a call to dc_com_write_event.
  *                           The callback is executed in the writing thread context.
  * @param  p_private_data  - address of consumer private context (optional).
  * @note                     This address is passed as a parameter of the callback
  * @retval dc_com_reg_id_t - return the identifier of the registered consumer or
  *                           DC_COM_INVALID_ENTRY in case of error
  */
dc_com_reg_id_t dc_com_register_gen_event_cb(dc_com_db_t *p_dc_db,
                                             dc_com_gen_event_callback_t notif_cb,
                                             const void *p_private_data)
{
  dc_com_reg_id_t consumer_id;

  if ((p_dc_db != NULL) && (notif_cb != NULL)  && (p_dc_db->consumer_appli_number < DC_COM_MAX_NB_SUBSCRIBER))
  {
    consumer_id = p_dc_db->consumer_appli_number;
    (void)rtosalMutexAcquire(dc_common_mutex, RTOSAL_WAIT_FOREVER);
    dc_com_register_gen_event_cb_common(p_dc_db, consumer_id, notif_cb, p_private_data);
    p_dc_db->consumer_appli_number++;
    (void)rtosalMutexRelease(dc_common_mutex);
  }
  else
  {
    consumer_id = DC_COM_INVALID_ENTRY;
    PRINT_ERR("dc_com_register_gen_event_cb : Impossible to register to events")
  }

  return consumer_id;
}

/**
  * @brief  Common registration process to allow a consumer to register to the Data Cache notifications.
  * @param  p_dc_db         - data base reference (Must be set to &dc_com_db)
  * @param  notif_cb        - address of callback.
  * @param  id              - index in the datacache consumer info where to store data.
  * @note                     This callback is called when a Data Cache event
  *                           is sent by a call to dc_com_write_event.
  *                           The callback is executed in the writing thread context.
  * @param  p_private_data  - address of consumer private context (optional).
  * @note                     This address is passed as a parameter of the callback
  * @retval dc_com_reg_id_t - return the identifier of the registered consumer or
  *                           DC_COM_INVALID_ENTRY in case of error
  */
void dc_com_register_gen_event_cb_common(dc_com_db_t *p_dc_db,
                                         uint8_t id,
                                         dc_com_gen_event_callback_t notif_cb,
                                         const void *p_private_data)
{
  p_dc_db->consumer_info[id].consumer_reg_id       = id;
  p_dc_db->consumer_info[id].notif_cb              = notif_cb;
  p_dc_db->consumer_info[id].private_consumer_data = p_private_data;
}


/**
  * @brief  Allow a Data Cache producer to update data associated to a Data Cache entry.
  * @param  p_dc            - data base reference (Must be set to &dc_com_db)
  * @param  res_id          - entry/resource id
  * @param  p_data          - data to write
  * @param  len             - length of p_data to write
  * @retval dc_com_status_t - return status with DC_COM_OK or DC_COM_ERROR
  */
dc_com_status_t dc_com_write(dc_com_db_t *p_dc, dc_com_res_id_t res_id, const void *p_data, uint32_t len)
{
  dc_com_reg_id_t reg_id;
  dc_base_rt_info_t *dc_base_rt_info;
  dc_com_status_t res;
  dc_com_db_t *com_db = (dc_com_db_t *)p_dc;

  if ((p_dc != NULL) && (res_id != DC_COM_INVALID_ENTRY) && (res_id < com_db->serv_number) &&
      (com_db->dc_db_len[res_id] >= len) && (p_data != NULL))
  {
    /* check that something has changed in data to write */
    /* if nothing changed, just do nothing */
    if (memcmp(com_db->p_dc_db[res_id], p_data, len) != 0)
    {
      /* Something changed, in the input data (p_data). Write the new data to the Data Cache structure */
      /* Avoid to be interrupted by another event before the end of first event processing */
      (void)rtosalMutexAcquire(dc_common_mutex, RTOSAL_WAIT_FOREVER);

      (void)memcpy((void *)(com_db->p_dc_db[res_id]), p_data, (uint32_t)len);
      dc_base_rt_info = (dc_base_rt_info_t *)(com_db->p_dc_db[res_id]);
      dc_base_rt_info->header.res_id = res_id;
      dc_base_rt_info->header.size   = len;

      /* In consumer_info array, core consumers are place at the beginning, and application after */
      /* So for loops from the beginning to the last application registered */
      for (reg_id = 0U; reg_id < p_dc->consumer_appli_number; reg_id++)
      {
        dc_com_consumer_info_t *consumer_info;
        consumer_info = &(com_db->consumer_info[reg_id]);

        if (consumer_info->notif_cb != NULL)
        {
          /* let's call now the call back                                                  */
          consumer_info->notif_cb((dc_com_event_id_t)res_id, consumer_info->private_consumer_data);
        }
      }
      (void)rtosalMutexRelease(dc_common_mutex);
    }
    res = DC_COM_OK;
  }
  else
  {
    res = DC_COM_ERROR;
    PRINT_ERR("dc_com_write : Impossible to write to datacache. Verify parameters.")
  }
  return res;
}

/**
  * @brief  Allow a consumer to read the currents data associated to a Data Cache entry.
  * @param  p_dc            - data base reference (Must be set to &dc_com_db)
  * @param  res_id          - entry/resource id
  * @param  p_data          - data to read
  * @param  len             - length of p_data to read
  * @retval dc_com_status_t - return status with DC_COM_OK or DC_COM_ERROR
  */
dc_com_status_t dc_com_read(dc_com_db_t *p_dc, dc_com_res_id_t res_id, void *p_data, uint32_t len)
{
  dc_com_status_t res;

  if ((p_dc != NULL) && (res_id != DC_COM_INVALID_ENTRY) && (res_id < p_dc->serv_number) &&
      (p_dc->dc_db_len[res_id] >= len))
  {
    (void)memcpy(p_data, (void *)p_dc->p_dc_db[res_id], (uint32_t)len);
    res = DC_COM_OK;
  }
  else
  {
    (void)memset(p_data, 0, (uint32_t)len); /* p_data->rt_state == 0 is DC_SERVICE_UNAVAIL */
    res = DC_COM_ERROR;
    PRINT_ERR("dc_com_write : Impossible to read from datacache. Verify parameters.")
  }
  return res;
}

/**
  * @brief  Send an event to DC.
  * @param  p_dc            - data base reference (Must be set to &dc_com_db)
  * @param  event_id        - event id
  * @retval dc_com_status_t - return status with DC_COM_OK or DC_COM_ERROR
  */
dc_com_status_t dc_com_write_event(dc_com_db_t *p_dc, dc_com_event_id_t event_id)
{
  dc_com_reg_id_t reg_id;
  const dc_com_db_t *com_db = (dc_com_db_t *)p_dc;
  dc_com_status_t res;

  if (p_dc != NULL)
  {
    /* Avoid to be interrupted by another event before the end of first event processing */
    (void)rtosalMutexAcquire(dc_common_mutex, RTOSAL_WAIT_FOREVER);

    /* Calls all registered callback notification */
    /* In consumer_info array, core consumers are place at the beginning, and application after */
    /* So for loops from the beginning to the last application registered */

    for (reg_id = 0U; reg_id <  p_dc->consumer_appli_number; reg_id++)
    {
      const dc_com_consumer_info_t *consumer_info;
      consumer_info = &(com_db->consumer_info[reg_id]);
      if (consumer_info->notif_cb != NULL)
      {
        consumer_info->notif_cb(event_id, consumer_info->private_consumer_data);
      }
    }
    (void)rtosalMutexRelease(dc_common_mutex);
    res = DC_COM_OK;
  }
  else
  {
    res = DC_COM_ERROR;
  }
  return res;
}

/**
  * @brief  Initialize the Data Cache module.
  * @param  p_dc - data base reference (Must be set to &dc_com_db)
  * @retval -
  */
void dc_com_init(dc_com_db_t *p_dc)
{
  (void)memset(p_dc, 0, sizeof(dc_com_db_t));
  /* Start to register the appli consumers after all the core consumers. */
  p_dc->consumer_appli_number = DC_COM_MAX_NB_CORE_SUBSCRIBER;

  dc_common_mutex = rtosalMutexNew(NULL);
  if (dc_common_mutex == NULL)
  {
    ERROR_Handler(DBG_CHAN_UTILITIES, 1, ERROR_FATAL);
  }
}

/**
  * @brief  Start Data Cache module.
  * @param  p_dc - data base reference (Unused)
  * @retval -
  */
void dc_com_start(dc_com_db_t *p_dc)
{
  UNUSED(p_dc);
  /* Nothing to do */
  __NOP();
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

