#ifndef _HIUSB_ANDROID
#define _HIUSB_ANDROID
#include <mach/platform.h>
#include <mach/lm.h>
#include <linux/usb.h>
#include <linux/usb/gadget.h>

#define PER_RST_EN_1_ADDR           (IO_ADDRESS(REG_BASE_SCTRL + 0x08C))
#define PER_RST_DIS_1_ADDR          (IO_ADDRESS(REG_BASE_SCTRL + 0x090))
#define PER_RST_STATUS_1_ADDR       (IO_ADDRESS(REG_BASE_SCTRL + 0x094))

#define IP_RST_HSICPHY              (1 << 25)
#define IP_RST_PICOPHY              (1 << 24)
#define IP_RST_NANOPHY              (1 << 23)

#define PER_RST_EN_3_ADDR           (IO_ADDRESS(REG_BASE_SCTRL + 0x0A4))
#define PER_RST_DIS_3_ADDR          (IO_ADDRESS(REG_BASE_SCTRL + 0x0A8))
#define PER_RST_STATUS_3_ADDR       (IO_ADDRESS(REG_BASE_SCTRL + 0xAC))

#define IP_PICOPHY_POR              (1 << 31)
#define IP_HSICPHY_POR              (1 << 30)
#define IP_NANOPHY_POR              (1 << 29)
#define IP_RST_USB2DVC_PHY          (1 << 28)
#define IP_RST_USB2DVC              (1 << 17)

#define PER_CLK_EN_1_ADDR           (IO_ADDRESS(REG_BASE_SCTRL + 0x30))
#define PER_CLK_DIS_1_ADDR          (IO_ADDRESS(REG_BASE_SCTRL + 0x34))
#define PER_CLK_EN_STATUS_1_ADDR    (IO_ADDRESS(REG_BASE_SCTRL + 0x38))
#define PER_CLK_STATUS_1_ADDR       (IO_ADDRESS(REG_BASE_SCTRL + 0x3C))

#define GT_CLK_USBHSICPHY480        (1 << 26)
#define GT_CLK_USBHSICPHY           (1 << 25)
#define GT_CLK_USBPICOPHY           (1 << 24)
#define GT_CLK_USBNANOPHY           (1 << 23)

#define PER_CLK_EN_3_ADDR           (IO_ADDRESS(REG_BASE_SCTRL + 0x50))
#define PER_CLK_DIS_3_ADDR          (IO_ADDRESS(REG_BASE_SCTRL + 0x54))
#define PER_CLK_EN_STATUS_3_ADDR    (IO_ADDRESS(REG_BASE_SCTRL + 0x58))
#define PER_CLK_STATUS_3_ADDR       (IO_ADDRESS(REG_BASE_SCTRL + 0x5C))

#define GT_CLK_USB2HST              (1 << 18)
#define GT_CLK_USB2DVC              (1 << 17)

#define PERI_CTRL0                  (IO_ADDRESS(REG_BASE_PCTRL + 0x0))
#define USB2DVC_NANOPHY_BIT         (1 << 7)

#define PERI_CTRL14                 (IO_ADDRESS(REG_BASE_PCTRL + 0x38))
#define NANOPHY_SIDDQ               (1 << 0)
#define NANOPHY_DP_PULLDOWN         (1 << 5)
#define NANOPHY_DM_PULLDOWN         (1 << 6)

#define PERI_CTRL16                 (IO_ADDRESS(REG_BASE_PCTRL + 0x40))
#define PICOPHY_SIDDQ               (1 << 0)
#define PERI_CTRL16_PICOPHY_TXPREEMPHASISTUNE (1 << 31)

#define PERI_CTRL17                 (IO_ADDRESS(REG_BASE_PCTRL + 0x44))

#define PERI_CTRL21                 (IO_ADDRESS(REG_BASE_PCTRL + 0x01F4))

#define BC_EN_ADDR                  (IO_ADDRESS(REG_BASE_PCTRL + 0x01A8))
#define BC_CMD_ADDR                 (IO_ADDRESS(REG_BASE_PCTRL + 0x01AC))
#define BC_ST_ADDR                  (IO_ADDRESS(REG_BASE_PCTRL + 0x01B0))
#define TSENSOR_ADDR                (IO_ADDRESS(REG_BASE_PCTRL + 0xEC))

#define PMU_STATUS0_ADDR            (IO_ADDRESS(REG_BASE_PMUSPI + (0x07 << 2)))
#define VBUS5P5_D10R                (1 << 3)
#define VBUS4P5_D10                 (1 << 2)
#define VBUS_COMP_VBAT_D20          (1 << 1)

#define PMU_IRQ2_ADDR            (IO_ADDRESS(REG_BASE_PMUSPI + (0x02 << 2)))
#define PMU_IRQ2_MASK_ADDR            (IO_ADDRESS(REG_BASE_PMUSPI + (0x05 << 2)))


enum usb_charger_type{
	CHARGER_TYPE_USB = 0,
	CHARGER_TYPE_BC_USB,
	CHARGER_TYPE_NON_STANDARD,
	CHARGER_TYPE_STANDARD,
	CHARGER_REMOVED,
    USB_EVENT_OTG_ID,
};

int hiusb_probe_phase1(struct lm_device *_dev);
int hiusb_probe_phase2(struct lm_device *_dev);
int hiusb_remove(struct lm_device *_dev);
int hiusb_charger_registe_notifier(struct notifier_block *nb);
int hiusb_charger_unregiste_notifier(struct notifier_block *nb);
int get_charger_name(void);
int hiusb_pullup(struct usb_gadget *gadget, int is_on);
int hiusb_suspend(struct lm_device *_dev);
int hiusb_resume(struct lm_device *_dev);

unsigned int hiusb_read_phy_param(void);
unsigned int hiusb_get_phy_param(void);
unsigned int hiusb_set_phy_param(unsigned int value);
void hiusb_switch_to_host_test_package(void);
void hiusb_switch_to_device_test_package(void);

enum {
	ID_RISE = 0,
	ID_FALL
};

int k3v2_otg_id_status_change(int id_status);

#endif

