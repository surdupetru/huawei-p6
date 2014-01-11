/**********************************************************************************
 * Si8240 Linux Driver
 *
 * Copyright (C) 2011-2012 Silicon Image Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed .as is. WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the
 * GNU General Public License for more details.
 *
 **********************************************************************************/

/**
 * @file osal.h
 *
 * Complete public API of the OSAL layer
 *
 * Don't use source control directives!
 * Don't use source control directives!
 * Don't use source control directives!
 *
 *****************************************************************************/

#ifndef _OSAL_H
#define _OSAL_H

/* TODO: check necessity of inclusion for KAL layer implementation */

#if !defined(__KERNEL__)
#include <os_compiler.h>
#include <os_linux.h>
#include "os_data.h"
#include <os_socket.h>
#include <os_file.h>
#include <os_string.h>
#if !defined(DO_NOT_USE_DMLS)
#include <os_dmls.h>
#endif

#else
#include "si_c99support.h"
#include "osal/include/os_types.h"
#include "osal/include/os_data.h"
#endif



#if (0)	//[dave]
/* used for simple prints during KAL layer integration */
#ifdef PLATFORM3
#if SHOW_PRINTF
/* output without stop */
#define TRC(a)                   printf(a", (%s, %d)\n",                                                       __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);
#define TRC_I(a,b)               printf(a", (%d), (%s, %d)\n",                                              b, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);
#define TRC_P(a,b)               printf(a", (%p), (%s, %d)\n",                                       (void*)b, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);
#define TRC_S(a,b)               printf(a", (%s), (%s, %d)\n",                                              b, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);
#define TRC_IX(a,b)              printf(a", (0x%08x), (%s, %d)\n",                                          b, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);
#define TRC_II(a,b,c)            printf(a", (%d), (%d), (%s, %d)\n",                                     b, c, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);
#define TRC_PI(a,b,c)            printf(a", (%p), (%d), (%s, %d)\n",                              (void*)b, c, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);
#define TRC_SI(a,b,c)            printf(a", (%s), (%d), (%s, %d)\n",                                     b, c, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);
#define TRC_SS(a,b,c)            printf(a", (%s), (%s), (%s, %d)\n",                                     b, c, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);
#define TRC_IXI(a,b,c)           printf(a", (0x%08x), (%d), (%s, %d)\n",                                 b, c, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);
#define TRC_SIX(a,b,c)           printf(a", (%s), (0x%08x), (%s, %d)\n",                                 b, c, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);
#define TRC_SPI(a,b,c,d)         printf(a", (%s), (%p), (%d), (%s, %d)\n",                     b, (void*)c, d, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);
#define TRC_IXII(a,b,c,d)        printf(a", (0x%08x), (%d), (%d), (%s, %d)\n",                        b, c, d, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);
#define TRC_SIXI(a,b,c,d)        printf(a", (%s), (0x%08x), (%d), (%s, %d)\n",                        b, c, d, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);
#define TRC_SII(a,b,c,d)         printf(a", (%s), (%d), (%d), (%s, %d)\n",                            b, c, d, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);
#define TRC_IXIII(a,b,c,d,e)     printf(a", (0x%08x), (%d), (%d), (%d), (%s, %d)\n",               b, c, d, e, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);
#define TRC_SIXII(a,b,c,d,e)     printf(a", (%s), (0x%08x), (%d), (%d), (%s, %d)\n",               b, c, d, e, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);
#define TRC_XXXX(a,b,c,d,e)      printf(a", (0x%02x), (0x%02x), (0x%02x), (0x%02x), (%s, %d)\n",   b, c, d, e, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);
#define TRC_SIXIII(a,b,c,d,e,f)  printf(a", (%s), (0x%08x), (%d), (%d), (%d), (%s, %d)\n",      b, c, d, e, f, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);
/* output with stop */
#define TRCS(a)                  printf(a", (%s, %d)\n",                                                       __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);getchar();
#define TRCS_I(a,b)              printf(a", (%d), (%s, %d)\n",                                              b, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);getchar();
#define TRCS_P(a,b)              printf(a", (%p), (%s, %d)\n",                                       (void*)b, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);getchar();
#define TRCS_S(a,b)              printf(a", (%s), (%s, %d)\n",                                              b, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);getchar();
#define TRCS_IX(a,b)             printf(a", (0x%08x), (%s, %d)\n",                                          b, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);getchar();
#define TRCS_II(a,b,c)           printf(a", (%d), (%d), (%s, %d)\n",                                     b, c, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);getchar();
#define TRCS_PI(a,b,c)           printf(a", (%p), (%d), (%s, %d)\n",                              (void*)b, c, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);getchar();
#define TRCS_SI(a,b,c)           printf(a", (%s), (%d), (%s, %d)\n",                                     b, c, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);getchar();
#define TRCS_SS(a,b,c)           printf(a", (%s), (%s), (%s, %d)\n",                                     b, c, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);getchar();
#define TRCS_IXI(a,b,c)          printf(a", (0x%08x), (%d), (%s, %d)\n",                                 b, c, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);getchar();
#define TRCS_SIX(a,b,c)          printf(a", (%s), (0x%08x), (%s, %d)\n",                                 b, c, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);getchar();
#define TRCS_SPI(a,b,c,d)        printf(a", (%s), (%p), (%d), (%s, %d)\n",                     b, (void*)c, d, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);getchar();
#define TRCS_IXII(a,b,c,d)       printf(a", (0x%08x), (%d), (%d), (%s, %d)\n",                        b, c, d, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);getchar();
#define TRCS_SIXI(a,b,c,d)       printf(a", (%s), (0x%08x), (%d), (%s, %d)\n",                        b, c, d, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);getchar();
#define TRCS_SII(a,b,c,d)        printf(a", (%s), (%d), (%d), (%s, %d)\n",                            b, c, d, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);getchar();
#define TRCS_IXIII(a,b,c,d,e)    printf(a", (0x%08x), (%d), (%d), (%d), (%s, %d)\n",               b, c, d, e, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);getchar();
#define TRCS_SIXII(a,b,c,d,e)    printf(a", (%s), (0x%08x), (%d), (%d), (%s, %d)\n",               b, c, d, e, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);getchar();
#define TRCS_XXXX(a,b,c,d,e)     printf(a", (0x%02x), (0x%02x), (0x%02x), (0x%02x), (%s, %d)\n",   b, c, d, e, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);getchar();
#define TRCS_SIXIII(a,b,c,d,e,f) printf(a", (%s), (0x%08x), (%d), (%d), (%d), (%s, %d)\n",      b, c, d, e, f, __BASE_FILE__, (uint32_t) __LINE__);fflush(stdout);getchar();
#else
#define TRC(a)
#define TRC_I(a,b)
#define TRC_P(a,b)
#define TRC_S(a,b)
#define TRC_IX(a,b)
#define TRC_II(a,b,c)
#define TRC_PI(a,b,c)
#define TRC_SI(a,b,c)
#define TRC_SS(a,b,c)
#define TRC_IXI(a,b,c)
#define TRC_SIX(a,b,c)
#define TRC_SPI(a,b,c,d)
#define TRC_IXII(a,b,c,d)
#define TRC_SIXI(a,b,c,d)
#define TRC_SII(a,b,c,d)
#define TRC_IXIII(a,b,c,d,e)
#define TRC_SIXII(a,b,c,d,e)
#define TRC_XXXX(a,b,c,d,e)
#define TRC_SIXIII(a,b,c,d,e,f)
#define TRCS(a)
#define TRCS_I(a,b)
#define TRCS_P(a,b)
#define TRCS_S(a,b)
#define TRCS_IX(a,b)
#define TRCS_II(a,b,c)
#define TRCS_PI(a,b,c)
#define TRCS_SI(a,b,c)
#define TRCS_SS(a,b,c)
#define TRCS_IXI(a,b,c)
#define TRCS_SIX(a,b,c)
#define TRCS_SPI(a,b,c,d)
#define TRCS_IXII(a,b,c,d)
#define TRCS_SIXI(a,b,c,d)
#define TRCS_SII(a,b,c,d)
#define TRCS_IXIII(a,b,c,d,e)
#define TRCS_SIXII(a,b,c,d,e)
#define TRCS_XXXX(a,b,c,d,e)
#define TRCS_SIXIII(a,b,c,d,e,f)
#endif /* SHOW_PRINTF */
#endif /* PLATFORM3 */

