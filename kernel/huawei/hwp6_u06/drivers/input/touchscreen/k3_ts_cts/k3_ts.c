#include <linux/module.h> 
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/mm.h>
#include <linux/input.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/time.h>
#include <linux/cpufreq.h>
#include <asm/byteorder.h>
#include <asm/dma.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <mach/hardware.h>
//#include <mach/timer.h>
#include <mach/platform.h>
//#include <mach/hi6421.h>
#include <linux/kthread.h>
#include <linux/earlysuspend.h>
//#include <mach/gpio.h>
//#include <mach/HiPmu.h>

//#include "i2c-k3.h"

#include "k3_ts.h"
#include "gpio.h" 
#include <mach/irqs.h>
//
//#define HISILICON_HAVE_TSLIB   1

#define ctp_I2C_ADDRESS 0x20

#define HI_TS_SAMPLE_RATE  3
#define HI_SAMPLE_DX    60
#define HI_MAX_DELDA    15
#define SUB(x, y)  ((x) > (y) ? ((x) - (y)) : ((y) - (x)))
#define ABS(x)  ((x) >= 0 ? (x) : (-(x)))

///* Touch screen sample 20 times/sec 6.5MHZ/5 */
//#define TIMER_TS_CLK                ((26 * 1000 * 1000) >> 1)

//#define TOUCH_PANEL_GPIO        GPIO_8_7
#define TOUCH_PANEL_GPIO        GPIO_3_6 //GPIO_1_1
#define TOUCH_IC_X              3537
#define TOUCH_IC_Y              5880

#define TOUCH_PANEL_X           640  //TOUCH_IC_X  //320*4
#define TOUCH_PANEL_Y           960  //TOUCH_IC_Y  //480*4

#define TOUCH_FIRST_POINT_X     (TOUCH_PANEL_X/40)   // 首个点的平滑距离
#define TOUCH_FIRST_POINT_Y     (TOUCH_PANEL_Y/40)

struct hisik3_ts_priv {
	struct i2c_client *client;
	struct input_dev *input;
	struct delayed_work work;
	int irq;
};

static struct i2c_client *ctp_i2c_client = NULL;

extern int hisik3_set_target(struct cpufreq_policy *policy,
				   unsigned int target_freq, unsigned int relation);

static int g_touch_is_poweron = 0, g_touch_is_resume = 0;
//static int g_touch_is_poweron = 1, g_touch_is_resume = 0;
static struct workqueue_struct *g_hisik3_ts_work_queue = NULL;
static void hisik3_ts_worker(struct work_struct *work);
//static void i2c_hisik3_init(Hik3_i2c *i2c);
DECLARE_WORK(g_hisik3_ts_work, hisik3_ts_worker);

static DEFINE_MUTEX(touch_lock);

void ts_lock(void)
{
	mutex_lock(&touch_lock);
}

void ts_release(void)
{
	mutex_unlock(&touch_lock);
}

static int ctp_read_reg8(u8 reg, u8* val, u8 len)
{
	struct i2c_msg msg[2];
	int ret;

	//printk("%s", __func__);

	msg[0].addr = ctp_I2C_ADDRESS;
	msg[0].len = 1;
	msg[0].buf = &reg;
	msg[0].flags = 0;
	//msg[0].flags &= ~I2C_M_RD;
	//msg[0].flags = 0x2;

	msg[1].addr = ctp_I2C_ADDRESS;
	msg[1].len = len;
	msg[1].buf = (u8 *)val;
	//msg[1].flags = 0;
	msg[1].flags = I2C_M_RD;

	if (ctp_i2c_client == NULL || ctp_i2c_client->adapter == NULL) {
		printk("%s: touch adapter error\n", __func__);
		return -1;
	}

	ret = i2c_transfer(ctp_i2c_client->adapter, msg, 2);
	if (ret < 0) {
		  printk("touch error1:%s:reg=%x\n",__func__,reg);
		return ret;
	}
	return 0;
}


