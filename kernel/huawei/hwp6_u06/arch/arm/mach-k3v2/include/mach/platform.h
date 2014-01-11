#ifndef __MACH_K3V2_PLATFORM_H
#define __MACH_K3V2_PLATFORM_H

#ifndef __ASSEMBLY__
#include <asm/sizes.h>
#include <linux/mm.h>
#endif

#if defined(CONFIG_MACH_K3V2OEM1)
/* T2 CS chip */
#define REG_BASE_I2C3                      (0xFCB0D000)
#define REG_BASE_I2C3_VIRT                 (0xFE7B8000)
#define REG_I2C3_IOSIZE                     PAGE_ALIGN(SZ_4K)

#define REG_BASE_I2C2                      (0xFCB0C000)
#define REG_BASE_I2C2_VIRT                 (0xFE7B7000)
#define REG_I2C2_IOSIZE                     PAGE_ALIGN(SZ_4K)

#define REG_BASE_MIPIHSI                   (0xFD203000)
#define REG_BASE_MIPIHSI_VIRT              (0xFE7B6000)
#define REG_MIPIHSI_IOSIZE                  PAGE_ALIGN(SZ_4K)

/* Peripheral */
#define REG_BASE_USB2DVC                   (0xFD240000)
#define REG_BASE_USB2DVC_VIRT              (0xFE776000)
#define REG_USB2DVC_IOSIZE                  PAGE_ALIGN(SZ_256K)

#define REG_BASE_USBOTG                    (0xFD240000)
#define REG_BASE_USBOTG_VIRT               (0xFE776000)
#define REG_USBOTG_IOSIZE                   PAGE_ALIGN(SZ_256K)

#define REG_BASE_USB2OHCI                  (0xFD202000)
#define REG_BASE_USB2OHCI_VIRT             (0xFE775000)
#define REG_USB2OHCI_IOSIZE                 PAGE_ALIGN(SZ_4K)

#define REG_BASE_USB2EHCI                  (0xFD201000)
#define REG_BASE_USB2EHCI_VIRT             (0xFE774000)
#define REG_USB2EHCI_IOSIZE                 PAGE_ALIGN(SZ_4K)

#define REG_BASE_NANDC_CFG                 (0xFD200000)
#define REG_BASE_NANDC_CFG_VIRT            (0xFE773000)
#define REG_NANDC_CFG_IOSIZE                PAGE_ALIGN(SZ_4K)

#define REG_BASE_NAND                      (0xFD100000)
#define REG_BASE_NAND_VIRT                 (0xFE673000)
#define REG_NAND_IOSIZE                     PAGE_ALIGN(SZ_1M)

#define REG_BASE_PCIE_CFG                  (0xFCE00000)
#define REG_BASE_PCIE_CFG_VIRT             (0xFE671000)
#define REG_PCIE_CFG_IOSIZE                 PAGE_ALIGN(SZ_8K)

#define REG_BASE_MMC3                      (0xFCD06000)
#define REG_BASE_MMC3_VIRT                 (0xFE670000)
#define REG_MMC3_IOSIZE                     PAGE_ALIGN(SZ_4K)

#define REG_BASE_MMC2                      (0xFCD05000)
#define REG_BASE_MMC2_VIRT                 (0xFE66F000)
#define REG_MMC2_IOSIZE                     PAGE_ALIGN(SZ_4K)

#define REG_BASE_MMC1                      (0xFCD04000)
#define REG_BASE_MMC1_VIRT                 (0xFE66E000)
#define REG_MMC1_IOSIZE                     PAGE_ALIGN(SZ_4K)

#define REG_BASE_MMC0                      (0xFCD03000)
#define REG_BASE_MMC0_VIRT                 (0xFE66D000)
#define REG_MMC0_IOSIZE                     PAGE_ALIGN(SZ_4K)

#define REG_BASE_VENC                      (0xFA209000)
#define REG_BASE_VENC_VIRT                 (0xFE66C000)
#define REG_VENC_IOSIZE                     PAGE_ALIGN(SZ_4K)

#define REG_BASE_VDEC                      (0xFA208000)
#define REG_BASE_VDEC_VIRT                 (0xFE66B000)
#define REG_VDEC_IOSIZE                     PAGE_ALIGN(SZ_4K)

#define REG_BASE_G2D                       (0xFA040000)
#define REG_BASE_G2D_VIRT                  (0xFE62B000)
#define REG_G2D_IOSIZE                      PAGE_ALIGN(SZ_256K)

