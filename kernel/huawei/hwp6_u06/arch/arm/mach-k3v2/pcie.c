#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/mbus.h>
#include <asm/irq.h>
#include <asm/mach/pci.h>
#include <asm/delay.h>
#include <linux/spinlock.h>
#include <asm/signal.h>
#include <mach/platform.h>
#include <mach/pcie.h>
#include <linux/clk.h>

static struct pcie_info pcie_info[1];
static unsigned int pcie0_sel = pcie0_x1_sel;	/*default controller is pcie controller 0 and X1 mode */
static unsigned int pcie0_work_mode = pcie_wm_rc; /*default work mode is root complex.*/
//static unsigned int pcie0_mem_space_size = 0x7E00000;   /*default mem space size: 128M nearly */
static unsigned int pcie0_mem_space_size = 0x4000000;  

/*pcie0 iatu table*/
struct pcie_iatu pcie0_iatu_table[3] = {
	/*config*/
	{
		.viewport	= 0,
		.region_ctrl_1	= 0x00000004,
		.region_ctrl_2	= 0x80000000,
		.lbar		= PCIE0_PHY_ADDR_PHYS,
		.ubar		= 0x0,
		.lar		= 0xF0000FFF,
		.ltar		= 0x00000000,
		.utar		= 0x00000000,
	},
	/*IO*/
	{
		.viewport	= 1,
		.region_ctrl_1	= 0x00000002,
		.region_ctrl_2	= 0x80000000,
		.lbar		= PCIE0_IO_SPACE_START,
		.ubar		= 0x0,
		.lar		= 0xF0001FFF,
		.ltar		= 0x00001000,
		.utar		= 0x00000000,
	},
	/*Memory*/
	{
		.viewport	= 2,
		.region_ctrl_1	= 0x00000000,
		.region_ctrl_2	= 0x80000000,
		.lbar		= PCIE0_BASE_ADDR_PHYS,
		.ubar		= 0x0,
		.lar		= 0xF01FFFFF,
		.ltar		= 0xF0100000,
		.utar		= 0x00000000,
	},
};

static void __init hisik3_pcie_preinit(void);
static int __init hisik3_pcie_setup(int nr, struct pci_sys_data* sys);
static struct pci_bus *__init hisik3_pcie_scan_bus(int nr, struct pci_sys_data* sys);
static int __init hisik3_pcie_map_irq(struct pci_dev* dev, u8 slot, u8 pin);

//static struct hw_pci hisik3_pcie __initdata = {
static struct hw_pci hisik3_pcie = {	
	.nr_controllers = 0,        /*default 1 controllers*/
	.preinit            = hisik3_pcie_preinit,
	.swizzle           = pci_std_swizzle,	/*useless??*/
	.setup             = hisik3_pcie_setup,
	.scan               = hisik3_pcie_scan_bus,
	.map_irq          = hisik3_pcie_map_irq,
};

volatile unsigned int  pcie_errorvalue=0;

static int hisik3_pci_fault(unsigned long addr, unsigned int fsr, struct pt_regs *regs)
{
	pcie_errorvalue=1;
	return 0;
}

/*locks: read lock & write lock.*/
static spinlock_t rlock = SPIN_LOCK_UNLOCKED;
static spinlock_t wlock = SPIN_LOCK_UNLOCKED;
/*format : pcie0_mem=0xa00000;pcie1_mem=0xa00000*/
static int __init pcie0_mem_parser(char* str){
	unsigned int size;
	size = (unsigned int)simple_strtoul(str, NULL, 16);
	/*if size >= 128M, set default 128M*/
	if(size >= 0x7E00000)
		size = 0x7E00000;
	pcie0_mem_space_size = size;
	pcie0_iatu_table[2].lbar = 0xF0100000 + pcie0_mem_space_size;
	return 1;	
}
__setup("pcie0_mem=", pcie0_mem_parser);
/*pcie sel boot args format: pcie0_sel=x1 pcie1=x1 or pcie1=x2. 
  any other value after "pcieX_sel=" prefix  will be treated as none controller will be selected.
  e.g. pcie0_sel=none will be treated as you will not selected pcie0 
 */
static int __init pcie0_sel_parser(char* str){
	if(strncasecmp(str, "x1",2) == 0)
		pcie0_sel = pcie0_x1_sel;
	else
		pcie0_sel = pcie_sel_none;

	return 1;
}
__setup("pcie0_sel=", pcie0_sel_parser);

