#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/i2c.h>
#include <linux/ctype.h>

static int i2c_id = -1;
static int i2c_addr = -1;
static int i2c_start = -1;
static int i2c_end = -1;

static ssize_t i2c_show_property(struct device *dev,
		struct device_attribute *attr,
		char *buf);
static ssize_t i2c_store_property(struct device *dev,
		struct device_attribute * attr,
		const char *buf, size_t count);

static int i2c_writes(int id, int addr, char *buf, int bytes)
{
	struct i2c_adapter *adap;
	struct i2c_msg msg;
	int ret;

	if ((id < 0) || (addr < 0)) {
		printk("%s: Please set i2c id and addr.\n",__func__);
		return -EINVAL;
	}

	adap = i2c_get_adapter(id);
	if (!adap)
		return -EINVAL;
	msg.addr = addr;
	msg.flags = 0;
	msg.len = bytes;
	msg.buf = buf;
	/*msg.scl_rate = 100 * 1000;*/
	/*msg.udelay = 5;*/

	ret = i2c_transfer(adap, &msg, 1);
	i2c_put_adapter(adap);
	return (ret == 1) ? bytes: ret;
}

static int i2c_reads(int id, int addr, char *buf, int bytes)
{
	struct i2c_adapter *adap;
	struct i2c_msg msg[2];
	int ret;
	char reg = buf[0];

	if ((id < 0) || (addr < 0)) {
		printk("%s: Please set i2c id and addr.\n",__func__);
		return -EINVAL;
	}

	adap = i2c_get_adapter(id);
	if (!adap)
		return -EINVAL;
	msg[0].addr = addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &reg;
	/*msg[0].scl_rate = 100 * 1000;*/
	/*msg[0].udelay = 5;*/

	msg[1].addr = addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = bytes;
	msg[1].buf = buf;
	/*msg[1].scl_rate = 100 * 1000;*/
	/*msg[1].udelay = 5;*/

	ret = i2c_transfer(adap, msg, 2);
	i2c_put_adapter(adap);
	return (ret == 1) ? bytes: ret;
}


#define I2C_ATTR(_name,_mode)							\
{										\
	.attr	= { .name = #_name, .mode = _mode },	\
	.show	= i2c_show_property,						\
	.store	= i2c_store_property,						\
}

static struct device_attribute i2c_attrs[] = {
	I2C_ATTR(id,	S_IRUGO | S_IWUGO),
	I2C_ATTR(addr,	S_IRUGO | S_IWUGO),
	I2C_ATTR(write,	S_IWUGO),
	I2C_ATTR(read,	S_IRUGO),
	I2C_ATTR(regs,	S_IRUGO | S_IWUGO),
};

enum {
	I2C_ID = 0,
	I2C_ADDR,
	I2C_WRITE,
	I2C_READ,
	I2C_REGS,
};

static int i2c_create_attrs(struct device *dev)
{
	int i, rc;

	for (i=0; i<ARRAY_SIZE(i2c_attrs); i++) {
		rc = device_create_file(dev, &i2c_attrs[i]);
		if (rc)
			goto i2c_attrs_failed;
	}
	goto succeed;

i2c_attrs_failed:
	while (i--)
		device_remove_file(dev,&i2c_attrs[i]);
succeed:
	return rc;
}
static int i2c_remove_attrs(struct device *dev)
{
	int i;

	for (i=0; i<ARRAY_SIZE(i2c_attrs); i++) {
		device_remove_file(dev,&i2c_attrs[i]);
	}
	return 0;
}

static char read_regaddr;
static ssize_t i2c_show_property(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	int i = 0, ret, reg;
	char data[1];
	const ptrdiff_t off = attr - i2c_attrs;

	switch (off) {
	case I2C_ID:
		i += scnprintf(buf+i, PAGE_SIZE-i, "0x%02x\n", i2c_id);
		break;
	case I2C_ADDR:
		i += scnprintf(buf+i, PAGE_SIZE-i, "0x%02x\n", i2c_addr);
		break;
	case I2C_READ:
		data[0] = read_regaddr;
		ret = i2c_reads(i2c_id, i2c_addr, data, 1);
		if (ret < 0) {
			printk("Error happen while read.\n");
			return -ENXIO;
		}
		i += scnprintf(buf+i, PAGE_SIZE-i, "0x%02x\n", data[0]);
		break;
	case I2C_REGS:
		if (i2c_start != -1 && i2c_end != -1)
			for (reg = i2c_start; reg <= i2c_end; reg++) {
				data[0] = reg;
				ret = i2c_reads(i2c_id, i2c_addr, data, 1);
				if (ret < 0) {
					printk("Error happen while read.\n");
					return -ENXIO;
				}
				i += scnprintf(buf+i, PAGE_SIZE-i, "reg[0x%02x]=0x%02x\n", reg, data[0]);
			}
		break;
	default:
		return -EINVAL;
	}

	return i;
}

static ssize_t i2c_store_property(struct device *dev,
		struct device_attribute * attr,
		const char *buf, size_t size)
{
	int i, ret = 0, bytes;
	char *data, *after, *poiter;
	size_t count = 0;
	const ptrdiff_t off = attr - i2c_attrs;

	data = kzalloc((size + 1) / 2, GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	poiter = data;

	while (count < size) {
		(*poiter) = (char)simple_strtol(buf, &after, 16);
		if (after == buf)
			return -EINVAL;
		poiter++;
		count += after - buf;
		if (*after && isspace(*after))
			count++;
		buf = after + 1;
	}
	bytes = poiter - data;
	printk("Data:");
	for (i=0; i<bytes; i++) {
		printk("0x%02x ", data[i]);
	}
	printk("\n");

	switch (off) {
	case I2C_ID:
	{
		struct i2c_adapter *adap;
		if (bytes != 1) {
			printk("ID not supported.\n");
			return -EINVAL;
		}
		adap = i2c_get_adapter(data[0]);
		if (!adap) {
			printk("I2C bus not exist.\n");
			return -EINVAL;
		}
		i2c_put_adapter(adap);
		i2c_id = data[0];
		break;
	}
	case I2C_ADDR:
		if (bytes != 1) {
			printk("Only 7-bits address supported.\n");
			return -EINVAL;
		}
		i2c_addr = data[0];
		break;
	case I2C_WRITE:
		if (bytes == 1)
			read_regaddr = data[0];
		else
        {
            ret = i2c_writes(i2c_id, i2c_addr, data, bytes);
            if (ret < 0) return -ENXIO;
        }
		break;
	case I2C_REGS:
		if (bytes == 2) {
			i2c_start = data[0];
			i2c_end = data[1];
		}
		break;
	default:
		return -EINVAL;
	}

	kfree(data);
	return count;
}

static struct miscdevice i2c_misdev = {
	.name	= "i2c-test",
	.minor	= MISC_DYNAMIC_MINOR,
};

static int __init i2c_init(void)
{
	int ret;

	printk("i2c-test initialization.\n");
	ret = misc_register(&i2c_misdev);
	if (ret < 0) {
		printk("Failed to register i2c-test.\n");
		return ret;
	}

	ret = i2c_create_attrs(i2c_misdev.this_device);
	if (ret) {
		printk("Failed to create i2c-test attrs.\n");
		return ret;
	}
	return 0;
}

static void __exit i2c_exit(void)
{
	i2c_remove_attrs(i2c_misdev.this_device);
	misc_deregister(&i2c_misdev);
}

module_init(i2c_init);
module_exit(i2c_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("liangxiaoju");
