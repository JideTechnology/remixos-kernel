/*****************************************************************************
* Copyright(c) O2Micro, 2013. All rights reserved.
*	
* O2Micro oz8806 battery gauge driver
* File: oz8806_battery.c

* Author: Eason.yuan
* $Source: /data/code/CVS
* $Revision: 4.00.01 $
*
* This program is free software and can be edistributed and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*	
* This Source Code Reference Design for O2MICRO oz8806 access (\u201cReference Design\u201d) 
* is sole for the use of PRODUCT INTEGRATION REFERENCE ONLY, and contains confidential 
* and privileged information of O2Micro International Limited. O2Micro shall have no 
* liability to any PARTY FOR THE RELIABILITY, SERVICEABILITY FOR THE RESULT OF PRODUCT 
* INTEGRATION, or results from: (i) any modification or attempted modification of the 
* Reference Design by any party, or (ii) the combination, operation or use of the 
* Reference Design with non-O2Micro Reference Design.
*****************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/fs.h>

#include <linux/string.h>

#include <linux/syscalls.h>
#include <linux/string.h>

#include <linux/miscdevice.h>

#include <asm/uaccess.h>

#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/mm.h>
#include <linux/wakelock.h>

#include <linux/suspend.h>

//you must add these here for O2MICRO
#include "parameter.h"
#include "table.h"
#include "o2_config.h"

#include "../power_supply.h"
#include <linux/acpi.h>

#define batt_dbg(fmt, args...) pr_err(KERN_ERR"[oz8806]:"pr_fmt(fmt)"\n", ## args)


/*****************************************************************************
* static variables/functions section 
****************************************************************************/
static uint8_t 	bmu_init_done = 0;
static uint8_t 	bmu_charge_end = 0;

static DEFINE_MUTEX(update_mutex);
static bmu_data_t 	*batt_info_ptr = NULL;
static gas_gauge_t *gas_gauge_ptr = NULL;
static uint8_t charger_finish = 0;

static void  (*bmu_polling_loop_callback)(void) = NULL;
static void  (*bmu_wake_up_chip_callback)(void) = NULL;
static void  (*bmu_power_down_chip_callback)(void) = NULL;
static void  (*bmu_charge_end_process_callback)(void) = NULL;
static void  (*bmu_discharge_end_process_callback)(void) = NULL;
static int32_t (*oz8806_temp_read_callback)(int32_t *voltage) = NULL;

static int oz8806_update_batt_info(struct oz8806_data *data);

static struct oz8806_data *the_oz8806;
static struct wake_lock battery_wake_lock;
static int8_t adapter_status = O2_CHARGER_BATTERY;
static int32_t rsoc_pre;
static uint8_t ext_thermal_read;
static int capacity_init_ok = 0;
static struct power_supply *battery_psy = NULL;
static struct power_supply *ac_psy = NULL;
static struct power_supply *usb_psy = NULL;
static int create_sys = -EINVAL;
static int save_capacity = INIT_CAP;

#ifdef O2_GAUGE_POWER_SUPPOLY_SUPPORT 
static enum power_supply_property oz8806_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN,
	POWER_SUPPLY_PROP_ENERGY_NOW,
	//POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW,
	//POWER_SUPPLY_PROP_TIME_TO_EMPTY_AVG,
	//POWER_SUPPLY_PROP_TIME_TO_FULL_NOW,
};
#endif


/****************************************************************************
 * mm_init related start
***************************************************************************/

#define PROC_MEMSHARE_DIR         "memshare"
#define PROC_MEMSHARE_PHYADDR     "phymem_addr"
#define PROC_MEMSHARE_SIZE        "phymem_size"

/*alloc 2 page. 8192 bytes*/
#define PAGE_ORDER                  2   //this is very important.we can only allocat 2^PAGE_ORDER pages
										//PAGES_NUMBER must equal 2^PAGE_ORDER !!!!!!!!!!!!!!!!!!!!!!!!

/*this value can get from PAGE_ORDER*/
#define PAGES_NUMBER     			4

//struct proc_dir_entry *proc_memshare_dir ;
static unsigned long kernel_memaddr = 0;
static unsigned long pa_memaddr= 0;
static unsigned long kernel_memsize= 0;

#define DEVICE_NAME	"OZ8806_REG"
#define OZ8806_MAGIC 12
#define OZ8806_IOCTL_RESETLANG 		_IO(OZ8806_MAGIC,0) 
#define OZ8806_IOCTL_GETLANG  		_IOR(OZ8806_MAGIC,1,int) 
#define OZ8806_IOCTL_SETLANG  		_IOW(OZ8806_MAGIC,2,int)
#define OZ8806_READ_INFO  			_IOR(OZ8806_MAGIC,3,int)
#define OZ8806_READ_CONFIG  		_IOR(OZ8806_MAGIC,4,int) 
#define OZ8806_READ_ADDR  			_IOR(OZ8806_MAGIC,5,int)
	
static	int mmap_fault_count = 0;
struct 	mmap_info {
	char *data;			/* the data */
	int reference;      /* how many times it is mmapped */
};

/****************************************************************************
 * mm_init related end
***************************************************************************/



/*****************************************************************************
 * Description:
 *		bmu_call
 * Parameters:
 *		None
 * Return:
 *		None
 *****************************************************************************/
void bmu_call(void)
{
	char * CM_PATH          = "/system/xbin/oz8806api";
	int result  = num_0;
	char* cmdArgv[]={CM_PATH,"/oz8806.txt",NULL};
	char* cmdEnvp[]={"HOME=/","PATH=/system/xbin:/system/bin:/sbin:/bin:/usr/bin:/mnt/sdcard",NULL};
	result=call_usermodehelper(CM_PATH,cmdArgv,cmdEnvp,UMH_WAIT_PROC);

	if(0 == result)
	{
		if(config_data.debug)
		{
			pr_err("test call_usermodehelper is %d\n",result);
		}
	}
	else
		pr_err("test call_usermodehelper is %d\n",result);
	
}
EXPORT_SYMBOL(bmu_call);


/*-------------------------------------------------------------------------*/
/*****************************************************************************
* Description:
*		below funciont is used to operate mmap
* Parameters:
*		description for each argument, new argument starts at new line
* Return:
*		what does this function returned?
*****************************************************************************/
/*-------------------------------------------------------------------------*/

