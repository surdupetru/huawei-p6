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
 * @file sii_hal_linux_timer.c
 *
 * @brief Linux implementation of timer support used by Silicon Image
 *        MHL devices.
 *
 * $Author: Dave Canfield
 * $Rev: $
 * $Date: Feb. 4, 2011
 *
 *****************************************************************************/

#define SII_HAL_LINUX_TIMER_C

/***** #include statements ***************************************************/
#include "sii_hal.h"
#include "sii_hal_priv.h"
#include "si_timer_cfg.h"
#include "si_drvisrconfig.h"   //TBS
#include "si_c99support.h"
#include "si_osdebug.h"

#if defined(__KERNEL__)
#if defined(DEBUG)
#define DEBUG_PRINT(str, args...) printk("sii_hal_linux_timer.c:%d, "str, __LINE__, args)
#else
#define DEBUG_PRINT(str, args...)
#endif
#else
#define DEBUG_PRINT(str, args...)
#endif

/***** local macro definitions ***********************************************/

/** Convert a value specified in milliseconds to nanoseconds */
#define MSEC_TO_NSEC(x)	(x * 1000000UL)


/***** local type definitions ************************************************/

/***** local variable declarations *******************************************/

static struct hrtimer	hiResTimer;

static bool timerRunning;

/***** local functions *******************************************************/

/*****************************************************************************/
/**
 * @brief Wait for the specified number of milliseconds to elapse.
 *
 *****************************************************************************/
#if 0
void HalTimerWait(uint16_t m_sec)
{
        //unsigned long	time_usec = m_sec * 1000;
        //usleep_range(time_usec, time_usec);
        msleep(m_sec);
}
#endif


/*****************************************************************************/
/**
 * @brief Terminate HAL timer support.
 *
 *****************************************************************************/
void HalTimerTerm(void)
{
        hrtimer_cancel(&hiResTimer);
        timerRunning = false;
}


#ifdef NEW_TIMER_API //(
#include "osal/include/osal.h"
/*
   The new timer API is callback based.
   i.e. a registered timer callback get called when the timer expires.
*/

typedef struct _SiiOsTimerListEntry_t
{
        char *pszName;
        void (*pfnCallBack)(void *);
        SiiOsTimer_t  timerId;
} SiiOsTimerListEntry_t,*PSiiOsTimerListEntry_t;
static SiiOsTimerListEntry_t g_timerCallBackList[]= {SI_DRV_TIMER_INIT};


void HalTimerInit(void)
{
        int i;
        for (i=0; i < sizeof(g_timerCallBackList)/sizeof(g_timerCallBackList[0]); ++i)
        {
                SiiOsTimerCreate(g_timerCallBackList[i].pszName, g_timerCallBackList[i].pfnCallBack,NULL, false,0, false, &g_timerCallBackList[i].timerId);
        }
}

void HalTimerSet( uint8_t index, uint16_t m_sec )
{
        SiiOsTimerSchedule(g_timerCallBackList[index].timerId, (uint32_t) m_sec);
}
#else //)(

/***** local variable declarations *******************************************/

static ktime_t		timerPeriod;
static DEFINE_SPINLOCK(timerLock);

/*****************************************************************************
 * Array of timer values
 *****************************************************************************/

static uint16_t g_timerCounters[ TIMER_COUNT ];

static uint16_t g_timerElapsed;
static uint16_t g_elapsedTick;
static uint16_t g_timerElapsedGranularity;

static uint16_t g_timerElapsed1;
static uint16_t g_elapsedTick1;
static uint16_t g_timerElapsedGranularity1;


//static uint8_t  g_timer_rsen_done     = 1;
//static uint8_t  g_timer_deglitch_done = 1;


/***** local function prototypes *********************************************/


/***** global variable declarations *******************************************/


/***** local functions *******************************************************/

/*****************************************************************************/
/*
 *  @brief Timer callback handler.
 *
 *  @param[in]	timer	Pointer to the timer structure responsible for this
 *  					function being called.
 *
 *  @return     Returns HRTIMER_RESTART if there are still counters in use.
 *  			Otherwise HRTIMER_NORESTART is returned to stop the timer.
 *
 *****************************************************************************/

