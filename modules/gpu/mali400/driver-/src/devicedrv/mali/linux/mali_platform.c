#include <linux/aw/platform.h>

extern unsigned long totalram_pages;

struct aw_fb_addr_para 
{
	long int fb_paddr;
	unsigned int fb_size;
};

// extern int ths_read_data(int value);
extern void sunxi_get_fb_addr_para(struct aw_fb_addr_para *fb_addr_para);
static void mali_dvfs(struct work_struct *work);
static void mali_gpu_utilization_callback(struct mali_gpu_utilization_data *data);
#if !defined(CONFIG_MALI_DT)
static int mali_platform_device_unregister(void);
#endif

static struct work_struct wq_work;

static struct mali_gpu_device_data mali_gpu_data =
{
	.control_interval = 250,
    .utilization_callback = mali_gpu_utilization_callback,
};

#if !defined(CONFIG_MALI_DT)
static struct resource mali_gpu_resources[]=
{
    MALI_GPU_RESOURCES_MALI400_MP2_PMU(GPU_PBASE, IRQ_GPU_GP, IRQ_GPU_GPMMU, \
                                        IRQ_GPU_PP0, IRQ_GPU_PPMMU0, IRQ_GPU_PP1, IRQ_GPU_PPMMU1)
};

static struct platform_device mali_gpu_device =
{
    .name = MALI_GPU_NAME_UTGARD,
    .id = 0,
    .dev.coherent_dma_mask = DMA_BIT_MASK(32),
};
#endif

/*
********************************************************************************************************
 @Function   :get_gpu_clk

 @Description:Get gpu related clocks

 @Input      :None

 @Return     :Zero or error code
********************************************************************************************************
*/
static int get_gpu_clk(void)
{
	int i;
#if defined(CONFIG_MALI_DT)
	struct device_node * gpu_dn = NULL;
	gpu_dn = of_find_compatible_node(NULL, NULL, "arm,mali-400");
#endif
	for(i = 0; i < sizeof(clk_data)/sizeof(clk_data[0]); i++)
	{
	#if defined(CONFIG_MALI_DT)
		clk_data[i].clk_handle = of_clk_get(gpu_dn, i);
	#else
		clk_data[i].clk_handle = clk_get(NULL, clk_data[i].clk_id);
	#endif
		if(!clk_data[i].clk_handle || IS_ERR(clk_data[i].clk_handle))
		{
			MALI_PRINT_ERROR(("Failed to get gpu %s clock!\n", clk_data[i].clk_name));
			return -1;
		}
	}

	return 0;
}

/*
********************************************************************************************************
 @Function   :set_clk_freq

 @Description:Set the frequency of gpu

 @Input	     :Frequency value

 @Return     :None
********************************************************************************************************
*/
static int set_clk_freq(int freq /* MHz */)
{	
	int i;

	if(freq <= vf_table[private_data.max_level].max_freq)
	{
		if(freq != clk_get_rate(clk_data[0].clk_handle)/(1000*1000))
		{
			for(i = 0; i < sizeof(clk_data)/sizeof(clk_data[0]); i++)
			{
				if(clk_data[i].expected_freq <= 0)
				{
					continue;
				}
				else if(clk_data[i].expected_freq > 1)
				{
					freq = clk_data[i].expected_freq;
				}
				
				if(clk_set_rate(clk_data[i].clk_handle, freq*1000*1000))
				{
					MALI_PRINT_ERROR(("Failed to set the frequency of gpu %s clock: Current frequency is %ld MHz, the frequency to be is %d MHz", clk_data[i].clk_name, clk_get_rate(clk_data[i].clk_handle)/(1000*1000), freq));

					return -1;
				}
			}
		}
	}
	
	return 0;
}

