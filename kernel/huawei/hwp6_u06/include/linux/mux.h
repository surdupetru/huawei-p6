#ifndef __MACH_MUX_H
#define __MACH_MUX_H

struct iomux_pin;
struct iomux_block;
struct block_config;

enum iomux_func {
	NORMAL = 0,/*blocks' function such as uart*/
	GPIO,		/*the function is the same as low power mode,but the driver strength and pull up or down can be set differently*/
	LOWPOWER,	/*if the block is free ,it can be set to this mode to reduce the power consumption*/
};

enum pull_updown {
	NOPULL = 0,/*no pull up or down*/
	PULLUP = 1 << 0,/*pull up*/
	PULLDOWN = 1 << 1,/*pull down*/
};

enum drive_strength {
	LEVEL0 = 0,/*2mA*/
	LEVEL1 = 1,/*4mA*/
	LEVEL2 = 3,/*6mA*/
	LEVEL3 = 7,/*8mA*/
	LEVEL4 = 15,/*10mA*/
};

/*
  * iomux_get_pin - get a pin by pin's name from pins table
  * @name: pin's name
  *
  * Returns a struct iomux_pin  or NULL
  */
struct iomux_pin 	*iomux_get_pin(char *name);

/*
  * iomux_get_block - get a block by block's name from blocks table
  * @name: block's name
  *
  * Returns a struct iomux_block  or NULL
  */
struct iomux_block *iomux_get_block(char *name);

/*
  * iomux_get_blockconfig - get a block's config by block's name from blocks table
  * @name: block's name
  *
  * Returns a struct block_config  or NULL
  */
struct block_config *iomux_get_blockconfig(char *name);

/*
  * pinmux_setpullupdown - set pin as pull up or pull down
  * @pin: pin's name
  * @pin_pull_updown: the pin should be set to pull up or down
  *
  * Returns 0  or negative value
  */
int pinmux_setpullupdown(struct  iomux_pin *pin, enum pull_updown pin_pull_updown);

/*
  * pinmux_setdrivestrength - set pin's driver strength
  * @pin: pin's name
  * @pin_drive_strength: the pin's drive strength
  *
  * Returns 0  or negative value
  */
int pinmux_setdrivestrength(struct  iomux_pin *pin, enum drive_strength pin_drive_strength);

/*
  * blockmux_set - set the function, pull up down, drive strength together with the config table
  * @blockmux:
  * @config: the block's config
  * @newmode: function
  *
  * Returns 0  or negative value
  */
int blockmux_set(struct iomux_block *blockmux, struct block_config *config, enum iomux_func newmode);

#endif