static int proc_read_phymem_addr(char *page, char **start, off_t off, int count)
{
        return sprintf(page, "%s\n", (unsigned char *)__pa(kernel_memaddr));
}

static int proc_read_phymem_size(char *page, char **start, off_t off, int count)
{
        return sprintf(page, "%lu\n", kernel_memsize);
}

static int __init mm_init(void)
{
	int * addr;
	struct page *page;
	int i;

	/*build proc dir "memshare"and two proc files: phymem_addr, phymem_size in the dir*/
	// proc_memshare_dir = proc_mkdir(PROC_MEMSHARE_DIR, NULL);
	// create_proc_info_entry(PROC_MEMSHARE_PHYADDR, 0, proc_memshare_dir, proc_read_phymem_addr);
	//create_proc_info_entry(PROC_MEMSHARE_SIZE, 0, proc_memshare_dir, proc_read_phymem_size);

	/*alloc one page*/
	pr_err("%s\n", __func__);
	kernel_memaddr =__get_free_pages(GFP_KERNEL, PAGE_ORDER);
	if(!kernel_memaddr)
	{
		pr_err("Allocate memory failure!\n");
	}
	else
	{
		page = virt_to_page(kernel_memaddr );
		for(i = 0;i < (1 << PAGE_ORDER);i++)
		{
			SetPageReserved(page);
			page++;
		}
		kernel_memsize = PAGES_NUMBER * PAGE_SIZE;
		//pr_err("Allocate memory success!. The phy mem addr=%08lx, size=%lu\n", __pa(kernel_memaddr), kernel_memsize);
	}
	pa_memaddr = __pa(kernel_memaddr);
	addr  = (int *)(kernel_memaddr);
	//*addr = 0x11223344;
	//pr_err("Allocate memory  addr is %d!\n",*addr);
	return 0;
}

static void __exit mm_close(void)
{
	struct page *page;
	int i;
	pr_err("%s\n", __func__);
	pr_err("The content written by user is: %s\n", (unsigned char *) kernel_memaddr);
	page = virt_to_page(kernel_memaddr);
	for (i = 0; i < (1 << PAGE_ORDER); i++)
	{
		ClearPageReserved(page);
		page++;
	}
	//ClearPageReserved(virt_to_page(kernel_memaddr));
	free_pages(kernel_memaddr, PAGE_ORDER);
	//remove_proc_entry(PROC_MEMSHARE_PHYADDR, proc_memshare_dir);
	//remove_proc_entry(PROC_MEMSHARE_SIZE, proc_memshare_dir);
	//remove_proc_entry(PROC_MEMSHARE_DIR, NULL);
	return;
}


/*-------------------------------------------------------------------------*/
/*****************************************************************************
* Description:
*		below function is used to operate io_ctrl
* Parameters:
*		description for each argument, new argument starts at new line
* Return:
*		what does this function returned?
*****************************************************************************/
/*-------------------------------------------------------------------------*/
static void mmap_open(struct vm_area_struct *vma)
{
	struct mmap_info *info = (struct mmap_info *)vma->vm_private_data;
	pr_err("mmp_open\n");
	info->reference++;
}

static void mmap_close(struct vm_area_struct *vma)
{
	struct mmap_info *info = (struct mmap_info *)vma->vm_private_data;
	pr_err("mmap_close\n");
	info->reference--;
}

static int mmap_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct page *page = NULL;
	char *pageptr = NULL; /* default to "missing" */
	int retval = VM_FAULT_NOPAGE;
	unsigned long offset;

	mmap_fault_count ++;
	pr_err("mmap_fault was called: %d times \n", mmap_fault_count);


	offset = (unsigned long)(vmf->virtual_address - vma->vm_start) + (vma->vm_pgoff << PAGE_SHIFT);
	//pr_err ("vmf->virtual_address = %d\n", vmf->virtual_address);
	//pr_err ("vma->vm_start = %d\n", vma->vm_start);
	//pr_err ("vma->vm_pgoff = %d\n", vma->vm_pgoff);
	
	pageptr = (char *)(kernel_memaddr + offset);
	//pr_err ("kernel_memaddr = %p\n", kernel_memaddr);
	//pr_err ("pageptr = %p\n", pageptr);
	if (!pageptr) 
		return retval;/* hole or end-of-file */

	
	/*
	 * After scullv lookup, "page" is now the address of the page
	 * needed by the current process. Since it's a vmalloc address,
	 * turn it into a struct page.
	 */
	//page = vmalloc_to_page(pageptr);

	
	page = virt_to_page(pageptr); //

	/* got it, now increment the count */
	get_page(page);

	vmf->page = page;
	retval = 0;

	return retval;
	
}

static struct vm_operations_struct mmap_vm_ops = {
	.open =     mmap_open,
	.close =    mmap_close,
	.fault =    mmap_fault,
};

static int oz_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int err;
	err = remap_pfn_range(vma,
						vma->vm_start,
						(pa_memaddr >> PAGE_SHIFT),
						vma->vm_end - vma->vm_start,
						vma->vm_page_prot);
	if (err) {
		pr_err("remap_pfn_range FAILED: %d\n", err);
		return err;
	}
	//pr_err("my_mmap\n");
	vma->vm_ops = &mmap_vm_ops;
	
#ifdef ANDROID_5_XX
	vma->vm_flags |= (VM_IO | VM_LOCKED | (VM_DONTEXPAND | VM_DONTDUMP));
#else
	vma->vm_flags |= VM_RESERVED;
#endif
 
	/* assign the file private data to the vm private data */
	vma->vm_private_data = filp->private_data;
	mmap_open(vma);
	return 0;
}

static int oz8806_reg_open(struct inode *inode, struct file *filp)
{
	struct mmap_info *info = kmalloc(sizeof(struct mmap_info), GFP_KERNEL);
	if (info != NULL) {
		info->data = (char*)kernel_memaddr;	
		filp->private_data = info;	//assign this info struct to the file
		pr_err("oz8806_reg_open\n");
		return 0;
	} 
	else
		return -1;
}     