static int ctp_write_reg8(u8 reg, u8 val)
{
	struct i2c_msg msg[1];
	int ret;
	u8  data[2];

	data[0] = reg;
	data[1] = val;

	msg[0].addr = ctp_I2C_ADDRESS;
	msg[0].len = 2;
	msg[0].buf = data;
	msg[0].flags = 0;

	if (ctp_i2c_client == NULL || ctp_i2c_client->adapter == NULL) {
		printk("%s:adapter error\n",__func__);
		return -1;
	}

	ret = i2c_transfer(ctp_i2c_client->adapter, msg, 1);
	if(ret < 0){
		printk("error:%s:reg=%x,val=%x\n",__func__,reg,val);
		return ret;
	}

	return 0;
}

/*
int HiPmuCtl(unsigned int cmd ,unsigned char *data,unsigned int len)
{
    return 1;
}*/

static void hisik3_ts_main(void);
static void hisik3_ts_worker(struct work_struct *work)
{
	hisik3_ts_main();	
}

// LDO10 manager
static void power_pm(int flage_on)
{
	//ANLDO_CTRL_t vout10;

	// lock I2C

	if(flage_on){
		if(0 == g_touch_is_poweron){
			//set power vout10 on
//			vout10.bSetOper = 1;
//			vout10.bEnLDO = 1;
//			vout10.bPowerSave = 0;
//			vout10.LDO_Volt = LDO4568910_VOLT_285;
//			HiPmuCtl(IOCTL_PMU_LDO10_OPER,(unsigned char *)&vout10,sizeof(vout10));
			g_touch_is_poweron = 1;
			msleep(40);
			printk("power_pm On.\r\n");
		}
	}
	else{
		if(0 != g_touch_is_poweron){
			//set power vout10 off
//			vout10.bSetOper=1;
//			vout10.bEnLDO=0;
//			vout10.bPowerSave=0;
//			vout10.LDO_Volt=LDO4568910_VOLT_28;
//			HiPmuCtl(IOCTL_PMU_LDO10_OPER,(unsigned char *)&vout10,sizeof(vout10));
			g_touch_is_poweron = 0;
			printk("power_pm Off.\r\n");
		}
	}
}

static void hisik3_ts_gpio_resume(void)
{
#if 0
	/*打开触摸屏中断IO*/
	writew(0x1, IO_ADDRESS(REG_IOMG030));
	writew(0x7, IO_ADDRESS(REG_IOCG098));
#endif

	//printk("the value iomg030 0x%x, iocg098 0x%x \n", readl(IO_ADDRESS(REG_IOMG030)), readl(IO_ADDRESS(REG_IOCG098)));

	/* set gpio direction: input */
	hisik3_gpio_set_direction(TOUCH_PANEL_GPIO, EGPIO_DIR_INPUT);
	hisik3_ts_trace(2, "the value group 8 pin 7 direction %x \n", hisik3_gpio_get_direction(TOUCH_PANEL_GPIO));

	/* set intr sense: edge */
	hisik3_gpio_setsense(TOUCH_PANEL_GPIO, EGPIO_INT_LEVEL);
	hisik3_ts_trace(2, "the value group 8 pin 7 edge %x \n", hisik3_gpio_getsense(TOUCH_PANEL_GPIO));

	/* set intr sense: single edge */
	hisik3_gpio_setedge(TOUCH_PANEL_GPIO, EGPIO_EDGE_SINGLE);
	hisik3_ts_trace(2, "the value group 8 pin 7 sigle edge %x \n", hisik3_gpio_getedge(TOUCH_PANEL_GPIO));

	/* set intr sense: low edge */
	hisik3_gpio_setevent(TOUCH_PANEL_GPIO, EGPIO_EVENT_LOW);
	hisik3_ts_trace(2, "the value group 8 pin 7 low edge %x \n", hisik3_gpio_getevent(TOUCH_PANEL_GPIO));

	/* clear intr */
	hisik3_gpio_IntrClr(TOUCH_PANEL_GPIO);

	/* enable intr  */
	hisik3_gpio_IntrEnable(TOUCH_PANEL_GPIO);
}