static enum hrtimer_restart TimerTickHandler(struct hrtimer *timer)
{
        uint16_t				countersDone = 0;
        uint8_t					i;
        enum hrtimer_restart	restartFlag;
        unsigned long 			flags;

//  DEBUG_PRINT("TimerTickHandler Called: %s \n", "");

        spin_lock_irqsave(&timerLock, flags);

        //decrement all active timers in array
        for ( i = 0; i < TIMER_COUNT; i++ )
        {
                if ( g_timerCounters[ i ] > 0 )
                {
                        g_timerCounters[ i ]--;
                        if (0 == g_timerCounters[i])
                        {
                                //CALL_SII_MHL_TX_DEVICE_TIMER_ISR(i)
                        }
                }
                countersDone |= g_timerCounters[ i ];
        }

        g_elapsedTick++;
        if ( g_elapsedTick == g_timerElapsedGranularity )
        {
                g_timerElapsed++;
                g_elapsedTick = 0;
        }
        g_elapsedTick1++;
        if ( g_elapsedTick1 == g_timerElapsedGranularity1 )
        {
                g_timerElapsed1++;
                g_elapsedTick1 = 0;
        }

        // Don't restart the timer if no one is requesting timer service.
        if(countersDone == 0 &&
                        g_timerElapsedGranularity == 0 &&
                        g_timerElapsedGranularity1 == 0)
        {
                timerRunning = false;
                restartFlag = HRTIMER_NORESTART;
        }
        else
        {
                timerPeriod = ktime_set(0, MSEC_TO_NSEC(1));
                hrtimer_forward(&hiResTimer, hiResTimer.base->get_time(), timerPeriod);
                restartFlag = HRTIMER_RESTART;
        }

        spin_unlock_irqrestore(&timerLock, flags);

        return restartFlag;
}


/***** public functions ******************************************************/


/*****************************************************************************/
/**
 * @brief Initialize timer support.
 *
 *****************************************************************************/
