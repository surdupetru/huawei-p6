/* *  hi542 camera driver head file * *  CopyRight (C) Hisilicon Co., Ltd.
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

#ifndef _HI542_H
#define _HI542_H


#include "../isp/k3_isp_io.h"
/***********************************************************************
 *
 * hi542 init sensor registers list
 *
 ***********************************************************************/
const struct _sensor_reg_t hi542_init_regs[] = {
//SW reset, LDO setting
{0x0001, 0x02},/* SW reset *///PWRCTLA//B[1]=soft reset, B[0]=sleep mode(0:normal operation, 1:sleep mode enable)
{0x0001, 0x01},/* SW sleep *///PWRCTLA//B[1]=soft reset, B[0]=sleep mode(0:normal operation, 1:sleep mode enable)
{0x03d4, 0x18},//[5:4]LDO level control
{0x03D2, 0xAD},//PLL reset
{0x0616, 0x00},//D-PHY reset
{0x0616, 0x01},//D-PHY reset disable
{0x03D2, 0xAC},//PLL reset disable
{0x03D0, 0xe9},
{0x03D1, 0x74},//for 20MHz
{0x0800, 0x07},//EMI disable
{0x0801, 0x08},
{0x0802, 0x02},

{0x0011, 0x94}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0020, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0021, 0x08}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0022, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0023, 0x18}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0038, 0x02}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0039, 0x2C}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x003C, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x003D, 0x0C}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x003E, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x003F, 0x0C}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0040, 0x00}, //Hblank H
{0x0041, 0x35}, //2E} Hblank L
{0x0042, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0043, 0x11}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0045, 0x07}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0046, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0047, 0xD0}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0050, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0052, 0x10}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0053, 0x10}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0054, 0x10}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0055, 0x08}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0056, 0x80}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0057, 0x08}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0058, 0x08}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0059, 0x08}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x005A, 0x08}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x005B, 0x02}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0070, 0x00}, //03}, EMI OFF
{0x0081, 0x01},//09}, //0B}, BLC scheme
{0x0082, 0x23}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0083, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0085, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0086, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x008C, 0x02}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x00A0, 0x0f},//0C},//0B}, RAMP DC OFFSET
{0x00A1, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x00A2, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x00A3, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x00A4, 0xFF}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x00A5, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x00A6, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x00A8, 0x7F}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x00A9, 0x7F}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x00AA, 0x7F}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x00B4, 0x00}, //08}, BLC offset
{0x00B5, 0x00}, //08},
{0x00B6, 0x02}, //07},
{0x00B7, 0x01}, //07},
{0x00D4, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x00D5, 0xaa},//a9}, RAMP T1
{0x00D6, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x00D7, 0xc9}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x00D8, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x00D9, 0x59}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x00DA, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x00DB, 0xb0}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x00DC, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x00DD, 0xc9},//c5}, /*rp_rst_flg_on1*/
{0x011C, 0x1F}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x011D, 0xFF}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x011E, 0xFF}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x011F, 0xFF}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x012A, 0xFF}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x012B, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0129, 0x40}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0210, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0212, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0213, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0216, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0219, 0x33},//66}, Pixel bias
{0x021B, 0x55}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x021C, 0x85}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x021D, 0xFF}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x021E, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x021F, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0220, 0x02}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0221, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0222, 0xA0}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0227, 0x0A}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0228, 0x5C}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0229, 0x2D}, //01},//41},//00},//2C}, RAMP swing range jwryu120120
{0x022A, 0x04}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x022C, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x022D, 0x23}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0237, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0238, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0239, 0xA5}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x023A, 0x20}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x023B, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x023C, 0x22}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x023F, 0x80}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0240, 0x04}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0241, 0x07}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0242, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0243, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0244, 0x80}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0245, 0xE0}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0246, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0247, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x024A, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x024B, 0x14}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x024D, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x024E, 0x03}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x024F, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0250, 0x53}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0251, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0252, 0x07}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0253, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0254, 0x4F}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0255, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0256, 0x07}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0257, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0258, 0x4F}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0259, 0x0C}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x025A, 0x0C}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x025B, 0x0C}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x026C, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x026D, 0x09}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x026E, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x026F, 0x4B}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0270, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0271, 0x09}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0272, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0273, 0x4B}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0274, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0275, 0x09}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0276, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0277, 0x4B}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0278, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0279, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x027A, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x027B, 0x55}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x027C, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x027D, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x027E, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x027F, 0x5E}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0280, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0281, 0x03}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0282, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0283, 0x45}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0284, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0285, 0x03}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0286, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0287, 0x45}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0288, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0289, 0x5c}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x028A, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x028B, 0x60}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x02A0, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x02A1, 0xe0}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x02A2, 0x02}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x02A3, 0x22}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x02A4, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x02A5, 0x5C}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x02A6, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x02A7, 0x60}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x02A8, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x02A9, 0x5C}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x02AA, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x02AB, 0x60}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x02D2, 0x0F}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x02DB, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x02DC, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x02DD, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x02DE, 0x0C}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x02DF, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x02E0, 0x04}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x02E1, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x02E2, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x02E3, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x02E4, 0x0F}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x02F0, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x02F1, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0310, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0311, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0312, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0313, 0x5A}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0314, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0315, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0316, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0317, 0x5A}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0318, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0319, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x031A, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x031B, 0x2F}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x031C, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x031D, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x031E, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x031F, 0x2F}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0320, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0321, 0xAB}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0322, 0x02}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0323, 0x55}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0324, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0325, 0xAB}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0326, 0x02}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0327, 0x55}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0328, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0329, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x032A, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x032B, 0x10}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x032C, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x032D, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x032E, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x032F, 0x10}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0330, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0331, 0x02}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0332, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0333, 0x2e}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0334, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0335, 0x02}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0336, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0337, 0x2e}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0358, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0359, 0x46}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x035A, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x035B, 0x59}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x035C, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x035D, 0x46}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x035E, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x035F, 0x59}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0360, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0361, 0x46}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0362, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0363, 0xa4}, //a2}, Black sun
{0x0364, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0365, 0x46}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0366, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0367, 0xa4}, //a2}, Black sun
{0x0368, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0369, 0x46}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x036A, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x036B, 0xa6},//a9}, S2 off
{0x036C, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x036D, 0x46}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x036E, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x036F, 0xa6},//a9}, S2 off
{0x0370, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0371, 0xb0}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0372, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0373, 0x59}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0374, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0375, 0xb0}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0376, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0377, 0x59}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0378, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0379, 0x45}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x037A, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x037B, 0xAA}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x037C, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x037D, 0x99}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x037E, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x037F, 0xAE}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0380, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0381, 0xB1}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0382, 0x02}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0383, 0x56}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0384, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0385, 0x6D}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0386, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0387, 0xDC}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x03A0, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x03A1, 0x5E}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x03A2, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x03A3, 0x62}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x03A4, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x03A5, 0xc9}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x03A6, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x03A7, 0x27}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x03A8, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x03A9, 0x59}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x03AA, 0x02}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x03AB, 0x55}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x03AC, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x03AD, 0xc5}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x03AE, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x03AF, 0x27}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x03B0, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x03B1, 0x55}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x03B2, 0x02}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x03B3, 0x55}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x03B4, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x03B5, 0x0A}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x03D3, 0x08}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x03D5, 0x44}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x03D6, 0x51}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */// hynix_test : 0x54-->0x51
{0x03D7, 0x56}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x03D8, 0x44}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x03D9, 0x06}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0580, 0x81}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0581, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0582, 0x80}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0583, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0584, 0x80}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0585, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0586, 0x80}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0587, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0588, 0x80}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0589, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x058A, 0x80}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x05C2, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x05C3, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0080, 0xC7}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0119, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x011A, 0x15}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x011B, 0xC0}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0115, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0116, 0x2A}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0117, 0x4C}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0118, 0x20}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0223, 0xED}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0224, 0xE4}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0225, 0x09}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0226, 0x36}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x023E, 0x80}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x05B0, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */

//{0x03D2, 0xAD}, //PLL reset 20120418 ryu add
//{0x0616, 0x00}, //D-PHY reset 20120418 ryu add
//{0x0616, 0x01}, //D-PHY reset disable 20120418 ryu add
//{0x03D2, 0xAC}, //PLL reset disable 20120418 ryu add
//{0x03D0, 0xE9}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
//{0x03D1, 0x74}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
//{0x0800, 0x07}, //0F}, EMI disable
//{0x0801, 0x08}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */

{0x0012, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0013, 0x40}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0024, 0x07}, /* windowing */
{0x0025, 0xA0},//A8}, /* windowing */
{0x0026, 0x0A}, /* windowing */
{0x0027, 0x30},//30}, /* windowing */
{0x0030, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0031, 0xFF}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0032, 0x06}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0033, 0xB0}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0034, 0x02}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0035, 0xD8}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x003A, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x003B, 0x2E}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x004A, 0x03}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x004B, 0xC0}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x004C, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x004D, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0C98, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0C99, 0x5E}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0C9A, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0C9B, 0x62}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x05A0, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0084, 0x30},//10}, BLC control
{0x008D, 0xFF}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0090, 0x02},//0b}, BLC defect pixel th
{0x00A7, 0x80},//FF}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x021A, 0x15},//05}, /*CDS bias */
{0x022B, 0xb0},//f0}, RAMP filter
{0x0232, 0x37}, //17}, black sun enable
{0x0010, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0740, 0x1A}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0742, 0x1A}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0743, 0x1A}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0744, 0x1A}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0745, 0x04}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0746, 0x32}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0747, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0748, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0749, 0x90}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x074A, 0x1A}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x074B, 0xB1}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
//{0x0500, 0x1f}, //0x19}, //1b}, LSC disable
{0x0510, 0x10}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */

