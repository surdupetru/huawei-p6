//------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
//  Use of this source code is subject to the terms of the Microsoft end-user
//  license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
//  If you did not accept the terms of the EULA, you are not authorized to use
//  this source code. For a copy of the EULA, please see the LICENSE.RTF on
//  your install media.
//
//------------------------------------------------------------------------------

/*******************************************************************************
 * Copyright:   Copyright (c) 2007. Hisilicon Technologies, CO., LTD. 
 * Version:     V300R001B03
 * Filename:    cspiClass.cpp
 * Description: spi总线驱动的功能实现
 * History:
                1.Created by wubin on 2007/06/07
*******************************************************************************/

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
//#include <linux/config.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
//#include <linux/devfs_fs_kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <asm/uaccess.h>
#include <mach/hardware.h>
#include <asm/io.h>
#include <asm/system.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/string.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <mach/platform.h>
#include <mach/early-debug.h>
#include <linux/timer.h>
#include <linux/sched.h>
/* Our header */
#include <linux/android_pmem.h>

//#include <mach/HiDriverError.h>
#include <mach/hisi_spi2.h>
//#include <mach/hisi_dmac.h>
#include "spi2.h"
//--------------------------------------------------------------------------
// Global Variables
volatile unsigned char         *g_pRegSystemControl = NULL;
volatile unsigned char          *g_IomgcgRegsAddress = NULL;
volatile unsigned char         *g_GPIO7RegsAddress = NULL;
volatile PCSP_CSPI_REG  g_pSpiReg = NULL;

#define M_SYSCTRL_SCPEREN    ((volatile unsigned int *)(g_pRegSystemControl+0x24))
#define M_SYSCTRL_SCPERDIS   ((volatile unsigned int *)(g_pRegSystemControl+0x28))
#define M_SYSCTRL_SCPERCLKEN ((volatile unsigned int *)(g_pRegSystemControl+0x2C))
#define M_SYSCTRL_SCPERSTAT  ((volatile unsigned int *)(g_pRegSystemControl+0x30))
#define M_SYSCTRL_SCPERCTRT5 ((volatile unsigned int *)(g_pRegSystemControl+0x58))
#define M_SYSCTRL_SCRSTCTRL2 ((volatile unsigned int *)(g_pRegSystemControl+0x48))
#define M_SYSCTRL_SCPERDIS   ((volatile unsigned int *)(g_pRegSystemControl+0x28))


static int spi2_trace_level = HISPI2_TRACE_LEVEL;

CEDEVICE_POWER_STATE m_dxCurrent = D0;

/* Protection for the above */
static DEFINE_MUTEX(m_cspiCs);

/*添加定时器模块*/
static struct timer_list timer;
static unsigned int detect_time = (HZ);


#if 0
typedef struct SPI_DMA_CFG_S
{
	unsigned int channel;
	wait_queue_head_t isrEvent;

	dmac_scatternode node;
	dmac_scatterlist sg;

	dma_addr_t dma_phys;
	void *dma_virt;
	unsigned int size;
}SPI_DMA_CFG;

SPI_DMA_CFG m_RcvDMACfg;
SPI_DMA_CFG m_XmitDMACfg;
#endif


BOOL    CspiClkControl(BOOL b);
void    CspiRelease(void);
unsigned int CspiBufRd8(void * pBuf);
void   CspiBufWrt8(void * pBuf, unsigned int data);
unsigned int CspiBufRd16(void * pBuf);
void   CspiBufWrt16(void * pBuf, unsigned int data);
void   CspiReset(unsigned int index);
BOOL   ChipSelectCS0(void);
BOOL   ChipSelectCS1(void);
BOOL   ChipSelectCS2(void);
BOOL   CspiDmaInit(void);
void CspiIoInit(unsigned int index, SPI_CS cs);
void CspiIoPark(unsigned int index);



/*定时器溢出处理: 对spi2下电*/
static void clk_detection(unsigned long arg)
{	 
    CspiIoPark(2);
    CspiClkControl(FALSE);  
    hispi2_trace(1,"spi2 timer out,spi2 clk close\n");
}

/*******************************************************************************
  Function:     CspiInitialize
  Description:  spi类数据成员初始化函数
  Input:        1.Index：spi模块索引号
                2.
  Output:       none
  Return：      TRUE: 初始化成功；FALSE：初始化不成功
  History:     
                1. Created by wubin on 2007/06/07
                2. .....
*******************************************************************************/
BOOL CspiInitialize(void)
{
#ifdef __ASIC__
    //得到虚拟地址
    g_pRegSystemControl = (unsigned char *)IO_ADDRESS(SYSTEM_CONTROL_BASE);
    
    g_IomgcgRegsAddress = (unsigned char *)IO_ADDRESS(SYSTEM_IOMGCG_BASE);
    
    g_GPIO7RegsAddress = (unsigned char *)IO_ADDRESS(REG_BASE_GPIO7);
#endif
    
    g_pSpiReg = (CSP_CSPI_REG *)IO_ADDRESS(CSP_BASE_REG_PA_CSPI2);
   
    //使能spi时钟
    CspiClkControl(TRUE);

    // disable CSPI to reset internal logic
    INSREG32(&g_pSpiReg->SSPCR1, CSP_BITFMASK(CSPI_SSPCR1_SSE),
             CSP_BITFVAL(CSPI_SSPCR1_SSE, CSPI_SSPCR1_SSE_DISABLE));

    // disable clock for spi after initialize for low power consumption
    CspiClkControl(FALSE);

   //使能SPI2的时钟
    CspiIoInit(2, CS1);//lcd

    //对DMA进行初始化
    if(!CspiDmaInit())
    {
        hispi2_trace(3, "Failed to initial DMA channel for SPI2.\r\n");
        CspiRelease();
        return FALSE;
    }

   /*对定时器进行初始化*/
   init_timer(&timer);
   timer.function = clk_detection;
   timer.expires = jiffies + detect_time;

    return TRUE;
}

/*******************************************************************************
  Function:     CspiRelease
  Description:  spi类数据成员去初始化函数,释放分配的内存，关闭句柄
  Input:        1.none
                2.
  Output:       none
  Return：      none
  History:     
                1. Created by wubin on 2007/06/07
                2. .....
*******************************************************************************/
void CspiRelease(void)
{
#if 0
#ifdef __ASIC__
    if(DMAC_STATUS_SUCCESS != dmac_channel_free(m_RcvDMACfg.channel))
    {
		hispi2_trace(3, "ERR:SPI2 free Recv channel failure\r\n");
    }
    
    if(DMAC_STATUS_SUCCESS != dmac_channel_free(m_XmitDMACfg.channel))
    {
    		hispi2_trace(3, "ERR:SPI2 free Xmit channel failure\r\n");
    }
#endif
#endif
}

