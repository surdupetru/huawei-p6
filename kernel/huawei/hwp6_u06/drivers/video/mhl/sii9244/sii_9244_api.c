/*
* SiIxxxx <Firmware or Driver>
*
* Copyright (C) 2011 Silicon Image Inc.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation version 2.
*
* This program is distributed .as is. WITHOUT ANY WARRANTY of any
* kind, whether express or implied; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR
* PURPOSE.  See the
* GNU General Public License for more details.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/wakelock.h>
#include <linux/mhl/mhl.h>
#include <linux/mux.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#ifdef SHARE_SWITCHER_WITH_USB
#include <linux/switch_usb.h>
#endif

#include "sii_9244_api.h"
#include "si_mhl_tx_api.h"
#include "si_mhl_defs.h"
#include "sii_reg_access.h"
#include "si_drv_mhl_tx.h"

#include "../../../../include/hsad/config_mgr.h"
#include <linux/string.h>


/* interrupt mode or polling mode for 9244 driver */
#define SiI9244DRIVER_INTERRUPT_MODE   1
#define APP_DEMO_RCP_SEND_KEY_CODE (0x41)
/* false: 0 = vbus output on; true: 1 = vbus output off;*/
bool_tt vbusPowerState = true;

static unsigned short Sii9244_key2code[] = {
	KEY_MENU,/* 139 */
	KEY_SELECT,/* 0x161 */
	KEY_UP,/* 103 */
	KEY_LEFT,/* 105 */
	KEY_RIGHT,/* 106 */
	KEY_DOWN,/* 108 */
	KEY_EXIT,/* 174 */
	KEY_1, /* 0x02 */
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9, /* 0x10 */
	KEY_0,
	KEY_DOT,/* 52 */
	KEY_ENTER,/* 28 */
	KEY_CLEAR,/* 0x163 *//* DEL */
	KEY_SOUND,/* 0x213 */
	KEY_PLAY,/* 207 */
	KEY_PAUSE,/* 119 */
	KEY_STOP,/* 128 */
	KEY_FASTFORWARD,/* 208 */
	KEY_REWIND,/* 168 */
	KEY_EJECTCD,/* 161 */
	KEY_FORWARD,/* 159 */
	KEY_BACK,/* 158 */
	KEY_PLAYCD,/* 200 */
	KEY_PAUSECD,/* 201 */
	KEY_STOP,/* 128 */
	KEY_F1,/* 59 */
	KEY_F2,/* 60 */
	KEY_F3,
	KEY_F4,
	KEY_F5,/* 63 */
	KEY_CHANNELUP, /* 0x192 */
	KEY_CHANNELDOWN, /* 0x193 */
	KEY_MUTE, /* 113 */
	KEY_PREVIOUS, /* 0x19c */
	KEY_VOLUMEDOWN, /* 114 */
	KEY_VOLUMEUP, /* 115 */
	KEY_RECORD, /* 167 */
	KEY_REPLY,
	KEY_PLAYPAUSE,
	KEY_PREVIOUSSONG, /* add  */
	KEY_BACKSPACE, /* DEL */
};
struct Sii9244_data {
	struct i2c_client *client;
	struct input_dev *input;
	unsigned short keycodes[ARRAY_SIZE(Sii9244_key2code)];
	struct wake_lock wake_lock;
};

struct Sii9244_data *Sii9244;

struct work_struct *sii9244work;
extern uint8_t fwPowerState;

uint8_t PAGE_0_0X72;
uint8_t PAGE_1_0X7A;
uint8_t PAGE_2_0X92;
uint8_t PAGE_CBUS_0XC8;

static void init_PAGE_values(void)
{
	if (get_mhl_ci2ca_value()) {
		PAGE_0_0X72 = 0x76;
		PAGE_1_0X7A = 0x7E;
		PAGE_2_0X92 = 0x96;
		PAGE_CBUS_0XC8 = 0xCC;
	} else {
		PAGE_0_0X72 = 0x72;
		PAGE_1_0X7A = 0x7A;
		PAGE_2_0X92 = 0x92;
		PAGE_CBUS_0XC8 = 0xC8;
	}
}

