/*==============================================================================*/
/** @file      mn66831hub.c
 *  @brief     Send Command module for MMC Driver for Linux
               This program corresponds to Panasonic SD HostIP and GPL MMC driver.
               
               This program is free software; you can redistribute it and/or modify
               it under the terms of the GNU General Public License version 2 as
               published by the Free Software Foundation.

 *  @author    Copyright (C) 2011 Panasonic Corporation Semiconductor Company
 *  @attention Thing that information on SD Specification is not programmed in
               function called from this file.
 */
/*==============================================================================*/
//Add for 2.6.35
#include <linux/slab.h>
#include <linux/time.h>

#include "mn66831hub.h"
//#include "emxx_sdc.h"
#include <linux/mmc/mn66831hub_api.h>

/* CONFIG : Enable SD-Hub Driver Extension */
#define		_SDHUB_EXTENSION_
#if			defined(_SDHUB_EXTENSION_)
#include <linux/mmc/mn66831hub_ext.h>
#endif	/*	defined(_SDHUB_EXTENSION_)	*/

#include <linux/mmc/mshci.h>
#include <mach/gpio.h>

/* CONFIG : MMC Core Version */
//#define		_SDHUB_OLD_MMC_SUSPEND_HOST_	/* DISABLE : If mmc core in kernel 2.6.35 or later. */

/* CONFIG : MMC Core Version */
//#define		_SDHUB_OLD_SEGS_				/* DISABLE : If mmc core in kernel 2.6.37 or later. */

/* CONFIG : Enable UHS-I */
#define		_SDHUB_ENABLE_UHS_I_			/* ENABLE : If mmc core in kernel 3.0.1 or later. */

/* CONFIG : Use SD-Hub I/F --- UHS-I(SDR50) */
//#define		_SDHUB_USE_HUBIF_SDR50_		/* ENABLE : If system uses UHS-I mode on SD-Hub I/F */

/* CONFIG : Use SD-Hub I/F --- 8-bit Bus */
//#define		_SDHUB_USE_HUBIF_8BIT_		/* ENABLE : If system uses 8-bit bus on SD-Hub I/F */

//#define SDHUB_DEBUG_SD_COMMAND_LOG

static	struct mn66831_package *mn66831_package_list[SDHUB_PACKAGE_COUNT] ;
#define	MN66831_GET_PACKAGE(id,package)	{ package = mn66831_package_list[id] ; }
#define	MN66831_SET_PACKAGE(id,package)	{ mn66831_package_list[id] = package ; }

#define		MN66831_CMD_RETRIES 		(0) /* max count of command retry */

#define		MN66831_MMC_INFO1			(0x001C)
#define		MN66831_MMC_INFO2			(0x001E)
#define		MN66831_MMC_INFO1_MASK		(0x0020)
#define		MN66831_MMC_INFO2_MASK		(0x0022)
#define		MN66831_MMC_CLK_CTRL		(0x0024)
#define		MN66831_MMC_OPTION			(0x0028)
#define		MN66831_MMC_IFMODE			(0x00DA)
#define		MN66831_MMC_SOFT_RST		(0x00E0)
#define		MN66831_MMC_EXT_MODE		(0x00F0)
#define		MN66831_MMC_PULLUP			(0x00F8)

#define		MN66831_SLOT_CTRL			(0x039A)
#define		MN66831_STBY_CTRL			(0x039C)
#define		MN66831_SD2SD_IFMODE		(0x03C4)
#define		MN66831_SUSPEND_CTRL		(0x03F4)
#define		MN66831_CLK_CTRL			(0x03F6)

#if			defined(_SDHUB_ENABLE_UHS_I_)
#define		MN66831_UHS1_REGSEL			(0x0748)
#define		MN66831_UHS1_REGSEL_MASK	(0x074A)
#define		MN66831_UHS1_REG_COUNT0		(0x074C)
#define		MN66831_UHS1_REG_COUNT1		(0x074E)
#define		MN66831_UHS1_REG_COUNT2		(0x0750)
#endif	/*	defined(_SDHUB_ENABLE_UHS_I_)	*/

/* Huawei k3v2 resource */
#define CLK_CON_ID	        "clk_clockout0"
#define	SDHUB_DEVICE_ID		(SDHUB_DEVID0)
#define CPRM_BLOCK_NAME     "block_cprm"
#define GPIO_VDDIO_EN       GPIO_8_7
#define GPIO_RESET          GPIO_17_0
#define GPIO_IRQ            GPIO_16_1
static struct clk       *pclk         = NULL;
static struct regulator *vddio_vddios = NULL;
static struct regulator *avdd         = NULL;
static struct iomux_block   *cprm_iomux_block;
static struct block_config  *cprm_iomux_config;
static int vddio_status = 0;


#define MMC_INFO2_RETRY_MAX_TIME   100

void mn66831_cmd_write_single_reg( struct mn66831_slot *slot_info, volatile unsigned int *address, unsigned int value, bool verify ) ;
void mn66831_cmd_read_single_reg( struct mn66831_slot *slot_info, volatile unsigned int *address, unsigned int *value ) ;
static void mn66831_ctrl_soft_reset( struct mn66831_slot *slot_info, unsigned int target );
static void mn66831_reg_setdata( struct mn66831_slot *slot_info, unsigned int offset, unsigned int regdata );
static void mn66831_reg_getdata( struct mn66831_slot *slot_info, unsigned int offset, unsigned int *regdata );

/*==============================================================================*/
/*  Function Name   void mn66831_delay(UW uw_Wait)  */
/*------------------------------------------------------------------------------*/
/** @brief      Wait function
 *  @param[in]  *uw_Wait    : wait time(msec)
 *  @return     void
 *  @attention  if uw_Wait smaller than 10ms , this function is busy wait.
*/ 
/*==============================================================================*/
static void mn66831_delay(UW uw_Wait) {
    if (uw_Wait<10) {
        mdelay(uw_Wait);
    }
    else {
        msleep(uw_Wait);
    }
	return ;
}
int clock_state = 0;

void SDHUB_clock_disable( void ) {

    if(clock_state == 1)
    {
        clk_disable(pclk);
        printk("SDHUB_clock_disable.\n");
    }

    clock_state = 0;
}
EXPORT_SYMBOL(SDHUB_clock_disable);


void SDHUB_clock_enable( void ) {
    
	int ret = 0 ;
    
    if(clock_state == 0)
    {
        ret = clk_enable(pclk);
        if(ret != 0)
            printk("SDHUB_clock_enable fatal error.\n");
        udelay(50);
        clock_state = 1;
        printk("SDHUB_clock_enable.\n");
    }
}
EXPORT_SYMBOL(SDHUB_clock_enable);




/*==============================================================================*/
/*  Function Name  void mn66831_master_set_clock( struct mmc_host *master_host, unsigned int clock ) */
/*------------------------------------------------------------------------------*/
/** @brief      Set SD-Hub bus clock to master mmc driver
 *  @param[in]  *master_host : information for mmc driver
 *  @param[in]  clock        : clock (Hz)
 *  @return     void
 *  @attention  
*/ 
/*==============================================================================*/
static void mn66831_master_set_clock( struct mmc_host *master_host, unsigned int clock ){
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:master_host->ios.clock = %d\n", __func__, master_host->ios.clock);
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:master_host->f_max = %d\n", __func__, master_host->f_max);
	if(clock>master_host->f_max){
		clock = master_host->f_max ;
	}
	master_host->ios.clock = clock ;
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:master_host->ios.clock = %d\n", __func__, master_host->ios.clock);
	master_host->ops->set_ios(master_host,&(master_host->ios));
	
	return ;
}

/*==============================================================================*/
/*  Function Name  void mn66831_master_set_bus_width( struct mmc_host *master_host, unsigned int width ) */
/*------------------------------------------------------------------------------*/
/** @brief      Set SD-Hub bus width to master mmc driver
 *  @param[in]  *master_host : information for mmc driver
 *  @param[in]  width        : bus width
 *  @return     void
 *  @attention  
*/ 
/*==============================================================================*/
static void mn66831_master_set_bus_width( struct mmc_host *master_host, unsigned int width ){
	master_host->ios.bus_width = width ;
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:master_host->ios.bus_width = %d\n", __func__, master_host->ios.bus_width);
	master_host->ops->set_ios(master_host,&(master_host->ios));
	
	return ;
}

/*==============================================================================*/
/*  Function Name  void mn66831_master_power_up( struct mmc_host *master_host ) */
/*------------------------------------------------------------------------------*/
/** @brief      Power up sequence for master mmc driver
 *  @param[in]  *master_host : information for mmc driver
 *  @return     void
 *  @attention  
*/ 
/*==============================================================================*/
static void mn66831_master_power_up( struct mmc_host *master_host ){
    int ret;
    
	master_host->ios.vdd = fls(master_host->ocr_avail) - 1 ;
	if(mmc_host_is_spi(master_host)) {
		master_host->ios.chip_select = MMC_CS_HIGH ;
		master_host->ios.bus_mode = MMC_BUSMODE_PUSHPULL ;
	}
	else {
		master_host->ios.chip_select = MMC_CS_DONTCARE ;
		master_host->ios.bus_mode = MMC_BUSMODE_OPENDRAIN ;
	}
	master_host->ios.power_mode = MMC_POWER_UP ;
	master_host->ios.bus_width = MMC_BUS_WIDTH_1 ;
	master_host->ios.timing = MMC_TIMING_LEGACY ;
//	master_host->ios.timing = MMC_TIMING_MMC_HS ;
//	master_host->ios.timing = MMC_TIMING_SD_HS ;
	master_host->ops->set_ios(master_host,&(master_host->ios));
	mn66831_delay(10);
	
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:master_host->ios.clock = %d\n", __func__, master_host->ios.clock);
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:master_host->f_max = %d\n", __func__, master_host->f_max);
	if( master_host->f_max > 25000000 ) {
		master_host->ios.clock = 25000000 ;
	}
	else{
		master_host->ios.clock = master_host->f_max ;
	}
	master_host->ios.power_mode = MMC_POWER_ON ;
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:master_host->ios.clock = %d\n", __func__, master_host->ios.clock);
	master_host->ops->set_ios(master_host,&(master_host->ios));
	mn66831_delay(10);

    
	return ;
}

/*==============================================================================*/
/*  Function Name  void mn66831_master_power_off( struct mmc_host *master_host ) */
/*------------------------------------------------------------------------------*/
/** @brief      Power off sequence for master mmc driver
 *  @param[in]  *master_host : information for mmc driver
 *  @return     void
 *  @attention  
*/ 
/*==============================================================================*/
static void mn66831_master_power_off( struct mmc_host *master_host ){
    
	master_host->ios.clock = 0;
	master_host->ios.vdd = 0;
	if(!mmc_host_is_spi(master_host)) {
		master_host->ios.bus_mode = MMC_BUSMODE_OPENDRAIN ;
		master_host->ios.chip_select = MMC_CS_DONTCARE ;
	}
	master_host->ios.power_mode = MMC_POWER_OFF ;
	master_host->ios.bus_width = MMC_BUS_WIDTH_1 ;
	master_host->ios.timing = MMC_TIMING_LEGACY ;
	master_host->ops->set_ios(master_host,&(master_host->ios));
	
	return ;
}

#if			defined(_SDHUB_ENABLE_UHS_I_)
/*==============================================================================*/
/*  Function Name  void mn66831_master_voltage_switch( struct mmc_host *master_host, unsigned int vol ) */
/*------------------------------------------------------------------------------*/
/** @brief      Power off sequence for master mmc driver
 *  @param[in]  *master_host : information for mmc driver
 *  @return     void
 *  @attention  
*/ 
/*==============================================================================*/
static void mn66831_master_voltage_switch( struct mmc_host *master_host, unsigned int vol ){
	
	if( vol == SDHUB_VOLTAGE_1_8V ) {
		master_host->ios.signal_voltage = MMC_SIGNAL_VOLTAGE_180 ;
	}
	else {
		master_host->ios.signal_voltage = MMC_SIGNAL_VOLTAGE_330 ;
	}
	if( master_host->ops->start_signal_voltage_switch ) {
		master_host->ops->start_signal_voltage_switch(master_host,&(master_host->ios));
	}
	mn66831_delay(1);
	
	return ;
}
#endif	/*	defined(_SDHUB_ENABLE_UHS_I_)	*/

#include "linux/mux.h"
static void mn66831_ctrl_host_power( struct mn66831_slot *slot_info, bool toggle ) 
{
	/* DUMMY FUNCTION */
	int ret = 0 ;
	struct iomux_pin *pin_temp = NULL;
	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s : toggle = %d\n", __func__, toggle );
    
    if (toggle)
    {
        vddio_status = 1;
        gpio_set_value(GPIO_VDDIO_EN, vddio_status);
        ret = regulator_enable(vddio_vddios);
    	if (ret) {
        	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s : regulator_enable() vddio_vddios Fail : ret = %d\n", __func__, ret );
        	return;
        }    
        ret = regulator_enable(avdd);
    	if (ret) {
        	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s : regulator_enable() avdd Fail : ret = %d\n", __func__, ret );
        	return;
        }    

        udelay(50);

        gpio_set_value(GPIO_RESET, 1);
        udelay(50);

        // set 26M INCLK
        SDHUB_clock_enable();
		ret = 0 ;
    	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:clk_enable start \n", __func__ );
        if (ret)
        {
        	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:clk_enable Fail \n", __func__ );
            return;
        }

    }
    else
    {
        SDHUB_clock_disable();

        gpio_set_value(GPIO_RESET, 0); // pull down XMNRST

        ret = regulator_disable(avdd);
    	if (ret) {
        	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s : regulator_disable() avdd Fail : ret = %d\n", __func__, ret );
        	return;
        }    
        ret = regulator_disable(vddio_vddios);
    	if (ret) {
        	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s : regulator_disable() vddio_vddios Fail : ret = %d\n", __func__, ret );
        	return;
        }    
        vddio_status = 0;
        gpio_set_value(GPIO_VDDIO_EN, vddio_status);
        mdelay(100);

    	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:All Power Supply Off\n", __func__ );
    }
}

static void mn66831_ctrl_card_power( struct mn66831_slot *slot_info, bool toggle ) {
	/* DUMMY FUNCTION */
}

#if			!defined(_SDHUB_ENABLE_UHS_I_)
static void mn66831_ctrl_card_voltage( struct mn66831_slot *slot_info, unsigned int vol ) {
	/* DUMMY FUNCTION */
	unsigned int regdata;

	mn66831_reg_ordata( slot_info, MN66831_UHS1_REGSEL, 0x0007 );
	mn66831_reg_getdata(slot_info, MN66831_UHS1_REGSEL, &regdata );
	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:MN66831_UHS1_REGSEL=0x%08x\n",__func__, regdata );
}
#endif	/*	!defined(_SDHUB_ENABLE_UHS_I_)	*/

static void mn66831_ctrl_card_timing( struct mn66831_slot *slot_info, unsigned int timing ) {
	/* DUMMY FUNCTION */
}

static bool mn66831_check_write_protect( struct mn66831_slot *slot_info ) {
	/* DUMMY FUNCTION : always returns false */
	return false ;	/* false means read/write enable */
}

/* dummy control option */
#define		MN66831_SLOT_A_DISABLE
#define		MN66831_SLOT_B_DISABLE
//#define		MN66831_SLOT_S_DISABLE
static bool mn66831_check_card( struct mn66831_slot *slot_info ) {
	bool card_exist = false ;
	
	/* DUMMY FUNCTION : always returns true */
	if( slot_info->host->pdev->id == 0 ) {
		if( slot_info->slotNo == 0 ) {
			/* MN66831 slot-A */
#ifndef		MN66831_SLOT_A_DISABLE
			card_exist = true ;
#endif	/*	MN66831_SLOT_A_DISABLE	*/
		}
		else if( slot_info->slotNo == 1 ){
			/* MN66831 slot-B */
#ifndef		MN66831_SLOT_B_DISABLE
			card_exist = true ;
#endif	/*	MN66831_SLOT_B_DISABLE	*/
		}
	}
	else if( slot_info->host->pdev->id == 1 ) {
		if( slot_info->slotNo == 0 ) {
			/* MN66831 slot-S */
#ifndef		MN66831_SLOT_S_DISABLE
			card_exist = true ;
#endif	/*	MN66831_SLOT_S_DISABLE	*/
		}
	}
	return card_exist ;
}

/*==============================================================================*/
/*  Function Name  unsigned int mn66831_reg_getbase( struct mn66831_slot *slot_info ) */
/*------------------------------------------------------------------------------*/
/** @brief      Get register offset parameter by slot information
 *  @param[in]  *slot     : slot information
 *  @return     register offset
 *  @attention  
*/ 
/*==============================================================================*/
static unsigned int mn66831_reg_getbase( struct mn66831_slot *slot_info ) {
	unsigned int regbase = 0x0000 ;
	if( slot_info->host->pdev->id == 1 ) {
		regbase = 0x0400 ;
	}
	return regbase ;
}

static unsigned int mn66831_reg_cnvaddr( struct mn66831_slot *slot_info, unsigned int offset ) {
	unsigned int regaddr = 0x0000 ;
	if( ( offset & 0x0700 ) == 0x0000 ) {
		regaddr = mn66831_reg_getbase( slot_info );
	}
	regaddr |= offset ;
	return regaddr ;
}

static void mn66831_reg_setdata( struct mn66831_slot *slot_info, unsigned int offset, unsigned int regdata ) {
	unsigned int regaddr ;
	
	regaddr = mn66831_reg_cnvaddr( slot_info, offset ) ;
	mn66831_cmd_write_single_reg(slot_info, (volatile unsigned int *)regaddr, regdata, true ) ;
}

static void mn66831_reg_getdata( struct mn66831_slot *slot_info, unsigned int offset, unsigned int *regdata ) {
	unsigned int regaddr ;
	
	regaddr = mn66831_reg_cnvaddr( slot_info, offset ) ;
	mn66831_cmd_read_single_reg(slot_info, (volatile unsigned int *)regaddr, regdata );
}

static void mn66831_reg_anddata( struct mn66831_slot *slot_info, unsigned int offset, unsigned int pattern ) {
	unsigned int regdata ;
	
	mn66831_reg_getdata( slot_info, offset, &regdata );
	regdata &= pattern ;
	mn66831_reg_setdata( slot_info, offset,  regdata );
}

static void mn66831_reg_ordata( struct mn66831_slot *slot_info, unsigned int offset, unsigned int pattern ) {
	unsigned int regdata ;
	
	mn66831_reg_getdata( slot_info, offset, &regdata ) ;
	regdata |= pattern ;
	mn66831_reg_setdata( slot_info, offset,  regdata );
}

static void mn66831_ctrl_card_clock(struct mn66831_slot *slot_info, bool toggle ) {
	if( toggle ) {
		mn66831_reg_ordata( slot_info, MN66831_MMC_CLK_CTRL, 0x0100 ) ;
	}
	else {
		mn66831_reg_anddata( slot_info, MN66831_MMC_CLK_CTRL, ~0x0100 ) ;
	}
}

