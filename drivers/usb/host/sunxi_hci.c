/*
 * drivers/usb/host/sunxi_hci.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * yangnaitian, 2011-5-24, create this file
 * javen, 2011-7-18, add clock and power switch
 *
 * sunxi HCI Driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/gpio.h>

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/dma-mapping.h>

#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/unaligned.h>
#include <linux/regulator/consumer.h>
#include  <linux/of.h>
#include  <linux/of_address.h>
#include  <linux/of_device.h>

#include  "sunxi_hci.h"

static u64 sunxi_hci_dmamask = DMA_BIT_MASK(32);

#ifndef CONFIG_OF
static char* usbc_name[2] 			= {"usbc0", "usbc1"};
#endif

#ifdef	CONFIG_USB_SUNXI_USB_MANAGER
int usb_otg_id_status(void);
#endif

static struct sunxi_hci_hcd sunxi_ohci0;
static struct sunxi_hci_hcd sunxi_ohci1;
static struct sunxi_hci_hcd sunxi_ehci0;
static struct sunxi_hci_hcd sunxi_ehci1;

#define  USBPHYC_REG_o_PHYCTL		    0x0404

static u32 usb1_set_vbus_cnt = 0;
static u32 usb2_set_vbus_cnt = 0;

static u32 usb1_enable_passly_cnt = 0;
static u32 usb2_enable_passly_cnt = 0;

static s32 request_usb_regulator_io(struct sunxi_hci_hcd *sunxi_hci)
{

	if(sunxi_hci->regulator_io != NULL){
		sunxi_hci->regulator_io_hdle= regulator_get(NULL, sunxi_hci->regulator_io);
		if(IS_ERR(sunxi_hci->regulator_io_hdle)) {
			DMSG_PANIC("ERR: some error happen, %s,regulator_io_hdle fail to get regulator!", sunxi_hci->hci_name);
			return 0;
		}


	}

	return 0;
}

static s32 release_usb_regulator_io(struct sunxi_hci_hcd *sunxi_hci)
{

	if(sunxi_hci->regulator_io != NULL){
		regulator_put(sunxi_hci->regulator_io_hdle);
	}

	return 0;
}

static __u32 USBC_Phy_TpWrite(struct sunxi_hci_hcd *sunxi_hci, __u32 addr, __u32 data, __u32 len)
{
	void __iomem *phyctl_val = NULL;
	__u32 temp = 0, dtmp = 0;
	__u32 j=0;

	if(sunxi_hci->otg_vbase == NULL){
		return 0;
	}

	phyctl_val = sunxi_hci->otg_vbase + USBPHYC_REG_o_PHYCTL;

	dtmp = data;
	for(j = 0; j < len; j++)
	{
		/* set the bit address to be write */
		temp = USBC_Readl(phyctl_val);
		temp &= ~(0xff << 8);
		temp |= ((addr + j) << 8);
		USBC_Writel(temp, phyctl_val);

		temp = USBC_Readb(phyctl_val);
		temp &= ~(0x1 << 7);
		temp |= (dtmp & 0x1) << 7;
		temp &= ~(0x1 << (sunxi_hci->usbc_no << 1));
		USBC_Writeb(temp, phyctl_val);

		temp = USBC_Readb(phyctl_val);
		temp |= (0x1 << (sunxi_hci->usbc_no << 1));
		USBC_Writeb( temp, phyctl_val);

		temp = USBC_Readb(phyctl_val);
		temp &= ~(0x1 << (sunxi_hci->usbc_no <<1 ));
		USBC_Writeb(temp, phyctl_val);
		dtmp >>= 1;
	}

	return data;
}

static __u32 USBC_Phy_Write(struct sunxi_hci_hcd *sunxi_hci, __u32 addr, __u32 data, __u32 len)
{
	return USBC_Phy_TpWrite(sunxi_hci, addr, data, len);
}