/*******************************************************************************
  Function:     CspiDataExchange
  Description:  在主模式下传输spi数据
  Input:        1.pXchPkt: 传输数据包的指针
                2.
  Output:       none
  Return：      返回完成传输的数据字节数,如果出错返回-1
  History:     
                1. Created by wubin on 2007/06/10
                2. .....
*******************************************************************************/
int CspiDataExchange(PCSPI_XCH_PKT_T pXchPkt)
{

    PCSPI_BUSCONFIG_T pBusCnfg =NULL ;
    void * pTxBuf = NULL;
    void * pRxBuf = NULL;
    unsigned int xchTxCnt = 0;
    unsigned int xchRxCnt = 0;


    volatile unsigned int tmp;
    BOOL bXchDone = FALSE;
    unsigned int (*pfnTxBufRd)(void *);
    void (*pfnRxBufWrt)(void *, unsigned int);
    unsigned char bufIncr;
    int   wait;

    	 
    if(NULL==pXchPkt)
    {
        hispi2_trace(3,"Input pXchPkt is NULL.\r\n");
        return -1;
    }

    if (0 == pXchPkt->xchCnt)
    {
        hispi2_trace(3,("Input xchCnt is 0.\r\n"));
        return -1;
    }

    pBusCnfg = pXchPkt->pBusCnfg;
    pTxBuf = pXchPkt->pTxBuf;
    pRxBuf = pXchPkt->pRxBuf;

    // check all translated pointers
    if ((NULL == pBusCnfg) || (NULL == pTxBuf))
    {
        hispi2_trace(3,"Input pBusCnfg or pTxBuf is NULL.\r\n");
        return -1;
    }

    // check burst_num
    if ((pBusCnfg->burst_num <1) || (pBusCnfg->burst_num >8))
    {
        hispi2_trace(HISPI2_TRACE_LEVEL,"burst_num beyond the range.\r\n");
        return -1;
    }


    // select access funtions based on exchange bit width
    //
    // bitcount        Tx/Rx Buffer Access Width
    // --------        -------------------------
    //   1 - 8           UINT8 (unsigned 8-bit)
    //   9 - 16          UINT16 (unsigned 16-bit)
    //  17 - 32          UINT32 (unsigned 32-bit)
    //
    if ((pBusCnfg->bitcount >= 3) && (pBusCnfg->bitcount <= 7))
    {
        // 8-bit access width
        pfnTxBufRd = CspiBufRd8;
        pfnRxBufWrt = CspiBufWrt8;
        bufIncr = sizeof(unsigned char);
    }
    else if ((pBusCnfg->bitcount >= 8) && (pBusCnfg->bitcount <= 15))
    {
        // 16-bit access width
        pfnTxBufRd = CspiBufRd16;
        pfnRxBufWrt = CspiBufWrt16;
        bufIncr = sizeof(unsigned short);
    }
    else
    {
        // unsupported access width
        hispi2_trace(3,"Unsupported bitcount %d.\r\n",pBusCnfg->bitcount);
        return -1;
    }


    
    if( (pBusCnfg->sph != 1) && (pBusCnfg->sph !=0 ) )
    {
        // unsupported access width
        hispi2_trace(3,"unsupported SPH %d.\r\n",pBusCnfg->sph);
        return -1;
    }

    if( (pBusCnfg->spo != 1) && (pBusCnfg->spo !=0 ) )
    {
        // unsupported access width
        hispi2_trace(3,"unsupported SPO %d.\r\n",pBusCnfg->spo);
        return -1;
    }


    /* Start SPI Exchange operation */   
    mutex_lock(&m_cspiCs);
    
     
    //如果当前定时器挂起，则需要删除定时器
    if(timer_pending(&timer))
    {
        del_timer(&timer);
        CspiIoInit(2, pXchPkt->spiDevice);
    }
    else    //如果定时器超时，则启动时钟和初始化io
    {
        CspiIoInit(2, pXchPkt->spiDevice);
        CspiClkControl(TRUE);
        CspiReset(2);	  
     }

    // set client CSPI bus configuration based
    //  default LBM = DISABLE
    //  default SSE = DISABLE
    //  default MS = MASTER
    //  default SOD = DISABLE
    OUTREG32(&g_pSpiReg->SSPCR1,
             CSP_BITFVAL(CSPI_SSPCR1_LBM, CSPI_SSPCR1_LBM_DISABLE) |
             CSP_BITFVAL(CSPI_SSPCR1_SSE, CSPI_SSPCR1_SSE_DISABLE) |
             CSP_BITFVAL(CSPI_SSPCR1_MS, CSPI_SSPCR1_MS_MASTER) |
             CSP_BITFVAL(CSPI_SSPCR1_SOD, CSPI_SSPCR1_SOD_DISABLE));

    OUTREG32(&g_pSpiReg->SSPCR0,
             CSP_BITFVAL(CSPI_SSPCR0_FRF, CSPI_SSPCR0_FRF_SPI) |
             CSP_BITFVAL(CSPI_SSPCR0_SPO, pBusCnfg->spo) |
             CSP_BITFVAL(CSPI_SSPCR0_SPH, pBusCnfg->sph) |
             CSP_BITFVAL(CSPI_SSPCR0_DSS, pBusCnfg->bitcount) |
             CSP_BITFVAL(CSPI_SSPCR0_SCR, pBusCnfg->scr));
//printk("spo=%d,sph =%d,bitcount=%d,scr=%d\n",pBusCnfg->spo,pBusCnfg->sph,pBusCnfg->bitcount,pBusCnfg->scr);

    //选择器件
    switch(pXchPkt->spiDevice)
    {
    case CS1:
        {
            ChipSelectCS1();
            hispi2_trace(3,"Cspi select CS1 succeed!\r\n");
            break;
        }
    case CS2:
        {
            ChipSelectCS2();
            hispi2_trace(3, "Cspi select CS2 succeed!\r\n");
            break;
        }
    case CS0:
        {
            ChipSelectCS0();
            hispi2_trace(3,"Cspi select CS0 succeed!\r\n");
            break;
        }
    case NO_DEVICE:
    default:
        {
            hispi2_trace(3,"Error CS enum value %d!\r\n",pXchPkt->spiDevice);
	     xchRxCnt = -1;
            goto  DATAEXCHANGELEAVE;
        }
    }

    // 关闭中断
    OUTREG32(&g_pSpiReg->SSPIMSC, 0);
    OUTREG32(&g_pSpiReg->SSPCPSR,
	    CSP_BITFVAL(CSPI_SSPCPSR_CPSDVSR, pBusCnfg->cpsdvsr));


    // until we are done with requested transfers
    while(!bXchDone)
    {
        unsigned char i = pBusCnfg->burst_num;
        // disable ssp
        INSREG32(&g_pSpiReg->SSPCR1, CSP_BITFMASK(CSPI_SSPCR1_SSE),
                 CSP_BITFVAL(CSPI_SSPCR1_SSE, CSPI_SSPCR1_SSE_DISABLE));
        // load Tx FIFO until full, or until we run out of data
        while ((INREG32(&g_pSpiReg->SSPSR) & CSP_BITFMASK(CSPI_SSPSR_TNF))//TNF TFE
                && (xchTxCnt < pXchPkt->xchCnt) && (i != 0))
        {
            // put next Tx data into CSPI FIFO
			//printk("---TxBuf=0x%x---\n", pfnTxBufRd(pTxBuf));
            OUTREG32(&g_pSpiReg->SSPDR, pfnTxBufRd(pTxBuf));  //从pTxBuf中读一个字节(或2，4个)
			
            // increment Tx Buffer to next data point
            pTxBuf = (void *) ((unsigned int) pTxBuf + bufIncr);
            // increment Tx exchange counter
            xchTxCnt += bufIncr;
            i-=1;
        }

        // enable ssp
        INSREG32(&g_pSpiReg->SSPCR1, CSP_BITFMASK(CSPI_SSPCR1_SSE),
                 CSP_BITFVAL(CSPI_SSPCR1_SSE, CSPI_SSPCR1_SSE_ENABLE));
			 
        // if we completed requested transfers, then we enable
        // interrupt for transfer completed (TCEN)
        if (xchTxCnt == pXchPkt->xchCnt)
        {

                // wait until transaction is complete
                for(wait = 0;
                        wait < CSPI_WAIT_TIMEOUT*10 && !(INREG32(&g_pSpiReg->SSPSR) & CSP_BITFMASK(CSPI_SSPSR_TFE));
                        wait ++)
                {}
						
				 
                if(wait == CSPI_WAIT_TIMEOUT*10)
                {
                   hispi2_trace(3,"Wait timeout 1.\r\n");
                }

            // set flag to indicate requested exchange done
            bXchDone = TRUE;
        }
        // otherwise we need to wait until FIFO has more room, so
        // we enable interrupt for Rx FIFO half-full (RHEN) to
        // ensure we can read out data that arrived during exchange
        else
        {

                for(wait = 0;
                        wait < CSPI_WAIT_TIMEOUT && !(INREG32(&g_pSpiReg->SSPSR) & CSP_BITFMASK(CSPI_SSPSR_TFE));
                        wait ++)
                {}

                //printk("wait = %d, SPISP = 0x%x \n",wait,(INREG32(&g_pSpiReg->SSPSR)));
		   
                if(wait == CSPI_WAIT_TIMEOUT)
                {
                    hispi2_trace(3,"Wait timeout 2.\r\n");
                }

        }

            while (xchRxCnt < xchTxCnt)
            {

                OUTREG32(&g_pSpiReg->SSPIMSC, 0);

	          // while there is data in Rx FIFO and we have buffer space
	          while ((INREG32(&g_pSpiReg->SSPSR) & CSP_BITFMASK(CSPI_SSPSR_RNE))
                                && (xchRxCnt < pXchPkt->xchCnt))
                {
                    tmp = INREG32(&g_pSpiReg->SSPDR);

                    // if receive data is not to be discarded
                    if (pRxBuf != NULL)
                        {
                            // get next Rx data from CSPI FIFO
                            pfnRxBufWrt(pRxBuf, tmp);

                            // increment Rx Buffer to next data point
                            pRxBuf = (void *) ((unsigned int) pRxBuf + bufIncr);
                        }

                        // increment Rx exchange counter
                        xchRxCnt += bufIncr;
                     }
				
                }

    } // while(!bXchDone)

    for(wait = 0;
            INREG32(&g_pSpiReg->SSPSR) & CSP_BITFMASK(CSPI_SSPSR_BSY) && wait < CSPI_WAIT_TIMEOUT;
            wait ++)
    {
        msleep(1);
    }

    if(wait == CSPI_WAIT_TIMEOUT)
    {
        hispi2_trace(3,"Wait timeout 3.\r\n");
    }
	

DATAEXCHANGELEAVE:

    mutex_unlock(&m_cspiCs);

    /*激活定时器*/
   mod_timer(&timer, jiffies + detect_time);

    return xchRxCnt;
}


