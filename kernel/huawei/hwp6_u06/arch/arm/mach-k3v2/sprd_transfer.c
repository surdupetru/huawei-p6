/*
created by huawei zhouzhigang 2012/01/13
2012-02-04  zhouzhigang40410 优化代码，解决同CP间中断冲突问题；优化发送时序；规避中断丢失问题
2012-03-08  zhouzhigang40410 增加电源管理，规避SPRDTRUM modem bug，优化中断处理
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/kthread.h>

#include <linux/spi/spi.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
//#include <linux/version.h>
#include <linux/proc_fs.h>

#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <mach/modem_boot_sprd8803g.h>
#include "sprd_transfer.h"
#include <linux/irq.h>
#include "sprd_mux_buffer.h" 
#include <net/sock.h>
#include <net/netlink.h>
#include <linux/skbuff.h>

#include <linux/platform_device.h>
#include <hsad/config_interface.h>
extern int modem_monitor_uevent_notify(int event);
/*=========================================================*/
//#define _MODEM_DEBUG
#define MAX_PACKAGE_SIZE (1664)  ////MUX 包最大1644byte
#define MAX_TRY_TIMES (30) 
#define MAX_MODEM_BUF (8192-256)
//#define GET_MODEM_STATE()  21//(modem_transfer_ctl.state)
#define MODEM_PACK_ALIG  (64)
#define MODEM_POWER_ON_WAIT_TIMES  6000

#define CP_RTS_REQ  sprd_modem_rts
#define CP_RDY_REQ  spr_modem_rdy
#define CP_RTS_DISABLE sprd_modem_rts_disable
#define CP_WAIT_COUNT  150

#define IS_TX()  (modem_transfer_ctl.is_tx)
#define IS_RX()  (modem_transfer_ctl.is_rx)
#define SET_TX()  {modem_transfer_ctl.is_tx=1;}
#define SET_RX()  {modem_transfer_ctl.is_rx=1;}
#define CLEAR_TX()  {modem_transfer_ctl.is_tx=0;}
#define CLEAR_RX()  {modem_transfer_ctl.is_rx=0;}
static struct sock *g_nlfd=NULL;
DEFINE_MUTEX(modem_receive_sem);
DEFINE_MUTEX(send_sem);

unsigned int gpio_cpap_rts;
unsigned int gpio_cpap_rdy;
unsigned int gpio_apcp_rts;
unsigned int gpio_apcp_rdy;

/* SPRD Modem Flashless SPI Packet Resend Scheme */

unsigned int gpio_cpap_resend;
unsigned int gpio_apcp_resend;

static unsigned int g_rild_pid = 0;
const char * sprd_event_str[] =
{
   "MODEM_OFF",
   "MODEM_POWER_ON",
   "MODEM_OK",
   "MODEM_NBOOT",
   "MODEM_RESET",
   "MODEM_FACTORY_MODE",
   "MODEM_AP_POWEROFF",
   0
};
/* Format of the message send from kernel to userspace */
struct sprd_nl_packet_msg {
	int sprd_event;
};
/* The message type send between rild and sprd modem */
typedef enum Sprd_KnlMsgType
{
      NETLINK_SPRD_REG=0,	//send from rild to kernel register the PID for netlink kernel socket
      NETLINK_SPRD_KER_MSG,	//send from kernel to rild
      NETLINK_SPRD_UNREG	//when rild exit send from rild to kernel  for unregister
}SPRD_MSG_TYPE_EN;
typedef enum ENUM_MODEM_STATE {
    MODEM_STATE_OFF=0,
    MODEM_STATE_POWER,
    MODEM_STATE_READY,
    MODEM_STATE_INVALID,
} ENUM_MODEM_STATE_T;
typedef struct 
{
	u16 tag;
	u16 type;	//data type
	u32 length;
	u32 frame_num;	
	u32 reserved2;
}packet_header;
char *modem_state_str_t_t[] =
{
  "OFF",
  "POWER_ON",
  "ON",
  "NBOOT",
  "RESET",
  "FACTORY_MODE",
  "AP_POWEROFF",
  "CTA_RESET",
  0
};

typedef struct
{
      unsigned char send_buf[MAX_MODEM_BUF+256];// max 8KByte, same to modem
      unsigned char read_buf[MAX_MODEM_BUF+256];// max 8KByte, same to modem
      unsigned char tx_write_buf[MAX_MODEM_BUF + sizeof(packet_header)+256];//used to send data to spi
      struct mux_ringbuffer rbuf;
      struct platform_device *modem_dev; //  not use
      struct work_struct	rx_work;
      struct work_struct	tx_work;
      struct work_struct	state_work;
      int state;
      int state_new;
      spinlock_t		write_lock; //not used
      struct semaphore trans_mutex; //not used
      wait_queue_head_t spi_read_wq; //_wq mean wait queue, not work queue, but used to wake up read workqueue when receive CP_RTS Disable signal
      wait_queue_head_t spi_write_wq;//__wq mean wait queue, not wake queue, used to wake write workqueue when receive CP_RDY signal
      unsigned int  is_tx; //AP is send data
      unsigned int  is_rx; //ap is receive data
}sprd_modem_transfer_ctl;

/*=========================================================*/
//static ssize_t sprd_modem_state_show(struct device *dev,
//				                                                  struct device_attribute *attr, char *buf);
//static ssize_t sprd_modem_state_store(struct device *dev,
//                                                                           struct device_attribute *attr, const char *buf, size_t count);
extern int get_modem_state(void);
extern unsigned int get_modem_flashless();
void set_modem_state(int state);

#define GET_MODEM_STATE()  get_modem_state()

/*=========================================================*/
//static DEVICE_ATTR(modem_state, S_IRUGO | S_IWUSR | S_IWGRP, sprd_modem_state_show, sprd_modem_state_store);

static struct workqueue_struct *modem_rx_wq;//  *Rx_work  //数据发送workqueue，包括队列和work stuct
static struct workqueue_struct *modem_tx_wq;//  *Tx_work //数据接收workqueue，包括队列和work stuct
static sprd_modem_transfer_ctl modem_transfer_ctl;