static void mn66831_ctrl_clock_config(struct mn66831_slot *slot_info, unsigned int clock) {
	int i ;
	unsigned int div ;
	unsigned int regdata = 0 ;
	unsigned int clk_regdata = 0 ;
	unsigned int ifmode ;
	bool bl_clken = false ;
    struct mn66831_pdata_host *confdata = (struct mn66831_pdata_host *)slot_info->host->pdev->dev.platform_data;
	const unsigned int clock_div_param[9] = {
		0x0000,	0x0001,	0x0003,	0x0007,	0x0008,	0x0009,	0x000A,	0x000B,	0x000C
	} ;
    Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:clock=%u, slot_info->mclk=%u\n",__func__, clock, slot_info->mclk );    /* for debug */

	mn66831_reg_getdata(slot_info, MN66831_MMC_CLK_CTRL, &regdata );
    Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:MN66831_MMC_CLK_CTRL=0x%08x\n",__func__, regdata );    /* for debug */

    if ( 0 == clock) {
        Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:Stop Card Clock\n",__func__ );    /* for debug */
		mn66831_ctrl_card_clock( slot_info, false );
        return;
    }

	if( ( regdata & 0x0100 ) != 0x0000 ) {
		bl_clken = true ;
        Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:Stop Card Clock\n",__func__ );    /* for debug */
		mn66831_ctrl_card_clock( slot_info, false );
	}
	regdata &= ~0x0100 ;
	
	/* SD_CLK_CTRL.SD_CLK_DIV設定値は、mclkに対する分周比により決定 */
	regdata &= ~0x000F ;
	for(i = 1; i < 9; i++ ) {
		if( clock >= ( slot_info->mclk >> i ) ) {
			break ;
		}
	}
	div = 0x0001 << i ;
//	regdata |= clock_div_param[i-1] ;
    regdata |= 0x0000 ;
    
	/* SD_CLK_CTRL.SDCLKSEL設定値は、"Default Speed Mode" or "High Speed Mode"なら0設定 */
	if( clock > ( slot_info->mclk >> 1 ) ) {
//		regdata |= 0x0400 ;     
		regdata &= ~0x0400 ;        // 20121023
	}else{
//		regdata |= 0x0400 ;     
		regdata &= ~0x0400 ;
	}
	mn66831_reg_setdata(slot_info, MN66831_MMC_CLK_CTRL, regdata );
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:MN66831_MMC_CLK_CTRL=0x%08x\n",__func__, regdata );    /* for debug */

	/* SD_IFMODE.SDIF値は、メディア種別(normal/embedded, SD/MMC)、voltage、および、Speed Modeで決定 */
	mn66831_reg_getdata(slot_info, MN66831_MMC_IFMODE, &regdata );
    Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:MN66831_IFMODE=0x%08x\n",__func__, regdata ); /* for debug */	
    ifmode = 0x0000 ;	/* [SD,eSD/2.7-3.6V/Default Speed], [MMC,eMMC/2.7-3.6V/Backward-compatible] */
	if( confdata->slot[slot_info->slotNo].enableHighSpeedMode ) {
		if( clock > 25000000 ) {
			div = 2;
			ifmode = 0x0001;	/* [SD,eSD/2.7-3.6V/High Speed] */
		}
	}
#if			defined(_SDHUB_ENABLE_UHS_I_)
	if( confdata->slot[slot_info->slotNo].enableUHS1Mode ) {
		if( clock > 50000000 ) {
			ifmode = 0x00BF;	/* [SD/1.70-1.95V/UHS-I SDR50] */
		}
	}
#endif	/*		defined(_SDHUB_ENABLE_UHS_I_)	*/
	if( ( slot_info->host->pdev->id == 0 ) && ( slot_info->slotNo == 1 ) ) {
		regdata &= 0x00FF ;		/* clear [15:8] */
		ifmode <<= 8 ;
	}
	else {
		regdata &= 0xFF00 ;	/* clear [7:0] */
	}
	regdata |= ifmode ;
	mn66831_reg_setdata(slot_info, MN66831_MMC_IFMODE, regdata );
	Sdhubs_DebugPrint(__LINE__,SDHUB_PDEBUG,"%s:MN66831_IFMODE=0x%08x\n",__func__, regdata ); /* for debug */    
    
	if( bl_clken == true ) {
        Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:Start Card Clock\n",__func__ );    /* for debug */
		mn66831_ctrl_card_clock( slot_info, true ); // Huawei comment : we think it will be 2-8 in C_MN66831_Power_Supply_Sequences_V100.pdf
	}
	
	return ;
}

static void mn66831_ctrl_bus_width( struct mn66831_slot *slot_info, unsigned int width ) {
	unsigned int regdata ;
	
	/* SD_OPTION(0x0X28) */
	mn66831_reg_getdata(slot_info, MN66831_MMC_OPTION, &regdata );
	regdata &= ~0xA000 ;
	switch( width ) {
		case SDHUB_BUS_WIDTH_1BIT :
			regdata |= 0x8000 ;
			break ;
		case SDHUB_BUS_WIDTH_8BIT :
			regdata |= 0x2000 ;
			break ;
		case SDHUB_BUS_WIDTH_4BIT :
		default :
			break ;
	}
	mn66831_reg_setdata(slot_info, MN66831_MMC_OPTION, regdata );
	
	return ;
}

static void mn66831_ctrl_suspend_disable( struct mn66831_slot *slot_info ) {
	mn66831_reg_setdata( slot_info, MN66831_CLK_CTRL, 0x8047 );
	mn66831_reg_anddata( slot_info, MN66831_SUSPEND_CTRL, ~0x0001 ) ;
	mn66831_reg_setdata( slot_info, MN66831_SLOT_CTRL, 0x0000 );
}

static void mn66831_ctrl_standby_disable( struct mn66831_slot *slot_info ) {
	mn66831_reg_anddata( slot_info, MN66831_STBY_CTRL, ~0x0303 ) ;
}

#if			defined(_SDHUB_EXTENSION_)
static void mn66831_ctrl_suspend( struct mn66831_slot *slot_info, int suspend ) {
	if( suspend ){
		mn66831_reg_ordata( slot_info, MN66831_SUSPEND_CTRL, 0x0001 ) ;
		Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:SUSPEND Enable\n", __func__ );
	}
	else{
		mn66831_reg_anddata( slot_info, MN66831_SUSPEND_CTRL, ~0x0001 ) ;
		Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:SUSPEND Disable\n", __func__ );
	}
}

#if 0
static void mn66831_ctrl_standby( struct mn66831_slot *slot_info, int standby ) {
	if( standby ) {
		mn66831_reg_ordata( slot_info, MN66831_STBY_CTRL, 0x0303 ) ;
		Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:STANDBY Enable\n", __func__ );
	}
	else{
		mn66831_reg_anddata( slot_info, MN66831_STBY_CTRL, ~0x0303 ) ;
		Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:STANDBY Disable\n", __func__ );
	}
}
#endif
#endif	/*	defined(_SDHUB_EXTENSION_)	*/

static void mn66831_ctrl_soft_reset( struct mn66831_slot *slot_info, unsigned int target ) {
	unsigned int regdata ;
	int counter = 0 ;

    if (SDHUB_SOFT_RESET_SDIF == target) {
    	mn66831_reg_getdata(slot_info, MN66831_MMC_SOFT_RST, &regdata );
    	regdata &= ~0x0001 ;
    	mn66831_reg_setdata(slot_info, MN66831_MMC_SOFT_RST, regdata );
    	regdata |= 0x0001 ;
    	mn66831_reg_setdata(slot_info, MN66831_MMC_SOFT_RST, regdata );
    	    
    	regdata = 0x001F;
    	if( slot_info->host->pdev->id == 0 ) {
    		if( slot_info->pdata->bus_width <= 4 ) {
    			regdata |= 0x1F00 ;
    		}
    		else {
    			regdata |= 0x0020 ;
    		}
    	}
    	mn66831_reg_setdata(slot_info, MN66831_MMC_PULLUP, regdata );
    	    
    	mn66831_reg_setdata(slot_info, MN66831_MMC_INFO1_MASK, 0x031D );
    	mn66831_reg_setdata(slot_info, MN66831_MMC_INFO2_MASK, 0x837F );
    	
    	regdata = 0x0004 ;
    	if( slot_info->host->pdev->id == 0 ) {
    		regdata |= 0x0008 ;
    	}
    	mn66831_reg_setdata(slot_info, MN66831_MMC_EXT_MODE, regdata );
    }else {
        do {
            mn66831_reg_getdata(slot_info, MN66831_MMC_INFO2, &regdata );
            msleep(1);
            if( ++counter > MMC_INFO2_RETRY_MAX_TIME )
                break;
        } while ( regdata & 0x4000 );
        
		Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s: regdata = 0x%08x  counter = %d\n", __func__, regdata, counter );

		mn66831_ctrl_card_clock( slot_info, false );

        mn66831_reg_ordata(slot_info, MN66831_MMC_EXT_MODE, ~0x0004);
    }

	return ;
}

#if			defined(_SDHUB_ENABLE_UHS_I_)
static void mn66831_ctrl_card_voltage( struct mn66831_slot *slot_info, unsigned int vol ) {
	unsigned int regdata;
	int i;

	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"MMC mn66831_ctrl_card_voltage() -> %d\n", vol);
	
	if( slot_info->host->pdev->id == 0 ) {
		// UHS-I レギュレータ対応は、Slot-Sのみ。Slot-A/Bの場合は制御しない。
		return ;
	}
	
	switch( vol ) {
	case SDHUB_VOLTAGE_0V:
		// UHS-I 用レギュレータOFF
		mn66831_reg_anddata( slot_info, MN66831_UHS1_REGSEL, ~0x0002 );
		mn66831_delay(1);
		// 0V 強制引き込み
		mn66831_reg_anddata( slot_info, MN66831_UHS1_REGSEL, ~0x0004 );
		break;
	case SDHUB_VOLTAGE_1_8V:
		// 放電時間カウント設定
		mn66831_reg_setdata(slot_info, MN66831_UHS1_REG_COUNT0, 0x007F );
		mn66831_reg_setdata(slot_info, MN66831_UHS1_REG_COUNT1, 0x203A );
		mn66831_reg_setdata(slot_info, MN66831_UHS1_REG_COUNT2, 0x7FFF );

		mn66831_reg_getdata(slot_info, MN66831_UHS1_REGSEL, &regdata );
		// 0V 自動制御 / UHS-I 用レギュレータON / 1.8V
		regdata = (regdata & 0xFFF8) | 0x0006;
		mn66831_reg_setdata(slot_info, MN66831_UHS1_REGSEL, regdata );
		// マスク解除
		mn66831_reg_anddata( slot_info, MN66831_UHS1_REGSEL_MASK, ~0x4000 );
		// レギュレータ放電シーケンス開始
		mn66831_reg_ordata( slot_info, MN66831_UHS1_REGSEL, 0x0100 );
		// シーケンス動作確認
		for(i = 0; i < 100; i++){
			mn66831_reg_getdata(slot_info, MN66831_UHS1_REGSEL, &regdata );
			Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"MMC Do Check -> 1.8V %x\n", regdata);
			if(!(regdata & 0x8000)){
				break;
			}
			mn66831_delay(1);
		}
		// シーケンス完了確認
		for(i = 0; i < 100; i++){
			mn66831_reg_getdata(slot_info, MN66831_UHS1_REGSEL, &regdata );
			Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"MMC Done Check -> 1.8V %x\n", regdata);
			if(regdata & 0x4000){
				break;
			}
			mn66831_delay(1);
		}
		break;
	case SDHUB_VOLTAGE_3_3V:
	default :
		// 0V 自動制御 / UHS-I 用レギュレータON / 3.3V
//		mn66831_reg_ordata( slot_info, MN66831_UHS1_REGSEL, 0x0007 );
		mn66831_reg_setdata( slot_info, MN66831_UHS1_REGSEL, 0x0007 );
		break;
	}

    mn66831_reg_getdata(slot_info, MN66831_UHS1_REGSEL, &regdata );
    Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:MN66831_UHS1_REGSEL=0x%08x\n",__func__, regdata );

	return;
}
#endif	/*	defined(_SDHUB_ENABLE_UHS_I_)	*/

/*==============================================================================*/
/*  Function Name  void mn66831_cmd_sdsd_reset( struct mn66831_slot *slot_info ) */
/*------------------------------------------------------------------------------*/
/** @brief      Send SDSD_RESET(CMD60)
 *  @param[in]  *slot_info : slot information
 *  @return     void
 *  @attention  after sending SDSD_RESET, SD-Hub status changes as follows.
                 - SD bus pull up : on
                 - SDDAT1 interrupt mode : disable
                 - SD bus width : 1 bit
                 - SD bus speed : 3.3V, default speed
*/
/*==============================================================================*/
void mn66831_cmd_sdsd_reset( struct mn66831_slot *slot_info ) {
	struct mn66831_package *package = slot_info->host->package ;
	int err;
	struct mmc_command bcmd ;
	unsigned int sub_resp ;
	
	memset(&bcmd, 0, sizeof(struct mmc_command));

	/* cmd : opcode, arg, resp[4], flags, retries, data, mrq */
	bcmd.opcode		= SDHUB_CMD_SDSD_RESET;
	bcmd.arg		= SDHUB_STUFF_BITS;
	bcmd.flags		= MMC_RSP_SPI_R1 | MMC_RSP_NONE | MMC_CMD_AC ;
	
//	mmc_claim_host(package->master_host);
	err = mmc_wait_for_cmd(package->master_host, &bcmd, 0);
//	mmc_release_host(package->master_host);
	
	mn66831_delay(1) ;
	
	if (err)
		return ;
	
	/* コマンドレスポンス[39:8](32bit分) */
//	sub_resp = ( ( bcmd.resp[2] & 0x000000FF ) << 24 ) | ( ( bcmd.resp[3] & 0xFFFFFF00 ) >> 8 ) ;
	sub_resp = bcmd.resp[0] ;
	
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"< %s\n", __func__ );
	
	return ;
}

/*==============================================================================*/
/*  Function Name  void mn66831_cmd_set_sdsd_param( struct mn66831_slot *slot_info ) */
/*------------------------------------------------------------------------------*/
/** @brief      Send SET_SDSD_PARAM(CMD61)
 *  @param[in]  *slot_info : slot information
 *  @return     void
 *  @attention  
 */
/*==============================================================================*/
void mn66831_cmd_set_sdsd_param( struct mn66831_slot *slot_info ) {
	struct mn66831_package *package = slot_info->host->package ;
	int err;
	struct mmc_command bcmd ;
	unsigned int sub_resp ;
	
	memset(&bcmd, 0, sizeof(struct mmc_command));

	/* cmd : opcode, arg, resp[4], flags, retries, data, mrq */
	bcmd.opcode		= SDHUB_CMD_SET_SDSD_PARAM;
	bcmd.arg 		= 0;
	bcmd.flags		= MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC ;
	
	/* Argument of SET_SDSD_PARAM for MN66831  */
	bcmd.arg |= ((unsigned long)package->sdsd_param.enablePullUp)	<< 29 ;
	bcmd.arg |= ((unsigned long)package->sdsd_param.setIntMode)	<< 26 ;
	bcmd.arg |= ((unsigned long)package->sdsd_param.setSdMode)		<< 18 ;
	bcmd.arg |= ((unsigned long)package->sdsd_param.setSdWidth)	<< 16 ;
	
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:arg = 0x%08X\n", __func__, bcmd.arg );
	
//	mmc_claim_host(package->master_host);
	err = mmc_wait_for_cmd(package->master_host, &bcmd, MN66831_CMD_RETRIES);
//	mmc_release_host(package->master_host);
	
	if (err){
		Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:wait_for_command() = %d\n", __func__,err );
		return ;
	}
	/* コマンドレスポンス[39:8](32bit分) */
//	sub_resp = ( ( bcmd.resp[2] & 0x000000FF ) << 24 ) | ( ( bcmd.resp[3] & 0xFFFFFF00 ) >> 8 ) ;
	sub_resp = bcmd.resp[0] ;
	
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"< %s\n", __func__ );
	
	return ;
}

/*==============================================================================*/
/*  Function Name  int mn66831_cmd_send_sdsd_status( struct mn66831_slot *slot_info, unsigned int *sdhub_status ) */
/*------------------------------------------------------------------------------*/
/** @brief      Send SEND_SDSD_STATUS(CMD59)
 *  @param[in]  *slot_info    : slot information
 *  @param[out] *sdhub_status : SD-Hub status
 *  @return     
 *  @attention  
 */
/*==============================================================================*/
int mn66831_cmd_send_sdsd_status( struct mn66831_slot *slot_info, unsigned int *sdhub_status ) {
	struct mn66831_package *package = slot_info->host->package ;
	int err = 0 ;
	struct mmc_command bcmd ;
	unsigned int sub_resp ;
	
	memset(&bcmd, 0, sizeof(struct mmc_command));

	/* cmd : opcode, arg, resp[4], flags, retries, data, mrq */
	bcmd.opcode	= SDHUB_CMD_SEND_SDSD_STATUS;
	bcmd.arg	= SDHUB_STUFF_BITS;
	bcmd.flags	= MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC ;

//	mmc_claim_host(package->master_host);
	err = mmc_wait_for_cmd(package->master_host, &bcmd, MN66831_CMD_RETRIES);
//	mmc_release_host(package->master_host);
	
	if (err)
		return err ;
	
	if( bcmd.error != 0 ) {
		return bcmd.error ;
	}
	
	/* コマンドレスポンス[39:8](32bit分) */
//	sub_resp = ( ( bcmd.resp[2] & 0x000000FF ) << 24 ) | ( ( bcmd.resp[3] & 0xFFFFFF00 ) >> 8 ) ;
	sub_resp = bcmd.resp[0] ;
	
	*sdhub_status = sub_resp ;
	
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:sdsd_status = 0x%08X\n", __func__, sub_resp );

	return err ;
}

/*==============================================================================*/
/*  Function Name  void mn66831_cmd_write_single_reg( struct mn66831_slot *slot_info, volatile unsigned int *address, unsigned int value, bool verify ) */
/*------------------------------------------------------------------------------*/
/** @brief      Send WRITE_SINGLE_REG(BCMD30)
 *  @param[in]  *slot_info    : slot information
 *  @param[in]  *address      : register address
 *  @param[in]  value         : write data
 *  @param[in]  verify        : true: enable verification for writing
 *  @return     void
 *  @attention  
 */