#endif // if (0)

/************************************************************************//**
*
* @brief Initialize the OSAL (public API)
*
* @return status code
* @retval SII_OS_STATUS_SUCCESS no error
* @retval SII_OS_STATUS_ERR_FAILED any kind of error
*
* @note Cannot be used from ISR.
*
******************************************************************************/
SiiOsStatus_t SiiOsInit(uint32_t maxChannels);


/************************************************************************//**
*
* @brief Cleanup the OSAL (public API)
*
* @return status code
* @retval SII_OS_STATUS_SUCCESS no error
* @retval SII_OS_STATUS_ERR_FAILED any kind of error
*
* @note Cannot be used from ISR.
*
******************************************************************************/
SiiOsStatus_t SiiOsTerm(void);


/* Semaphore APIs */


/************************************************************************//**
*
* @brief Create an OSAL semaphore (public API)
*
* @param[in]     pName         name of the semaphore (16 characters at most)
* @param[in]     maxCount      Maximum semaphore count. Value of 1 means binary semaphore.
* @param[in]     initialValue  Initial count of semaphore. Zero means semaphore is unavailable.
* @param[out]    pRetSemId     The created semaphore identifier
*
* @return status code
* @retval SII_OS_STATUS_SUCCESS no error
* @retval SII_OS_STATUS_ERR_INVALID_PARAM invalid parameter
* @retval SII_OS_STATUS_ERR_NOT_AVAIL insufficient memory
* @retval SII_OS_STATUS_ERR_FAILED any other kind of error
*
* @note Cannot be used from ISR.
* @note Can be called from a non-OSAL thread.
* @note Binary semaphores, aka "critical sections" have
*       maxCount == initialValue == 1 .
* @note The name of the semaphore is mandatory.
*
******************************************************************************/
SiiOsStatus_t SiiOsSemaphoreCreate
(
        const char *pName,
        uint32_t maxCount,
        uint32_t initialValue,
        SiiOsSemaphore_t *pRetSemId
);


