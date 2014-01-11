#include <linux/module.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include <hsad/config_interface.h>
#include <hsad/configdata.h>
#include <hsad/config_mgr.h>
#include <hsad/config_general_struct.h>
#include <hsad/config_debugfs.h>

#define MAX_KEY_LENGTH 48
#define MAX_SENSOR_LAYOUT_LEN 10
#define MAX_SPK_TYPE_LEN 10 
#define MAX_WIFI_FW_NAME_LEN 25

/**
 * this function just make a judge whether the file node contain
 * the string pname.
**/
bool product_type(char *pname)
{
	char product_name[MAX_KEY_LENGTH];
	memset(product_name, 0, sizeof(product_name));
	get_hw_config_string("product/name", product_name, MAX_SENSOR_LAYOUT_LEN, NULL);
	if (strstr(product_name, pname))
		return true;
	else
		return false;
}

/**
 * this function make a full comparsion between two strings
**/
bool product_fullname(char *pname)
{
	char product_name[MAX_KEY_LENGTH];
	memset(product_name, 0, sizeof(product_name));
	get_hw_config_string("product/name", product_name, MAX_SENSOR_LAYOUT_LEN, NULL);
	if (!strcmp(product_name, pname))
		return true;
	else
		return false;
}

bool get_spk_pa(char *spk_pa)  
{  
	char spk_name[MAX_KEY_LENGTH];  
	memset(spk_name, 0, sizeof(spk_name));  
	get_hw_config_string("audio/spk_pa", spk_name, MAX_SPK_TYPE_LEN, NULL);  
	if (strstr(spk_name, spk_pa))  
		return true;  
	else  
		return false;  
}  

int get_touchscreen_type(void)
{
	unsigned int type = 0;

	bool ret = get_hw_config_int("touchscreen/touchscreen_type", &type, NULL);
	HW_CONFIG_DEBUG("hsad: touchscreen_type = %d, ret = %d\n", type, ret);
	if (ret == true) {
		return type;
	}

	return -1;
}

int get_usbphy_tune(void)
{
	unsigned int value = 0;

	bool ret = get_hw_config_int("usbphy/usbphy_type", &value, NULL);
	HW_CONFIG_DEBUG("hsad: usbphy_type = %d,  ret = %d\n", value, ret);
	if (ret == true) {
		return value;
	}

	return -1;
}

int get_board_type(void)
{
	unsigned int type = 0;

	bool ret = get_hw_config_int("board/board_type", &type, NULL);
	HW_CONFIG_DEBUG("hsad: board_type = %d\n,  ret = %d", type, ret);
	if (ret == true) {
		return type;
	}

	return -1;
}

int get_sd_detect_type(void)
{
	unsigned int type = 0;

	bool ret = get_hw_config_int("board/sd_detect_type", &type, NULL);
	HW_CONFIG_DEBUG("hsad: sd_detect_type = %d, ret = %d\n", type, ret);
	if (ret == true) {
		return type;
	}

	return GPIO_HIGH_MEAN_DETECTED;
}

int get_sensor_type(void)
{
	unsigned int type = 0;

	bool ret = get_hw_config_int("sensor/sensor_type", &type, NULL);
	HW_CONFIG_DEBUG("hsad: sensor_type = %d,  ret = %d\n", type, ret);
	if (ret == true) {
		return type;
	}

	return -1;
}
int get_compass_softiron_type(void)
{
	unsigned int type = 0;

	bool ret = get_hw_config_int("sensor/compass_softiron_type", &type, NULL);
	HW_CONFIG_DEBUG("hsad: compass_softiron_type = %d,  ret = %d\n", type, ret);
	if (ret == true) {
		return type;
	}

	return -1;
}
int get_gyro_exist_info(void)
{
	unsigned int type = 0;

	bool ret = get_hw_config_int("sensor/gyro_exist_info", &type, NULL);
	HW_CONFIG_DEBUG("hsad: gyro_exist = %d,  ret = %d\n", type, ret);
	if (ret == true) {
		return type;
	}

	return -1;
}

