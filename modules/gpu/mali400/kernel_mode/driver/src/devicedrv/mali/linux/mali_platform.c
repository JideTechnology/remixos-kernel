/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Copyright (C) 2015 Allwinner Technology Co., Ltd.
 *
 * Author: Xiangyun Yu <yuxyun@allwinnertech.com>
 */

#include <linux/aw/platform.h>

extern unsigned long totalram_pages;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0))
extern int ths_read_data(int value);
#else
extern int sunxi_get_sensor_temp(u32 sensor_num, long *temperature);
#endif

static void mali_change_freq(struct work_struct *work);
static void mali_gpu_utilization_callback(struct mali_gpu_utilization_data *data);

static struct work_struct wq_work;

static struct mali_gpu_device_data mali_gpu_data =
{
	.control_interval     = 250,
    .utilization_callback = mali_gpu_utilization_callback,
};

#ifndef CONFIG_MALI_DT
static struct platform_device mali_gpu_device =
{
    .name = MALI_GPU_NAME_UTGARD,
    .id = 0,
};
#endif

/*
***************************************************************
 @Function   :get_gpu_clk
 @Description:Get gpu related clocks
***************************************************************
*/
static bool get_gpu_clk(void)
{
	int i;
#ifdef CONFIG_MALI_DT
	struct device_node * np_gpu = NULL;
	np_gpu = of_find_compatible_node(NULL, NULL, "arm,mali-400");
#endif /* CONFIG_MALI_DT */
	for(i = 0; i < sizeof(clk_data)/sizeof(clk_data[0]); i++)
	{
#ifdef CONFIG_MALI_DT
		clk_data[i].clk_handle = of_clk_get(np_gpu, i);
#else
		clk_data[i].clk_handle = clk_get(NULL, clk_data[i].clk_id);
#endif /* CONFIG_MALI_DT */
		if(!clk_data[i].clk_handle || IS_ERR(clk_data[i].clk_handle))
		{
			MALI_PRINT_ERROR(("Failed to get gpu %s clock!\n", clk_data[i].clk_name));
			return 0;
		}
	}

	return 1;
}

/*
***************************************************************
 @Function   :get_current_freq
 @Description:Get current frequency of gpu
***************************************************************
*/
static int get_current_freq(void)
{
	return clk_get_rate(clk_data[0].clk_handle)/(1000*1000);
}
 
/*
***************************************************************
 @Function   :set_clk_freq
 @Description:Set clock's frequency
***************************************************************
*/
static bool set_clk_freq(int freq /* MHz */)
{	
	int i;

	if(freq <= vf_table[private_data.dvfs_data.max_level].max_freq)
	{
		for(i = 0; i < sizeof(clk_data)/sizeof(clk_data[0]); i++)
		{
			if(clk_set_rate(clk_data[i].clk_handle, freq*1000*1000))
			{
				MALI_PRINT_ERROR(("Failed to set the frequency of gpu %s clock: Current frequency is %ld MHz, the frequency to be is %d MHz\n", clk_data[i].clk_name, clk_get_rate(clk_data[i].clk_handle)/(1000*1000), freq));
				return 0;
			}
		}
	}

	return 1;
}

/*
***************************************************************
 @Function   :set_gpu_freq
 @Description:Set the frequency of gpu
***************************************************************
*/
static void set_gpu_freq(int freq /* MHz */)
{
	if(freq > vf_table[private_data.dvfs_data.max_level].max_freq)
	{
		MALI_PRINT_ERROR(("Failed to set the frequency of gpu: The frequency to be is %d MHz, it is beyond the permitted frequency boundary %d MHz\n", freq, vf_table[private_data.dvfs_data.max_level].max_freq));
		return;
	}

	mutex_lock(&private_data.dvfs_data.dvfs_lock);
	mali_dev_pause();
	if(set_clk_freq(freq))
	{
		MALI_PRINT(("Set gpu frequency to %d MHz\n", freq));
	}
	mali_dev_resume();
	mutex_unlock(&private_data.dvfs_data.dvfs_lock);
}

/*
***************************************************************
 @Function   :enable_gpu_clk
 @Description:Enable gpu related clock gatings
***************************************************************
*/
bool enable_gpu_clk(void)
{
	if(!private_data.clk_status)
	{
		int i;
		for(i=0; i<sizeof(clk_data)/sizeof(clk_data[0]); i++)
		{
			if(clk_prepare_enable(clk_data[i].clk_handle))
			{
				MALI_PRINT_ERROR(("Failed to enable %s clock!\n", clk_data[i].clk_name));
				return 0;
			}
		}
		private_data.clk_status = 1;
	}

	return 1;
}