static long oz8806_reg_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	uint8_t data = 0;
	char buf[100];
	int ret = 0;

	//pr_err("cmd is %d  \n",cmd);
	//pr_err("arg is %d  \n",arg);

	mutex_lock(&update_mutex);

	switch(cmd)
	{
		case OZ8806_IOCTL_GETLANG:
			//pr_err("OZ8806_IOCTL_GETLANG\n");
			//afe_register_read_byte(arg, &data);
			ret = data;
			goto end;

		case OZ8806_IOCTL_SETLANG:
			//pr_err("OZ8806_IOCTL_SETLANG\n");
			ret = copy_from_user(buf, (int *)arg, 100);
			pr_err("%s",buf);
			//afe_register_write_byte(uint8_t index, uint8_t dat);
			goto end;

		case OZ8806_READ_INFO:

			//pr_err("OZ8806_READ_INFO\n");
			if (!batt_info_ptr)
			{
				ret = -EINVAL;
				goto end;
			}

			ret = copy_to_user((int *)arg, batt_info_ptr, sizeof(bmu_data_t)); 
			goto end;

		case OZ8806_READ_CONFIG:

			//pr_err("OZ8806_READ_CONFIG\n");
			//ret = copy_to_user((int *)arg, &config_data, sizeof(config_data_t)); 
			goto end;

		case OZ8806_READ_ADDR:

			//pr_err("OZ8806_READ_ADDR\n");
			ret = copy_to_user((int *)arg, &pa_memaddr, 4); 
			//pr_err("kernel_memaddr is %d\n",*(char *)kernel_memaddr);
			goto end;

		default:
			ret = -EINVAL;
			goto end;
	}

end:
	mutex_unlock(&update_mutex);
	return ret;
}

static int oz8806_reg_close(struct inode *inode, struct file *filp)
{
	struct mmap_info *info = filp->private_data;
	kfree(info);
	filp->private_data = NULL;	
	return 0;
}


static struct file_operations dev_fops = {
	.owner			= THIS_MODULE,
	.unlocked_ioctl	= oz8806_reg_ioctl,
	.open 			= oz8806_reg_open,
	.release 		= oz8806_reg_close,
	.mmap			= oz_mmap,
};

static struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &dev_fops,
};

static int __init dev_init(void)
{
	int ret;
	pr_err("%s\n", __func__);
	ret = misc_register(&misc);
	return ret;
}

static void __exit dev_exit(void)
{
	pr_err("%s\n", __func__);
	misc_deregister(&misc);
}




#ifdef O2_GAUGE_POWER_SUPPOLY_SUPPORT 
/*-------------------------------------------------------------------------*/
/*****************************************************************************
* Description:
*		below function is linux power section
* Parameters:
*		description for each argument, new argument starts at new line
* Return:
*		what does this function returned?
*****************************************************************************/
/*-------------------------------------------------------------------------*/

static int oz8806_battery_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
	int ret = 0;
	
	struct oz8806_data *data = container_of(psy, struct oz8806_data, bat);

	switch (psp) {
	
	case POWER_SUPPLY_PROP_STATUS:
		
		if (adapter_status == O2_CHARGER_BATTERY)
		{
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING; /*discharging*/
		}
		else if(adapter_status == O2_CHARGER_USB ||
		        adapter_status == O2_CHARGER_AC )
		{
			if (data->batt_info.batt_soc == 100)
				val->intval = POWER_SUPPLY_STATUS_FULL;
			else
				val->intval = POWER_SUPPLY_STATUS_CHARGING;	/*charging*/

		}
		else
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;

		
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = data->batt_info.batt_voltage * 1000;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = 1;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = data->batt_info.batt_current * 1000;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = data->batt_info.batt_soc;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;	
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;

	case POWER_SUPPLY_PROP_TEMP:
		val->intval = data->batt_info.batt_temp * 10;
		break;

	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		val->intval = data->batt_info.batt_fcc_data;
		break;

	case POWER_SUPPLY_PROP_CHARGE_NOW:
		val->intval = data->batt_info.batt_capacity;
		break;

	case POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN:
		val->intval = data->batt_info.batt_fcc_data;
		break;

	case POWER_SUPPLY_PROP_ENERGY_NOW:
		val->intval = data->batt_info.batt_capacity;
		break;

	default:
		return -EINVAL;
	}

	return ret;
}

#ifdef INTEL_MACH
static int oz8806_battery_set_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
	struct oz8806_data *data = container_of(psy,
			struct oz8806_data, bat);
	int ret = 0;
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		if (data->status != val->intval) {
			data->status = val->intval;
#ifdef O2_GAUGE_WORK_SUPPORT
			schedule_delayed_work(&data->work,0);
#endif
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}
#endif

static void oz8806_external_power_changed(struct power_supply *psy)
{
	struct oz8806_data *data = container_of(psy,
			struct oz8806_data, bat);

	power_supply_changed(&data->bat);
#ifdef O2_GAUGE_WORK_SUPPORT
	schedule_delayed_work(&data->work,0);
#endif
}

static void oz8806_powersupply_init(struct oz8806_data *data)
{
	data->bat.name = "battery";
	data->bat.type = POWER_SUPPLY_TYPE_BATTERY;
	data->bat.properties = oz8806_battery_props;
	data->bat.num_properties = ARRAY_SIZE(oz8806_battery_props);
	data->bat.get_property = oz8806_battery_get_property;
	data->bat.external_power_changed = oz8806_external_power_changed;
#ifdef INTEL_MACH
	data->bat.set_property = oz8806_battery_set_property;
#endif
}
#endif

#if 1
/*****************************************************************************
 * Description:
 *		oz8806_register_store
 * Parameters:
 * 		read example: echo 88 > register ---read register 0x88
 * 		write example:echo 8807 > register ---write 0x07 into register 0x88
 * Return:
 *		
 * Note:
 *		
 *****************************************************************************/

static ssize_t oz8806_register_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t _count)
{
	char data[num_3];
	uint8_t address;
	uint8_t value = 0;
	char *endp; 
	int len;

	pr_err("oz8806 register storre\n");
	//pr_err("%s \n",buf);
	
	data[num_0] = buf[num_0];
 	data[num_1] =buf[num_1];
 	data[num_2] =num_0;
	//pr_err("data is %s \n",data);


	address = simple_strtoul(data, &endp, 16); 

  	//pr_err("address   is %x \n",address);

	//pr_err("--------------------------- \n");


    len = strlen(buf);

	//pr_err("len is %d \n",len);
	if(len > num_3)
	{
		pr_err("write data \n");
        data[num_0] = buf[num_2];
        data[num_1] =buf[num_3];
        data[num_2] =num_0;

        value = simple_strtoul(data, &endp, 16); 
		
        //afe_register_write_byte(address,value);
        pr_err("value is %x \n",value);
        return _count;
	}
	else
	{
		pr_err("read data \n");
		//afe_register_read_byte(address,&value);
		pr_err("value is %x \n",value);
		return value;	

	}
}