/* handle group 8 pin 7 */
static void hisik3_ts_gpio_init(void)
{
#if 0
	unsigned int regValue;
	unsigned int scbase = IO_ADDRESS(REG_BASE_SC);
	int delay = 100;
	hisik3_ts_trace(2,"start hisik3_ts_gpio_init\n");

	/* enable GPIO8 clk first*/
	regValue = (1 << 20);
	writel(regValue, scbase + REG_SC_PEREN1);

	do{
		regValue = readl(scbase + REG_SC_PERSTAT1);
		delay --;
		msleep(10);
		if(delay <= 0){
			printk("delay wait GPIO0 clk enable failed\n");
			break;
		}
	}while((regValue & (1 << 20)) == 0);
#endif

	hisik3_ts_gpio_resume();

#if 0
	regValue = readw(scbase + REG_SC_SYSINTMASK);
	hisik3_ts_trace(2, "the sys common intr mask is 0x%x \n", readw(scbase + REG_SC_SYSINTMASK));
	regValue |= (0x1<<13);
	writew(regValue, scbase + REG_SC_SYSINTMASK);
	hisik3_ts_trace(2, "the sys common intr mask is 0x%x \n", readw(scbase + REG_SC_SYSINTMASK));
#endif
}

static void hisik3_ts_gpio_deinit(void)
{
	hisik3_ts_trace(2,"hisik3_ts_gpio_deinit\n");

	/* disable intr  */
	hisik3_gpio_IntrDisable(TOUCH_PANEL_GPIO);

	/* set gpio direction: output */
	hisik3_gpio_set_direction(TOUCH_PANEL_GPIO, EGPIO_DIR_OUTPUT);

	/* set gpio value: low */
	hisik3_gpio_setvalue(TOUCH_PANEL_GPIO, EGPIO_DATA_LOW);

#if 0
	/* 关闭触摸屏中断IO*/
	writew(0x0, IO_ADDRESS(REG_IOCG098));
#endif
}

static int hisik3_ts_hkadc_get_sample_value(hisik3_ts_info *pts_info)
{
#define K3_TOUCH_DATA_NUM   7
	unsigned char idata[K3_TOUCH_DATA_NUM];
	int ret;
	//int i = 1;

	memset(idata, 0x0, K3_TOUCH_DATA_NUM);
	ret = ctp_read_reg8(0x0, idata, K3_TOUCH_DATA_NUM);
	if(0 != ret){
		return ret;
	}

	//printk("gpio intr touch I2c read 0x0. 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
	//idata[0], idata[1], idata[2], idata[3], idata[4], idata[5], idata[6], idata[7], idata[8]);

	pts_info->y =  (idata[2]<<8)+idata[3];
	pts_info->x =  (idata[4]<<8)+idata[5];
	pts_info->num = idata[0]&0x7;

//	printk("sample_x 0x%x \n", pts_info->x);
//	printk("sample_y 0x%x  \n\n", pts_info->y);

	return 0;
}

static struct input_dev  *ts_input_dev = NULL;
int check_ts_gpio_low(void)
{
	int  ulUpLvlNum=0, ulDownLvlNum=0;
	unsigned int regvalue;

	while((ulUpLvlNum<3) && (ulDownLvlNum<3)){
		regvalue = hisik3_gpio_getvalue(TOUCH_PANEL_GPIO);
		if(EGPIO_DATA_HIGH == regvalue){
			ulUpLvlNum++;
			ulDownLvlNum=0;
		}
		else{
			ulDownLvlNum++;
			ulUpLvlNum=0;
		}
		msleep(10);
	}

	if (ulDownLvlNum>=3){
		return 1;
	}
	else{
		return 0;
	}
}

