/**
  ******************************************************************************
  * @file           rtosal.c
  * @author         MCD Application Team
  * @brief          This file provides code to ensure compliance with both
  *                 CMSIS RTOS V1 and V2.
  * @note           It implementents only the services used by Cellular
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
#include "rtosal.h"

/* Private typedef -----------------------------------------------------------*/
typedef char RTOS_CHAR_t;

/* Private defines -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/

/*********************************** KERNEL ***********************************/

/**
  * @brief  Initialize the RTOS kernel.
  * @retval rtosalStatus - indicate the execution status of the function.
  */
rtosalStatus rtosalKernelInitialize(void)
{
  rtosalStatus status;
#if (osCMSIS < 0x20000U)
  status = osOK; /* function not available with CMSIS RTOS V1 API */
#else
  status = osKernelInitialize();
#endif /* osCMSIS < 0x20000U */
  return (status);
}

/**
  * @brief  Start the RTOS kernel scheduler.
  * @retval rtosalStatus - indicate the execution status of the function.
  */
rtosalStatus rtosalKernelStart(void)
{
  rtosalStatus status;
  status = osKernelStart();
  return (status);
}

/**
  * @brief  Get the RTOS kernel system timer count.
  * @retval uint32_t - RTOS kernel current system timer count as 32-bit value.
  */
uint32_t rtosalGetSysTimerCount(void)
{
  uint32_t retval;
#if (osCMSIS < 0x20000U)
  retval = osKernelSysTick();
#else
  retval = osKernelGetSysTimerCount();
#endif /* osCMSIS < 0x20000U */
  return (retval);
}

/*********************************** THREAD ***********************************/

/**
  * @brief  Create a Thread and Add it to Active Threads.
  * @note   With CMSIS RTOS V1, osThreadDef_t->instances is fixed to 0.
  * @note   With CMSIS RTOS V2, this function creates a detached thread.
  * @param  p_name     - thread name.
  * @param  func       - thread function.
  * @param  priority   - initial thread priority.
  * @note   Only CMSIS RTOS V1 osPriority values should be used.
  * @param  stacksize  - stack size requirements in bytes.
  * @note   To be independent osCMSIS version used, allocated size is : stacksize * sizeof(StackType_t)
  * @param  p_arg      - argument passed to the thread function when it is started.
  * @retval osThreadId - thread ID for reference by other functions or NULL in case of error.
  */
osThreadId rtosalThreadNew(const rtosal_char_t *p_name, os_pthread func, osPriority priority, uint32_t stacksize,
                           void *p_arg)
{
  osThreadId retval;

#if (osCMSIS < 0x20000U)
  /* Thread definition */

  const osThreadDef_t rtosal_thread_def =
  {
    .name      = (RTOS_CHAR_t *)p_name,
    .pthread   = func,
    .tpriority = priority,
    .instances = 0U,
    .stacksize = stacksize /* allocated size : stacksize * sizeof(StackType_t) done by CMSIS */
  };

  retval = osThreadCreate(&rtosal_thread_def, p_arg);
#else
  /* Attributes structure for thread */
  const osThreadAttr_t rtosal_thread_attr =
  {
    .name = (const RTOS_CHAR_t *)p_name,
    .attr_bits = osThreadDetached, /* create a detached thread */
    .stack_size = stacksize * sizeof(StackType_t), /* allocated size : stacksize * sizeof(StackType_t)
                                                      not done by CMSIS - to be homogeneous do it here */
    .priority = priority
  };

  retval = osThreadNew((osThreadFunc_t)func, p_arg, &rtosal_thread_attr);
#endif /* osCMSIS < 0x20000U */

  return (retval);
}

/**
  * @brief  Return the thread ID of the current running thread.
  * @retval osThreadId - thread ID reference by other functions or NULL in case of error.
   */
osThreadId rtosalThreadGetId(void)
{
  osThreadId retval;
  retval = osThreadGetId();
  return (retval);
}

/**
  * @brief  Terminate execution of a thread and remove it from Active Threads.
  * @param  thread_id    - thread ID obtained by rtosalThreadNew.
  * @retval rtosalStatus - indicate the execution status of the function.
  */
rtosalStatus rtosalThreadTerminate(osThreadId thread_id)
{
  rtosalStatus status;
  status = osThreadTerminate(thread_id);
  return (status);
}

/********************************* SEMAPHORE **********************************/

