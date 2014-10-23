/*
 * drivers/video/tegra/dc/dsi2lvds.c
 *
 * Copyright (c) 2012, NVIDIA Corporation.
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

#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/sn65dsi83.h>
#include <mach/dc.h>

#include "dc_priv.h"
#include "dsi2lvds.h"
#include "dsi.h"

static struct tegra_dc_dsi2lvds_data *dsi2lvds;

#define BIRDGE_TO_1280X800 0x0001
#define BIRDGE_TO_1920x1200 0x0002
#define BIRDGE_TO_TEST 0x0004

#if defined(CONFIG_MACH_TERRA10)
#define  Foxconn_SN65DSI85_Bridge BIRDGE_TO_1920x1200
#else
#define  Foxconn_SN65DSI85_Bridge BIRDGE_TO_1280X800
#endif
#define foxconn_Display_Debug 0

enum i2c_transfer_type {
	I2C_WRITE,
	I2C_READ,
};

#if Foxconn_SN65DSI85_Bridge
#define DSI2LVDS_TEGRA_I2C_BUS	0
#else
#define DSI2LVDS_TEGRA_I2C_BUS	3
#endif

#define DSI2LVDS_REG_VAL(addr, val)	{(addr), (val)}
#define BRIDGE_TABLE_END 0xFF

static u8 dsi2lvds_disable_config_clk[][2] = {
	DSI2LVDS_REG_VAL(0x0d, 0x00), /* pLL disable */
};

static u8 dsi2lvds_config_clk[][2] = {
	DSI2LVDS_REG_VAL(0x0d, 0x00), /* pLL disable */
#if (Foxconn_SN65DSI85_Bridge == BIRDGE_TO_1280X800)
	DSI2LVDS_REG_VAL(0x0a,0x05), /* configure PLL *///lvds clk range 62.5< rclk <87.5
	DSI2LVDS_REG_VAL(0x0b,0x10), /* configure PLL *///dsi clk divi divide by 3
#elif (Foxconn_SN65DSI85_Bridge == BIRDGE_TO_1920x1200)
	DSI2LVDS_REG_VAL(0x0a,0x05), /* configure PLL *///lvds clk range 62.5< rclk <87.5
	DSI2LVDS_REG_VAL(0x0b,0x28), /* configure PLL *///dsi clk divi divide by 3
#else
	DSI2LVDS_REG_VAL(0x0a, 0x00), /* configure PLL */
	DSI2LVDS_REG_VAL(0x0b, 0x00), /* configure PLL */
#endif
	DSI2LVDS_REG_VAL(0x0d, 0x01), /* pLL enable */
};

static u8 dsi2lvds_config_dsi[][2] = {
#if (Foxconn_SN65DSI85_Bridge == BIRDGE_TO_1280X800)
	DSI2LVDS_REG_VAL(0x10,0x26), /* default left right ganged mode */ //four lane
	DSI2LVDS_REG_VAL(0x12,0x29), /* channel A clk range *///205<Ra<210
	DSI2LVDS_REG_VAL(0x13,0x00), /* channel B clk range */
#elif (Foxconn_SN65DSI85_Bridge == BIRDGE_TO_1920x1200)
	DSI2LVDS_REG_VAL(0x10,0x26), /* default left right ganged mode */ //four lane
	DSI2LVDS_REG_VAL(0x12,0x5d), /* channel A clk range *///221<Ra<226
	DSI2LVDS_REG_VAL(0x13,0x00), /* channel B clk range */
#else	
	DSI2LVDS_REG_VAL(0x10, 0x80), /* default left right ganged mode */
	DSI2LVDS_REG_VAL(0x12, 0x08), /* channel A clk range */
	DSI2LVDS_REG_VAL(0x13, 0x08), /* channel B clk range */
#endif
}; 

