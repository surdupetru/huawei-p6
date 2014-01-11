


#include <asm/atomic.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/pm_qos_params.h>
#include <linux/time.h>

#include "k3v2_freq_ctl.h"
/*You can open this define for debug or test, must not open it for release project.*/
/*#define DEBUG_WIFI_FREQ_LOCK*/

#ifdef  DEBUG_WIFI_FREQ_LOCK
#define lfprintf(format, arg...)   printk("Freq gov msg: "format, ##arg)
static unsigned long  pre_freq_cfg_jiffies = 0;
static unsigned long  pre_release_jiffies = 0;
#else
#define lfprintf(format, arg...) 
#endif

static unsigned long wifi_rxtx_tot;
static unsigned long  pre_jiffies = 0;
#define  SPEED_CALC_TRIG_VALUE   655360  /* 5Mbits data bytes, unit:bytes,  triger speed */
                                                                    /* calculation bytes,  = (5*1024*1024/8)*/

/*speed_freq_level_t struct show the relationship of throughput and CPU lock frequency. */  
typedef  struct  {
    unsigned int speed_level;         /*Kbits/s*/
    unsigned int min_cpu_freq;      /*KHz,Minimal need CPU Frequency for the speed level*/
    unsigned int min_ddr_freq;      /*KHz, Minimal need DDR Frequency for the speed level, */
                                                       /* 0 means not need DDR Frequency lock.*/
} speed_freq_level_t;

const speed_freq_level_t  speed_freq_level[3] =
{
    {8000,   600000, 0},            /*level 1 frequency >= 600000KHz, min_ddr_freq = 0,It means not need to set DDR.*/
    {16000, 800000, 0},            /*level 2 frequency >= 800000KMHz, min_ddr_freq = 0,It means not need to set DDR.*/
    {28000, 900000, 120000},    /*level 3 frequency >= 900000KMHz, min_ddr_freq >= 120000*/
};

#define  MAX_SPEED_FREQ_LEVEL            3
#define  START_DDR_FREQ_LOCK_LEVEL  3
#define  FREQ_LOCK_DISABLE                  0
#define  FREQ_LOCK_ENABLE                   1
#define  FREQ_LOCK_TIMEOUT_VAL         1000  /*unit: ms*/
/*lock release magic number, This magic flag is used to avoid doing the frequency lock request 
 * and release operation at the same time. Without this the request may being release just after 
 the request process .*/
#define  LR_DISABLE_MAGIC            0  /*lock release process not start*/
#define  LR_START_MAGIC               1  /*lock release process is start*/
#define  LR_SHOULD_DROP_MAGIC  2  /*lock release work should be drop.*/
typedef struct {
    unsigned char  lock_mod;                  /*FREQ_LOCK_DISABLE: frequency lock disabled mode, 
                                                                FREQ_LOCK_ENABLE: frequency lock enabled mode*/
    unsigned char  lock_level;                  /*range: 1 to MAX_SPEED_FREQ_LEVEL*/
    unsigned char  req_lock_level;           /*range: 1 to MAX_SPEED_FREQ_LEVEL*/
    unsigned char  reserver;                   /*not use now,only for 4byte align.*/
    unsigned int    release_work_state;  
    struct mutex   lock_freq_mtx;
    spinlock_t       speed_calc_lock;

    struct timer_list lock_freq_timer_list;
    struct workqueue_struct *freq_ctl_wq;
    struct work_struct release_lock_work;
    struct work_struct do_freq_lock_work;
    struct pm_qos_request_list cpu_qos_request;
    struct pm_qos_request_list ddr_qos_request;

} freq_lock_control_t;

freq_lock_control_t  * freq_lock_control_ptr = NULL;