/**
  * @brief  Create and Initialize a Semaphore object.
  * @param  p_name        - semaphore name.
  * @note   With CMSIS RTOS V1, name is unused.
  * @param  count         - number of available resources.
  * @note   At creation semaphore max count is set to count.
  * @retval osSemaphoreId - semaphore ID for reference by other functions or NULL in case of error.
  */
osSemaphoreId rtosalSemaphoreNew(const rtosal_char_t *p_name, uint32_t count)
{
  osSemaphoreId retval;

#if (osCMSIS < 0x20000U)
  (void)(p_name); /* To avoid gcc/g++ warnings */
  osSemaphoreDef(SEM); /* true name can not be used when (osCMSIS < 0x20000U) */
  retval = osSemaphoreCreate(osSemaphore(SEM), (int32_t)count); /* No issue with cast to (int32_t) */
#else
  /* Attributes structure for semaphore */
  const osSemaphoreAttr_t rtosal_sem_attr =
  {
    .name = (const RTOS_CHAR_t *)p_name
  };

  retval = osSemaphoreNew(count, count, &rtosal_sem_attr); /* maximum count is set to initial count */
#endif /* osCMSIS < 0x20000U */

  return (retval);
}

/**
  * @brief  Acquire a Semaphore token or timeout if no tokens are available.
  * @param  semaphore_id - semaphore ID obtained by rtosalSemaphoreNew.
  * @param  timeout      - timeout value (in ms) or 0 in case of no time-out.
  * @retval rtosalStatus - indicate the execution status of the function.
  * @note   With CMSIS RTOS V1, this function returns osErrorOS when no token is available.
  *         With CMSIS RTOS V2, this function returns osErrorResource when no token is available.
  *         Conclusion: test (rtosalStatus != osOK) to be independent of osCMSIS version.
  */
rtosalStatus rtosalSemaphoreAcquire(osSemaphoreId semaphore_id, uint32_t timeout)
{
  rtosalStatus status;

#if (osCMSIS < 0x20000U)
  /* Due to incompatibility between V1 and V2 return type retval is converted to osStatus
     see V1 API documentation for more details */
  status = (osStatus)osSemaphoreWait(semaphore_id, timeout);
#else
  status = osSemaphoreAcquire(semaphore_id, timeout);
#endif /* osCMSIS < 0x20000U */

  return (status);
}

/**
  * @brief  Release a Semaphore token.
  * @param  semaphore_id - semaphore ID obtained by rtosalSemaphoreNew.
  * @retval rtosalStatus - indicate the execution status of the function.
  */
rtosalStatus rtosalSemaphoreRelease(osSemaphoreId semaphore_id)
{
  rtosalStatus status;
  status = osSemaphoreRelease(semaphore_id);
  return (status);
}

/**
  * @brief  Delete a Semaphore object.
  * @param  semaphore_id - semaphore ID obtained by rtosalSemaphoreNew.
  * @retval rtosalStatus - indicate the execution status of the function.
  */
rtosalStatus rtosalSemaphoreDelete(osSemaphoreId semaphore_id)
{
  rtosalStatus status;
  status = osSemaphoreDelete(semaphore_id);
  return (status);
}

/*********************************** MUTEX ************************************/

/**
  * @brief  Create and Initialize a Mutex object.
  * @note   With CMSIS RTOS V2, this function creates a non-recursive mutex.
  * @param  p_name    - mutex name.
  * @note   With CMSIS RTOS V1, name is unused.
  * @retval osMutexId - mutex ID for reference by other functions or NULL in case of error.
  */
osMutexId rtosalMutexNew(const rtosal_char_t *p_name)
{
  osMutexId retval;

#if (osCMSIS < 0x20000U)
  (void)(p_name); /* To avoid gcc/g++ warnings */
  osMutexDef(MUTEX); /* true name can not be used when (osCMSIS < 0x20000U) */
  retval = osMutexCreate(osMutex(MUTEX));
#else
  /* Attributes structure for mutex */
  const osMutexAttr_t rtosal_mutex_attr =
  {
    .name = (const RTOS_CHAR_t *)p_name,
    .attr_bits = 0U /* create a non-recursive mutex */
  };

  retval = osMutexNew(&rtosal_mutex_attr);
#endif /* osCMSIS < 0x20000U */

  return (retval);
}