static u8 dsi2lvds_config_lvds[][2] = {
#if (Foxconn_SN65DSI85_Bridge == BIRDGE_TO_1280X800)
	DSI2LVDS_REG_VAL(0x18,0x72),//disable CHB
	DSI2LVDS_REG_VAL(0x19,0x00),//1.2v
	DSI2LVDS_REG_VAL(0x1a,0x03),//CHA/B use def order
	DSI2LVDS_REG_VAL(0x1b,0x00),
#elif (Foxconn_SN65DSI85_Bridge == BIRDGE_TO_1920x1200)
	DSI2LVDS_REG_VAL(0x18,0x6c),//enable CHB
	DSI2LVDS_REG_VAL(0x19,0x00),//1.2v
	DSI2LVDS_REG_VAL(0x1a,0x03),//CHA/B use def order
	DSI2LVDS_REG_VAL(0x1b,0x00),
#endif
};

static u8 dsi2lvds_config_video[][2] = {
#if (Foxconn_SN65DSI85_Bridge == BIRDGE_TO_1280X800)
	DSI2LVDS_REG_VAL(0x20, 0x20),
	DSI2LVDS_REG_VAL(0x21, 0x03),
	DSI2LVDS_REG_VAL(0x22, 0x00),
	DSI2LVDS_REG_VAL(0x23, 0x00),
	DSI2LVDS_REG_VAL(0x24, 0x00),
	DSI2LVDS_REG_VAL(0x25, 0x00),
	DSI2LVDS_REG_VAL(0x26, 0x00),
	DSI2LVDS_REG_VAL(0x27, 0x00),
	DSI2LVDS_REG_VAL(0x28, 0x21),
	DSI2LVDS_REG_VAL(0x29, 0x00),
	DSI2LVDS_REG_VAL(0x2a, 0x00),
	DSI2LVDS_REG_VAL(0x2b, 0x00),
	DSI2LVDS_REG_VAL(0x2c, 0x20),
	DSI2LVDS_REG_VAL(0x2d, 0x00),
	DSI2LVDS_REG_VAL(0x2e, 0x00),
	DSI2LVDS_REG_VAL(0x2f, 0x00),
	DSI2LVDS_REG_VAL(0x30, 0x01),
	DSI2LVDS_REG_VAL(0x31, 0x00),
	DSI2LVDS_REG_VAL(0x32, 0x00),
	DSI2LVDS_REG_VAL(0x33, 0x00),
	DSI2LVDS_REG_VAL(0x34, 0x10),
	DSI2LVDS_REG_VAL(0x35, 0x00),
	DSI2LVDS_REG_VAL(0x36, 0x00),
	DSI2LVDS_REG_VAL(0x37, 0x00),
	DSI2LVDS_REG_VAL(0x38, 0x00),
	DSI2LVDS_REG_VAL(0x39, 0x00),
	DSI2LVDS_REG_VAL(0x3a, 0x00),
	DSI2LVDS_REG_VAL(0x3b, 0x00),
	DSI2LVDS_REG_VAL(0x3c, 0x00),
#elif (Foxconn_SN65DSI85_Bridge == BIRDGE_TO_1920x1200)
	DSI2LVDS_REG_VAL(0x20, 0x80),
	DSI2LVDS_REG_VAL(0x21, 0x07),
	DSI2LVDS_REG_VAL(0x22, 0x00),
	DSI2LVDS_REG_VAL(0x23, 0x00),
	DSI2LVDS_REG_VAL(0x24, 0x00),
	DSI2LVDS_REG_VAL(0x25, 0x00),
	DSI2LVDS_REG_VAL(0x26, 0x00),
	DSI2LVDS_REG_VAL(0x27, 0x00),
	DSI2LVDS_REG_VAL(0x28, 0x20),
	DSI2LVDS_REG_VAL(0x29, 0x00),
	DSI2LVDS_REG_VAL(0x2a, 0x00),
	DSI2LVDS_REG_VAL(0x2b, 0x00),
	DSI2LVDS_REG_VAL(0x2c, 0x02),
	DSI2LVDS_REG_VAL(0x2d, 0x00),
	DSI2LVDS_REG_VAL(0x2e, 0x00),
	DSI2LVDS_REG_VAL(0x2f, 0x00),
	DSI2LVDS_REG_VAL(0x30, 0x02),
	DSI2LVDS_REG_VAL(0x31, 0x00),
	DSI2LVDS_REG_VAL(0x32, 0x00),
	DSI2LVDS_REG_VAL(0x33, 0x00),
	DSI2LVDS_REG_VAL(0x34, 0x34),
	DSI2LVDS_REG_VAL(0x35, 0x00),
	DSI2LVDS_REG_VAL(0x36, 0x00),
	DSI2LVDS_REG_VAL(0x37, 0x00),
	DSI2LVDS_REG_VAL(0x38, 0x00),
	DSI2LVDS_REG_VAL(0x39, 0x00),
	DSI2LVDS_REG_VAL(0x3a, 0x00),
	DSI2LVDS_REG_VAL(0x3b, 0x00),
	DSI2LVDS_REG_VAL(0x3c, 0x00),
#else	
	DSI2LVDS_REG_VAL(0x20, 0x40), /* horizontal pixels on dsi channel A */
	DSI2LVDS_REG_VAL(0x21, 0x01),
	DSI2LVDS_REG_VAL(0x22, 0x40), /* horizontal pixels on dsi channel B */
	DSI2LVDS_REG_VAL(0x23, 0x01),
	DSI2LVDS_REG_VAL(0x24, 0xe0), /* vertical pixels on lvds channel A */
	DSI2LVDS_REG_VAL(0x25, 0x01),
	DSI2LVDS_REG_VAL(0x26, 0x00), /* vertical pixels on lvds channel B */
	DSI2LVDS_REG_VAL(0x27, 0x00),
	DSI2LVDS_REG_VAL(0x28, 0x40), /* Pixel clk delay from dsi to */
	DSI2LVDS_REG_VAL(0x29, 0x01), /* lvds channel A */
	DSI2LVDS_REG_VAL(0x2a, 0x10), /* Pixel clk delay from dsi to */
	DSI2LVDS_REG_VAL(0x2b, 0x00), /* lvds channel B */
	DSI2LVDS_REG_VAL(0x2c, 0x40), /* hsync width channel A */
	DSI2LVDS_REG_VAL(0x2d, 0x00),
	DSI2LVDS_REG_VAL(0x2e, 0x40), /* hsync width channel B */
	DSI2LVDS_REG_VAL(0x2f, 0x00),
	DSI2LVDS_REG_VAL(0x30, 0x02), /* vsync width channel A */
	DSI2LVDS_REG_VAL(0x31, 0x00),
	DSI2LVDS_REG_VAL(0x32, 0x00), /* vsync width channel B */
	DSI2LVDS_REG_VAL(0x33, 0x00),
	DSI2LVDS_REG_VAL(0x34, 0x10), /* h back porch channel A */
	DSI2LVDS_REG_VAL(0x35, 0x00), /* h back porch channel B */
	DSI2LVDS_REG_VAL(0x36, 0x21), /* v back porch channel A */
	DSI2LVDS_REG_VAL(0x37, 0x00), /* v back porch channel B */
	DSI2LVDS_REG_VAL(0x38, 0x10), /* h front porch channel A */
	DSI2LVDS_REG_VAL(0x39, 0x00), /* h front porch channel B */
	DSI2LVDS_REG_VAL(0x3a, 0x0a), /* v front porch channel A */
	DSI2LVDS_REG_VAL(0x3b, 0x00), /* v front porch channel B */
	DSI2LVDS_REG_VAL(0x3c, 0x00), /* channel A/B test pattern */
#endif
};