int get_rgb_sensor_type(void)
{
	unsigned int type = 0;

	bool ret = get_hw_config_int("sensor/sensor_rgb_type", &type, NULL);
	HW_CONFIG_DEBUG("hsad: rgb_sensor_type = %d\n", type);
	if (ret == true) {
		return type;
	}

	return -1;
}
int get_apds_type(void)
{
	unsigned int type = 0;

	bool ret = get_hw_config_int("apds/apds_type", &type, NULL);
	HW_CONFIG_DEBUG("hsad: apds_type = %d,  ret = %d\n", type, ret);
	if (ret == true) {
		return type;
	}

	return -1;
}
int get_sensor_timing_type(void)
{
	unsigned int type = 0;

	bool ret = get_hw_config_int("camera/camera_timing_type", &type, NULL);
	HW_CONFIG_DEBUG("hsad: camera_timing_type = %d,  ret = %d\n", type, ret);
	if (ret == true) {
		return type;
	}

	return -1;
}

int get_primary_sensor_flip_type(void)
{
        unsigned int type = E_CAMERA_SENSOR_FLIP_TYPE_NONE;

        bool ret = get_hw_config_int("camera/primary_sensor_flip_type", &type, NULL);
        HW_CONFIG_DEBUG("hsad: primary_sensor_flip_type = %d\n", type);
        if (ret == true) {
                return type;
        }

        return E_CAMERA_SENSOR_FLIP_TYPE_NONE;
}

int get_secondary_sensor_flip_type(void)
{
        unsigned int type = E_CAMERA_SENSOR_FLIP_TYPE_H_V;

        bool ret = get_hw_config_int("camera/secondary_sensor_flip_type", &type, NULL);
        HW_CONFIG_DEBUG("hsad: secondary_sensor_flip_type = %d\n", type);
        if (ret == true) {
                return type;
        }

        return E_CAMERA_SENSOR_FLIP_TYPE_H_V;
}

int check_suspensory_camera(char *cname)
{
	unsigned int type = 0;
	bool ret;
	char camera_name[32] = "camera/";

	strcat(camera_name, cname);
	ret = get_hw_config_int(camera_name, &type, NULL);
	HW_CONFIG_DEBUG("hsad: camera_name = %s, camera_type = %d\n", camera_name,type);
	if (ret == true) {
		return type;
	}

	return -1;
}

int get_sensor_tp_type(void)
{
	unsigned int type = 0;

	bool ret = get_hw_config_int("sensor_v4/sensor_tp_type", &type, NULL);
	HW_CONFIG_DEBUG("hsad: sensor_tp_type = %d\n", type);
	if (ret == true) {
		return type;
	}

	return -1;
}
int get_rgb_sensor_value(void)
{
	int value = 0;
	bool ret = get_hw_config_int("RGB_sensor/rgb_sensor_value", &value, NULL);
	if( ret == true)
		{
		 return value;
		}
	return 0;
}

int get_iomux_type(void)
{
	unsigned int type = 0;

	bool ret = get_hw_config_int("iomux/iomux_type", &type, NULL);
	HW_CONFIG_DEBUG("hsad: iomux_type = %d,  ret = %d\n", type, ret);
	if (ret == true) {
#if defined(CONFIG_IO_COMPATIBILITY_DIFFRENT_LCD)
#if defined(CONFIG_LCD_CMI_OTM1280A)
		type = E_IOMUX_PALTFORM_CMI_OTM1280A;
#elif defined(CONFIG_LCD_TOSHIBA_MDW70)
		type = E_IOMUX_PALTFORM_TOSHIBA_MW70;
#elif defined(CONFIG_LCD_TOSHIBA_MDY90)
		type = E_IOMUX_PALTFORM_TOSHIBA_MY90;
#endif
#endif
		return type;
	}

	return -1;
}

/* gpio get gpio struct */
#ifdef CONFIG_HUAWEI_GPIO_UNITE
struct gpio_config_type *get_gpio_config_table(void)
{
	struct board_id_general_struct *gpios_ptr = get_board_id_general_struct(GPIO_MODULE_NAME);
	struct gpio_config_type *gpio_ptr;

