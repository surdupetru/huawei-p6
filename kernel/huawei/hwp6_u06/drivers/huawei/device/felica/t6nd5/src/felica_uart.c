/*
 * Copyright(C) 2012 NTT DATA MSE CORPORATION. All right reserved.
 *
 * @file felica_uart.c
 * @brief FeliCa Driver
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
#include <linux/err.h>					/* PTR_ERR/IS_ERR           */
#include <linux/string.h>				/* memset                   */
#include <linux/errno.h>				/* errno                    */
#include <linux/module.h>				/* Module parameters        */
#include <linux/init.h>					/* module_init, module_exit */
#include <linux/fs.h>					/* struct file_operations   */
#include <linux/kernel.h>				/* kernel                   */
#include <linux/param.h>				/* HZ const                 */
#include <linux/delay.h>				/* udelay, mdelay           */
#include <linux/fcntl.h>				/* O_RDONLY, O_WRONLY       */
#include <linux/cdev.h>					/* cdev_init add del        */
#include <asm/uaccess.h>			/* copy_from_user, copy_to_user */
#include <asm/ioctls.h>					/* FIONREAD                 */
#include <linux/termios.h>				/* termios                  */
#include "../inc/felica_uart.h"				/* private header file      */
#include <hsad/config_interface.h>

/*
 * Function prottype
 */
 static int     __init felica_uart_init(void);
 static void    __exit felica_uart_exit(void);
 static int     felica_uart_open(struct inode *, struct file *);
 static int     felica_uart_close(struct inode *, struct file *);
 static ssize_t felica_uart_write(struct file *, const char *, size_t, loff_t *);
 static ssize_t felica_uart_read(struct file *, char *, size_t, loff_t *);
 static long    felica_uart_ioctl( struct file * , unsigned int, unsigned long);
 static int     felica_uart_fsync(struct file*, int);
 static int  felica_uart_tty_open(void);
 static void felica_uart_tty_close(void);
 static int  felica_uart_tty_write(const char *, size_t);
 static int  felica_uart_tty_read(char *, size_t);
 static int  felica_uart_tty_fsync(void);
 static int  felica_uart_tty_available(int *);

/*
 * sturut of reginsting External if
 */
 static struct file_operations felica_fops = {
	.owner			= THIS_MODULE,
	.read			= felica_uart_read,
	.write			= felica_uart_write,
	.unlocked_ioctl = felica_uart_ioctl,
	.fsync			= felica_uart_fsync,
	.open			= felica_uart_open,
	.release		= felica_uart_close
 };

/*
 * static paramater & global
 */
 static struct cdev g_cdev_fuart;
 static spinlock_t timer_lock;
 static struct file *uart_tty_filp;
 static int uart_tty_open_count = 0;
 static unsigned char uart_tty_readbuff[UART_TTY_READBUFF_SIZE];
 static unsigned char uart_tty_writebuff[UART_TTY_WRITEBUFF_SIZE];

/*
 * extern
 */
 extern dev_t dev_id_uart;


