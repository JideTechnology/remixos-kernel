/*
 * linux/arch/arm/mach-sunxi/sunxi_bootup_extend.c
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * allwinner sunxi platform bootup extend code.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/slab.h>
#include <linux/hwspinlock.h>
#include <linux/reboot.h>
#include <linux/of_address.h>


static int bootup_extend_enabel = 0;
static void __iomem *rtc_base_addr = NULL;

#define GENERAL_RTC_REG_MAX   (0x03)
#define GENERAL_RTC_REG_MIN   (0x01)

#define INTC_HWSPINLOCK_TIMEOUT      (4000)

static struct hwspinlock *intc_mgr_hwlock;
static int __rtc_reg_write(u32 addr, u32 data)
{
    unsigned long hwflags;

    if ((addr < GENERAL_RTC_REG_MIN) || (addr > GENERAL_RTC_REG_MAX)) {
        printk(KERN_ERR "%s: rtc address error, address:0x%x\n", __func__, addr);
        return -1;
    }

    if (data > 0xff) {
        printk(KERN_ERR "%s: rtc data error, data:0x%x\n", __func__, data);
        return -1;
    }

    if (!hwspin_lock_timeout_irqsave(intc_mgr_hwlock, INTC_HWSPINLOCK_TIMEOUT, &hwflags)) {

	    if(rtc_base_addr != NULL)
	    {
	    	writel(data, (rtc_base_addr + 0x100 + 0x4 * addr));
	    }
	    else
	    {
	    	printk(KERN_ERR "%s: rtc rtc_base_addr error, data:0x%x\n", __func__, data);
	    }

        hwspin_unlock_irqrestore(intc_mgr_hwlock, &hwflags);
        printk(KERN_DEBUG "%s: write rtc reg success, rtc reg 0x%x:0x%x\n", __func__, addr, data);
        return 0;
    }

    printk(KERN_DEBUG "%s: get hwspinlock unsuccess\n", __func__);
    return -1;
}

void sunxi_bootup_extend_fix(unsigned int *cmd)
{
    if (bootup_extend_enabel == 1) {
        if (*cmd == LINUX_REBOOT_CMD_POWER_OFF) {
            __rtc_reg_write(2, 0x2);
            *cmd = LINUX_REBOOT_CMD_RESTART;
            printk(KERN_INFO "will enter boot_start_os\n");
        } else if (*cmd == LINUX_REBOOT_CMD_RESTART ||
                   *cmd == LINUX_REBOOT_CMD_RESTART2 ) {
			printk(KERN_INFO "not enter boot_start_os\n");
            __rtc_reg_write(2, 0xf);
        }
    }
}
EXPORT_SYMBOL(sunxi_bootup_extend_fix);

static int __init sunxi_bootup_extend_init(void)
{
	struct device_node *dev_node;
	struct device_node *rtc_node;
	const char *used_string = NULL;
	

	dev_node = of_find_compatible_node(NULL, NULL, "allwinner,box_start_os");
	if(!dev_node)
	{
		printk(KERN_INFO "%s: bootup extend not support \n", __func__);
		return 0;
	}

	if (of_property_read_string(dev_node, "status", &used_string)) {
		
		printk(KERN_INFO "%s: bootup extend not get status \n", __func__);
		return 0;
	}

	if(strcmp("okay", used_string))
	{
		printk(KERN_INFO "%s: bootup extend not used \n", __func__);
		return 0;
	}

	intc_mgr_hwlock = hwspin_lock_request_specific(SUNXI_INTC_HWSPINLOCK);
	if (!intc_mgr_hwlock)
	{
		printk(KERN_ERR "%s,%d request hwspinlock faild!\n", __func__, __LINE__);
		return 0;
	}

    rtc_node = of_find_compatible_node(NULL, NULL, "allwinner,sun50i-rtc");
    if (!rtc_node) {
		printk(KERN_ERR "%s,%d request rtc node faild!\n", __func__, __LINE__);
		return 0;
	}

	rtc_base_addr = of_iomap(rtc_node, 0);
	if (rtc_base_addr == NULL)
	{
		printk(KERN_ERR "%s,%d request rtc add faild!\n", __func__, __LINE__);
		return 0;
	}

	bootup_extend_enabel = 1;
    printk(KERN_INFO "%s: bootup extend state %d\n", __func__, bootup_extend_enabel);
    return 0;
}
late_initcall(sunxi_bootup_extend_init);
