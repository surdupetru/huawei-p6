#ifndef HI3620_PCM_H
#define HI3620_PCM_H
#include <mach/io_mutex.h>

/* register address offset */
#define ASP_TX0         ( 0x0000 )  /* TX0 CONFIG */
#define ASP_TX1         ( 0x0004 )  /* TX1 CONFIG */
#define ASP_TX2         ( 0x0008 )  /* TX2 CONFIG */
#define ASP_TX3         ( 0x000c )  /* TX3 CONFIG */
#define ASP_RX          ( 0x0010 )  /* RX ENABLE */
#define ASP_DER         ( 0x0014 )  /* DMA ENABLE */
#define ASP_DSTOP       ( 0x0018 )  /* DMA STOP */
#define ASP_IRSR        ( 0x0020 )  /* INTERRPUTE STATE */
#define ASP_IER         ( 0x0024 )  /* INTERRPUTE ENABLE */
#define ASP_ICR         ( 0x002C )  /* INTERRPUTE CLEAR */
#define ASP_TXNSSR      ( 0x0030 )  /* NEW SONG START */
#define ASP_TX0RSRR     ( 0x0034 )  /* TX0 RESAMPLE RATE */
#define ASP_TX1RSRR     ( 0x0038 )  /* TX1 RESAMPLE RATE */
#define ASP_TX2RSRR     ( 0x003C )  /* TX2 RESAMPLE RATE */
#define ASP_TX3RSRR     ( 0x0040 )  /* TX3 RESAMPLE RATE */
#define ASP_FADEINEN0   ( 0x0044 )  /* TX3 RESAMPLE RATE */
#define ASP_FADEOUTEN0  ( 0x0048 )  /* TX3 RESAMPLE RATE */
#define ASP_FADERATE0   ( 0x004C )  /* TX3 RESAMPLE RATE */

#define ASP_P0LCGR      ( 0x0074 )  /* ASP_P0LCGR */
#define ASP_P0RCGR      ( 0x0078 )  /* ASP_P0RCGR */
#define ASP_P1LCGR      ( 0x007C )  /* ASP_P1LCGR */
#define ASP_P1RCGR      ( 0x0080 )  /* ASP_P1RCGR */
#define ASP_P2LCGR      ( 0x0084 )  /* ASP_P2LCGR */
#define ASP_P2RCGR      ( 0x0088 )  /* ASP_P2RCGR */
#define ASP_P30CGR      ( 0x008C )  /* ASP_P30CGR */
#define ASP_P31CGR      ( 0x0090 )  /* ASP_P31CGR */
#define ASP_P32CGR      ( 0x0094 )  /* ASP_P32CGR */
#define ASP_P33CGR      ( 0x0098 )  /* ASP_P33CGR */
#define ASP_P34CGR      ( 0x009C )  /* ASP_P34CGR */
#define ASP_P35CGR      ( 0x00A0 )  /* ASP_P35CGR */
#define ASP_P36CGR      ( 0x00A4 )  /* ASP_P36CGR */
#define ASP_P37CGR      ( 0x00A8 )  /* ASP_P37CGR */

#define ASP_RXASAR      ( 0x00B0 )  /* START ADDR */
#define ASP_RXADLR      ( 0x00B4 )  /* DATA LENGTH */
#define ASP_RXBSAR      ( 0x00B8 )  /* START ADDR */
#define ASP_RXBDLR      ( 0x00BC )  /* DATA LENGTH */
#define ASP_TX0ASAR     ( 0x00C0 )  /* START ADDR */
#define ASP_TX0ADLR     ( 0x00C4 )  /* DATA LENGTH */
#define ASP_TX0BSAR     ( 0x00C8 )  /* START ADDR */
#define ASP_TX0BDLR     ( 0x00CC )  /* DATA LENGTH */
#define ASP_TX1ASAR     ( 0x00D0 )  /* START ADDR */
#define ASP_TX1ADLR     ( 0x00D4 )  /* DATA LENGTH */
#define ASP_TX1BSAR     ( 0x00D8 )  /* START ADDR */
#define ASP_TX1BDLR     ( 0x00DC )  /* DATA LENGTH */
#define ASP_TX2ASAR     ( 0x00E0 )  /* START ADDR */
#define ASP_TX2ADLR     ( 0x00E4 )  /* DATA LENGTH */
#define ASP_TX2BSAR     ( 0x00E8 )  /* START ADDR */
#define ASP_TX2BDLR     ( 0x00EC )  /* DATA LENGTH */
#define ASP_TX3ASAR     ( 0x00F0 )  /* START ADDR */
#define ASP_TX3ADLR     ( 0x00F4 )  /* DATA LENGTH */
#define ASP_TX3BSAR     ( 0x00F8 )  /* START ADDR */
#define ASP_TX3BDLR     ( 0x00FC )  /* DATA LENGTH */

#define ASP_SPDIFSELR   ( 0x0100 )  /* SPDIF/HDMI SELECT */

#define ASP_FIRST_REG   ( 0x0000 )
#define ASP_LAST_REG    ( 0x0154 )