/*
AppVbusControl

This function or macro is invoked from MhlTx driver to ask application to
control the VBUS power. If powerOn is sent as non-zero, one should assume
peer does not need power so quickly remove VBUS power.

if value of "powerOn" is 0, then application must turn the VBUS power on
within 50ms of this call to meet MHL specs timing.

Application module must provide this function.
*/
#if (VBUS_POWER_CHK == ENABLE)
void AppVbusControl(bool_tt powerOn)
{
	if (powerOn) {
		/*pinMHLTxVbus_CTRL = 1;*/
		MHLSinkOrDonglePowerStatusCheck();
		TX_API_PRINT(("Mhl:API: Peer's POW bit is set. Turn the VBUS power OFF here.\n"));
	} else {
		/*pinMHLTxVbus_CTRL = 0;*/
		TX_API_PRINT(("Mhl:API: Peer's POW bit is cleared. Turn the VBUS power ON here.\n"));
	}
}
#endif

/******************************************************************************
******************************************************************************
************************** Linux platform related ***********************************
******************************************************************************
******************************************************************************/

#define MHL_DRIVER_NAME "sii9244drv"

/***** public type definitions ***********************************************/

typedef struct {
	struct task_struct *pTaskStruct;
	/* event data wait for retrieval*/
	uint8_t pendingEvent;
	/* by user mode application*/
	uint8_t pendingEventData;

} MHL_DRIVER_CONTEXT_T, *PMHL_DRIVER_CONTEXT_T;

/***** global variables ********************************************/

MHL_DRIVER_CONTEXT_T gDriverContext;

struct i2c_client *mhl_Sii9244_page0;
struct i2c_client *mhl_Sii9244_page1;
struct i2c_client *mhl_Sii9244_page2;
struct i2c_client *mhl_Sii9244_cbus;

static bool_tt match_id(const struct i2c_device_id *id, const struct i2c_client *client)
{
	if (strcmp(client->name, id->name) == 0)
		return true;

	return false;
}

/*****************************************************************************/
/**
* @brief Wait for the specified number of milliseconds to elapse.
*
*****************************************************************************/
void HalTimerWait(uint16_t m_sec)
{
	unsigned long time_usec = m_sec * 1000;
	usleep_range(time_usec, time_usec);
}

/* I2C functions used by the driver. */
uint8_t I2C_ReadByte(uint8_t SlaveAddr, uint8_t RegAddr)
{
	uint8_t ReadData = 0;

	if (SlaveAddr == PAGE_0_0X72)
		ReadData = i2c_smbus_read_byte_data(mhl_Sii9244_page0, RegAddr);
	else if (SlaveAddr == PAGE_1_0X7A)
		ReadData = i2c_smbus_read_byte_data(mhl_Sii9244_page1, RegAddr);
	else if (SlaveAddr == PAGE_2_0X92)
		ReadData = i2c_smbus_read_byte_data(mhl_Sii9244_page2, RegAddr);
	else if (SlaveAddr == PAGE_CBUS_0XC8)
		ReadData = i2c_smbus_read_byte_data(mhl_Sii9244_cbus, RegAddr);

	return ReadData;
}

void I2C_WriteByte(uint8_t SlaveAddr, uint8_t RegAddr, uint8_t Data)
{
	if (SlaveAddr == PAGE_0_0X72)
		i2c_smbus_write_byte_data(mhl_Sii9244_page0, RegAddr, Data);
	else if (SlaveAddr == PAGE_1_0X7A)
		i2c_smbus_write_byte_data(mhl_Sii9244_page1, RegAddr, Data);
	else if (SlaveAddr == PAGE_2_0X92)
		i2c_smbus_write_byte_data(mhl_Sii9244_page2, RegAddr, Data);
	else if (SlaveAddr == PAGE_CBUS_0XC8)
		i2c_smbus_write_byte_data(mhl_Sii9244_cbus, RegAddr, Data);
}

void input_keycode(unsigned short key_code)
{
	struct input_dev *input = Sii9244->input;

	input_report_key(input, key_code, 1);
	input_sync(input);
	msleep(200);
	if (key_code == KEY_FASTFORWARD || key_code == KEY_REWIND) {
		msleep(3000);
		input_report_key(input, key_code, 0);
		input_sync(input);
		TX_API_PRINT(("Mhl:API:key_count =%d\n", key_code));
	} else {
		input_report_key(input, key_code, 0);
		input_sync(input);
		TX_API_PRINT(("Mhl:API:key_count =%d\n", key_code));
	}
}

