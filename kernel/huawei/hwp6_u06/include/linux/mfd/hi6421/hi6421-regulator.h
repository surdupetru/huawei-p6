/*
 * kernel/driver/mfd/hi6421-regulator.h
 *
 * Copyright (C) 2011 Hisilicon
 * License terms: GNU General Public License (GPL) version 2
 *
 */

 /*
 * HI6421 - Regulator  num
 */
#define HI6421_LDO0  0
#define HI6421_LDO1  1
#define HI6421_LDO2  2
#define HI6421_LDO3  3
#define HI6421_LDO4  4
#define HI6421_LDO5  5
#define HI6421_LDO6  6
#define HI6421_LDO7  7
#define HI6421_LDO8  8
#define HI6421_LDO9  9
#define HI6421_LDO10  10
#define HI6421_LDO11  11
#define HI6421_LDO12  12
#define HI6421_LDO13  13
#define HI6421_LDO14  14
#define HI6421_LDO15  15
#define HI6421_LDO16  16
#define HI6421_LDO17  17
#define HI6421_LDO18  18
#define HI6421_LDO19  19
#define HI6421_LDO20  20
#define HI6421_LDOAUDIO  21
#define HI6421_BUCK0  22
#define HI6421_BUCK1  23
#define HI6421_BUCK2  24
#define HI6421_BUCK3  25
#define HI6421_BUCK4  26
#define HI6421_BUCK5  27
#define HI6421_USB_CHG_PUMP 28
#define HI6421_HDMI_CHG_PUMP 29
/*
 * HI6421 - Regulator ctrl
 */
#define 	LDO0_CTRL_REG			(0x20 << 2)
#define	LDO1_CTRL_REG			(0x21 << 2)
#define	LDO2_CTRL_REG			(0x22 << 2)
#define	LDO3_CTRL_REG 			(0x23 << 2)
#define	LDO4_CTRL_REG 			(0x24 << 2)
#define	LDO5_CTRL_REG 			(0x25 << 2)
#define	LDO6_CTRL_REG 			(0x26 << 2)
#define	LDO7_CTRL_REG			(0x27 << 2)
#define	LDO8_CTRL_REG			(0x28 << 2)
#define	LDO9_CTRL_REG			(0x29 << 2)
#define	LDO10_CTRL_REG			(0x2A << 2)
#define	LDO11_CTRL_REG			(0x2B << 2)
#define	LDO12_CTRL_REG 		(0x2C << 2)
#define	LDO13_CTRL_REG 		(0x2D << 2)
#define	LDO14_CTRL_REG			(0x2E << 2)
#define	LDO15_CTRL_REG			(0x2F << 2)
#define	LDO16_CTRL_REG 		(0x30 << 2)
#define	LDO17_CTRL_REG			(0x31 << 2)
#define	LDO18_CTRL_REG			(0x32 << 2)
#define	LDO19_CTRL_REG			(0x33 << 2)
#define	LDO20_CTRL_REG			(0x34 << 2)
#define	LDO_AUDIO_CTRL_REG	(0x36 << 2)
#define	BUCK0_CTRL_REG			(0x0C << 2)
#define	BUCK1_CTRL_REG			(0x0E << 2)
#define	BUCK2_CTRL_REG			(0x10 << 2)
#define	BUCK3_CTRL_REG			(0x12 << 2)
#define	BUCK4_CTRL_REG			(0x14 << 2)
#define	BUCK5_CTRL_REG			(0x16 << 2)

