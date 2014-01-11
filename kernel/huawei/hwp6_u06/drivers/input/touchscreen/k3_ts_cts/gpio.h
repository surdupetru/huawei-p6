/* linux/include/asm-arm/arch-hi3620/gpio.h
*
* Copyright (c) 2011 Hisilicon Co., Ltd. 
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*
*/
//#include <linux/autoconf.h>
#include <mach/platform.h>
#include <mach/hardware.h>

#ifndef __ASM_ARCH_GPIO_H_
#define __ASM_ARCH_GPIO_H_
//-----------------------------------------------
#define GPIO_INTR_BASE			64
#define GPIO_INTR_LVL           112

//------------------------------------------------------------------------------
// single GPIO by ID define
//------------------------------------------------------------------------------

#define GPIO_1_INT          (GPIO_INTR_BASE + 0)
#define GPIO_2_INT          (GPIO_INTR_BASE + 1)
#define GPIO_3_INT          (GPIO_INTR_BASE + 2)
#define GPIO_4_INT          (GPIO_INTR_BASE + 3)
#define GPIO_5_INT          (GPIO_INTR_BASE + 4)
#define GPIO_6_INT          (GPIO_INTR_BASE + 5)
#define GPIO_7_INT          (GPIO_INTR_BASE + 6)
#define GPIO_8_INT          (GPIO_INTR_BASE + 7)

#define GPIO_9_INT          (GPIO_INTR_BASE + 8)
#define GPIO_10_INT         (GPIO_INTR_BASE + 9)
#define GPIO_11_INT         (GPIO_INTR_BASE + 10)
#define GPIO_12_INT         (GPIO_INTR_BASE + 11)
#define GPIO_13_INT         (GPIO_INTR_BASE + 12)
#define GPIO_14_INT         (GPIO_INTR_BASE + 13)
#define GPIO_15_INT         (GPIO_INTR_BASE + 14)
#define GPIO_16_INT         (GPIO_INTR_BASE + 15)

#define GPIO_17_INT         (GPIO_INTR_BASE + 16)
#define GPIO_18_INT         (GPIO_INTR_BASE + 17)
#define GPIO_19_INT         (GPIO_INTR_BASE + 18)
#define GPIO_20_INT         (GPIO_INTR_BASE + 19)
#define GPIO_21_INT         (GPIO_INTR_BASE + 20)
#define GPIO_22_INT         (GPIO_INTR_BASE + 21)
#define GPIO_23_INT         (GPIO_INTR_BASE + 22)
#define GPIO_24_INT         (GPIO_INTR_BASE + 23)

#define GPIO_25_INT         (GPIO_INTR_BASE + 24)
#define GPIO_26_INT         (GPIO_INTR_BASE + 25)
#define GPIO_27_INT         (GPIO_INTR_BASE + 26)
#define GPIO_28_INT         (GPIO_INTR_BASE + 27)
#define GPIO_29_INT         (GPIO_INTR_BASE + 28)
#define GPIO_30_INT         (GPIO_INTR_BASE + 29)
#define GPIO_31_INT         (GPIO_INTR_BASE + 30)
#define GPIO_32_INT         (GPIO_INTR_BASE + 31)

#define GPIO_33_INT         (GPIO_INTR_BASE + 32)
#define GPIO_34_INT         (GPIO_INTR_BASE + 33)
#define GPIO_35_INT         (GPIO_INTR_BASE + 34)
#define GPIO_36_INT         (GPIO_INTR_BASE + 35)
#define GPIO_37_INT         (GPIO_INTR_BASE + 36)
#define GPIO_38_INT         (GPIO_INTR_BASE + 37)
#define GPIO_39_INT         (GPIO_INTR_BASE + 38)
#define GPIO_40_INT         (GPIO_INTR_BASE + 39)

#define GPIO_41_INT         (GPIO_INTR_BASE + 40)
#define GPIO_42_INT         (GPIO_INTR_BASE + 41)
#define GPIO_43_INT         (GPIO_INTR_BASE + 42)
#define GPIO_44_INT         (GPIO_INTR_BASE + 43)
#define GPIO_45_INT         (GPIO_INTR_BASE + 44)
#define GPIO_46_INT         (GPIO_INTR_BASE + 45)
#define GPIO_47_INT         (GPIO_INTR_BASE + 46)
#define GPIO_48_INT         (GPIO_INTR_BASE + 47)