/*****************************************************************************
Description    : 1s frequency control release timer timeout, this function will be triggered. It will
                       release the minimal CPU and DDR frequency lock.	This function run in the frequency 
                       control work quenue.
Prototype      : void k3v2_release_freq_lock_work(struct work_struct *work)
Input  Param   :
Output  Param  :
Return  Value  :
******************************************************************************/
static void k3v2_release_freq_lock_work(struct work_struct *work)
{
    freq_lock_control_t *fl_control_ptr =
        container_of(work, freq_lock_control_t, release_lock_work);
    lfprintf("k3v2_release_freq_lock_work call enter.\n");
#ifdef DEBUG_WIFI_FREQ_LOCK
    pre_release_jiffies = jiffies;
#endif
    if(NULL == fl_control_ptr){
        printk("k3v2_release_freq_lock_work  NULL point error!\n");     
        return;
    }

    mutex_lock(&fl_control_ptr->lock_freq_mtx);
    if(LR_SHOULD_DROP_MAGIC == fl_control_ptr->release_work_state ){
        lfprintf("k3v2_release_freq_lock_work work has been drop, not do lock release, return here.\n");
        fl_control_ptr->release_work_state = LR_DISABLE_MAGIC;
        return;
    }
    
    if(FREQ_LOCK_ENABLE == fl_control_ptr->lock_mod){
#ifdef CONFIG_CPU_FREQ_GOV_K3HOTPLUG        
        /*lock freq timer timeout, do frequency lock release process here.*/
        pm_qos_remove_request(&fl_control_ptr->cpu_qos_request);
        if(fl_control_ptr->lock_level >= START_DDR_FREQ_LOCK_LEVEL){
            pm_qos_remove_request(&fl_control_ptr->ddr_qos_request);
        }
#endif
        fl_control_ptr->release_work_state = LR_DISABLE_MAGIC;
        fl_control_ptr->lock_mod = FREQ_LOCK_DISABLE;
    }
    mutex_unlock(&fl_control_ptr->lock_freq_mtx);
    
}

/*****************************************************************************
Description    : 
Prototype      : void k3v2_release_freq_lock_timer(unsigned long data)
Input  Param   :
Output  Param  :
Return  Value  :
******************************************************************************/
static void k3v2_release_freq_lock_timer(unsigned long data)
{
    freq_lock_control_t  * fl_control_ptr =
        (freq_lock_control_t  *) data;

    if(NULL == fl_control_ptr){
        printk("k3v2_release_freq_lock_timer  NULL point error!\n");
        return;
    }
    /*!! Can not and not need use mutex_lock here. this function is a softirq,
	* not allow schedule and sleep that are trigger by mutex_lock in this function.*/
    queue_work(fl_control_ptr->freq_ctl_wq, &fl_control_ptr->release_lock_work);
    fl_control_ptr->release_work_state = LR_START_MAGIC;
}