#define REG_BASE_G3D                       (0xFA000000)
#define REG_BASE_G3D_VIRT                  (0xFE5EB000)
#define REG_G3D_IOSIZE                      PAGE_ALIGN(SZ_256K)

/* define CONFIG_MACH_TC45MSU3 */
#elif defined(CONFIG_MACH_TC45MSU3)
/* CONFIG K3 FPGA */
/* config_f4 */
#define REG_BASE_G2D                       (0xFE900000)
#define REG_BASE_G2D_VIRT                  (0xFE788000)
#define REG_G2D_IOSIZE                      PAGE_ALIGN(SZ_256K)

#define REG_BASE_G3D                       (0xFE800000)
#define REG_BASE_G3D_VIRT                  (0xFE748000)
#define REG_G3D_IOSIZE                      PAGE_ALIGN(SZ_256K)

/* config_f1 */
#define REG_BASE_USB2DVC                   (0xFE600000)
#define REG_BASE_USB2DVC_VIRT              (0xFE708000)
#define REG_USB2DVC_IOSIZE                  PAGE_ALIGN(SZ_256K)

#define REG_BASE_USB2OHCI                  (0xFE501000)
#define REG_BASE_USB2OHCI_VIRT             (0xFE707000)
#define REG_USB2OHCI_IOSIZE                 PAGE_ALIGN(SZ_4K)

#define REG_BASE_USB2EHCI                  (0xFE500000)
#define REG_BASE_USB2EHCI_VIRT             (0xFE706000)
#define REG_USB2EHCI_IOSIZE                 PAGE_ALIGN(SZ_4K)

#define REG_BASE_NANDC_CFG                 (0xFE400000)
#define REG_BASE_NANDC_CFG_VIRT            (0xFE705000)
#define REG_NANDC_CFG_IOSIZE                PAGE_ALIGN(SZ_4K)

#define REG_BASE_NAND                      (0xFE300000)
#define REG_BASE_NAND_VIRT                 (0xFE605000)
#define REG_NAND_IOSIZE                     PAGE_ALIGN(SZ_1M)

#define REG_BASE_PCIE_CFG                  (0xFE200000)
#define REG_BASE_PCIE_CFG_VIRT             (0xFE5F5000)
#define REG_PCIE_CFG_IOSIZE                 PAGE_ALIGN(SZ_64K)

#define REG_BASE_USB2                      (0xFE109000)
#define REG_BASE_USB2_VIRT                 (0xFE5F4000)
#define REG_USB2_IOSIZE                     PAGE_ALIGN(SZ_4K)

#define REG_BASE_USB1                      (0xFE108000)
#define REG_BASE_USB1_VIRT                 (0xFE5F3000)
#define REG_USB1_IOSIZE                     PAGE_ALIGN(SZ_4K)

#define REG_BASE_USB0                      (0xFE107000)
#define REG_BASE_USB0_VIRT                 (0xFE5F2000)
#define REG_USB0_IOSIZE                     PAGE_ALIGN(SZ_4K)

#define REG_BASE_GPIO22                    (0xFE105000)
#define REG_BASE_GPIO22_VIRT               (0xFE5F1000)
#define REG_GPIO22_IOSIZE                   PAGE_ALIGN(SZ_4K)

#define REG_BASE_VENC                      (0xFE800000)
#define REG_BASE_VENC_VIRT                 (0xFE5F0000)
#define REG_VENC_IOSIZE                     PAGE_ALIGN(SZ_4K)

#define REG_BASE_MMC3                      (0xFE103000)
#define REG_BASE_MMC3_VIRT                 (0xFE5EF000)
#define REG_MMC3_IOSIZE                     PAGE_ALIGN (SZ_4K)

#define REG_BASE_MMC2                      (0xFE102000)
#define REG_BASE_MMC2_VIRT                 (0xFE5EE000)
#define REG_MMC2_IOSIZE                     PAGE_ALIGN(SZ_4K)

#define REG_BASE_MMC1                      (0xFE101000)
#define REG_BASE_MMC1_VIRT                 (0xFE5ED000)
#define REG_MMC1_IOSIZE                     PAGE_ALIGN(SZ_4K)

#define REG_BASE_MMC0                      (0xFE100000)
#define REG_BASE_MMC0_VIRT                 (0xFE5EC000)
#define REG_MMC0_IOSIZE                     PAGE_ALIGN(SZ_4K)

