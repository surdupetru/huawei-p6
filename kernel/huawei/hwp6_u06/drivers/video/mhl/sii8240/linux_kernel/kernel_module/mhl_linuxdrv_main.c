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
 * @file mhl_linuxdrv_main.c
 *
 * @brief Main entry point of the Linux driver for Silicon Image MHL transmitters.
 *
 * $Author: Dave Canfield
 * $Rev: $
 * $Date: Jan. 20, 2011
 *
 *****************************************************************************/

#define MHL_LINUXDRV_MAIN_C

/***** #include statements ***************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/input.h>
#include <linux/delay.h>
#include "si_common.h"
#include "si_mhl_defs.h"
#include "mhl_linuxdrv.h"
#include "si_mhl_tx_api.h"
#include "si_cra.h"
#include "si_osdebug.h"
#include "si_mhl_tx_base_drv_api.h"

#include <hsad/config_mgr.h>
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <linux/hw_dev_dec.h>
#endif

/***** local macro definitions ***********************************************/


/***** local variable declarations *******************************************/
static int32_t devMajor = 0;    /**< default device major number */
static struct cdev siiMhlCdev;
static struct class *siiMhlClass;
static char *buildTime = "Build: " __DATE__"-" __TIME__ "\n";
static char *buildVersion = "1.00.";

//static struct device *siiMhlClassDevice;


/***** global variable declarations *******************************************/

MHL_DRIVER_CONTEXT_T gDriverContext;

#if defined(DEBUG)
unsigned char DebugChannelMasks[SII_OSAL_DEBUG_NUM_CHANNELS/8+1]= {0xFF,0xFF,0xFF,0xFF};
//ulong DebugChannelMask = 0xFFFFFFFF;
module_param_array(DebugChannelMasks, byte, NULL, S_IRUGO | S_IWUSR);

ushort DebugFormat = SII_OS_DEBUG_FORMAT_FILEINFO;
module_param(DebugFormat, ushort, S_IRUGO | S_IWUSR);
#endif

void HalTimerWait(uint16_t m_sec)
{
        msleep(m_sec);
}


/*************************************RCP function report added by garyyuan*********************************/
struct input_dev *rmt_input=NULL;
void mhl_init_rmt_input_dev(void)
{
        /*
           if(NULL == rmt_input){
               return;
           }*/
        printk(KERN_INFO "%s:%d:.................................................\n", __func__,__LINE__);
        rmt_input = input_allocate_device();
        rmt_input->name = "mhl_rcp";
        set_bit(EV_KEY,rmt_input->evbit);
        set_bit(KEY_SELECT, rmt_input->keybit);
        set_bit(KEY_UP, rmt_input->keybit);
        set_bit(KEY_DOWN, rmt_input->keybit);
        set_bit(KEY_LEFT, rmt_input->keybit);
        set_bit(KEY_RIGHT, rmt_input->keybit);


        set_bit(KEY_MENU, rmt_input->keybit);

        set_bit(KEY_EXIT, rmt_input->keybit);


        set_bit(KEY_NUMERIC_0, rmt_input->keybit);
        set_bit(KEY_NUMERIC_1,rmt_input->keybit);
        set_bit(KEY_NUMERIC_2, rmt_input->keybit);
        set_bit(KEY_NUMERIC_3, rmt_input->keybit);
        set_bit(KEY_NUMERIC_4, rmt_input->keybit);
        set_bit(KEY_NUMERIC_5, rmt_input->keybit);
        set_bit(KEY_NUMERIC_6,rmt_input->keybit);
        set_bit(KEY_NUMERIC_7, rmt_input->keybit);
        set_bit(KEY_NUMERIC_8, rmt_input->keybit);
        set_bit(KEY_NUMERIC_9, rmt_input->keybit);

        //set_bit(KEY_DOT, rmt_input->keybit);
        set_bit(KEY_ENTER, rmt_input->keybit);
        set_bit(KEY_CLEAR, rmt_input->keybit);


        set_bit(KEY_PLAY, rmt_input->keybit);
        set_bit(KEY_STOP, rmt_input->keybit);
        set_bit(KEY_PAUSE, rmt_input->keybit);


        set_bit(KEY_REWIND, rmt_input->keybit);
        set_bit(KEY_FASTFORWARD, rmt_input->keybit);
        set_bit(KEY_EJECTCD, rmt_input->keybit);
        set_bit(KEY_FORWARD, rmt_input->keybit);
        set_bit(KEY_BACK, rmt_input->keybit);


}


/***** local functions *******************************************************/

/**
 *  @brief Start the MHL transmitter device
 *
 *  This function is called during driver startup to initialize control of the
 *  MHL transmitter device by the driver.
 *
 *  @return     0 if successful, negative error code otherwise
 *
 *****************************************************************************/
int32_t StartMhlTxDevice(void)
{
        halReturn_t		halStatus;
        //SiiOsStatus_t	osalStatus;


        pr_info("Starting %s\n", MHL_PART_NAME);

        // Initialize the Common Register Access (CRA) layer.
        if(!SiiCraInitialize())
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE, "Initialization of CRA layer failed!\n");
                return -EIO;
        }
#if 0
        // Initialize the OS Abstraction Layer (OSAL) support.
        osalStatus = SiiOsInit(0);
        if (osalStatus != SII_OS_STATUS_SUCCESS)
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                "Initialization of OSAL failed, error code: %d\n",osalStatus);
                return -EIO;
        }