#define GPIO_49_INT         (GPIO_INTR_BASE + 48)
#define GPIO_50_INT         (GPIO_INTR_BASE + 49)
#define GPIO_51_INT         (GPIO_INTR_BASE + 50)
#define GPIO_52_INT         (GPIO_INTR_BASE + 51)
#define GPIO_53_INT         (GPIO_INTR_BASE + 52)
#define GPIO_54_INT         (GPIO_INTR_BASE + 53)
#define GPIO_55_INT         (GPIO_INTR_BASE + 54)
#define GPIO_56_INT         (GPIO_INTR_BASE + 55)

#define GPIO_57_INT         (GPIO_INTR_BASE + 56)
#define GPIO_58_INT         (GPIO_INTR_BASE + 57)
#define GPIO_59_INT         (GPIO_INTR_BASE + 58)
#define GPIO_60_INT         (GPIO_INTR_BASE + 59)
#define GPIO_61_INT         (GPIO_INTR_BASE + 60)
#define GPIO_62_INT         (GPIO_INTR_BASE + 61)
#define GPIO_63_INT         (GPIO_INTR_BASE + 62)
#define GPIO_64_INT         (GPIO_INTR_BASE + 63)

#define GPIO_65_INT         (GPIO_INTR_BASE + 64)
#define GPIO_66_INT         (GPIO_INTR_BASE + 65)
#define GPIO_67_INT         (GPIO_INTR_BASE + 66)
#define GPIO_68_INT         (GPIO_INTR_BASE + 67)
#define GPIO_69_INT         (GPIO_INTR_BASE + 68)
#define GPIO_70_INT         (GPIO_INTR_BASE + 69)
#define GPIO_71_INT         (GPIO_INTR_BASE + 70)
#define GPIO_72_INT         (GPIO_INTR_BASE + 71)

#define GPIO_73_INT         (GPIO_INTR_BASE + 72)
#define GPIO_74_INT         (GPIO_INTR_BASE + 73)
#define GPIO_75_INT         (GPIO_INTR_BASE + 74)
#define GPIO_76_INT         (GPIO_INTR_BASE + 75)
#define GPIO_77_INT         (GPIO_INTR_BASE + 76)
#define GPIO_78_INT         (GPIO_INTR_BASE + 77)
#define GPIO_79_INT         (GPIO_INTR_BASE + 78)
#define GPIO_80_INT         (GPIO_INTR_BASE + 79)

#define GPIO_81_INT         (GPIO_INTR_BASE + 80)
#define GPIO_82_INT         (GPIO_INTR_BASE + 81)
#define GPIO_83_INT         (GPIO_INTR_BASE + 82)
#define GPIO_84_INT         (GPIO_INTR_BASE + 83)
#define GPIO_85_INT         (GPIO_INTR_BASE + 84)
#define GPIO_86_INT         (GPIO_INTR_BASE + 85)
#define GPIO_87_INT         (GPIO_INTR_BASE + 86)
#define GPIO_88_INT         (GPIO_INTR_BASE + 87)

#define GPIO_89_INT         (GPIO_INTR_BASE + 88)
#define GPIO_90_INT         (GPIO_INTR_BASE + 89)
#define GPIO_91_INT         (GPIO_INTR_BASE + 90)
#define GPIO_92_INT         (GPIO_INTR_BASE + 91)
#define GPIO_93_INT         (GPIO_INTR_BASE + 92)
#define GPIO_94_INT         (GPIO_INTR_BASE + 93)
#define GPIO_95_INT         (GPIO_INTR_BASE + 94)
#define GPIO_96_INT         (GPIO_INTR_BASE + 95)

#define GPIO_97_INT         (GPIO_INTR_BASE + 96)
#define GPIO_98_INT         (GPIO_INTR_BASE + 97)
#define GPIO_99_INT         (GPIO_INTR_BASE + 98)
#define GPIO_100_INT        (GPIO_INTR_BASE + 99)
#define GPIO_101_INT        (GPIO_INTR_BASE + 100)
#define GPIO_102_INT        (GPIO_INTR_BASE + 101)
#define GPIO_103_INT        (GPIO_INTR_BASE + 102)
#define GPIO_104_INT        (GPIO_INTR_BASE + 103)

