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
 * @file sii_osal_linux_timer.c
 *
 * @brief This file provides the Linux implementation of the timer support
 * 		  defined by the Silicon Image Operating System Abstraction Layer (OSAL)
 * 		  specification.
 *
 * $Author: Dave Canfield
 * $Rev: $
 * $Date: March. 16, 2011
 *
 *****************************************************************************/

#define SII_OSAL_LINUX_TIMER_C

/***** #include statements ***************************************************/
#include <linux/workqueue.h>
#include <linux/hrtimer.h>
#include <linux/slab.h>
#include "sii_hal.h"
#include "osal/include/osal.h"
#include "si_osdebug.h"

/***** local macro definitions ***********************************************/

/** Convert a value specified in milliseconds to nanoseconds */
#define MSEC_TO_NSEC(x)		(x * 1000000UL)


#define MAX_TIMER_NAME_LEN		16


/***** local type definitions ************************************************/

/* Define structure used to maintain list of outstanding timer objects. */
typedef struct _SiiOsTimerInfo_t
{
        struct	list_head		listEntry;
        struct	work_struct		workItem;
        uint8_t					flags;
        char					timerName[MAX_TIMER_NAME_LEN];
        struct hrtimer			hrTimer;
        timerCallbackHandler_t	callbackHandler;
        void					*callbackParam;
        uint32_t				timeMsec;
        bool					bPeriodic;
} timerObject_t;

// timerObject_t flag field definitions
#define TIMER_OBJ_FLAG_WORK_IP	0x01	// timer's work item callback is in process
#define TIMER_OBJ_FLAG_DEL_REQ	0x02	// a request is pending to delete this timer


/***** local variable declarations *******************************************/

static struct list_head			timerList;
static struct workqueue_struct		*timerWorkQueue;



/***** local function prototypes *********************************************/


/***** global variable declarations *******************************************/


/***** local functions *******************************************************/


/*****************************************************************************/
/*
 *  @brief This function is the unit of work that has been placed on the
 *  	   work queue when a timer expires.
 *
 *  @param[in]	work	Pointer to the workItem field of the timerObject_t
 *  					that is responsible for this function being called.
 *
 *  @return     Nothing
 *
 *****************************************************************************/
static void WorkHandler(struct work_struct *work)
{
        timerObject_t		*pTimerObj = container_of(work, timerObject_t, workItem);


        pTimerObj->flags |= TIMER_OBJ_FLAG_WORK_IP;
        if (HalAcquireIsrLock() == HAL_RET_SUCCESS)
        {
                (pTimerObj->callbackHandler)(pTimerObj->callbackParam);
                HalReleaseIsrLock();
        }
        pTimerObj->flags &= ~TIMER_OBJ_FLAG_WORK_IP;

        if(pTimerObj->flags & TIMER_OBJ_FLAG_DEL_REQ)
        {
                // Deletion of this timer was requested during the execution of
                // the callback handler so go ahead and delete it now.
                kfree(pTimerObj);
        }
}



/*****************************************************************************/
/*
 *  @brief Timer callback handler.
 *
 *  @param[in]	timer	Pointer to the timer structure responsible for this
 *  					function being called.
 *
 *  @return     Returns HRTIMER_RESTART if the timer is periodic or
 *  			HRTIMER_NORESTART if the timer is not periodic.
 *
 *****************************************************************************/
static enum hrtimer_restart TimerHandler(struct hrtimer *timer)
{
        timerObject_t		*pTimerObj = container_of(timer, timerObject_t, hrTimer);
        ktime_t				timerPeriod;


        queue_work(timerWorkQueue, &pTimerObj->workItem);

        if(pTimerObj->bPeriodic)
        {
                timerPeriod = ktime_set(0, MSEC_TO_NSEC(pTimerObj->timeMsec));

                hrtimer_forward(&pTimerObj->hrTimer,
                                pTimerObj->hrTimer.base->get_time(), timerPeriod);
                return HRTIMER_RESTART;
        }

        return HRTIMER_NORESTART;
}



/***** public functions ******************************************************/



/*****************************************************************************/
/**
 * @brief Initialize OSAL timer support.
 *
 *****************************************************************************/
SiiOsStatus_t SiiOsInit(uint32_t maxChannels)
{
        // Initialize list head used to track allocated timer objects.
        INIT_LIST_HEAD(&timerList);

        timerWorkQueue = create_workqueue("Sii_timer_work");

        if(timerWorkQueue == NULL)
        {
                return SII_OS_STATUS_ERR_NOT_AVAIL;
        }

        return SII_OS_STATUS_SUCCESS;
}



/*****************************************************************************/
/**
 * @brief Terminate OSAL timer support.
 *
 *****************************************************************************/