/*==============================================================================*/
void mn66831_cmd_write_single_reg( struct mn66831_slot *slot_info, volatile unsigned int *address, unsigned int value, bool verify ) {
	struct mn66831_package *package = slot_info->host->package ;
	int err;
	struct mmc_command bcmd ;
	unsigned int sub_resp ;
	
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:slot_info = 0x%08X\n", __func__, (unsigned int)slot_info );
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:address = 0x%08X\n", __func__, (unsigned int)address );
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:value = 0x%08X\n", __func__, value );
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:verify = %d\n", __func__, verify );
	
	memset(&bcmd, 0, sizeof(struct mmc_command));

	/* cmd : opcode, arg, resp[4], flags, retries, data, mrq */
	bcmd.opcode = SDHUB_BCMD_WRITE_SINGLE_REG;
	bcmd.arg = 0;
	bcmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC ;
	
	/* Argument of WRITE_SINGLE_REG for MN66831  */
	bcmd.arg = ( value << 16 ) | ( verify << 15 ) | ( (unsigned int)address & 0x000007FF ) ;
	
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:bcmd.opcode = %d\n", __func__, bcmd.opcode );
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:bcmd.arg = 0x%08X\n", __func__, bcmd.arg );
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:bcmd.flags = 0x%08X\n", __func__, bcmd.flags );
	
//	mmc_claim_host(package->master_host);
	err = mmc_wait_for_cmd(package->master_host, &bcmd, MN66831_CMD_RETRIES);
//	mmc_release_host(package->master_host);
	if (err) {
		Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:wait_for_command() = %d\n", __func__,err );
		return ;
	}
	
	/* コマンドレスポンス[39:8](32bit分) */
//	sub_resp = ( ( bcmd.resp[2] & 0x000000FF ) << 24 ) | ( ( bcmd.resp[3] & 0xFFFFFF00 ) >> 8 ) ;
	sub_resp = bcmd.resp[0] ;
	
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:value = 0x%08X\n", __func__, (sub_resp>>16) );
	
	return ;
}

/*==============================================================================*/
/*  Function Name  void mn66831_cmd_read_single_reg( struct mn66831_slot *slot_info, volatile unsigned int *address, unsigned int *value ) */
/*------------------------------------------------------------------------------*/
/** @brief      Send READ_SINGLE_REG(BCMD31)
 *  @param[in]  *slot_info    : slot information
 *  @param[in]  *address      : register address
 *  @param[out] *value        : read data
 *  @return     void
 *  @attention  
 */
/*==============================================================================*/
void mn66831_cmd_read_single_reg( struct mn66831_slot *slot_info, volatile unsigned int *address, unsigned int *value ){
	struct mn66831_package *package = slot_info->host->package ;
	int err;
	struct mmc_command bcmd ;
	unsigned int sub_resp ;
	
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:slot_info = 0x%08X\n", __func__, (unsigned int)slot_info );
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:address = 0x%08X\n", __func__, (unsigned int)address );
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:&value = 0x%08X\n", __func__, (unsigned int)value );
	
	memset(&bcmd, 0, sizeof(struct mmc_command));

	/* cmd : opcode, arg, resp[4], flags, retries, data, mrq */
	bcmd.opcode = SDHUB_BCMD_READ_SINGLE_REG;
	bcmd.arg = 0;
	bcmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC ;

	/* Argument of READ_SINGLE_REG for MN66831  */
	bcmd.arg = (unsigned int)address & 0x000007FF ;
	
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:bcmd.opcode = %d\n", __func__, bcmd.opcode );
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:bcmd.arg = 0x%08X\n", __func__, bcmd.arg );
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:bcmd.flags = 0x%08X\n", __func__, bcmd.flags );
	
//	mmc_claim_host(package->master_host);
	err = mmc_wait_for_cmd(package->master_host, &bcmd, MN66831_CMD_RETRIES);
//	mmc_release_host(package->master_host);
	
	if (err) {
		Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:wait_for_command() = %d\n", __func__,err );
		return ;
	}
	/* コマンドレスポンス[39:8](32bit分) */
//	sub_resp = ( ( bcmd.resp[2] & 0x000000FF ) << 24 ) | ( ( bcmd.resp[3] & 0xFFFFFF00 ) >> 8 ) ;
	sub_resp = bcmd.resp[0] ;

    Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:sub_resp = 0x%08x\n", __func__, sub_resp );

	*value = (( sub_resp & 0xFFFF0000 ) >> 16 ) ;
	
	return ;
}

/*==============================================================================*/
/*  Function Name  int mn66831_cmd_sd_path_set_slot( struct mn66831_slot *slot_info ) */
/*------------------------------------------------------------------------------*/
/** @brief      Send SD_PATH_SET_SLOT(BCMD40)
 *  @param[in]  *slot_info    : slot information
 *  @return     void
 *  @attention  
 */
/*==============================================================================*/
int mn66831_cmd_sd_path_set_slot( struct mn66831_slot *slot_info ){
	int err = 0 ;
	struct mn66831_package *package = slot_info->host->package ;
	struct mmc_command bcmd ;
	unsigned int rca ;
	unsigned int slot_id ;
	unsigned int sub_resp ;
	
	memset( &bcmd, 0, sizeof(struct mmc_command) ) ;
	
	/* CMD55/CMD13自動発行は使用しないので、0設定にしておく */
//	rca = slot_info->upHost->card->rca ; 
	rca = 0x0000 ;
	
	slot_id = 0 ;
	if( slot_info->host->pdev->id == 1 ) {
		slot_id = 2 ;
	}
	else if( slot_info->slotNo == 1 ) {
		slot_id = 1 ;
	}
	
	/* cmd : opcode, arg, resp[4], flags, retries, data, mrq */
	bcmd.opcode = SDHUB_BCMD_SD_PATH_SET_SLOT;
	bcmd.arg = (rca<<16)|slot_id;
	bcmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC ;
	
//	mmc_claim_host(package->master_host);
	err = mmc_wait_for_cmd(package->master_host, &bcmd, MN66831_CMD_RETRIES);
//	mmc_release_host(package->master_host);
	
	if( err != 0 ) {
		slot_info->mrq->cmd->error = bcmd.error ;
		return err ;
	}
	
	/* コマンドレスポンス[39:8](32bit分) */
//	sub_resp = ( ( bcmd.resp[2] & 0x000000FF ) << 24 ) | ( ( bcmd.resp[3] & 0xFFFFFF00 ) >> 8 ) ;
	sub_resp = bcmd.resp[0] ;
    Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:sub_resp = 0x%08x\n", __func__, sub_resp );

	return err ;
}

/*==============================================================================*/
/*  Function Name  int mn66831_cmd_sd_path_set_param( struct mn66831_slot *slot_info ) */
/*------------------------------------------------------------------------------*/
/** @brief      Send SD_PATH_SET_PARAM(BCMD41)
 *  @param[in]  *slot_info    : slot information
 *  @return     void
 *  @attention  
 */
/*==============================================================================*/
int mn66831_cmd_sd_path_set_param( struct mn66831_slot *slot_info ){
	int err = 0 ;
	struct mn66831_package *package = slot_info->host->package ;
	struct mmc_request mrq ;
	struct mmc_command *cmd = slot_info->mrq->cmd ;
	struct mmc_command bcmd ;
	struct mmc_data data ;
	struct scatterlist sg ;
	unsigned int sub_resp ;

	memset(&mrq, 0, sizeof(struct mmc_request));
	memset(&bcmd, 0, sizeof(struct mmc_command));
	memset(&data, 0, sizeof(struct mmc_data));

	/* mrq : cmd, data, stop, done_data, done */
	mrq.cmd = &bcmd;
	mrq.data = &data;

	/* cmd : opcode, arg, resp[4], flags, retries, data, mrq */
	bcmd.opcode		= SDHUB_BCMD_SD_PATH_SET_PARAM;
	bcmd.arg		= SDHUB_STUFF_BITS;
	bcmd.flags		= MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	/* data : timeout_ns, timeout_clks, blksz, blocks, error, flags, bytes_xfered, stop, sg_len, sg */
	data.blksz = 16;
	data.blocks = 1;
	data.flags = MMC_DATA_WRITE;
	data.sg = &sg;
	data.sg_len = 1;
	data.timeout_ns = 1000000000 ;	/* 1sec */
	data.timeout_clks = 0 ;

	memset(&package->bcmd41buf, 0, SDHUB_SIZE_SD_PATH_SET_PARAM );
	
	/* CMD_INDEX,  */
	package->bcmd41buf[0] = cmd->opcode & 0x3F ;
	
	/* レスポンスタイプ設定 */
	switch( mmc_resp_type(cmd) ) {
		case MMC_RSP_NONE :
			/* none設定 */
			package->bcmd41buf[1] = 0x03 ;
			break ;
		case MMC_RSP_R1 :
//		case MMC_RSP_R6 :
//		case MMC_RSP_R5 :
//		case MMC_RSP_R7 :
			/* R1,R6,R5,R7設定 */
			package->bcmd41buf[1] = 0x04 ;
			break ;
		case MMC_RSP_R1B :
			/* R1b,R5b設定 */
			package->bcmd41buf[1] = 0x05 ;
			break ;
		case MMC_RSP_R2 :
			/* R2設定 */
			package->bcmd41buf[1] = 0x06 ;
			break ;
		case MMC_RSP_R3 :
//		case MMC_RSP_R4 :
			/* R3,R4設定 */
			package->bcmd41buf[1] = 0x07 ;
			break ;
		default :
//			w_Result = SDD_ERR_PARAM ;
			break ;
	}

	package->bcmd41buf[2] = (unsigned char)(( cmd->arg & 0xFF000000 ) >> 24 ) ;
	package->bcmd41buf[3] = (unsigned char)(( cmd->arg & 0x00FF0000 ) >> 16 ) ;
	package->bcmd41buf[4] = (unsigned char)(( cmd->arg & 0x0000FF00 ) >>  8 ) ;
	package->bcmd41buf[5] = (unsigned char)(( cmd->arg & 0x000000FF ) >>  0 ) ;
	
	/* パススルー時マルチブロック BCMD41内データのCMD12自動発行ON */
	/* BCMD18/25の場合は、Master側からCMD12を投げる必要あり。*/
	/* 転送データ有り */
	if( cmd->data ) {
		cmd->data->error = 0 ;
		
		/* command with data */
		package->bcmd41buf[7] |= 0x10;
		
		/* data direction */
		if( cmd->data->flags & MMC_DATA_READ ) {
			/* 読出し方向設定 */
			package->bcmd41buf[7] |= 0x08 ;
		}
		
		switch( cmd->opcode ) {
			case SDHUB_BCMD_READ_MULTIPLE_BLOCK :
			case SDHUB_BCMD_WRITE_MULTIPLE_BLOCK :
				if(slot_info->pdata->enableSdPathAutoCmd12){
					/* auto stop transmission mode */
					package->bcmd41buf[7] |= 0x02 ;
				}
				break ;
			
			default :
				break ;
		}
		
		if(slot_info->pdata->enableSdPathAutoCmd13){
			/* auto card status check */
			package->bcmd41buf[7] |= 0x01 ;
		}
		
//		if( cmd->data->blksz >= 512 ) {
			/* 転送ブロック数設定 */
			package->bcmd41buf[9]  = (unsigned char)(( cmd->data->blocks & 0x00FF0000 ) >> 16 ) ;
			package->bcmd41buf[10] = (unsigned char)(( cmd->data->blocks & 0x0000FF00 ) >>  8 ) ;
			package->bcmd41buf[11] = (unsigned char)(( cmd->data->blocks & 0x000000FF ) >>  0 ) ;
//		}
		/* ブロックサイズ設定 */
		package->bcmd41buf[12] = (unsigned char)(( cmd->data->blksz & 0x00000300 ) >>  8 ) ;
		package->bcmd41buf[13] = (unsigned char)(( cmd->data->blksz & 0x000000FF ) >>  0 ) ;
		
	}
	
//	if(slot_info->pdata->enableSdPathAutoCmd55){
//		/* auto app_cmd mode */
//		package->bcmd41buf[7] |= 0x04 ;
//	}
	
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:sd_init_one()\n", __func__ );
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:package->bcmd41buf = 0x%08X\n", __func__, (unsigned int)package->bcmd41buf );
	sg_init_one(&sg, &(package->bcmd41buf[0]), 16);
	
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:sd_copy_from_buffer()\n", __func__ );
//	sg_copy_from_buffer( &sg, 1, package->bcmd41buf, SDHUB_SIZE_SD_PATH_SET_PARAM ) ;

//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s: SendRequest Begin\n", __func__ );
//	mmc_claim_host(package->master_host);
	mmc_wait_for_req(package->master_host, &mrq);
//	mmc_release_host(package->master_host);
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s: SendRequest End\n", __func__ );
	
	if( bcmd.error ) {
		slot_info->mrq->cmd->error = bcmd.error ;
//		slot_info->mrq->cmd->data->error = data.error ;
		Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"<%s:bcmd.error = %d\n", __func__ , bcmd.error);
		return bcmd.error ;
	}
	if( data.error ) {
        slot_info->mrq->cmd->error = bcmd.error ;
        if (NULL != slot_info->mrq->cmd->data) {
    		slot_info->mrq->cmd->data->error = data.error ;            
        } else {
    		Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"<%s:slot_info->mrq->cmd->data = NULL\n", __func__);        
        }
		Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"<%s:data.error = %d\n", __func__ , data.error);
		return data.error ;
	}
	
	/* コマンドレスポンス[39:8](32bit分) */
//	sub_resp = ( ( bcmd.resp[2] & 0x000000FF ) << 24 ) | ( ( bcmd.resp[3] & 0xFFFFFF00 ) >> 8 ) ;
	sub_resp = bcmd.resp[0] ;
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:sub_resp = 0x%08X\n", __func__ , sub_resp);
	
	return err ;
}

/*==============================================================================*/
/*  Function Name  int mn66831_cmd_sd_path_execute( struct mn66831_slot *slot_info ) */
/*------------------------------------------------------------------------------*/
/** @brief      Send SD_PATH_EXECUTE(BCMD42)
 *  @param[in]  *slot_info    : slot information
 *  @return     void
 *  @attention  
 */
/*==============================================================================*/
int mn66831_cmd_sd_path_execute( struct mn66831_slot *slot_info ){
	int err = 0 ;
	struct mn66831_package *package = slot_info->host->package ;
	struct mmc_request mrq ;
	struct mmc_command *cmd = slot_info->mrq->cmd ;
	struct mmc_command bcmd ;
	struct mmc_data data ;
//	struct mmc_command stop ;
	unsigned int sub_resp ;
//	unsigned char *sg_ptr ;
	
	memset(&mrq, 0, sizeof(struct mmc_request));
	memset(&bcmd, 0, sizeof(struct mmc_command));
	memset(&data, 0, sizeof(struct mmc_data));
	
	mrq.cmd  = &bcmd ;
	
	/* パススルー時マルチブロック BCMD41内データのCMD12自動発行ON、Master側の自動発行OFF */
	/* BCMD18/25の場合は、Master側からCMD12を投げる必要あり。 */
	
	/* cmd : opcode, arg, resp[4], flags, retries, data, mrq */
	bcmd.opcode = SDHUB_BCMD_SD_PATH_EXECUTE;
	bcmd.arg = 0;
	bcmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC ;
	bcmd.retries = 0 ;
	bcmd.mrq = &mrq ;
	
	/* 転送データ有り */
	if( cmd->data ) {
		mrq.data = &data ;
		bcmd.data = &data ;
		bcmd.arg = cmd->data->blocks & 0x00FFFFFF ;
		bcmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC ;
		
		/* data : timeout_ns, timeout_clks, blksz, blocks, error, flags, bytes_xfered, stop, sg_len, sg */
		data.timeout_ns = cmd->data->timeout_ns ;
		data.timeout_clks = cmd->data->timeout_clks ;
		data.blksz   = cmd->data->blksz ;
		data.blocks  = cmd->data->blocks ;
		data.flags   = cmd->data->flags ;
		data.sg      = cmd->data->sg ;
		data.sg_len  = cmd->data->sg_len ;
	}
	
//	mmc_claim_host(package->master_host);
	mmc_wait_for_req(package->master_host, &mrq);
//	mmc_release_host(package->master_host);
	
	if( bcmd.error ) {
		slot_info->mrq->cmd->error = bcmd.error ;
		return bcmd.error ;
	}
	if( data.error ) {
		slot_info->mrq->cmd->error = bcmd.error ;
		slot_info->mrq->cmd->data->error = data.error ;
		return data.error ;
	}
	
	/* コマンドレスポンス[39:8](32bit分) */
//	sub_resp = ( ( bcmd.resp[2] & 0x000000FF ) << 24 ) | ( ( bcmd.resp[3] & 0xFFFFFF00 ) >> 8 ) ;
	sub_resp = bcmd.resp[0] ;
	
	if( cmd->data ) {
		cmd->data->bytes_xfered = data.bytes_xfered ;
		cmd->data->error = data.error ;
	}
	
#if 0
	if( cmd->data ){
		sg_ptr = sg_virt(cmd->data->sg);
		Sdhubs_DebugBufPrint(__LINE__, SDHUB_PDEBUG, sg_ptr, cmd->data->blksz ) ;
	}
#endif
	
	return err ;
}

/*==============================================================================*/
/*  Function Name  int mn66831_cmd_sd_path_send_response( struct mn66831_slot *slot_info ) */
/*------------------------------------------------------------------------------*/
/** @brief      Send SD_PATH_SEND_RESPONSE(BCMD43)
 *  @param[in]  *slot_info    : slot information
 *  @return     void
 *  @attention  
 */