	if (NULL == gpios_ptr) {
		HW_CONFIG_DEBUG(" can not find  module:gpio\n");
		return NULL;
	}

	gpio_ptr = (struct gpio_config_type *)gpios_ptr->data_array.gpio_ptr;

    if (NULL != gpio_ptr) {
		return gpio_ptr;
	} else {
		HW_CONFIG_DEBUG(" return NULL\n");
		return NULL;
	}
}

/*gpio get number by name*/
int get_gpio_num_by_name(char *name)
{
    int min = 0;
    int max = NR_GPIO_IRQS - 1;
    int result;
    int new_cursor;
	struct gpio_config_type *gpio_ptr = get_gpio_config_table();

    if (NULL == gpio_ptr) {
		HW_CONFIG_DEBUG(" get gpio struct failed.\n");
		return -EFAULT;
    }

	while (min <= max) {
		new_cursor = (min+max)/2;

		if (!(strcmp((gpio_ptr + new_cursor)->name, ""))) {
			result = 1;
		} else {
			result = strcmp((gpio_ptr+new_cursor)->name, name);
		}

		if (0 == result) {
			/*found it, just return*/
			return (gpio_ptr+new_cursor)->gpio_number;
		} else if (result > 0) {
			/* key is smaller, update max*/
			max = new_cursor-1;
		} else if (result < 0) {
			/* key is bigger, update min*/
			min = new_cursor+1;
		}
	}

	return -EFAULT;
}

struct pm_gpio_cfg_t *get_pm_gpio_config_table(void)
{
	struct board_id_general_struct *pm_gpios_ptr = get_board_id_general_struct(PM_GPIO_MODULE_NAME);
	struct pm_gpio_cfg_t *pm_gpio_ptr;

	if (NULL == pm_gpios_ptr) {
		HW_CONFIG_DEBUG(" can not find  module:pm gpio\n");
		return NULL;
	}

	pm_gpio_ptr = (struct pm_gpio_cfg_t *)pm_gpios_ptr->data_array.pm_gpio_ptr;

    if (NULL != pm_gpio_ptr) {
		return pm_gpio_ptr;
	} else {
		HW_CONFIG_DEBUG(" return NULL\n");
		return NULL;
	}
}

int get_pm_gpio_num_by_name(char *name)
{
	int min = 0;
    int max = PM8921_GPIO_NUM - 1;
    int result;
    int new_cursor;
    struct pm_gpio_cfg_t *pm_gpio_ptr = get_pm_gpio_config_table();

	if (NULL == pm_gpio_ptr) {
		HW_CONFIG_DEBUG(" get pm gpio config table failed.\n");
		return -EFAULT;
    }

    while (min <= max) {
		new_cursor = (min + max) / 2;

		if (!(strcmp((pm_gpio_ptr + new_cursor)->name, ""))) {
			result = 1;
		} else {
			result = strcmp((pm_gpio_ptr + new_cursor)->name, name);
		}

		if (0 == result) {
			/*found it, just return*/
			return (pm_gpio_ptr+new_cursor)->gpio_number;
		} else if (result > 0) {
			/* key is smaller, update max*/
			max = new_cursor-1;
		} else if (result < 0) {
			/* key is bigger, update min*/
			min = new_cursor+1;
		}
	}

	return -EFAULT;
}
#endif

#ifdef CONFIG_HW_POWER_TREE
struct hw_config_power_tree *get_power_config_table(void)
{
	struct board_id_general_struct *power_general_struct = get_board_id_general_struct(POWER_MODULE_NAME);
	struct hw_config_power_tree *power_ptr = NULL;

	if (NULL == power_general_struct) {
		HW_CONFIG_DEBUG("Can not find module:regulator\n");
		return NULL;
	}

	power_ptr = (struct hw_config_power_tree *)power_general_struct->data_array.power_tree_ptr;

	if (NULL == power_ptr) {
		HW_CONFIG_DEBUG("hw_config_power_tree return  NULL\n");
	}
	return power_ptr;
}
#endif

