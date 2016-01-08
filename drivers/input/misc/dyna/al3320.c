
#include "light.h"
#ifdef CONFIG_ACPI
#include <linux/acpi.h>
#endif
static const struct light_reg_s reg = {
	.sys_cfg		= 0x00,
	.flg_sts		= 0x01,
	.int_cfg		= 0x02,
	.waiting		= 0x06,
	.gain_cfg		= 0x07,
	.persist		= 0x08,
	.mean_time		= 0x09,
	.dummy			= 0x0A,
	.raw_l			= 0x22,
	.raw_h			= 0x23,
	.raw_data		= 0x22,
	.low_thres_l		= 0x30,
	.low_thres_h		= 0x31,
	.high_thres_l		= 0x32,
	.high_thres_h		= 0x33,
	.calibration		= 0x34,
	.addr_bound		= 0x34,
};

static const struct light_reg_command_s command = {
				  //mask, lsb, max
	.sw_rst			= { ~0x04, 2, 0x01 },
	.active			= { ~0x01, 0, 0x01 },
	.int_flg		= { ~0x01, 0, 0x01 },
	.int_en			= { ~0x08, 3, 0x01 },
	.suspend		= { ~0x04, 2, 0x01 },
	.clear			= NOT_EXIST_COMMAND,
	.waiting		= { ~0xFF, 0, 0xFF },
	.gain			= { ~0x06, 1, 0x03 },
	.ext_gain		= { ~0x01, 0, 0x01 },
	.persist		= { ~0x3F, 0, 0x3F },
	.mean_time		= { ~0x0F, 0, 0x0F },
	.dummy			= { ~0xFF, 0, 0xFF },
};

int range[2][4] = { {100000,25000,6250,1950}, {33280,8320,2080,650} };

const struct light_reg_s* get_light_reg(void)
{
	return &reg;
}

const struct light_reg_command_s* get_light_reg_command(void)
{
	return &command;
}

int get_light_range(const struct light_data* const data)
{	
	if((data->ext_gain <= command.ext_gain.max) && (data->gain <= command.gain.max))
		return range[data->ext_gain][data->gain];
	else
		return -1;
}


/////////////////////////////////////////////////////////////////////

#define AL3320_DRV_NAME			"al3320"
#define DRIVER_VERSION			"2.0"

#define FORM_REG_VALUE(temp, value, reg_cmd) { temp &= reg_cmd.mask; temp |= (value<<reg_cmd.lsb); }

static int chip_sw_reset(struct i2c_client *client)
{
	u8 temp=0;

	FORM_REG_VALUE(temp, 1, command.sw_rst);
	if (i2c_single_write(client, reg.sys_cfg, temp))
		return -1;
	return 0;
}

static int al3320_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	printk("%s\n", __func__);
	chip_sw_reset(client);
	light_probe(client, id);
	return 0;
}


static int al3320_remove(struct i2c_client *client)
{
	printk("%s\n", __func__);
	light_remove(client);
	return 0;
}

static const struct i2c_device_id al3320_id[] = {
    { AL3320_DRV_NAME, 0 },
    {}
};

MODULE_DEVICE_TABLE(i2c, al3320_id);

#ifdef CONFIG_ACPI
static const struct acpi_device_id als_acpi_match[] = {
	{"DAL3320B",0},
	{ }
};
#endif
static struct i2c_driver al3320_driver = {
	.driver = {
		.name	= AL3320_DRV_NAME,
		.owner	= THIS_MODULE,
#ifdef CONFIG_ACPI
		.acpi_match_table = ACPI_PTR(als_acpi_match),
#endif
	},
	.probe	= al3320_probe,
	.remove	= al3320_remove,
	.id_table = al3320_id,
};

static int __init al3320_init(void)
{
	printk("%s\n", __func__);
	return i2c_add_driver(&al3320_driver);
}

static void __exit al3320_exit(void)
{
	printk("%s\n", __func__);
	i2c_del_driver(&al3320_driver);
}

MODULE_AUTHOR("Dyna-Image");
MODULE_DESCRIPTION("AL3320 driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(DRIVER_VERSION);

module_init(al3320_init);
module_exit(al3320_exit);