/*
********************************************************************************************************
 @Function   :set_gpu_freq

 @Description:Set the frequency of gpu, which is used by dvfs

 @Input	     :Frequency value

 @Return     :None
********************************************************************************************************
*/
static void set_gpu_freq(int freq /* MHz */)
{
	int i;

	if(freq > vf_table[private_data.max_level].max_freq)
	{
		MALI_PRINT_ERROR(("Failed to set the frequency of gpu: The frequency to be is %d MHz, but the current frequency is %ld MHz, it is beyond the permitted frequency boundary %d MHz", freq, clk_get_rate(clk_data[0].clk_handle)/(1000*1000), vf_table[private_data.max_level].max_freq));
		return;
	}
	
	if(freq == clk_get_rate(clk_data[0].clk_handle)/(1000*1000))
	{
		return;
	}
	
	for(i = 0; i <= private_data.max_level; i++)
	{
		if(freq > vf_table[i].max_freq)
		{
			continue;
		}
		mutex_lock(&private_data.dvfs_lock);
		mali_dev_pause();

		if(vf_table[i].vol == regulator_get_voltage(private_data.regulator)/1000)
		{
			set_clk_freq(freq);
		}
		else
		{
			if(freq > clk_get_rate(clk_data[0].clk_handle))
			{
				set_clk_freq(freq);
			}
			else
			{
				set_clk_freq(freq);
			}
		}
		mali_dev_resume();
		
		MALI_PRINT_ERROR(("set_gpu_freq  freq = %d, vf_table[private_data.max_level].max_freq = %d\n", freq, vf_table[private_data.max_level].max_freq));
		mutex_unlock(&private_data.dvfs_lock);
		break;
	}
}

/*
********************************************************************************************************
 @Function   :set_clk_parent

 @Description:Set the parent clock of all gpu related clocks

 @Input	     :None

 @Return     :None
********************************************************************************************************
*/
static void set_clk_parent(void)
{
	int i, j;
	for(i = 0; i < sizeof(clk_data)/sizeof(clk_data[0]); i++)
	{
		if(NULL != clk_data[i].clk_parent_id)
		{
			for(j = 0; j < sizeof(clk_data)/sizeof(clk_data[0]); j++)
			{
				if(clk_data[i].clk_parent_id == clk_data[j].clk_id)
				{
					if (clk_set_parent(clk_data[i].clk_handle, clk_data[j].clk_handle))
					{
						MALI_PRINT_ERROR(("Failed to set the parent of gpu %s clock!", clk_data[i].clk_name));
					}
				}
			}
		}
	}
}

/*
********************************************************************************************************
 @Function   :enable_gpu_clk

 @Description:Enable gpu related clocks

 @Input	     :None

 @Return     :None
********************************************************************************************************
*/
int enable_gpu_clk(void)
{
	int i;
	
	if(!private_data.status.clk_enable)
	{
		for(i=0; i<sizeof(clk_data)/sizeof(clk_data[0]); i++)
		{
			if(clk_prepare_enable(clk_data[i].clk_handle))
			{
				MALI_PRINT_ERROR(("Failed to enable %s!\n", clk_data[i].clk_name));
				return -1;
			}
		}
		private_data.status.clk_enable = 1;
	}
	
	return 0;
}

/*
********************************************************************************************************
 @Function   :disable_gpu_clk
	
 @Description:Disable gpu related clocks
	
 @Input	     :None

 @Return     :None
********************************************************************************************************
*/
void disable_gpu_clk(void)
{
	int i;
	if(private_data.status.clk_enable)
	{
		for(i=sizeof(clk_data)/sizeof(clk_data[0])-1; i>=0; i--)
		{
			clk_disable_unprepare(clk_data[i].clk_handle);
		}

		private_data.status.clk_enable = 0;
	}
}

/*
********************************************************************************************************
 @Function   :manual_dvfs_show

 @Description:Show the current gpu frequency when read the manual file node

 @Input      :dev, attr, buf

 @Return     :The value of current gpu frequency
********************************************************************************************************
*/
static ssize_t manual_dvfs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%ld MHz\n", clk_get_rate(clk_data[0].clk_handle)/(1000*1000));
}