#endif

        halStatus = HalInit();
        if (halStatus != HAL_RET_SUCCESS)
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                "Initialization of HAL failed, error code: %d\n",halStatus);
                //SiiOsTerm();
                return -EIO;
        }

        halStatus = HalOpenI2cDevice(MHL_PART_NAME, MHL_DRIVER_NAME);
        if (halStatus != HAL_RET_SUCCESS)
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                "Opening of I2c device %s failed, error code: %d\n",
                                MHL_PART_NAME, halStatus);
                HalTerm();
                //SiiOsTerm();
                return -EIO;
        }

        /* Initialize the MHL Tx code a polling interval of 30ms. */
        HalAcquireIsrLock();
        SiiMhlTxInitialize(EVENT_POLL_INTERVAL_30_MS);
        HalReleaseIsrLock();
        //for RCP report function by garyyuan
        mhl_init_rmt_input_dev();

        halStatus = HalInstallIrqHandler(SiiMhlTxDeviceIsr);
        if (halStatus != HAL_RET_SUCCESS)
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                "Initialization of HAL interrupt support failed, error code: %d\n",
                                halStatus);
                HalCloseI2cDevice();
                HalTerm();
                //SiiOsTerm();
                return -EIO;
        }

        return 0;
}



/**
 *  @brief Stop the MHL transmitter device
 *
 *  This function shuts down control of the transmitter device so that
 *  the driver can exit
 *
 *  @return     0 if successful, negative error code otherwise
 *
 *****************************************************************************/
int32_t StopMhlTxDevice(void)
{
        halReturn_t		halStatus;

        pr_info("Stopping %s\n", MHL_PART_NAME);

        HalRemoveIrqHandler();

        halStatus = HalCloseI2cDevice();
        if (halStatus != HAL_RET_SUCCESS)
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                "Closing of I2c device failed, error code: %d\n",halStatus);
                return -EIO;
        }

        halStatus = HalTerm();
        if (halStatus != HAL_RET_SUCCESS)
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                "Termination of HAL failed, error code: %d\n",halStatus);
                return -EIO;
        }

        //SiiOsTerm();
        return 0;
}




/***** public functions ******************************************************/

/**
 * @brief Handle read request to the connection_state attribute file.
 */
ssize_t ShowConnectionState(struct device *dev, struct device_attribute *attr,
                            char *buf)
{
        if(gDriverContext.flags & MHL_STATE_FLAG_CONNECTED)
        {
                return scnprintf(buf, PAGE_SIZE, "connected %s_ready",
                                 gDriverContext.flags & MHL_STATE_FLAG_RCP_READY? "rcp" : "not_rcp");
        }
        else
        {
                return scnprintf(buf, PAGE_SIZE, "not connected");
        }
}


/**
 * @brief Handle read request to the rcp_keycode attribute file.
 */
ssize_t ShowRcp(struct device *dev, struct device_attribute *attr,
                char *buf)
{
        int		status = 0;

        if(HalAcquireIsrLock() != HAL_RET_SUCCESS)
        {
                return -ERESTARTSYS;
        }

        if(gDriverContext.flags &
                        (MHL_STATE_FLAG_RCP_SENT | MHL_STATE_FLAG_RCP_RECEIVED))
        {
                status = scnprintf(buf, PAGE_SIZE, "0x%02x %s",
                                   gDriverContext.keyCode,
                                   gDriverContext.flags & MHL_STATE_FLAG_RCP_SENT? "sent" : "received");
        }

        HalReleaseIsrLock();
        return status;
}



/**
 * @brief Handle write request to the rcp_keycode attribute file.
 */
ssize_t SendRcp(struct device *dev, struct device_attribute *attr,
                const char *buf, size_t count)
{
        unsigned long	keyCode;
        int				status = -EINVAL;

        SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE, "SendRcp received string: ""%s""\n", buf);

        if(HalAcquireIsrLock() != HAL_RET_SUCCESS)
        {
                return -ERESTARTSYS;
        }

        while(gDriverContext.flags & MHL_STATE_FLAG_RCP_READY)
        {

                if(strict_strtoul(buf, 0, &keyCode))
                {
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE, "Unable to convert keycode string\n");
                        break;
                }

                if(keyCode >= 0xFE)
                {
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                        "keycode (0x%x) is too large to be valid\n", keyCode);
                        break;
                }

                gDriverContext.flags &= ~(MHL_STATE_FLAG_RCP_RECEIVED |
                                          MHL_STATE_FLAG_RCP_ACK |
                                          MHL_STATE_FLAG_RCP_NAK);
                gDriverContext.flags |= MHL_STATE_FLAG_RCP_SENT;
                gDriverContext.keyCode = (uint8_t)keyCode;
                SiiMhlTxRcpSend((uint8_t)keyCode);
                status = count;
                break;
        }

        HalReleaseIsrLock();

        return status;
}


/**
 * @brief Handle write request to the rcp_ack attribute file.
 *
 * This file is used to send either an ACK or NAK for a received
 * Remote Control Protocol (RCP) key code.
 *
 * The format of the string in buf must be:
 * 	"keycode=<keyvalue> errorcode=<errorvalue>
 * 	where:	<keyvalue>		is replaced with value of the RCP to be ACK'd or NAK'd
 * 			<errorvalue>	0 if the RCP key code is to be ACK'd
 * 							non-zero error code if the RCP key code is to be NAK'd
 */