//{0x0500, 0x15},
//{0x0580, 0x81},

{0x0589, 0x00},
{0x058A, 0x80},

{0x0217, 0x44}, //adaptive NCP on
{0x0218, 0x00}, //scn_sel

{0x02ac, 0x00}, //outdoor on
{0x02ad, 0x00},
{0x02ae, 0x00}, //outdoor off
{0x02af, 0x00},
{0x02b0, 0x00}, //indoor on
{0x02b1, 0x00},
{0x02b2, 0x00}, //indoor off
{0x02b3, 0x00},
{0x02b4, 0x60}, //dark1 on
{0x02b5, 0x21},
{0x02b6, 0x66}, //dark1 off
{0x02b7, 0x8a},

{0x02c0, 0x36}, //outdoor NCP en
{0x02c1, 0x36}, //indoor NCP en
{0x02c2, 0x36}, //dark1 NCP en
{0x02c3, 0x36}, //3f}, //dark2 NCP disable jwryu 20120120
{0x02c4, 0xE4}, //outdoor NCP voltage
{0x02c5, 0xE4}, //indoor NCP voltage
{0x02c6, 0xE4}, //dark1 NCP voltage
{0x02c7, 0xdb}, //24}, //dark2 NCP voltage

{0x061A, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x061B, 0x04},//03},ryu 20120416 /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x061C, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x061D, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x061E, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x061F, 0x03}, /* ryu 20120416 man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0613, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0615, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0616, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0617, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0619, 0x01}, //gated clk mode 20120418 ryu add
{0x0651, 0x02},
{0x0652, 0x10},
{0x0654, 0x0a},
{0x0655, 0x05},

{0x0008, 0x0F}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0630, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0631, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0632, 0x03}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0633, 0xC0}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */

//LSC
{0x0540, 0x10}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0541, 0x1f}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0550, 0x80}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0551, 0x80}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0552, 0x80}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0553, 0x80}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0554, 0x52}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0555, 0x52}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0556, 0x52}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0571, 0x60}, //60}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */

{0x0663, 0x05},//0a}, trail time /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
{0x0660, 0x03}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */

{0x0020, 0x00},
{0x0021, 0x04},
{0x0022, 0x00},
{0x0023, 0x08},

{0x0024, 0x07},
{0x0025, 0xA0},
{0x0026, 0x0A},
{0x0027, 0x20},

{0x0119, 0x00},
{0x011A, 0x2b},
{0x011B, 0x9c},

{0x0010, 0x00},
//{0x0011, 0x40},
{0x0500, 0x1f},

{0x0630, 0x0A},
{0x0631, 0x20},
{0x0632, 0x07},
{0x0633, 0xA0},//A8},

{0x0042, 0x01},
{0x0043, 0x3d},
#if 0
{0x0540, 0x16},
{0x0557, 0xFE},
{0x0558, 0xFC},
#endif
};

const struct _sensor_reg_t hi542_framesize_full_15fps[] = {

//Stand by
{0x0632, 0x00}, //mipi_act_row_size_h, B[3:0]=MIPI active row size higher byte
{0x0633, 0x00}, //mipi_act_row_size_l, B[7:0]=MIPI active row size lower byte, Legal value : 0~4095={mipi_active_row_h,mipi_active_row_l}
{0x0001, 0x01}, //PWRCTLA//B[1]=soft reset, B[0]=sleep mode(0:normal operation, 1:sleep mode enable)
{0x0617, 0x01}, //mipi_en_n, B[0]=MIPI Tx enable (0:On, 1:Off)
{0x0001, 0x00}, //PWRCTLA//B[1]=soft reset, B[0]=sleep mode(0:normal operation, 1:sleep mode enable)
{0x0001, 0x01}, //PWRCTLA//B[1]=soft reset, B[0]=sleep mode(0:normal operation, 1:sleep mode enable)
{0x0617, 0x00}, //mipi_en_n, B[0]=MIPI Tx enable (0:On, 1:Off)

//Capture,2592x1952
{0x0020, 0x00},
{0x0021, 0x04},
{0x0022, 0x00},
{0x0023, 0x08},

{0x0024, 0x07},
{0x0025, 0xA0},
{0x0026, 0x0A},
{0x0027, 0x20},

{0x0010, 0x00},
//{0x0011, 0x04},
{0x0500, 0x17}, //0x13,0x11},

{0x0630, 0x0A},
{0x0631, 0x20},
{0x0632, 0x07},
{0x0633, 0xA0},

//Start streaming
{0x0001, 0x00}, //PWRCTLA//B[1]=soft reset, B[0]=sleep mode(0:normal operation, 1:sleep mode enable)
};

