/* -------------------------------------------------------------------------
 * Copyright (C) 2014-2015, Intel Corporation
 *
 * Derived from:
 *  gslX68X.c
 *  Copyright (C) 2010-2015, Shanghai Sileadinc Co.Ltd
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * ------------------------------------------------------------------------- */

#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/acpi.h>
#include <linux/interrupt.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/pm.h>
#include <linux/irq.h>

#define SILEAD_TS_NAME "silead_ts"

#define SILEAD_REG_RESET	0xE0
#define SILEAD_REG_DATA		0x80
#define SILEAD_REG_TOUCH_NR	0x80
#define SILEAD_REG_POWER	0xBC
#define SILEAD_REG_CLOCK	0xE4
#define SILEAD_REG_STATUS	0xB0
#define SILEAD_REG_ID		0xFC
#define SILEAD_REG_MEM_CHECK	0xB0

#define SILEAD_STATUS_OK	0x5A5A5A5A
#define SILEAD_TS_DATA_LEN	44
#define SILEAD_CLOCK		0x04

#define SILEAD_CMD_RESET	0x88
#define SILEAD_CMD_START	0x00

#define SILEAD_POINT_DATA_LEN	0x04
#define SILEAD_POINT_X_OFF	0x02
#define SILEAD_POINT_ID_OFF	0x03
#define SILEAD_X_HSB_MASK	0xF0
#define SILEAD_TOUCH_ID_MASK	0x0F

#define SILEAD_DP_X_MAX		"resolution-x"
#define SILEAD_DP_Y_MAX		"resolution-y"
#define SILEAD_DP_MAX_FINGERS	"max-fingers"
#define SILEAD_DP_FW_NAME	"fw-name"
#define SILEAD_PWR_GPIO_NAME	"power"

#define SILEAD_CMD_SLEEP_MIN	10000
#define SILEAD_CMD_SLEEP_MAX	20000
#define SILEAD_POWER_SLEEP	20
#define SILEAD_STARTUP_SLEEP	30

enum silead_ts_power {
	SILEAD_POWER_ON  = 1,
	SILEAD_POWER_OFF = 0
};

struct silead_ts_data {
	struct i2c_client *client;
	struct gpio_desc *gpio_power;
	struct input_dev *input;
	const char *custom_fw_name;
	char fw_name[I2C_NAME_SIZE];
	u16 x_max;
	u16 y_max;
	u8 max_fingers;
	u32 chip_id;
};

struct silead_fw_data {
	u32 offset;
	u32 val;
};

static int silead_ts_request_input_dev(struct silead_ts_data *data)
{
	struct device *dev = &data->client->dev;
	int ret;

	data->input = devm_input_allocate_device(dev);
	if (!data->input) {
		dev_err(dev,
			"Failed to allocate input device\n");
		return -ENOMEM;
	}

	input_set_capability(data->input, EV_ABS, ABS_X);
	input_set_capability(data->input, EV_ABS, ABS_Y);

	input_set_abs_params(data->input, ABS_MT_POSITION_X, 0,
			     data->x_max, 0, 0);
	input_set_abs_params(data->input, ABS_MT_POSITION_Y, 0,
			     data->y_max, 0, 0);

	input_mt_init_slots(data->input, data->max_fingers,
			    INPUT_MT_DIRECT | INPUT_MT_DROP_UNUSED);

	data->input->name = SILEAD_TS_NAME;
	data->input->phys = "input/ts";
	data->input->id.bustype = BUS_I2C;

	ret = input_register_device(data->input);
	if (ret) {
		dev_err(dev, "Failed to register input device: %d\n", ret);
		return ret;
	}

	return 0;
}

static void silead_ts_report_touch(struct silead_ts_data *data, u16 x, u16 y,
				   u8 id)
{
	input_mt_slot(data->input, id);
	input_mt_report_slot_state(data->input, MT_TOOL_FINGER, true);
	input_report_abs(data->input, ABS_MT_POSITION_X, x);
	input_report_abs(data->input, ABS_MT_POSITION_Y, y);
}

static void silead_ts_set_power(struct i2c_client *client,
				enum silead_ts_power state)
{
	struct silead_ts_data *data = i2c_get_clientdata(client);

	gpiod_set_value_cansleep(data->gpio_power, state);
	msleep(SILEAD_POWER_SLEEP);
}