#ifdef SHARE_SWITCHER_WITH_USB
static bool if_mhl_cable_plugged_in = false;

void usb_switch_to_mhl(bool if_switch_to_MHL)
{
	if_mhl_cable_plugged_in = if_switch_to_MHL;
	TX_API_PRINT(("Mhl:API:usb_switch_to_mhl: if_switch_to_MHL =%d\n", if_switch_to_MHL));

	if (if_switch_to_MHL)
	{
		switch_usb_set_state_through_fs(USB_TO_MHL);
	}
	else
	{
		switch_usb_set_state_through_fs(USB_TO_AP);
	}
	return;
}

bool GetMhlCableConnect(void)
{
	return if_mhl_cable_plugged_in;
}
#endif


int SiiMhlReset(void)
{
	struct mhl_platform_data *pdata = NULL;
	pdata = mhl_Sii9244_page0->dev.platform_data;
	if (!pdata) {
		TX_API_PRINT((KERN_ERR "%s:%d:pdata is NULL!\n", __func__, __LINE__));
		return -ENOMEM;
	}
	gpio_set_value(pdata->gpio_reset, 0);
	msleep(100);
	gpio_set_value(pdata->gpio_reset, 1);
	return 0;
}

#ifdef SiI9244DRIVER_INTERRUPT_MODE
static void work_queue(struct work_struct *work)
{
	uint8_t Int_count = 0;

	wake_lock(&Sii9244->wake_lock);
	enable_irq(mhl_Sii9244_page0->irq);

	for (Int_count = 0; Int_count < 10; Int_count++) {
		SiiMhlTxDeviceIsr();

		if (POWER_STATE_D3 == fwPowerState)
			break;

		msleep(50);
	}
	wake_unlock(&Sii9244->wake_lock);
}

static irqreturn_t Sii9244_mhl_interrupt(int irq, void *dev_id)
{
	disable_irq_nosync(irq);

	TX_API_PRINT(("Mhl:API:The most of sii902xA interrupt work will be done by following tasklet..\n"));

	schedule_work(sii9244work);

	return IRQ_HANDLED;
}
#else
static int SiI9244_mhl_loop(void *nothing)
{
	TX_API_PRINT(("Mhl:API:%s EventThread starting up\n", MHL_DRIVER_NAME));

	while (true) {
		/* Look for any events that might have occurred. */
		/* SiiMhlTxGetEvents( &event, &eventParameter ); */
		SiiMhlTxDeviceIsr();
		msleep(20);
	}
	return 0;
}

/*****************************************************************************/
/**
* @brief Start driver's event monitoring thread.
*
*****************************************************************************/
void StartEventThread(void)
{
	gDriverContext.pTaskStruct = kthread_run(SiI9244_mhl_loop, &gDriverContext, MHL_DRIVER_NAME);
}

/*****************************************************************************/
/**
* @brief Stop driver's event monitoring thread.
*
*****************************************************************************/
void  StopEventThread(void)
{
	kthread_stop(gDriverContext.pTaskStruct);
}
#endif

static struct i2c_device_id mhl_Sii9244_idtable[] = {
	{"mhl_Sii9244_page0", 0},
	{"mhl_Sii9244_page1", 0},
	{"mhl_Sii9244_page2", 0},
	{"mhl_Sii9244_cbus", 0},
};

