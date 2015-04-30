/*
 * arch/arm/mach-tegra/board-macallan-sdhci.c
 *
 * Copyright (c) 2013-2014, NVIDIA CORPORATION. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/resource.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>
#include <linux/mmc/host.h>
#include <linux/etherdevice.h>
#include <linux/fs.h>


#include <asm/mach-types.h>
#include <mach/irqs.h>
#include <mach/iomap.h>
#include <mach/sdhci.h>
#include <mach/gpio-tegra.h>
#include <mach/io_dpd.h>
#include <linux/wlan_plat.h>
#include <asm/uaccess.h>

#include "gpio-names.h"
#include "board.h"
#include "board-macallan.h"
#include "dvfs.h"
#include "fuse.h"

#define FUSE_CORE_SPEEDO_0	0x134
#define MACALLAN_SD_CD	TEGRA_GPIO_PV2
//#define MACALLAN_SD_WP	TEGRA_GPIO_PQ4
#define MACALLAN_WLAN_PWR	TEGRA_GPIO_PCC5
#define MACALLAN_WLAN_WOW	TEGRA_GPIO_PU5
#if defined(CONFIG_BCMDHD_EDP_SUPPORT)
/* Wifi power levels */
#define ON  1080 /* 1080 mW */
#define OFF 0
static unsigned int wifi_states[] = {ON, OFF};
#endif

static void (*wifi_status_cb)(int card_present, void *dev_id);
static void *wifi_status_cb_devid;
static int macallan_wifi_status_register(void (*callback)(int , void *), void *);

static int macallan_wifi_power(int on);
static int macallan_wifi_set_carddetect(int val);
static int macallan_wifi_reset(int on);
//static int macallan_wifi_get_mac_addr(unsigned char *buf);

extern int tegra_get_wifi_mac(unsigned char *buf);

static struct wifi_platform_data macallan_wifi_control = {
	.set_power	= macallan_wifi_power,
	.set_reset	= macallan_wifi_reset,
	.set_carddetect	= macallan_wifi_set_carddetect,
//	.get_mac_addr   = macallan_wifi_get_mac_addr,
#if defined(CONFIG_BCMDHD_EDP_SUPPORT)
	/* set the wifi edp client information here */
	.client_info    = {
		.name       = "wifi_edp_client",
		.states     = wifi_states,
		.num_states = ARRAY_SIZE(wifi_states),
		.e0_index   = 0,
		.priority   = EDP_MAX_PRIO,
	},
#endif
};

static struct resource wifi_resource[] = {
	[0] = {
		.name	= "bcm4329_wlan_irq",
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL
				| IORESOURCE_IRQ_SHAREABLE,
	},
};

static struct platform_device macallan_wifi_device = {
	.name		= "bcm4329_wlan",
	.id		= 1,
	.num_resources	= 1,
	.resource	= wifi_resource,
	.dev		= {
		.platform_data = &macallan_wifi_control,
	},
};