const struct _sensor_reg_t hi542_framesize_pre_30fps[] = {

//stand by
{0x0632, 0x00}, //mipi_act_row_size_h, B[3:0]=MIPI active row size higher byte
{0x0633, 0x00}, //mipi_act_row_size_l, B[7:0]=MIPI active row size lower byte, Legal value : 0~4095={mipi_active_row_h,mipi_active_row_l}
{0x0001, 0x01}, //PWRCTLA//B[1]=soft reset, B[0]=sleep mode(0:normal operation, 1:sleep mode enable)
{0x0617, 0x01}, //mipi_en_n, B[0]=MIPI Tx enable (0:On, 1:Off)
{0x0001, 0x00}, //PWRCTLA//B[1]=soft reset, B[0]=sleep mode(0:normal operation, 1:sleep mode enable)
{0x0001, 0x01}, //PWRCTLA//B[1]=soft reset, B[0]=sleep mode(0:normal operation, 1:sleep mode enable)
{0x0617, 0x00}, //mipi_en_n, B[0]=MIPI Tx enable (0:On, 1:Off)

//Preview,1280x960
{0x0020, 0x00},
{0x0021, 0x04},
{0x0022, 0x00},
{0x0023, 0x0C},

{0x0024, 0x07}, /* windowing */
{0x0025, 0xA0}, /* windowing */
{0x0026, 0x0A}, /* windowing */
{0x0027, 0x30}, /* windowing */
//{0x0119, 0x00},
//{0x011A, 0x15},
//{0x011B, 0xC0},

{0x0010, 0x01},
//{0x0011, 0x04},
{0x0500, 0x1F}, //1B}, LSC OFF

{0x0630, 0x05},
{0x0631, 0x00},
{0x0632, 0x03},
{0x0633, 0xC0},
//Start streaming
{0x0001, 0x00}, //PWRCTLA//B[1]=soft reset, B[0]=sleep mode(0:normal operation, 1:sleep mode enable)
};

const struct _sensor_reg_t hi542_1080P[] = {
//stand by
{0x0632, 0x00}, //mipi_act_row_size_h, B[3:0]=MIPI active row size higher byte
{0x0633, 0x00}, //mipi_act_row_size_l, B[7:0]=MIPI active row size lower byte, Legal value : 0~4095={mipi_active_row_h,mipi_active_row_l}
{0x0001, 0x01}, //PWRCTLA//B[1]=soft reset, B[0]=sleep mode(0:normal operation, 1:sleep mode enable)
{0x0617, 0x01}, //mipi_en_n, B[0]=MIPI Tx enable (0:On, 1:Off)
{0x0001, 0x00}, //PWRCTLA//B[1]=soft reset, B[0]=sleep mode(0:normal operation, 1:sleep mode enable)
{0x0001, 0x01}, //PWRCTLA//B[1]=soft reset, B[0]=sleep mode(0:normal operation, 1:sleep mode enable)
{0x0617, 0x00}, //mipi_en_n, B[0]=MIPI Tx enable (0:On, 1:Off)

//Preview,1920x1088
{0x0020, 0x00},
{0x0021, 0x00},
{0x0022, 0x01},
{0x0023, 0x58},

{0x0024, 0x04}, /* windowing */
{0x0025, 0x40}, /* windowing */
{0x0026, 0x07}, /* windowing */
{0x0027, 0x80}, /* windowing */

{0x0010, 0x40},
//{0x0011, 0x04},
{0x0500, 0x13}, //1B}, LSC OFF

{0x0030, 0x01},
{0x0031, 0xB7},
{0x0032, 0x05},
{0x0033, 0xF8},
{0x0034, 0x04},
{0x0035, 0x40},

{0x0630, 0x07},
{0x0631, 0x80},
{0x0632, 0x04},
{0x0633, 0x40},
//stand by
{0x0001, 0x00}, //PWRCTLA//B[1]=soft reset, B[0]=sleep mode(0:normal operation, 1:sleep mode enable)
};
#endif /* HI542_H_INCLUDED */

/************************** END ***************************/