static void silead_ts_read_data(struct i2c_client *client)
{
	struct silead_ts_data *data = i2c_get_clientdata(client);
	struct device *dev = &client->dev;
	u8 buf[SILEAD_TS_DATA_LEN];
	int x, y, id, touch_nr, ret, i, offset;

	ret = i2c_smbus_read_i2c_block_data(client, SILEAD_REG_DATA,
					    SILEAD_TS_DATA_LEN, buf);
	if (ret < 0) {
		dev_err(dev, "Data read error %d\n", ret);
		return;
	}

	touch_nr = buf[0];

	if (touch_nr < 0)
		return;

	dev_dbg(dev, "Touch number: %d\n", touch_nr);

	for (i = 1; i <= touch_nr; i++) {
		offset = i * SILEAD_POINT_DATA_LEN;

		/* The last 4 bits are the touch id */
		id = buf[offset + SILEAD_POINT_ID_OFF] & SILEAD_TOUCH_ID_MASK;

		/* The 1st 4 bits are part of X */
		buf[offset + SILEAD_POINT_ID_OFF] =
			(buf[offset + SILEAD_POINT_ID_OFF] & SILEAD_X_HSB_MASK)
			>> 4;

		y = le16_to_cpup((__le16 *)(buf + offset));
		x = le16_to_cpup((__le16 *)(buf + offset + SILEAD_POINT_X_OFF));

		dev_dbg(dev, "x=%d y=%d id=%d\n", x, y, id);
		silead_ts_report_touch(data, x, y, id);
	}

	input_mt_sync_frame(data->input);
	input_sync(data->input);
}

static int silead_ts_init(struct i2c_client *client)
{
	struct silead_ts_data *data = i2c_get_clientdata(client);
	int ret;

	ret = i2c_smbus_write_byte_data(client, SILEAD_REG_RESET,
					SILEAD_CMD_RESET);
	if (ret)
		goto i2c_write_err;
	usleep_range(SILEAD_CMD_SLEEP_MIN, SILEAD_CMD_SLEEP_MAX);

	ret = i2c_smbus_write_byte_data(client, SILEAD_REG_TOUCH_NR,
					data->max_fingers);
	if (ret)
		goto i2c_write_err;
	usleep_range(SILEAD_CMD_SLEEP_MIN, SILEAD_CMD_SLEEP_MAX);

	ret = i2c_smbus_write_byte_data(client, SILEAD_REG_CLOCK, SILEAD_CLOCK);
	if (ret)
		goto i2c_write_err;
	usleep_range(SILEAD_CMD_SLEEP_MIN, SILEAD_CMD_SLEEP_MAX);

	ret = i2c_smbus_write_byte_data(client, SILEAD_REG_RESET,
					SILEAD_CMD_START);
	if (ret)
		goto i2c_write_err;
	usleep_range(SILEAD_CMD_SLEEP_MIN, SILEAD_CMD_SLEEP_MAX);

	return 0;

i2c_write_err:
	dev_err(&client->dev, "Registers clear error %d\n", ret);
	return ret;
}

static int silead_ts_reset(struct i2c_client *client)
{
	int ret;

	ret = i2c_smbus_write_byte_data(client, SILEAD_REG_RESET,
					SILEAD_CMD_RESET);
	if (ret)
		goto i2c_write_err;
	usleep_range(SILEAD_CMD_SLEEP_MIN, SILEAD_CMD_SLEEP_MAX);

	ret = i2c_smbus_write_byte_data(client, SILEAD_REG_CLOCK, SILEAD_CLOCK);
	if (ret)
		goto i2c_write_err;
	usleep_range(SILEAD_CMD_SLEEP_MIN, SILEAD_CMD_SLEEP_MAX);

	ret = i2c_smbus_write_byte_data(client, SILEAD_REG_POWER,
					SILEAD_CMD_START);
	if (ret)
		goto i2c_write_err;
	usleep_range(SILEAD_CMD_SLEEP_MIN, SILEAD_CMD_SLEEP_MAX);

	return 0;

i2c_write_err:
	dev_err(&client->dev, "Chip reset error %d\n", ret);
	return ret;
}

static int silead_ts_startup(struct i2c_client *client)
{
	int ret;

	ret = i2c_smbus_write_byte_data(client, SILEAD_REG_RESET, 0x00);
	if (ret) {
		dev_err(&client->dev, "Startup error %d\n", ret);
		return ret;
	}
	msleep(SILEAD_STARTUP_SLEEP);

	return 0;
}