static u8 dsi2lvds_soft_reset[][2] = {
	DSI2LVDS_REG_VAL(0x09, 0x01),
};
static u8 dsi2lvds_config_TestPattern[][2] = {
		
	DSI2LVDS_REG_VAL(0x09,0x00),
	DSI2LVDS_REG_VAL(0x0A,0x05/*0x04*/),
	DSI2LVDS_REG_VAL(0x0B,0x28/*0x008*/),
	DSI2LVDS_REG_VAL(0x0D,0x01),
	DSI2LVDS_REG_VAL(0x10,0x3E),
	DSI2LVDS_REG_VAL(0x11,0x00),
	DSI2LVDS_REG_VAL(0x12,0x52/*0x05*/),
	DSI2LVDS_REG_VAL(0x13,0x00),
	DSI2LVDS_REG_VAL(0x18,0x6C),
	DSI2LVDS_REG_VAL(0x19,0x00),
	DSI2LVDS_REG_VAL(0x1A,0x03),
	DSI2LVDS_REG_VAL(0x1B,0x00),
	DSI2LVDS_REG_VAL(0x20,0x80),
	DSI2LVDS_REG_VAL(0x21,0x07),
	DSI2LVDS_REG_VAL(0x22,0x00),
	DSI2LVDS_REG_VAL(0x23,0x00),
	DSI2LVDS_REG_VAL(0x24,0x38),
	DSI2LVDS_REG_VAL(0x25,0x04),
	DSI2LVDS_REG_VAL(0x26,0x00),
	DSI2LVDS_REG_VAL(0x27,0x00),
	DSI2LVDS_REG_VAL(0x28,0x20),
	DSI2LVDS_REG_VAL(0x29,0x00),
	DSI2LVDS_REG_VAL(0x2A,0x00),	
	DSI2LVDS_REG_VAL(0x2B,0x00),
	DSI2LVDS_REG_VAL(0x2C,0x20),
	DSI2LVDS_REG_VAL(0x2D,0x00),
	DSI2LVDS_REG_VAL(0x2E,0x00),
	DSI2LVDS_REG_VAL(0x2F,0x00),
	DSI2LVDS_REG_VAL(0x30,0x04),
	DSI2LVDS_REG_VAL(0x31,0x00),
	DSI2LVDS_REG_VAL(0x32,0x00),
	DSI2LVDS_REG_VAL(0x33,0x00),
	DSI2LVDS_REG_VAL(0x34,0x30),
	DSI2LVDS_REG_VAL(0x35,0x00),
	DSI2LVDS_REG_VAL(0x36,0x00),
	DSI2LVDS_REG_VAL(0x37,0x00),
	DSI2LVDS_REG_VAL(0x38,0x00),
	DSI2LVDS_REG_VAL(0x39,0x00),
	DSI2LVDS_REG_VAL(0x3A,0x00),
	DSI2LVDS_REG_VAL(0x3B,0x00),
	DSI2LVDS_REG_VAL(0x3C,0x10),
	DSI2LVDS_REG_VAL(0x3D,0x00),
	DSI2LVDS_REG_VAL(0x3E,0x00),
	DSI2LVDS_REG_VAL(0xE0,0x00),
	DSI2LVDS_REG_VAL(0xE1,0x00),
	DSI2LVDS_REG_VAL(0xE2,0x00),
	DSI2LVDS_REG_VAL(0xE5,0x21),
	DSI2LVDS_REG_VAL(0xE6,0x0 ),

};

