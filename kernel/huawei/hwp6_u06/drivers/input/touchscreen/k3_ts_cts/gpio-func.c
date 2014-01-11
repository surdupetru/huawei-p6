#include <asm/io.h>
#include <asm/gpio.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/delay.h>
//#include <mach/gpio.h>
#include "gpio.h"
#include <mach/platform.h>
#include <mach/early-debug.h>



void hi_set_bit(unsigned long nr, volatile void * addr)
{
	int *m = ((int *) addr) + (nr >> 5);

	*m |= 1 << (nr & 31);
}


void hi_clear_bit(unsigned long nr, volatile void * addr)
{
	int *m = ((int *) addr) + (nr >> 5);

	*m &= ~(1 << (nr & 31));
}

unsigned int hisik3_get_gpio_baseaddr(unsigned int group_id)
{    
	switch(group_id)
	{
		case GPIO_GROUP0:
			return BASE_ADDR_GPIO_GROUP0;   
		case GPIO_GROUP1:
			return BASE_ADDR_GPIO_GROUP1;
		case GPIO_GROUP2:
			return BASE_ADDR_GPIO_GROUP2;
		case GPIO_GROUP3:
			return BASE_ADDR_GPIO_GROUP3;
		case GPIO_GROUP4:
			return BASE_ADDR_GPIO_GROUP4;
		case GPIO_GROUP5:
			return BASE_ADDR_GPIO_GROUP5;
		case GPIO_GROUP6:
			return BASE_ADDR_GPIO_GROUP6;
		case GPIO_GROUP7:
			return BASE_ADDR_GPIO_GROUP7;
		case GPIO_GROUP8:
			return BASE_ADDR_GPIO_GROUP8;
		case GPIO_GROUP9:
			return BASE_ADDR_GPIO_GROUP9;
		case GPIO_GROUP10:
			return BASE_ADDR_GPIO_GROUP10;
		case GPIO_GROUP11:
			return BASE_ADDR_GPIO_GROUP11;
		case GPIO_GROUP12:
			return BASE_ADDR_GPIO_GROUP12;
		case GPIO_GROUP13:
			return BASE_ADDR_GPIO_GROUP13;
		default:
			return 0;
	}
}
EXPORT_SYMBOL(hisik3_get_gpio_baseaddr);

unsigned int hisik3_setup_gpio_regvalue(unsigned long addr, unsigned int bit, unsigned int data)
{
	if( 1 == data ){
		hi_set_bit(bit,(void*)addr);
		return 0;
	}; 
	
	if( 0 == data){
		hi_clear_bit(bit,(void*)addr);
		return 0;
	}; 
	
	return data;
}
EXPORT_SYMBOL(hisik3_setup_gpio_regvalue);

unsigned int hisik3_get_gpio_regvalue(unsigned long addr, unsigned int bit)
{
	unsigned int value;

	value = readl(addr);
	value = (value >> bit) & 0x01;
	return value;
}

e_gpio_dir hisik3_gpio_get_direction(unsigned int gpio_id)
{
	e_gpio_dir value = EGPIO_DIR_NUM;
	unsigned int gpio_group = (gpio_id - GPIO_ID_BASE) >> GPIO_GROUP_BIT_POS;
	unsigned int gpio_num = (gpio_id - GPIO_ID_BASE) & GPIO_ID_MASK;
	unsigned long gpio_addr = 0;

	if(gpio_group >= GPIO_GROUP_MAX){ 
		return value;
	}
   
	gpio_addr = IO_ADDRESS(hisik3_get_gpio_baseaddr(gpio_group));

	value = (e_gpio_dir)hisik3_get_gpio_regvalue((gpio_addr + REG_GPIO_DIR), gpio_num);

	return value;
}
EXPORT_SYMBOL(hisik3_gpio_get_direction);

unsigned int hisik3_gpio_set_direction(unsigned int gpio_id, e_gpio_dir dir)
{
	unsigned int value = 0;
	unsigned int gpio_group = (gpio_id - GPIO_ID_BASE) >> GPIO_GROUP_BIT_POS;
	unsigned int gpio_num = (gpio_id - GPIO_ID_BASE) & GPIO_ID_MASK;
	unsigned long gpio_addr = 0;

	if((gpio_group >= GPIO_GROUP_MAX) || (dir >= EGPIO_DIR_NUM)){ 
		return -1;
	}
	gpio_addr = IO_ADDRESS(hisik3_get_gpio_baseaddr(gpio_group));
	printk("gpio addr 0x%lx  gpio num %u dir %u \n", gpio_addr, gpio_num, dir);
	value = (e_gpio_dir) hisik3_setup_gpio_regvalue((gpio_addr + REG_GPIO_DIR), gpio_num, (unsigned int)dir);

	return 0;
}
EXPORT_SYMBOL(hisik3_gpio_set_direction);