/************************************************************************//**
*
* @brief Delete an OSAL semaphore (public API)
*
* @param[in]    semId        The semaphore identifier
*
* @return status code
* @retval SII_OS_STATUS_SUCCESS no error
* @retval SII_OS_STATUS_ERR_INVALID_PARAM invalid parameter
* @retval SII_OS_STATUS_ERR_FAILED any other kind of error
*
* @note Cannot be used from ISR.
* @note Can be called from a non-OSAL thread.
* @note Regarding tasks waiting on a deleted semaphore, the behaviour depends
*       on the underlying OS; the OSAL does not guarantee a default behaviour
*       (hence, applications should avoid this scenario). Typically, an
*       error is returned to the waiting tasks.
*
******************************************************************************/
SiiOsStatus_t SiiOsSemaphoreDelete(SiiOsSemaphore_t semId);


/************************************************************************//**
*
* @brief Release an OSAL semaphore (public API)
*
* @param[in]    semId        The semaphore identifier
*
* @return status code
* @retval SII_OS_STATUS_SUCCESS no error
* @retval SII_OS_STATUS_ERR_INVALID_PARAM invalid parameter
* @retval SII_OS_STATUS_ERR_SEM_COUNT_EXCEEDED too many release operations
* @retval SII_OS_STATUS_ERR_FAILED any other kind of error
*
* @note Can be used from ISR.
* @note Can be called from a non-OSAL thread.
* @note With the runtime check flag enabled, this API can track if the
* semaphore is released beyond it's maximum count specified during creation.
*
******************************************************************************/
SiiOsStatus_t SiiOsSemaphoreGive(SiiOsSemaphore_t semId);