/*work mode format: pcie0_wm=rc/ep/lep pcie1_wm=rc/ep/lep*/
static int __init pcie0_work_mode_parser(char* str){
	if(strncasecmp(str, "rc", 2) == 0)
		pcie0_work_mode = pcie_wm_rc;
	else if(strncasecmp(str, "ep", 2) == 0)
		pcie0_work_mode = pcie_wm_ep;
	else if(strncasecmp(str, "lep", 3) == 0)
		pcie0_work_mode = pcie_wm_lep;
	else 
		pcie_error("Invalid pcie_wm value for pcie0!\n");
	return 1;
}
__setup("pcie0_wm=", pcie0_work_mode_parser);

static int __init pcie_sw_init(void){
	unsigned int val;
	if(pcie0_sel == pcie_sel_none){
		pcie_error("Pcie0 not enabled!\n");
		return -EIO;
	}
	
	if(hisik3_pcie.nr_controllers){
		pcie_error("inavild param!\n");
		return -EIO;
	}
	
	pcie_info[0].root_bus_nr = -1;
	
	/*if enabled pcie0*/
	if(pcie0_sel == pcie0_x1_sel){
		/*work at rc mode*/
		if(pcie0_work_mode == pcie_wm_rc){
			pcie_info[hisik3_pcie.nr_controllers].controller = pcie_controller_0;
			pcie_info[hisik3_pcie.nr_controllers].conf_base_addr = (unsigned long)IO_ADDRESS(REG_BASE_PCIE_CFG);
			pcie_info[hisik3_pcie.nr_controllers].base_addr =  (unsigned long)IO_ADDRESS(REG_BASE_PCIE);
			printk(KERN_ALERT "PCI base addr 0x%x, cfg addr 0x%x\n",pcie_info[hisik3_pcie.nr_controllers].base_addr, pcie_info[hisik3_pcie.nr_controllers].conf_base_addr); 

			hisik3_pcie.nr_controllers++;
		}
		/*work at ep mode*/
		else{
			/*FIXME:
			  This is roughly initializtion of ep mode.
			  Maybe I should to create a new func to initialize ep  mode!
			  TODO: Do reset and enable controllers after set corresponding work mode.
			 */
			printk(KERN_ERR "ERR:we just support RC mode currently.\n"); 
			val = readl(IO_ADDRESS(REG_BASE_PCTRL) + PERI_PCIE2);
			val &= (~(0xf << pcie0_slv_device_type));
			writel(val, IO_ADDRESS(REG_BASE_PCTRL) + PERI_PCIE2);
		}
	}
	/*pcie controllers work at non-rc mode or is disabled, we do nothing*/
	if(!hisik3_pcie.nr_controllers){
		pcie_error("None of the pcie controllers is enabled!");
		return -EIO;
	}
	return 0;
}

