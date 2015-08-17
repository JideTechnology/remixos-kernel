/*
*************************************************************************************
*                         			      Linux
*					           AHCI SATA platform driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: ahci_sunxi.c
*
* Author 		: danielwang
*
* Description 	: SATA Host Controller Driver for A20 Platform
*
* Notes         :
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*    danielwang        2013-3-28            1.0          create this file
*
*************************************************************************************
*/

#include <linux/kernel.h>
#include <linux/gfp.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/libata.h>
#include <linux/ahci_platform.h>
#include "ahci.h"

#include <linux/clk.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

#define DRV_NAME "ahci-sunxi"

#define AHCI_BISTAFR	0x00a0
#define AHCI_BISTCR	0x00a4
#define AHCI_BISTFCTR	0x00a8
#define AHCI_BISTSR	0x00ac
#define AHCI_BISTDECR	0x00b0
#define AHCI_DIAGNR0	0x00b4
#define AHCI_DIAGNR1	0x00b8
#define AHCI_OOBR	0x00bc
#define AHCI_PHYCS0R	0x00c0
#define AHCI_PHYCS1R	0x00c4
#define AHCI_PHYCS2R	0x00c8
#define AHCI_TIMER1MS	0x00e0
#define AHCI_GPARAM1R	0x00e8
#define AHCI_GPARAM2R	0x00ec
#define AHCI_PPARAMR	0x00f0
#define AHCI_TESTR	0x00f4
#define AHCI_VERSIONR	0x00f8
#define AHCI_IDR	0x00fc
#define AHCI_RWCR	0x00fc
#define AHCI_P0DMACR	0x0170
#define AHCI_P0PHYCR	0x0178
#define AHCI_P0PHYSR	0x017c

static struct scsi_host_template ahci_platform_sht = {
	AHCI_SHT(DRV_NAME),
};

static char* ahci_sunxi_gpio_name = "sata_power_en";

static void ahci_sunxi_dump_reg(struct device *dev);

static void sunxi_clrbits(void __iomem *reg, u32 clr_val)
{
	u32 reg_val;

	reg_val = readl(reg);
	reg_val &= ~(clr_val);
	writel(reg_val, reg);
}

static void sunxi_setbits(void __iomem *reg, u32 set_val)
{
	u32 reg_val;

	reg_val = readl(reg);
	reg_val |= set_val;
	writel(reg_val, reg);
}

static void sunxi_clrsetbits(void __iomem *reg, u32 clr_val, u32 set_val)
{
	u32 reg_val;

	reg_val = readl(reg);
	reg_val &= ~(clr_val);
	reg_val |= set_val;
	writel(reg_val, reg);
}

static u32 sunxi_getbits(void __iomem *reg, u8 mask, u8 shift)
{
	return (readl(reg) >> shift) & mask;
}

static int ahci_sunxi_phy_init(struct device *dev, void __iomem *reg_base)
{
	u32 reg_val;
	int timeout;

	/* This magic is from the original code */
	writel(0, reg_base + AHCI_RWCR);
	msleep(5);

	sunxi_setbits(reg_base + AHCI_PHYCS1R, BIT(19));
	sunxi_clrsetbits(reg_base + AHCI_PHYCS0R,
			 (0x7 << 24),
			 (0x5 << 24) | BIT(23) | BIT(18));
	sunxi_clrsetbits(reg_base + AHCI_PHYCS1R,
			 (0x3 << 16) | (0x1f << 8) | (0x3 << 6),
			 (0x2 << 16) | (0x6 << 8) | (0x2 << 6));
	sunxi_setbits(reg_base + AHCI_PHYCS1R, BIT(28) | BIT(15));
	sunxi_clrbits(reg_base + AHCI_PHYCS1R, BIT(19));
	sunxi_clrsetbits(reg_base + AHCI_PHYCS0R,
			 (0x7 << 20), (0x3 << 20));
	sunxi_clrsetbits(reg_base + AHCI_PHYCS2R,
			 (0x1f << 5), (0x19 << 5));
	msleep(5);

	sunxi_setbits(reg_base + AHCI_PHYCS0R, (0x1 << 19));

	timeout = 250; /* Power up takes aprox 50 us */
	do {
		reg_val = sunxi_getbits(reg_base + AHCI_PHYCS0R, 0x7, 28);
		if (reg_val == 0x02)
			break;

		if (--timeout == 0) {
			dev_err(dev, "PHY power up failed.\n");
			return -EIO;
		}
		udelay(1);
	} while (1);

	sunxi_setbits(reg_base + AHCI_PHYCS2R, (0x1 << 24));

	timeout = 100; /* Calibration takes aprox 10 us */
	do {
		reg_val = sunxi_getbits(reg_base + AHCI_PHYCS2R, 0x1, 24);
		if (reg_val == 0x00)
			break;

		if (--timeout == 0) {
			dev_err(dev, "PHY calibration failed.\n");
			return -EIO;
		}
		udelay(1);
	} while (1);

	msleep(15);

	writel(0x7, reg_base + AHCI_RWCR);

	return 0;
}