static void UsbPhyInit(struct sunxi_hci_hcd *sunxi_hci)
{

	if(sunxi_hci->usbc_no == 0){
		USBC_Phy_Write(sunxi_hci, 0x0c, 0x01, 1);
	}

	/* adjust USB0 PHY's rate and range */
	USBC_Phy_Write(sunxi_hci, 0x20, 0x14, 5);

	/* adjust disconnect threshold value */
	USBC_Phy_Write(sunxi_hci, 0x2a, 3, 2); /*by wangjx*/

	return;
}

static void USBC_SelectPhyToHci(struct sunxi_hci_hcd *sunxi_hci)
{
	int reg_value = 0;
	reg_value = USBC_Readl(sunxi_hci->otg_vbase + 0x420);
	reg_value &= ~(0x01);
	USBC_Writel(reg_value, (sunxi_hci->otg_vbase + 0x420));

	return;
}

static void USBC_Clean_SIDDP(struct sunxi_hci_hcd *sunxi_hci)
{
	int reg_value = 0;
	reg_value = USBC_Readl(sunxi_hci->usb_vbase + 0x810);
	reg_value &= ~(0x01 << 1);
	USBC_Writel(reg_value, (sunxi_hci->usb_vbase + 0x810));

	return;
}

static int open_clock(struct sunxi_hci_hcd *sunxi_hci, u32 ohci)
{
	DMSG_INFO("[%s]: open clock, is_open: %d\n", sunxi_hci->hci_name, sunxi_hci->clk_is_open);

#ifdef  SUNXI_USB_FPGA
	fpga_config_use_hci(sunxi_hci);
#endif

	/*otg and hci0 Controller Shared phy in SUN50I*/
	if(sunxi_hci->usbc_no == HCI0_USBC_NO){
		USBC_SelectPhyToHci(sunxi_hci);
	}

	USBC_Clean_SIDDP(sunxi_hci);

	UsbPhyInit(sunxi_hci);

	if(sunxi_hci->ahb && sunxi_hci->mod_usbphy && !sunxi_hci->clk_is_open){
		sunxi_hci->clk_is_open = 1;
		if(clk_prepare_enable(sunxi_hci->ahb)){
			DMSG_PANIC("ERR:try to prepare_enable %s_ahb failed!\n", sunxi_hci->hci_name);
		}
		udelay(10);

		if(clk_prepare_enable(sunxi_hci->mod_usbphy)){
			DMSG_PANIC("ERR:try to prepare_enable %s_usbphy failed!\n", sunxi_hci->hci_name);
		}
		udelay(10);

	}else{
		DMSG_PANIC("[%s]: wrn: open clock failed, (0x%p, 0x%p, %d, 0x%p)\n",
			sunxi_hci->hci_name,
			sunxi_hci->ahb, sunxi_hci->mod_usbphy, sunxi_hci->clk_is_open,
			sunxi_hci->mod_usb);
	}

	DMSG_INFO("[%s]: open clock end\n", sunxi_hci->hci_name);
	return 0;
}

static int close_clock(struct sunxi_hci_hcd *sunxi_hci, u32 ohci)
{
	DMSG_INFO("[%s]: close clock, is_open: %d\n", sunxi_hci->hci_name, sunxi_hci->clk_is_open);

	if(sunxi_hci->ahb && sunxi_hci->mod_usbphy && sunxi_hci->clk_is_open){
		sunxi_hci->clk_is_open = 0;
		clk_disable_unprepare(sunxi_hci->mod_usbphy);
		clk_disable_unprepare(sunxi_hci->ahb);
		udelay(10);
	}else{
		DMSG_PANIC("[%s]: wrn: open clock failed, (0x%p, 0x%p, %d, 0x%p)\n",
				sunxi_hci->hci_name,sunxi_hci->ahb,
				sunxi_hci->mod_usbphy, sunxi_hci->clk_is_open,
				sunxi_hci->mod_usb);
	}
	return 0;
}