ssize_t SendRcpAck(struct device *dev, struct device_attribute *attr,
                   const char *buf, size_t count)
{
        unsigned long	keyCode = 0x100;	// initialize with invalid values
        unsigned long	errCode = 0x100;
        char			*pStr;
        int				status = -EINVAL;

        SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE, "SendRcpAck received string: ""%s""\n", buf);

        // Parse the input string and extract the RCP key code and error code
        do
        {
                pStr = strstr(buf, "keycode=");
                if(pStr != NULL)
                {
                        if(strict_strtoul(pStr + 8, 0, &keyCode))
                        {
                                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE, "Unable to convert keycode string\n");
                                break;
                        }
                }
                else
                {
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE, "Invalid string format, can't "\
                                        "find ""keycode"" value\n");
                        break;
                }

                pStr = strstr(buf, "errorcode=");
                if(pStr != NULL)
                {
                        if(strict_strtoul(pStr + 10, 0, &errCode))
                        {
                                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE, "Unable to convert errorcode string\n");
                                break;
                        }
                }
                else
                {
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE, "Invalid string format, can't "\
                                        "find ""errorcode"" value\n");
                        break;
                }
        }
        while(false);

        if((keyCode > 0xFF) || (errCode > 0xFF))
        {
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE, "Invalid key code or error code "\
                                "specified, key code: 0x%02x  error code: 0x%02x\n",
                                keyCode, errCode);
                return status;
        }

        if(HalAcquireIsrLock() != HAL_RET_SUCCESS)
        {
                return -ERESTARTSYS;
        }

        while(gDriverContext.flags & MHL_STATE_FLAG_RCP_READY)
        {

                if((keyCode != gDriverContext.keyCode)
                                || !(gDriverContext.flags & MHL_STATE_FLAG_RCP_RECEIVED))
                {

                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                        "Attempting to ACK a key code that was not received!\n");
                        break;
                }

                if(errCode == 0)
                {
                        SiiMhlTxRcpkSend((uint8_t)keyCode);
                }
                else
                {
                        SiiMhlTxRcpeSend((uint8_t)errCode);
                }

                status = count;
                break;
        }

        HalReleaseIsrLock();

        return status;
}



/**
 * @brief Handle read request to the rcp_ack attribute file.
 *
 * Reads from this file return a string detailing the last RCP
 * ACK or NAK received by the driver.
 *
 * The format of the string returned in buf is:
 * 	"keycode=<keyvalue> errorcode=<errorvalue>
 * 	where:	<keyvalue>		is replaced with value of the RCP key code for which
 * 							an ACK or NAK has been received.
 * 			<errorvalue>	0 if the last RCP key code was ACK'd or
 * 							non-zero error code if the RCP key code was NAK'd
 */
ssize_t ShowRcpAck(struct device *dev, struct device_attribute *attr,
                   char *buf)
{
        int				status = -EINVAL;

        SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE, "ShowRcpAck called\n");

        if(HalAcquireIsrLock() != HAL_RET_SUCCESS)
        {
                return -ERESTARTSYS;
        }

        if(gDriverContext.flags & (MHL_STATE_FLAG_RCP_ACK | MHL_STATE_FLAG_RCP_NAK))
        {

                status = scnprintf(buf, PAGE_SIZE, "keycode=0x%02x errorcode=0x%02x",
                                   gDriverContext.keyCode, gDriverContext.errCode);
        }

        HalReleaseIsrLock();

        return status;
}



/**
 * @brief Handle write request to the devcap attribute file.
 *
 * Writes to the devcap file are done to set the offset of a particular
 * Device Capabilities register to be returned by a subsequent read
 * from this file.
 *
 * All we need to do is validate the specified offset and if valid
 * save it for later use.
 */
ssize_t SelectDevCap(struct device *dev, struct device_attribute *attr,
                     const char *buf, size_t count)
{
        unsigned long	devCapOffset;
        int				status = -EINVAL;

        SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE, "SelectDevCap received string: ""%s""\n", buf);

        do
        {

                if(strict_strtoul(buf, 0, &devCapOffset))
                {
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                        "Unable to convert register offset string\n");
                        break;
                }

                if(devCapOffset >= 0x0F)
                {
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                        "dev cap offset (0x%x) is too large to be valid\n",
                                        devCapOffset);
                        break;
                }

                gDriverContext.devCapOffset = (uint8_t)devCapOffset;

                status = count;

        }
        while(false);

        return status;
}


static ssize_t LogSwitchShow(struct device *dev, struct device_attribute *attr,char *buf)
{
        unsigned int logSwitchState =0;
        logSwitchState = SiiOsDebugGetLogSwitchState();
        return sprintf(buf, "%d\n", logSwitchState);
}

static ssize_t LogSwitchStore(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
        unsigned int logSwitchState =0;
        sscanf(buf, "%d", &logSwitchState);
        SiiOsDebugSetLogSwitchState(logSwitchState);
        return size;
}