/* config_f3 */
#define REG_BASE_VDEC                      (0xFE000000)
#define REG_BASE_VDEC_VIRT                 (0xFE5EB000)
#define REG_VDEC_IOSIZE                     PAGE_ALIGN(SZ_4K)
#endif
/* end define CONFIG_MACH_TC45MSU3 */

#define REG_BASE_CA9dbgAPB                 (0xFD400000)
#define REG_BASE_CA9dbgAPB_VIRT            (0xFE3EB000)
#define REG_CA9dbgAPB_IOSIZE                PAGE_ALIGN(SZ_2M)

#define REG_BASE_SECENG                    (0xFD204000)
#define REG_BASE_SECENG_VIRT               (0xFE3E7000)
#define REG_SECENG_IOSIZE                   PAGE_ALIGN(SZ_16K)

#define REG_BASE_MCU                       (0xFD000000)
#define REG_BASE_MCU_VIRT                  (0xFE3D7000)
#define REG_MCU_IOSIZE                      PAGE_ALIGN(SZ_64K)

#define REG_BASE_DMAC                      (0xFCD02000)
#define REG_BASE_DMAC_VIRT                 (0xFE3D6000)
#define REG_DMAC_IOSIZE                     PAGE_ALIGN(SZ_4K)

#define REG_BASE_DDRC_CFG                  (0xFCD00000)
#define REG_BASE_DDRC_CFG_VIRT             (0xFE3D4000)
#define REG_DDRC_CFG_IOSIZE                 PAGE_ALIGN(SZ_8K)

#define REG_BASE_PMUSPI                    (0xFCC00000)
#define REG_BASE_PMUSPI_VIRT               (0xFE2D4000)
#define REG_PMUSPI_IOSIZE                   PAGE_ALIGN(SZ_1M)

/* Config */
#define REG_BASE_GPS                       (0xFCB0B000)
#define REG_BASE_GPS_VIRT                  (0xFE2D3000)
#define REG_GPS_IOSIZE                      PAGE_ALIGN(SZ_4K)

#define REG_BASE_SCI                       (0xFCB0A000)
#define REG_BASE_SCI_VIRT                  (0xFE2D2000)
#define REG_SCI_IOSIZE                      PAGE_ALIGN(SZ_4K)

#define REG_BASE_I2C1                      (0xFCB09000)
#define REG_BASE_I2C1_VIRT                 (0xFE2D1000)
#define REG_I2C1_IOSIZE                     PAGE_ALIGN(SZ_4K)

#define REG_BASE_I2C0                      (0xFCB08000)
#define REG_BASE_I2C0_VIRT                 (0xFE2D0000)
#define REG_I2C0_IOSIZE                     PAGE_ALIGN(SZ_4K)

#define REG_BASE_SPI2                      (0xFCB07000)
#define REG_BASE_SPI2_VIRT                 (0xFE2CF000)
#define REG_SPI2_IOSIZE                     PAGE_ALIGN(SZ_4K)

#define REG_BASE_SPI1                      (0xFCB06000)
#define REG_BASE_SPI1_VIRT                 (0xFE2CE000)
#define REG_SPI1_IOSIZE                     PAGE_ALIGN(SZ_4K)

#define REG_BASE_SPI0                      (0xFCB05000)
#define REG_BASE_SPI0_VIRT                 (0xFE2CD000)
#define REG_SPI0_IOSIZE                     PAGE_ALIGN(SZ_4K)

#define REG_BASE_UART4                     (0xFCB04000)
#define REG_BASE_UART4_VIRT                (0xFE2CC000)
#define REG_UART4_IOSIZE                    PAGE_ALIGN(SZ_4K)

#if defined(CONFIG_CONNECTIVE_CONTROL_FPGA)
#define REG_BASE_UART3                     (0xFE10C000)
#else
#define REG_BASE_UART3                     (0xFCB03000)
#endif
#define REG_BASE_UART3_VIRT                (0xFE2CB000)
#define REG_UART3_IOSIZE                    PAGE_ALIGN(SZ_4K)

#if defined(CONFIG_CONNECTIVE_CONTROL_FPGA)
#define REG_BASE_UART2                     (0xFE10D000)
#else
#define REG_BASE_UART2                     (0xFCB02000)
#endif
#define REG_BASE_UART2_VIRT                (0xFE2CA000)
#define REG_UART2_IOSIZE                    PAGE_ALIGN(SZ_4K)