static void usb_passby(struct sunxi_hci_hcd *sunxi_hci, u32 enable)
{
	unsigned long reg_value = 0;
	spinlock_t lock;
	unsigned long flags = 0;

	spin_lock_init(&lock);
	spin_lock_irqsave(&lock, flags);

	/*enable passby*/
	if(sunxi_hci->usbc_no == HCI0_USBC_NO){
		reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE);
		if(enable && usb1_enable_passly_cnt == 0){
			reg_value |= (1 << 10);		/* AHB Master interface INCR8 enable */
			reg_value |= (1 << 9);		/* AHB Master interface burst type INCR4 enable */
			reg_value |= (1 << 8);		/* AHB Master interface INCRX align enable */
#ifdef SUNXI_USB_FPGA
			reg_value |= (0 << 0);		/* enable ULPI, disable UTMI */
#else
			reg_value |= (1 << 0);		/* enable UTMI, disable ULPI */
#endif
		}else if(!enable && usb1_enable_passly_cnt == 1){
			reg_value &= ~(1 << 10);	/* AHB Master interface INCR8 disable */
			reg_value &= ~(1 << 9);		/* AHB Master interface burst type INCR4 disable */
			reg_value &= ~(1 << 8);		/* AHB Master interface INCRX align disable */
			reg_value &= ~(1 << 0);		/* ULPI bypass disable */
		}
		USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE));

		if(enable){
			usb1_enable_passly_cnt++;
		}else{
			usb1_enable_passly_cnt--;
		}
	}else if(sunxi_hci->usbc_no == HCI1_USBC_NO){
		reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE);
		if(enable && usb2_enable_passly_cnt == 0){
			reg_value |= (1 << 10);		/* AHB Master interface INCR8 enable */
			reg_value |= (1 << 9);		/* AHB Master interface burst type INCR4 enable */
			reg_value |= (1 << 8);		/* AHB Master interface INCRX align enable */
			reg_value |= (1 << 0);		/* ULPI bypass enable */
		}else if(!enable && usb2_enable_passly_cnt == 1){
			reg_value &= ~(1 << 10);	/* AHB Master interface INCR8 disable */
			reg_value &= ~(1 << 9);		/* AHB Master interface burst type INCR4 disable */
			reg_value &= ~(1 << 8);		/* AHB Master interface INCRX align disable */
			reg_value &= ~(1 << 0);		/* ULPI bypass disable */
		}
		USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE));

		if(enable){
			usb2_enable_passly_cnt++;
		}else{
			usb2_enable_passly_cnt--;
		}
	}else{
		DMSG_PANIC("EER: unkown usbc_no(%d)\n", sunxi_hci->usbc_no);
		spin_unlock_irqrestore(&lock, flags);
		return;
	}

	spin_unlock_irqrestore(&lock, flags);

	return;
}

static int alloc_pin(struct sunxi_hci_hcd *sunxi_hci)
{
	u32 ret = 1;

	if(sunxi_hci->drv_vbus_gpio_valid){
		ret = gpio_request(sunxi_hci->drv_vbus_gpio_set.gpio.gpio, NULL);
		if(ret != 0){
			DMSG_PANIC("request %s gpio:%d\n", sunxi_hci->hci_name, sunxi_hci->drv_vbus_gpio_set.gpio.gpio);
		}else{
			gpio_direction_output(sunxi_hci->drv_vbus_gpio_set.gpio.gpio, 0);
		}
	}

	return 0;
}

static void free_pin(struct sunxi_hci_hcd *sunxi_hci)
{

	if(sunxi_hci->drv_vbus_gpio_valid){
		gpio_free(sunxi_hci->drv_vbus_gpio_set.gpio.gpio);
		sunxi_hci->drv_vbus_gpio_valid = 0;
	}

	return;
}

