/**
  ******************************************************************************
  * @file           rtosal.h
  * @author         MCD Application Team
  * @brief          Header for rtosal.c
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef RTOSAL_H
#define RTOSAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* FreeRTOS headers are included in order to avoid warning during compilation */
/* FreeRTOS is a Third Party so MISRAC messages linked to it are ignored */
/*cstat -MISRAC2012-* */
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
/*cstat +MISRAC2012-* */

/* Exported constants --------------------------------------------------------*/
/* MISRAC 2012 issue link to osWaitForever usage */
/* Adding U in order to solve MISRAC2012-Dir-7.2 */
#define RTOSAL_WAIT_FOREVER  (0xFFFFFFFFU)

/* Exported types ------------------------------------------------------------*/
typedef uint8_t     rtosal_char_t;

#if (osCMSIS < 0x20000U)
typedef osStatus    rtosalStatus;
#else
typedef osStatus_t  rtosalStatus;
#endif /* osCMSIS < 0x20000U */

/* External variables --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
/*********************************** KERNEL ***********************************/
/**
  * @brief  Initialize the RTOS kernel.
  * @retval rtosalStatus - indicate the execution status of the function.
  */
rtosalStatus rtosalKernelInitialize(void);

/**
  * @brief  Start the RTOS kernel scheduler.
  * @retval rtosalStatus - indicate the execution status of the function.
  */
rtosalStatus rtosalKernelStart(void);

/**
  * @brief  Get the RTOS kernel system timer count.
  * @retval uint32_t - RTOS kernel current system timer count as 32-bit value.
  */
uint32_t rtosalGetSysTimerCount(void);

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
                           void *p_arg);

/**
  * @brief  Return the thread ID of the current running thread.
  * @retval osThreadId - thread ID reference by other functions or NULL in case of error.
   */
osThreadId rtosalThreadGetId(void);

/**
  * @brief  Terminate execution of a thread and remove it from Active Threads.
  * @param  thread_id    - thread ID obtained by rtosalThreadNew.
  * @retval rtosalStatus - indicate the execution status of the function.
  */
rtosalStatus rtosalThreadTerminate(osThreadId thread_id);

/********************************* SEMAPHORE **********************************/
/**
  * @brief  Create and Initialize a Semaphore object.
  * @param  p_name        - semaphore name.
  * @note   With CMSIS RTOS V1, name is unused.
  * @param  count         - number of available resources.
  * @note   At creation semaphore max count is set to count.
  * @retval osSemaphoreId - semaphore ID for reference by other functions or NULL in case of error.
  */
osSemaphoreId rtosalSemaphoreNew(const rtosal_char_t *p_name, uint32_t count);

/**
  * @brief  Acquire a Semaphore token or timeout if no tokens are available.
  * @param  semaphore_id - semaphore ID obtained by rtosalSemaphoreNew.
  * @param  timeout      - timeout value (in ms) or 0 in case of no time-out.
  * @retval rtosalStatus - indicate the execution status of the function.
  * @note   With CMSIS RTOS V1, this function returns osErrorOS when no token is available.
  *         With CMSIS RTOS V2, this function returns osErrorResource when no token is available.
  *         Conclusion: test (rtosalStatus != osOK) to be independent of CMSIS RTOS version.
  */
rtosalStatus rtosalSemaphoreAcquire(osSemaphoreId semaphore_id, uint32_t timeout);

/**
  * @brief  Release a Semaphore token.
  * @param  semaphore_id - semaphore ID obtained by rtosalSemaphoreNew.
  * @retval rtosalStatus - indicate the execution status of the function.
  */
rtosalStatus rtosalSemaphoreRelease(osSemaphoreId semaphore_id);


/**
  * @brief  Delete a Semaphore object.
  * @param  semaphore_id - semaphore ID obtained by rtosalSemaphoreNew.
  * @retval rtosalStatus - indicate the execution status of the function.
  */
rtosalStatus rtosalSemaphoreDelete(osSemaphoreId semaphore_id);

