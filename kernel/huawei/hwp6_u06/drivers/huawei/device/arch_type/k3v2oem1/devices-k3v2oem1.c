/*sensor unification for differ platform*/
/*==============================================================================
History

Problem NO.         Name        Time         Reason

==============================================================================*/
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <mach/gpio.h>
#include <mach/boardid.h>

#include "../../accelerometer/lis3dh.h"
#include "../../accelerometer/adxl34x.h"
#include "../../accelerometer/gs_mma8452.h"
#include "../../gyroscope/l3g4200d.h"
#define CONFIG_HUAWEI_FEATURE_SENSORS_ACC_GYRO_LSM330
#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_ACC_GYRO_LSM330
#include "../../accelerometer/lsm330.h"
#endif
#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_ACC_COMPASS_LSM303D
#include "../../accelerometer/lsm303d.h"
#endif
#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AKM8975
#include "../../compass/akm8975.h"
#endif
#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AKM8963
#include "../../compass/akm8963.h"
#endif
#include "../../light/apds990x.h"
#include "../../compass/yas.h"
#include "../../compass/yas_cfg.h"
#include <linux/board_sensors.h>
#include <hsad/config_interface.h>

unsigned int g_compass_softiron_type = 0;
#define GSENSOR_INT_GPIO	GPIO_18_4
#define COMPASS_INT_GPIO	GPIO_15_5
#define PROXIMITY_INT_GPIO	GPIO_15_6
#define E_SENSOR_TYPE_C9800D 1
#define E_SENSOR_TYPE_T9800D 2
#define E_SENSOR_TYPE_U9800D 3
#define E_SENSOR_TYPE_C9800D_V4  4
#define E_SENSOR_TYPE_C9701L 5
#define E_SENSOR_TYPE_U9700L 6
#define E_SENSOR_TYPE_UEDGE  8
#define E_SENSOR_TYPE_CEDGE  9


static struct lis3dh_acc_platform_data gs_platform_data = {

	.poll_interval = 10,
	.min_interval = 10,

	.g_range = 0x00,

	.axis_map_x = 0,
	.axis_map_y = 1,
	.axis_map_z = 2,

	.negate_x = 1,
	.negate_y = 1,
	.negate_z = 0,

	.gpio_int1 = -1, /* if used this irq,set gpio_int1=GPIO_18_4 */
	.gpio_int2 = -1, /* if used this irq,set gpio_int2=GPIO_6_3	*/
};

struct mma8452_acc_platform_data mma8452_platform_data = {
};

#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_ACC_GYRO_LSM330
static struct lsm330_acc_platform_data lsm330_acc_pdata = {
	.fs_range = LSM330_ACC_G_2G,
	.axis_map_x = 0,
	.axis_map_y = 1,
	.axis_map_z = 2,
	.negate_x = 0,
	.negate_y = 0,
	.negate_z = 0,
	.poll_interval = 10,
	.min_interval = LSM330_ACC_MIN_POLL_PERIOD_MS,
	.gpio_int1 = LSM330_ACC_DEFAULT_INT1_GPIO,
	.gpio_int2 = LSM330_ACC_DEFAULT_INT2_GPIO,
};
static struct lsm330_gyr_platform_data lsm330_gyr_pdata = {
	.fs_range = LSM330_GYR_FS_2000DPS,
	.axis_map_x = 0,
	.axis_map_y = 1,
	.axis_map_z = 2,
	.negate_x = 0,
	.negate_y = 0,
	.negate_z = 0,

	.poll_interval = 10,
	.min_interval = LSM330_GYR_MIN_POLL_PERIOD_MS,	/* 2ms */

	.gpio_int1 = LSM330_GYR_DEFAULT_INT1_GPIO,
	.gpio_int2 = LSM330_GYR_DEFAULT_INT2_GPIO,	/* int for fifo */
};
#endif
#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_ACC_COMPASS_LSM303D
static struct lsm303d_acc_platform_data lsm303d_acc_pdata = {
	.fs_range = LSM303D_ACC_FS_2G,
	.axis_map_x = 0,
	.axis_map_y = 1,
	.axis_map_z = 2,
	.negate_x = 0,
	.negate_y = 0,
	.negate_z = 0,
	.poll_interval = 100,
	.min_interval = LSM303D_ACC_MIN_POLL_PERIOD_US,
};