#define REG_BASE_UART1                     (0xFCB01000)
#define REG_BASE_UART1_VIRT                (0xFE2C9000)
#define REG_UART1_IOSIZE                    PAGE_ALIGN(SZ_4K)

#define REG_BASE_UART0                     (0xFCB00000)
#define REG_BASE_UART0_VIRT                (0xFE2C8000)
#define REG_UART0_IOSIZE                    PAGE_ALIGN(SZ_4K)

#define REG_BASE_HDMI_EFC                  (0xFCA0B000)
#define REG_BASE_HDMI_EFC_VIRT             (0xFE2C7000)
#define REG_HDMI_EFC_IOSIZE                 PAGE_ALIGN(SZ_4K)

#define REG_BASE_EFUSEC                    (0xFCA0A000)
#define REG_BASE_EFUSEC_VIRT               (0xFE2C6000)
#define REG_EFUSEC_IOSIZE                   PAGE_ALIGN(SZ_4K)

#define REG_BASE_PCTRL                     (0xFCA09000)
#define REG_BASE_PCTRL_VIRT                (0xFE2C5000)
#define REG_PCTRL_IOSIZE                    PAGE_ALIGN(SZ_4K)

#define REG_BASE_PMCTRL                    (0xFCA08000)
#define REG_BASE_PMCTRL_VIRT               (0xFE2C4000)
#define REG_PMCTRL_IOSIZE                   PAGE_ALIGN(SZ_4K)

#define REG_BASE_TBC                       (0xFCA07000)
#define REG_BASE_TBC_VIRT                  (0xFE2C3000)
#define REG_TBC_IOSIZE                      PAGE_ALIGN(SZ_4K)

#define REG_BASE_PWM1                      (0xFCA06000)
#define REG_BASE_PWM1_VIRT                 (0xFE2C2000)
#define REG_PWM1_IOSIZE                     PAGE_ALIGN(SZ_4K)

#define REG_BASE_PWM0                      (0xFCA05000)
#define REG_BASE_PWM0_VIRT                 (0xFE2C1000)
#define REG_PWM0_IOSIZE                     PAGE_ALIGN(SZ_4K)

#define REG_BASE_WD                        (0xFCA04000)
#define REG_BASE_WD_VIRT                   (0xFE2C0000)
#define REG_WD_IOSIZE                       PAGE_ALIGN(SZ_4K)

#define REG_BASE_TIMER4                    (0xFCA03000)
#define REG_BASE_TIMER4_VIRT               (0xFE2BF000)
#define REG_TIMER4_IOSIZE                   PAGE_ALIGN(SZ_4K)

#define REG_BASE_TIMER3                    (0xFCA02000)
#define REG_BASE_TIMER3_VIRT               (0xFE2BE000)
#define REG_TIMER3_IOSIZE                   PAGE_ALIGN(SZ_4K)

#define REG_BASE_TIMER2                    (0xFCA01000)
#define REG_BASE_TIMER2_VIRT               (0xFE2BD000)
#define REG_TIMER2_IOSIZE                   PAGE_ALIGN(SZ_4K)

#define REG_BASE_TZPC                      (0xFCA00000)
#define REG_BASE_TZPC_VIRT                 (0xFE2BC000)
#define REG_TZPC_IOSIZE                     PAGE_ALIGN(SZ_4K)

#if defined(CONFIG_CONNECTIVE_CONTROL_FPGA)
#define REG_BASE_GPIO21                    (0xFE10F000)
#else
#define REG_BASE_GPIO21                    (0xFC81B000)
#endif
#define REG_BASE_GPIO21_VIRT               (0xFE2BB000)
#define REG_GPIO21_IOSIZE          	    PAGE_ALIGN(SZ_4K)

#if defined(CONFIG_CONNECTIVE_CONTROL_FPGA)
#define REG_BASE_GPIO20                    (0xFE10E000)
#else
#define REG_BASE_GPIO20                    (0xFC81A000)
#endif
#define REG_BASE_GPIO20_VIRT               (0xFE2BA000)
#define REG_GPIO20_IOSIZE                   PAGE_ALIGN(SZ_4K)

#define REG_BASE_GPIO19                    (0xFC819000)
#define REG_BASE_GPIO19_VIRT               (0xFE2B9000)
#define REG_GPIO19_IOSIZE                   PAGE_ALIGN(SZ_4K)

#define REG_BASE_GPIO18                    (0xFC818000)
#define REG_BASE_GPIO18_VIRT               (0xFE2B8000)
#define REG_GPIO18_IOSIZE                   PAGE_ALIGN(SZ_4K)