//-----------------------------------------------------------------------------
//
// Function: CspiBufRd8
//
// This function is used to access a buffer as an array of 8-bit (UINT8)
// values and read data from the specified location.
//
// Parameters:
//      pBuf
//          [in] Pointer to buffer from which data will be read.
//
// Returns:
//      Returns data pointed to by specified buffer, promoted to UINT32.
//
//-----------------------------------------------------------------------------
unsigned int CspiBufRd8(void * pBuf)
{
    unsigned char *p;

    p = (unsigned char *) pBuf;

    return *p;
}


//-----------------------------------------------------------------------------
//
// Function: CspiBufRd16
//
// This function is used to access a buffer as an array of 16-bit (UINT16)
// values and read data from the specified location.
//
// Parameters:
//      pBuf
//          [in] Pointer to buffer from which data will be read.
//
// Returns:
//      Returns data pointed to by specified buffer, promoted to UINT32.
//
//-----------------------------------------------------------------------------
unsigned int CspiBufRd16(void * pBuf)
{
    unsigned short *p;

    p = (unsigned short *) pBuf;

    return *p;
}


//-----------------------------------------------------------------------------
//
// Function: CspiBufWrt8
//
// This function is used to access a buffer as an array of 8-bit (UINT8)
// values and writes data to the specified buffer location.
//
// Parameters:
//      pBuf
//          [in] Pointer to buffer to which data will be written.
//
//      data
//          [in] Data to be written demoted to UINT8.
//
// Returns:
//      None.
//
//-----------------------------------------------------------------------------
void CspiBufWrt8(void * pBuf, unsigned int data)
{
    unsigned char *p;

    p = (unsigned char *) pBuf;

    *p = (unsigned char) data;
}