/*****************************************************************************
Description    : Set the frequency control as the req_lock_level indicate and start or refresh 
                      the 1s frequency control release timer. This function run in the frequency control
                      work quenue.
Prototype      : void  k3v2_do_freq_lock_work(struct work_struct *work)
Input  Param   :
Output  Param  :
Return  Value  :
******************************************************************************/
static void  k3v2_do_freq_lock_work(struct work_struct *work){
    unsigned char req_lock_level = 0;
    freq_lock_control_t *fl_control_ptr =
        container_of(work, freq_lock_control_t, do_freq_lock_work);
#ifdef DEBUG_WIFI_FREQ_LOCK
    unsigned long temp_jiffies;
    unsigned long  freq_cfg_duration;
#endif
    
    if(NULL == fl_control_ptr){
        printk("k3v2_do_freq_lock_work  NULL point error!\n");
        return;
    }
    
    mutex_lock(&fl_control_ptr->lock_freq_mtx);
    req_lock_level = fl_control_ptr->req_lock_level;
    if((req_lock_level > MAX_SPEED_FREQ_LEVEL) || 
        (0 == req_lock_level)){
        printk("k3v2_do_freq_lock_work  invalid lock_level error!\n");
        return;
    }

    if(LR_START_MAGIC == fl_control_ptr->release_work_state){
        fl_control_ptr->release_work_state = LR_SHOULD_DROP_MAGIC;
        /*disable the pending release lock work in queue here.*/
        lfprintf("WARM: k3v2_do_freq_lock_work set magic for delete pending release lock work.\n");
    }
    
    if(FREQ_LOCK_ENABLE == fl_control_ptr->lock_mod){       
        if (req_lock_level != fl_control_ptr->lock_level ){
#ifdef CONFIG_CPU_FREQ_GOV_K3HOTPLUG
            /* Frequency lock level has been changed. Do new level's frequency lock here.*/
            pm_qos_update_request(&fl_control_ptr->cpu_qos_request, 
                                        speed_freq_level[req_lock_level-1].min_cpu_freq);

            if(fl_control_ptr->lock_level < START_DDR_FREQ_LOCK_LEVEL){
                /*Old DDR frequency lock is not open*/
                if(req_lock_level >= START_DDR_FREQ_LOCK_LEVEL){
                    /* need open DDR frequency lock now.*/
                    pm_qos_add_request(&fl_control_ptr->ddr_qos_request, PM_QOS_DDR_MIN_PROFILE, 
                        speed_freq_level[req_lock_level-1].min_ddr_freq);
                }
                /*If old and new lock level all smaller than START_DDR_FREQ_LOCK_LEVEL, 
                *  not need request the DDR frequency lock still.*/
            }else{
                /*Old DDR frequency lock is open*/
                if(req_lock_level >= START_DDR_FREQ_LOCK_LEVEL){
                    /*New DDR frequency lock need open also,just update it.*/
                    pm_qos_update_request(&fl_control_ptr->ddr_qos_request, 
                            speed_freq_level[req_lock_level-1].min_ddr_freq);
                }else{
                    /*Old DDR frequency lock is open, new lock level not need it to be open, 
                    *  release the DDR frequency lock .*/
                    pm_qos_remove_request(&fl_control_ptr->ddr_qos_request);
                }
            }
            fl_control_ptr->lock_level = req_lock_level;
#endif  /*end of  #ifdef CONFIG_CPU_FREQ_GOV_K3HOTPLUG*/

#ifdef DEBUG_WIFI_FREQ_LOCK
            temp_jiffies = jiffies;
            freq_cfg_duration = jiffies_to_msecs(temp_jiffies - pre_freq_cfg_jiffies);
            pre_freq_cfg_jiffies = temp_jiffies;
            lfprintf("k3v2_do_freq_lock_work enter refresh lock level, with lock_level=%d, interval=%lu.%lu s\n", 
                req_lock_level, freq_cfg_duration/1000, freq_cfg_duration%1000);
#endif
            
        }       
        
    }else{  
#ifdef DEBUG_WIFI_FREQ_LOCK
        temp_jiffies = jiffies;
        freq_cfg_duration = jiffies_to_msecs(temp_jiffies - pre_release_jiffies);
        lfprintf("k3v2_do_freq_lock_work new lock request from last release time: %lu.%lu s \n", 
             freq_cfg_duration/1000, freq_cfg_duration%1000);
#endif  
        /*freq lock is disabled, enable it now*/
        fl_control_ptr->lock_mod = FREQ_LOCK_ENABLE;
        fl_control_ptr->lock_level = req_lock_level;
#ifdef CONFIG_CPU_FREQ_GOV_K3HOTPLUG        
        pm_qos_add_request(&fl_control_ptr->cpu_qos_request, PM_QOS_CPU_MIN_PROFILE, 
            speed_freq_level[req_lock_level-1].min_cpu_freq);
        if(req_lock_level >= START_DDR_FREQ_LOCK_LEVEL){
            pm_qos_add_request(&fl_control_ptr->ddr_qos_request, PM_QOS_DDR_MIN_PROFILE, 
                speed_freq_level[req_lock_level-1].min_ddr_freq);
        }
#endif 
    }
    
    /*start or reset release timer */
    mod_timer(&freq_lock_control_ptr->lock_freq_timer_list,
                  jiffies +
                  msecs_to_jiffies(FREQ_LOCK_TIMEOUT_VAL)); 
    mutex_unlock(&fl_control_ptr->lock_freq_mtx);
}