void HalTimerInit(void)
{
        uint8_t		i;

        spin_lock_init(&timerLock);

        //initializer timer counters in array
        for ( i = 0; i < TIMER_COUNT; i++ )
        {
                g_timerCounters[ i ] = 0;
        }
        g_timerElapsed  = 0;
        g_timerElapsed1 = 0;
        g_elapsedTick   = 0;
        g_elapsedTick1  = 0;
        g_timerElapsedGranularity   = 0;
        g_timerElapsedGranularity1  = 0;

        // Initialize and start a periodic 1ms. timer.
        hrtimer_init(&hiResTimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        hiResTimer.function = TimerTickHandler;

        timerRunning = false;
}



/*****************************************************************************/
/**
 * @brief Set a timer counter.
 *
 *****************************************************************************/
void HalTimerSet( uint8_t index, uint16_t m_sec )
{
        /*
            switch ( index )
            {
            case TIMER_TO_DO_RSEN_CHK:
            	HalTimerWait(m_sec);
            	DEBUG_PRINT("  TIMER_TO_DO_RSEN_CHK m_sec = %d ms\n", m_sec);
            	g_timer_rsen_done = 2;
                break;

            case TIMER_TO_DO_RSEN_DEGLITCH:
            	HalTimerWait(m_sec);
            	DEBUG_PRINT("  TIMER_TO_DO_RSEN_DEGLITCH 1 m_sec = %d ms\n", m_sec);
            	g_timer_deglitch_done = 2;
                break;

        	default:
        		DEBUG_PRINT(" HalTimerSet -- no such timer index %d, m_sec:%d ms\n", index, m_sec);
                break;
            }
        */
        unsigned long flags;

        DEBUG_PRINT("HalTimerSet: m_sec = %d ms\n", m_sec);

        // prevent any races with the timer tick handler
        spin_lock_irqsave(&timerLock, flags);

        switch ( index )
        {
        case ELAPSED_TIMER:
                DEBUG_PRINT("  ELAPSED_TIMER m_sec = %d ms\n", m_sec);
                g_timerElapsedGranularity = m_sec;
                g_timerElapsed = 0;
                g_elapsedTick = 0;
                break;

        case ELAPSED_TIMER1:
                DEBUG_PRINT("  m_sec = %d ms\n", m_sec);
                g_timerElapsedGranularity1 = m_sec;
                g_timerElapsed1 = 0;
                g_elapsedTick1 = 0;
                break;

        default:
                if (index < TIMER_COUNT)
                {
                        DEBUG_PRINT("  index %d, m_sec:%d ms\n", index, m_sec);
                        g_timerCounters[ index ] = m_sec;
                }
                break;
        }

        spin_unlock_irqrestore(&timerLock, flags);

        DEBUG_PRINT("  Timer running = %d\n", timerRunning);

        if (timerRunning)
        {
                DEBUG_PRINT("  Restarting timer -- State = %2x, timerRunning = %d\n", (unsigned int)hiResTimer.state, timerRunning);
                hrtimer_restart(&hiResTimer);
                DEBUG_PRINT("  Timer Started -- State = %2x, timerRunning = %d\n", (unsigned int)hiResTimer.state, timerRunning);
        }
        else
        {
                //DEBUG_PRINT("  Setting Timer -- period:%d ms:  State = %2x timerRunning = %d\n", timerPeriod.tv.nsec/1000000, (unsigned int)hiResTimer.state, timerRunning);
                timerPeriod = ktime_set(0, MSEC_TO_NSEC(1));
                hrtimer_start(&hiResTimer, timerPeriod, HRTIMER_MODE_REL);
                DEBUG_PRINT("  Timer Active = %2x timerRunning = %d\n", (unsigned int)hiResTimer.state, timerRunning);
        }

        if (hrtimer_active(&hiResTimer))
        {
                timerRunning = true;
        }
        else
        {
                timerRunning = false;
        }
}


/*****************************************************************************/
/**
 * @brief Check if the specified timer has expired.
 *
 *****************************************************************************/
uint8_t HalTimerExpired (uint8_t timerIndex)
{
        /*
        	bool  bReturn = true;

            switch ( timerIndex )
            {
            case TIMER_TO_DO_RSEN_CHK:
            	g_timer_rsen_done--;
            	if (g_timer_rsen_done == 0)
            	{
            		bReturn           = false;
            	}

                break;

            case TIMER_TO_DO_RSEN_DEGLITCH:
            	g_timer_deglitch_done--;
        		if (g_timer_deglitch_done == 0)
        	    {
        			bReturn           = false;
        	    }
                break;

        	default:
        		DEBUG_PRINT(" bad index %d \n", timerIndex);
                break;
            }

        	return bReturn;
        */
        uint16_t		remainingTime;
        unsigned long   flags;

        if(timerIndex >= TIMER_COUNT)
        {
                return 0;
        }

        // prevent any races with the timer tick handler   //TB - since this is a read, I'm skipping the race condition  stuff.
        spin_lock_irqsave(&timerLock, flags);

        remainingTime = g_timerCounters[timerIndex];
        //DEBUG_PRINT("Timer %d  remainingTime:%d ms\n", timerIndex, remainingTime);

        spin_unlock_irqrestore(&timerLock, flags);    //TB - since this is a read, I'm skipping the race condition  stuff.

        DEBUG_PRINT("Timer %d Expired == %s\n", timerIndex, (remainingTime == 0 ? "TRUE" : "FALSE"));

        return(remainingTime == 0);
}



/*****************************************************************************/
/**
 * @brief Read the current value of one of the elapsed timer counters.
 *
 *****************************************************************************/

uint16_t HalTimerElapsed(uint8_t elapsedTimerIndex)
{
        uint16_t		elapsedTime = 0;

        unsigned long flags;

        // prevent any races with the timer tick handler
        spin_lock_irqsave(&timerLock, flags);

        if (elapsedTimerIndex == ELAPSED_TIMER)
        {
                elapsedTime = g_timerElapsed;
        }
        else if (elapsedTimerIndex == ELAPSED_TIMER1)
        {
                elapsedTime = g_timerElapsed1;
        }

        spin_unlock_irqrestore(&timerLock, flags);

        return elapsedTime;
}


#endif //)

