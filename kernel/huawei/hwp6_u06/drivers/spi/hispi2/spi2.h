/*******************************************************************************
 * Copyright:   Copyright (c) 2007. Hisilicon Technologies, CO., LTD. 
 * Version:     V300R001B03
 * Filename:    spi2.h
 * Description: spi2总线驱动头文件(for android)
 * History:
                1.Created by wubin on 2007/06/07
                2.Modified by c00168914 on 2010/07/30
*******************************************************************************/

#include <mach/hisi_spi2.h>

#ifndef __SPI2_H__
#define __SPI2_H__

#ifdef __cplusplus
extern "C" {
#endif


#define HISPI2_TRACE_LEVEL 4

#define HISPI2_TRACE_FMT KERN_INFO

#define hispi2_trace(level, msg...) do { \
	if((level) >= spi2_trace_level) { \
		printk(HISPI2_TRACE_FMT "hispi2:%s:%d:", __FUNCTION__, __LINE__); \
		printk(msg); \
	} \
}while(0)


//------------------------------------------------------------------------------
// Defines

#define CSPI_WAIT_TIMEOUT       99999

#define SPI_MAX_PACKET_LEN (2048+1024)

//------------------------------------------------------------------------------
// REGISTER LAYOUT
//------------------------------------------------------------------------------
typedef struct
{
    unsigned int SSPCR0;
    unsigned int SSPCR1;
    unsigned int SSPDR;
    unsigned int SSPSR;
    unsigned int SSPCPSR;
    unsigned int SSPIMSC;
    unsigned int SSPRIS;
    unsigned int SSPMIS;
    unsigned int SSPICR;
    unsigned int SSPDMACR;
} CSP_CSPI_REG, *PCSP_CSPI_REG;


//------------------------------------------------------------------------------
// REGISTER BIT FIELD POSITIONS (LEFT SHIFT)
//------------------------------------------------------------------------------
#define CSPI_SSPCR0_DSS_LSH         0
#define CSPI_SSPCR0_FRF_LSH         4
#define CSPI_SSPCR0_SPO_LSH         6
#define CSPI_SSPCR0_SPH_LSH         7
#define CSPI_SSPCR0_SCR_LSH         8

#define CSPI_SSPCR1_LBM_LSH         0
#define CSPI_SSPCR1_SSE_LSH         1
#define CSPI_SSPCR1_MS_LSH          2
#define CSPI_SSPCR1_SOD_LSH         3

#define CSPI_SSPDR_DATA_LSH         0

#define CSPI_SSPSR_TFE_LSH          0
#define CSPI_SSPSR_TNF_LSH          1
#define CSPI_SSPSR_RNE_LSH          2
#define CSPI_SSPSR_RFF_LSH          3
#define CSPI_SSPSR_BSY_LSH          4

#define CSPI_SSPCPSR_CPSDVSR_LSH    0

#define CSPI_SSPIMSC_RORIM_LSH      0
#define CSPI_SSPIMSC_RTIM_LSH       1    
#define CSPI_SSPIMSC_RXIM_LSH       2
#define CSPI_SSPIMSC_TXIM_LSH       3

#define CSPI_SSPRIS_RORRIS_LSH      0
#define CSPI_SSPRIS_RTRIS_LSH       1    
#define CSPI_SSPRIS_RXRIS_LSH       2
#define CSPI_SSPRIS_TXRIS_LSH       3

#define CSPI_SSPMIS_RORMIS_LSH      0
#define CSPI_SSPMIS_RTMIS_LSH       1
#define CSPI_SSPMIS_RXMIS_LSH       2
#define CSPI_SSPMIS_TXMIS_LSH       3

#define CSPI_SSPICR_RORIC_LSH       0
#define CSPI_SSPICR_RTIC_LSH        1

#define CSPI_SSPDMACR_RXDMAE_LSH    0
#define CSPI_SSPDMACR_TXDMAE_LSH    1

//------------------------------------------------------------------------------
// REGISTER BIT FIELD WIDTHS
//------------------------------------------------------------------------------
#define CSPI_SSPCR0_DSS_WID         4
#define CSPI_SSPCR0_FRF_WID         2
#define CSPI_SSPCR0_SPO_WID         1
#define CSPI_SSPCR0_SPH_WID         1
#define CSPI_SSPCR0_SCR_WID         8

