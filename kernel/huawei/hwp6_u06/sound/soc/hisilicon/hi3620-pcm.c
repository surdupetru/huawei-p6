#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#define LOG_TAG "hi3620-pcm"
#include "hi3620-pcm.h"
#include "hi3620_log.h"

#ifdef HDMI_DISPLAY
#include "../../../drivers/video/k3/hdmi/k3_hdmi.h"
#endif

/*
* PLAYBACK SUPPORT FORMATS
* BITS : 8/16/24  18/20
* LITTLE_ENDIAN / BIG_ENDIAN
* MONO / STEREO
* UNSIGNED / SIGNED
*/
#define HI3620_PB_FORMATS  (SNDRV_PCM_FMTBIT_S8 | \
                            SNDRV_PCM_FMTBIT_U8 | \
                            SNDRV_PCM_FMTBIT_S16_LE | \
                            SNDRV_PCM_FMTBIT_S16_BE | \
                            SNDRV_PCM_FMTBIT_U16_LE | \
                            SNDRV_PCM_FMTBIT_U16_BE | \
                            SNDRV_PCM_FMTBIT_S24_LE | \
                            SNDRV_PCM_FMTBIT_S24_BE | \
                            SNDRV_PCM_FMTBIT_U24_LE | \
                            SNDRV_PCM_FMTBIT_U24_BE)
/*
*                           SNDRV_PCM_FMTBIT_S20_3LE |
*                           SNDRV_PCM_FMTBIT_S20_3BE |
*                           SNDRV_PCM_FMTBIT_U20_3LE |
*                           SNDRV_PCM_FMTBIT_U20_3BE |
*                           SNDRV_PCM_FMTBIT_S18_3LE |
*                           SNDRV_PCM_FMTBIT_S18_3BE |
*                           SNDRV_PCM_FMTBIT_U18_3LE |
*                           SNDRV_PCM_FMTBIT_U18_3BE
*/

/*
* PLAYBACK SUPPORT RATES
* 8/11.025/16/22.05/32/44.1/48/88.2/96kHz
*/
#define HI3620_PB_RATES    (SNDRV_PCM_RATE_8000_48000 | \
                            SNDRV_PCM_RATE_88200 | \
                            SNDRV_PCM_RATE_96000)

#define HI3620_PB_MIN_CHANNELS  ( 1 )
#define HI3620_PB_MAX_CHANNELS  ( 2 )
#define HI3620_PB_FIFO_SIZE     ( SIO_TX_FIFO_SIZE )

/* CAPTURE SUPPORT FORMATS : SIGNED 16/24bit */
#define HI3620_CP_FORMATS  ( SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE)

/* CAPTURE SUPPORT RATES : 48/96kHz */
#define HI3620_CP_RATES    ( SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000 )

#define HI3620_CP_MIN_CHANNELS  ( 2 )
#define HI3620_CP_MAX_CHANNELS  ( 2 )
#define HI3620_CP_FIFO_SIZE     ( SIO_RX_FIFO_SIZE )

#define HI3620_MODEM_RATES      ( SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 )
#define HI3620_BT_RATES         ( SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 )
#define HI3620_FM_RATES         ( SNDRV_PCM_RATE_48000 )

#define HI3620_MAX_BUFFER_SIZE  ( 128 * 1024 )    /* 0x20000 */
#define HI3620_BUFFER_SIZE_MM   ( 32 * 1024 )
#define HI3620_MIN_BUFFER_SIZE  ( 32 )
#define HI3620_MAX_PERIODS      ( 32 )
#define HI3620_MIN_PERIODS      ( 2 )

/* sleep time to check DMA & IRS state */
#define HI3620_MIN_DMA_TIME_US  ( 22000 ) /*time of one period*/
#define HI3620_MAX_DMA_TIME_US  ( 25000 )
#define HI3620_MIN_STOP_DMA_TIME_US ( 1000 )
#define HI3620_MAX_STOP_DMA_TIME_US ( 2000 )

#define SPDIF_OUTPUT_ONLY       ( 2 )
#define SPDIF_OUTPUT_ENABLE     ( 1 )

static DEFINE_SPINLOCK(g_lock);

static const unsigned int freq[] = {
    8000,   11025,  12000,  16000,
    22050,  24000,  32000,  44100,
    48000,  88200,  96000,  176400,
    192000,
};

#define HI3620_RATE_INDEX_32000       6
#define HI3620_RATE_INDEX_44100       7
#define HI3620_RATE_INDEX_48000       8
#define HI3620_RATE_INDEX_96000       10
#define HI3620_RATE_INDEX_176400      11
#define HI3620_RATE_INDEX_192000      12
#define HI3620_RATE_48000       ( 48000 )

static AUDIO_CLK_STRU g_clk_asp_hdmi = {0};

/* HI3620 ASP IRQ */
static unsigned int g_hi3620_asp_irq_line = -1;

/* HI3620 ASP REGISTER BASE ADDR */
static unsigned int g_hi3620_asp_reg_base_addr = -1;

struct proc_dir_entry *audio_pcm_dir;
//add for handling an asynchronous work at one DMA interrupt [end]
static int g_digitaloutStatus = 0; //indicate the current audio digital out scene.
static int g_spdifout = 0;  //indicate the spdif output is or not enable, disabled by default
static volatile bool g_tx0fadeoutstate = false;  // false: fadein , true:fadeout
static volatile bool g_tx0fadeoutcmd = false;  // false: fadein , true:fadeout

static struct snd_soc_dai_driver hi3620_dai[] = {
    {
        .name = "hi3620-mm",
        .playback = {
            .stream_name  = "hi3620-mm Playback",
            .channels_min = HI3620_PB_MIN_CHANNELS,
            .channels_max = HI3620_PB_MAX_CHANNELS,
            .rates        = HI3620_PB_RATES,
            .formats      = HI3620_PB_FORMATS},
        .capture = {
            .stream_name  = "hi3620-mm Capture",
            .channels_min = HI3620_CP_MIN_CHANNELS,
            .channels_max = HI3620_CP_MAX_CHANNELS,
            .rates        = HI3620_CP_RATES,
            .formats      = HI3620_CP_FORMATS
        },
    },
    {
        .name = "hi3620-modem",
        .playback = {
            .stream_name  = "hi3620-modem Playback",
            .channels_min = HI3620_PB_MIN_CHANNELS,
            .channels_max = HI3620_PB_MAX_CHANNELS,
            .rates        = HI3620_MODEM_RATES,
            .formats      = HI3620_PB_FORMATS
        },
    },
    {
        .name = "hi3620-fm",
        .playback = {
            .stream_name  = "hi3620-fm Playback",
            .channels_min = HI3620_PB_MIN_CHANNELS,
            .channels_max = HI3620_PB_MAX_CHANNELS,
            .rates        = HI3620_FM_RATES,
            .formats      = HI3620_PB_FORMATS
        },
    },
    {
        .name = "hi3620-bt",
        .playback = {
            .stream_name  = "hi3620-bt Playback",
            .channels_min = HI3620_PB_MIN_CHANNELS,
            .channels_max = HI3620_PB_MAX_CHANNELS,
            .rates        = HI3620_BT_RATES,
            .formats      = HI3620_PB_FORMATS},
    },
};

/* define the capability of playback channel */
static const struct snd_pcm_hardware hi3620_hardware_playback = {
    .info             = SNDRV_PCM_INFO_INTERLEAVED
                      | SNDRV_PCM_INFO_MMAP
                      | SNDRV_PCM_INFO_MMAP_VALID,
    .formats          = SNDRV_PCM_FORMAT_S16_LE,
    .channels_min     = HI3620_PB_MIN_CHANNELS,
    .channels_max     = HI3620_PB_MAX_CHANNELS,
    .buffer_bytes_max = HI3620_MAX_BUFFER_SIZE,
    .period_bytes_min = HI3620_MIN_BUFFER_SIZE,
    .period_bytes_max = HI3620_MAX_BUFFER_SIZE,
    .periods_min      = HI3620_MIN_PERIODS,
    .periods_max      = HI3620_MAX_PERIODS,
    .fifo_size        = HI3620_PB_FIFO_SIZE,
};