#define GPIO_105_INT        (GPIO_INTR_BASE + 104)
#define GPIO_106_INT        (GPIO_INTR_BASE + 105)
#define GPIO_107_INT        (GPIO_INTR_BASE + 106)
#define GPIO_108_INT        (GPIO_INTR_BASE + 107)
#define GPIO_109_INT        (GPIO_INTR_BASE + 108)
#define GPIO_110_INT        (GPIO_INTR_BASE + 109)
#define GPIO_111_INT        (GPIO_INTR_BASE + 110)
#define GPIO_112_INT        (GPIO_INTR_BASE + 111)

//------------------------------------------------------------------------------
// single GPIO from group define
//------------------------------------------------------------------------------
#define GPIO_0_0_INT        GPIO_1_INT
#define GPIO_0_1_INT        GPIO_2_INT
#define GPIO_0_2_INT        GPIO_3_INT
#define GPIO_0_3_INT        GPIO_4_INT
#define GPIO_0_4_INT        GPIO_5_INT
#define GPIO_0_5_INT        GPIO_6_INT
#define GPIO_0_6_INT        GPIO_7_INT
#define GPIO_0_7_INT        GPIO_8_INT

#define GPIO_1_0_INT        GPIO_9_INT
#define GPIO_1_1_INT        GPIO_10_INT
#define GPIO_1_2_INT        GPIO_11_INT
#define GPIO_1_3_INT        GPIO_12_INT
#define GPIO_1_4_INT        GPIO_13_INT
#define GPIO_1_5_INT        GPIO_14_INT
#define GPIO_1_6_INT        GPIO_15_INT
#define GPIO_1_7_INT        GPIO_16_INT

#define GPIO_2_0_INT        GPIO_17_INT
#define GPIO_2_1_INT        GPIO_18_INT
#define GPIO_2_2_INT        GPIO_19_INT
#define GPIO_2_3_INT        GPIO_20_INT
#define GPIO_2_4_INT        GPIO_21_INT
#define GPIO_2_5_INT        GPIO_22_INT
#define GPIO_2_6_INT        GPIO_23_INT
#define GPIO_2_7_INT        GPIO_24_INT

#define GPIO_3_0_INT        GPIO_25_INT
#define GPIO_3_1_INT        GPIO_26_INT
#define GPIO_3_2_INT        GPIO_27_INT
#define GPIO_3_3_INT        GPIO_28_INT
#define GPIO_3_4_INT        GPIO_29_INT
#define GPIO_3_5_INT        GPIO_30_INT
#define GPIO_3_6_INT        GPIO_31_INT
#define GPIO_3_7_INT        GPIO_32_INT

#define GPIO_4_0_INT        GPIO_33_INT
#define GPIO_4_1_INT        GPIO_34_INT
#define GPIO_4_2_INT        GPIO_35_INT
#define GPIO_4_3_INT        GPIO_36_INT
#define GPIO_4_4_INT        GPIO_37_INT
#define GPIO_4_5_INT        GPIO_38_INT
#define GPIO_4_6_INT        GPIO_39_INT
#define GPIO_4_7_INT        GPIO_40_INT

#define GPIO_5_0_INT        GPIO_41_INT
#define GPIO_5_1_INT        GPIO_42_INT
#define GPIO_5_2_INT        GPIO_43_INT
#define GPIO_5_3_INT        GPIO_44_INT
#define GPIO_5_4_INT        GPIO_45_INT
#define GPIO_5_5_INT        GPIO_46_INT
#define GPIO_5_6_INT        GPIO_47_INT
#define GPIO_5_7_INT        GPIO_48_INT

#define GPIO_6_0_INT        GPIO_49_INT
#define GPIO_6_1_INT        GPIO_50_INT
#define GPIO_6_2_INT        GPIO_51_INT
#define GPIO_6_3_INT        GPIO_52_INT
#define GPIO_6_4_INT        GPIO_53_INT
#define GPIO_6_5_INT        GPIO_54_INT
#define GPIO_6_6_INT        GPIO_55_INT
#define GPIO_6_7_INT        GPIO_56_INT

#define GPIO_7_0_INT        GPIO_57_INT
#define GPIO_7_1_INT        GPIO_58_INT
#define GPIO_7_2_INT        GPIO_59_INT
#define GPIO_7_3_INT        GPIO_60_INT
#define GPIO_7_4_INT        GPIO_61_INT
#define GPIO_7_5_INT        GPIO_62_INT
#define GPIO_7_6_INT        GPIO_63_INT
#define GPIO_7_7_INT        GPIO_64_INT