static struct resource sdhci_resource0[] = {
	[0] = {
		.start  = INT_SDMMC1,
		.end    = INT_SDMMC1,
		.flags  = IORESOURCE_IRQ,
	},
	[1] = {
		.start	= TEGRA_SDMMC1_BASE,
		.end	= TEGRA_SDMMC1_BASE + TEGRA_SDMMC1_SIZE-1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct resource sdhci_resource2[] = {
	[0] = {
		.start  = INT_SDMMC3,
		.end    = INT_SDMMC3,
		.flags  = IORESOURCE_IRQ,
	},
	[1] = {
		.start	= TEGRA_SDMMC3_BASE,
		.end	= TEGRA_SDMMC3_BASE + TEGRA_SDMMC3_SIZE-1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct resource sdhci_resource3[] = {
	[0] = {
		.start  = INT_SDMMC4,
		.end    = INT_SDMMC4,
		.flags  = IORESOURCE_IRQ,
	},
	[1] = {
		.start	= TEGRA_SDMMC4_BASE,
		.end	= TEGRA_SDMMC4_BASE + TEGRA_SDMMC4_SIZE-1,
		.flags	= IORESOURCE_MEM,
	},
};

#ifdef CONFIG_MMC_EMBEDDED_SDIO
static struct embedded_sdio_data embedded_sdio_data0 = {
	.cccr   = {
		.sdio_vsn       = 2,
		.multi_block    = 1,
		.low_speed      = 0,
		.wide_bus       = 0,
		.high_power     = 1,
		.high_speed     = 1,
	},
	.cis  = {
		.vendor	 = 0x02d0,
		.device	 = 0xa94d,
	},
};
#endif

static struct tegra_sdhci_platform_data tegra_sdhci_platform_data0 = {
	.mmc_data = {
		.register_status_notify	= macallan_wifi_status_register,
#ifdef CONFIG_MMC_EMBEDDED_SDIO
		.embedded_sdio = &embedded_sdio_data0,
#endif
		.built_in = 0,
		.ocr_mask = MMC_OCR_1V8_MASK,
	},
#ifndef CONFIG_MMC_EMBEDDED_SDIO
	.pm_flags = MMC_PM_KEEP_POWER,
#endif
	.cd_gpio = -1,
	.wp_gpio = -1,
	.power_gpio = -1,
	.tap_delay = 0x2,
	.trim_delay = 0x2,
	.ddr_clk_limit = 41000000,
	.max_clk_limit = 82000000,
	.uhs_mask = MMC_UHS_MASK_DDR50 | MMC_UHS_MASK_SDR104,
	.edp_support = false,
};

static struct tegra_sdhci_platform_data tegra_sdhci_platform_data2 = {
	.cd_gpio = MACALLAN_SD_CD,
	.wp_gpio = -1,
	.power_gpio = -1,
	.tap_delay = 0x3,
	.trim_delay = 0x3,
	.ddr_clk_limit = 41000000,
	.max_clk_limit = 156000000,
	.uhs_mask = MMC_UHS_MASK_DDR50,
	.edp_support = true,
	.edp_states = {966, 0},
};

static struct tegra_sdhci_platform_data tegra_sdhci_platform_data3 = {
	.cd_gpio = -1,
	.wp_gpio = -1,
	.power_gpio = -1,
	.is_8bit = 1,
	.tap_delay = 0x5,
	.trim_delay = 0x3,
	.ddr_clk_limit = 41000000,
	.max_clk_limit = 156000000,
	.mmc_data = {
		.built_in = 1,
		.ocr_mask = MMC_OCR_1V8_MASK,
	},
	.edp_support = true,
	.edp_states = {966, 0},
};

static struct platform_device tegra_sdhci_device0 = {
	.name		= "sdhci-tegra",
	.id		= 0,
	.resource	= sdhci_resource0,
	.num_resources	= ARRAY_SIZE(sdhci_resource0),
	.dev = {
		.platform_data = &tegra_sdhci_platform_data0,
	},
};

static struct platform_device tegra_sdhci_device2 = {
	.name		= "sdhci-tegra",
	.id		= 2,
	.resource	= sdhci_resource2,
	.num_resources	= ARRAY_SIZE(sdhci_resource2),
	.dev = {
		.platform_data = &tegra_sdhci_platform_data2,
	},
};

static struct platform_device tegra_sdhci_device3 = {
	.name		= "sdhci-tegra",
	.id		= 3,
	.resource	= sdhci_resource3,
	.num_resources	= ARRAY_SIZE(sdhci_resource3),
	.dev = {
		.platform_data = &tegra_sdhci_platform_data3,
	},
};

static int macallan_wifi_status_register(
		void (*callback)(int card_present, void *dev_id),
		void *dev_id)
{
	if (wifi_status_cb)
		return -EAGAIN;
	wifi_status_cb = callback;
	wifi_status_cb_devid = dev_id;
	return 0;
}

static int macallan_wifi_set_carddetect(int val)
{
	pr_debug("%s: %d\n", __func__, val);
	if (wifi_status_cb)
		wifi_status_cb(val, wifi_status_cb_devid);
	else
		pr_warning("%s: Nobody to notify\n", __func__);
	return 0;
}

static int macallan_wifi_power(int on)
{
	pr_debug("%s: %d\n", __func__, on);

	gpio_set_value(MACALLAN_WLAN_PWR, on);
	mdelay(100);

	return 0;
}

static int macallan_wifi_reset(int on)
{
	pr_debug("%s: do nothing\n", __func__);
	return 0;
}

static int __init macallan_wifi_init(void)
{
	int rc;

	rc = gpio_request(MACALLAN_WLAN_PWR, "wlan_power");
	if (rc)
		pr_err("WLAN_PWR gpio request failed:%d\n", rc);
	
	rc = gpio_request(MACALLAN_WLAN_WOW, "wlan_wow");
	if (rc)
		pr_err("WLAN_WOW gpio request failed:%d\n", rc);

	rc = gpio_direction_output(MACALLAN_WLAN_PWR, 0);
	if (rc)
		pr_err("WLAN_PWR gpio direction configuration failed:%d\n", rc);
	

	rc = gpio_direction_input(MACALLAN_WLAN_WOW);
	if (rc < 0) {
		gpio_free(MACALLAN_WLAN_WOW);
		gpio_free(MACALLAN_WLAN_PWR);
		return rc;
	}
	
	wifi_resource[0].start = wifi_resource[0].end =
		gpio_to_irq(MACALLAN_WLAN_WOW);

		platform_device_register(&macallan_wifi_device);

	return 0;
}


/*
static int nvram_read(char *filename, char *buf, ssize_t len, int offset)
{	
    struct file *fd;
    int retLen = -1;
    mm_segment_t old_fs;

    printk("zzss test nvram_read filename = %s \n",filename);
    fd = filp_open(filename, O_RDONLY, 0);
    
    if(IS_ERR(fd)) 
    {
        printk("[BCMDHD][nvram_read] : failed to open!!!\n");
        return -1;
    }
    old_fs = get_fs();
    set_fs(KERNEL_DS);
    do{
        if ((fd->f_op == NULL) || (fd->f_op->read == NULL))
    	{
           printk("[BCMDHD][nvram_read] : file can not be read!!\n");
           break;
    	} 
    		
        if (fd->f_pos != offset) 
	{
            if (fd->f_op->llseek) 
		{
        		if(fd->f_op->llseek(fd, offset, 0) != offset) 
			{
				printk("[BCMDHD][nvram_read] : failed to seek!!\n");
				break;
        		}
        	}
	    else 
		{
        		fd->f_pos = offset;
        	}
        }    		
        
    		retLen = fd->f_op->read(fd,	buf,  len, &fd->f_pos);			
    		
    }while(false);
    
    filp_close(fd, NULL);
    
    set_fs(old_fs);
    
    return retLen;
}


#define	   WIFI_NVRAM_FILE_NAME		"/sys/kernel/tegra_nct/wifi_mac_addr"



static int macallan_wifi_get_mac_addr(unsigned char *buf)
{
   	int len , i = 0 , num = 0;
   	unsigned char macbuf[18] = {0};	
	unsigned int  machexbuf[7] = {0};

	len = nvram_read(WIFI_NVRAM_FILE_NAME, macbuf , 17 , 0);
	if( len == -1)
		return len;	
	sscanf(macbuf, "%x:%x:%x:%x:%x:%x", &machexbuf[0], &machexbuf[1], &machexbuf[2],&machexbuf[3], &machexbuf[4], &machexbuf[5]);
	for(i = 0 ; i < 6 ; i++ )
	{	
		buf[i] = (unsigned char)machexbuf[i];
		if(buf[i] == 0)
			num++;
	}
	if(num == 6)
	{
		printk("NCT wifi MAC null ,use random addr \n");
		random_ether_addr(buf);
	}	

	printk("NCT wifi MAC :buf[0] = %x,buf[1] = %x,buf[2] = %x,buf[3] = %x,buf[4] = %x,buf[5] = %x",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
	return 0;
}

*/

/*
static int macallan_wifi_get_mac_addr(unsigned char *buf)
{
	
	int len;

	
        len = tegra_get_wifi_mac(buf);
	if(len < 0)
	{
		printk("NCT wifi MAC null ,use random addr \n");
		random_ether_addr(buf);
	}


//	random_ether_addr(buf);
	printk("FACTORY wifi MAC :buf[0] = %x,buf[1] = %x,buf[2] = %x,buf[3] = %x,buf[4] = %x,buf[5] = %x",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);

	return 0;
} 
*/

#ifdef CONFIG_TEGRA_PREPOWER_WIFI
static int __init macallan_wifi_prepower(void)
{
	if (!machine_is_macallan())
		return 0;

	macallan_wifi_power(1);

	return 0;
}

subsys_initcall_sync(macallan_wifi_prepower);
#endif

int __init macallan_sdhci_init(void)
{
	int nominal_core_mv;
	int min_vcore_override_mv;
	int boot_vcore_mv;
	int speedo;

	nominal_core_mv =
		tegra_dvfs_rail_get_nominal_millivolts(tegra_core_rail);
	if (nominal_core_mv > 0) {
		tegra_sdhci_platform_data0.nominal_vcore_mv = nominal_core_mv;
		tegra_sdhci_platform_data2.nominal_vcore_mv = nominal_core_mv;
		tegra_sdhci_platform_data3.nominal_vcore_mv = nominal_core_mv;
	}
	min_vcore_override_mv =
		tegra_dvfs_rail_get_override_floor(tegra_core_rail);
	if (min_vcore_override_mv) {
		tegra_sdhci_platform_data0.min_vcore_override_mv =
			min_vcore_override_mv;
		tegra_sdhci_platform_data2.min_vcore_override_mv =
			min_vcore_override_mv;
		tegra_sdhci_platform_data3.min_vcore_override_mv =
			min_vcore_override_mv;
	}
	boot_vcore_mv = tegra_dvfs_rail_get_boot_level(tegra_core_rail);
	if (boot_vcore_mv) {
		tegra_sdhci_platform_data0.boot_vcore_mv = boot_vcore_mv;
		tegra_sdhci_platform_data2.boot_vcore_mv = boot_vcore_mv;
		tegra_sdhci_platform_data3.boot_vcore_mv = boot_vcore_mv;
	}

	speedo = tegra_fuse_readl(FUSE_CORE_SPEEDO_0);
	tegra_sdhci_platform_data0.cpu_speedo = speedo;
	tegra_sdhci_platform_data2.cpu_speedo = speedo;
	tegra_sdhci_platform_data3.cpu_speedo = speedo;

	if ((tegra_sdhci_platform_data3.uhs_mask & MMC_MASK_HS200)
		&& (!(tegra_sdhci_platform_data3.uhs_mask &
		MMC_UHS_MASK_DDR50)))
		tegra_sdhci_platform_data3.trim_delay = 0;

	platform_device_register(&tegra_sdhci_device3);
	platform_device_register(&tegra_sdhci_device2);
	platform_device_register(&tegra_sdhci_device0);
	macallan_wifi_init();
	return 0;
}