static int ahci_sunxi_is_enabled(struct device *dev)
{
	int ret = 0;
	int is_enabled = 0;

	const char  *used_status;

	ret = of_property_read_string(dev->of_node, "status", &used_status);
	if (ret) {
		dev_err(dev, "get ahci_sunxi_is_enabled is fail, %d\n", -ret);
		is_enabled = 0;
	}else if (!strcmp(used_status, "okay")) {
		is_enabled = 1;
	}else {
		dev_err(dev, "AHCI is not enabled\n");
		is_enabled = 0;
	}

	return is_enabled;
}

static int ahci_sunxi_gpio_set(struct device *dev, int value)
{
	int rc = 0;
	int ret;
	u32 pio_hdle;

	ret = of_get_named_gpio(dev->of_node, ahci_sunxi_gpio_name, 0);
	if (!gpio_is_valid(ret)) {
		dev_err(dev, "SATA power enable do not exist!!\n");
		rc = 0;
		goto err0;
	}else{
		pio_hdle = ret;
	}

	ret = gpio_request(pio_hdle, "sata");
	if (ret) {
		dev_err(dev, "SATA request power enable failed\n");
		return -EINVAL;
		goto err0;
	}

	gpio_direction_output(pio_hdle, value);
	gpio_free(pio_hdle);

err0:
	return rc;
}

static int ahci_sunxi_start(struct device *dev, void __iomem *addr)
{
	struct clk *pclk;
	struct clk *mclk;

	int rc = 0;

	if(!ahci_sunxi_is_enabled(dev)){
		return -ENODEV;
	}

	/*Enable mclk and pclk for AHCI*/
	mclk = of_clk_get(dev->of_node, 1);
	if (IS_ERR_OR_NULL(mclk)){
		dev_err(dev, "Error to get module clk for AHCI\n");
		rc = -EINVAL;
		goto err1;
    }

	pclk = of_clk_get(dev->of_node, 0);
	if (IS_ERR_OR_NULL(pclk)){
		dev_err(dev, "Error to get pll clk for AHCI\n");
		rc = -EINVAL;
		goto err0;
	}

	/*Enable SATA Clock in SATA PLL*/
//	ahci_writel(CCMU_PLL6_VBASE, 0, ahci_readl(CCMU_PLL6_VBASE, 0)|(0x1<<14));
	clk_prepare_enable(mclk);
	clk_prepare_enable(pclk);

	rc = ahci_sunxi_phy_init(dev, addr);

	rc = ahci_sunxi_gpio_set(dev, 1);

	clk_put(pclk);
err0:
	clk_put(mclk);
err1:
	return rc;
}

