/*
 *
 * arch/arm/mach-k3v2/iomux.c
 *
 * Copyright (C) 2011 Hisilicon
 * License terms: GNU General Public License (GPL) version 2
 *
 */
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/mux.h>
#include <mach/boardid.h>
#include <linux/io.h>
#include "iomux.h"
#include <hsad/config_interface.h>
#include "k3v2_iomux_blocks.h"

static DEFINE_MUTEX(iomux_lock);
static DEFINE_MUTEX(iomux_lock_debugfs);
#define iomux_lock_save(c, flags)			\
	do {						\
		spin_lock_irqsave(&c->spinlock, flags);	\
	} while (0)

#define iomux_unlock_restore(c, flags)				\
	do {							\
		spin_unlock_irqrestore(&c->spinlock, flags);	\
	} while (0)

/*get the pin by name*/
struct  iomux_pin *iomux_get_pin(char *name)
{
	int i = 0;
	int chip_id = 0;
	struct  iomux_pin *pin = NULL;
	struct iomux_pin_table *pintable = &pins_table_cs[0];

	chip_id = get_chipid();
	if (chip_id == CS_CHIP_ID) {
		pintable = &pins_table_cs[0];
	} else if (chip_id == DI_CHIP_ID) {
		pintable = &pins_table[0];
	} else {
		pr_err("%s %d no proper chip id find.\n", __func__, __LINE__);
	}

	while (pintable[i].pinname) {
		if (strncmp(name, pintable[i].pinname, MAX_NAME_CHARS)) {
			i++;
		} else {
			pin = pintable[i].iomux_pin;
			if (pin->init == 0) {
				spin_lock_init(&pin->spinlock);
				pin->init = 1;
			}
			break;
		}
	}

	if (pin == NULL)
		pr_err("IOMUX:%s pin get failed.\r\n", name);

	return pin;
}
EXPORT_SYMBOL(iomux_get_pin);

 /*set the pin as pull up or down*/
int pinmux_setpullupdown(struct  iomux_pin *pin, enum pull_updown pin_pull_updown)
{
	unsigned long flags;
	int ret = 0;
	char *free_mode = "lowpower";

	iomux_lock_save(pin, flags);
	/*
	 *when the pin is used as low power,can't set the pull
	 *up down, only it's set as gpio and normal
	 */
	if (pin->pin_owner && !strncmp(pin->pin_owner, free_mode, MAX_NAME_CHARS)) {
		pr_err("IOMUX:the pin %s is set as low power,can't set pull.\r\n", pin->pin_name);
		ret = -INVALID;
		goto out;
	}

	/*
	 *when it's set as low power mode,the pull and strength
	 *drive are set as low power ,can't be changed
	 */
	if (pin && pin->ops && pin->ops->pin_setpullupdown) {
		ret = pin->ops->pin_setpullupdown(pin, pin_pull_updown);
	} else {
		pr_err("IOMUX:pull up or down setting failed.\r\n");
		ret = -INVALID;
	}
out:
	iomux_unlock_restore(pin, flags);
	return ret;
}
EXPORT_SYMBOL(pinmux_setpullupdown);

/*set the pin'driver strength*/
int pinmux_setdrivestrength(struct  iomux_pin *pin, enum drive_strength pin_drive_strength)
{
	unsigned long flags;
	int ret = 0;
	char *free_mode = "lowpower";

	iomux_lock_save(pin, flags);
	/*
	 *when the pin is used by low power,can't set the drviver strength,
	 *only it's used by gpio and normal
	 */
	if (pin->pin_owner && !strncmp(pin->pin_owner, free_mode, FREEMODE_CHARS)) {
		pr_err("IOMUX:the pin %s is low power,drive strength setting failed.\r\n", pin->pin_name);
		ret = -INVALID;
		goto out;
	}

	if (pin && pin->ops && pin->ops->pin_setdrivestrength) {
		/*
		 *when it's set as low power mode, the pull and strength
		 *drive are set as low power, can't be changed
		 */
		ret = pin->ops->pin_setdrivestrength(pin, pin_drive_strength);
	} else {
		pr_err("IOMUX:driver strength setting failed.\r\n");
		ret = -INVALID;
	}
out:
	iomux_unlock_restore(pin, flags);
	return ret;
}
EXPORT_SYMBOL(pinmux_setdrivestrength);