#define PCIE0_MODE_SEL  (1 << 0)
#define PCIE1_MODE_SEL  (1 << 1)
static int __init pcie_sys_init(unsigned int mode_sel){
	unsigned int val;
	int i;
	unsigned long sctrl_addr = (unsigned long)IO_ADDRESS(REG_BASE_SCTRL);
	unsigned long pctrl_addr = (unsigned long)IO_ADDRESS(REG_BASE_PCTRL);
	unsigned long pcie_cfg_addr = (unsigned long)IO_ADDRESS(REG_BASE_PCIE_CFG);
	//struct clk          *pcie_clk;   //Need be support k3v2_clock.c
	
	/*setup pcie ip, need spinlock??*/
	
	if((!sctrl_addr)||(!pctrl_addr)||(!pcie_cfg_addr))
	{
		pcie_error("Cannot map physical base for pcie");
		return -EIO;
	}

	/*Software shoule be driver ltssm signal to low firstly.*/
	val = readl(pctrl_addr + PERI_PCIE7);
	val &= (~(1 << pcie0_app_ltssm_enable));
	writel(val, pctrl_addr + PERI_PCIE7);
	
	/*setup phy*/	
	/*mpll_ncy:0x05, mpll_ncys:0x01, mpll_prescale:0x2, los_lvl:5b10100, use refclk*/
	writel(0x05605000, pctrl_addr + PERI_PCIE8);
	/*rx_eq_val_0:0x2, tx_boost_0:0xA, tx_lvl:0x06*/
	writel(0x20050006, pctrl_addr + PERI_PCIE9);
#if 1  
	/*enable pcie clock*/
	val = readl(sctrl_addr  + SCPEREN3);
	val |= (0x1 << pcie_clk_req);
	writel(val, sctrl_addr + SCPEREN3);
#else if   //Need be support k3v2_clock.c
	pcie_clk = clk_get(NULL, "clk_pcie");
	if (IS_ERR(pcie_clk)) {
		pcie_error("Failed to enable pcie clock");
		return -EIO;
	}
	clk_enable(pcie_clk);
#endif
	udelay(100);
	
	/*reset pcie*/
	val = readl(sctrl_addr  + SCPERRSTEN3);
	val |= (0x1 << pcie0_rst_req);
	writel(val, sctrl_addr + SCPERRSTEN3);
	udelay(100);

	/*undo reset pcie*/
	val = readl(sctrl_addr  + SCPERRSTDIS3);
	val |= (0x1 << pcie0_rst_req);
	writel(val, sctrl_addr + SCPERRSTDIS3);
	udelay(1000);

	if(mode_sel & PCIE0_MODE_SEL){
		int i;

		/*set work mode: defualt is Root Port of PCI Express Root Complex.*/
		val = readl(pctrl_addr + PERI_PCIE0);
		val &=~(0xf << pcie0_slv_device_type);
		val |= (pcie0_work_mode << pcie0_slv_device_type);
		writel(val, pctrl_addr + PERI_PCIE0);
		
		/*setup correct classe code : PCI_CLASS_BRIDGE_PCI*/
		val = readl(pcie_cfg_addr + 0x8);
		val &= ~(0xffffff00);
		val |= (0x60400<<8);
		writel(val, pcie_cfg_addr + 0x8);
		
		/*Dll link enable, set fast link,and set Link mode is X1*/
		val = readl(pcie_cfg_addr + 0x710);
		val |= 0x100a0;
		writel(val, pcie_cfg_addr + 0x710);

		/*Enter ASPM L1 without receive in L0s */
		val = readl(pcie_cfg_addr + 0x70c);
		val |= 0x40000000;
		writel(val, pcie_cfg_addr + 0x70c);    
		
		/*set PM reg*/
		val = readl(pcie_cfg_addr + 0x40);
		val |= 0xfec35001;
		writel(val, pcie_cfg_addr + 0x40); 

		/*PME enable*/
		val = readl(pcie_cfg_addr + 0x44);
		val |= 0x100;
		writel(val, pcie_cfg_addr + 0x44);
		val = readl(pcie_cfg_addr + 0x44);
		
		/*set CAP reg*/
		val = readl(pcie_cfg_addr + 0x78);
		val |= 0x2430;
		writel(val, pcie_cfg_addr + 0x78); 

		val = readl(pcie_cfg_addr + 0x7c);
		val |= 0xc00;
		writel(val, pcie_cfg_addr + 0x7c); 

		val = readl(pcie_cfg_addr + 0x80);
		val |= 0x3;
		writel(val, pcie_cfg_addr + 0x80); 

		/*set hotplug capability*/
		//val = readl(dbi_base + 0x70);
		//val |= (1<<24);
		//writel(val, dbi_base + 0x70);
		/*llz for hotplug*/
	
		//writel(0xff | (1 << 18), dbi_base+0x84);
		//writel(0xff , dbi_base+0x84);

		//val = readl(dbi_base +0x88);
		//val |= 0x2f;
		//writel(val, dbi_base +0x88);	
				
		/*setup iatu table*/
		for( i = 0; i < ARRAY_SIZE(pcie0_iatu_table); i++){
		    writel(pcie0_iatu_table[i].viewport, pcie_cfg_addr + 0x700 + 0x200);
		    writel(pcie0_iatu_table[i].region_ctrl_1, pcie_cfg_addr + 0x700 + 0x204);
		    writel(pcie0_iatu_table[i].region_ctrl_2, pcie_cfg_addr + 0x700 + 0x208);
		    writel(pcie0_iatu_table[i].lbar, pcie_cfg_addr + 0x700 + 0x20c);
		    writel(pcie0_iatu_table[i].ubar, pcie_cfg_addr + 0x700 + 0x210);
		    writel(pcie0_iatu_table[i].lar, pcie_cfg_addr + 0x700 + 0x214);
		    writel(pcie0_iatu_table[i].ltar, pcie_cfg_addr + 0x700 + 0x218);
		    writel(pcie0_iatu_table[i].utar, pcie_cfg_addr + 0x700 + 0x21c);
		}
	}

	/*Software shoule be driver ltssm signal to high when all is ready. then enter loop detect status.*/
	if(mode_sel & PCIE0_MODE_SEL){
		val = readl(pctrl_addr + PERI_PCIE7);
		val |= 1 << pcie0_app_ltssm_enable;
		writel(val, pctrl_addr + PERI_PCIE7);
	}
	
    	for(i=0;i<TIME_TO_WAIT;i++)
	{
		udelay(100);
		if((1<<pcie_phy_link_up) & readl(pctrl_addr+PCIE_STAT0)) {
			printk(KERN_ALERT "PCI: pcie phy link up.\n");
			break;
		}
	}

	if(!((1<<pcie_phy_link_up) & readl(pctrl_addr+PCIE_STAT0)))
	{
		printk(KERN_ALERT "PCI: The pci device isn't online,Please check it.\n");
		return -EIO;
	}

	if(mode_sel & PCIE0_MODE_SEL){
		val = readl(pcie_cfg_addr + 0x4);
		val &= (~7);
		val |= 7;
		writel(val, pcie_cfg_addr + 0x4);
	}

	return 0;
}

