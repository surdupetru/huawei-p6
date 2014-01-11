/*
created by huawei zhouzhigang 2012/01/13
2012-03-10  zhouzhigang40410 modify SPI MODE from 3 to 1
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
#include <linux/version.h>
#include <linux/proc_fs.h>

#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

/*=========================================================*/
//#define _SPI_DEBUG

/*=========================================================*/
struct spi_device *spi_g;

/*=========================================================*/
static ssize_t sprd_spi_drv_show(struct device *dev,
				   struct device_attribute *attr, char *buf);
static ssize_t sprd_spi_drv_store(struct device *dev,
	                       struct device_attribute *attr, const char *buf, size_t count);
int mux2spi_write(const void *buf, int len);
int mux2spi_read(void *buf, int len);
int mux2spi_write_and_read(const void *t_buf, void *r_buf, int len);
static DEVICE_ATTR(spi_config, S_IRUGO | S_IWUSR, sprd_spi_drv_show, sprd_spi_drv_store);

/*=========================================================*/
//extern int device_create_file(struct device *dev,
//		       const struct device_attribute *attr);

/*=========================================================*/
//#define _SPI_DEBUG
#ifdef _SPI_DEBUG
#define SPRD_SPI_DEBUG(format, arg...) printk(format,##arg)


/*
  data_len    the len of buf
  dbg_len     only debug some data,this num should be 8 alginent.
*/
void SPI_DATA_DEBUG(const void *buf, int data_len, int dbg_len)
{
    int i = dbg_len, j = 0;
	unsigned char *cbuf = (unsigned char *)buf;
	
    if(dbg_len > data_len)
    {
        return;
    }
	
	printk("len:%d\n", data_len);

	while(i > 7)
	{
	    printk("%d | %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x\n", 
			j, cbuf[j],cbuf[j+1],cbuf[j+2],cbuf[j+3],cbuf[j+4],cbuf[j+5],cbuf[j+6],cbuf[j+7]);

		i -= 8;
		j += 8;
	}
}
#else
#define SPI_DATA_DEBUG(x,y,z)
#define SPRD_SPI_DEBUG(format,  arg...)
#endif


static ssize_t sprd_spi_drv_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
   int status;
   
   SPRD_SPI_DEBUG("%s\n", __func__);
        
   if(attr == &dev_attr_spi_config) 
   {
       status = snprintf(buf, PAGE_SIZE,"mode:%d, bit:%d, hz:%d \n", 
   		   spi_g->mode, spi_g->bits_per_word, spi_g->max_speed_hz);
   } 
   else
   {
       status = -EINVAL;
   }
    
   return status;;
}

/*
eg: echo "61  128"
0x61--> 'a'
*/
static ssize_t sprd_spi_drv_store(struct device *dev,
	                       struct device_attribute *attr, const char *buf, size_t count)
{
#ifdef _SPI_DEBUG
    unsigned char t_buf[256+64], r_buf[256+64];
	int ch = -1, len = -1 ;
#endif
	int speed = -1 , mod = -1, bits = -1;

	if(count > 0)
	{
#ifdef _SPI_DEBUG
	if(5 != sscanf(buf, "%d %d %d %d %d",&mod, &bits, &speed, &ch, &len))
	{
	    SPRD_SPI_DEBUG("%s _SPI_DEBUG call sscanf error",__func__);
	    return count;
	}
#else
       if(3 != sscanf(buf, "%d %d %d",&mod, &bits, &speed))
        {
            SPRD_SPI_DEBUG("%s call sscanf error",__func__);
            return count;
        }
#endif
	}

	SPRD_SPI_DEBUG("%s, buf:%s, count:%d\n", __func__, buf, count);
	SPRD_SPI_DEBUG("mod:%d, bits:%d, speed:%d, ch:%d, len:%d\n", mod, bits, speed, ch, len);

	if(mod > 3)
	{
	    printk("mod is err\n");
		return count;
	}

	if(speed > 48000000)
	{
	    printk("speed is err\n");
		return count;
	}

	if(bits > 32)
	{
	    printk("bits is err\n");
		return count;
	}

	spi_g->mode= mod;
	spi_g->max_speed_hz = speed;
	spi_g->bits_per_word = bits;
#ifdef _SPI_DEBUG
	if((len < sizeof(t_buf))&&(len > 0))
	{
	    memset(t_buf, ch, len);
		
		mux2spi_write(t_buf,len);
		msleep(1000);
		mux2spi_read(r_buf,len);
		msleep(1000);
		
	    mux2spi_write_and_read(t_buf, r_buf, len);
	}
#endif
	return count;
}

