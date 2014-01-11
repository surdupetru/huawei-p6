#ifndef __MACH_IOMUX_H
#define __MACH_IOMUX_H

#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/mux.h>

#define RESERVE -1/*used when don't need to set driver strength*/
#define NOINPUT -2
#define INVALID 16
#define MAX_NAME_CHARS 30
#define FREEMODE_CHARS 10
enum lowlayer_func {
	FUNC0 = 0,
	FUNC1,
	FUNC2,
	FUNC3,
	FUNC4,
	FUNC5,
	FUNC6,
};

/*iomg description*/
struct iomux_iomg {
	char *name;
	void __iomem	*iomg_reg;
	int *regValue;
};

/*iocg description*/
struct iomux_iocg {
	char *name;
	void __iomem	*iocg_reg;
	unsigned int iocg_pullupdown_mask;
	unsigned int iocg_drivestrength_mask;
};

/*pin description*/
struct iomux_pin {
	char *pin_name;
	char *pin_owner;
	enum lowlayer_func pin_func;
	enum pull_updown pin_pull_updown;
	enum drive_strength pin_drive_strength;
	struct iomux_ops *ops;
	struct iomux_iomg *pin_iomg;
	struct iomux_iocg  *pin_iocg;
	struct mutex	mutex;
	unsigned int	init;
	spinlock_t	spinlock;
};

struct iomux_block {
	char *block_name;
	enum iomux_func block_func;
	/*the pointer of pins' array,these pins are containd by block*/
	struct iomux_pin **pins;
	struct iomux_ops *ops;
	struct mutex	mutex;
	unsigned int init;
	spinlock_t spinlock;
};

/*this is the value should be written to the bins*/
struct block_config {
	/*the pointer of pins' function,these pins are containd by block*/
	enum lowlayer_func *func_value;
	/*the pointer of pins' status,these pins are containd by block*/
	enum pull_updown *pullud_value;
	/*the pointer of pins' drive strength,these pins are containd by block*/
	enum drive_strength *drvstrength_value;
};

struct block_table {
	char *name;
	struct iomux_block *block;
	/*the pointer of config array,this array contains normal config, lowpower config*/
	struct block_config *config_array;
};

struct iomux_ops {
	int (*pin_setfunc)(struct  iomux_pin *pin,
			enum lowlayer_func pin_pull_updown);
	int (*pin_setpullupdown)(struct  iomux_pin *pin,
			enum pull_updown pin_pull_updown);
	int (*pin_setdrivestrength)(struct  iomux_pin *pin,
			enum drive_strength pin_drive_strength);
	int (*pin_getstatus)(struct iomux_pin *pin);
	int (*block_setfunc)(struct iomux_block  *blockmux,
			struct block_config *config, enum iomux_func newfunc);
};

struct iomux_pin_table {
	char *pinname;
	struct  iomux_pin *iomux_pin;
	struct list_head	node;
};

 /*TODO:every config of blocks*/
 #define BLOCK_CONFIG(_name, _block, _configarray) \
 {\
         .name = _name,\
         .block = _block,\
         .config_array = _configarray,\
 },
void iomux_init_blocks(void);
extern struct block_table *block_config_tables[];
extern struct iomux_pin_table pins_table[];
extern struct iomux_pin_table pins_table_cs[];

#ifdef	CONFIG_LOWPM_DEBUG
#include <linux/console.h>
struct iocfg_lp {
	unsigned int uiomg_off;
	int iomg_val;
	unsigned int uiocg_off;
	int iocg_val;
	unsigned int ugpiog;
	unsigned int ugpio_bit;
	int gpio_dir;
	int gpio_val;
};
extern void iomux_debug_set(void);
extern void iomux_debug_show(int check);
#endif

#endif