static void __sunxi_set_vbus(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{

	DMSG_INFO("[%s]: Set USB Power %s\n", sunxi_hci->hci_name, (is_on ? "ON" : "OFF"));

	/* set power flag */
	sunxi_hci->power_flag = is_on;

	if((sunxi_hci->regulator_io != NULL) && (sunxi_hci->regulator_io_hdle != NULL)){
		if(is_on){
			if(regulator_enable(sunxi_hci->regulator_io_hdle) < 0){
				DMSG_INFO("%s: regulator_enable fail\n", sunxi_hci->hci_name);
			}
		}else{
			if(regulator_disable(sunxi_hci->regulator_io_hdle) < 0){
				DMSG_INFO("%s: regulator_disable fail\n", sunxi_hci->hci_name);
			}
		}
	}

//no care of usb0 vbus when otg connect pc setup system without battery  and to return
#ifdef	CONFIG_USB_SUNXI_USB_MANAGER
	if(sunxi_hci->usbc_no == HCI0_USBC_NO){
		if(usb_otg_id_status() == 1){
			return;
		}
	}
#endif

	if(sunxi_hci->drv_vbus_gpio_valid){
		__gpio_set_value(sunxi_hci->drv_vbus_gpio_set.gpio.gpio, is_on);
	}

	return;
}

static void sunxi_set_vbus(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{

	DMSG_DEBUG("[%s]: sunxi_set_vbus cnt %d\n",
		sunxi_hci->hci_name,
		(sunxi_hci->usbc_no == 1) ? usb1_set_vbus_cnt : usb2_set_vbus_cnt);

	if(sunxi_hci->usbc_no == HCI0_USBC_NO){
		if(is_on && usb1_set_vbus_cnt == 0){
			__sunxi_set_vbus(sunxi_hci, is_on);  /* power on */
		}else if(!is_on && usb1_set_vbus_cnt == 1){
			__sunxi_set_vbus(sunxi_hci, is_on);  /* power off */
		}

		if(is_on){
			usb1_set_vbus_cnt++;
		}else{
			usb1_set_vbus_cnt--;
		}
	}else if(sunxi_hci->usbc_no == HCI1_USBC_NO){
		if(is_on && usb2_set_vbus_cnt == 0){
			__sunxi_set_vbus(sunxi_hci, is_on);  /* power on */
		}else if(!is_on && usb2_set_vbus_cnt == 1){
			__sunxi_set_vbus(sunxi_hci, is_on);  /* power off */
		}

		if(is_on){
			usb2_set_vbus_cnt++;
		}else{
			usb2_set_vbus_cnt--;
		}
	}else{
		DMSG_INFO("[%s]: sunxi_set_vbus no: %d\n", sunxi_hci->hci_name, sunxi_hci->usbc_no);
	}

	return;
}

static int sunxi_get_hci_base(struct platform_device *pdev, struct sunxi_hci_hcd *sunxi_hci)
{
	struct device_node *np = pdev->dev.of_node;
	struct resource res;
	int ret = 0;

	sunxi_hci->usb_vbase  = of_iomap(np, 0);
	if (sunxi_hci->usb_vbase == NULL) {
		dev_err(&pdev->dev, "%s, can't get vbase resource\n", sunxi_hci->hci_name);
		return -EINVAL;
	}

	if(sunxi_hci->usbc_no == HCI0_USBC_NO){
		sunxi_hci->otg_vbase  = of_iomap(np, 2);
		if (sunxi_hci->otg_vbase == NULL) {
			dev_err(&pdev->dev, "%s, can't get otg_vbase resource\n", sunxi_hci->hci_name);
			return -EINVAL;
		}

		DMSG_INFO("OTG,Vbase:0x%p\n", sunxi_hci->otg_vbase);
	}

	ret = of_address_to_resource(np, 0, &res);
	if (ret) {
		dev_err(&pdev->dev, "could not get regs\n");
	}

	sunxi_hci->usb_base_res = &res;

	DMSG_INFO("%s,Vbase:0x%p, base res:0x%p\n", sunxi_hci->hci_name, sunxi_hci->usb_vbase, sunxi_hci->usb_base_res);

	return 0;
}

static int sunxi_get_hci_clock(struct platform_device *pdev, struct sunxi_hci_hcd *sunxi_hci)
{
	struct device_node *np = pdev->dev.of_node;

	sunxi_hci->ahb = of_clk_get(np, 1);
	if (IS_ERR(sunxi_hci->ahb)) {
		sunxi_hci->ahb = NULL;
		DMSG_PANIC("ERR: %s get usb ahb_otg clk failed.\n", sunxi_hci->hci_name);
		return -EINVAL;
	}

	sunxi_hci->mod_usbphy = of_clk_get(np, 0);
	if (IS_ERR(sunxi_hci->mod_usbphy)) {
		sunxi_hci->mod_usbphy = NULL;
		DMSG_PANIC("ERR: %s get usb mod_usbphy failed.\n", sunxi_hci->hci_name);
		return -EINVAL;
	}
	return 0;
}

static int get_usb_cfg(struct platform_device *pdev, struct sunxi_hci_hcd *sunxi_hci)
{
#ifdef CONFIG_OF
	struct device_node *usbc_np = NULL;
	char np_name[10];
	int ret = -1;

	sprintf(np_name, "usbc%d", sunxi_get_hci_num(pdev));

	usbc_np = of_find_node_by_type(NULL, np_name);

	/* usbc enable */
	ret = of_property_read_string(usbc_np, "status", &sunxi_hci->used_status);
	if (ret) {
		 DMSG_PRINT("get %s used is fail, %d\n", sunxi_hci->hci_name, -ret);
		 sunxi_hci->used = 0;
	}else if (!strcmp(sunxi_hci->used_status, "okay")) {

		 sunxi_hci->used = 1;
	}else {
		 sunxi_hci->used = 0;
	}

	/* usbc init_state */
	ret = of_property_read_u32(usbc_np, KEY_USB_HOST_INIT_STATE, &sunxi_hci->host_init_state);
	if (ret) {
		 DMSG_PRINT("get %s init_state is fail, %d\n", sunxi_hci->hci_name, -ret);
	}

	/* usbc wakeup_suspend */
	ret = of_property_read_u32(usbc_np, KEY_USB_WAKEUP_SUSPEND, &sunxi_hci->wakeup_suspend);
	if (ret) {
		 DMSG_PRINT("get %s wakeup_suspend is fail, %d\n", sunxi_hci->hci_name, -ret);
	}

	sunxi_hci->drv_vbus_gpio_set.gpio.gpio = of_get_named_gpio_flags(usbc_np, KEY_USB_DRVVBUS_GPIO, 0, &sunxi_hci->gpio_flags);
	if(gpio_is_valid(sunxi_hci->drv_vbus_gpio_set.gpio.gpio)){
		sunxi_hci->drv_vbus_gpio_valid = 1;
	}else{
		sunxi_hci->drv_vbus_gpio_valid = 0;
		DMSG_PRINT("get %s drv_vbus_gpio is fail\n", sunxi_hci->hci_name);
	}

	/* usbc regulator_io */
	ret = of_property_read_string(usbc_np, KEY_USB_REGULATOR_IO, &sunxi_hci->regulator_io);
	if (ret){
		printk("get %s, regulator_io is fail, %d\n", sunxi_hci->hci_name, -ret);
		sunxi_hci->regulator_io = NULL;
	}else{
		if (!strcmp(sunxi_hci->regulator_io, "nocare")) {
			 printk("get %s, regulator_io is no nocare\n", sunxi_hci->hci_name);
			sunxi_hci->regulator_io = NULL;
		}else{
			ret = of_property_read_u32(usbc_np, KEY_USB_REGULATOR_IO_VOL, &sunxi_hci->regulator_value);
			if (ret) {
				printk("get %s, regulator_value is fail, %d\n", sunxi_hci->hci_name, -ret);
				sunxi_hci->regulator_io = NULL;
			}
		}
	}

#else
	script_item_value_type_e type = 0;
	script_item_u item_temp;

	/* usbc enable */
	type = script_get_item(usbc_name[sunxi_hci->usbc_no], KEY_USB_ENABLE, &item_temp);
	if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
		sunxi_hci->used = item_temp.val;
	}else{
		DMSG_INFO("get %s usbc enable failed\n" ,sunxi_hci->hci_name);
		sunxi_hci->used = 0;
	}

	/* host_init_state */
	type = script_get_item(usbc_name[sunxi_hci->usbc_no], KEY_USB_HOST_INIT_STATE, &item_temp);
	if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
		sunxi_hci->host_init_state = item_temp.val;
	}else{
		DMSG_INFO("script_parser_fetch host_init_state failed\n");
		sunxi_hci->host_init_state = 1;
	}

	type = script_get_item(usbc_name[sunxi_hci->usbc_no], KEY_USB_WAKEUP_SUSPEND, &item_temp);
	if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
		sunxi_hci->not_suspend = item_temp.val;
	}else{
		DMSG_INFO("get usb_restrict_flag failed\n");
		sunxi_hci->not_suspend = 0;
	}


	/* usbc drv_vbus */
	type = script_get_item(usbc_name[sunxi_hci->usbc_no], KEY_USB_DRVVBUS_GPIO, &sunxi_hci->drv_vbus_gpio_set);
	if(type == SCIRPT_ITEM_VALUE_TYPE_PIO){
		sunxi_hci->drv_vbus_gpio_valid = 1;
	}else{
		DMSG_INFO("%s(drv vbus) is invalid\n", sunxi_hci->hci_name);
		sunxi_hci->drv_vbus_gpio_valid = 0;
	}


	/* get regulator io information */
	type = script_get_item(usbc_name[sunxi_hci->usbc_no], KEY_USB_REGULATOR_IO, &item_temp);
	if (type == SCIRPT_ITEM_VALUE_TYPE_STR) {
		if (!strcmp(item_temp.str, "nocare")) {
			DMSG_INFO("get usb_regulator is nocare\n");
			sunxi_hci->regulator_io = NULL;
		}else{
			sunxi_hci->regulator_io = item_temp.str;

			type = script_get_item(usbc_name[sunxi_hci->usbc_no], KEY_USB_REGULATOR_IO_VOL, &item_temp);
			if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
				sunxi_hci->regulator_value = item_temp.val;
			}else{
				DMSG_INFO("get usb_voltage is failed\n");
				sunxi_hci->regulator_value = 0;
				sunxi_hci->regulator_io = NULL;
			}
		}
	}else {
		DMSG_INFO("get usb_regulator is failed\n");
		sunxi_hci->regulator_io = NULL;
	}

