/* kernel/drivers/input/keyboard/k3_keypad.c
 *
 * K3 Keypad Driver
 * Copyright (C) 2011 Hisilicon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/mux.h>
#include <asm/irq.h>
#include <mach/k3_keypad.h>

/*The switch to support long-press key and combo key*/
#define ADVANCED_FUNCTION_SUPPORT

#define KPC_MAX_ROWS                                    (8)
#define KPC_MAX_COLS                                    (8)
#define KEY_RELEASE                                     (0)
#define KEY_PRESS                                       (1)
#define KPC_BIT_PER_KEY                                 (6)
#define KPC_BIT_PER_KEYROW                              (3)

/*KPC clock frequency*/
#define KPC_CLK_RATE									(32768)

/*KPC Register Offset*/
#define KPC_CONTROL_OFFSET                              (0x000)
#define KPC_INTERVAL_SHORT_OFFSET                       (0x004)
#define KPC_INT_STATUS_OFFSET                           (0x01C)
#define KPC_INT_CLR_OFFSET                              (0x058)
#define KPC_RELEASE_INT_CLR_OFFSET                      (0x06C)

/*BITMASK in KPC_INT_STATUS REG */
#define KEY_NUM_BITMASK                                 (0x3C0000)
#define KEY_NUM_BITPOS                                  (18)
#define KEY_VALUE_BITMASK                               (0xFFF)
#define KEY_VALUE_BITPOS                                (0)
#define KPC_RELEASE_INT_BITMASK                         (0x40000000)
#define KPC_RELEASE_INT_BITPOS                          (30)

/*REG config value */
#define KPC_VAL_CTLREG                                  (0x023)     /* [8:0]: 0_0000_0011    */
#define KPC_VAL_SHORT_INTERVAL                          (0x28)      /* 120 x 250us = 30ms    */

struct k3v2_keypad {
	struct input_dev *input_dev;
	void __iomem     *base;
	int				 irq;
	struct clk       *clk;
	int               rows;
	int               cols;
	int               row_shift;
	unsigned short    keycodes[KPC_MAX_ROWS * KPC_MAX_COLS];      /* Used for keymap*/
	unsigned char     scancode_state[KPC_MAX_ROWS];               /* Used for result of keypad scan*/
	uint16_t          keycode_state[KPC_MAX_ROWS * 2];            /* Used for store all keycode state*/
};


static struct keypad_remap *keypad_long_remap;
static struct keypad_remap_state *g_long_remap_state;

static int k3v2_keypad_open(struct input_dev *dev)
{
	struct k3v2_keypad *keypad = input_get_drvdata(dev);
	struct iomux_block *block = NULL;
	int ret = 0;

	//disable_irq(keypad->irq);

	if(keypad == NULL){
		printk(KERN_ERR "get invalid keypad pointer\n");
		return -EINVAL;
	}

	block = iomux_get_block("block_kpc");
	if (!block) {
		dev_warn(&keypad->input_dev->dev, "Failed to get KPC GPIO BLOCK\n");
		ret = -EINVAL;
	}
	/*Clean interrupt*/
	writel(0x1, keypad->base + KPC_INT_CLR_OFFSET);
	writel(0x1, keypad->base + KPC_RELEASE_INT_CLR_OFFSET);
	/*config KPC_CONTROL REG*/
	writel(KPC_VAL_CTLREG, keypad->base + KPC_CONTROL_OFFSET);
	/*config KPC_INTERVAL_SHORT REG*/
	writel(KPC_VAL_SHORT_INTERVAL, keypad->base + KPC_INTERVAL_SHORT_OFFSET);

	enable_irq(keypad->irq);
	return ret;
}

static void k3v2_keypad_close(struct input_dev *dev)
{
	struct k3v2_keypad *keypad = input_get_drvdata(dev);

	if(keypad == NULL){
		printk(KERN_ERR "get invalid keypad pointer\n");
		return;
	}

	disable_irq(keypad->irq);
}