/*judge the pin is used*/
static int iomux_canget_pin(struct  iomux_pin *pin, struct  iomux_block *block)
{
	char *free_mode = "lowpower";
	char *gpio_mode = "gpio";

	if (pin->pin_owner) {
		if (!strncmp(pin->pin_owner, free_mode, FREEMODE_CHARS) || !strncmp(pin->pin_owner, gpio_mode, FREEMODE_CHARS) ||
			!strncmp(pin->pin_owner, block->block_name, MAX_NAME_CHARS)) {
			/*if the pin is free,can get it*/
			return 0;
		} else {
			/*if the pin is used by other block can't get it*/
			pr_err("IOMUX:%s is used by %s,can't use it \
				here.\r\n", pin->pin_name, pin->pin_owner);
			return -INVALID;
		}
	}

	return 0;
}

/*get the block by name*/
struct iomux_block *iomux_get_block(char *name)
{
	int ret = 0;
	int iomux_type = 0;
	struct  iomux_block *block_temp = NULL;
	struct  iomux_pin **pins_temp = NULL;
	struct  block_table *table_temp = NULL;

	mutex_lock(&iomux_lock);
	iomux_type = get_iomux_type();
	if (iomux_type == -1) {
		pr_err("Get IOMUX type is failed,%s %d.\r\n", __func__, __LINE__);
		goto out;
	}
	table_temp = block_config_tables[iomux_type];
	while ((*table_temp).name) {
		if (strncmp(name, (*table_temp).name, MAX_NAME_CHARS)) {
			table_temp++;
		} else {
			block_temp = (*table_temp).block;
			break;
		}
	}

	/*
	 *if there's pin has been used by another block, this block
	 *can't use it here,so get block failed,return NULL
	 */
	if (block_temp) {
		/* spin lock need init */
		if (block_temp->init == 0) {
			spin_lock_init(&block_temp->spinlock);
			block_temp->init = 1;
		}

		pins_temp = block_temp->pins;
		while (*pins_temp) {
			/*
			 *when the pin's function is lowpower, it isn't used
			 *by other block if its owner isn't low power, it must
			 *be used as normal by other block or as gpio, the
			 *current block can't use it.
			 */
			ret = iomux_canget_pin(*pins_temp, block_temp);
			if (ret < 0) {
				block_temp = NULL;
				pr_err("IOMUX:block %s get failed.\r\n", name);
				goto out;
			}
			pins_temp++;
		}
	}
out:
	mutex_unlock(&iomux_lock);
	return block_temp;
}
EXPORT_SYMBOL(iomux_get_block);

struct block_config *iomux_get_blockconfig(char *name)
{
	int iomux_type = 0;
	struct block_config *config_temp = NULL;
	struct  block_table *table_temp = NULL;

	iomux_type = get_iomux_type();
	if (iomux_type == -1) {
		pr_err("Get IOMUX type is failed,%s %d.\r\n", __func__, __LINE__);
		return NULL;
	}
	table_temp = block_config_tables[iomux_type];
	while ((*table_temp).name) {
		if (strncmp(name, (*table_temp).name, MAX_NAME_CHARS)) {
			table_temp++;
		} else {
			config_temp = (*table_temp).config_array;
			break;
		}
	}

	return config_temp;
}
EXPORT_SYMBOL(iomux_get_blockconfig);

int blockmux_set(struct iomux_block *blockmux,
	struct block_config *config, enum iomux_func newmode)
{
	int ret = 0;
	unsigned long flags;

	if (!blockmux || !config)
		return -INVALID;

	iomux_lock_save(blockmux, flags);
	if (blockmux->ops && blockmux->ops->block_setfunc) {
		ret = blockmux->ops->block_setfunc(blockmux, config, newmode);
		if (ret < 0) {
			pr_err("IOMUX:%s's function setting failed.\r\n", blockmux->block_name);
			goto out;
		}
	} else {
		pr_err("IOMUX:function setting failed.\r\n");
		ret = -INVALID;
	}
out:
	iomux_unlock_restore(blockmux, flags);
	return ret;
}
EXPORT_SYMBOL(blockmux_set);