int get_hsd_invert(void)
{
	unsigned int type = 0;

	bool ret = get_hw_config_int("audio/hsd_invert", &type, NULL);
	HW_CONFIG_DEBUG("hsad: hsd_invert = %d,  ret = %d\n", type, ret);
	if (ret == true) {
		return type;
	}

	return -1;
}

int get_hs_keys(void)
{
	unsigned int type = 0;

	bool ret = get_hw_config_int("audio/hs_keys", &type, NULL);
	HW_CONFIG_DEBUG("hsad: hs_keys = %d,  ret = %d\n", type, ret);
	if (ret == true) {
		return type;
	}

	return -1;
}

int get_audience_i2c_addr(void)
{
	unsigned int type = 0;

	bool ret = get_hw_config_int("audio/audience_addr", &type, NULL);
	HW_CONFIG_DEBUG("hsad: audience_addr = %d\n", type);
	if (ret == true) {
		return type;
	}

	return -1;
}

#define MAX_MODEM_TYPE_LENGTH 48
int get_modem_type(char *ptype)
{
    unsigned int type = 0;
    char modem_type[MAX_MODEM_TYPE_LENGTH];

        snprintf(modem_type, sizeof(modem_type), "modem/%s", ptype);
	bool ret = get_hw_config_int(modem_type, &type, NULL);
	HW_CONFIG_DEBUG("hsad: modem_type. \"%s\". %d\n", modem_type,type);
	if (ret == true) {
		return type;
	}

	return -1;
}

int get_if_use_3p75M_uart_clock(void)
{
	unsigned int value =0;
	bool ret = get_hw_config_int("clock/use_3p75Mbps_uart", &value, NULL);
	if (ret == true) {
		return value;
	}
	
	return 0;
}

#define MAX_DEVICES_LENGTH 64
int get_I2C_devices(char *ptype)
{
    unsigned int type = 0;
    bool ret ;
    char I2C_devices[MAX_DEVICES_LENGTH];

    snprintf(I2C_devices, sizeof(I2C_devices), "i2c/%s", ptype);
    ret = get_hw_config_int(I2C_devices, &type, NULL);
    HW_CONFIG_DEBUG("hsad: I2C_devices. \"%s\". %d\n", I2C_devices,type);
    if (ret == true) {
        return type;
    }

    return -1;
}

int get_connectivity_devices(char *ptype)
{
    unsigned int type = 0;
    bool ret ;
    char connectivity_devices[MAX_DEVICES_LENGTH];

    snprintf(connectivity_devices, sizeof(connectivity_devices), "connectivity/%s", ptype);
    ret = get_hw_config_int(connectivity_devices, &type, NULL);
    HW_CONFIG_DEBUG("hsad: connectivity_devices. \"%s\". %d\n", connectivity_devices,type);
    if (ret == true) {
        return type;
    }

    return -1;
}

int get_gpu_dcdc_supply(void)
{
	unsigned int supply = 0;

	bool ret = get_hw_config_int("gpu/gpu_dcdc_supply", &supply, NULL);
	HW_CONFIG_DEBUG("hsad: gpu_dcdc_supply = %d\n", supply);
	if (ret == true) {
		return supply;
	} else {
        return 0;
    }
}

int get_motor_board_type(void)
{
	unsigned int type = 0;

	bool ret = get_hw_config_int("audio/motor_type", &type, NULL);
	HW_CONFIG_DEBUG("hsad: motor_type = %d\n", type);
	if (ret == true) {
		return type;
	}

	return -1;
}

int get_wifi_wl18xx_dcdc_config(void)
{
	unsigned int type = 0;

	bool ret = get_hw_config_int("wifi/wl1873_dcdc", &type, NULL);
	HW_CONFIG_DEBUG("hsad: wl1873_dcdc = %d\n", type);
	if (ret == true) {
		return type;
	}

	return -1;	
}
EXPORT_SYMBOL(get_wifi_wl18xx_dcdc_config);