static struct bridge_reg mode_common[] = {
{0x09,0x00},
{0x0A,0x05},
{0x0B,0x00},
{0x0D,0x00},
{0x10,0x26},
{0x11,0x00},
{0x12,0x0D},//old 0x10 test 0x0d
{0x13,0x00},
{0x18,0xF0},//org 0x70
{0x19,0x00},
{0x1A,0x03},
{0x1B,0x00},
{0x20,0x20},
{0x21,0x03},
{0x22,0x00},
{0x23,0x00},
{0x24,0x00},
{0x25,0x00},
{0x26,0x00},
{0x27,0x00},
{0x28,0xe5},
{0x29,0x07},
{0x2A,0x00},
{0x2B,0x00},
{0x2C,0x40},
{0x2D,0x00},
{0x2E,0x00},
{0x2F,0x00},
{0x30,0x01},
{0x31,0x00},
{0x32,0x00},
{0x33,0x00},
{0x34,0x80},
{0x35,0x00},
{0x36,0x00},
{0x37,0x00},
{0x38,0x00},
{0x39,0x00},
{0x3A,0x00},
{0x3B,0x00},
{0x3C,0x00},
{0x3D,0x00},
{0x3E,0x00},
{0x0D,0x01},
{BRIDGE_TABLE_END,0x00},
};