/*****************************************************************************
Description    : Do  memory allocation, workqueue set up etc while the WiFi driver opening.
Prototype      : int  k3v2_wifi_freq_ctl_init(void)
Input  Param   :
Output  Param  :
Return  Value  :
******************************************************************************/
int  k3v2_wifi_freq_ctl_init(void){
#ifdef CONFIG_CPU_FREQ_GOV_K3HOTPLUG        
    lfprintf("k3v2_wifi_freq_ctl_init  enter,ulong:%d, uint:%d\n", sizeof(unsigned long), sizeof(unsigned int));
    /*start cumulate rx tx bytes, record the first start time*/
    pre_jiffies = jiffies;
#ifdef DEBUG_WIFI_FREQ_LOCK
    pre_release_jiffies = jiffies;
    pre_freq_cfg_jiffies = jiffies;
#endif
    wifi_rxtx_tot = 0;
    
    /*init frequency lock control module.*/
    freq_lock_control_ptr = kzalloc(sizeof(freq_lock_control_t), GFP_KERNEL);
    if (!freq_lock_control_ptr){
        printk("k3v2_wifi_freq_ctl_init  kzalloc  error!\n");
        goto out;
    }
    
    mutex_init(&freq_lock_control_ptr->lock_freq_mtx);
    
    mutex_lock(&freq_lock_control_ptr->lock_freq_mtx);
    spin_lock_init(&freq_lock_control_ptr->speed_calc_lock);
    INIT_WORK(&freq_lock_control_ptr->release_lock_work, k3v2_release_freq_lock_work);
    INIT_WORK(&freq_lock_control_ptr->do_freq_lock_work, k3v2_do_freq_lock_work);
    setup_timer(&freq_lock_control_ptr->lock_freq_timer_list, k3v2_release_freq_lock_timer,
                   (unsigned long) freq_lock_control_ptr);  
    freq_lock_control_ptr->freq_ctl_wq = create_singlethread_workqueue("wifi_lock_freq");
    if (!freq_lock_control_ptr->freq_ctl_wq){
        printk("k3v2_wifi_freq_ctl_init  create_singlethread_workqueue  error!\n");
        mutex_unlock(&freq_lock_control_ptr->lock_freq_mtx);
        goto cleanup;
    }
    freq_lock_control_ptr->release_work_state = LR_DISABLE_MAGIC;
    
    mutex_unlock(&freq_lock_control_ptr->lock_freq_mtx);
    printk("wl monitor init ok!\n");
    return 0;
//err_wq:
//    destroy_workqueue(freq_lock_control_ptr->freq_ctl_wq);
cleanup:
    kfree(freq_lock_control_ptr);
    freq_lock_control_ptr = NULL;
out:
#endif /*end of  #ifdef CONFIG_CPU_FREQ_GOV_K3HOTPLUG*/
    return -1;
}

/*****************************************************************************
Description    : Do  workqueue release , mamery freee etc while the WiFi driver closing.
Prototype      : int  k3v2_wifi_freq_ctl_destroy(void)
Input  Param   :
Output  Param  :
Return  Value  :
******************************************************************************/
int  k3v2_wifi_freq_ctl_destroy(void)
{
#ifdef CONFIG_CPU_FREQ_GOV_K3HOTPLUG        
    lfprintf("k3v2_wifi_freq_ctl_destroy  enter\n");
    mutex_lock(&freq_lock_control_ptr->lock_freq_mtx);

    del_timer_sync(&freq_lock_control_ptr->lock_freq_timer_list);
    /*If the Wi-Fi is closing, but the frequency lock release timer is not timeout, 
    * must release the lock directly here!*/
    if(FREQ_LOCK_ENABLE == freq_lock_control_ptr->lock_mod){
        pm_qos_remove_request(&freq_lock_control_ptr->cpu_qos_request);
        if(freq_lock_control_ptr->lock_level >= START_DDR_FREQ_LOCK_LEVEL){
            pm_qos_remove_request(&freq_lock_control_ptr->ddr_qos_request);
        }
        freq_lock_control_ptr->release_work_state = LR_DISABLE_MAGIC;
        freq_lock_control_ptr->lock_mod = FREQ_LOCK_DISABLE;
        lfprintf("k3v2_wifi_freq_ctl_destroy freq lock release here!\n");
    }
#ifdef DEBUG_WIFI_FREQ_LOCK
    else{
        lfprintf("k3v2_wifi_freq_ctl_destroy freq lock has already been released!\n");
    }
#endif
    
    cancel_work_sync(&freq_lock_control_ptr->release_lock_work);
    cancel_work_sync(&freq_lock_control_ptr->do_freq_lock_work);
    
    destroy_workqueue(freq_lock_control_ptr->freq_ctl_wq);

    mutex_unlock(&freq_lock_control_ptr->lock_freq_mtx);
    
    mutex_destroy(&freq_lock_control_ptr->lock_freq_mtx);
    kfree(freq_lock_control_ptr);
    freq_lock_control_ptr = NULL;
	printk("wl monitor exit ok!\n");
#endif   /*end of  #ifdef CONFIG_CPU_FREQ_GOV_K3HOTPLUG*/
    return 0;
}