unsigned int hisik3_gpio_setvalue(unsigned int gpio_id, e_gpio_data data)
{
	unsigned int value = 0;
	unsigned int gpio_group = (gpio_id - GPIO_ID_BASE) >> GPIO_GROUP_BIT_POS;
	unsigned int gpio_num = (gpio_id - GPIO_ID_BASE) & GPIO_ID_MASK;
	unsigned long gpio_addr = 0;

	if((gpio_group >= GPIO_GROUP_MAX) || (data >= EGPIO_DATA_NUM)){
		return -1;
	}

	gpio_addr = IO_ADDRESS(hisik3_get_gpio_baseaddr(gpio_group));
    
	printk("gpio addr 0x%lx  gpio num %u data %u \n", gpio_addr, gpio_num, data);
	value = hisik3_setup_gpio_regvalue(gpio_addr + REG_GPIO_DATA, gpio_num, data);
  
	return 0;
}
EXPORT_SYMBOL(hisik3_gpio_setvalue);

/*add by w56462*/

unsigned int hisik3_gpio_IntrEnable(unsigned int gpio_id)
{
	unsigned int gpio_group = (gpio_id - GPIO_ID_BASE) >> GPIO_GROUP_BIT_POS;
	unsigned int gpio_num = (gpio_id - GPIO_ID_BASE) & GPIO_ID_MASK;
	unsigned long gpio_addr = 0;

	if(gpio_group >= GPIO_GROUP_MAX)
	{ 
		return -1;
	}
   
	gpio_addr = IO_ADDRESS(hisik3_get_gpio_baseaddr(gpio_group));
	hisik3_setup_gpio_regvalue((gpio_addr + REG_GPIO_INTMASK), gpio_num, 0x1);

	return 0;
}

EXPORT_SYMBOL(hisik3_gpio_IntrEnable);


unsigned int hisik3_gpio_IntrDisable(unsigned int gpio_id)
{
	unsigned int gpio_group = (gpio_id - GPIO_ID_BASE) >> GPIO_GROUP_BIT_POS;
	unsigned int gpio_num = (gpio_id - GPIO_ID_BASE) & GPIO_ID_MASK;
	unsigned long gpio_addr = 0;

	if(gpio_group >= GPIO_GROUP_MAX)
	{ 
		return -1;
	}
   
	gpio_addr = IO_ADDRESS(hisik3_get_gpio_baseaddr(gpio_group));
	//printk("gpio addr 0x%lx  gpio num %u dir %u \n", gpio_addr, gpio_num, dir);
	hisik3_setup_gpio_regvalue((gpio_addr + REG_GPIO_INTMASK), gpio_num, 0x0);

	return 0;
}

EXPORT_SYMBOL(hisik3_gpio_IntrDisable);

unsigned int hisik3_gpio_IntrClr(unsigned int gpio_id)
{
	unsigned int gpio_group = (gpio_id - GPIO_ID_BASE) >> GPIO_GROUP_BIT_POS;
	unsigned int gpio_num = (gpio_id - GPIO_ID_BASE) & GPIO_ID_MASK;
	unsigned long gpio_addr = 0;

	if(gpio_group >= GPIO_GROUP_MAX)
	{ 
		return -1;
	}
   
	gpio_addr = IO_ADDRESS(hisik3_get_gpio_baseaddr(gpio_group));
	//printk("gpio addr 0x%lx  gpio num %u dir %u \n", gpio_addr, gpio_num, dir);
	hisik3_setup_gpio_regvalue((gpio_addr + REG_GPIO_INTCLEAR), gpio_num, 0x1);

	return 0;
}

EXPORT_SYMBOL(hisik3_gpio_IntrClr);


unsigned int hisik3_gpio_Get_IntrStats(unsigned int gpio_id)
{
	unsigned int value = -1;
	unsigned int gpio_group = (gpio_id - GPIO_ID_BASE) >> GPIO_GROUP_BIT_POS;
	unsigned int gpio_num = (gpio_id - GPIO_ID_BASE) & GPIO_ID_MASK;
	unsigned long gpio_addr = 0;

	if(gpio_group >= GPIO_GROUP_MAX)
	{ 
		return value;
	}
   
	gpio_addr = IO_ADDRESS(hisik3_get_gpio_baseaddr(gpio_group));

	value = hisik3_get_gpio_regvalue((gpio_addr + REG_GPIO_INTMASKEDSTATUS), gpio_num);

	return value;
}
EXPORT_SYMBOL(hisik3_gpio_Get_IntrStats);