/* define the capability of capture channel */
static const struct snd_pcm_hardware hi3620_hardware_capture = {
    .info             = SNDRV_PCM_INFO_INTERLEAVED,
    .formats          = SNDRV_PCM_FORMAT_S16_LE,
    .rates            = SNDRV_PCM_RATE_48000,
    .channels_min     = HI3620_CP_MIN_CHANNELS,
    .channels_max     = HI3620_CP_MAX_CHANNELS,
    .buffer_bytes_max = HI3620_MAX_BUFFER_SIZE,
    .period_bytes_min = HI3620_MIN_BUFFER_SIZE,
    .period_bytes_max = HI3620_MAX_BUFFER_SIZE,
    .periods_min      = HI3620_MIN_PERIODS,
    .periods_max      = HI3620_MAX_PERIODS,
    .fifo_size        = HI3620_CP_FIFO_SIZE,
};

/* define the capability of playback channel for Modem */
static const struct snd_pcm_hardware hi3620_hardware_modem_playback = {
    .info             = SNDRV_PCM_INFO_INTERLEAVED
                      | SNDRV_PCM_INFO_NONINTERLEAVED
                      | SNDRV_PCM_INFO_BLOCK_TRANSFER
                      | SNDRV_PCM_INFO_PAUSE,
    .formats          = SNDRV_PCM_FORMAT_S16_LE,
    .channels_min     = HI3620_PB_MIN_CHANNELS,
    .channels_max     = HI3620_PB_MAX_CHANNELS,
    .buffer_bytes_max = HI3620_MAX_BUFFER_SIZE,
    .period_bytes_min = HI3620_MIN_BUFFER_SIZE,
    .period_bytes_max = HI3620_MAX_BUFFER_SIZE,
    .periods_min      = HI3620_MIN_PERIODS,
    .periods_max      = HI3620_MAX_PERIODS,
    .fifo_size        = HI3620_PB_FIFO_SIZE,
};

static inline bool is_addr_valid(unsigned int addr)
{
    if (ASP_FIRST_REG <= addr && ASP_LAST_REG >= addr)
        return true;

    if (SIO0_FIRST_REG <= addr && SIO0_LAST_REG >= addr)
        return true;

    if (SIO1_FIRST_REG <= addr && SIO1_LAST_REG >= addr)
        return true;

    if (SPDIF_FIRST_REG <= addr && SPDIF_LAST_REG >= addr)
        return true;

    return false;
}

static inline int hi3620_reg_read(unsigned int addr)
{
    BUG_ON(!is_addr_valid(addr));
    return readl_mutex(g_hi3620_asp_reg_base_addr + addr);
}

static inline void hi3620_reg_write(unsigned int value, unsigned int addr)
{
    BUG_ON(!is_addr_valid(addr));
    writel_asp_mutex(value, g_hi3620_asp_reg_base_addr + addr);
}

static inline void hi3620_set_bits(unsigned int value, unsigned int addr)
{
    unsigned int reg_val = 0;
    unsigned long flags = 0;
    spin_lock_irqsave(&g_lock, flags);
    reg_val = hi3620_reg_read(addr);
    reg_val |= value;
    hi3620_reg_write(reg_val, addr);
    spin_unlock_irqrestore(&g_lock, flags);
}

static inline void hi3620_clr_bits(unsigned int value, unsigned int addr)
{
    unsigned int reg_val = 0;
    unsigned long flags = 0;
    spin_lock_irqsave(&g_lock, flags);
    reg_val = hi3620_reg_read(addr);
    reg_val &= ~value;
    hi3620_reg_write(reg_val, addr);
    spin_unlock_irqrestore(&g_lock, flags);
}

/* config dma start address & data length, then enable dma */
static void enable_dma(unsigned int dma, unsigned int addr,
            int period_next, int size)
{
    unsigned int dma_addr_reg = 0;
    unsigned int dma_len_reg  = 0;

    switch (dma) {
    case TX0_DMA_A :
        dma_addr_reg = ASP_TX0ASAR;
        dma_len_reg  = ASP_TX0ADLR;
        break;

    case TX0_DMA_B :
        dma_addr_reg = ASP_TX0BSAR;
        dma_len_reg  = ASP_TX0BDLR;
        break;

    case TX1_DMA_A :
        dma_addr_reg = ASP_TX1ASAR;
        dma_len_reg  = ASP_TX1ADLR;
        break;

    case TX1_DMA_B :
        dma_addr_reg = ASP_TX1BSAR;
        dma_len_reg  = ASP_TX1BDLR;
        break;

    case RX_DMA_A :
        dma_addr_reg = ASP_RXASAR;
        dma_len_reg  = ASP_RXADLR;
        break;

    case RX_DMA_B :
        dma_addr_reg = ASP_RXBSAR;
        dma_len_reg  = ASP_RXBDLR;
        break;

    default :
        loge("%s : error DMA num : %#x", __FUNCTION__, dma);
        break;
    }

    hi3620_reg_write(addr + period_next * size, dma_addr_reg);
    hi3620_reg_write(size, dma_len_reg);
    hi3620_reg_write(dma, ASP_DER);
}

static void wait_dma_stop(unsigned int bits)
{
    unsigned int der = 0;
    unsigned int irsr = 0;
    unsigned int i = 3;

    /*don't use ASP_DSTOP whick can create delayed interrupt*/
    do {
        der = hi3620_reg_read(ASP_DER);
        irsr = hi3620_reg_read(ASP_IRSR);
        if ((der | irsr) & bits) {
            usleep_range(HI3620_MIN_DMA_TIME_US,
                HI3620_MAX_DMA_TIME_US);
        } else {
            break;
        }
    } while(--i);

    if (!i)
        loge("%s exit with der=%#x, irsr=%#x\n", __FUNCTION__, der, irsr);
}

static void check_and_stop_rxdma(void)
{
    unsigned int der = 0;
    unsigned int irsr = 0;
    unsigned int i = 3;

    do {
        der = hi3620_reg_read(ASP_DER) & RX_DMAS;
        irsr = hi3620_reg_read(ASP_IRSR) & RX_DMAS;
        if (der | irsr) {
            hi3620_reg_write(RX_DMAS, ASP_DSTOP);
            usleep_range(HI3620_MIN_STOP_DMA_TIME_US,
                HI3620_MAX_STOP_DMA_TIME_US);
        } else {
            break;
        }
    } while(--i);

    if (!i)
        loge("%s exit with der=%#x, irsr=%#x\n", __FUNCTION__, der, irsr);
}

void hi3620_pcm_fadein_internal(bool bfadeout,bool binternal) {
    if (binternal == false) {
        g_tx0fadeoutcmd = bfadeout;
    }
    if (bfadeout == false) {
        if (hi3620_reg_read(ASP_FADEOUTEN0) == 0) {
            hi3620_reg_write(0x200, ASP_FADERATE0);
            hi3620_reg_write(1, ASP_FADEINEN0);
        }
    } else {
        if (hi3620_reg_read(ASP_FADEINEN0) == 0) {
            hi3620_reg_write(0, ASP_FADERATE0);
            hi3620_reg_write(1, ASP_FADEOUTEN0);
        }
    }
}
void hi3620_pcm_hdmi_event(bool hdmiin,bool beforeNotify) {
    if (hdmiin == true) {
        (void)clk_enable(g_clk_asp_hdmi.clk_hdmi_m);
        (void)clk_enable(g_clk_asp_hdmi.clk_asp);
        (void)clk_enable(g_clk_asp_hdmi.clk_aspsio);
        (void)clk_enable(g_clk_asp_hdmi.clk_aspspdif);
        /*if beforeNotify is true, fadeout; if beforeNotify is false, fadein;*/
        hi3620_pcm_fadein_internal(beforeNotify, false);
    } else {
        clk_disable(g_clk_asp_hdmi.clk_hdmi_m);
        clk_disable(g_clk_asp_hdmi.clk_asp);
        clk_disable(g_clk_asp_hdmi.clk_aspsio);
        clk_disable(g_clk_asp_hdmi.clk_aspspdif);
        hi3620_pcm_fadein_internal(false, false);
    }
}
EXPORT_SYMBOL(hi3620_pcm_hdmi_event);