/*
* i2c client ftn.
*/
static int __devinit mhl_Sii9244_probe(struct i2c_client *client,
									   const struct i2c_device_id *dev_id)
{
	int ret = 0;
	uint8_t pollIntervalMs = MONITORING_PERIOD;
	int sii9244_ID_low = 0;
	int sii9244_ID_high = 0;
	struct mhl_platform_data *pdata = NULL;
	struct iomux_pin *pinTemp = NULL;
	int i = 0;
	struct input_dev *input = NULL;

	TX_API_PRINT((KERN_INFO "Mhl:API:%s:%d:\n", __func__, __LINE__));
	init_PAGE_values();

	if (match_id(&mhl_Sii9244_idtable[0], client)) {
		mhl_Sii9244_page0 = client;
		dev_info(&client->adapter->dev, "Mhl:API:attached %s "
			"into i2c adapter successfully\n", dev_id->name);
	} else if (match_id(&mhl_Sii9244_idtable[1], client)) {
		mhl_Sii9244_page1 = client;
		dev_info(&client->adapter->dev, "Mhl:API:attached %s "
			"into i2c adapter successfully \n", dev_id->name);
	} else if (match_id(&mhl_Sii9244_idtable[2], client)) {
		mhl_Sii9244_page2 = client;
		dev_info(&client->adapter->dev, "Mhl:API:attached %s "
			"into i2c adapter successfully \n", dev_id->name);
	} else if (match_id(&mhl_Sii9244_idtable[3], client)) {
		mhl_Sii9244_cbus = client;
		dev_info(&client->adapter->dev, "Mhl:API:attached %s "
			"into i2c adapter successfully\n", dev_id->name);
	} else {
		dev_err(&client->adapter->dev, "Mhl:API:invalid i2c adapter: can not found dev_id matched\n");
		return -EIO;
	}

	if (mhl_Sii9244_page0 != NULL
		&& mhl_Sii9244_page1 != NULL
		&& mhl_Sii9244_page2 != NULL
		&& mhl_Sii9244_cbus != NULL) {

			/* Announce on RS232c port.*/
			TX_API_PRINT((KERN_INFO "Mhl:API:SiI9244 Linux Driver V1.22 \n"));

			pinTemp = iomux_get_pin(MHL_PIN);
			if (!pinTemp) {
				TX_API_PRINT((KERN_ERR "Mhl:API:iomux_get_pin is error\n"));
				return -EIO;
			}

			ret = pinmux_setpullupdown(pinTemp, PULLUP);
			if (ret < 0) {
				TX_API_PRINT((KERN_ERR "Mhl:API:%s:%d:pinmux_setpullupdown failed\n", __func__, __LINE__));
				return -EIO;
			}
			pdata = mhl_Sii9244_page0->dev.platform_data;

			if (!pdata) {
				TX_API_PRINT((KERN_ERR "Mhl:API:%s:%d:pdata is NULL!\n", __func__, __LINE__));
				ret =  -ENOMEM;
				goto mhl_failed_1;
			}
			if (gpio_request(pdata->gpio_wake_up, "mhl_wake_up") < 0) {
				TX_API_PRINT((KERN_ERR "Mhl:API:%s:%d:mhl_wake_up GPIO request failed\n", __func__, __LINE__));
				ret =  -ENOMEM;
				goto mhl_failed_1;
			}

			if (gpio_request(pdata->gpio_int, "mhl_int") < 0) {
				TX_API_PRINT((KERN_ERR "Mhl:API:%s:%d:mhl_int GPIO request failed\n", __func__, __LINE__));
				ret =  -ENOMEM;
				goto mhl_failed_2;
			}

			if (gpio_request(pdata->gpio_reset, "mhl_reset") < 0) {
				TX_API_PRINT((KERN_ERR "Mhl:API:%s:%d:mhl_reset GPIO request failed\n", __func__, __LINE__));
				ret =  -ENOMEM;
				goto mhl_failed_3;
			}

			ret = gpio_direction_output(pdata->gpio_wake_up, 0);
			if (ret < 0) {
				TX_API_PRINT((KERN_ERR "Mhl:API:%s:%d:mhl_wake_up GPIO direction failed\n", __func__, __LINE__));
				ret =  -ENOMEM;
				goto mhl_failed_4;
			}

			ret = gpio_direction_input(pdata->gpio_int);
			if (ret < 0) {
				TX_API_PRINT((KERN_ERR "Mhl:API:%s:%d:mhl_gpio_int GPIO direction failed\n", __func__, __LINE__));
				ret =  -ENOMEM;
				goto mhl_failed_4;
			}

			ret = gpio_direction_output(pdata->gpio_reset, 1);
			if (ret < 0) {
				TX_API_PRINT((KERN_ERR "Mhl:API:%s:%d:mhl_gpio_reset GPIO direction failed\n", __func__, __LINE__));
				ret =  -ENOMEM;
				goto mhl_failed_4;
			}
			/* Initialize the registers as required. Setup firmware vars.*/

			sii9244_ID_low = I2C_ReadByte(PAGE_0_0X72, 0x02);

			sii9244_ID_high = I2C_ReadByte(PAGE_0_0X72, 0x03);

			if ((sii9244_ID_low != 0x34) || (sii9244_ID_high != 0x92)) {
				TX_API_PRINT((KERN_ERR "Mhl:API:the sii9244 is not exist\n"));
				ret = -EIO;
				goto mhl_failed_4;
			}

			Sii9244 = kzalloc(sizeof(struct Sii9244_data), GFP_KERNEL);
			if (!Sii9244) {
				dev_err(&client->dev, "Mhl:API:insufficient memory to alloc sii9244\n");
				ret = -ENOMEM;
				goto mhl_failed_4;
			}
			input = input_allocate_device();
			if (!input) {
				dev_err(&client->dev, "Mhl:API:insufficient memory to alloc input device\n");
				ret = -ENOMEM;
				goto mhl_failed_5;
			}
			Sii9244->client = mhl_Sii9244_page0;
			Sii9244->input = input;
			wake_lock_init(&Sii9244->wake_lock, WAKE_LOCK_SUSPEND, "mhl wake lock");

			dev_set_drvdata(&input->dev, Sii9244);
			input->name = "Sii9244";
			input->id.bustype = BUS_I2C;

			input->keycode = Sii9244->keycodes;
			input->keycodesize = sizeof(Sii9244->keycodes[0]);
			input->keycodemax = ARRAY_SIZE(Sii9244_key2code);

			__set_bit(EV_KEY, input->evbit);
			__clear_bit(EV_REP, input->evbit);

			for (i = 0; i < ARRAY_SIZE(Sii9244_key2code); i++) {
				Sii9244->keycodes[i] = Sii9244_key2code[i];
				input_set_capability(Sii9244->input, EV_KEY, Sii9244->keycodes[i]);
			}
			__clear_bit(KEY_RESERVED, input->keybit);

			SiiMhlTxInitialize(pollIntervalMs);

#ifdef SiI9244DRIVER_INTERRUPT_MODE
			sii9244work = kzalloc(sizeof(*sii9244work), GFP_ATOMIC);
			if (!sii9244work) {
				ret = -ENOMEM;
				goto mhl_failed_6;
			}

			INIT_WORK(sii9244work, work_queue);

			mhl_Sii9244_page0->irq = gpio_to_irq(pdata->gpio_int);

			ret = request_irq(mhl_Sii9244_page0->irq, Sii9244_mhl_interrupt,
				/* modified by l00196665, for otg host */
				IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND,
				mhl_Sii9244_page0->name, mhl_Sii9244_page0);
			if (ret) {
				TX_API_PRINT((KERN_ERR "Mhl:API:%s:%d:could not request irq %d, status %d\n", __func__, __LINE__, gpio_to_irq(client->irq), ret));
				ret = -ENOMEM;
				goto mhl_failed_7;
			} else {
				/* enable_irq_wake(gpio_to_irq(pdata->gpio_int)); */
				TX_API_PRINT((KERN_INFO "Mhl:API:%s:%d:request Sii9244 interrupt successed\n", __func__, __LINE__));
			}
#else
			StartEventThread(); /* begin monitoring for events */
#endif
			ret = input_register_device(Sii9244->input);
			if (ret) {
				dev_err(&client->dev, "Mhl:API:Failed to register input device\n");
				goto mhl_failed_8;
			}
	}
	return ret;
mhl_failed_8:
#ifdef SiI9244DRIVER_INTERRUPT_MODE
	free_irq(gpio_to_irq(pdata->gpio_int), mhl_Sii9244_page0);
#endif
mhl_failed_7:
#ifdef SiI9244DRIVER_INTERRUPT_MODE
	kfree(sii9244work);
	sii9244work = NULL;
#endif
mhl_failed_6:
	dev_set_drvdata(&input->dev, NULL);
	input_free_device(input);
mhl_failed_5:
	kfree(Sii9244);
mhl_failed_4:
	gpio_free(pdata->gpio_reset);
mhl_failed_3:
	gpio_free(pdata->gpio_int);
mhl_failed_2:
	gpio_free(pdata->gpio_wake_up);
mhl_failed_1:
	return ret;
}