/*==============================================================================*/
int mn66831_cmd_sd_path_send_response( struct mn66831_slot *slot_info ){
	int err = 0 ;
	struct mn66831_package *package = slot_info->host->package ;
	struct mmc_command *cmd = slot_info->mrq->cmd ;
	struct mmc_request mrq ;
	struct mmc_command bcmd ;
	struct mmc_data data ;
	struct scatterlist sg ;
	int i,j ;
	unsigned char *rp;
	unsigned char *wp;
	unsigned char chk_resp ;
	unsigned int sub_resp ;
	
	memset(&mrq, 0, sizeof(struct mmc_request));
	memset(&bcmd, 0, sizeof(struct mmc_command));
	memset(&data, 0, sizeof(struct mmc_data));

	/* mrq : cmd, data, stop, done_data, done */
	mrq.cmd  = &bcmd;
	mrq.data = &data;
	
	/* cmd : opcode, arg, resp[4], flags, retries, data, mrq */
	bcmd.opcode		= SDHUB_BCMD_SD_PATH_SEND_RESPONSE;
	bcmd.arg		= SDHUB_STUFF_BITS;
	bcmd.flags		= MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	/* data : timeout_ns, timeout_clks, blksz, blocks, error, flags, bytes_xfered, stop, sg_len, sg */
	data.blksz   = SDHUB_SIZE_SD_PATH_SEND_RESPONSE;
	data.blocks  = 1;
	data.flags   = MMC_DATA_READ;
	data.sg      = &sg;
	data.sg_len  = 1;
	
	data.timeout_ns = 1000000000 ;	/* 1秒設定 */
	data.timeout_clks = 0 ;

//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:sd_init_one()\n", __func__ );
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:package->bcmd43buf = 0x%08X\n", __func__, (unsigned int)package->bcmd43buf );
	sg_init_one(&sg, package->bcmd43buf, SDHUB_SIZE_SD_PATH_SEND_RESPONSE);
	
//	mmc_claim_host(package->master_host);
	mmc_wait_for_req(package->master_host, &mrq);
//	mmc_release_host(package->master_host);
	
	if (bcmd.error)
		return bcmd.error ;
	if (data.error)
		return data.error ;
	
	/* コマンドレスポンス[39:8](32bit分) */
//	sub_resp = ( ( bcmd.resp[2] & 0x000000FF ) << 24 ) | ( ( bcmd.resp[3] & 0xFFFFFF00 ) >> 8 ) ;
	sub_resp = bcmd.resp[0] ;
	
//	sg_copy_to_buffer( &sg, 1, package->bcmd43buf, SDHUB_SIZE_SD_PATH_SEND_RESPONSE ) ;
	
	for(i=0;i<4;i++){
		cmd->resp[i] = 0x00000000 ;
	}
	chk_resp = 0 ;
	if( mmc_resp_type(cmd) & MMC_RSP_136 ) {
		/* パススルーコマンドのレスポンスを構造体にデータコピー(120bit分) */
//		Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:bcmd43buf[ 2- 5] = 0x%02X%02X%02X%02X\n", __func__, package->bcmd43buf[2], package->bcmd43buf[3], package->bcmd43buf[4], package->bcmd43buf[5] );
//		Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:bcmd43buf[ 6- 9] = 0x%02X%02X%02X%02X\n", __func__, package->bcmd43buf[6], package->bcmd43buf[7], package->bcmd43buf[8], package->bcmd43buf[9] );
//		Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:bcmd43buf[10-13] = 0x%02X%02X%02X%02X\n", __func__, package->bcmd43buf[10], package->bcmd43buf[11], package->bcmd43buf[12], package->bcmd43buf[13] );
//		Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:bcmd43buf[14-17] = 0x%02X%02X%02X%02X\n", __func__, package->bcmd43buf[14], package->bcmd43buf[15], package->bcmd43buf[16], 0x00 );
		
		wp = ((unsigned char *)&(cmd->resp[0])) ;
		rp = &(package->bcmd43buf[2]) ;
		for( i = 0; i <= 15; i++ ) {
			j = (i - (i % 4)) + (3 - (i % 4)) ;
			if( j < 15 ) {
				*(wp + i) = *(rp + j) ;
				chk_resp |= *(wp + i) ;
			}
		}
		
		Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:resp[0] = 0x%08X\n", __func__, cmd->resp[0] );
		Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:resp[1] = 0x%08X\n", __func__, cmd->resp[1] );
		Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:resp[2] = 0x%08X\n", __func__, cmd->resp[2] );
		Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:resp[3] = 0x%08X\n", __func__, cmd->resp[3] );
	}
	else {
		/* パススルーコマンドのレスポンスを構造体にデータコピー(40bit分) */
		wp = ((unsigned char *)&(cmd->resp[0])) ;
		rp = &(package->bcmd43buf[13]) ;
		for( i = 0; i < 4; i++ ) {
			*(wp + i) = *(rp + (3 - (i % 4))) ;
			chk_resp |= *(wp + i) ;
		}
		
		Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:resp[0] = 0x%08X\n", __func__, cmd->resp[0] );
	}
		
	if( chk_resp == 0x00 ) {
		/* レスポンスなし判定 */
		cmd->error = SDD_ERR_IP_TIMEOUT ;		/* -EIO */
		return cmd->error ;
	}
	
#if 1
	/* 自動発行されたCMD12のレスポンスを構造体にデータコピー(32bit分) */
	chk_resp = 0 ;
	wp = ((unsigned char *)&sub_resp) ;
	rp = &(package->bcmd43buf[18]) ;
	for( i = 0; i < 4; i++ ) {
		*(wp + i) = *(rp + (3 - (i % 4))) ;
		chk_resp |= *(wp + i) ;
	}
	
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:resp[0] = 0x%08X\n", __func__, cmd->resp[0] );
//	if( chk_resp = 0x00 ) {
//		/* レスポンスなし判定 */
//		stop->error = SDD_ERR_IP_TIMEOUT ;		/* -EIO */
//		return cmd->error ;
//	}
#endif
	
#if 1
	/* 自動発行されたCMD13のレスポンスを構造体にデータコピー(32bit分) */
	chk_resp = 0 ;
	wp = ((unsigned char *)&sub_resp) ;
	rp = &(package->bcmd43buf[18]) ;
	for( i = 0; i < 4; i++ ) {
		*(wp + i) = *(rp + (3 - (i % 4))) ;
		chk_resp |= *(wp + i) ;
	}
//	if( chk_resp = 0x00 ) {
//		/* レスポンスなし判定 */
//		stop->error = SDD_ERR_IP_TIMEOUT ;		/* -EIO */
//		return cmd->error ;
//	}
#endif
	
	return err ;
}

/*==============================================================================*/
/*  Function Name  int mn66831_cmd_read_write_multiple_block( struct mn66831_slot *slot_info ) */
/*------------------------------------------------------------------------------*/
/** @brief      Send READ_MULTIPLE_BLOCK(BCMD18) or WRITE_MULTIPLE_BLOCK(BCMD25)
 *  @param[in]  *slot_info    : slot information
 *  @return     void
 *  @attention  
 */
/*==============================================================================*/
int mn66831_cmd_read_write_multiple_block( struct mn66831_slot *slot_info ) {
	int err = 0 ;
	struct mn66831_package *package = slot_info->host->package ;
	struct mmc_request mrq ;
	struct mmc_command *cmd = slot_info->mrq->cmd ;
	struct mmc_command bcmd ;
	struct mmc_command stop ;
	struct mmc_data data ;
//	struct scatterlist sg ;
//	unsigned char *sg_ptr ;
	
	memset(&mrq, 0, sizeof(struct mmc_request));
	memset(&bcmd, 0, sizeof(struct mmc_command));
	memset(&data, 0, sizeof(struct mmc_data));
	memset(&stop, 0, sizeof(struct mmc_command));

	/* mrq : cmd, data, stop, done_data, done */
	mrq.cmd = &bcmd;
	mrq.data = &data;

	/* cmd : opcode, arg, resp[4], flags, retries, data, mrq */
	if(slot_info->mrq->cmd->opcode == 18 ) {
		bcmd.opcode = SDHUB_BCMD_READ_MULTIPLE_BLOCK;
	}
	else{
		bcmd.opcode = SDHUB_BCMD_WRITE_MULTIPLE_BLOCK;
	}
	bcmd.arg = slot_info->mrq->cmd->arg ;
	bcmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
	bcmd.data = &data ;
	bcmd.mrq = &mrq ;
	
	/* data : timeout_ns, timeout_clks, blksz, blocks, error, flags, bytes_xfered, stop, sg_len, sg */
	/* CMD18/25ならば、転送データ有りのはず */
	data.timeout_ns = cmd->data->timeout_ns ;
	data.timeout_clks = cmd->data->timeout_clks ;
	data.blksz   = cmd->data->blksz ;
	data.blocks  = cmd->data->blocks ;
	data.flags   = cmd->data->flags ;
	data.sg      = cmd->data->sg ;
	data.sg_len  = cmd->data->sg_len ;
	data.stop = &stop ;
	data.mrq = &mrq ;
	
//	memcpy( &data, slot_info->mrq->cmd->data, sizeof( struct mmc_data ) );
	
	/* BCMD18/25の場合は、Master側からCMD12を投げる必要あり。 */

	/* stop : opcode, arg, resp[4], flags, retries, data, mrq */
	stop.opcode     = SDHUB_BCMD_STOP_TRANSMISSION ;
	stop.arg		= SDHUB_STUFF_BITS ;
	//stop.flags      = MMC_RSP_SPI_R1 | MMC_RSP_NONE | MMC_CMD_AC ;
	stop.flags      = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
	stop.mrq        = &mrq ;

#if			defined(SDHUB_DEBUG_SD_COMMAND_LOG)
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:mn66831_cmd_read_write_multiple_block(): blksz %d, blocks %d, sg %d, sg_len %d\n",
        __func__, data.blksz, data.blocks, data.sg, data.sg_len );
#endif	/*	defined(SDHUB_DEBUG_SD_COMMAND_LOG)	*/
	
//	mmc_claim_host(package->master_host);
	mmc_wait_for_req(package->master_host, &mrq);
//	mmc_release_host(package->master_host);
	
	if (bcmd.error){
		slot_info->mrq->cmd->error = bcmd.error ;
		slot_info->mrq->cmd->data->error = data.error ;
		return bcmd.error ;
	}
	if (data.error){
		slot_info->mrq->cmd->error = bcmd.error ;
		slot_info->mrq->cmd->data->error = data.error ;
		return data.error ;
	}
	
	if( cmd->data ) {
		cmd->data->bytes_xfered = data.bytes_xfered ;
		cmd->data->error = data.error ;
	}

	return err ;
}

#if			defined(_SDHUB_EXTENSION_)
/*==============================================================================*/
/*  Function Name  int mn66831_send_control_command( struct mn66831_slot *slot_info ) */
/*------------------------------------------------------------------------------*/
/** @brief      Send Command
 *  @param[in]  *slot_info    : slot information
 *  @return     void
 *  @attention  
 */
/*==============================================================================*/
int mn66831_send_control_command( struct mn66831_slot *slot_info, unsigned int cmd, unsigned int arg, unsigned int *resp,
						void *buff, unsigned int size, unsigned int dir, unsigned int timeout, int *error ){
	int err = 0 ;
	struct mn66831_package *package = slot_info->host->package ;
	struct mmc_request mrq ;
	struct mmc_command bcmd ;
	struct mmc_data data ;
	struct scatterlist *sg_list = NULL ;
	int i;

//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s: cmd   =  0x%04X\n", __func__, cmd );
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s: arg   =  0x%08X\n", __func__, arg );
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s: resp  = (0x%08X)\n", __func__, (unsigned int)resp );
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s: buff  = (0x%08X)\n", __func__, (unsigned int)buff );
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s: size  =  %u\n", __func__, size );
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s: dir   =  %u\n", __func__, dir );
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s: T/O   =  %u\n", __func__, timeout );
//	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s: error = (0x%08X)\n", __func__, (unsigned int)error );
	memset(&mrq, 0, sizeof(struct mmc_request));
	memset(&bcmd, 0, sizeof(struct mmc_command));
	memset(&data, 0, sizeof(struct mmc_data));

	/* mrq : cmd, data, stop, done_data, done */
	mrq.cmd  = &bcmd;

	/* cmd : opcode, arg, resp[4], flags, retries, data, mrq */
	bcmd.opcode		= cmd;
	bcmd.arg		= arg;
	bcmd.flags		= MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC;
	bcmd.retries	= 0 ;
	bcmd.mrq		= &mrq ;

	/* data : timeout_ns, timeout_clks, blksz, blocks, error, flags, bytes_xfered, stop, sg_len, sg */
	if( buff ) {
		mrq.data = &data;
		
		bcmd.data = &data ;
		bcmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC ;
		
		/* data : timeout_ns, timeout_clks, blksz, blocks, error, flags, bytes_xfered, stop, sg_len, sg */
		if( size < SDHUB_SECTOR_SIZE ) {
			data.blksz   = size ;
		} else {
			data.blksz   = SDHUB_SECTOR_SIZE ;
		}
		data.blocks  = ( size + ( SDHUB_SECTOR_SIZE - 1 ) ) / SDHUB_SECTOR_SIZE ;
		if( dir ) {
			data.flags   = MMC_DATA_READ;
		} else {
			data.flags   = MMC_DATA_WRITE;
		}
		data.timeout_ns = timeout ;	/* 1秒設定 */
		data.timeout_clks = 0 ;
		data.sg_len = (u32)(size + slot_info->upHost->max_seg_size-1)/slot_info->upHost->max_seg_size;

		sg_list = kzalloc(data.sg_len * sizeof(struct scatterlist), GFP_KERNEL);
		if (!sg_list) {
			Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s: sg_list malloc error !\n", __func__ );
			err = -ENOMEM;
			return err;
		}

                data.sg = sg_list;
                sg_init_table(data.sg, data.sg_len);
                for (i = 0; i < data.sg_len - 1; i++) {
                        sg_set_buf(&sg_list[i], buff + slot_info->upHost->max_seg_size * i, slot_info->upHost->max_seg_size);
                }
                sg_set_buf(&sg_list[i], buff + slot_info->upHost->max_seg_size * i, size - slot_info->upHost->max_seg_size * i);
                Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s: sg set ok !\n", __func__);
	}

//	mmc_claim_host(package->master_host);
	mmc_wait_for_req(package->master_host, &mrq);
//	mmc_release_host(package->master_host);

	/* コマンドレスポンス[39:8](32bit分) */
	*resp = bcmd.resp[0] ;

        if (sg_list)
                kfree(sg_list);

	if (bcmd.error)
		return bcmd.error ;
	if (data.error)
		return data.error ;
	
	return err ;
}
#endif	/*	defined(_SDHUB_EXTENSION_)	*/

/*==============================================================================*/
/*  Function Name  static void mn66831_send_command( struct mn66831_slot *slot_info ) */
/*------------------------------------------------------------------------------*/
/** @brief      Send command using path through commands(BCMD40/41/42/43)
 *  @param[in]  *slot_info    : slot information
 *  @return     void
 *  @attention  
 */
/*==============================================================================*/
static void mn66831_send_command( struct mn66831_slot *slot_info ) {
	int err_tmp = SDD_SUCCESS ;
	unsigned int sdhub_status = 0 ;

#if			defined(SDHUB_DEBUG_SD_COMMAND_LOG)
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:mn66831_send_command(): CMD=%u, arg %08x, flags %08x\n", __func__,
        slot_info->mrq->cmd->opcode, slot_info->mrq->cmd->arg, slot_info->mrq->cmd->flags );
#endif	/*	defined(SDHUB_DEBUG_SD_COMMAND_LOG)	*/
	
	/* BCMD40 */
	err_tmp = mn66831_cmd_sd_path_set_slot( slot_info );
	if( err_tmp != SDD_SUCCESS ) {
		Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:Sdhubs_SdPathSetSlot=0x%X\n", __func__, err_tmp );
		return ;
	}
	
	/* BCMD30 */
	/* SDCLK Enable */
	mn66831_ctrl_card_clock( slot_info, true );
	
	switch( slot_info->mrq->cmd->opcode ){
		case SDHUB_BCMD_READ_MULTIPLE_BLOCK :
		case SDHUB_BCMD_WRITE_MULTIPLE_BLOCK :
			/* BCMD18/BCMD25 */
			//err_tmp = mn66831_cmd_read_write_multiple_block( slot_info ) ;
			//break ;
		default :
			/* BCMD41 */
			err_tmp = mn66831_cmd_sd_path_set_param( slot_info );
			if( err_tmp != SDD_SUCCESS ) {
				Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:Sdhubs_SdPathSetParam=0x%X\n", __func__, err_tmp );
				break ;
			}
			
			/* BCMD42 */
			err_tmp = mn66831_cmd_sd_path_execute( slot_info );
			if( err_tmp != SDD_SUCCESS ) {
				Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:Sdhubs_SdPathExecute=0x%X\n", __func__, err_tmp );
			}
			
			break ;
	}
	if( err_tmp ) {
		return ;
	}

	while(1){
		/* CMD59 */
		err_tmp = mn66831_cmd_send_sdsd_status( slot_info, &sdhub_status );
		if( err_tmp != SDD_SUCCESS ) {
			Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:Sdhubs_SendSdsdStatus=0x%X\n", __func__, err_tmp );
			return ;
		}
        
//		if( ( sdhub_status & 0x00001E00 ) == 0x00000800 ) {
			/* resp[4]の格納状態は、little endianで、[0]127:96 [1]95:64 [2]63:32 [3]31:0 */
			/* SDIPからは最下位8bitが出てこないので注意 */
			if( ( (sdhub_status & 0x00006000) == 0x00000000 ) && ( ( sdhub_status & 0x00000008 ) == 0 ) ){
				break ;
			}
			if( ( (sdhub_status & 0x00006000) == 0x00002000 ) && ( ( sdhub_status & 0x00000010 ) == 0 ) ){
				break ;
			}
			if( ( (sdhub_status & 0x00006000) == 0x00004000 ) && ( ( sdhub_status & 0x00000020 ) == 0 ) ){
				break ;
			}
//		}
		
	}
    
//    }
	/* BCMD43 */
	err_tmp = mn66831_cmd_sd_path_send_response( slot_info );
	if( err_tmp != SDD_SUCCESS ) {
		Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:mn66831_cmd_sd_path_send_response=0x%X\n", __func__, err_tmp );
		return ;
	}
	
	/* エラー判定：レスポンスデータがall 0の場合、タイムアウトと判定する。 */
	
	/* BCMD30 */
    /* Clock disable */
	mn66831_ctrl_card_clock( slot_info, false );
	
	return ;
}

/*==============================================================================*/
/*  Function Name   static void mn66831_request( struct mmc_host *mmc, struct mmc_request *req ) */
/*------------------------------------------------------------------------------*/
/** @brief      command request rutine for SdHubSendCommand.(for MMC Linux)
 *  @param[in]  *mmc : information for mmc driver
 *  @param[in]  *mrq : information for mmc driver
 *  @return     void
 *  @attention  
*/ 
/*==============================================================================*/
static void mn66831_request( struct mmc_host *mmc, struct mmc_request *req ) {
	struct mn66831_slot	*slot_info = mmc_priv(mmc) ;
	struct mn66831_host	*host_info = slot_info->host ;
	struct mn66831_package *package = host_info->package ;
	
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"MMC request(%d,0x%08X)\n", req->cmd->opcode, req->cmd->arg );
	
	do {
		slot_info->mrq = req ;					/* 受信したリクエストを保持 */
		
		mmc_claim_host(package->master_host);
		slot_info->mrq->cmd->error = 0 ;
		if( slot_info->blMedia ) {
			slot_info->mrq->cmd->error = SDD_ERR_NO_CARD ;
		}
		else {
			mn66831_send_command( slot_info ) ;
		}
		mmc_release_host(package->master_host);
		
		mmc_request_done( mmc, req );
		
	} while( 0 ) ;
	
	return ;
}