bool get_wifi_wl18xx_rf_fw_name(char *board_name)
{
    if(board_name == NULL)
        return false;
        
    if(get_hw_config_string("wifi/wl1873_conf", board_name, MAX_WIFI_FW_NAME_LEN, NULL)){
        return true;
    }
    else{
        return false;
    }
}
EXPORT_SYMBOL(get_wifi_wl18xx_rf_fw_name);

bool is_felica_exist(void)
{
    char *psKey = NULL;
    bool bRet = false;

    if (NULL == (psKey = kmalloc(MAX_KEY_LENGTH, GFP_KERNEL)))
    {
        return false;
    }
    memset(psKey, 0, MAX_KEY_LENGTH);
    sprintf(psKey, "felica/felica");
    
    get_hw_config_bool(psKey, &bRet, NULL);
    
    kfree(psKey);

    return bRet;
}

int get_felica_board_type(void)
{
	unsigned int type = 0;

	bool ret = get_hw_config_int("felica/board_type", &type, NULL);
	HW_CONFIG_DEBUG("hsad: felica_board_type = %d\n", type);
	if (ret == true) {
		return type;
	}

	return -1;
}

int get_uart_headset_type(void)
{
        unsigned int type = E_UART_HEADSET_TYPE_NOT_SUPPORTED;

        bool ret = get_hw_config_int("uart_headset/type", &type, NULL);
        HW_CONFIG_DEBUG("hsad: uart_headset_type = %d\n", type);
        if (ret == true) {
                return type;
        }

        return E_UART_HEADSET_TYPE_NOT_SUPPORTED;
}

int get_touchkey_key_port(void)
{
    unsigned int type = 0;

    bool ret = get_hw_config_int("touchkey/key_port", &type, NULL);
    HW_CONFIG_DEBUG("hsad: get_touchkey_key_port = %d\n", type);
    if (ret == true) {
        return type;
    }

    return -1;
}
int get_touchkey_led_gpio_enable(void)
{
    int type = 0;

    bool ret = get_hw_config_int("touchkey/led_gpio_008", &type, NULL);
    HW_CONFIG_DEBUG("hsad: get_touchkey_led_gpio_enable = %d\n", type);
    if (ret == true) {
        return type;
    }

    return -1;
}
int get_touchkey_regulator_vout(void)
{
    unsigned int type = 0;

    bool ret = get_hw_config_int("touchkey/regulator_vout", &type, NULL);
    HW_CONFIG_DEBUG("hsad: get_touchkey_regulator_vout = %d\n", type);
    if (ret == true) {
        return type;
    }

    return -1;
}
int get_touchkey_led_brightness(void)
{
    unsigned int type = 0;

    bool ret = get_hw_config_int("touchkey/led_brightness", &type, NULL);
    HW_CONFIG_DEBUG("hsad: get_touchkey_led_brightness = %d\n", type);
    if (ret == true) {
        return type;
    }

    return -1;
}

int get_touchkey_sensitivity(void)
{
    unsigned int type = 0;

    bool ret = get_hw_config_int("touchkey/key_sensitivity", &type, NULL);
    HW_CONFIG_DEBUG("hsad: get_touchkey_sensitivity = %d\n", type);
    if (ret == true) {
        return type;
    }

    return -1;
}

/* get correct axis align for touch begin*/
int get_touchscreen_axis_align(void)
{
	bool ret;
	unsigned int type = 0;
	ret = get_hw_config_int("touchscreen/touchscreen_axis_align", &type, NULL);
	HW_CONFIG_DEBUG("hsad: get_touchscreen_axis_align = %d\n", type);
	if (true == ret)
		return type;

	return -1;
}

/* get correct axis align for touch begin*/

int get_touchscreen_fw_type(void)
{
	bool ret;
	unsigned int type = 0;
	ret = get_hw_config_int("touchscreen/firmware_type", &type, NULL);
	HW_CONFIG_DEBUG("hsad: get_touchscreen_fw_type = %d\n", type);
	if (ret == true)
		return type;

	return -1;
}