static int sprd_modem_rts = 0, spr_modem_rdy = 0, sprd_modem_rts_disable = 0;//确保中断丢失也能处理
static int modem_reset_times = 0;   //modem重启的次数
/*=========================================================*/
//extern int ts0710_rx_handler_buf(unsigned char *buf, ssize_t len);
extern int mux2spi_write(const void *buf, int len);
extern int mux2spi_read(void *buf, int len);
extern int mux2spi_write_and_read(const void *t_buf, void *r_buf, int len);
extern int ts0710_rx_handler_buf(unsigned char *buf, ssize_t len);
extern int is_cmux_mode(void);
/*=========================================================*/
#ifdef _MODEM_DEBUG
static int cpap_rts_enable = 0, cpap_rts_disable = 0, cpap_rts_irq_count = 0;
static int cpap_rdy_enable = 0, cpap_rdy_disable = 0, cpap_rdy_irq_count = 0;

#define SPRD_MODEM_DEBUG(format, arg...)  printk(format,##arg)
#define SPRD_MODEM_ASSERT(x,y)  {\
	if(x != y)  \
	{ \
	    printk("modem times:x:%d, y:%d\n", x, y);\
	}\
	else\
	{\
	   printk("y:%d\n", y);\
	}\
}


#define PLUS_CPAP_RTS_ENABLE()  {cpap_rts_enable++; cpap_rts_irq_count++;}
#define PLUS_CPAP_RTS_DISABLE() {cpap_rts_disable++; cpap_rts_irq_count++;}
#define PLUS_CPAP_RDY_ENABLE()  {cpap_rdy_enable++; cpap_rdy_irq_count++;}
#define PLUS_CPAP_RDY_DISABLE() {cpap_rdy_disable++; cpap_rdy_irq_count++;}

#define CPAP_RTS_IRQ_DEBUG() {\
   printk("cpap_rts_enable:%d, cpap_rts_disable:%d, cpap_rts_irq_count:%d\n",\
      cpap_rts_enable, cpap_rts_disable, cpap_rts_irq_count);\
   SPRD_MODEM_ASSERT(cpap_rts_enable, cpap_rts_disable);\
}
   
#define CPAP_RDY_IRQ_DEBUG() {\
   printk("cpap_rdy_enable:%d, cpap_rdy_disable:%d, cpap_rdy_irq_count:%d\n",\
      cpap_rdy_enable, cpap_rdy_disable, cpap_rdy_irq_count);\
   SPRD_MODEM_ASSERT(cpap_rdy_enable, cpap_rdy_disable);\
}

/*
  data_len    the len of buf
  dbg_len     only debug some data,this num should be 8 alginent.
*/
static void MODEM_DATA_DEBUG(const void *buf, int data_len, int dbg_len)
{
    int i = dbg_len, j = 0;
	unsigned char *cbuf = (unsigned char *)buf;
	
    if(dbg_len > data_len)
    {
        return;
    }
	
	printk("<modem>len:%d\n", data_len);

	while(i > 7)
	{
	    printk("%d | %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x\n", 
			j, cbuf[j],cbuf[j+1],cbuf[j+2],cbuf[j+3],cbuf[j+4],cbuf[j+5],cbuf[j+6],cbuf[j+7]);

		i -= 8;
		j += 8;
	}
}
#else
#define MODEM_DATA_DEBUG(x,y,z)
#define SPRD_MODEM_DEBUG(format,  arg...)
#define SPRD_MODEM_ASSERT(x,y)
#define PLUS_CPAP_RTS_ENABLE()  
#define PLUS_CPAP_RTS_DISABLE() 
#define PLUS_CPAP_RDY_ENABLE()  
#define PLUS_CPAP_RDY_DISABLE()
#define CPAP_RTS_IRQ_DEBUG()
#define CPAP_RDY_IRQ_DEBUG()
#endif

int is_spi_busy_mode(void)
{
     return (IS_TX() || IS_RX());
}

/*
    save data to modem send buffer

    if do not find buffer 30s, return err, then reset RIL and modem
    else return 0    
*/
 /* Receive the message from rild,to register rild PID or unregister the PID*/
static void kernel_sprd_receive(struct sk_buff *skb)
{
	mutex_lock(&modem_receive_sem);

	if(skb->len >= sizeof(struct nlmsghdr))
	{
		struct nlmsghdr *nlh;
		nlh = (struct nlmsghdr *)skb->data;
        
		if((nlh->nlmsg_len >= sizeof(struct nlmsghdr))&& (skb->len >= nlh->nlmsg_len))
		{
			if(nlh->nlmsg_type == NETLINK_SPRD_REG)
			{
				printk("SPRD: %s, netlink receive reg packet ,Rild with PID %d\n",__func__,nlh->nlmsg_pid);
				g_rild_pid = nlh->nlmsg_pid;
			}
			else if(nlh->nlmsg_type == NETLINK_SPRD_UNREG)
			{
				printk("SPRD: %s, netlink NETLINK_TIMER_UNREG ;\n",__func__ );
				g_rild_pid = 0;
			}
		}
	}
        mutex_unlock(&modem_receive_sem);
}
/*
 Setup the netlink kernel socket with type SPRD_XMD=22
*/
static void sprd_netlink_init(void)
{
	g_nlfd =netlink_kernel_create(&init_net, NETLINK_SPRD, 0, kernel_sprd_receive, NULL, THIS_MODULE);
	if(!g_nlfd)
		printk("SPRD: %s, netlink_kernel_create faile ;\n",__func__ );
	else
		printk("SPRD: %s, netlink_kernel_create success ;\n",__func__ );
	return;
}

