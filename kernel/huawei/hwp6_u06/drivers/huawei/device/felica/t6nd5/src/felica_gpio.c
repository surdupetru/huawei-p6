/*
 * Copyright(C) 2012 NTT DATA MSE CORPORATION. All right reserved.
 *
 * @file felica_gpio.c
 * @brief FeliCa code
 *
 * @date 2012/03/16
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA]
 *
 */

/*
 * include files
 */
#include <linux/err.h>					/* PTR_ERR/IS_ERR           */
#include <linux/string.h>				/* memset                   */
#include <linux/errno.h>				/* errno                    */
#include <linux/module.h>				/* Module parameters        */
#include <linux/init.h>					/* module_init, module_exit */
#include <linux/fs.h>					/* struct file_operations   */
#include <linux/kernel.h>				/* kernel                   */
#include <linux/mm.h>					/* kmallo, GFP_KERNEL       */
#include <linux/slab.h>					/* kmalloc, kfree           */
#include <linux/timer.h>				/* init_timer               */
#include <linux/param.h>				/* HZ const                 */
#include <linux/poll.h>					/* poll                     */
#include <linux/delay.h>				/* udelay, mdelay           */
#include <linux/wait.h>					/* wait_queue_head_t        */
#include <linux/device.h>				/* class_create             */
#include <linux/interrupt.h>			/* request_irq              */
#include <linux/fcntl.h>				/* O_RDONLY, O_WRONLY       */
#include <linux/cdev.h>					/* cdev_init add del        */
#include <linux/gpio.h>					/* GPIO                     */
#include <linux/regulator/consumer.h>	/* regulator                */
#include <asm/uaccess.h>			/* copy_from_user, copy_to_user */
#include <asm/ioctls.h>					/* FIONREAD                 */
#include "../inc/felica_gpio.h"				/* private header file      */
#include <mach/irqs.h>
#include <hsad/config_interface.h>
#include <linux/mux.h>
#include <linux/platform_device.h>
#include <linux/suspend.h>
#include <linux/earlysuspend.h>

/*
 * Function prottype
 */
/* External if */
 static int __init  felica_gpio_init(void);
 static void __exit felica_gpio_exit(void);
 static int felica_gpio_open(struct inode *, struct file *);
 static int felica_gpio_close(struct inode *, struct file *);
 static unsigned int felica_gpio_poll(struct file *, struct poll_table_struct *);
 static ssize_t felica_gpio_write(struct file *, const char *, size_t, loff_t *);
 static ssize_t felica_gpio_read(struct file *, char *, size_t, loff_t *);
 static int felica_hvdd_write(unsigned char);
 static int felica_tvdd_write(unsigned char);
 
/* Interrupt handler */
 static irqreturn_t felica_gpio_int_INT(int, void *);
 static void felica_gpio_timer_hnd_general(unsigned long);

/* Internal function */
 static int felica_gpio_ctrl_enable(struct FELICA_TIMER *);
 static int felica_gpio_ctrl_unenable(struct FELICA_TIMER *, struct FELICA_TIMER *);
 static int felica_gpio_ctrl_online(struct FELICA_TIMER *);
 static int felica_gpio_ctrl_offline(void);
 static int felica_gpio_term_cen_read(unsigned char *);
 static int felica_gpio_term_rfs_read(unsigned char *);
 static int felica_gpio_term_int_read(unsigned char *);
 static int felica_gpio_term_cen_write(unsigned char);
 static int felica_gpio_term_pon_write(unsigned char);
 static int felica_gpio_int_ctrl_init(unsigned char);
 static void felica_gpio_int_ctrl_exit(unsigned char);
 static int felica_gpio_int_ctrl_regist(unsigned char);
 static void felica_gpio_int_ctrl_unregist(unsigned char);
 static void felica_gpio_drv_timer_data_init(struct FELICA_TIMER *);
 static void felica_gpio_drv_init_cleanup(int);
 static int felica_gpio_dev_pon_open(struct inode *, struct file *);
 static int felica_gpio_dev_cen_open(struct inode *, struct file *);
 static int felica_gpio_dev_rfs_open(struct inode *, struct file *);
 static int felica_gpio_dev_itr_open(struct inode *, struct file *);
 static int felica_gpio_dev_pon_close(struct inode *, struct file *);
 static int felica_gpio_dev_cen_close(struct inode *, struct file *);
 static int felica_gpio_dev_rfs_close(struct inode *, struct file *);
 static int felica_gpio_dev_itr_close(struct inode *, struct file *);
extern int ak6921af_write_eeprom(int value);
extern int ak6921af_read_eeprom(void);



/*
 * static paramater & global
 */
 static struct class *felica_class;			/* device class information */
 static struct FELICA_DEV_INFO gPonCtrl;	/* device control info(PON) */
 static struct FELICA_DEV_INFO gCenCtrl;	/* device control info(CEN) */
 static struct FELICA_DEV_INFO gRfsCtrl;	/* device control info(RFS) */
 static struct FELICA_DEV_INFO gItrCtrl;	/* device control info(ITR) */
 static spinlock_t itr_lock;				/* spin_lock param          */
 static unsigned long itr_lock_flag;		/* spin_lock flag           */
 static struct cdev g_cdev_fpon;			/* charactor device of PON  */
 static struct cdev g_cdev_fcen;			/* charactor device of CEN  */
 static struct cdev g_cdev_frfs;			/* charactor device of RFS  */
 static struct cdev g_cdev_fitr;			/* charactor device of ITR  */
 static struct device *pon_device = NULL;
 static dev_t dev_id_pon;					/* Major/Minor Number(PON)  */
 static dev_t dev_id_cen;					/* Major/Minor Number(CEN)  */
 static dev_t dev_id_rfs;					/* Major/Minor Number(RFS)  */
 static dev_t dev_id_itr;					/* Major/Minor Number(ITR)  */
 dev_t dev_id_uart;					/* Major/Minor Number(UART) */
 dev_t dev_id_rws;					/* Major/Minor Number(RWS)  */
 EXPORT_SYMBOL_GPL(dev_id_rws);
 EXPORT_SYMBOL_GPL(dev_id_uart);
 static struct regulator *felica_tvdd;
 static struct regulator *felica_hvdd;
 
/*
 * set gpio status when init, suspend and resume
 */