int get_touchkey_sensitivity_glove(void)
{
    unsigned int type = 0;

    bool ret = get_hw_config_int("touchkey/key_sensitivity_glove", &type, NULL);
    HW_CONFIG_DEBUG("hsad: get_touchkey_sensitivity_glove = %d\n", type);
    if (ret == true) {
        return type;
    }
    return -1;
}

int get_vibrator_vout_number(void)
{
    unsigned int value = 0;

    bool ret = get_hw_config_int("audio/type_1_vout_number", &value, NULL);
    if (ret == true){
	return (int)value;
    }

    return -1;
}

int get_vibrator_vout_min_voltage(void)
{
    unsigned int value = 0;

    bool ret = get_hw_config_int("audio/type_1_vout_min_voltage", &value, NULL);
    if (ret == true){
	return (int)value;
    }

    return -1;
}

int get_vibrator_vout_max_voltage(void)
{
    unsigned int value = 0;

    bool ret = get_hw_config_int("audio/type_1_vout_max_voltage", &value, NULL);
    if (ret == true){
	return (int)value;
    }

    return -1;
}

bool get_so340010_enable(void)
{
    bool ret;
    bool value=false;

    ret = get_hw_config_bool("touchkey/so340010_enable", &value, NULL);
    if (ret == true){
        return value;
    }

    /* default enable */
    return true;
}

bool get_cyttsp4_enable(void)
{
    bool ret;
    bool value=false;

    ret = get_hw_config_bool("touchscreen/cyttsp4_enable", &value, NULL);
    if (ret == true){
        return value;
    }

    /* default enable */
    return true;
}

bool get_jdi_tk_enable(void)
{
    bool ret;
    bool value=false;

    ret = get_hw_config_bool("touchkey/jdi_tk_enable", &value, NULL);
    if (ret == true){
        return value;
    }

    /* default enable */
    return true;
}

int get_camera_focus_key(void)
{
    unsigned int type = 0;

    bool ret = get_hw_config_int("camera/camera_focus_key", &type, NULL);
    HW_CONFIG_DEBUG("hsad: get_camera_focus_key = %d\n", type);
    printk("%s ret=%d, type=%d.\n", __func__, ret, type);
    if (ret == true) {
        return type;
    }

    return 0;
}

int get_u9700_ldo_ctrl(void)
{
    unsigned int type = 0;

    bool ret = get_hw_config_int("u9700_ldo/sleep_ctrl", &type, NULL);
    HW_CONFIG_DEBUG("hsad: get_u9700_ldo_sleep_ctrl = %d\n", type);
    if (ret == true) {
        return type;
    }

    return -1;
}

bool get_sd_enable(void)
{
    bool ret;
    bool value=false;

    ret = get_hw_config_bool("sd/enable", &value, NULL);
    if (true == ret){
        return value;
    }

    /* default enable */
    return true;
}

bool get_pmu_out26m_enable(void)
{
    bool ret;
    bool value=false;

    ret = get_hw_config_bool("clock/pmu_out26m_enable", &value, NULL);
    if (ret == true){
        return value;
    }

    /* default enable */
    return true;
}

int get_nfc_module(void)
{
        unsigned int type = 0;

	bool ret = get_hw_config_int("nfc/nfc_module", &type, NULL);
	HW_CONFIG_DEBUG("hsad: nfc_module = %d, ret = %d\n", type, ret);
	if (ret == true) {
		return (int)type;
	}

	return 0;
}

int get_nfc_product(void)
{
	unsigned int type = 0;
	bool ret = get_hw_config_int("nfc/nfc_product", &type, NULL);
	HW_CONFIG_DEBUG("hsad: nfc_product = %d\n", type);
	if (ret == true) {
	         return (int)type;
	}
	return 0;
}

bool is_modem_switch_support(void)
{
    char name[MAX_KEY_LENGTH]={0};
    if(get_hw_config_string("audio/modem_switch", name, MAX_KEY_LENGTH, NULL)){
        if (!strncmp(name, "MODEM_SWITCH", strlen("MODEM_SWITCH"))) {
            return true;
        }
    }
    return false;
}
