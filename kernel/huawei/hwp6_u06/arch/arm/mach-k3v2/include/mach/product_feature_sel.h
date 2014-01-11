
#ifndef _PRODUCT_FEATURE_SEL_H__
#define _PRODUCT_FEATURE_SEL_H__

enum PROD_FEATURE_SEL_TYPE{
	PROD_FEATURE_GPU_DCDC_SUPPLY,
	PROD_FEATURE_SDP_CHARGER_CURRENT,
	PROD_FEATURE_USB_OUT_5V_SUPPLY,
	PROD_FEATURE_MAX_NUM,
};

unsigned int get_product_feature(enum PROD_FEATURE_SEL_TYPE feature_type);
void init_product_feature_array();
#endif
