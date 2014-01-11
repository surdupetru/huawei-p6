/*
 * Copyright(C) 2012 NTT DATA MSE CORPORATION. All right reserved.
 *
 * @file felica_rws.c
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

/*
 * include files
 */
#include <linux/err.h>				/* PTR_ERR/IS_ERR           */
#include <linux/string.h>			/* memset                   */
#include <linux/errno.h>			/* errno                    */
#include <linux/module.h>			/* Module parameters        */
#include <linux/init.h>				/* module_init, module_exit */
#include <linux/fs.h>				/* struct file_operations   */
#include <linux/kernel.h>			/* kernel                   */
#include <linux/fcntl.h>			/* O_RDONLY,O_WRONLY        */
#include <linux/cdev.h>				/* cdev_init add del        */
#include <asm/uaccess.h>		/* copy_from_user, copy_to_user */
#include "../inc/felica_rws.h"				/* private header file      */
#include <hsad/config_interface.h>

/*
 * static paramater & global
 */
static struct cdev g_cdev_frws;
static unsigned char g_rws_info = FELICA_OUT_RWS_AVAILABLE;

/*
 * Function prottype
 */
static int  __init felica_rws_init(void);
static void __exit felica_rws_exit(void);
static int  felica_rws_open(struct inode *, struct file *);
static int  felica_rws_close(struct inode *, struct file *);
static ssize_t felica_rws_read(struct file *, char *, size_t, loff_t *);

/*
 * sturut of reginsting External if
 */
static struct file_operations felica_fops = {
    .owner	= THIS_MODULE,
    .read	= felica_rws_read,
    .open	= felica_rws_open,
    .release= felica_rws_close
};

/*
 * extern
 */
extern dev_t dev_id_rws;


static int __init felica_rws_init( void )
{
	int ret;
    if(!is_felica_exist())
		return -ENODEV;
        
    g_rws_info = FELICA_OUT_RWS_AVAILABLE;

	cdev_init(&g_cdev_frws, &felica_fops);
	g_cdev_frws.owner = THIS_MODULE;
	ret = cdev_add(
			&g_cdev_frws,
			dev_id_rws,
			FELICA_DEV_NUM);
	if( ret < 0 ) {
		printk("cdev_init(RWS) failed.");
	}

	return ret;
}
module_init(felica_rws_init);


static void __exit felica_rws_exit( void )
{
	cdev_del(&g_cdev_frws);
	unregister_chrdev_region(dev_id_rws, FELICA_DEV_NUM);
	return;
}
module_exit(felica_rws_exit);


static int felica_rws_open(struct inode *pinode, struct file *pfile)
{
	int ret				= 0;
	unsigned char minor	= 0;

	if(( pfile == NULL ) || ( pinode == NULL )) {
		return -EINVAL;
	}

	minor = MINOR(pinode->i_rdev);

	if( minor == FELICA_RWS_MINOR_NO_RWS ) {
	}
	else {
		ret = -ENODEV;
	}

	return ret;
}


static int felica_rws_close(struct inode *pinode, struct file *pfile)
{
	int ret				= 0;
	unsigned char minor	= 0;

	if(( pfile == NULL ) || ( pinode == NULL )) {
		return -EINVAL;
	}

	minor = MINOR(pinode->i_rdev);

	if( minor == FELICA_RWS_MINOR_NO_RWS ) {
	}
	else {
		ret = -ENODEV;
	}

	return ret;
}


static ssize_t felica_rws_read(
							struct file *pfile,
							char		*buff,
							size_t		count,
							loff_t		*poff )
{
	ssize_t			ret = 0;
	unsigned char	minor = 0;
	unsigned char	local_buff[FELICA_OUT_RWS_SIZE];
	unsigned long	ret_cpy = 0;

	if(( pfile == NULL ) || ( buff == NULL )) {
		return (ssize_t)(-EINVAL);
	}

	minor = MINOR(pfile->f_dentry->d_inode->i_rdev);

	if( FELICA_RWS_MINOR_NO_RWS == minor ) {
		if( count != FELICA_OUT_RWS_SIZE ) {
			return (ssize_t)(-EINVAL);
		}
		local_buff[0] = g_rws_info;
		ret_cpy = copy_to_user(buff, local_buff, FELICA_OUT_RWS_SIZE);
		if( ret_cpy != 0 ) {
			return (ssize_t)(-EFAULT);
		}
		else {
			ret = FELICA_OUT_RWS_SIZE;
		}
	}
	else {
		ret = (ssize_t)(-ENODEV);
	}

	return ret;
}

MODULE_AUTHOR("NTT DATA MSE Co., Ltd.");
MODULE_DESCRIPTION("FeliCa RWS Driver");
MODULE_LICENSE("GPL");
