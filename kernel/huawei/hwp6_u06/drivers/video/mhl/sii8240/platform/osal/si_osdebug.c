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
/*
   @file si_osdebug.c
*/
#include "si_c99support.h"
#include "si_memsegsupport.h"
#include "si_common.h"
#include "si_osdebug.h"
#include <linux/slab.h>
#include <linux/string.h>

#define ChannelIndex(channel) (channel >> 3)
#define ChannelMask(channel) (1 << (channel & 7))


extern unsigned char DebugChannelMasks[];
extern ushort DebugFormat;

// add for control log on/off by sysfile
static unsigned int gIsLogSwitchOn =0;
unsigned int SiiOsDebugGetLogSwitchState(void)
{
        return gIsLogSwitchOn;
}
void SiiOsDebugSetLogSwitchState(unsigned int state)
{
        gIsLogSwitchOn = state;
        return;
}

void SiiOsDebugChannelEnable(SiiOsalDebugChannels_e channel)
{
#if defined(DEBUG)
        uint8_t index = ChannelIndex(channel);
        uint8_t mask  = ChannelMask(channel) ;

        DebugChannelMasks[index] |= mask;
#endif
}

void SiiOsDebugChannelDisable(SiiOsalDebugChannels_e channel)
{
#if defined(DEBUG)
        uint8_t index =ChannelIndex(channel);
        uint8_t mask  =ChannelMask(channel) ;

        DebugChannelMasks[index] &= ~mask;
#endif
}

bool_t SiiOsDebugChannelIsEnabled(SiiOsalDebugChannels_e channel)
{
#if defined(DEBUG)
        uint8_t index = ChannelIndex(channel);
        uint8_t mask  = ChannelMask(channel) ;

        return (DebugChannelMasks[index] & mask)?true:false;
#else
        return false;
#endif
}



void SiiOsDebugSetConfig(uint16_t flags)
{
#if defined(DEBUG)
        DebugFormat = flags;
#endif
}



uint16_t SiiOsDebugGetConfig(void)
{
#if defined(DEBUG)
        return DebugFormat;
#else
        return 0;
#endif
}

void SiiOsDebugPrintSimple(SiiOsalDebugChannels_e channel, char *pszFormat,...)
{
#if defined(DEBUG)
        if (SiiOsDebugChannelIsEnabled( channel ))
        {
                va_list ap;
                va_start(ap,pszFormat);
                printk(pszFormat, ap);
                va_end(ap);
        }
#endif
}

void SiiOsDebugPrintSimpleAlways(char *pszFormat,...)
{
#if defined(DEBUG)
        va_list ap;
        va_start(ap,pszFormat);
        printk(pszFormat, ap);
        va_end(ap);

#endif

}

void SiiOsDebugPrintShort(SiiOsalDebugChannels_e channel, char *pszFormat,...)
{
#if defined(DEBUG)
        if (SiiOsDebugChannelIsEnabled( channel ))
        {
                va_list ap;

                va_start(ap,pszFormat);
                SiiOsDebugPrint(NULL, 0, channel, pszFormat, ap);
                va_end(ap);
        }
#endif
}

char *findLastSlash(const char *pszFileName)
{
// only print the file name, not the full path.
        char *pc;
        for(pc = (char *)&pszFileName[strlen(pszFileName)]; pc  >= pszFileName; --pc)
        {
                if ('\\' == *pc)
                {
                        ++pc;
                        break;
                }
                if ('/' ==*pc)
                {
                        ++pc;
                        break;
                }
        }
        return pc;
}

#define MAX_DEBUG_MSG_SIZE	512

char g_PrintBuffer[MAX_DEBUG_MSG_SIZE];  //TB -- use the global buffer instead, since Linux driver crashes with the kmallocs.

