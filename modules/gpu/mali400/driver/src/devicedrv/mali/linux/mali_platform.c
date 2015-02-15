#include <linux/mali/mali_utgard.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/clk-private.h>
#include <linux/sys_config.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of.h>
#include <linux/dma-mapping.h>
#include <linux/of_address.h>

static struct clk *mali_clk = NULL;
static struct clk *gpu_pll  = NULL;

extern unsigned long totalram_pages;

static struct mali_gpu_device_data mali_gpu_data;

/*
***************************************************************
 @Function   :get_gpu_clk

 @Description:Get gpu related clocks

 @Input      :None

 @Return     :Zero or error code
***************************************************************
*/
static int get_gpu_clk(void)
{
	struct device_node * np_gpu = NULL;

	np_gpu = of_find_compatible_node(NULL, NULL, "arm,mali-400");

	gpu_pll = of_clk_get(np_gpu, 0);
	printk(KERN_ERR "gpu_pll_name = %s\n", gpu_pll->name);

	mali_clk = of_clk_get(np_gpu, 1);
	printk(KERN_ERR "mali_clk_name = %s\n", mali_clk->name);

	return 0;
}

/*
***************************************************************
 @Function   :set_gpu_freq

 @Description:Set the frequency of gpu related clocks

 @Input	     :Frequency value

 @Return     :Zero or error code
***************************************************************
*/
static int set_gpu_freq(int freq ) /* MHz */
{
	if(clk_set_rate(gpu_pll, freq*1000*1000))
    {
        printk(KERN_ERR "Failed to set gpu pll clock!\n");
		return -1;
    }

	if(clk_set_rate(mali_clk, freq*1000*1000))
	{
		printk(KERN_ERR "Failed to set mali clock!\n");
		return -1;
	}

	return 0;
}

/*
***************************************************************
 @Function   :enable_gpu_clk

 @Description:Enable gpu related clocks

 @Input	     :None

 @Return     :None
***************************************************************
*/
void enable_gpu_clk(void)
{
	if(mali_clk->enable_count == 0)
	{
		if(clk_prepare_enable(gpu_pll))
		{
			printk(KERN_ERR "Failed to enable gpu pll!\n");
		}

		if(clk_prepare_enable(mali_clk))
		{
			printk(KERN_ERR "Failed to enable mali clock!\n");
		}
	}
}

/*
***************************************************************
 @Function   :disable_gpu_clk

 @Description:Disable gpu related clocks

 @Input	     :None

 @Return     :None
***************************************************************
*/
void disable_gpu_clk(void)
{
	if(mali_clk->enable_count == 1)
	{
		clk_disable_unprepare(mali_clk);
		clk_disable_unprepare(gpu_pll);
	}
}

int mali_platform_device_deinit(struct platform_device *device)
{
	disable_gpu_clk();

	return 0;
}

/*
***************************************************************
 @Function   :mali_platform_init

 @Description:Init the power and clocks of gpu

 @Input	     :None

 @Return     :Zero or error code
***************************************************************
*/
static int mali_platform_init(struct platform_device *mali_pdev)
{
	printk(KERN_ERR "OK to init Mali gpu in get_gpu_clk and mali_platform_init!\n");
	if(get_gpu_clk())
	{
		printk(KERN_ERR "Failed to init Mali gpu in get_gpu_clk and mali_platform_init!\n");
		goto err_out;
	}
	printk(KERN_ERR "OK to init Mali gpu in get_gpu_clk and mali_platform_init!\n");

	if(set_gpu_freq(456))
	{
		printk(KERN_ERR "Failed to init Mali gpu in set_gpu_freq and mali_platform_init!\n");
		goto err_out;
	}
	printk(KERN_ERR "OK to init Mali gpu in set_gpu_freq and mali_platform_init!\n");

	enable_gpu_clk();

	printk(KERN_ERR "Init Mali gpu successfully\n");
    return 0;

err_out:
	printk(KERN_ERR "Failed to init Mali gpu!\n");
	return -1;
}

static u64 sunxi_dma_mask = DMA_BIT_MASK(32);
int mali_platform_device_init(struct platform_device *mali_pdev)
{
	int err=0;

	mali_pdev->dev.dma_mask = &sunxi_dma_mask;
	mali_pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	
	mali_gpu_data.shared_mem_size = totalram_pages * PAGE_SIZE; /* B */

	err |= platform_device_add_data(mali_pdev, &mali_gpu_data, sizeof(mali_gpu_data));

	if(0 == err){
		err = mali_platform_init(mali_pdev);

		if(0 != err){
			printk(KERN_ERR "Failed to mali_platform_init in mali_platform_device_init!\n");
			return -1;
		}

#if defined(CONFIG_PM_RUNTIME)
		pm_runtime_set_autosuspend_delay(&(mali_pdev.dev), 1000);
		pm_runtime_use_autosuspend(&(mali_pdev.dev));
		pm_runtime_enable(&(mali_pdev.dev));
#endif /* CONFIG_PM_RUNTIME */

	} else{
		printk(KERN_ERR "Failed to platform_device_add_data in mali_platform_device_init!\n");
	}

    return err;
}