/**
 * @brief Handle read request to the devcap attribute file.
 *
 * Reads from this file return the hexadecimal string value of the last
 * Device Capability register offset written to this file.
 *
 * The return value is the number characters written to buf, or EAGAIN
 * if the driver is busy and cannot service the read request immediately.
 * If EAGAIN is returned the caller should wait a little and retry the
 * read.
 *
 * The format of the string returned in buf is:
 * 	"offset:<offset>=<regvalue>
 * 	where:	<offset>	is the last Device Capability register offset
 * 						written to this file
 * 			<regvalue>	the currentl value of the Device Capability register
 * 						specified in offset
 */
ssize_t ReadDevCap(struct device *dev, struct device_attribute *attr,
                   char *buf)
{
        uint8_t		regValue;
        int			status = -EINVAL;

        SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE, "ReadDevCap called\n");

        if(HalAcquireIsrLock() != HAL_RET_SUCCESS)
        {
                return -ERESTARTSYS;
        }

        do
        {
                if(gDriverContext.flags & MHL_STATE_FLAG_CONNECTED)
                {

                        status = SiiTxGetPeerDevCapEntry(gDriverContext.devCapOffset,
                                                         &regValue);
                        if(status != 0)
                        {
                                // Driver is busy and cannot provide the requested DEVCAP
                                // register value right now so inform caller they need to
                                // try again later.
                                status = -EAGAIN;
                                break;
                        }
                        status = scnprintf(buf, PAGE_SIZE, "offset:0x%02x=0x%02x",
                                           gDriverContext.devCapOffset, regValue);
                }
        }
        while(false);

        HalReleaseIsrLock();

        return status;
}

void ReportKeyCode(unsigned short keyCode)
{
        struct input_dev *pRcpDev = gDriverContext.pRcpDevice;

        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nReportKeyCode: Keycode = %d\n",keyCode);

        input_report_key(pRcpDev, keyCode, 1); /* 1 means press down the key*/
        input_sync(pRcpDev);
        msleep(200);
        if (keyCode == KEY_FASTFORWARD || keyCode == KEY_REWIND)
        {
                msleep(3000);
        }

        input_report_key(pRcpDev, keyCode, 0); /* 0 means release the key*/
        input_sync(pRcpDev);
}

#define MAX_EVENT_STRING_LEN 40
/*****************************************************************************/
/**
 * @brief Handler for MHL hot plug detect status change notifications
 *  from the MhlTx layer.
 *
 *****************************************************************************/
void  AppNotifyMhlDownStreamHPDStatusChange(bool_t connected)
{
        char	event_string[MAX_EVENT_STRING_LEN];
        char	*envp[] = {event_string, NULL};


        SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                        "AppNotifyMhlDownStreamHPDStatusChange called, "\
                        "HPD status is: %s\n", connected? "CONNECTED" : "NOT CONNECTED");

        snprintf(event_string, MAX_EVENT_STRING_LEN, "MHLEVENT=%s",
                 connected? "HPD" : "NO_HPD");
        kobject_uevent_env(&gDriverContext.pDevice->kobj,
                           KOBJ_CHANGE, envp);
}



/*****************************************************************************/
/**
 * @brief Handler for most of the event notifications from the MhlTx layer.
 *
 *****************************************************************************/
