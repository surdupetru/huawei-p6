/*
* HI6421 ALSA Soc codec driver
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/notifier.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/clk.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/tlv.h>
#include <asm/io.h>
#include <mach/gpio.h>

#include <linux/switch.h>
#include <linux/wakelock.h>

#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>

#include <linux/hkadc/hiadc_hal.h>
#include <linux/hkadc/hiadc_linux.h>

/*  Reason: For hook  */
#include <sound/jack.h>
/*KEY_MEDIA defined in linux/input.h*/
#include <linux/input/matrix_keypad.h>
#include <sound/hi6421_common.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/irqnr.h>
#include <mach/boardid.h>
#include <hsad/config_interface.h>
#include <linux/uart_output.h>
#include "hi6421.h"
#include <hsad/config_interface.h>

#define DUMP_REG ( 0 )

#define PMUID_V200 0x20
#define PMUID_V220 0x22

#define AUDIO_NORMAL_MODE   ( 100 * 1000 )
#define AUDIO_ECO_MODE      (  5  * 1000 )
#define HI6421_HS_BTN_MAX_WAIT_TIME_US  (90000)
#define HI6421_HS_BTN_MIN_WAIT_TIME_US  (85000)
#define HI6421_HS_PLUG_OUT_MAX_WAIT_TIME_US  (201000)
#define HI6421_HS_PLUG_OUT_MIN_WAIT_TIME_US  (200000)
#define HI6421_HKADC_MIN_WAIT_TIME_US   (30000)
#define HI6421_HKADC_MAX_WAIT_TIME_US   (35000)

#define HI6421_PB_MIN_CHANNELS ( 2 )
#define HI6421_PB_MAX_CHANNELS ( 2 )
#define HI6421_CP_MIN_CHANNELS ( 2 )
#define HI6421_CP_MAX_CHANNELS ( 2 )
#define HI6421_FORMATS     ( SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE)
#define HI6421_RATES       ( SNDRV_PCM_RATE_8000  | SNDRV_PCM_RATE_16000 |\
                             SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000 )

#define HI6421_PORT_MM    ( 0 )
#define HI6421_PORT_MODEM ( 1 )
#define HI6421_PORT_FM    ( 2 )
#define HI6421_PORT_BT    ( 3 )
#define HS_UART_STATUS  GPIO_7_5

#define MODEM_SPRD8803G   "sprd8803g"

#ifdef CONFIG_MFD_HI6421_IRQ
/* CODEC STATE */
/* Reason: Add for headset detect of hook and plug with mic or no mic */
enum headset_voltage { /* mV */
    HI6421_PB_VOLTAGE_MIN    = 70,
    HI6421_PB_DC_VOLTAGE_MIN = 70,
    HI6421_PB_DC_VOLTAGE_MAX = 850,
    HI6421_PB_VOLTAGE_MAX    = 1800,/*900*/
};

enum btn_voltage { /* mV */
    HI6421_BTN_FORWARD_VOLTAGE_MIN = 1890,
    HI6421_BTN_FORWARD_VOLTAGE_MAX = 1950,
    HI6421_BTN_PLAY_VOLTAGE_MIN    = 2080,
    HI6421_BTN_PLAY_VOLTAGE_MAX    = 2120,
    HI6421_BTN_BACK_VOLTAGE_MIN    = 1630,
    HI6421_BTN_BACK_VOLTAGE_MAX    = 1690,
    HI6421_SINGLE_BTN_VOLTAGE_MIN  = 785,
    HI6421_SINGLE_BTN_VOLTAGE_MAX  = 2700
};

enum voltage_factor { /* mV */
    HI6421_NORMAL_VOLTAGE_FACTOR = 1000,
    HI6421_ECO_VOLTAGE_FACTOR    = 1100, /* 2.75/2.5*1000 */
};

enum hi6421_jack_states {
    HI6421_JACK_BIT_NONE           = 0, /* unpluged */
    HI6421_JACK_BIT_HEADSET        = 1, /* pluged with mic */
    HI6421_JACK_BIT_HEADSET_NO_MIC = 2, /* pluged without mic */
};

struct hi6421_jack_data {
    struct snd_soc_jack *jack;
    int report;
    struct switch_dev sdev;
};
#endif

enum delay_time { /* ms */
    HI6421_HS_PLUG_DETECT_TIME    = 1000,
    HI6421_HS_PLUG_OUT_MAX_WAIT_TIME = 201,
    HI6421_HS_POWERDOWN_WAITTIME  = 200,
    HI6421_HS_WAKE_LOCK_TIMEOUT   = 50,
    HI6421_HKADC_SLEEP_TIME       = 30,
    HI6421_PLL_STABLE_WAIT_TIME   = 20,
    HI6421_HS_BTN_JUDGEMENT_TIME  = 50,
    HI6421_PLL_DETECT_TIME        = 40,
};

/* codec private data */
struct hi6421_data {
#ifdef CONFIG_MFD_HI6421_IRQ
    struct hi6421_jack_data hs_jack;
    struct wake_lock wake_lock;
    struct wake_lock plugin_wake_lock;
#ifdef CONFIG_K3_ADC
    struct workqueue_struct *headset_btn_down_delay_wq;
    struct delayed_work  headset_btn_down_delay_work;
    struct workqueue_struct *headset_btn_up_delay_wq;
    struct delayed_work  headset_btn_up_delay_work;
    int check_voltage;
#endif
    struct workqueue_struct *headset_plug_in_delay_wq;
    struct delayed_work  headset_plug_in_delay_work;
    struct workqueue_struct *headset_plug_out_delay_wq;
    struct delayed_work  headset_plug_out_delay_work;
    struct workqueue_struct *headset_judgement_delay_wq;
    struct delayed_work  headset_judgement_delay_work;;
#endif
    spinlock_t lock;
    struct mutex io_mutex;
    struct snd_soc_codec *codec;
    bool lower_power;
    unsigned int pmuid;
    struct workqueue_struct *pll_delay_wq;
    struct delayed_work  pll_delay_work;
};

/* feature */
static bool hi6421_is_cs = false;
static int hi6421_hsd_inv = 0;
static int hi6421_hs_keys = 1;

/* HI6421 MUTEX */
static struct mutex hi6421_power_lock;
static bool hi6421_pcm_mm_pb = false;
static bool hi6421_pcm_mm_cp = false;
static bool hi6421_pcm_modem = false;
static bool hi6421_pcm_fm = false;
/* reference count */
static int hi6421_audioldo_enabled = 0;
static int hi6421_ibias_enabled = 0;
static int hi6421_pll_enabled = 0;

/* HI6421 PMUSPI CLOCK */
static struct clk *g_clk = NULL;

/* HI6421 AUD_LDO */
static struct regulator *g_regu = NULL;

/* HI6421 PMUSPI REGISTER BASE ADDR */
static unsigned int g_hi6421_reg_base_addr = -1;

/* HI6421 CODEC */
static struct snd_soc_codec *g_codec = NULL;

/* Headset jack */
static struct snd_soc_jack g_hs_jack;

#ifdef CONFIG_MFD_HI6421_IRQ
/* Headset jack */
static volatile enum hi6421_jack_states hs_status = HI6421_JACK_BIT_NONE;

#endif

static DEFINE_SPINLOCK(pll_lock);

/* codec register array & default register value */
unsigned int codec_registers[] = {
    0x40,//0x90
    0x06,//0x91
    0x00,//0x92  RO
    0x44,//0x93
    0x24,//0x94
    0x9A,//0x95
    0xAE,//0x96
    0x51,//0x97
    0x99,//0x98     //AGC ENABLE BIT[7]
    0x00,//0x99
    0x24,//0x9A
    0x9A,//0x9B
    0xAE,//0x9C
    0x51,//0x9D
    0x99,//0x9E     //AGC ENABLE BIT[7]
    0x00,//0x9F
    0x50,//0xA0
    0x40,//0xA1
    0x00,//0xA2
    0x00,//0xA3
    0x00,//0xA4
    0x00,//0xA5
    0x3F,//0xA6
    0x1F,//0xA7
    0x1F,//0xA8
    0x1F,//0xA9
    0x1F,//0xAA
    0x1F,//0xAB
    0x1F,//0xAC
    0x1F,//0xAD
    0x3F,//0xAE
    0x1F,//0xAF
    0x1F,//0xB0
    0x1F,//0xB1
    0x1F,//0xB2
    0x1F,//0xB3
    0x1F,//0xB4
    0x1F,//0xB5
    0x00,//0xB6
    0x00,//0xB7  digital clk en 1
    0x00,//0xB8  digital clk en 2
    0x00,//0xB9  digital clk en 3
    0x00,//0xBA  digital clk en 4
    0x60,//0xBB  AP config
    0x08,//0xBC  BT config
    0x08,//0xBD  MODEM config
    0x00,//0xBE  interface freq
    0x7F,//0xBF  bypass config
    0x37,//0xC0  mux config
    0x38,//0xC1  DMIC config & ADC SDM dither
    0x00,//0xC2  RO
    0x00,//0xC3  RO
    0x00,//0xC4  RO
    0x00,//0xC5  RO
    0x00,//0xC6  RO
    0x00,//0xC7  RO
    0x00,//0xC8  RO
    0x7F,//0xC9  codec pd 1
    0xFD,//0xCA  codec pd 2    during initialization, PLL pd vref[1:0] = 11b(5k); After
            /*initialization vrefsel[1:0] = 01b(100k) for precise measurement*/
            /*vrefsel[1:0] = 10b(500k) for lower power*/
    0xFF,//0xCB  codec pd 3
    0x7F,//0xCC  codec pd 4
    0x9E,//0xCD  linein l pga
    0x9E,//0xCE  linein r pga
    0x24,//0xCF  aux pga
    0x64,//0xD0  main pga
    0x1E,//0xD1  side pga
    0x00,//0xD2  mixer in l
    0x00,//0xD3  mixer in r
    0x12,//0xD4
    0x12,//0xD5
    0x15,//0xD6
    0x00,//0xD7  MONO mixer
    0x00,//0xD8  mixer out l
    0x00,//0xD9  mixer out r
    0x00,//0xDA  mixer lineout l
    0x28,//0xDB  mixer lineout l pga
    0x00,//0xDC  mixer lineout r
    0x28,//0xDD  mixer lineout r pga
    0x01,//0xDE  headset ctl     HP_CM : 11B : 1.2V
    0x2A,//0xDF  headset l pga
    0x2A,//0xE0  headset r pga
    0x38,//0xE1  earpiece pga
    0x61,//0xE2  classd pga     // normal mute
    0x00,//0xE3  classd ctl 1
    0x00,//0xE4  classd ctl 2
    0x3C,//0xE5  zero cross ctl 1
    0x3C,//0xE6  zero cross ctl 2
    0xB0,//0xE7  PLL ctl 1
    0x7D,//0xE8  PLL ctl 2
    0x1F,//0xE9  PLL ctl 3
    0x87,//0xEA  PLL ctl 4
    0x00,//0xEB  PLL ctl 5
    0x10,/* 0xEC  MIC BIAS ctl 1 [4]1b: eco*/
    0xDA,//0xED  MIC BIAS ctl 2   // sw_irq_th[7:4]=1101b  ///Main&Sub MIC 2.75V
    0x55,//0xEE
    0x55,//0xEF
    0x2a,//0xF0 [6:5]01b:5u,[4:3]00b:3u,[2:1]10b:7u,[0]0b:5u
    0x00,//0xF1
    0x06,//0xF2  HSD ctl [1]1b:high level trigger
};

static inline unsigned int hi6421_reg_read(struct snd_soc_codec *codec,
            unsigned int reg)
{
    int ret = 0;
    ret = clk_enable(g_clk);
    if (ret) {
        pr_err("%s : enable clk failed", __FUNCTION__);
        clk_put(g_clk);
        return ret;
    }

    ret = readl(g_hi6421_reg_base_addr + (reg << 2));
    clk_disable(g_clk);
    return ret;
}

static inline int hi6421_reg_write(struct snd_soc_codec *codec,
            unsigned int reg, unsigned int value)
{
    int ret = 0;
    ret = clk_enable(g_clk);
    if (ret) {
        pr_err("%s : enable clk failed", __FUNCTION__);
        clk_put(g_clk);
        return ret;
    }
    writel(value, g_hi6421_reg_base_addr + (reg << 2));
    clk_disable(g_clk);
    return 0;
}

/*
* return trun  : ap interface is set 48kHz
* return false : ap interface is set 96kHz
*/
static inline bool check_ap_interface_48khz(void)
{
    unsigned int value = hi6421_reg_read(g_codec, HI6421_REG_FREQ_CONFIG);
    return HI6421_FREQ_AP_48KHZ == (value & HI6421_FREQ_AP_MASK);
}

/*
* return trun  : modem interface is set 8kHz
* return false : modem interface is set 16kHz
*/
static inline bool check_modem_interface_8khz(void)
{
    unsigned int value = hi6421_reg_read(g_codec, HI6421_REG_FREQ_CONFIG);
    return HI6421_FREQ_MODEM_8KHZ == (value & HI6421_FREQ_MODEM_MASK);
}

static void hi6421_set_reg_bit(unsigned int reg, unsigned int shift)
{
    unsigned int val = 0;
    unsigned long flags = 0;
    struct hi6421_data *priv = snd_soc_codec_get_drvdata(g_codec);
    if (priv) {
        spin_lock_irqsave (&priv->lock, flags);
        val = hi6421_reg_read(g_codec, reg) | (1 << shift);
        hi6421_reg_write(g_codec, reg, val);
        spin_unlock_irqrestore (&priv->lock, flags);
    }
}

static void hi6421_clr_reg_bit(unsigned int reg, unsigned int shift)
{
    unsigned int val = 0;
    unsigned long flags = 0;
    struct hi6421_data *priv = snd_soc_codec_get_drvdata(g_codec);
    if (priv) {
        spin_lock_irqsave (&priv->lock, flags);
        val = hi6421_reg_read(g_codec, reg) & ~(1 << shift);
        hi6421_reg_write(g_codec, reg, val);
        spin_unlock_irqrestore (&priv->lock, flags);
    }
}

#if DUMP_REG
static void dump_reg(struct snd_soc_codec *codec)
{
    unsigned int i = 0;
    pr_info("===========%s : start ===========",__FUNCTION__);
    for (i = HI6421_CODEC_REG_FIRST; i <= HI6421_CODEC_REG_LAST; i++)
        pr_info("%#04x : %#04x",i,hi6421_reg_read(codec, i));
    pr_info("===========%s :  end  ===========",__FUNCTION__);
}
#endif

/* VOLUME CONTROLS */

/*
* MAIN MIC GAIN volume control:
* from 0 to 26 dB in 2 dB steps
* MAX VALUE is 13
*/
static DECLARE_TLV_DB_SCALE(main_mic_tlv, 0, 200, 0);

/*
* SUB MIC GAIN volume control:
* from 0 to 26 dB in 2 dB steps
* MAX VALUE is 13
*/
static DECLARE_TLV_DB_SCALE(sub_mic_tlv, 0, 200, 0);

/*
* SIDE MIC GAIN volume control:
* from -30 to 16 dB in 2 dB steps
* MAX VALUE is 23
*/
static DECLARE_TLV_DB_SCALE(side_mic_tlv, -3000, 200, 0);

/*
* LINEIN GAIN volume control:
* from -30 to 16 dB in 2 dB steps
* MAX VALUE is 23
*/
static DECLARE_TLV_DB_SCALE(linein_tlv, -3000, 200, 0);

/*
* LINEOUT GAIN volume control:
* from -42 to 6 dB in 1.5 dB steps
* MAX VALUE is 32
*/
static DECLARE_TLV_DB_SCALE(lineout_tlv, -4200, 150, 0);

/*
* EARPIECE GAIN volume control:
* from -42 to 6 dB in 1.5 dB steps
* MAX VALUE is 32
*/
static DECLARE_TLV_DB_SCALE(earpiece_tlv, -4200, 150, 0);

/*
* CLASSD GAIN volume control:
* from -36 to 12 dB in 1.5 dB steps
* MAX VALUE is 32
*/
static DECLARE_TLV_DB_SCALE(classd_tlv, -3600, 150, 0);

/*
* HEADSET GAIN volume control:
* from -42 to 6 dB in 1.5 dB steps
* MAX VALUE is 32
*/
static DECLARE_TLV_DB_SCALE(headset_tlv, -4200, 150, 0);

/* SOUND KCONTROLS */