static void ahci_sunxi_stop(struct device *dev)
{
	struct clk *pclk;
	struct clk *mclk;

	int rc = 0;

	mclk = of_clk_get(dev->of_node, 1);
	if (IS_ERR(mclk)){
		dev_err(dev, "Error to get module clk for AHCI\n");
		rc = -EINVAL;
		goto err1;
    }

	pclk = of_clk_get(dev->of_node, 0);
	if (IS_ERR(pclk)){
		dev_err(dev, "Error to get pll clk for AHCI\n");
		rc = -EINVAL;
		goto err0;
	}

	rc = ahci_sunxi_gpio_set(dev, 0);

	/*Disable mclk and pclk for AHCI*/
	clk_disable_unprepare(mclk);
	clk_disable_unprepare(pclk);
	clk_put(pclk);
err0:
	clk_put(mclk);
err1:
	return;// rc;
}

static struct ata_port_info ahci_sunxi_port_info = {
	.flags = AHCI_FLAG_COMMON,
	//.link_flags = ,
	.pio_mask = ATA_PIO4,
	//.mwdma_mask = ,
	.udma_mask = ATA_UDMA6,
	.port_ops = &ahci_ops,
	.private_data = (void*)(AHCI_HFLAG_32BIT_ONLY | AHCI_HFLAG_NO_MSI
							| AHCI_HFLAG_NO_PMP | AHCI_HFLAG_YES_NCQ),
};

static int __init ahci_sunxi_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ahci_platform_data *pdata = dev_get_platdata(dev);

	struct ata_port_info pi = ahci_sunxi_port_info;
	const struct ata_port_info *ppi[] = { &pi, NULL };
	struct ahci_host_priv *hpriv;
	struct ata_host *host;
	struct resource *mem;
	int irq;
	int n_ports;
	int i;
	int rc;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(dev, "no mmio space\n");
		return -EINVAL;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq <= 0) {
		dev_err(dev, "no irq\n");
		return -EINVAL;
	}

	if (pdata && pdata->ata_port_info)
		pi = *pdata->ata_port_info;

	hpriv = devm_kzalloc(dev, sizeof(*hpriv), GFP_KERNEL);
	if (!hpriv) {
		dev_err(dev, "can't alloc ahci_host_priv\n");
		return -ENOMEM;
	}

	hpriv->flags |= (unsigned long)pi.private_data;

	hpriv->mmio = devm_ioremap(dev, mem->start, resource_size(mem));
	if (!hpriv->mmio) {
		dev_err(dev, "can't map %pR\n", mem);
		return -ENOMEM;
	}

	/*
	 * Some platforms might need to prepare for mmio region access,
	 * which could be done in the following init call. So, the mmio
	 * region shouldn't be accessed before init (if provided) has
	 * returned successfully.
	 */

	rc = ahci_sunxi_start(dev, hpriv->mmio);
	if (rc)
		return rc;


	ahci_save_initial_config(dev, hpriv,
		pdata ? pdata->force_port_map : 0,
		pdata ? pdata->mask_port_map  : 0);

	/* prepare host */
	if (hpriv->cap & HOST_CAP_NCQ)
		pi.flags |= ATA_FLAG_NCQ;

	if (hpriv->cap & HOST_CAP_PMP)
		pi.flags |= ATA_FLAG_PMP;

	ahci_set_em_messages(hpriv, &pi);

	/* CAP.NP sometimes indicate the index of the last enabled
	 * port, at other times, that of the last possible port, so
	 * determining the maximum port number requires looking at
	 * both CAP.NP and port_map.
	 */
	n_ports = max(ahci_nr_ports(hpriv->cap), fls(hpriv->port_map));

	host = ata_host_alloc_pinfo(dev, ppi, n_ports);
	if (!host) {
		rc = -ENOMEM;
		goto err0;
	}

	host->private_data = hpriv;

	if (!(hpriv->cap & HOST_CAP_SSS) || ahci_ignore_sss)
		host->flags |= ATA_HOST_PARALLEL_SCAN;
	else
		printk(KERN_INFO "ahci: SSS flag set, parallel bus scan disabled\n");

	if (pi.flags & ATA_FLAG_EM)
		ahci_reset_em(host);

	for (i = 0; i < host->n_ports; i++) {
		struct ata_port *ap = host->ports[i];

		ata_port_desc(ap, "mmio %pR", mem);
		ata_port_desc(ap, "port 0x%x", 0x100 + ap->port_no * 0x80);

		/* set enclosure management message type */
		if (ap->flags & ATA_FLAG_EM)
			ap->em_message_type = hpriv->em_msg_type;

		/* disabled/not-implemented port */
		if (!(hpriv->port_map & (1 << i)))
			ap->ops = &ata_dummy_port_ops;
	}

	rc = ahci_reset_controller(host);
	if (rc)
		goto err0;

	ahci_init_controller(host);
	ahci_print_info(host, "platform");

	rc = ata_host_activate(host, irq, ahci_interrupt, IRQF_SHARED,
			       &ahci_platform_sht);
	if (rc)
		goto err0;

	return 0;