/* Update the new_keycode_state*/
static void k3v2_keypad_update_keycode(struct k3v2_keypad *keypad,
	unsigned char *new_scancode_state,
	uint16_t *new_keycode_state)
{
	int row = 0;
	int col = 0;
	int r = 0;
	int c = 0;
	int index = 0;
	uint8_t keycode = 0;

	for (row = 0; row < KPC_MAX_ROWS; row++) {
		for (col = 0; col < KPC_MAX_COLS; col++) {
			if (new_scancode_state[row] & (1 << col)) {
				index = MATRIX_SCAN_CODE(row, col, keypad->row_shift);
				keycode = keypad->keycodes[index];
				c = keycode & 0x0F;
				r = keycode >> 4;
				new_keycode_state[r] |= (1 << c);
			}
		}
	}
}

/*Remap long-press func key or combo keys to target keys.*/
static void k3v2_keypad_remap_keycode(struct k3v2_keypad *keypad,
			struct keypad_remap_state *remap_state,
			uint16_t      *new_keycode_state)
{
	int i = 0;
	int j = 0;

	unsigned long current_time = jiffies_to_msecs(jiffies);

	if (!remap_state)
		return;

	for (i = 0; i < keypad_long_remap->count; i++) {

		int down_num = 0;
		uint16_t keycode;
		unsigned char key_down_state = 0;
		const struct keypad_remap_item *item = &keypad_long_remap->items[i];
		struct keypad_remap_state *state = &remap_state[i];

		for (j = 0; j < item->keynum; j++) {
			keycode = item->keycodes[j];
			if (KEYPAD_CHECK_KEYCODE(new_keycode_state, keycode) != 0) {
				key_down_state |= (1 << j);
				down_num++;
			}
		}
		/*the number of down keys are enough to remap.*/
		if (down_num >= item->keynum) {
			if (item->keynum > 1) {
				/*clean all mapping keys in new_keycode_state*/
				for (j = 0; j < item->keynum; j++) {
					keycode = item->keycodes[j];
					KEYPAD_CLR_KEYCODE(new_keycode_state, keycode);
				}
				/*set the remapped keycode*/
				keycode = item->target_keycode;
				KEYPAD_SET_KEYCODE(new_keycode_state, keycode);
				state->pending = false;
				state->remapped = true;
			} else {
				/*start pending period*/
				if ((state->pending == false) && (state->remapped == false)) {
					state->pending = true;
					state->time = current_time + item->delay;
				}
				KEYPAD_CLR_KEYCODE(new_keycode_state, item->keycodes[0]);
				/*if pending, then check if it is timeout*/
				if (state->pending == true) {
					/*it behinds timeout, then set the remapped keycode*/
					if (current_time >= state->time) {
						keycode = item->target_keycode;
						KEYPAD_SET_KEYCODE(new_keycode_state, keycode);
						state->pending = false;
						state->remapped = true;
					}
				} else if (state->remapped == true) {
					keycode = item->target_keycode;
					KEYPAD_SET_KEYCODE(new_keycode_state, keycode);
				}
			}
		}
		/*keys down, but not enough number for combo keys*/
		else if (down_num > 0) {
			if ((state->remapped == true) || (state->pending == false)
				|| (current_time < state->time)) {

				if ((state->pending == false) && (state->remapped == false)) {
					state->pending = true;
					state->time = current_time + item->delay;
				}

				for (j = 0; j < item->keynum; j++) {
					keycode = item->keycodes[j];
					KEYPAD_CLR_KEYCODE(new_keycode_state, keycode);
				}
			}
		} else {
			/*All keys are up.
			 *If pending, set the cleaned remapping keys back.
			 *Then call timer to report again.*/
			if (state->pending) {
				for (j = 0; j < item->keynum; j++) {
					if (((state->down_state) & (1 << j)) != 0) {
						keycode = item->keycodes[j];
						input_report_key(keypad->input_dev, keycode, 1);
						input_report_key(keypad->input_dev, keycode, 0);
						input_sync(keypad->input_dev);
					}
				}
				state->pending = false;
			}
			state->remapped = false;
		}
		/*save keys*/
		state->down_state = key_down_state;
	}
}