static struct lsm303d_mag_platform_data lsm303d_mag_pdata = {
	.poll_interval = 100,
	.min_interval = LSM303D_MAG_MIN_POLL_PERIOD_US,
	.fs_range = LSM303D_MAG_FS_2G,
	.axis_map_x = 0,
	.axis_map_y = 1,
	.axis_map_z = 2,
	.negate_x = 0,
	.negate_y = 0,
	.negate_z = 0,
};
static struct lsm303d_main_platform_data lsm303d_platform_data = {
	.pdata_acc = &lsm303d_acc_pdata,
	.pdata_mag = &lsm303d_mag_pdata,
};
#endif
#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AKM8975
static struct akm8975_platform_data compass_akm8975_platform_data = {
	.gpio_DRDY = GPIO_15_5 ,
};
#endif
#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AKM8963
static struct akm8963_platform_data compass_akm8963_platform_data = {
	.outbit = 1,
	.gpio_DRDY = GPIO_15_5 ,/* GPIO-125	*/
	.gpio_RST = 0,
};
#endif
static struct l3g4200d_gyr_platform_data l3g4200d_gyr_platform_data = {
	.poll_interval = 10,
	.min_interval = 10,

	.fs_range = 0x30,

	.axis_map_x = 1,
	.axis_map_y = 0,
	.axis_map_z = 2,

	.negate_x = 1,
	.negate_y = 1,
	.negate_z = 0,
};

static struct adxl34x_platform_data adxl34x_default_init = {
	.tap_threshold = 35,
	.tap_duration = 3,
	.tap_latency = 20,
	.tap_window = 20,
	.tap_axis_control = ADXL_TAP_X_EN | ADXL_TAP_Y_EN | ADXL_TAP_Z_EN,
	.act_axis_control = 0xFF,
	.activity_threshold = 6,
	.inactivity_threshold = 4,
	.inactivity_time = 3,
	.free_fall_threshold = 8,
	.free_fall_time = 0x20,
	.data_rate = 10,
	/* .data_rate = 8, */
	.data_range = ADXL_FULL_RES,

	.ev_type = EV_ABS,
	.ev_code_x = ABS_X,    /* EV_REL */
	.ev_code_y = ABS_Y,    /* EV_REL */
	.ev_code_z = ABS_Z,    /* EV_REL */

	.ev_code_tap_x = BTN_TOUCH,    /* EV_KEY */
	.ev_code_tap_y = BTN_TOUCH,    /* EV_KEY */
	.ev_code_tap_z = BTN_TOUCH,    /* EV_KEY */
	.power_mode = ADXL_LINK,
	.fifo_mode = FIFO_STREAM,
	.watermark = 0,
};

struct apds990x_platform_data apds990x_light_platform_data = {
};
#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_YAMAHA_COMPASS
static struct yamaha_platform_data yamaha_compass_platform_data = {
	.gpio_DRDY = GPIO_15_5,
	.gpio_RST = 48,
	.layout = 3,
	.outbit = 1,
};
#endif