static const struct snd_kcontrol_new hi6421_v200_snd_controls[] = {
    /* VOLUME */
    SOC_SINGLE_TLV("Main PGA Volume",
            HI6421_REG_MAINPGA, HI6421_MAINPGA_GAIN_BIT,
            13, /* MAX VALUE(13) SEE IN MAIN MIC GAIN NOTE */
            0, main_mic_tlv),
    SOC_SINGLE_TLV("Sub PGA Volume",
            HI6421_REG_SUBPGA, HI6421_SUBPGA_GAIN_BIT,
            13, /* MAX VALUE(13) SEE IN SUB MIC GAIN NOTE */
            0, sub_mic_tlv),
    SOC_SINGLE_TLV("Side PGA Volume",
            HI6421_REG_SIDEPGA, HI6421_SIDEPGA_GAIN_BIT,
            23, /* MAX VALUE(23) SEE IN SIDE MIC GAIN NOTE */
            0, side_mic_tlv),
    SOC_SINGLE_TLV("Earpiece Volume",
            HI6421_REG_EARPIECEPGA, HI6421_EARPIECEPGA_GAIN_BIT,
            32, /* MAX VALUE(32) SEE IN EARPIECE GAIN NOTE */
            0, earpiece_tlv),
    SOC_SINGLE_TLV("Speaker Volume",
            HI6421_REG_CLASSDPGA, HI6421_CLASSDPGA_GAIN_BIT,
            32, /* MAX VALUE(32) SEE IN CLASSD GAIN NOTE */
            0, classd_tlv),

    SOC_SINGLE_TLV("LineinL Volume",
            HI6421_REG_LINEINLPGA, HI6421_LINEOUTPGA_GAIN_BIT,
            23, /* MAX VALUE(23) SEE IN LINEIN GAIN NOTE */
            0, linein_tlv),
    SOC_SINGLE_TLV("LineinR Volume",
            HI6421_REG_LINEINRPGA, HI6421_LINEOUTPGA_GAIN_BIT,
            23, /* MAX VALUE(23) SEE IN LINEIN GAIN NOTE */
            0, linein_tlv),
    SOC_SINGLE_TLV("LineoutL Volume",
            HI6421_REG_LINEOUTLPGA, HI6421_LINEOUTPGA_GAIN_BIT,
            32, /* MAX VALUE(32) SEE IN LINEOUT GAIN NOTE */
            0, lineout_tlv),
    SOC_SINGLE_TLV("LineoutR Volume",
            HI6421_REG_LINEOUTRPGA, HI6421_LINEOUTPGA_GAIN_BIT,
            32, /* MAX VALUE(32) SEE IN LINEOUT GAIN NOTE */
            0, lineout_tlv),
    SOC_SINGLE_TLV("HeadsetL Volume",
            HI6421_REG_HEADSETPGAL, HI6421_HEADSETPGA_GAIN_BIT,
            32, /* MAX VALUE(32) SEE IN HEADSET GAIN NOTE */
            0, headset_tlv),
    SOC_SINGLE_TLV("HeadsetR Volume",
            HI6421_REG_HEADSETPGAR, HI6421_HEADSETPGA_GAIN_BIT,
            32, /* MAX VALUE(32) SEE IN HEADSET GAIN NOTE */
            0, headset_tlv),

    SOC_SINGLE("Main PGA Boost",
               HI6421_REG_MAINPGA, HI6421_MAINPGA_BOOST_BIT, 1, 0),
    SOC_SINGLE("Main PGA Mute",
               HI6421_REG_MAINPGA, HI6421_MAINPGA_MUTE_BIT, 1, 0),
    SOC_SINGLE("Sub PGA Boost",
               HI6421_REG_SUBPGA, HI6421_SUBPGA_BOOST_BIT, 1, 0),
    SOC_SINGLE("Sub PGA Mute",
               HI6421_REG_SUBPGA, HI6421_SUBPGA_MUTE_BIT, 1, 0),
    SOC_SINGLE("Side PGA Mute",
               HI6421_REG_SIDEPGA, HI6421_SIDEPGA_MUTE_BIT, 1, 0),
    SOC_SINGLE("Earpiece Mute",
               HI6421_REG_EARPIECEPGA, HI6421_EARPIECEPGA_MUTE_BIT, 1, 0),
    SOC_SINGLE("Speaker Mute",
               HI6421_REG_CLASSDPGA, HI6421_CLASSDPGA_MUTE_BIT, 1, 0),

    SOC_SINGLE("LineinL Mute",
               HI6421_REG_LINEINLPGA, HI6421_LINEINPGA_MUTE_BIT, 1, 0),
    SOC_SINGLE("LineinR Mute",
               HI6421_REG_LINEINRPGA, HI6421_LINEINPGA_MUTE_BIT, 1, 0),
    SOC_SINGLE("LineoutL Mute",
               HI6421_REG_LINEOUTLPGA, HI6421_LINEOUTPGA_MUTE_BIT, 1, 0),
    SOC_SINGLE("LineoutR Mute",
               HI6421_REG_LINEOUTRPGA, HI6421_LINEOUTPGA_MUTE_BIT, 1, 0),
    SOC_SINGLE("HeadsetL Mute",
               HI6421_REG_HEADSETPGAL, HI6421_HEADSETPGA_MUTE_BIT, 1, 0),
    SOC_SINGLE("HeadsetR Mute",
               HI6421_REG_HEADSETPGAR, HI6421_HEADSETPGA_MUTE_BIT, 1, 0),

    /* INTERFACE */
    /* 0xBB - AP */
    SOC_SINGLE("INTERFACE AP HZ",
               HI6421_REG_AP_CONFIG, HI6421_INTERFACE_HZ_BIT, 1, 0),
    SOC_SINGLE("INTERFACE AP BITS",
               HI6421_REG_AP_CONFIG, HI6421_INTERFACE_DATABITS_BIT, 3, 0),
    SOC_SINGLE("INTERFACE AP DATATYPE",
               HI6421_REG_AP_CONFIG, HI6421_INTERFACE_DATATYPE_BIT, 1, 0),
    SOC_SINGLE("INTERFACE AP PORT",
               HI6421_REG_AP_CONFIG, HI6421_INTERFACE_PORT_BIT, 1, 0),
    SOC_SINGLE("INTERFACE AP EDGE",
               HI6421_REG_AP_CONFIG, HI6421_INTERFACE_EDGE_BIT, 1, 0),
    SOC_SINGLE("INTERFACE AP FSP",
               HI6421_REG_AP_CONFIG, HI6421_INTERFACE_FSP_BIT, 1, 0),
    /* 0xBC - BT */
    SOC_SINGLE("INTERFACE BT HZ",
               HI6421_REG_BT_CONFIG, HI6421_INTERFACE_HZ_BIT, 1, 0),
    SOC_SINGLE("INTERFACE BT BITS",
               HI6421_REG_BT_CONFIG, HI6421_INTERFACE_DATABITS_BIT, 3, 0),
    SOC_SINGLE("INTERFACE BT DATATYPE",
               HI6421_REG_BT_CONFIG, HI6421_INTERFACE_DATATYPE_BIT, 1, 0),
    SOC_SINGLE("INTERFACE BT PORT",
               HI6421_REG_BT_CONFIG, HI6421_INTERFACE_PORT_BIT, 1, 0),
    SOC_SINGLE("INTERFACE BT EDGE",
               HI6421_REG_BT_CONFIG, HI6421_INTERFACE_EDGE_BIT, 1, 0),
    SOC_SINGLE("INTERFACE BT FSP",
               HI6421_REG_BT_CONFIG, HI6421_INTERFACE_FSP_BIT, 1, 0),
    /* 0xBD - MODEM */
    SOC_SINGLE("INTERFACE MODEM HZ",
               HI6421_REG_MODEM_CONFIG, HI6421_INTERFACE_HZ_BIT, 1, 0),
    SOC_SINGLE("INTERFACE MODEM BITS",
               HI6421_REG_MODEM_CONFIG, HI6421_INTERFACE_DATABITS_BIT, 3, 0),
    SOC_SINGLE("INTERFACE MODEM DATATYPE",
               HI6421_REG_MODEM_CONFIG, HI6421_INTERFACE_DATATYPE_BIT, 1, 0),
    SOC_SINGLE("INTERFACE MODEM PORT",
               HI6421_REG_MODEM_CONFIG, HI6421_INTERFACE_PORT_BIT, 1, 0),
    SOC_SINGLE("INTERFACE MODEM EDGE",
               HI6421_REG_MODEM_CONFIG, HI6421_INTERFACE_EDGE_BIT, 1, 0),
    SOC_SINGLE("INTERFACE MODEM FSP",
               HI6421_REG_MODEM_CONFIG, HI6421_INTERFACE_FSP_BIT, 1, 0),

	/*0xBE - FREQ*/
    SOC_SINGLE("INTERFACE BT FS",
               HI6421_REG_FREQ_CONFIG, HI6421_FREQ_BT_BIT, 3, 0),
    SOC_SINGLE("INTERFACE MODEM FS",
               HI6421_REG_FREQ_CONFIG, HI6421_FREQ_MODEM_BIT, 1, 0),

    /* AUDIO TUNING TOOL */
    /* 0x90 */
    SOC_SINGLE("HI6421_DMNS_CONFIG",
               HI6421_REG_CIC_DMNS_CONFIG, HI6421_DMNS_CFG_BIT, 0x3, 0),
    /* 0x91 */
    SOC_SINGLE("HI6421_DMNS_GAIN",
               HI6421_REG_DMNS_GAIN, 0, 0xF, 0),
    /* 0x93 */
    SOC_SINGLE("HI6421_AHARM_GAIN",
               HI6421_REG_AHARM_GAIN, 0, 0xFF, 0),
    /* 0xBF */
    SOC_SINGLE("HI6421_ADCL_HPF",
               HI6421_REG_BYPASS_CONFIG, HI6421_BYPASS_ADCL_HPF_BIT, 1, 0),
    SOC_SINGLE("HI6421_ADCR_HPF",
               HI6421_REG_BYPASS_CONFIG, HI6421_BYPASS_ADCR_HPF_BIT, 1, 0),
    SOC_SINGLE("HI6421_DACL_DEEMP",
               HI6421_REG_BYPASS_CONFIG, HI6421_BYPASS_DACL_DEEMP_BIT, 1, 0),
    SOC_SINGLE("HI6421_DACR_DEEMP",
               HI6421_REG_BYPASS_CONFIG, HI6421_BYPASS_DACR_DEEMP_BIT, 1, 0),
    SOC_SINGLE("HI6421_DACL_AHARM",
               HI6421_REG_BYPASS_CONFIG, HI6421_BYPASS_DACL_AHARM_BIT, 1, 0),
    SOC_SINGLE("HI6421_DACR_AHARM",
               HI6421_REG_BYPASS_CONFIG, HI6421_BYPASS_DACR_AHARM_BIT, 1, 0),
    /* 0xC1 */
    SOC_SINGLE("HI6421_DACL_SDM_DMIC",
               HI6421_DMIC_SDM_CFG, HI6421_DACL_SDM_DITHER, 1, 0),
    SOC_SINGLE("HI6421_DACR_SDM_DMIC",
               HI6421_DMIC_SDM_CFG, HI6421_DACR_SDM_DITHER, 1, 0),
    SOC_SINGLE("HI6421_DACV_SDM_DMIC",
               HI6421_DMIC_SDM_CFG, HI6421_DACV_SDM_DITHER, 1, 0),

    /* CAPTURE AGC LEFT : 0x94 - 0x99 */
    SOC_SINGLE("HI6421_AGC_C_L_CONFIG_1",
               HI6421_REG_AGC_C_L_CONFIG_1, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_C_L_CONFIG_2",
               HI6421_REG_AGC_C_L_CONFIG_2, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_C_L_CONFIG_3",
               HI6421_REG_AGC_C_L_CONFIG_3, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_C_L_CONFIG_4",
               HI6421_REG_AGC_C_L_CONFIG_4, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_C_L_CONFIG_5",
               HI6421_REG_AGC_C_L_CONFIG_5, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_C_L_GAIN",
               HI6421_REG_AGC_C_L_GAIN, 0, 0xFF, 0),

    /* CAPTURE AGC RIGTH : 0x9A - 0x9F */
    SOC_SINGLE("HI6421_AGC_C_R_CONFIG_1",
               HI6421_REG_AGC_C_R_CONFIG_1, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_C_R_CONFIG_2",
               HI6421_REG_AGC_C_R_CONFIG_2, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_C_R_CONFIG_3",
               HI6421_REG_AGC_C_R_CONFIG_3, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_C_R_CONFIG_4",
               HI6421_REG_AGC_C_R_CONFIG_4, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_C_R_CONFIG_5",
               HI6421_REG_AGC_C_R_CONFIG_5, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_C_R_GAIN",
               HI6421_REG_AGC_C_R_GAIN, 0, 0xFF, 0),

    /* GAIN FRACT : 0xA1 */
    SOC_SINGLE("HI6421_APL_GAIN_FRACT",
               HI6421_REG_GAIN_FRACT, HI6421_V200_APL_GAIN_FRACT_BIT, 1, 0),
    SOC_SINGLE("HI6421_APR_GAIN_FRACT",
               HI6421_REG_GAIN_FRACT, HI6421_V200_APR_GAIN_FRACT_BIT, 1, 0),
    SOC_SINGLE("HI6421_MODEMDL_GAIN_FRACT",
               HI6421_REG_GAIN_FRACT, HI6421_MODEMDL_GAIN_FRACT_BIT, 1, 0),
    SOC_SINGLE("HI6421_MODEMUL_GAIN_FRACT",
               HI6421_REG_GAIN_FRACT, HI6421_MODEMUL_GAIN_FRACT_BIT, 1, 0),
    SOC_SINGLE("HI6421_AGC_C_L_GAIN_FRACT",
               HI6421_REG_GAIN_FRACT, HI6421_AGC_C_L_GAIN_FRACT_BIT, 1, 0),
    SOC_SINGLE("HI6421_AGC_C_R_GAIN_FRACT",
               HI6421_REG_GAIN_FRACT, HI6421_AGC_C_R_GAIN_FRACT_BIT, 1, 0),

    /* PLAYBACK GAIN LEFT : 0xA2 */
    SOC_SINGLE("HI6421_APL_GAIN",
               HI6421_APL_GAIN, 0, 0xFF, 0),
    /* PLAYBACK GAIN RIGHT : 0xA3 */
    SOC_SINGLE("HI6421_APR_GAIN",
               HI6421_APR_GAIN, 0, 0xFF, 0),
    /* MODME DOWNLINK GAIN : 0xA4 */
    SOC_SINGLE("HI6421_MODEMDL_GAIN",
               HI6421_MODEMD_GAIN, 0, 0xFF, 0),
    /* MODME UPLINK GAIN : 0xA5 */
    SOC_SINGLE("HI6421_MODEMUL_GAIN",
               HI6421_MODEMU_GAIN, 0, 0xFF, 0),

    /* MODEMDL EQ : 0xA6 - 0xAD */
    SOC_SINGLE("HI6421_MODEMDL_EQ_CONFIG",
               HI6421_MODEMDL_EQ_CONFIG, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_MODEMDL_EQ_125HZ",
               HI6421_MODEMDL_EQ_125HZ, 0, 0x1F, 0),
    SOC_SINGLE("HI6421_MODEMDL_EQ_250HZ",
               HI6421_MODEMDL_EQ_250HZ, 0, 0x1F, 0),
    SOC_SINGLE("HI6421_MODEMDL_EQ_500HZ",
               HI6421_MODEMDL_EQ_500HZ, 0, 0x1F, 0),
    SOC_SINGLE("HI6421_MODEMDL_EQ_1KHZ",
               HI6421_MODEMDL_EQ_1KHZ, 0, 0x1F, 0),
    SOC_SINGLE("HI6421_MODEMDL_EQ_2KHZ",
               HI6421_MODEMDL_EQ_2KHZ, 0, 0x1F, 0),
    SOC_SINGLE("HI6421_MODEMDL_EQ_HS",
               HI6421_MODEMDL_EQ_HS, 0, 0x1F, 0),
    SOC_SINGLE("HI6421_MODEMDL_EQ_LS",
               HI6421_MODEMDL_EQ_LS, 0, 0x1F, 0),
    /* MODEMUL EQ : 0xAE - 0xB5 */
    SOC_SINGLE("HI6421_MODEMUL_EQ_CONFIG",
               HI6421_MODEMUL_EQ_CONFIG, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_MODEMUL_EQ_125HZ",
               HI6421_MODEMUL_EQ_125HZ, 0, 0x1F, 0),
    SOC_SINGLE("HI6421_MODEMUL_EQ_250HZ",
               HI6421_MODEMUL_EQ_250HZ, 0, 0x1F, 0),
    SOC_SINGLE("HI6421_MODEMUL_EQ_500HZ",
               HI6421_MODEMUL_EQ_500HZ, 0, 0x1F, 0),
    SOC_SINGLE("HI6421_MODEMUL_EQ_1KHZ",
               HI6421_MODEMUL_EQ_1KHZ, 0, 0x1F, 0),
    SOC_SINGLE("HI6421_MODEMUL_EQ_2KHZ",
               HI6421_MODEMUL_EQ_2KHZ, 0, 0x1F, 0),
    SOC_SINGLE("HI6421_MODEMUL_EQ_HS",
               HI6421_MODEMUL_EQ_HS, 0, 0x1F, 0),
    SOC_SINGLE("HI6421_MODEMUL_EQ_LS",
               HI6421_MODEMUL_EQ_LS, 0, 0x1F, 0),
};

static const struct snd_kcontrol_new hi6421_v220_snd_controls[] = {
    /* VOLUME */
    SOC_SINGLE_TLV("Main PGA Volume",
            HI6421_REG_MAINPGA, HI6421_MAINPGA_GAIN_BIT,
            13, /* MAX VALUE(13) SEE IN MAIN MIC GAIN NOTE */
            0, main_mic_tlv),
    SOC_SINGLE_TLV("Sub PGA Volume",
            HI6421_REG_SUBPGA, HI6421_SUBPGA_GAIN_BIT,
            13, /* MAX VALUE(13) SEE IN SUB MIC GAIN NOTE */
            0, sub_mic_tlv),
    SOC_SINGLE_TLV("Side PGA Volume",
            HI6421_REG_SIDEPGA, HI6421_SIDEPGA_GAIN_BIT,
            23, /* MAX VALUE(23) SEE IN SIDE MIC GAIN NOTE */
            0, side_mic_tlv),
    SOC_SINGLE_TLV("Earpiece Volume",
            HI6421_REG_EARPIECEPGA, HI6421_EARPIECEPGA_GAIN_BIT,
            32, /* MAX VALUE(32) SEE IN EARPIECE GAIN NOTE */
            0, earpiece_tlv),
    SOC_SINGLE_TLV("Speaker Volume",
            HI6421_REG_CLASSDPGA, HI6421_CLASSDPGA_GAIN_BIT,
            32, /* MAX VALUE(32) SEE IN CLASSD GAIN NOTE */
            0, classd_tlv),

    SOC_SINGLE_TLV("LineinL Volume",
            HI6421_REG_LINEINLPGA, HI6421_LINEOUTPGA_GAIN_BIT,
            23, /* MAX VALUE(23) SEE IN LINEIN GAIN NOTE */
            0, linein_tlv),
    SOC_SINGLE_TLV("LineinR Volume",
            HI6421_REG_LINEINRPGA, HI6421_LINEOUTPGA_GAIN_BIT,
            23, /* MAX VALUE(23) SEE IN LINEIN GAIN NOTE */
            0, linein_tlv),
    SOC_SINGLE_TLV("LineoutL Volume",
            HI6421_REG_LINEOUTLPGA, HI6421_LINEOUTPGA_GAIN_BIT,
            32, /* MAX VALUE(32) SEE IN LINEOUT GAIN NOTE */
            0, lineout_tlv),
    SOC_SINGLE_TLV("LineoutR Volume",
            HI6421_REG_LINEOUTRPGA, HI6421_LINEOUTPGA_GAIN_BIT,
            32, /* MAX VALUE(32) SEE IN LINEOUT GAIN NOTE */
            0, lineout_tlv),
    SOC_SINGLE_TLV("HeadsetL Volume",
            HI6421_REG_HEADSETPGAL, HI6421_HEADSETPGA_GAIN_BIT,
            32, /* MAX VALUE(32) SEE IN HEADSET GAIN NOTE */
            0, headset_tlv),
    SOC_SINGLE_TLV("HeadsetR Volume",
            HI6421_REG_HEADSETPGAR, HI6421_HEADSETPGA_GAIN_BIT,
            32, /* MAX VALUE(32) SEE IN HEADSET GAIN NOTE */
            0, headset_tlv),

    SOC_SINGLE("Main PGA Boost",
               HI6421_REG_MAINPGA, HI6421_MAINPGA_BOOST_BIT, 1, 0),
    SOC_SINGLE("Main PGA Mute",
               HI6421_REG_MAINPGA, HI6421_MAINPGA_MUTE_BIT, 1, 0),
    SOC_SINGLE("Sub PGA Boost",
               HI6421_REG_SUBPGA, HI6421_SUBPGA_BOOST_BIT, 1, 0),
    SOC_SINGLE("Sub PGA Mute",
               HI6421_REG_SUBPGA, HI6421_SUBPGA_MUTE_BIT, 1, 0),
    SOC_SINGLE("Side PGA Mute",
               HI6421_REG_SIDEPGA, HI6421_SIDEPGA_MUTE_BIT, 1, 0),
    SOC_SINGLE("Earpiece Mute",
               HI6421_REG_EARPIECEPGA, HI6421_EARPIECEPGA_MUTE_BIT, 1, 0),
    SOC_SINGLE("Speaker Mute",
               HI6421_REG_CLASSDPGA, HI6421_CLASSDPGA_MUTE_BIT, 1, 0),

    SOC_SINGLE("LineinL Mute",
               HI6421_REG_LINEINLPGA, HI6421_LINEINPGA_MUTE_BIT, 1, 0),
    SOC_SINGLE("LineinR Mute",
               HI6421_REG_LINEINRPGA, HI6421_LINEINPGA_MUTE_BIT, 1, 0),
    SOC_SINGLE("LineoutL Mute",
               HI6421_REG_LINEOUTLPGA, HI6421_LINEOUTPGA_MUTE_BIT, 1, 0),
    SOC_SINGLE("LineoutR Mute",
               HI6421_REG_LINEOUTRPGA, HI6421_LINEOUTPGA_MUTE_BIT, 1, 0),
    SOC_SINGLE("HeadsetL Mute",
               HI6421_REG_HEADSETPGAL, HI6421_HEADSETPGA_MUTE_BIT, 1, 0),
    SOC_SINGLE("HeadsetR Mute",
               HI6421_REG_HEADSETPGAR, HI6421_HEADSETPGA_MUTE_BIT, 1, 0),

    /* INTERFACE */
    /* 0xBB - AP */
    SOC_SINGLE("INTERFACE AP HZ",
               HI6421_REG_AP_CONFIG, HI6421_INTERFACE_HZ_BIT, 1, 0),
    SOC_SINGLE("INTERFACE AP BITS",
               HI6421_REG_AP_CONFIG, HI6421_INTERFACE_DATABITS_BIT, 3, 0),
    SOC_SINGLE("INTERFACE AP DATATYPE",
               HI6421_REG_AP_CONFIG, HI6421_INTERFACE_DATATYPE_BIT, 1, 0),
    SOC_SINGLE("INTERFACE AP PORT",
               HI6421_REG_AP_CONFIG, HI6421_INTERFACE_PORT_BIT, 1, 0),
    SOC_SINGLE("INTERFACE AP EDGE",
               HI6421_REG_AP_CONFIG, HI6421_INTERFACE_EDGE_BIT, 1, 0),
    SOC_SINGLE("INTERFACE AP FSP",
               HI6421_REG_AP_CONFIG, HI6421_INTERFACE_FSP_BIT, 1, 0),
    /* 0xBC - BT */
    SOC_SINGLE("INTERFACE BT HZ",
               HI6421_REG_BT_CONFIG, HI6421_INTERFACE_HZ_BIT, 1, 0),
    SOC_SINGLE("INTERFACE BT BITS",
               HI6421_REG_BT_CONFIG, HI6421_INTERFACE_DATABITS_BIT, 3, 0),
    SOC_SINGLE("INTERFACE BT DATATYPE",
               HI6421_REG_BT_CONFIG, HI6421_INTERFACE_DATATYPE_BIT, 1, 0),
    SOC_SINGLE("INTERFACE BT PORT",
               HI6421_REG_BT_CONFIG, HI6421_INTERFACE_PORT_BIT, 1, 0),
    SOC_SINGLE("INTERFACE BT EDGE",
               HI6421_REG_BT_CONFIG, HI6421_INTERFACE_EDGE_BIT, 1, 0),
    SOC_SINGLE("INTERFACE BT FSP",
               HI6421_REG_BT_CONFIG, HI6421_INTERFACE_FSP_BIT, 1, 0),
    /* 0xBD - MODEM */
    SOC_SINGLE("INTERFACE MODEM HZ",
               HI6421_REG_MODEM_CONFIG, HI6421_INTERFACE_HZ_BIT, 1, 0),
    SOC_SINGLE("INTERFACE MODEM BITS",
               HI6421_REG_MODEM_CONFIG, HI6421_INTERFACE_DATABITS_BIT, 3, 0),
    SOC_SINGLE("INTERFACE MODEM DATATYPE",
               HI6421_REG_MODEM_CONFIG, HI6421_INTERFACE_DATATYPE_BIT, 1, 0),
    SOC_SINGLE("INTERFACE MODEM PORT",
               HI6421_REG_MODEM_CONFIG, HI6421_INTERFACE_PORT_BIT, 1, 0),
    SOC_SINGLE("INTERFACE MODEM EDGE",
               HI6421_REG_MODEM_CONFIG, HI6421_INTERFACE_EDGE_BIT, 1, 0),
    SOC_SINGLE("INTERFACE MODEM FSP",
               HI6421_REG_MODEM_CONFIG, HI6421_INTERFACE_FSP_BIT, 1, 0),

	/*0xBE - FREQ*/
    SOC_SINGLE("INTERFACE BT FS",
               HI6421_REG_FREQ_CONFIG, HI6421_FREQ_BT_BIT, 3, 0),
    SOC_SINGLE("INTERFACE MODEM FS",
               HI6421_REG_FREQ_CONFIG, HI6421_FREQ_MODEM_BIT, 1, 0),

    /* AUDIO TUNING TOOL */
    /* 0x90 */
    SOC_SINGLE("HI6421_DMNS_CONFIG",
               HI6421_REG_CIC_DMNS_CONFIG, HI6421_DMNS_CFG_BIT, 0x3, 0),
    /* 0x91 */
    SOC_SINGLE("HI6421_DMNS_GAIN",
               HI6421_REG_DMNS_GAIN, 0, 0xF, 0),
    /* 0x93 */
    SOC_SINGLE("HI6421_AHARM_GAIN",
               HI6421_REG_AHARM_GAIN, 0, 0xFF, 0),
    /* 0xBF */
    SOC_SINGLE("HI6421_ADCL_HPF",
               HI6421_REG_BYPASS_CONFIG, HI6421_BYPASS_ADCL_HPF_BIT, 1, 0),
    SOC_SINGLE("HI6421_ADCR_HPF",
               HI6421_REG_BYPASS_CONFIG, HI6421_BYPASS_ADCR_HPF_BIT, 1, 0),
    SOC_SINGLE("HI6421_DACL_DEEMP",
               HI6421_REG_BYPASS_CONFIG, HI6421_BYPASS_DACL_DEEMP_BIT, 1, 0),
    SOC_SINGLE("HI6421_DACR_DEEMP",
               HI6421_REG_BYPASS_CONFIG, HI6421_BYPASS_DACR_DEEMP_BIT, 1, 0),
    SOC_SINGLE("HI6421_DACL_AHARM",
               HI6421_REG_BYPASS_CONFIG, HI6421_BYPASS_DACL_AHARM_BIT, 1, 0),
    SOC_SINGLE("HI6421_DACR_AHARM",
               HI6421_REG_BYPASS_CONFIG, HI6421_BYPASS_DACR_AHARM_BIT, 1, 0),
    /* 0xC1 */
    SOC_SINGLE("HI6421_DACL_SDM_DMIC",
               HI6421_DMIC_SDM_CFG, HI6421_DACL_SDM_DITHER, 1, 0),
    SOC_SINGLE("HI6421_DACR_SDM_DMIC",
               HI6421_DMIC_SDM_CFG, HI6421_DACR_SDM_DITHER, 1, 0),
    SOC_SINGLE("HI6421_DACV_SDM_DMIC",
               HI6421_DMIC_SDM_CFG, HI6421_DACV_SDM_DITHER, 1, 0),

    /* CAPTURE AGC LEFT : 0x94 - 0x 99*/
    SOC_SINGLE("HI6421_AGC_C_L_CONFIG_1",
               HI6421_REG_AGC_C_L_CONFIG_1, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_C_L_CONFIG_2",
               HI6421_REG_AGC_C_L_CONFIG_2, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_C_L_CONFIG_3",
               HI6421_REG_AGC_C_L_CONFIG_3, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_C_L_CONFIG_4",
               HI6421_REG_AGC_C_L_CONFIG_4, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_C_L_CONFIG_5",
               HI6421_REG_AGC_C_L_CONFIG_5, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_C_L_GAIN",
               HI6421_REG_AGC_C_L_GAIN, 0, 0xFF, 0),

    /* CAPTURE AGC RIGHT : 0x9A - 0x9F */
    SOC_SINGLE("HI6421_AGC_C_R_CONFIG_1",
               HI6421_REG_AGC_C_R_CONFIG_1, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_C_R_CONFIG_2",
               HI6421_REG_AGC_C_R_CONFIG_2, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_C_R_CONFIG_3",
               HI6421_REG_AGC_C_R_CONFIG_3, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_C_R_CONFIG_4",
               HI6421_REG_AGC_C_R_CONFIG_4, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_C_R_CONFIG_5",
               HI6421_REG_AGC_C_R_CONFIG_5, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_C_R_GAIN",
               HI6421_REG_AGC_C_R_GAIN, 0, 0xFF, 0),

    /* GAIN FRACT : 0xA1 */
    SOC_SINGLE("HI6421_AGC_P_L_GAIN_FRACT",
               HI6421_REG_GAIN_FRACT, HI6421_V220_AGC_P_L_GAIN_FRACT_BIT, 1, 0),
    SOC_SINGLE("HI6421_AGC_P_R_GAIN_FRACT",
               HI6421_REG_GAIN_FRACT, HI6421_V220_AGC_P_R_GAIN_FRACT_BIT, 1, 0),
    SOC_SINGLE("HI6421_AGC_C_L_GAIN_FRACT",
               HI6421_REG_GAIN_FRACT, HI6421_AGC_C_L_GAIN_FRACT_BIT, 1, 0),
    SOC_SINGLE("HI6421_AGC_C_R_GAIN_FRACT",
               HI6421_REG_GAIN_FRACT, HI6421_AGC_C_R_GAIN_FRACT_BIT, 1, 0),
    SOC_SINGLE("HI6421_MODEMUL_L_GAIN_FRACT",
               HI6421_REG_GAIN_FRACT, HI6421_MODEMUL_L_GAIN_FRACT_BIT, 1, 0),
    SOC_SINGLE("HI6421_MODEMUL_R_GAIN_FRACT",
               HI6421_REG_GAIN_FRACT, HI6421_MODEMUL_R_GAIN_FRACT_BIT, 1, 0),
    SOC_SINGLE("HI6421_MODEMDL_GAIN_FRACT",
               HI6421_REG_GAIN_FRACT, HI6421_MODEMDL_GAIN_FRACT_BIT, 1, 0),

    /* MODME UPLINK GAIN RIGHT : 0xA2 */
    SOC_SINGLE("HI6421_MODEMUL_R_GAIN",
               HI6421_MODEMUL_R_GAIN, 0, 0xFF, 0),
    /* MODME DOWNLINK GAIN : 0xA4 */
    SOC_SINGLE("HI6421_MODEMDL_GAIN",
               HI6421_MODEMDL_GAIN, 0, 0xFF, 0),
    /* MODME UPLINK GAIN LEFT : 0xA5 */
    SOC_SINGLE("HI6421_MODEMUL_L_GAIN",
               HI6421_MODEMUL_L_GAIN, 0, 0xFF, 0),

    /* PLAYBACK AGC LEFT : 0xAE - 0xB3 */
    SOC_SINGLE("HI6421_AGC_P_L_CONFIG_1",
               HI6421_REG_AGC_P_L_CONFIG_1, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_P_L_CONFIG_2",
               HI6421_REG_AGC_P_L_CONFIG_2, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_P_L_CONFIG_3",
               HI6421_REG_AGC_P_L_CONFIG_3, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_P_L_CONFIG_4",
               HI6421_REG_AGC_P_L_CONFIG_4, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_P_L_CONFIG_5",
               HI6421_REG_AGC_P_L_CONFIG_5, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_P_L_GAIN",
               HI6421_REG_AGC_P_L_GAIN, 0, 0xFF, 0),

    /* PLAYBACK AGC RIGHT : 0xC2 - 0xC7 */
    SOC_SINGLE("HI6421_AGC_P_R_CONFIG_1",
               HI6421_REG_AGC_P_R_CONFIG_1, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_P_R_CONFIG_2",
               HI6421_REG_AGC_P_R_CONFIG_2, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_P_R_CONFIG_3",
               HI6421_REG_AGC_P_R_CONFIG_3, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_P_R_CONFIG_4",
               HI6421_REG_AGC_P_R_CONFIG_4, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_P_R_CONFIG_5",
               HI6421_REG_AGC_P_R_CONFIG_5, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_AGC_P_R_GAIN",
               HI6421_REG_AGC_P_R_GAIN, 0, 0xFF, 0),

    /* MODEMDL EQ : 0xA6 - 0xAD */
    SOC_SINGLE("HI6421_MODEMDL_EQ_CONFIG",
               HI6421_MODEMDL_EQ_CONFIG, 0, 0xFF, 0),
    SOC_SINGLE("HI6421_MODEMDL_EQ_125HZ",
               HI6421_MODEMDL_EQ_125HZ, 0, 0x1F, 0),
    SOC_SINGLE("HI6421_MODEMDL_EQ_250HZ",
               HI6421_MODEMDL_EQ_250HZ, 0, 0x1F, 0),
    SOC_SINGLE("HI6421_MODEMDL_EQ_500HZ",
               HI6421_MODEMDL_EQ_500HZ, 0, 0x1F, 0),
    SOC_SINGLE("HI6421_MODEMDL_EQ_1KHZ",
               HI6421_MODEMDL_EQ_1KHZ, 0, 0x1F, 0),
    SOC_SINGLE("HI6421_MODEMDL_EQ_2KHZ",
               HI6421_MODEMDL_EQ_2KHZ, 0, 0x1F, 0),
    SOC_SINGLE("HI6421_MODEMDL_EQ_HS",
               HI6421_MODEMDL_EQ_HS, 0, 0x1F, 0),
    SOC_SINGLE("HI6421_MODEMDL_EQ_LS",
               HI6421_MODEMDL_EQ_LS, 0, 0x1F, 0),
};