static void hisik3_ts_main(void)
{
	struct input_dev  *input = ts_input_dev;
	int x_coord, y_coord;
	static int bts_is_down = 0;
	static int x_coord_tmp, y_coord_tmp;
	int ret;
	hisik3_ts_info st_ts_info = {0};

	//msleep(10);
	ts_lock();

	//printk("[%s]: Get intr lock OK.\r\n", __func__);
	/* touch panel is power off */
	if(0 == g_touch_is_poweron){
		if(bts_is_down && g_touch_is_resume){
			bts_is_down = 0;
			//printk("intr touch release.\n");
			input_report_key(input, BTN_TOUCH, 0);
			input_report_abs(input, ABS_PRESSURE, 0);
			input_sync(input);
		}
		ts_release();
		return;
	}

	/* effective irq conver coordinate */
	ret = hisik3_ts_hkadc_get_sample_value(&st_ts_info);

	//printk("k3 touch intr report is x %d, y %d, num %d\n", st_ts_info.x, st_ts_info.y, st_ts_info.num);

	/* clear intr */
	hisik3_gpio_IntrClr(TOUCH_PANEL_GPIO);

	/* enable intr  */
	hisik3_gpio_IntrEnable(TOUCH_PANEL_GPIO);

	if(0 != ret){
		ts_release();
		return;
	}
		
#if 0
	hisik3_set_target(0, MAX_CPU_FREQ, 0);    
#endif

	//printk("x_coord_raw %u y_coord_raw %u\n", x_coord_raw,y_coord_raw);
	if(st_ts_info.num > 0){
		if((st_ts_info.x>0) && (st_ts_info.x<TOUCH_IC_X) 
			&& (st_ts_info.y<TOUCH_IC_Y) && (st_ts_info.y>0)){
			x_coord = st_ts_info.x * TOUCH_PANEL_X / TOUCH_IC_X;
			y_coord = (TOUCH_IC_Y -st_ts_info.y)*TOUCH_PANEL_Y/TOUCH_IC_Y;

			if(0 == bts_is_down){
				bts_is_down = 1;
				x_coord_tmp = x_coord;
				y_coord_tmp = y_coord;
			}
			else{
				#define COEFFPARAMETERA   6
				#define COEFFPARAMETERB   10

				x_coord_tmp = (COEFFPARAMETERA * x_coord + COEFFPARAMETERB * x_coord_tmp) >> 4;
				y_coord_tmp = (COEFFPARAMETERA * y_coord + COEFFPARAMETERB * y_coord_tmp) >> 4;
			}

			//printk("intr touch press\n");
			input_report_abs(input, ABS_X, x_coord);
			input_report_abs(input, ABS_Y, y_coord);
			input_report_abs(input, ABS_PRESSURE, 1);
			input_report_key(input, BTN_TOUCH, 1);
			input_sync(input);
			//printk("intr touch press:  %d, %d\n",x_coord, y_coord);
		}
		//printk("time start OK\n");
	}
	else if(bts_is_down){
		bts_is_down = 0;
		//printk("intr touch release.\n");
		input_report_key(input, BTN_TOUCH, 0);
		input_report_abs(input, ABS_PRESSURE, 0);
		input_sync(input);
		//printk("intr touch release OK.\n");
	}
	ts_release();
}

static irqreturn_t hisik3_ts_interrupt(int irq, void *dev_id)
{
	//hisik3_ts_trace(2,"go into hisik3_ts_interrupt\n");
	//printk("go into hisik3_ts_interrupt\n");

	/* irq judgement */
	if(!hisik3_gpio_Get_IntrStats(TOUCH_PANEL_GPIO)){
		//printk("hisik3_ts_interrupt not ts 0x%x\n", regvalue);
		return IRQ_NONE;
	}

	/* disable irq */
	hisik3_gpio_IntrDisable(TOUCH_PANEL_GPIO);

	//hisik3_ts_main();
	queue_work(g_hisik3_ts_work_queue, &g_hisik3_ts_work);

	return IRQ_HANDLED;
}

