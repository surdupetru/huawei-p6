#include "lowmemorykiller_stub.h"


int syslowMem_major;
char syslowMemData[BUFFER_SIZE];
int isread = FALSE; /*lint -esym(528,isread)*/
int iswrite = FALSE;/*lint -esym(551,iswrite)*/
int lowMemoffset = 0;
int lowMemCount = 0; /*lint -esym(551,lowMemCount)*/

dev_t devid;
struct cdev *my_cdev;
struct class *my_class;

struct file_operations syslowMemOpe = {
        .owner = THIS_MODULE,
        .read = syslowMem_read
};

/*----------------------------------------------------------------------
    Function:         syslowMem_read
    Description:      read the information about the killed processed 
                      from socket buffer
    Parameter:        file- struct file
                      userbuf- store the input data 
                      length- the length of data to read
                      offset- the offset post to begin to read data
    return:           the size of the read data actually
*---------------------------------------------------------------------*/
ssize_t syslowMem_read(struct file * file, char __user *userbuf, size_t length, loff_t * offset)
{
    int read_count = 0;
    int bytes;
    isread = TRUE;
    
    /* synchronous control read and write */
    if (FALSE == iswrite)
    {
        if (NULL == userbuf || 0 == lowMemoffset)
        {
            isread = FALSE;
            return 0;
        }
        read_count = lowMemCount;
        // resolve following compile error: ignoring return value of 'copy_to_user'
        // it is stupid but works well.
        bytes = copy_to_user(userbuf, syslowMemData, lowMemoffset);
        bytes = bytes;
        
        /* reset the information about the readable buffer */
        lowMemoffset = 0;
        lowMemCount = 0;
    }
    isread = FALSE;
    return read_count;
}

/*----------------------------------------------------------------------
    Function:         sysLowKernel_write
    Description:      write the information about the killed processed 
                      to global variable
    Parameter:        selected- record the process infromation
    return:           none
*---------------------------------------------------------------------*/
void sysLowKernel_write(struct task_struct *selected)
{
    char *combegin = NULL;
    struct timeval now;
    int millisecond;
    
    do_gettimeofday (&now);
    
    /* microsecond changes to millisecond */
    millisecond = now.tv_usec / MILLISECOND_UNIT;

    /* synchronous control read and write */
    iswrite = TRUE;

    if (lowMemoffset >= BUFFER_SIZE - REMAIN_BUFFER_SIZE)
    {
        iswrite = FALSE;
        return;
    }
    
    /* only get the string after char '.' or '/' */
    combegin = strrchr(selected->comm, '.');
    if (NULL == combegin)
    {
        combegin = strrchr(selected->comm, '/');
    }
    else
    {
        combegin ++;
    }
    
    if (NULL != combegin)
    {
        /* prepare the data to send, including the time when the process is killed and the name of the killed process */
        sprintf(syslowMemData + lowMemoffset, "%ld.%d%d%d;%s;",
                    now.tv_sec,
                    GET_HUNDRED_BIT(millisecond),
                    GET_TEN_BIT(millisecond),
                    GET_LAST_BIT(millisecond),
                    combegin);
        
        lowMemoffset = strlen(syslowMemData);
        lowMemCount ++;
    }
    iswrite = FALSE;
}/*lint !e550*/

/*----------------------------------------------------------------------
    Function:         registerlowmem
    Description:      register the device for transfering data
    Parameter:        none
    return:           none
*---------------------------------------------------------------------*/
void registerlowmem(void)
{
    int err;
    do{
        /* register the device */
        if(alloc_chrdev_region(&devid, 0, 1, DEVICE_NAME))
        {
            break;
        }

        /* get the major device number */
        syslowMem_major = MAJOR(devid);
        if(0 == (my_cdev = cdev_alloc()))
        {
            break;
        }
        my_cdev->owner = THIS_MODULE;
        
        /* initialize struct cdev*/
        cdev_init(my_cdev, &syslowMemOpe);
        err = cdev_add(my_cdev, devid, 1);
        if (err)
        {
            cdev_del(my_cdev);
            unregister_chrdev_region(devid, 1);
            break;
        }

        /* create a device class */
        my_class = class_create(THIS_MODULE, DEVICE_CLASS_NAME);
        if (IS_ERR(my_class))
        {
            cdev_del(my_cdev);
            unregister_chrdev_region(devid, 1);
            break;
        }

        /* create a device */
        device_create(my_class, NULL, devid, NULL, DEVICE_NAME "%d", MINOR(devid));
    }while(0);
}

/*----------------------------------------------------------------------
    Function:         unregisterlowmem
    Description:      unregister the device 
    Parameter:        none
    return:           none
*---------------------------------------------------------------------*/
void unregisterlowmem(void)
{
    device_destroy(my_class, devid);
    class_destroy(my_class);
    cdev_del(my_cdev);
    unregister_chrdev_region(devid, 1);
}