/*
	example:cat register
*/
static ssize_t oz8806_register_show(struct device *dev, struct device_attribute *attr,char *buf)
{	
	struct oz8806_data *data;
   	int yy = 3850;
	
	data = dev_get_drvdata(dev);
	
	//*buf = 0xaa;
	return sprintf(buf,"%d\n",yy);
}

static ssize_t oz8806_debug_show(struct device *dev, struct device_attribute *attr,char *buf)
{
	return sprintf(buf,"%d\n", config_data.debug);
}
/*****************************************************************************
 * Description:
 *		oz8806_debug_store
 * Parameters:
 *		write example: echo 1 > debug ---open debug
 *****************************************************************************/
static ssize_t oz8806_debug_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t _count)
{
	int val;
	if (kstrtoint(buf, 10, &val))
		return -EINVAL;

	if(val == 1)
	{
		config_data.debug = 1;
		pr_err("DEBUG ON \n");
	}
	else if (val == 0)
	{
		config_data.debug = 0;
		pr_err("DEBUG CLOSE \n");
	}
	else
	{
		pr_err("invalid command\n");
		return -EINVAL;
	}

	return _count;
}

static DEVICE_ATTR(registers, S_IRUGO | S_IWUSR, oz8806_register_show, oz8806_register_store);
static DEVICE_ATTR(debug, S_IRUGO | S_IWUGO, oz8806_debug_show, oz8806_debug_store);

static struct attribute *oz8806_attributes[] = {
	&dev_attr_registers.attr,
	&dev_attr_debug.attr,
	NULL,
};

static struct attribute_group oz8806_attribute_group = {
	.attrs = oz8806_attributes,
};
static ssize_t oz8806_bmu_init_done_show(struct device *dev, struct device_attribute *attr,char *buf)
{
	return sprintf(buf,"%d\n", capacity_init_ok);
}

static ssize_t oz8806_save_capacity_show(struct device *dev, struct device_attribute *attr,char *buf)
{
	//start: only for debug
	if (the_oz8806)                                       
		    schedule_delayed_work(&the_oz8806->work, 0);      
	//end: only for debug
	return sprintf(buf,"%d\n", save_capacity);
}

static ssize_t oz8806_save_capacity_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int val;

	if (kstrtoint(buf, 10, &val))
		return -EINVAL;
	save_capacity = val;
	pr_err("save_capacity:%d\n", save_capacity);

	if (the_oz8806)                                       
		    schedule_delayed_work(&the_oz8806->work, 0);      
	return count;
}


static DEVICE_ATTR(bmu_init_done, S_IRUGO, oz8806_bmu_init_done_show, NULL);
static DEVICE_ATTR(save_capacity, S_IRUGO | S_IWUGO, oz8806_save_capacity_show, oz8806_save_capacity_store);


static struct attribute *battery_psy_attributes[] = {
	&dev_attr_bmu_init_done.attr,
	&dev_attr_save_capacity.attr,
	NULL,
};

static struct attribute_group battery_psy_attribute_group = {
	.attrs = battery_psy_attributes,
};

#endif
static int oz8806_create_sys(struct device *dev, const struct attribute_group * grp)
{
	int err;

	pr_err("oz8806_create_sysfs\n");
	
	if(NULL == dev){
		pr_err(KERN_ERR "[BATT]: failed to register battery\n");
		return -EINVAL;
	}

	err = sysfs_create_group(&(dev->kobj), grp);

	if (!err) 
	{
		pr_err("creat oz8806 sysfs group ok \n");	
	} 
	else 
	{
		pr_err("creat oz8806 sysfs group fail\n");
		return -EIO;
	}
	return err;
}
static int get_battery_psy(void)
{
	int ret = -EINVAL;

	battery_psy = power_supply_get_by_name ("battery"); 

	if (battery_psy)
		ret = oz8806_create_sys(battery_psy->dev, &battery_psy_attribute_group);
	else
		pr_err(KERN_ERR "%s: failed to get battery power_supply\n", __func__);

	return ret;
}

static void discharge_end_fun(struct oz8806_data *data)
{
	//End discharge
	//this may jump 2%
	if (!gas_gauge_ptr || !batt_info_ptr)
		return;

	if(batt_info_ptr->fVolt < (config_data.discharge_end_voltage - 50))

	{
		if(batt_info_ptr->fRSOC > 0)
		{
			batt_info_ptr->fRSOC --;

			if(batt_info_ptr->fRSOC < 0)
			{
				batt_info_ptr->fRSOC = 0;
				batt_info_ptr->sCaMAH = data->batt_info.batt_fcc_data / num_100 -1;
				if (bmu_discharge_end_process_callback)
					bmu_discharge_end_process_callback();
			}
			else
				batt_info_ptr->sCaMAH = batt_info_ptr->fRSOC * data->batt_info.batt_fcc_data / num_100;
				
		}
	}
}