/* DAPM KCONTROLS */

/* MONO MIXER */
static const struct snd_kcontrol_new hi6421_mixermono_controls[] = {
    SOC_DAPM_SINGLE("DAC Left Switch",
                    HI6421_REG_MONOMIX, HI6421_MIXER_DACL_BIT, 1, 0),
    SOC_DAPM_SINGLE("DAC Right Switch",
                    HI6421_REG_MONOMIX, HI6421_MIXER_DACR_BIT, 1, 0),
    SOC_DAPM_SINGLE("DAC Voice Switch",
                    HI6421_REG_MONOMIX, HI6421_MIXER_DACV_BIT, 1, 0),
    SOC_DAPM_SINGLE("Linein Left Switch",
                    HI6421_REG_MONOMIX, HI6421_MIXER_LINEINL_BIT, 1, 0),
    SOC_DAPM_SINGLE("Linein Right Switch",
                    HI6421_REG_MONOMIX, HI6421_MIXER_LINEINR_BIT, 1, 0),
    SOC_DAPM_SINGLE("Main PGA Switch",
                    HI6421_REG_MONOMIX, HI6421_MIXER_MAINMIC_BIT, 1, 0),
    SOC_DAPM_SINGLE("Side PGA Switch",
                    HI6421_REG_MONOMIX, HI6421_MIXER_SIDETONE_BIT, 1, 0),
};

/* LEFT OUT MIXER */
static const struct snd_kcontrol_new hi6421_mixeroutleft_controls[] = {
    SOC_DAPM_SINGLE("DAC Left Switch",
                    HI6421_REG_MIXOUTL, HI6421_MIXER_DACL_BIT, 1, 0),
    SOC_DAPM_SINGLE("DAC Right Switch",
                    HI6421_REG_MIXOUTL, HI6421_MIXER_DACR_BIT, 1, 0),
    SOC_DAPM_SINGLE("DAC Voice Switch",
                    HI6421_REG_MIXOUTL, HI6421_MIXER_DACV_BIT, 1, 0),
    SOC_DAPM_SINGLE("Linein Left Switch",
                    HI6421_REG_MIXOUTL, HI6421_MIXER_LINEINL_BIT, 1, 0),
    SOC_DAPM_SINGLE("Linein Right Switch",
                    HI6421_REG_MIXOUTL, HI6421_MIXER_LINEINR_BIT, 1, 0),
    SOC_DAPM_SINGLE("Main PGA Switch",
                    HI6421_REG_MIXOUTL, HI6421_MIXER_MAINMIC_BIT, 1, 0),
    SOC_DAPM_SINGLE("Side PGA Switch",
                    HI6421_REG_MIXOUTL, HI6421_MIXER_SIDETONE_BIT, 1, 0),
};

/* RIGHT OUT MIXER */
static const struct snd_kcontrol_new hi6421_mixeroutright_controls[] = {
    SOC_DAPM_SINGLE("DAC Left Switch",
                    HI6421_REG_MIXOUTR, HI6421_MIXER_DACL_BIT, 1, 0),
    SOC_DAPM_SINGLE("DAC Right Switch",
                    HI6421_REG_MIXOUTR, HI6421_MIXER_DACR_BIT, 1, 0),
    SOC_DAPM_SINGLE("DAC Voice Switch",
                    HI6421_REG_MIXOUTR, HI6421_MIXER_DACV_BIT, 1, 0),
    SOC_DAPM_SINGLE("Linein Left Switch",
                    HI6421_REG_MIXOUTR, HI6421_MIXER_LINEINL_BIT, 1, 0),
    SOC_DAPM_SINGLE("Linein Right Switch",
                    HI6421_REG_MIXOUTR, HI6421_MIXER_LINEINR_BIT, 1, 0),
    SOC_DAPM_SINGLE("Main PGA Switch",
                    HI6421_REG_MIXOUTR, HI6421_MIXER_MAINMIC_BIT, 1, 0),
    SOC_DAPM_SINGLE("Side PGA Switch",
                    HI6421_REG_MIXOUTR, HI6421_MIXER_SIDETONE_BIT, 1, 0),
};

/* LEFT IN MIXER */
static const struct snd_kcontrol_new hi6421_mixerinleft_controls[] = {
    SOC_DAPM_SINGLE("DAC Left Switch",
                    HI6421_REG_MIXINL, HI6421_MIXER_DACL_BIT, 1, 0),
    SOC_DAPM_SINGLE("DAC Right Switch",
                    HI6421_REG_MIXINL, HI6421_MIXER_DACR_BIT, 1, 0),
    SOC_DAPM_SINGLE("DAC Voice Switch",
                    HI6421_REG_MIXINL, HI6421_MIXER_DACV_BIT, 1, 0),
    SOC_DAPM_SINGLE("Linein Left Switch",
                    HI6421_REG_MIXINL, HI6421_MIXER_LINEINL_BIT, 1, 0),
    SOC_DAPM_SINGLE("Linein Right Switch",
                    HI6421_REG_MIXINL, HI6421_MIXER_LINEINR_BIT, 1, 0),
    SOC_DAPM_SINGLE("Main PGA Switch",
                    HI6421_REG_MIXINL, HI6421_MIXER_MAINMIC_BIT, 1, 0),
    SOC_DAPM_SINGLE("Sub PGA Switch",
                    HI6421_REG_MIXINL, HI6421_MIXER_SUBMIC_BIT, 1, 0),
};

/* RIGHT IN MIXER */
static const struct snd_kcontrol_new hi6421_mixerinright_controls[] = {
    SOC_DAPM_SINGLE("DAC Left Switch",
                    HI6421_REG_MIXINR, HI6421_MIXER_DACL_BIT, 1, 0),
    SOC_DAPM_SINGLE("DAC Right Switch",
                    HI6421_REG_MIXINR, HI6421_MIXER_DACR_BIT, 1, 0),
    SOC_DAPM_SINGLE("DAC Voice Switch",
                    HI6421_REG_MIXINR, HI6421_MIXER_DACV_BIT, 1, 0),
    SOC_DAPM_SINGLE("Linein Left Switch",
                    HI6421_REG_MIXINR, HI6421_MIXER_LINEINL_BIT, 1, 0),
    SOC_DAPM_SINGLE("Linein Right Switch",
                    HI6421_REG_MIXINR, HI6421_MIXER_LINEINR_BIT, 1, 0),
    SOC_DAPM_SINGLE("Main PGA Switch",
                    HI6421_REG_MIXINR, HI6421_MIXER_MAINMIC_BIT, 1, 0),
    SOC_DAPM_SINGLE("Sub PGA Switch",
                    HI6421_REG_MIXINR, HI6421_MIXER_SUBMIC_BIT, 1, 0),
};

/* LINEOUT LEFT MIXER */
static const struct snd_kcontrol_new hi6421_mixerlineoutleft_controls[] = {
    SOC_DAPM_SINGLE("Mixer Left Switch",
                    HI6421_REG_LINEOUTMIXL, HI6421_LINEOUTMIXER_DACL_BIT, 1, 0),
    SOC_DAPM_SINGLE("Mixer Right Switch",
                    HI6421_REG_LINEOUTMIXL, HI6421_LINEOUTMIXER_DACR_BIT, 1, 0),
    SOC_DAPM_SINGLE("Mixer Mono Switch",
                    HI6421_REG_LINEOUTMIXL, HI6421_LINEOUTMIXER_MONO_BIT, 1, 0),
};

/* LINEOUT RIGHT MIXER */
static const struct snd_kcontrol_new hi6421_mixerlineoutright_controls[] = {
    SOC_DAPM_SINGLE("Mixer Left Switch",
                    HI6421_REG_LINEOUTMIXR, HI6421_LINEOUTMIXER_DACL_BIT, 1, 0),
    SOC_DAPM_SINGLE("Mixer Right Switch",
                    HI6421_REG_LINEOUTMIXR, HI6421_LINEOUTMIXER_DACR_BIT, 1, 0),
    SOC_DAPM_SINGLE("Mixer Mono Switch",
                    HI6421_REG_LINEOUTMIXR, HI6421_LINEOUTMIXER_MONO_BIT, 1, 0),
};

/* MUX KCONTROLS */

/* AP DIGITAL DAC FILTER LEFT MUX CONTROL */
static const char *hi6421_playback_mux_texts[] = {
    "INPUT LR MIX", /* 0 : mix dacl & dacr */
    "INPUT LEFT"    /* 1 : dacl only */
};
static const struct soc_enum hi6421_playback_mux_enum =
    SOC_ENUM_SINGLE(HI6421_REG_BYPASS_CONFIG, HI6421_BYPASS_DACLR_MIX_BIT,
            ARRAY_SIZE(hi6421_playback_mux_texts), hi6421_playback_mux_texts);
static const struct snd_kcontrol_new hi6421_dapm_playback_mux_controls =
    SOC_DAPM_ENUM("Playback Mux", hi6421_playback_mux_enum);

/* AP MIX MUX */
static const char *hi6421_capture_mux_texts[] = {
    "CAPTURE AGCL",     /* 0 : adcl only */
    "CAPTURE AGCLR",    /* 1 : mix adcl & adcr */
    "CAPTURE AGCR"      /* 2 : adcr only */
};
static const struct soc_enum hi6421_capture_mux_enum =
    SOC_ENUM_SINGLE(HI6421_REG_MUX_CONFIG, HI6421_MUX_AGCLR_MIX_BIT,
            ARRAY_SIZE(hi6421_capture_mux_texts), hi6421_capture_mux_texts);
static const struct snd_kcontrol_new hi6421_dapm_capture_mux_controls =
    SOC_DAPM_ENUM("Capture Mux", hi6421_capture_mux_enum);

/* MODEM MUX */
static const char *hi6421_modem_mux_texts[] = {
    "VOICE MIC",     /* 0 : analog mic */
    "VOICE BT",      /* 1 : bt mic */
};
static const struct soc_enum hi6421_modem_mux_enum =
    SOC_ENUM_SINGLE(HI6421_REG_MUX_CONFIG, HI6421_MUX_VOICEU_BIT,
            ARRAY_SIZE(hi6421_modem_mux_texts), hi6421_modem_mux_texts);
static const struct snd_kcontrol_new hi6421_dapm_modem_mux_controls =
    SOC_DAPM_ENUM("Modem Mux", hi6421_modem_mux_enum);

/* BT MUX - V200 */
static const char *hi6421_v200_bt_mux_texts[] = {
    "BT AGCL",       /* 0 : bt agcl  */
    "BT PCM",        /* 1 : bt fm */
    "BT VOICE",      /* 2 : bt incall */
    "BT OFF",        /* 3 : bt off */
};
static const struct soc_enum hi6421_v200_bt_mux_enum =
    SOC_ENUM_SINGLE(HI6421_REG_MUX_CONFIG, HI6421_MUX_BT_BIT,
            ARRAY_SIZE(hi6421_v200_bt_mux_texts), hi6421_v200_bt_mux_texts);
static const struct snd_kcontrol_new hi6421_v200_dapm_bt_mux_controls =
    SOC_DAPM_ENUM("BT Mux V200", hi6421_v200_bt_mux_enum);


/* AGC LEFT MUX */
static const char *hi6421_agc_c_l_mux_texts[] = {
    "AGCL ADCL",     /* 0 : AGCL Mux choose adcl */
    "AGCL VOICEU",   /* 1 : AGCL Mux choose voiceu */
};
static const struct soc_enum hi6421_agc_c_l_mux_enum =
    SOC_ENUM_SINGLE(HI6421_REG_MUX_CONFIG, HI6421_MUX_AGCL_BIT,
            ARRAY_SIZE(hi6421_agc_c_l_mux_texts), hi6421_agc_c_l_mux_texts);
static const struct snd_kcontrol_new hi6421_dapm_agc_c_l_mux_controls =
    SOC_DAPM_ENUM("AGCL Mux", hi6421_agc_c_l_mux_enum);

/* AGC RIGHT MUX */
static const char *hi6421_agc_c_r_mux_texts[] = {
    "AGCR ADCR",     /* 0 : AGCR Mux choose adcr */
    "AGCR VOICED",   /* 1 : AGCR Mux choose voiced */
};
static const struct soc_enum hi6421_agc_c_r_mux_enum =
    SOC_ENUM_SINGLE(HI6421_REG_MUX_CONFIG, HI6421_MUX_AGCR_BIT,
            ARRAY_SIZE(hi6421_agc_c_r_mux_texts), hi6421_agc_c_r_mux_texts);
static const struct snd_kcontrol_new hi6421_dapm_agc_c_r_mux_controls =
    SOC_DAPM_ENUM("AGCR Mux", hi6421_agc_c_r_mux_enum);

/* ADC FLITER LEFT & RIGHT MUX */
static const char *hi6421_adc_mux_texts[] = {
    "ADC ADC",      /* 0 : ADC FILTER Mux choose ADC */
    "ADC DIGMIC",   /* 1 : ADC FILTER Mux choose Digital Mic */
    "ADC DACL",     /* 2 : ADC FILTER Mux choose DAC Filter L */
    "ADC DACR",     /* 3 : ADC FILTER Mux choose DAC Filter R */
    "ADC DACV"      /* 4 : ADC FILTER Mux choose DAC Filter V */
};

static const struct soc_enum hi6421_adcl_mux_enum =
    SOC_ENUM_SINGLE(HI6421_REG_CIC_DMNS_CONFIG, HI6421_ADCL_CIC_BIT,
            ARRAY_SIZE(hi6421_adc_mux_texts), hi6421_adc_mux_texts);
static const struct snd_kcontrol_new hi6421_dapm_adcl_mux_controls =
    SOC_DAPM_ENUM("ADCL Mux", hi6421_adcl_mux_enum);

static const struct soc_enum hi6421_adcr_mux_enum =
    SOC_ENUM_SINGLE(HI6421_REG_CIC_DMNS_CONFIG, HI6421_ADCR_CIC_BIT,
            ARRAY_SIZE(hi6421_adc_mux_texts), hi6421_adc_mux_texts);
static const struct snd_kcontrol_new hi6421_dapm_adcr_mux_controls =
    SOC_DAPM_ENUM("ADCR Mux", hi6421_adcr_mux_enum);

/* DMNS MUX */
static const char *hi6421_dmns_mux_texts[] = {
    "DISABLE",      /* 0 : NOT USE DMNS */
    "ENABLE"        /* 1 : USE DMNS */
};
static const struct soc_enum hi6421_dmns_mux_enum =
    SOC_ENUM_SINGLE(HI6421_REG_MUX_CONFIG, HI6421_MUX_DENOISE_BIT,
            ARRAY_SIZE(hi6421_dmns_mux_texts), hi6421_dmns_mux_texts);
static const struct snd_kcontrol_new hi6421_dapm_dmns_mux_controls =
    SOC_DAPM_ENUM("DMNS Mux", hi6421_dmns_mux_enum);

static const char *hi6421_mic_mux_texts[] = {
    "MIC OFF",      /* 00/11 : off */
    "MIC MAIN",     /* 01 : enable mainmic */
    "MIC HS"        /* 10 : enable hs_mic */
};
static const struct soc_enum hi6421_mic_mux_enum =
    SOC_ENUM_SINGLE(HI6421_REG_MAINPGA, HI6421_MAINPGA_SEL_BIT,
            ARRAY_SIZE(hi6421_mic_mux_texts), hi6421_mic_mux_texts);
static const struct snd_kcontrol_new hi6421_dapm_mic_mux_controls =
    SOC_DAPM_ENUM("MIC Mux", hi6421_mic_mux_enum);

/* KCONTROLS */
/* DACL SWITCH */
static const struct snd_kcontrol_new hi6421_dapm_dacl_switch_controls =
    SOC_DAPM_SINGLE("DACL Switch",
            HI6421_DACL_EN_REG, HI6421_DACL_EN_BIT, 1, 0);

/* DACR SWITCH */
static const struct snd_kcontrol_new hi6421_dapm_dacr_switch_controls =
    SOC_DAPM_SINGLE("DACR Switch",
            HI6421_DACR_EN_REG, HI6421_DACR_EN_BIT, 1, 0);

/* DACV SWITCH */
static const struct snd_kcontrol_new hi6421_dapm_dacv_switch_controls =
    SOC_DAPM_SINGLE("DACV Switch",
            HI6421_DACV_EN_REG, HI6421_DACV_EN_BIT, 1, 0);

/* DIGMIC SWITCH */
static const struct snd_kcontrol_new hi6421_dapm_digmic_switch_controls =
    SOC_DAPM_SINGLE("Digmic Switch",
            HI6421_DIGMIC_EN_REG, HI6421_DIGMIC_EN_BIT, 1, 0);

/* MODEM UL SWITCH */
static const struct snd_kcontrol_new hi6421_dapm_modemul_switch_controls =
    SOC_DAPM_SINGLE("Modem_UL Switch",
            HI6421_MODEM_UL_GAIN_EN_REG, HI6421_MODEM_UL_GAIN_EN_BIT, 1, 0);

/* MODEM_DL SWITCH */
static const struct snd_kcontrol_new hi6421_dapm_modemdl_switch_controls =
    SOC_DAPM_SINGLE("Modem_DL Switch",
            HI6421_REG_EN_CFG_3, 7, 1, 0);

/* MODEM UL L SWITCH */
static const struct snd_kcontrol_new hi6421_dapm_modemull_switch_controls =
    SOC_DAPM_SINGLE("Modem_UL_L Switch",
            HI6421_MODEM_UL_L_GAIN_EN_REG, HI6421_MODEM_UL_L_GAIN_EN_BIT, 1, 0);

/* MODEM UL R SWITCH */
static const struct snd_kcontrol_new hi6421_dapm_modemulr_switch_controls =
    SOC_DAPM_SINGLE("Modem_UL_R Switch",
            HI6421_MODEM_UL_R_GAIN_EN_REG, HI6421_MODEM_UL_R_GAIN_EN_BIT, 1, 0);

/* SRCIU SWITCH */
static const struct snd_kcontrol_new hi6421_srciu_switch_controls =
    SOC_DAPM_SINGLE("Srciu Switch",
            HI6421_SRCIU_EN_REG, HI6421_SRCIU_EN_BIT, 1, 0);

/* SRCIU SWITCH */
static const struct snd_kcontrol_new hi6421_srcid_switch_controls =
    SOC_DAPM_SINGLE("Srcid Switch",
            HI6421_SRCID_EN_REG, HI6421_SRCID_EN_BIT, 1, 0);

/* SPEAKER SWITCH */
static const struct snd_kcontrol_new hi6421_dapm_speaker_switch_controls =
    SOC_DAPM_SINGLE("Speaker Switch",
            HI6421_CLASSD_PD_REG, HI6421_CLASSD_PD_BIT, 1, 1);

/* EARPIECE SWITCH */
static const struct snd_kcontrol_new hi6421_dapm_earpiece_switch_controls =
    SOC_DAPM_SINGLE("Earpiece Switch",
            HI6421_EARPIECE_PD_REG, HI6421_EARPIECE_PD_BIT, 1, 1);

/* HEADSET_L SWITCH */
static const struct snd_kcontrol_new hi6421_dapm_headsetl_switch_controls =
    SOC_DAPM_SINGLE("Headset_L Switch",
            HI6421_HEADSETL_PD_REG, HI6421_HEADSETL_PD_BIT, 1, 1);

/* HEADSET_R SWITCH */
static const struct snd_kcontrol_new hi6421_dapm_headsetr_switch_controls =
    SOC_DAPM_SINGLE("Headset_R Switch",
            HI6421_HEADSETR_PD_REG, HI6421_HEADSETR_PD_BIT, 1, 1);

/* LINEOUT_L SWITCH */
static const struct snd_kcontrol_new hi6421_dapm_lineoutl_switch_controls =
    SOC_DAPM_SINGLE("Lineout_L Switch",
            HI6421_LINEOUTL_PD_REG, HI6421_LINEOUTL_PD_BIT, 1, 1);

/* LINEOUT_R SWITCH */
static const struct snd_kcontrol_new hi6421_dapm_lineoutr_switch_controls =
    SOC_DAPM_SINGLE("Lineout_R Switch",
            HI6421_LINEOUTR_PD_REG, HI6421_LINEOUTR_PD_BIT, 1, 1);

/* BT INCALL SWITCH */
static const struct snd_kcontrol_new hi6421_dapm_bt_incall_switch_controls =
    SOC_DAPM_SINGLE("Bt_incall Switch",
            HI6421_REG_MUX_CONFIG, HI6421_BT_INCALL_BIT, 1, 0);

/* BT PLAYBACK SWITCH */
static const struct snd_kcontrol_new hi6421_dapm_bt_playback_switch_controls =
    SOC_DAPM_SINGLE("Bt_playback Switch",
            HI6421_REG_MUX_CONFIG, HI6421_BT_PLAYBACK_BIT, 1, 0);

/* DAPM EVENT */