/* handle one TX DMA interrupt at a time */
static irqreturn_t hi3620_intr_handle_pb(struct snd_pcm_substream *substream)
{
    struct hi3620_runtime_data *prtd = substream->runtime->private_data;
    unsigned int period_size = 0;
    unsigned int rt_period_size = substream->runtime->period_size;
    unsigned int num_period = substream->runtime->periods;
    snd_pcm_uframes_t avail = 0;
    int txindex = 0;
    int dmacur = 0;
    int dmanext = 0;
    /*second channel status*/
    int sec_dmacur = 0;
    int sec_dmanext = 0;
    int sec_curusing = 0;
    int sec_nextusing = 0;
    unsigned int dma_enabled = 0;

    unsigned int irs = hi3620_reg_read(ASP_IRSR) & (TX0_DMAS  | TX1_DMAS);

    if (NULL == prtd) {
        loge("%s PCM = NULL \n", __FUNCTION__);
        /* CLEAR ALL TX DMA INTERRUPT */
        hi3620_reg_write(irs, ASP_ICR);
        return IRQ_HANDLED;
    }

    if (prtd->tx[0].dmas == (irs & prtd->tx[0].dmas))
        logd("%s : TWO INTS COME TOGETHER::PLAYBACK\n", __FUNCTION__);
    if ((prtd->tx[1].dmas != 0)
        && (prtd->tx[1].dmas == (irs & prtd->tx[1].dmas)))
        logd("%s : TWO INTS2 COME TOGETHER::PLAYBACK\n", __FUNCTION__);

    if (0 == (irs & prtd->tx[0].dmas)) {
            loge("%s : unexpected interrupt\n",__FUNCTION__);
            hi3620_reg_write(irs, ASP_ICR);
            return IRQ_HANDLED;
    }

    period_size = prtd->period_size;

    dma_enabled = hi3620_reg_read(ASP_DER);
    if (irs & prtd->tx[0].dmaa) {
        txindex = 0;
        dmacur = prtd->tx[0].dmaa;
        dmanext = prtd->tx[0].dmab;
        sec_dmacur = prtd->tx[1].dmaa;
        sec_dmanext = prtd->tx[1].dmab;
        sec_curusing = dma_enabled & sec_dmacur;
        sec_nextusing = dma_enabled & sec_dmanext;
    } else {
        txindex = 0;
        dmacur = prtd->tx[0].dmab;
        dmanext = prtd->tx[0].dmaa;
        sec_dmacur = prtd->tx[1].dmab;
        sec_dmanext = prtd->tx[1].dmaa;
        sec_curusing = dma_enabled & sec_dmacur;
        sec_nextusing = dma_enabled & sec_dmanext;
    }
    hi3620_reg_write(dmacur, ASP_ICR);
    prtd->tx[txindex].period_cur++;

    spin_lock(&prtd->lock);
    prtd->period_cur = prtd->tx[0].period_cur % num_period;
    spin_unlock(&prtd->lock);

    snd_pcm_period_elapsed(substream);

    spin_lock(&prtd->lock);

    /* DMA IS STOPPED CLEAR INTERRUPT */
    if (STATUS_STOP == prtd->status) {
        logd("%s : stop dma, irs = %#x\n", __FUNCTION__, irs);
        hi3620_reg_write(irs, ASP_ICR);
        spin_unlock(&prtd->lock);
        return IRQ_HANDLED;
    }

    avail = snd_pcm_playback_hw_avail(substream->runtime);

    if(avail < rt_period_size) {
        prtd->tx[0].two_dma_flag = false;
        logd("Run out of data in both DMAs, disable both DMAs\n");
        spin_unlock(&prtd->lock);
        return IRQ_HANDLED;
    } else {
        //start 2rd-ch dma when last request completed
        if ((prtd->use2tx_flag == true) && (sec_curusing == 0)) {
            enable_dma(sec_dmacur, substream->runtime->dma_addr,
                        prtd->tx[txindex].period_next, period_size);
        } else if (prtd->use2tx_flag == true) {
            loge("2rd-ch slow\n");
        }
        /* avail >= rt_period_size, enable one dma at least */
        enable_dma(dmacur, substream->runtime->dma_addr,
                    prtd->tx[txindex].period_next, period_size);
        prtd->tx[txindex].period_next = (prtd->tx[txindex].period_next + 1) % num_period;

        if ((!prtd->tx[txindex].two_dma_flag) && (avail >= rt_period_size * 2 )) {
            logd("enable both DMAs\n");
            /* enable DMA B */
            prtd->tx[txindex].two_dma_flag = true;

            if ((prtd->use2tx_flag == true) && (sec_nextusing == 0)) {
                enable_dma(sec_dmanext, substream->runtime->dma_addr,
                            prtd->tx[txindex].period_next, period_size);
            } else if (prtd->use2tx_flag == true) {
                loge("2rd-ch slow\n");
            }
            enable_dma(dmanext, substream->runtime->dma_addr,
                        prtd->tx[txindex].period_next, period_size);
            prtd->tx[txindex].period_next = (prtd->tx[txindex].period_next + 1) % num_period;
        }
    }

    spin_unlock(&prtd->lock);

    return IRQ_HANDLED;
}

/* handle one RX DMA interrupt at a time */
static irqreturn_t hi3620_intr_handle_cp(struct snd_pcm_substream *substream)
{
    struct hi3620_runtime_data *prtd = substream->runtime->private_data;
    unsigned int period_size = 0;
    unsigned int rt_period_size = substream->runtime->period_size;
    unsigned int num_period = substream->runtime->periods;
    snd_pcm_uframes_t avail = 0;

    unsigned int irs = hi3620_reg_read(ASP_IRSR) & (RX_DMAS);

    if (RX_DMAS == irs)
        logd("%s : TWO INTS COME TOGETHER::CAPTURE\n", __FUNCTION__);

    if (NULL == prtd) {
        loge("%s PCM = NULL \n", __FUNCTION__);
        /* CLEAR ALL TX DMA INTERRUPT */
        hi3620_reg_write(irs, ASP_ICR);
        return IRQ_HANDLED;
    }

    period_size = prtd->period_size;

    spin_lock(&prtd->lock);
    prtd->period_cur = (prtd->period_cur+1) % num_period;
    spin_unlock(&prtd->lock);

    snd_pcm_period_elapsed(substream);

    spin_lock(&prtd->lock);

    /* DMA IS STOPPED CLEAR INTERRUPT */
    if (STATUS_STOP == prtd->status) {
        logd("%s : stop dma, irs = %#x\n", __FUNCTION__, irs);
        hi3620_reg_write(irs, ASP_ICR);
        spin_unlock(&prtd->lock);
        return IRQ_HANDLED;
    }

    avail = snd_pcm_capture_hw_avail(substream->runtime);

    if (irs & RX_DMA_A)
        hi3620_reg_write(RX_DMA_A, ASP_ICR);
    else
        hi3620_reg_write(RX_DMA_B, ASP_ICR);

    if(avail < rt_period_size) {
        if (prtd->two_dma_flag) {
            prtd->two_dma_flag = false;
            pr_warn("Run out of data in one DMA, disable one DMA\n");
        } else {
            logd("Run out of data in both DMAs, disable both DMAs\n");
        }

        spin_unlock(&prtd->lock);
        return IRQ_HANDLED;
    } else {
        /* avail >= rt_period_size, enable one dma at least */
        if (irs & RX_DMA_A) {
            enable_dma(RX_DMA_A, substream->runtime->dma_addr,
                        prtd->period_next, period_size);
            prtd->period_next = (prtd->period_next + 1) % num_period;

            if ((!prtd->two_dma_flag) && (avail >= rt_period_size * 2 )) {
                logd("enable both DMAs\n");
                /* enable DMA B */
                prtd->two_dma_flag = true;
                enable_dma(RX_DMA_B, substream->runtime->dma_addr,
                            prtd->period_next, period_size);
                prtd->period_next = (prtd->period_next + 1) % num_period;
            }
        } else {
            enable_dma(RX_DMA_B, substream->runtime->dma_addr,
                        prtd->period_next, period_size);
            prtd->period_next = (prtd->period_next + 1) % num_period;

            if((!prtd->two_dma_flag) && (avail >= rt_period_size * 2)) {
                logd("enable both DMAs\n");
                /* enable DMA A */
                prtd->two_dma_flag = true;
                enable_dma(RX_DMA_A, substream->runtime->dma_addr,
                            prtd->period_next, period_size);
                prtd->period_next = (prtd->period_next + 1) % num_period;
            }
        }
    }

    spin_unlock(&prtd->lock);

    return IRQ_HANDLED;
}

