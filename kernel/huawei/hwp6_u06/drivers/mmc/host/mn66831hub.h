/*==============================================================================*/
/** @file      mn66831hub.h
 *  @brief     common Header file for Local Information.
 
               This program is free software; you can redistribute it and/or modify
               it under the terms of the GNU General Public License version 2 as
               published by the Free Software Foundation.

 *  @author    Copyright (C) 2011 Panasonic Corporation Semiconductor Company
 *  @attention Thing that information on SD Specification is not programmed in
               function called from this file.
 */
/*==============================================================================*/
#ifndef		_MN66831HUB_H_
#define		_MN66831HUB_H_


/* Configuration */
/** 
	Print level for Debug.
	default:2
	0:No print.
	1:print only error no.
	2:print warrning information.
    3:print debug information.
    4:print read/write buffer information.
    5:print register access.
 */
#define SDHUB_DEBUG_PRINT_LEVEL 1 /**< Debug Print Level */

/*
 * include 
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/highmem.h>
#include <linux/log2.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/amba/bus.h>
#include <linux/clk.h>
#include <linux/scatterlist.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/dma-mapping.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>

#include <asm/cacheflush.h>
#include <asm/div64.h>
#include <asm/io.h>
#include <asm/sizes.h>
//#include <asm/semaphore.h>
#include <asm/atomic.h>

/** Error Code */
#define SDD_SUCCESS						0
#define SDD_ERR_PARAM					-EINVAL
#define SDD_ERR_NO_CARD					-ENOMEDIUM
#define SDD_ERR_IP_TIMEOUT				-EIO

typedef signed char B;
typedef signed short H;
typedef signed long W;
typedef unsigned char UB;
typedef unsigned short UH;
typedef unsigned long UW;
typedef signed long ID;
typedef unsigned char BOOL;
typedef unsigned long long UDW;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/** Bus Width */
#define		SDHUB_BUS_WIDTH_1BIT 1   /**> Bus Width(1bit) */
#define		SDHUB_BUS_WIDTH_4BIT 4   /**> Bus Width(4bit) */
#define		SDHUB_BUS_WIDTH_8BIT 8   /**> Bus Width(8bit) */

/** Voltage level */
#define		SDHUB_VOLTAGE_0V     0 /**> 0V   */
#define		SDHUB_VOLTAGE_1_8V   1 /**> 1.8V */
#define		SDHUB_VOLTAGE_3_3V   2 /**> 3.3V */

/** I/O Timing */
#define		SDHUB_IO_NORMAL_TIMING   0 /**> Normal Timing */
#define		SDHUB_IO_MMC_HS_TIMING   1 /**> Timing for MMC HighSpeed   */
#define		SDHUB_IO_SD_HS_TIMING    2 /**> Timing for SD HighSpeed   */

/** reset target */
#define		SDHUB_SOFT_RESET_SDIF    1 /**> SDIF reset */

/* MN66831 Command ID */
#define		SDHUB_CMD_SDSD_RESET				(60)
#define		SDHUB_CMD_SET_SDSD_PARAM			(61)
#define		SDHUB_CMD_SEND_SDSD_STATUS			(59)

#define		SDHUB_BCMD_WRITE_SINGLE_REG			(30)
#define		SDHUB_BCMD_READ_SINGLE_REG			(31)

#define		SDHUB_BCMD_SD_PATH_SET_SLOT			(40)
#define		SDHUB_BCMD_SD_PATH_SET_PARAM		(41)
#define		SDHUB_BCMD_SD_PATH_EXECUTE			(42)
#define		SDHUB_BCMD_SD_PATH_SEND_RESPONSE	(43)

#define		SDHUB_BCMD_STOP_TRANSMISSION		(12)
#define		SDHUB_BCMD_READ_MULTIPLE_BLOCK		(18)
#define		SDHUB_BCMD_WRITE_MULTIPLE_BLOCK		(25)

#define		SDHUB_SIZE_SD_PATH_SET_PARAM		(16)
#define		SDHUB_SIZE_SD_PATH_SEND_RESPONSE	(32)