/*==============================================================================*/
/*    Function Name   static void mn66831_set_ios( struct mmc_host *mmc, struct mmc_ios *ios ) */
/*------------------------------------------------------------------------------*/
/** @brief      Ios rutine for SdHubSendCommand.(for MMC Linux)
 *  @param[in]  *mmc : information for mmc driver
 *  @param[in]  *ios : information for mmc driver
 *  @return     void
 *  @attention  
*/ 
/*==============================================================================*/
static void mn66831_set_ios( struct mmc_host *mmc, struct mmc_ios *ios ) {
	struct mn66831_slot	*slot_info = mmc_priv(mmc) ;
	struct mn66831_host	*host_info = slot_info->host ;
	struct mn66831_package *package = host_info->package ;
	struct mn66831_pdata_host *dev_conf = (struct mn66831_pdata_host *)host_info->pdev->dev.platform_data;
	struct mmc_host *master_host ;
	unsigned int clock ;
	unsigned int bus_width ;
    int ret;

	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"MMC set_ios()\n");
	
	/* マスタ側スロットとのコネクションが確立されていなければ返る */
	if( slot_info->host->package->flag_connection == false ) {
		Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"<%s:flag_connection is false\n", __func__ );
		return ;
	}
	
	mmc_claim_host(package->master_host);
	master_host = package->master_host ;
	
	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:ios->power_mode = %u\n", __func__, ios->power_mode );
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:ios->bus_width = %u\n", __func__, ios->bus_width );
	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:ios->clock = %u\n", __func__, ios->clock );
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:ios->bus_mode = %u\n", __func__, ios->bus_mode );
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:ios->chip_select = %u\n", __func__, ios->chip_select );
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:ios->timing = %u\n", __func__, ios->timing );
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:ios->vdd = %d\n", __func__, ios->vdd );
	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:slot_info->ios.power_mode = %u\n", __func__, slot_info->ios.power_mode );
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:slot_info->ios.bus_width = %u\n", __func__, slot_info->ios.bus_width );
	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:slot_info->ios.clock = %u\n", __func__, slot_info->ios.clock );
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:slot_info->ios.bus_mode = %u\n", __func__, slot_info->ios.bus_mode );
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:slot_info->ios.chip_select = %u\n", __func__, slot_info->ios.chip_select );
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:slot_info->ios.timing = %u\n", __func__, slot_info->ios.timing );
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:slot_info->ios.vdd = %d\n", __func__, slot_info->ios.vdd );

	/* ios->power_mode := [ MMC_POWER_OFF | MMC_POWER_UP | MMC_POWER_ON ] */
	if(slot_info->ios.power_mode != ios->power_mode ) {
		switch( ios->power_mode ) {
			case MMC_POWER_UP :
				if( slot_info->ios.power_mode != MMC_POWER_OFF ) {
					/* no effects */
					break ;
				}
                ret	= gpio_direction_output(GPIO_RESET, 0);    // pull up XMNRST
            	if (ret) {
                	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:gpio_direction_output GPIO_RESET Fail \n", __func__ );
            	}

            	ret	= gpio_direction_output(GPIO_VDDIO_EN, vddio_status); // open vddio and vddios
            	if (ret) {
                	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:gpio_direction_output GPIO_VDDIO_EN Fail \n", __func__ );
            	}
                ret = blockmux_set(cprm_iomux_block, cprm_iomux_config, NORMAL);
            	if (ret) {
                	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:blockmux_set() Error : ret = %d\n", __func__, ret );
            	}
				if( package->power_control == 0 ) {
#if			defined(_SDHUB_ENABLE_UHS_I_)
#if			defined(_SDHUB_USE_HUBIF_SDR50_)
					/* Master Voltage Switch */
					if( master_host->caps & MMC_CAP_UHS_SDR50 ) {
						mn66831_master_voltage_switch( master_host, SDHUB_VOLTAGE_1_8V ) ;
					}
					else{
						mn66831_master_voltage_switch( master_host, SDHUB_VOLTAGE_3_3V ) ;
					}
#else	/*	defined(_SDHUB_USE_HUBIF_SDR50_)	*/
					mn66831_master_voltage_switch( master_host, SDHUB_VOLTAGE_3_3V ) ;
#endif	/*	defined(_SDHUB_USE_HUBIF_SDR50_)	*/
#endif	/*	defined(_SDHUB_ENABLE_UHS_I_)	*/
					/* Host Power ON */
					mn66831_ctrl_host_power(slot_info, true);   // Huawei comment : we power up here
					/* Master Host Power ON / Card Power ON */
					mn66831_master_power_up(master_host);       // Huawei comment : Host IP already power up while it init into kernel
					
					/* SD-Hub I/F Setting */
					package->sdsd_param.enablePullUp = false ;	/* 初期値 : プルアップOFF */
					package->sdsd_param.setIntMode   = 0 ;		/* 初期値 : SDDAT1Hで割込み通知しない */
					
					clock = master_host->f_max ;
					package->sdsd_param.setSdMode    = 0 ;		/* 初期値 : default speed */
					if( master_host->caps & (MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED) ) {
						if( master_host->f_max > 50000000 ) {
#if			defined(SDHUB_DEBUG_SPEED_MAX)
							clock = master_host->f_max ;
#else 	/*	defined(SDHUB_DEBUG_SPEED_MAX)	*/
							clock = 50000000 ;
#endif	/*	defined(SDHUB_DEBUG_SPEED_MAX)	*/
							package->sdsd_param.setSdMode    = 1 ;		/* 初期値 : high speed */
						}
						else if( master_host->f_max > 25000000 ) {
							clock = master_host->f_max ;
							package->sdsd_param.setSdMode    = 1 ;		/* 初期値 : high speed */
						}
					}
					
#if			defined(_SDHUB_USE_HUBIF_SDR50_)
					/* UHS-I(SDR50)に対応している場合 */
					if( master_host->caps & MMC_CAP_UHS_SDR50 ) {
						if( master_host->f_max > 100000000 ) {
							clock = 100000000 ;
							package->sdsd_param.setSdMode    = 3 ;		/* 初期値 : ultra high speed */
						}
						else if( master_host->f_max > 50000000 ) {
							clock = master_host->f_max ;
							package->sdsd_param.setSdMode    = 3 ;		/* 初期値 : ultra high speed */
						}
						/* SD2SD_IFMODE => 1.8V signal対応  */
						mn66831_reg_setdata(slot_info, MN66831_SD2SD_IFMODE, 0x0012 );
					}
					else {
						/* SD2SD_IFMODE => 3.3V signal対応  */
						mn66831_reg_setdata(slot_info, MN66831_SD2SD_IFMODE, 0x0000 );
					}
#endif	/*	defined(_SDHUB_USE_HUBIF_SDR50_)	*/
					
					bus_width = MMC_BUS_WIDTH_1;
					package->sdsd_param.setSdWidth   = 0 ;		/* 初期値 : SD bus 1bit mode */
					if( master_host->caps & MMC_CAP_4_BIT_DATA ) {
						bus_width = MMC_BUS_WIDTH_4;
                    	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s : MMC_CAP_4_BIT_DATA\n", __func__ );
						package->sdsd_param.setSdWidth   = 1 ;		/* 初期値 : SD bus 4bit mode */
					}
#if			defined(_SDHUB_USE_HUBIF_8BIT_)
					/* SD-Hub 8bitに対応している場合のみ */
					if( master_host->caps & MMC_CAP_8_BIT_DATA ) {
						bus_width = MMC_BUS_WIDTH_8;
						package->sdsd_param.setSdWidth   = 2 ;		/* 初期値 : SD bus 8bit mode */
					}
#endif	/*	defined(_SDHUB_USE_HUBIF_8BIT_)	*/

					mn66831_cmd_set_sdsd_param(slot_info) ;
					/* Master Host SDCLK */
					mn66831_master_set_clock(master_host,clock);
					/* Master Host Bus Width */
					mn66831_master_set_bus_width(master_host,bus_width);
					/* Suspend mode disable */
					mn66831_ctrl_suspend_disable(slot_info);        // Huawei comment : we think it will be 1-12 in C_MN66831_Power_Supply_Sequences_V100.pdf
					/* Standby mode disable */
					mn66831_ctrl_standby_disable(slot_info);
				}
				package->power_control++;
		        udelay(50);

				if( host_info->power_control == 0 ) {
					/* reset host */
					mn66831_ctrl_soft_reset(slot_info, SDHUB_SOFT_RESET_SDIF);
				}
				host_info->power_control++;
				
				slot_info->ios.power_mode = MMC_POWER_UP ;
				break ;	/* この後、上位層で、電圧上昇待ち */
			
			case MMC_POWER_ON :
				if( slot_info->ios.power_mode != MMC_POWER_UP ) {
					/* no effects */
					break ;
				}
				/* Card Power ON */
				mn66831_ctrl_card_power(slot_info, true);
				
				slot_info->ios.power_mode = MMC_POWER_ON ;

				break ;	/* この後、上位層で、電圧安定待ち */
			
			case MMC_POWER_OFF :
                mn66831_ctrl_soft_reset(slot_info, false);
                mn66831_ctrl_card_voltage(slot_info, SDHUB_VOLTAGE_0V);
                udelay(500);
                /* Card Power OFF */
				mn66831_ctrl_card_power(slot_info, false);
				
				host_info->power_control--;

				package->power_control--;
				if( package->power_control == 0 ) {
					/* Master Card Power OFF / Host Power OFF */
					mn66831_master_power_off(master_host);
					/* Host Power OFF */
					mn66831_ctrl_host_power(slot_info, false) ;
				}
				//package->flag_connection = false ;

                

                ret	= gpio_direction_input(GPIO_RESET);
            	if (ret) {
                	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:gpio_direction_input GPIO_RESET Fail \n", __func__ );
                	mmc_release_host(package->master_host);
                	return;
            	}
                ret	= gpio_direction_input(GPIO_VDDIO_EN);
            	if (ret) {
                	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:gpio_direction_input GPIO_VDDIO_EN Fail \n", __func__ );
                	mmc_release_host(package->master_host);
                	return;
            	}
                ret = blockmux_set(cprm_iomux_block, cprm_iomux_config, LOWPOWER);
            	if (ret) {
                	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:blockmux_set() Error : ret = %d\n", __func__, ret );
                	mmc_release_host(package->master_host);
                	return;
            	}

                ios->bus_width = slot_info->ios.bus_width;
                ios->clock = slot_info->ios.clock;
                ios->vdd = slot_info->ios.vdd;
                ios->timing = slot_info->ios.timing;
				slot_info->ios.power_mode = MMC_POWER_OFF ;
				break ;
			default :
				break;
		}
	}
	
	/* ios->bus_width := [ MMC_BUS_WIDTH_1 | MMC_BUS_WIDTH_4 | MMC_BUS_WIDTH_8 ] */
	if(slot_info->ios.bus_width != ios->bus_width ) {
		switch( ios->bus_width ) {
			case MMC_BUS_WIDTH_1 :
				/* set 1bit */
				mn66831_ctrl_bus_width(slot_info, SDHUB_BUS_WIDTH_1BIT);
				break;
			case MMC_BUS_WIDTH_4 :
				/* set 4bit */
				mn66831_ctrl_bus_width(slot_info, SDHUB_BUS_WIDTH_4BIT);
				break;
//			case MMC_BUS_WIDTH_8 :
//				/* set 8bit */
//				mn66831_ctrl_bus_width(slot_info, SDHUB_BUS_WIDTH_8BIT);
//				break;
			default :
				break;
		}
		slot_info->ios.bus_width = ios->bus_width ;
	}
	
	/* ios->clock */
	if(slot_info->ios.clock != ios->clock ) {
		if(ios->clock) {
			/* set clock */
        	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:slot_info->ios.clock = %u, ios->clock = %u\n", __func__, slot_info->ios.clock, ios->clock);
			mn66831_ctrl_clock_config(slot_info, ios->clock);
			slot_info->clock = ios->clock ;
		}
		slot_info->ios.clock = ios->clock ;
	}
	
	/* ios->bus_mode := [ MMC_BUSMODE_OPENDRAIN | MMC_BUSMODE_PUSHPULL ] */
	if(slot_info->ios.bus_mode != ios->bus_mode ) {
		switch( ios->bus_mode ) {
			case MMC_BUSMODE_OPENDRAIN :
			case MMC_BUSMODE_PUSHPULL :
			default :
				/* SPI Mode doesn't respond */
				break;
		}
		slot_info->ios.bus_mode = ios->bus_mode ;
	}
	
	/* ios->chip_select := [ MMC_CS_DONTCARE | MMC_CS_HIGH | MMC_CS_LOW ] */
	if(slot_info->ios.chip_select != ios->chip_select ) {
		switch( ios->chip_select ) {
			case MMC_CS_DONTCARE :
			case MMC_CS_HIGH :
			case MMC_CS_LOW :
			default :
				/* SPI Mode doesn't respond */
				break;
		}
		slot_info->ios.chip_select = ios->chip_select ;
	}
	
	/* ios->vdd := [MMC_VDD_*_*] */
	if(slot_info->ios.vdd != ios->vdd ) {
		if( dev_conf->slot[slot_info->slotNo].enableLowVolRange && (ios->vdd == ( ffs(slot_info->upHost->ocr_avail) - 1 ) ) ) {
			/* OCR 1.8V(MMC 7bit, SD 24bit) */
			//mn66831_ctrl_card_voltage(slot_info, SDHUB_VOLTAGE_1_8V);//Panasonic bugfix for vddsds
		}
		if( dev_conf->slot[slot_info->slotNo].enable0VolRange && ( ios->vdd == 0 ) ) {
			/* OCR 0V */
			mn66831_ctrl_card_voltage(slot_info, SDHUB_VOLTAGE_0V);
		}
		else {
			/* default 3.3V */
			mn66831_ctrl_card_voltage(slot_info, SDHUB_VOLTAGE_3_3V);
		}
		slot_info->ios.vdd = ios->vdd ;
	}
	
	/* ios->timing := [ MMC_TIMING_LEGACY | MMC_TIMING_MMC_HS | MMC_TIMING_SD_HS ] */
	if(slot_info->ios.timing != ios->timing ) {
		switch( ios->timing ) {
			case MMC_TIMING_MMC_HS :
				/* Timing for MMC High Speed Card */
				mn66831_ctrl_card_timing(slot_info,SDHUB_IO_MMC_HS_TIMING);
				break;
			case MMC_TIMING_SD_HS :
				/* Timing for SD High Speed Card */
				mn66831_ctrl_card_timing(slot_info,SDHUB_IO_SD_HS_TIMING);
				break;
			case MMC_TIMING_LEGACY :
			default :
				/* Normal Timing */
				mn66831_ctrl_card_timing(slot_info,SDHUB_IO_NORMAL_TIMING);
				break;
		}
		slot_info->ios.timing = ios->timing ;
	}
	mmc_release_host(package->master_host);
	return ;
}

/*==============================================================================*/
/*    Function Name   static int mn66831_get_ro( struct mmc_host *mmc ) */
/*------------------------------------------------------------------------------*/
/** @brief      Get Write Protect Information for SdHubSendCommand.(for MMC Linux)
 *  @param[in]  *mmc : information for mmc driver
 *  @return     0:        Not Protect
                -ENOSYS : Write Protect 
 *  @attention  
*/ 
/*==============================================================================*/
static int mn66831_get_ro( struct mmc_host *mmc ) {
	struct mn66831_slot	*slot_info = mmc_priv(mmc) ;
	struct mn66831_host	*host_info = slot_info->host ;
	struct mn66831_package *package = host_info->package ;
	int ret ;
	
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"MMC get_ro()\n");
	
	mmc_claim_host(package->master_host);
	do {
		/* 実装依存 */
		/* メカニカルスイッチの状態取得が可能な構成であれば、状態取得 */
		
		/* ライトプロテクト検出 */
		if( mn66831_check_write_protect(slot_info) ) {
			Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"<%s : WriteProtected\n", __func__ );
			ret = -ENOSYS ;
		}
		else {
			Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"<%s : NoProtect\n", __func__ );
			ret = 0 ;
		}
	} while( 0 ) ;
	mmc_release_host(package->master_host);
	
	return ret ;
}

/*==============================================================================*/
/*    Function Name    static int mn66831_get_cd( struct mmc_host *mmc ) */
/*------------------------------------------------------------------------------*/
/** @brief      Get Card Detect Information for SdHubSendCommand.(for MMC Linux)
 *  @param[in]  *mmc : information for mmc driver
 *  @return     1:        Card Detect
                -ENOSYS : No Card 
 *  @attention  
*/ 
/*==============================================================================*/
static int mn66831_get_cd( struct mmc_host *mmc ) {
	struct mn66831_slot	*slot_info = mmc_priv(mmc) ;
	struct mn66831_host	*host_info = slot_info->host ;
	struct mn66831_package *package = host_info->package ;
	int ret ;
	
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"MMC get_cd()\n");
	
	do {
		/* SDHUB_Connectによるマスタ側スロット登録があるまでは、カード無しを返す。 */
		if( package->flag_connection == false ) {
			Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"<%s:flag_connection is false\n", __func__ );
			ret = 0 ;
			break ;
		}
		
		mmc_claim_host(package->master_host);
		/* 実装依存 */
		/* カード有無の状態取得が可能な構成であれば、状態取得 */
		/* SDDAT3によるカード検出は非対応 */
		ret = mn66831_check_card( slot_info ) ;
		
		mmc_release_host(package->master_host);
	} while( 0 ) ;
	
	return ret ;
}

/*==============================================================================*/
/*    Function Name   static void mn66831_enable_sdio_irq( struct mmc_host *mmc, int enable ) */
/*------------------------------------------------------------------------------*/
/** @brief      Get Write Protect Information for SdHubSendCommand.(for MMC Linux)
 *  @param[in]  *mmc : information for mmc driver
 *  @param[in]  en : 1: SDIO IRQ Enable 0:SDIO IRQ Disable
 *  @return     void
 *  @attention  
*/ 
/*==============================================================================*/
static void mn66831_enable_sdio_irq( struct mmc_host *mmc, int enable ) {
	struct mn66831_slot	*slot_info = mmc_priv(mmc) ;
	struct mn66831_host	*host_info = slot_info->host ;
	struct mn66831_package *package = host_info->package ;
	
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"MMC enable_sdio_irq()\n");
	mmc_claim_host(package->master_host);
	do {
		host_info = slot_info->host ;
		
	} while( 0 ) ;
	mmc_release_host(package->master_host);
	
	return ;
}

#if			defined(_SDHUB_ENABLE_UHS_I_)
static int mn66831_voltage_switch( struct mmc_host *mmc, struct mmc_ios *ios ) {
	struct mn66831_slot	*slot_info = mmc_priv(mmc) ;
	struct mn66831_host	*host_info = slot_info->host ;
	struct mn66831_package *package = host_info->package ;

	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"MMC mn66831_voltage_switch() ios->signal_voltage = %d\n",ios->signal_voltage);
	mmc_claim_host(package->master_host);
	
	do {
		mn66831_ctrl_card_clock( slot_info, false );
		
		if (ios->signal_voltage == MMC_SIGNAL_VOLTAGE_330) {
			mn66831_ctrl_card_voltage(slot_info, SDHUB_VOLTAGE_3_3V);
		} else {
			mn66831_ctrl_card_voltage(slot_info, SDHUB_VOLTAGE_1_8V);
		}
		
		mn66831_ctrl_card_clock( slot_info, true );
		
		mn66831_delay(1);
		
	} while( 0 ) ;
	mmc_release_host(package->master_host);
	
	return 0;
}
#endif	/*	defined(_SDHUB_ENABLE_UHS_I_)	*/

/** Data Instance : */
static struct mn66831_pdata_slot mn66831_sdhub0_AB_datas[] = {
	{
	.mclk = 100000000,
	.maxclk = 100000000,
	.bus_width = 4,
	.maxDMALength = 0xFFFF,
	.maxBufSize = 256,
	.registerUnitSize = 1,
	.chThisDevice = "MN66831",
	.chMasterDevice = "MN66831",
	.enableStandby = true,
	.enableSuspend = true,
	.enableHighSpeedMode = true,
	.enableUHS1Mode = false,
	.enableDMA = false,
	.enablePowerSave = false,
	.enableLowVolRange = false,
	.enable0VolRange = false,
	.enableWp = true,
	.enableDetectCard = true,
	.enableHub = false,
	.enableSdPathAutoCmd55 = false,
	.enableSdPathAutoCmd12 = true,
	.enableSdPathAutoCmd13 = false,
	},
	{
	.mclk = 100000000,
	.maxclk = 100000000,
	.bus_width = 4,
	.maxDMALength = 0xFFFF,
	.maxBufSize = 256,
	.registerUnitSize = 1,
	.chThisDevice = "MN66831",
	.chMasterDevice = "MN66831",
	.enableStandby = true,
	.enableSuspend = true,
	.enableHighSpeedMode = true,
	.enableUHS1Mode = false,
	.enableDMA = false,
	.enablePowerSave = false,
	.enableLowVolRange = false,
	.enable0VolRange = false,
	.enableWp = true,
	.enableDetectCard = true,
	.enableHub = false,
	.enableSdPathAutoCmd55 = false,
	.enableSdPathAutoCmd12 = true,
	.enableSdPathAutoCmd13 = false,
	},
} ;