#define REG_BASE_GPIO17                    (0xFC817000)
#define REG_BASE_GPIO17_VIRT               (0xFE2B7000)
#define REG_GPIO17_IOSIZE                   PAGE_ALIGN(SZ_4K)

#define REG_BASE_GPIO16                    (0xFC816000)
#define REG_BASE_GPIO16_VIRT               (0xFE2B6000)
#define REG_GPIO16_IOSIZE                   PAGE_ALIGN(SZ_4K)

#define REG_BASE_GPIO15                    (0xFC815000)
#define REG_BASE_GPIO15_VIRT               (0xFE2B5000)
#define REG_GPIO15_IOSIZE                   PAGE_ALIGN(SZ_4K)

#define REG_BASE_GPIO14                    (0xFC814000)
#define REG_BASE_GPIO14_VIRT               (0xFE2B4000)
#define REG_GPIO14_IOSIZE                   PAGE_ALIGN(SZ_4K)

#define REG_BASE_GPIO13                    (0xFC813000)
#define REG_BASE_GPIO13_VIRT               (0xFE2B3000)
#define REG_GPIO13_IOSIZE                   PAGE_ALIGN(SZ_4K)

#define REG_BASE_GPIO12                    (0xFC812000)
#define REG_BASE_GPIO12_VIRT               (0xFE2B2000)
#define REG_GPIO12_IOSIZE                   PAGE_ALIGN(SZ_4K)

#define REG_BASE_GPIO11                    (0xFC811000)
#define REG_BASE_GPIO11_VIRT               (0xFE2B1000)
#define REG_GPIO11_IOSIZE                   PAGE_ALIGN(SZ_4K)

#define REG_BASE_GPIO10                    (0xFC810000)
#define REG_BASE_GPIO10_VIRT               (0xFE2B0000)
#define REG_GPIO10_IOSIZE                   PAGE_ALIGN(SZ_4K)

#define REG_BASE_GPIO9                     (0xFC80F000)
#define REG_BASE_GPIO9_VIRT                (0xFE2AF000)
#define REG_GPIO9_IOSIZE                    PAGE_ALIGN(SZ_4K)

#define REG_BASE_GPIO8                     (0xFC80E000)
#define REG_BASE_GPIO8_VIRT                (0xFE2AE000)
#define REG_GPIO8_IOSIZE                    PAGE_ALIGN(SZ_4K)

#define REG_BASE_GPIO7                     (0xFC80D000)
#define REG_BASE_GPIO7_VIRT                (0xFE2AD000)
#define REG_GPIO7_IOSIZE                    PAGE_ALIGN(SZ_4K)

#define REG_BASE_GPIO6                     (0xFC80C000)
#define REG_BASE_GPIO6_VIRT                (0xFE2AC000)
#define REG_GPIO6_IOSIZE                    PAGE_ALIGN(SZ_4K)

#define REG_BASE_GPIO5                     (0xFC80B000)
#define REG_BASE_GPIO5_VIRT                (0xFE2AB000)
#define REG_GPIO5_IOSIZE                    PAGE_ALIGN(SZ_4K)

#define REG_BASE_GPIO4                     (0xFC80A000)
#define REG_BASE_GPIO4_VIRT                (0xFE2AA000)
#define REG_GPIO4_IOSIZE                    PAGE_ALIGN(SZ_4K)

#define REG_BASE_GPIO3                     (0xFC809000)
#define REG_BASE_GPIO3_VIRT                (0xFE2A9000)
#define REG_GPIO3_IOSIZE                    PAGE_ALIGN(SZ_4K)

#define REG_BASE_GPIO2                     (0xFC808000)
#define REG_BASE_GPIO2_VIRT                (0xFE2A8000)
#define REG_GPIO2_IOSIZE                    PAGE_ALIGN(SZ_4K)

#define REG_BASE_GPIO1                     (0xFC807000)
#define REG_BASE_GPIO1_VIRT                (0xFE2A7000)
#define REG_GPIO1_IOSIZE           	       PAGE_ALIGN(SZ_4K)

#define REG_BASE_GPIO0                     (0xFC806000)
#define REG_BASE_GPIO0_VIRT                (0xFE2A6000)
#define REG_GPIO0_IOSIZE                    PAGE_ALIGN(SZ_4K)

#define REG_BASE_KPC                       (0xFC805000)
#define REG_BASE_KPC_VIRT                  (0xFE2A5000)
#define REG_KPC_IOSIZE                      PAGE_ALIGN(SZ_4K)