#define 	LDO0_VSET_REG			(0x20 << 2)
#define	LDO1_VSET_REG			(0x21 << 2)
#define	LDO2_VSET_REG			(0x22 << 2)
#define	LDO3_VSET_REG 			(0x23 << 2)
#define	LDO4_VSET_REG 			(0x24 << 2)
#define	LDO5_VSET_REG 			(0x25 << 2)
#define	LDO6_VSET_REG 			(0x26 << 2)
#define	LDO7_VSET_REG			(0x27 << 2)
#define	LDO8_VSET_REG			(0x28 << 2)
#define	LDO9_VSET_REG			(0x29 << 2)
#define	LDO10_VSET_REG			(0x2A << 2)
#define	LDO11_VSET_REG			(0x2B << 2)
#define	LDO12_VSET_REG 		(0x2C << 2)
#define	LDO13_VSET_REG 		(0x2D << 2)
#define	LDO14_VSET_REG			(0x2E << 2)
#define	LDO15_VSET_REG			(0x2F << 2)
#define	LDO16_VSET_REG 		(0x30 << 2)
#define	LDO17_VSET_REG			(0x31 << 2)
#define	LDO18_VSET_REG			(0x32 << 2)
#define	LDO19_VSET_REG			(0x33 << 2)
#define	LDO20_VSET_REG			(0x34 << 2)
#define	LDO_AUDIO_VSET_REG	(0x36 << 2)
#define	BUCK0_VSET_REG			(0x0D << 2)
#define	BUCK1_VSET_REG			(0x0F << 2)
#define	BUCK2_VSET_REG			(0x11 << 2)
#define	BUCK3_VSET_REG			(0x13 << 2)
#define	BUCK4_VSET_REG			(0x15 << 2)
#define	BUCK5_VSET_REG			(0x17 << 2)
#define 	PMU_REST_REG			(0x35 << 2)
#define	OCP_CURRENT_PROTECT_REG	(0x51 << 2)
#define	OCP_CURRENT_PROTECT_MASK	0x2

#define HI6421_BUCK_ENA					0x1     /* BUCK_ENA */
/*irda enable clk_26M*/
#define	TCXO_CTRL	(0x4f <<2)
#define   HI6421_CLK_26_EN 0x03

#define HI6421_BUCK_ENA_MASK			0x1		/* BUCK_MASK */
#define HI6421_BUCK012_VSEL_MASK		0x7F		/* BUCK0 BUCK1 BUCK2 VSEL - [6:0] */
#define HI6421_BUCK345_VSEL_MASK		        0x07		/* BUCK3 BUCK4 BUCK5 VSEL - [2:0] */
#define HI6421_BUCK_ENTRY_ECO_MODE		0x10			/*BUCK ENTRY ECO*/
#define HI6421_BUCK012_MIN_VOLTAGE			700000
#define HI6421_BUCK_SPECIAL				7087

#define HI6421_LDO_ENA					0x10		/* LDO_ENA */
#define HI6421_LDO_ENA_MASK				0x10		/* LDO_MASK */
#define HI6421_LDO1_VSEL_MASK			0x03		/* LDO1 VSEL - [1:0] */
#define HI6421_LDO_OTHERS_VSEL_MASK	0x07		/* LDO0 LDO2-20 VSEL - [2:0] */
#define HI6421_LDO_ENTRY_ECO_MODE		0x20		/*LDO ENTRY ECO*/

#define HI6421_LDOAUDIO_ENA				0x1		/* LDO_AUDIO_ENA */
#define HI6421_LDOAUDIO_ENA_MASK			0x1		/* LDO_AUDIO_MASK */
#define HI6421_LDOAUDIO_VSEL_MASK		0x70		/* LDOAUDIO VSEL - [4:6] */
#define HI6421_LDOAUDIO_ENTRY_ECO_MODE		0x2	/*LDOAUDIO ENTRY ECO*/


#define NUM_OF_HI6421_REGULATOR		30//28
#define NUM_OF_HI6421_REGULATOR_VSET		8
#define REGULATOR_ECO_UA_1	8000
#define REGULATOR_ECO_UA_2	5000

#define HI6421_USB_HDMI_CHG_PUMP_CTRL_REG		(0x4E << 2)
#define HI6421_USB_CHG_PUMP_ENA_MASK			(0x20)
#define HI6421_HDMI_CHG_PUMP_ENA_MASK			(0x2)