//-----------------------------------------------------------------------------
//
// Function: CspiBufWrt16
//
// This function is used to access a buffer as an array of 16-bit (UINT16)
// values and writes data to the specified buffer location.
//
// Parameters:
//      pBuf
//          [in] Pointer to buffer to which data will be written.
//
//      data
//          [in] Data to be written demoted to UINT16.
//
// Returns:
//      None.
//
//-----------------------------------------------------------------------------
void CspiBufWrt16(void * pBuf, unsigned int data)
{
    unsigned short *p;

    p = (unsigned short *) pBuf;

    *p = (unsigned short) data;
}

//-----------------------------------------------------------------------------
//
// Function: CspiReset
//
// Parameters:
//      index
//          [in] choose SPI1, 2, or 3.
//
// Returns:
//      None.
//
//-----------------------------------------------------------------------------
void CspiReset(unsigned int  index)
{
#ifdef __ASIC__
    volatile unsigned int     oldReg, newReg;

    switch(index)
    {
    case 1:
        break;

    case 2:
        do
        {
            oldReg = *(unsigned int *)(M_SYSCTRL_SCRSTCTRL2);  // 读取寄存器值
            newReg = ( oldReg & (~(1U << 25)) );  // 修改为需要写入的值
            *(unsigned *)(M_SYSCTRL_SCRSTCTRL2) = newReg;
        }while(0);
        //while (InterlockedTestExchange((LPLONG)(M_SYSCTRL_SCRSTCTRL2), (LONG)oldReg, (LONG)newReg) != (LONG)oldReg);	  // 该函数保证在写入新值时，原始的值是否被修改，如果被修改则返回FALSE。

        //un-reset UART1 IP
        do
        {
            oldReg = *(unsigned *)(M_SYSCTRL_SCRSTCTRL2);		  // 读取寄存器值
            newReg = ( oldReg | (1U << 25) );   // 修改为需要写入的值
            *(unsigned *)(M_SYSCTRL_SCRSTCTRL2) = newReg;
        }while(0);
        //while (InterlockedTestExchange((LPLONG)(M_SYSCTRL_SCRSTCTRL2), (LONG)oldReg, (LONG)newReg) != (LONG)oldReg);	  // 该函数保证在写入新值时，原始的值是否被修改，如果被修改则返回FALSE。
        break;

    case 3:
    default:
        break;
    }
#endif
}

//-----------------------------------------------------------------------------
//
// Function: CspiIoPark
//
// 在SPI外设下电的时候，将host连接到SPI外设的IO配置到安全模式。
// 具体的说，也就是将输出管脚配置为输出低电平。这样则可以避免向外设漏电。
// 对于输入管脚不需要处理。
// 目前SPI1不使用，SPI2下挂的MTV/LCDC可能下电，SPI3下挂的PMU不会下电。因此
// 只需要关心SPI2。
// MTV和LCDC必须同时上下电。
//
// 目前在FPGA版本上面时钟控制不能生效，在ASIC版本上面再打开
//
// Parameters:
//      index
//          [in] choose SPI1, 2, or 3.
//
// Returns:
//      None.
//
//-----------------------------------------------------------------------------
void CspiIoPark(unsigned int index)
{
#ifdef __ASIC__
    switch(index)
    {
    case 1:
        // now we don't use SPI1
        break;

    case 2:
        /* IOCG configuration --
        iocg075:0x20041000+0x92C: SPI2_CLK管脚下拉，输入关闭
        iocg076:0x20041000+0x930: SPI2_DI管脚下拉，输入关闭
        iocg077:0x20041000+0x934: SPI2_DO管脚下拉，输入关闭
        iocg078:0x20041000+0x938: SPI2_CS0_N管脚下拉，输入关闭
        iocg079:0x20041000+0x93C: SPI2_CS1_N管脚下拉，输入关闭
        SPI2_CS2_N配置为高阻，不需要配置IOCG
        */
#ifdef _CPU_INT_LCD
        *(g_IomgcgRegsAddress + 9*4 ) = 0x2;
        *(g_IomgcgRegsAddress + 0x800 + 45*4 ) = 0x0;
#else

        *(g_IomgcgRegsAddress + 0x800 +75*4 ) = 0x1;
        *(g_IomgcgRegsAddress + 0x800 +76*4 )= 0x1;
        *(g_IomgcgRegsAddress + 0x800 +77*4 ) = 0x1;
        *(g_IomgcgRegsAddress + 0x800 +78*4 ) = 0x1;
        *(g_IomgcgRegsAddress + 0x800 +79*4 ) = 0x1;

        /* IOMG configuration --
        iomg018控制SPI2_CLK、SPI2_CS0_N、SPI2_DI、SPI2_DO管脚复用，相应配置值如下：
        iomg019控制SPI2_CS1_N管脚复用，相应配置值如下：
        */
        *(g_IomgcgRegsAddress + 18*4 ) = 0x1;
        *(g_IomgcgRegsAddress + 19*4 ) = 0x1;

        /* iomg009 has to be special deal with */
        /* SPI2_CS2_N 配置为高阻 */
        *(g_IomgcgRegsAddress + 9*4 ) = 0x2;
#endif
        break;

    case 3:
        break;

    default:
        break;
    }
#endif
}