#define CSPI_SSPCR1_LBM_WID         1
#define CSPI_SSPCR1_SSE_WID         1
#define CSPI_SSPCR1_MS_WID          1
#define CSPI_SSPCR1_SOD_WID         1

#define CSPI_SSPDR_DATA_WID         16

#define CSPI_SSPSR_TFE_WID          1
#define CSPI_SSPSR_TNF_WID          1
#define CSPI_SSPSR_RNE_WID          1
#define CSPI_SSPSR_RFF_WID          1
#define CSPI_SSPSR_BSY_WID          1

#define CSPI_SSPCPSR_CPSDVSR_WID    8

#define CSPI_SSPIMSC_RORIM_WID      1
#define CSPI_SSPIMSC_RTIM_WID       1    
#define CSPI_SSPIMSC_RXIM_WID       1
#define CSPI_SSPIMSC_TXIM_WID       1

#define CSPI_SSPRIS_RORRIS_WID      1
#define CSPI_SSPRIS_RTRIS_WID       1    
#define CSPI_SSPRIS_RXRIS_WID       1
#define CSPI_SSPRIS_TXRIS_WID       1

#define CSPI_SSPMIS_RORMIS_WID      1
#define CSPI_SSPMIS_RTMIS_WID       1
#define CSPI_SSPMIS_RXMIS_WID       1
#define CSPI_SSPMIS_TXMIS_WID       1

#define CSPI_SSPICR_RORIC_WID       1
#define CSPI_SSPICR_RTIC_WID        1

#define CSPI_SSPDMACR_RXDMAE_WID    1
#define CSPI_SSPDMACR_TXDMAE_WID    1




//------------------------------------------------------------------------------
// REGISTER BIT WRITE VALUES
//------------------------------------------------------------------------------
//SSPCR0
#define CSPI_SSPCR0_DSS_4BIT            0x03    // 4-bit transfer
#define CSPI_SSPCR0_DSS_5BIT            0x04    // 5-bit transfer
#define CSPI_SSPCR0_DSS_6BIT            0x05    // 6-bit transfer
#define CSPI_SSPCR0_DSS_7BIT            0x06    // 7-bit transfer
#define CSPI_SSPCR0_DSS_8BIT            0x07    // 8-bit transfer
#define CSPI_SSPCR0_DSS_9BIT            0x08    // 9-bit transfer
#define CSPI_SSPCR0_DSS_10BIT           0x09    // 10-bit transfer
#define CSPI_SSPCR0_DSS_11BIT           0x0A    // 11-bit transfer
#define CSPI_SSPCR0_DSS_12BIT           0x0B    // 12-bit transfer
#define CSPI_SSPCR0_DSS_13BIT           0x0C    // 13-bit transfer
#define CSPI_SSPCR0_DSS_14BIT           0x0D    // 14-bit transfer
#define CSPI_SSPCR0_DSS_15BIT           0x0E    // 15-bit transfer
#define CSPI_SSPCR0_DSS_16BIT           0x0F    // 16-bit transfer

#define CSPI_SSPCR0_FRF_SPI             0x00    // Motorola SPI frame format
#define CSPI_SSPCR0_FRF_TI              0x01    // TI synchronous serial frame format
#define CSPI_SSPCR0_FRF_NM              0x02    // National Microwire frame format
#define CSPI_SSPCR0_FRF_RESERVED        0x03    // Reserved

#define CSPI_SSPCR0_SPO_ACTIVE_LOW      0x0     // Active low polarity
#define CSPI_SSPCR0_SPO_ACTIVE_HIGH     0x1     // Active high polarity

#define CSPI_SSPCR0_SPH_TRAILING        0x0     // Phase 0 operation
#define CSPI_SSPCR0_SPH_LEADING         0x1     // Phase 1 operation

//SSPCR1
#define CSPI_SSPCR1_LBM_DISABLE         0x0     // Loop back mode disabled
#define CSPI_SSPCR1_LBM_ENABLE          0x1     // Loop back mode enabled