#define REG_BASE_RTC                       (0xFC804000)
#define REG_BASE_RTC_VIRT                  (0xFE2A4000)
#define REG_RTC_IOSIZE                      PAGE_ALIGN(SZ_4K)

#define REG_BASE_IOC                       (0xFC803000)
#define REG_BASE_IOC_VIRT                  (0xFE2A3000)
#define REG_IOC_IOSIZE                      PAGE_ALIGN(SZ_4K)

#define REG_BASE_SCTRL                     (0xFC802000)
#define REG_BASE_SCTRL_VIRT                (0xFE2A2000)
#define REG_SCTRL_IOSIZE                    PAGE_ALIGN(SZ_4K)

#define REG_BASE_TIMER1                    (0xFC801000)
#define REG_BASE_TIMER1_VIRT               (0xFE2A1000)
#define REG_TIMER1_IOSIZE                   PAGE_ALIGN(SZ_4K)

#define REG_BASE_TIMER0                    (0xFC800000)
#define REG_BASE_TIMER0_VIRT               (0xFE2A0000)
#define REG_TIMER0_IOSIZE                   PAGE_ALIGN(SZ_4K)

/* CPU */
#define REG_BASE_L2CC                      (0xFC100000)
#define REG_BASE_L2CC_VIRT                 (0xFE1A0000)
#define REG_L2CC_IOSIZE                     PAGE_ALIGN(SZ_1M)

#define REG_BASE_A9PER                      (0xFC000000)
#define REG_BASE_A9PER_VIRT                 (0xFE09F000)
#define REG_A9PER_IOSIZE                     PAGE_ALIGN(SZ_1M)

/* media */
#define REG_BASE_VPP                       (0xFA20B000)
#define REG_BASE_VPP_VIRT                  (0xFE09E000)
#define REG_VPP_IOSIZE                      PAGE_ALIGN(SZ_4K)

/* define DISP1=0xFA204000 now. EDC1 belong to DISP1 subsys */
#define REG_BASE_EDC1                      (0xFA206000)
#define REG_BASE_EDC1_VIRT                 (0xFE09C000)
#define REG_EDC1_IOSIZE                     PAGE_ALIGN(SZ_8K)

#define REG_BASE_HDMI                      (0xFA204000)
#define REG_BASE_HDMI_VIRT                 (0xFE09A000)
#define REG_HDMI_IOSIZE                     PAGE_ALIGN(SZ_8K)
/* define DISP0=0xFA200000 now EDC0 belong to DISP0 subsys*/
#define REG_BASE_EDC0                      (0xFA202000)
#define REG_BASE_EDC0_VIRT                 (0xFE098000)
#define REG_EDC0_IOSIZE                     PAGE_ALIGN(SZ_8K)

#define REG_BASE_ASP                       (0xFA100000)
#define REG_BASE_ASP_VIRT                  (0xFE094000)
#define REG_ASP_IOSIZE                      PAGE_ALIGN(SZ_16K)

#define REG_BASE_ISP                       (0xFA080000)
#define REG_BASE_ISP_VIRT                  (0xFE014000)
#define REG_ISP_IOSIZE                      PAGE_ALIGN(SZ_512K)

/* SRAM 80K */
#define REG_BASE_SECRAM                    (0xF8000000)
#define REG_BASE_SECRAM_VIRT               (0xFE000000)
#define REG_SECRAM_IOSIZE                   PAGE_ALIGN(0x14000)

/* PCIE */
#define REG_BASE_PCIE                      (0xF0000000)
#define REG_BASE_PCIE_VIRT                 (0xF6000000)
#define REG_PCIE_IOSIZE                     PAGE_ALIGN(SZ_128M)

/* DRAM 768M */
#define REG_BASE_DRAM3                     (0xC0000000)
#define REG_DRAM3_IOSIZE                    PAGE_ALIGN(0x30000000)

#define REG_BASE_DRAM2                     (0x80000000)
#define REG_DRAM2_IOSIZE                    PAGE_ALIGN(SZ_1G)

#define REG_BASE_DRAM1                     (0x40000000)
#define REG_DRAM1_IOSIZE                    PAGE_ALIGN(SZ_1G)

#define REG_BASE_DRAM0                     (0x00000000)
#define REG_DRAM0_IOSIZE                    PAGE_ALIGN(SZ_1G)

/*modules base and size*/

#endif	/* __MACH_K3V2_PLATFORM_H */
