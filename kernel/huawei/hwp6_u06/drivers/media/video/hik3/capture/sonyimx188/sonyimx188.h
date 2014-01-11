/* *  sonyimx188 camera driver head file * *  CopyRight (C) Hisilicon Co., Ltd.
*	Author : *  Version:  1.2
* This program is free software; you can
* redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY;without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _SONYIMX188_H
#define _SONYIMX188_H


#include "../isp/k3_isp_io.h"
/***********************************************************************
 *
 * sonyimx188 init sensor registers list
 *
 ***********************************************************************/
const struct _sensor_reg_t sonyimx188_init_regs[] = {
	{0x0100, 0x00}, /*stand by*/
	{0x3094, 0x32},
	{0x309A, 0xA3},
	{0x309E, 0x00},
	{0x3166, 0x1C},
	{0x3167, 0x1B},
	{0x3168, 0x32},
	{0x3169, 0x31},
	{0x316A, 0x1C},
	{0x316B, 0x1B},
	{0x316C, 0x32},
	{0x316D, 0x31},
	{0x0101, 0x00},//add
	{0x0100, 0x00}, /*stand by*/
	//PLL Setting (INCK=20MHz)		
	{0x0305, 0x02},
	{0x0307, 0x2A},
	{0x303C, 0x3F},
	{0x30A4, 0x02},
	//Mode Setting		
	{0x0112, 0x0A},
	{0x0113, 0x0A},
	{0x0340, 0x03},
	{0x0341, 0x84},
	{0x0342, 0x06},
	{0x0343, 0x13},
	{0x0344, 0x00},
	{0x0345, 0x10},
	{0x0346, 0x00},
	{0x0347, 0x08},
	{0x0348, 0x05},
	{0x0349, 0x0F},
	{0x034A, 0x03},
	{0x034B, 0x29},
	{0x034C, 0x05},
	{0x034D, 0x00},
	{0x034E, 0x03},
	{0x034F, 0x22},
	{0x0381, 0x01},
	{0x0383, 0x01},
	{0x0385, 0x01},
	{0x0387, 0x01},
	{0x3040, 0x08},
	{0x3041, 0x97},
	{0x3048, 0x00},
	{0x304E, 0x0A},
	{0x3050, 0x01},
	{0x309B, 0x00},
	{0x30D5, 0x00},
	{0x31A1, 0x07},
	{0x31A2, 0x00},
	{0x31A3, 0x00},
	{0x31A4, 0x00},
	{0x31A5, 0x00},
	{0x31AA, 0x00},
	{0x31AB, 0x00},
	{0x31AC, 0x00},
	{0x31AD, 0x00},
	{0x31B0, 0x00},
	{0x3301, 0x05},
	{0x3318, 0x66},
	{0x0100, 0x01}, /*stream on*/
};

const struct _sensor_reg_t sonyimx188_framesize_full_30fps[] = {
};

#endif /* SONYIMX188_H_INCLUDED */

/************************** END ***************************/
