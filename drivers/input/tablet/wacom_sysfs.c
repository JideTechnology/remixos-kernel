#include "wacom.h"

#define CLASS_NAME "wacom_class"
#define DEVICE_NAME "wacom_emr"

static void store_data(u8 *data, const char *buf, unsigned long *size)
{
	memcpy((data + *size), buf, MAX_SYSFS_SIZE);
	*size += MAX_SYSFS_SIZE;
}

static ssize_t wacom_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct wacom_sysfs *wacom_sysfs = dev_get_drvdata(dev);
	unsigned long i = 0;
	int ret;
	u8 *data = wacom_sysfs->data;
	bool bRet = false;
	
	if (wacom_sysfs->storing_started) {
		if (count == MAX_SYSFS_SIZE) {
			store_data(data, buf, &wacom_sysfs->data_count);
			goto out;
		} else {
			if ((wacom_sysfs->data_count + count) > MAX_DATASIZE) {
				printk("Data size exceeds the limit for the firmware \n");
				wacom_sysfs->storing_started = false;
				wacom_sysfs->data_count = 0;
				goto err;
			}

			store_data(data, buf, &wacom_sysfs->data_count);
			wacom_sysfs->storing_started = false;
			wacom_sysfs->data_count = 0;

		}
	} else {

		switch (buf[0]) {
		case 0:
			printk("flash started: %d \n", count);
			wacom_sysfs->storing_started = true;
			wacom_sysfs->data_count = 0;

			/*2014/10/14 check carefully if memcpy works as expected here*/
			memcpy(data, buf, MAX_SYSFS_SIZE);
			wacom_sysfs->data_count += MAX_SYSFS_SIZE;

			goto out;

		case 1:
			printk("Entering wacom boot started \n");
			ret = wacom_enter(wacom_sysfs, 0);

			break;

		case 2:
			
			if (counter >= 0)
				for (i = 0; i < counter; i++)
					if (wacom_sysfs->pid[i] == 0x94) {
						printk("Starting flash \n");
						ret = wacom_flash(wacom_sysfs, i, data);
						if (ret < 0) {
							goto err;
						}
						
						goto out;
					}
			goto nodev;
			
		case 3:
			printk("Exiting boot mode \n");
			bRet = wacom_out(wacom_sysfs, 0);
			if (!bRet) {
				goto err;
			}

			break;


		default:
			printk("No operation is registered for this \n");
			goto out;

		}
	}	
			



 out:
	return count;
	
 err:
	return -1;

 nodev:
	return -NOBOOTDEV;
}

static DEVICE_ATTR(flash, 0666, NULL, wacom_write);

static struct attribute *wacom_attributes[] = {
	&dev_attr_flash.attr,
	NULL,
};

static struct attribute_group wacom_attr_group = {
	.attrs = wacom_attributes,
};

int register_sysfs(struct wacom_sysfs *wacom)
{
	int error;
	
	/*Below added for sysfs*/
	/*If everything done without any failure, register sysfs*/
	wacom->dev_t = MKDEV(66, 0);
	wacom->class = class_create(THIS_MODULE, CLASS_NAME);
	wacom->dev = device_create(wacom->class, NULL, wacom->dev_t, NULL, DEVICE_NAME);
	if (IS_ERR(&wacom->dev)) {
		dev_err(wacom->dev,
			"Failed to create device, \"wac_i2c\"\n");
		goto err_create_class;
	}
	
	dev_set_drvdata(wacom->dev, wacom);
	
	error = sysfs_create_group(&wacom->dev->kobj, &wacom_attr_group);
	if (error) {
		dev_err(wacom->dev,
			"Failed to create sysfs group, \"wacom_attr_group\": (%d) \n", error);
		goto err_create_sysfs;
	}
	
	return 0;
	
 err_create_sysfs:
	sysfs_remove_group(&wacom->dev->kobj, &wacom_attr_group);
 err_create_class:
	device_destroy(wacom->class, wacom->dev_t);
	class_destroy(wacom->class);
	
	return -ERR_REGISTER_SYSFS;
}

void remove_sysfs(struct wacom_sysfs *wacom)
{
	/*Clear sysfs resources*/
	sysfs_remove_group(&wacom->dev->kobj, &wacom_attr_group);
	device_destroy(wacom->class, wacom->dev_t);
	class_destroy(wacom->class);
}