int write_data_to_modem_buf(const void *buf, int len)
{
    int i, try_time = 0;
#ifdef _MODEM_DEBUG
	unsigned char *p = (unsigned char*)buf;
#endif
	SPRD_MODEM_DEBUG("%s,len:%d, %02x %02x .... %02x %02x\n", __func__,
		len, p[0], p[1], p[len-2], p[len-1]);
	
    if((NULL == buf) || (len > MAX_PACKAGE_SIZE))
    {
       printk("write_data_to_modem_buf, invaild parameter");
       return - EINVAL;
    }

if(MODEM_OK == GET_MODEM_STATE() || MODEM_AP_POWEROFF == GET_MODEM_STATE() )
{
serch_empty_package:	
    if(try_time > MAX_TRY_TIMES)
    {
        printk("modem err:send data fail\n");
        if( MODEM_OK == GET_MODEM_STATE())
        {
              printk("sprd:modem state is ok and we reset modem\n");
              set_modem_state(MODEM_RESET);
        }
        return -EIO; 
    }

	//find empyt buf
	if(!mux_ringbuffer_write(&modem_transfer_ctl.rbuf, buf, len))
	{
	   msleep(100);
	   try_time++;
         printk("serch_empty_package fail  try_time %d\n",try_time);
	   goto serch_empty_package;
	}

	queue_work(modem_tx_wq, &modem_transfer_ctl.tx_work);
}else{
         printk("modem err:state not ok:%d\n",GET_MODEM_STATE());
         return -EIO;
	}
	
	return 0;
}
EXPORT_SYMBOL(write_data_to_modem_buf);
#if 0
int get_modem_state(void)
{
     printk("%s\n", __func__);

     int state;

     state = modem_transfer_ctl.state;

     printk("modem_transfer_ctl.state is %d\n",state);
     return state;

}
#endif
void set_modem_state(int state)
{
   //if state not eque
   printk("%s\n", __func__);
#if 0   
   if(modem_transfer_ctl.state == state)
   {
       printk("modem state out\n");
      return;	   
   }

   if((state < MODEM_OFF) || (state >= MODEM_STATE_MAX))
   {
       printk("modem err: new state err:%d", state);
   	   return;
   }

   modem_transfer_ctl.state_new = state;
   queue_work(modem_rx_wq, &modem_transfer_ctl.state_work);//USE RX WORKqueue
#endif
    modem_monitor_uevent_notify(MODEM_STATE_OFF);
}
#if 0
//cp2ap_dint low is wrong
static int is_cp2ap_dint_enable(void)
{
    return  !gpio_get_value(CP2AP_DINT);
}
//cp2ap_flag high is right
static int is_cp2ap_flag_enable(void)
{
   return  gpio_get_value(CP2AP_FLAG);
}
#endif
/*
   ap2cp_rts _________
        ____|         |_____
        ______           _____
              |_________|
   cp2ap_rdy 
*/
static int is_cpap_rdy_enable(void)
{
    return !gpio_get_value(gpio_cpap_rdy);
}

/*
   ap2cp_rts _________
        ____|         |_____
        ______           _____
              |_________|
   cp2ap_rdy 
*/
static void ap2cp_rts_enable(void)
{
   gpio_set_value(gpio_apcp_rts, 1);
}

/*
   ap2cp_rts _________
        ____|         |_____
        ______           _____
              |_________|
   cp2ap_rdy 
*/
static void ap2cp_rts_disable(void)
{
   gpio_set_value(gpio_apcp_rts, 0);
}

/*
    cp2ap_rts  _____               ________
                    |_____________|
    ap2cp_rdy  _______           _________
                      |_________|
*/
static void ap2cp_rdy_enable(void)
{
   gpio_set_value(gpio_apcp_rdy, 0);
}

/*
    cp2ap_rts  _____               ________
                    |_____________|
    ap2cp_rdy  _______           _________
                      |_________|
*/
static void ap2cp_rdy_disable(void)
{
   gpio_set_value(gpio_apcp_rdy, 1);
}

/*
    cp2ap_rts  _____               ________
                    |_____________|
    ap2cp_rdy  _______           _________
                      |_________|
*/
static int is_cp2ap_rts_enable(void)
{
   return !gpio_get_value(gpio_cpap_rts);
}

/*
   ap2cp_rts _________
        ____|         |_____
        ______           _____
              |_________|
   cp2ap_rdy 
*/
static int is_cp2ap_rdy_enable(void)
{
   return !gpio_get_value(gpio_cpap_rdy);
}

#if 0
static int is_modem_flag_enable(void)
{
   return gpio_get_value(CP2AP_FLAG);
}
#endif

/*
   ap2cp_rts _________
        ____|         |_____
        ______           _____
              |_________|
   cp2ap_rdy 
*/
static irqreturn_t cp2ap_rdy_handle(int irq, void *handle)
{
    //set modem lever
    if(is_cpap_rdy_enable())
    {
       CP_RDY_REQ = 1;
       PLUS_CPAP_RDY_ENABLE();
    }
	else
	{
	   CP_RDY_REQ = 0;
	   PLUS_CPAP_RDY_DISABLE();
	}

      if(MODEM_OK == GET_MODEM_STATE() || MODEM_AP_POWEROFF == GET_MODEM_STATE()
        ||( (MODEM_POWER_ON == GET_MODEM_STATE()) && get_modem_flashless()) )
    {
    //wake up modem write wq,use event
    wake_up(&modem_transfer_ctl.spi_write_wq);
    }
    return IRQ_HANDLED;
}