/*
********************************************************************************************************
 @Function   :manual_dvfs_store

 @Description:Change the current gpu frequency when write the manual file node

 @Input      :dev, attr, buf

 @Return     :count
********************************************************************************************************
*/
static ssize_t manual_dvfs_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int err;
	unsigned long freq;	
	
	err = strict_strtoul(buf, 10, &freq);
	if(err)
	{
		MALI_PRINT_ERROR(("Invalid parameter!"));
		goto err_out;
	}

	set_gpu_freq(freq);

err_out:
	return count;
}

static DEVICE_ATTR(manual, S_IRUGO|S_IWUGO, manual_dvfs_show, manual_dvfs_store);

static struct attribute *gpu_attributes[] =
{
    &dev_attr_manual.attr,   
    NULL
};

struct attribute_group gpu_attribute_group = {
  .name = "dvfs",
  .attrs = gpu_attributes
};

/*
********************************************************************************************************
 @Function   :mali_platform_init

 @Description:Init the power and clocks of gpu

 @Input	     :None

 @Return     :Zero or error code
********************************************************************************************************
*/
static int mali_platform_init(void)
{
	private_data.max_level = sizeof(vf_table)/sizeof(vf_table[0]) - 1;
	
	if(get_gpu_clk() < 0)
	{
		goto err_out;
	}

	mutex_init(&private_data.dvfs_lock);

	INIT_WORK(&wq_work, mali_dvfs);

	set_clk_parent();

	set_clk_freq(vf_table[private_data.max_level].max_freq);

	if(enable_gpu_clk() < 0)
	{
		goto err_out;
	}
	
	MALI_PRINT(("Init Mali gpu successfully\n"));
    return 0;

err_out:
	MALI_PRINT_ERROR(("Failed to init Mali gpu!\n"));
	return -1;
}

/*
********************************************************************************************************
 @Function   :mali_platform_device_register

 @Description:Register mali platform device

 @Input	     :None

 @Return     :Zero or error code
********************************************************************************************************
*/
#if defined(CONFIG_MALI_DT)
int mali_platform_device_init(struct platform_device *mali_pdev)
#else
int mali_platform_device_register(void)
#endif
{
	int err;
    struct aw_fb_addr_para fb_addr_para = {0};
	struct platform_device *mali_pdev_tmp;
#if defined(CONFIG_MALI_DT)
	u64 sunxi_dma_mask = DMA_BIT_MASK(32);
	mali_pdev->dev.dma_mask = &sunxi_dma_mask;
	mali_pdev->dev.coherent_dma_mask = sunxi_dma_mask;
#endif

    sunxi_get_fb_addr_para(&fb_addr_para);

#if !defined(CONFIG_MALI_DT)
    err = platform_device_add_resources(&mali_gpu_device, mali_gpu_resources, sizeof(mali_gpu_resources) / sizeof(mali_gpu_resources[0]));
	if(err)
	{
		MALI_PRINT_ERROR(("Failed to add platform device resources!\n"));
		return -1;
	}
#endif
    mali_gpu_data.fb_start = fb_addr_para.fb_paddr;
    mali_gpu_data.fb_size = fb_addr_para.fb_size;
	mali_gpu_data.shared_mem_size = totalram_pages * PAGE_SIZE; /* B */

#if defined(CONFIG_MALI_DT)
	mali_pdev_tmp = mali_pdev;
#else
	mali_pdev_tmp = &mali_gpu_device;
#endif
    err = platform_device_add_data(mali_pdev_tmp, &mali_gpu_data, sizeof(mali_gpu_data));
    if(0 == err)
	{
	#if !defined(CONFIG_MALI_DT)
        err = platform_device_register(mali_pdev_tmp);	
        if(err)
		{
			MALI_PRINT_ERROR(("Failed to register platform device!\n"));
			return -1;
		}
	#endif
        if(0 != mali_platform_init())
		{
			return -1;
		}
#if defined(CONFIG_PM_RUNTIME)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
		pm_runtime_set_autosuspend_delay(&(mali_pdev_tmp.dev), 1000);
		pm_runtime_use_autosuspend(&(mali_pdev_tmp.dev));
#endif
		pm_runtime_enable(&(mali_pdev_tmp.dev));
#endif /* CONFIG_PM_RUNTIME */

		sysfs_create_group(&(mali_pdev_tmp->dev.kobj), &gpu_attribute_group);

        return 0;
#if !defined(CONFIG_MALI_DT)
        mali_platform_device_unregister();
#endif
    }

    return err;
}