static irqreturn_t hi3620_intr_handle(int irq, void *dev_id)
{
    struct snd_pcm *pcm = dev_id;
    struct snd_pcm_substream *substream;
    unsigned int irs = hi3620_reg_read(ASP_IRSR);
    irqreturn_t ret = IRQ_NONE;
    unsigned int irs_valid = hi3620_reg_read(ASP_IER) & irs;

    /* clean unexpected interrupt*/
    if (irs != irs_valid) {
        irs = irs & (~irs_valid);
        hi3620_reg_write(irs, ASP_ICR);
        irs = irs_valid;
        ret = IRQ_HANDLED;
    }

    /* BUSERROR INTERRUPT */
    if (ASP_BUS_ERROR & irs) {
        loge("asp bus error\n");
        hi3620_reg_write(ASP_BUS_ERROR, ASP_ICR);
        ret = IRQ_HANDLED;
    }

    /*TX0 FADEIN-FADEOUT INTERRUPT */
    if ((TX0_FADEIN | TX0_FADEOUT) & irs) {
        if (TX0_FADEIN & irs) {
            hi3620_reg_write(TX0_FADEIN, ASP_ICR);
            g_tx0fadeoutstate = false;
        } else {
            hi3620_reg_write(TX0_FADEOUT, ASP_ICR);
            g_tx0fadeoutstate = true;
        }
        if (g_tx0fadeoutcmd != g_tx0fadeoutstate) {
            hi3620_pcm_fadein_internal(g_tx0fadeoutcmd, true);
        }
        logi("fadeoutstate %d fadeoutcmd %d  irs %x\n", g_tx0fadeoutstate,g_tx0fadeoutcmd, irs);
        ret = IRQ_HANDLED;
    }

    /* PLAYBACK INTERRUPT */
    if ((TX0_DMAS  | TX1_DMAS)& irs) {
        substream = pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream;
        if (substream) {
            ret = hi3620_intr_handle_pb(substream);
        } else {
            loge("%s : PLAYBACK is NULL\n",__FUNCTION__);
            hi3620_reg_write((TX0_DMAS  | TX1_DMAS) & irs, ASP_ICR);
            ret = IRQ_HANDLED;
        }
    }

    /* CAPTURE INTERRUPT */
    if ((RX_DMAS) & irs) {
        substream = pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream;
        if (substream) {
            ret = hi3620_intr_handle_cp(substream);
        } else {
            loge("%s : CAPTURE is NULL\n",__FUNCTION__);
            hi3620_reg_write((RX_DMAS) & irs, ASP_ICR);
            ret = IRQ_HANDLED;
        }
    }

    return ret;
}

static void hi3620_set_tx123_outfreq(unsigned int outfreq_index)
{
    unsigned int value = hi3620_reg_read(ASP_TX1);

    value = value & 0xff;
    /* set  output rate */
    // for passthrough, this register is invalid.
    if (outfreq_index > HI3620_RATE_INDEX_176400) {
        value |= TX_192K_OUTPUT_BIT;
    } else if (outfreq_index > HI3620_RATE_INDEX_48000) {
        value |= TX_96K_OUTPUT_BIT;
    }
    hi3620_reg_write(value, ASP_TX1);
}

/*get route for audio data*/
static unsigned int hi3620_get_aout_route(bool compressed,unsigned int infreq_index,unsigned int* outfreq_index,unsigned int channels)
{
    AUDIO_ROUTE_ENUM eroute = AUDIO_ROUTE_PB_MAX;

    //by default, input freq is equal to output freq
    *outfreq_index = infreq_index;

    //pcm
    if(freq[infreq_index] > HI3620_RATE_48000) {
        *outfreq_index = HI3620_RATE_INDEX_96000;
    } else {
        *outfreq_index = HI3620_RATE_INDEX_48000;
    }

    if (g_spdifout == SPDIF_OUTPUT_ONLY)
    {
        //only SPDIF output is enabled
        eroute = AUDIO_ROUTE_PCM_SPDIF_ONLY;
    } else if (g_spdifout == SPDIF_OUTPUT_ENABLE) {
        //SPDIF output is enabled
        if (g_digitaloutStatus == 2) {
            // only HDMI output is enabled
            eroute = AUDIO_ROUTE_PCM_SPDIF_ONLY;
        } else {
            // both SPDIF and DAC are enabled
            eroute = AUDIO_ROUTE_PCM_DAC_SPDIF;
        }
    } else {
        //SPDIF output is disabled
        switch(g_digitaloutStatus) {
        case 3:
            eroute = AUDIO_ROUTE_PCM_DAC_SPDIF;
            break;
        case 2:
            eroute = AUDIO_ROUTE_PCM_SPDIF_ONLY;
            break;
        case 0:
        default:
            eroute = AUDIO_ROUTE_PCM_DAC_ONLY;
            break;
        }
    }
    if (g_digitaloutStatus != 0)
    {
        (void)clk_set_rate(g_clk_asp_hdmi.clk_hdmi_m, freq[*outfreq_index]*256);
        (void)clk_set_rate(g_clk_asp_hdmi.clk_asphdmi,0xC42F36D);
    }
    return eroute;
}

static void hi3620_pcm_tx0_enable(unsigned int format, unsigned int infreq_index, unsigned int outfreq_index,struct hi3620_tx_runtime_data* tx)
{
    unsigned int value = 0;

    IN_FUNCTION;

    tx->dmaa = TX0_DMA_A;
    tx->dmab = TX0_DMA_B;
    tx->dmas = TX0_DMAS;

    /* Set SUPPORT RATE */
    hi3620_reg_write(infreq_index, ASP_TX0RSRR);

    value = format | HIGHBIT_IS_LEFT | TX0_EN_BIT;
    if (HI3620_RATE_INDEX_48000 < outfreq_index) {
        value |= TX_96K_OUTPUT_BIT; //bits same as tx0
    } else {
        value &= ~TX_96K_OUTPUT_BIT;
    }
    hi3620_reg_write(value, ASP_TX0);

    /* ENABLE TX0 DMA INTERRUPT */
    wait_dma_stop(TX0_DMAS);
    hi3620_reg_write(TX0_DMAS, ASP_ICR);
    hi3620_set_bits((TX0_DMAS | TX0_FADEIN | TX0_FADEOUT | ASP_BUS_ERROR), ASP_IER);

    /* CONFIG SIO0 INTERFACE FOR PB */
    hi3620_reg_write(SIO_TX_ENABLE | SIO_TX_FIFO, SIO0_I2S_CLR);//bits same as SIO0
    hi3620_set_bits((SIO_TX_ENABLE | SIO_TX_FIFO), SIO0_I2S_SET);

    OUT_FUNCTION;
}