/*
    cp2ap_rts  _____               ________
                    |_____________|
    ap2cp_rdy  _______           _________
                      |_________|
*/
static irqreturn_t cp2ap_rts_handle(int irq, void *handle)
{  
    //check modem state
  static int rts_disable_pending = 0;


  if(MODEM_OK == GET_MODEM_STATE() || MODEM_AP_POWEROFF == GET_MODEM_STATE()
          ||( (MODEM_POWER_ON == GET_MODEM_STATE()) && get_modem_flashless()) )
  {
    //active wait event or queue_work
    if(is_cp2ap_rts_enable()) ///1,
    {
        CP_RTS_REQ = 1;
        ap2cp_rdy_enable(); ///2
        PLUS_CPAP_RTS_ENABLE();

		if(rts_disable_pending)
		{
		   CP_RTS_DISABLE = 1;
		   wake_up(&modem_transfer_ctl.spi_read_wq);//wake up send wq, use event
		}
		//use_rts_enable = 1;
        queue_work(modem_rx_wq, &modem_transfer_ctl.rx_work);
		rts_disable_pending = 1;
    }
    else
    {
        rts_disable_pending = 0;
        PLUS_CPAP_RTS_DISABLE();
		CP_RTS_DISABLE = 1;
        wake_up(&modem_transfer_ctl.spi_read_wq);//wake up send wq, use event
    }
  }
	
    return IRQ_HANDLED;
}
#if 0
static irqreturn_t cp2ap_dint_handle(int irq, void *handle)
{
      if(GET_MODEM_STATE() != MODEM_OFF)
      {
         if(is_cp2ap_dint_enable())
           {
           set_modem_state(MODEM_RESET);
           }
       }
    return IRQ_HANDLED;
}
#endif
#if 0
void gpio_config_confirm(void)
{
    //config ap output gpio
    gpio_direction_output(AP2CP_RTS, 0);     //default low
    gpio_direction_output(AP2CP_RDY, 1);     //default high
    gpio_direction_output(AP2CP_FLAG, 1);    //to be confirm
    gpio_direction_output(AP2CP_RSTN, 0);   //high modem reset
    gpio_direction_output(AP2CP_NBOOT, 1);//low modem enter download mode
    gpio_direction_output(AP2CP_WAKE, 0);//low modem enter download mode
    //config ap input gpio
    gpio_direction_input(CP2AP_FLAG);
    gpio_direction_input(CP2AP_DINT);
    gpio_direction_input(CP2AP_RDY);
    gpio_direction_input(CP2AP_RTS);
}

static void power_on_modem(void)
{
    printk("SPRD: %s\n", __func__);

    sprd_modem_platform_data.sprd_modem_ctrl_init();
    //power off modem
    gpio_direction_output(AP2CP_PWD, 0);
    msleep(100);

    /*initialize GPIO to normal state. so modem can work*/
   sprd_modem_platform_data.sprd_gpio_init_resume();
   //confirm gpio
   gpio_config_confirm();

    if (request_irq(gpio_to_irq(CP2AP_RDY), cp2ap_rdy_handle,
                 IRQF_TRIGGER_RISING |IRQF_TRIGGER_FALLING, CP2AP_RDY_NAME, 0) < 0)
    {
    printk("SPRD: request cp2ap rdy irq fail:%d", CP2AP_RDY);
    return;
    }

    if (request_irq(gpio_to_irq(CP2AP_RTS), cp2ap_rts_handle,
                IRQF_TRIGGER_RISING |IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND , CP2AP_RTS_NAME, 0) < 0)
    {
    printk("SPRD: request cp2ap CP2AP_RTS irq fail:%d", CP2AP_RTS);
    return;
    }
#if 0
    if (request_irq(gpio_to_irq(CP2AP_DINT), cp2ap_dint_handle,
                IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND, CP2AP_DINT_NAME, 0) < 0)
    {
    printk("SPRD: request cp2ap CP2AP_DINT irq fail:%d", CP2AP_DINT);
    return;
    }
#endif
    enable_irq_wake(gpio_to_irq(CP2AP_RDY));
    enable_irq_wake(gpio_to_irq(CP2AP_RTS));
    //enable_irq_wake(gpio_to_irq(CP2AP_DINT));

    //power up modem
    //gpio_direction_output(AP2CP_PWD, 1);
    //msleep(800);  //for avoid the CP2AP_FLAG mistake
}
#endif
#if 0
static void power_off_modem(void)
{
    printk("SPRD:%s\n", __func__);

    /*
     //unregister irq
    disable_irq_wake(gpio_to_irq(CP2AP_RDY));
    disable_irq_wake(gpio_to_irq(CP2AP_RTS));
    disable_irq_wake(gpio_to_irq(CP2AP_DINT));

    //power off modem
    gpio_direction_output(AP2CP_PWD, 0);

    free_irq(gpio_to_irq(CP2AP_RDY), 0);
    free_irq(gpio_to_irq(CP2AP_RTS), 0);
    free_irq(gpio_to_irq(CP2AP_DINT), 0);

    */
    /*nitialize GPIO to safe mode. when modem is powered off*/
    sprd_modem_platform_data.sprd_gpio_exit();
}
#endif
#if 0
/*
    return the state of modem
    POWER ON
    ON
    OFF
*/
static ssize_t sprd_modem_state_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
   int status;
   char *str = modem_state_str_t_t[modem_transfer_ctl.state];
   
   SPRD_MODEM_DEBUG("%s\n", __func__);
        
   if(attr == &dev_attr_modem_state) 
   {
       status = snprintf(buf, PAGE_SIZE,"%s\n", str);
   } 
   else
   {
       status = 0;
   }
    
   return status;
}
/*
eg: echo "POWER_ON" or "OFF"
0x61--> 'a'
*/
static ssize_t sprd_modem_state_store(struct device *dev,
	                       struct device_attribute *attr, const char *buf, size_t count)
{
    int i = 0;
	char t_buf[32];
	
	SPRD_MODEM_DEBUG("%s, buf:%s, count:%d\n", __func__, buf, count);

    if(count > 0)
    {
        memset(t_buf, 0, sizeof(t_buf));
        sscanf(buf, "%s",t_buf);
		
        for(i = MODEM_OFF; i<MODEM_STATE_MAX; i++)
        {
           if(!strncmp(t_buf,modem_state_str_t_t[i], 32))
		   	 break;
        }

		if((i < MODEM_STATE_MAX)&&(i != modem_transfer_ctl.state))
		{
		    modem_transfer_ctl.state_new = i;

			printk("change modem state from:%s to %s\n", modem_state_str_t_t[modem_transfer_ctl.state],
				    modem_state_str_t_t[modem_transfer_ctl.state_new]);
 			set_modem_state(i);
		}
    }