/*
********************************************************************************************************
 @Function   :mali_platform_device_deinit/mali_platform_device_unregister

 @Description:Release the mali resource

 @Input	     :device/None

 @Return     :Zero
********************************************************************************************************
*/
#if defined(CONFIG_MALI_DT)
int mali_platform_device_deinit(struct platform_device *device)
#else
static int mali_platform_device_unregister(void)
#endif
{
#if !defined(CONFIG_MALI_DT)
	platform_device_unregister(&mali_gpu_device);
#endif
	disable_gpu_clk();

	return 0;
}

/*
********************************************************************************************************
 @Function   :mali_dvfs

 @Description:Change the gpu frequency

 @Input	     :work

 @Return     :None
********************************************************************************************************
*/
static void mali_dvfs(struct work_struct *work)
{
	int cur_freq = clk_get_rate(clk_data[0].clk_handle)/(1000*1000);
	int high_level = private_data.max_level, low_level = 0, temp_level, level_num;

	if(cur_freq > vf_table[private_data.max_level].max_freq)
	{
		set_gpu_freq(vf_table[private_data.max_level].max_freq);
		return;
	}

	if(private_data.status.dvfs_enable && private_data.dvfs_flag)
	{
		if(private_data.dvfs_flag < 0 && cur_freq <= vf_table[0].max_freq)
		{
			return;
		}
		else if(private_data.dvfs_flag > 0 && cur_freq == vf_table[private_data.max_level].max_freq)
		{
			return;
		}
		
		/* Find the nearest levels */		
		while(high_level-low_level != 1)
		{
			level_num = high_level - low_level + 1;
			temp_level = low_level + level_num/2 + level_num%2 -1;
			
			if(cur_freq > vf_table[temp_level].max_freq)
			{
				low_level = temp_level;
			}
			else
			{
				high_level = temp_level;
			}
		}

		if(private_data.dvfs_flag > 0 && cur_freq == vf_table[high_level].max_freq)
		{
			if(high_level < private_data.max_level)
			{
				high_level += 1;
			}
		}
		else if(private_data.dvfs_flag < 0 && cur_freq == vf_table[low_level].max_freq)
		{
			if(high_level < private_data.max_level)
			{
				high_level += 1;
			}
		}

		set_gpu_freq(vf_table[private_data.dvfs_flag < 0 ? low_level : high_level].max_freq);

		private_data.dvfs_flag = 0;
	}
}

/*
********************************************************************************************************
 @Function   :mali_gpu_utilization_callback

 @Description:Decide whether to call mali_dvfs function according to gpu utilization and gpu temperature 

 @Input	     :data

 @Return     :None
********************************************************************************************************
*/
static void mali_gpu_utilization_callback(struct mali_gpu_utilization_data *data)
{
	int utilization = 0;
	int temperature = 0;
	int i = 0;

	// temperature = ths_read_data(0);
	/* Change the private_data.max_level according to the sensor temperature */
	for(i = sizeof(thermal_ctrl_data)/sizeof(thermal_ctrl_data[0]) -1; i >= 0; i--)
	{
		if(temperature >= thermal_ctrl_data[i].temp)
		{
			private_data.max_level = thermal_ctrl_data[i].max_level;
			goto out;
		}
	}

	if(private_data.status.dvfs_enable)
	{
		utilization = data->utilization_gpu;
		/* Determine the frequency change */
		if(utilization > 250)
		{
			private_data.dvfs_flag = 1;
		}
		else if(utilization < 50)
		{
			private_data.dvfs_flag = -1;
		}
		else
		{
			return;
		}
	}

out:
	schedule_work(&wq_work);
}