static struct resource pcie0_mem_space = {
	.name   = "PCIE0 memory space",
	.start  = PCIE0_BASE_ADDR_PHYS,
	.end    = 0, 
	.flags  = IORESOURCE_MEM,
};
static struct resource pcie0_io_space = {
	.name   = "PCIE0 io space",
	.start  = 0,
	.end    = 0, 
	.flags  = IORESOURCE_IO,
};
static void __init hisik3_pcie_preinit(void){
	/*setup resource*/
	pcie0_mem_space.end = PCIE0_BASE_ADDR_PHYS + pcie0_mem_space_size - 1;
	pcie0_io_space.start = PCIE0_IO_SPACE_START; 
	pcie0_io_space.end = PCIE0_IO_SPACE_END; 
}

static int __init hisik3_pcie_setup(int nr, struct pci_sys_data* sys){
	struct pcie_info* info;
	int ret;

	if(nr >= hisik3_pcie.nr_controllers)	
		return 0;

	info = &pcie_info[nr];
	/*record busnr for cfg ops use*/
	info->root_bus_nr = sys->busnr;/*do i need to set busnr to pcie controller???,if so, how??*/
	sys->mem_offset = 0;   /*what's this? is't right?*/

	/*requeset resources for corresponding controller*/
	if(info->controller == pcie_controller_0){
		ret = request_resource(&ioport_resource, &pcie0_io_space);
		if(ret){
			pcie_error("Cannot request io resource for pcie0,pcie0_io_space.start=0x%x,end:0x%x\n",pcie0_io_space.start,pcie0_io_space.end);
			return ret;
		}
		ret = request_resource(&iomem_resource, &pcie0_mem_space);
		if(ret){
			pcie_error("Cannot request mem resource for pcie0");
			release_resource(&pcie0_io_space);
			return ret;
		}
		sys->resource[0] = &pcie0_io_space;
		sys->resource[1] = &pcie0_mem_space;
	}
	sys->resource[2] = NULL;

	return 1;
}
static struct pcie_info* bus_to_info(int busnr){
	int i = hisik3_pcie.nr_controllers, j=0;
	for( j=i-1 ; i > 0; i--,j=i-1 ){
		if(pcie_info[j].controller != pcie_controller_none 
				&& pcie_info[j].root_bus_nr <= busnr
				&& pcie_info[j].root_bus_nr != -1)
			return &pcie_info[j];
	}
	return NULL;
}
static int __init hisik3_pcie_map_irq(struct pci_dev* dev, u8 slot, u8 pin){
	struct pcie_info* info = bus_to_info(dev->bus->number);
	if(!info){
		pcie_error("Cannot find corresponding controller for appointed device!");
		BUG();
		return -1;
	}

	if(info->controller == pcie_controller_0){
		switch(pin){
			case PCIE_INTA_PIN: return PCIE0_IRQ_INTA;
			case PCIE_INTB_PIN: return PCIE0_IRQ_INTB;
			case PCIE_INTC_PIN: return PCIE0_IRQ_INTC;
			case PCIE_INTD_PIN: return PCIE0_IRQ_INTD;
			default :
				pcie_error("Unkown pin for mapping irq!");
				return -1;
		}
	}	
	pcie_error("Why I'm here??");
	BUG();
	return -1;
}
#define PCIE_CFG_BUS(busnr)	((busnr & 0xff) << 16)
#define PCIE_CFG_DEV(devfn)	((devfn & 0xff)  << 8)
#define PCIE_CFG_REG(reg)	((reg & 0xffc))/*set dword align*/