/*
write data to spi ,this fucntion will block.

return 0  successful
others    fail
*/
int mux2spi_write(const void *buf, int len)
{  
   int status, data_len = len;

   SPRD_SPI_DEBUG("%s,data_len:%d\n", __func__, len);

   if((NULL == buf) || (NULL == spi_g))
   {
       printk("mux2spi_write fail\n");
   	   return -EINVAL;
   }
   data_len = len + ((len%16==0)? 0: (16-len%16));
   SPI_DATA_DEBUG(buf, data_len , data_len);
   status = spi_write(spi_g,buf,data_len);
   // printk("%s status=%d.\n", __func__, status);
   if(status)
   {
      printk("mux2spi_write fail:%d\n", status);
   }
   
   return status;
}

/*
send data to spi ,this fucntion will block.

return 0  successful
others    fail
*/
int mux2spi_read(void *buf, int len)
{
   int status, data_len = len;

   SPRD_SPI_DEBUG("%s\n", __func__);

   if((NULL == buf) || (NULL == spi_g))
   {
       printk("mux2spi_read fail\n");
   	   return -EINVAL;
   }

   //adjust to 64byte alig
   data_len = len + ((len%16==0)? 0: (16-len%16));
   //be careful, the spi.h have bug,refer to next function
   status = spi_read(spi_g, buf, data_len);   
   if(status)
   {
      printk("mux2spi_read fail:%d\n", status);
   }
   
   SPI_DATA_DEBUG(buf, data_len , data_len);
   return status;
}

/*
write and send data to spi ,this fucntion will block.

return 0  successful
others    fail
*/
int mux2spi_write_and_read(const void *t_buf, void *r_buf, int len)
{
   struct spi_transfer	t;
   struct spi_message	m;
   int status, data_len = len;

   SPRD_SPI_DEBUG("%s\n", __func__);

   if((NULL == t_buf) || (NULL == spi_g) || (NULL == r_buf))
   {
       printk("mux2spi_write_and_read fail\n");
   	   return -EINVAL;
   }

   data_len = len + ((len%16==0)? 0: (16-len%16));
   SPI_DATA_DEBUG(t_buf, data_len , data_len);

   //fix bug
   memset(&t, 0x0, sizeof(t));
   t.rx_buf = r_buf;
   t.tx_buf = t_buf;
   t.len = data_len;
   t.bits_per_word = spi_g->bits_per_word;
   t.speed_hz = spi_g->max_speed_hz;
   
   spi_message_init(&m);
   spi_message_add_tail(&t, &m);
   
   status = spi_sync(spi_g, &m); 

   if(status)
   {
      printk("mux2spi_write_and_read fail:%d\n", status);
   }

   SPRD_SPI_DEBUG("spi_sync > status:%d \n", status);
   SPI_DATA_DEBUG(r_buf, data_len , data_len);
   
   return status;
}
static int mux2spi_probe(struct spi_device *spi)
{
	int status;

	SPRD_SPI_DEBUG("%s\n", __func__);

	if(NULL == spi)
	{
	   printk("spi device is NULL\n");
	   return -EINVAL;
	}
	
	spi_g = spi;
	spi_g->mode = SPI_MODE_1;
	spi_g->max_speed_hz = 13000000;
	//spi_g->bits_per_word = 32;
	spi_g->bits_per_word = 16;
	status = spi_setup(spi_g);

	if(status)
	{
	   printk("spi drv setup fail\n");
	   return status;
	}

	status = device_create_file(&(spi_g->dev), &dev_attr_spi_config);

	if(status)
	{
	    printk("spi device create attr file fail!\n");
	}

	printk("<Du Wei> spi probe success\n");

	return status;
}
static struct spi_driver sprd_spi_drv = {
	.driver = {
		.name =         "sprd_modem_spi3.0",
		.owner =        THIS_MODULE,
	},
	.probe =        mux2spi_probe,
	.remove =       NULL,
};

static int __init sprd_spi_init(void)
{
	return spi_register_driver(&sprd_spi_drv);	
}

static void __exit sprd_spi_exit(void)
{
	spi_unregister_driver(&sprd_spi_drv);		
}


EXPORT_SYMBOL(mux2spi_write);
EXPORT_SYMBOL(mux2spi_read);
EXPORT_SYMBOL(mux2spi_write_and_read);

module_init(sprd_spi_init);
module_exit(sprd_spi_exit);