static void charge_end_fun(void)
{
#define CHG_END_PERIOD_MS	(MSEC_PER_SEC * 300) //300 s = 5 minutes 
#define FORCE_FULL_MS	(MSEC_PER_SEC * 600) //600 s = 10 minutes 
#define CHG_END_PURSUE_STEP    11

	static unsigned long time_accumulation;
	static unsigned long start_jiffies;
	int oz8806_eoc = 0;
	static unsigned char chgr_full_soc_pursue_cnt = 0;

#ifdef ENABLE_10MIN_END_CHARGE_FUN
	static unsigned long start_record_jiffies;
	static uint8_t start_record_flag = 0;
#endif
	
	if (!gas_gauge_ptr || !batt_info_ptr)
		return;

	if((adapter_status == O2_CHARGER_BATTERY) ||
		(batt_info_ptr->fCurr < DISCH_CURRENT_TH) || 
		(batt_info_ptr->fCurr >  config_data.charge_end_current))
	{
		charger_finish   = 0;
		time_accumulation = 0;
		start_jiffies = 0;
		chgr_full_soc_pursue_cnt = 0;
#ifdef ENABLE_10MIN_END_CHARGE_FUN
		start_record_jiffies = 0;
		start_record_flag = 0;
#endif
		return;
	}	
	/*
	if(adapter_status == CHARGER_USB)
	{
		gas_gauge_ptr->fast_charge_step = 1;
	}
	else
	{
		gas_gauge_ptr->fast_charge_step = 2;
	}
	*/
	
	if(adapter_status == O2_CHARGER_USB)
		oz8806_eoc = 100;
	else if(adapter_status == O2_CHARGER_AC)
		oz8806_eoc = config_data.charge_end_current;

	if((batt_info_ptr->fVolt >= (config_data.charge_cv_voltage -50))&&(batt_info_ptr->fCurr >= DISCH_CURRENT_TH) &&
		(batt_info_ptr->fCurr < oz8806_eoc)&& (!bmu_charge_end))
	{
		if (!start_jiffies)
			start_jiffies = jiffies;

		time_accumulation = jiffies_to_msecs(jiffies - start_jiffies);

		//time accumulation is over 5 minutes
		if (time_accumulation >= CHG_END_PERIOD_MS)
		{
			charger_finish	 = 1;
			pr_err("enter exteral charger finish \n");
		}
		else
		{
			charger_finish	= 0;
		}
	}
	else
	{
		time_accumulation = 0;
		start_jiffies = 0;
		charger_finish = 0;
	}
	batt_dbg("%s, time_accumulation:%ld, charger_finish:%d",
			__func__, time_accumulation, charger_finish);

	batt_dbg("voltage:%d, cv:%d, fcurr:%d, 8806 eoc:%d, bmu_charge_end:%d",
			batt_info_ptr->fVolt, config_data.charge_cv_voltage,
			batt_info_ptr->fCurr, oz8806_eoc, bmu_charge_end);
	
#ifdef ENABLE_10MIN_END_CHARGE_FUN
	if((batt_info_ptr->fRSOC == 99) &&(!start_record_flag) &&(batt_info_ptr->fCurr > oz8806_eoc))
	{
		start_record_jiffies = jiffies;
		start_record_flag = 1;
		pr_err("start_record_flag: %d, at %d ms\n",start_record_flag, jiffies_to_msecs(jiffies));
	}
	if((batt_info_ptr->fRSOC != 99) ||(batt_info_ptr->fCurr < oz8806_eoc))
	{
		start_record_flag = 0;
	}

	if((start_record_flag) && (batt_info_ptr->fCurr > oz8806_eoc))
	{
		if(jiffies_to_msecs(jiffies - start_record_jiffies) > FORCE_FULL_MS)
		{
			pr_err("start_record_flag: %d, at %d ms\n",start_record_flag, jiffies_to_msecs(jiffies));
			charger_finish	 = 1;
			start_record_flag = 0;
			pr_err("enter charge timer finish\n");
		}
	}
#endif

	if(charger_finish)
	{
		if(!bmu_charge_end)
		{
			if(batt_info_ptr->fRSOC < 99)
			{
				if(batt_info_ptr->fRSOC <= rsoc_pre){

					if(chgr_full_soc_pursue_cnt == CHG_END_PURSUE_STEP){

						chgr_full_soc_pursue_cnt = 0;
						batt_info_ptr->fRSOC++;
					} else {
						chgr_full_soc_pursue_cnt++ ;
					}
				}

				if(batt_info_ptr->fRSOC > 100)
					batt_info_ptr->fRSOC = 100;
				
				batt_info_ptr->sCaMAH = batt_info_ptr->fRSOC * the_oz8806->batt_info.batt_fcc_data / num_100;
  				batt_info_ptr->sCaMAH += the_oz8806->batt_info.batt_fcc_data / num_100 - 1;	
				pr_err("enter charger finsh update soc:%d \n",batt_info_ptr->fRSOC);
			}
			else
			{
				pr_err("enter charger charge end\n");
				gas_gauge_ptr->charge_end = 1;
				if (bmu_charge_end_process_callback)
					bmu_charge_end_process_callback();
				charger_finish = 0;
				chgr_full_soc_pursue_cnt = 0;
			}
			
		}
		else {
			charger_finish = 0;
			chgr_full_soc_pursue_cnt = 0;
		}

	} else chgr_full_soc_pursue_cnt = 0;

}


