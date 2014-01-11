#ifndef LOW_MEMORY_KILLER_STUB
#define LOW_MEMORY_KILLER_STUB

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/oom.h>
#include <linux/sched.h>
#include <linux/notifier.h>

#include <linux/fs.h>
#include <linux/time.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define BUFFER_SIZE 1024*1024

/* the threshold of the free buffer size to allow to write data to buffer*/
#define REMAIN_BUFFER_SIZE 50

/* the device name for data transfer */
#define DEVICE_NAME "lowmemkiller"

/**/
#define DEVICE_CLASS_NAME "hw_sst_lowmemkiller_class"
#define TRUE 1
#define FALSE 0
#define MILLISECOND_UNIT 1000

/* get hundred bit of a digit */
#define GET_HUNDRED_BIT(data) ((data) % 1000 / 100)

/* get ten bit of a digit */
#define GET_TEN_BIT(data)     ((data) % 100 / 10) 

/* get the last bit of a digit */
#define GET_LAST_BIT(data)    ((data) % 10)  

void registerlowmem(void);
void unregisterlowmem(void);
void sysLowKernel_write(struct task_struct *selected);
ssize_t syslowMem_read(struct file * file, char __user *userbuf, size_t length, loff_t * offset);

#endif // #ifndef LOW_MEMORY_KILLER_STUB