#define GPIO_8_0_INT        GPIO_65_INT
#define GPIO_8_1_INT        GPIO_66_INT
#define GPIO_8_2_INT        GPIO_67_INT
#define GPIO_8_3_INT        GPIO_68_INT
#define GPIO_8_4_INT        GPIO_69_INT
#define GPIO_8_5_INT        GPIO_70_INT
#define GPIO_8_6_INT        GPIO_71_INT
#define GPIO_8_7_INT        GPIO_72_INT

#define GPIO_9_0_INT        GPIO_73_INT
#define GPIO_9_1_INT        GPIO_74_INT
#define GPIO_9_2_INT        GPIO_75_INT
#define GPIO_9_3_INT        GPIO_76_INT
#define GPIO_9_4_INT        GPIO_77_INT
#define GPIO_9_5_INT        GPIO_78_INT
#define GPIO_9_6_INT        GPIO_79_INT
#define GPIO_9_7_INT        GPIO_80_INT

#define GPIO_10_0_INT       GPIO_81_INT
#define GPIO_10_1_INT       GPIO_82_INT
#define GPIO_10_2_INT       GPIO_83_INT
#define GPIO_10_3_INT       GPIO_84_INT
#define GPIO_10_4_INT       GPIO_85_INT
#define GPIO_10_5_INT       GPIO_86_INT
#define GPIO_10_6_INT       GPIO_87_INT
#define GPIO_10_7_INT       GPIO_88_INT

#define GPIO_11_0_INT       GPIO_89_INT
#define GPIO_11_1_INT       GPIO_90_INT
#define GPIO_11_2_INT       GPIO_91_INT
#define GPIO_11_3_INT       GPIO_92_INT
#define GPIO_11_4_INT       GPIO_93_INT
#define GPIO_11_5_INT       GPIO_94_INT
#define GPIO_11_6_INT       GPIO_95_INT
#define GPIO_11_7_INT       GPIO_96_INT

#define GPIO_12_0_INT       GPIO_97_INT
#define GPIO_12_1_INT       GPIO_98_INT
#define GPIO_12_2_INT       GPIO_99_INT
#define GPIO_12_3_INT       GPIO_100_INT
#define GPIO_12_4_INT       GPIO_101_INT
#define GPIO_12_5_INT       GPIO_102_INT
#define GPIO_12_6_INT       GPIO_103_INT
#define GPIO_12_7_INT       GPIO_104_INT

#define GPIO_13_0_INT       GPIO_105_INT
#define GPIO_13_1_INT       GPIO_106_INT
#define GPIO_13_2_INT       GPIO_107_INT
#define GPIO_13_3_INT       GPIO_108_INT
#define GPIO_13_4_INT       GPIO_109_INT
#define GPIO_13_5_INT       GPIO_110_INT
#define GPIO_13_6_INT       GPIO_111_INT
#define GPIO_13_7_INT       GPIO_112_INT

#define GPIO_INTR_MAX       (GPIO_INTR_BASE + GPIO_INTR_LVL)
/*end of GPIO INT */
//------------------------------------------------------------------------------
// GPIO group define
//------------------------------------------------------------------------------
#define GPIO_GROUP0     0x0
#define GPIO_GROUP1     0x1
#define GPIO_GROUP2     0x2
#define GPIO_GROUP3     0x3
#define GPIO_GROUP4     0x4
#define GPIO_GROUP5     0x5
#define GPIO_GROUP6     0x6
#define GPIO_GROUP7     0x7
#define GPIO_GROUP8     0x8
#define GPIO_GROUP9     0x9
#define GPIO_GROUP10    0xA
#define GPIO_GROUP11    0xB
#define GPIO_GROUP12    0xC
#define GPIO_GROUP13    0xD