unsigned int HBA_CONFIG_ADDR;

static inline unsigned int to_pcie_address(struct pci_bus* bus,unsigned int devfn, int where){
	struct pcie_info* info = bus_to_info(bus->number);
	unsigned int address = 0;
	if(!info){
		pcie_error("Cannot find corresponding controller for appointed device!");
		BUG();
	}

	address = info->base_addr + PCIE_CFG_REG(where);
	HBA_CONFIG_ADDR = info->base_addr;
	return address;
}
static inline int pcie_check_link_status(void){
	int val;
	val = readl(IO_ADDRESS(REG_BASE_PCTRL) + PCIE_STAT0);
	val &= (1 << pcie_phy_link_up);
	return val == (1 << pcie_phy_link_up) ? 1 : 0;
}
static int pcie_read_from_device(struct pci_bus* bus, unsigned int devfn, int where, int size, u32* value){
	struct pcie_info* info = bus_to_info(bus->number);
	unsigned int val=0x00000000;
	unsigned int addr;
	unsigned long flag;
	int i;
	
	for (i = 0; i < 1000; i++){   //Wait for 1s
		if(pcie_check_link_status()){
			break;
		}
		udelay(1000);
	}

	if(i >= 1000){
		pcie_debug(PCIE_DEBUG_LEVEL_MODULE, "pcie%d not link up!",info->controller == pcie_controller_0 ? 0: 1);
		return -1;
	}
	spin_lock_irqsave(&rlock, flag);
	
	addr = to_pcie_address(bus, devfn, where);
	val = readl(addr);
	
	i=0;
#if 1
	while(i<20){
		__asm__ __volatile__("nop\n");
		i++;
	}
#endif

	if(pcie_errorvalue==1){
		pcie_errorvalue=0;
		val=0xffffffff;
	}

	/*if got data is dword align, and val got from offset 0, i need to calculate which byte is wanted*/
	/*is this right? need to check*/
	if(size == 1)
		*value = ((val >> ((where & 0x3) << 3)) & 0xff);
	else if(size == 2)
		*value = ((val >> ((where & 0x3) << 3)) & 0xffff);
	else if(size == 4)
		*value = val;
	else{
		pcie_error("Unkown size(%d) for read ops\n",size);
		BUG();
	}

	spin_unlock_irqrestore(&rlock, flag);
	return PCIBIOS_SUCCESSFUL;
}

unsigned int HOST_CONFIG_ADDR;