void __init iomux_init_blocks(void)
{
	int ret;
	int iomux_type = 0;
	struct  iomux_block *block_temp = NULL;
	struct block_config *config_temp = NULL;
	struct  block_table *table_temp = NULL;

	iomux_type = get_iomux_type();
	if (iomux_type == -1) {
		pr_err("Get IOMUX type is failed,%s %d.\r\n", __func__, __LINE__);
		return ;
	}
	table_temp = block_config_tables[iomux_type];
	while ((*table_temp).name) {
		block_temp = (*table_temp).block;
		config_temp = (*table_temp).config_array;
		ret = blockmux_set(block_temp, config_temp, LOWPOWER);
		if (ret) {
			pr_err("IOMUX:iomux initialized failed.\r\n");
			break;
		}
		table_temp++;
	}
}

#ifdef	CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#include <linux/seq_file.h>
static char g_pullInfo[3][10] = {"NOPULL", "pullup", "pulldown"};

char *pin_pull_status(struct  iomux_pin *pin)
{
	if (pin->pin_pull_updown <= 2)
		return g_pullInfo[pin->pin_pull_updown];
	else
		return NULL;
}
static int dbg_pinmux_show(struct seq_file *s, void *unused)
{
	int i = 0;
	struct  iomux_pin *pin_temp = NULL;

	seq_printf(s, "pinname	       pin_owner        func        \
			pullud        driverstrength		\n");
	seq_printf(s, "-----------------------------------------"
			"-----------------------------------------------\n");

	mutex_lock(&iomux_lock);
	while (pins_table[i].pinname) {
		pin_temp = iomux_get_pin(pins_table[i].pinname);
		if (pin_temp) {
			seq_printf(s, "%s        %s        %d        %s        %d\r\n",
				pin_temp->pin_name,
				pin_temp->pin_owner,
				pin_temp->pin_func,
				pin_pull_status(pin_temp),
				pin_temp->pin_drive_strength);
		}
		i++;
	}
	mutex_unlock(&iomux_lock);
	return 0;
}

static int dbg_pinmux_open(struct inode *inode, struct file *file)
{
	return single_open(file, dbg_pinmux_show, &inode->i_private);
}