#define BASE_ADDR_GPIO 		REG_BASE_GPIO0
#define BASE_ADDR_GPIO_GROUP0   (BASE_ADDR_GPIO+0x3FC)
#define BASE_ADDR_GPIO_GROUP1   (BASE_ADDR_GPIO+0x5000+0x3FC)
#define BASE_ADDR_GPIO_GROUP2   (BASE_ADDR_GPIO+0x2000+0x3FC)
#define BASE_ADDR_GPIO_GROUP3   (BASE_ADDR_GPIO+0x3000+0x3FC)
#define BASE_ADDR_GPIO_GROUP4   (BASE_ADDR_GPIO+0x4000+0x3FC)
#define BASE_ADDR_GPIO_GROUP5   (BASE_ADDR_GPIO+0x5000+0x3FC)
#define BASE_ADDR_GPIO_GROUP6   (BASE_ADDR_GPIO+0x6000+0x3FC)
#define BASE_ADDR_GPIO_GROUP7   (BASE_ADDR_GPIO+0x7000+0x3FC)
#define BASE_ADDR_GPIO_GROUP8   (BASE_ADDR_GPIO+0x8000+0x3FC)
#define BASE_ADDR_GPIO_GROUP9   (BASE_ADDR_GPIO+0x9000+0x3FC)
#define BASE_ADDR_GPIO_GROUP10  (BASE_ADDR_GPIO+0xA000+0x3FC)
#define BASE_ADDR_GPIO_GROUP11  (BASE_ADDR_GPIO+0xB000+0x3FC)
#define BASE_ADDR_GPIO_GROUP12  (BASE_ADDR_GPIO+0xC000+0x3FC)
#define BASE_ADDR_GPIO_GROUP13  (BASE_ADDR_GPIO+0xD000+0x3FC)

//------------------------------------------------------------------------------
// single GPIO from group define
//------------------------------------------------------------------------------
#define GPIO_0_0	GPIO_0_0_INT	
#define GPIO_0_1	GPIO_0_1_INT	
#define GPIO_0_2	GPIO_0_2_INT	
#define GPIO_0_3	GPIO_0_3_INT	
#define GPIO_0_4	GPIO_0_4_INT	
#define GPIO_0_5	GPIO_0_5_INT	
#define GPIO_0_6	GPIO_0_6_INT	
#define GPIO_0_7	GPIO_0_7_INT	

#define GPIO_1_0	GPIO_1_0_INT	
#define GPIO_1_1	GPIO_1_1_INT	
#define GPIO_1_2	GPIO_1_2_INT	
#define GPIO_1_3	GPIO_1_3_INT	
#define GPIO_1_4	GPIO_1_4_INT	
#define GPIO_1_5	GPIO_1_5_INT	
#define GPIO_1_6	GPIO_1_6_INT	
#define GPIO_1_7	GPIO_1_7_INT	

#define GPIO_2_0	GPIO_2_0_INT	
#define GPIO_2_1	GPIO_2_1_INT	
#define GPIO_2_2	GPIO_2_2_INT	
#define GPIO_2_3	GPIO_2_3_INT	
#define GPIO_2_4	GPIO_2_4_INT	
#define GPIO_2_5	GPIO_2_5_INT	
#define GPIO_2_6	GPIO_2_6_INT	
#define GPIO_2_7	GPIO_2_7_INT	

#define GPIO_3_0	GPIO_3_0_INT	
#define GPIO_3_1	GPIO_3_1_INT	
#define GPIO_3_2	GPIO_3_2_INT	
#define GPIO_3_3	GPIO_3_3_INT	
#define GPIO_3_4	GPIO_3_4_INT	
#define GPIO_3_5	GPIO_3_5_INT	
#define GPIO_3_6	GPIO_3_6_INT	
#define GPIO_3_7	GPIO_3_7_INT	

#define GPIO_4_0	GPIO_4_0_INT	
#define GPIO_4_1	GPIO_4_1_INT	
#define GPIO_4_2	GPIO_4_2_INT	
#define GPIO_4_3	GPIO_4_3_INT	
#define GPIO_4_4	GPIO_4_4_INT	
#define GPIO_4_5	GPIO_4_5_INT	
#define GPIO_4_6	GPIO_4_6_INT	
#define GPIO_4_7	GPIO_4_7_INT	

#define GPIO_5_0	GPIO_5_0_INT	
#define GPIO_5_1	GPIO_5_1_INT	
#define GPIO_5_2	GPIO_5_2_INT	
#define GPIO_5_3	GPIO_5_3_INT	
#define GPIO_5_4	GPIO_5_4_INT	
#define GPIO_5_5	GPIO_5_5_INT	
#define GPIO_5_6	GPIO_5_6_INT	
#define GPIO_5_7	GPIO_5_7_INT	