unsigned int hisik3_gpio_Get_RawIntrStats(unsigned int gpio_id)
{
	unsigned int value = -1;
	unsigned int gpio_group = (gpio_id - GPIO_ID_BASE) >> GPIO_GROUP_BIT_POS;
	unsigned int gpio_num = (gpio_id - GPIO_ID_BASE) & GPIO_ID_MASK;
	unsigned long gpio_addr = 0;

	if(gpio_group >= GPIO_GROUP_MAX)
	{ 
		return value;
	}
   
	gpio_addr = IO_ADDRESS(hisik3_get_gpio_baseaddr(gpio_group));

	value = hisik3_get_gpio_regvalue((gpio_addr + REG_GPIO_INTRAWSTATUS), gpio_num);

	return value;
}
EXPORT_SYMBOL(hisik3_gpio_Get_RawIntrStats);

e_gpio_data hisik3_gpio_getvalue(unsigned int gpio_id)
{
	e_gpio_data value = EGPIO_DATA_NUM;
	unsigned int gpio_group = (gpio_id - GPIO_ID_BASE) >> GPIO_GROUP_BIT_POS;
	unsigned int gpio_num = (gpio_id - GPIO_ID_BASE) & GPIO_ID_MASK;
	unsigned long gpio_addr = 0;

	if(gpio_group >= GPIO_GROUP_MAX){
		return value;
	}

	gpio_addr = IO_ADDRESS(hisik3_get_gpio_baseaddr(gpio_group));

	value = (e_gpio_data)hisik3_get_gpio_regvalue(gpio_addr + REG_GPIO_DATA, gpio_num);

	return value;
}
EXPORT_SYMBOL(hisik3_gpio_getvalue);

int hisik3_gpio_initialize(unsigned int gpio_id, e_gpio_dir dir, e_gpio_data data)
{
		int ret = -1;
	if(hisik3_gpio_set_direction(gpio_id, dir) == 0){
		if(EGPIO_DIR_OUTPUT == dir){
			ret = hisik3_gpio_setvalue(gpio_id, data);
		}
		else{   
			ret = 0;
		}
	}

	return ret;
}
EXPORT_SYMBOL(hisik3_gpio_initialize);

int hisik3_gpio_mode_select(unsigned int gpio_id, e_gpio_mode mode)
{
	unsigned int gpio_group = (gpio_id - GPIO_ID_BASE) >> GPIO_GROUP_BIT_POS;
	unsigned int gpio_num = (gpio_id - GPIO_ID_BASE) & GPIO_ID_MASK;
	unsigned long gpio_addr = 0;
	unsigned int value;

	//printk("hisik3_gpio_mode_select gpio group %u num %u mode %u out of range \n", gpio_group, gpio_num, mode);
	if((gpio_group >= GPIO_GROUP_MAX) || (mode >= EGPIO_MODE_NUM)){
		printk("gpio group %u mode %u out of range \n", gpio_group, mode);
		return -1;
	}
   
	gpio_addr = IO_ADDRESS(hisik3_get_gpio_baseaddr(gpio_group));

	value = hisik3_setup_gpio_regvalue((gpio_addr + REG_GPIO_MODE), gpio_num, mode);

	return value;
}
EXPORT_SYMBOL(hisik3_gpio_mode_select);

unsigned int hisik3_gpio_setsense(unsigned int gpio_id, e_gpio_sense data)
{
	unsigned int value = 0;
	unsigned int gpio_group = (gpio_id - GPIO_ID_BASE) >> GPIO_GROUP_BIT_POS;
	unsigned int gpio_num = (gpio_id - GPIO_ID_BASE) & GPIO_ID_MASK;
	unsigned long gpio_addr = 0;

	if((gpio_group >= GPIO_GROUP_MAX) || (data >= EGPIO_EDGE_NUM)){
		return -1;
	}

	gpio_addr = IO_ADDRESS(hisik3_get_gpio_baseaddr(gpio_group));
    
	value = hisik3_setup_gpio_regvalue(gpio_addr + REG_GPIO_INTSENSE, gpio_num, data);
  
	return 0;
}
EXPORT_SYMBOL(hisik3_gpio_setsense);