//this is very important code customer need change
//customer should change charge discharge status according to your system
//O2micro
static void system_charge_discharge_status(struct oz8806_data *data)
{
	union power_supply_propval val_ac;
	union power_supply_propval val_usb;

	if (!ac_psy)
		ac_psy = power_supply_get_by_name ("ac"); 

	if (!usb_psy)
		usb_psy= power_supply_get_by_name ("usb"); 

	if (ac_psy && !ac_psy->get_property (ac_psy, POWER_SUPPLY_PROP_ONLINE, &val_ac))
	{
		if (val_ac.intval)
			adapter_status = O2_CHARGER_AC;
	}
	else
		val_ac.intval = 0;

	if (usb_psy && !usb_psy->get_property (usb_psy, POWER_SUPPLY_PROP_ONLINE, &val_usb))
	{
		if (val_usb.intval)
			adapter_status = O2_CHARGER_USB;
	}
	else
		val_usb.intval = 0;

	if (val_ac.intval == 0 && val_usb.intval == 0)
		adapter_status = O2_CHARGER_BATTERY;

#ifdef INTEL_MACH
	if (val_ac.intval || val_usb.intval)
	{
		if (ac_psy && !ac_psy->get_property (ac_psy, POWER_SUPPLY_PROP_CABLE_TYPE, &val_ac))
		{
			if (val_ac.intval == POWER_SUPPLY_TYPE_MAINS)
				adapter_status = O2_CHARGER_AC;
			else
				adapter_status = O2_CHARGER_USB;
		}
		else
			val_ac.intval = 0;
		if (usb_psy && !usb_psy->get_property (usb_psy, POWER_SUPPLY_PROP_CABLE_TYPE, &val_usb))
		{
			if (val_usb.intval == POWER_SUPPLY_TYPE_MAINS)
				adapter_status = O2_CHARGER_AC;
			else
				adapter_status = O2_CHARGER_USB;
		}
		else
			val_usb.intval = 0;

		if (val_ac.intval == 0 && val_usb.intval == 0)
			pr_err("fail to get POWER_SUPPLY_PROP_CABLE_TYPE\n");
	}
#endif
//---------------------------------------------
#ifdef TQ210_TEST

#ifdef TEMPERATURE_CHECK
	if(adapter_status != O2_CHARGER_BATTERY)
	    temp_adjust_setting_extern(data->batt_info.batt_temp);
#endif
	//bluewhale_get_charging_status_extern();


//---------------------------------------------
#elif defined MTK_MACH_SUPPORT
	pr_err("BMT_status.charger_exist: %d \n",BMT_status.charger_exist);
	if(BMT_status.charger_exist == KAL_TRUE)
	{
        if ( (BMT_status.charger_type == NONSTANDARD_CHARGER) || 
			 (BMT_status.charger_type == STANDARD_CHARGER) )
			adapter_status = O2_CHARGER_AC;
		else if ( (BMT_status.charger_type == STANDARD_HOST) ||
                  (BMT_status.charger_type == CHARGING_HOST) )
			adapter_status = O2_CHARGER_USB;
	}
	else
	{
		adapter_status = O2_CHARGER_BATTERY;
	}

//---------------------------------------------	
#elif defined QUALCOMM_MACH_SUPPORT
//---------------------------------------------

#endif

	pr_err("adapter_status:%d\n", adapter_status);

#if defined (O2_CHARGER_SUPPORT) && !defined(TQ210_TEST)

	if(adapter_status != O2_CHARGER_BATTERY)
	{
		bluewhale_get_charging_status_extern();
#ifdef TEMPERATURE_CHECK
	    temp_adjust_setting_extern(data->batt_info.batt_temp);
#endif
	}
#endif
}


static void oz8806_battery_work(struct work_struct *work)
{
	struct oz8806_data *data = container_of(work, struct oz8806_data, work.work);
	union power_supply_propval val;

	unsigned long time_since_last_update_ms;
	static unsigned long cur_jiffies;

	//pr_err("%s: !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", __func__);
	time_since_last_update_ms = jiffies_to_msecs(jiffies - cur_jiffies);
	cur_jiffies = jiffies;

	if (create_sys)
		create_sys = get_battery_psy();

	system_charge_discharge_status(data);

	rsoc_pre = data->batt_info.batt_soc;

	//you must add this code here for O2MICRO
	//Notice: don't nest mutex
	mutex_lock(&update_mutex);

	if (bmu_polling_loop_callback)
		bmu_polling_loop_callback();

	oz8806_update_batt_info(data);

	charge_end_fun();

	if(adapter_status == O2_CHARGER_BATTERY)
		discharge_end_fun(data);

	mutex_unlock(&update_mutex);

	if (!capacity_init_ok && bmu_init_done) {
#if  defined (MTK_MACH_SUPPORT)
		//wake_up_bat_bmu(); //don't use wake_up_dat(), this is for charger
#endif
		do {
			msleep(500);
			if (battery_psy 
				&& !battery_psy->get_property (battery_psy, POWER_SUPPLY_PROP_CAPACITY, &val))
			{
				if (val.intval == data->batt_info.batt_soc)
					capacity_init_ok = 1;
			}
			else
			{
				pr_err(KERN_ERR"fail to get val of POWER_SUPPLY_PROP_CAPACITY\n");
				break;
			}
		} while (!capacity_init_ok);
	}


	if(adapter_status == O2_CHARGER_AC)
	{	if(!wake_lock_active(&battery_wake_lock))
		{
			wake_lock(&battery_wake_lock);
		}
	}
	else
	{
		if(wake_lock_active(&battery_wake_lock))
		{
			wake_unlock(&battery_wake_lock);
		}

	}
	
	batt_dbg("l=%d v=%d t=%d c=%d ch=%d",
			data->batt_info.batt_soc, data->batt_info.batt_voltage, 
			data->batt_info.batt_temp, data->batt_info.batt_current, adapter_status);

#ifdef O2_GAUGE_POWER_SUPPOLY_SUPPORT
	power_supply_changed(&data->bat);
#endif
	
	/* reschedule for the next time */
#ifdef O2_GAUGE_WORK_SUPPORT
	if (!bmu_init_done) 
	{
		data->interval = 2000;
	} 
	else 
	{
		data->interval = BATTERY_WORK_INTERVAL * 1000;
	}
#endif

	schedule_delayed_work(&data->work, msecs_to_jiffies(data->interval));

	batt_dbg("since last batt update = %lu ms, interval:%d ms",
			  time_since_last_update_ms, data->interval);
}

static int oz8806_suspend_notifier(struct notifier_block *nb,
				unsigned long event,
				void *dummy)
{
	struct oz8806_data *data = container_of(nb, struct oz8806_data, pm_nb);
	
	switch (event) {
	case PM_SUSPEND_PREPARE:
		pr_err("oz88106 PM_SUSPEND_PREPARE \n");
#ifdef O2_GAUGE_WORK_SUPPORT
		cancel_delayed_work_sync(&data->work);
#endif
			
		system_charge_discharge_status(data);
		return NOTIFY_OK;
	case PM_POST_SUSPEND:
        pr_err("oz88106 PM_POST_SUSPEND \n");

		mutex_lock(&update_mutex);
		// if AC charge can't wake up every 1 min,you must remove the if.
		if(adapter_status == O2_CHARGER_BATTERY)
			if (bmu_wake_up_chip_callback)
				bmu_wake_up_chip_callback();

		oz8806_update_batt_info(data);
		mutex_unlock(&update_mutex);


#ifdef O2_GAUGE_WORK_SUPPORT	
		schedule_delayed_work(&data->work, 0);
#endif

		//this code must be here,very carefull.
		if(adapter_status == O2_CHARGER_BATTERY)
		{
			if(data->batt_info.batt_current >= data->batt_info.discharge_current_th)
			{
				if (batt_info_ptr) batt_info_ptr->fCurr = -20;

				data->batt_info.batt_current = -20;
				pr_err("drop current\n");
			}
		}
		
		return NOTIFY_OK;
	default:
		return NOTIFY_DONE;
	}

}

