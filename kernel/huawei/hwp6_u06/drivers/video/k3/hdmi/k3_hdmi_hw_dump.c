/* Copyright (c) 2008-2011, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include <linux/kernel.h>

#include "k3_hdmi_hw.h"
#include "k3_hdmi_log.h"

#define BIT_0   0
#define BIT_1   1
#define BIT_2   2
#define BIT_3   3
#define BIT_4   4
#define BIT_5   5
#define BIT_6   6
#define BIT_7   7

#define BIT_MAX 8

#define DUMPREGVAL(v, r) printk("\n%-30s, addr=%02x, val=%02x\n", #r, (unsigned int)r/4, v);
#define DUMPREG(g, r) printk("\n%-30s, addr=%02x, val=%02x\n", #r, (unsigned int)r/4, read_reg(g, r));
#define DUMPONEBIT(bit, val, str0, str1, label) printk("Bit %d: Label: %s; val: %d -- %s\n", bit, label, FLD_GET(val, bit, bit), (FLD_GET(val, bit, bit) ? str1 : str0));
#define DUMPBITS(start, end, val, label) printk("Bit %d:%d Label: %s; val: %d\n", start, end, label, FLD_GET(val, start, end));
#define DUMPBITLOG(bit, val, label) printk("Bit %d: Label: %s; val: %d\n", bit, label, FLD_GET(val, bit, bit));

void dump_reg(void)
{
    u32 val = 0;

    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_SYS_VND_IDL);
    printk("Provides unique vendor identification through I2C  Default:0x01(Low)\n");
    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_SYS_DEV_IDL);
    printk("Provides unique device type identification through I2C.  Default:0x36 (Low)\n");
    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_SYS_DEV_IDH);
    printk("Provides unique device type identification through I2C.  Default:0x97 (High)\n");
    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_SYS_DEV_REV);
    printk("Allows distinction between revisions of same device.  Default:0x00\n");

    val = read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_SRST);
    DUMPREGVAL(val, HDMI_CORE_SYS_SRST);
    DUMPONEBIT(BIT_0, val, "0 = Normal Operation", "1 = Reset all sections, including audio FIFO, but not writable registers or HDCP", "SWRST; Software reset");
    DUMPONEBIT(BIT_1, val, "0 = Normal Operation", "1 = Reset (flush) audio FIFO", "FIFORST; Audio FIFO reset");

    val = read_reg(HDMI_CORE_SYS, HDMI_CORE_CTRL1);
    DUMPREGVAL(val, HDMI_CORE_CTRL1);
    DUMPONEBIT(BIT_0, val, "When LOW, interrupts are in power-down mode.", "HIGH is normal operation.", "Power down mode");
    DUMPONEBIT(BIT_1, val, "0 = Latch Input on Falling Edge", "1 = Latch Input on Rising Edge", "Edge select");
    DUMPONEBIT(BIT_2, val, "0 = 12-bit", "1 = 24-bit", "Input bus select");
    DUMPONEBIT(BIT_4, val, "0 = Fixed LOW", "1 = Follow VSYNC input", "HSYNC enable");
    DUMPONEBIT(BIT_5, val, "0 = Fixed LOW", "1 = Follow VSYNC input", "VSYNC enable");
    DUMPBITLOG(BIT_6, val, "The current status of the VSYNC input pin. Refer to the INTR2 register (0x72:0x72), described on page 30, for an interrupt tied to VSYNC active edge.");

    val = read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_SYS_STAT);
    DUMPREGVAL(val, HDMI_CORE_SYS_SYS_STAT);
    DUMPBITLOG(BIT_0, val, "IDCK (io_pclkpin) to TMDS clock (v_ck2x) is stable and the Transmitter can send reliable data on the TMDS link. A change to the IDCK sets this bit LOW. After a subsequent LOW to HIGH transition, indicating a stable input clock, a software reset is recommended.");
    DUMPBITLOG(BIT_1, val, "Hot Plug Detect: The state of the Hot Plug Detect pin can be read here.");
    DUMPONEBIT(BIT_2, val, "0 = No Receiver connected", "1 = Receiver is connected and powered on", "Receiver Sense (works in DC-coupled systems only)");
    DUMPBITLOG(BIT_7, val, "VREF mode. Always HIGH");

    val = read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_DATA_CTRL);
    DUMPREGVAL(val, HDMI_CORE_SYS_DATA_CTRL);
    DUMPONEBIT(BIT_0, val, "When connected to 1b0, enables firmware to take the decision whether to send unencrypted data or not.", "By connecting to 1b1, HDMI Tx will be able to send ONLY encrypted data.", "This register gets the value of the pin io_hdcp_sel.");
    DUMPONEBIT(BIT_1, val, "0 = Do not send zeros in audio packet", "1 = Send zeros in audio packet", "AUD_MUTE");
    DUMPONEBIT(BIT_2, val, "0 = No Receiver connected", "1 = Receiver is connected and powered on", "Receiver Sense (works in DC-coupled systems only)");

    val = read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_VID_CTRL);
    DUMPREGVAL(val, HDMI_CORE_SYS_VID_CTRL);
    DUMPBITS(BIT_1, BIT_0, val, "Clock mode:\n 0b00 = Pixel data is not replicated\n 0b01 = Pixels are replicated once (each sent twice)\n 0b10 = RSVD\n 0b11 = Pixels are replicated 4 times (each sent four times)");
    DUMPONEBIT(BIT_4, val, "0 = BT.601 conversion", "1 = BT.709 conversion", "Color Space Conversion Standard select");
    DUMPONEBIT(BIT_5, val, "0 = All 8-bit input modes", "1 = All 12-bit 4:2:2 input modes", "Extended Bit mode");
    DUMPBITLOG(BIT_6, val, "Do not write this bit to 1.");
    DUMPONEBIT(BIT_7, val, "0 = Do not invert field bit","1 = Invert field bit", "Invert field polarity");

    
    val = read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_VID_ACEN);
    DUMPREGVAL(val, HDMI_CORE_SYS_VID_ACEN);
    DUMPONEBIT(BIT_0, val, "0 = Disabled", "1 = Enabled", "Enable down sampler 4:4:4 to 4:2:2");
    DUMPONEBIT(BIT_1, val, "0 = Disabled", "1 = Enabled", "Enable Range Compress 0-255 to 16-234");
    DUMPONEBIT(BIT_2, val, "0 = Disabled", "1 = Enabled", "Enable RGB to YCbCr color-space converter");
    DUMPONEBIT(BIT_3, val, "0 = Disabled", "1 = Enabled", "Enable Range Clip from 16 to 235 (RGB and Y)/ 240 (CbCr)");
    DUMPONEBIT(BIT_4, val, "0 = Output color space is RGB", "1 = Output color space is YCbCr", "Identifies the output color space on the link - used by the Clipper block to determine which way to clip");
    DUMPBITS(BIT_7, BIT_6, val, "Identifies the number of bits per input video channel: \n 0b00 = 8 bits per channel or 24-bit bus mode \n 0b01 = 10 bits per channel or 30-bit bus mode\n 0b10 = 12 bits per channel or 36-bit bus mode\n 0b11 = Reserved");
  
    val = read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_VID_MODE);
    DUMPREGVAL(val, HDMI_CORE_SYS_VID_MODE);
    DUMPONEBIT(BIT_0, val, "0 = Disabled", "1 = Enabled", "Embedded Sync Extraction");
    DUMPONEBIT(BIT_1, val, "0 = Disabled", "1 = Enabled", "One- to Two-Data-Channel Demux");
    DUMPONEBIT(BIT_2, val, "0 = Disabled", "1 = Enabled", "Upsampling 4:2:2 to 4:4:4");
    DUMPONEBIT(BIT_3, val, "0 = Disabled", "1 = Enabled", "YcbCr to RGB Color Space Conversion");
    DUMPONEBIT(BIT_4, val, "0 = Disabled", "1 = Enabled", "Data Range 16-to-235 to 0-to-255 expansion");
    DUMPONEBIT(BIT_5, val, "0 = Dither disabled", "1 = Dither enabled", "the video output is truncated to the output width specified in DITHER_MODE [7:6]");
    DUMPBITS(BIT_7, BIT_6, val, "Identifies the number of bits per output video channel: \n 0b00 = Dither to 8 bits\n 0b01 = Dither to 10 bits\n 0b10 = Dither to 12 bits\n 0b11 = Reserved");
    
    val = read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_INTR_STATE);
    DUMPREGVAL(val, HDMI_CORE_SYS_INTR_STATE);
    DUMPBITLOG(BIT_7, val, "Interrupt State. When an interrupt is asserted, this bit is set to 1. The polarity of the INT output signal is set using this bit and the POLARITY# bit in the INT_CTRL register (0x72:0x79). Only INTR1, INTR2, INTR3, and INTR4 bits with matching set bits in INT_UNMASK can contribute to setting the INTR bit.");

    
    val = read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_INTR1);
    DUMPREGVAL(val, HDMI_CORE_SYS_INTR1);
    DUMPBITLOG(BIT_0, val, "Audio FIFO Underflow.\n Similar to OVER_RUN. This interrupt occurs when the audio FIFO empties");
    DUMPBITLOG(BIT_1, val, "Audio FIFO Overflow.\n This interrupt occurs if the audio FIFO overflows when more samples are written into it than are drawn out across the HDMI link. Such a condition can occur from a transient change in the Fs or pixel clock rate.");
    DUMPBITLOG(BIT_2, val, "Input counted past frame count threshold set in RI_128_COMP register.\n This interrupt occurs when the count written to register 0x72:0x24 is matched by the VSYNC (frame) counter in the HDMI Transmitter. It should trigger the firmware to perform a link integrity check. Such a match occurs every 128 frames.");
    DUMPBITLOG(BIT_3, val, "Input S/PDIF stream has bi-phase error. This can occur when there is noise or an Fs rate change on the S/PDIF input.");
    DUMPBITLOG(BIT_4, val, "New preamble forced to drop sample (S/PDIF input only). \n If the HDMI Transmitter detects an 8-bit preamble in the S/PDIF input stream before the subframe has been captured, this interrupt is set. A S/PDIF input that stops signaling or a flat line condition can create such a premature preamble.");
    DUMPBITLOG(BIT_5, val, "Receiver Sense Interrupt asserted if RSEN has changed. \n This interrupt is set whenever VCC is applied to, or removed from, the attached HDMI Receiver chip. A Receiver with multiple input ports can also disconnect the TMDS termination to the unused port, which triggers this RSEN interrupt.");
    DUMPBITLOG(BIT_6, val, "Monitor Detect Interrupt - asserted if Hot Plug Detect has changed state. \n The HDMI Transmitter signals a change in the connectivity to a Sink, either unplug or plug. HDMI specifies that Hot Plug be active only when the Sinks EDID is ready to be read and that Hot Plug be toggled any time there is a change in connectivity downstream of an attached Repeater.");
    DUMPBITLOG(BIT_7, val, "Software Induced Interrupt - allows the firmware to generate an interrupt directly.");

    val = read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_INTR2);
    DUMPREGVAL(val, HDMI_CORE_SYS_INTR2);
    DUMPBITLOG(BIT_0, val, "Asserted when VSYNC active edge is recognized. It is useful for triggering firmware actions that occur during vertical blanking.");
    DUMPBITLOG(BIT_1, val, "TCLK_STABLE (register 0x72:0x09[0]) changes state. \n Whenever IDCK changes, there is a temporary instability in the internal clocking. This interrupt is set when the internal clocking has stabilized.");
    DUMPBITLOG(BIT_2, val, "ACR Packet Overwrite. \n This interrupt occurs if the HDMI Transmitter puts a NCTS packet into the queue before the previous NCTS packet has been sent. This can happen if very long active data times do not allow for sufficient NCTS packet bandwidth. For all CEA-861D modes, no ACR_OVR interrupt should occur.");
    DUMPBITLOG(BIT_3, val, "S/PDIF Parity Error. \n The S/PDIF stream includes a parity (P) bit at the end of each sub-frame. An interrupt occurs if the calculated parity does not match the state of this bit.");
    DUMPBITLOG(BIT_4, val, "This condition is the opposite of the condition that causes DROP_SAMPLE (0x72:0x71[4]). This interrupt occurs if a preamble is expected but not found when decoding the S/PDIF stream.");
    DUMPBITLOG(BIT_5, val, "The ENC_EN bit (0x72.0x0F[0]) changed from 1 to 0. \n This interrupt occurs if encryption is turned off.");
    DUMPBITLOG(BIT_6, val, "S/PDIF Parity Error. \n The S/PDIF stream includes a parity (P) bit at the end of each sub-frame. An interrupt occurs if the calculated parity does not match the state of this bit.");
    DUMPBITLOG(BIT_7, val, "If set, this interrupt detected that the FIFORDY bit (0x74:0x40[5]) is set to 1.");

    val = read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_INTR3);
    DUMPREGVAL(val, HDMI_CORE_SYS_INTR3);
    DUMPBITLOG(BIT_0, val, "DDC FIFO is empty.");
    DUMPBITLOG(BIT_1, val, "DDC FIFO is full.");
    DUMPBITLOG(BIT_2, val, "DDC FIFO is half full.");
    DUMPBITLOG(BIT_3, val, "DDC command is complete.");
    DUMPBITLOG(BIT_4, val, "Ri and Ri do not match during frame #127 (ICNT 每1).");
    DUMPBITLOG(BIT_5, val, "Ri and Ri do not match during frame #0 (ICNT).");
    DUMPBITLOG(BIT_6, val, "Ri did not change between frame #127 and #0.");
    DUMPBITLOG(BIT_7, val, "Ri not read within one frame.");
    
    val = read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_INTR4);
    DUMPREGVAL(val, HDMI_CORE_SYS_INTR4);
    DUMPBITLOG(BIT_0, val, "DSD stream got invalid sequence: more then 24 bits of the same value. Asserted if set to 1. Write 1 to clear");
    DUMPBITLOG(BIT_1, val, "reg_intr4_stat1");
    DUMPBITLOG(BIT_2, val, "reg_intr4_stat2");
    DUMPBITLOG(BIT_3, val, "reg_intr4_stat3 CEC interrupt");

    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_SYS_INTR1_MASK);
    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_SYS_INTR2_MASK);
    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_SYS_INTR3_MASK);
    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_SYS_INTR4_MASK);
    printk("Each bit corresponds to one bit in INTR1, INTR2, INTR3, or INTR4: \n 0 = Disable corresponding interrupt from INT output \n 1 = Enable corresponding interrupt to INT output\n");
    
    val = read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_TMDS_CTRL);
    DUMPREGVAL(val, HDMI_CORE_SYS_TMDS_CTRL);
    DUMPONEBIT(BIT_0, val, "0 = Disable", "1 = Enable", "Internal source termination");
    DUMPBITLOG(BIT_1, val, "1: BGR output is connected to ext_swing pin");
    DUMPBITS(BIT_3, BIT_2, val, "Output Swing Control \n 11: -18% \n 10: -9% \n 01: 0% \n 00: +9%");
    DUMPBITS(BIT_6, BIT_5, val, "Selects FPLL multiple of the IDCK: \n 0b00 = FPLL is 0.5*IDCK \n 0b01 = FPLL is 1.0*IDCK \n 0b10 = FPLL is 2.0*IDCK \n 0b11 = FPLL is 4.0*IDCK");

    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_SYS_HDCP_CTRL);
    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_SYS_HDCP_BKSV_ADDR);
    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_SYS_HDCP_AN_ADDR);
    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_SYS_HDCP_AKSV_ADDR);
    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_SYS_HDCP_Ri_ADDR);
    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_SYS_HDCP_RI_STAT);
    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_SYS_HDCP_RI_CMD_ADDR);
    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_SYS_HDCP_RI_START);
    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_SYS_HDCP_RI_RX_1);
    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_SYS_HDCP_RI_RX_2);
    
    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_SYS_DE_DLY);
    printk("Width of the area to the left of the active display. The unit of measure is pixels. This register should be set to the sum of (HSYNC width) + (horizontal back porch) + (horizontal left border), and is used only for DE generation. \n Note: This 12-bit value includes four bits from register 0x72:0x33. \n The valid range is 1-4095. 0 is invalid.\n");
    
    val = read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_DE_CTRL);
    DUMPREGVAL(val, HDMI_CORE_SYS_DE_CTRL);
    DUMPONEBIT(BIT_4, val, "0 = Positive polarity (leading edge rises).", "1 = Negative polarity (leading edge falls).", "HSYNC polarity");
    DUMPONEBIT(BIT_5, val, "0 = Positive polarity (leading edge rises).", "1 = Negative polarity (leading edge falls).", "VSYNC polarity");
    DUMPONEBIT(BIT_6, val, "0 = Disabled", "1 = Enabled", "Generate DE signal");

    val = FLD_GET(read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_DE_CTRL), 3, 0) << BIT_MAX;
    val &= read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_DE_DLY);
    printk("Bits 11:8 of the DE_DLY value (see register 0x72:0x32). value = %d", val);

    
    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_SYS_DE_TOP);
    printk("Defines the height of the area above the active display. The unit of measure is lines (HSYNC pulses). This register should be set to the sum of (VSYNC width) + (vertical back porch) + (vertical top border). \n The valid range is 1-127. 0 is invalid.\n");
    
    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_SYS_DE_CNTL);
    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_SYS_DE_CNTH);
    printk("Bits 11:0 Defines the width of the active display. The unit of measure is pixels. This register should be set to the desired horizontal resolution.\n The valid range is 1-4095. 0 is invalid.\n");
    val = FLD_GET(read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_DE_CNTH), 3, 0) << BIT_MAX;
    val &= read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_DE_CNTL);
    printk("Bits 11:0 val=%d\n", val);
    
    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_SYS_DE_LINL);
    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_SYS_DE_LINH_1);
    printk("Bits 10:0 Defines the height of the active display. The unit of measure is lines (HSYNC pulses). Set this register to the desired vertical resolution. For interlaced modes, set this register to the number of lines per field, which is half the overall vertical resolution. \n The valid range is 1-2047. 0 is invalid.\n");
    val = FLD_GET(read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_DE_LINH_1), 2, 0) << BIT_MAX;
    val &= read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_DE_LINL);
    printk("Bits 10:0 val=%d\n", val);

    val = read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_INT_CNTRL);
    DUMPREGVAL(val, HDMI_CORE_SYS_INT_CNTRL);
    DUMPONEBIT(BIT_1, val, "0 = Assert HIGH", "1 = Assert LOW", "Set software interrupt");
    DUMPONEBIT(BIT_2, val, "0 = Push/Pull", "1 = Open Drain pin", "INT pin output type");
    DUMPONEBIT(BIT_3, val, "0 = Clear interrupt", "1 = Set interrupt", "INT pin assertion level");
    
    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_SYS_VID_BLANK1);

    val = read_reg(HDMI_CORE_SYS, HDMI_CORE_DDC_CMD);
    DUMPREGVAL(val, HDMI_CORE_DDC_CMD);
    DUMPBITLOG(BIT_3, val, "DDC command: \n 0b1111 = Abort Transaction \n 0b1001 = Clear FIFO \n 0b1010 = Clock SCL \n 0b0000 = Current Address Read with no ACK on last byte \n 0b0010 = Sequential Read with no ACK on last byte \n 0b0100 = Enhanced DDC Read with no ACK on last byte \n 0b0110 = Sequential Write ignoring ACK on last byte \n 0b0111 = Sequential Write requiring ACK on last byte \n Writing to this register immediately initiates the I2C transaction on the DDC bus.");
    DUMPONEBIT(BIT_4, val, "0 = Enabled", "1 = Disabled", "Enable 3ns glitch filtering on the DDC clock and data line");
    DUMPONEBIT(BIT_5, val, "0 = Enabled", "1 = Disabled", "Enable the DDC delay");

    val = read_reg(HDMI_CORE_SYS, HDMI_CORE_DDC_STATUS);
    DUMPREGVAL(val, HDMI_CORE_DDC_STATUS);
    DUMPBITLOG(BIT_0, val, "1 = DDC FIFO Write In Use");
    DUMPBITLOG(BIT_1, val, "1 = DDC FIFO Read In Use");
    DUMPBITLOG(BIT_2, val, "1 = DDC FIFO Empty");
    DUMPBITLOG(BIT_3, val, "1 = DDC FIFO Full");
    DUMPBITLOG(BIT_4, val, "1 = DDC operation in progress");
    DUMPBITLOG(BIT_5, val, "1 = HDMI Transmitter did not receive an ACK from slave device during address or data write");
    DUMPBITLOG(BIT_6, val, "1 = I2C transaction did not start because I2C bus is pulled LOW by an external device");
    
    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_DDC_ADDR);
    printk("Bits 7:1 DDC device address\n");

    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_DDC_SEGM);
    printk("Bits 7:0 DDC segment address\n");

    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_DDC_OFFSET);
    printk("Bits 7:0 DDC offset address\n");
    
    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_DDC_COUNT1);
    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_DDC_COUNT2);
    printk("Bits 9:0 The total number of bytes to be read from the slave or written to the slave before a Stop bit is sent on the DDC bus. For example, if the HDCP KSV FIFO length is 635 bytes (127 devices x 5 bytes/KSV), the DDC_COUNT must be 0x27B\n");
    val = FLD_GET(read_reg(HDMI_CORE_SYS, HDMI_CORE_DDC_COUNT2), 1, 0) << BIT_MAX;
    val &= read_reg(HDMI_CORE_SYS, HDMI_CORE_DDC_COUNT1);
    printk("Bits 9:0 val=%d\n", val);
    
    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_DDC_DATA);
    printk("Bits 7:0 DDC data input\n");
    
    DUMPREG(HDMI_CORE_SYS, HDMI_CORE_DDC_FIFOCNT);
    printk("Bits 7:0 FIFO data byte count (the number of bytes in the FIFO). \n The DDC FIFO size is 16. The maximum value for DDC_FIFOCNT is 0x10\n");

    val = read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_SHA_CONTROL);
    DUMPREGVAL(val, HDMI_CORE_SYS_SHA_CONTROL);
    DUMPBITLOG(BIT_0, val, "Firmware starts the SHA generation by writing ※1§; which generates 1 clock strobe. If ＆1 is written, the state is inverted.");
    DUMPBITLOG(BIT_1, val, "If 1, means that SHA picked up the \"SHA go stat\" command.");
    DUMPONEBIT(BIT_2, val, "0 - regular mode", "1 - select SHA debug mode when firmware can overwrite the M0.", "Selects SHA mode");
    DUMPONEBIT(BIT_3, val, "0 = M0 internal mode (M0 non-reable over I2C)", "1 = M0 external mode (M0 readable over I2C)", "M0 mode");

    val = read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_RI_CMD);
    DUMPREGVAL(val, HDMI_CORE_SYS_RI_CMD);
    DUMPONEBIT(BIT_0, val, "0 = Disabled", "1 = Enabled", "Enable Ri check. Check bit 0 of the Ri_STAT register (0x72:0x26) for firmware and hardware DDC control handshaking");
    DUMPONEBIT(BIT_1, val, "0 = Disable", "1 = Enable", "Enable polling of the BCAP_DONE bit (0x72:0x72[7]) \n Note: To poll the BCAP_DONE bit, the ENC_EN (0x72:0x0F[0]) bit and the Ri_EN bit must be enabled on the HDMI Transmitter");

    val = read_reg(HDMI_CORE_SYS, HDMI_CORE_SYS_EPCM);
    DUMPREGVAL(val, HDMI_CORE_SYS_EPCM);
    DUMPBITLOG(BIT_4, val, "Command: \n 0b00011 = Run all BIST tests \n 0b00100 = Run only CRC test \n 0b01000 = Run only BIST self authentication test 1 \n 0b10000 = Run only BIST self authentication test 2 \n All other values are reserved. \n Before writing a new value into this register, verify that the previous command is complete by checking the 0x72.0xF9[0] register.");
    DUMPBITLOG(BIT_5, val, "1 = Enable loading of KSV from OTP \n Write 0 before enabling again.");

    val = read_reg(HDMI_CORE_AV, HDMI_CORE_AV_ACR_CTRL);
    DUMPREGVAL(val, HDMI_CORE_AV_ACR_CTRL);
    DUMPONEBIT(BIT_0, val, "0 = Send HW-updated CTS value in N/CTS packet (recommended)", "1 = Send SW-updated CTS value in N/CTS packet (for diagnostic use)", "CTS Source Select:");
    DUMPONEBIT(BIT_1, val, "0 = N/CTS packet disabled", "1 = N/CTS packet enabled", "CTS Request Enable");
    DUMPONEBIT(BIT_2, val, "0: Disable the MCLK input", "1: Enable the MCLK input", "In case of parallel audio I/F, the audio path can be operated without MCLK. Data rate adjustment is then done via TCLK and ACR at the Rx via programmed CTS values. TCLK must be stable enough to ensure audio output quality according to HDMI 1.3 Specification. \n SIMG disclaims any responsibility on audio output quality at the Rx if MCLK is disabled.");

    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_FREQ_SVAL);
    printk("MCLK input mode: \n 0b000 = MCLK is 128*Fs \n 0b001 = MCLK is 256*Fs \n 0b010 = MCLK is 384*Fs \n 0b011 = MCLK is 512*Fs\n 0b100 = MCLK is 768*Fs\n 0b101 = MCLK is 1024*Fs \n 0b110 = MCLK is 1152*Fs \n 0b111 = MCLK is 192*Fs \n The HDMI Transmitter uses these bits to divide the MCLK input to produce CTS values according to the 128*Fs formula. The MCLK to Fs ratio is for the input Fs, not the down-sampled output Fs (see the ASRC register (0x7A:0x23), on page 64).\n");

    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_N_SVAL1);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_N_SVAL2);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_N_SVAL3);
    printk("Bits 19:0 N Value for audio clock regeneration method; a 20-bit value. This must be written to the registers to create the correct divisor for audio clock regeneration. Only values greater than 0 are valid. This register must be written after a hardware reset.\n");
    val = FLD_GET(read_reg(HDMI_CORE_AV, HDMI_CORE_AV_N_SVAL3), 3, 0) << BIT_MAX;
    val = (val & read_reg(HDMI_CORE_AV, HDMI_CORE_AV_N_SVAL2)) << BIT_MAX;
    val = val & read_reg(HDMI_CORE_AV, HDMI_CORE_AV_N_SVAL1);
    printk("Bits 19:0 val=%d\n", val);

    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_CTS_SVAL1);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_CTS_SVAL2);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_CTS_SVAL3);
    printk("Bits 19:0 CTS Value for audio clock regeneration method; a 20-bit value. For diagnostic use and applied only when the CTS_SEL bit (0x7A:0x01[0]) is set to 1. \n In case of parallel audio I/F, the audio path can be operated without MCLK. Data rate adjustment is then done via TCLK and ACR at the Rx via programmed CTS values. TCLK must be stable enough to ensure audio output quality according to HDMI 1.3 Specification. \n SIMG disclaims any responsibility on audio output quality at the Rx if MCLK is disabled.\n");
    val = FLD_GET(read_reg(HDMI_CORE_AV, HDMI_CORE_AV_CTS_SVAL3), 3, 0) << BIT_MAX;
    val = (val & read_reg(HDMI_CORE_AV, HDMI_CORE_AV_CTS_SVAL2)) << BIT_MAX;
    val = val & read_reg(HDMI_CORE_AV, HDMI_CORE_AV_CTS_SVAL1);
    printk("Bits 19:0 val=%d\n", val);

    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_CTS_HVAL1);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_CTS_HVAL2);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_CTS_HVAL3);
    printk("Bits 19:0 CTS Value for audio clock regeneration method; a 20-bit value. This value is measured and stored here by the hardware when MCLK is active and N is valid, after 128Fs/N cycles of MCLK.\n");
    val = FLD_GET(read_reg(HDMI_CORE_AV, HDMI_CORE_AV_CTS_HVAL3), 3, 0) << BIT_MAX;
    val = (val & read_reg(HDMI_CORE_AV, HDMI_CORE_AV_CTS_HVAL2)) << BIT_MAX;
    val = val & read_reg(HDMI_CORE_AV, HDMI_CORE_AV_CTS_HVAL1);
    printk("Bits 19:0 val=%d\n", val);

    val = read_reg(HDMI_CORE_AV, HDMI_CORE_AV_AUD_MODE);
    DUMPREGVAL(val, HDMI_CORE_AV_AUD_MODE);
    DUMPONEBIT(BIT_0, val, "0 = Disable", "1 = Enable", "Audio input stream enable");
    DUMPONEBIT(BIT_1, val, "0 = Disable", "1 = Enable", "S/PDIF input stream enable");
    DUMPONEBIT(BIT_2, val, "0: Parallel audio input disabled", "1: Parallel audio input enabled", "AUD_PAR_EN");
    DUMPONEBIT(BIT_3, val, "0 = Disable", "1 = Enable", "Direct Stream Digital Audio enable");
    DUMPONEBIT(BIT_4, val, "0 = Disable", "1 = Enable", "I2S input channel #0");
    DUMPONEBIT(BIT_5, val, "0 = Disable", "1 = Enable", "I2S input channel #1");
    DUMPONEBIT(BIT_6, val, "0 = Disable", "1 = Enable", "I2S input channel #2");
    DUMPONEBIT(BIT_7, val, "0 = Disable", "1 = Enable", "I2S input channel #3");

    val = read_reg(HDMI_CORE_AV, HDMI_CORE_AV_SPDIF_CTRL);
    DUMPREGVAL(val, HDMI_CORE_AV_SPDIF_CTRL);
    DUMPONEBIT(BIT_1, val, "0 = Use input S/PDIF streams detected FS", "1 = Use software FS in I2S_CHST4 register (0x7A:0x21)", "S/PDIF input stream override");
    DUMPONEBIT(BIT_3, val, "0 = Detected change on the S/PDIF input", "1 = No change detected on the S/PDIF input", "No S/PDIF audio");

    val = read_reg(HDMI_CORE_AV, HDMI_CORE_AV_HW_SPDIF_FS);
    DUMPREGVAL(val, HDMI_CORE_AV_HW_SPDIF_FS);
    DUMPBITLOG(BIT_3, val, "Bits 3:0 Set to the FS extracted from the S/PDIF input channel status bits 24-27");
    DUMPONEBIT(BIT_4, val, "0 = Maximum sample length is 20 bits", "1 = Maximum sample length is 24 bits", "Maximum sample length (channel status bit 32)");
    DUMPBITLOG(BIT_7, val, "Bits 7:5 Channel status bits 33 to 35 (bit 33=LSB, bit 35=MSB) \n Combines with HW_MAXLEN to indicate sample size: \n Bits  Max 24  Max 20 \n 001  20 bits 16 bits \n 010  22 bits 18 bits \n 100  23 bits 19 bits \n 101  24 bits 20 bits \n 110  21 bits 17 bits");

    val = read_reg(HDMI_CORE_AV, HDMI_CORE_AV_SWAP_I2S);
    DUMPREGVAL(val, HDMI_CORE_AV_SWAP_I2S);
    DUMPBITLOG(BIT_3, val, "Bits 3:0 Reserved 每 do not modify. Default 0x9");
    DUMPONEBIT(BIT_4, val, "0 = Do not swap left and right", "1 = Swap left and right", "Swap left-right channels for I2S Channel 0.");
    DUMPONEBIT(BIT_5, val, "0 = Do not swap left and right", "1 = Swap left and right", "Swap left-right channels for I2S Channel 1.");
    DUMPONEBIT(BIT_6, val, "0 = Do not swap left and right", "1 = Swap left and right", "Swap left-right channels for I2S Channel 2.");
    DUMPONEBIT(BIT_7, val, "0 = Do not swap left and right", "1 = Swap left and right", "Swap left-right channels for I2S Channel 3.");

    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_SPDIF_ERTH);
    printk("Specifies the error threshold level. The frame is marked as invalid if the number of bi-phase mark encoding errors in the audio stream exceeds this threshold level during frame decoding.\n");

    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_I2S_IN_MAP);
    printk("Bits 7:6 Channel map to FIFO #3 (for HDMI Layout 1), Bits 5:4 to FIFO #2, Bits 3:2 to FIFO #1, Bits 1:0 Channel map to FIFO #0 (for HDMI Layout 0 or 1) \n 0b00 = Map SD0 to FIFO #0 \n 0b01 = Map SD1 to FIFO #0 \n 0b10 = Map SD2 to FIFO #0 \n 0b11 = Map SD3 to FIFO #0 \n");

    val = read_reg(HDMI_CORE_AV, HDMI_CORE_AV_I2S_IN_CTRL);
    DUMPREGVAL(val, HDMI_CORE_AV_I2S_IN_CTRL);
    DUMPONEBIT(BIT_0, val, "0 = First bit shift (refer to the Philips Specification)", "1 = No shift", "WS to SD first bit shift");
    DUMPONEBIT(BIT_1, val, "0 = MSB shifted first", "1 = LSB shifted first", "SD direction");
    DUMPONEBIT(BIT_2, val, "0 = Data is left-justified", "1 = Data is right-justified", "SD justify");
    DUMPONEBIT(BIT_3, val, "0 = Left polarity when WS is LOW", "1 = Left polarity when WS is HIGH", "WS polarity");
    DUMPONEBIT(BIT_4, val, "0 = PCM", "1 = Compressed", "V bit value");
    DUMPBITLOG(BIT_5, val, "This bit should be set to 1 for High Bit Rate Audio");
    DUMPONEBIT(BIT_6, val, "0 = Sample edge is falling; SD3-SD0 and WS source should change state on the rising edge of SCK", "1 = Sample clock is rising; SD3-SD0 and WS source should change state on the falling edge of SCK", "SCK sample edge");
    DUMPONEBIT(BIT_7, val, "0 = Input stream is not high bit rate", "1 = Input stream is high bit rate. All of the I2S control bits will apply to the control of the High Bit Rate Audio.", "High Bit Rate Audio On");

    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_I2S_CHST0);
    printk("Channel Status Byte #0\n");

    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_I2S_CHST1);
    printk("Channel Status Byte #1: Category Code\n");

    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_I2S_CHST2);
    printk("Bits 7:4 Channel Status Byte #2: Source Number \n Bits 3:0 Channel Status Byte #2: Source Number\n");

    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_I2S_CHST4);
    printk("Bits 7:4 Clock Accuracy. \n Bits 3:0 Sampling frequency as set by software, which is inserted into the HDMI audio stream if FS_OVERRIDE (0x7A:0x15[1]) is enabled\n");

    val = read_reg(HDMI_CORE_AV, HDMI_CORE_AV_I2S_CHST5);
    DUMPREGVAL(val, HDMI_CORE_AV_I2S_CHST5);
    DUMPONEBIT(BIT_0, val, "0 = 20 bits","1 = 24 bits", "Maximum audio sample word length");
    printk("Bits 7:4 Original Fs. \n Bits 3:0 Audio sample word length: Defined in bits with I2S_MAXLEN: \nI2S_LEN  I2S_MAXLEN=0  I2S_MAXLEN=1 \n 0b001       16            20\n 0b010       18            22\n 0b100       19            23\n 0b101       20            24\n 0b110       17            21\n");

    val = read_reg(HDMI_CORE_AV, HDMI_CORE_AV_ASRC);
    DUMPREGVAL(val, HDMI_CORE_AV_ASRC);
    DUMPONEBIT(BIT_0, val, "0 = Disabled", "1 = Enabled", "Audio sample rate conversion");
    DUMPONEBIT(BIT_1, val, "0 = Down-sample 2-to-1 when SRC_EN is set to 1", "1 = Down-sample 4-to-1 when SRC_EN is set to 1", "Sample rate down-conversion ratio");
    printk("Bits 7:4 Mask for the sample present and flat bit of the High Bit Rate Audio header. Each bit masks out one of the subpacket sample present bits.\n 0 = Mask out. 1 = Unmask \n Bits 7:4 must be programmed to 0b0000 when HBRA mode is selected\n");

    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_I2S_IN_LEN);
    printk("Bits 7:4 The ID of the High Bit Rate Audio packet header.\n");
    printk("Bits 3:0 Number of valid bits in the input I2S stream. Used for the extraction of the I2S data from the input stream. \n 0b1111 每 0b1110 = N/A\n 0b1101 = 21 bit \n 0b1100 = 17 bit \n 0b1011 = 24 bit \n 0b1010 = 20 bit \n 0b1001 = 23 bit \n 0b1000 = 19 bit \n 0b0111 每 0b0110 = N/A \n 0b0101 = 22 bit \n 0b0100 = 18 bit \n 0b0011 = N/A \n 0b0010 = 16 bit \n 0b0001 每 0b0000 = N/A\n");
    
    val = read_reg(HDMI_CORE_AV, HDMI_CORE_AV_AUDO_TXSTAT);
    DUMPREGVAL(val, HDMI_CORE_AV_AUDO_TXSTAT);
    DUMPBITLOG(BIT_0, val, "Enables null packet flooding all the time");
    DUMPBITLOG(BIT_1, val, "Enables null packet flooding only when VSync is high.");
    DUMPONEBIT(BIT_2, val, "0 = No packet with SETAVM=1 has been sent", "1 = A packet with SETAVM=1 has been sent", "General Control Packet mute status");
    
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_AUD_PAR_BUSCLK_1);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_AUD_PAR_BUSCLK_2);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_AUD_PAR_BUSCLK_3);
    
    val = read_reg(HDMI_CORE_AV, HDMI_CORE_AV_TEST_TXCTRL);
    DUMPREGVAL(val, HDMI_CORE_AV_TEST_TXCTRL);
    DUMPONEBIT(BIT_2, val, "0 - normal operation (default)", "1 - Input pins muxed to TMDS Tx core; to emulate discrete Tx.", "TMDS Core Isolation Enable");
    DUMPONEBIT(BIT_3, val, "0 - normal operation (default).", "1 - bypass DVI encoder logic.", "DVI encoder bypass");

    val = read_reg(HDMI_CORE_AV, HDMI_CORE_AV_HDMI_CTRL);
    DUMPREGVAL(val, HDMI_CORE_AV_HDMI_CTRL);
    DUMPONEBIT(BIT_0, val, "0 = Disabled", "1 = Enabled", "HDMI mode");
    DUMPONEBIT(BIT_1, val, "0b00 = Layout 0 (2-channel)", "0b01 = Layout 1 (up to 8 channels)", "Audio packet header layout indicator");
    DUMPBITLOG(BIT_5, val, "Specifies the number of bits per pixel sent to the paketizer: \n 0b100 = 24 bits per pixel (8 bits per pixel; no packing) \n 0b101 = 30 bits per pixel (10 bits per pixel pack to 8 bits) \n 0b110 = 36 bits per pixel (12 bits per pixel pack to 8 bits) \n 0b111 = 48 bits per pixel (16 bits per pixel; no packing) \n Note: The firmware must program 24 bits per pixel (8 bits per pixel; no packing) for initialization.");
    DUMPONEBIT(BIT_6, val, "0 = Do not send deep-color related information in the packet to the HDMI Receiver", "1 = Send deep-color related information in the packet to the HDMI Receiver", "Deep-color packet enable: \n The following data is sent in data byte #1 of the packet: \n 7 = Reserved \n 6 = PP2 \n 5 = PP1 \n 4 = PP0 \n 3 = PP_valid \n 2 = CD2 \n 1 = CD1 \n 0 = CD0 \n The CD bits indicate deep-color mode (defined in the same register bits 5:3). The PP bits indicate the fragment∩s phase related information that comes from the hardware state machine");

    val = read_reg(HDMI_CORE_AV, HDMI_CORE_AV_DPD);
    DUMPREGVAL(val, HDMI_CORE_AV_DPD);
    DUMPBITLOG(BIT_0, val, "Power down total: \n a = Power down everything; INT source is RSEN. \n b = Normal operation \n Active high or low depends on used Phy");
    DUMPBITLOG(BIT_1, val, "Power down Internal Oscillator. This disables the I2C port to the internal ROM (disabling loading), halts all interrupt updates, and disables the Master DDC block. \n a = Power down \n b = Normal operation \n Active high or low depends on used Phy");
    DUMPONEBIT(BIT_2, val, "0 = Power down; gate off IDCK signal to disable all IDCK-based logic", "1 = Normal operation", "Power down IDCK input");
    DUMPBITLOG(BIT_3, val, "Selects the TCLK phase: \n a = Default phase; the same as TMDS core \n b = Invert TCLK; change the phase 180 degrees \n Active high or low depends on used Phy");
    DUMPONEBIT(BIT_7, val, "0: Disable", "1: Enable", "Enable bypath of the video path. \n The core_iso_en bit in (reg. 0x13C[1]) must be set and the HDCP cypher must be disabled (reg. 0x72:=x0F[0] == 0)");
    
    val = read_reg(HDMI_CORE_AV, HDMI_CORE_AV_PB_CTRL1);
    DUMPREGVAL(val, HDMI_CORE_AV_PB_CTRL1);
    DUMPONEBIT(BIT_0, val, "0 = Disabled (send once after enable bit is set)", "1 = Enabled (send in every VBLANK period)", "Repeat AVI InfoFrame transmission");
    DUMPONEBIT(BIT_1, val, "0 = Disabled", "1 = Enabled", "Enable AVI InfoFrame transmission:");
    DUMPONEBIT(BIT_2, val, "0 = Disabled (send once after enable bit is set)", "1 = Enabled (send in every VBLANK period)", "Repeat SPD InfoFrame transmission");
    DUMPONEBIT(BIT_3, val, "0 = Disabled", "1 = Enabled", "Enable SPD InfoFrame transmission");
    DUMPONEBIT(BIT_4, val, "0 = Disabled (send once after enable bit is set)", "1 = Enabled (send in every VBLANK period)", "Repeat Audio InfoFrame transmission");
    DUMPONEBIT(BIT_5, val, "0 = Disabled", "1 = Enabled", "Enable Audio InfoFrame transmission");
    DUMPONEBIT(BIT_6, val, "0 = Disabled (send once after enable bit is set)", "1 = Enabled (send in every VBLANK period)", "Repeat MPEG InfoFrame transmission");
    DUMPONEBIT(BIT_7, val, "0 = Disabled", "1 = Enabled", "Enable MPEG InfoFrame transmission");

    val = read_reg(HDMI_CORE_AV, HDMI_CORE_AV_PB_CTRL2);
    DUMPREGVAL(val, HDMI_CORE_AV_PB_CTRL2);
    DUMPONEBIT(BIT_0, val, "0 = Disable (send once after enable bit is set)", "1 = Enable (send in every VBLANK period)", "Repeat Generic Packet transmission");
    DUMPONEBIT(BIT_1, val, "0 = Disabled", "1 = Enabled", "Enable Generic Packet transmission");
    DUMPONEBIT(BIT_2, val, "0 = Disabled (send once after enable bit is set)", "1 = Enabled (send in every VBLANK period)", "Repeat General Control Packet transmission");
    DUMPONEBIT(BIT_3, val, "0 = Disabled", "1 = Enabled", "Enable General Control Packet transmission");
    DUMPONEBIT(BIT_4, val, "0 = Disabled (send once after enable bit is set)", "1 = Enabled (send in every VBLANK period)", "Repeat Generic #2 Packet transmission");
    DUMPONEBIT(BIT_5, val, "0 = Disabled", "1 = Enabled", "Enable Generic #2 Packet transmission");
    DUMPONEBIT(BIT_6, val, "0 = Disabled", "1 = Enabled", "Repeat Gamut Metadata InfoFrame Packet data each frame");
    DUMPONEBIT(BIT_7, val, "0 = Disabled", "1 = Enabled", "Enable Gamut Metadata InfoFrame transmission on HDMI");

    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_AVI_TYPE);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_AVI_VERS);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_AVI_LEN);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_AVI_CHSUM);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_AVI_DBYTE);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_SPD_TYPE);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_SPD_VERS);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_SPD_LEN);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_SPD_CHSUM);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_SPD_DBYTE);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_AUDIO_TYPE);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_AUDIO_VERS);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_AUDIO_LEN);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_AUDIO_CHSUM);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_AUDIO_DBYTE);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_MPEG_TYPE);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_MPEG_VERS);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_MPEG_LEN);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_MPEG_CHSUM);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_MPEG_DBYTE);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_GEN_DBYTE);
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_GEN2_DBYTE);
    
    val = read_reg(HDMI_CORE_AV, HDMI_CORE_AV_CP_BYTE1);
    DUMPREGVAL(val, HDMI_CORE_AV_CP_BYTE1);
    DUMPBITLOG(BIT_0, val, "Set AV Mute flag. \n When the AVMUTE flag is set, the HDMI Transmitter sends a General Control Packet on the TMDS link to inform the Sink that the data may be incorrect. The HDMI Transmitter sends blank-level data for all video packets and 0x00 for all audio packet data. \n When the AVMUTE flag is set, the Sink assumes that no valid data is being received. Optionally, the Sink can apply a mute function to the audio data and/or a blank function to the video data.");
    DUMPBITLOG(BIT_4, val, "Clear AV Mute flag.");
    
    DUMPREG(HDMI_CORE_AV, HDMI_CORE_AV_CEC_ADDR_ID);
    printk("CEC I2C slave address ID\n");
}