MhlTxNotifyEventsStatus_e  AppNotifyMhlEvent(uint8_t eventCode, uint8_t eventParam,void *pRefData)
{
        char	event_string[MAX_EVENT_STRING_LEN];
        char	*envp[] = {event_string, NULL};
        MhlTxNotifyEventsStatus_e rtnStatus = MHL_TX_EVENT_STATUS_PASSTHROUGH;
        unsigned short keyCode = KEY_RESERVED;

        SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                        "AppNotifyEvent called, eventCode: 0x%02x eventParam: 0x%02x\n",
                        eventCode, eventParam);

        // Save the info on the most recent event.  This is done to support the
        // SII_MHL_GET_MHL_TX_EVENT IOCTL.  If at some point in the future the
        // driver's IOCTL interface is abandoned in favor of using sysfs attributes
        // this can be removed.
        gDriverContext.pendingEvent = eventCode;
        gDriverContext.pendingEventData = eventParam;

        switch(eventCode)
        {

        case MHL_TX_EVENT_CONNECTION:
                gDriverContext.flags |= MHL_STATE_FLAG_CONNECTED;
                strncpy(event_string, "MHLEVENT=connected", MAX_EVENT_STRING_LEN);
                kobject_uevent_env(&gDriverContext.pDevice->kobj,
                                   KOBJ_CHANGE, envp);
#ifdef	BYPASS_VBUS_HW_SUPPORT //(
                // turn off VBUS power here
#endif //)
                break;

        case MHL_TX_EVENT_RCP_READY:
                gDriverContext.flags |= MHL_STATE_FLAG_RCP_READY;
                strncpy(event_string, "MHLEVENT=rcp_ready", MAX_EVENT_STRING_LEN);
                kobject_uevent_env(&gDriverContext.pDevice->kobj,
                                   KOBJ_CHANGE, envp);
                break;

        case MHL_TX_EVENT_DISCONNECTION:
                gDriverContext.flags = 0;
                gDriverContext.keyCode = 0;
                gDriverContext.errCode = 0;
                strncpy(event_string, "MHLEVENT=disconnected", MAX_EVENT_STRING_LEN);
                kobject_uevent_env(&gDriverContext.pDevice->kobj,
                                   KOBJ_CHANGE, envp);
#ifdef SHARE_SWITCHER_WITH_USB
                HalSwitcherSwitchToMhl(false);
#endif
                break;

        case MHL_TX_EVENT_RCP_RECEIVED:
                gDriverContext.flags &= ~MHL_STATE_FLAG_RCP_SENT;
                gDriverContext.flags |= MHL_STATE_FLAG_RCP_RECEIVED;
                gDriverContext.keyCode = eventParam;
                snprintf(event_string, MAX_EVENT_STRING_LEN,
                         "MHLEVENT=received_RCP key code=0x%02x", eventParam);
                kobject_uevent_env(&gDriverContext.pDevice->kobj,
                                   KOBJ_CHANGE, envp);
                if(eventParam > 0x7F)
                {
                        break;
                }
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"Received an	RCP	key code = %02X\n", (int)eventParam);

                /* Added RCP key printf and interface with	UI. */
                switch (gDriverContext.keyCode)
                {
                case MHL_RCP_CMD_SELECT:
                        keyCode	= KEY_REPLY;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_REPLY received\n\n");
                        break;
                case MHL_RCP_CMD_UP:
                        keyCode	= KEY_UP;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_UP received\n\n");
                        break;
                case MHL_RCP_CMD_DOWN:
                        keyCode	= KEY_DOWN;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_DOWN received\n\n");
                        break;
                case MHL_RCP_CMD_LEFT:
                        keyCode	= KEY_LEFT;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_LEFT received\n\n");
                        break;
                case MHL_RCP_CMD_RIGHT:
                        keyCode	= KEY_RIGHT;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_RIGHT received\n\n");
                        break;
                case MHL_RCP_CMD_ROOT_MENU:
                        keyCode	= KEY_HOMEPAGE;//KEY_MENU;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_MENU received\n\n");
                        break;
                case MHL_RCP_CMD_EXIT:
                        keyCode	= KEY_BACK;//KEY_EXIT;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_EXIT received\n\n");
                        break;
                case MHL_RCP_CMD_NUM_0:
                        keyCode	= KEY_0;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_0 received\n\n");
                        break;
                case MHL_RCP_CMD_NUM_1:
                        keyCode	= KEY_1;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_1 received\n\n");
                        break;
                case MHL_RCP_CMD_NUM_2:
                        keyCode	= KEY_2;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_2 received\n\n");
                        break;
                case MHL_RCP_CMD_NUM_3:
                        keyCode	= KEY_3;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_3 received\n\n");
                        break;
                case MHL_RCP_CMD_NUM_4:
                        keyCode	= KEY_4;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_4 received\n\n");
                        break;
                case MHL_RCP_CMD_NUM_5:
                        keyCode	= KEY_5;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_5 received\n\n");
                        break;
                case MHL_RCP_CMD_NUM_6:
                        keyCode	= KEY_6;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_6 received\n\n");
                        break;
                case MHL_RCP_CMD_NUM_7:
                        keyCode	= KEY_7;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_7 received\n\n");
                        break;
                case MHL_RCP_CMD_NUM_8:
                        keyCode	= KEY_8;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_8 received\n\n");
                        break;
                case MHL_RCP_CMD_NUM_9:
                        keyCode	= KEY_9;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_9 received\n\n");
                        break;
                case MHL_RCP_CMD_DOT:
                        keyCode	= KEY_DOT;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_DOT received\n\n");
                        break;
                case MHL_RCP_CMD_ENTER:
                        keyCode	= KEY_ENTER;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_ENTER received\n\n");
                        break;
                case MHL_RCP_CMD_CLEAR:
                        keyCode	= KEY_BACKSPACE;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_BACKSPACE received\n\n");
                        break;
                case MHL_RCP_CMD_SOUND_SELECT:
                        keyCode	= KEY_SOUND;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_SOUND  received\n\n");
                        break;
                case MHL_RCP_CMD_PLAY:
                        keyCode	= KEY_PLAY;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_PLAY received\n\n");
                        break;
                case MHL_RCP_CMD_PAUSE:
                        keyCode	= KEY_PLAYPAUSE;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_PLAYPAUSE received\n\n");
                        break;
                case MHL_RCP_CMD_STOP:
                        keyCode	= KEY_STOP;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_STOP received\n\n");
                        break;
                case MHL_RCP_CMD_FAST_FWD:
                        keyCode	= KEY_FASTFORWARD;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_FASTFORWARD received\n\n");
                        break;
                case MHL_RCP_CMD_REWIND:
                        keyCode	= KEY_REWIND;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_REWIND received\n\n");
                        break;
                case MHL_RCP_CMD_EJECT:
                        keyCode	= KEY_EJECTCD;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_EJECTCD received\n\n");
                        break;
                case MHL_RCP_CMD_FWD:
                        keyCode	= KEY_FORWARD;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_FORWARD	received\n\n");
                        break;
                case MHL_RCP_CMD_BKWD:
                        keyCode	= KEY_PREVIOUSSONG;//KEY_BACK
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_BACK	 received\n\n");
                        break;
                case MHL_RCP_CMD_PLAY_FUNC:
                        keyCode	= KEY_PLAYCD;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_PLAYCD  received\n\n");
                        break;
                case MHL_RCP_CMD_PAUSE_PLAY_FUNC:
                        keyCode	= KEY_PAUSECD;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_PAUSECD  received\n\n");
                        break;
                case MHL_RCP_CMD_STOP_FUNC:
                        keyCode	= KEY_STOP;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_STOP  received\n\n");
                        break;
                case MHL_RCP_CMD_F1:
                        keyCode	= KEY_F1;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_F1 received\n\n");
                        break;
                case MHL_RCP_CMD_F2:
                        keyCode	= KEY_F2;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_F2 received\n\n");
                        break;
                case MHL_RCP_CMD_F3:
                        keyCode	= KEY_F3;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_F3 received\n\n");
                        break;
                case MHL_RCP_CMD_F4:
                        keyCode	= KEY_F4;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_F4 received\n\n");
                        break;
                case MHL_RCP_CMD_F5:
                        keyCode	= KEY_F5;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_F5 received\n\n");
                        break;
                case	MHL_RCP_CMD_CH_UP:
                        keyCode = KEY_CHANNELUP;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_CHANNELUP received\n\n");
                        break;
                case	MHL_RCP_CMD_CH_DOWN:
                        keyCode = KEY_CHANNELDOWN;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_CHANNELDOWN received\n\n");
                        break;
                case	MHL_RCP_CMD_PRE_CH:
                        keyCode = KEY_PREVIOUSSONG;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_PREVIOUS received\n\n");
                        break;
                case	MHL_RCP_CMD_MUTE:
                        keyCode = KEY_MUTE;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_MUTE	received\n\n");
                        break;
                case	MHL_RCP_CMD_VOL_UP:
                        keyCode = KEY_VOLUMEUP;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_VOLUMEUP received\n\n");
                        break;
                case	MHL_RCP_CMD_VOL_DOWN:
                        keyCode = KEY_VOLUMEDOWN;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_VOLUMEDOWN received\n\n");
                        break;
                case	MHL_RCP_CMD_RECORD:
                        keyCode = KEY_RECORD;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_RECORD received\n\n");
                        break;
                case	MHL_RCP_CMD_RECORD_FUNC:
                        keyCode = KEY_RECORD;
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_RECORD received\n\n");
                        break;
                default:
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\nMhlTx:KEY_Unknow  received\n\n");
                        break;
                }

                ReportKeyCode(keyCode); /* report the key to App */
                /* Send the ACK to the peer device(TV), That the key was processed  */
                if(HalAcquireIsrLock() != HAL_RET_SUCCESS)
                {
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_APP_TRACE,"\n Acquire ISR Lock failed\n");
                        return MHL_TX_EVENT_STATUS_PASSTHROUGH;
                }

                while(gDriverContext.flags & MHL_STATE_FLAG_RCP_READY)
                {

                        if( !(gDriverContext.flags & MHL_STATE_FLAG_RCP_RECEIVED))
                        {

                                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                                "Attempting to ACK a key code that was not received!\n");
                                break;
                        }

                        SiiMhlTxRcpkSend(gDriverContext.keyCode);

                        break;
                }
                HalReleaseIsrLock();
                rtnStatus = MHL_TX_EVENT_STATUS_HANDLED;
                break;

        case MHL_TX_EVENT_RCPK_RECEIVED:
                if((gDriverContext.flags & MHL_STATE_FLAG_RCP_SENT)
                                && (gDriverContext.keyCode == eventParam))
                {

                        gDriverContext.flags |= MHL_STATE_FLAG_RCP_ACK;

                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                        "Generating RCPK received event, keycode: 0x%02x\n",
                                        eventParam);
                        snprintf(event_string, MAX_EVENT_STRING_LEN,
                                 "MHLEVENT=received_RCPK key code=0x%02x", eventParam);
                        kobject_uevent_env(&gDriverContext.pDevice->kobj,
                                           KOBJ_CHANGE, envp);
                }
                else
                {
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                        "Ignoring unexpected RCPK received event, keycode: 0x%02x\n",
                                        eventParam);
                }
                break;

        case MHL_TX_EVENT_RCPE_RECEIVED:
                if(gDriverContext.flags & MHL_STATE_FLAG_RCP_SENT)
                {

                        gDriverContext.errCode = eventParam;
                        gDriverContext.flags |= MHL_STATE_FLAG_RCP_NAK;

                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                        "Generating RCPE received event, error code: 0x%02x\n",
                                        eventParam);
                        snprintf(event_string, MAX_EVENT_STRING_LEN,
                                 "MHLEVENT=received_RCPE error code=0x%02x", eventParam);
                        kobject_uevent_env(&gDriverContext.pDevice->kobj,
                                           KOBJ_CHANGE, envp);
                }
                else
                {
                        SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                        "Ignoring unexpected RCPE received event, error code: 0x%02x\n",
                                        eventParam);
                }
                break;

        case MHL_TX_EVENT_DCAP_CHG:
                snprintf(event_string, MAX_EVENT_STRING_LEN,
                         "MHLEVENT=DEVCAP change");
                kobject_uevent_env(&gDriverContext.pDevice->kobj,
                                   KOBJ_CHANGE, envp);

                break;

        case MHL_TX_EVENT_DSCR_CHG:	// Scratch Pad registers have changed
                snprintf(event_string, MAX_EVENT_STRING_LEN,
                         "MHLEVENT=SCRATCHPAD change");
                kobject_uevent_env(&gDriverContext.pDevice->kobj,
                                   KOBJ_CHANGE, envp);
                break;

        case MHL_TX_EVENT_POW_BIT_CHG:	// Peer's power capability has changed
                if (eventParam)
                {
#ifdef BYPASS_VBUS_HW_SUPPORT //(
                        // Since downstream device is supplying VBUS power we should
                        // turn off our VBUS power here.  If the platform application
                        // can control VBUS power it should turn off it's VBUS power
                        // now and return status of MHL_TX_EVENT_STATUS_HANDLED.  If
                        // platform cannot control VBUS power it should return
                        // MHL_TX_EVENT_STATUS_PASSTHROUGH to allow the MHL layer to
                        // try to turn it off.
                        rtnStatus = MHL_TX_EVENT_STATUS_HANDLED;
#else //)(
                        // In this sample driver all that is done is to report an
                        // event describing the requested state of VBUS power and
                        // return MHL_TX_EVENT_STATUS_PASSTHROUGH to allow lower driver
                        // layers to control VBUS power if possible.
#endif //)
                        snprintf(event_string, MAX_EVENT_STRING_LEN,
                                 "MHLEVENT=MHL VBUS power OFF");
                }
                else
                {
                        snprintf(event_string, MAX_EVENT_STRING_LEN,
                                 "MHLEVENT=MHL VBUS power ON");
#ifdef BYPASS_VBUS_HW_SUPPORT //(
                        rtnStatus = MHL_TX_EVENT_STATUS_HANDLED;
#else //)(
#endif //)
                }

                kobject_uevent_env(&gDriverContext.pDevice->kobj,
                                   KOBJ_CHANGE, envp);
                break;
        case MHL_TX_EVENT_RGND_MHL:
#ifdef SHARE_SWITCHER_WITH_USB
                HalSwitcherSwitchToMhl(true);
#endif
#ifdef BYPASS_VBUS_HW_SUPPORT //(
                // RGND measurement has determine that the peer is an MHL device.
                // If platform application can determine that the attached device
                // is not supplying VBUS power it should turn on it's VBUS power
                // here and return MHL_TX_EVENT_STATUS_HANDLED to indicate to
                // indicate to the caller that it handled the notification.
                rtnStatus = MHL_TX_EVENT_STATUS_HANDLED;
#else //)(
                // In this sample driver all that is done is to report the event
                // and return MHL_TX_EVENT_STATUS_PASSTHROUGH to allow lower driver
                // layers to control VBUS power if possible.
#endif //)
                snprintf(event_string, MAX_EVENT_STRING_LEN,
                         "MHLEVENT=MHL device detected");
                kobject_uevent_env(&gDriverContext.pDevice->kobj,
                                   KOBJ_CHANGE, envp);
                break;
        case MHL_TX_EVENT_TMDS_ENABLED:
                break;
        case MHL_TX_EVENT_TMDS_DISABLED:
                break;

        default:
                SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
                                "AppNotifyEvent called with unrecognized event code!\n");
        }
        return rtnStatus;
}