static void hi3620_pcm_tx1_enable(unsigned int format, unsigned int infreq_index,unsigned int outfreq_index,
                                                                    bool enable_irq, struct hi3620_tx_runtime_data* tx)
{
    unsigned int value = 0;

    IN_FUNCTION;

    tx->dmaa = TX1_DMA_A;
    tx->dmab = TX1_DMA_B;
    tx->dmas = TX1_DMAS;

    /* Set SUPPORT RATE */
    hi3620_reg_write(infreq_index, ASP_TX1RSRR);

    /* ASP TX1 enable*/
    value = format | HIGHBIT_IS_LEFT | TX1_EN_BIT;
    hi3620_reg_write(value, ASP_TX1);
    hi3620_set_tx123_outfreq(outfreq_index);

    wait_dma_stop(TX1_DMAS);
    hi3620_reg_write(TX1_DMAS, ASP_ICR);
    if (enable_irq) {
        value = hi3620_reg_read(ASP_IER);
        value |= (TX1_DMAS | ASP_BUS_ERROR);
        hi3620_reg_write(value, ASP_IER);
    }

    /*2-channel pcm, use spdif*/
    hi3620_reg_write(0x0, SPDIF_CTRL); //Reset SPDIF, for the Left and Right channel's data might abnormal exchange when audio paused and restarted
    hi3620_reg_write(1, ASP_SPDIFSELR);
    hi3620_reg_write(0, SPDIF_CONFIG);//line-pcm
    hi3620_reg_write(0x7, SPDIF_IRQ_MASK);//interrupt mask

    hi3620_reg_write(0x0, SPDIF_CH_STATUS1);// line-pcm
//    hi3620_reg_write(0x202, SPDIF_CH_STATUS2); //PCM source
    hi3620_reg_write(0x0, SPDIF_CH_STATUS2); //PCM source
    hi3620_reg_write(0x2010, SPDIF_CH_STATUS3);//left-->left,right-->right

    /* set  output rate */
    /*for PCM, only 48K and 96K are valid */
    if (HI3620_RATE_INDEX_48000 < outfreq_index) {
        value = 0x5050; //source sample rate 96K
        hi3620_reg_write(0xA0A, SPDIF_CH_STATUS4);//sample rate  96K
    } else {
        value = 0xd0d0; //source sample rate 48K
        hi3620_reg_write(0x202, SPDIF_CH_STATUS4);//sample rate  48K
    }

    value |= 0x202;   //16bits valid
    hi3620_reg_write(value,SPDIF_CH_STATUS5);

    hi3620_reg_write(0x5, SPDIF_CTRL); //FIFO 16,enable
#ifdef HDMI_DISPLAY
    if (g_digitaloutStatus != 0) {
        k3_hdmi_audio_set_param(freq[outfreq_index], HDMI_SAMPLE_24BITS, false, 0, 1 );
    }
#endif

    OUT_FUNCTION;
}

static void hi3620_pcm_tx_disable(struct hi3620_runtime_data *prtd)
{
//ASP_TX1 should be disable ?
    IN_FUNCTION;

    /* disable DMA interrupt */
    hi3620_clr_bits(prtd->tx[0].dmas, ASP_IER);

    if (prtd->use2tx_flag) {
        prtd->use2tx_flag = false;
        hi3620_reg_write(prtd->tx[1].dmas, ASP_ICR);
        /* disable DMA interrupt */
        hi3620_clr_bits(prtd->tx[1].dmas, ASP_IER);
    }

    hi3620_reg_write(0, ASP_TX0);
    hi3620_reg_write(0, ASP_TX1);
    hi3620_reg_write(SIO_TX_ENABLE | SIO_TX_FIFO, SIO0_I2S_CLR);
    hi3620_reg_write(SIO_TX_ENABLE | SIO_TX_FIFO, SIO1_I2S_CLR);

    OUT_FUNCTION;
}

void hi3620_pcm_clk_enable(AUDIO_ROUTE_ENUM eroute)
{
    logd("%s : clk_enable\n", __FUNCTION__);

    if (AUDIO_ROUTE_CAP == eroute) {
        /*clock enable for capture*/
        (void)clk_enable(g_clk_asp_hdmi.clk_asp);
        (void)clk_enable(g_clk_asp_hdmi.clk_aspsio);
        return;
    }
    /*clock enable for pb*/
    (void)clk_enable(g_clk_asp_hdmi.clk_asp);
    g_clk_asp_hdmi.enable_clk_asp = true;
    switch(eroute) {
    case AUDIO_ROUTE_PCM_SPDIF_ONLY:
        (void)clk_enable(g_clk_asp_hdmi.clk_aspspdif);
        g_clk_asp_hdmi.enable_clk_aspspdif = true;
        if (g_digitaloutStatus != 0) {
            (void)clk_enable(g_clk_asp_hdmi.clk_hdmi_m);
            g_clk_asp_hdmi.enable_clk_hdmi_m = true;
        }
        break;
    case AUDIO_ROUTE_PCM_DAC_SPDIF:
        (void)clk_enable(g_clk_asp_hdmi.clk_aspsio);
        g_clk_asp_hdmi.enable_clk_aspsio = true;
        (void)clk_enable(g_clk_asp_hdmi.clk_aspspdif);
        g_clk_asp_hdmi.enable_clk_aspspdif = true;
        if (g_digitaloutStatus != 0) {
            (void)clk_enable(g_clk_asp_hdmi.clk_hdmi_m);
            g_clk_asp_hdmi.enable_clk_hdmi_m = true;
        }
        break;
    case AUDIO_ROUTE_AUX_SPDIF:
        (void)clk_enable(g_clk_asp_hdmi.clk_aspspdif);
        g_clk_asp_hdmi.enable_clk_aspspdif = true;
        if (g_digitaloutStatus != 0) {
            (void)clk_enable(g_clk_asp_hdmi.clk_hdmi_m);
            g_clk_asp_hdmi.enable_clk_hdmi_m = true;
        }
        break;
    case AUDIO_ROUTE_PCM_SIO_ONLY:
    case AUDIO_ROUTE_AUX_SIO:
        (void)clk_enable(g_clk_asp_hdmi.clk_aspsio);
        g_clk_asp_hdmi.enable_clk_aspsio = true;
        (void)clk_enable(g_clk_asp_hdmi.clk_hdmi_m);
        g_clk_asp_hdmi.enable_clk_hdmi_m = true;
        break;
    case AUDIO_ROUTE_PCM_DAC_ONLY:
    default:
        (void)clk_enable(g_clk_asp_hdmi.clk_aspsio);
        g_clk_asp_hdmi.enable_clk_aspsio = true;
        break;
    }
}