/** Data Instance : */
static struct mn66831_pdata_slot mn66831_sdhub0_S_datas[] = {
	{
	.mclk = 26000000,       // 20121023
	.maxclk = 26000000,     // 20121023
	.bus_width = 4,
	.maxDMALength = 0xFFFF,
	.maxBufSize = 256,
	.registerUnitSize = 1,
	.chThisDevice = "MN66831",
	.chMasterDevice = "MN66831",
	.enableStandby = true,
	.enableSuspend = true,
	.enableHighSpeedMode = true,
#if			!defined(_SDHUB_ENABLE_UHS_I_)
	.enableUHS1Mode = false,
#else	/*	!defined(_SDHUB_ENABLE_UHS_I_)	*/
	.enableUHS1Mode = true,
#endif	/*	!defined(_SDHUB_ENABLE_UHS_I_)	*/
	.enableDMA = false,
	.enablePowerSave = false,
#if			!defined(_SDHUB_ENABLE_UHS_I_)
	.enableLowVolRange = false,
	.enable0VolRange = false,
#else	/*	!defined(_SDHUB_ENABLE_UHS_I_)	*/
	.enableLowVolRange = true,
	.enable0VolRange = true,
#endif	/*	!defined(_SDHUB_ENABLE_UHS_I_)	*/
	.enableWp = true,
	.enableDetectCard = true,
	.enableHub = false,
	.enableSdPathAutoCmd55 = false,
	.enableSdPathAutoCmd12 = true,
	.enableSdPathAutoCmd13 = false,
	},
} ;

/** Data Instance */
static struct mn66831_pdata_host	mn66831_sdhub0_AB_conf = {
	.name = "MN66831",
	.id   = MN66831_HUB0_ID,
	.slot = mn66831_sdhub0_AB_datas,
	.nr_parts = ARRAY_SIZE(mn66831_sdhub0_AB_datas),
} ;

/** Data Instance */
static struct mn66831_pdata_host	mn66831_sdhub0_S_conf = {
	.name = "MN66831",
	.id   = MN66831_HUB0_ID,
	.slot = mn66831_sdhub0_S_datas,
	.nr_parts = ARRAY_SIZE(mn66831_sdhub0_S_datas),
} ;

/** Data Instance */
struct platform_device mn66831_sdhub0_AB = {
	.name					= "SDHUBDEVICE",
	.id						= 0,
	.dev = {
		.platform_data 		= &mn66831_sdhub0_AB_conf,
	},
};

/** Data Instance */
struct platform_device mn66831_sdhub0_S = {
	.name					= "SDHUBDEVICE",
	.id						= 1,
	.dev = {
		.platform_data 		= &mn66831_sdhub0_S_conf,
        .init_name          = "sdhub",
	},
};

/** Data Instance */
static struct platform_device *SdHubDeviceList[] = {
	&mn66831_sdhub0_AB,
	&mn66831_sdhub0_S,
};

/** Data Instance */
struct mn66831_platform_device SdHubDevices = {
	.conf_data = SdHubDeviceList,
	.nr_devices = ARRAY_SIZE(SdHubDeviceList),
};

/** Data Instance */
static struct mmc_host_ops mn66831_ops = {
	.request			= mn66831_request,
	.set_ios			= mn66831_set_ios,
	.get_ro				= mn66831_get_ro,
	.get_cd				= mn66831_get_cd,
	.enable_sdio_irq	= mn66831_enable_sdio_irq,
#if			defined(_SDHUB_ENABLE_UHS_I_)
	.start_signal_voltage_switch	= mn66831_voltage_switch,
#endif	/*	defined(_SDHUB_ENABLE_UHS_I_)	*/
};


/*==============================================================================*/
/*  Function Name   void mn66831_slot_init(struct mn66831_host *host, struct mn66831_slot *slot,  struct mn66831_pdata_slot *slot_data, int slotno) */
/*------------------------------------------------------------------------------*/
/** @brief      Init for slot configuration data
 *  @param[in]  *host        : struct for SDD Host
 *  @param[in]  slot         : Pratform Configuration Data by slot
 *  @param[in]  *slot_data : struct for Slot Data in this Platform.
 *  @param[in]  slotNo       : Port No.
 *  @return     void
 *  @attention  
*/ 
/*==============================================================================*/
void mn66831_slot_init(struct mn66831_host *host_info, struct mn66831_slot *slot_info,  struct mn66831_pdata_slot *slot_data, int slotno) {
	
    slot_info->blMedia = false;
    slot_info->appcmd = false;
    slot_info->clock = 0;
    slot_info->mclk             =  slot_data->mclk;
    slot_info->maxclk           =  slot_data->maxclk;
    slot_info->maxDMALength     =  slot_data->maxDMALength;
    slot_info->maxBufSize       =  slot_data->maxBufSize;
    slot_info->registerUnitSize =  slot_data->registerUnitSize;
    slot_info->chThisDevice     = slot_data->chThisDevice;
    slot_info->chMasterDevice   = slot_data->chMasterDevice;
    slot_info->slotNo = slotno;
    slot_info->pdata = slot_data;

	/* スロットの初期状態をセット */
	slot_info->ios.clock = 0 ;
	slot_info->ios.vdd = 0 ;
	slot_info->ios.bus_mode = MMC_BUSMODE_OPENDRAIN ;
	slot_info->ios.chip_select = MMC_CS_DONTCARE ;
	slot_info->ios.power_mode = MMC_POWER_OFF ;
	slot_info->ios.bus_width = MMC_BUS_WIDTH_4 ;
//	slot_info->ios.bus_width = MMC_BUS_WIDTH_1 ;
	slot_info->ios.timing = MMC_TIMING_LEGACY ;

	slot_info->host = host_info;
	host_info->slot[slotno] = slot_info;
	
}


/*==============================================================================*/
/*  Function Name   void mn66831_slot_exit(struct mn66831_host *host, struct mn66831_slot *slot,  struct mn66831_pdata_slot *slot_data, int slotno) */
/*------------------------------------------------------------------------------*/
/** @brief      Init for slot configuration data
 *  @param[in]  *host        : struct for SDD Host
 *  @param[in]  slot         : Pratform Configuration Data by slot
 *  @param[in]  *slot_data : struct for Slot Data in this Platform.
 *  @param[in]  slotNo       : Port No.
 *  @return     void
 *  @attention  
*/ 
/*==============================================================================*/
void mn66831_slot_exit(struct mn66831_host *host_info, struct mn66831_slot *slot_info,  struct mn66831_pdata_slot *slot_data, int slotno) {
	
	slot_info->host = host_info;
	host_info->slot[slotno] = NULL ;
	
}

#define K3V2_MAX_SEGS	8	/* MAX 64KByte */
#define K3V2_DMA_SG_NUM 256

/*==============================================================================*/
/*    Function Name   int mn66831_append_slot( struct mn66831_host *host_info, struct mn66831_pdata_slot *slot_data, unsigned int id ) */
/*------------------------------------------------------------------------------*/
/** @brief      Append slot information to host information.
 *  @param[in]  *host_info : host information
 *  @param[in]  *slot_data : slot unique information
 *  @param[in]  id         : slot number
 *  @return     void
 *  @attention  
*/ 
/*==============================================================================*/
int mn66831_append_slot( struct mn66831_host *host_info, struct mn66831_pdata_slot *slot_data, unsigned int id ) {
	struct mmc_host *mmc ;
	struct mn66831_slot *slot_info ;
	int ret ;
	
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:host_info=0x%08X\n", __func__, (unsigned int)host_info );
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:slot_data=0x%08X\n", __func__, (unsigned int)slot_data );
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:id=%u\n", __func__, id );
	
	/* MN66831のスロットを生成 */
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:mmc_alloc_host>\n", __func__ );
	mmc = mmc_alloc_host(sizeof(struct mn66831_slot), &host_info->pdev->dev);
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:<mmc_alloc_host\n", __func__ );
	if( !mmc ) {
		Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:mmc_alloc_host\n", __func__ );
		return -ENOMEM ;
	}
	
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:mmc=0x%08X\n", __func__, (unsigned int)mmc );
	slot_info = mmc_priv( mmc ) ;
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:slot_info=0x%08X\n", __func__, (unsigned int)slot_info );
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:host_info->slot=0x%08X\n", __func__, (unsigned int)host_info->slot );
	slot_info->upHost = mmc ;
	/*
	 * Set slot data by Platform configuration.
	 */
	mn66831_slot_init( host_info, slot_info, slot_data, id ) ;
	
	mmc->ops   = &mn66831_ops;
	mmc->f_max = slot_data->maxclk ;
//	mmc->f_min = slot_data->mclk / 512 ;
	mmc->f_min = 390625 ;						/* 391kHz */
	mmc->ocr_avail = MMC_VDD_33_34 ;
	
	if( slot_data->bus_width >= 4 ) {
		mmc->caps |= MMC_CAP_4_BIT_DATA ;
	}
	if( !slot_data->enableDetectCard ) {
		//mmc->caps |= MMC_CAP_NEEDS_POLL ; 
		/* Polling有効にすると、coreよりget_cdが周期的(1sec毎)に呼ばれてくるため、無効化 */
	}
	if( slot_data->enableHighSpeedMode ) {
		mmc->caps |= MMC_CAP_MMC_HIGHSPEED ;
		mmc->caps |= MMC_CAP_SD_HIGHSPEED ;
	}
#if			defined(_SDHUB_ENABLE_UHS_I_)
	if( slot_data->enableUHS1Mode ) {
		mmc->caps |= MMC_CAP_UHS_SDR12 ;
		mmc->caps |= MMC_CAP_UHS_SDR25 ;
		mmc->caps |= MMC_CAP_UHS_SDR50 ;
	}
#endif	/*	defined(_SDHUB_ENABLE_UHS_I_)	*/
	if( !slot_data->enablePowerSave ) {
//		mmc->caps = MMC_CAP_4_BIT_DATA | MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED ;
		/* mmc->caps 設定保留 → MMC_CAP_SDIO_IRQ  MMC_CAP_SPI  MMC_CAP_NEEDS_POLL  MMC_CAP_8_BIT_DATA */
	}

	/* MMC core transfer sizes tunable parameters */
	/* ★マスター側のスロットのパラメータに合わせる必要あり。 */
#ifdef CONFIG_MMC_BLOCK_BOUNCE
#if			defined(_SDHUB_OLD_SEGS_)
	mmc->max_hw_segs   = 1;
	mmc->max_phys_segs = 1;
#else	/*	defined(_SDHUB_OLD_SEGS_)	*/
	mmc->max_segs      = 1;
#endif	/*	defined(_SDHUB_OLD_SEGS_)	*/
	mmc->max_blk_size  = 512;
	mmc->max_blk_count = 4096;
	mmc->max_req_size  = mmc->max_blk_size * mmc->max_blk_count;/* 2048KB */
	mmc->max_seg_size  = mmc->max_req_size;/* 2048KB */
#else
#if			defined(_SDHUB_OLD_SEGS_)
	mmc->max_hw_segs   = K3V2_MAX_SEGS;
	mmc->max_phys_segs = K3V2_MAX_SEGS;
#else	/*	defined(_SDHUB_OLD_SEGS_)	*/
	mmc->max_segs      = K3V2_DMA_SG_NUM;
#endif	/*	defined(_SDHUB_OLD_SEGS_)	*/
	mmc->max_blk_size  = 512;
	mmc->max_blk_count = 0xffff;
	mmc->max_req_size  = 0x80000;
	mmc->max_seg_size  = 4096;
#endif /* CONFIG_MMC_BLOCK_BOUNCE */

	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:mmc_add_host>\n", __func__ );
	ret = mmc_add_host(mmc);
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:<mmc_add_host\n", __func__ );
	if( ret ) {
		mmc_free_host(mmc) ;
		return ret ;
	}
	
	/* mn66831_host構造体とmn66831_slot構造体を紐付け */
	host_info->slot[id] = slot_info ;
	
	return 0 ;
}

/*==============================================================================*/
/*    Function Name   void mn66831_remove_slot( struct mn66831_host *host_info, struct mn66831_pdata_slot *slot_data, unsigned int id ) */
/*------------------------------------------------------------------------------*/
/** @brief      Remove slot information from host information.
 *  @param[in]  *host_info : host information
 *  @param[in]  *slot_data : slot unique information
 *  @param[in]  id         : slot number
 *  @return     void
 *  @attention  
*/ 
/*==============================================================================*/
void mn66831_remove_slot( struct mn66831_host *host_info, struct mn66831_pdata_slot *slot_data, unsigned int id ) {
	struct mn66831_slot *slot_info ;
	
	slot_info = host_info->slot[id] ;
	
	mn66831_slot_exit( host_info, slot_info, slot_data, id ) ;
	
	mmc_remove_host(slot_info->upHost);
	mmc_free_host(slot_info->upHost);
	
	host_info->slot[id] = NULL ;
	
}

/*==============================================================================*/
/*    Function Name   void SDHUB_Connect( struct mmc_host *master_host, unsigned int package_id ) */
/*------------------------------------------------------------------------------*/
/** @brief      Connect master slot and slave package.
 *  @param[in]  *master_host : information for mmc driver
 *  @param[in]  package_id   : ID# to specify a package.
 *  @return     void
 *  @attention  
*/ 
/*==============================================================================*/
void SDHUB_Connect( struct mmc_host *master_host, unsigned int package_id ) {
	struct mn66831_package *package = NULL ;
	struct mn66831_host *host_info;
	struct mn66831_slot *slot_info;
	int i,j;
    int ret;
#if 1/*delete this for hotplug function*/
	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s\n", __func__ );
    ret	= gpio_direction_output(GPIO_RESET, 0);    // pull up XMNRST
	if (ret) {
    	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:gpio_direction_output GPIO_RESET Fail \n", __func__ );
		return;
	}
	ret	= gpio_direction_output(GPIO_VDDIO_EN, 0); // open vddio and vddios
	if (ret) {
    	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:gpio_direction_output GPIO_VDDIO_EN Fail \n", __func__ );
		return;
	}
    ret = blockmux_set(cprm_iomux_block, cprm_iomux_config, NORMAL);
	if (ret) {
    	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:blockmux_set() Error : ret = %d\n", __func__, ret );
		return;
	}
    vddio_status = 1;    
    gpio_set_value(GPIO_VDDIO_EN, vddio_status);
#endif
	/* マスター側スロット情報と接続したSD Hubの設定を受け取り、それに従ってスロットを生成 */
	if( package_id >= SDHUB_PACKAGE_COUNT ) {
		return ;
	}
	MN66831_GET_PACKAGE( package_id, package );
    if ( NULL == package ) {
		Sdhubs_DebugPrint(__LINE__, SDHUB_PERR, "%s: NULL == package \n", __func__ );
        return;
    }

	package->master_host = master_host ;
	package->flag_connection = true ;
	package->num_devices = MN66831_NUM_OF_DEVICES ;
	
	/* 全スロットに対してカード有無チェック */
	/* この時点では、マスタ側、スレーブ側共に電源OFFのため、レジスタ読出しによるカード検出は不可 */
	for(i=0;i<package->num_devices;i++){
		host_info = package->device_info[i] ;
		if( !host_info ) {
			continue ;
		}
		
		for(j=0;j<host_info->num_slots;j++){
			slot_info = host_info->slot[j] ;
			
#if 1
			/* マスター側スロットのパラメータに補正 */
			slot_info->upHost->max_seg_size  = master_host->max_seg_size ;
			//slot_info->upHost->max_hw_segs   = master_host->max_hw_segs ;
			//slot_info->upHost->max_phys_segs = master_host->max_phys_segs ;
			slot_info->upHost->max_segs      = master_host->max_segs;
            slot_info->upHost->max_req_size  = master_host->max_req_size ;
			slot_info->upHost->max_blk_size  = master_host->max_blk_size ;
			slot_info->upHost->max_blk_count = master_host->max_blk_count ;
			Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:mmc(host[%d]->slot[%d])\n", __func__, i, j );
			Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:mmc->max_seg_size  = %u\n", __func__, master_host->max_seg_size );
			//Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:mmc->max_hw_segs   = %u\n", __func__, master_host->max_hw_segs );
			//Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:mmc->max_phys_segs = %u\n", __func__, master_host->max_phys_segs );
			Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:mmc->max_segs  = %u\n", __func__, master_host->max_segs );
			Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:mmc->max_req_size  = %u\n", __func__, master_host->max_req_size );
			Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:mmc->max_blk_size  = %u\n", __func__, master_host->max_blk_size );
			Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:mmc->max_blk_count = %u\n", __func__, master_host->max_blk_count );
#endif
			/* カード挿抜状態変化を通知 */
			Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:mmc_detect_change(host[%d]->slot[%d])\n", __func__, i, j );
			mmc_detect_change(slot_info->upHost,msecs_to_jiffies(100));	/* delay時間は要精査 */
			Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s :host[%d]->slot[%d]:CardExist\n", __func__, i, j );
		}
	}
	
	return ;
}
EXPORT_SYMBOL(SDHUB_Connect);