static int oz8806_init_batt_info(struct oz8806_data *data)
{
	data->batt_info.batt_soc = 50; 
	data->batt_info.batt_voltage = 3999;
	data->batt_info.batt_current = -300;
	data->batt_info.batt_temp = 27;
	data->batt_info.batt_capacity = 4000;

	data->batt_info.batt_fcc_data = config_data.design_capacity;
	data->batt_info.discharge_current_th = DISCH_CURRENT_TH;
	data->batt_info.charge_end = 0;

	return 0;
}
static int oz8806_update_batt_info(struct oz8806_data *data)
{
	int ret = 0;

	//Notice: before call this function, use mutex_lock(&update_mutex)
	//Notice: don't nest mutex
	if (!batt_info_ptr || !gas_gauge_ptr)
	{
		ret = -EINVAL;
		goto end;
	}

	data->batt_info.batt_soc = batt_info_ptr->fRSOC; 
	data->batt_info.batt_voltage = batt_info_ptr->fVolt;
	data->batt_info.batt_current = batt_info_ptr->fCurr;
	data->batt_info.batt_temp = batt_info_ptr->fCellTemp;
	data->batt_info.batt_capacity = batt_info_ptr->sCaMAH;

	data->batt_info.charge_end = gas_gauge_ptr->charge_end;

#ifdef INTEL_MACH
	data->batt_info.batt_temp = 27;
#endif
#if defined (TQ210_TEST)  
	data->batt_info.batt_temp = 27;

#elif  defined (MTK_MACH_SUPPORT)
	data->batt_info.batt_temp = BMT_status.temperature;

#elif defined (QUALCOMM_MACH_SUPPORT)
	data->batt_info.batt_temp = ROUND_SINGLE_DIGITS(oz8806_battery_get_temp(data));

#elif defined (XXX_MACH_SUPPORT)
	data->batt_info.batt_temp = XXX;
	batt_info_ptr->fCellTemp = batt_info_ptr->fCellTemp;
#endif

end:
	return ret;
}
int32_t oz8806_vbus_voltage(void)
{
	int32_t vubs_voltage = 0;

	if (oz8806_temp_read_callback)
		oz8806_temp_read_callback(&vubs_voltage);

	batt_dbg("voltage from oz8806:%d", vubs_voltage);
	vubs_voltage =  vubs_voltage * (RPULL + RDOWN) / RDOWN;

	return vubs_voltage;
}
EXPORT_SYMBOL(oz8806_vbus_voltage);

int32_t oz8806_get_init_status(void)
{
	return bmu_init_done;
}
EXPORT_SYMBOL(oz8806_get_init_status);
	

struct i2c_client * oz8806_get_client(void)
{
	if (the_oz8806)
	return the_oz8806->myclient;
	else
	{
		pr_err("the_oz8806 is NULL, oz8806_probe didn't call\n");
		return NULL;
	}
}
EXPORT_SYMBOL(oz8806_get_client);

unsigned long oz8806_get_kernel_memaddr(void)
{
	return kernel_memaddr;
}
EXPORT_SYMBOL(oz8806_get_kernel_memaddr);

int8_t get_adapter_status(void)
{
	return adapter_status;
}
EXPORT_SYMBOL(get_adapter_status);

void oz8806_register_bmu_callback(void *bmu_polling_loop_func,
		void *bmu_wake_up_chip_func,
		void *bmu_power_down_chip_func,
		void *charge_end_process_func,
		void *discharge_end_process_func,
		void *oz8806_temp_read_func
		)
{
	mutex_lock(&update_mutex);

	bmu_polling_loop_callback = bmu_polling_loop_func;
	bmu_wake_up_chip_callback = bmu_wake_up_chip_func;
	bmu_power_down_chip_callback = bmu_power_down_chip_func;
	bmu_charge_end_process_callback = charge_end_process_func;
	bmu_discharge_end_process_callback = discharge_end_process_func;
	oz8806_temp_read_callback = oz8806_temp_read_func;

	if(bmu_polling_loop_callback)
	{
		bmu_polling_loop_callback();
		schedule_delayed_work(&the_oz8806->work, 0);
	}

	mutex_unlock(&update_mutex);
}
EXPORT_SYMBOL(oz8806_register_bmu_callback);

void unregister_bmu_callback(void)
{
	mutex_lock(&update_mutex);

	bmu_polling_loop_callback = NULL;
	bmu_wake_up_chip_callback = NULL;
	bmu_power_down_chip_callback = NULL;
	bmu_charge_end_process_callback = NULL;
	bmu_discharge_end_process_callback = NULL;
	oz8806_temp_read_callback = NULL;

	oz8806_init_batt_info(the_oz8806);

	batt_info_ptr = NULL;
	gas_gauge_ptr = NULL;

	mutex_unlock(&update_mutex);
}
EXPORT_SYMBOL(unregister_bmu_callback);

void oz8806_set_batt_info_ptr(bmu_data_t  *batt_info)
{
	mutex_lock(&update_mutex);

	batt_info_ptr = batt_info;

	mutex_unlock(&update_mutex);
}
EXPORT_SYMBOL(oz8806_set_batt_info_ptr);

void oz8806_set_gas_gauge(gas_gauge_t *gas_gauge)
{
	mutex_lock(&update_mutex);
	gas_gauge_ptr = gas_gauge;
	if (gas_gauge_ptr->fcc_data != 0)
	the_oz8806->batt_info.batt_fcc_data = gas_gauge_ptr->fcc_data;
	if (gas_gauge_ptr->discharge_current_th != 0)
	the_oz8806->batt_info.discharge_current_th = gas_gauge_ptr->discharge_current_th;
	batt_dbg("batt_fcc_data:%d, discharge_current_th:%d", 
			the_oz8806->batt_info.batt_fcc_data, the_oz8806->batt_info.discharge_current_th);
	mutex_unlock(&update_mutex);
}
EXPORT_SYMBOL(oz8806_set_gas_gauge);

void oz8806_set_init_status(uint8_t st)
{
	bmu_init_done = st;
}
EXPORT_SYMBOL(oz8806_set_init_status);

void oz8806_set_charge_end_flag(uint8_t st)
{
	bmu_charge_end = st;
}
EXPORT_SYMBOL(oz8806_set_charge_end_flag);