#define SIO0_ASPIF_SEL  ( 0x1000 )  /* SIO0_ASPIF_SEL */
#define SIO0_I2S_SET    ( 0x101C )  /* SIO I2S CONFIG */
#define SIO0_I2S_CLR    ( 0x1020 )  /* SIO I2S CONFIG CLEAR */
#define SIO0_I2S_16BIT  ( 0x103C )  /* SIO I2S RECORD 16BIT */
#define SIO0_FIRST_REG  ( 0x1000 )
#define SIO0_LAST_REG   ( 0x104C )

#define SIO1_ASPIF_SEL  ( 0x2000 )  /* SIO1_ASPIF_SEL */
#define SIO1_INT_ENABLE ( 0x2010 )  /* SIO1_INT_ENABLE */
#define SIO1_I2S_SET    ( 0x201C )  /* SIO I2S CONFIG */
#define SIO1_I2S_CLR    ( 0x2020 )  /* SIO I2S CONFIG CLEAR */
#define SIO1_FIRST_REG  ( 0x2000 )
#define SIO1_LAST_REG   ( 0x205C )

#define SPDIF_CTRL          ( 0x3000 )  /* SPDIF CONTROL */
#define SPDIF_CONFIG        ( 0x3004 )  /* SPDIF CONFIG */
#define SPDIF_IRQ_MASK      ( 0x300C )  /* SPDIF INTERRUPT MASK */
#define SPDIF_CH_STATUS1    ( 0x3020 )  /* SPDIF_CH_STATUS1 */
#define SPDIF_CH_STATUS2    ( 0x3024 )  /* SPDIF_CH_STATUS2 */
#define SPDIF_CH_STATUS3    ( 0x3028 )  /* SPDIF_CH_STATUS3 */
#define SPDIF_CH_STATUS4    ( 0x302C )  /* SPDIF_CH_STATUS4 */
#define SPDIF_CH_STATUS5    ( 0x3030 )  /* SPDIF_CH_STATUS5 */
#define SPDIF_FIRST_REG     ( 0x3000 )
#define SPDIF_LAST_REG      ( 0x30F0 )

/* register bits */
/* TX0/RX DMA A&B BITS FOR DER/DSTOP/IRSR/IER/ICR */
#define TX0_DMA_A       ( 1 << 2 )
#define TX0_DMA_B       ( 1 << 3 )
#define TX0_DMAS        ( TX0_DMA_A | TX0_DMA_B )
#define TX1_DMA_A       ( 1 << 4 )
#define TX1_DMA_B       ( 1 << 5 )
#define TX1_DMAS        ( TX1_DMA_A | TX1_DMA_B )
#define TX2_DMA_A       ( 1 << 6 )
#define TX2_DMA_B       ( 1 << 7 )
#define TX2_DMAS        ( TX2_DMA_A | TX2_DMA_B )
#define TX3_DMA_A       ( 1 << 8 )
#define TX3_DMA_B       ( 1 << 9 )
#define TX3_DMAS        ( TX3_DMA_A | TX3_DMA_B )
#define TX_DMAS         ( TX0_DMAS | TX1_DMAS | TX2_DMAS | TX3_DMAS )

#define RX_DMA_A        ( 1 << 0 )
#define RX_DMA_B        ( 1 << 1 )
#define RX_DMAS         ( RX_DMA_A | RX_DMA_B )
#define TX0_FADEIN      ( 1 << 10 )
#define TX0_FADEOUT     ( 1 << 14 )
#define ASP_BUS_ERROR   ( 1 << 18 )

/* value */
#define TX_96K_OUTPUT_BIT   ( 1 << 8 )  /* interface set in 96kHz */
#define TX_192K_OUTPUT_BIT  ( 1 << 9 )  /* interface set in 192kHz */
#define GAIN_EN_BIT         ( 1 << 7 )  /* enable tx0 gain */
#define TX0_EN_BIT          ( 1 << 6 )  /* enable tx0 */
#define TX1_EN_BIT          ( 1 << 6 )  /* enable tx1 */
#define TX2_EN_BIT          ( 1 << 6 )  /* enable tx2 */
#define TX2_TSEN_BIT        ( 1 << 8 )  /* enable tx2 ts*/
#define TX3_EN_BIT          ( 1 << 2 )  /* enable tx3 */
#define TX3_TSEN_BIT        ( 1 << 4 )  /* enable tx3 ts*/
#define UNSIGNED_BIT        ( 1 << 5 )  /* data is unsigned */
#define BIG_DIAN_BIT        ( 1 << 4 )
#define HIGHBIT_IS_LEFT     ( 1 << 3 )  /*this is revert as description*/

#define TX01_NEWSONG        ( 0x3 )     /* tx new song mask */
#define TX23_NEWSONG        ( 0xC )     /* tx new song mask */
#define RX_CONFIG           ( 1 )       /* rx enable */