/*
***************************************************************
 @Function   :disable_gpu_clk
 @Description:Disable gpu related clock gatings
***************************************************************
*/
void disable_gpu_clk(void)
{
	if(private_data.clk_status)
	{
		int i;
		for(i=sizeof(clk_data)/sizeof(clk_data[0])-1; i>=0; i--)
		{
			clk_disable_unprepare(clk_data[i].clk_handle);
		}

		private_data.clk_status = 0;
	}
}

#ifdef CONFIG_MALI_DT
/*
***************************************************************
 @Function   :mali_platform_device_deinit
 @Description:Remove the resource gpu used, called when mali 
			  driver is removed
***************************************************************
*/
int mali_platform_device_deinit(struct platform_device *device)
{
	disable_gpu_clk();

	return 0;
}
#endif /* CONFIG_MALI_DT */

/*
***************************************************************
 @Function   :dvfs_manual_show
 @Description:Show the gpu frequency for manual
***************************************************************
*/
static ssize_t dvfs_manual_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d MHz\n", get_current_freq());
}

/*
***************************************************************
 @Function   :dvfs_manual_store
 @Description:Change gpu frequency for manual
***************************************************************
*/
static ssize_t dvfs_manual_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int err;
	unsigned long freq;	

	err = strict_strtoul(buf, 10, &freq);
	if (err)
	{
		MALI_PRINT_ERROR(("Invalid parameter!\n"));
		goto err_out;
	}

	set_gpu_freq(freq);

err_out:
	return count;
}

/*
***************************************************************
 @Function   :dvfs_manual_show
 @Description:Show the gpu frequency for android
***************************************************************
*/
static ssize_t dvfs_android_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return dvfs_manual_show(dev, attr, buf);
}

/*
***************************************************************
 @Function   :dvfs_android_store
 @Description:Change gpu frequency for android
***************************************************************
*/
static ssize_t dvfs_android_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    if(private_data.dvfs_data.dvfs_status)
	{
		return dvfs_manual_store(dev, attr, buf, count);
	}

	return count;
}

/*
***************************************************************
 @Function   :status_dvfs_show
 @Description:Show the dvfs status
***************************************************************
*/
static ssize_t status_dvfs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", private_data.dvfs_data.dvfs_status);
}

/*
***************************************************************
 @Function   :status_dvfs_store
 @Description:Change the dvfs status
***************************************************************
*/
static ssize_t status_dvfs_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int err;
	unsigned long status;
	err = strict_strtoul(buf, 10, &status);
	if (err)
	{
		MALI_PRINT_ERROR(("Invalid parameter!\n"));
		goto out;
	}

	private_data.dvfs_data.dvfs_status = status;

out:
	return count;
}

/*
***************************************************************
 @Function   :status_tempctrl_show
 @Description:Show the temperature control status
***************************************************************
*/
static ssize_t status_tempctrl_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", private_data.temp_ctrl_status);
}

/*
***************************************************************
 @Function   :status_tempctrl_store
 @Description:Change the temperature control status
***************************************************************
*/
static ssize_t status_tempctrl_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int err;
	unsigned long status;
	err = strict_strtoul(buf, 10, &status);
	if (err)
	{
		MALI_PRINT_ERROR(("Invalid parameter!\n"));
		goto out;
	}

	private_data.temp_ctrl_status = status;

out:
	return count;
}

static DEVICE_ATTR(manual, S_IRUGO|S_IWUGO, dvfs_manual_show, dvfs_manual_store);
static DEVICE_ATTR(android, S_IRUGO|S_IWUGO, dvfs_android_show, dvfs_android_store);
static DEVICE_ATTR(dvfs_status, S_IRUGO|S_IWUGO, status_dvfs_show, status_dvfs_store);
static DEVICE_ATTR(tempctrl_status, S_IRUGO|S_IWUGO, status_tempctrl_show, status_tempctrl_store);

static struct attribute *gpu_attributes[] =
{
    &dev_attr_manual.attr,
	&dev_attr_android.attr,
	&dev_attr_dvfs_status.attr,
	&dev_attr_tempctrl_status.attr,
	
    NULL,
};

struct attribute_group gpu_attribute_group = 
{
	.name = "dvfs",
	.attrs = gpu_attributes,
};

/*
***************************************************************
 @Function   :mali_platform_init
 @Description:Init the clocks of gpu
***************************************************************
*/
static int mali_platform_init(struct platform_device *device)
{
	bool err = 0;

	private_data.dvfs_data.max_level = sizeof(vf_table)/sizeof(vf_table[0]) - 1;

	err = get_gpu_clk();
	err &= set_clk_freq(vf_table[private_data.dvfs_data.max_level].max_freq);	
	err &= enable_gpu_clk();

	if(!err)
	{
		goto err_out;
	}

	MALI_PRINT(("Init Mali gpu clocks successfully\n"));
    return 0;

err_out:
	MALI_PRINT_ERROR(("Failed to init Mali gpu clocks!\n"));
	return -1;
}