static int pcie_read_from_dbi(struct pcie_info* info, unsigned int devfn, int where, int size, u32* value){
	unsigned long flag;
	u32 v;
	/*for host-side config space read, ignore device func nr.*/
	if(devfn > 0)
		return -EIO;
	
	spin_lock_irqsave(&rlock, flag);
	v = (u32)readl((void*)(info->conf_base_addr + (where & (~0x3))));

	HOST_CONFIG_ADDR = info->conf_base_addr;

	if(1 == size)
		*value = (v >> ((where & 0x3) << 3)) & 0xff;
	else if(2 == size)
		*value = (v >> ((where & 0x3) << 3)) & 0xffff;
	else if(4 == size)
		*value = v;
	else{
		pcie_error("Unkown size for config read operation!");
		BUG();
	}

	spin_unlock_irqrestore(&rlock, flag);

	return PCIBIOS_SUCCESSFUL; 
}
static int pcie_write_to_device(struct pci_bus* bus, unsigned int devfn, int where, int size, u32 value);
static int pcie_read_conf(struct pci_bus* bus, unsigned int devfn, int where, int size, u32* value){
	struct pcie_info* info = bus_to_info(bus->number);
	int ret;
	
	if(unlikely(!info)){
		pcie_error("Cannot find corresponding controller for appointed device!");
		BUG();
	}
	if(bus->number == info->root_bus_nr){
		ret = pcie_read_from_dbi(info, devfn, where, size, value);
	}
	else{
		ret = pcie_read_from_device(bus, devfn, where, size, value);
	}
	return ret;
}
static int pcie_write_to_device(struct pci_bus* bus, unsigned int devfn, int where, int size, u32 value){
	struct pcie_info* info = bus_to_info(bus->number);
	unsigned int addr;
	unsigned int org;
	unsigned long flag;
	if(!pcie_check_link_status()){
		pcie_debug(PCIE_DEBUG_LEVEL_MODULE, "pcie%d not link up!",info->controller == pcie_controller_0 ? 0: 1);
		return -1;
	}

	spin_lock_irqsave(&wlock, flag);	
	pcie_read_from_device(bus, devfn, where, 4, &org);

	addr = to_pcie_address(bus, devfn, where);

	if(size == 1){
		org &= (~(0xff << ((where & 0x3) << 3)));
		org |= (value << ((where & 0x3) << 3));
	}else if(size == 2){
		org &= (~(0xffff << ((where & 0x3) << 3)));
		org |= (value << ((where & 0x3) << 3));
	}else if(size == 4){
		org = value;
	}else{
		pcie_error("Unkown size(%d) for read ops\n",size);
		BUG();	
	}
	writel(org, addr);

	spin_unlock_irqrestore(&wlock, flag);
	return PCIBIOS_SUCCESSFUL;
}
static int pcie_write_to_dbi(struct pcie_info* info, unsigned int devfn, int where, int size, u32 value){
	unsigned long flag;
	unsigned int org;

	spin_lock_irqsave(&wlock, flag);

	if(pcie_read_from_dbi(info, devfn, where, 4, &org)){
		pcie_error("Cannot read from dbi! 0x%x:0x%x:0x%x!",0, devfn, where);
		spin_unlock_irqrestore(&wlock, flag);
		return -EIO;
	}
	if(size == 1){
		org &= (~(0xff << ((where & 0x3) << 3)));
		org |= (value << ((where & 0x3) << 3));
	}else if(size == 2){
		org &= (~(0xffff << ((where & 0x3) << 3)));
		org |= (value << ((where & 0x3) << 3));
	}else if(size == 4){
		org = value;
	}else{
		pcie_error("Unkown size(%d) for read ops\n",size);
		BUG();	
	}
	writel(org, info->conf_base_addr + (where & (~0x3)));

	spin_unlock_irqrestore(&wlock, flag);
	return PCIBIOS_SUCCESSFUL;
}
static int pcie_write_conf(struct pci_bus *bus, unsigned int devfn, int where, int size, u32 value){
	struct pcie_info* info = bus_to_info(bus->number);
	if(unlikely(!info)){
		pcie_error("Cannot find corresponding controller for appointed device!");
		BUG();
	}
	if(bus->number == info->root_bus_nr){
		return pcie_write_to_dbi(info, devfn, where, size, value);
	}
	else{
		return pcie_write_to_device(bus, devfn, where, size, value);
	}
}
static struct pci_ops hisik3_pcie_ops = {
	.read = pcie_read_conf,
	.write = pcie_write_conf,
};

static struct pci_bus *__init hisik3_pcie_scan_bus(int nr, struct pci_sys_data* sys){
	struct pci_bus *bus;
		
	if (nr < hisik3_pcie.nr_controllers) {
		bus = pci_scan_bus(sys->busnr, &hisik3_pcie_ops, sys);
	} else {
		bus = NULL;
		pcie_error("Unkown controller nr :0x%x!",nr);
		BUG();
	}
	return bus;
}

static int __init hisik3_pcie_init(void){
	pcie_debug(PCIE_DEBUG_LEVEL_MODULE, "Init param: pcie0 sel: 0x%x, pcie0 memsize:0x%x. pcie0_iatu_table[2].lbar=0x%x\n",
			pcie0_sel,  pcie0_mem_space_size,pcie0_iatu_table[2].lbar);
	
	/*Hook data abort exception*/
	hook_fault_code(22,hisik3_pci_fault,SIGBUS, 7, "external abort on non-linefetch");
	
	//Software init.
	if(pcie_sw_init()){
		return -EIO;
	}
	
	//This branch was just in PCIE0_MODE_SEL mode.
	if(pcie_sys_init(PCIE0_MODE_SEL)){
		return -EIO;
	}
	/*Mount point for k3 pcie bus driver.*/
	pci_common_init(&hisik3_pcie);
	return 0;
}

subsys_initcall(hisik3_pcie_init);