/*
* DIGITAL PLAYBACK EVENT
* enable/disable clks : ap_gain, aharm, hbf1i(if ap interface set in 48kHz)
*/
static int hi6421_digharmonic_power_mode_event(struct snd_soc_dapm_widget *w,
        struct snd_kcontrol *kcontrol, int event)
{
    /*pr_info("%s : event = %d\n",__FUNCTION__, event);*/
    switch (event) {
    case SND_SOC_DAPM_PRE_PMU:
        hi6421_set_reg_bit(HI6421_AHARM_EN_REG, HI6421_AHARM_EN_BIT);
        /* The sample rate of AP's interface is 48kHz, need enable hbf1i */
        if (check_ap_interface_48khz())
            hi6421_set_reg_bit(HI6421_HBF1I_EN_REG, HI6421_HBF1I_EN_BIT);
        else
            hi6421_clr_reg_bit(HI6421_HBF1I_EN_REG, HI6421_HBF1I_EN_BIT);
        break;
    case SND_SOC_DAPM_POST_PMD:
        hi6421_clr_reg_bit(HI6421_HBF1I_EN_REG, HI6421_HBF1I_EN_BIT);
        hi6421_clr_reg_bit(HI6421_AHARM_EN_REG, HI6421_AHARM_EN_BIT);
        break;
    default :
        pr_warn("%s : power mode event err : %d\n", __FUNCTION__, event);
        break;
    }

    return 0;
}

/*
* DIGITAL PLAYBACK EVENT
* enable/disable clks : ap_gain, aharm, hbf1i(if ap interface set in 48kHz)
*/
static int hi6421_digplayback_power_mode_event(struct snd_soc_dapm_widget *w,
        struct snd_kcontrol *kcontrol, int event)
{
    pr_info("%s : event = %d\n",__FUNCTION__, event);
    switch (event) {
    case SND_SOC_DAPM_PRE_PMU:
        if (!hi6421_is_cs)
            hi6421_set_reg_bit(HI6421_V200_AP_GAIN_EN_REG, HI6421_V200_AP_GAIN_EN_BIT);
        hi6421_set_reg_bit(HI6421_AHARM_EN_REG, HI6421_AHARM_EN_BIT);
        /* The sample rate of AP's interface is 48kHz, need enable hbf1i */
        if (check_ap_interface_48khz())
            hi6421_set_reg_bit(HI6421_HBF1I_EN_REG, HI6421_HBF1I_EN_BIT);
        else
            hi6421_clr_reg_bit(HI6421_HBF1I_EN_REG, HI6421_HBF1I_EN_BIT);
        break;
    case SND_SOC_DAPM_POST_PMD:
        hi6421_clr_reg_bit(HI6421_HBF1I_EN_REG, HI6421_HBF1I_EN_BIT);
        hi6421_clr_reg_bit(HI6421_AHARM_EN_REG, HI6421_AHARM_EN_BIT);
        if (!hi6421_is_cs)
            hi6421_clr_reg_bit(HI6421_V200_AP_GAIN_EN_REG, HI6421_V200_AP_GAIN_EN_BIT);
        break;
    default :
        pr_warn("%s : power mode event err : %d\n", __FUNCTION__, event);
        break;
    }

    return 0;
}

/*
* AGC LEFT EVENT
* enable/disable clks : agc left, hbf1d(if ap interface set in 48kHz)
*/
static int hi6421_captureagcl_power_mode_event(struct snd_soc_dapm_widget *w,
        struct snd_kcontrol *kcontrol, int event)
{
    pr_info("%s : event = %d\n",__FUNCTION__, event);
    switch (event) {
    case SND_SOC_DAPM_PRE_PMU:
        if (hi6421_is_cs)
            hi6421_set_reg_bit(HI6421_V220_AGC_C_L_EN_REG, HI6421_V220_AGC_C_L_EN_BIT);
        else
            hi6421_set_reg_bit(HI6421_V200_AGC_C_L_EN_REG, HI6421_V200_AGC_C_L_EN_BIT);
        if (check_ap_interface_48khz())
            hi6421_set_reg_bit(HI6421_HBF1DL_EN_REG, HI6421_HBF1DL_EN_BIT);
        else
            hi6421_clr_reg_bit(HI6421_HBF1DL_EN_REG, HI6421_HBF1DL_EN_BIT);
        break;
    case SND_SOC_DAPM_POST_PMD:
        hi6421_clr_reg_bit(HI6421_HBF1DL_EN_REG, HI6421_HBF1DL_EN_BIT);
        if (hi6421_is_cs)
            hi6421_clr_reg_bit(HI6421_V220_AGC_C_L_EN_REG, HI6421_V220_AGC_C_L_EN_BIT);
        else
            hi6421_clr_reg_bit(HI6421_V200_AGC_C_L_EN_REG, HI6421_V200_AGC_C_L_EN_BIT);
        break;
    default :
        pr_warn("%s : power mode event err : %d\n", __FUNCTION__, event);
        break;
    }

    return 0;
}

/*
* AGC RIGHT EVENT
* enable/disable clks : agc right, hbf1d(if ap interface set in 48kHz)
*/
static int hi6421_captureagcr_power_mode_event(struct snd_soc_dapm_widget *w,
        struct snd_kcontrol *kcontrol, int event)
{
    pr_info("%s : event = %d\n",__FUNCTION__, event);
    switch (event) {
    case SND_SOC_DAPM_PRE_PMU:
        if (hi6421_is_cs)
            hi6421_set_reg_bit(HI6421_V220_AGC_C_R_EN_REG, HI6421_V220_AGC_C_R_EN_BIT);
        else
            hi6421_set_reg_bit(HI6421_V200_AGC_C_R_EN_REG, HI6421_V200_AGC_C_R_EN_BIT);
        if (check_ap_interface_48khz())
            hi6421_set_reg_bit(HI6421_HBF1DR_EN_REG, HI6421_HBF1DR_EN_BIT);
        else
            hi6421_clr_reg_bit(HI6421_HBF1DR_EN_REG, HI6421_HBF1DR_EN_BIT);
        break;
    case SND_SOC_DAPM_POST_PMD:
        hi6421_clr_reg_bit(HI6421_HBF1DR_EN_REG, HI6421_HBF1DR_EN_BIT);
        if (hi6421_is_cs)
            hi6421_clr_reg_bit(HI6421_V220_AGC_C_R_EN_REG, HI6421_V220_AGC_C_R_EN_BIT);
        else
            hi6421_clr_reg_bit(HI6421_V200_AGC_C_R_EN_REG, HI6421_V200_AGC_C_R_EN_BIT);
        break;
    default :
        pr_warn("%s : power mode event err : %d\n", __FUNCTION__, event);
        break;
    }

    return 0;
}

static int hi6421_modem_power_mode_event(struct snd_soc_dapm_widget *w,
        struct snd_kcontrol *kcontrol, int event)
{
    pr_info("%s : event = %d\n",__FUNCTION__, event);
    switch (event) {
    case SND_SOC_DAPM_PRE_PMU:
		if(1==get_modem_type(MODEM_SPRD8803G))
		{
        	hi6421_reg_write(g_codec, 0x86, 0x55);
        	usleep_range(100, 105);
        	hi6421_reg_write(g_codec, 0x86, 0x66);
		}
        hi6421_set_reg_bit(HI6421_MODEM_DL_GAIN_EN_REG, HI6421_MODEM_DL_GAIN_EN_BIT);
        break;
    case SND_SOC_DAPM_POST_PMD:
        hi6421_clr_reg_bit(HI6421_MODEM_DL_GAIN_EN_REG, HI6421_MODEM_DL_GAIN_EN_BIT);
        break;
    default :
        pr_warn("%s : power mode event err : %d\n", __FUNCTION__, event);
        break;
    }

    return 0;
}

/*
* SRCIU EVENT in MODEM 8kHz
* enable/disable clks : srciu2
*/
static int hi6421_srciu_power_mode_event(struct snd_soc_dapm_widget *w,
        struct snd_kcontrol *kcontrol, int event)
{
    pr_info("%s : event = %d\n",__FUNCTION__, event);
    switch (event) {
    case SND_SOC_DAPM_PRE_PMU:
        hi6421_set_reg_bit(HI6421_SRCIU_I2_EN_REG, HI6421_SRCIU_I2_EN_BIT);
        break;
    case SND_SOC_DAPM_POST_PMD:
        hi6421_clr_reg_bit(HI6421_SRCIU_I2_EN_REG, HI6421_SRCIU_I2_EN_BIT);
        break;
    default :
        pr_warn("%s : power mode event err : %d\n", __FUNCTION__, event);
        break;
    }

    return 0;
}

/*
* SRCID EVENT in MODEM 8kHz
* enable/disable clks : srcid2
*/
static int hi6421_srcid_power_mode_event(struct snd_soc_dapm_widget *w,
        struct snd_kcontrol *kcontrol, int event)
{
    pr_info("%s : event = %d\n",__FUNCTION__, event);
    switch (event) {
    case SND_SOC_DAPM_PRE_PMU:
        hi6421_set_reg_bit(HI6421_SRCID_I2_EN_REG, HI6421_SRCID_I2_EN_BIT);
        break;
    case SND_SOC_DAPM_POST_PMD:
        hi6421_clr_reg_bit(HI6421_SRCID_I2_EN_REG, HI6421_SRCID_I2_EN_BIT);
        break;
    default :
        pr_warn("%s : power mode event err : %d\n", __FUNCTION__, event);
        break;
    }

    return 0;
}

/*
* MICBIAS
* enable micbias when hs_mic is not plugin
*/
int hi6421_mainmic_power_mode_event(struct snd_soc_dapm_widget *w,
        struct snd_kcontrol *kcontrol, int event)
{
#ifdef CONFIG_MFD_HI6421_IRQ
    if (HI6421_JACK_BIT_HEADSET == hs_status) {
        pr_info("%s : hs_status = %d\n",__FUNCTION__, hs_status);
        return 0;
    }
#endif

    pr_info("%s : event = %d\n",__FUNCTION__, event);
    switch (event) {
    case SND_SOC_DAPM_PRE_PMU:
        hi6421_clr_reg_bit(HI6421_MAINMICBIAS_PD_REG, HI6421_MAINMICBIAS_PD_BIT);
        break;
    case SND_SOC_DAPM_POST_PMD:
        hi6421_set_reg_bit(HI6421_MAINMICBIAS_PD_REG, HI6421_MAINMICBIAS_PD_BIT);
        break;
    default :
        pr_warn("%s : power mode event err : %d\n", __FUNCTION__, event);
        break;
    }

    return 0;
}

static void hi6421_audioldo_enable(bool enable)
{
    if (enable) {
        if (0 == hi6421_audioldo_enabled) {
            regulator_set_optimum_mode(g_regu, AUDIO_NORMAL_MODE);
            msleep(HI6421_PLL_STABLE_WAIT_TIME);
        }
        hi6421_audioldo_enabled++;
    } else {
        hi6421_audioldo_enabled--;
        if (0 == hi6421_audioldo_enabled)
            regulator_set_optimum_mode(g_regu, AUDIO_ECO_MODE);
    }
}

static void hi6421_ibias_enable(bool enable)
{
    if (enable) {
        if (0 == hi6421_ibias_enabled) {
            hi6421_audioldo_enable(true);
            hi6421_clr_reg_bit(HI6421_IBIAS_PD_REG, HI6421_IBIAS_PD_BIT);
        }
        hi6421_ibias_enabled++;
    } else {
        hi6421_ibias_enabled--;
        if (0 == hi6421_ibias_enabled) {
            hi6421_set_reg_bit(HI6421_IBIAS_PD_REG, HI6421_IBIAS_PD_BIT);
            hi6421_audioldo_enable(false);
        }
    }
}

static inline bool hi6421_is_pll_enabled(void)
{
    return (0 == (hi6421_reg_read(g_codec, HI6421_PLL_PD_REG) & (1 << HI6421_PLL_PD_BIT)));
}

static void hi6421_reset_pll(void)
{
    unsigned char reg = hi6421_reg_read(g_codec, HI6421_REG_PLL_RO_CTL);

    if (HI6421_PLL_VALID_VALUE_MIN > reg) {
        pr_info("reset pll\n");
        hi6421_set_reg_bit(HI6421_PLL_PD_REG, HI6421_PLL_PD_BIT);
        mdelay(HI6421_PLL_STABLE_WAIT_TIME);
        hi6421_clr_reg_bit(HI6421_PLL_PD_REG, HI6421_PLL_PD_BIT);
    }
}

void hi6421_check_pll(void)
{
    pr_info("hi6421_check_pll entry\n");
    spin_lock(&pll_lock);
    if (!hi6421_is_pll_enabled()) {
        /* pll is not enabled */
        spin_unlock(&pll_lock);
        return;
    }
    hi6421_reset_pll();
    spin_unlock(&pll_lock);
}
EXPORT_SYMBOL_GPL(hi6421_check_pll);

static void hi6421_pll_work_func(struct work_struct *work)
{
    pr_info("hi6421_pll_work_func\n");

    while(1) {
        spin_lock(&pll_lock);
        if (hi6421_is_pll_enabled()) {
            hi6421_reset_pll();
            spin_unlock(&pll_lock);
        } else {
            spin_unlock(&pll_lock);
            break;
        }
        msleep(HI6421_PLL_DETECT_TIME);
    }

    pr_info("hi6421_pll_work_func exit\n");
}

static void hi6421_pll_enable(bool enable)
{
    if (enable) {
        if (0 == hi6421_pll_enabled) {
            hi6421_audioldo_enable(true);
            hi6421_ibias_enable(true);
            spin_lock(&pll_lock);
            hi6421_clr_reg_bit(HI6421_PLL_PD_REG, HI6421_PLL_PD_BIT);
            spin_unlock(&pll_lock);
        }
        hi6421_pll_enabled++;
    } else {
        hi6421_pll_enabled--;
        if (0 == hi6421_pll_enabled) {
            spin_lock(&pll_lock);
            hi6421_set_reg_bit(HI6421_PLL_PD_REG, HI6421_PLL_PD_BIT);
            spin_unlock(&pll_lock);
            hi6421_ibias_enable(false);
            hi6421_audioldo_enable(false);
        }
    }
}

/* PLL EVENT */
int hi6421_pll_power_mode_event(struct snd_soc_dapm_widget *w,
        struct snd_kcontrol *kcontrol, int event)
{
    /*pr_info("%s : event = %d\n",__FUNCTION__, event);*/
    switch (event) {
    case SND_SOC_DAPM_PRE_PMU:
        hi6421_pll_enable(true);
        break;
    case SND_SOC_DAPM_POST_PMD:
        hi6421_pll_enable(false);
        break;
    default :
        pr_warn("%s : power mode event err : %d\n", __FUNCTION__, event);
        break;
    }

    return 0;
}

/* IBIAS EVENT */
int hi6421_ibias_power_mode_event(struct snd_soc_dapm_widget *w,
        struct snd_kcontrol *kcontrol, int event)
{
    /*pr_info("%s : event = %d\n",__FUNCTION__, event);*/
    switch (event) {
    case SND_SOC_DAPM_PRE_PMU:
        hi6421_ibias_enable(true);
        break;
    case SND_SOC_DAPM_POST_PMD:
        hi6421_ibias_enable(false);
        break;
    default :
        pr_warn("%s : power mode event err : %d\n", __FUNCTION__, event);
        break;
    }

    return 0;
}