static int felica_uart_tty_open( void )
{
	mm_segment_t	oldfs;
	struct file		*filp;
	struct inode	*inode = NULL;
	struct termios	options;
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	filp = filp_open(TTY_FILE_PATH, O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

	if (IS_ERR(filp)) {
		set_fs(oldfs);
		return -1;
	}
	inode = filp->f_path.dentry->d_inode;
	if (!filp->f_op || !inode) {
		filp_close(filp, NULL);
		set_fs(oldfs);
		return -1;
	}
	if (filp->f_op->unlocked_ioctl(filp, (unsigned int)TCFLSH, (unsigned long)TCIOFLUSH) < 0) {
		filp_close(filp, NULL);
		set_fs(oldfs);
		return -1;
	}
	if (filp->f_op->unlocked_ioctl(filp, (unsigned int)TCGETS, (unsigned long)&options) < 0) {
		filp_close(filp, NULL);
		set_fs(oldfs);
		return -1;
	}
	options.c_cc[VTIME] = 0;
	options.c_cc[VMIN]  = 0;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= (CS8 | CLOCAL | CREAD | CRTSCTS);
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	options.c_cflag = (options.c_cflag & ~CBAUD) | (B460800 & CBAUD);
	if (filp->f_op->unlocked_ioctl(filp, (unsigned int)TCSETS, (unsigned long)&options) < 0) {
		filp_close(filp, NULL);
		set_fs(oldfs);
		return -1;
	}
	set_fs(oldfs);
	uart_tty_filp  = filp;
	return 0;
}


static void felica_uart_tty_close( void )
{
	filp_close(uart_tty_filp, NULL);
	uart_tty_filp = NULL;

}


static int felica_uart_tty_write(const char *buff, size_t count)
{
	struct file		*filp;
	int				ret = 0;
	mm_segment_t	oldfs;


	/* parameter check */
	if (NULL == buff) {
		return -1;
	}
	
	if (count == 0) {
		return 0;
	}

	filp = uart_tty_filp;
	filp->f_pos = 0;

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	ret = filp->f_op->write(filp, buff, count, &filp->f_pos);
	set_fs(oldfs);
	if (ret < count) {
		return -1;
	}

	return ret;
}


static int felica_uart_tty_read(char *buff, size_t count)
{
	struct file		*filp;
	int				ret = 0;
	mm_segment_t	oldfs;


	/* parameter check */
	if (NULL == buff) {
		return -1;
	}
	if (count == 0){
		return 0;
	}

	filp = uart_tty_filp;
	filp->f_pos = 0;

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	ret = filp->f_op->read(filp, buff, count, &filp->f_pos);
	set_fs(oldfs);

	if (ret < 0) {
		return ret;
	}

	return ret;
}


static int felica_uart_tty_fsync( void )
{
	struct file		*filp;
	int				ret = 0;
	mm_segment_t	oldfs;
	int				size;
	int				i = 0;


	filp = uart_tty_filp;
	filp->f_pos = 0;

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	/* output-buffer-size check */
	ret = filp->f_op->unlocked_ioctl(filp, (unsigned int)TIOCOUTQ, (unsigned long)&size);
	set_fs(oldfs);
	while( size != 0 ) { /* become zero until continue  */
		oldfs = get_fs();
		set_fs(KERNEL_DS);
		ret = filp->f_op->unlocked_ioctl(filp, (unsigned int)TIOCOUTQ, (unsigned long)&size);
		set_fs(oldfs);
		msleep(10);
		i++;
		if( i > 1000 ) { /* gaurd counter */
			break;
		}
	}

	return ret;
}


static int felica_uart_tty_available( int * value )
{
	struct file		*filp;
	int				ret = 0;
	mm_segment_t	oldfs;
	int				size;


	filp = uart_tty_filp;
	filp->f_pos = 0;

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	ret = filp->f_op->unlocked_ioctl(filp, (unsigned int)FIONREAD, (unsigned long)&size);
	set_fs(oldfs);

	if (ret < 0) {
		size = 0;
	}
	*value = size;

	return ret;
}


static int felica_uart_open(struct inode *pinode, struct file  *pfile)
{
	int ret				= 0;
	unsigned char minor	= 0;

	if(( pfile == NULL ) || ( pinode == NULL )) {
		return -EINVAL;
	}
	minor = MINOR(pinode->i_rdev);

	if( minor == FELICA_UART_MINOR_NO_UART ) {
		if ( uart_tty_open_count == 0 ) {
			if ( felica_uart_tty_open() < 0 ) {
				return -EACCES;
			}
		}
		uart_tty_open_count++;
	}
	else {
		ret = -ENODEV;
	}

	return ret;
}


static int felica_uart_close(struct inode *pinode, struct file *pfile)
{
	int ret				= 0;
	unsigned char minor	= 0;


	if(( pfile == NULL ) || ( pinode == NULL )) {
		return -EINVAL;
	}

	minor = MINOR(pinode->i_rdev);

	if( minor == FELICA_UART_MINOR_NO_UART ) {
		uart_tty_open_count--;
		if ( uart_tty_open_count == 0 ) {
			felica_uart_tty_close();
		}
	}
	else {
		ret = -ENODEV;
	}

	return ret;
}


static ssize_t felica_uart_write(
							struct file	*pfile
							,const char	*buff
							,size_t		count
							,loff_t		*poff )
{
	ssize_t			ret = 0;
	unsigned char	minor = 0;
	int				write_ret;


	if (( pfile == NULL ) || ( buff == NULL )) {
		return (ssize_t)(-EINVAL);
	}

	minor = MINOR(pfile->f_dentry->d_inode->i_rdev);

	if(FELICA_UART_MINOR_NO_UART == minor) {
		if ( count == 0 ) {
			return 0;
		}

		if (count > UART_TTY_WRITEBUFF_SIZE) {
			count = UART_TTY_WRITEBUFF_SIZE;
		}

		if ( (copy_from_user((void*)uart_tty_writebuff, (void*)buff, (unsigned int)count)) != 0 )
		{
			return (ssize_t)(-EFAULT);
		}

		write_ret = felica_uart_tty_write((const char *)uart_tty_writebuff, count);
		if( write_ret < 0 ) {
			return (ssize_t)(-EIO);
		}
		return write_ret;
	}
	else {
		ret = (ssize_t)(-ENODEV);
	}

	return ret;
}


static ssize_t felica_uart_read(
							struct file	*pfile,
							char		*buff,
							size_t		count,
							loff_t		*poff )
{
	ssize_t				ret = 0;
	unsigned char		minor = 0;

	if (( pfile == NULL ) || ( buff == NULL )) {
		return (ssize_t)(-EINVAL);
	}

	minor = MINOR(pfile->f_dentry->d_inode->i_rdev);

	if(FELICA_UART_MINOR_NO_UART == minor) {

		if (count > UART_TTY_READBUFF_SIZE) {
			count = UART_TTY_READBUFF_SIZE;
		}

		ret = felica_uart_tty_read(uart_tty_readbuff, count);
		if (ret < 0) {
			return (ssize_t)(-EIO);
		}
		else if(ret == 0) {
			msleep(10);
		}
		else {
		}

		if ( (copy_to_user((void *)buff, (void*)uart_tty_readbuff, ret)) != 0 )
		{
			return (ssize_t)(-EFAULT);
		}
	}
	else {
		ret = (ssize_t)(-ENODEV);
	}

	return ret;
}


static long felica_uart_ioctl(struct file *pfile, unsigned int cmd, unsigned long arg)
{
	int				ret = 0;
	int				ret_sub;
	unsigned char	minor = 0;


	if( NULL == pfile ) {
		return -EINVAL;
	}

	minor = MINOR(pfile->f_dentry->d_inode->i_rdev);

	if(FELICA_UART_MINOR_NO_UART == minor) {
			if( cmd == FIONREAD ) {
				ret = felica_uart_tty_available(&ret_sub);
				if (ret < 0) {
					ret = -EIO;
					return ret;
				}
				if (copy_to_user((void*)arg, &ret_sub, sizeof(int)) != 0) {
					return -EFAULT;
				}
			}
			else {
				ret = -ENOSYS;
			}
	}
	else {
			ret = -ENODEV;
	}

	return ret;
}


static int felica_uart_fsync(struct file *pfile, int datasync)
{
	int				ret = 0;
	unsigned char	minor;


	if( NULL == pfile ){
		return -EINVAL;
	}

	minor = MINOR(pfile->f_dentry->d_inode->i_rdev);

	if(FELICA_UART_MINOR_NO_UART == minor) {
			ret = felica_uart_tty_fsync();
			if(ret < 0) {
				ret = (ssize_t)(-EIO);
			}
	}
	else {
			ret = -ENODEV;
	}

	return ret;
}


static int __init felica_uart_init( void )
{
	int ret;
    if(!is_felica_exist())
		return -ENODEV;
	spin_lock_init(&timer_lock);

	cdev_init(&g_cdev_fuart, &felica_fops);
	g_cdev_fuart.owner = THIS_MODULE;
	ret = cdev_add(&g_cdev_fuart, dev_id_uart, FELICA_DEV_NUM);
	if (ret < 0) {
		printk("cdev_init(UART) failed.");
	}
	return ret;
}
module_init( felica_uart_init );


static void __exit felica_uart_exit( void )
{
	cdev_del(&g_cdev_fuart);
	unregister_chrdev_region(dev_id_uart, FELICA_DEV_NUM);
	return;
}
module_exit( felica_uart_exit );

MODULE_AUTHOR("NTT DATA MSE Co., Ltd.");
MODULE_DESCRIPTION("FeliCa UART Driver");
MODULE_LICENSE("GPL");