/************************************************************************//**
*
* @brief Take an OSAL semaphore (public API)
*
* @param[in]    semId       The semaphore identifier
* @param[in]    timeMsec    timeout in milliseconds,
*                            or OS_NO_WAIT or OS_INFINITE_WAIT
*
* @return status code
* @retval SII_OS_STATUS_SUCCESS no error
* @retval SII_OS_STATUS_ERR_INVALID_PARAM invalid parameter
* @retval SII_OS_STATUS_ERR_TIMEOUT timeout
* @retval SII_OS_STATUS_ERR_FAILED any other kind of error
*
* @note Cannot be used from ISR.
* @note Can be called from a non-OSAL thread.
*
******************************************************************************/
SiiOsStatus_t SiiOsSemaphoreTake
(
        SiiOsSemaphore_t semId,
        int32_t timeMsec
);


#ifndef PLATFORM3
#if defined(OS_CONFIG_DEBUG)
/************************************************************************//**
*
* @brief Dump all existing semaphore objects (for debugging)
*
* @param[in] channel    debug channel
*
* @note Instead of the function, use the macro SII_OS_SEMAPHORE_DUMP_LIST()
*       which automatically gets compiled out for non-debug builds.
*
* @note Available only in debug builds.
* @note Cannot be used from ISR.
*
******************************************************************************/
void SiiOsSemaphoreDumpList(uint32_t channel);
#define SII_OS_SEMAPHORE_DUMP_LIST(channel) SiiOsSemaphoreDumpList(channel)
#else
#define SII_OS_SEMAPHORE_DUMP_LIST(channel) /* nothing */
#endif /* OS_CONFIG_DEBUG */
#endif /* !PLATFORM3 */


/*----------------------------------------------------------------------------------------*/
/* Message Queue APIs */


/************************************************************************//**
*
* @brief Create an OSAL message queue (public API)
*
* @param[in]     pName        name of the queue (16 characters at most)
* @param[in]     elementSize  size of a single message element, in bytes
* @param[in]     maxElements  queue cacacity, in number of messages
* @param[out]    pRetQueueId  the created queue identifier
*
* @return status code
* @retval SII_OS_STATUS_SUCCESS no error
* @retval SII_OS_STATUS_ERR_INVALID_PARAM invalid parameter
* @retval SII_OS_STATUS_ERR_FAILED any other kind of error
*
* @note Cannot be used from ISR.
* @note Can be called from a non-OSAL thread.
* @note The name of the message queue is mandatory.
*
******************************************************************************/
SiiOsStatus_t SiiOsQueueCreate
(
        const char *pName,
        size_t   elementSize,
        uint32_t maxElements,
        SiiOsQueue_t *pRetQueueId
);


/************************************************************************//**
*
* @brief Delete an OSAL message queue (public API)
*
* @param[in] queueId    queue identifier
*
* @return status code
* @retval SII_OS_STATUS_SUCCESS no error
* @retval SII_OS_STATUS_ERR_INVALID_PARAM invalid parameter
* @retval SII_OS_STATUS_ERR_FAILED any other kind of error
*
* @note Cannot be used from ISR.
* @note Can be called from a non-OSAL thread.
* @note Regarding tasks waiting on a deleted message queue, the behaviour
*       depends on the underlying OS; the OSAL does not guarantee a default
*       behaviour (hence, applications should avoid this scenario). Typically,
*       an error is returned to the waiting tasks.
*
******************************************************************************/
SiiOsStatus_t SiiOsQueueDelete(SiiOsQueue_t queueId);


/************************************************************************//**
*
* @brief Write message to an OSAL message queue (public API)
*
* @param[in] queueId    queue identifier
* @param[in] pBuffer    message
* @param[in] size       message size
*
* @return status code
* @retval SII_OS_STATUS_SUCCESS no error
* @retval SII_OS_STATUS_ERR_INVALID_PARAM invalid parameter
* @retval SII_OS_STATUS_ERR_QUEUE_FULL queue is full
* @retval SII_OS_STATUS_ERR_FAILED any other kind of error
*
* @note Can be used from ISR.
* @note Can be called from a non-OSAL thread.
*
******************************************************************************/
SiiOsStatus_t SiiOsQueueSend
(
        SiiOsQueue_t queueId,
        const void *pBuffer,
        size_t size
);