/* DAPM WIDGETS */
static const struct snd_soc_dapm_widget hi6421_v200_dapm_widgets[] = {
    /* DIGITAL PART */
    /* INPUT */
    SND_SOC_DAPM_INPUT("AP PLAYBACK"),
    SND_SOC_DAPM_INPUT("BT IN"),
    SND_SOC_DAPM_INPUT("MODEM DOWNLINK"),
    SND_SOC_DAPM_INPUT("DIGITAL MIC"),
    SND_SOC_DAPM_INPUT("DIGMIC"),
    SND_SOC_DAPM_INPUT("ANALOG DAC L"),
    SND_SOC_DAPM_INPUT("ANALOG DAC R"),
    SND_SOC_DAPM_INPUT("ANALOG DAC V"),
    SND_SOC_DAPM_INPUT("LINEINL"),
    SND_SOC_DAPM_INPUT("LINEINR"),
    SND_SOC_DAPM_INPUT("MAINMIC"),
    SND_SOC_DAPM_INPUT("SUBMIC"),
    SND_SOC_DAPM_INPUT("HSMIC"),

    /* OUTPUT */
    SND_SOC_DAPM_OUTPUT("DIGITAL DAC L"),
    SND_SOC_DAPM_OUTPUT("DIGITAL DAC R"),
    SND_SOC_DAPM_OUTPUT("DIGITAL DAC V"),
    SND_SOC_DAPM_OUTPUT("AP CAPTURE L"),
    SND_SOC_DAPM_OUTPUT("AP CAPTURE R"),
    SND_SOC_DAPM_OUTPUT("BT OUT"),
    SND_SOC_DAPM_OUTPUT("MODEM UPLINK"),
    SND_SOC_DAPM_OUTPUT("OUT MONO"),
    SND_SOC_DAPM_OUTPUT("OUT L"),
    SND_SOC_DAPM_OUTPUT("OUT R"),
    SND_SOC_DAPM_OUTPUT("LINEOUT L"),
    SND_SOC_DAPM_OUTPUT("LINEOUT R"),

    /* MICS */
    SND_SOC_DAPM_MIC("MAIN MIC", hi6421_mainmic_power_mode_event),
    SND_SOC_DAPM_MIC("SUB MIC", NULL),
    SND_SOC_DAPM_MIC("HS MIC", NULL),

    /* MICBIAS */
    SND_SOC_DAPM_MICBIAS("SUB MICBIAS",
                         HI6421_SUBMICBIAS_PD_REG, HI6421_SUBMICBIAS_PD_BIT, 1),

    /* SUPPLY */
    SND_SOC_DAPM_SUPPLY("AP CLK",
                        HI6421_AP_INTERFACE_EN_REG, HI6421_AP_INTERFACE_EN_BIT, 0,
                        NULL, 0),
    SND_SOC_DAPM_SUPPLY("BT CLK",
                        HI6421_BT_INTERFACE_EN_REG, HI6421_BT_INTERFACE_EN_BIT, 0,
                        NULL, 0),
    SND_SOC_DAPM_SUPPLY("PLL CLK",
                        SND_SOC_NOPM, 0, 0,
                        hi6421_pll_power_mode_event,
                        SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
    SND_SOC_DAPM_SUPPLY("IBIAS",
                        SND_SOC_NOPM, 0, 0,
                        hi6421_ibias_power_mode_event,
                        SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
    SND_SOC_DAPM_SUPPLY("IMB_EN",
                        HI6421_REG_MICBIAS, HI6421_MICBIAS_MAINMIC_EN_BIT, 0,
                        NULL, 0),

    /* VMID */
    SND_SOC_DAPM_SUPPLY("VMID",
                        HI6421_VMID_PD_REG, HI6421_VMID_PD_BIT, 1,
                        NULL, 0),

    /* PGA */
    SND_SOC_DAPM_PGA("MAINMIC PGA",
                     HI6421_MAINPGA_PD_REG, HI6421_MAINPGA_PD_BIT, 1,
                     NULL, 0),
    SND_SOC_DAPM_PGA("SUBMIC PGA",
                     HI6421_SUBPGA_PD_REG, HI6421_SUBPGA_PD_BIT, 1,
                     NULL, 0),
    SND_SOC_DAPM_PGA("SIDEMIC PGA",
                     HI6421_SIDEPGA_PD_REG, HI6421_SIDEPGA_PD_BIT, 1,
                     NULL, 0),
    SND_SOC_DAPM_PGA("LINEINL PGA",
                     HI6421_LINEINLPGA_PD_REG, HI6421_LINEINLPGA_PD_BIT, 1,
                     NULL, 0),
    SND_SOC_DAPM_PGA("LINEINR PGA",
                     HI6421_LINEINRPGA_PD_REG, HI6421_LINEINRPGA_PD_BIT, 1,
                     NULL, 0),
    SND_SOC_DAPM_PGA("MODEM_UL GAIN",
                     HI6421_MODEM_UL_GAIN_EN_REG, HI6421_MODEM_UL_GAIN_EN_BIT, 0,
                     NULL, 0),
    SND_SOC_DAPM_PGA("MODEM_DL GAIN",
                     HI6421_MODEM_DL_GAIN_EN_REG, HI6421_MODEM_DL_GAIN_EN_BIT, 0,
                     NULL, 0),

    /* VIRTUAL PGA */
    SND_SOC_DAPM_PGA("DMNS",
                     HI6421_DMNS_EN_REG, HI6421_DMNS_EN_BIT, 0,
                     NULL, 0),
    SND_SOC_DAPM_PGA("HBFVD",
                     HI6421_HBFVD_EN_REG, HI6421_HBFVD_EN_BIT, 0,
                     NULL, 0),
    SND_SOC_DAPM_PGA("HBFVI",
                     HI6421_HBFVI_EN_REG, HI6421_HBFVI_EN_BIT, 0,
                     NULL, 0),
    SND_SOC_DAPM_PGA("ADC FILTER L",
                     HI6421_ADCL_EN_REG, HI6421_ADCL_EN_BIT, 0,
                     NULL, 0),
    SND_SOC_DAPM_PGA("ADC FILTER R",
                     HI6421_ADCR_EN_REG, HI6421_ADCR_EN_BIT, 0,
                     NULL, 0),
    SND_SOC_DAPM_PGA("DAC L",
                     HI6421_DACL_PD_REG, HI6421_DACL_PD_BIT, 1,
                     NULL, 0),
    SND_SOC_DAPM_PGA("DAC R",
                     HI6421_DACR_PD_REG, HI6421_DACR_PD_BIT, 1,
                     NULL, 0),
    SND_SOC_DAPM_PGA("DAC V",
                     HI6421_DACV_PD_REG, HI6421_DACV_PD_BIT, 1,
                     NULL, 0),
    SND_SOC_DAPM_PGA("ADC L",
                     HI6421_ADCL_PD_REG, HI6421_ADCL_PD_BIT, 1,
                     NULL, 0),
    SND_SOC_DAPM_PGA("ADC R",
                     HI6421_ADCR_PD_REG, HI6421_ADCR_PD_BIT, 1,
                     NULL, 0),

    /* PGA EVENT */
    SND_SOC_DAPM_PGA_E("DIG PLAYBACK",
                       SND_SOC_NOPM, 0, 0,
                       NULL, 0,
                       hi6421_digplayback_power_mode_event,
                       SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
    SND_SOC_DAPM_PGA_E("CAPTURE AGC PGA L",
                       SND_SOC_NOPM, 0, 0,
                       NULL, 0,
                       hi6421_captureagcl_power_mode_event,
                       SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
    SND_SOC_DAPM_PGA_E("CAPTURE AGC PGA R",
                       SND_SOC_NOPM, 0, 0,
                       NULL, 0,
                       hi6421_captureagcr_power_mode_event,
                       SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

    /* MUX */
    SND_SOC_DAPM_MUX("PLAYBACK MUX",
                     SND_SOC_NOPM, 0, 0,
                     &hi6421_dapm_playback_mux_controls),
    SND_SOC_DAPM_MUX("AGCL MUX",
                     SND_SOC_NOPM, 0, 0,
                     &hi6421_dapm_agc_c_l_mux_controls),
    SND_SOC_DAPM_MUX("AGCR MUX",
                     SND_SOC_NOPM, 0, 0,
                     &hi6421_dapm_agc_c_r_mux_controls),
    SND_SOC_DAPM_MUX("ADCL MUX",
                     SND_SOC_NOPM, 0, 0,
                     &hi6421_dapm_adcl_mux_controls),
    SND_SOC_DAPM_MUX("ADCR MUX",
                     SND_SOC_NOPM, 0, 0,
                     &hi6421_dapm_adcr_mux_controls),
    SND_SOC_DAPM_MUX("DMNS MUX",
                     SND_SOC_NOPM, 0, 0,
                     &hi6421_dapm_dmns_mux_controls),
    SND_SOC_DAPM_MUX("CAPTURE MUX",
                     SND_SOC_NOPM, 0, 0,
                     &hi6421_dapm_capture_mux_controls),
    SND_SOC_DAPM_MUX("MODEM MUX",
                     SND_SOC_NOPM, 0, 0,
                     &hi6421_dapm_modem_mux_controls),
    SND_SOC_DAPM_MUX("BT MUX V200",
                     SND_SOC_NOPM, 0, 0,
                     &hi6421_v200_dapm_bt_mux_controls),
    SND_SOC_DAPM_MUX("MIC MUX",
                     SND_SOC_NOPM, 0, 0,
                     &hi6421_dapm_mic_mux_controls),

    /* SWITCH */
    SND_SOC_DAPM_SWITCH("DACL SWITCH",
                        SND_SOC_NOPM, 0, 0,
                        &hi6421_dapm_dacl_switch_controls),
    SND_SOC_DAPM_SWITCH("DACR SWITCH",
                        SND_SOC_NOPM, 0, 0,
                        &hi6421_dapm_dacr_switch_controls),
    SND_SOC_DAPM_SWITCH("DACV SWITCH",
                        SND_SOC_NOPM, 0, 0,
                        &hi6421_dapm_dacv_switch_controls),
    SND_SOC_DAPM_SWITCH("DIGMIC SWITCH",
                        SND_SOC_NOPM, 0, 0,
                        &hi6421_dapm_digmic_switch_controls),
    SND_SOC_DAPM_SWITCH("MODEM_UL SWITCH",
                        SND_SOC_NOPM, 0, 0,
                        &hi6421_dapm_modemul_switch_controls),
    SND_SOC_DAPM_SWITCH_E("MODEM_DL SWITCH",
                          SND_SOC_NOPM, 0, 0,
                          &hi6421_dapm_modemdl_switch_controls,
                          hi6421_modem_power_mode_event,
                          SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

    /* SWITCH EVENT */
    SND_SOC_DAPM_SWITCH_E("SRCIU SWITCH",
                          SND_SOC_NOPM, 0, 0,
                          &hi6421_srciu_switch_controls,
                          hi6421_srciu_power_mode_event,
                          SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
    SND_SOC_DAPM_SWITCH_E("SRCID SWITCH",
                          SND_SOC_NOPM, 0, 0,
                          &hi6421_srcid_switch_controls,
                          hi6421_srcid_power_mode_event,
                          SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

    /* MIXERS */
    SND_SOC_DAPM_MIXER("MIXER MONO",
                        HI6421_MIXERMONO_PD_REG,
                        HI6421_MIXERMONO_PD_BIT,
                        1, /* INVERT */
                        hi6421_mixermono_controls,
                        ARRAY_SIZE(hi6421_mixermono_controls)),
    SND_SOC_DAPM_MIXER( "MIXER OUT LEFT",
                        HI6421_MIXEROUTL_PD_REG,
                        HI6421_MIXEROUTL_PD_BIT,
                        1, /* INVERT */
                        hi6421_mixeroutleft_controls,
                        ARRAY_SIZE(hi6421_mixeroutleft_controls)),
    SND_SOC_DAPM_MIXER( "MIXER OUT RIGHT",
                        HI6421_MIXEROUTR_PD_REG,
                        HI6421_MIXEROUTR_PD_BIT,
                        1, /* INVERT */
                        hi6421_mixeroutright_controls,
                        ARRAY_SIZE(hi6421_mixeroutright_controls)),
    SND_SOC_DAPM_MIXER( "MIXER IN LEFT",
                        HI6421_MIXERINL_PD_REG,
                        HI6421_MIXERINL_PD_BIT,
                        1, /* INVERT */
                        hi6421_mixerinleft_controls,
                        ARRAY_SIZE(hi6421_mixerinleft_controls)),
    SND_SOC_DAPM_MIXER( "MIXER IN RIGHT",
                        HI6421_MIXERINR_PD_REG,
                        HI6421_MIXERINR_PD_BIT,
                        1, /* INVERT */
                        hi6421_mixerinright_controls,
                        ARRAY_SIZE(hi6421_mixerinright_controls)),
    SND_SOC_DAPM_MIXER( "MIXER LINEOUT LEFT",
                        HI6421_MIXERLINEOUTL_PD_REG,
                        HI6421_MIXERLINEOUTL_PD_BIT,
                        1, /* INVERT */
                        hi6421_mixerlineoutleft_controls,
                        ARRAY_SIZE(hi6421_mixerlineoutleft_controls)),
    SND_SOC_DAPM_MIXER( "MIXER LINEOUT RIGHT",
                        HI6421_MIXERLINEOUTR_PD_REG,
                        HI6421_MIXERLINEOUTR_PD_BIT,
                        1, /* INVERT */
                        hi6421_mixerlineoutright_controls,
                        ARRAY_SIZE(hi6421_mixerlineoutright_controls)),

    /* PA SWITCH */
    SND_SOC_DAPM_SWITCH("SPEAKER SWITCH",
                        SND_SOC_NOPM, 0, 0,
                        &hi6421_dapm_speaker_switch_controls),
    SND_SOC_DAPM_SWITCH("EARPIECE SWITCH",
                        SND_SOC_NOPM, 0, 0,
                        &hi6421_dapm_earpiece_switch_controls),
    SND_SOC_DAPM_SWITCH("HEADSET_L SWITCH",
                        SND_SOC_NOPM, 0, 0,
                        &hi6421_dapm_headsetl_switch_controls),
    SND_SOC_DAPM_SWITCH("HEADSET_R SWITCH",
                        SND_SOC_NOPM, 0, 0,
                        &hi6421_dapm_headsetr_switch_controls),
    SND_SOC_DAPM_SWITCH("LINEOUT_L SWITCH",
                        SND_SOC_NOPM, 0, 0,
                        &hi6421_dapm_lineoutl_switch_controls),
    SND_SOC_DAPM_SWITCH("LINEOUT_R SWITCH",
                        SND_SOC_NOPM, 0, 0,
                        &hi6421_dapm_lineoutr_switch_controls),

    SND_SOC_DAPM_INPUT("MONO MIXER"),
    SND_SOC_DAPM_INPUT("OUTL MIXER"),
    SND_SOC_DAPM_INPUT("OUTR MIXER"),
    SND_SOC_DAPM_INPUT("LINEOUTL MIXER"),
    SND_SOC_DAPM_INPUT("LINEOUTR MIXER"),

    SND_SOC_DAPM_OUTPUT("EARPIECE"),
    SND_SOC_DAPM_OUTPUT("SPEAKER"),
    SND_SOC_DAPM_OUTPUT("HEADSETL"),
    SND_SOC_DAPM_OUTPUT("HEADSETR"),
    SND_SOC_DAPM_OUTPUT("LINEOUTL"),
    SND_SOC_DAPM_OUTPUT("LINEOUTR"),
};


static const struct snd_soc_dapm_widget hi6421_v220_dapm_widgets[] = {
    /* DIGITAL PART */
    /* INPUT */
    SND_SOC_DAPM_INPUT("AP PLAYBACK L"),
    SND_SOC_DAPM_INPUT("AP PLAYBACK R"),
    SND_SOC_DAPM_INPUT("BT IN"),
    SND_SOC_DAPM_INPUT("MODEM DOWNLINK"),
    SND_SOC_DAPM_INPUT("DIGITAL MIC"),
    SND_SOC_DAPM_INPUT("DIGMIC"),
    SND_SOC_DAPM_INPUT("ANALOG DAC L"),
    SND_SOC_DAPM_INPUT("ANALOG DAC R"),
    SND_SOC_DAPM_INPUT("ANALOG DAC V"),
    SND_SOC_DAPM_INPUT("LINEINL"),
    SND_SOC_DAPM_INPUT("LINEINR"),
    SND_SOC_DAPM_INPUT("MAINMIC"),
    SND_SOC_DAPM_INPUT("SUBMIC"),
    SND_SOC_DAPM_INPUT("HSMIC"),

    /* OUTPUT */
    SND_SOC_DAPM_OUTPUT("DIGITAL DAC L"),
    SND_SOC_DAPM_OUTPUT("DIGITAL DAC R"),
    SND_SOC_DAPM_OUTPUT("DIGITAL DAC V"),
    SND_SOC_DAPM_OUTPUT("AP CAPTURE L"),
    SND_SOC_DAPM_OUTPUT("AP CAPTURE R"),
    SND_SOC_DAPM_OUTPUT("BT OUT"),
    SND_SOC_DAPM_OUTPUT("MODEM UPLINK L"),
    SND_SOC_DAPM_OUTPUT("MODEM UPLINK R"),
    SND_SOC_DAPM_OUTPUT("OUT MONO"),
    SND_SOC_DAPM_OUTPUT("OUT L"),
    SND_SOC_DAPM_OUTPUT("OUT R"),
    SND_SOC_DAPM_OUTPUT("LINEOUT L"),
    SND_SOC_DAPM_OUTPUT("LINEOUT R"),

    /* MICS */
    SND_SOC_DAPM_MIC("MAIN MIC", hi6421_mainmic_power_mode_event),
    SND_SOC_DAPM_MIC("SUB MIC", NULL),
    SND_SOC_DAPM_MIC("HS MIC", NULL),

    /* MICBIAS */
    SND_SOC_DAPM_MICBIAS("SUB MICBIAS", HI6421_SUBMICBIAS_PD_REG,
                         HI6421_SUBMICBIAS_PD_BIT, 1),

    /* SUPPLY */
    SND_SOC_DAPM_SUPPLY("AP CLK",
                        HI6421_AP_INTERFACE_EN_REG, HI6421_AP_INTERFACE_EN_BIT, 0,
                        NULL, 0),
    SND_SOC_DAPM_SUPPLY("BT CLK",
                        HI6421_BT_INTERFACE_EN_REG, HI6421_BT_INTERFACE_EN_BIT, 0,
                        NULL, 0),
    SND_SOC_DAPM_SUPPLY("PLL CLK",
                        SND_SOC_NOPM, 0, 0,
                        hi6421_pll_power_mode_event,
                        SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
    SND_SOC_DAPM_SUPPLY("IBIAS",
                        SND_SOC_NOPM, 0, 0,
                        hi6421_ibias_power_mode_event,
                        SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
    SND_SOC_DAPM_SUPPLY("IMB_EN",
                        HI6421_REG_MICBIAS, HI6421_MICBIAS_MAINMIC_EN_BIT, 0,
                        NULL, 0),

    /* VMID */
    SND_SOC_DAPM_SUPPLY("VMID",
                        HI6421_VMID_PD_REG, HI6421_VMID_PD_BIT, 1,
                        NULL, 0),

    /* PGA */
    SND_SOC_DAPM_PGA("MAINMIC PGA",
                     HI6421_MAINPGA_PD_REG, HI6421_MAINPGA_PD_BIT, 1,
                     NULL, 0),
    SND_SOC_DAPM_PGA("SUBMIC PGA",
                     HI6421_SUBPGA_PD_REG, HI6421_SUBPGA_PD_BIT, 1,
                     NULL, 0),
    SND_SOC_DAPM_PGA("SIDETONE PGA",
                     HI6421_SIDEPGA_PD_REG, HI6421_SIDEPGA_PD_BIT, 1,
                     NULL, 0),
    SND_SOC_DAPM_PGA("LINEINL PGA",
                     HI6421_LINEINLPGA_PD_REG, HI6421_LINEINLPGA_PD_BIT, 1,
                     NULL, 0),
    SND_SOC_DAPM_PGA("LINEINR PGA",
                     HI6421_LINEINRPGA_PD_REG, HI6421_LINEINRPGA_PD_BIT, 1,
                     NULL, 0),
    SND_SOC_DAPM_PGA("PLAYBACK AGC PGA L",
                     HI6421_V220_AGC_P_L_EN_REG, HI6421_V220_AGC_P_L_EN_BIT, 0,
                     NULL, 0),
    SND_SOC_DAPM_PGA("PLAYBACK AGC PGA R",
                     HI6421_V220_AGC_P_R_EN_REG, HI6421_V220_AGC_P_R_EN_BIT, 0,
                     NULL, 0),

    /* VIRTUAL PGA */
    SND_SOC_DAPM_PGA("DMNS",
                     HI6421_DMNS_EN_REG, HI6421_DMNS_EN_BIT, 0,
                     NULL, 0),
    SND_SOC_DAPM_PGA("MODEM_UL_L HBFVD",
                     HI6421_MODEM_UL_L_HBFVD_EN_REG, HI6421_MODEM_UL_L_HBFVD_EN_BIT, 0,
                     NULL, 0),
    SND_SOC_DAPM_PGA("MODEM_UL_R HBFVD",
                     HI6421_MODEM_UL_R_HBFVD_EN_REG, HI6421_MODEM_UL_R_HBFVD_EN_BIT, 0,
                     NULL, 0),
    SND_SOC_DAPM_PGA("MODEM_DL HBFVI",
                     HI6421_HBFVI_EN_REG, HI6421_HBFVI_EN_BIT, 0,
                     NULL, 0),
    SND_SOC_DAPM_PGA("MODEM_DL EQ",
                     HI6421_MODEM_DL_EQ_EN_REG, HI6421_MODEM_DL_EQ_EN_BIT, 0,
                     NULL, 0),
    SND_SOC_DAPM_PGA("ADC FILTER L",
                     HI6421_ADCL_EN_REG, HI6421_ADCL_EN_BIT, 0,
                     NULL, 0),
    SND_SOC_DAPM_PGA("ADC FILTER R",
                     HI6421_ADCR_EN_REG, HI6421_ADCR_EN_BIT, 0,
                     NULL, 0),
    SND_SOC_DAPM_PGA("DAC L",
                     HI6421_DACL_PD_REG, HI6421_DACL_PD_BIT, 1,
                     NULL, 0),
    SND_SOC_DAPM_PGA("DAC R",
                     HI6421_DACR_PD_REG, HI6421_DACR_PD_BIT, 1,
                     NULL, 0),
    SND_SOC_DAPM_PGA("DAC V",
                     HI6421_DACV_PD_REG, HI6421_DACV_PD_BIT, 1,
                     NULL, 0),
    SND_SOC_DAPM_PGA("ADC L",
                     HI6421_ADCL_PD_REG, HI6421_ADCL_PD_BIT, 1,
                     NULL, 0),
    SND_SOC_DAPM_PGA("ADC R",
                     HI6421_ADCR_PD_REG, HI6421_ADCR_PD_BIT, 1,
                     NULL, 0),

    /* PGA EVENT */
    SND_SOC_DAPM_PGA_E("DIG HARMONIC",
                       SND_SOC_NOPM, 0, 0,
                       NULL, 0,
                       hi6421_digharmonic_power_mode_event,
                       SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
    SND_SOC_DAPM_PGA_E("CAPTURE AGC PGA L",
                       SND_SOC_NOPM, 0, 0,
                       NULL, 0,
                       hi6421_captureagcl_power_mode_event,
                       SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
    SND_SOC_DAPM_PGA_E("CAPTURE AGC PGA R",
                       SND_SOC_NOPM, 0, 0,
                       NULL, 0,
                       hi6421_captureagcr_power_mode_event,
                       SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

    /* MUX */
    SND_SOC_DAPM_MUX("PLAYBACK MUX",
                     SND_SOC_NOPM, 0, 0,
                     &hi6421_dapm_playback_mux_controls),
    SND_SOC_DAPM_MUX("AGCL MUX",
                     SND_SOC_NOPM, 0, 0,
                     &hi6421_dapm_agc_c_l_mux_controls),
    SND_SOC_DAPM_MUX("AGCR MUX",
                     SND_SOC_NOPM, 0, 0,
                     &hi6421_dapm_agc_c_r_mux_controls),
    SND_SOC_DAPM_MUX("ADCL MUX",
                     SND_SOC_NOPM, 0, 0,
                     &hi6421_dapm_adcl_mux_controls),
    SND_SOC_DAPM_MUX("ADCR MUX",
                     SND_SOC_NOPM, 0, 0,
                     &hi6421_dapm_adcr_mux_controls),
    SND_SOC_DAPM_MUX("DMNS MUX",
                     SND_SOC_NOPM, 0, 0,
                     &hi6421_dapm_dmns_mux_controls),
    SND_SOC_DAPM_MUX("CAPTURE MUX",
                     SND_SOC_NOPM, 0, 0,
                     &hi6421_dapm_capture_mux_controls),
    SND_SOC_DAPM_MUX("MODEM MUX",
                     SND_SOC_NOPM, 0, 0,
                     &hi6421_dapm_modem_mux_controls),
    SND_SOC_DAPM_MUX("MIC MUX",
                     SND_SOC_NOPM, 0, 0,
                     &hi6421_dapm_mic_mux_controls),

    /* SWITCH */
    SND_SOC_DAPM_SWITCH("DACL SWITCH",
                        SND_SOC_NOPM, 0, 0,
                        &hi6421_dapm_dacl_switch_controls),
    SND_SOC_DAPM_SWITCH("DACR SWITCH",
                        SND_SOC_NOPM, 0, 0,
                        &hi6421_dapm_dacr_switch_controls),
    SND_SOC_DAPM_SWITCH("DACV SWITCH",
                        SND_SOC_NOPM, 0, 0,
                        &hi6421_dapm_dacv_switch_controls),
    SND_SOC_DAPM_SWITCH("DIGMIC SWITCH",
                        SND_SOC_NOPM, 0, 0,
                        &hi6421_dapm_digmic_switch_controls),
    SND_SOC_DAPM_SWITCH_E("MODEM_DL SWITCH",
                          SND_SOC_NOPM, 0, 0,
                          &hi6421_dapm_modemdl_switch_controls,
                          hi6421_modem_power_mode_event,
                          SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
    SND_SOC_DAPM_SWITCH("MODEM_UL_L SWITCH",
                        SND_SOC_NOPM, 0, 0,
                        &hi6421_dapm_modemull_switch_controls),
    SND_SOC_DAPM_SWITCH("MODEM_UL_R SWITCH",
                        SND_SOC_NOPM, 0, 0,
                        &hi6421_dapm_modemulr_switch_controls),
    SND_SOC_DAPM_SWITCH("BT_INCALL SWITCH",
                        SND_SOC_NOPM, 0, 0,
                        &hi6421_dapm_bt_incall_switch_controls),
    SND_SOC_DAPM_SWITCH("BT_PLAYBACK SWITCH",
                        SND_SOC_NOPM, 0, 0,
                        &hi6421_dapm_bt_playback_switch_controls),

    /* SWITCH EVENT */
    SND_SOC_DAPM_SWITCH_E("SRCIU SWITCH",
                          SND_SOC_NOPM, 0, 0,
                          &hi6421_srciu_switch_controls,
                          hi6421_srciu_power_mode_event,
                          SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
    SND_SOC_DAPM_SWITCH_E("SRCID SWITCH",
                          SND_SOC_NOPM, 0, 0,
                          &hi6421_srcid_switch_controls,
                          hi6421_srcid_power_mode_event,
                          SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

    /* MIXERS */
    SND_SOC_DAPM_MIXER("MIXER MONO",
                        HI6421_MIXERMONO_PD_REG,
                        HI6421_MIXERMONO_PD_BIT,
                        1, /* INVERT */
                        hi6421_mixermono_controls,
                        ARRAY_SIZE(hi6421_mixermono_controls)),
    SND_SOC_DAPM_MIXER( "MIXER OUT LEFT",
                        HI6421_MIXEROUTL_PD_REG,
                        HI6421_MIXEROUTL_PD_BIT,
                        1, /* INVERT */
                        hi6421_mixeroutleft_controls,
                        ARRAY_SIZE(hi6421_mixeroutleft_controls)),
    SND_SOC_DAPM_MIXER( "MIXER OUT RIGHT",
                        HI6421_MIXEROUTR_PD_REG,
                        HI6421_MIXEROUTR_PD_BIT,
                        1, /* INVERT */
                        hi6421_mixeroutright_controls,
                        ARRAY_SIZE(hi6421_mixeroutright_controls)),
    SND_SOC_DAPM_MIXER( "MIXER IN LEFT",
                        HI6421_MIXERINL_PD_REG,
                        HI6421_MIXERINL_PD_BIT,
                        1, /* INVERT */
                        hi6421_mixerinleft_controls,
                        ARRAY_SIZE(hi6421_mixerinleft_controls)),
    SND_SOC_DAPM_MIXER( "MIXER IN RIGHT",
                        HI6421_MIXERINR_PD_REG,
                        HI6421_MIXERINR_PD_BIT,
                        1, /* INVERT */
                        hi6421_mixerinright_controls,
                        ARRAY_SIZE(hi6421_mixerinright_controls)),
    SND_SOC_DAPM_MIXER( "MIXER LINEOUT LEFT",
                        HI6421_MIXERLINEOUTL_PD_REG,
                        HI6421_MIXERLINEOUTL_PD_BIT,
                        1, /* INVERT */
                        hi6421_mixerlineoutleft_controls,
                        ARRAY_SIZE(hi6421_mixerlineoutleft_controls)),
    SND_SOC_DAPM_MIXER( "MIXER LINEOUT RIGHT",
                        HI6421_MIXERLINEOUTR_PD_REG,
                        HI6421_MIXERLINEOUTR_PD_BIT,
                        1, /* INVERT */
                        hi6421_mixerlineoutright_controls,
                        ARRAY_SIZE(hi6421_mixerlineoutright_controls)),

    /* PA SWITCH */
    SND_SOC_DAPM_SWITCH("SPEAKER SWITCH",
                        SND_SOC_NOPM, 0, 0,
                        &hi6421_dapm_speaker_switch_controls),
    SND_SOC_DAPM_SWITCH("EARPIECE SWITCH",
                        SND_SOC_NOPM, 0, 0,
                        &hi6421_dapm_earpiece_switch_controls),
    SND_SOC_DAPM_SWITCH("HEADSET_L SWITCH",
                        SND_SOC_NOPM, 0, 0,
                        &hi6421_dapm_headsetl_switch_controls),
    SND_SOC_DAPM_SWITCH("HEADSET_R SWITCH",
                        SND_SOC_NOPM, 0, 0,
                        &hi6421_dapm_headsetr_switch_controls),
    SND_SOC_DAPM_SWITCH("LINEOUT_L SWITCH",
                        SND_SOC_NOPM, 0, 0,
                        &hi6421_dapm_lineoutl_switch_controls),
    SND_SOC_DAPM_SWITCH("LINEOUT_R SWITCH",
                        SND_SOC_NOPM, 0, 0,
                        &hi6421_dapm_lineoutr_switch_controls),

    SND_SOC_DAPM_INPUT("MONO MIXER"),
    SND_SOC_DAPM_INPUT("OUTL MIXER"),
    SND_SOC_DAPM_INPUT("OUTR MIXER"),
    SND_SOC_DAPM_INPUT("LINEOUTL MIXER"),
    SND_SOC_DAPM_INPUT("LINEOUTR MIXER"),

    SND_SOC_DAPM_OUTPUT("EARPIECE"),
    SND_SOC_DAPM_OUTPUT("SPEAKER"),
    SND_SOC_DAPM_OUTPUT("HEADSETL"),
    SND_SOC_DAPM_OUTPUT("HEADSETR"),
    SND_SOC_DAPM_OUTPUT("LINEOUTL"),
    SND_SOC_DAPM_OUTPUT("LINEOUTR"),
};

static const struct snd_soc_dapm_route route_map_v200[] = {
    /* AUDIO PATH - DIGITAL */

    /* PLAYBACK */
    {"DIG PLAYBACK",        NULL,                   "AP PLAYBACK"},
    {"PLAYBACK MUX",        "INPUT LEFT",           "DIG PLAYBACK"},
    {"PLAYBACK MUX",        "INPUT LR MIX",         "DIG PLAYBACK"},
    {"DACL SWITCH",         "DACL Switch",          "PLAYBACK MUX"},
    {"DACR SWITCH",         "DACR Switch",          "DIG PLAYBACK"},
    {"DIGITAL DAC L",       NULL,                   "DACL SWITCH"},
    {"DIGITAL DAC R",       NULL,                   "DACR SWITCH"},

    /* MODEM UL/DL */
    {"MODEM MUX",           "VOICE BT",             "BT IN"},
    {"MODEM MUX",           "VOICE MIC",            "HBFVD"},

    {"HBFVD",               NULL,                   "DMNS MUX"},
    {"MODEM_UL GAIN",       NULL,                   "MODEM MUX"},
    {"MODEM_UL SWITCH",     "Modem_UL Switch",      "MODEM_UL GAIN"},
    {"MODEM UPLINK",        NULL,                   "MODEM_UL SWITCH"},

    {"MODEM_DL SWITCH",     "Modem_DL Switch",      "MODEM DOWNLINK"},
    {"MODEM_DL GAIN",       NULL,                   "MODEM_DL SWITCH"},
    {"HBFVI",               NULL,                   "MODEM_DL GAIN"},
    {"DACV SWITCH",         "DACV Switch",          "HBFVI"},
    {"DIGITAL DAC V",       NULL,                   "DACV SWITCH"},

    /* BT */
    {"BT MUX V200",         "BT AGCL",              "CAPTURE MUX"},
    {"BT MUX V200",         "BT PCM",               "MODEM_UL SWITCH"},
    {"BT MUX V200",         "BT VOICE",             "MODEM_DL GAIN"},

    {"BT OUT",              NULL,                   "BT MUX V200"},
    {"BT OUT",              NULL,                   "CAPTURE AGC PGA R"},

    /* DIGITAL MIC */
    {"DIGMIC SWITCH",       "Digmic Switch",        "DIGMIC"},

    /* ADC L */
    {"ADCL MUX",            "ADC ADC",              "ADC L"},
    {"ADCL MUX",            "ADC DIGMIC",           "DIGMIC SWITCH"},
    {"ADCL MUX",            "ADC DACL",             "DACL SWITCH"},
    {"ADCL MUX",            "ADC DACR",             "DACR SWITCH"},
    {"ADCL MUX",            "ADC DACV",             "DACV SWITCH"},
    {"ADC FILTER L",        NULL,                   "ADCL MUX"},
    /* ADC R */
    {"ADCR MUX",            "ADC ADC",              "ADC R"},
    {"ADCR MUX",            "ADC DIGMIC",           "DIGMIC SWITCH"},
    {"ADCR MUX",            "ADC DACL",             "DACL SWITCH"},
    {"ADCR MUX",            "ADC DACR",             "DACR SWITCH"},
    {"ADCR MUX",            "ADC DACV",             "DACV SWITCH"},
    {"ADC FILTER R",        NULL,                   "ADCR MUX"},

    /* DMNS */
    {"DMNS",                NULL,                   "ADC FILTER L"},
    {"DMNS",                NULL,                   "ADC FILTER R"},
    {"DMNS MUX",            "DISABLE",              "ADC FILTER L"},
    {"DMNS MUX",            "ENABLE",               "DMNS"},

    /* CAPTURE AGC PGA L */
    {"AGCL MUX",            "AGCL ADCL",            "DMNS MUX"},
    {"AGCL MUX",            "AGCL VOICEU",          "SRCIU SWITCH"},
    {"CAPTURE AGC PGA L",   NULL,                   "AGCL MUX"},
    {"AP CAPTURE L",        NULL,                   "CAPTURE AGC PGA L"},
    /* CAPTURE AGC PGA R */
    {"AGCR MUX",            "AGCR ADCR",            "ADC FILTER R"},
    {"AGCR MUX",            "AGCR VOICED",          "SRCID SWITCH"},
    {"CAPTURE AGC PGA R",   NULL,                   "AGCR MUX"},
    {"AP CAPTURE R",        NULL,                   "CAPTURE AGC PGA R"},

    /* INCALL RECORD */
    {"SRCIU SWITCH",        "Srciu Switch",         "MODEM_UL SWITCH"},
    {"SRCID SWITCH",        "Srcid Switch",         "HBFVI"},

    /* CLOCK SUPPLY */
    {"AP PLAYBACK",         NULL,                   "AP CLK"},
    {"CAPTURE AGC PGA L",   NULL,                   "AP CLK"},
    {"CAPTURE AGC PGA R",   NULL,                   "AP CLK"},
    {"BT OUT",              NULL,                   "BT CLK"},
    {"BT IN",               NULL,                   "BT CLK"},
    {"AP CLK",              NULL,                   "PLL CLK"},
    {"BT CLK",              NULL,                   "PLL CLK"},
    {"MODEM_DL SWITCH",     NULL,                   "PLL CLK"},
    {"PLL CLK",             NULL,                   "IBIAS"},

    /* DAC */
    {"DAC L",               NULL,                   "ANALOG DAC L"},
    {"DAC R",               NULL,                   "ANALOG DAC R"},
    {"DAC V",               NULL,                   "ANALOG DAC V"},

    /* MIXER */
    {"MIXER MONO",          "DAC Left Switch",      "DAC L"},
    {"MIXER MONO",          "DAC Right Switch",     "DAC R"},
    {"MIXER MONO",          "DAC Voice Switch",     "DAC V"},
    {"MIXER MONO",          "Linein Left Switch",   "LINEINL PGA"},
    {"MIXER MONO",          "Linein Right Switch",  "LINEINR PGA"},
    {"MIXER MONO",          "Main PGA Switch",      "MAINMIC PGA"},
    {"MIXER MONO",          "Side PGA Switch",      "SIDEMIC PGA"},

    {"MIXER OUT LEFT",      "DAC Left Switch",      "DAC L"},
    {"MIXER OUT LEFT",      "DAC Right Switch",     "DAC R"},
    {"MIXER OUT LEFT",      "DAC Voice Switch",     "DAC V"},
    {"MIXER OUT LEFT",      "Linein Left Switch",   "LINEINL PGA"},
    {"MIXER OUT LEFT",      "Linein Right Switch",  "LINEINR PGA"},
    {"MIXER OUT LEFT",      "Main PGA Switch",      "MAINMIC PGA"},
    {"MIXER OUT LEFT",      "Side PGA Switch",      "SIDEMIC PGA"},

    {"MIXER OUT RIGHT",     "DAC Left Switch",      "DAC L"},
    {"MIXER OUT RIGHT",     "DAC Right Switch",     "DAC R"},
    {"MIXER OUT RIGHT",     "DAC Voice Switch",     "DAC V"},
    {"MIXER OUT RIGHT",     "Linein Left Switch",   "LINEINL PGA"},
    {"MIXER OUT RIGHT",     "Linein Right Switch",  "LINEINR PGA"},
    {"MIXER OUT RIGHT",     "Main PGA Switch",      "MAINMIC PGA"},
    {"MIXER OUT RIGHT",     "Side PGA Switch",      "SIDEMIC PGA"},

    {"MIXER IN LEFT",       "DAC Left Switch",      "DAC L"},
    {"MIXER IN LEFT",       "DAC Right Switch",     "DAC R"},
    {"MIXER IN LEFT",       "DAC Voice Switch",     "DAC V"},
    {"MIXER IN LEFT",       "Linein Left Switch",   "LINEINL PGA"},
    {"MIXER IN LEFT",       "Linein Right Switch",  "LINEINR PGA"},
    {"MIXER IN LEFT",       "Main PGA Switch",      "MAINMIC PGA"},
    {"MIXER IN LEFT",       "Sub PGA Switch",       "SUBMIC PGA" },

    {"MIXER IN RIGHT",      "DAC Left Switch",      "DAC L"},
    {"MIXER IN RIGHT",      "DAC Right Switch",     "DAC R"},
    {"MIXER IN RIGHT",      "DAC Voice Switch",     "DAC V"},
    {"MIXER IN RIGHT",      "Linein Left Switch",   "LINEINL PGA"},
    {"MIXER IN RIGHT",      "Linein Right Switch",  "LINEINR PGA"},
    {"MIXER IN RIGHT",      "Main PGA Switch",      "MAINMIC PGA"},
    {"MIXER IN RIGHT",      "Sub PGA Switch",       "SUBMIC PGA" },

    {"MIXER LINEOUT LEFT",  "Mixer Left Switch",    "MIXER OUT LEFT"},
    {"MIXER LINEOUT LEFT",  "Mixer Right Switch",   "MIXER OUT RIGHT"},
    {"MIXER LINEOUT LEFT",  "Mixer Mono Switch",    "MIXER MONO"},

    {"MIXER LINEOUT RIGHT", "Mixer Left Switch",    "MIXER OUT LEFT"},
    {"MIXER LINEOUT RIGHT", "Mixer Right Switch",   "MIXER OUT RIGHT"},
    {"MIXER LINEOUT RIGHT", "Mixer Mono Switch",    "MIXER MONO"},

    /* OUTPUT */
    {"OUT MONO",            NULL,                   "MIXER MONO"},
    {"OUT L",               NULL,                   "MIXER OUT LEFT"},
    {"OUT R",               NULL,                   "MIXER OUT RIGHT"},
    {"LINEOUT L",           NULL,                   "MIXER LINEOUT LEFT"},
    {"LINEOUT R",           NULL,                   "MIXER LINEOUT RIGHT"},
    {"ADC L",               NULL,                   "MIXER IN LEFT"},
    {"ADC R",               NULL,                   "MIXER IN RIGHT"},

    /* main mic need to poweron main micbias */
    {"MAINMIC",             NULL,                   "MAIN MIC"},
    /* headset always poweron micbias */
    {"HSMIC",               NULL,                   "HS MIC"},

    {"SUB MICBIAS",         NULL,                   "SUB MIC"},
    {"SUBMIC",              NULL,                   "SUB MICBIAS"},

    /* MIC */
    {"MAINMIC",             NULL,                   "IMB_EN"},
    {"MIC MUX",             "MIC MAIN",             "MAINMIC"},
    {"MIC MUX",             "MIC HS",               "HSMIC"},
    {"MAINMIC PGA",         NULL,                   "MIC MUX"},
    {"SUBMIC PGA",          NULL,                   "SUBMIC"},
    {"SIDEMIC PGA",         NULL,                   "MAINMIC PGA"},

    /* LINEIN */
    {"LINEINL PGA",         NULL,                   "LINEINL"},
    {"LINEINR PGA",         NULL,                   "LINEINR"},

    /* IBIAS SUPPLY */
    {"MIXER MONO",          NULL,                   "IBIAS"},
    {"MIXER OUT LEFT",      NULL,                   "IBIAS"},
    {"MIXER OUT RIGHT",     NULL,                   "IBIAS"},
    {"MIXER IN LEFT",       NULL,                   "IBIAS"},
    {"MIXER IN RIGHT",      NULL,                   "IBIAS"},
    {"MIXER LINEOUT LEFT",  NULL,                   "IBIAS"},
    {"MIXER LINEOUT RIGHT", NULL,                   "IBIAS"},

    /* PA SWITCH */
    {"SPEAKER SWITCH",      "Speaker Switch",       "MONO MIXER"},
    {"EARPIECE SWITCH",     "Earpiece Switch",      "MONO MIXER"},
    {"HEADSET_L SWITCH",    "Headset_L Switch",     "OUTL MIXER"},
    {"HEADSET_R SWITCH",    "Headset_R Switch",     "OUTR MIXER"},
    {"LINEOUT_L SWITCH",    "Lineout_L Switch",     "LINEOUTL MIXER"},
    {"LINEOUT_R SWITCH",    "Lineout_R Switch",     "LINEOUTR MIXER"},

    {"EARPIECE",            NULL,                   "SPEAKER SWITCH"},
    {"SPEAKER",             NULL,                   "EARPIECE SWITCH"},
    {"HEADSETL",            NULL,                   "HEADSET_L SWITCH"},
    {"HEADSETR",            NULL,                   "HEADSET_R SWITCH"},
    {"LINEOUTL",            NULL,                   "LINEOUT_L SWITCH"},
    {"LINEOUTR",            NULL,                   "LINEOUT_R SWITCH"},

    {"HEADSET_L SWITCH",    NULL,                   "VMID"},
    {"HEADSET_R SWITCH",    NULL,                   "VMID"},
    {"LINEOUT_L SWITCH",    NULL,                   "VMID"},
};

static const struct snd_soc_dapm_route route_map_v220[] = {
    /* PLAYBACK */
    {"PLAYBACK AGC PGA L",  NULL,                   "AP PLAYBACK L"},
    {"PLAYBACK AGC PGA R",  NULL,                   "AP PLAYBACK R"},
    {"DIG HARMONIC",        NULL,                   "PLAYBACK AGC PGA L"},
    {"DIG HARMONIC",        NULL,                   "PLAYBACK AGC PGA R"},
    {"PLAYBACK MUX",        "INPUT LEFT",           "DIG HARMONIC"},
    {"PLAYBACK MUX",        "INPUT LR MIX",         "DIG HARMONIC"},
    {"DACL SWITCH",         "DACL Switch",          "PLAYBACK MUX"},
    {"DACR SWITCH",         "DACR Switch",          "DIG HARMONIC"},
    {"DIGITAL DAC L",       NULL,                   "DACL SWITCH"},
    {"DIGITAL DAC R",       NULL,                   "DACR SWITCH"},

    /* ADC L */
    {"ADCL MUX",            "ADC ADC",              "ADC L"},
    {"ADCL MUX",            "ADC DIGMIC",           "DIGMIC SWITCH"},
    {"ADCL MUX",            "ADC DACL",             "DACL SWITCH"},
    {"ADCL MUX",            "ADC DACR",             "DACR SWITCH"},
    {"ADCL MUX",            "ADC DACV",             "DACV SWITCH"},
    {"ADC FILTER L",        NULL,                   "ADCL MUX"},
    /* ADC R */
    {"ADCR MUX",            "ADC ADC",              "ADC R"},
    {"ADCR MUX",            "ADC DIGMIC",           "DIGMIC SWITCH"},
    {"ADCR MUX",            "ADC DACL",             "DACL SWITCH"},
    {"ADCR MUX",            "ADC DACR",             "DACR SWITCH"},
    {"ADCR MUX",            "ADC DACV",             "DACV SWITCH"},
    {"ADC FILTER R",        NULL,                   "ADCR MUX"},

    /* CAPTURE AGC PGA L */
    {"AGCL MUX",            "AGCL ADCL",            "DMNS MUX"},
    {"AGCL MUX",            "AGCL VOICEU",          "SRCIU SWITCH"},
    {"CAPTURE AGC PGA L",   NULL,                   "AGCL MUX"},
    {"AP CAPTURE L",        NULL,                   "CAPTURE AGC PGA L"},
    /* CAPTURE AGC PGA R */
    {"AGCR MUX",            "AGCR ADCR",            "ADC FILTER R"},
    {"AGCR MUX",            "AGCR VOICED",          "SRCID SWITCH"},
    {"CAPTURE AGC PGA R",   NULL,                   "AGCR MUX"},
    {"AP CAPTURE R",        NULL,                   "CAPTURE AGC PGA R"},

    /* BT */
    {"BT_PLAYBACK SWITCH",  "Bt_playback Switch",   "MODEM_UL_L HBFVD"},
    {"BT_INCALL SWITCH",    "Bt_incall Switch",     "MODEM_DL EQ"},
    {"BT OUT",              NULL,                   "BT_PLAYBACK SWITCH"},
    {"BT OUT",              NULL,                   "BT_INCALL SWITCH"},
    /* BT PLAYBACK/INCALL SWITCHS ARE OFF MEANS CAPTURE AGC PGA L */
    /* BT FM STEREO PATH */
     {"BT OUT",              NULL,                   "CAPTURE AGC PGA L"}, 
    /* {"BT OUT",              NULL,                   "CAPTURE AGC PGA R"}, */

    /* MODEM UL/DL */
    {"MODEM_DL SWITCH",     "Modem_DL Switch",      "MODEM DOWNLINK"},
    {"MODEM_DL EQ",         NULL,                   "MODEM_DL SWITCH"},
    {"MODEM_DL HBFVI",      NULL,                   "MODEM_DL EQ"},
    {"DACV SWITCH",         "DACV Switch",          "MODEM_DL HBFVI"},
    {"DIGITAL DAC V",       NULL,                   "DACV SWITCH"},

    {"MODEM_UL_L HBFVD",    NULL,                   "DMNS MUX"},
    {"MODEM MUX",           "VOICE BT",             "BT IN"},
    {"MODEM MUX",           "VOICE MIC",            "MODEM_UL_L HBFVD"},
    {"MODEM_UL_L SWITCH",   "Modem_UL_L Switch",    "MODEM MUX"},
    {"MODEM UPLINK L",      NULL,                   "MODEM_UL_L SWITCH"},

    {"MODEM_UL_R HBFVD",    NULL,                   "ADC FILTER R"},
    {"MODEM_UL_R SWITCH",   "Modem_UL_R Switch",    "MODEM_UL_R HBFVD"},
    {"MODEM UPLINK R",      NULL,                   "MODEM_UL_R SWITCH"},

    /* DIGITAL MIC */
    {"DIGMIC SWITCH",       "Digmic Switch",        "DIGMIC"},

    /* DMNS */
    {"DMNS",                NULL,                   "ADC FILTER L"},
    {"DMNS",                NULL,                   "ADC FILTER R"},
    {"DMNS MUX",            "DISABLE",              "ADC FILTER L"},
    {"DMNS MUX",            "ENABLE",               "DMNS"},

    /* INCALL RECORD */
    {"SRCIU SWITCH",        "Srciu Switch",         "MODEM_UL_L SWITCH"},
    {"SRCID SWITCH",        "Srcid Switch",         "MODEM_DL HBFVI"},

    /* CLOCK SUPPLY */
    {"AP PLAYBACK L",       NULL,                   "AP CLK"},
    {"AP PLAYBACK R",       NULL,                   "AP CLK"},
    {"CAPTURE AGC PGA L",   NULL,                   "AP CLK"},
    {"CAPTURE AGC PGA R",   NULL,                   "AP CLK"},
    {"BT OUT",              NULL,                   "BT CLK"},
    {"BT IN",               NULL,                   "BT CLK"},
    {"AP CLK",              NULL,                   "PLL CLK"},
    {"BT CLK",              NULL,                   "PLL CLK"},
    {"MODEM_DL SWITCH",     NULL,                   "PLL CLK"},
    {"PLL CLK",             NULL,                   "IBIAS"},

    /* DAC */
    {"DAC L",               NULL,                   "ANALOG DAC L"},
    {"DAC R",               NULL,                   "ANALOG DAC R"},
    {"DAC V",               NULL,                   "ANALOG DAC V"},

    /* MIXER */
    {"MIXER MONO",          "DAC Left Switch",      "DAC L"},
    {"MIXER MONO",          "DAC Right Switch",     "DAC R"},
    {"MIXER MONO",          "DAC Voice Switch",     "DAC V"},
    {"MIXER MONO",          "Linein Left Switch",   "LINEINL PGA"},
    {"MIXER MONO",          "Linein Right Switch",  "LINEINR PGA"},
    {"MIXER MONO",          "Main PGA Switch",      "MAINMIC PGA"},
    {"MIXER MONO",          "Side PGA Switch",      "SIDETONE PGA"},

    {"MIXER OUT LEFT",      "DAC Left Switch",      "DAC L"},
    {"MIXER OUT LEFT",      "DAC Right Switch",     "DAC R"},
    {"MIXER OUT LEFT",      "DAC Voice Switch",     "DAC V"},
    {"MIXER OUT LEFT",      "Linein Left Switch",   "LINEINL PGA"},
    {"MIXER OUT LEFT",      "Linein Right Switch",  "LINEINR PGA"},
    {"MIXER OUT LEFT",      "Main PGA Switch",      "MAINMIC PGA"},
    {"MIXER OUT LEFT",      "Side PGA Switch",      "SIDETONE PGA"},

    {"MIXER OUT RIGHT",     "DAC Left Switch",      "DAC L"},
    {"MIXER OUT RIGHT",     "DAC Right Switch",     "DAC R"},
    {"MIXER OUT RIGHT",     "DAC Voice Switch",     "DAC V"},
    {"MIXER OUT RIGHT",     "Linein Left Switch",   "LINEINL PGA"},
    {"MIXER OUT RIGHT",     "Linein Right Switch",  "LINEINR PGA"},
    {"MIXER OUT RIGHT",     "Main PGA Switch",      "MAINMIC PGA"},
    {"MIXER OUT RIGHT",     "Side PGA Switch",      "SIDETONE PGA"},

    {"MIXER IN LEFT",       "DAC Left Switch",      "DAC L"},
    {"MIXER IN LEFT",       "DAC Right Switch",     "DAC R"},
    {"MIXER IN LEFT",       "DAC Voice Switch",     "DAC V"},
    {"MIXER IN LEFT",       "Linein Left Switch",   "LINEINL PGA"},
    {"MIXER IN LEFT",       "Linein Right Switch",  "LINEINR PGA"},
    {"MIXER IN LEFT",       "Main PGA Switch",      "MAINMIC PGA"},
    {"MIXER IN LEFT",       "Sub PGA Switch",       "SUBMIC PGA" },

    {"MIXER IN RIGHT",      "DAC Left Switch",      "DAC L"},
    {"MIXER IN RIGHT",      "DAC Right Switch",     "DAC R"},
    {"MIXER IN RIGHT",      "DAC Voice Switch",     "DAC V"},
    {"MIXER IN RIGHT",      "Linein Left Switch",   "LINEINL PGA"},
    {"MIXER IN RIGHT",      "Linein Right Switch",  "LINEINR PGA"},
    {"MIXER IN RIGHT",      "Main PGA Switch",      "MAINMIC PGA"},
    {"MIXER IN RIGHT",      "Sub PGA Switch",       "SUBMIC PGA" },

    {"MIXER LINEOUT LEFT",  "Mixer Left Switch",    "MIXER OUT LEFT"},
    {"MIXER LINEOUT LEFT",  "Mixer Right Switch",   "MIXER OUT RIGHT"},
    {"MIXER LINEOUT LEFT",  "Mixer Mono Switch",    "MIXER MONO"},

    {"MIXER LINEOUT RIGHT", "Mixer Left Switch",    "MIXER OUT LEFT"},
    {"MIXER LINEOUT RIGHT", "Mixer Right Switch",   "MIXER OUT RIGHT"},
    {"MIXER LINEOUT RIGHT", "Mixer Mono Switch",    "MIXER MONO"},

    /* OUTPUT */
    {"OUT MONO",            NULL,                   "MIXER MONO"},
    {"OUT L",               NULL,                   "MIXER OUT LEFT"},
    {"OUT R",               NULL,                   "MIXER OUT RIGHT"},
    {"LINEOUT L",           NULL,                   "MIXER LINEOUT LEFT"},
    {"LINEOUT R",           NULL,                   "MIXER LINEOUT RIGHT"},
    {"ADC L",               NULL,                   "MIXER IN LEFT"},
    {"ADC R",               NULL,                   "MIXER IN RIGHT"},

    /* main mic need to poweron main micbias */
    {"MAINMIC",             NULL,                   "MAIN MIC"},
    /* headset always poweron micbias */
    {"HSMIC",               NULL,                   "HS MIC"},

    {"SUB MICBIAS",         NULL,                   "SUB MIC"},
    {"SUBMIC",              NULL,                   "SUB MICBIAS"},

    /* MIC */
    {"MAINMIC",             NULL,                   "IMB_EN"},
    {"MIC MUX",             "MIC MAIN",             "MAINMIC"},
    {"MIC MUX",             "MIC HS",               "HSMIC"},
    {"MAINMIC PGA",         NULL,                   "MIC MUX"},
    {"SUBMIC PGA",          NULL,                   "SUBMIC"},
    {"SIDETONE PGA",        NULL,                   "MAINMIC PGA"},

    /* LINEIN */
    {"LINEINL PGA",         NULL,                   "LINEINL"},
    {"LINEINR PGA",         NULL,                   "LINEINR"},

    /* IBIAS SUPPLY */
    {"MIXER MONO",          NULL,                   "IBIAS"},
    {"MIXER OUT LEFT",      NULL,                   "IBIAS"},
    {"MIXER OUT RIGHT",     NULL,                   "IBIAS"},
    {"MIXER IN LEFT",       NULL,                   "IBIAS"},
    {"MIXER IN RIGHT",      NULL,                   "IBIAS"},
    {"MIXER LINEOUT LEFT",  NULL,                   "IBIAS"},
    {"MIXER LINEOUT RIGHT", NULL,                   "IBIAS"},

    /* PA SWITCH */
    {"SPEAKER SWITCH",      "Speaker Switch",       "MONO MIXER"},
    {"EARPIECE SWITCH",     "Earpiece Switch",      "MONO MIXER"},
    {"HEADSET_L SWITCH",    "Headset_L Switch",     "OUTL MIXER"},
    {"HEADSET_R SWITCH",    "Headset_R Switch",     "OUTR MIXER"},
    {"LINEOUT_L SWITCH",    "Lineout_L Switch",     "LINEOUTL MIXER"},
    {"LINEOUT_R SWITCH",    "Lineout_R Switch",     "LINEOUTR MIXER"},

    {"EARPIECE",            NULL,                   "SPEAKER SWITCH"},
    {"SPEAKER",             NULL,                   "EARPIECE SWITCH"},
    {"HEADSETL",            NULL,                   "HEADSET_L SWITCH"},
    {"HEADSETR",            NULL,                   "HEADSET_R SWITCH"},
    {"LINEOUTL",            NULL,                   "LINEOUT_L SWITCH"},
    {"LINEOUTR",            NULL,                   "LINEOUT_R SWITCH"},

    {"HEADSET_L SWITCH",    NULL,                   "VMID"},
    {"HEADSET_R SWITCH",    NULL,                   "VMID"},
    {"LINEOUT_L SWITCH",    NULL,                   "VMID"},
};

#ifdef CONFIG_MFD_HI6421_IRQ
static void mask_irq_of_plugout (struct snd_soc_codec *codec, bool mask) {
    struct hi6421_codec_platform_data *pdata = dev_get_platdata(codec->dev);
    struct irq_desc *desc_pmu_plug_out;

    if (!pdata) {
        pr_err(" %s(%u) error: pdate is NULL\n", __FUNCTION__, __LINE__);
        return;
    }

    desc_pmu_plug_out = irq_to_desc(pdata->irq[0]);
    if (!desc_pmu_plug_out) {
        pr_err(" %s(%u) error: desc_pmu_plug_out is NULL\n", __FUNCTION__, __LINE__);
        return;
    }

    pr_info(" %s(%u) : mask = %s\n", __FUNCTION__, __LINE__, mask?"T":"F");

    if (mask) {
        /* mask interrupts */
        desc_pmu_plug_out->irq_data.chip->irq_mask(&desc_pmu_plug_out->irq_data);
    } else {
        /* unmask interrupts */
        desc_pmu_plug_out->irq_data.chip->irq_unmask(&desc_pmu_plug_out->irq_data);
    }
}

static void mask_irq_of_btn (struct snd_soc_codec *codec, bool mask) {
    struct hi6421_codec_platform_data *pdata = dev_get_platdata(codec->dev);
    struct irq_desc *desc_pmu_btn_press;
    struct irq_desc *desc_pmu_btn_release;

    if (!pdata) {
        pr_err(" %s(%u) error: pdate is NULL\n", __FUNCTION__, __LINE__);
        return;
    }

    desc_pmu_btn_press = irq_to_desc(pdata->irq[2]);

    if (!desc_pmu_btn_press ) {
        pr_err(" %s(%u) error: desc_pmu_btn_press is  NULL\n", __FUNCTION__, __LINE__);
        return;
    }

    desc_pmu_btn_release = irq_to_desc(pdata->irq[3]);

    if (!desc_pmu_btn_release ) {
        pr_err(" %s(%u) error: desc_pmu_btn_release is  NULL\n", __FUNCTION__, __LINE__);
        return;
    }

    pr_info(" %s(%u) : mask = %s\n", __FUNCTION__, __LINE__, mask?"T":"F");

    if (mask) {
        /* mask interrupts */
        desc_pmu_btn_press->irq_data.chip->irq_mask(&desc_pmu_btn_press->irq_data);
        desc_pmu_btn_release->irq_data.chip->irq_mask(&desc_pmu_btn_release->irq_data);
    } else {
        hi6421_set_reg_bit(HI6421_REG_IRQ3, HI6421_HDS_BTN_RELEASE_BIT);
        hi6421_set_reg_bit(HI6421_REG_IRQ3, HI6421_HDS_BTN_PRESS_BIT);
        /* unmask interrupts */
        desc_pmu_btn_press->irq_data.chip->irq_unmask(&desc_pmu_btn_press->irq_data);
        desc_pmu_btn_release->irq_data.chip->irq_unmask(&desc_pmu_btn_release->irq_data);
    }
}

static inline bool is_headset_pluged(struct snd_soc_codec *codec)
{
    /* read pmu status, then compare pluged bit */
    return (hi6421_reg_read(codec, HI6421_REG_STATUS0) & HI6421_PLUGCOMP);
}

static void jack_report_switch_state(struct snd_soc_jack *hi6421_jack,
                                     int status,  struct switch_dev *sdev)
{
    /* read jack and report then change headset state */
    snd_soc_jack_report(hi6421_jack, status, SND_JACK_HEADSET | SND_JACK_BTN_0);
    switch_set_state(sdev, hs_status);
}

#ifdef CONFIG_K3_ADC
static inline void hi6421_hkadc_ctl_probe(struct snd_soc_codec *codec, bool is_on)
{
    if (is_on) {
        hi6421_clr_reg_bit(HI6421_MAINMICBIAS_PD_REG, HI6421_MAINMICBIAS_PD_BIT); //CA  -> 328
        hi6421_set_reg_bit(HI6421_REG_MICBIAS, HI6421_MICBIAS_HS_MIC_EN_BIT);
        //EC  ->3B0
        /*SWM_EN is to do with PB_DETECT*/
        hi6421_set_reg_bit(HI6421_REG_MICBIAS, HI6421_MICBIAS_SWM_EN_BIT);
    } else {
        unsigned int value = hi6421_reg_read(codec, HI6421_MAINPGA_PD_REG);
        if (value & (1 << HI6421_MAINPGA_PD_BIT)) {
            pr_info("%s: mainpga is OFF",  __FUNCTION__);
            hi6421_set_reg_bit(HI6421_MAINMICBIAS_PD_REG, HI6421_MAINMICBIAS_PD_BIT);
        } else {
            pr_info("%s: mainpga is ON",  __FUNCTION__);
        }
        hi6421_clr_reg_bit(HI6421_REG_MICBIAS, HI6421_MICBIAS_SWM_EN_BIT);
        hi6421_clr_reg_bit(HI6421_REG_MICBIAS, HI6421_MICBIAS_HS_MIC_EN_BIT);
    }
}

static inline void hi6421_hkadc_ctl(struct snd_soc_codec *codec, bool is_on)
{
    struct hi6421_data *priv = snd_soc_codec_get_drvdata(codec);

    if (!priv) {
        pr_err(" %s(%u) error: priv is NULL\n", __FUNCTION__, __LINE__);
        return;
    }

    if (is_on) {
        if (E_UART_HEADSET_TYPE_SMART == get_uart_headset_type())
        {
            if (gpio_get_value(HS_UART_STATUS) == 0)
            {
                gpio_direction_output(HS_UART_STATUS, 0);
            }
            else
            {
                //do not turn on BIAS since uart cable is plugged in
                return;
            }
        }
        hi6421_clr_reg_bit(HI6421_MAINMICBIAS_PD_REG, HI6421_MAINMICBIAS_PD_BIT);
        hi6421_set_reg_bit(HI6421_REG_HSD, HI6421_HS_FAST_DISCHARGE_BIT);
        msleep(HI6421_HS_POWERDOWN_WAITTIME);
        hi6421_clr_reg_bit(HI6421_REG_HSD, HI6421_HS_FAST_DISCHARGE_BIT);
        hi6421_set_reg_bit(HI6421_REG_MICBIAS, HI6421_MICBIAS_HS_MIC_EN_BIT);
        /*SWM_EN is to do with PB_DETECT*/
        hi6421_set_reg_bit(HI6421_REG_MICBIAS, HI6421_MICBIAS_SWM_EN_BIT);
    } else {
        if (E_UART_HEADSET_TYPE_SMART == get_uart_headset_type())
        {
            //set back to input anyway
            gpio_direction_input(HS_UART_STATUS);
        }
        unsigned int value = hi6421_reg_read(codec, HI6421_MAINPGA_PD_REG);
        unsigned int eco = hi6421_reg_read(codec, HI6421_REG_MICBIAS);
        hi6421_clr_reg_bit(HI6421_REG_MICBIAS, HI6421_MICBIAS_HS_MIC_EN_BIT);
        hi6421_clr_reg_bit(HI6421_REG_MICBIAS, HI6421_MICBIAS_ECO_BIT); /*Normal*/
        hi6421_clr_reg_bit(HI6421_REG_MICBIAS, HI6421_MICBIAS_SWM_EN_BIT);

        if (value & (1 << HI6421_MAINPGA_PD_BIT)) {
            pr_info("%s: mainpga is OFF",  __FUNCTION__);
            hi6421_set_reg_bit(HI6421_MAINMICBIAS_PD_REG, HI6421_MAINMICBIAS_PD_BIT);
        } else {
            pr_info("%s: mainpga is ON",  __FUNCTION__);
        }

        hi6421_set_reg_bit(HI6421_REG_HSD, HI6421_HS_FAST_DISCHARGE_BIT);
        msleep(HI6421_HS_POWERDOWN_WAITTIME);
        hi6421_clr_reg_bit(HI6421_REG_HSD, HI6421_HS_FAST_DISCHARGE_BIT);

        if (eco &(1<<HI6421_MICBIAS_ECO_BIT)) {
            hi6421_set_reg_bit(HI6421_REG_MICBIAS, HI6421_MICBIAS_ECO_BIT);
        }
    }
}

static irqreturn_t hi6421_btn_down_handler(int irq, void *data)
{
    struct snd_soc_codec *codec = data;
    struct hi6421_data *priv = snd_soc_codec_get_drvdata(codec);

    if (!priv) {
        pr_err(" %s(%u) error: priv is NULL\n", __FUNCTION__, __LINE__);
        return IRQ_HANDLED;
    }

    queue_delayed_work(priv->headset_btn_down_delay_wq,
                       &priv->headset_btn_down_delay_work,
                       msecs_to_jiffies(0));

    return IRQ_HANDLED;
}

static irqreturn_t hi6421_btn_up_handler(int irq, void *data)
{
    struct snd_soc_codec *codec = data;
    struct hi6421_data *priv = snd_soc_codec_get_drvdata(codec);

    if (!priv) {
        pr_err(" %s(%u) error: priv is NULL\n", __FUNCTION__, __LINE__);
        return IRQ_HANDLED;
    }

    wake_lock_timeout(&priv->wake_lock,
            HI6421_HS_BTN_JUDGEMENT_TIME + HI6421_HS_WAKE_LOCK_TIMEOUT);
    queue_delayed_work(priv->headset_btn_up_delay_wq,
            &priv->headset_btn_up_delay_work,
            msecs_to_jiffies(HI6421_HS_BTN_JUDGEMENT_TIME));

    return IRQ_HANDLED;
}

int hi6421_get_voltage(struct hi6421_data *priv)
{
    int voltage = 0;
    int factor = priv->lower_power ?
            HI6421_ECO_VOLTAGE_FACTOR :
            HI6421_NORMAL_VOLTAGE_FACTOR;

    voltage = k3_adc_get_value(ADC_PB_DETECT, NULL);

    if (priv->check_voltage > 0)
        voltage = (voltage * factor) / priv->check_voltage;

    pr_info("%s:%u voltage = %d\n", __FUNCTION__, __LINE__, voltage);

    return voltage;
}

void hi6421_btn_down_work_func(struct work_struct *work)
{
    struct hi6421_data *priv =
            container_of(work, struct hi6421_data, headset_btn_down_delay_work.work);
    struct snd_soc_codec *codec;
    struct hi6421_jack_data *hi6421_hs_jack;
    int ret =  0;

    if (!priv) {
        pr_err(" %s(%u) error: priv is NULL\n", __FUNCTION__, __LINE__);
        return;
    }

    codec = priv->codec;
    wake_lock_timeout(&priv->wake_lock,
            HI6421_HS_BTN_JUDGEMENT_TIME + HI6421_HS_WAKE_LOCK_TIMEOUT);
    msleep(HI6421_HS_BTN_JUDGEMENT_TIME);

    hi6421_hs_jack = &priv->hs_jack;
    mutex_lock(&priv->io_mutex);
    if (HI6421_JACK_BIT_HEADSET ==  hs_status) {
        ret = hi6421_get_voltage(priv);

        if (is_headset_pluged(codec)) {
            if (ret > HI6421_SINGLE_BTN_VOLTAGE_MIN && ret < HI6421_SINGLE_BTN_VOLTAGE_MAX) {
                hi6421_hs_jack->jack->jack->type = SND_JACK_BTN_0;
                hi6421_hs_jack->report |= SND_JACK_BTN_0;
                jack_report_switch_state(hi6421_hs_jack->jack,
                                         hi6421_hs_jack->report,
                                         &priv->hs_jack.sdev);
                pr_info("%s: ADC_PB_DETECT SINGLE_BTN = %d hs_status = %d\n",
                        __FUNCTION__, ret, hs_status);
            } else {
                pr_info("%s: ADC_PB_DETECT OTHER = %d\n",  __FUNCTION__, ret);
            }
        } else {
            pr_info("%s: PLUG OUT\n", __FUNCTION__);
        }
    }
    mutex_unlock(&priv->io_mutex);

    return;
}

void hi6421_btn_up_work_func(struct work_struct *work)
{
    struct hi6421_data *priv =
            container_of(work, struct hi6421_data, headset_btn_up_delay_work.work);
    struct hi6421_jack_data *hi6421_hs_jack;

    if (!priv) {
        pr_err(" %s(%u) error: priv is NULL\n", __FUNCTION__, __LINE__);
        return;
    }

    hi6421_hs_jack = &priv->hs_jack;
    hi6421_hs_jack->jack->jack->type = SND_JACK_BTN_0;
    hi6421_hs_jack->report &= ~SND_JACK_BTN_0;
    jack_report_switch_state(hi6421_hs_jack->jack,
                            hi6421_hs_jack->report,
                            &priv->hs_jack.sdev);

    return;
}
#endif
void hi6421_hs_btn_regester(struct snd_soc_codec *codec,
                struct snd_soc_jack *hi6421_jack,  int report)
{
    struct hi6421_data *priv = snd_soc_codec_get_drvdata(codec);
    struct hi6421_jack_data *hi6421_hs_jack;

    if (!priv) {
        pr_err(" %s(%u) error: priv is NULL\n", __FUNCTION__, __LINE__);
        return;
    }

    hi6421_hs_jack = &priv->hs_jack;
    hi6421_hs_jack->jack = hi6421_jack;
    hi6421_hs_jack->report = 0;
    snd_jack_set_key(hi6421_jack->jack, SND_JACK_BTN_0,KEY_MEDIA);
    snd_soc_jack_report(hi6421_jack, SND_JACK_HEADSET, SND_JACK_HEADSET | SND_JACK_BTN_0);

}

static irqreturn_t hi6421_hp_plug_out_handler(int irq, void *data)
{
    struct snd_soc_codec *codec = data;
    struct hi6421_data *priv = snd_soc_codec_get_drvdata(codec);

    if (!priv) {
        pr_err(" %s(%u) error: priv is NULL\n", __FUNCTION__, __LINE__);
        return IRQ_HANDLED;
    }

    queue_delayed_work(priv->headset_plug_out_delay_wq,
                       &priv->headset_plug_out_delay_work,
                       msecs_to_jiffies(0));

    return IRQ_HANDLED;
}

static irqreturn_t hi6421_hp_plug_in_handler(int irq, void *data)
{
    struct snd_soc_codec *codec = data;
    struct hi6421_data *priv = snd_soc_codec_get_drvdata(codec);

    if (!priv) {
        pr_err(" %s(%u) error: priv is NULL\n", __FUNCTION__, __LINE__);
        return IRQ_HANDLED;
    }

    wake_lock_timeout(&priv->wake_lock, HI6421_HS_WAKE_LOCK_TIMEOUT);
    /* mask hsd interrupt */
    mask_irq_of_plugout(codec, true);

    queue_delayed_work(priv->headset_judgement_delay_wq,
                       &priv->headset_judgement_delay_work,
                       msecs_to_jiffies(0));

    return IRQ_HANDLED;
}

void hi6421_audio_judgement_work_func(struct work_struct *work)
{
    struct hi6421_data *priv =
        container_of(work, struct hi6421_data, headset_judgement_delay_work.work);
    int ret = 0;

    if (!priv) {
        pr_err(" %s(%u) error: priv is NULL\n", __FUNCTION__, __LINE__);
        return;
    }

    wake_lock_timeout(&priv->wake_lock,
            HI6421_HS_PLUG_DETECT_TIME + HI6421_HS_WAKE_LOCK_TIMEOUT);
    ret = cancel_delayed_work(&priv->headset_plug_in_delay_work);

    if (!ret)
        flush_workqueue(priv->headset_plug_in_delay_wq);

    queue_delayed_work(priv->headset_plug_in_delay_wq,
                       &priv->headset_plug_in_delay_work,
                       msecs_to_jiffies(HI6421_HS_PLUG_DETECT_TIME));
}

void hi6421_audio_plug_in_work_func(struct work_struct *work)
{
    struct hi6421_data *priv =
        container_of(work, struct hi6421_data, headset_plug_in_delay_work.work);
    struct hi6421_jack_data *hi6421_hs_jack;
    struct snd_soc_codec *codec;
    int ret = 0;

    if (!priv) {
        pr_err(" %s(%u) error: priv is NULL\n", __FUNCTION__, __LINE__);
        return;
    }

    hi6421_hs_jack = &priv->hs_jack;
    codec = priv->codec;

    if (!is_headset_pluged(codec)) {
        pr_info(" %s(%u)  hs pluged out\n", __FUNCTION__, __LINE__);
        return;
    }

    wake_lock(&priv->plugin_wake_lock);
    mutex_lock(&priv->io_mutex);

#ifdef CONFIG_K3_ADC
    hi6421_hkadc_ctl(codec, true);
    msleep(HI6421_HKADC_SLEEP_TIME);
    /**
     * 1. micbias/hkadc power on
     * 2. detect & report headset type; unmask btn irq if headset type is headset.
     * 3. micbias power off if headset type is headphone
     */
    ret = hi6421_get_voltage(priv);

    pr_info("%s:ret = %umV\n",__FUNCTION__, ret);

    if (ret < HI6421_PB_VOLTAGE_MAX && ret > HI6421_PB_VOLTAGE_MIN) {
        pr_info("%s:%u: It is 4 plug!\n",__FUNCTION__, __LINE__);
        if (HI6421_JACK_BIT_HEADSET != hs_status) {
            hi6421_hs_jack->report = SND_JACK_HEADSET;
            hs_status = HI6421_JACK_BIT_HEADSET;
            mask_irq_of_btn(codec, false);
        }
    } else {
        /* It is a plug without mic*/
        pr_info("%s:%u: It is 3 plug! \n",__FUNCTION__, __LINE__);
        if (HI6421_JACK_BIT_HEADSET_NO_MIC != hs_status) {
            hi6421_hs_jack->report = SND_JACK_HEADPHONE;
            hs_status = HI6421_JACK_BIT_HEADSET_NO_MIC;
            mask_irq_of_btn(codec, true);
            hi6421_hkadc_ctl(codec, false);
        }
    }

    if (!is_headset_pluged(codec)) {
        /* headset has pluged out, do not detect any more */
        pr_info(" %s(%u)  hs pluged out\n", __FUNCTION__, __LINE__);
        hs_status = HI6421_JACK_BIT_NONE;
        hi6421_hkadc_ctl(codec, false);
        mask_irq_of_btn(codec, true);
        goto mw_unlock;
    }
#else
    pr_info("%s:%u: It is 3 plug! \n",__FUNCTION__, __LINE__);
    hi6421_hs_jack->report = SND_JACK_HEADPHONE;
    hs_status = HI6421_JACK_BIT_HEADSET_NO_MIC;

    if (!is_headset_pluged(codec)) {
        /* headset has pluged out, do not detect any more */
        pr_info(" %s(%u)  hs pluged out\n", __FUNCTION__, __LINE__);
        hs_status = HI6421_JACK_BIT_NONE;
        goto mw_unlock;
    }
#endif

    hi6421_set_reg_bit(HI6421_REG_IRQ2, HI6421_HDS_PLUG_OUT_BIT);
    mask_irq_of_plugout(codec, false);

    jack_report_switch_state(hi6421_hs_jack->jack,
                            hi6421_hs_jack->report,
                            &priv->hs_jack.sdev);
 mw_unlock :
    mutex_unlock(&priv->io_mutex);
    wake_unlock(&priv->plugin_wake_lock);

    return;
}

void hi6421_audio_plug_out_work_func(struct work_struct *work)
{
    struct hi6421_data *priv =
        container_of(work, struct hi6421_data, headset_plug_out_delay_work.work);
    struct hi6421_jack_data *hi6421_hs_jack;
    struct snd_soc_codec *codec;

    if (!priv) {
        pr_err(" %s(%u) error: priv is NULL\n", __FUNCTION__, __LINE__);
        return;
    }

    hi6421_hs_jack = &priv->hs_jack;
    codec = priv->codec;
    wake_lock_timeout(&priv->wake_lock,
            HI6421_HS_POWERDOWN_WAITTIME + HI6421_HS_WAKE_LOCK_TIMEOUT);
    mask_irq_of_btn(codec, true);
#ifdef CONFIG_K3_ADC
    hi6421_hkadc_ctl(codec, false);
#endif
    mutex_lock(&priv->io_mutex);
    hs_status = HI6421_JACK_BIT_NONE;
    priv->hs_jack.report = 0;
    jack_report_switch_state(hi6421_hs_jack->jack,
                            hi6421_hs_jack->report,
                            &priv->hs_jack.sdev);
    mutex_unlock(&priv->io_mutex);

    return;
}
#endif

static int hi6421_add_widgets(struct snd_soc_codec *codec, unsigned int pmuid)
{
    switch (pmuid) {
    case PMUID_V200:
        pr_info("%s : pmuid = %x\n", __FUNCTION__, pmuid);
        snd_soc_add_controls(codec, hi6421_v200_snd_controls,
                ARRAY_SIZE(hi6421_v200_snd_controls));
        snd_soc_dapm_new_controls(&codec->dapm, hi6421_v200_dapm_widgets,
                ARRAY_SIZE(hi6421_v200_dapm_widgets));
        snd_soc_dapm_add_routes(&codec->dapm, route_map_v200,
                ARRAY_SIZE(route_map_v200));
        hi6421_is_cs = false;
        break;
    case PMUID_V220:
    default:
        pr_info("%s : pmuid = %x\n", __FUNCTION__, pmuid);
        snd_soc_add_controls(codec, hi6421_v220_snd_controls,
                ARRAY_SIZE(hi6421_v220_snd_controls));
        snd_soc_dapm_new_controls(&codec->dapm, hi6421_v220_dapm_widgets,
                ARRAY_SIZE(hi6421_v220_dapm_widgets));
        snd_soc_dapm_add_routes(&codec->dapm, route_map_v220,
                ARRAY_SIZE(route_map_v220));
        hi6421_is_cs = true;
        break;
    }

    snd_soc_dapm_new_widgets(&codec->dapm);

    return 0;
}

static inline void hi6421_power_on(struct snd_soc_codec *codec)
{
    struct hi6421_data *priv = snd_soc_codec_get_drvdata(codec);

    if (!priv) {
        pr_err(" %s(%u) error: priv is NULL\n", __FUNCTION__, __LINE__);
        return;
    }

    hi6421_clr_reg_bit(HI6421_REG_MICBIAS, HI6421_MICBIAS_ECO_BIT);
    priv->lower_power = false;
}

static inline void hi6421_power_off(struct snd_soc_codec *codec)
{
    struct hi6421_data *priv = snd_soc_codec_get_drvdata(codec);

    if (!priv) {
        pr_err(" %s(%u) error: priv is NULL\n", __FUNCTION__, __LINE__);
        return;
    }

    hi6421_set_reg_bit(HI6421_REG_MICBIAS, HI6421_MICBIAS_ECO_BIT);
    priv->lower_power = true;

}

/* INIT REGISTER VALUE FROM ARRAY */
static void init_reg_value(struct snd_soc_codec *codec)
{
    /* todo */
    unsigned int i = 0;

    hi6421_reg_write(codec, HI6421_REG_CIC_DMNS_CONFIG, codec_registers[0]);
    hi6421_reg_write(codec, HI6421_REG_DMNS_GAIN,       codec_registers[1]);

    for (i = HI6421_CODEC_REG_RWLOOP1_FIRST; i <= HI6421_CODEC_REG_RWLOOP1_LAST; i++)
        hi6421_reg_write(codec, i, codec_registers[i - HI6421_CODEC_REG_FIRST]);

    for (i = HI6421_CODEC_REG_RWLOOP2_FIRST; i <= HI6421_CODEC_REG_RWLOOP2_LAST; i++)
        hi6421_reg_write(codec, i, codec_registers[i - HI6421_CODEC_REG_FIRST]);

    /* restore output register in the end*/
    for (i = HI6421_REG_CODECENA_1; i <= HI6421_REG_CODECENA_4; i++)
        hi6421_reg_write(codec, i, codec_registers[i - HI6421_CODEC_REG_FIRST]);
}

static inline bool hi6421_has_active_pcm(void)
{
    return (hi6421_pcm_mm_pb ||
            hi6421_pcm_mm_cp ||
            hi6421_pcm_modem ||
            hi6421_pcm_fm);
}

static inline bool hi6421_is_pll_active(void)
{
    return (hi6421_pcm_mm_pb ||
            hi6421_pcm_mm_cp ||
            hi6421_pcm_modem);
}

static int hi6421_startup(struct snd_pcm_substream *substream,
                          struct snd_soc_dai *dai)
{
    struct hi6421_data *priv = snd_soc_codec_get_drvdata(g_codec);

    if (!priv ) {
        pr_err("%s(%u): priv is null !\n", __FUNCTION__, __LINE__);
        return -ENOMEM;
    }

    mutex_lock(&hi6421_power_lock);
    if ((HI6421_PORT_MODEM == substream->pcm->device) ||
           (HI6421_PORT_MM == substream->pcm->device)) {
        pr_info( "hi6421_startup check pll work function running\n");
        if (!hi6421_is_pll_active()) {
            queue_delayed_work(priv->pll_delay_wq,
                               &priv->pll_delay_work,
                               msecs_to_jiffies(HI6421_PLL_DETECT_TIME));
        }
    }

    if (!hi6421_has_active_pcm())
        hi6421_power_on(g_codec);

    switch(substream->pcm->device) {
    case HI6421_PORT_MM:
        if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream) {
            hi6421_pcm_mm_pb = true;
        } else {
            hi6421_pcm_mm_cp = true;
        }
        break;
    case HI6421_PORT_MODEM:
        hi6421_pcm_modem = true;
        break;
    case HI6421_PORT_FM:
        hi6421_pcm_fm = true;
        break;
    }
    mutex_unlock(&hi6421_power_lock);

    return 0;
}

static void hi6421_shutdown(struct snd_pcm_substream *substream,
                            struct snd_soc_dai *dai)
{
    /* do nothing with pll work function */
    mutex_lock(&hi6421_power_lock);
    switch(substream->pcm->device) {
    case HI6421_PORT_MM:
        if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream) {
            hi6421_pcm_mm_pb = false;
        } else {
            hi6421_pcm_mm_cp = false;
        }
        break;
    case HI6421_PORT_MODEM:
        hi6421_pcm_modem = false;
        break;
    case HI6421_PORT_FM:
        hi6421_pcm_fm = false;
        break;
    }

    if (!hi6421_has_active_pcm())
        hi6421_power_off(g_codec);
    mutex_unlock(&hi6421_power_lock);
}

static int hi6421_hw_params(struct snd_pcm_substream *substream,
        struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
    if (HI6421_PORT_MM == substream->pcm->device) {
        /* bit HI6421_FREQ_AP_BIT means :
         * 0 : 48kHz
         * 1 : 96kHz
         */
        if (params_rate(params) > 48000) {
            pr_info("%s : set codec rate 96kHz\n", __FUNCTION__);
            hi6421_set_reg_bit(HI6421_REG_FREQ_CONFIG, HI6421_FREQ_AP_BIT);
        } else {
            pr_info("%s : set codec rate 48kHz\n", __FUNCTION__);
            hi6421_clr_reg_bit(HI6421_REG_FREQ_CONFIG, HI6421_FREQ_AP_BIT);
        }
    }

    if (HI6421_PORT_MODEM == substream->pcm->device) {
        /* bit HI6421_FREQ_MODEM_BIT means :
         * 0 : 8kHz
         * 1 : 16kHz
         */
        if (params_rate(params) == 16000) {
            pr_info("%s : set modem rate 16kHz\n", __FUNCTION__);
            hi6421_set_reg_bit(HI6421_REG_FREQ_CONFIG, HI6421_FREQ_MODEM_BIT);
        } else {
            pr_info("%s : set modem rate 8kHz\n", __FUNCTION__);
            hi6421_clr_reg_bit(HI6421_REG_FREQ_CONFIG, HI6421_FREQ_MODEM_BIT);
        }
    }

    return 0;
}

struct snd_soc_dai_ops hi6421_dai_ops = {
    .startup    = hi6421_startup,
    .shutdown   = hi6421_shutdown,
    .hw_params  = hi6421_hw_params,
};

struct snd_soc_dai_driver hi6421_dai = {
    .name = "hi6421-dai",
    .playback = {
        .stream_name    = "Playback",
        .channels_min   = HI6421_PB_MIN_CHANNELS,
        .channels_max   = HI6421_PB_MAX_CHANNELS,
        .rates          = HI6421_RATES,
        .formats        = HI6421_FORMATS},
    .capture = {
        .stream_name    = "Capture",
        .channels_min   = HI6421_CP_MIN_CHANNELS,
        .channels_max   = HI6421_CP_MAX_CHANNELS,
        .rates          = HI6421_RATES,
        .formats        = HI6421_FORMATS},
    .ops = &hi6421_dai_ops,

};

static int hi6421_codec_probe(struct snd_soc_codec *codec)
{
    int ret = 0;
    struct hi6421_data *priv = NULL;
    struct snd_soc_dapm_context *dapm = &codec->dapm;

#ifdef CONFIG_MFD_HI6421_IRQ
    struct hi6421_jack_data *hi6421_hs_jack = NULL;
    struct hi6421_codec_platform_data *pdata = dev_get_platdata(codec->dev);

    if (!pdata) {
        pr_err("%s(%u) : pdata is NULL", __FUNCTION__,__LINE__);
        return -ENOMEM;
    }

#endif
    priv = kzalloc(sizeof(struct hi6421_data), GFP_KERNEL);

    if (!priv) {
        pr_err("%s(%u) : kzalloc failed", __FUNCTION__,__LINE__);
        return -ENOMEM;
    }

    snd_soc_codec_set_drvdata(codec, priv);
    priv->codec = codec;
    g_codec = codec;

    mutex_init(&hi6421_power_lock);

    spin_lock_init(&priv->lock);
    mutex_init(&priv->io_mutex);

    init_reg_value(codec);

    /* set ldo 5mA(eco mode) */
    regulator_set_optimum_mode(g_regu, AUDIO_ECO_MODE);
    ret = regulator_enable(g_regu);

    if (ret < 0) {
        pr_err("%s: regulator enable failed\n", __FUNCTION__);
        goto regulator_err;
    }

    if (E_UART_HEADSET_TYPE_NOT_SUPPORTED != get_uart_headset_type())
    {
        /*config the gpio 61 for co-exist of headset-detect and uart-debug*/
        ret = gpio_request(HS_UART_STATUS, "headset_uart_judge");
        if (ret < 0){
            pr_err("%s: gpio request GPIO_061 error\n", __FUNCTION__);
            goto regulator_err;
        }
    }

    if (E_UART_HEADSET_TYPE_SMART == get_uart_headset_type())
    {
        ret = gpio_direction_input(HS_UART_STATUS);
        if (ret < 0){
            pr_err("%s: set GPIO_061 output 1 error\n", __FUNCTION__);
            goto regulator_err;
        }
    }
    else if (E_UART_HEADSET_TYPE_LEGACY == get_uart_headset_type())
    {
        if (if_uart_output_enabled())
        {
            gpio_direction_output(HS_UART_STATUS, 1);
        }
        else
        {
            gpio_direction_output(HS_UART_STATUS, 0);
        }
    }

    priv->pmuid = get_pmuid();

    hi6421_hsd_inv = get_hsd_invert();
    hi6421_hs_keys = get_hs_keys();

    hi6421_reg_write(codec, HI6421_REG_HSD, hi6421_hsd_inv ? 6 : 4);
    hi6421_add_widgets(codec, priv->pmuid);
    ret = snd_soc_dapm_sync(dapm);

    if (ret) {
        pr_err("%s : dapm sync error, errornum = %d\n", __FUNCTION__, ret);
        goto snd_soc_dapm_sync_err;
    }

    /* Headset jack detection */
    ret = snd_soc_jack_new(codec, "Headset Jack", SND_JACK_HEADSET, &g_hs_jack);
    if (ret) {
        pr_err("%s : jack new error, errornum = %d\n", __FUNCTION__, ret);
        goto snd_soc_jack_new_err;
    }

    /*ret = snd_soc_jack_add_pins(&hs_jack, ARRAY_SIZE(hs_jack_pins),
                hs_jack_pins);*/

#ifdef CONFIG_MFD_HI6421_IRQ
    hi6421_hs_btn_regester(codec, &g_hs_jack, SND_JACK_HEADSET);

    wake_lock_init(&priv->wake_lock, WAKE_LOCK_SUSPEND, "hi6421");
    wake_lock_init(&priv->plugin_wake_lock, WAKE_LOCK_SUSPEND, "hi6421_plugin");

    /* switch-class based headset detection */
    hi6421_hs_jack = &priv->hs_jack;
    hi6421_hs_jack->sdev.name = "h2w";
    ret = switch_dev_register(&hi6421_hs_jack->sdev);

    if (ret) {
        pr_err("%s(%u):error registering switch device %d\n", __FUNCTION__, __LINE__, ret);
        goto snd_soc_err;
    }

#ifdef CONFIG_K3_ADC

    hi6421_hkadc_ctl_probe(codec, true);
    usleep_range(HI6421_HKADC_MIN_WAIT_TIME_US,HI6421_HKADC_MAX_WAIT_TIME_US);
    priv->check_voltage = k3_adc_get_value(ADC_500K, NULL);
    pr_info("%s(%u) : check_voltage = %d",  __FUNCTION__,__LINE__,priv->check_voltage);
    if(priv->check_voltage < 0) {
        priv->check_voltage = 0;
    }
    hi6421_hkadc_ctl_probe(codec, false);
#endif

    priv->pll_delay_wq =
        create_singlethread_workqueue("pll_delay_wq");
    if (!(priv->pll_delay_wq)) {
        pr_err("%s(%u) : workqueue create failed", __FUNCTION__,__LINE__);
        ret = -ENOMEM;
        goto pll_wq_err;
    }
    INIT_DELAYED_WORK(&priv->pll_delay_work, hi6421_pll_work_func);

    priv->headset_plug_in_delay_wq =
        create_singlethread_workqueue("headset_plug_in_delay_wq");
    if (!(priv->headset_plug_in_delay_wq)) {
        pr_err("%s(%u) : workqueue create failed", __FUNCTION__,__LINE__);
        ret = -ENOMEM;
        goto headset_plug_in_wq_err;
    }
    INIT_DELAYED_WORK(&priv->headset_plug_in_delay_work, hi6421_audio_plug_in_work_func);

    priv->headset_plug_out_delay_wq =
        create_singlethread_workqueue("headset_plug_out_delay_wq");
    if (!(priv->headset_plug_out_delay_wq)) {
        pr_err("%s(%u) : workqueue create failed", __FUNCTION__,__LINE__);
        ret = -ENOMEM;
        goto headset_plug_out_wq_err;
    }
    INIT_DELAYED_WORK(&priv->headset_plug_out_delay_work, hi6421_audio_plug_out_work_func);

    priv->headset_judgement_delay_wq =
        create_singlethread_workqueue("headset_judgement_delay_wq");
    if (!(priv->headset_judgement_delay_wq)) {
        pr_err("%s(%u) : workqueue create failed", __FUNCTION__,__LINE__);
        ret = -ENOMEM;
        goto headset_judgement_wq_err;
    }
    INIT_DELAYED_WORK(&priv->headset_judgement_delay_work, hi6421_audio_judgement_work_func);

#ifdef CONFIG_K3_ADC
    priv->headset_btn_down_delay_wq = create_singlethread_workqueue("headset_btn_down_delay_wq");
    if (!(priv->headset_btn_down_delay_wq)) {
        pr_err("%s(%u) : workqueue create failed", __FUNCTION__,__LINE__);
        ret = -ENOMEM;
        goto headset_btn_down_wq_err;
    }
    INIT_DELAYED_WORK(&priv->headset_btn_down_delay_work, hi6421_btn_down_work_func);

    priv->headset_btn_up_delay_wq = create_singlethread_workqueue("headset_btn_up_delay_wq");
    if (!(priv->headset_btn_up_delay_wq)) {
        pr_err("%s(%u) : workqueue create failed", __FUNCTION__,__LINE__);
        ret = -ENOMEM;
        goto headset_btn_up_wq_err;
    }
    INIT_DELAYED_WORK(&priv->headset_btn_up_delay_work, hi6421_btn_up_work_func);
#endif
    /* clean interrupts of plug in&out */
    hi6421_set_reg_bit(HI6421_REG_IRQ2, HI6421_HDS_PLUG_IN_BIT);
    hi6421_set_reg_bit(HI6421_REG_IRQ2, HI6421_HDS_PLUG_OUT_BIT);

    ret = request_irq(pdata->irq[0], hi6421_hp_plug_out_handler,
        IRQF_SHARED |IRQF_NO_SUSPEND, "plug_out", codec);

    if (ret) {
        pr_err("%s(%u):IRQ_HEADSET_PLUG_OUT irq error %d\n", __FUNCTION__,__LINE__,ret);
        goto hi6421_hp_plug_out_err;
    }

    ret = request_irq(pdata->irq[1], hi6421_hp_plug_in_handler,
        IRQF_SHARED |IRQF_NO_SUSPEND, "plug_in", codec);  /*IRQ_HEADSET_PLUG_IN*/
    if (ret) {
        pr_err("%s(%u):IRQ_HEADSET_PLUG_IN irq error %d\n", __FUNCTION__,__LINE__,ret);
        goto work_irq_plug_in_err;
    }
#ifdef CONFIG_K3_ADC
    /*clean interrupts of btn down&up*/
    hi6421_set_reg_bit(HI6421_REG_IRQ3, HI6421_HDS_BTN_PRESS_BIT);
    hi6421_set_reg_bit(HI6421_REG_IRQ3, HI6421_HDS_BTN_RELEASE_BIT);

    ret = request_irq(pdata->irq[2], hi6421_btn_down_handler,
        IRQF_SHARED |IRQF_NO_SUSPEND, "press_down", codec);

    if (ret) {
        pr_err("%s(%u):IRQ_BSD_BTN_PRESS_DOWN irq error %d\n", __FUNCTION__,__LINE__,ret);
        goto work_irq_press_down_err;
    }

    ret = request_irq(pdata->irq[3], hi6421_btn_up_handler,
        IRQF_SHARED |IRQF_NO_SUSPEND, "press_up", codec);

    if (ret) {
        pr_err("%s(%u):IRQ_BSD_BTN_PRESS_UP irq error %d\n", __FUNCTION__,__LINE__,ret);
        goto work_irq_press_up_err;
    }

    mask_irq_of_btn( codec, true);

#endif
    queue_delayed_work(priv->headset_plug_in_delay_wq,
                       &priv->headset_plug_in_delay_work,
                       msecs_to_jiffies(0));
#endif

#if DUMP_REG
    dump_reg(codec);
#endif

    return ret;
#ifdef CONFIG_MFD_HI6421_IRQ
#ifdef CONFIG_K3_ADC
work_irq_press_up_err:
    free_irq(pdata->irq[2],codec);  /*IRQ_BSD_BTN_PRESS_DOWN*/
work_irq_press_down_err:
    free_irq(pdata->irq[1],codec);  /*IRQ_HEADSET_PLUG_IN*/
#endif
work_irq_plug_in_err:
    free_irq(pdata->irq[0],codec);  /*IRQ_HEADSET_PLUG_OUT*/
hi6421_hp_plug_out_err:
#ifdef CONFIG_K3_ADC
    if(priv->headset_btn_up_delay_wq) {
        cancel_delayed_work(&priv->headset_btn_up_delay_work);
        flush_workqueue(priv->headset_btn_up_delay_wq);
        destroy_workqueue(priv->headset_btn_up_delay_wq);
    }
headset_btn_up_wq_err:
    if(priv->headset_btn_down_delay_wq) {
        cancel_delayed_work(&priv->headset_btn_down_delay_work);
        flush_workqueue(priv->headset_btn_down_delay_wq);
        destroy_workqueue(priv->headset_btn_down_delay_wq);
    }
headset_btn_down_wq_err:
#endif
    if(priv->headset_plug_out_delay_wq) {
        cancel_delayed_work(&priv->headset_plug_out_delay_work);
        flush_workqueue(priv->headset_plug_out_delay_wq);
        destroy_workqueue(priv->headset_plug_out_delay_wq);
    }
headset_plug_out_wq_err:
    if(priv->headset_plug_in_delay_wq) {
        cancel_delayed_work(&priv->headset_plug_in_delay_work);
        flush_workqueue(priv->headset_plug_in_delay_wq);
        destroy_workqueue(priv->headset_plug_in_delay_wq);
    }
headset_plug_in_wq_err:
    if(priv->headset_judgement_delay_wq) {
        cancel_delayed_work(&priv->headset_judgement_delay_work);
        flush_workqueue(priv->headset_judgement_delay_wq);
        destroy_workqueue(priv->headset_judgement_delay_wq);
    }
headset_judgement_wq_err:
    if(priv->pll_delay_wq) {
        cancel_delayed_work(&priv->pll_delay_work);
        flush_workqueue(priv->pll_delay_wq);
        destroy_workqueue(priv->pll_delay_wq);
    }
pll_wq_err:
    switch_dev_unregister(&hi6421_hs_jack->sdev);
snd_soc_err:
    wake_lock_destroy(&priv->wake_lock);
    wake_lock_destroy(&priv->plugin_wake_lock);
#endif
    regulator_disable(g_regu);
snd_soc_jack_new_err:
snd_soc_dapm_sync_err:
regulator_err:
    regulator_put(g_regu);

    if (!IS_ERR(g_clk)) {
        clk_disable(g_clk);
        clk_put(g_clk);
    }
    kfree(priv);
    g_codec = NULL;
    return ret;
}

static int hi6421_codec_remove(struct snd_soc_codec *codec)
{
    struct hi6421_data *priv = NULL;
#ifdef CONFIG_MFD_HI6421_IRQ
    struct hi6421_jack_data *hi6421_hs_jack = NULL;
    struct hi6421_codec_platform_data *pdata = NULL;
#endif
    /*pr_info("Entry %s\n",__FUNCTION__);*/

    if (codec != NULL) {
#ifdef CONFIG_MFD_HI6421_IRQ
        pdata = dev_get_platdata(codec->dev);

        if (pdata != NULL) {
#ifdef CONFIG_K3_ADC
            free_irq(pdata->irq[2], codec); /*IRQ_BSD_BTN_PRESS_DOWN*/
            free_irq(pdata->irq[3], codec); /*IRQ_BSD_BTN_PRESS_UP*/
#endif
            free_irq(pdata->irq[0], codec); /*IRQ_HEADSET_PLUG_OUT*/
            free_irq(pdata->irq[1], codec); /*IRQ_HEADSET_PLUG_IN*/
        }
#endif
        priv = snd_soc_codec_get_drvdata(codec);
        if (priv != NULL) {
#ifdef CONFIG_MFD_HI6421_IRQ
#ifdef CONFIG_K3_ADC
           if(priv->headset_btn_down_delay_wq) {
                cancel_delayed_work(&priv->headset_btn_down_delay_work);
                flush_workqueue(priv->headset_btn_down_delay_wq);
                destroy_workqueue(priv->headset_btn_down_delay_wq);
           }
#endif

           if(priv->headset_plug_in_delay_wq) {
                cancel_delayed_work(&priv->headset_plug_in_delay_work);
                flush_workqueue(priv->headset_plug_in_delay_wq);
                destroy_workqueue(priv->headset_plug_in_delay_wq);
            }

           if(priv->headset_plug_out_delay_wq) {
                cancel_delayed_work(&priv->headset_plug_out_delay_work);
                flush_workqueue(priv->headset_plug_out_delay_wq);
                destroy_workqueue(priv->headset_plug_out_delay_wq);
            }

            if(priv->pll_delay_wq) {
                cancel_delayed_work(&priv->pll_delay_work);
                flush_workqueue(priv->pll_delay_wq);
                destroy_workqueue(priv->pll_delay_wq);
            }

            hi6421_hs_jack = &priv->hs_jack;
            switch_dev_unregister(&hi6421_hs_jack->sdev);
            wake_lock_destroy(&priv->wake_lock);
            wake_lock_destroy(&priv->plugin_wake_lock);
#endif
            kfree(priv);
        }
        kfree(codec);
    }
    g_codec = NULL;
    regulator_disable(g_regu);
    return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_hi6421 = {
    .probe          = hi6421_codec_probe,
    .remove         = hi6421_codec_remove,
    .read           = hi6421_reg_read,
    .write          = hi6421_reg_write,
};

static int __devinit hi6421_probe(struct platform_device *pdev)
{
    int ret = -ENODEV;
    struct resource* res;

    pr_info("%s\n",__FUNCTION__);

    /* get pmuspi clock */
    g_clk = clk_get(NULL, "clk_pmuspi");
    if (IS_ERR(g_clk)) {
        pr_err("%s : Could not get clk_pmuspi\n", __FUNCTION__);
        return -ENODEV;
    }

    /* get audio_ldo */
    g_regu = regulator_get(&pdev->dev, "audio-vcc");
    if (IS_ERR(g_regu)) {
        pr_err("%s: Could not get regulator : audio-vcc\n", __FUNCTION__);
        goto probe_err;
    }

    /* get pmuspi register base address */
    res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "hi6421_base");
    if (!res) {
        pr_err("%s: Could not get register base\n", __FUNCTION__);
        goto register_err;
    }

    g_hi6421_reg_base_addr = (unsigned int)ioremap(res->start, resource_size(res));

    ret = snd_soc_register_codec(&pdev->dev, &soc_codec_dev_hi6421,
                                    &hi6421_dai, 1);
    if (ret < 0) {
        pr_err("%s: snd_soc_register_codec failed\n", __FUNCTION__);
        goto register_err;
    }

    return ret;

register_err:
    regulator_put(g_regu);
probe_err:
    clk_put(g_clk);

    return ret;
}

static int __devexit hi6421_remove(struct platform_device *pdev)
{
    pr_info("%s\n",__FUNCTION__);

    snd_soc_unregister_codec(&pdev->dev);

    if (!IS_ERR(g_regu))
        regulator_put(g_regu);

    if (!IS_ERR(g_clk))
        clk_put(g_clk);

    return 0;
}

void hi6421_lock_sysclk(bool lock)
{
    /*pr_info("%s: status = %s\n", __FUNCTION__, lock ? "lock" : "unlock");*/
    hi6421_reg_write(g_codec, HI6421_REG_CLK_CTL, lock ? 0x6 : 0);
}
EXPORT_SYMBOL_GPL(hi6421_lock_sysclk);

static struct platform_driver hi6421_driver = {
    .driver = {
        .name  = "hi6421-codec",
        .owner = THIS_MODULE,
    },
    .probe  = hi6421_probe,
    .remove = __devexit_p(hi6421_remove),
};

static int __init hi6421_codec_init(void)
{
    pr_info("%s\n",__FUNCTION__);
    return platform_driver_register(&hi6421_driver);
}
module_init(hi6421_codec_init);

static void __exit hi6421_codec_exit(void)
{
    pr_info("%s\n",__FUNCTION__);
    platform_driver_unregister(&hi6421_driver);
}
module_exit(hi6421_codec_exit);

MODULE_DESCRIPTION("ASoC Hi6421 driver");
MODULE_AUTHOR("c00166660");
MODULE_LICENSE("GPL");