#define		SDHUB_SIZE_CID						(16)
#define		SDHUB_SIZE_CSD						(16)
#define		SDHUB_SIZE_RCA						( 2)

#define		SDHUB_STUFF_BITS					(0x00000000)

/** Value for SDD */
#define		SDHUB_SECTOR_SIZE 512  		    /**> Sector Size */

#define		DRIVER_NAME						"SDHUBDEVICE"

struct mn66831_package ;

/** Pratform Configuration Data by slot */
struct mn66831_pdata_slot {
    unsigned int mclk;         /**< Master Clock for Host*/
    unsigned int maxclk;       /**< Max Clock for Host (If maxclk = mclk/1div then maxclk is set value as mclk) */
    unsigned int bus_width;    /**< Bus Width(1,4,8) */
    unsigned int maxDMALength; /**< Max Transfer length for DMA(H/W) */
    unsigned int maxBufSize;   /**< Max memorySize for Transfer(Unit of Sector) */
    unsigned int registerUnitSize;      /**< Unit size for register*/
    char *chThisDevice;       /**< This Device Name. */
    char *chMasterDevice;     /**< Master Device Name */
    bool enableStandby;       /**< TRUE: this slot can be change stand-by mode */
    bool enableSuspend;       /**< TRUE: this slot can be change suspend mode */
    bool enableHighSpeedMode; /**< TRUE: this slot can be change High Speed Transfer mode(50MHz) */
    bool enableUHS1Mode;      /**< TRUE: this slot can be change UHS-1 Transfer mode(100MHz) */
    bool enableDMA;           /**< TRUE: This slot can transfer by dma. */
    bool enablePowerSave;     /**< TRUE: This slot can change power save mode. */
    bool enableLowVolRange;   /**< TRUE: This slot can change 1.8V. */
    bool enable0VolRange;     /**< TRUE: This slot can change 0V. */
    bool enableWp;            /**< TRUE: This slot has WP signal port. */
    bool enableDetectCard;    /**< TRUE: This slot has detect Card signal port. */
    bool enableHub;           /**< TRUE: This slot has SD-Hub. */
	bool enableSdPathAutoCmd55;
	bool enableSdPathAutoCmd12;
	bool enableSdPathAutoCmd13;
};

/** Pratform Configuration Data */
struct mn66831_pdata_host {
  const char 			*name ;				/**< Name  */
  unsigned int 			id ;				/**< Identifier  */
  struct mn66831_pdata_slot	*slot;      		/**< Platform data for Slot */
  unsigned int			nr_parts;			/**< Number of  */
};

/** Data for Upper Driver(Host) */
struct mn66831_host {
	struct platform_device	*pdev;         /**< Configuration Driver Data for SDD */
	struct mn66831_package  *package ;             /**< Package Data */
    struct mn66831_slot     **slot;                /**< Slot Data for SDD */
	unsigned int		num_slots;             /**< Number of Slots */
	unsigned int		power_control ;        /**< Power Control Data */
};

/** Data for Upper Driver(Slot) */
struct mn66831_slot {
	struct mmc_host	*upHost;           /**< Information for Upper Driver */
    struct mn66831_host     *host;             /**< Information for upper host(Host) */
    struct mmc_request *mrq;             /**< Request for upper host(slot) */
    struct mmc_ios   *ioctl;           /**< Ioctl for upper host(slot) */
	struct mmc_ios   ios ;					/**< Current ios data */
    bool blMedia;                          /**< Media status (true:In, false No media) */
    bool appcmd;                           /**< Send CMD55(True:Sended, false:No send) */
    unsigned char  resp[16];               /**< response data */
    struct mn66831_pdata_slot *pdata;          /**< slot configuration data */
    char *chThisDevice;                    /**< This Device Name. */
    char *chMasterDevice;                  /**< Master Device Name. */
    unsigned int mclk;                     /**< Master Clock for Host*/
    unsigned int maxclk;                   /**< Max Clock for Host (If maxclk = mclk/1div then maxclk is set value as mclk) */
    unsigned int clock;                    /**< closk setting */
    unsigned int 		maxDMALength;      /**< Max Transfer length for DMA(H/W) */
    unsigned int 		maxBufSize;        /**< Max memorySize for Transfer(Unit of Sector) */
    unsigned int 		registerUnitSize;  /**< Unit size for register*/
    int slotNo;                            /**< This Slot No */
	