static struct i2c_board_info  k3v2oem1_i2c_0_boardinfo[] = {

#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AKM8975
	{
		I2C_BOARD_INFO(AKM8975C_I2C_NAME, AKM8975C_I2C_ADDR),
		.platform_data = &compass_akm8975_platform_data,
		.irq = IRQ_GPIO(COMPASS_INT_GPIO),
	},
#endif
#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AKM8963
	{
		I2C_BOARD_INFO(AKM8963_I2C_NAME, AKM8963_I2C_ADDR),
		.platform_data = &compass_akm8963_platform_data,
		.irq = IRQ_GPIO(COMPASS_INT_GPIO),
	},
#endif
#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_ACC_GYRO_LSM330
	{
		I2C_BOARD_INFO(LSM330_GYR_DEV_NAME, LSM330_GYR_I2C_ADDR),  //0x6b
		.platform_data = &lsm330_gyr_pdata,
	},
	{
		I2C_BOARD_INFO(LSM330_ACC_DEV_NAME, LSM330_ACC_I2C_ADDR),
		.platform_data = &lsm330_acc_pdata,
	},
#endif
#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_ACC_COMPASS_LSM303D
	{
		I2C_BOARD_INFO(LSM303D_DEV_NAME,LSM303D_I2C_ADDR),
		.platform_data = &lsm303d_platform_data,
	},
#endif
#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_YAMAHA_COMPASS
    {
        I2C_BOARD_INFO("yamaha_geomagnetic", 0x2e),//7 bit addr, no write bit
        .platform_data = &yamaha_compass_platform_data,
		.irq = IRQ_GPIO(COMPASS_INT_GPIO),
	},
#endif
#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_ACCELEROMETER_ST_LIS3XH
	{
		I2C_BOARD_INFO(LIS3DH_I2C_NAME, LIS3DH_I2C_ADDR),
		.platform_data = &gs_platform_data,
	},
#endif

#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_ACCELEROMETER_MMA8452
	{
		I2C_BOARD_INFO(MMA8452_I2C_NAME, MMA8452_I2C_ADDR),
		.platform_data = &mma8452_platform_data,
	},
#endif

#ifdef CONFIG_HUAWEI_FEATURE_GYROSCOPE_L3G4200DH
	{
		I2C_BOARD_INFO(L3G4200D_I2C_NAME, L3G4200D_I2C_ADDR),
		.platform_data = &l3g4200d_gyr_platform_data,
	},
#endif

#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_ACCELEROMETER_ADI_ADXL346
	{
		I2C_BOARD_INFO(ADXL34X_I2C_NAME, ADXL34X_I2C_ADDR),
		.irq = IRQ_GPIO(GSENSOR_INT_GPIO),
		.platform_data = &adxl34x_default_init,
	},
#endif

#ifdef CONFIG_HUAWEI_FEATURE_PROXIMITY_APDS990X
	{
		I2C_BOARD_INFO(APDS990x_I2C_NAME, APDS990x_I2C_ADDR),
		.irq = IRQ_GPIO(PROXIMITY_INT_GPIO),
		.platform_data = &apds990x_light_platform_data,
	},
#endif
#ifdef CONFIG_HUAWEI_FEATURE_CM3320
       {
        I2C_BOARD_INFO(CM3320_I2C_NAME, CM3320_I2C_ADDR),
       },
#endif

//add three_color_led drivered by tca6507
	{
		I2C_BOARD_INFO("led_colors", 0x45),
    },

};