static int silead_ts_load_fw(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct silead_ts_data *data = i2c_get_clientdata(client);
	unsigned int fw_size, i;
	const struct firmware *fw;
	struct silead_fw_data *fw_data;
	int ret;

	dev_dbg(dev, "Firmware file name: %s", data->fw_name);

	if (data->custom_fw_name)
		ret = request_firmware(&fw, data->custom_fw_name, dev);
	else
		ret = request_firmware(&fw, data->fw_name, dev);

	if (ret) {
		dev_err(dev, "Firmware request error %d\n", ret);
		return ret;
	}

	fw_size = fw->size / sizeof(*fw_data);
	fw_data = (struct silead_fw_data *)fw->data;

	for (i = 0; i < fw_size; i++) {
		ret = i2c_smbus_write_i2c_block_data(client, fw_data[i].offset,
						     4, (u8 *)&fw_data[i].val);
		if (ret) {
			dev_err(dev, "Firmware load error %d\n", ret);
			goto release_fw_err;
		}
	}

	release_firmware(fw);
	return 0;

release_fw_err:
	release_firmware(fw);
	return ret;
}

static u32 silead_ts_get_status(struct i2c_client *client)
{
	int ret;
	u32 status;

	ret = i2c_smbus_read_i2c_block_data(client, SILEAD_REG_STATUS, 4,
					    (u8 *)&status);
	if (ret < 0) {
		dev_err(&client->dev, "Status read error %d\n", ret);
		return ret;
	}

	return status;
}

static int silead_ts_get_id(struct i2c_client *client)
{
	struct silead_ts_data *data = i2c_get_clientdata(client);
	int ret;

	ret = i2c_smbus_read_i2c_block_data(client, SILEAD_REG_ID, 4,
					    (u8 *)&data->chip_id);
	if (ret < 0) {
		dev_err(&client->dev, "Chip ID read error %d\n", ret);
		return ret;
	}

	return 0;
}

static int silead_ts_setup(struct i2c_client *client)
{
	struct silead_ts_data *data = i2c_get_clientdata(client);
	struct device *dev = &client->dev;
	int ret;
	u32 status;

	silead_ts_set_power(client, SILEAD_POWER_OFF);
	silead_ts_set_power(client, SILEAD_POWER_ON);

	ret = silead_ts_get_id(client);
	if (ret)
		return ret;
	dev_dbg(dev, "Chip ID: 0x%8X", data->chip_id);

	ret = silead_ts_init(client);
	if (ret)
		return ret;

	ret = silead_ts_reset(client);
	if (ret)
		return ret;

	ret = silead_ts_load_fw(client);
	if (ret)
		return ret;

	ret = silead_ts_startup(client);
	if (ret)
		return ret;

	status = silead_ts_get_status(client);
	if (status != SILEAD_STATUS_OK) {
		dev_err(dev, "Initialization error, status: 0x%X\n", status);
		return -ENODEV;
	}

	return 0;
}

static irqreturn_t silead_ts_threaded_irq_handler(int irq, void *id)
{
	struct silead_ts_data *data = (struct silead_ts_data *)id;
	struct i2c_client *client = data->client;

	silead_ts_read_data(client);

	return IRQ_HANDLED;
}

static int silead_ts_read_props(struct i2c_client *client)
{
	struct silead_ts_data *data = i2c_get_clientdata(client);
	struct device *dev = &client->dev;
	int ret;

	ret = device_property_read_u16(dev, SILEAD_DP_X_MAX, &data->x_max);
	if (ret) {
		dev_err(dev, "Resolution X read error %d\n", ret);
		goto error;
	}

	ret = device_property_read_u16(dev, SILEAD_DP_Y_MAX, &data->y_max);
	if (ret) {
		dev_err(dev, "Resolution Y read error %d\n", ret);
		goto error;
	}

	ret = device_property_read_u8(dev, SILEAD_DP_MAX_FINGERS,
				      &data->max_fingers);
	if (ret) {
		dev_err(dev, "Max fingers read error %d\n", ret);
		goto error;
	}

	ret = device_property_read_string(dev, SILEAD_DP_FW_NAME,
					  &data->custom_fw_name);
	if (ret)
		dev_info(dev, "Firmware file name read error. Using default.");

	dev_dbg(dev, "X max = %d, Y max = %d, max fingers = %d",
		data->x_max, data->y_max, data->max_fingers);

	return 0;

error:
	return ret;
}

#ifdef CONFIG_ACPI
static const struct acpi_device_id silead_ts_acpi_match[];