	spinlock_t			lock ;				/**< for Re-Initialize */
	atomic_t			sync ;				/**< for Re-Initialize */
	wait_queue_head_t	wq ;				/**< for Re-Initialize */
};


/** Data structure for LSI package information */
struct mn66831_package {
	unsigned int		package_id ;		/**< Identifier of LSI package */
	unsigned int		profile_id ;		/**< Identifier of profile */
	unsigned int 		num_devices ;		/**< Number of devices in package */
	unsigned int		count_device ;		/**< Number of registerd devices  */
	unsigned int		power_control ;		/**< Flag of power control */
	struct mmc_host		*master_host ;		/**< Pointer to mmc_host which is in master mmc driver */
	unsigned char		bcmd41buf[SDHUB_SIZE_SD_PATH_SET_PARAM] ;	/**< Data Buffer for SD_PATH_SET_PARAM */
	unsigned char		bcmd43buf[SDHUB_SIZE_SD_PATH_SEND_RESPONSE] ; /**< Data Buffer for SD_PATH_SEND_RESPONSE */
	
	/* for SD Hub I/F setting */
	struct {
		bool			enablePullUp ;		/**<  */
		unsigned char	setIntMode ;		/**<  */
		unsigned char	setSdMode ;			/**<  */
		unsigned char	setSdWidth ;		/**<  */
	} sdsd_param ;							
	bool				flag_connection ;	/**<  */
	
	struct mn66831_host		**device_info ;		/**< Pointer to array of mn66831_host  */
} ;


/** Data structure for a set of platform_device */
struct mn66831_platform_device {
	struct platform_device **conf_data ;	/**< Pointer to array of platform devices */
	unsigned int nr_devices ;				/**< Number of platform devices in mn66831 package */
};


#define		MN66831_PROFILE_ID			(1)
#define		MN66831_NUM_OF_DEVICES		(2)


/*--------------------------------------*/
/*
 *  FOR DEBUG (DO NOT CHANGE)
 */
/*--------------------------------------*/

/* 
 * If you want to get register value, you set this setting.
 * Program print out register value in this setting.
 */
#define SDHUB_PREGISTER 5

/* 
 * If you want to get read/write buffer data, you set this setting.
 * Program print out buffer data in this setting.
 */
#define SDHUB_PBUFDEBUG 4

/* 
 * If you want to get debug information, you set this setting.
 * Program print out debug information in this setting.
 */
#define SDHUB_PDEBUG 3

/* 
 * If you want to get warnning data, you set this setting.
 * Program print out warnning data by Sdhubs_SetErr function in this setting.
 */
#define SDHUB_PWARN 2

/* 
 * If you want to get only api error no, you set this setting.
 * Program print out only api error no in this setting.
 */
#define SDHUB_PERR 1


/*
 * Functions
 */
#define Sdhubs_DebugPrint(iLine, iLevel, fmt, args...) {\
if (iLevel <= SDHUB_DEBUG_PRINT_LEVEL) {\
printk(KERN_ALERT "mn66831:%d " fmt, iLine, ## args);\
}\
}
#define Sdhubs_DebugBufPrint(iLine, iLevel, buf, num) {\
		int i;										 \
        if (iLevel <= SDHUB_DEBUG_PRINT_LEVEL) {     \
            for(i=0; i<num; i++) {					 \
                if (i!=0 && i%16==0) {				 \
                    printk("\n");		             \
                }									 \
                printk("%02X ", (unsigned char)buf[i]);	         \
            }										 \
            printk("\n");                            \
        }											 \
    }


#endif	/*	_MN66831HUB_H_	*/