#define GPIO_6_0	GPIO_6_0_INT	
#define GPIO_6_1	GPIO_6_1_INT	
#define GPIO_6_2	GPIO_6_2_INT	
#define GPIO_6_3	GPIO_6_3_INT	
#define GPIO_6_4	GPIO_6_4_INT	
#define GPIO_6_5	GPIO_6_5_INT	
#define GPIO_6_6	GPIO_6_6_INT	
#define GPIO_6_7	GPIO_6_7_INT	

#define GPIO_7_0	GPIO_7_0_INT	
#define GPIO_7_1	GPIO_7_1_INT	
#define GPIO_7_2	GPIO_7_2_INT	
#define GPIO_7_3	GPIO_7_3_INT	
#define GPIO_7_4	GPIO_7_4_INT	
#define GPIO_7_5	GPIO_7_5_INT	
#define GPIO_7_6	GPIO_7_6_INT	
#define GPIO_7_7	GPIO_7_7_INT	

#define GPIO_8_0	GPIO_8_0_INT	
#define GPIO_8_1	GPIO_8_1_INT	
#define GPIO_8_2	GPIO_8_2_INT	
#define GPIO_8_3	GPIO_8_3_INT	
#define GPIO_8_4	GPIO_8_4_INT	
#define GPIO_8_5	GPIO_8_5_INT	
#define GPIO_8_6	GPIO_8_6_INT	
#define GPIO_8_7	GPIO_8_7_INT	

#define GPIO_9_0	GPIO_9_0_INT	
#define GPIO_9_1	GPIO_9_1_INT	
#define GPIO_9_2	GPIO_9_2_INT	
#define GPIO_9_3	GPIO_9_3_INT	
#define GPIO_9_4	GPIO_9_4_INT	
#define GPIO_9_5	GPIO_9_5_INT	
#define GPIO_9_6	GPIO_9_6_INT	
#define GPIO_9_7	GPIO_9_7_INT	

#define GPIO_10_0	GPIO_10_0_INT   
#define GPIO_10_1	GPIO_10_1_INT   
#define GPIO_10_2	GPIO_10_2_INT   
#define GPIO_10_3	GPIO_10_3_INT   
#define GPIO_10_4	GPIO_10_4_INT   
#define GPIO_10_5	GPIO_10_5_INT   
#define GPIO_10_6	GPIO_10_6_INT   
#define GPIO_10_7	GPIO_10_7_INT   

#define GPIO_11_0	GPIO_11_0_INT   
#define GPIO_11_1	GPIO_11_1_INT   
#define GPIO_11_2	GPIO_11_2_INT   
#define GPIO_11_3	GPIO_11_3_INT   
#define GPIO_11_4	GPIO_11_4_INT   
#define GPIO_11_5	GPIO_11_5_INT   
#define GPIO_11_6	GPIO_11_6_INT   
#define GPIO_11_7	GPIO_11_7_INT   

#define GPIO_12_0	GPIO_12_0_INT   
#define GPIO_12_1	GPIO_12_1_INT   
#define GPIO_12_2	GPIO_12_2_INT   
#define GPIO_12_3	GPIO_12_3_INT   
#define GPIO_12_4	GPIO_12_4_INT   
#define GPIO_12_5	GPIO_12_5_INT   
#define GPIO_12_6	GPIO_12_6_INT   
#define GPIO_12_7	GPIO_12_7_INT   

#define GPIO_13_0	GPIO_13_0_INT   
#define GPIO_13_1	GPIO_13_1_INT   
#define GPIO_13_2	GPIO_13_2_INT   
#define GPIO_13_3	GPIO_13_3_INT   
#define GPIO_13_4	GPIO_13_4_INT   
#define GPIO_13_5	GPIO_13_5_INT   
#define GPIO_13_6	GPIO_13_6_INT   
#define GPIO_13_7	GPIO_13_7_INT   

#define GPIO_ID_BASE	64

//------------------------------------------------------------------------------
// single GPIO by ID define
//------------------------------------------------------------------------------
#define GPIO_1      GPIO_0_0
#define GPIO_2      GPIO_0_1
#define GPIO_3      GPIO_0_2
#define GPIO_4      GPIO_0_3
#define GPIO_5      GPIO_0_4
#define GPIO_6      GPIO_0_5
#define GPIO_7      GPIO_0_6
#define GPIO_8      GPIO_0_7

#define GPIO_9      GPIO_1_0
#define GPIO_10     GPIO_1_1
#define GPIO_11     GPIO_1_2
#define GPIO_12     GPIO_1_3
#define GPIO_13     GPIO_1_4
#define GPIO_14     GPIO_1_5
#define GPIO_15     GPIO_1_6
#define GPIO_16     GPIO_1_7