static void mhl_Sii9244_shutdown(struct i2c_client *client)
{
	struct mhl_platform_data *pdata = NULL;

	printk("[%s] +\n", __func__);

	if (false == match_id(&mhl_Sii9244_idtable[3], client)) {
		return;
	}
	pdata = mhl_Sii9244_page0->dev.platform_data;
	if (!pdata) {
		TX_API_PRINT((KERN_ERR "Mhl:API:%s:%d:pdata is NULL!\n", __func__, __LINE__));
		printk("[%s] -\n", __func__);
		return;
	}
#ifdef SiI9244DRIVER_INTERRUPT_MODE
	free_irq(gpio_to_irq(pdata->gpio_int), mhl_Sii9244_page0);
	cancel_work_sync(sii9244work);
#endif
	/* set Sii9244 power state to D3*/
	if (POWER_STATE_D3 != fwPowerState) {
		gpio_direction_output(pdata->gpio_reset, 0);
		msleep(100);
		gpio_direction_output(pdata->gpio_reset, 1);
		SiiMhlTxInitialize(MONITORING_PERIOD);
	}
	printk("[%s] -\n", __func__);

	return;
}
static int mhl_Sii9244_remove(struct i2c_client *client)
{
	struct mhl_platform_data *pdata = NULL;

	if (false == match_id(&mhl_Sii9244_idtable[3], client)) {
		return 0;
	}
	dev_info(&client->adapter->dev, "Mhl:API:detached s5p_mhl "
		"from i2c adapter successfully\n");
	pdata = mhl_Sii9244_page0->dev.platform_data;
	if (!pdata) {
		TX_API_PRINT((KERN_ERR "Mhl:API:%s:%d:pdata is NULL!\n", __func__, __LINE__));
		return -ENOMEM;
	}

	free_irq(gpio_to_irq(pdata->gpio_int), mhl_Sii9244_page0);

	kfree(sii9244work);
	sii9244work = NULL;

	gpio_free(pdata->gpio_reset);
	gpio_free(pdata->gpio_int);
	gpio_free(pdata->gpio_wake_up);
	return 0;
}