#define CSPI_SSPCR1_SSE_DISABLE         0x0     // SSP disabled
#define CSPI_SSPCR1_SSE_ENABLE          0x1     // SSP enabled

#define CSPI_SSPCR1_MS_MASTER           0x0     // Master mode
#define CSPI_SSPCR1_MS_SLAVE            0x1     // Slave mode

#define CSPI_SSPCR1_SOD_ENABLE          0x0     // Enable drive out in slave mode
#define CSPI_SSPCR1_SOD_DISABLE         0x1     // Disable drive out in slave mode

//SSPIMSC
#define CSPI_SSPIMSC_RORIM_DISABLE      0x0     // Disable Receive FIFO overrun interrupt
#define CSPI_SSPIMSC_RORIM_ENABLE       0x1     // Enable Receive FIFO overrun interrupt

#define CSPI_SSPIMSC_RTIM_DISABLE       0x0     // Disable receive overtime interrupt
#define CSPI_SSPIMSC_RTIM_ENABLE        0x1     // Enable receive overtime interrupt
                                        
#define CSPI_SSPIMSC_RXIM_DISABLE       0x0     // Disable RxFIFO half interrupt
#define CSPI_SSPIMSC_RXIM_ENABLE        0x1     // Enable RxFIFO half interrupt
                                        
#define CSPI_SSPIMSC_TXIM_DISABLE       0x0     // Disable TxFIFO half interrupt
#define CSPI_SSPIMSC_TXIM_ENABLE        0x1     // Enable TxFIFO half interrupt


//------------------------------------------------------------------------------
// System Control Registers
//------------------------------------------------------------------------------
#define SYSTEM_CONTROL_BASE             (0x20040000)
#define SYSTEM_IOMGCG_BASE              (0x20041000)
#define CSP_BASE_REG_PA_CSPI1           (0x30080000)
#ifdef CONFIG_TRUELY_WVGA_B020
#define CSP_BASE_REG_PA_CSPI2           (0xFCB06000)
#else
#define CSP_BASE_REG_PA_CSPI2           (0xFCB05000)
#endif
//#define CSP_BASE_REG_PA_CSPI3           (0x300c6000)

//#define IOMGCG_REGS_LENGTH              (0x1000)
//extern DWORD g_dwSPI2LogMask;
//#define SPI2MSG(cond,printf_exp) ((g_dwSPI2LogMask&(cond))?NKDbgPrintfW(TEXT("[SPI2|%s]"),TEXT(__FUNCTION__)),(NKDbgPrintfW printf_exp),1:0)
//#define SPI2MSG(cond,printf_exp)   printk("%s:%d\n", __FUNCTION__, __LINE__);

//#define SPI2LOG_ERR                   (0x0001)//ERROR logger information
//#define SPI2LOG_BASE                  (0x0002)//Basement logger information
//#define SPI2LOG_DETAIL                (0x0004)//Detail logger information
//#define SPI2_IOCTL_MAGIC_CMD       (0x3610)

/*
#define SPI2_CLK            GPIO_7_2
#define SPI2_DI             GPIO_7_3
#define SPI2_DO             GPIO_7_4
#define SPI2_CS0_N          GPIO_7_5
#define SPI2_CS1_N          GPIO_7_6
*/


//外设时钟使能寄存器。写1使能对应的时钟；写0不影响时钟使能的状态
//#define SYSCTRL_SCPEREN	                      (SYSTEM_CONTROL_BASE + 0x00000024)

//外设时钟禁止寄存器。写1禁止对应的时钟；写0不影响时钟使能的状态
//#define SYSCTRL_SCPERDIS	                  (SYSTEM_CONTROL_BASE + 0x00000028)

//外设时钟使能状态寄存器，用于回读各外设时钟使能控制信号的状态。值为1表明时钟被使能；值为0表明时钟被禁止
//#define SYSCTRL_SCPERCLKEN	                  (SYSTEM_CONTROL_BASE + 0x0000002C)

/* 系统控制器寄存器SCPERCTRL5[12:10]控制将片选信号送给SPI_CS0_N、SPI_CS1_N和SPI_CS2_N其中一个管脚*/
//#define SYSCTRL_SCPERCTRL5                    (SYSTEM_CONTROL_BASE + 0x00000058)

