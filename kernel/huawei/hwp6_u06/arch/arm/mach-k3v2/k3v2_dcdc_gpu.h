#ifndef __MACH_K3V2_DCDC_GPU_H
#define __MACH_K3V2_DCDC_GPU_H
#include <linux/ipps.h>
#include <mach/platform.h>
#define MODULE_NAME			"k3v2-dcdc-gpu"
#define G3D_DVFS_ENABLE (0x150)
#define G3D_CLK_PROFILE (0x158)
#define G3D_VOL_PROFILE (0x15C)
#define G3D_MON_COUNTER (IO_ADDRESS(REG_BASE_G3D) + 0x78)
#define G3D_DVFS_CLR (0x008)
#define G3D_DVFS_START (0x154)
#define DEFAULT_GPUDVFS_PERIOD			(50)/*ms*/
#define PLL_RATE_600M	(0x00912C34)/*600M*/
#define EN_PLL_RATE_600M	(0x00912C35)/*600M*/
#define PLL_RATE_BYPASS_600M	(0x00912C36)/*600M bypass*/
#define EN_PLL_RATE_BYPASS_600M	(0x00912C37)/*600M bypass*/
#define DVFS_DONE_REG	(IO_ADDRESS(REG_BASE_PMCTRL) + 0x004)
#define G3D_DVFS_INTR_STATUS	(1 << 1)
#define USBPLL_DIV_REG	(IO_ADDRESS(REG_BASE_PMCTRL) + 0x01C)
#define G3D_CLK_SEL2	(0x0B0)
#define G3D_CHAN_MASK (0x00E00000)
#define TP_ENABLE (0x02)
#define TEMP_STATUS (0x502C)
#define MCU_REG_BASE (IO_ADDRESS(REG_BASE_MCU))
#define TEMP_VAL_REG	(IO_ADDRESS(REG_BASE_PCTRL) + 0xEC)
#define POWERSAVE_POLICY (0x0)
#define NORMAL_POLICY (0x01)
#define PERF_POLICY (0x02)
#define SPEC01_POLICY (0x03)
#define SPEC02_POLICY (0x04)
#define SPEC03_POLICY (0x05)
#define SPEC04_POLICY (0x06)
#define SPEC05_POLICY (0x07)
#define SPEC06_POLICY (0x08)
#define SPEC07_POLICY (0x09)
#define SPEC08_POLICY (0x0A)
#define SPEC09_POLICY (0x0B)
#define SPEC0A_POLICY (0x0C)
#define SPEC0B_POLICY (0x0D)
#define TEMP_MON_ENABLE	(0x01)
#define TEMP_MON_DISABLE	(0x00)
#define TEMP_LOW	(0xA0)
#define TEMP_HIGH	(0xAC)
enum gpu_profile_id {
	GPU_PROFILE0,
	GPU_PROFILE1,
	GPU_PROFILE2,
	GPU_PROFILE3,
	GPU_PROFILE4,
	GPU_PROFILE5,
	GPU_PROFILE6,
	PROFILE_END,
};

#undef WITH_G3D_600M_PROF
#define G3D_PLLSW_PROF_ID (5)

union param {
	struct {
		u32 min:8;
		u32 max:8;
		u32 safe:8;
	} freq;
	u32 ul32;
};

struct ipps_gpu {
	int curr_mode;
	int curr_profile;
	int curr_policy;
	union param gpu;
	u8 uptimes;
	u8 downtimes;
	void __iomem *pmctrl_base;
	struct ipps_device *idev;
	struct platform_device *pdev;
	struct regulator *regulator_dcdc;
	struct workqueue_struct	*gpu_dvfs_wq;
	struct delayed_work k_gpu_work;
	struct clk *clk_hdmipll;
	u8 temp_enable;
};

struct gpu_profile_info {
	u32 freq;
	u32 gpu_clk_profile;
	u32 gpu_vol_profile;
	u32 dcdc_vol;
};

struct gpu_policy_info {
	u8 uptimes;
	u8 downtimes;
	u32 up_threshold;
	u32 down_threshold;
};

extern struct gpu_profile_info gpu_profile_dcdc[];
extern struct gpu_policy_info *policy_table[];

struct plat_data {
	unsigned int obj;
	unsigned int cur_profile;
	unsigned int mode;
	unsigned int policy;
	u32 min:8;
	u32 max:8;
	u32 safe:8;
};
#endif