static const struct file_operations debug_pin_fops = {
	.open		= dbg_pinmux_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int dbg_blockmux_show(struct seq_file *s, void *unused)
{
	int iomux_type = 0;
	struct  iomux_block *block_temp = NULL;
	struct  iomux_pin **arraryp = NULL;
	struct  block_table *table_temp = NULL;

	seq_printf(s, "blockname	       func  		\n");
	seq_printf(s, "-----------------------------------------"
			"-----------------------------------------------\n");

	mutex_lock(&iomux_lock_debugfs);
	iomux_type = get_iomux_type();
	if (iomux_type == -1) {
		pr_err("Get IOMUX type is failed,%s %d.\r\n", __func__, __LINE__);
		mutex_unlock(&iomux_lock);
		return -INVALID;
	}
	table_temp = block_config_tables[iomux_type];
	while ((*table_temp).name) {
		block_temp = iomux_get_block((*table_temp).name);
		if (block_temp) {
			seq_printf(s, "Block:%s, Mode:%d\r\n",
				block_temp->block_name, block_temp->block_func);

			seq_printf(s, "currfun        pinname	  func  	pullud        driverstrength		\n");
			seq_printf(s, "-----------------------------------------"
			"-----------------------------------------------\n");

			arraryp = block_temp->pins;

			while (*arraryp) {
				seq_printf(s, "%s        %s  	%d        %s       %d\r\n",
				(*arraryp)->pin_owner,
				(*arraryp)->pin_name,
				(*arraryp)->pin_func,
				pin_pull_status(*arraryp),
				(*arraryp)->pin_drive_strength);
				arraryp++;
			}

		} else {
			seq_printf(s, "block get failed\r\n");
		}

		table_temp++;
	}
	mutex_unlock(&iomux_lock_debugfs);

	return 0;
}

static int dbg_blockmux_open(struct inode *inode, struct file *file)
{
	return single_open(file, dbg_blockmux_show, &inode->i_private);
}

static const struct file_operations debug_block_fops = {
	.open		= dbg_blockmux_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init iomux_debuginit(void)
{
	struct dentry *d;

	d = debugfs_create_dir("iomux", NULL);
	if (!d)
		return -ENOMEM;

	(void) debugfs_create_file("k3v2_blockmux", S_IRUSR,
					d, NULL, &debug_block_fops);
	(void) debugfs_create_file("k3v2_pinmux", S_IRUSR,
					d, NULL, &debug_pin_fops);
	return 0;
}
late_initcall(iomux_debuginit);
#endif
#ifdef	CONFIG_LOWPM_DEBUG
#define IOC_BASE_ADDR	((void __iomem *) IO_ADDRESS(REG_BASE_IOC))
#define GPIO_BASE_ADDR	((void __iomem *) IO_ADDRESS(REG_BASE_GPIO0))
#define GPIO_DIR(x)		(GPIO_BASE_ADDR + (x) * 0x1000 + 0x400)
#define GPIO_DATA(x)		(GPIO_BASE_ADDR + (x) * 0x1000 + 0x3FC)
#define GPIO_BIT(x, y)		((x) << (y))
#define GPIO_IS_SET(x)		(((uregv) >> (x)) & 0x1)

void iomux_debug_set(void)
{
	int i = 0;
	int io_type = 0;
	unsigned int uregv = 0;
	struct iocfg_lp *iocfg_lookups = NULL;

	io_type = get_iomux_type();
	if (io_type == -1) {
		pr_err("Get IOMUX type is failed,%s %d.\r\n", __func__, __LINE__);
		return ;
	}
	iocfg_lookups = io_suspend_config_tables[io_type];

	for (i = 0; i < IO_LIST_LENGTH; i++) {

		uregv = ((iocfg_lookups[i].ugpiog<<3)+iocfg_lookups[i].ugpio_bit);

		/*uart0 suspend printk*/
		if ((0 == console_suspend_enabled)
			&& ((uregv >= 117) && (uregv <= 120)))
			continue;

		if (E_BOARD_TYPE_PLATFORM == get_board_type()) {
			/*oem board*/
			if ((uregv == 40) || (uregv == 83))
				continue;

			if ((uregv >= 129) && (uregv <= 132))
				continue;

			if ((uregv >= 137) && (uregv <= 140))
				continue;
		} else {
			if ((uregv == 145) || (uregv == 146))
				continue;
		}

		uregv = readl(IOC_BASE_ADDR + (iocfg_lookups[i].uiomg_off));
		if (iocfg_lookups[i].iomg_val != -1) {
			if ((uregv&0x1) == iocfg_lookups[i].iomg_val)
				writel(uregv, IOC_BASE_ADDR + (iocfg_lookups[i].uiomg_off));
			else
				writel(iocfg_lookups[i].iomg_val, IOC_BASE_ADDR + (iocfg_lookups[i].uiomg_off));
		}

		uregv = readl(IOC_BASE_ADDR + 0x800 + (iocfg_lookups[i].uiocg_off));
		if (iocfg_lookups[i].iocg_val != -1) {
			if ((uregv&0x3) == iocfg_lookups[i].iocg_val)
				writel(uregv, IOC_BASE_ADDR + 0x800 + (iocfg_lookups[i].uiocg_off));
			else
				writel(iocfg_lookups[i].iocg_val, IOC_BASE_ADDR + 0x800 + (iocfg_lookups[i].uiocg_off));
		}

		uregv = readl(GPIO_DIR(iocfg_lookups[i].ugpiog));
		uregv &= ~GPIO_BIT(1, iocfg_lookups[i].ugpio_bit);
		uregv |= GPIO_BIT(iocfg_lookups[i].gpio_dir, iocfg_lookups[i].ugpio_bit);
		writel(uregv, GPIO_DIR(iocfg_lookups[i].ugpiog));

		uregv = readl(GPIO_DIR(iocfg_lookups[i].ugpiog));
		uregv = readl(GPIO_DATA(iocfg_lookups[i].ugpiog));
		uregv &= ~GPIO_BIT(1, iocfg_lookups[i].ugpio_bit);
		uregv |= GPIO_BIT(iocfg_lookups[i].gpio_val, iocfg_lookups[i].ugpio_bit);
		writel(uregv, GPIO_DATA(iocfg_lookups[i].ugpiog));

	}
}
EXPORT_SYMBOL(iomux_debug_set);

void iomux_debug_show(int check)
{
	int i = 0;
	int io_type = 0;
	int iflg = 0;
	unsigned int uregv = 0;
	struct iocfg_lp *iocfg_lookups = NULL;

	io_type = get_iomux_type();
	if (io_type == -1) {
		pr_err("Get IOMUX type is failed,%s %d.\r\n", __func__, __LINE__);
		return ;
	}
	iocfg_lookups = io_suspend_config_tables[io_type];

	for (i = 0; i < IO_LIST_LENGTH; i++) {

		iflg = 0;

		printk("GPIO_%02d_%d (%03d) ",\
			iocfg_lookups[i].ugpiog, iocfg_lookups[i].ugpio_bit,\
				((iocfg_lookups[i].ugpiog<<3)+iocfg_lookups[i].ugpio_bit));

		uregv = readl(IOC_BASE_ADDR + (iocfg_lookups[i].uiomg_off));
		printk("IOMG=0x%02X ", uregv);

		if (check == 1) {
			if ((uregv == iocfg_lookups[i].iomg_val)\
				|| (-1 == iocfg_lookups[i].iomg_val))
				printk("(0x%02X) ", (unsigned char)uregv);
			else {
				iflg = 1;
				printk("(0x%02X) ", (unsigned char)iocfg_lookups[i].iomg_val);
			}
		}

		uregv = readl(IOC_BASE_ADDR + 0x800 + (iocfg_lookups[i].uiocg_off));
		printk("IOCG=0x%02X ", uregv);

		if (check == 1) {
			if (((uregv & 0x3) == iocfg_lookups[i].iocg_val)\
				|| (-1 == iocfg_lookups[i].iocg_val))
				printk("(0x%02X) ", (unsigned char)uregv);
			else {
				iflg = 1;
				printk("(0x%02X) ", (unsigned char)iocfg_lookups[i].iocg_val);
			}
		}

		uregv = readl(GPIO_DIR(iocfg_lookups[i].ugpiog));
		printk("DIR=0x%02X ", GPIO_IS_SET(iocfg_lookups[i].ugpio_bit));

		if (check == 1) {
			if ((uregv & GPIO_BIT(1, iocfg_lookups[i].ugpio_bit))\
				== (GPIO_BIT(iocfg_lookups[i].gpio_dir, iocfg_lookups[i].ugpio_bit)))
				printk("(0x%02X) ", GPIO_IS_SET(iocfg_lookups[i].ugpio_bit));
			else {
				iflg = 1;
				printk("(0x%02X) ", (unsigned char)iocfg_lookups[i].gpio_dir);
			}
		}

		uregv = readl(GPIO_DATA(iocfg_lookups[i].ugpiog));
		printk("VAL=0x%02X ", GPIO_IS_SET(iocfg_lookups[i].ugpio_bit));

		if (check == 1) {
			if (((uregv & GPIO_BIT(1, iocfg_lookups[i].ugpio_bit))\
				== GPIO_BIT(iocfg_lookups[i].gpio_val, iocfg_lookups[i].ugpio_bit))\
					|| (uregv & GPIO_BIT(iocfg_lookups[i].iocg_val, iocfg_lookups[i].ugpio_bit)))
				printk("(0x%02X) ", GPIO_IS_SET(iocfg_lookups[i].ugpio_bit));
			else {
				iflg = 1;
				printk("(0x%02X) ", (unsigned char)iocfg_lookups[i].gpio_val);
			}
		}

		if (iflg == 1)
			printk("e");

		printk("\n");
	}
}
EXPORT_SYMBOL(iomux_debug_show);
#endif