//-----------------------------------------------------------------------------
//
// Function: CspiIoInit
//
//     IO configration when initliaztion
//
// Parameters:
//      index
//          [in] choose SPI1, 2, or 3.
//
// Returns:
//      None.
//
//-----------------------------------------------------------------------------
void CspiIoInit(unsigned int index, SPI_CS cs)
{
#ifdef __ASIC__
    unsigned int gpio_afsel;
    switch(index)
    {
    case 1:
        // now we don't use SPI1
        break;

    case 2:
#ifdef _CPU_INT_LCD
        *(g_IomgcgRegsAddress + 9*4 ) = 0x1;
        *(g_IomgcgRegsAddress + 0x800 + 45*4 ) = 0x0;
#else
        /* IOCG configuration --
        iocg075:0x20041000+0x92C=0x8:驱动能力4ma，SPI2_CLK管脚没有上下拉，输入关闭
        iocg076:0x20041000+0x930=0xd:驱动能力4ma，SPI2_DI管脚下拉，输入开启
        iocg077:0x20041000+0x934=0x8:驱动能力4ma，SPI2_DO管脚没有上下拉，输入关闭
        iocg078:0x20041000+0x938=0x8:驱动能力4ma，SPI2_CS0_N管脚没有上下拉，输入关闭
        iocg079:0x20041000+0x93C=0x8:驱动能力4ma，SPI2_CS1_N管脚没有上下拉，输入关闭
        */

        *(g_IomgcgRegsAddress + 0x800 +75*4 ) = 0x0;
        *(g_IomgcgRegsAddress + 0x800 +76*4 ) = 0x5;
        *(g_IomgcgRegsAddress + 0x800 +77*4 ) = 0x0;
        *(g_IomgcgRegsAddress + 0x800 +78*4 ) = 0x0;
        *(g_IomgcgRegsAddress + 0x800 +79*4 ) = 0x0;

        /* iocg045 has to be special deal with */
        *(g_IomgcgRegsAddress + 0x800 +45*4 ) = 0x0;

        /* IOMG configuration --
        iomg018控制SPI2_CLK、SPI2_CS0_N、SPI2_DI、SPI2_DO管脚复用，相应配置值如下：
        iomg018：0x20041000+0x048=0x0；
        iomg019控制SPI2_CS1_N管脚复用，相应配置值如下：
        iomg019：0x20041000+0x04C=0x0 
        */
        
        if(cs == CS1)
        {
            *(g_IomgcgRegsAddress + 18*4 ) = 0x0;
            
            //set all gpio7-2/3/4 as software mode, spi in/out/clk is spi mode because iocg18=0            
            gpio_afsel = *(g_GPIO7RegsAddress + 0x420 );
            gpio_afsel &=0xe3;
            *(g_GPIO7RegsAddress + 0x420 ) = gpio_afsel;
            
            
        }
        else if(cs == CS0)
        {
            //cs0 used as gpio7-5
            *(g_IomgcgRegsAddress + 18*4 ) = 0x1;            
            
            //set all gpio7-2/3/4 as hardware mode, spi in/out/clk is spi mode because iocg18=1
            gpio_afsel = *(g_GPIO7RegsAddress + 0x420 );
            gpio_afsel |=0x1c;
            *(g_GPIO7RegsAddress + 0x420 ) = gpio_afsel;
        }
        
        *(g_IomgcgRegsAddress + 19*4 ) = 0x0;

        /* iomg009 has to be special deal with */
        *(g_IomgcgRegsAddress + 9*4 ) = 0x0;
#endif

        break;

    case 3:
        /* the pins of SPI3 are not multiplexed，no need to config */
        break;

    default:
        break;
    }
#endif
}

//对系统外设时钟寄存器的相应位置上置位使能或禁止时钟
BOOL CspiClkControl(BOOL b)
{
#ifdef __ASIC__
    int     circle=0;

    if(b)
    {
        /* make sure the clock enable is success */
        *(M_SYSCTRL_SCPEREN)=(SCPER_SPI2);

        while(!(*(M_SYSCTRL_SCPERSTAT) & (SCPER_SPI2)) )
        {
            circle++;
            if(circle>1000)
            {
                hispi2_trace(3,"Enable SPI2 Clock timeout.\n\r");
                return FALSE;
            }
        }

    }
    else
    {
        //disable clock
        *(M_SYSCTRL_SCPERDIS)=(SCPER_SPI2);

        while(*(M_SYSCTRL_SCPERSTAT) & (SCPER_SPI2) )
        {
            circle++;
            if(circle>1000)
            {
                hispi2_trace(3,"Error on disable SPI2 Clk.\n\r");
                return FALSE;
            }
        }
    }
#endif

    return TRUE;
}



static void cspiDMAReadISR(void)
{
	//wake_up_interruptible(&m_RcvDMACfg.isrEvent);
}


static void cspiDMAWriteISR(void)
{
	//do nothing
	//wake_up_interruptible(&m_XmitDMACfg.isrEvent);
}