/*
***************************************************************
 @Function   :mali_platform_device_init/
              aw_mali_platform_device_register
 @Description:Init the essential data of gpu
***************************************************************
*/
#ifdef CONFIG_MALI_DT
int mali_platform_device_init(struct platform_device *device)
#else
int aw_mali_platform_device_register(void)
#endif
{
	int err=0;
	struct platform_device *pdev;

#ifdef CONFIG_MALI_DT
	pdev = device;
#else
	pdev = &mali_gpu_device;
#endif

	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;

	mali_gpu_data.shared_mem_size = totalram_pages * PAGE_SIZE; /* B */

	err = platform_device_add_data(pdev, &mali_gpu_data, sizeof(mali_gpu_data));

	if(0 == err)
	{
		err = mali_platform_init(pdev);

		if(0 != err)
		{
			return -1;
		}

#if defined(CONFIG_PM_RUNTIME)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
		pm_runtime_set_autosuspend_delay(&(pdev->dev), 1000);
		pm_runtime_use_autosuspend(&(pdev->dev));
#endif
		pm_runtime_enable(&(pdev->dev));
#endif /* CONFIG_PM_RUNTIME */

		mutex_init(&private_data.dvfs_data.dvfs_lock);

		INIT_WORK(&wq_work, mali_change_freq);

		err = sysfs_create_group(&pdev->dev.kobj, &gpu_attribute_group);
		if (err)
		{
			kobject_put(&pdev->dev.kobj);
		}
	}

    return err;
}

/*
***************************************************************
 @Function   :mali_change_freq
 @Description:Change gpu frequency
***************************************************************
*/
static void mali_change_freq(struct work_struct *work)
{
	int cur_freq = get_current_freq();
	int high_level = private_data.dvfs_data.max_level, low_level = 0, temp_level, level_num;

	if(cur_freq > vf_table[private_data.dvfs_data.max_level].max_freq)
	{
		set_gpu_freq(vf_table[private_data.dvfs_data.max_level].max_freq);
		cur_freq = get_current_freq();
	}
	
	if(private_data.dvfs_data.dvfs_status && private_data.dvfs_data.dvfs_flag)
	{
		if(private_data.dvfs_data.dvfs_flag < 0 && cur_freq <= vf_table[0].max_freq)
		{
			goto out;
		}
		else if(private_data.dvfs_data.dvfs_flag > 0 && cur_freq == vf_table[private_data.dvfs_data.max_level].max_freq)
		{
			goto out;
		}

		/* Find the nearest level */
		while(high_level-low_level != 1)
		{
			level_num = high_level - low_level + 1;
			temp_level = low_level + level_num/2;

			if(cur_freq > vf_table[temp_level].max_freq)
			{
				low_level = temp_level;
			}
			else
			{
				high_level = temp_level;
			}
		}

		if(private_data.dvfs_data.dvfs_flag > 0 && cur_freq == vf_table[high_level].max_freq)
		{
			if(high_level < private_data.dvfs_data.max_level)
			{
				high_level += 1;
			}
		}
		else if(private_data.dvfs_data.dvfs_flag < 0 && cur_freq == vf_table[low_level].max_freq)
		{
			if(high_level < private_data.dvfs_data.max_level)
			{
				high_level += 1;
			}
		}

		set_gpu_freq(vf_table[private_data.dvfs_data.dvfs_flag < 0 ? low_level : high_level].max_freq);

out:
		private_data.dvfs_data.dvfs_flag = 0;
	}
}

/*
***************************************************************
 @Function   :mali_gpu_utilization_callback
 @Description:Determine whether to change the gpu frequency
***************************************************************
*/
static void mali_gpu_utilization_callback(struct mali_gpu_utilization_data *data)
{
	int utilization = 0;
	int i = 0;

	if(private_data.temp_ctrl_status)
	{
		long temperature = 0;
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0))
		temperature = ths_read_data(private_data.sensor_num);
	#else
		if(sunxi_get_sensor_temp(private_data.sensor_num, &temperature))
		{
			MALI_PRINT_ERROR(("Failed to get the temperature information from sensor %d!\n", private_data.sensor_num));
		}
	#endif

		/* Change the private_data.dvfs_data.max_level according to the sensor temperature */
		for(i = sizeof(tl_table)/sizeof(tl_table[0]) -1; i >= 0; i--)
		{
			if(temperature >= tl_table[i].temp)
			{
				private_data.dvfs_data.max_level = tl_table[i].level;
			}
		}
	}

	if(private_data.dvfs_data.dvfs_status)
	{
		utilization = data->utilization_gpu;

		/* Determine the frequency change */
		if(utilization > 160)
		{
			private_data.dvfs_data.dvfs_flag = 1;
		}
		else if(utilization < 50)
		{
			private_data.dvfs_data.dvfs_flag = -1;
		}
	}

	schedule_work(&wq_work);
}