SiiOsStatus_t SiiOsTerm(void)
{
        timerObject_t		*timerObj;
        int					status;

        // Make sure all outstanding timer objects are canceled and the
        // memory allocated for them is freed.
        while(!list_empty(&timerList))
        {

                timerObj = list_first_entry(&timerList, timerObject_t, listEntry);
                status = hrtimer_try_to_cancel(&timerObj->hrTimer);
                if(status >= 0)
                {
                        list_del(&timerObj->listEntry);
                        kfree(timerObj);
                }
        }

        flush_workqueue(timerWorkQueue);
        destroy_workqueue(timerWorkQueue);
        timerWorkQueue = NULL;

        return SII_OS_STATUS_SUCCESS;
}



/*****************************************************************************/
/**
 * @brief Allocate a new OSAL timer object.
 *
 *****************************************************************************/
SiiOsStatus_t SiiOsTimerCreate(const char *pName, void(*pTimerFunction)(void *pArg),
                               void *pTimerArg, bool_t timerStartFlag,
                               uint32_t timeMsec, bool_t periodicFlag,
                               SiiOsTimer_t *pTimerId)
{
        timerObject_t		*timerObj;
        SiiOsStatus_t		status = SII_OS_STATUS_SUCCESS;


        if(pTimerFunction == NULL)
        {
                return SII_OS_STATUS_ERR_INVALID_PARAM;
        }

        timerObj = kmalloc(sizeof(timerObject_t), GFP_KERNEL);
        if(timerObj == NULL)
        {
                return SII_OS_STATUS_ERR_NOT_AVAIL;
        }

        strncpy(timerObj->timerName, pName, MAX_TIMER_NAME_LEN-1);
        timerObj->timerName[MAX_TIMER_NAME_LEN-1] = 0;

        timerObj->callbackHandler = pTimerFunction;
        timerObj->callbackParam = pTimerArg;
        timerObj->timeMsec = timeMsec;
        timerObj->bPeriodic = periodicFlag;

        INIT_WORK(&timerObj->workItem, WorkHandler);

        list_add(&timerObj->listEntry, &timerList);

        hrtimer_init(&timerObj->hrTimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        timerObj->hrTimer.function = TimerHandler;

        if(timerStartFlag)
        {
                status = SiiOsTimerSchedule(timerObj, timeMsec);
        }

        *pTimerId = timerObj;
        return status;
}



/*****************************************************************************/
/**
 * @brief Delete a previously allocated OSAL timer object.
 *
 *****************************************************************************/
SiiOsStatus_t SiiOsTimerDelete(SiiOsTimer_t timerId)
{
        timerObject_t	*timerObj;

        list_for_each_entry(timerObj, &timerList, listEntry)
        {
                if(timerObj == timerId)
                {
                        break;
                }
        }

        if(timerObj != timerId)
        {
                SII_PRINT_FULL(SII_OSAL_DEBUG_TRACE,
                               "Invalid timerId %p passed to SiiOsTimerDelete\n", timerId);
                return SII_OS_STATUS_ERR_INVALID_PARAM;
        }

        list_del(&timerObj->listEntry);

        hrtimer_cancel(&timerObj->hrTimer);

        if(timerObj->flags & TIMER_OBJ_FLAG_WORK_IP)
        {
                // Request to delete timer object came from within the timer's
                // callback handler.  If we were to proceed with the timer deletion
                // we would deadlock at cancel_work_sync().  So instead just flag
                // that the user wants the timer deleted.  Later when the timer
                // callback completes the timer's work handler will complete the
                // process of deleting this timer.
                timerObj->flags |= TIMER_OBJ_FLAG_DEL_REQ;
                return SII_OS_STATUS_SUCCESS;
        }

        cancel_work_sync(&timerObj->workItem);

        kfree(timerObj);

        return SII_OS_STATUS_SUCCESS;
}



/*****************************************************************************/
/**
 * @brief Schedule a callback from a previously allocated OSAL timer object.
 *
 *****************************************************************************/
SiiOsStatus_t SiiOsTimerSchedule(SiiOsTimer_t timerId, uint32_t timeMsec)
{
        timerObject_t	*timerObj;
        ktime_t			timerPeriod;


        list_for_each_entry(timerObj, &timerList, listEntry)
        {
                if(timerObj == timerId)
                {
                        break;
                }
        }

        if(timerObj != timerId)
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                "Invalid timerId %p passed to SiiOsTimerSchedule\n", timerId);
                return SII_OS_STATUS_ERR_INVALID_PARAM;
        }

        timerPeriod = ktime_set(0, MSEC_TO_NSEC(timeMsec));
        hrtimer_start(&timerObj->hrTimer, timerPeriod, HRTIMER_MODE_REL);

        return SII_OS_STATUS_SUCCESS;
}



/*****************************************************************************/
/**
 * @brief Return the timer resolution in msecs. of this implemention of
 *        the OSAL timer support.
 *
 *****************************************************************************/
uint32_t SiiOsGetTimeResolution(void)
{
        return 1;
}