static struct i2c_driver tegra_dsi2lvds_i2c_slave_driver = {
	.driver = {
		.name = "dsi2lvds_bridge",
	},
};

static struct i2c_client *init_i2c_slave(struct tegra_dc_dsi_data *dsi)
{
	struct i2c_adapter *adapter;
	struct i2c_client *client;
	struct i2c_board_info p_data = {
		.type = "dsi2lvds_bridge",
#if Foxconn_SN65DSI85_Bridge
		.addr = 0x2C,
#else		
		.addr = 0x2D,
#endif		
	};
	int bus = DSI2LVDS_TEGRA_I2C_BUS;
	int err = 0;

	adapter = i2c_get_adapter(bus);
	if (!adapter) {
		dev_err(&dsi->dc->ndev->dev,
			"dsi2lvds: can't get adpater for bus %d\n", bus);
		err = -EBUSY;
		goto err;
	}

	client = i2c_new_device(adapter, &p_data);
	i2c_put_adapter(adapter);
	if (!client) {
		dev_err(&dsi->dc->ndev->dev,
			"dsi2lvds: can't add i2c slave device\n");
		err = -EBUSY;
		goto err;
	}

	err = i2c_add_driver(&tegra_dsi2lvds_i2c_slave_driver);
	if (err) {
		dev_err(&dsi->dc->ndev->dev,
			"dsi2lvds: can't add i2c slave driver\n");
		goto err_free;
	}

	return client;
err:
	return ERR_PTR(err);
err_free:
	i2c_unregister_device(client);
	return ERR_PTR(err);
}

static int tegra_dsi2lvds_init(struct tegra_dc_dsi_data *dsi)
{
	int err = 0;
#if foxconn_Display_Debug	
	pr_warn("tegra_dsi2lvds_init++++++++tegra_dsi2lvds_init\n");
#endif	 
	if (dsi2lvds) {
		tegra_dsi_set_outdata(dsi, dsi2lvds);
		return err;
	}

	dsi2lvds = devm_kzalloc(&dsi->dc->ndev->dev, sizeof(*dsi2lvds), GFP_KERNEL);
	if (!dsi2lvds)
		return -ENOMEM;

	dsi2lvds->client_i2c = init_i2c_slave(dsi);
	if (IS_ERR_OR_NULL(dsi2lvds->client_i2c)) {
		dev_err(&dsi->dc->ndev->dev,
			"dsi2lvds: i2c slave setup failure\n");
	}

	dsi2lvds->dsi = dsi;

	tegra_dsi_set_outdata(dsi, dsi2lvds);

	mutex_init(&dsi2lvds->lock);
	dsi2lvds->dsi2lvds_enabled = true;
#if foxconn_Display_Debug
	pr_warn("tegra_dsi2lvds_init--------------tegra_dsi2lvds_init\n"); 
#endif
	return err;
}

static void tegra_dsi2lvds_destroy(struct tegra_dc_dsi_data *dsi)
{
	struct tegra_dc_dsi2lvds_data *dsi2lvds = tegra_dsi_get_outdata(dsi);

	if (!dsi2lvds)
		return;

	mutex_lock(&dsi2lvds->lock);
	i2c_del_driver(&tegra_dsi2lvds_i2c_slave_driver);
	i2c_unregister_device(dsi2lvds->client_i2c);
	mutex_unlock(&dsi2lvds->lock);
	mutex_destroy(&dsi2lvds->lock);
	kfree(dsi2lvds);
}