	return count;
}
#endif
static void  setup_packet_sum(void * packet, u32 len_ret)
{
    packet_header header;
    static unsigned int tx_packet_count = 0;
    unsigned char *data = (unsigned char *)packet;
    unsigned int sum = 0;
    unsigned int i;

    ++tx_packet_count;

    header.tag = 0x7e7f;
    header.type = 0xaa55;
    header.length = len_ret;
    header.frame_num = tx_packet_count; //sending_cnt;

    /*calculate SPI data sum*/
    for(i = sizeof(packet_header); i < (len_ret + sizeof(packet_header)); i++) {
        sum += data[i];
    }
    header.reserved2 =  sum;

    memcpy(packet, &header, sizeof (packet_header));
    return;
}

static void  setup_packet(void * packet, u32 len_ret)
{
    packet_header header;
	static unsigned int tx_packet_count = 0;

    ++tx_packet_count;
    	
    header.tag = 0x7e7f;
    header.type = 0xaa55;
    header.length = len_ret;
    header.frame_num = tx_packet_count; //sending_cnt;
    header.reserved2 = 0xabcdef00;
    memcpy(packet, &header, sizeof (packet_header));
    return;
}

/*send data to spi devices, config gpio signal
  this function should be careful
  signal as below:
  
   ap2cp_rts _________
        ____|         |_____
        ______           _____
              |_________|
   cp2ap_rdy      
*/
static void send_data_2_modem(const void*buf, int len)
{
    int try_time = 0, re_send = 0;
	int count = 0;
#ifdef _MODEM_DEBUG
	unsigned char *p = (unsigned char *)buf;	
	struct timeval time_start, time_end;
	long s,us;

	do_gettimeofday(&time_start);
#endif	
	
    //test if modem send data
start_send:
    while(is_cp2ap_rts_enable())
    {
        SPRD_MODEM_DEBUG("Oh!modem is sending data, sleep 10ms, try:%d\n", try_time);
        msleep(10);
		
		if(try_time++ > 1000)
		{
		   printk("modem err:send_data_2_modem fail\n");
#ifndef _MODEM_DEBUG
         if( MODEM_OK == GET_MODEM_STATE())
           {
               printk("modem state is MODEM_OK and we rest modem\n");
               set_modem_state(MODEM_RESET);
           }
           return;
#else
           break;
#endif
		}
    }
	CP_RDY_REQ = 0;
	ap2cp_rts_enable();///1 
	
	down(&(modem_transfer_ctl.trans_mutex));		

    //set up packet header

	if(get_modem_flashless())
	{
	    /* SPRD Modem Flashless use checksum for SPI Packet */
	    setup_packet_sum((void*)buf, len);
	}
 	else
 	{
 	    setup_packet((void*)buf, len);
 	}


	count = 0;
	while(count < CP_WAIT_COUNT)
	{
		count++;

		//check cp rts signal. CP send data has highest pirority
		if(is_cp2ap_rts_enable())//we have very little chance to meet rts irq singal
		{
			ap2cp_rts_disable();
			up(&(modem_transfer_ctl.trans_mutex));
		    goto	start_send;
		}

		 //wati rdy enable signal
        wait_event_timeout(modem_transfer_ctl.spi_write_wq,
	         (is_cp2ap_rdy_enable()), msecs_to_jiffies(10));///2
		
		if(is_cp2ap_rdy_enable())
		{
			break;
		}		
    }

	if(count == CP_WAIT_COUNT)
	{
	    printk("modem err:wait cp rdy enable signal timeout:%d ms \n", count*10);
		ap2cp_rts_disable();
		up(&(modem_transfer_ctl.trans_mutex));

        //prevent modem unexpeced err
		if(re_send++ > 5)
		{
                   if( MODEM_OK == GET_MODEM_STATE())
                   {
                      printk("modem state is MODEM_OK and we rest modem\n");
                      set_modem_state(MODEM_RESET);
                    }
                    printk("modem err: send data timeout:%d\n", re_send);
                    return;
                }
		goto	start_send;
	}
	else
	{
	    SPRD_MODEM_DEBUG("wait rdy enable:%dms\n", count*10);
	}

	mux2spi_write(buf ,len+sizeof(packet_header));
        CP_RDY_REQ = 0;
	ap2cp_rts_disable();///3

        count = 0;
	while(count < CP_WAIT_COUNT)
	{
		count++;

		//wati rdy enable signal
        wait_event_timeout(modem_transfer_ctl.spi_write_wq,
	            (!is_cp2ap_rdy_enable()), msecs_to_jiffies(10));///4
		
		if(!is_cp2ap_rdy_enable())
		{
		    break;
		}

		if(CP_RDY_REQ)
		{
		     printk("modem err:rdy disable interrupt missed!\n ");
			 break;
		}
    }

	if(count == CP_WAIT_COUNT)
	{
	    printk("modem err:wait cp rdy disable signal timeout:%d ms \n", count*10);
	}
	else
	{
	    SPRD_MODEM_DEBUG("wait rdy disable:%dms\n", count*10);
	}

    up(&(modem_transfer_ctl.trans_mutex));
    
    if (gpio_get_value(gpio_cpap_resend) && get_modem_flashless()) {
        count = 0;
        printk("%s resend spi packet to cp.\n", __func__);
        goto start_send;
    }

#ifdef _MODEM_DEBUG
	do_gettimeofday(&time_end);
	s = (time_end.tv_usec >= time_start.tv_usec)? 
		            time_end.tv_sec - time_start.tv_sec : 
		            time_end.tv_sec - time_start.tv_sec -1;
	us = (time_end.tv_usec >= time_start.tv_usec)? 
		            time_end.tv_usec - time_start.tv_usec : 
		            1000000 + time_end.tv_usec - time_start.tv_usec;
					
	printk("tx time:%d:%06d\n", s,us);
#endif	
    CPAP_RDY_IRQ_DEBUG();
}
/*
   this work used to send data to spi device
*/
int get_mux_free()
{
    int ret = 0;
    ret = mux_ringbuffer_free(&modem_transfer_ctl.rbuf);

    return ret;
}