static int silead_ts_set_default_fw_name(struct silead_ts_data *data,
					 const struct i2c_device_id *id)
{
	const struct acpi_device_id *acpi_id;
	struct device *dev = &data->client->dev;
	int i;

	if (ACPI_HANDLE(dev)) {
		acpi_id = acpi_match_device(silead_ts_acpi_match, dev);
		if (!acpi_id)
			return -ENODEV;

		sprintf(data->fw_name, "%s.fw", acpi_id->id);

		for (i = 0; i < strlen(data->fw_name); i++)
			data->fw_name[i] = tolower(data->fw_name[i]);
	} else {
		sprintf(data->fw_name, "%s.fw", id->name);
	}

	return 0;
}
#else
static int silead_ts_set_default_fw_name(struct silead_ts_data *data,
					 const struct i2c_device_id *id)
{
	sprintf(data->fw_name, "%s.fw", id->name);
	return 0;
}
#endif

static int silead_ts_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct silead_ts_data *data;
	struct device *dev = &client->dev;
	int ret;

	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_I2C |
				     I2C_FUNC_SMBUS_READ_I2C_BLOCK |
				     I2C_FUNC_SMBUS_WRITE_I2C_BLOCK)) {
		dev_err(dev, "I2C functionality check failed\n");
		return -ENXIO;
	}

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	i2c_set_clientdata(client, data);
	data->client = client;

	ret = silead_ts_set_default_fw_name(data, id);
	if (ret)
		return ret;

	/* If the IRQ is not filled by DT or ACPI subsytem
	 * we can't continue without it */
	if (client->irq <= 0)
		return -ENODEV;

	/* Power GPIO pin */
	data->gpio_power = devm_gpiod_get(dev, SILEAD_PWR_GPIO_NAME,
					  GPIOD_OUT_LOW);
	if (IS_ERR(data->gpio_power)) {
		dev_err(dev, "Shutdown GPIO request failed\n");
		return PTR_ERR(data->gpio_power);
	}

	ret = silead_ts_read_props(client);
	if (ret)
		return ret;

	ret = silead_ts_setup(client);
	if (ret)
		return ret;

	ret = silead_ts_request_input_dev(data);
	if (ret)
		return ret;

	ret = devm_request_threaded_irq(dev, client->irq, NULL,
					silead_ts_threaded_irq_handler,
					IRQF_ONESHOT | IRQ_TYPE_EDGE_RISING,
					client->name, data);
	if (ret) {
		dev_err(dev, "IRQ request failed %d\n", ret);
		return ret;
	}

	dev_dbg(dev, "Probing succeded\n");
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int silead_ts_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);

	silead_ts_set_power(client, SILEAD_POWER_OFF);
	return 0;
}

static int silead_ts_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	int ret, status;

	silead_ts_set_power(client, SILEAD_POWER_ON);

	ret = silead_ts_reset(client);
	if (ret)
		return ret;

	ret = silead_ts_startup(client);
	if (ret)
		return ret;

	status = silead_ts_get_status(client);
	if (status != SILEAD_STATUS_OK) {
		dev_err(dev, "Resume error, status: 0x%X\n", status);
		return -ENODEV;
	}

	return 0;
}

static SIMPLE_DEV_PM_OPS(silead_ts_pm, silead_ts_suspend, silead_ts_resume);
#endif

static const struct i2c_device_id silead_ts_id[] = {
	{ "gsl1680", 0 },
	{ "gsl1688", 0 },
	{ "gsl3670", 0 },
	{ "gsl3675", 0 },
	{ "gsl3692", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, silead_ts_id);

#ifdef CONFIG_ACPI
static const struct acpi_device_id silead_ts_acpi_match[] = {
	{ "GSL1680", 0 },
	{ "GSL1688", 0 },
	{ "GSL3670", 0 },
	{ "GSL3675", 0 },
	{ "GSL3692", 0 },
	{ }
};
MODULE_DEVICE_TABLE(acpi, silead_ts_acpi_match);
#endif

static struct i2c_driver silead_ts_driver = {
	.probe = silead_ts_probe,
	.id_table = silead_ts_id,
	.driver = {
		.name = SILEAD_TS_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_ACPI
		.acpi_match_table = ACPI_PTR(silead_ts_acpi_match),
#endif
#ifdef CONFIG_PM_SLEEP
		.pm = &silead_ts_pm,
#endif
	},
};
module_i2c_driver(silead_ts_driver);

MODULE_AUTHOR("Robert Dolca <robert.dolca@intel.com>");
MODULE_DESCRIPTION("Silead I2C touchscreen driver");
MODULE_LICENSE("GPL");