/*****************************************************************************/
/**
 * @brief Handler for MHL transmitter reset requests.
 *
 * This function is called by the MHL layer to request that the MHL transmitter
 * chip be reset.  Since the MHL layer is platform agnostic and therefore doesn't
 * know how to control the transmitter's reset pin each platform application is
 * required to implement this function to perform the requested reset operation.
 *
 * @param[in]	hwResetPeriod	Time in ms. that the reset pin is to be asserted.
 * @param[in]	hwResetDelay	Time in ms. to wait after reset pin is released.
 *
 *****************************************************************************/
void AppResetMhlTx(uint16_t hwResetPeriod,uint16_t hwResetDelay)
{

        // Reset the TX chip,
        HalGpioSetTxResetPin(LOW);
        HalTimerWait(hwResetPeriod);
        HalGpioSetTxResetPin(HIGH);

        HalTimerWait(hwResetDelay);
}

/**
 *  File operations supported by the MHL driver
 */
static const struct file_operations siiMhlFops =
{
        .owner			= THIS_MODULE,
        .open			= SiiMhlOpen,
        .release		= SiiMhlRelease,
        .unlocked_ioctl	= SiiMhlIoctl
};


/*
 * Sysfs attribute files supported by this driver.
 */
struct device_attribute driver_attribs[] =
{
        __ATTR(connection_state, 0444, ShowConnectionState, NULL),
        __ATTR(rcp_keycode, 0664, ShowRcp, SendRcp),
        __ATTR(rcp_ack, 0664, ShowRcpAck, SendRcpAck),
        __ATTR(devcap, 0664, ReadDevCap, SelectDevCap),
        __ATTR(log_switch, 0664, LogSwitchShow, LogSwitchStore),
        __ATTR_NULL
};