static void sprd_modem_tx_work(struct work_struct *work)
{
    int len = mux_ringbuffer_avail(&modem_transfer_ctl.rbuf);

	SET_TX();
	
   	SPRD_MODEM_DEBUG("%s, len:%d\n", __func__, len);
	
    while(len)
    {
		if((len <= MAX_MODEM_BUF)&&(len > 0))
		{
		   mux_ringbuffer_read(&modem_transfer_ctl.rbuf, 
		   	        &modem_transfer_ctl.tx_write_buf[sizeof(packet_header)], len, 0);
		   send_data_2_modem(&modem_transfer_ctl.tx_write_buf, len);
		}
		else
		{
		    printk("modem err, sprd_modem_tx_work:%d", len);
		}
		
		len = mux_ringbuffer_avail(&modem_transfer_ctl.rbuf);
    }

	CLEAR_TX();
}

int spi_packet_verify(unsigned char *packet)
{
    unsigned int i;
    unsigned int  sum_packet  = 0;
    unsigned char *data = (unsigned char *)packet;
    packet_header *header = (packet_header*)packet;

    /*check header*/
    if ((header->tag != 0x7e7f) || (header->type != 0xaa55)
        || (header->length > MAX_MODEM_BUF)) {
         printk("[SPI ERROR]Receive packet tag(%x), type(%x),length(%d).\n",\
                                            header->tag, header->type, header->length);
        return -1;
    }

    /*check sum*/
    for (i = sizeof(packet_header);  i < (header->length + sizeof(packet_header)); i++) {
        sum_packet += packet[i];
    }

    if (header->reserved2 == sum_packet) {
        return 0;
    } else {
        printk("[SPI ERROR]Receive packet sum(%d) != head.reserved2(%d).\n",\
                                    sum_packet, header->reserved2);
        return -1;
    }
}


/*
read data throuth spi devices
    cp2ap_rts  _____               ________
                    |_____________|
    ap2cp_rdy  _______           _________
                      |_________|
*/
static void sprd_modem_rx_work(struct work_struct *work)
{
   packet_header *packet;
   unsigned short *p;
   static int  err_count = 0;
   int count = 0;
   int spi_read_error = 0;
   unsigned int read_len_alig, temp;
#ifdef _MODEM_DEBUG
   struct timeval time_start, time_end;
   long s,us;
	
	do_gettimeofday(&time_start);
#endif

   SET_RX();

   SPRD_MODEM_DEBUG("%s\n", __func__);

   down(&(modem_transfer_ctl.trans_mutex)); 

   count = CP_WAIT_COUNT*2;

   start_read:
   spi_read_error = mux2spi_read(&modem_transfer_ctl.read_buf[0], MODEM_PACK_ALIG);

   while(spi_read_error && count)
    {
	 up(&(modem_transfer_ctl.trans_mutex));

         msleep(5);
         count--;
         printk("SPRD:find spi_read_error :%d and waite times is: %d\n",spi_read_error,(CP_WAIT_COUNT*2-count)*5);
         goto start_read;
     }
    if(!count)
    {
        printk("SPRD:spi_read_error timeout 15*150ms\n");
        
        if( MODEM_OK == GET_MODEM_STATE())
        {
            printk("modem state is MODEM_OK and we rest modem\n");
            set_modem_state(MODEM_RESET);
        }
        
        return;
    }

   p = &modem_transfer_ctl.read_buf[0];
   
   count = 0;
   while((*p != 0x7e7f)&&(count < MODEM_PACK_ALIG))
   {
       count += 2;
       printk("modem err, packet head err:%04x\n", *p);
       p++;
   }

   if(count < (MODEM_PACK_ALIG-sizeof(packet_header)))
   	    packet = (packet_header*)p;
   else 
   	    goto done;

   if((packet->tag != 0x7e7f)||(packet->type != 0xaa55)||(packet->length > MAX_MODEM_BUF))
   {
       unsigned char *p = &modem_transfer_ctl.read_buf[0];
       printk("modem err, head err, tag:0x%04x, type:0x%04x, len:%08x, frame_num:%08x, resv:0x%08x\n",
   	          packet->tag, packet->type, packet->length, packet->frame_num, packet->reserved2);
	   mux2spi_read(&modem_transfer_ctl.read_buf[0], MAX_MODEM_BUF-MODEM_PACK_ALIG);
   	   goto done;
   }
   else if(packet->length <= (MODEM_PACK_ALIG- sizeof(packet_header)- count))
   {
       if(count)
	   	   printk("count:%d, packet_len:%d\n",count,packet->length);
	   
          if (spi_packet_verify(&modem_transfer_ctl.read_buf[count]) && get_modem_flashless()) {
                gpio_set_value(gpio_apcp_resend, 1);
                count = CP_WAIT_COUNT*2;
                goto start_read;
          } else {
                gpio_set_value(gpio_apcp_resend, 0);
          }

	   ts0710_rx_handler_buf(&modem_transfer_ctl.read_buf[sizeof(packet_header)+count], packet->length);
   }
   else//the second read
   {	   
	   temp = packet->length + sizeof(packet_header);
	   read_len_alig = ((temp+MODEM_PACK_ALIG-1)&(~(MODEM_PACK_ALIG-1))) - 
	          (MODEM_PACK_ALIG - count);
	   
	   mux2spi_read(&modem_transfer_ctl.read_buf[MODEM_PACK_ALIG], read_len_alig);

	   if(count)
	   	   printk("count:%d, packet_len:%d\n",count,packet->length);


          if (spi_packet_verify(&modem_transfer_ctl.read_buf[count]) && get_modem_flashless()) {
                gpio_set_value(gpio_apcp_resend, 1);
                count = CP_WAIT_COUNT*2;
                goto start_read;
          } else {
                gpio_set_value(gpio_apcp_resend, 0);
          }

	   ts0710_rx_handler_buf(&modem_transfer_ctl.read_buf[sizeof(packet_header)+count], packet->length);
   }
done:
   CP_RTS_REQ = 0;
   CP_RTS_DISABLE = 0;
   count = 0;
   ap2cp_rdy_disable(); ///3

	while(count < CP_WAIT_COUNT)
	{
		count++;

		 //wati rts disable signal
        wait_event_timeout(modem_transfer_ctl.spi_read_wq,
                            (CP_RTS_DISABLE), msecs_to_jiffies(10));///4

		if(CP_RTS_DISABLE && CP_RTS_REQ)
		{
		   //printk("\n===>we met rts disable and enable\n ");
		}

		if(CP_RTS_DISABLE)
		{
		    break;
		}

		if(CP_RTS_REQ)
		{
			 break;
		}
	}

   if(count == CP_WAIT_COUNT)
   {
      printk("\nmodem err:wait cp rts disable signal timeout:%d ms \n", count*10);
   }
   else
   {
      SPRD_MODEM_DEBUG("\nwait rts disable:%dms\n", count*10);
   }
   up(&(modem_transfer_ctl.trans_mutex));
#ifdef _MODEM_DEBUG
   do_gettimeofday(&time_end);
	s = (time_end.tv_usec >= time_start.tv_usec)? 
		            time_end.tv_sec - time_start.tv_sec : 
		            time_end.tv_sec - time_start.tv_sec -1;
	us = (time_end.tv_usec >= time_start.tv_usec)? 
		            time_end.tv_usec - time_start.tv_usec : 
		            1000000 + time_end.tv_usec - time_start.tv_usec;
					
	printk("rx time:%d:%06d\n", s,us);
#endif
   CPAP_RTS_IRQ_DEBUG();
   CLEAR_RX();
}
static int transfer_has_config = 0;
void sprd_transfer_config(struct gpio gpios[])
{
    if(transfer_has_config)
	return;
    gpio_cpap_rts = gpios[MODEM_SPRD_CPAP_RTS].gpio;
    gpio_cpap_rdy = gpios[MODEM_SPRD_CPAP_RDY].gpio;
    gpio_apcp_rts = gpios[MODEM_SPRD_APCP_RTS].gpio;
    gpio_apcp_rdy = gpios[MODEM_SPRD_APCP_RDY].gpio;
    if(get_modem_flashless())
    {
         gpio_apcp_resend = gpios[MODEM_SPRD_APCP_RESEND].gpio;
         gpio_cpap_resend = gpios[MODEM_SPRD_CPAP_RESEND].gpio;
    }
    

    if (request_irq(gpio_to_irq(gpio_cpap_rdy), cp2ap_rdy_handle,
                      IRQF_TRIGGER_RISING |IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND, "GPIO_CPAP_RDY", 0) < 0)
    {
        printk("SPRD: request cp2ap rdy irq fail:%d", gpio_cpap_rdy);
        return;
    }
    
    if (request_irq(gpio_to_irq(gpio_cpap_rts), cp2ap_rts_handle,
                     IRQF_TRIGGER_RISING |IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND , "GPIO_CPAP_RTS", 0) < 0)
    {
        printk("SPRD: request cp2ap CP2AP_RTS irq fail:%d", gpio_cpap_rts);
        return;
    }
    transfer_has_config = 1;
    printk("%d %d %d %d %d,%d\n",gpio_cpap_rts,gpio_cpap_rdy,gpio_apcp_rts,gpio_apcp_rdy,gpio_apcp_resend,gpio_cpap_resend);
    
}