//外设时钟使能和禁止寄存器中，SPI对应的BIT位
//#define SCPER_SPI1                              (1 << 24)
#define SCPER_SPI2                              (1 << 25)
//#define SCPER_SPI3                              (1 << 26)

//------------------------------------------------------------------------------
// IO defination
//------------------------------------------------------------------------------

	
// Bitfield macros that use rely on bitfield width/shift information
// defined in CSP header files
#define CSP_BITFMASK(bit) (((1U << (bit ## _WID)) - 1) << (bit ## _LSH))
#define CSP_BITFVAL(bit, val) ((val) << (bit ## _LSH))

// Undefine previous implementations of peripheral access macros since
// we want to "own" the definitions and avoid redefinition warnings
// resulting from source code that includes oal_io.h
#undef INREG8
#undef OUTREG8
#undef SETREG8
#undef CLRREG8
#undef INREG16
#undef OUTREG16
#undef SETREG16
#undef CLRREG16
#undef INREG32
#undef OUTREG32
#undef SETREG32
#undef CLRREG32

#define READ_REGISTER_ULONG(addr)             (*(volatile unsigned long *)(addr))
#define WRITE_REGISTER_ULONG(addr, value)     ((*(volatile unsigned long *)(addr)) = (value))

// Macros for accessing peripheral registers using DDK macros/functions
#define INREG8(x)           READ_REGISTER_UCHAR((unsigned char*)(x))
#define OUTREG8(x, y)       WRITE_REGISTER_UCHAR((unsigned char*)(x), (unsigned char)(y))
#define SETREG8(x, y)       OUTREG8(x, INREG8(x)|(y))
#define CLRREG8(x, y)       OUTREG8(x, INREG8(x)&~(y))
#define INSREG8(addr, mask, val) OUTREG8(addr, ((INREG8(addr)&(~(mask))) | val))
#define EXTREG8(addr, mask, lsh) ((INREG8(addr) & mask) >> lsh)
#define EXTREG8BF(addr, bit) (EXTREG8(addr, CSP_BITFMASK(bit), (bit ## _LSH)))
#define INSREG8BF(addr, bit, val) (INSREG8(addr, CSP_BITFMASK(bit), CSP_BITFVAL(bit, val)))

#define INREG16(x)          READ_REGISTER_USHORT((unsigned short *)(x))
#define OUTREG16(x, y)      WRITE_REGISTER_USHORT((unsigned short *)(x),(unsigned short *)(y))
#define SETREG16(x, y)      OUTREG16(x, INREG16(x)|(y))
#define CLRREG16(x, y)      OUTREG16(x, INREG16(x)&~(y))
#define INSREG16(addr, mask, val) OUTREG16(addr, ((INREG16(addr)&(~(mask))) | val))
#define EXTREG16(addr, mask, lsh) ((INREG16(addr) & mask) >> lsh)
#define EXTREG16BF(addr, bit) (EXTREG16(addr, CSP_BITFMASK(bit), (bit ## _LSH)))
#define INSREG16BF(addr, bit, val) (INSREG16(addr, CSP_BITFMASK(bit), CSP_BITFVAL(bit, val)))

#define INREG32(x)          READ_REGISTER_ULONG((unsigned long*)(x))
#define OUTREG32(x, y)      WRITE_REGISTER_ULONG((unsigned long *)(x), (unsigned long)(y))
#define SETREG32(x, y)      OUTREG32(x, INREG32(x)|(y))
#define CLRREG32(x, y)      OUTREG32(x, INREG32(x)&~(y))
#define INSREG32(addr, mask, val) OUTREG32(addr, ((INREG32(addr)&(~(mask))) | val))
#define EXTREG32(addr, mask, lsh) ((INREG32(addr) & mask) >> lsh)
#define EXTREG32BF(addr, bit) (EXTREG32(addr, CSP_BITFMASK(bit), (bit ## _LSH)))
#define INSREG32BF(addr, bit, val) (INSREG32(addr, CSP_BITFMASK(bit), CSP_BITFVAL(bit, val)))


#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE  1
#endif

#ifndef BOOL
#define BOOL  unsigned int
#endif


#ifdef __cplusplus
}
#endif


#endif // __SPI2_H__