static void k3v2_keypad_report_keycode(struct k3v2_keypad *keypad, uint16_t *new_keycode_state)
{
	int row = 0;
	int col = 0;
	unsigned int keycode = 0;
	unsigned int  pressed = 0;
	uint16_t changed = 0;

	for (row = 0; row < KPC_MAX_ROWS * 2; row++) {
		changed = keypad->keycode_state[row] ^ new_keycode_state[row];
		if (0 == changed)
			continue;
		for (col = 0; col < KPC_MAX_COLS * 2; col++) {
			if (changed & (1 << col)) {
				keycode = (row << 4) | (col & 0x0F);
				pressed = (new_keycode_state[row] & (1 << col)) >> col;
				dev_dbg(&keypad->input_dev->dev, 
					"row = %d, col = %d, keycode = %d, press = %d\n", row, col, keycode, pressed);
				input_report_key(keypad->input_dev, keycode, pressed);
			}
		}
	}
	input_sync(keypad->input_dev);
}


static irqreturn_t k3v2_keypad_irq_handler(int irq, void *dev_id)
{
	struct k3v2_keypad  *keypad       = dev_id;
	unsigned int         reg          = 0;
	unsigned int         key_num      = 0;
	unsigned int         key_val      = 0;

	int                  row          = 0;
	int                  col          = 0;
	int                  i            = 0;
#ifndef ADVANCED_FUNCTION_SUPPORT
	unsigned int  changed;
	unsigned int  pressed;
	unsigned int  val = 0;
#endif /* #ifndef ADVANCED_FUNCTION_SUPPORT */
	unsigned char new_scancode_state[ARRAY_SIZE(keypad->scancode_state)];
	uint16_t      new_keycode_state[ARRAY_SIZE(keypad->keycode_state)];
	
	
	memset(new_scancode_state, 0, sizeof(new_scancode_state));
	memset(new_keycode_state, 0, sizeof(new_keycode_state));

	reg = readl(keypad->base + KPC_INT_STATUS_OFFSET);
	key_num =  (reg & KEY_NUM_BITMASK)   >> KEY_NUM_BITPOS;
	key_val =  (reg & KEY_VALUE_BITMASK) >> KEY_VALUE_BITPOS;	
	
	
	for (i = 0; i < key_num; i++) {
		row = (key_val >> (i*KPC_BIT_PER_KEY)) & 0x7;
		col = (key_val >> (i*KPC_BIT_PER_KEY + KPC_BIT_PER_KEYROW)) & 0x7;
		new_scancode_state[row] |= (1 << col);
	}

#ifndef ADVANCED_FUNCTION_SUPPORT
	for (row = 0; row < keypad->rows; row++) {
		changed = new_scancode_state[row] ^ keypad->scancode_state[row];
		if (!changed)
			continue;

		for (col = 0; col < keypad->cols; col++) {
			if (changed & (1 << col)) {
				val = MATRIX_SCAN_CODE(row, col, keypad->row_shift);
				pressed = (new_scancode_state[row] & (1 << col)) >> col;
				input_report_key(keypad->input_dev, keypad->keycodes[val], pressed);
				dev_dbg(&keypad->input_dev->dev, 
					"key_num = %d, row = %d, col = %d, press = %d\n", key_num, row, col, pressed);
			}
		}
	}
	input_sync(keypad->input_dev);
	memcpy(keypad->scancode_state, new_scancode_state, sizeof(new_scancode_state));
#else
	k3v2_keypad_update_keycode(keypad, new_scancode_state, new_keycode_state);
	k3v2_keypad_remap_keycode(keypad, g_long_remap_state, new_keycode_state);
	k3v2_keypad_report_keycode(keypad, new_keycode_state);
	memcpy(keypad->scancode_state, new_scancode_state, sizeof(new_scancode_state));
	memcpy(keypad->keycode_state, new_keycode_state, sizeof(new_keycode_state));
#endif

	/*Clean interrupt*/
	writel(0x1, keypad->base + KPC_INT_CLR_OFFSET);
	writel(0x1, keypad->base + KPC_RELEASE_INT_CLR_OFFSET);
	return IRQ_HANDLED;
}