void sprd_transfer_deconfig(void)
{
    if(!transfer_has_config)
         return;
    disable_irq(gpio_to_irq(gpio_cpap_rdy));
    disable_irq(gpio_to_irq(gpio_cpap_rts));
    
    free_irq(gpio_to_irq(gpio_cpap_rdy), 0);
    free_irq(gpio_to_irq(gpio_cpap_rts), 0);
    transfer_has_config = 0;
}

#define DATA_LENGTH  32768
unsigned char send_buf[DATA_LENGTH]={0};
static ssize_t sprdspi_write(struct file *f, const char __user *buf, size_t len, loff_t *pos)
{
    int try_time = 0, re_send = 0;
    int count = 0;
    //int i =0;

    memset(send_buf,0,sizeof(send_buf));

#ifdef _MODEM_DEBUG
    unsigned char *p = (unsigned char *)buf;
    struct timeval time_start, time_end;
    long s,us;
    do_gettimeofday(&time_start);
#endif

    if (copy_from_user(send_buf+sizeof(packet_header),buf,len)) {
        return -EFAULT;
    }

start_send:
    while(is_cp2ap_rts_enable())
    {
        printk("Oh!modem is sending data, sleep 10ms, try:%d\n", try_time);
        msleep(10);

        if(try_time++ > 1000)
        {
           printk("modem err:send_data_2_modem fail\n");
#ifndef _MODEM_DEBUG
         if( MODEM_OK == GET_MODEM_STATE())
           {
               printk("modem state is MODEM_OK and we rest modem1\n");
               set_modem_state(MODEM_RESET);
           }
           return;
#else
           break;
#endif
        }
    }

    CP_RDY_REQ = 0;
    ap2cp_rts_enable();

    down(&(modem_transfer_ctl.trans_mutex));

    //set up packet header
    setup_packet((void*)send_buf, len);
#if 0 //debug
    for(i=0;i<8;i++){
        printk("%s spi write buf is %x .\n", __func__, send_buf[i]);
    }

    printk("%s spi write len is %d .\n", __func__, len);
#endif
    count = 0;
    while(count < CP_WAIT_COUNT)
    {
        count++;

        if(is_cp2ap_rts_enable())
        {
            ap2cp_rts_disable();
            up(&(modem_transfer_ctl.trans_mutex));
            printk("---> sprspi_write goto start_send\n");
            goto    start_send;
        }

        wait_event_timeout(modem_transfer_ctl.spi_write_wq,
             (is_cp2ap_rdy_enable()), msecs_to_jiffies(10));

        if(is_cp2ap_rdy_enable())
        {
            printk("---> is_cp2ap_rdy_enable ok\n");
            break;
        }
    }

    if(count == CP_WAIT_COUNT)
    {
        ap2cp_rts_disable();
        up(&(modem_transfer_ctl.trans_mutex));
        if(re_send++ > 5)
        {
            if( MODEM_OK == GET_MODEM_STATE())
            {
                printk("modem state is MODEM_OK and we rest modem2\n");
                set_modem_state(MODEM_RESET);
            }
            printk("modem err: send data timeout:%d\n", re_send);
            return;
        }
        goto    start_send;
    }
    else
    {
        SPRD_MODEM_DEBUG("wait rdy enable:%dms\n", count*10);
        printk("wait rdy enable:%dms\n", count*10);
    }

    //printk("%s begin to write buffer to the modem \n",__func__);

    mux2spi_write(send_buf ,len+sizeof(packet_header));

    CP_RDY_REQ = 0;
    ap2cp_rts_disable();

    count = 0;
    while(count < CP_WAIT_COUNT)
    {
        count++;

        wait_event_timeout(modem_transfer_ctl.spi_write_wq,
                (!is_cp2ap_rdy_enable()), msecs_to_jiffies(10));

        if(!is_cp2ap_rdy_enable())
        {
            break;
        }

        if(CP_RDY_REQ)
        {
             printk("modem err:rdy disable interrupt missed!\n ");
             break;
        }
    }

    if(count == CP_WAIT_COUNT)
    {
        printk("modem err:wait cp rdy disable signal timeout:%d ms \n", count*10);
    }
    else
    {
        SPRD_MODEM_DEBUG("wait rdy disable:%dms\n", count*10);
    }

    up(&(modem_transfer_ctl.trans_mutex));

#ifdef _MODEM_DEBUG
    do_gettimeofday(&time_end);
    s = (time_end.tv_usec >= time_start.tv_usec) ?
                    time_end.tv_sec - time_start.tv_sec :
                    time_end.tv_sec - time_start.tv_sec -1;
    us = (time_end.tv_usec >= time_start.tv_usec) ?
                    time_end.tv_usec - time_start.tv_usec :
                    1000000 + time_end.tv_usec - time_start.tv_usec;

    printk("tx time:%d:%06d\n", s,us);
#endif
    CPAP_RDY_IRQ_DEBUG();

    return 0;
}