#endif

	return 0;
}

int sunxi_get_hci_num(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	int ret = 0;
	int hci_num = 0;

	ret = of_property_read_u32(np, HCI_USBC_NO, &hci_num);
	if (ret) {
		 DMSG_PANIC("get hci_ctrl_num is fail, %d\n", -ret);
	}

	return hci_num;
}

static int sunxi_get_hci_name(struct platform_device *pdev, struct sunxi_hci_hcd *sunxi_hci)
{
	struct device_node *np = pdev->dev.of_node;

	sprintf(sunxi_hci->hci_name, "%s", np->name);

	return 0;
}

static int sunxi_get_hci_irq_no(struct platform_device *pdev, struct sunxi_hci_hcd *sunxi_hci)
{
	sunxi_hci->irq_no = platform_get_irq(pdev, 0);

	DMSG_INFO("%s,irq_no:%d\n", sunxi_hci->hci_name, sunxi_hci->irq_no);
	return 0;
}

#ifdef  SUNXI_USB_FPGA
static int sunxi_get_sram_base(struct platform_device *pdev, struct sunxi_hci_hcd *sunxi_hci)
{

	struct device_node *np = pdev->dev.of_node;

	if(sunxi_hci->usbc_no == HCI0_USBC_NO){
		sunxi_hci->sram_vbase = of_iomap(np, 1);
		if (sunxi_hci->sram_vbase == NULL) {
			dev_err(&pdev->dev, "%s, can't get sram resource\n", sunxi_hci->hci_name);
			return -EINVAL;
		}

		DMSG_INFO("%s sram_vbase: %p\n", sunxi_hci->hci_name, sunxi_hci->sram_vbase);
	}

	return 0;
}
#endif

