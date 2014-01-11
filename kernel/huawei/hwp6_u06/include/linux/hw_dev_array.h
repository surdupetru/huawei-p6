#ifndef _HUAWEI_DEV_ARRAY_H_
#define _HUAWEI_DEV_ARRAY_H_
#include "hw_dev_dec.h"

typedef struct {
	int devices_id;
	char* devices_name;
}hw_dec_struct;

static hw_dec_struct hw_dec_device_array[] =
{
	{ DEV_I2C_TOUCH_PANEL,"touch_panel" },
	{ DEV_I2C_COMPASS,"compass" },
	{ DEV_I2C_G_SENSOR,"g_sensor" },
	{ DEV_I2C_CAMERA_MAIN,"camera_main" },
	{ DEV_I2C_CAMERA_SLAVE,"camera_slave" },
	{ DEV_I2C_KEYPAD,"keypad" },
	{ DEV_I2C_APS,"aps" },
	{ DEV_I2C_GYROSCOPE,"gyroscope" },
	{ DEV_I2C_NFC,"nfc" },
	{ DEV_I2C_DC_DC,"dc_dc" },
	{ DEV_I2C_SPEAKER,"speark" },
	{ DEV_I2C_OFN,"ofn" },
	{ DEV_I2C_TPS,"tps" },
	{ DEV_I2C_L_SENSOR,"l_sensor" },
	{ DEV_I2C_CHARGER,"charge" },
	{ DEV_I2C_BATTERY,"battery" },
	{ DEV_I2C_NTC,"ntc" },
	{ DEV_I2C_MHL,"mhl" },
	{ DEV_I2C_AUDIENCE,"audience" },
	{ DEV_I2C_IRDA,"irda" },	
	{ DEV_I2C_MAX,"NULL" },
	{ DEV_CONNECTIVITY_WIFI,"wifi" },
	{ DEV_CONNECTIVITY_BT,"bt" },
	{ DEV_CONNECTIVITY_FM,"fm" },
	{ DEV_CONNECTIVITY_GPS,"gps" },
};

#endif