void SDHUB_Connect_hotplug( struct mmc_host *master_host, unsigned int package_id ) {
	struct mn66831_package *package = NULL ;
	struct mn66831_host *host_info;
	struct mn66831_slot *slot_info;
	int i,j;
    int ret;
#if 0/*delete this for hotplug function*/
	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s\n", __func__ );
    ret	= gpio_direction_output(GPIO_RESET, 0);    // pull up XMNRST
	if (ret) {
    	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:gpio_direction_output GPIO_RESET Fail \n", __func__ );
		return;
	}
	ret	= gpio_direction_output(GPIO_VDDIO_EN, 0); // open vddio and vddios
	if (ret) {
    	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:gpio_direction_output GPIO_VDDIO_EN Fail \n", __func__ );
		return;
	}
    ret = blockmux_set(cprm_iomux_block, cprm_iomux_config, NORMAL);
	if (ret) {
    	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:blockmux_set() Error : ret = %d\n", __func__, ret );
		return;
	}
    vddio_status = 1;    
    gpio_set_value(GPIO_VDDIO_EN, vddio_status);
#endif
	/* マスター側スロット情報と接続したSD Hubの設定を受け取り、それに従ってスロットを生成 */
	if( package_id >= SDHUB_PACKAGE_COUNT ) {
		return ;
	}
	MN66831_GET_PACKAGE( package_id, package );
    if ( NULL == package ) {
		Sdhubs_DebugPrint(__LINE__, SDHUB_PERR, "%s: NULL == package \n", __func__ );
        return;
    }

	package->master_host = master_host ;
	package->flag_connection = true ;
	package->num_devices = MN66831_NUM_OF_DEVICES ;
	
	/* 全スロットに対してカード有無チェック */
	/* この時点では、マスタ側、スレーブ側共に電源OFFのため、レジスタ読出しによるカード検出は不可 */
	for(i=0;i<package->num_devices;i++){
		host_info = package->device_info[i] ;
		if( !host_info ) {
			continue ;
		}
		
		for(j=0;j<host_info->num_slots;j++){
			slot_info = host_info->slot[j] ;
			
#if 1
			/* マスター側スロットのパラメータに補正 */
			slot_info->upHost->max_seg_size  = master_host->max_seg_size ;
			//slot_info->upHost->max_hw_segs   = master_host->max_hw_segs ;
			//slot_info->upHost->max_phys_segs = master_host->max_phys_segs ;
			slot_info->upHost->max_segs      = master_host->max_segs;
            slot_info->upHost->max_req_size  = master_host->max_req_size ;
			slot_info->upHost->max_blk_size  = master_host->max_blk_size ;
			slot_info->upHost->max_blk_count = master_host->max_blk_count ;
			Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:mmc(host[%d]->slot[%d])\n", __func__, i, j );
			Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:mmc->max_seg_size  = %u\n", __func__, master_host->max_seg_size );
			//Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:mmc->max_hw_segs   = %u\n", __func__, master_host->max_hw_segs );
			//Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:mmc->max_phys_segs = %u\n", __func__, master_host->max_phys_segs );
			Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:mmc->max_segs  = %u\n", __func__, master_host->max_segs );
			Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:mmc->max_req_size  = %u\n", __func__, master_host->max_req_size );
			Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:mmc->max_blk_size  = %u\n", __func__, master_host->max_blk_size );
			Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:mmc->max_blk_count = %u\n", __func__, master_host->max_blk_count );
#endif
			/* カード挿抜状態変化を通知 */
			Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:mmc_detect_change(host[%d]->slot[%d])\n", __func__, i, j );
			mmc_detect_change(slot_info->upHost,msecs_to_jiffies(100));	/* delay時間は要精査 */
			Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s :host[%d]->slot[%d]:CardExist\n", __func__, i, j );
		}
	}
	
	return ;
}
EXPORT_SYMBOL(SDHUB_Connect_hotplug);



int SDHUB_Set_LowPower_Mode(void)
{
    int ret;

	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:Enter\n", __func__ );
    ret	= gpio_direction_input(GPIO_RESET);
	if (ret) {
    	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:gpio_direction_input GPIO_RESET Fail \n", __func__ );
    	return -1;
	}
    ret	= gpio_direction_input(GPIO_VDDIO_EN);
	if (ret) {
    	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:gpio_direction_input GPIO_VDDIO_EN Fail \n", __func__ );
    	return -1;
	}
    ret = blockmux_set(cprm_iomux_block, cprm_iomux_config, LOWPOWER);
	if (ret) {
    	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:blockmux_set() Error : ret = %d\n", __func__, ret );
    	return -1;
	}

    return 0;
}
EXPORT_SYMBOL(SDHUB_Set_LowPower_Mode);

/*==============================================================================*/
/*    Function Name   void SDHUB_Disconnect( struct mmc_host *master_host, unsigned int package_id ) */
/*------------------------------------------------------------------------------*/
/** @brief      Disconnect master slot and slave package.
 *  @param[in]  *master_host : information for mmc driver
 *  @param[in]  package_id   : ID# to specify a package.
 *  @return     void
 *  @attention  
*/ 
/*==============================================================================*/
void SDHUB_Disconnect( struct mmc_host *master_host, unsigned int package_id ) {
	struct mn66831_package *package = NULL ;
	struct mn66831_pdata_host *pdata = NULL ;
	struct mn66831_host *host_info;
	struct mn66831_slot *slot_info;
	int i,j;
    int ret;

	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s\n", __func__ );
	if( package_id >= SDHUB_PACKAGE_COUNT ) {
		return ;
	}
	MN66831_GET_PACKAGE( package_id, package );
    if ( NULL == package ) {
		Sdhubs_DebugPrint(__LINE__, SDHUB_PERR, "%s: NULL == package \n", __func__ );
        return;
    }

    ret	= gpio_direction_input(GPIO_RESET);
	if (ret) {
    	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:gpio_direction_input GPIO_RESET Fail \n", __func__ );
		return;
	}
	ret	= gpio_direction_input(GPIO_VDDIO_EN);
	if (ret) {
    	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:gpio_direction_input GPIO_VDDIO_EN Fail \n", __func__ );
		return;
	}
    ret = blockmux_set(cprm_iomux_block, cprm_iomux_config, LOWPOWER);
	if (ret) {
    	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:blockmux_set() Error : ret = %d\n", __func__, ret );
		return;
	}
	
	//package->master_host = NULL ;
	
	//package->master_host = NULL ;
	package->flag_connection = false ;
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"<%s : package->flag_connection = false is delete\n", __func__ );
	
	
	/* 全スロットに対してカードチェック */
	/* 関連付け解除の時点で全てのカードが抜けたことにする */
	for(i=0;i<package->num_devices;i++){
		host_info = package->device_info[i] ;
		if( !host_info ) {
			continue ;
		}
		
		pdata = host_info->pdev->dev.platform_data ;
		for(j=0;j<host_info->num_slots;j++){
			slot_info = host_info->slot[j] ;

			/* カード挿抜状態変化を通知 */
			Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:mmc_detect_change(host[%d]->slot[%d])\n", __func__, i, j );
			mmc_detect_change(slot_info->upHost,msecs_to_jiffies(100));	/* delay時間 100msec */
			Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"<%s : NoCard\n", __func__ );
		}
	}
	
	return ;
}
EXPORT_SYMBOL(SDHUB_Disconnect);




#if			defined(_SDHUB_EXTENSION_)
/*==============================================================================*/
/*    Function Name   void *SDHUB_SendCommand( int slot, unsigned int cmd, unsigned int arg, unsigned int *resp, void *buff, unsigned int size, unsigned int timeout, int *error ) */
/*------------------------------------------------------------------------------*/
/** @brief      Create/Get package information.
 *  @param[in]  slot       : slot identifier ( fixed to 1 )
 *  @param[in]  cmd        : command
 *  @param[in]  arg        : command argument(32bit)
 *  @param[in]  *resp      : command response(32bit)
 *  @param[in]  *buff      : read/write buffer
 *  @param[in]  size       : read/write buffer size
 *  @param[in]  timeout    : timeout value(ns)
 *  @param[in]  *error     : error information
 *  @return     void
 *  @attention  
 */ 
/*==============================================================================*/
void SDHUB_SendCommand( int slot, unsigned int cmd, unsigned int arg, unsigned int *resp,
						void *buff, unsigned int size, unsigned int dir, unsigned int timeout, int *error ) {
	struct mn66831_package *package = NULL ;
	struct mn66831_host *host_info;
	struct mn66831_slot *slot_info;
	unsigned int package_id = 0;
	
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"EXT request(%d,0x%08X)\n", cmd, arg);
	
	/* SDHUB_SeqLock()で『ロックしている状態』で呼び出すこと */
	do{
		if((slot!=1)||(!resp)||(!error)){
			Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:CHECK - slot(%d), resp(0x%08X), error(0x%08X)\n", __func__, slot, (unsigned int)resp, (unsigned int)error );
			break ;
		}
		MN66831_GET_PACKAGE( package_id, package );
        if ( NULL == package ) {
    		Sdhubs_DebugPrint(__LINE__, SDHUB_PERR, "%s: NULL == package \n", __func__ );
            return;
        }
		if(!(package->device_info)){
			Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:CHECK - package->device_info(0x%08X)\n", __func__, (unsigned int)package->device_info );
			break ;
		}
		host_info = package->device_info[1] ;
		if(!(host_info->slot)) {
			Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:CHECK - host_info->slot(0x%08X)\n", __func__, (unsigned int)host_info->slot );
			break ;
		}
		slot_info = host_info->slot[0] ;
		if(!(slot_info->upHost)) {
			Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:CHECK - slot_info->upHost(0x%08X)\n", __func__, (unsigned int)slot_info->upHost );
			break ;
		}
		
		mn66831_send_control_command(slot_info, cmd, arg, resp, buff, size, dir, timeout, error );
		
	}while(0) ;
	/* SDHUB_SeqLock()で『ロックしている状態』で呼び出すこと */
	
	return ;
}
EXPORT_SYMBOL(SDHUB_SendCommand);

/*==============================================================================*/
/*    Function Name   void SDHUB_GetRCA( int slot, void *rca ) */
/*------------------------------------------------------------------------------*/
/** @brief      Create/Get package information.
 *  @param[in]  slot       : slot identifier ( fixed to 1 )
 *  @param[out] *rca       : RCA Register
 *  @return     void
 *  @attention  
 */ 
/*==============================================================================*/
void SDHUB_GetRCA( int slot, void *rca ) {
	struct mn66831_package *package = NULL ;
	struct mn66831_host *host_info;
	struct mn66831_slot *slot_info;
	unsigned int package_id = 0;
	
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"EXT GetRCA()\n");
	
	/* SDHUB_SeqLock()で『ロックしている状態』で呼び出すこと */
	do{
		if((slot!=1)||(!rca)){
			Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:CHECK - slot(%d), rca(0x%08X)\n", __func__, slot, (unsigned int)rca );
			break ;
		}
		MN66831_GET_PACKAGE( package_id, package );
        if ( NULL == package ) {
    		Sdhubs_DebugPrint(__LINE__, SDHUB_PERR, "%s: NULL == package \n", __func__ );
            return;
        }
		if(!(package->device_info)){
			Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:CHECK - package->device_info(0x%08X)\n", __func__, (unsigned int)package->device_info );
			break ;
		}
		host_info = package->device_info[1] ;
		if(!(host_info->slot)) {
			Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:CHECK - host_info->slot(0x%08X)\n", __func__, (unsigned int)host_info->slot );
			break ;
		}
		slot_info = host_info->slot[0] ;
		if(!(slot_info->upHost)) {
			Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:CHECK - slot_info->upHost(0x%08X)\n", __func__, (unsigned int)slot_info->upHost );
			break ;
		}
		if(!(slot_info->upHost->card)) {
			Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:CHECK - slot_info->upHost->card(0x%08X)\n", __func__, (unsigned int)slot_info->upHost->card );
			break ;
		}
		
		//*rca = slot_info->upHost->card->rca ;
#ifdef CONFIG_ARCH_EMXX
		memcpy( rca, &(slot_info->upHost->card[0]->rca), SDHUB_SIZE_RCA );
#else /* CONFIG_ARCH_EMXX */
		memcpy( rca, &(slot_info->upHost->card->rca), SDHUB_SIZE_RCA );
#endif /* CONFIG_ARCH_EMXX */
		
	}while(0) ;
	/* SDHUB_SeqLock()で『ロックしている状態』で呼び出すこと */
	
	return ;
}
EXPORT_SYMBOL(SDHUB_GetRCA);

/*==============================================================================*/
/*    Function Name   void SDHUB_GetCSD( int slot, void *csd ) */
/*------------------------------------------------------------------------------*/
/** @brief      Create/Get package information.
 *  @param[in]  slot       : slot identifier ( fixed to 1 )
 *  @param[out] *csd       : CSD Register
 *  @return     void
 *  @attention  
 */ 
/*==============================================================================*/
void SDHUB_GetCSD( int slot, void *csd ) {
	struct mn66831_package *package = NULL ;
	struct mn66831_host *host_info;
	struct mn66831_slot *slot_info;
	unsigned int package_id = 0;
	
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"EXT GetCSD()\n");
	
	/* SDHUB_SeqLock()で『ロックしている状態』で呼び出すこと */
	do{
		if((slot!=1)||(!csd)){
			Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:CHECK - slot(%d), csd(0x%08X)\n", __func__, slot, (unsigned int)csd );
			break ;
		}
		MN66831_GET_PACKAGE( package_id, package );
        if ( NULL == package ) {
    		Sdhubs_DebugPrint(__LINE__, SDHUB_PERR, "%s: NULL == package \n", __func__ );
            return;
        }
		if(!(package->device_info)){
			Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:CHECK - package->device_info(0x%08X)\n", __func__, (unsigned int)package->device_info );
			break ;
		}
		host_info = package->device_info[1] ;
		if(!(host_info->slot)) {
			Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:CHECK - host_info->slot(0x%08X)\n", __func__, (unsigned int)host_info->slot );
			break ;
		}
		slot_info = host_info->slot[0] ;
		if(!(slot_info->upHost)) {
			Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:CHECK - slot_info->upHost(0x%08X)\n", __func__, (unsigned int)slot_info->upHost );
			break ;
		}
		if(!(slot_info->upHost->card)) {
			Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:CHECK - slot_info->upHost->card(0x%08X)\n", __func__, (unsigned int)slot_info->upHost->card );
			break ;
		}
//		*csd = slot_info->upHost->card->raw_csd[0-4] ;
#ifdef CONFIG_ARCH_EMXX
		memcpy( csd, slot_info->upHost->card[0]->raw_csd, SDHUB_SIZE_CSD ) ;
#else  /* CONFIG_ARCH_EMXX */
		memcpy( csd, slot_info->upHost->card->raw_csd, SDHUB_SIZE_CSD ) ;
#endif /* CONFIG_ARCH_EMXX */
	}while(0) ;
	/* SDHUB_SeqLock()で『ロックしている状態』で呼び出すこと */
	
	return ;
}
EXPORT_SYMBOL(SDHUB_GetCSD);

/*==============================================================================*/
/*    Function Name   void SDHUB_GetCID( int slot, void *cid ) */
/*------------------------------------------------------------------------------*/
/** @brief      Create/Get package information.
 *  @param[in]  slot       : slot identifier ( fixed to 1 )
 *  @param[out] *cid       : CID Register
 *  @return     void
 *  @attention  
 */ 
/*==============================================================================*/
void SDHUB_GetCID( int slot, void *cid ) {
	struct mn66831_package *package = NULL ;
	struct mn66831_host *host_info;
	struct mn66831_slot *slot_info;
	unsigned int package_id = 0;
	
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"EXT GetCID()\n");
	
	/* SDHUB_SeqLock()で『ロックしている状態』で呼び出すこと */
	do{
		if((slot!=1)||(!cid)){
			Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:CHECK - slot(%d), cid(0x%08X)\n", __func__, slot, (unsigned int)cid );
			break ;
		}
		MN66831_GET_PACKAGE( package_id, package );
        if ( NULL == package ) {
    		Sdhubs_DebugPrint(__LINE__, SDHUB_PERR, "%s: NULL == package \n", __func__ );
            return;
        }
		if(!(package->device_info)){
			Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:CHECK - package->device_info(0x%08X)\n", __func__, (unsigned int)package->device_info );
			break ;
		}
		host_info = package->device_info[1] ;
		if(!(host_info->slot)) {
			Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:CHECK - host_info->slot(0x%08X)\n", __func__, (unsigned int)host_info->slot );
			break ;
		}
		slot_info = host_info->slot[0] ;
		if(!(slot_info->upHost)) {
			Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:CHECK - slot_info->upHost(0x%08X)\n", __func__, (unsigned int)slot_info->upHost );
			break ;
		}
		if(!(slot_info->upHost->card)) {
			Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:CHECK - slot_info->upHost->card(0x%08X)\n", __func__, (unsigned int)slot_info->upHost->card );
			break ;
		}
//		*cid = slot_info->upHost->card->raw_cid[0-4] ;
#ifdef CONFIG_ARCH_EMXX
		memcpy( cid, slot_info->upHost->card[0]->raw_cid, SDHUB_SIZE_CID ) ;
#else
		memcpy( cid, slot_info->upHost->card->raw_cid, SDHUB_SIZE_CID ) ;
#endif		
	}while(0) ;
	/* SDHUB_SeqLock()で『ロックしている状態』で呼び出すこと */
	
	return ;
}
EXPORT_SYMBOL(SDHUB_GetCID);
static struct wake_lock SDHUB_SeqLock_wake_lock;
/*==============================================================================*/
/*    Function Name   void SDHUB_SeqLock( int lock ) */
/*------------------------------------------------------------------------------*/
/** @brief      
 *  @param[in]  lock       : lock/unclock flag( 0:unlock, 1:lock )
 *  @return     void
 *  @attention  
 */ 
/*==============================================================================*/
void SDHUB_SeqLock( int lock ) {
	struct mn66831_package *package = NULL ;
	struct mn66831_host *host_info;
	struct mn66831_slot *slot_info;
	unsigned int package_id = 0;
	int i, j;
	
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"EXT SeqLock(%d)\n",lock);
	if(lock)
	{
		printk("SDHUB_SeqLock wake_lock\n");
	    wake_lock(&SDHUB_SeqLock_wake_lock);
	}
	/* mmc_claim_host/mmc_release_hostでmn66831の全てのmmc_hostをlock/unlockする */
	do{
		MN66831_GET_PACKAGE( package_id, package );
        if ( NULL == package ) {
    		Sdhubs_DebugPrint(__LINE__, SDHUB_PERR, "%s: NULL == package \n", __func__ );
            return;
        }
		if( !lock ) {
			mmc_release_host(package->master_host);
		}
		for(i=0;i<package->num_devices;i++){
			host_info = package->device_info[i] ;
			if( !host_info ) {
				continue ;
			}
			for(j=0;j<host_info->num_slots;j++){
				slot_info = host_info->slot[j] ;
				if( ( !slot_info ) || ( !(slot_info->upHost) ) ) {
					continue ;
				}
				/* lock変数によりlock/unlock実行 */
				if( lock ) {
					mmc_claim_host( slot_info->upHost );
//					Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:mmc_claim_host[%d/%d]\n", __func__, i, j );
				}
				else {
//					Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:mmc_release_host[%d/%d]\n", __func__, i, j );
					mmc_release_host( slot_info->upHost );
				}
			}
		}
		if( lock ) {
			mmc_claim_host(package->master_host);
		}
	}while(0) ;
	
	if(!lock)
	{
		printk("SDHUB_SeqLock wake_unlock\n");
	    wake_unlock(&SDHUB_SeqLock_wake_lock);
	}
	
	return ;
}
EXPORT_SYMBOL(SDHUB_SeqLock);


/*==============================================================================*/
/*    Function Name   void SDHUB_Suspend( int suspend ) */
/*------------------------------------------------------------------------------*/
/** @brief      
 *  @param[in]  suspend   : 0 - Suspend Disable, 1 - Suspend Enable
 *  @return     void
 *  @attention  
 */ 