int CspiDMARead(PCSPI_XCH_PKT_T pXchPkt)
{
    unsigned int retValue=-1;

#if 0
#ifdef __ASIC__
    //TODO:Require Count is 256 bytes or times of 256 bytes

    PCSPI_BUSCONFIG_T pBusCnfg =NULL ;
    void * pTxBuf = NULL;
    void * pRxBuf = NULL;



    if(NULL==pXchPkt)
    {
        hispi2_trace(3,"Input pXchPkt is NULL.\r\n");
        //SetLastError(ERR_DRV_SPI_INVALID_PARA);
        return -1;
    }

    if (0 == pXchPkt->xchCnt)
    {
        hispi2_trace(3,"Input xchCnt is 0.\r\n");
        //SetLastError(ERR_DRV_SPI_INVALID_PARA);
        return -1;
    }
    else if(pXchPkt->xchCnt > SPI_MAX_PACKET_LEN)
    {
	 hispi2_trace(HISPI2_TRACE_LEVEL,"Operation length must not beyond %d.\r\n",SPI_MAX_PACKET_LEN);
        //SetLastError(ERR_DRV_SPI_INVALID_PARA);
        return -1;
    }

    // now we can dereference the mapped pointer and map other caller pointers
    //pBusCnfg = (PCSPI_BUSCONFIG_T) MapCallerPtr(pXchPkt->pBusCnfg,sizeof(CSPI_BUSCONFIG_T));
    pBusCnfg = pXchPkt->pBusCnfg;
    //pTxBuf = (LPVOID) MapCallerPtr(pXchPkt->pTxBuf,pXchPkt->xchCnt);
    pTxBuf = pXchPkt->pTxBuf;
    //pRxBuf = (LPVOID) MapCallerPtr(pXchPkt->pRxBuf,pXchPkt->xchCnt);
    pRxBuf = pXchPkt->pRxBuf;

    // check all translated pointers
    if (NULL == pBusCnfg )
    {
        hispi2_trace(3,"Input pBusCnfg is NULL.\r\n");
        //SetLastError(ERR_DRV_SPI_INVALID_PARA);
        return -1;
    }

    if (NULL==pRxBuf)
    {
        hispi2_trace(3,"Input pRxBuf is NULL.\r\n");
        //SetLastError(ERR_DRV_SPI_INVALID_PARA);
        return -1;
    }
  
    if( (pBusCnfg->sph != 1) && (pBusCnfg->sph !=0 ) )
    {
        // unsupported access width
        hispi2_trace(3,"unsupported SPH %d.\r\n",pBusCnfg->sph);
        //SetLastError(ERR_DRV_SPI_INVALID_PARA);
        return -1;
    }

    if( (pBusCnfg->spo != 1) && (pBusCnfg->spo !=0 ) )
    {
        // unsupported access width
        hispi2_trace(3,"unsupported SPO %d.\r\n",pBusCnfg->spo);
        //SetLastError(ERR_DRV_SPI_INVALID_PARA);
        return -1;
    }

    mutex_lock(&m_cspiCs);


    /*如果有定时器挂着就删除掉*/
    if(timer_pending(&timer))
    {
		del_timer(&timer);
		CspiIoInit(2, pXchPkt->spiDevice);
    }
    else
    {
     		CspiIoInit(2, pXchPkt->spiDevice);
		CspiClkControl(1);
		CspiReset(2);
     }
	

    // set client CSPI bus configuration based
    //  default LBM = DISABLE
    //  default SSE = DISABLE
    //  default MS = MASTER
    //  default SOD = DISABLE
    
    OUTREG32(&g_pSpiReg->SSPCR1,
             CSP_BITFVAL(CSPI_SSPCR1_LBM, CSPI_SSPCR1_LBM_DISABLE) |
             CSP_BITFVAL(CSPI_SSPCR1_SSE, CSPI_SSPCR1_SSE_DISABLE) |
             CSP_BITFVAL(CSPI_SSPCR1_MS, CSPI_SSPCR1_MS_MASTER) |
             CSP_BITFVAL(CSPI_SSPCR1_SOD, CSPI_SSPCR1_SOD_DISABLE));


    OUTREG32(&g_pSpiReg->SSPCR0,
             CSP_BITFVAL(CSPI_SSPCR0_FRF, CSPI_SSPCR0_FRF_SPI) |
             CSP_BITFVAL(CSPI_SSPCR0_SPO, pBusCnfg->spo) |
             CSP_BITFVAL(CSPI_SSPCR0_SPH, pBusCnfg->sph) |
             CSP_BITFVAL(CSPI_SSPCR0_DSS, pBusCnfg->bitcount) |
             CSP_BITFVAL(CSPI_SSPCR0_SCR, pBusCnfg->scr));


    switch(pXchPkt->spiDevice)
    {
    case CS1:
        {
            ChipSelectCS1();
            hispi2_trace(3,"Cspi select CS1 succeed!\r\n");
            break;
        }
    case CS2:
        {
            ChipSelectCS2();
            hispi2_trace(3, "Cspi select CS2 succeed!\r\n");
            break;
        }
    case CS0:
        {
            ChipSelectCS0();
            hispi2_trace(3,"Cspi select CS0 succeed!\r\n");
            break;
        }
    case NO_DEVICE:
    default:
        {
            //SetLastError(ERR_DRV_SPI_INVALID_DEV);
            hispi2_trace(3,"Error CS enum value %d!\r\n",pXchPkt->spiDevice);
            goto DMAREADLEAVE;
        }
    }

    //屏蔽中断
    OUTREG32(&g_pSpiReg->SSPIMSC, 0);
    //设置分频比
    OUTREG32(&g_pSpiReg->SSPCPSR,
             CSP_BITFVAL(CSPI_SSPCPSR_CPSDVSR, pBusCnfg->cpsdvsr));
    //使能dma 接收发送寄存器
    OUTREG32(&g_pSpiReg->SSPDMACR,
             CSP_BITFVAL(CSPI_SSPDMACR_RXDMAE, 0x03));  //enable recv and xmit DMA

    //Config Xmit DMA

    if(NULL==pTxBuf)
    {
        memset((void*)(m_XmitDMACfg.dma_virt),0,m_XmitDMACfg.size);        
    }
    else
    {
        memcpy((void *)(m_XmitDMACfg.dma_virt),(void *)pTxBuf,pXchPkt->xchCnt);
    }

    //Config Xmit DMA
    m_XmitDMACfg.node.size = pXchPkt->xchCnt;
    m_XmitDMACfg.node.phys = m_XmitDMACfg.dma_phys;

    m_XmitDMACfg.sg.scattercnt = 1;
    m_XmitDMACfg.sg.transtype = DMAC_MEM_TO_DEVICE;
    m_XmitDMACfg.sg.pnode = &m_XmitDMACfg.node;
    
    //Config Recv DMA
    memset((void*)(m_RcvDMACfg.dma_virt), 0, m_RcvDMACfg.size);
    m_RcvDMACfg.node.size = pXchPkt->xchCnt;
    m_RcvDMACfg.node.phys = m_RcvDMACfg.dma_phys;

    m_RcvDMACfg.sg.scattercnt = 1;
    m_RcvDMACfg.sg.transtype = DMAC_DEVICE_TO_MEM;
    m_RcvDMACfg.sg.pnode = &m_RcvDMACfg.node;

    //Start DMA transfer
    if(DMAC_STATUS_SUCCESS != dmac_channel_start(m_RcvDMACfg.channel, &m_RcvDMACfg.sg))
    {        
        hispi2_trace(3,"Recv DMA_IOCTL_TRANSFER error.\r\n");
        goto DMAREADLEAVE;
    }

    if(DMAC_STATUS_SUCCESS != dmac_channel_start(m_XmitDMACfg.channel, &m_XmitDMACfg.sg))
    {
        hispi2_trace(3,"Xmit DMA_IOCTL_TRANSFER error.\r\n");
        goto DMAREADLEAVE;
    }

    //Start SPI2 Transfer
    	 hispi2_trace(3,"Enable SPI2 to start DMA tx and rx.\n");
    INSREG32(&g_pSpiReg->SSPCR1, CSP_BITFMASK(CSPI_SSPCR1_SSE),
             CSP_BITFVAL(CSPI_SSPCR1_SSE, CSPI_SSPCR1_SSE_ENABLE));
   

    //Wait DMA transfer end
    interruptible_sleep_on(&m_RcvDMACfg.isrEvent);
  
    hispi2_trace(3, "Recv interruptible_sleep_on out");
    
    //if pBuffer is NULL, means Data is Copy to DMAbuffer already
    //Config SPI2 register
    if(NULL!=pRxBuf)
    {
        //Copy Data to DMA Buffer
        memcpy((void *)pRxBuf, (void *)m_RcvDMACfg.dma_virt, pXchPkt->xchCnt);
    }

    retValue = 0;  //操作成功


DMAREADLEAVE:
    //CspiClkControl(FALSE);
    mutex_unlock(&m_cspiCs);

    //启动定时器
   mod_timer(&timer, jiffies + detect_time);
#endif
#endif
	

    return  retValue;
}