static int __devinit k3v2_keypad_probe(struct platform_device* pdev)
{
	const struct k3v2_keypad_platdata *platdata;
	const struct matrix_keymap_data   *keymap_data;
	struct k3v2_keypad                *keypad;
	struct resource                   *res;
	struct input_dev                  *input_dev;
	int      err;
	

	platdata = pdev->dev.platform_data;
	if (!platdata) {
		dev_err(&pdev->dev, "platform data is null!\n");
		return -EINVAL;
	}

	keymap_data = platdata->keymap_data;
	if (!keymap_data) {
		dev_err(&pdev->dev, "keymap data is null!\n");
		return -EINVAL;
	}

	if (!platdata->rows || platdata->rows > KPC_MAX_ROWS) {
		dev_err(&pdev->dev, "keypad rows is null or bigger than the max rows!\n");
		return -EINVAL;
	}

	if (!platdata->cols || platdata->cols > KPC_MAX_COLS) {
		dev_err(&pdev->dev, "keypad cols is null or bigger than the max cols!\n");
		return -EINVAL;
	}

	keypad = kzalloc(sizeof(struct k3v2_keypad) , GFP_KERNEL);
	if(!keypad) {
		dev_err(&pdev->dev, "Failed to allocate struct k3v2_keypad!\n");
		err = -ENOMEM;
	}
	
	input_dev = input_allocate_device();
	if (!input_dev) {
		dev_err(&pdev->dev, "Failed to allocate struct k3v2_keypad or input_dev!\n");
		err = -ENOMEM;
		goto err_alloc_input_device;
	}

	
	keypad_long_remap = platdata->keypad_remap;
	if (!keypad_long_remap) {
		dev_err(&pdev->dev, "Failed to get_keypad_long_remap!\n");
		err = -EINVAL;
		goto err_get_keypad_remap;
	}

	g_long_remap_state = (struct keypad_remap_state *)kzalloc(
		keypad_long_remap->count * sizeof(struct keypad_remap_state), GFP_KERNEL);
	if (!g_long_remap_state) {
		dev_err(&pdev->dev, "Failed to allocate g_long_remap_state!\n");
		err = -ENOMEM;
		goto err_alloc_remap_state;
	}
	/*Get REG_BASE_KPC address*/
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "Failed to get KPC base address!\n");
		err = -ENODEV;
		goto err_get_base;
	}

	keypad->base = ioremap(res->start, resource_size(res));
	if (!keypad->base) {
		dev_err(&pdev->dev, "Failed to remap KPC base address!\n");
		err = -EBUSY;
		goto err_ioremap_base;
	}

	/*get clock*/
	keypad->clk = clk_get(&pdev->dev, NULL);
	if (IS_ERR(keypad->clk)) {
		dev_err(&pdev->dev, "Failed to get clk_kpc!\n");
		err = -ENODEV;
		goto err_get_clk;
	}
	err = clk_set_rate(keypad->clk, KPC_CLK_RATE);
	if (err < 0) {
		dev_err(&pdev->dev, "Failed to set clk rate!\n");
		goto err_set_clk;
	}
	clk_enable(keypad->clk);

	keypad->input_dev = input_dev;
	keypad->row_shift = platdata->row_shift;
	keypad->rows = platdata->rows;
	keypad->cols = platdata->cols;

	input_dev->name = pdev->name;
	input_dev->id.bustype = BUS_HOST;
	input_dev->dev.parent = &pdev->dev;
	input_set_drvdata(input_dev, keypad);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_SYN, input_dev->evbit);
	input_dev->keycode = keypad->keycodes;
	input_dev->keycodesize = sizeof(keypad->keycodes[0]);
	input_dev->open = k3v2_keypad_open;
	input_dev->close = k3v2_keypad_close;

	matrix_keypad_build_keymap(keymap_data, keypad->row_shift,
			input_dev->keycode, input_dev->keybit);

	keypad->irq = platform_get_irq(pdev, 0);
	if (keypad->irq < 0) {
		dev_err(&pdev->dev, "Failed to get irq!\n");
		err = keypad->irq;
		goto err_get_irq;

	}

	err = request_irq(keypad->irq, k3v2_keypad_irq_handler, IRQF_NO_SUSPEND, pdev->name, keypad);
	if (err) {
		dev_err(&pdev->dev, "Failed to request interupt handler!\n");
		goto err_request_irq;
	}

	disable_irq(keypad->irq);

	err = input_register_device(keypad->input_dev);
	if (err) {
		dev_err(&pdev->dev, "Failed to register input device!\n");
		goto err_register_device;
	}

	device_init_wakeup(&pdev->dev, true);
	platform_set_drvdata(pdev, keypad);
	dev_info(&pdev->dev, "k3v2 keypad probe successfully!\n");
	return 0;