/*****************************************************************************
Description    : Calculate the WiFi throughput base on the duration time length and total transimit and receive
                      data length in this time duration. And check if the throughput reach any of the three frequency 
                      control level. 
Prototype      : int k3v2_speed_calc_process (unsigned int bytes)
Input  Param   :    total transimit and receive data length in the speed calculation time duration.
Output  Param  :
Return  Value  :
******************************************************************************/
static int k3v2_speed_calc_process (unsigned int bytes){
    unsigned long duration;
    unsigned long temp_jiffies;
    unsigned long speed;
    unsigned int level_idx;

    temp_jiffies = jiffies;
    duration = jiffies_to_msecs(temp_jiffies - pre_jiffies);
    /*reset next triger start time record.*/
    pre_jiffies = temp_jiffies;
	
    /*duration maybe 0, need this check*/
    if(0 == duration){
        return 0;
    }
	
    speed = bytes*8 / duration; /*unit: kbits/s */

    /*lfprintf("Speed calc triger in total bytes: %d, speed calculation duration: %lu.%lu s, speed:%lu kbits/s\n", 
    *   bytes, duration/1000, duration%1000, speed);
    */

    /*In most condition , the speed will lower than lock level 1, so we should try level 1 first.*/
    for(level_idx = 0; level_idx < MAX_SPEED_FREQ_LEVEL; level_idx++){
        if(speed > speed_freq_level[level_idx].speed_level){
            continue;/*try next lock level*/
        }else{
            break;
        }
    }

    if(level_idx > 0){
        if(NULL == freq_lock_control_ptr){
            printk("k3v2_speed_calc_process  NULL point error!\n");
            return -1;
        }
        
        /*the new frequency lock level is : level_idx*/
        freq_lock_control_ptr->req_lock_level = level_idx;      
        /*If call k3v2_do_freq_lock_work(i) directly,  fatal error will happen.*/
        queue_work(freq_lock_control_ptr->freq_ctl_wq, &freq_lock_control_ptr->do_freq_lock_work);
    }

    return 0;
}

/*****************************************************************************
Description    : This function called every rx and tx packet, the effection is very important!!!
Prototype      : int k3v2_wifi_txrx_count(int size)
Input  Param   :
Output  Param  :
Return  Value  :
******************************************************************************/
int k3v2_wifi_txrx_count(int size) {
#ifdef CONFIG_CPU_FREQ_GOV_K3HOTPLUG        
    unsigned long  tot_bytes = 0;
    if(NULL == freq_lock_control_ptr){
        return -1;
    }
    /*Should use spin_lock_bh to block softirq. It may cause deadlock by using 
	 *spin_lock here. Because softirq can call this function too*/
    spin_lock_bh(&freq_lock_control_ptr->speed_calc_lock);
    wifi_rxtx_tot += size;
    if(wifi_rxtx_tot > SPEED_CALC_TRIG_VALUE){
            tot_bytes = wifi_rxtx_tot;
            wifi_rxtx_tot = 0;
    }
    spin_unlock_bh(&freq_lock_control_ptr->speed_calc_lock);
    
    if(0 != tot_bytes){
        k3v2_speed_calc_process(tot_bytes);
    }
#endif
    return 0;
}