static struct iomux_block *g_felica_gpio_block;
static struct block_config *g_felica_gpio_block_config;
static int felica_gpio_block_init()
{
	int ret = 0;

	/* get gpio block*/
	g_felica_gpio_block = iomux_get_block(FELICA_GPIO_BOLCK_NAME);
	if (IS_ERR(g_felica_gpio_block)) {
		printk("%s: failed to get gpio block,iomux_get_block failed\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	/* get gpio block config*/
	g_felica_gpio_block_config = iomux_get_blockconfig(FELICA_GPIO_BOLCK_NAME);
	if (IS_ERR(g_felica_gpio_block_config)) {
		printk("%s: failed to get gpio block config\n", __func__);
		ret = -EINVAL;
		goto err_block_config;
	}

	/* config gpio work mode*/
	ret = blockmux_set(g_felica_gpio_block, g_felica_gpio_block_config, NORMAL);
	if (ret) {
		printk("%s: failed to config gpio,blockmux_set failed\n", __func__);
		ret = -EINVAL;
		goto err_mux_set;
	}

	return ret;

err_mux_set:
	if (g_felica_gpio_block_config)
		g_felica_gpio_block_config = NULL;
err_block_config:
	if (g_felica_gpio_block)
		g_felica_gpio_block = NULL;

	return ret;

}

/* Interrupt Control Informaion */
 static struct FELICA_ITR_INFO gFeliCaItrInfo[DFD_ITR_TYPE_NUM] = {
	{
		.irq	= 0,
		.handler= NULL,
		.flags	= IRQF_NO_SUSPEND | IRQF_TRIGGER_FALLING,
		.name	= FELICA_CTRL_ITR_STR_RFS,
		.dev	= NULL
	},
	{
		.irq	= 0,
		.handler= felica_gpio_int_INT,
		.flags	= IRQF_NO_SUSPEND | IRQF_TRIGGER_FALLING,
		.name	= FELICA_CTRL_ITR_STR_INT,
		.dev	= NULL
	}
 };

/*
 * sturut of reginsting External if
 */
 static struct file_operations felica_fops = {
	.owner	= THIS_MODULE,
	.poll	= felica_gpio_poll,
	.read	= felica_gpio_read,
	.write	= felica_gpio_write,
	.open	= felica_gpio_open,
	.release= felica_gpio_close
 };

static int felica_gpio_gpio_request(int gpio_no, const unsigned char *name)
{
	int rc = 0;

	rc = gpio_request(gpio_no, name);
	if (rc) {
		pr_err("%s: Failed to request gpio %d\n", __func__, gpio_no);
		return rc;
	}

	rc = gpio_direction_input(gpio_no);
	if (rc) {
		pr_err("%s: Failed to direction_input gpio %d\n", __func__, gpio_no);
	}

	return rc;
}

static int felica_gpio_gpio_pon_request(int gpio_no, const unsigned char *name)
{
	int rc = 0;

	rc = gpio_request(gpio_no, name);
	if (rc) {
		pr_err("%s: Failed to request gpio %d\n", __func__, gpio_no);
		return rc;
	}

	rc = gpio_direction_output(gpio_no, 0);
	if (rc) {
		pr_err("%s: Failed to direction_output gpio %d\n", __func__, gpio_no);
	}

	return rc;
}

static int felica_hvdd_write(unsigned char data)
{
	int ret = 0;
    int rc = 0;
	int ret_sub = 0;
	u8 w_wdata = 0;
	static int status = 0;
    unsigned int felica_board_type;
    felica_board_type = get_felica_board_type();
    
	switch (data) {
	case DFD_OUT_L:
		w_wdata = 0;
		break;
	case DFD_OUT_H:
		w_wdata = 1;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	if (ret == 0) {
		if ( (w_wdata == 1) && (status == 0) ) {
            switch (felica_board_type) {
            case E_FELICA_BOARD_TYPE_U9701:
                felica_hvdd = regulator_get(NULL, "felica-hvdd-u9701l-vcc");
                if (IS_ERR(felica_hvdd)) { 
                    ret = -ENXIO;
                    break;
                }
                
                ret = regulator_set_voltage(felica_hvdd, 1800000, 1800000);
                if (ret < 0) {
                    regulator_put(felica_hvdd);
                    ret = -ENXIO;
                    break;
                }
                
                ret = regulator_enable(felica_hvdd);
                if (ret < 0) {
                    regulator_put(felica_hvdd);
                    ret = -ENXIO;                   
                }
                break;
            case E_FELICA_BOARD_TYPE_U9700:
                felica_hvdd = regulator_get(NULL, "felica-hvdd-u9700l-vcc");
                if (IS_ERR(felica_hvdd)) { 
                    ret = -ENXIO;
                    break;
                }
                
                ret = regulator_set_voltage(felica_hvdd, 1800000, 1800000);
                if (ret < 0) {
                    regulator_put(felica_hvdd);
                    ret = -ENXIO;
                    break;
                }
                
                ret = regulator_enable(felica_hvdd); 
                if (ret < 0) {
                    regulator_put(felica_hvdd);
                    ret = -ENXIO;                   
                }
                break;
            default:
                pr_err("felica_board_type unsupported\n");
                break;    
            }			
            mdelay(1);
			status = 1;
		}
	
		if ( (w_wdata == 0) && (status == 1) ) {
            switch (felica_board_type) {
            case E_FELICA_BOARD_TYPE_U9701:
                regulator_disable(felica_hvdd);
                regulator_put(felica_hvdd);
            break;
            case E_FELICA_BOARD_TYPE_U9700:
                regulator_disable(felica_hvdd);
                regulator_put(felica_hvdd);
                break;
            default:
                pr_err("felica_board_type unsupported\n");
                break;    
            }				
			status = 0;
		}
	}

	return ret;
}

static int felica_tvdd_write(unsigned char data)
{
	int ret = 0;
	int ret_sub = 0;
	u8 w_wdata = 0;
	static int status = 0;
    unsigned int felica_board_type;
    felica_board_type = get_felica_board_type();
    
	switch (data) {
	case DFD_OUT_L:
		w_wdata = 0;
		break;
	case DFD_OUT_H:
		w_wdata = 1;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	if (ret == 0) {
		if ( (w_wdata == 1) && (status == 0) ) {
            switch (felica_board_type) {
            case E_FELICA_BOARD_TYPE_U9701:
               
                break;
            case E_FELICA_BOARD_TYPE_U9700:
                felica_tvdd = regulator_get(NULL, "felica-tvdd-u9700l-vcc");
                if (IS_ERR(felica_tvdd)) { 
                    ret = -ENXIO;
                    break;
                }
                
                ret = regulator_set_voltage(felica_tvdd, 1800000, 1800000);
                if (ret < 0) {
                    regulator_put(felica_tvdd);
                    ret = -ENXIO;
                    break;
                }
                
                ret = regulator_enable(felica_tvdd);
                if (ret < 0) {
                    regulator_put(felica_tvdd);
                    ret = -ENXIO;                   
                }
                break;
            default:
                pr_err("felica_board_type unsupported\n");
                break;    
            }			
            mdelay(1);
			status = 1;
		}
		
		if ( (w_wdata == 0) && (status == 1) ) {
            switch (felica_board_type) {
            case E_FELICA_BOARD_TYPE_U9701:
              
                break;
            case E_FELICA_BOARD_TYPE_U9700:
                regulator_disable(felica_tvdd);
                regulator_put(felica_tvdd);
                break;
            default:
                pr_err("felica_board_type unsupported\n");
                break;    
            }	
			status = 0;
		}
	}

	return ret;
}

/* gpio initial */
static int felica_gpio_gpio_init(void)
{
	int ret = 0;

	/* GPIO initial setting */
	ret = felica_gpio_gpio_pon_request(
				FELICA_GPIO_PORT_PON
				, FELICA_GPIO_STR_PON);
	if (ret) {
		return ret;
	}

	ret = felica_gpio_gpio_request(
				FELICA_GPIO_PORT_RFS
				, FELICA_GPIO_STR_RFS);
	if (ret) {
		return ret;
	}

	ret = felica_gpio_gpio_request(
				FELICA_GPIO_PORT_INT
				, FELICA_GPIO_STR_INT);
	if (ret) {
		return ret;
	}

	return ret;
}


/* gpio terminate */
static void felica_gpio_gpio_exit(void)
{
	gpio_free(FELICA_GPIO_PORT_PON);
	gpio_free(FELICA_GPIO_PORT_RFS);
	gpio_free(FELICA_GPIO_PORT_INT);

	return;
}

/*
 * Functions
 */

/**
 *   @brief Initialize module function
 *
 *   @par   Outline:\n
 *          Register FeliCa device driver, \n
 *          and relate system call and the I/F function
 *
 *   @param none
 *
 *   @retval 0   Normal end
 *   @retval <0  Error no of register device
 *
 *   @par Special note
 *     - none
 **/
static int __init felica_gpio_init(void)
{
	int ret = 0;
	int cleanup = DFD_CLUP_NONE;
	struct device *devt;
    int rc = 0;
    if(!is_felica_exist())
		return -ENODEV;      
	memset(&gPonCtrl, 0x00, sizeof(gPonCtrl));
	memset(&gCenCtrl, 0x00, sizeof(gCenCtrl));
	memset(&gRfsCtrl, 0x00, sizeof(gRfsCtrl));
	memset(&gItrCtrl, 0x00, sizeof(gItrCtrl));

	/* init gpio */
    ret = felica_gpio_block_init();
    if(ret < 0) {        
        return ret;
    }

	ret = felica_gpio_gpio_init();
	if (ret < 0) {
		return ret;
	}     
	ret = felica_gpio_int_ctrl_init(DFD_ITR_TYPE_INT);    
    ret = felica_hvdd_write(DFD_OUT_H);
    mdelay(1);

#if 0    
    ret = felica_gpio_term_cen_write(DFD_OUT_H);
    
    mdelay(10);

    ret = felica_gpio_term_pon_write(DFD_OUT_H); 
#endif   
 
	do {
		/* alloc_chrdev_region() for felica_gpio */
		ret = alloc_chrdev_region(&dev_id_pon, FELICA_CTRL_MINOR_NO_PON, FELICA_DEV_NUM, FELICA_DEV_NAME_CTRL_PON);
		if (ret < 0) {
			break;
		}
		cdev_init(&g_cdev_fpon, &felica_fops);
		g_cdev_fpon.owner = THIS_MODULE;
		ret = cdev_add(
				&g_cdev_fpon,
				dev_id_pon,
				FELICA_DEV_NUM);
		if (ret < 0) {
			cleanup = DFD_CLUP_UNREG_CDEV_PON;
			break;
		}
		ret = alloc_chrdev_region(&dev_id_cen, FELICA_CTRL_MINOR_NO_CEN, FELICA_DEV_NUM, FELICA_DEV_NAME_CTRL_CEN);
		if (ret < 0) {
			cleanup = DFD_CLUP_CDEV_DEL_PON;
			break;
		}
		cdev_init(&g_cdev_fcen, &felica_fops);
		g_cdev_fcen.owner = THIS_MODULE;
		ret = cdev_add(
				&g_cdev_fcen,
				dev_id_cen,
				FELICA_DEV_NUM);
		if (ret < 0) {
			cleanup = DFD_CLUP_UNREG_CDEV_CEN;
			break;
		}
		ret = alloc_chrdev_region(&dev_id_rfs, FELICA_CTRL_MINOR_NO_RFS, FELICA_DEV_NUM, FELICA_DEV_NAME_CTRL_RFS);
		if (ret < 0) {
			cleanup = DFD_CLUP_CDEV_DEL_CEN;
			break;
		}
		cdev_init(&g_cdev_frfs, &felica_fops);
		g_cdev_frfs.owner = THIS_MODULE;
		ret = cdev_add(
				&g_cdev_frfs,
				dev_id_rfs,
				FELICA_DEV_NUM);
		if (ret < 0) {
			cleanup = DFD_CLUP_UNREG_CDEV_RFS;
			break;
		}
		ret = alloc_chrdev_region(&dev_id_itr, FELICA_CTRL_MINOR_NO_ITR, FELICA_DEV_NUM, FELICA_DEV_NAME_CTRL_ITR);
		if (ret < 0) {
			cleanup = DFD_CLUP_CDEV_DEL_RFS;
			break;
		}
		cdev_init(&g_cdev_fitr, &felica_fops);
		g_cdev_fitr.owner = THIS_MODULE;
		ret = cdev_add(
				&g_cdev_fitr,
				dev_id_itr,
				FELICA_DEV_NUM);
		if (ret < 0) {
			cleanup = DFD_CLUP_UNREG_CDEV_ITR;
			break;
		}
		ret = alloc_chrdev_region(&dev_id_uart, FELICA_UART_MINOR_NO_UART, FELICA_DEV_NUM, FELICA_DEV_NAME_UART);
		if (ret < 0) {
			unregister_chrdev_region(dev_id_uart, FELICA_DEV_NUM);
			break;
		}
		ret = alloc_chrdev_region(&dev_id_rws, FELICA_RWS_MINOR_NO_RWS, FELICA_DEV_NUM, FELICA_DEV_NAME_RWS);
		if (ret < 0) {
			unregister_chrdev_region(dev_id_rws, FELICA_DEV_NUM);
			break;
		}

		/* class_create() for felica_gpio, felica */
		felica_class = class_create(THIS_MODULE, FELICA_DEV_NAME);
		if (IS_ERR(felica_class)) {
			ret = PTR_ERR(felica_class);
			cleanup = DFD_CLUP_CDEV_DEL_ITR;
			break;
		}

		/* device_create() for felica_gpio */
		devt = device_create(
			felica_class,
			NULL,
			dev_id_pon,
			NULL,
			FELICA_DEV_NAME_CTRL_PON);
		if (IS_ERR(devt)) {
			ret = PTR_ERR(devt);
			cleanup = DFD_CLUP_CLASS_DESTORY;
			break;
		} 
		pon_device = devt;
		devt = device_create(
				felica_class,
				NULL,
				dev_id_cen,
				NULL,
				FELICA_DEV_NAME_CTRL_CEN);
		if (IS_ERR(devt)) {
			ret = PTR_ERR(devt);
			cleanup = DFD_CLUP_DEV_DESTORY_PON;
			break;
		}
		devt = device_create(
				felica_class,
				NULL,
				dev_id_rfs,
				NULL,
				FELICA_DEV_NAME_CTRL_RFS);
		if (IS_ERR(devt)) {
			ret = PTR_ERR(devt);
			cleanup = DFD_CLUP_DEV_DESTORY_CEN;
			break;
		}
		devt = device_create(
				felica_class,
				NULL,
				dev_id_itr,
				NULL,
				FELICA_DEV_NAME_CTRL_ITR);
		if (IS_ERR(devt)) {
			ret = PTR_ERR(devt);
			cleanup = DFD_CLUP_DEV_DESTORY_RFS;
			break;
		}
		/* device_create() for felica */
		devt = device_create(
				felica_class,
				NULL,
				dev_id_uart,
				NULL,
				FELICA_DEV_NAME_UART);
		if (IS_ERR(devt)) {
			ret = PTR_ERR(devt);
			cleanup = DFD_CLUP_DEV_DESTORY_ITR;
			break;
		}
		/* device_create() for felica_rws */
		devt = device_create(
				felica_class,
				NULL,
				dev_id_rws,
				NULL,
				FELICA_DEV_NAME_RWS);
		if (IS_ERR(devt)) {
			ret = PTR_ERR(devt);
			cleanup = DFD_CLUP_DEV_DESTORY_UART;
			break;
		}

	} while(0);
	/* cleanup device driver when failing in the initialization f*/
	felica_gpio_drv_init_cleanup(cleanup);

	return ret;
}

/**
 *   @brief Exit module function
 *
 *   @par   Outline:\n
 *          Erases subscription of device driver
 *
 *   @param none
 *
 *   @retval none
 *
 *   @par Special note
 *     - none
 **/
static void __exit felica_gpio_exit(void)
{
	felica_gpio_drv_init_cleanup(DFD_CLUP_ALL);
	felica_gpio_int_ctrl_exit(DFD_ITR_TYPE_INT);
	felica_gpio_gpio_exit();

	return;
}

/**
 *   @brief Open felica drivers function
 *
 *   @par   Outline:\n
 *          Acquires information on the FeliCa device control, and initializes
 *
 *   @param[in] *pinode  pointer of inode infomation struct
 *   @param[in] *pfile   pointer of file infomation struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -ENODEV  No such device
 *   @retval -EACCES  Permission denied
 *   @retval -ENOMEM  Out of memory
 *   @retval -EIO     I/O error
 *
 *   @par Special note
 *     - none
 **/
static int felica_gpio_open(struct inode *pinode, struct file *pfile)
{
	int ret = 0;
	unsigned char minor = 0;

	if ((pinode == NULL) || (pfile == NULL)) {
		return -EINVAL;
	}

	minor = MINOR(pinode->i_rdev);

	switch (minor) {
	case FELICA_CTRL_MINOR_NO_PON:
		ret = felica_gpio_dev_pon_open(pinode, pfile);
		break;
	case FELICA_CTRL_MINOR_NO_CEN:
		ret = felica_gpio_dev_cen_open(pinode, pfile);
		break;
	case FELICA_CTRL_MINOR_NO_RFS:
		ret = felica_gpio_dev_rfs_open(pinode, pfile);
		break;
	case FELICA_CTRL_MINOR_NO_ITR:
		ret = felica_gpio_dev_itr_open(pinode, pfile);
		break;
	default:
		ret = -ENODEV;
		break;
	}

	return ret;
}

/**
 *   @brief Close felica drivers function
 *
 *   @par   Outline:\n
 *          Releases information on the FeliCa driver control
 *
 *   @param[in] *pinode  pointer of inode infomation struct
 *   @param[in] *pfile   pointer of file infomation struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -ENODEV  No such device
 *   @retval -EIO     I/O error
 *
 *   @par Special note
 *     - none
 **/
static int felica_gpio_close(struct inode *pinode, struct file *pfile)
{
	int ret = 0;
	unsigned char minor = 0;


	if ((pinode == NULL) || (pfile == NULL)) {
		return -EINVAL;
	}

	minor = MINOR(pinode->i_rdev);

	switch (minor) {
	case FELICA_CTRL_MINOR_NO_PON:
		ret = felica_gpio_dev_pon_close(pinode, pfile);
		break;
	case FELICA_CTRL_MINOR_NO_CEN:
		ret = felica_gpio_dev_cen_close(pinode, pfile);
		break;
	case FELICA_CTRL_MINOR_NO_RFS:
		ret = felica_gpio_dev_rfs_close(pinode, pfile);
		break;
	case FELICA_CTRL_MINOR_NO_ITR:
		ret = felica_gpio_dev_itr_close(pinode, pfile);
		break;
	default:
		ret = -ENODEV;
		break;
	}

	return ret;
}


/**
 *   @brief poll function
 *
 *   @par   Outline:\n
 *          Acquires whether or not it is writable to the device,\n
 *          and whether or not it is readable to it
 *
 *   @param[in]     *pfile  pointer of file infomation struct
 *   @param[in/out] *pwait  polling table structure address
 *
 *   @retval Success\n
 *             Returns in the OR value of the reading, the writing in\n
 *               case reading\n
 *                 0                    block's there being\n
 *                 POLLIN | POLLRDNORM  not in the block\n
 *               case writing\n
 *                 0                    block's there being\n
 *                 POLLOUT | POLLWRNORM not in the block\n
 *           Failure\n
 *                 POLLHUP | POLLERR    not in the block of reading and writing
 *
 *   @par Special note
 *     - As for the writing in, because it doesn't use, \n
 *       it always specifies block prosecution.
 *
 **/
static unsigned int felica_gpio_poll(struct file *pfile, struct poll_table_struct *pwait)
{
	unsigned int ret = 0;
	unsigned char minor = 0;
    struct FELICA_CTRLDEV_ITR_INFO *pItr = NULL;

	if (pfile == NULL) {
		ret = (POLLHUP | POLLERR);
		return ret;
	}

	minor = MINOR(pfile->f_dentry->d_inode->i_rdev);

	switch (minor) {
	case FELICA_CTRL_MINOR_NO_ITR:
		pItr = (struct FELICA_CTRLDEV_ITR_INFO*)gItrCtrl.Info;
		if (NULL == pItr) {
			ret = (POLLHUP | POLLERR);
			break;
		}
		poll_wait(pfile, &pItr->RcvWaitQ, pwait);

		/* Readable check */
		if (FELICA_EDGE_NON != pItr->INT_Flag) {
			ret |= (POLLIN  | POLLRDNORM);
		}
		if (FELICA_EDGE_NON != pItr->RFS_Flag) {
			ret |= (POLLIN  | POLLRDNORM);
		}

		/* Writeable is allways not block */
		ret |= (POLLOUT | POLLWRNORM);
		break;
	default:
		ret = (POLLHUP | POLLERR);
		break;
	}

	return ret;
}

/**
 *   @brief Writes felica drivers function
 *
 *   @par   Outline:\n
 *          Writes data in the device from the user space
 *
 *   @param[in] *pfile  pointer of file infomation struct
 *   @param[in] *buff   user buffer address which maintains the data to write
 *   @param[in]  count  The request data transfer size
 *   @param[in] *poff   The file position of the access by the user
 *
 *   @retval >0       Wrote data size
 *   @retval -EINVAL  Invalid argument
 *   @retval -ENODEV  No such device
 *   @retval -EIO     I/O error
 *   @retval -EFAULT  Bad address
 *
 *   @par Special note
 *     - none
 *
 **/
static ssize_t felica_gpio_write(
				struct file	*pfile,
				const char	*buff,
				size_t		count,
				loff_t		*poff)
{
	ssize_t ret = 0;
	unsigned char minor = 0;
	unsigned long ret_cpy = 0;
	int ret_sub = 0;
	struct FELICA_CTRLDEV_PON_INFO *pPonCtrl = NULL;
	struct FELICA_CTRLDEV_CEN_INFO *pCenCtrl = NULL;
	unsigned char local_buff[FELICA_OUT_SIZE];

	if ((pfile == NULL) || (buff == NULL)) {
		return -EINVAL;
	}

	minor = MINOR(pfile->f_dentry->d_inode->i_rdev);

	switch (minor) {
	case FELICA_CTRL_MINOR_NO_PON:
		pPonCtrl = (struct FELICA_CTRLDEV_PON_INFO*)gPonCtrl.Info;
		if (NULL == pPonCtrl) {
			return -EINVAL;
		}

		if (count != FELICA_OUT_SIZE) {
			return -EINVAL;
		}

		ret_cpy = copy_from_user(local_buff, buff, FELICA_OUT_SIZE);
		if (ret_cpy != 0) {
			return -EFAULT;
		}

		if (FELICA_OUT_L == local_buff[0]) {
			ret_sub = felica_gpio_ctrl_offline();
		}
		else if (FELICA_OUT_H == local_buff[0]) {
			ret_sub = felica_gpio_ctrl_online(&(pPonCtrl->PON_Timer));
		}
		else {
			ret_sub = -EINVAL;
		}

		if (ret_sub == 0) {
			ret = FELICA_OUT_SIZE;
		}
		else {
			ret = ret_sub;
		}
		break;

	case FELICA_CTRL_MINOR_NO_CEN:
		pCenCtrl = (struct FELICA_CTRLDEV_CEN_INFO*)gCenCtrl.Info;
		if (NULL == pCenCtrl) {
			return -EINVAL;
		}

		if (count != FELICA_OUT_SIZE) {
			return -EINVAL;
		}

		ret_cpy = copy_from_user(local_buff, buff, FELICA_OUT_SIZE);
		if (ret_cpy != 0) {
			return -EFAULT;
		}

		if (FELICA_OUT_L == local_buff[0]) {
			ret_sub = felica_gpio_ctrl_unenable(
						  &pCenCtrl->PON_Timer
						, &pCenCtrl->CEN_Timer);
		}
		else if (FELICA_OUT_H == local_buff[0]) {
			ret_sub = felica_gpio_ctrl_enable(
						  &pCenCtrl->CEN_Timer);
		}
		else {
			ret_sub = -EINVAL;
		}

		if (ret_sub == 0) {
			ret = FELICA_OUT_SIZE;
		}
		else {
			ret = ret_sub;
		}
		break;

	default:
		ret = -ENODEV;
		break;
	}

	return ret;
}

/**
 *   @brief Read felica drivers function
 *
 *   @par   Outline:\n
 *          Read data from the device to the user space
 *
 *   @param[in] *pfile  pointer of file infomation struct
 *   @param[in] *buff   user buffer address which maintains the data to read
 *   @param[in]  count  The request data transfer size
 *   @param[in] *poff   The file position of the access by the user
 *
 *   @retval >0        Read data size
 *   @retval -EINVAL   Invalid argument
 *   @retval -EFAULT   Bad address
 *   @retval -EBADF    Bad file number
 *   @retval -ENODEV   No such device
 *
 *   @par Special note
 *     - none
 *
 **/
static ssize_t felica_gpio_read(
				struct file	*pfile,
				char		*buff,
				size_t		count,
				loff_t		*poff)
{
	ssize_t 		ret = 0;
	unsigned char	local_buff[max(FELICA_OUT_SIZE, FELICA_EDGE_OUT_SIZE)];
	unsigned char	minor = 0;
	int 			ret_sub = 0;
	unsigned char	w_rdata;
	unsigned long	ret_cpy = 0;
	struct FELICA_CTRLDEV_ITR_INFO *pItr;

	if ((pfile == NULL) || (buff == NULL)) {
		return -EINVAL;
	}

	minor = MINOR(pfile->f_dentry->d_inode->i_rdev);

	switch (minor) {
	case FELICA_CTRL_MINOR_NO_CEN:
		if (count != FELICA_OUT_SIZE) {
			return -EINVAL;
		}

		ret_sub = felica_gpio_term_cen_read(&w_rdata);
		if (ret_sub != 0) {
			return ret_sub;
		}

		switch (w_rdata) {
		case DFD_OUT_H:
			local_buff[0] = FELICA_OUT_H;
			break;
		case DFD_OUT_L:
		default:
			local_buff[0] = FELICA_OUT_L;
			break;
		}

		ret_cpy = copy_to_user(buff, local_buff, FELICA_OUT_SIZE);
		if (ret_cpy != 0) {
			return -EFAULT;
		}
		else {
			ret = FELICA_OUT_SIZE;
		}
		break;

	case FELICA_CTRL_MINOR_NO_RFS:
		if (count != FELICA_OUT_SIZE) {
			return -EINVAL;
		}

		ret_sub = felica_gpio_term_rfs_read(&w_rdata);
		if (ret_sub != 0) {
			return ret_sub;
		}

		switch (w_rdata) {
		case DFD_OUT_L:
			local_buff[0] = FELICA_OUT_RFS_L;
			break;
		case DFD_OUT_H:
		default:
			local_buff[0] = FELICA_OUT_RFS_H;
			break;
		}

		ret_cpy = copy_to_user(buff, local_buff, FELICA_OUT_SIZE);
		if (ret_cpy != 0) {
			return -EFAULT;
		}
		else {
			ret = FELICA_OUT_SIZE;
		}
		break;

	case FELICA_CTRL_MINOR_NO_ITR:
		pItr = (struct FELICA_CTRLDEV_ITR_INFO*)gItrCtrl.Info;
		if (NULL == pItr) {
			return -EINVAL;
		}

		if (count != FELICA_EDGE_OUT_SIZE) {
			return -EINVAL;
		}

		/* Begin disabling interrupts for protecting RFS and INT Flag */
		spin_lock_irqsave(&itr_lock, itr_lock_flag);

		local_buff[0] = pItr->RFS_Flag;
		local_buff[1] = pItr->INT_Flag;
		pItr->RFS_Flag = FELICA_EDGE_NON;
		pItr->INT_Flag = FELICA_EDGE_NON;

		/* End disabling interrupts for protecting RFS and INT Flag */
		spin_unlock_irqrestore(&itr_lock, itr_lock_flag);

		ret_cpy = copy_to_user(buff, local_buff, FELICA_EDGE_OUT_SIZE);
		if (ret_cpy != 0) {
			return -EFAULT;
		}
		else {
			ret = FELICA_EDGE_OUT_SIZE;
		}
		break;

	default:
		ret = -ENODEV;
		break;
	}

	return ret;
}

/**
 *   @brief INT interrupts handler
 *
 *   @par   Outline:\n
 *          Called when INT interrups from FeliCa device
 *
 *   @param[in]  i_req     IRQ number
 *   @param[in] *pv_devid  All-round pointer\n
 *                         (The argument which was set at the function "request_irq()")
 *   @retval IRQ_HANDLED  Interrupt was handled by this device
 *
 *   @par Special note
 *     - none
 *
 **/
static irqreturn_t felica_gpio_int_INT(int i_req, void *pv_devid)
{
	struct FELICA_CTRLDEV_ITR_INFO *pItr = (struct FELICA_CTRLDEV_ITR_INFO*)gItrCtrl.Info;
	int ret_sub = 0;
	unsigned char rdata = DFD_OUT_H;

	if (NULL == pItr) {
		return IRQ_HANDLED;
	}

	ret_sub = felica_gpio_term_int_read(&rdata);

	if ((ret_sub == 0) && (rdata == DFD_OUT_L)) {
		pItr->INT_Flag = FELICA_EDGE_L;
		wake_up_interruptible(&(pItr->RcvWaitQ));
	}

	return IRQ_HANDLED;
}


/**
 *   @brief Generic timer handler
 *
 *   @par   Outline:\n
 *          Called when timeout for generic
 *
 *   @param[in] p  Address of the timer structure
 *
 *   @retval none
 *
 *   @par Special note
 *     - none
 *
 **/
static void felica_gpio_timer_hnd_general(unsigned long p)
{
	struct FELICA_TIMER *pTimer;

	pTimer = (struct FELICA_TIMER *)p;
	if (pTimer != NULL) {
		wake_up_interruptible(&pTimer->wait);
	}

	return;
}


/**
 *   @brief Enable FeliCa control
 *
 *   @param[in] *pTimer  pointer of timer data(CEN terminal control timer)
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EBUSY   Device or resource busy
 *
 *   @par Special note
 *     - none
 **/
static int felica_gpio_ctrl_enable(struct FELICA_TIMER *pTimer)
{
	int ret = 0;
	int ret_sub = 0;

	if (NULL == pTimer) {
		return -EINVAL;
	}

	ret_sub = timer_pending(&pTimer->Timer);

	if (ret_sub != 0) {
		return -EBUSY;
	}

	ret = felica_gpio_term_cen_write(DFD_OUT_H);

	if (ret >= 0) {
		/* wait cen timer */
		pTimer->Timer.function = felica_gpio_timer_hnd_general;
		pTimer->Timer.data	   = (unsigned long)pTimer;
		mod_timer(&pTimer->Timer, jiffies + FELICA_TIMER_CEN_TERMINAL_WAIT);

		wait_event_interruptible(
			pTimer->wait,
			(timer_pending(&pTimer->Timer) == 0));

		ret_sub = timer_pending(&pTimer->Timer);
		if (ret_sub == 1) {
			del_timer(&pTimer->Timer);
		}
	}

	return ret;
}

/**
 *   @brief Disable FeliCa control
 *
 *   @param[in] *pPonTimer  pointer of timer data(PON terminal control timer)
 *   @param[in] *pCenTimer  pointer of timer data(PON terminal control timer)
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EBUSY   Device or resource busy
 *
 *   @par Special note
 *     - none
 **/
static int felica_gpio_ctrl_unenable(
					struct FELICA_TIMER *pPonTimer,
					struct FELICA_TIMER *pCenTimer)
{
	int ret = 0;
	int ret_sub = 0;

	if ((NULL == pPonTimer) || (NULL == pCenTimer)) {
		return -EINVAL;
	}

	(void)felica_gpio_ctrl_offline();

	/* wait pon timer */
	pPonTimer->Timer.function = felica_gpio_timer_hnd_general;
	pPonTimer->Timer.data	   = (unsigned long)pPonTimer;
	mod_timer(&pPonTimer->Timer, jiffies + FELICA_TIMER_PON_TERMINAL_WAIT);

	wait_event_interruptible(
		pPonTimer->wait,
		(timer_pending(&pPonTimer->Timer) == 0));

	ret_sub = timer_pending(&pPonTimer->Timer);
	if (ret_sub == 1) {
		del_timer(&pPonTimer->Timer);
	}

	ret = felica_gpio_term_cen_write(DFD_OUT_L);

	if (ret == 0) {
		pCenTimer->Timer.function = felica_gpio_timer_hnd_general;
		pCenTimer->Timer.data	   = (unsigned long)pCenTimer;
		mod_timer(&pCenTimer->Timer, jiffies + FELICA_TIMER_CEN_TERMINAL_WAIT);

		wait_event_interruptible(
			pCenTimer->wait,
			(timer_pending(&pCenTimer->Timer) == 0));

		ret_sub = timer_pending(&pCenTimer->Timer);
		if (ret_sub == 1) {
			del_timer(&pCenTimer->Timer);
		}
	}

	return ret;
}

/**
 *   @brief Enable FeliCa IC function
 *
 *   @param[in] *pTimer  pointer of timer data(PON terminal control timer)
 *
 *   @retval 0     Normal end
 *   @retval -EIO  I/O error
 *
 *   @par Special note
 *     - none
 **/
static int felica_gpio_ctrl_online(struct FELICA_TIMER *pTimer)
{
	int ret = 0;
	int ret_sub = 0;
	unsigned char rdata;

	if (NULL == pTimer) {
		return -EINVAL;
	}

	ret_sub = felica_gpio_term_cen_read(&rdata);
	if ((ret_sub != 0) || (rdata == DFD_OUT_L)) {
		ret = -EIO;
	}

	if (ret == 0) {
		ret_sub = felica_gpio_term_pon_write(DFD_OUT_H);
		if (ret_sub != 0) {
			ret = ret_sub;
		}
	
	}
	if (ret == 0) {
		pTimer->Timer.function = felica_gpio_timer_hnd_general;
		pTimer->Timer.data     = (unsigned long)pTimer;
		mod_timer(&pTimer->Timer, jiffies + FELICA_TIMER_PON_TERMINAL_WAIT);

		wait_event_interruptible(
			pTimer->wait,
			(timer_pending(&pTimer->Timer) == 0));

		ret_sub = timer_pending(&pTimer->Timer);
		if (ret_sub == 1) {
			del_timer(&pTimer->Timer);
		}
	}

	return ret;
}

/**
 *   @brief Disable FeliCa IC function
 *
 *   @param  none
 *
 *   @retval 0     Normal end
 *   @retval -EIO  I/O error
 *
 *   @par Special note
 *     - none
 **/
static int felica_gpio_ctrl_offline(void)
{
	int ret = 0;
	int ret_sub = 0;
	unsigned char rdata;

	ret_sub = felica_gpio_term_cen_read(&rdata);
	if ((ret_sub != 0) || (rdata == DFD_OUT_L)) {
		ret = -EIO;
	}
	if (ret == 0) {
		ret_sub = felica_gpio_term_pon_write(DFD_OUT_L);

		if (ret_sub != 0) {
			ret = ret_sub;
		}
	}

	return ret;
}

/**
 *   @brief Read CEN Terminal settings
 *
 *   @param[out] cen_data  Status of the CEN terminal
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EIO     I/O error
 *
 *   @par Special note
 *     - none
 **/
static int felica_gpio_term_cen_read(unsigned char *cen_data)
{
	int ret = 0;
    int data = 0;
    
	data = ak6921af_read_eeprom();
	if (data == 1) {
		*cen_data = DFD_OUT_H;
	}
	else if (data == 0) {
		*cen_data = DFD_OUT_L;
	}
	else {
		ret = -EIO;
	}

	return ret;
}

/**
 *   @brief Read RFS Terminal settings
 *
 *   @param[out] rfs_data  Status of the RFS terminal
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EIO     I/O error
 *
 *   @par Special note
 *     - none
 **/
static int felica_gpio_term_rfs_read(unsigned char *rfs_data)
{

	int ret = 0;
	int w_rdata;

	if (NULL == rfs_data) {
		return -EINVAL;
	}

	w_rdata = gpio_get_value(FELICA_GPIO_PORT_RFS);

	if (w_rdata == 0) {
		*rfs_data = DFD_OUT_L;
	}
	else {
		*rfs_data = DFD_OUT_H;
	}

	return ret;
}

/**
 *   @brief Read INT Terminal settings
 *
 *   @param[out] int_data  Status of the INT terminal
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *
 *   @par Special note
 *     - none
 **/
static int felica_gpio_term_int_read(unsigned char *int_data)
{
	int ret = 0;
	int w_rdata;

	if (NULL == int_data) {
		return -EINVAL;
	}

	w_rdata = gpio_get_value(FELICA_GPIO_PORT_INT);

	if (w_rdata == 0) {
		*int_data = DFD_OUT_L;
	}
	else {
		*int_data = DFD_OUT_H;
	}

	return ret;
}

/**
 *   @brief Write CEN Terminal settings
 *
 *   @param[in] cen_data  Value of the CEN terminal to set
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EIO     I/O error
 *
 *   @par Special note
 *     - none
 **/
static int felica_gpio_term_cen_write(unsigned char cen_data)
{
	int ret = 0;
	int w_wdata = 0;

	switch (cen_data) {
	case DFD_OUT_H:
		w_wdata = 1;
		break;
	case DFD_OUT_L:
		w_wdata = 0;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	if (ret == 0) {
		ret = ak6921af_write_eeprom(w_wdata);
	}

	return ret;
}

/**
 *   @brief Write PON Terminal settings
 *
 *   @param[in] pon_data  Value of the PON terminal to set
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EIO     I/O error
 *
 *   @par Special note
 *     - none
 **/
static int felica_gpio_term_pon_write(unsigned char pon_data)
{
	int ret = 0;
	int ret_sub = 0;
	u8 w_wdata = 0;
	static int status = 0;

	switch (pon_data) {
	case DFD_OUT_L:
		w_wdata = 0;
		break;
	case DFD_OUT_H:
		w_wdata = 1;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	if (ret == 0) {
		if ( (w_wdata == 1) && (status == 0) ) {
			felica_tvdd_write(DFD_OUT_H);
            mdelay(1);
			status = 1;
		}
		ret_sub = gpio_direction_output(FELICA_GPIO_PORT_PON, w_wdata);
		if (ret_sub != 0) {
			ret = -EIO;
		}
		if ( (w_wdata == 0) && (status == 1) ) {
			felica_tvdd_write(DFD_OUT_L);
			status = 0;
		}
	}

	return ret;
}

/**
 *   @brief Initialize interrupt control
 *
 *   @param[in] itr_type  interrupt type
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EIO     I/O error
 *
 *   @par Special note
 *     - none
 **/
static int felica_gpio_int_ctrl_init(unsigned char itr_type)
{
	int ret = 0;
	unsigned gpio_type = 0;
	const char *gpio_label = NULL;

	switch (itr_type) {
	case DFD_ITR_TYPE_RFS:
		gpio_type = FELICA_GPIO_PORT_RFS;
		gpio_label = FELICA_GPIO_STR_RFS;
		break;
	case DFD_ITR_TYPE_INT:
		gpio_type = FELICA_GPIO_PORT_INT;
		gpio_label = FELICA_GPIO_STR_INT;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	if (ret == 0) {
		gFeliCaItrInfo[itr_type].irq = IRQ_GPIO(gpio_type);
	}

	return ret;
}

/**
 *   @brief Exit interrupt control
 *
 *   @param[in] itr_type  interrupt type
 *
 *   @retval none
 *
 *   @par Special note
 *     - none
 **/
static void felica_gpio_int_ctrl_exit(unsigned char itr_type)
{
	int ret = 0;
	unsigned gpio_type = 0;

	switch (itr_type) {
	case DFD_ITR_TYPE_RFS:
		gpio_type = FELICA_GPIO_PORT_RFS;
		break;
	case DFD_ITR_TYPE_INT:
		gpio_type = FELICA_GPIO_PORT_INT;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	if (ret == 0) {
	}

	return;
}

/**
 *   @brief Register interrupt control
 *
 *   @param[in] itr_type  interrupt type
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EIO     I/O error
 *
 *   @par Special note
 *     - none
 **/
static int felica_gpio_int_ctrl_regist(unsigned char itr_type)
{
	int ret = 0;
	int ret_sub = 0;

	switch (itr_type) {
	case DFD_ITR_TYPE_RFS:
	case DFD_ITR_TYPE_INT:
		break;
	default:
		ret = -EINVAL;
		break;
	}

	if (ret == 0) {
		ret_sub = request_irq(
					gFeliCaItrInfo[itr_type].irq,
					gFeliCaItrInfo[itr_type].handler,
					gFeliCaItrInfo[itr_type].flags,
					gFeliCaItrInfo[itr_type].name,
					gFeliCaItrInfo[itr_type].dev);
		if (ret_sub != 0) {
			ret = -EIO;
		} else {
            enable_irq_wake(gFeliCaItrInfo[itr_type].irq);
		}
	}

	return ret;
}

/**
 *   @brief Unregister interrupt control
 *
 *   @param[in] itr_type  interrupt type
 *
 *   @retval none
 *
 *   @par Special note
 *     - none
 **/
static void felica_gpio_int_ctrl_unregist(unsigned char itr_type)
{
	int ret = 0;

	switch (itr_type) {
	case DFD_ITR_TYPE_RFS:
	case DFD_ITR_TYPE_INT:
		break;
	default:
		ret = -EINVAL;
		break;
	}

	if (ret == 0) {
        disable_irq_wake(gFeliCaItrInfo[itr_type].irq);
		free_irq(
				gFeliCaItrInfo[itr_type].irq,
				gFeliCaItrInfo[itr_type].dev);
	}

	return;
}


/**
 *   @brief Initialize timer data
 *
 *   @param[in] pTimer  timer data
 *
 *   @retval none
 *
 *   @par Special note
 *     - none
 **/
static void felica_gpio_drv_timer_data_init(struct FELICA_TIMER *pTimer)
{

	if (pTimer != NULL) {
		init_timer(&pTimer->Timer);
		init_waitqueue_head(&pTimer->wait);
	}

	return;
}


/**
 *   @brief cleanup driver setup
 *
 *   @par   Outline:\n
 *          For cleaning device driver setup info
 *
 *   @param[in] cleanup  cleanup point
 *
 *   @retval none
 *
 *   @par Special note
 *     - none
 **/
static void felica_gpio_drv_init_cleanup(int cleanup)
{

	/* cleanup */
	switch (cleanup) {
	case DFD_CLUP_ALL:
	case DFD_CLUP_DEV_DESTORY_RWS:
		device_destroy(
			felica_class,
			dev_id_rws);
	case DFD_CLUP_DEV_DESTORY_UART:
		device_destroy(
			felica_class,
			dev_id_uart);
	case DFD_CLUP_DEV_DESTORY_ITR:
		device_destroy(
			felica_class,
			dev_id_itr);
	case DFD_CLUP_DEV_DESTORY_RFS:
		device_destroy(
			felica_class,
			dev_id_rfs);
	case DFD_CLUP_DEV_DESTORY_CEN:
		device_destroy(
			felica_class,
			dev_id_cen);
	case DFD_CLUP_DEV_DESTORY_PON:
		device_destroy(
			felica_class,
			dev_id_pon);
	case DFD_CLUP_CLASS_DESTORY:
		class_destroy(felica_class);
	case DFD_CLUP_CDEV_DEL_ITR:
		cdev_del(&g_cdev_fitr);
	case DFD_CLUP_UNREG_CDEV_ITR:
		unregister_chrdev_region(
			dev_id_itr,
			FELICA_DEV_NUM);
	case DFD_CLUP_CDEV_DEL_RFS:
		cdev_del(&g_cdev_frfs);
	case DFD_CLUP_UNREG_CDEV_RFS:
		unregister_chrdev_region(
			dev_id_rfs,
			FELICA_DEV_NUM);
	case DFD_CLUP_CDEV_DEL_CEN:
		cdev_del(&g_cdev_fcen);
	case DFD_CLUP_UNREG_CDEV_CEN:
		unregister_chrdev_region(
			dev_id_cen,
			FELICA_DEV_NUM);
	case DFD_CLUP_CDEV_DEL_PON:
		cdev_del(&g_cdev_fpon);
	case DFD_CLUP_UNREG_CDEV_PON:
		unregister_chrdev_region(
			dev_id_pon,
			FELICA_DEV_NUM);
	case DFD_CLUP_NONE:
	default:
		break;
	}

	return;
}

/**
 *   @brief Open PON device contorl driver of FeliCa
 *
 *   @par   Outline:\n
 *          Initialize internal infomation, and count up open counter
 *
 *   @param[in] *pinode  pointer of inode infomation struct
 *   @param[in] *pfile   pointer of file infomation struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EACCES  Permission denied
 *   @retval -ENOMEM  Out of memory
 *
 *   @par Special note
 *     - none
 **/
static int felica_gpio_dev_pon_open(struct inode *pinode, struct file *pfile)
{
	int ret = 0;
	struct FELICA_CTRLDEV_PON_INFO *pCtrl = NULL;

	if ((NULL == pfile) || (NULL == pinode)) {
		return -EINVAL;
	}

	if ((pfile->f_flags & O_ACCMODE) == O_RDONLY) {
		return -EACCES;
	}
	else if ((pfile->f_flags & O_ACCMODE) != O_WRONLY) {
		return -EINVAL;
	}
	else {
	}

	if ((gPonCtrl.w_cnt == 0) && (gPonCtrl.Info != NULL)) {
		kfree(gPonCtrl.Info);
		gPonCtrl.Info = NULL;
	}

	pCtrl = gPonCtrl.Info;

	if (NULL == pCtrl) {
		pCtrl = kmalloc(sizeof(*pCtrl), GFP_KERNEL);
		if (NULL == pCtrl) {
			return -ENOMEM;
		}
		gPonCtrl.Info = pCtrl;
		felica_gpio_drv_timer_data_init(&pCtrl->PON_Timer);

		gPonCtrl.w_cnt = 0;
	}

	gPonCtrl.w_cnt++;

	return ret;
}

/**
 *   @brief Open CEN device contorl driver of FeliCa
 *
 *   @par   Outline:\n
 *          Initialize internal infomation, and count up open counter
 *
 *   @param[in] *pinode  pointer of inode infomation struct
 *   @param[in] *pfile   pointer of file infomation struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EACCES  Permission denied
 *   @retval -ENOMEM  Out of memory
 *
 *   @par Special note
 *     - none
 **/
static int felica_gpio_dev_cen_open(struct inode *pinode, struct file *pfile)
{
	int ret = 0;
	struct FELICA_CTRLDEV_CEN_INFO *pCtrl = NULL;

	if ((NULL == pfile) || (NULL == pinode)) {
		return -EINVAL;
	}

	if (((pfile->f_flags & O_ACCMODE) != O_RDONLY)
		&& ((pfile->f_flags & O_ACCMODE) != O_WRONLY)) {
		return -EINVAL;
    }

	if ((gCenCtrl.w_cnt == 0)
				&& (gCenCtrl.r_cnt == 0)
				&& (NULL != gCenCtrl.Info)) {
		kfree(gCenCtrl.Info);
		gCenCtrl.Info = NULL;
	}

	pCtrl = gCenCtrl.Info;

	if (NULL == pCtrl) {
		pCtrl = kmalloc(sizeof(*pCtrl), GFP_KERNEL);
		if (NULL == pCtrl) {
			return -ENOMEM;
		}
		gCenCtrl.Info = pCtrl;
		felica_gpio_drv_timer_data_init(&pCtrl->PON_Timer);
		felica_gpio_drv_timer_data_init(&pCtrl->CEN_Timer);

		gCenCtrl.r_cnt = 0;
		gCenCtrl.w_cnt = 0;
	}

	if ((pfile->f_flags & O_ACCMODE) == O_RDONLY) {
		gCenCtrl.r_cnt++;
	}
	else {
		gCenCtrl.w_cnt++;
	}

	return ret;
}

/**
 *   @brief Open RFS device contorl driver of FeliCa
 *
 *   @par   Outline:\n
 *          Initialize internal infomation, and count up open counter
 *
 *   @param[in] *pinode  pointer of inode infomation struct
 *   @param[in] *pfile   pointer of file infomation struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EACCES  Permission denied
 *
 *   @par Special note
 *     - none
 **/
static int felica_gpio_dev_rfs_open(struct inode *pinode, struct file *pfile)
{
	int ret = 0;

	if ((NULL == pfile) || (NULL == pinode)) {
		return -EINVAL;
	}

	if ((pfile->f_flags & O_ACCMODE) == O_WRONLY) {
		return -EACCES;
	}
	else if ((pfile->f_flags & O_ACCMODE) != O_RDONLY) {
		return -EINVAL;
	}
	else {
	}

	gRfsCtrl.r_cnt++;

	return ret;
}

/**
 *   @brief Open ITR device contorl driver of FeliCa
 *
 *   @par   Outline:\n
 *          Initialize internal infomation, and count up open counter
 *
 *   @param[in] *pinode  pointer of inode infomation struct
 *   @param[in] *pfile   pointer of file infomation struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EACCES  Permission denied
 *   @retval -ENOMEM  Out of memory
 *
 *   @par Special note
 *     - none
 **/
static int felica_gpio_dev_itr_open(struct inode *pinode, struct file *pfile)
{
	int ret = 0;
	struct FELICA_CTRLDEV_ITR_INFO *pCtrl = NULL;

	if ((NULL == pfile) || (NULL == pinode)) {
		return -EINVAL;
	}

	if ((pfile->f_flags & O_ACCMODE) == O_WRONLY) {
		return -EACCES;
	}
	else if ((pfile->f_flags & O_ACCMODE) != O_RDONLY) {
		return -EINVAL;
	}
	else if (gItrCtrl.r_cnt >= FELICA_DEV_ITR_MAX_OPEN_NUM) {
		return -EINVAL;
	}
	else {
	}

	if ((gItrCtrl.r_cnt == 0) && (gItrCtrl.Info != NULL)) {
		kfree(gItrCtrl.Info);
		gItrCtrl.Info = NULL;
	}

	pCtrl = gItrCtrl.Info;

	if (NULL == pCtrl) {
		pCtrl = kmalloc(sizeof(*pCtrl), GFP_KERNEL);
		if (NULL == pCtrl) {
			return -ENOMEM;
		}
		gItrCtrl.Info = pCtrl;
		init_waitqueue_head(&pCtrl->RcvWaitQ);
		spin_lock_init(&itr_lock);
		gItrCtrl.r_cnt = 0;
		gItrCtrl.w_cnt = 0;
		pCtrl->INT_Flag = FELICA_EDGE_NON;
		pCtrl->RFS_Flag = FELICA_EDGE_NON;
		gFeliCaItrInfo[DFD_ITR_TYPE_INT].dev = pCtrl;
	}

	if (gItrCtrl.r_cnt == 0) {
		do {
			ret = felica_gpio_int_ctrl_regist(DFD_ITR_TYPE_INT);
			if (ret != 0) {
				break;
			}
		} while (0);
	}

	if (ret == 0) {
		gItrCtrl.r_cnt++;
	}
	else if (gItrCtrl.r_cnt == 0) {
		kfree(gItrCtrl.Info);
		gItrCtrl.Info = NULL;
	}
	else {
	}

	return ret;
}

/**
 *   @brief Close PON device contorl driver of FeliCa
 *
 *   @par   Outline:\n
 *          Free internal infomation, and count down open counter
 *
 *   @param[in] *pinode  pointer of inode infomation struct
 *   @param[in] *pfile   pointer of file infomation struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *
 *   @par Special note
 *     - none
 **/
static int felica_gpio_dev_pon_close(struct inode *pinode, struct file *pfile)
{
	int    ret = 0;
	struct FELICA_CTRLDEV_PON_INFO *pCtrl =
				 (struct FELICA_CTRLDEV_PON_INFO*)gPonCtrl.Info;

	if ((NULL == pfile) || (NULL == pinode)) {
		return -EINVAL;
	}

	if (NULL == pCtrl) {
		return -EINVAL;
	}

	if (((pfile->f_flags & O_ACCMODE) == O_WRONLY) && (gPonCtrl.w_cnt > 0)) {
		gPonCtrl.w_cnt--;
	}
	else {
		return -EINVAL;
	}

	if (gPonCtrl.w_cnt == 0) {
		if (timer_pending(&pCtrl->PON_Timer.Timer) == 1) {
			del_timer(&pCtrl->PON_Timer.Timer);
			wake_up_interruptible(&pCtrl->PON_Timer.wait);
		}

		(void)felica_gpio_term_pon_write(DFD_OUT_L);

		kfree(gPonCtrl.Info);
		gPonCtrl.Info = NULL;
	}

	return ret;
}

/**
 *   @brief Close CEN device contorl driver of FeliCa
 *
 *   @par   Outline:\n
 *          Free internal infomation, and count down open counter
 *
 *   @param[in] *pinode  pointer of inode infomation struct
 *   @param[in] *pfile   pointer of file infomation struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *
 *   @par Special note
 *     - none
 **/
static int felica_gpio_dev_cen_close(struct inode *pinode, struct file *pfile)
{
	int    ret = 0;
	struct FELICA_CTRLDEV_CEN_INFO *pCtrl =
				(struct FELICA_CTRLDEV_CEN_INFO*)gCenCtrl.Info;

	if ((NULL == pfile) || (NULL == pinode)) {
		return -EINVAL;
	}

	if (NULL == pCtrl) {
		return -EINVAL;
	}

	if (((pfile->f_flags & O_ACCMODE) == O_RDONLY)
							&& (gCenCtrl.r_cnt > 0)) {
		gCenCtrl.r_cnt--;
	}
	else if (((pfile->f_flags & O_ACCMODE) == O_WRONLY)	&& (gCenCtrl.w_cnt > 0)) {
		gCenCtrl.w_cnt--;
	}
	else {
		return -EINVAL;
	}

	if ((gCenCtrl.r_cnt == 0) && (gCenCtrl.w_cnt == 0)) {
		if (timer_pending(&pCtrl->PON_Timer.Timer) == 1) {
			del_timer(&pCtrl->PON_Timer.Timer);
			wake_up_interruptible(&pCtrl->PON_Timer.wait);
		}

		if (timer_pending(&pCtrl->CEN_Timer.Timer) == 1) {
			del_timer(&pCtrl->CEN_Timer.Timer);
			wake_up_interruptible(&pCtrl->CEN_Timer.wait);
		}

		kfree(gCenCtrl.Info);
		gCenCtrl.Info = NULL;
	}

	return ret;
}

/**
 *   @brief Close RFS device contorl driver of FeliCa
 *
 *   @par   Outline:\n
 *          Free internal infomation, and count down open counter
 *
 *   @param[in] *pinode  pointer of inode infomation struct
 *   @param[in] *pfile   pointer of file infomation struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *
 *   @par Special note
 *     - none
 **/
static int felica_gpio_dev_rfs_close(
					struct inode *pinode,
					struct file  *pfile)
{
	int ret = 0;

	if ((NULL == pfile) || (NULL == pinode)) {
		return -EINVAL;
	}

	if (((pfile->f_flags & O_ACCMODE) == O_RDONLY)
							&& (gRfsCtrl.r_cnt > 0)) {
		gRfsCtrl.r_cnt--;
	}
	else {
		return -EINVAL;
	}

	return ret;
}

/**
 *   @brief Close ITR device contorl driver of FeliCa
 *
 *   @par   Outline:\n
 *          Free internal infomation, and count down open counter
 *
 *   @param[in] *pinode  pointer of inode infomation struct
 *   @param[in] *pfile   pointer of file infomation struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *
 *   @par Special note
 *     - none
 **/
static int felica_gpio_dev_itr_close(
					struct inode *pinode,
					struct file  *pfile)
{
	int ret = 0;
	struct FELICA_CTRLDEV_ITR_INFO *pCtrl =
				(struct FELICA_CTRLDEV_ITR_INFO*)gItrCtrl.Info;

	if ((NULL == pfile) || (NULL == pinode)) {
		return -EINVAL;
	}

	if (NULL == pCtrl) {
		return -EINVAL;
	}

	if (((pfile->f_flags & O_ACCMODE) == O_RDONLY)
		&& (gItrCtrl.r_cnt > 0)
		&& (gItrCtrl.r_cnt <= FELICA_DEV_ITR_MAX_OPEN_NUM)) {
		gItrCtrl.r_cnt--;
	}
	else {
		return -EINVAL;
	}

	wake_up_interruptible(&pCtrl->RcvWaitQ);

	if (gItrCtrl.r_cnt == 0) {
		felica_gpio_int_ctrl_unregist(DFD_ITR_TYPE_INT);

		kfree(gItrCtrl.Info);
		gItrCtrl.Info = NULL;
	}

	return ret;
}

static int felica_pm_suspend(struct platform_device *dev)
{
    int ret = 0;    
    ret = blockmux_set(g_felica_gpio_block, g_felica_gpio_block_config, LOWPOWER);
    if (ret < 0) {
        printk(KERN_ERR "%s: failed to config gpio\n", __func__);
    }
    return 0;
}

static int felica_pm_resume(struct platform_device *dev)
{
    int ret = 0;    
    ret = blockmux_set(g_felica_gpio_block, g_felica_gpio_block_config, NORMAL);
    if (ret<0) {
        printk(KERN_ERR "%s: failed to config gpio\n", __func__);
    }
	return 0;
}

static int felica_probe(struct platform_device *dev)
{
    return felica_gpio_init();
}

static int felica_remove(struct platform_device *dev)
{
	felica_gpio_exit();
	return 0;
}

static struct platform_driver felica_plat_driver = {
	.probe = felica_probe,
    .remove = felica_remove,
	.driver	= {
		.name = "felica",
		.owner = THIS_MODULE,		
	},
    .suspend = felica_pm_suspend,
	.resume = felica_pm_resume, 
};

static struct platform_device felica_plat_device = {
	.name = "felica",	
};

static int __init felica_init_module(void)
{
    int ret;
    if(!is_felica_exist())
		return -ENODEV;
    	
	ret = platform_device_register(&felica_plat_device);
	return platform_driver_register(&felica_plat_driver);
}

static void __exit felica_exit_module(void)
{ 
    platform_device_unregister(&felica_plat_device);
    platform_driver_unregister(&felica_plat_driver);    
}

module_init(felica_init_module);
module_exit(felica_exit_module);
MODULE_AUTHOR("NTT DATA MSE Co., Ltd.");
MODULE_DESCRIPTION("FeliCa GPIO Driver");
MODULE_LICENSE("GPL");