static int sensor_read_reg8_addr8(struct i2c_client *client, u8 addr, u8 *val)
{
	int err;
	struct i2c_msg msg[2];
	unsigned char data[2];

	if (!client->adapter)
		return -ENODEV;

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = data;

	/* high byte goes out first */
	data[0] = (u8) (addr & 0xff);

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = data + 1;

	err = i2c_transfer(client->adapter, msg, 2);

	if (err<0)
		return -EINVAL;

        //swap(*(data+2),*(data+3)); //swap high and low byte to match table format
	memcpy(val, data+1, 1);

	return 0;
}
static void sn65dsi83_dump_regs(struct i2c_client *client,u8 addr){
	u8 val=0x00;
	sensor_read_reg8_addr8(client,addr,&val);
	dev_info(&client->dev,
		"%s:  sn65dsi83 0x%02x = 0x%02x\n", client->name,addr,val);
}
static int bridge_read_table(struct i2c_client *client,
			      const struct bridge_reg table[])
{
	const struct bridge_reg *next;

    
	dev_info(&client->dev," %s: =========>exe start\n",__func__);
       next = table ;       

	for (next = table; next->addr!= BRIDGE_TABLE_END; next++) {
	       sn65dsi83_dump_regs(client, next->addr);
	}
	dev_info(&client->dev," %s: =========>exe end\n",__func__);
	return 0;
}

static int dsi2lvds_i2c_transfer(struct tegra_dc_dsi2lvds_data *dsi2lvds,
					u8 transfers[][2], u32 no_of_tranfers,
					enum i2c_transfer_type type)
{
	struct i2c_msg *i2c_msg_transfer;
	struct i2c_client *client = dsi2lvds->client_i2c;
	int err = 0;
	u32 cnt = 0;

	i2c_msg_transfer = kzalloc
		(no_of_tranfers * sizeof(*i2c_msg_transfer), GFP_KERNEL);
	if (!i2c_msg_transfer)
		return -ENOMEM;

	for (cnt = 0; cnt < no_of_tranfers; cnt++) {
		i2c_msg_transfer[cnt].addr = client->addr;
		i2c_msg_transfer[cnt].flags = type;
		i2c_msg_transfer[cnt].len = 2;
		i2c_msg_transfer[cnt].buf = transfers[cnt];
	}

	for (cnt = 0; cnt < no_of_tranfers; cnt++) {
		err = i2c_transfer(client->adapter, &i2c_msg_transfer[cnt], 1);
		if (err < 0) {
			dev_err(&dsi2lvds->dsi->dc->ndev->dev,
				"dsi2lvds: i2c write failed\n");
			break;
		}
		msleep(10);
	}

	kfree(i2c_msg_transfer);
	return err;
}
static void tegra_dsi2lvds_enable(struct tegra_dc_dsi_data *dsi)
{
	struct tegra_dc_dsi2lvds_data *dsi2lvds = tegra_dsi_get_outdata(dsi);
	int err = 0;
#if Foxconn_SN65DSI85_Bridge
	//int k=1; //for test pattern
	int k=0;
#else
	int k=1;
#endif	
#if foxconn_Display_Debug	
	pr_warn("%s:++++++++tegra_dsi2lvds_enable ganged_type=%d\n",__func__,dsi2lvds->dsi->info.ganged_type);
#endif	 
	if (dsi2lvds && dsi2lvds->dsi2lvds_enabled){
		dev_info(&dsi->dc->ndev->dev,"dsi2lvds: dsi have inited!\n"); 
		return;
	}

	mutex_lock(&dsi2lvds->lock);
	
if(k==1){	
	err = dsi2lvds_i2c_transfer(dsi2lvds, dsi2lvds_config_TestPattern,
			ARRAY_SIZE(dsi2lvds_config_TestPattern), I2C_WRITE);
	if (err < 0) {
		dev_err(&dsi->dc->ndev->dev, "dsi2lvds: Init video failed\n");
		goto fail;
	}		
}else{	
	err = dsi2lvds_i2c_transfer(dsi2lvds, dsi2lvds_soft_reset,
			ARRAY_SIZE(dsi2lvds_soft_reset), I2C_WRITE);
	if (err < 0) {
		dev_err(&dsi->dc->ndev->dev, "dsi2lvds: Soft reset failed\n");
		goto fail;
	}

	err = dsi2lvds_i2c_transfer(dsi2lvds, dsi2lvds_config_clk,
			ARRAY_SIZE(dsi2lvds_config_clk), I2C_WRITE);
	if (err < 0) {
		dev_err(&dsi->dc->ndev->dev, "dsi2lvds: Init clk failed\n");
		goto fail;
	}

	if (dsi2lvds->dsi->info.ganged_type ==
		TEGRA_DSI_GANGED_SYMMETRIC_EVEN_ODD) {
		u32 cnt;
		for (cnt = 0;
		cnt < ARRAY_SIZE(dsi2lvds_config_dsi); cnt++) {
			if (dsi2lvds_config_dsi[cnt][0] == 0x10)
				/* select odd even ganged mode */
				dsi2lvds_config_dsi[cnt][1] &= ~(1 << 7);
		}
	}

	err = dsi2lvds_i2c_transfer(dsi2lvds, dsi2lvds_config_dsi,
			ARRAY_SIZE(dsi2lvds_config_dsi), I2C_WRITE);
	if (err < 0) {
		dev_err(&dsi->dc->ndev->dev, "dsi2lvds: Init dsi failed\n");
		goto fail;
	}

	err = dsi2lvds_i2c_transfer(dsi2lvds, dsi2lvds_config_lvds,
			ARRAY_SIZE(dsi2lvds_config_lvds), I2C_WRITE);
	if (err < 0) {
		dev_err(&dsi->dc->ndev->dev, "dsi2lvds: Init lvds failed\n");
		goto fail;
	}

	err = dsi2lvds_i2c_transfer(dsi2lvds, dsi2lvds_config_video,
			ARRAY_SIZE(dsi2lvds_config_video), I2C_WRITE);
	if (err < 0) {
		dev_err(&dsi->dc->ndev->dev, "dsi2lvds: Init video failed\n");
		goto fail;
	}
	if(foxconn_Display_Debug >1) 	
	bridge_read_table(dsi2lvds->client_i2c,mode_common);

}
	dsi2lvds->dsi2lvds_enabled = true;
#if foxconn_Display_Debug		
	pr_warn("%s-------------tegra_dsi2lvds_enable\n",__func__);
#endif	
fail:
#if foxconn_Display_Debug
	pr_warn("%s-------------enable end\n",__func__);
#endif	
	mutex_unlock(&dsi2lvds->lock);
}