unsigned int hisik3_gpio_getsense(unsigned int gpio_id)
{
	unsigned int value = 0;
	unsigned int gpio_group = (gpio_id - GPIO_ID_BASE) >> GPIO_GROUP_BIT_POS;
	unsigned int gpio_num = (gpio_id - GPIO_ID_BASE) & GPIO_ID_MASK;
	unsigned long gpio_addr = 0;

	if(gpio_group >= GPIO_GROUP_MAX)
		return -1;

	gpio_addr = IO_ADDRESS(hisik3_get_gpio_baseaddr(gpio_group));
    
	value = hisik3_get_gpio_regvalue(gpio_addr + REG_GPIO_INTSENSE, gpio_num);
  
	return value;
}
EXPORT_SYMBOL(hisik3_gpio_getsense);

unsigned int hisik3_gpio_setedge(unsigned int gpio_id, e_gpio_edge data)
{
	unsigned int value = 0;
	unsigned int gpio_group = (gpio_id - GPIO_ID_BASE) >> GPIO_GROUP_BIT_POS;
	unsigned int gpio_num = (gpio_id - GPIO_ID_BASE) & GPIO_ID_MASK;
	unsigned long gpio_addr = 0;

	if((gpio_group >= GPIO_GROUP_MAX) || (data >= EGPIO_EDGE_NUM)){
		return -1;
	}

	gpio_addr = IO_ADDRESS(hisik3_get_gpio_baseaddr(gpio_group));
    
	value = hisik3_setup_gpio_regvalue(gpio_addr + REG_GPIO_INTEDGE, gpio_num, data);
  
	return 0;
}
EXPORT_SYMBOL(hisik3_gpio_setedge);

unsigned int hisik3_gpio_getedge(unsigned int gpio_id)
{
	unsigned int value = 0;
	unsigned int gpio_group = (gpio_id - GPIO_ID_BASE) >> GPIO_GROUP_BIT_POS;
	unsigned int gpio_num = (gpio_id - GPIO_ID_BASE) & GPIO_ID_MASK;
	unsigned long gpio_addr = 0;

	if(gpio_group >= GPIO_GROUP_MAX)
		return -1;

	gpio_addr = IO_ADDRESS(hisik3_get_gpio_baseaddr(gpio_group));
    
	value = hisik3_get_gpio_regvalue(gpio_addr + REG_GPIO_INTEDGE, gpio_num);
  
	return value;
}
EXPORT_SYMBOL(hisik3_gpio_getedge);

unsigned int hisik3_gpio_setevent(unsigned int gpio_id, e_gpio_event data)
{
	unsigned int value = 0;
	unsigned int gpio_group = (gpio_id - GPIO_ID_BASE) >> GPIO_GROUP_BIT_POS;
	unsigned int gpio_num = (gpio_id - GPIO_ID_BASE) & GPIO_ID_MASK;
	unsigned long gpio_addr = 0;

	if((gpio_group >= GPIO_GROUP_MAX) || (data >= EGPIO_EVENT_NUM)){
		return -1;
	}

	gpio_addr = IO_ADDRESS(hisik3_get_gpio_baseaddr(gpio_group));
    
	value = hisik3_setup_gpio_regvalue(gpio_addr + REG_GPIO_INTEVENT, gpio_num, data);
  
	return 0;
}
EXPORT_SYMBOL(hisik3_gpio_setevent);

unsigned int hisik3_gpio_getevent(unsigned int gpio_id)
{
	unsigned int value = 0;
	unsigned int gpio_group = (gpio_id - GPIO_ID_BASE) >> GPIO_GROUP_BIT_POS;
	unsigned int gpio_num = (gpio_id - GPIO_ID_BASE) & GPIO_ID_MASK;
	unsigned long gpio_addr = 0;

	if(gpio_group >= GPIO_GROUP_MAX)
		return -1;

	gpio_addr = IO_ADDRESS(hisik3_get_gpio_baseaddr(gpio_group));
    
	value = hisik3_get_gpio_regvalue(gpio_addr + REG_GPIO_INTEVENT, gpio_num);
  
	return value;
}
EXPORT_SYMBOL(hisik3_gpio_getevent);

int t_gpio_to_irq(unsigned gpio)
{
	/* GPIO can never have been requested or set as input */
	WARN_ON(1);
	return -EINVAL;
}
EXPORT_SYMBOL(t_gpio_to_irq);