static int sunxi_get_hci_resource(struct platform_device *pdev, struct sunxi_hci_hcd *sunxi_hci, int usbc_no)
{

	if(sunxi_hci == NULL){
		dev_err(&pdev->dev, "sunxi_hci is NULL\n");
		return -1;
	}

	memset(sunxi_hci, 0, sizeof(struct sunxi_hci_hcd));

	sunxi_hci->usbc_no = usbc_no;
	sunxi_get_hci_name(pdev, sunxi_hci);
	get_usb_cfg(pdev, sunxi_hci);

	if(sunxi_hci->used == 0){
		DMSG_INFO("sunxi %s is no enable\n", sunxi_hci->hci_name);
		return -1;
	}

	sunxi_get_hci_base(pdev, sunxi_hci);
	sunxi_get_hci_clock(pdev, sunxi_hci);
	sunxi_get_hci_irq_no(pdev, sunxi_hci);

	request_usb_regulator_io(sunxi_hci);
	sunxi_hci->open_clock	= open_clock;
	sunxi_hci->close_clock	= close_clock;
	sunxi_hci->set_power	= sunxi_set_vbus;
	sunxi_hci->usb_passby	= usb_passby;

#ifdef  SUNXI_USB_FPGA
	sunxi_get_sram_base(pdev, sunxi_hci);
	fpga_config_use_hci(sunxi_hci);
#endif

	alloc_pin(sunxi_hci);

	pdev->dev.platform_data = sunxi_hci;
	return 0;
}