static int __init SiiMhlInit(void)
{
        int32_t	i;
        int32_t	ret;
        dev_t	devno;
        struct input_dev  *pInputDevice = NULL;
        char mhl_chip_name[15];
        bool get_xml_ret =0;

        get_xml_ret = get_hw_config_string("MHL/chip_name",mhl_chip_name,15,NULL);
        if(get_xml_ret)
        {
		pr_info("get_xml_ret = true\n");
                if(strstr(mhl_chip_name,"none"))
                {
                        pr_info("sii8240 doesn't support on this production\n");
                        return 0;
                }
		else
		{
			pr_info("support MHL,mhl_chip_name = %s\n",mhl_chip_name);
		}
        }
	else
	{
		pr_info("get_xml_ret = false,Default support MHL  on this production\n");
	}

        pr_info("%s driver starting!\n", MHL_DRIVER_NAME);
        pr_info("Version: %s%d\n", buildVersion,BUILDNUM);
        pr_info("%s", buildTime);

        /* register chrdev */
        pr_info("register_chrdev %s\n", MHL_DRIVER_NAME);

        /* If a major device number has already been selected use it,
         * otherwise dynamically allocate one.
         */
        if (devMajor)
        {
                devno = MKDEV(devMajor, 0);
                ret = register_chrdev_region(devno, MHL_DRIVER_MINOR_MAX,
                                             MHL_DRIVER_NAME);
        }
        else
        {
                ret = alloc_chrdev_region(&devno,
                                          0, MHL_DRIVER_MINOR_MAX,
                                          MHL_DRIVER_NAME);
                devMajor = MAJOR(devno);
        }
        if (ret)
        {
                pr_info("register_chrdev %d, %s failed, error code: %d\n",
                        devMajor, MHL_DRIVER_NAME, ret);
                return ret;
        }

        cdev_init(&siiMhlCdev, &siiMhlFops);
        siiMhlCdev.owner = THIS_MODULE;
        ret = cdev_add(&siiMhlCdev, devno, MHL_DRIVER_MINOR_MAX);
        if (ret)
        {
                pr_info("cdev_add %s failed %d\n", MHL_DRIVER_NAME, ret);
                goto free_chrdev;
        }

        siiMhlClass = class_create(THIS_MODULE, "mhl");
        if (IS_ERR(siiMhlClass))
        {
                pr_info("class_create failed %d\n", ret);
                ret = PTR_ERR(siiMhlClass);
                goto free_cdev;
        }

        siiMhlClass->dev_attrs = driver_attribs;

        gDriverContext.pDevice  = device_create(siiMhlClass, NULL,
                                                MKDEV(devMajor, 0),  NULL,
                                                "%s", MHL_DEVICE_NAME);
        if (IS_ERR(gDriverContext.pDevice))
        {
                pr_info("class_device_create failed %s %d\n", MHL_DEVICE_NAME, ret);
                ret = PTR_ERR(gDriverContext.pDevice);
                goto free_class;
        }
        pInputDevice = input_allocate_device();
        if (!pInputDevice)
        {
                ret = -ENOMEM;
                pr_info("insufficient memory to alloc input device %s %d\n", MHL_DEVICE_NAME,ret);
                goto free_class;
        }

        gDriverContext.pRcpDevice = pInputDevice;
        //dev_set_drvdata(&pInputDevice->dev, );
        pInputDevice->name = MHL_DRIVER_NAME;
        pInputDevice->id.bustype = BUS_I2C;

        pInputDevice->keycode = gDriverContext.RcpKeyCode;
        pInputDevice->keycodesize = sizeof(gDriverContext.RcpKeyCode[0]);
        pInputDevice->keycodemax = ARRAY_SIZE(gDriverContext.RcpKeyCode);

        __set_bit(EV_KEY, pInputDevice->evbit);
        __clear_bit(EV_REP, pInputDevice->evbit);

        for (i = 0; i < ARRAY_SIZE(gDriverContext.RcpKeyCode); i++)
        {
                gDriverContext.RcpKeyCode[i] = rcpKeyCode[i];
                input_set_capability(pInputDevice, EV_KEY, gDriverContext.RcpKeyCode[i]);
        }
        __clear_bit(KEY_RESERVED, pInputDevice->keybit);

        ret = input_register_device(gDriverContext.pRcpDevice);
        if(ret)
        {
                pr_info("register input device failed %s %d\n", MHL_DEVICE_NAME,ret);
                goto free_inputdev;
        }
        ret = StartMhlTxDevice();
        if(ret == 0)
        {
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
                /* detect current device successful, set the flag as present */
                set_hw_dev_flag(DEV_I2C_MHL);
#endif
                return 0;

        }
        else
        {
                // Transmitter startup failed so fail the driver load.
                device_destroy(siiMhlClass, MKDEV(devMajor, 0));
        }


free_inputdev:
        input_free_device(gDriverContext.pRcpDevice);

free_class:
        class_destroy(siiMhlClass);

free_cdev:
        cdev_del(&siiMhlCdev);

free_chrdev:
        unregister_chrdev_region(MKDEV(devMajor, 0), MHL_DRIVER_MINOR_MAX);

        return ret;
}



static void __exit SiiMhlExit(void)
{
        pr_info("%s driver exiting!\n", MHL_DRIVER_NAME);

        StopMhlTxDevice();

        device_destroy(siiMhlClass, MKDEV(devMajor, 0));
        class_destroy(siiMhlClass);
        unregister_chrdev_region(MKDEV(devMajor, 0), MHL_DRIVER_MINOR_MAX);
}

late_initcall(SiiMhlInit);
module_exit(SiiMhlExit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Silicon Image <http://www.siliconimage.com>");
MODULE_DESCRIPTION(MHL_DRIVER_DESC);