/************************************************************************//**
*
* @brief Read message from an OSAL message queue (public API)
*
* @param[in] queueId    queue identifier
* @param[out] pBuffer   message
* @param[in] timeMsec   timeout in milliseconds,
*                        or OS_NO_WAIT or OS_INFINITE_WAIT
* @param[in,out] pSize  in: buffer size, out: message size
*
* @return status code
* @retval SII_OS_STATUS_SUCCESS no error
* @retval SII_OS_STATUS_ERR_INVALID_PARAM invalid parameter, e.g.
*                                        message buffer too small
* @retval SII_OS_STATUS_ERR_QUEUE_EMPTY timeout, queue is empty
* @retval SII_OS_STATUS_ERR_FAILED any other kind of error
*
* @note Cannot be used from ISR.
* @note Can be called from a non-OSAL thread.
*
******************************************************************************/
SiiOsStatus_t SiiOsQueueReceive
(
        SiiOsQueue_t queueId,
        void *pBuffer,
        int32_t timeMsec,
        size_t *pSize
);

#ifndef PLATFORM3
#if defined(OS_CONFIG_DEBUG)
/************************************************************************//**
*
* @brief Dump all existing message queue objects (for debugging)
*
* @param[in] channel    debug channel
*
* @note Instead of the function, use the macro SII_OS_QUEUE_DUMP_LIST()
*       which automatically gets compiled out for non-debug builds.
*
* @note Available only in debug builds.
* @note Cannot be used from ISR.
*
******************************************************************************/
void SiiOsQueueDumpList(uint32_t channel);
#define SII_OS_QUEUE_DUMP_LIST(channel) SiiOsQueueDumpList(channel)
#else
#define SII_OS_QUEUE_DUMP_LIST(channel) /* nothing */
#endif /* OS_CONFIG_DEBUG */
#endif /* !PLATFORM3 */


/*----------------------------------------------------------------------------------------*/
/* Task management APIs */


/************************************************************************//**
*
* @brief Create an OSAL task (public API)
*
* @param[in]     pName           Name of the task (16 characters at most)
* @param[in]     pTaskFunction   Function to run when the task starts.
* @param[in]     pTaskArg        Parameter passed to task function
* @param[in]     priority        priority ranging from 0-31, with 0 being
*                                the lowest priority
* @param[in]     stackSize       stack size in bytes - do not include the stack
*                                required by the OS itself, this value will be
*                                added internally to the stack size. Can be set to
*                                PTHREAD_STACK_MIN to choose OS
*                                recommended default size.
* @param[out]    pRetTaskId      The created task identifier
*
* @return status code
* @retval SII_OS_STATUS_SUCCESS no error
* @retval SII_OS_STATUS_ERR_INVALID_PARAM invalid parameter
* @retval SII_OS_STATUS_ERR_NOT_AVAIL insufficient memory
* @retval SII_OS_STATUS_ERR_FAILED any other kind of error
*
* @note Can be called from a non-OSAL thread
* @note Cannot be used from ISR.
* @note The name of the task is mandatory.
* @note Certain OSs, such as Linux, require use of separate APIs for creating
*       user-mode and kernel-mode tasks. The OSAL APIs will map to the
*       corresponding OS API depending upon whether the OSAL API is invoked
*       from kernel space or user space.@n
*       The lower half of the priority range (0-14) is for user-space tasks.
*       The upper half of the priority range (15-31) is for kernel-space tasks.
* @note Certain OSs, such as ThreadX, support only 32 priority levels.
*
******************************************************************************/
SiiOsStatus_t SiiOsTaskCreate
(
        const char *pName,
        void (*pTaskFunction)(void *pArg),
        void *pTaskArg,
        uint32_t priority,
        size_t stackSize,
        SiiOsTask_t *pRetTaskId
);


/************************************************************************//**
*
* @brief Delete an OSAL task, called by itself (public API)
*
* @note Cannot be used from ISR.
* @note Cannot be called from a non-OSAL thread.
*
******************************************************************************/
void SiiOsTaskSelfDelete(void);


/************************************************************************//**
*
* @brief Put current task to sleep for specified time (public API)
*
* @param[in]    timeUsec        time to sleep, in microseconds.
*
* @return status code
* @retval SII_OS_STATUS_SUCCESS no error
* @retval SII_OS_STATUS_ERR_INVALID_PARAM invalid parameter
* @retval SII_OS_STATUS_ERR_FAILED any other kind of error
*
* @note Cannot be used from ISR.
*
******************************************************************************/
SiiOsStatus_t SiiOsTaskSleepUsec(uint64_t timeUsec);