/**
  * @brief  Acquire a Mutex or timeout if it is locked.
  * @param  mutex_id     - mutex ID obtained by rtosalMutexNew.
  * @param  timeout      - timeout value (in ms) or 0 in case of no time-out.
  * @retval rtosalStatus - indicate the execution status of the function.
  * @note   With CMSIS RTOS V1, this function returns osErrorOS when no mutex is available.
  *         With CMSIS RTOS V2, this function returns osErrorResource when no mutex is available.
  *         Conclusion: test (rtosalStatus != osOK) to be independent of osCMSIS version.
  */
rtosalStatus rtosalMutexAcquire(osMutexId mutex_id, uint32_t timeout)
{
  rtosalStatus status;

#if (osCMSIS < 0x20000U)
  status = osMutexWait(mutex_id, timeout);
#else
  status = osMutexAcquire(mutex_id, timeout);
#endif /* osCMSIS < 0x20000U */

  return (status);
}

/**
  * @brief  Release a Mutex that was acquired by rtosalMutexAcquire.
  * @param  mutex_id     - mutex ID obtained by rtosalMutexNew.
  * @retval rtosalStatus - indicate the execution status of the function.
  */
rtosalStatus rtosalMutexRelease(osMutexId mutex_id)
{
  rtosalStatus status;
  status = osMutexRelease(mutex_id);
  return (status);
}

/**
  * @brief  Delete a Mutex object.
  * @param  mutex_id - mutex ID obtained by rtosalMutexNew.
  * @retval rtosalStatus - indicate the execution status of the function.
  */
rtosalStatus rtosalMutexDelete(osMutexId mutex_id)
{
  rtosalStatus status;
  status = osMutexDelete(mutex_id);
  return (status);
}

/******************************* MESSAGE QUEUE ********************************/

/**
  * @brief  Create and Initialize a Message Queue object.
  * @note   This implementation supports 32-bit sized messages only.
  * @param  p_name       - message queue name.
  * @note   With CMSIS RTOS V1, name is unused.
  * @param  queue_size   - maximum number of messages in queue.
  * @retval osMessageQId - message queue ID for reference by other functions or NULL in case of error.
  */
osMessageQId rtosalMessageQueueNew(const rtosal_char_t *p_name, uint32_t queue_size)
{
  osMessageQId retval;

#if (osCMSIS < 0x20000U)
  (void)(p_name); /* To avoid gcc/g++ warnings */
  const osMessageQDef_t rtosal_queue_def =
  {
    .queue_sz = queue_size,
    .item_sz = sizeof(uint32_t) /* This implementation supports 32-bit sized messages only */
  };

  retval = osMessageCreate(&rtosal_queue_def, NULL);
#else
  /* Attributes structure for message queue */
  const osMessageQueueAttr_t rtosal_message_attr =
  {
    .name = (const RTOS_CHAR_t *)p_name,
  };

  /* This implementation supports 32-bit sized messages only */
  retval = osMessageQueueNew(queue_size, sizeof(uint32_t), &rtosal_message_attr);
#endif /* osCMSIS < 0x20000U */

  return (retval);
}

/**
  * @brief Put a Message into a Queue or timeout if Queue is full.
  * @param mq_id         - message queue ID obtained by rtosalMessageNew.
  * @param msg           - message to put into a queue.
  * @param timeout       - timeout value (in ms) or 0 in case of no time-out.
  * @retval rtosalStatus - indicate the execution status of the function.
  * @note   With CMSIS RTOS V1, this function returns osErrorOS if the queue is full.
  *         With CMSIS RTOS V2, this function returns osErrorResource if the queue is fulle.
  *         Conclusion: test (rtosalStatus != osOK) to be independent of osCMSIS version.
  */
rtosalStatus rtosalMessageQueuePut(osMessageQId mq_id, uint32_t msg, uint32_t timeout)
{
  rtosalStatus status;

#if (osCMSIS < 0x20000U)
  status = osMessagePut(mq_id, msg, timeout);
#else
  /* Message priority always set to same priority : 0 */
  status = osMessageQueuePut(mq_id, (const void *)&msg, 0U, timeout);
#endif /* osCMSIS < 0x20000U */

  return (status);
}