uint8_t oz8806_get_ext_thermal(void)
{
	return ext_thermal_read;
}
EXPORT_SYMBOL(oz8806_get_ext_thermal);

uint8_t oz8806_get_save_capacity(void)
{
	return save_capacity;
}
EXPORT_SYMBOL(oz8806_get_save_capacity);

static int oz8806_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	int ret;
	struct oz8806_data *data;

	data = devm_kzalloc(&client->dev, sizeof(struct oz8806_data), GFP_KERNEL);
	if (!data) {
		dev_err(&client->dev, "Can't alloc oz8806_data struct\n");
		return -ENOMEM;
	}

	// Init real i2c_client 
	i2c_set_clientdata(client, data);

	the_oz8806 = data;
	data->myclient = client;
	data->myclient->addr = 0x2f;
	data->interval = BATTERY_WORK_INTERVAL * 1000;

	//init battery information
	oz8806_init_batt_info(data);

#ifdef O2_GAUGE_POWER_SUPPOLY_SUPPORT 
	oz8806_powersupply_init(data);
	
	ret = power_supply_register(&client->dev, &the_oz8806->bat);
	
	if (ret) {
		pr_err(KERN_ERR "failed to register power_supply battery\n");
		goto register_fail;
	}
#endif

	/*
	 * /sys/class/i2c-dev/i2c-2/device/2-002f/
	 */
	ret = oz8806_create_sys(&(client->dev), &oz8806_attribute_group);
	if(ret){
		pr_err(KERN_ERR"[BATT]: Err failed to creat oz8806 attributes\n");
		goto bat_failed;
	}

	pr_err("oz8806 oz8806_probe 2 \n");
	
   //alternative suspend/resume method
	data->pm_nb.notifier_call = oz8806_suspend_notifier;
	register_pm_notifier(&data->pm_nb);

	wake_lock_init(&battery_wake_lock, WAKE_LOCK_SUSPEND, "oz8806_wake");

#ifdef EXT_THERMAL_READ
	ext_thermal_read = 1;
#else
	ext_thermal_read = 0;
#endif

#ifdef O2_GAUGE_WORK_SUPPORT
	INIT_DELAYED_WORK(&data->work, oz8806_battery_work);
	schedule_work(&data->work.work);

#endif


	return 0;					//return Ok
bat_failed:
#ifdef O2_GAUGE_POWER_SUPPOLY_SUPPORT 
	power_supply_unregister(&the_oz8806->bat);
#endif
register_fail:
	pr_err(KERN_ERR "%s() fail: return %d\n", __func__, ret);
	the_oz8806 = NULL;
	return ret;
}


static int oz8806_remove(struct i2c_client *client)
{
	struct oz8806_data *data = i2c_get_clientdata(client);

#ifdef O2_GAUGE_WORK_SUPPORT
	cancel_delayed_work(&data->work);
#endif

	if (battery_psy && !create_sys)
		sysfs_remove_group(&(battery_psy->dev->kobj), &battery_psy_attribute_group);

	sysfs_remove_group(&(data->myclient->dev.kobj), &oz8806_attribute_group);

#ifdef O2_GAUGE_POWER_SUPPOLY_SUPPORT 
	power_supply_unregister(&data->bat);
#endif

	mutex_lock(&update_mutex);

	if (bmu_power_down_chip_callback)
		bmu_power_down_chip_callback();

	oz8806_update_batt_info(data);

	mutex_unlock(&update_mutex);

	return 0;
}

static void oz8806_shutdown(struct i2c_client *client)
{
	struct oz8806_data *data = i2c_get_clientdata(client);
	pr_err("oz8806 shutdown\n");

#ifdef O2_GAUGE_WORK_SUPPORT
	cancel_delayed_work(&data->work);
#endif

	if (battery_psy && !create_sys)
		sysfs_remove_group(&(battery_psy->dev->kobj), &battery_psy_attribute_group);

	sysfs_remove_group(&(data->myclient->dev.kobj), &oz8806_attribute_group);

#ifdef O2_GAUGE_POWER_SUPPOLY_SUPPORT 
	power_supply_unregister(&data->bat);
#endif
	mutex_lock(&update_mutex);
	if (bmu_power_down_chip_callback)
		bmu_power_down_chip_callback();

	oz8806_update_batt_info(data);
	mutex_unlock(&update_mutex);
}
/*-------------------------------------------------------------------------*/

static const struct i2c_device_id oz8806_id[] = {
	{"EXCG0000", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, oz8806_id);

#ifdef CONFIG_ACPI
static struct acpi_device_id extgauge_acpi_match[] = {
        {"EXFG0000", 0 },
        {}
};
MODULE_DEVICE_TABLE(acpi, extgauge_acpi_match);
#endif

static struct i2c_driver oz8806_driver = {
	.driver = {
		.name	= MYDRIVER,
#ifdef CONFIG_ACPI
		.acpi_match_table = ACPI_PTR(extgauge_acpi_match),
#endif
	},
	.probe			= oz8806_probe,
	//.resume			= oz8806_resume,
	.remove         = oz8806_remove,
	.shutdown		= oz8806_shutdown,
	.id_table		= oz8806_id,
};

/*-------------------------------------------------------------------------*/

static struct i2c_board_info __initdata i2c_oz8806={ I2C_BOARD_INFO("oz8806", 0x2f)};

static int __init oz8806_init(void)
{

	struct i2c_adapter *i2c_adap;
	static struct i2c_client *oz8806_client;

	pr_err("%s\n", __func__);
	i2c_adap = i2c_get_adapter(OZ88106_I2C_NUM);
	oz8806_client = i2c_new_device(i2c_adap, &i2c_oz8806);
	i2c_put_adapter(i2c_adap);
	
	return i2c_add_driver(&oz8806_driver);
}

static void __exit oz8806_exit(void)
{
	pr_err("%s\n", __func__);
	i2c_del_driver(&oz8806_driver);
}

/*-------------------------------------------------------------------------*/

MODULE_DESCRIPTION("oz8806 Battery Monitor IC Driver");
MODULE_LICENSE("GPL");

//subsys_initcall_sync(oz8806_init);
module_init(oz8806_init);
module_exit(oz8806_exit);

module_init(mm_init);
module_exit(mm_close);

module_init(dev_init);
module_exit(dev_exit);