/*only AUDIO_ROUTE_CAP is used for distinguish pb and cap*/
void hi3620_pcm_clk_disable(AUDIO_ROUTE_ENUM eroute)
{
    logd("%s : clk_disable\n", __FUNCTION__);

    if (AUDIO_ROUTE_CAP == eroute) {
        /*clock disable for capture*/
        clk_disable(g_clk_asp_hdmi.clk_asp);
        clk_disable(g_clk_asp_hdmi.clk_aspsio);
    } else {
        /*clock disable for playback*/
        if (g_clk_asp_hdmi.enable_clk_asp == true) {
            g_clk_asp_hdmi.enable_clk_asp = false;
            clk_disable(g_clk_asp_hdmi.clk_asp);
        }

        if (g_clk_asp_hdmi.enable_clk_aspsio == true) {
            g_clk_asp_hdmi.enable_clk_aspsio = false;
            clk_disable(g_clk_asp_hdmi.clk_aspsio);
        }

        if (g_clk_asp_hdmi.enable_clk_aspspdif == true) {
            g_clk_asp_hdmi.enable_clk_aspspdif = false;
            clk_disable(g_clk_asp_hdmi.clk_aspspdif);
        }

        if (g_clk_asp_hdmi.enable_clk_hdmi_m == true) {
            g_clk_asp_hdmi.enable_clk_hdmi_m = false;
            clk_disable(g_clk_asp_hdmi.clk_hdmi_m);
        }
    }
}
int hi3620_pcm_init_txs(struct snd_pcm_runtime *runtime, struct snd_pcm_hw_params *params)
{
    struct hi3620_runtime_data *prtd = runtime->private_data;
    unsigned int params_value = 0;
    unsigned int channels = 0;
    unsigned int format = 0;
    unsigned int infreq_index = 0;
    unsigned int outfreq_index = 0;
    AUDIO_ROUTE_ENUM eroute = 0;
    bool compressed = false;
    BUG_ON(!prtd);

    /*get parameters */
    /* CHECK SUPPORT CHANNELS : mono or stereo */
    params_value = params_channels(params);
    if (2 == params_value || 1 == params_value) {
        channels = params_value;
    } else {
        loge("%s : DAC not support %d channels\n", __FUNCTION__, params_value);
        return -EINVAL;
    }

    /* CHECK SUPPORT RATE */
    params_value = params_rate(params);
    logd("%s : set rate = %d \n",__FUNCTION__, params_value);
    for (infreq_index = 0; infreq_index < ARRAY_SIZE(freq); infreq_index++) {
        if(params_value == freq[infreq_index])
            break;
    }
    if ( ARRAY_SIZE(freq) <= infreq_index) {
        loge("%s : set rate = %d error\n",__FUNCTION__, params_value);
        return -EINVAL;
    }

    params_value = params_format(params);
    /* check formats */
    switch (params_value) {
    case SNDRV_PCM_FORMAT_S16_BE :
        format |= BIG_DIAN_BIT;
        /* full through*/
    case SNDRV_PCM_FORMAT_S16_LE :
        if (2 == channels)
            format |= STEREO_16BIT;
        else
            format |= MONO_16BIT;
        break;

    default :
        loge("%s : format err : %d, not support\n",
                __FUNCTION__, params_value);
        return -EINVAL;
    }

    /*get audio out route*/
    eroute = hi3620_get_aout_route(compressed, infreq_index, &outfreq_index, channels);
    hi3620_pcm_clk_enable(eroute);

    /*init tx channel and dma*/
    memset(prtd->tx, 0, sizeof(prtd->tx));
    /*logi("hi3620_get_aout_route %d\r\n",eroute);*/

    prtd->use2tx_flag = false;
    prtd->data_convert = false;
    switch(eroute) {
    case AUDIO_ROUTE_PCM_DAC_SPDIF://HDMI/SPDIF PCM;  txo and tx1 both enable
        prtd->use2tx_flag = true;
        hi3620_pcm_tx0_enable(format, infreq_index, outfreq_index, &prtd->tx[0]);
        hi3620_pcm_tx1_enable(format, infreq_index, outfreq_index, false, &prtd->tx[1]);
        break;
    case AUDIO_ROUTE_PCM_SPDIF_ONLY: //HDMI/SPDIF PCM;  DAC disable
        hi3620_pcm_tx1_enable(format, infreq_index, outfreq_index, true, &prtd->tx[0]);
        break;
    case AUDIO_ROUTE_PCM_DAC_ONLY:// no HDMI/SPDIF; only  DAC enable
    default :/*if return EINVAL, hi3620_pcm_open is called repeatly,so use TX0 as default route*/
        hi3620_pcm_tx0_enable(format, infreq_index, outfreq_index, &prtd->tx[0]);
        break;
    }
    return 0;
}

static int hi3620_pcm_rx_enable(struct snd_pcm_hw_params *params)
{
    unsigned int value = 0;

    value = params_channels(params);
    if (value > 2) {
        loge("%s : capture not support %d channels\n",
                __FUNCTION__, value);
        return -EINVAL;
    }

    hi3620_pcm_clk_enable(AUDIO_ROUTE_CAP);

    /* ENABLE RX CHANNEL */
    hi3620_reg_write(RX_CONFIG, ASP_RX);
    /* record : 16 or 24bits */
    value = 0;
    if (16 == snd_pcm_format_width(params_format(params)))
        value = HI3620_RECODE_16BIT | HI3620_HIGHBIT_LEFT;
    else
        value = HI3620_RECODE_24BIT;
    hi3620_reg_write(value, SIO0_I2S_16BIT);

    /* ENABLE RX DMA INTERRUPT */
    check_and_stop_rxdma();
    hi3620_reg_write(RX_DMAS, ASP_ICR);
    hi3620_set_bits(RX_DMAS | ASP_BUS_ERROR, ASP_IER);

    return 0;
}

static void hi3620_pcm_rx_disable(void)
{
    /* disable DMA interrupt */
    hi3620_reg_write(0, ASP_RX);
    hi3620_reg_write(SIO_RX_ENABLE | SIO_RX_FIFO, SIO0_I2S_CLR);
    hi3620_clr_bits(RX_DMAS, ASP_IER);
}

static int hi3620_pcm_asp_hw_params(struct snd_pcm_substream *substream,
        struct snd_pcm_hw_params *params)
{
    unsigned long bytes = params_buffer_bytes(params);
    struct hi3620_runtime_data *prtd = substream->runtime->private_data;
    int ret = 0;

    logd("%s entry : %s\n", __FUNCTION__,
            substream->stream == SNDRV_PCM_STREAM_PLAYBACK
            ? "PLAYBACK" : "CAPTURE");

    ret = snd_pcm_lib_malloc_pages(substream, bytes);
    if ( ret < 0 ) {
        loge("snd_pcm_lib_malloc_pages ret : %d\n", ret);
        return ret;
    }
    prtd->period_size = params_period_bytes(params);
    prtd->period_next = 0;

    /* PLAYBACK */
    if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream) {
        ret = hi3620_pcm_init_txs(substream->runtime, params);
        if ( ret < 0 ) {
            loge("hi3620_pcm_init_txs ret : %d\n", ret);
            snd_pcm_lib_free_pages(substream);
            return ret;
        }
    } else {
        /* CAPTURE */
        ret = hi3620_pcm_rx_enable(params);
        if ( ret < 0 ) {
            loge("hi3620_pcm_rx_enable ret : %d\n", ret);
            snd_pcm_lib_free_pages(substream);
            return ret;
        }
    }

    OUT_FUNCTION;

    return ret;
}

static int hi3620_pcm_asp_hw_free(struct snd_pcm_substream *substream)
{
    struct hi3620_runtime_data *prtd = substream->runtime->private_data;

    logd("%s : %s\n", __FUNCTION__,
            substream->stream == SNDRV_PCM_STREAM_PLAYBACK
            ? "PLAYBACK" : "CAPTURE");

    if(SNDRV_PCM_STREAM_PLAYBACK == substream->stream) {
        wait_dma_stop(TX0_DMAS | TX1_DMAS);
        hi3620_pcm_tx_disable(prtd);
        /* disable clk. for pb, we don't care the value of eroute*/
        hi3620_pcm_clk_disable(AUDIO_ROUTE_PCM_DAC_ONLY);
    } else {
        check_and_stop_rxdma();
        hi3620_pcm_rx_disable();
        hi3620_pcm_clk_disable(AUDIO_ROUTE_CAP);
    }

    return snd_pcm_lib_free_pages(substream);
}

static int hi3620_pcm_asp_prepare(struct snd_pcm_substream *substream)
{
    struct hi3620_runtime_data *prtd = substream->runtime->private_data;
    unsigned long flags = 0;

    logd("%s : %s\n", __FUNCTION__,
            substream->stream == SNDRV_PCM_STREAM_PLAYBACK
            ? "PLAYBACK" : "CAPTURE");

    spin_lock_irqsave(&prtd->lock, flags);
    /* init prtd */
    prtd->status = STATUS_STOP;
    prtd->period_next = 0;
    prtd->period_cur = 0;
    prtd->two_dma_flag = true;
    spin_unlock_irqrestore(&prtd->lock, flags);

    if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream) {
        wait_dma_stop((TX0_DMAS  | TX1_DMAS));
        hi3620_reg_write(TX01_NEWSONG, ASP_TXNSSR); /* clear fifo */
    } else {
        check_and_stop_rxdma();
    }

    return 0;
}