err0:
	ahci_sunxi_stop(dev);
	return rc;
}

void ahci_sunxi_dump_reg(struct device *dev)
{
	struct ata_host *host = dev_get_drvdata(dev);
	struct ahci_host_priv *hpriv = host->private_data;
	void __iomem *base = hpriv->mmio;
	int i = 0;

	for(i=0; i<0x200; i+=0x10) {
		printk("0x%3x = 0x%x, 0x%3x = 0x%x, 0x%3x = 0x%x, 0x%3x = 0x%x\n", i, readl(base+i), i+4, readl(base+ i+4), i+8, readl(base+i+8), i+12, readl(base+i+12));
	}
}

#ifdef CONFIG_PM

static int ahci_sunxi_suspend(struct device *dev)
{

	printk("ahci_sunxi: ahci_sunxi_suspend\n"); //danielwang
//	ahci_sunxi_dump_reg(dev);

	ahci_sunxi_stop(dev);

	return 0;
}

extern int ahci_hardware_recover_for_controller_resume(struct ata_host *host);
static int ahci_sunxi_resume(struct device *dev)
{
	struct ata_host *host = dev_get_drvdata(dev);
	struct ahci_host_priv *hpriv = host->private_data;

	printk("ahci_sunxi: ahci_sunxi_resume\n"); //danielwang

	ahci_sunxi_start(dev, hpriv->mmio);
//	ahci_sunxi_dump_reg(dev);

	ahci_hardware_recover_for_controller_resume(host);

	return 0;
}


static const struct dev_pm_ops  ahci_sunxi_pmops = {
	.suspend	= ahci_sunxi_suspend,
	.resume		= ahci_sunxi_resume,
};

#define AHCI_SUNXI_PMOPS &ahci_sunxi_pmops
#else
#define AHCI_SUNXI_PMOPS NULL
#endif

static const struct of_device_id ahci_sunxi_of_match[] = {
	{ .compatible = "allwinner,sun8i-sata", },
	{},
};
MODULE_DEVICE_TABLE(of, ahci_sunxi_of_match);

static struct platform_driver ahci_sunxi_driver = {
//	.probe = ahci_sunxi_probe,
	.remove = ata_platform_remove_one,
	.driver = {
		.name = "ahci_sunxi",
		.owner = THIS_MODULE,
		.of_match_table = ahci_sunxi_of_match,
		.pm = AHCI_SUNXI_PMOPS,
	},
};

static int __init ahci_sunxi_init(void)
{
	return platform_driver_probe(&ahci_sunxi_driver, ahci_sunxi_probe);
}
module_init(ahci_sunxi_init);

static void __exit ahci_sunxi_exit(void)
{
	platform_driver_unregister(&ahci_sunxi_driver);
}
module_exit(ahci_sunxi_exit);

MODULE_DESCRIPTION("SW AHCI SATA platform driver");
MODULE_AUTHOR("Daniel Wang <danielwang@allwinnertech.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ahci_sunxi");