#define GPIO_17     GPIO_2_0
#define GPIO_18     GPIO_2_1
#define GPIO_19     GPIO_2_2
#define GPIO_20     GPIO_2_3
#define GPIO_21     GPIO_2_4
#define GPIO_22     GPIO_2_5
#define GPIO_23     GPIO_2_6
#define GPIO_24     GPIO_2_7

#define GPIO_25     GPIO_3_0
#define GPIO_26     GPIO_3_1
#define GPIO_27     GPIO_3_2
#define GPIO_28     GPIO_3_3
#define GPIO_29     GPIO_3_4
#define GPIO_30     GPIO_3_5
#define GPIO_31     GPIO_3_6
#define GPIO_32     GPIO_3_7

#define GPIO_33     GPIO_4_0
#define GPIO_34     GPIO_4_1
#define GPIO_35     GPIO_4_2
#define GPIO_36     GPIO_4_3
#define GPIO_37     GPIO_4_4
#define GPIO_38     GPIO_4_5
#define GPIO_39     GPIO_4_6
#define GPIO_40     GPIO_4_7

#define GPIO_41     GPIO_5_0
#define GPIO_42     GPIO_5_1
#define GPIO_43     GPIO_5_2
#define GPIO_44     GPIO_5_3
#define GPIO_45     GPIO_5_4
#define GPIO_46     GPIO_5_5
#define GPIO_47     GPIO_5_6
#define GPIO_48     GPIO_5_7

#define GPIO_49     GPIO_6_0
#define GPIO_50     GPIO_6_1
#define GPIO_51     GPIO_6_2
#define GPIO_52     GPIO_6_3
#define GPIO_53     GPIO_6_4
#define GPIO_54     GPIO_6_5
#define GPIO_55     GPIO_6_6
#define GPIO_56     GPIO_6_7

#define GPIO_57     GPIO_7_0
#define GPIO_58     GPIO_7_1
#define GPIO_59     GPIO_7_2
#define GPIO_60     GPIO_7_3
#define GPIO_61     GPIO_7_4
#define GPIO_62     GPIO_7_5
#define GPIO_63     GPIO_7_6
#define GPIO_64     GPIO_7_7

#define GPIO_65     GPIO_8_0
#define GPIO_66     GPIO_8_1
#define GPIO_67     GPIO_8_2
#define GPIO_68     GPIO_8_3
#define GPIO_69     GPIO_8_4
#define GPIO_70     GPIO_8_5
#define GPIO_71     GPIO_8_6
#define GPIO_72     GPIO_8_7

#define GPIO_73     GPIO_9_0
#define GPIO_74     GPIO_9_1
#define GPIO_75     GPIO_9_2
#define GPIO_76     GPIO_9_3
#define GPIO_77     GPIO_9_4
#define GPIO_78     GPIO_9_5
#define GPIO_79     GPIO_9_6
#define GPIO_80     GPIO_9_7

#define GPIO_81     GPIO_10_0
#define GPIO_82     GPIO_10_1
#define GPIO_83     GPIO_10_2
#define GPIO_84     GPIO_10_3
#define GPIO_85     GPIO_10_4
#define GPIO_86     GPIO_10_5
#define GPIO_87     GPIO_10_6
#define GPIO_88     GPIO_10_7

#define GPIO_89     GPIO_11_0
#define GPIO_90     GPIO_11_1
#define GPIO_91     GPIO_11_2
#define GPIO_92     GPIO_11_3
#define GPIO_93     GPIO_11_4
#define GPIO_94     GPIO_11_5
#define GPIO_95     GPIO_11_6
#define GPIO_96     GPIO_11_7

#define GPIO_97     GPIO_12_0
#define GPIO_98     GPIO_12_1
#define GPIO_99     GPIO_12_2
#define GPIO_100    GPIO_12_3
#define GPIO_101    GPIO_12_4
#define GPIO_102    GPIO_12_5
#define GPIO_103    GPIO_12_6
#define GPIO_104    GPIO_12_7

#define GPIO_105    GPIO_13_0
#define GPIO_106    GPIO_13_1
#define GPIO_107    GPIO_13_2
#define GPIO_108    GPIO_13_3
#define GPIO_109    GPIO_13_4
#define GPIO_110    GPIO_13_5
#define GPIO_111    GPIO_13_6
#define GPIO_112    GPIO_13_7

