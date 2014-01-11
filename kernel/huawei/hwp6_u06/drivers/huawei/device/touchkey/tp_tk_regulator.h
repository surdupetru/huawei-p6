#ifndef _TP_TK_REGULATOR_H
#define _TP_TK_REGULATOR_H

int TP_VCI_GET(struct platform_device *pdev);
int TP_VCI_PUT(void);
int TP_VCI_ENABLE(void);
int TP_VCI_DISABLE(void);

/*-----------------------------------------------------TP_VDDIO---------------------  ------*/
int TP_VDDIO_GET(struct platform_device *pdev);
int TP_VDDIO_PUT(void);
int TP_VDDIO_ENABLE(void);
int TP_VDDIO_DISABLE(void);

/*-----------------------------------------------------TK_VCI---------------------  ------*/
int TK_VCI_GET(struct platform_device *pdev);
int TK_VCI_PUT(void);
int TK_VCI_ENABLE(void);
int TK_VCI_DISABLE(void);


int TK_set_iomux_init(void);
int TK_set_iomux_normal(void);
int TK_set_iomux_lowpower(void);

int TP_set_iomux_init(void);
int TP_set_iomux_normal(void);
int TP_set_iomux_lowpower(void);

int TP_set_gpio_config_lowpower(void);
int TP_set_gpio_config_normal(void);

#endif