static int hi3620_pcm_asp_trigger(struct snd_pcm_substream *substream, int cmd)
{
    unsigned long flags = 0;
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct hi3620_runtime_data *prtd = substream->runtime->private_data;
    unsigned int num_periods = runtime->periods;
    unsigned int period_size = prtd->period_size;
    int ret = 0;

    logd("%s entry : %s,cmd: %d\n", __FUNCTION__,
            substream->stream == SNDRV_PCM_STREAM_PLAYBACK
            ? "PLAYBACK" : "CAPTURE", cmd);

#ifdef HDMI_DISPLAY
    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
    case SNDRV_PCM_TRIGGER_RESUME:
    case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
        if (g_digitaloutStatus != 0) {
            k3_hdmi_audio_set_power(true);
        }
        if (g_tx0fadeoutcmd != g_tx0fadeoutstate) {
            logi("g_tx0fadeoutstate %d g_tx0fadeoutcmd %d\n", g_tx0fadeoutstate, g_tx0fadeoutcmd);
            hi3620_pcm_fadein_internal(g_tx0fadeoutcmd, true);
        }
        break;

    case SNDRV_PCM_TRIGGER_STOP:
    case SNDRV_PCM_TRIGGER_SUSPEND:
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        if (g_digitaloutStatus != 0) {
            k3_hdmi_audio_set_power(false);
        }
        break;
    }
#endif

    spin_lock_irqsave(&prtd->lock, flags);

    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
    case SNDRV_PCM_TRIGGER_RESUME:
    case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
        prtd->status = STATUS_RUNNING;
        prtd->two_dma_flag = true;
        if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream) {
            /*enable tx[0]*/
            prtd->tx[0].two_dma_flag = true;
            if (prtd->use2tx_flag) {
                enable_dma(prtd->tx[1].dmaa, runtime->dma_addr, prtd->tx[0].period_next, period_size);
            }
            enable_dma(prtd->tx[0].dmaa, runtime->dma_addr, prtd->tx[0].period_next, period_size);
            prtd->tx[0].period_next = (prtd->tx[0].period_next + 1) % num_periods;
            if (prtd->use2tx_flag) {
                enable_dma(prtd->tx[1].dmab, runtime->dma_addr, prtd->tx[0].period_next, period_size);
            }
            enable_dma(prtd->tx[0].dmab, runtime->dma_addr, prtd->tx[0].period_next, period_size);
            prtd->tx[0].period_next = (prtd->tx[0].period_next + 1) % num_periods;
        } else {
            enable_dma(RX_DMA_A, runtime->dma_addr, prtd->period_next, period_size);
            prtd->period_next = (prtd->period_next + 1) % num_periods;
            enable_dma(RX_DMA_B, runtime->dma_addr, prtd->period_next, period_size);
            prtd->period_next = (prtd->period_next + 1) % num_periods;

            /* CONFIG SIO INTERFACE FOR CP */
            hi3620_reg_write((SIO_RX_ENABLE | SIO_RX_FIFO), SIO0_I2S_CLR);
            hi3620_set_bits((SIO_RX_ENABLE | SIO_RX_FIFO), SIO0_I2S_SET);
        }
        break;

    case SNDRV_PCM_TRIGGER_STOP:
    case SNDRV_PCM_TRIGGER_SUSPEND:
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        prtd->status = STATUS_STOP;
        break;

    default:
        loge("%s cmd error : %d", __FUNCTION__, cmd);
        ret = -EINVAL;
        break;
    }

    spin_unlock_irqrestore(&prtd->lock, flags);

    return ret;
}

static snd_pcm_uframes_t hi3620_pcm_asp_pointer(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct hi3620_runtime_data *prtd = runtime->private_data;
    unsigned long frame = 0L;

    frame = bytes_to_frames(runtime, prtd->period_cur * prtd->period_size);
    if(frame >= runtime->buffer_size)
        frame = 0;

    return frame;
}

static int hi3620_pcm_asp_open(struct snd_pcm_substream *substream)
{
    struct hi3620_runtime_data *prtd = NULL;

    logd("%s : %s\n", __FUNCTION__,
            substream->stream == SNDRV_PCM_STREAM_PLAYBACK
            ? "PLAYBACK" : "CAPTURE");

    prtd = kzalloc(sizeof(struct hi3620_runtime_data), GFP_KERNEL);
    if (NULL == prtd) {
        loge("%s : kzalloc hi3620_runtime_data error!\n", __FUNCTION__);
        return -ENOMEM;
    }

    spin_lock_init(&prtd->lock);

    substream->runtime->private_data = prtd;

    if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream)
        snd_soc_set_runtime_hwparams(substream, &hi3620_hardware_playback);
    else
        snd_soc_set_runtime_hwparams(substream, &hi3620_hardware_capture);

    return 0;
}

static int hi3620_pcm_asp_close(struct snd_pcm_substream *substream)
{
    struct hi3620_runtime_data *prtd = substream->runtime->private_data;

    logd("%s entry : %s\n", __FUNCTION__,
            substream->stream == SNDRV_PCM_STREAM_PLAYBACK
            ? "PLAYBACK" : "CAPTURE");

    if(NULL == prtd)
        loge("hisik3_pcm_close called with prtd == NULL\n");

    if (prtd) {
        kfree(prtd);
        substream->runtime->private_data = NULL;
    }

    return 0;
}

static int hi3620_pcm_hw_params(struct snd_pcm_substream *substream,
        struct snd_pcm_hw_params *params)
{
    int ret = 0;
    if (substream->pcm->device == 0)
        ret = hi3620_pcm_asp_hw_params(substream, params);
    return ret;
}

static int hi3620_pcm_hw_free(struct snd_pcm_substream *substream)
{
    int ret = 0;
    if (substream->pcm->device == 0)
        ret = hi3620_pcm_asp_hw_free(substream);
    return ret;
}

static int hi3620_pcm_prepare(struct snd_pcm_substream *substream)
{
    int ret = 0;
    if (substream->pcm->device == 0)
        ret = hi3620_pcm_asp_prepare(substream);
    return ret;
}

static int hi3620_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
    int ret = 0;
    if (substream->pcm->device == 0)
        ret = hi3620_pcm_asp_trigger(substream, cmd);
    return ret;
}

static snd_pcm_uframes_t hi3620_pcm_pointer(struct snd_pcm_substream *substream)
{
    unsigned long ret = 0L;
    if (substream->pcm->device == 0)
        ret = hi3620_pcm_asp_pointer(substream);
    return ret;
}

static int hi3620_pcm_open(struct snd_pcm_substream *substream)
{
    int ret = 0;
    if (substream->pcm->device == 0)
        ret = hi3620_pcm_asp_open(substream);
    else {
        if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream)
            snd_soc_set_runtime_hwparams(substream, &hi3620_hardware_modem_playback);
    }
    return ret;
}

static int hi3620_pcm_close(struct snd_pcm_substream *substream)
{
    int ret = 0;
    if (substream->pcm->device == 0)
        ret = hi3620_pcm_asp_close(substream);
    return ret;
}

/* define all pcm ops of hi3620 pcm */
static struct snd_pcm_ops hi3620_pcm_ops = {
    .open       = hi3620_pcm_open,
    .close      = hi3620_pcm_close,
    .ioctl      = snd_pcm_lib_ioctl,
    .hw_params  = hi3620_pcm_hw_params,
    .hw_free    = hi3620_pcm_hw_free,
    .prepare    = hi3620_pcm_prepare,
    .trigger    = hi3620_pcm_trigger,
    .pointer    = hi3620_pcm_pointer,
};

static u64 hi3620_pcm_dmamask = (u64)(0xffffffff);

static int hi3620_pcm_new(struct snd_card *card,
        struct snd_soc_dai *dai, struct snd_pcm *pcm)
{
    int ret = 0;

    IN_FUNCTION;

    if (!card->dev->dma_mask) {
        logi("%s : dev->dma_mask not set\n", __FUNCTION__);
        card->dev->dma_mask = &hi3620_pcm_dmamask;
    }

    if (!card->dev->coherent_dma_mask) {
        logi("%s : dev->coherent_dma_mask not set\n", __FUNCTION__);
        card->dev->coherent_dma_mask = hi3620_pcm_dmamask;
    }
    if (pcm->device == 0) {
        ret = snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV,
                pcm->card->dev, HI3620_MAX_BUFFER_SIZE, HI3620_MAX_BUFFER_SIZE);
        if (ret) {
            loge("snd_pcm_lib_preallocate_pages_for_all error : %d\n", ret);
            return ret;
        }
        logi("%s: pcm->device = 0\n", __FUNCTION__);
        ret = request_irq(g_hi3620_asp_irq_line, hi3620_intr_handle,
                IRQF_SHARED, "ASP", pcm);