int CspiDMAWrite(PCSPI_XCH_PKT_T pXchPkt)
{
    unsigned int retValue=-1;
 
#if 0
#ifdef __ASIC__
    //TODO:Require Count is 256 bytes or times of 256 bytes

    PCSPI_BUSCONFIG_T pBusCnfg =NULL ;
    void * pTxBuf = NULL;
    void * pRxBuf = NULL;


	
    if(NULL==pXchPkt)
    {
        hispi2_trace(3,"Input pXchPkt is NULL.\r\n");
        //SetLastError(ERR_DRV_SPI_INVALID_PARA);
        return -1;
    }

    if (0 == pXchPkt->xchCnt)
    {
        hispi2_trace(3,"Input xchCnt is 0.\r\n");
        //SetLastError(ERR_DRV_SPI_INVALID_PARA);
        return -1;
    }
    else if(pXchPkt->xchCnt > SPI_MAX_PACKET_LEN)
    {
        hispi2_trace(HISPI2_TRACE_LEVEL,"Operation length must not beyond %d.\r\n",SPI_MAX_PACKET_LEN);
        //SetLastError(ERR_DRV_SPI_INVALID_PARA);
        return -1;
    }

    // now we can dereference the mapped pointer and map other caller pointers
    //pBusCnfg = (PCSPI_BUSCONFIG_T) MapCallerPtr(pXchPkt->pBusCnfg,sizeof(CSPI_BUSCONFIG_T));
    pBusCnfg = pXchPkt->pBusCnfg;
    //pTxBuf = (LPVOID) MapCallerPtr(pXchPkt->pTxBuf,pXchPkt->xchCnt);
    pTxBuf = pXchPkt->pTxBuf;
    //pRxBuf = (LPVOID) MapCallerPtr(pXchPkt->pRxBuf,pXchPkt->xchCnt);
    pRxBuf = pXchPkt->pRxBuf;

    // check all translated pointers
    if (NULL == pBusCnfg )
    {
        hispi2_trace(3,"Input pBusCnfg is NULL.\r\n");
        //SetLastError(ERR_DRV_SPI_INVALID_PARA);
        return -1;
    }

    if ( NULL == pTxBuf )
    {
        hispi2_trace(3,"Input pTxBuf or pRxBuf is NULL.\r\n");
        //SetLastError(ERR_DRV_SPI_INVALID_PARA);
        return -1;
    }

    if( (pBusCnfg->sph != 1) && (pBusCnfg->sph !=0 ) )
    {
        // unsupported access width
        hispi2_trace(3,"unsupported SPH %d.\r\n",pBusCnfg->sph);
        //SetLastError(ERR_DRV_SPI_INVALID_PARA);
        return -1;
    }

    if( (pBusCnfg->spo != 1) && (pBusCnfg->spo !=0 ) )
    {
        // unsupported access width
        hispi2_trace(3,"unsupported SPO %d.\r\n",pBusCnfg->spo);
        //SetLastError(ERR_DRV_SPI_INVALID_PARA);
        return -1;
    }

    mutex_lock(&m_cspiCs);


    //Copy Data to DMA Buffer
    memcpy((void *)(m_XmitDMACfg.dma_virt),(void *)pTxBuf,pXchPkt->xchCnt);


    //ResetEvent(m_Spi2DmaRcvEvent);
    //ResetEvent(m_Spi2DmaXmitEvent);

    //查询定时器状态
    if(timer_pending(&timer))
    {
	del_timer(&timer);
	CspiIoInit(2, pXchPkt->spiDevice);
    }
    else
    {
     		CspiIoInit(2, pXchPkt->spiDevice);
		CspiClkControl(TRUE);
		CspiReset(2);
     }


    // set client CSPI bus configuration based
    //  default LBM = DISABLE
    //  default SSE = DISABLE
    //  default MS = MASTER
    //  default SOD = DISABLE
    OUTREG32(&g_pSpiReg->SSPCR1,
             CSP_BITFVAL(CSPI_SSPCR1_LBM, CSPI_SSPCR1_LBM_DISABLE) |
             CSP_BITFVAL(CSPI_SSPCR1_SSE, CSPI_SSPCR1_SSE_DISABLE) |
             CSP_BITFVAL(CSPI_SSPCR1_MS, CSPI_SSPCR1_MS_MASTER) |
             CSP_BITFVAL(CSPI_SSPCR1_SOD, CSPI_SSPCR1_SOD_DISABLE));


    OUTREG32(&g_pSpiReg->SSPCR0,
             CSP_BITFVAL(CSPI_SSPCR0_FRF, CSPI_SSPCR0_FRF_SPI) |
             CSP_BITFVAL(CSPI_SSPCR0_SPO, pBusCnfg->spo) |
             CSP_BITFVAL(CSPI_SSPCR0_SPH, pBusCnfg->sph) |
             CSP_BITFVAL(CSPI_SSPCR0_DSS, pBusCnfg->bitcount) |
             CSP_BITFVAL(CSPI_SSPCR0_SCR, pBusCnfg->scr));


    switch(pXchPkt->spiDevice)
    {
    case CS1:
        {
            ChipSelectCS1();
            hispi2_trace(3,"Cspi select CS1 succeed!\r\n");
            break;
        }
    case CS2:
        {
            ChipSelectCS2();
            hispi2_trace(3, "Cspi select CS2 succeed!\r\n");
            break;
        }
    case CS0:
        {
            ChipSelectCS0();
            hispi2_trace(3,"Cspi select CS0 succeed!\r\n");
            break;
        }
    case NO_DEVICE:
    default:
        {
            //SetLastError(ERR_DRV_SPI_INVALID_DEV);
            hispi2_trace(3,"Error CS enum value %d!\r\n",pXchPkt->spiDevice);
            goto DMAWRITELEAVE;
        }
    }

    OUTREG32(&g_pSpiReg->SSPIMSC, 0);

    OUTREG32(&g_pSpiReg->SSPCPSR,
             CSP_BITFVAL(CSPI_SSPCPSR_CPSDVSR, pBusCnfg->cpsdvsr));


    OUTREG32(&g_pSpiReg->SSPDMACR,
             CSP_BITFVAL(CSPI_SSPDMACR_RXDMAE, 0x03));  //enable recv and xmit DMA

    //Config Xmit DMA
    m_XmitDMACfg.node.size = pXchPkt->xchCnt;
    m_XmitDMACfg.node.phys = m_XmitDMACfg.dma_phys;

    m_XmitDMACfg.sg.scattercnt = 1;
    m_XmitDMACfg.sg.transtype = DMAC_MEM_TO_DEVICE;
    m_XmitDMACfg.sg.pnode = &m_XmitDMACfg.node;
    
    //Config Recv DMA
    memset((void*)(m_RcvDMACfg.dma_virt), 0, m_RcvDMACfg.size);
    m_RcvDMACfg.node.size = pXchPkt->xchCnt;
    m_RcvDMACfg.node.phys = m_RcvDMACfg.dma_phys;

    m_RcvDMACfg.sg.scattercnt = 1;
    m_RcvDMACfg.sg.transtype = DMAC_DEVICE_TO_MEM;
    m_RcvDMACfg.sg.pnode = &m_RcvDMACfg.node;

    //Start DMA transfer
    if(DMAC_STATUS_SUCCESS != dmac_channel_start(m_RcvDMACfg.channel, &m_RcvDMACfg.sg))
    {        
        hispi2_trace(3,"Recv DMA_IOCTL_TRANSFER error.\r\n");
        goto DMAWRITELEAVE;
    }

    if(DMAC_STATUS_SUCCESS != dmac_channel_start(m_XmitDMACfg.channel, &m_XmitDMACfg.sg))
    {
        hispi2_trace(3,"Xmit DMA_IOCTL_TRANSFER error.\r\n");
        goto DMAWRITELEAVE;
    }

    //Start SPI2 Transfer
    INSREG32(&g_pSpiReg->SSPCR1, CSP_BITFMASK(CSPI_SSPCR1_SSE),
             CSP_BITFVAL(CSPI_SSPCR1_SSE, CSPI_SSPCR1_SSE_ENABLE));
    

    //Wait DMA transfer end    
    interruptible_sleep_on(&m_RcvDMACfg.isrEvent);
    hispi2_trace(3, "Recv interruptible_sleep_on out");
    
    //if pBuffer is NULL, means Data is Copy to DMAbuffer already
    //Config SPI2 register
    if(NULL!=pRxBuf)
    {
        //Copy Data to DMA Buffer
        memcpy((void *)pRxBuf, (void *)m_RcvDMACfg.dma_virt, pXchPkt->xchCnt);
    }

    retValue = 0;

DMAWRITELEAVE:
  //  CspiClkControl(FALSE);
    mutex_unlock(&m_cspiCs);

    //启动定时器
    mod_timer(&timer, jiffies + detect_time);
#endif
#endif

	
    return  retValue;
}