#define SIO_TX_ENABLE       ( 1 << 16 )
#define SIO_RX_ENABLE       ( 1 << 17 )
#define SIO_TX_FIFO_SHIFT_BIT   ( 4 )   /* clear sio tx fifo threshold */
#define SIO_RX_FIFO_SHIFT_BIT   ( 8 )   /* clear sio rx fifo threshold */
#define SIO_TX_FIFO_SIZE        ( 16 )
#define SIO_RX_FIFO_SIZE        ( 32 )
#define SIO_TX_FIFO     ( (SIO_TX_FIFO_SIZE - 1) << SIO_TX_FIFO_SHIFT_BIT )
#define SIO_RX_FIFO     ( (SIO_RX_FIFO_SIZE - 1) << SIO_RX_FIFO_SHIFT_BIT )

#define HI3620_RECODE_16BIT     ( 1 )   /* record 16bit */
#define HI3620_RECODE_24BIT     ( 0 )   /* record 24bit */
#define HI3620_HIGHBIT_LEFT     ( 1 << 1 )

enum HI3620_STATUS {
    STATUS_STOP = 0,
    STATUS_RUNNING,
};

enum HI3620_FORMAT {
    STEREO_16BIT = 0,
    MONO_16BIT,
    STEREO_8BIT,
    MONO_8BIT,
    STEREO_18BIT,
    STEREO_20BIT,
    STEREO_24BIT,
};

typedef enum{
    AFORMAT_RESERVED = 0,
    AFORMAT_PCM,
    AFORMAT_AC3,
    AFORMAT_MPEG1,
    AFORMAT_MP3,
    AFORMAT_MPEG2,  //5
    AFORMAT_AAC,
    AFORMAT_DTS,
    AFORMAT_ATRAC,
    AFORMAT_OBA,
    AFORMAT_DDP,  // 10
    AFORMAT_DTSHD,
    AFORMAT_MAT_MLP,
    AFORMAT_DST,
    AFORMAT_WMAP,
    AFORMAT_MULTI_PCM, // 15   5.1/ 7.1 pcm
}AUDIO_FORMAT;

typedef enum{
    AUDIO_ROUTE_PCM_DAC_ONLY = 0, // no HDMI/SPDIF; only  DAC enable
    AUDIO_ROUTE_PCM_SPDIF_ONLY, // HDMI/SPDIF PCM; DAC disable
    AUDIO_ROUTE_PCM_SIO_ONLY, // HDMI 5.1/7.1 PCM; DAC disable
    AUDIO_ROUTE_PCM_DAC_SPDIF, // HDMI/SPDIF PCM;  txo and tx1 both enable
    AUDIO_ROUTE_AUX_SPDIF, // HDMI/SPDIF compressed data;  low bit rate
    AUDIO_ROUTE_AUX_SIO, // HDMI compressed data;  high bitrate
    AUDIO_ROUTE_PB_MAX,
    AUDIO_ROUTE_CAP   //audio capture
}AUDIO_ROUTE_ENUM;

typedef struct {
    bool enable_clk_asp;
    bool enable_clk_aspsio;
    bool enable_clk_hdmi_m;
    bool enable_clk_aspspdif;
    /* HI3620 ASP CLOCK */
    struct clk *clk_asp;
    /* HI3620 SIO CLOCK */
    struct clk *clk_aspsio;
    /* HI3620 ASP/HDMI MCLK */
    struct clk *clk_asphdmi; /*needn't enable*/
    struct clk *clk_hdmi_m;
    struct clk *clk_aspspdif;
}AUDIO_CLK_STRU ;

struct dc_dma_para{
    struct work_struct irq_work;
    unsigned int dma;
    unsigned int sour_dma_addr;     //this is physical addr used by DMA
    unsigned char *sour_vir_addr;   //this is virtual addr used by CPU
    unsigned int dst_dma_addr;      //this is physical addr used by DMA
    unsigned char *dst_vir_addr;    //this is virtual addr used by CPU
    int period_next;
    int dma_size;
};

struct hi3620_tx_runtime_data {
    unsigned int  period_next;  /* record which period to fix dma next time */
    unsigned int  period_cur;   /* record which period using now */
    bool two_dma_flag;          /* flag of using two dma,for playback */
    unsigned int  dmaa;   /* dmaa for this channel */
    unsigned int  dmab;   /* dmab for this channel */
    unsigned int  dmas;   /* dmaa&dmab for this channel */
};

struct hi3620_runtime_data {
    spinlock_t lock;            /* protect hi3620_runtime_data */
    bool use2tx_flag;          /* flag of using two channel*/
    struct hi3620_tx_runtime_data tx[2];/*tx[0] for tx0/tx2/tx3 & tx[1] for tx1*/
    unsigned int  period_size;  /* DMA SIZE */
    unsigned int  period_next;  /* record which period to fix dma next time */
    unsigned int  period_cur;   /* record which period using now */
    bool two_dma_flag;          /* flag of using two dma,for capture */
    enum HI3620_STATUS status;  /* pcm status running or stop */
    bool data_convert;          /* flag of converting data format, used for 8-ch pcm or compressed data, 32bits data to two  32bits data with 16'0b high bits*/
    struct dc_dma_para dmaa_para;  /*para for data format convertion*/
    struct dc_dma_para dmab_para; /*para for data format convertion*/
};

#endif