static void tegra_dsi2lvds_disable(struct tegra_dc_dsi_data *dsi)
{
	struct tegra_dc_dsi2lvds_data *dsi2lvds = tegra_dsi_get_outdata(dsi);
	int err = 0;
	/* To be done */
#if foxconn_Display_Debug		
	pr_warn("%s-------------tegra_dsi2lvds_disable\n",__func__);
#endif	
	if (dsi2lvds && !dsi2lvds->dsi2lvds_enabled){
		dev_info(&dsi->dc->ndev->dev,"dsi2lvds: dsi have disable!\n"); 
		return;
	}
	mutex_lock(&dsi2lvds->lock);
	
	err = dsi2lvds_i2c_transfer(dsi2lvds, dsi2lvds_disable_config_clk,
			ARRAY_SIZE(dsi2lvds_disable_config_clk), I2C_WRITE);
	if (err < 0) {
		dev_err(&dsi->dc->ndev->dev, "dsi2lvds: disable clk failed\n");
	}
#if foxconn_Display_Debug
	pr_warn("%s-------------disable end\n",__func__);
#endif	
	dsi2lvds->dsi2lvds_enabled = false;
	mutex_unlock(&dsi2lvds->lock);
}

#ifdef CONFIG_PM
static void tegra_dsi2lvds_suspend(struct tegra_dc_dsi_data *dsi)
{
	dsi2lvds->dsi2lvds_enabled = false;
#if foxconn_Display_Debug
	pr_warn("%s-------------exe\n",__func__);
#endif
	/* To be done */
}

static void tegra_dsi2lvds_resume(struct tegra_dc_dsi_data *dsi)
{
	/* To be done */
}
#endif

struct tegra_dsi_out_ops tegra_dsi2lvds_ops = {
	.init = tegra_dsi2lvds_init,
	.destroy = tegra_dsi2lvds_destroy,
	.enable = tegra_dsi2lvds_enable,
	.disable = tegra_dsi2lvds_disable,
#ifdef CONFIG_PM
	.suspend = tegra_dsi2lvds_suspend,
	.resume = tegra_dsi2lvds_resume,
#endif
};