int exit_sunxi_hci(struct sunxi_hci_hcd *sunxi_hci)
{
	release_usb_regulator_io(sunxi_hci);
	free_pin(sunxi_hci);
	return 0;
}

int init_sunxi_hci(struct platform_device *pdev, int usbc_type)
{
	struct sunxi_hci_hcd *sunxi_hci = NULL;
	int usbc_no = 0;
	int hci_num = -1;
	int ret = -1;

#ifdef CONFIG_OF
	pdev->dev.dma_mask = &sunxi_hci_dmamask;
	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
#endif

	hci_num = sunxi_get_hci_num(pdev);

	switch(hci_num){
		case HCI0_USBC_NO:
			usbc_no = HCI0_USBC_NO;
			if(usbc_type == SUNXI_USB_EHCI){
				sunxi_hci =  &sunxi_ehci0;
			}else if(usbc_type == SUNXI_USB_OHCI){
				sunxi_hci =  &sunxi_ohci0;
			}else{
				dev_err(&pdev->dev, "get hci num fail: %d\n", hci_num);
				return -1;
			}
		break;

		case HCI1_USBC_NO:
			usbc_no = HCI1_USBC_NO;
			if(usbc_type == SUNXI_USB_EHCI){
				sunxi_hci =  &sunxi_ehci1;
			}else if(usbc_type == SUNXI_USB_OHCI){
				sunxi_hci =  &sunxi_ohci1;
			}else{
				dev_err(&pdev->dev, "get hci num fail: %d\n", hci_num);
				return -1;
			}
		break;

		default:
			dev_err(&pdev->dev, "get hci num fail: %d\n", hci_num);
		return -1;

	}

	ret = sunxi_get_hci_resource(pdev, sunxi_hci, usbc_no);

	return ret;
}