/* sensor layout init for different board */
void sensor_layout_init(void)
{
	unsigned int sensor_type;
       unsigned int compass_softiron_type;

	sensor_type = get_sensor_type();
       compass_softiron_type = get_compass_softiron_type();
       g_compass_softiron_type = compass_softiron_type;
	switch (sensor_type) {
	//case E_SENSOR_TYPE_PHONE:
        case E_SENSOR_TYPE_C9800D:
		gs_platform_data.negate_x = 1;
		gs_platform_data.negate_y = 1;
                gs_platform_data.negate_z = 0;
		mma8452_platform_data.config_mxc_mma_position = 1;
		adxl34x_default_init.config_adxl34x_position = 0;
		#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AKM8975
		compass_akm8975_platform_data.config_akm_position = 3;
		#endif
		#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AKM8963
		compass_akm8963_platform_data.layout = 3;
		#endif
		l3g4200d_gyr_platform_data.negate_x = 1;
		l3g4200d_gyr_platform_data.negate_y = 1;
		l3g4200d_gyr_platform_data.negate_z = 0;

		break;
	case E_SENSOR_TYPE_C9800D_V4:
		gs_platform_data.negate_x = 0;
		gs_platform_data.negate_y = 1;
                gs_platform_data.negate_z = 1;
		mma8452_platform_data.config_mxc_mma_position = 7;
		adxl34x_default_init.config_adxl34x_position = 0;
		#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AKM8975
		compass_akm8975_platform_data.config_akm_position = 3;
		#endif
		#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AKM8963
		compass_akm8963_platform_data.layout = 3;
		#endif
		l3g4200d_gyr_platform_data.negate_x = 1;
		l3g4200d_gyr_platform_data.negate_y = 1;
		l3g4200d_gyr_platform_data.negate_z = 0;

		break;
	case E_SENSOR_TYPE_T9800D:
		gs_platform_data.negate_x = 0;
		gs_platform_data.negate_y = 1;
                gs_platform_data.negate_z = 1;
		mma8452_platform_data.config_mxc_mma_position = 7;
		adxl34x_default_init.config_adxl34x_position = 0;
		#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AKM8975
		compass_akm8975_platform_data.config_akm_position = 3;
		#endif
		#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AKM8963
		compass_akm8963_platform_data.layout = 3;
		#endif
		l3g4200d_gyr_platform_data.negate_x = 0;
		l3g4200d_gyr_platform_data.negate_y = 1;
                l3g4200d_gyr_platform_data.negate_z = 1;
		break;
    case E_SENSOR_TYPE_U9800D:
		gs_platform_data.negate_x = 0;
		gs_platform_data.negate_y = 1;
                gs_platform_data.negate_z = 1;
		mma8452_platform_data.config_mxc_mma_position = 7;
		adxl34x_default_init.config_adxl34x_position = 0;
		#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AKM8975
		compass_akm8975_platform_data.config_akm_position = 3;
		#endif
		#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AKM8963
		compass_akm8963_platform_data.layout = 3;
		#endif
		l3g4200d_gyr_platform_data.negate_x = 1;
		l3g4200d_gyr_platform_data.negate_y = 1;
                l3g4200d_gyr_platform_data.negate_z = 0;
		break;
	case E_SENSOR_TYPE_PLATFORM:
		gs_platform_data.negate_x = 0;
		gs_platform_data.negate_y = 0;
		mma8452_platform_data.config_mxc_mma_position = 3;
		adxl34x_default_init.config_adxl34x_position = 2;
		#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AKM8975
		compass_akm8975_platform_data.config_akm_position = 1;
		#endif
		#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AKM8963
		compass_akm8963_platform_data.layout = 1;
		#endif
		l3g4200d_gyr_platform_data.negate_x = 0;
		l3g4200d_gyr_platform_data.negate_y = 0;
		break;
        case E_SENSOR_TYPE_C9701L:
		gs_platform_data.negate_x = 1;
		gs_platform_data.negate_y = 1;
                gs_platform_data.negate_z = 0;
		mma8452_platform_data.config_mxc_mma_position = 1;
		adxl34x_default_init.config_adxl34x_position = 0;
		#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AKM8975
		compass_akm8975_platform_data.config_akm_position = 3;
		#endif
		#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AKM8963
		compass_akm8963_platform_data.layout = 7;
		#endif
              #ifdef CONFIG_HUAWEI_FEATURE_SENSORS_YAMAHA_COMPASS
		yamaha_compass_platform_data.layout = 7;
		#endif
		l3g4200d_gyr_platform_data.negate_x = 1;
		l3g4200d_gyr_platform_data.negate_y = 1;
		l3g4200d_gyr_platform_data.negate_z = 0;

		break;
        case E_SENSOR_TYPE_U9700L:
		gs_platform_data.negate_x = 0;
		gs_platform_data.negate_y = 1;
                gs_platform_data.negate_z = 1;
		mma8452_platform_data.config_mxc_mma_position = 7;
		adxl34x_default_init.config_adxl34x_position = 0;
		#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AKM8975
		compass_akm8975_platform_data.config_akm_position = 3;
		#endif
		#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AKM8963
		compass_akm8963_platform_data.layout = 3;
		#endif
              #ifdef CONFIG_HUAWEI_FEATURE_SENSORS_YAMAHA_COMPASS
		yamaha_compass_platform_data.layout = 3;
		#endif
		l3g4200d_gyr_platform_data.negate_x = 0;
		l3g4200d_gyr_platform_data.negate_y = 1;
                l3g4200d_gyr_platform_data.negate_z = 1;
		break;
	case E_SENSOR_TYPE_UEDGE:
		#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_ACC_GYRO_LSM330
		lsm330_acc_pdata.axis_map_x = 1,
		lsm330_acc_pdata.axis_map_y = 0,
		lsm330_acc_pdata.axis_map_z = 2,
		lsm330_acc_pdata.negate_x = 0,
		lsm330_acc_pdata.negate_y = 1,
		lsm330_acc_pdata.negate_z = 0,
		lsm330_gyr_pdata.axis_map_x =1,
		lsm330_gyr_pdata.axis_map_y = 0,
		lsm330_gyr_pdata.axis_map_z = 2,
		lsm330_gyr_pdata.negate_x = 0,
		lsm330_gyr_pdata.negate_y = 0,
		lsm330_gyr_pdata.negate_z = 0,
                  #endif
		gs_platform_data.axis_map_x = 1,
		gs_platform_data.axis_map_y = 0,
		gs_platform_data.axis_map_z = 2,
		gs_platform_data.negate_x = 0;
		gs_platform_data.negate_y = 0;
		gs_platform_data.negate_z = 1;
		mma8452_platform_data.config_mxc_mma_position = 6;
		#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AKM8963
		compass_akm8963_platform_data.layout = 4;
		#endif
                  #ifdef CONFIG_HUAWEI_FEATURE_SENSORS_YAMAHA_COMPASS
		yamaha_compass_platform_data.layout = 0;
		#endif
		l3g4200d_gyr_platform_data.negate_x = 0;
		l3g4200d_gyr_platform_data.negate_y = 0;
                  l3g4200d_gyr_platform_data.negate_z = 0;
		break;
	case E_SENSOR_TYPE_CEDGE:
		#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_ACC_GYRO_LSM330
                  lsm330_acc_pdata.axis_map_x = 0,
		lsm330_acc_pdata.axis_map_y = 1,
		lsm330_acc_pdata.axis_map_z = 2,
		lsm330_acc_pdata.negate_x = 0,
		lsm330_acc_pdata.negate_y = 1,
		lsm330_acc_pdata.negate_z = 1,
		lsm330_gyr_pdata.axis_map_x =0,
		lsm330_gyr_pdata.axis_map_y = 1,
		lsm330_gyr_pdata.axis_map_z = 2,
		lsm330_gyr_pdata.negate_x = 0,
		lsm330_gyr_pdata.negate_y = 0,
		lsm330_gyr_pdata.negate_z = 1,
                  #endif
		gs_platform_data.negate_x = 0;
		gs_platform_data.negate_y = 1;
                  gs_platform_data.negate_z = 1;
		mma8452_platform_data.config_mxc_mma_position = 7;
		#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AKM8963
		compass_akm8963_platform_data.layout = 7;
		#endif
                  #ifdef CONFIG_HUAWEI_FEATURE_SENSORS_YAMAHA_COMPASS
		yamaha_compass_platform_data.layout = 7;
		#endif
		l3g4200d_gyr_platform_data.axis_map_x = 0;
		l3g4200d_gyr_platform_data.axis_map_y = 1;
                  l3g4200d_gyr_platform_data.axis_map_z = 2;
		l3g4200d_gyr_platform_data.negate_x = 0;
		l3g4200d_gyr_platform_data.negate_y = 0;
                  l3g4200d_gyr_platform_data.negate_z = 1;
		break;
	default:
		pr_err("sensor_type unsupported\n");
		break;
	}
}

static int __devinit hw_devices_init(void)
{
	sensor_layout_init();
	i2c_register_board_info(0, k3v2oem1_i2c_0_boardinfo, ARRAY_SIZE(k3v2oem1_i2c_0_boardinfo));
	return 0;
}

core_initcall(hw_devices_init);

MODULE_AUTHOR("huawei skf55108");
MODULE_DESCRIPTION("huawei devices init");
MODULE_LICENSE("GPL");