#ifndef PLATFORM3
#if defined(OS_CONFIG_DEBUG)
/************************************************************************//**
*
* @brief Dump all existing task objects (for debugging)
*
* @param[in] channel    debug channel
*
* @note Instead of the function, use the macro SII_OS_TASK_DUMP_LIST()
*       which automatically gets compiled out for non-debug builds.
*
* @note Available only in debug builds.
* @note Cannot be used from ISR.
*
******************************************************************************/
void SiiOsTaskDumpList(uint32_t channel);
#define SII_OS_TASK_DUMP_LIST(channel) SiiOsTaskDumpList(channel)
#else
#define SII_OS_TASK_DUMP_LIST(channel) /* nothing */
#endif /* OS_CONFIG_DEBUG */
#endif /* !PLATFORM3 */


/*----------------------------------------------------------------------------------------*/
/* Timer APIs */


/************************************************************************//**
*
* @brief Create an OSAL timer (public API)
*
* @param[in]     pName           Name of the timer (16 characters at most)
* @param[in]     pTimerFunction  Function to run when the timer fires.
* @param[in]     pTimerArg       Parameter passed to timer function
* @param[in]     timerStartFlag  true: adds timer to system queue immediately.@n
*                                false: timer must be explicitly added to
*                                       system queue using
*                                       @ref SiiOsTimerSchedule().
* @param[in]     timeMsec        Timeout interval in milliseconds for firing
*                                either one-shot or periodic timer.
*                                The time value is relative, i.e., it is the
*                                delta from the current time.
*                                This parameter is valid only if timerStartFlag = true.
* @param[in]     periodicFlag    true: periodic timer,@n
*                                false: one-shot timer
* @param[out]    pRetTimerId     The created timer identifier
*
* @return status code
* @retval SII_OS_STATUS_SUCCESS no error
* @retval SII_OS_STATUS_ERR_INVALID_PARAM invalid parameter
* @retval SII_OS_STATUS_ERR_NOT_AVAIL insufficient memory
* @retval SII_OS_STATUS_ERR_FAILED any other kind of error
*
* @note Cannot be used from ISR.
* @note The name of the timer is mandatory.
*
******************************************************************************/
SiiOsStatus_t SiiOsTimerCreate
(
        const char *pName,
        void (*pTimerFunction)(void *pArg),
        void *pTimerArg,
        bool_t timerStartFlag,
        uint32_t timeMsec,
        bool_t periodicFlag,
        SiiOsTimer_t *pRetTimerId
);


/************************************************************************//**
*
* @brief Delete an OSAL timer (public API)
*
* @param[in]    timerId        the timer identifier
*
* @return status code
* @retval SII_OS_STATUS_SUCCESS no error
* @retval SII_OS_STATUS_ERR_INVALID_PARAM invalid parameter
* @retval SII_OS_STATUS_ERR_FAILED any other kind of error
*
* @note Cannot be used from ISR.
* @note Can be called from a non-OSAL thread
* @note Firing (or expiry) of a timer simply removes it from the system queue;
*       the timer still needs to be explicitly deleted. However, timer deletion
*       automatically removes it from system queue (if the timer hasn't fired, yet).
*
******************************************************************************/
SiiOsStatus_t SiiOsTimerDelete(SiiOsTimer_t timerId);


/************************************************************************//**
*
* @brief Modify timeout value of timer and add it to system queue (public API)
*
* @param[in]    timerId        The timer identifier
* @param[in]    timeMsec       New timeout interval, in milliseconds. The timeout
*                              value is relative, i.e., it is the delta from
*                              the current time.
*
* @return status code
* @retval SII_OS_STATUS_SUCCESS no error
* @retval SII_OS_STATUS_ERR_INVALID_PARAM invalid parameter
* @retval SII_OS_STATUS_ERR_FAILED any other kind of error
*
* @note Cannot be used from ISR.
* @note Can be called from a non-OSAL thread
* @note It can be used within the timer function to add the timer back to the queue.
*
******************************************************************************/
SiiOsStatus_t SiiOsTimerSchedule
(
        SiiOsTimer_t timerId,
        uint32_t timeMsec
);