        if (ret) {
            loge("request_irq error : %d\n", ret);
            snd_pcm_lib_preallocate_free_for_all(pcm);
        }
    } else {
        ret = snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV,
                pcm->card->dev, HI3620_BUFFER_SIZE_MM, HI3620_BUFFER_SIZE_MM);
        if (ret) {
            loge("snd_pcm_lib_preallocate_pages_for_all error : %d\n", ret);
            return ret;
        }
    }

    OUT_FUNCTION;

    return ret;
}

static void hi3620_pcm_free(struct snd_pcm *pcm)
{
    IN_FUNCTION;

    if (pcm->device == 0) {
        logi("%s: pcm->device = 0\n", __FUNCTION__);
        free_irq(g_hi3620_asp_irq_line, pcm);
    }
    snd_pcm_lib_preallocate_free_for_all(pcm);

    OUT_FUNCTION;
}

struct snd_soc_platform_driver hi3620_pcm_platform = {
    .ops      = &hi3620_pcm_ops,
    .pcm_new  = hi3620_pcm_new,
    .pcm_free = hi3620_pcm_free,
};

static void hi3620_platform_close_clocks(void)
{
    if (g_clk_asp_hdmi.clk_aspsio != NULL) {
        clk_put(g_clk_asp_hdmi.clk_aspsio);
        g_clk_asp_hdmi.clk_aspsio = NULL;
    }

    if (g_clk_asp_hdmi.clk_asp != NULL) {
        clk_put(g_clk_asp_hdmi.clk_asp);
        g_clk_asp_hdmi.clk_asp = NULL;
    }

    if (g_clk_asp_hdmi.clk_asphdmi != NULL) {
        clk_put(g_clk_asp_hdmi.clk_asphdmi);
        g_clk_asp_hdmi.clk_asphdmi = NULL;
    }

    if (g_clk_asp_hdmi.clk_hdmi_m != NULL) {
        clk_put(g_clk_asp_hdmi.clk_hdmi_m);
        g_clk_asp_hdmi.clk_hdmi_m = NULL;
    }

    if (g_clk_asp_hdmi.clk_aspspdif != NULL) {
        clk_put(g_clk_asp_hdmi.clk_aspspdif);
        g_clk_asp_hdmi.clk_aspspdif = NULL;
    }
}
static int __devinit hi3620_platform_probe(struct platform_device *pdev)
{
    int ret = -ENODEV;
    struct resource* res;

    IN_FUNCTION;

    /* get ASP clock */
    memset(&g_clk_asp_hdmi, 0 ,sizeof(g_clk_asp_hdmi));
    g_clk_asp_hdmi.clk_asp = clk_get(NULL, "clk_asp");
    if (IS_ERR(g_clk_asp_hdmi.clk_asp)) {
        loge("%s : Could not get clk_asp\n", __FUNCTION__);
        return ret;
    }
    /* get SIO clock */
    g_clk_asp_hdmi.clk_aspsio = clk_get(NULL, "clk_aspsio");
    if (IS_ERR(g_clk_asp_hdmi.clk_aspsio)) {
        loge("%s : Could not get clk_aspsio\n", __FUNCTION__);
        goto probe_failed;
    }

    /* get ASPHDMI mclck */
    g_clk_asp_hdmi.clk_asphdmi = clk_get(NULL, "clk_asphdmi");
    if (IS_ERR(g_clk_asp_hdmi.clk_asphdmi)) {
        loge("%s : Could not get clk_asphdmi\n", __FUNCTION__);
        goto probe_failed;
    }

    g_clk_asp_hdmi.clk_hdmi_m = clk_get(NULL, "clk_hdmi_m");
    if (IS_ERR(g_clk_asp_hdmi.clk_hdmi_m)) {
        loge("%s : Could not get clk_hdmi_m\n", __FUNCTION__);
        goto probe_failed;
    }

    g_clk_asp_hdmi.clk_aspspdif = clk_get(NULL, "clk_aspspdif");
    if (IS_ERR(g_clk_asp_hdmi.clk_aspspdif)) {
        loge("%s : Could not get clk_aspspdif\n", __FUNCTION__);
        goto probe_failed;
    }

    /* get pmuspi register base address */
    res = platform_get_resource_byname(pdev, IORESOURCE_IRQ, "asp_irq");
    if (!res) {
        loge("%s: Could not get irq line\n", __FUNCTION__);
        goto probe_failed;
    }
    g_hi3620_asp_irq_line = res->start;

    /* get pmuspi register base address */
    res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "asp_base");
    if (!res) {
        loge("%s: Could not get register base\n", __FUNCTION__);
        goto probe_failed;
    }
    g_hi3620_asp_reg_base_addr =
            (unsigned int)ioremap(res->start, resource_size(res));

    /* register dai (name : hi3620-asp) */
    ret = snd_soc_register_dais(&pdev->dev, hi3620_dai, ARRAY_SIZE(hi3620_dai));
    if (ret) {
        loge("%s: snd_soc_register_dai return %d\n", __FUNCTION__ ,ret);
        goto probe_failed;
    }

    /* register platform (name : hi3620-asp) */
    ret = snd_soc_register_platform(&pdev->dev, &hi3620_pcm_platform);
    if (ret) {
        loge("%s: snd_soc_register_platform return %d\n", __FUNCTION__ ,ret);
        goto probe_failed;
    }

    OUT_FUNCTION;

    return ret;

probe_failed:
    hi3620_platform_close_clocks();

    OUT_FUNCTION;
    return ret;
}

static int __devexit hi3620_platform_remove(struct platform_device *pdev)
{
    snd_soc_unregister_platform(&pdev->dev);
    snd_soc_unregister_dais(&pdev->dev, ARRAY_SIZE(hi3620_dai));

    hi3620_platform_close_clocks();
    return 0;
}

static struct platform_driver hi3620_platform_driver = {
    .driver = {
        .name = "hi3620-asp",
        .owner = THIS_MODULE,
    },
    .probe  = hi3620_platform_probe,
    .remove = __devexit_p(hi3620_platform_remove),
};

static int status_read_proc_astatus(char *page, char **start, off_t offset,
                    int count, int *eof, void *data)
{
    *eof = 1;
    return sprintf(page, "%d", g_digitaloutStatus);
}

static int status_write_proc_astatus(struct file *file, const char *buffer,
                    unsigned long count, void *data)
{
    char buf[64] = {0};
    int value = 0;

    if (count < 1)
        return -EINVAL;
    if (count > 60)
        count  = 60;

    if (copy_from_user(buf, buffer, count)) {
        loge("copy_from_user Error\n");
        return -EFAULT;
    }
    if (sscanf(buf, "0x%8x", &value) != 1) {
        logd("set the audiostatus error\r\n");
    }
    logd("get the audiostatus %d\r\n",value);
    g_spdifout = value >> 16; //SPDIF output config
    g_digitaloutStatus = value & 0xffff;

    return count;
}

static int __init hi3620_init(void)
{
    struct proc_dir_entry *ent;

    audio_pcm_dir = proc_mkdir("apcm", NULL);
    if (audio_pcm_dir == NULL) {
        printk("Unable to create /proc/apcm directory");
        return -ENOMEM;
    }

    /* Creating read/write "status" entry */
    ent = create_proc_entry("status", 0777, audio_pcm_dir);
    if (ent == NULL) {
        remove_proc_entry("apcm", 0);
        printk("Unable to create /proc/apcm/status entry");
        return -ENOMEM;
    }
    ent->read_proc = status_read_proc_astatus;
    ent->write_proc = status_write_proc_astatus;

    return platform_driver_register(&hi3620_platform_driver);
}
module_init(hi3620_init);

static void __exit hi3620_exit(void)
{
    remove_proc_entry("status", audio_pcm_dir);
    remove_proc_entry("adigital", 0);

    platform_driver_unregister(&hi3620_platform_driver);
}
module_exit(hi3620_exit);

MODULE_AUTHOR("C00166660");
MODULE_DESCRIPTION("Hi3620 ASP platform driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:asp");