static ssize_t sprdspi_read(struct file *f, char __user *buf, size_t count, loff_t *pos)
{
    int len = strlen(&modem_transfer_ctl.read_buf[0]);
    unsigned short *p;
    char *ss;
    packet_header *packet;

    p = &modem_transfer_ctl.read_buf[0];

    packet = (packet_header*)p;
    if (len == 0) {
        return len;
    }

    ss = &modem_transfer_ctl.read_buf[sizeof(packet_header)];
    if (copy_to_user(buf, &modem_transfer_ctl.read_buf[sizeof(packet_header)], packet->length)) {
        return -EFAULT;
    }

    memset(modem_transfer_ctl.read_buf, 0, sizeof(modem_transfer_ctl.read_buf));

    return len;
}

static int sprdspi_open(struct inode *inode, struct file *f)
{
    printk("sprspi_open enter\n");
    memset(modem_transfer_ctl.read_buf, 0, sizeof(modem_transfer_ctl.read_buf));
    return nonseekable_open(inode, f);
}

static int sprdspi_release(struct inode *inode, struct file *f)
{
    memset(modem_transfer_ctl.read_buf, 0, sizeof(modem_transfer_ctl.read_buf));
    return 0;
}
static const struct file_operations sprdspi_fops = {
    .owner = THIS_MODULE,
    .open = sprdspi_open,
    .release = sprdspi_release,
    .write = sprdspi_write,
    .read = sprdspi_read,
};

static struct miscdevice sprd_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "sprdspi",
    .fops = &sprdspi_fops,
};

static int __init sprd_transfer_init(void)
{
    int ret = -1;

    ret = get_modem_type( MODEM_SPRD8803G );
    pr_info("MODEM#%s: get_modem_type \"%s\". %d\n",MODEM_DEVICE_BOOT(MODEM_SPRD8803G),MODEM_SPRD8803G,ret);
    if( ret<=0 ) {
        pr_err("MODEM#%s: modem \"%s\" not support on this board. %d\n",MODEM_DEVICE_BOOT(MODEM_SPRD8803G),MODEM_SPRD8803G,ret);
        return( -1 );
    }

    memset(&modem_transfer_ctl, 0, sizeof(sprd_modem_transfer_ctl));
    modem_transfer_ctl.state = MODEM_OFF;
    modem_transfer_ctl.state_new = MODEM_OFF;
    CLEAR_TX();
    CLEAR_RX();

    ret = misc_register(&sprd_dev);
    if (ret) {
        printk("%s register sprdspi dev error !!!\n",__func__);
    }
    mux_ringbuffer_init(&modem_transfer_ctl.rbuf, &modem_transfer_ctl.send_buf, MAX_MODEM_BUF);
    
    init_waitqueue_head(&(modem_transfer_ctl.spi_write_wq));
    init_waitqueue_head(&(modem_transfer_ctl.spi_read_wq));
    
    modem_tx_wq = create_singlethread_workqueue("sprd_modem_tx_wq");
    if(NULL == modem_tx_wq) {
        return -1;
    }
    
    modem_rx_wq = create_singlethread_workqueue("sprd_modem_rx_wq");
    if(NULL == modem_rx_wq) {
        destroy_workqueue(modem_tx_wq);
        return -1;
    }

    INIT_WORK(&(modem_transfer_ctl.tx_work), sprd_modem_tx_work);
    INIT_WORK(&(modem_transfer_ctl.rx_work), sprd_modem_rx_work);
    //INIT_WORK(&(modem_transfer_ctl.state_work), sprd_modem_state_work);
    
    spin_lock_init(&(modem_transfer_ctl.write_lock));
    sema_init(&(modem_transfer_ctl.trans_mutex), 1);

    sprd_netlink_init();

    return 0;
}

static void __exit sprd_transfer_exit(void)
{ 
    netlink_kernel_release(g_nlfd);
    
    misc_deregister(&sprd_dev);
    destroy_workqueue(modem_tx_wq);
    destroy_workqueue(modem_rx_wq);
}
module_init(sprd_transfer_init);
module_exit(sprd_transfer_exit);