BOOL CspiDmaInit(void)
{
#if 0
#ifdef __ASIC__
    unsigned int page_num;
    page_num = (SPI_MAX_PACKET_LEN/PAGE_SIZE)+1;
	
    //receive dma config
    init_waitqueue_head(&m_RcvDMACfg.isrEvent);
    
    //send dma config
    init_waitqueue_head(&m_XmitDMACfg.isrEvent);
    
    //alloc dma receive channel  
    m_RcvDMACfg.channel = dmac_channel_allocate(DMAC_ID_SPI2_RX, cspiDMAReadISR);
    m_XmitDMACfg.channel = dmac_channel_allocate(DMAC_ID_SPI2_TX, cspiDMAWriteISR);

    //alloc 2 buffers
    m_RcvDMACfg.size = page_num*PAGE_SIZE;
    m_XmitDMACfg.size = page_num*PAGE_SIZE;
    
    m_RcvDMACfg.dma_virt = dma_alloc_coherent(NULL, m_RcvDMACfg.size, &m_RcvDMACfg.dma_phys, GFP_DMA|__GFP_WAIT);
    if (NULL == m_RcvDMACfg.dma_virt){
        hispi2_trace(3,"can't get receive mem from system\n");
        return FALSE;
    }
    else
    {
         hispi2_trace(3,"virt 0x%x, phys 0x%x\n", m_RcvDMACfg.dma_virt, m_RcvDMACfg.dma_phys);
    }
    
    m_XmitDMACfg.dma_virt = dma_alloc_coherent(NULL, m_XmitDMACfg.size, &m_XmitDMACfg.dma_phys, GFP_DMA|__GFP_WAIT);
    if (NULL == m_XmitDMACfg.dma_virt){
        hispi2_trace(3,"can't get transmit mem from system\n");
        return FALSE;
    }
    else
    {
         hispi2_trace(3,"virt 0x%x, phys 0x%x\n", m_XmitDMACfg.dma_virt, m_XmitDMACfg.dma_phys);
    }
#endif
#endif
    
    return TRUE;
}


BOOL ChipSelectCS0(void)
{
#ifdef __ASIC__
    volatile unsigned int oldReg, newReg;
    //int                 wait;

    do
    {
        oldReg = *(unsigned int *)(M_SYSCTRL_SCPERCTRT5);		  // 读取寄存器值
        newReg = ( oldReg & (~(0x00001C00) ) ) | 0x0000400 ;  // 修改为需要写入的值
        *(unsigned int *)(M_SYSCTRL_SCPERCTRT5) = newReg;
    }while(0);
    //while (InterlockedTestExchange((LPLONG)(M_SYSCTRL_SCPERCTRT5), (LONG)oldReg, (LONG)newReg) != (LONG)oldReg);	  
    // 该函数保证在写入新值时，原始的值是否被修改，如果被修改则返回FALSE。
#endif
    return TRUE;

}

BOOL ChipSelectCS1(void)
{
#ifdef __ASIC__
    volatile unsigned int oldReg, newReg;
    //int                 wait;

    do
    {
        oldReg = *(unsigned int *)(M_SYSCTRL_SCPERCTRT5);		  // 读取寄存器值
        newReg = ( oldReg & (~(0x00001C00) ) ) | 0x00000800 ;  // 修改为需要写入的值
        *(unsigned int *)(M_SYSCTRL_SCPERCTRT5) = newReg;
    }while(0);
    //while (InterlockedTestExchange((LPLONG)(M_SYSCTRL_SCPERCTRT5), (LONG)oldReg, (LONG)newReg) != (LONG)oldReg);	  
    // 该函数保证在写入新值时，原始的值是否被修改，如果被修改则返回FALSE。
#endif
    return TRUE;
}

BOOL ChipSelectCS2(void)
{
#ifdef __ASIC__
    volatile unsigned int oldReg, newReg;
    //int                 wait;

    do
    {
        oldReg = *(unsigned int *)(M_SYSCTRL_SCPERCTRT5);		  // 读取寄存器值
        newReg = ( oldReg & (~(0x00001C00) ) ) | 0x00001000 ;  // 修改为需要写入的值
        *(unsigned int *)(M_SYSCTRL_SCPERCTRT5) = newReg;
    }while(0);
    //while (InterlockedTestExchange((LPLONG)(M_SYSCTRL_SCPERCTRT5), (LONG)oldReg, (LONG)newReg) != (LONG)oldReg);	  
    // 该函数保证在写入新值时，原始的值是否被修改，如果被修改则返回FALSE。
#endif
    return TRUE;
}



static int __init hispi2_module_init(void)
{		
	CspiInitialize();
	printk("hisilicon spi2 driver init\n");
	return 0;
}

static void __exit hispi2_module_exit(void)
{
    CspiRelease();
}

EXPORT_SYMBOL(CspiDataExchange);
EXPORT_SYMBOL(CspiDMARead);
EXPORT_SYMBOL(CspiDMAWrite);

arch_initcall(hispi2_module_init);
//module_init(hispi2_module_init);
//module_exit(hispi2_module_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("hi_driver_group");

