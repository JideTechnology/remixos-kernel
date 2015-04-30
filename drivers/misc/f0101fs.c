#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/suspend.h>
#include <mach/gpio-tegra.h>
#include "../../arch/arm/mach-tegra/gpio-names.h"
/************************************************************************************************/
/*furao add BT/WIFI/NFC definitions*/
#define APH_WF_EN			TEGRA_GPIO_PCC5
#define CWL_WF2AP_WAKE			TEGRA_GPIO_PU5
#define BT_EN				TEGRA_GPIO_PR1
#define BT_IRQ				TEGRA_GPIO_PU6
#define APH_BT_WAKEUP			TEGRA_GPIO_PEE1
#define NFC_IRQ_N			TEGRA_GPIO_PR4
#define NFC_WAKE			TEGRA_GPIO_PR5
#define NFC_EN				TEGRA_GPIO_PR6
/*FURAO TEMP definitions */
#define MIPI_LVDS_INT			TEGRA_GPIO_PO0
#define HDMI_INT			TEGRA_GPIO_PN7
#define APB_EN_HDMI_VDD			TEGRA_GPIO_PK6
#define AMB_INT				TEGRA_GPIO_PC7
#define GYRO_IRQ			TEGRA_GPIO_PR3
#define FG_BATT_LOW			TEGRA_GPIO_PQ5
#define APH_AUD_GPIO1			TEGRA_GPIO_PV3
/*furao add CTP Touchscreen definitions */
#define MAXIM_CAP_TP_RESET		TEGRA_GPIO_PP3
#define MAXIM_CAP_TP_INT		TEGRA_GPIO_PV1
/*furao add ETP Touchscreen definitions */
#define HV0868_ETP_INT			TEGRA_GPIO_PO5
#define HV0868_ETP_RESET		TEGRA_GPIO_PP0
#define HV0868_ETP_SLEEP		TEGRA_GPIO_PO6
#define HV0868_ETP_CFG0			TEGRA_GPIO_PO7
/*furao add SC7702 BB definitions */
#define SC7702_BB_RESET			TEGRA_GPIO_PP2
#define SC7702_BB_KPDPWR		TEGRA_GPIO_PH4
#define SC7702_AP_WAKEUP_BB		TEGRA_GPIO_PP1
#define SC7702_BB_WAKEUP_AP		TEGRA_GPIO_PV0
#define SC7702_3G_PWR_EN		TEGRA_GPIO_PH5
/*furao add GPS definitions */
#define BCM4752_GPS_EN			TEGRA_GPIO_PP1
/*----------------------------------------------------------------------------------------------------------*/
static 	ssize_t gps_enable_func(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int val;
	int rc;
	if (!(sscanf(buf, "%u\n", &val))) return -EINVAL;
		rc = gpio_request(BCM4752_GPS_EN,"bcm4752_gps_en");
		switch(val) {
			case 0:				
			printk("----------------------gps_enable_func 0 on-----------------");
			gpio_direction_output(BCM4752_GPS_EN, 0);
			break;

			case 1:
			printk("----------------------gps_enable_func 1 on-----------------");
			gpio_direction_output(BCM4752_GPS_EN, 1);
			break;

			default:
			break;
		}
		gpio_free(BCM4752_GPS_EN);
	return count;
}
static	DEVICE_ATTR(gps_enable, S_IRWXUGO, NULL, gps_enable_func);
/*----------------------------------------------------------------------------------------------------------*/
static 	ssize_t bb_on_func(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int val;

	if (!(sscanf(buf, "%u\n", &val))) return -EINVAL;

		switch(val) {
			case 0:				
			printk("----------------------bb_on_func 0 on-----------------");

			break;

			case 1:
			printk("----------------------bb_on_func 1 on-----------------");

			break;

			default:
			break;
		}
	return count;
}
static	DEVICE_ATTR(bb_on, S_IRWXUGO, NULL, bb_on_func);
/*----------------------------------------------------------------------------------------------------------*/
static 	ssize_t bb_reset_func(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int val;

	if (!(sscanf(buf, "%u\n", &val))) return -EINVAL;

		switch(val) {
			case 0:				
			printk("----------------------bb_reset_func 0 on-----------------");

			break;

			case 1:
			printk("----------------------bb_reset_func 1 on-----------------");

			break;

			default:
			break;
		}
	return count;
}
static	DEVICE_ATTR(bb_reset, S_IRWXUGO, NULL, bb_reset_func);
/*----------------------------------------------------------------------------------------------------------*/
static 	ssize_t bbpower_on_func(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int val;

	if (!(sscanf(buf, "%u\n", &val))) return -EINVAL;

		switch(val) {
			case 0:				
			printk("----------------------bbpower_on 0 on-----------------");

			break;

			case 1:
			printk("----------------------bbpower_on 1 on-----------------");

			break;

			default:
			break;
		}
	return count;
}
static	DEVICE_ATTR(bbpower_on, S_IRWXUGO, NULL, bbpower_on_func);

/*-------------------------------------------------------------------------------------------------------------*/
static struct attribute *f0101_board_sysfs_entries[] = {
	&dev_attr_gps_enable.attr,
	&dev_attr_bb_on.attr,
	&dev_attr_bb_reset.attr,
	&dev_attr_bbpower_on.attr,
	NULL
};

static const struct attribute_group f0101_board_sysfs_attr_group = {
	.name = NULL,
	.attrs = f0101_board_sysfs_entries,
};
/*-------------------------------------------------------------------------------------------------------------*/
static int f0101_board_sysfs_probe(struct platform_device *pdev)
{
	return sysfs_create_group(&pdev->dev.kobj, &f0101_board_sysfs_attr_group);
}

static int f0101_board_sysfs_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &f0101_board_sysfs_attr_group);
	return 0;
}
static int f0101_board_sysfs_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int f0101_board_sysfs_resume(struct platform_device *pdev)
{
	return 0;
}
/*-------------------------------------------------------------------------------------------------------------*/
static struct platform_driver f0101_board_platform_driver = {
	.driver = {
		.name = "sysf0101fs",
		.owner = THIS_MODULE,
	},
	.probe = f0101_board_sysfs_probe,
	.remove = f0101_board_sysfs_remove,
	.suspend = f0101_board_sysfs_suspend,
	.resume = f0101_board_sysfs_resume,
};
static struct platform_device *f0101_board_platform_device = NULL;
/*-------------------------------------------------------------------------------------------------------------*/
static int f0101_board_sysfs_init(void)
{
	unsigned int tmp;
	f0101_board_platform_device = platform_device_register_data(NULL, "sysf0101fs",-1, NULL, 0);
	platform_driver_register(&f0101_board_platform_driver);	
	return 0;
}

static void f0101_board_sysfs_exit(void)
{	platform_device_unregister(f0101_board_platform_device);
	platform_driver_unregister(&f0101_board_platform_driver);
}
module_init(f0101_board_sysfs_init);
module_exit(f0101_board_sysfs_exit);
/*-------------------------------------------------------------------------------------------------------------*/
MODULE_DESCRIPTION("SYSFS driver for F0101 T40 board");
MODULE_AUTHOR("FURAO");
MODULE_LICENSE("GPL");