/*==============================================================================*/
void SDHUB_Suspend( int suspend ) {
	struct mn66831_package *package = NULL ;
	struct mn66831_host *host_info;
	struct mn66831_slot *slot_info;
	unsigned int package_id = 0;
	
	/* SDHUB_SeqLock()で『ロックしている状態』で呼び出すこと */
	do{
		MN66831_GET_PACKAGE( package_id, package );
        if ( NULL == package ) {
    		Sdhubs_DebugPrint(__LINE__, SDHUB_PERR, "%s: NULL == package \n", __func__ );
            return;
        }
		if(!(package->device_info)){
			Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:CHECK - package->device_info(0x%08X)\n", __func__, (unsigned int)package->device_info );
			break ;
		}
		host_info = package->device_info[1] ;
		if(!(host_info->slot)) {
			Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:CHECK - host_info->slot(0x%08X)\n", __func__, (unsigned int)host_info->slot );
			break ;
		}
		slot_info = host_info->slot[0] ;
		if(!(slot_info->upHost)) {
			Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:CHECK - slot_info->upHost(0x%08X)\n", __func__, (unsigned int)slot_info->upHost );
			break ;
		}
		
		/* MN66831サスペンドモード設定 */
		mn66831_ctrl_suspend( slot_info, suspend ) ;
		
	}while(0) ;
	/* SDHUB_SeqLock()で『ロックしている状態』で呼び出すこと */
	
	return ;
}
EXPORT_SYMBOL(SDHUB_Suspend);
#endif	/*	defined(_SDHUB_EXTENSION_)	*/


/*==============================================================================*/
/*    Function Name   struct mn66831_package *mn66831_get_package( struct platform_device *pdev, struct mn66831_host *host_info ) */
/*------------------------------------------------------------------------------*/
/** @brief      Create/Get package information.
 *  @param[in]  *pdev      : information for mmc driver
 *  @param[in]  *host_info : host information
 *  @return     package information
 *  @attention  
 */ 
/*==============================================================================*/
struct mn66831_package *mn66831_get_package( struct platform_device *pdev, struct mn66831_host *host_info ) {
	struct mn66831_pdata_host *pdata = NULL ;
	struct mn66831_package *package = NULL ;
	unsigned int profile_id ;
	unsigned int num_devices = 0 ;
	int i;
	
	if(!pdev){
		return NULL ;
	}
	
	/* pdevに紐付けされたstruct mn66831_pdata_host型を取得 */
	pdata = pdev->dev.platform_data ;
	if( !pdata ) {
		return NULL ;
	}
	
	/* Profile ID for MN66831 */
	profile_id = MN66831_PROFILE_ID ;
	MN66831_GET_PACKAGE( pdata->id, package );	/* pdata->id : パッケージのID */
	if( package == NULL ) {
		Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:kzalloc()\n", __func__ );
		package = kzalloc( sizeof( struct mn66831_package ), GFP_KERNEL ) ;
		if( !package ) {
			return NULL ;
		}
		num_devices = MN66831_NUM_OF_DEVICES ;
		
		/* デバイスの個数分だけデバイス情報配列生成 */
		Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:kzalloc()\n", __func__ );
		package->device_info = kzalloc( sizeof( struct mn66831_host * ) * num_devices, GFP_KERNEL ) ;
		if( !package->device_info ) {
			Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:kfree()\n", __func__ );
			kfree( package );
			return NULL ;
		}
		package->package_id = pdata->id ;
		package->profile_id = profile_id ;
		package->count_device = 0 ;
		package->power_control = 0 ;
		package->flag_connection = false ;
		for(i=0;i<num_devices;i++){
			package->device_info[i] = NULL ;
		}
		package->sdsd_param.enablePullUp = true ;	/* 初期値 : プルアップON */
		package->sdsd_param.setIntMode   = 0 ;		/* 初期値 : SDDAT1Hで割込み通知しない */
		package->sdsd_param.setSdMode    = 0 ;		/* 初期値 : default speed */
		package->sdsd_param.setSdWidth   = 0 ;		/* 初期値 : SD bus 1bit mode */
		
		MN66831_SET_PACKAGE( pdata->id, package ) ;
	}
	if( host_info ){
		package->device_info[pdev->id] = host_info ;
		package->count_device ++ ;
		
		host_info->package = package ;
	}
	
	return package ;
}

/*==============================================================================*/
/*    Function Name   void mn66831_put_package( struct platform_device *pdev, struct mn66831_host *host_info  ) */
/*------------------------------------------------------------------------------*/
/** @brief      Delete/Put package information.
 *  @param[in]  *pdev      : information for mmc driver
 *  @param[in]  *host_info : host information
 *  @return     void
 *  @attention  
 */ 
/*==============================================================================*/
void mn66831_put_package( struct platform_device *pdev, struct mn66831_host *host_info  ) {
	struct mn66831_pdata_host *pdata = NULL ;
	struct mn66831_package *package ;
	
	if(!pdev){
		return ;
	}
	
	/* pdevに紐付けされたstruct mn66831_pdata_host型を取得 */
	pdata = pdev->dev.platform_data ;
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"<%s:pdata = 0x%08X\n", __func__, (unsigned int)pdata );
	if( !pdata ) {
		return ;
	}
	
	MN66831_GET_PACKAGE( pdata->id, package );
    if ( NULL == package ) {
		Sdhubs_DebugPrint(__LINE__, SDHUB_PERR, "%s: NULL == package \n", __func__ );
        return;
    }
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"<%s:pdata->id = %d\n", __func__, pdata->id );
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"<%s:package = 0x%08X\n", __func__, (unsigned int)package );
	
	if( host_info ) {
		host_info->package = NULL ;
		
		package->device_info[pdev->id] = NULL ;
		package->count_device -- ;
	}
	
	if((!package)||(package->count_device!=0)){
		Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"<%s\n", __func__ );
		return ;
	}
	
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"<%s:package->count_device = %d\n", __func__, package->count_device );
	
	if( package->count_device == 0 ) {
		
		MN66831_SET_PACKAGE( package->package_id, NULL ) ;
		
		if(package->device_info){
			Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:kfree()\n", __func__ );
			kfree(package->device_info);
		}
		Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:kfree()\n", __func__ );
		kfree(package);
	}
	
	return ;
}

static int mn66831_k3v2_get_resource(void)
{
 	int ret = 0 ;
    static bool is_powerup = false;

	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s : Powerup\n", __func__, ret );
    if (is_powerup)
        return 0;
        
    wake_lock_init(&SDHUB_SeqLock_wake_lock, WAKE_LOCK_SUSPEND, "mn66831");
	printk("mn66831 SDHUB_SeqLock_wake_lock init success.\n");

	ret	= gpio_request(GPIO_VDDIO_EN, "gpio_vddio_enable");   // switch for vddio and vddios
	if (ret) {
    	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s : gpio_request() GPIO_VDDIO_EN Fail : ret = %d\n", __func__, ret );
    	goto err_request_gpio_vddio_enable;
    }       
	ret	= gpio_request(GPIO_RESET, "gpio_reset"); // XMNRST
	if (ret) {
    	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s : gpio_request() GPIO_RESET Fail : ret = %d\n", __func__, ret );
    	goto err_request_gpio_reset;
    }   
    cprm_iomux_block = iomux_get_block(CPRM_BLOCK_NAME);
	if (!cprm_iomux_block) {
    	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s : iomux_get_block Fail\n", __func__ );
    	goto err_request_gpio_reset;
	}
    cprm_iomux_config = iomux_get_blockconfig(CPRM_BLOCK_NAME);
	if (!cprm_iomux_config) {
    	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s : iomux_get_blockconfig Fail\n", __func__ );
    	goto err_request_gpio_reset;
	}

    pclk = clk_get(NULL, CLK_CON_ID);   // get 26M INCLK
    if (IS_ERR(pclk)) {
    	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s : clk_get Fail : PTR_ERR(pclk) = %d\n", __func__, PTR_ERR(pclk) );
    	goto err_request_gpio_reset;
    }
    writel(0x0, IO_ADDRESS(REG_BASE_IOC + 0x180));
    writel(0x10, IO_ADDRESS(REG_BASE_IOC + 0xad8));        
    clk_set_rate(pclk, 26000000);

    // supply vddio and vddios
    vddio_vddios = regulator_get(NULL, "sdhub-vddio-vddios");
	if (IS_ERR(vddio_vddios)) {
    	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:regulator_get sdhub-vddio-vddios Fail \n", __func__ );
		goto err_request_inclk;
    }
    ret = regulator_set_voltage(vddio_vddios, 1800000, 1800000);
	if (ret) {
    	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s : regulator_set_voltage() vddio_vddios Fail : ret = %d\n", __func__, ret );
    	goto err_request_vddio;
    }    
    // supply avdd
    avdd = regulator_get(NULL, "sdhub-avdd");
	if (IS_ERR(avdd)) {
    	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s:regulator_get avdd Fail \n", __func__ );
		goto err_request_vddio;
    }
    ret = regulator_set_voltage(avdd, 2850000, 2850000);
	if (ret) {
    	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s : regulator_set_voltage() avdd Fail : ret = %d\n", __func__, ret );
    	goto err_request_avdd;
    }    
    is_powerup = true;
    
    return 0;

err_request_avdd:
    regulator_put(avdd);
err_request_vddio:
    regulator_put(vddio_vddios);
err_request_inclk:
    clk_put(pclk);
err_request_gpio_reset:    
    gpio_free(GPIO_RESET);
err_request_gpio_vddio_enable:    
    gpio_free(GPIO_VDDIO_EN);

    return -1;
}


static int mn66831_k3v2_put_resource(void)
{
    gpio_free(GPIO_VDDIO_EN);
    gpio_free(GPIO_RESET);
    clk_put(pclk);
    regulator_put(avdd);
    regulator_put(vddio_vddios);

    return 0;
}


/*==============================================================================*/
/*    Function Name   static int __init mn66831_probe( struct platform_device *pdev ) */
/*------------------------------------------------------------------------------*/
/** @brief      Probe for SdHubSendCommand module.(for MMC Linux)
 *  @param[in]  *pdev struct for platform_device
 *  @return     return value by platform_get_resource and request_mem_region.
                -ENOMEM : No Memory for mmc_host or cannot remap SDIP
                -ENXIO  : No SDIP defined.
                -EBUSY  : SDIP already in use
                -ENODEV : Other Errors.
 *  @attention  
*/ 
/*==============================================================================*/
static int __init mn66831_probe( struct platform_device *pdev ) {
	int ret = 0 ;
	struct mn66831_package *package ;
	struct mn66831_pdata_host *pdata ;
	struct mn66831_host *host_info ;
	int i,j;

    if (0 != mn66831_k3v2_get_resource())
    {
		Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"<%s:mn66831_k3v2_get_resource() Fail\n", __func__ );
		return -EBUSY ;
    }

    
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:pdev=0x%08X\n", __func__, (unsigned int)pdev );

	/* pdevに紐付けされたstruct mn66831_pdata_host型を取得 */
	pdata = pdev->dev.platform_data ;
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:pdata = 0x%08X\n", __func__, (unsigned int)pdata );
	if( !pdata ) {
		Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"<%s:pdata = 0x%08X\n", __func__, (unsigned int)pdata );
		return -ENXIO ;
	}
	
	/* デバイス情報の生成 */
	host_info = kzalloc( sizeof( struct mn66831_host ), GFP_KERNEL ) ;
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:host_info = 0x%08X\n", __func__, (unsigned int)host_info );
	if( !host_info ){
		Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"<%s:kzalloc\n", __func__ );
		return -ENOMEM ;
	}
	
	/* パッケージ情報の生成or取得 */
	package = mn66831_get_package( pdev, host_info );
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:package=0x%08X\n", __func__, (unsigned int)package );
	if( !package ) {
		Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"<%s:mn66831_get_package\n", __func__ );
		return -ENXIO ;
	}
	
	host_info->slot = kzalloc( (sizeof(struct mn66831_slot *) * pdata->nr_parts ), GFP_KERNEL ) ;
	host_info->num_slots = pdata->nr_parts ;
	host_info->power_control = 0 ;
	host_info->pdev = pdev ;
	
	platform_set_drvdata( pdev, host_info ) ;
	
	/* スロットの生成 */
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:pdata->nr_parts = %u\n", __func__, pdata->nr_parts );
	for( i = 0; i < pdata->nr_parts; i++ ) {
		ret = mn66831_append_slot( host_info, &pdata->slot[i], i ) ;
		if( ret ) {
			for( j = 0; j < i; j++ ) {
				mn66831_remove_slot( host_info, &pdata->slot[j], j ) ;
			}
			mn66831_put_package( pdev, host_info );
			return ret ;
		}
	}

	return 0 ;
}

/*==============================================================================*/
/*    Function Name   static int __exit mn66831_remove( struct platform_device *pdev ) */
/*------------------------------------------------------------------------------*/
/** @brief      Removee for SdHubSendCommand module.(for MMC Linux)
 *  @param[in]  *pdev struct for platform_device
 *  @return     -1:False 0: Success
 *  @attention  
*/ 
/*==============================================================================*/
static int mn66831_remove( struct platform_device *pdev ) {
	struct mn66831_host *host_info = platform_get_drvdata(pdev) ;
	struct mn66831_pdata_host *pdata ;
	int i ;
	
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:pdev = 0x%08X\n", __func__, (unsigned int)pdev );
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:pdev->id = %d\n", __func__, pdev->id );
	Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:host_info = 0x%08X\n", __func__, (unsigned int)host_info );

    if (0 != mn66831_k3v2_put_resource())
    {
		Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"<%s:mn66831_k3v2_put_resource() Fail\n", __func__ );
		return -EBUSY ;
    }
    
	/* pdevに紐付けされたstruct mn66831_pdata_host型を取得 */
	pdata = pdev->dev.platform_data ;
	if( !pdata ) {
		return -ENXIO ;
	}
	
	for( i=0; i<host_info->num_slots; i++ ) {
		mn66831_remove_slot( host_info, &(pdata->slot[i]), i );
	}
	
	kfree(host_info->slot);
	/* iounmapは不要 */
	platform_set_drvdata(pdev,NULL);
	kfree(host_info);
	
	/* パッケージ情報の解放 */
	mn66831_put_package( pdev, host_info );
	
	return 0;
}
extern int sdcard_init_status;
#ifdef	CONFIG_PM

/*==============================================================================*/
/*    Function Name   static int mn66831_suspend( struct platform_device *pdev, pm_message_t state ) */
/*------------------------------------------------------------------------------*/
/** @brief      Suspend for SdHubSendCommand module.(for MMC Linux)
 *  @param[in]  *pdev struct for platform_device
 *  @param[in]  state : suspend mode
 *  @return     return value by mmc_suspend_host
 *  @attention  
*/ 
/*==============================================================================*/
static int mn66831_suspend( struct platform_device *pdev, pm_message_t state ) {
	struct mn66831_host *host_info = platform_get_drvdata( pdev );
	int ret = 0;
	int i;
	struct mn66831_package *package = NULL ;
	unsigned int package_id = 0;
	MN66831_GET_PACKAGE( package_id, package );

	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s: host_info->num_slots = %d  package->flag_connection= %d sdcard_init_status = %d\n",
		__func__, host_info->num_slots, package->flag_connection, sdcard_init_status );
        
	/*sd card gpio detected success but inited fail.*/
	if(package->flag_connection == true && !sdcard_init_status)
	{
		printk("mn66831_suspend error:sd card gpio detected success but inited fail.\n");
		return ret;
	}
    
	for( i = 0; i < host_info->num_slots; i++ ) {
#if			defined(_SDHUB_OLD_MMC_SUSPEND_HOST_)
		ret = mmc_suspend_host( host_info->slot[i]->upHost, state ) ;
#else	//	defined(_SDHUB_OLD_MMC_SUSPEND_HOST_)	
		ret = mmc_suspend_host( host_info->slot[i]->upHost ) ;
#endif	//	defined(_SDHUB_OLD_MMC_SUSPEND_HOST_)	
	}
	
	SDHUB_clock_disable();
    
	return ret ;
}

/*==============================================================================*/
/*    Function Name   static int mn66831_resume( struct platform_device *pdev ) */
/*------------------------------------------------------------------------------*/
/** @brief      Resume for SdHubSendCommand module.(for MMC Linux)
 *  @param[in]  *pdev struct for platform_device
 *  @return     return value by mmc_resume_host
 *  @attention  
*/ 
/*==============================================================================*/
static int mn66831_resume( struct platform_device *pdev ) {
	struct mn66831_host *host_info = platform_get_drvdata( pdev );
	int ret = 0;
	int i;
	struct mn66831_package *package = NULL ;
	unsigned int package_id = 0;
	MN66831_GET_PACKAGE( package_id, package );

	Sdhubs_DebugPrint(__LINE__, SDHUB_PERR,"%s: host_info->num_slots = %d  package->flag_connection= %d sdcard_init_status = %d\n",
		__func__, host_info->num_slots, package->flag_connection, sdcard_init_status );
        
	/*sd card gpio detected success but inited fail.*/
	if(package->flag_connection == true && !sdcard_init_status)
	{
		printk("mn66831_resume error:sd card gpio detected success but inited fail.\n");
		return ret;
	}

	for( i = 0; i < host_info->num_slots; i++ ) {
		Sdhubs_DebugPrint(__LINE__, SDHUB_PDEBUG,"%s:mmc_resume_host(slot[%d])\n", __func__, i );
		ret = mmc_resume_host( host_info->slot[i]->upHost ) ;
	}

	return ret ;
}
#else
#define	mn6683x_suspend	NULL
#define	mn6683x_resume	NULL
#endif

/* Data Instance */
static struct platform_driver mn6683x_hub_driver = {
	.probe		= mn66831_probe,
	.remove		= mn66831_remove,
	.suspend	= mn66831_suspend,
	.resume		= mn66831_resume,
	.driver		= {
			.name = DRIVER_NAME,
	},
};


/*==============================================================================*/
/*    Function Name   static int __init mn66831_init( void ) */
/*------------------------------------------------------------------------------*/
/** @brief      module_init for SdHubSendCommand.(for MMC Linux)
 *  @param[in]  void
 *  @return     
 *  @attention  
*/ 
/*==============================================================================*/
static int __init mn66831_init( void ) {
	int		result ;
	int		i;
	struct mn66831_platform_device *conf_devs = &SdHubDevices ;		/* 使用デバイス一覧 */

	/* パッケージの配列を初期化 */
	for(i=0;i<SDHUB_PACKAGE_COUNT;i++){
		MN66831_SET_PACKAGE( i, NULL ) ;
	}
	result = platform_add_devices( conf_devs->conf_data, conf_devs->nr_devices) ;
	result = platform_driver_register( &mn6683x_hub_driver ) ;
	if (result) {
		return result;
	}
	
	return result ;
}

/*==============================================================================*/
/*    Function Name   void __exit mn66831_exit( void )*/
/*------------------------------------------------------------------------------*/
/** @brief      module_exit for SdHubSendCommand.(for MMC Linux)
 *  @param[in]  void
 *  @return     void
 *  @attention  
*/ 
/*==============================================================================*/
static void __exit mn66831_exit( void ) {
	int i;
	struct mn66831_platform_device *conf_devs = &SdHubDevices ;		/* 使用デバイス一覧 */
	
	/*  */
	for (i=0;i<conf_devs->nr_devices;i++) {
		platform_device_unregister( conf_devs->conf_data[i] );
	}
	platform_driver_unregister(&mn6683x_hub_driver);

	return ;
}

module_init(mn66831_init);
module_exit(mn66831_exit);

MODULE_DESCRIPTION("MN66831 SD Hub I/F driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS(DRIVER_NAME);
MODULE_AUTHOR("Panasonic Corporation");