/**
  * @brief Get a Message from a Queue or timeout if Queue is empty.
  * @param mq_id         - message queue id obtained by rtosalMessageNew.
  * @param p_msg         - pointer to buffer for message to get from a queue.
  * @param timeout       - timeout value (in ms) or 0 in case of no time-out.
  * @retval rtosalStatus - indicate the execution status of the function.
  * @note with CMSIS RTOS V1, this function returns osEventMessage if a msg is available.
  *       with CMSIS RTOS V2, this function returns osOK if a msg is available.
  *       Conclusion: initialize p_msg to an impossible value
  *                   test (rtosalStatus == osEventTimeout) to treat timeout (if any)
  *                   then test (*p_msg != impossible value) to ensure a msg is available
  */
rtosalStatus rtosalMessageQueueGet(osMessageQId mq_id, uint32_t *p_msg, uint32_t timeout)
{
  rtosalStatus status;

#if (osCMSIS < 0x20000U)
  osEvent event;

  if (p_msg == NULL)  /* Check parameter */
  {
    status = osErrorParameter;
  }
  else
  {
    event = osMessageGet(mq_id, timeout);

    /* Retrieve the status from the returned structure */
    status = event.status;
    /* if a msg has been received then store it in user buffer */
    if (status == osEventMessage)
    {
      *p_msg = event.value.v;
    }
  }
#else
  /* msg_prio is not managed, so set to NULL */
  status = osMessageQueueGet(mq_id, p_msg, NULL, timeout);
#endif /* osCMSIS < 0x20000U */

  return (status);
}

/*********************************** TIMER ************************************/

/**
  * @brief Create and Initialize a Timer object.
  * @param   p_name   - timer name.
  * @note    With CMSIS RTOS V1, name is unused.
  * @param   func     - function pointer to timer callback function.
  * @param   type     - osTimerOnce for one-shot or osTimerPeriodic for periodic behavior
  * @param   p_arg    - argument passed to the timer callback function when it is called.
  * @retval osTimerId - timer ID for reference by other functions or NULL in case of error.
  */
osTimerId rtosalTimerNew(const rtosal_char_t *p_name, os_ptimer func, os_timer_type type, void *p_arg)
{
  osTimerId retval;

#if (osCMSIS < 0x20000U)
  (void)(p_name); /* To avoid gcc/g++ warnings */
  osTimerDef(TIM, func); /* true name can not be used when (osCMSIS < 0x20000U) */
  retval = osTimerCreate(osTimer(TIM), type, p_arg);
#else
  /* Attributes structure for timer */
  const osTimerAttr_t rtosal_timer_attr =
  {
    .name = (const RTOS_CHAR_t *)p_name,
  };

  retval = osTimerNew((osTimerFunc_t)func, (osTimerType_t)type, p_arg, &rtosal_timer_attr);
#endif /* osCMSIS < 0x20000U */

  return (retval);
}

/**
  * @brief Start or Restart a Timer.
  * @param  timer_id     - timer ID obtained by rtosalTimerNew.
  * @param  ticks        - "time ticks" value of the timer.
  * @retval rtosalStatus - indicate the execution status of the function.
  */
rtosalStatus rtosalTimerStart(osTimerId timer_id, uint32_t ticks)
{
  rtosalStatus status;
  status = osTimerStart(timer_id, ticks);
  return (status);
}

/**
  * @brief Stop a Timer.
  * @param  timer_id     - timer ID obtained by rtosalTimerNew.
  * @retval rtosalStatus - indicate the execution status of the function.
  * @note   With CMSIS RTOS V1, this function returns osOK if the timer is not started.
  *         With CMSIS RTOS V2, this function returns osErrorResource if the timer is not started.
  */
rtosalStatus rtosalTimerStop(osTimerId timer_id)
{
  rtosalStatus status;
  status = osTimerStop(timer_id);
  return (status);
}

/**
  * @brief Delete a Timer object.
  * @param   timer_id    - timer ID obtained by rtosalTimerNew.
  * @retval rtosalStatus - indicate the execution status of the function.
  */
rtosalStatus rtosalTimerDelete(osTimerId timer_id)
{
  rtosalStatus status;
  status = osTimerDelete(timer_id);
  return (status);
}

/*********************************** DELAY ************************************/

/**
  * @brief Wait for Timeout (Time Delay).
  * @param ticks         - "time ticks" value.
  * @retval rtosalStatus - indicate the execution status of the function.
  */
rtosalStatus rtosalDelay(uint32_t ticks)
{
  rtosalStatus status;
  status = osDelay(ticks);
  return (status);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