static int mhl_Sii9244_suspend(struct i2c_client *cl, pm_message_t mesg)
{
	return 0;
};

static int mhl_Sii9244_resume(struct i2c_client *cl)
{
	return 0;
};


MODULE_DEVICE_TABLE(i2c, mhl_Sii9244_idtable);

static struct i2c_driver mhl_Sii9244_driver = {
	.driver = {
		.name 	= "Sii9244_Driver",
	},
	.id_table	= mhl_Sii9244_idtable,
	.probe		= mhl_Sii9244_probe,
	.remove		= __devexit_p(mhl_Sii9244_remove),
	.shutdown	= mhl_Sii9244_shutdown,
	.suspend	= mhl_Sii9244_suspend,
	.resume		= mhl_Sii9244_resume,
};

static int __init mhl_Sii9244_init(void)
{

	char mhl_chip_name[15];
	bool ret =0;
	ret = get_hw_config_string("MHL/chip_name",mhl_chip_name,15,NULL);
	if(ret)
	{
	       if(strstr(mhl_chip_name,"none"))
	       {
	              printk("sii9244 doesn't support on this production");
	              return 0;
	       }
	}
	//else
	//{
	//      if not set mhl config,default this chip is support on this production
	//}
	return i2c_add_driver(&mhl_Sii9244_driver);
}

static void __exit mhl_Sii9244_exit(void)
{
	i2c_del_driver(&mhl_Sii9244_driver);
}

late_initcall(mhl_Sii9244_init);
module_exit(mhl_Sii9244_exit);

MODULE_VERSION("1.22");
MODULE_AUTHOR("HUAWEI, Inc.");
MODULE_DESCRIPTION("sii9244 transmitter Linux driver");
MODULE_ALIAS("platform:MHL_sii9244");