// GPIO Group bit mask
#define GPIO_GROUP_BIT_POS  3
#define GPIO_GROUP_MASK 	(0xF << GPIO_GROUP_BIT_POS)
#define GPIO_ID_MASK    	((1 << GPIO_GROUP_BIT_POS)-1)

/* GPIO REG */
#define REG_GPIO_DATA                   0x00
#define REG_GPIO_DIR                    0x04
#define REG_GPIO_INTSENSE               0x08
#define REG_GPIO_INTEDGE                0x0C
#define REG_GPIO_INTEVENT               0x10
#define REG_GPIO_INTMASK                0x14
#define REG_GPIO_INTRAWSTATUS           0x18
#define REG_GPIO_INTMASKEDSTATUS        0x1C
#define REG_GPIO_INTCLEAR               0x20
#define REG_GPIO_MODE                   0x24
// GPIO mode select 
typedef enum {
	EGPIO_INT_EDGE = 0,
	EGPIO_INT_LEVEL = 1,
	EGPIO_INT_SENSE_NUM
}e_gpio_sense;

	// GPIO mode select 
typedef enum {
    EGPIO_MODE_SOFT = 0,
    EGPIO_MODE_HARD = 1,
    EGPIO_MODE_NUM
}e_gpio_mode;

// GPIO direction
typedef enum {
    EGPIO_DIR_INPUT = 0,
    EGPIO_DIR_OUTPUT = 1,
    EGPIO_DIR_NUM
}e_gpio_dir;

// GPIO data 
typedef enum {
    EGPIO_DATA_LOW = 0,
    EGPIO_DATA_HIGH = 1,
    EGPIO_DATA_NUM
}e_gpio_data;

// GPIO EDGE
typedef enum {
    EGPIO_EDGE_SINGLE = 0,
    EGPIO_EDGE_DOUBLE = 1,
    EGPIO_EDGE_NUM
}e_gpio_edge;

// GPIO EVENT 
typedef enum {
    EGPIO_EVENT_LOW = 0,
    EGPIO_EVENT_HIGH = 1,
    EGPIO_EVENT_NUM
}e_gpio_event;

// The max number of GPIO group
#define GPIO_GROUP_MAX  14

// The number of GPIO interrupt group
#define GPIO_INTR_GROUP_NUM     GPIO_GROUP_MAX

// The max number of GPIO 
#define ARCH_NR_GPIOS		112

unsigned int hisik3_get_gpio_baseadrr(unsigned int group_id);
e_gpio_dir hisik3_gpio_get_direction(unsigned int gpio_id);
unsigned int hisik3_gpio_set_direction(unsigned int gpio_id, e_gpio_dir dir);
unsigned int hisik3_gpio_setvalue(unsigned int gpio_id, e_gpio_data data);
e_gpio_data hisik3_gpio_getvalue(unsigned int gpio_id);
int hisik3_gpio_initialize(unsigned int gpio_id, e_gpio_dir dir, e_gpio_data data);
int hisik3_gpio_mode_select(unsigned int gpio_id, e_gpio_mode mode);
unsigned int hisik3_gpio_IntrEnable(unsigned int gpio_id);
unsigned int hisik3_gpio_IntrDisable(unsigned int gpio_id);
unsigned int hisik3_gpio_IntrClr(unsigned int gpio_id);
unsigned int hisik3_gpio_Get_IntrStats(unsigned int gpio_id);
unsigned int hisik3_gpio_Get_RawIntrStats(unsigned int gpio_id);
unsigned int hisik3_setup_gpio_regvalue(unsigned long addr, unsigned int bit, unsigned int data);
unsigned int hisik3_get_gpio_baseaddr(unsigned int group_id);
unsigned int hisik3_gpio_setsense(unsigned int gpio_id, e_gpio_sense data);
unsigned int hisik3_gpio_getsense(unsigned int gpio_id);
unsigned int hisik3_gpio_setedge(unsigned int gpio_id, e_gpio_edge data);
unsigned int hisik3_gpio_getedge(unsigned int gpio_id);
unsigned int hisik3_gpio_setevent(unsigned int gpio_id, e_gpio_event data);
unsigned int hisik3_gpio_getevent(unsigned int gpio_id);
int gpio_to_irq(unsigned gpio);
#endif


