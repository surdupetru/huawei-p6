
/******************** (C) COPYRIGHT 2010 STMicroelectronics ********************
*
* File Name          : lis3dh_misc.h
* Authors            : MH - C&I BU - Application Team
*		     : Matteo Dameno (matteo.dameno@st.com)
*		     : Carmine Iascone (carmine.iascone@st.com)
* Version            : V 1.0.8
* Date               : 2011/Apr/01
*
********************************************************************************
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
* OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
* PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
* THIS SOFTWARE IS SPECIFICALLY DESIGNED FOR EXCLUSIVE USE WITH ST PARTS.
*
*******************************************************************************/
/*******************************************************************************
Version History.

Revision 1-0-0 05/11/2009
	First Release
Revision 1-0-1 26/01/2010
	Linux K&R Compliant Release
Revision 1-0-5 16/08/2010
	Interrupt Management
Revision 1-0-6 10/11/2010
	ioclt no more supported
	sysfs support
Revision 1-0-7 26/11/2010
	moved to input/misc
	manages use/non-use of interrupts
 Revision 1.0.8 2010/Apr/01
  corrects a bug in interrupt pin management in 1.0.7 acc side

*******************************************************************************/
/*==============================================================================
History

Problem NO.         Name        Time         Reason

==============================================================================*/

#ifdef	__KERNEL__

#define MMA8452_ACC_DEV_NAME		"mma8452"
#define	FREESCALE_ACCL_POWER_NAME	"FS_ACCELEROMETER_VDD_SENSOR"
#define MMA8452_RETRY_COUNT			3
#define MMA8452_I2C_ADDR			0x1C
#define MMA8451_ID					0x1A
#define MMA8452_ID					0x2A

#define POLL_INTERVAL_MIN			1
#define POLL_INTERVAL_MAX			500
#define POLL_INTERVAL				10 /* msecs */
#define INPUT_FUZZ					4
#define INPUT_FLAT					4
#define MODE_CHANGE_DELAY_MS		100
#define STANDBNY_100HZ                  0x18
#define NORMAL_TM                       10000000  /*10HZ*/

#define MMA8452_STATUS_ZYXDR		0x08
#define MMA8452_BUF_SIZE                7
#define ACCL_DATA_SIZE				6
#define	CONFIG_MXC_MMA_POSITION		0

/* register enum for mma8452 registers */
enum {
	MMA8452_STATUS = 0x00,
	MMA8452_OUT_X_MSB,
	MMA8452_OUT_X_LSB,
	MMA8452_OUT_Y_MSB,
	MMA8452_OUT_Y_LSB,
	MMA8452_OUT_Z_MSB,
	MMA8452_OUT_Z_LSB,

	MMA8452_SYSMOD = 0x0B,
	MMA8452_INT_SOURCE,
	MMA8452_WHO_AM_I,
	MMA8452_XYZ_DATA_CFG,
	MMA8452_HP_FILTER_CUTOFF,

	MMA8452_PL_STATUS,
	MMA8452_PL_CFG,
	MMA8452_PL_COUNT,
	MMA8452_PL_BF_ZCOMP,
	MMA8452_PL_P_L_THS_REG,

	MMA8452_FF_MT_CFG,
	MMA8452_FF_MT_SRC,
	MMA8452_FF_MT_THS,
	MMA8452_FF_MT_COUNT,

	MMA8452_TRANSIENT_CFG = 0x1D,
	MMA8452_TRANSIENT_SRC,
	MMA8452_TRANSIENT_THS,
	MMA8452_TRANSIENT_COUNT,

	MMA8452_PULSE_CFG,
	MMA8452_PULSE_SRC,
	MMA8452_PULSE_THSX,
	MMA8452_PULSE_THSY,
	MMA8452_PULSE_THSZ,
	MMA8452_PULSE_TMLT,
	MMA8452_PULSE_LTCY,
	MMA8452_PULSE_WIND,

	MMA8452_ASLP_COUNT,
	MMA8452_CTRL_REG1,
	MMA8452_CTRL_REG2,
	MMA8452_CTRL_REG3,
	MMA8452_CTRL_REG4,
	MMA8452_CTRL_REG5,

	MMA8452_OFF_X,
	MMA8452_OFF_Y,
	MMA8452_OFF_Z,

	MMA8452_REG_END,
};

/* The sensitivity is represented in counts/g. In 2g mode the
sensitivity is 1024 counts/g. In 4g mode the sensitivity is 512
counts/g and in 8g mode the sensitivity is 256 counts/g.
 */
enum {
	MODE_2G = 0,
	MODE_4G,
	MODE_8G,
};

enum {
	MMA_STANDBY = 0,
	MMA_ACTIVED,
};

/* mma8452 status */
struct mma8452_status {
	u8 mode;
	u8 ctl_reg1;
	int active;
	int position;
};

struct mma8452_acc_platform_data {
	int (*power_on)(struct device *);
	void (*power_off)(void);
	int config_mxc_mma_position;
};

#endif	/* __KERNEL__ */