#if 0
static void hisik3_ts_suspend(struct early_suspend *h)
{
	//char pmu_reg;
	printk("k3 ts suspend+\n");

	ts_lock();
	hisik3_ts_gpio_deinit();

	// disable tsc pmu; LDO10
	power_pm(0);
	g_touch_is_resume = 0;
	ts_release();
	printk("k3 ts suspend-\n");
}

static void hisik3_ts_resume(struct early_suspend *h)
{
	//char pmu_reg;
	u8 idata = 0;

	printk("k3 ts resume+\n");

	g_touch_is_resume = 1;

	hisik3_ts_main();

	ts_lock();

	// enable tsc pmu ; LDO10
	// set LDO8 2.8V
	power_pm(1);

	hisik3_ts_gpio_resume();
	ts_release();

	ctp_read_reg8(0x24, &idata, 1);
	ctp_write_reg8(0x24, 0x10);
	//ctp_read_reg8(0x24, &idata, 1);
	printk("k3 ts resume-\n");
}

static struct early_suspend k3ts_early_suspend_desc = {
	.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN - 1,
	.suspend = hisik3_ts_suspend,
	.resume = hisik3_ts_resume,
};
#endif

static int hisik3_ts_events_probe(struct i2c_client *client,
				const struct i2c_device_id *idp)
{
	//unsigned long base = IO_ADDRESS(REG_BASE_SCTL + REG_SC_SYSINTMASK);
	unsigned long sys_mask, flage; 
	struct hisik3_ts_priv *priv;
	struct input_dev *input;
	int error;
	int ret;
	u8 idata = 0;

	printk("k3 touchscreen device start client 0x%lx idp 0x%lx\n", (unsigned long)client, (unsigned long)idp);

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dev_err(&client->dev, "failed to allocate driver data\n");
		error = -ENOMEM;
		goto err0;
	}

	dev_set_drvdata(&client->dev, priv);

	input = input_allocate_device();
	if(!input){
		printk("erro k3 touchscreen probe 2 \n");
		goto fail;
	}

	input->name = K3_TS_DRV_NAME;
	input->phys = K3_TS_DRV_NAME;

	//edev->input->open = hisik3_ts_input_open;
	//edev->input->close = hisik3_ts_input_close;
	input->open = NULL;
	input->close = NULL;

	input->id.bustype = BUS_I2C;
	input->dev.parent = &client->dev;

	input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

#if 0
    __set_bit(EV_ABS, input->evbit);
    __set_bit(EV_KEY, input->evbit);
    __set_bit(BTN_TOUCH, input->keybit);
#endif

	input_set_abs_params(input, ABS_X, 0, TOUCH_PANEL_X, 0, 0);
	input_set_abs_params(input, ABS_Y, 0, TOUCH_PANEL_Y, 0, 0);
	input_set_abs_params(input, ABS_PRESSURE, 0, 1, 0, 0);

	input_set_drvdata(input, priv);

	priv->client = client;
	priv->input = input;

	priv->irq = IRQ_GPIO(3*8 + 6);  //99; //42;
	client->irq = IRQ_GPIO(3*8 + 6);//99; //42;

	ctp_i2c_client = client;

	ret = input_register_device(input);
	if (ret){
		printk("erro k3 touchscreen probe 3 \n");
		goto fail;
	}

	if((error=request_irq(priv->irq, hisik3_ts_interrupt, IRQF_SHARED, "hisik3_ts_events", priv)) < 0){
		printk("erro k3 touchscreen probe 4 .ERR=%d\n",error);
		goto fail1;
	}

#if 0
	// enable GPIO TOUCH SCREEN IRQ (GPIO GROUP8)
	local_irq_save(flage);
	sys_mask = readl(base);
	sys_mask |= 1 << 13;
	writel(sys_mask, base);
	//set_bit(13, sc_base);
	local_irq_restore(flage);
#endif

	/* record to static glob usage */
	ts_input_dev = input;

	//register_early_suspend(&k3ts_early_suspend_desc);

	printk("k3 touchscreen device probed \n");

	// enable tsc pmu ; LDO10
	// set LDO10 2.8V
	g_touch_is_poweron = 0;
	g_touch_is_resume = 0;
	power_pm(1);

	//  i2c_hisik3_init(i2c_1);
	hisik3_ts_gpio_init();