/*********************************** MUTEX ************************************/
/**
  * @brief  Create and Initialize a Mutex object.
  * @note   With CMSIS RTOS V2, this function creates a non-recursive mutex.
  * @param  p_name    - mutex name.
  * @note   With CMSIS RTOS V1, name is unused.
  * @retval osMutexId - mutex ID for reference by other functions or NULL in case of error.
  */
osMutexId rtosalMutexNew(const rtosal_char_t *p_name);

/**
  * @brief  Acquire a Mutex or timeout if it is locked.
  * @param  mutex_id     - mutex ID obtained by rtosalMutexNew.
  * @param  timeout      - timeout value (in ms) or 0 in case of no time-out.
  * @retval rtosalStatus - indicate the execution status of the function.
  * @note   With CMSIS RTOS V1, this function returns osErrorOS when no mutex is available.
  *         With CMSIS RTOS V2, this function returns osErrorResource when no mutex is available.
  *         Conclusion: test (rtosalStatus != osOK) to be independent of osCMSIS version.
  */
rtosalStatus rtosalMutexAcquire(osMutexId mutex_id, uint32_t timeout);

/**
  * @brief  Release a Mutex that was acquired by rtosalMutexAcquire.
  * @param  mutex_id     - mutex ID obtained by rtosalMutexNew.
  * @retval rtosalStatus - indicate the execution status of the function.
  */
rtosalStatus rtosalMutexRelease(osMutexId mutex_id);

/**
  * @brief  Delete a Mutex object.
  * @param  mutex_id - mutex ID obtained by rtosalMutexNew.
  * @retval rtosalStatus - indicate the execution status of the function.
  */
rtosalStatus rtosalMutexDelete(osMutexId mutex_id);

/******************************* MESSAGE QUEUE ********************************/
/**
  * @brief  Create and Initialize a Message Queue object.
  * @note   This implementation supports 32-bit sized messages only.
  * @param  p_name       - message queue name.
  * @note   With CMSIS RTOS V1, name is unused.
  * @param  queue_size   - maximum number of messages in queue.
  * @retval osMessageQId - message queue ID for reference by other functions or NULL in case of error.
  */
osMessageQId rtosalMessageQueueNew(const rtosal_char_t *p_name, uint32_t queue_size);

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
rtosalStatus rtosalMessageQueuePut(osMessageQId mq_id, uint32_t msg, uint32_t timeout);

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
rtosalStatus rtosalMessageQueueGet(osMessageQId mq_id, uint32_t *p_msg, uint32_t timeout);

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
osTimerId rtosalTimerNew(const rtosal_char_t *p_name, os_ptimer func, os_timer_type type, void *p_arg);

/**
  * @brief Start or Restart a Timer.
  * @param  timer_id     - timer ID obtained by rtosalTimerNew.
  * @param  ticks        - "time ticks" value of the timer.
  * @retval rtosalStatus - indicate the execution status of the function.
  */
rtosalStatus rtosalTimerStart(osTimerId timer_id, uint32_t ticks);

/**
  * @brief Stop a Timer.
  * @param  timer_id     - timer ID obtained by rtosalTimerNew.
  * @retval rtosalStatus - indicate the execution status of the function.
  * @note   With CMSIS RTOS V1, this function returns osOK if the timer is not started.
  *         With CMSIS RTOS V2, this function returns osErrorResource if the timer is not started.
  */
rtosalStatus rtosalTimerStop(osTimerId timer_id);

/**
  * @brief Delete a Timer object.
  * @param   timer_id    - timer ID obtained by rtosalTimerNew.
  * @retval rtosalStatus - indicate the execution status of the function.
  */
rtosalStatus rtosalTimerDelete(osTimerId timer_id);

/*********************************** DELAY ************************************/
/**
  * @brief Wait for Timeout (Time Delay).
  * @param ticks         - "time ticks" value.
  * @retval rtosalStatus - indicate the execution status of the function.
  */
rtosalStatus rtosalDelay(uint32_t ticks);


#ifdef __cplusplus
}
#endif

#endif /* RTOSAL_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