err_register_device:
	free_irq(keypad->irq, keypad);
err_request_irq:
err_get_irq:
	clk_disable(keypad->clk);
err_set_clk:
	clk_put(keypad->clk);
err_get_clk:
	iounmap(keypad->base);
err_ioremap_base:
err_get_base:
	kfree(g_long_remap_state);
err_alloc_remap_state:
err_get_keypad_remap:
	input_free_device(input_dev);
err_alloc_input_device:
	kfree(keypad);

	pr_info("K3v2 keypad probe failed! ret = %d\n", err);
	return err;
}

static int __devexit k3v2_keypad_remove(struct platform_device* pdev)
{
	struct k3v2_keypad *keypad =  platform_get_drvdata(pdev);

	if(keypad == NULL){
		printk(KERN_ERR "get invalid keypad pointer\n");
		return -EINVAL;
	}

	free_irq(keypad->irq, keypad);
	iounmap(keypad->base);

	clk_disable(keypad->clk);
	clk_put(keypad->clk);

	input_unregister_device(keypad->input_dev);
	platform_set_drvdata(pdev, NULL);
	kfree(keypad);

	if (!g_long_remap_state)
		kfree(g_long_remap_state);

	return 0;
}

#ifdef CONFIG_PM
static int k3v2_keypad_suspend(struct platform_device *pdev, pm_message_t state)
{
	pr_info("[keypad]suspend successfully\n");
	return 0;
}

static int k3v2_keypad_resume(struct platform_device *pdev)
{
	pr_info("[keypad]resume successfully\n");
	return 0;
}
#endif


struct platform_driver k3v2_keypad_driver = {
	.probe = k3v2_keypad_probe,
	.remove = __devexit_p(k3v2_keypad_remove),
	.driver = {
		.name = "k3_keypad",
		.owner = THIS_MODULE,
	},
	#ifdef CONFIG_PM
	.suspend = k3v2_keypad_suspend,
	.resume = k3v2_keypad_resume,
	#endif
};

static int __init k3v2_keypad_init(void)
{
	pr_info("k3v2 keypad init!\n");
	return platform_driver_register(&k3v2_keypad_driver);
}

static void __exit k3v2_keypad_exit(void)
{
	platform_driver_unregister(&k3v2_keypad_driver);
}

module_init(k3v2_keypad_init);
module_exit(k3v2_keypad_exit);
MODULE_AUTHOR("Hisilicon K3 Driver Group");
MODULE_DESCRIPTION("K3v2 keypad platform driver");
MODULE_LICENSE("GPL");