void SiiOsDebugPrint(const char *pszFileName, uint32_t iLineNum, uint32_t channel, const char *pszFormat, ...)
{
#if defined(DEBUG)
        uint8_t		*pBuf = NULL;
        uint8_t		*pBufOffset;
        int			remainingBufLen = MAX_DEBUG_MSG_SIZE;
        int			len;
        va_list		ap;


        if (SiiOsDebugChannelIsEnabled( channel ))
        {
                //pBuf = kmalloc(remainingBufLen, GFP_KERNEL);  //TB - removed kmalloc
                pBuf = g_PrintBuffer;
                if(pBuf == NULL)
                {
                        return;
                }
                pBufOffset = pBuf;

                if(pszFileName != NULL && (SII_OS_DEBUG_FORMAT_FILEINFO & DebugFormat))
                {
                        len = scnprintf(pBufOffset, remainingBufLen, "%s:%d ",findLastSlash(pszFileName),(int)iLineNum);
                        if(len < 0)
                        {
                                //kfree(pBuf);  //TB - no need to free using global buffer
                                return;
                        }

                        remainingBufLen -= len;
                        pBufOffset += len;
                }

                if (SII_OS_DEBUG_FORMAT_CHANNEL & DebugFormat)
                {
                        len = scnprintf(pBufOffset, remainingBufLen, "Chan:%d ", channel);
                        if(len < 0)
                        {
                                //kfree(pBuf);  //TB - no need to free using global buffer
                                return;
                        }
                        remainingBufLen -= len;
                        pBufOffset += len;
                }

                va_start(ap,pszFormat);
                vsnprintf(pBufOffset, remainingBufLen, pszFormat, ap);
                va_end(ap);

                printk(pBuf);
                //kfree(pBuf);  //TB - no need to free using global buffer
        }
#endif // #if defined(DEBUG)
}


//#ifndef C99_VA_ARG_SUPPORT //(  //TB  include these
unsigned int g_debugLineNo=0;
char *g_debugFileName=NULL;
SiiOsalDebugChannels_e g_channelArg=0;

void SiiOsDebugPrintAlways(char *pszFormat,...)
{
        if(gIsLogSwitchOn)
        {
                va_list ap;
                printk("%s:%d ",g_debugFileName,g_debugLineNo);
                va_start(ap,pszFormat);
                vprintk(pszFormat,ap);
                va_end(ap);
        }
}

void SiiOsDebugPrintAlwaysShort(char *pszFormat,...)
{
        if(gIsLogSwitchOn)
        {
                va_list ap;
                va_start(ap,pszFormat);
                vprintk(pszFormat,ap);
                va_end(ap);
        }
}

void SiiOsDebugPrintUseGlobal(
#ifndef __KERNEL__
        const
#endif
        char *pszFormat, ...)
{
#if defined(DEBUG)
        uint8_t		*pBuf = NULL;
        uint8_t		*pBufOffset;
        int			remainingBufLen = MAX_DEBUG_MSG_SIZE;
        int			len;
        va_list		ap;

        if (SiiOsDebugChannelIsEnabled( g_channelArg ))
        {
                pBuf = kmalloc(remainingBufLen, GFP_KERNEL);
                if(pBuf == NULL)
                {
                        return;
                }
                pBufOffset = pBuf;

                if(g_debugFileName!= NULL && (SII_OS_DEBUG_FORMAT_FILEINFO & DebugFormat))
                {
                        len = scnprintf(pBufOffset, remainingBufLen, "%s:%d ",findLastSlash(g_debugFileName),(int)g_debugLineNo);
                        if(len < 0)
                        {
                                kfree(pBuf);
                                return;
                        }

                        remainingBufLen -= len;
                        pBufOffset += len;
                }

                if (SII_OS_DEBUG_FORMAT_CHANNEL & DebugFormat)
                {
                        len = scnprintf(pBufOffset, remainingBufLen, "Chan:%d ", g_channelArg);
                        if(len < 0)
                        {
                                kfree(pBuf);
                                return;
                        }
                        remainingBufLen -= len;
                        pBufOffset += len;
                }
                if (NULL != pszFormat)
                {
                        va_start(ap,pszFormat);
                        vsnprintf(pBufOffset, remainingBufLen, pszFormat, ap);
                        va_end(ap);
                        printk(pBuf);
                }

                kfree(pBuf);
        }
#endif // #if defined(DEBUG)
}
//#endif //)  //TB - included above