#if 0
	// bug fix: GPIO_0_7在不接电阻式触摸屏时，处于悬空状态。需要处理
	/* disable intr  */
	hisik3_gpio_IntrDisable(GPIO_0_7);

	/* set gpio direction: input */
	hisik3_gpio_set_direction(GPIO_0_7, EGPIO_DIR_INPUT);
#endif
	// 初始化触摸屏
	ctp_write_reg8(0x24, 0x10);
	ctp_read_reg8(0x24, &idata, 1);
	printk("touch I2c read 0x24 OK. 0x%x\n", idata);

	return 0;

fail1:
	input_free_device(input);
fail:
	if(input){
		input_unregister_device(input);
		kfree(input);
	}
err0:
	if(priv){
		kfree(priv);
	}
	return -EINVAL;
}

static int hisik3_ts_events_remove(struct i2c_client *client)
{
	struct hisik3_ts_priv *priv = dev_get_drvdata(&client->dev);

	power_pm(0);

	//unregister_early_suspend(&k3ts_early_suspend_desc);
	free_irq(priv->irq, priv);

	input_free_device(priv->input);
	input_unregister_device(priv->input);

	kfree(priv);

	hisik3_ts_trace(2,"end of hisik3_ts_events_remove\n");

	return 0;
}

static struct i2c_driver ctp_i2c_driver; 
static int ctp_attach(struct i2c_adapter *adapter)
{
	struct i2c_board_info info;
	struct i2c_client *client;

	printk("ctp_attach begin, adapter->nr= %i\n", adapter->nr);
	/* mark */
	if (0 != adapter->nr){ 
		printk("touch attach error: %s\n", adapter->name);
		return -1;
	}
	memset(&info, 0, sizeof(struct i2c_board_info));
	strlcpy(info.type, "hisik3_ts_i2c", I2C_NAME_SIZE);
	info.addr = 0x24;
	
	client = i2c_new_device(adapter, &info);
	if (!client){
		printk("ctp_attach i2c_new_device fail\n");
		return -ENODEV;
	}
	
	list_add_tail(&client->detected, &ctp_i2c_driver.clients);

	client->adapter = adapter;

	printk("ctp_attach end\n");
	return 0;
}

static const struct i2c_device_id hisik3_ts_id[] = {
	{ "hisik3_ts_i2c", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, hisik3_ts);

static struct i2c_driver ctp_i2c_driver = {
	.driver =   {
		.owner = THIS_MODULE,
		.name = "hisik3_ts_i2c",
	},
	.attach_adapter = ctp_attach,
	.id_table = hisik3_ts_id,
	.probe = hisik3_ts_events_probe,
	.remove = hisik3_ts_events_remove,

};

int __init hisik3_ts_init(void)
{
	int ret;

	g_hisik3_ts_work_queue = create_workqueue("hisik3_ts_worker");
	if(NULL == g_hisik3_ts_work_queue){
		printk("touchscreen register create_workqueue failed\n");
	goto fail;
	}

	//print_debug("%s", __FUNCTION__); 
	ret = i2c_add_driver(&ctp_i2c_driver);
	if(0 != ret){
		ret = -ENOTSUPP;
		goto fail1;
	}

	printk("touchscreen registered \n");

	return 0;

fail1:
	destroy_workqueue(g_hisik3_ts_work_queue);
fail:
	i2c_del_driver(&ctp_i2c_driver);
	return ret;
}

void __exit hisik3_ts_exit(void)
{
	destroy_workqueue(g_hisik3_ts_work_queue);
	i2c_del_driver(&ctp_i2c_driver);
}

module_init(hisik3_ts_init);
module_exit(hisik3_ts_exit);
MODULE_DESCRIPTION("K3 CTS Driver");
MODULE_AUTHOR("c58721");
MODULE_LICENSE("GPL");