/************************************************************************//**
*
* @brief Get minimum supported time resolution (public API)
*
* @return Minimum supported time resolution in milliseconds
*
* @note Can be used from ISR.
* @note Can be called from a non-OSAL thread
* @note The value will be same across OSs and will equal the worst-case resolution
* (i.e., highest numeric value) among the targeted OSs. Currently, VxWorks has
* the worst-case resolution equal to 17 milliseconds.
*
******************************************************************************/
uint32_t SiiOsGetTimeResolution(void);


/*************************************************************************//**
*
* @brief Get current time (public API)
* @param[out] pRetTime structure describing the current time
*
******************************************************************************/
void SiiOsGetTimeCurrent(SiiOsTime_t *pRetTime);


/*************************************************************************//**
*
* @brief Compare two time values and get difference (t1 - t2) in milliseconds (public API)
* @param[in] pTime1 first time value
* @param[in] pTime2 second time value
*
* @return time difference (time1 - time2) in milliseconds
*
* @note In case t1 is smaller (older) than t2, the result will be negative.
*
*****************************************************************************/
int64_t SiiOsGetTimeDifferenceMs
(
        const SiiOsTime_t *pTime1,
        const SiiOsTime_t *pTime2
);

#ifndef PLATFORM3
#if defined(OS_CONFIG_DEBUG)
/************************************************************************//**
*
* @brief Dump all existing timer objects (for debugging)
*
* @param[in] channel    debug channel
*
* @note Instead of the function, use the macro SII_OS_TIMER_DUMP_LIST()
*       which automatically gets compiled out for non-debug builds.
*
* @note Available only in debug builds.
* @note Cannot be used from ISR.
*
******************************************************************************/
void SiiOsTimerDumpList(uint32_t channel);
#define SII_OS_TIMER_DUMP_LIST(channel) SiiOsTimerDumpList(channel)
#else
#define SII_OS_TIMER_DUMP_LIST(channel) /* nothing */
#endif /* OS_CONFIG_DEBUG */
#endif /* !PLATFORM3 */

/*----------------------------------------------------------------------------------------*/
/* Memory APIs */


/************************************************************************//**
*
* @brief Allocate a contiguous block of memory (public API)
*
* @param[in]   pName  name of the block (currently unused)
* @param[in]   size   size in bytes
* @param[in]   flags  SII_OS_MEMORY_SHARED: memory will be shared across a
*                     user-kernel interface.@n
*                     SII_OS_MEMORY_CONTIGUOUS: memory needs to be physically
*                     contiguous.@n
*                     Irrespective of this flag, the memory will always be
*                     virtually contiguous.
*
* @return pointer to allocated block
* @retval NULL out of memory or other failure
*
* @note Cannot be used from ISR.
* @note The block may currently not exceed 4 GiB.
*
******************************************************************************/
void *SiiOsAlloc
(
        const char *pName,
        size_t size,
        uint32_t flags
);


/************************************************************************//**
*
* @brief Allocate and clear a contiguous block of memory (public API)
*
* @param[in]   pName  name of the block (currently unused)
* @param[in]   size   size in bytes
* @param[in]   flags  SII_OS_MEMORY_SHARED: memory will be shared across a
*                     user-kernel interface.@n
*                     SII_OS_MEMORY_CONTIGUOUS: memory needs to be physically
*                     contiguous.@n
*                     Irrespective of this flag, the memory will always be
*                     virtually contiguous.
*
* @return pointer to allocated block
* @retval NULL out of memory or other failure
*
* @note Identical to @ref SiiOsAlloc(), but additionally initializes the
*       allocated memory to zero
* @note Cannot be used from ISR.
* @note The block may currently not exceed 4 GiB.
*
******************************************************************************/
void *SiiOsCalloc
(
        const char *pName,
        size_t size,
        uint32_t flags
);


/************************************************************************//**
*
* @brief Free a previously allocated block of memory (public API)
*
* @param[in]   pAddr  address of the block
*
* @note Cannot be used from ISR.
*
******************************************************************************/
void SiiOsFree(void *pAddr);



#endif /* _OSAL_H */
