/*
 * phy-fusb300.c: fusb300 usb phy driver for type-c and PD
 *
 * Copyright (C) 2014 Intel Corporation
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. Seee the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Author: Kannappan, R <r.kannappan@intel.com>
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/pm.h>
#include <linux/mod_devicetable.h>
#include <linux/power_supply.h>
#include <linux/regmap.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/acpi.h>
#include <linux/pm_runtime.h>
#include <linux/notifier.h>
#include <linux/suspend.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/usb_typec_phy.h>
#include "usb_typec_detect.h"
#include "pd/system_policy.h"

/* Status register bits */
#define FUSB300_STAT0_REG		0x40
#define FUSB300_STAT0_VBUS_OK		BIT(7)
#define FUSB300_STAT0_ACTIVITY		BIT(6)
#define FUSB300_STAT0_COMP		BIT(5)
#define FUSB300_STAT0_CRCCHK		BIT(4)
#define FUSB300_STAT0_ALERT		BIT(3)
#define FUSB300_STAT0_WAKE		BIT(2)
#define FUSB300_STAT0_BC_LVL		(BIT(1)|BIT(0))
#define FUSB300_STAT0_BC_LVL_MASK	3
#define FUSB300_BC_LVL_VRA		0
#define FUSB300_BC_LVL_USB		1
#define FUSB300_BC_LVL_1500		2
#define FUSB300_BC_LVL_3000		3

#define FUSB300_STAT1_REG		0x41
#define FUSB300_STAT1_RXSOP2		BIT(7)
#define FUSB300_STAT1_RXSOP1		BIT(6)
#define FUSB300_STAT1_RXEMPTY		BIT(5)
#define FUSB300_STAT1_RXFULL		BIT(4)
#define FUSB300_STAT1_TXEMPTY		BIT(3)
#define FUSB300_STAT1_TXFULL		BIT(2)
#define FUSB300_STAT1_OVERTEMP		BIT(1)
#define FUSB300_STAT1_SHORT		BIT(0)

#define FUSB300_INT_REG		0x42
#define FUSB300_INT_VBUS_OK	BIT(7)
#define FUSB300_INT_ACTIVITY	BIT(6)
#define FUSB300_INT_COMP		BIT(5)
#define FUSB300_INT_CRCCHK		BIT(4)
#define FUSB300_INT_ALERT		BIT(3)
#define FUSB300_INT_WAKE		BIT(2)
#define FUSB300_INT_COLLISION	BIT(1)
#define FUSB300_INT_BC_LVL		BIT(0)
/* Interrupt mask bits */
#define FUSB300_INT_MASK_REG		0xa
#define FUSB300_INT_MASK_VBUS_OK	BIT(7)
#define FUSB300_INT_MASK_ACTIVITY	BIT(6)
#define FUSB300_INT_MASK_COMP		BIT(5)
#define FUSB300_INT_MASK_CRCCHK		BIT(4)
#define FUSB300_INT_MASK_ALERT		BIT(3)
#define FUSB300_INT_MASK_WAKE		BIT(2)
#define FUSB300_INT_MASK_COLLISION	BIT(1)
#define FUSB300_INT_MASK_BC_LVL		BIT(0)

#define FUSB300_SLICE_REG		0x5
/* control */
#define FUSB300_CONTROL0_REG		0x6
#define FUSB300_CONTROL0_TX_FLUSH	BIT(6)
#define FUSB300_CONTROL0_MASK_INT	BIT(5)
#define FUSB300_CONTROL0_LOOPBACK	BIT(4)
#define FUSB300_CONTROL0_HOST_CUR	(BIT(3)|BIT(2))
#define FUSB300_CONTROL0_AUTO_PREAMBLE	BIT(1)
#define FUSB300_CONTROL0_TX_START	BIT(0)

#define FUSB300_HOST_CUR_MASK		3
#define FUSB300_HOST_CUR_SHIFT		2
#define FUSB300_HOST_CUR(x)		(((x) >> FUSB300_HOST_CUR_SHIFT) & \
					FUSB300_HOST_CUR_MASK)
#define FUSB300_HOST_CUR_DISABLE	0
#define FUSB300_HOST_CUR_USB_SDP	1
#define FUSB300_HOST_CUR_1500		2
#define FUSB300_HOST_CUR_3000		3

#define FUSB300_CONTROL1_REG		0x7
#define FUSB300_CONTROL1_ENSOP1		BIT(0)
#define FUSB300_CONTROL1_ENSOP2		BIT(1)
#define FUSB300_CONTROL1_RX_FLUSH	BIT(2)
#define FUSB302_CONTROL1_FAST_I2C	BIT(3)
#define FUSB300_CONTROL1_BIST_MODE	BIT(4)

#define FUSB302_CONTROL2_REG		0x8
#define FUSB302_CONTROL2_TOGGLE_EN	BIT(0)

#define FUSB302_CONTROL2_TOG_MODE_SHIFT	1
#define FUSB302_CONTROL2_TOG_MODE_MASK	3

#define FUSB302_TOG_MODE_DFP		3
#define FUSB302_TOG_MODE_UFP		2
#define FUSB302_TOG_MODE_DRP		1

#define FUSB302_CONTROL3_REG		0x9
#define FUSB302_CONTROL3_AUTO_RETRY	BIT(0)
#define FUSB302_CONTROL3_N_RETRY_SHIFT	1
#define FUSB302_CONTROL3_AUTO_SOFT_RST	BIT(3)
#define FUSB302_CONTROL3_AUTO_HARD_RST	BIT(4)
#define FUSB302_CONTROL3_SEND_HARD_RST	BIT(6)

#define FUSB302_CONTROL3_RETRY3		3
#define FUSB302_CONTROL3_RETRY2		2
#define FUSB302_CONTROL3_RETRY1		1
#define FUSB302_CONTROL3_NO_RETRY	0

#define FUSB300_SOFT_POR_REG		0xc
#define FUSB300_SOFT_POR		BIT(0)
#define FUSB300_PD_POR			BIT(1)

#define FUSB300_SWITCH0_REG		0x2
#define FUSB300_SWITCH0_PD_CC1_EN	BIT(0)
#define FUSB300_SWITCH0_PD_CC2_EN	BIT(1)
#define FUSB300_SWITCH0_PU_CC1_EN	BIT(6)
#define FUSB300_SWITCH0_PU_CC2_EN	BIT(7)
#define FUSB300_SWITCH0_PU_EN		(BIT(7)|BIT(6))
#define FUSB300_SWITCH0_PD_EN		(BIT(0)|BIT(1))
#define FUSB300_SWITCH0_PU_PD_MASK	3
#define FUSB300_SWITCH0_PU_SHIFT	6
#define FUSB300_SWITCH0_PD_SHIFT	0
#define FUSB300_SWITCH0_MEASURE_CC1	BIT(2)
#define FUSB300_SWITCH0_MEASURE_CC2	BIT(3)
#define FUSB300_VCONN_CC1_EN		BIT(4)
#define FUSB300_VCONN_CC2_EN		BIT(5)

#define FUSB300_SWITCH1_REG		0x3
#define FUSB300_SWITCH1_TXCC1		BIT(0)
#define FUSB300_SWITCH1_TXCC2		BIT(1)
#define FUSB302_SWITCH1_AUTOCRC		BIT(2)
#define FUSB302_SWITCH1_DATAROLE	BIT(4)
#define FUSB302_SWITCH1_PWRROLE		BIT(7)
#define FUSB300_SWITCH1_TX_MASK		3

#define FUSB300_MEAS_REG		0x4
#define FUSB300_MEAS_VBUS		BIT(6)
#define FUSB300_MEAS_RSLT_MASK		0x3f

#define FUSB300_MEASURE_VBUS		1
#define FUSB300_MEASURE_CC		2

#define FUSB300_HOST_RD_MIN		0x24
#define FUSB300_HOST_RD_MAX		0x3e
#define FUSB300_HOST_RA_MIN		0xa
#define FUSB300_HOST_RA_MAX		0x13

#define FUSB300_PWR_REG			0xb
#define FUSB300_PWR_BG_WKUP		BIT(0)
#define FUSB300_PWR_BMC			BIT(1)
#define FUSB300_PWR_MEAS		BIT(2)
#define FUSB300_PWR_OSC			BIT(3)
#define FUSB300_PWR_SHIFT		0

#define FUSB300_FIFO_REG		0x43

#define FUSB300_COMP_RD_LOW		0x24
#define FUSB300_COMP_RD_HIGH		0x3e
#define FUSB300_COMP_RA_LOW		0xa
#define FUSB300_COMP_RA_HIGH		0x12

#define FUSB30x_DEV_ID_REG		0x1
#define FUSB30x_VER_ID_MASK		0xf0
#define FUSB30x_REV_ID_MASK		0x0f
#define FUSB300_VER_ID			0x60
#define FUSB300_REV_ID			0x0
#define FUSB302_VER_ID			0x80


#define FUSB302_INT_MASKA_REG		0xe
#define FUSB302_MASK_HARD_RST		BIT(0)
#define FUSB302_MASK_SOFT_RST		BIT(1)
#define FUSB302_MASK_TX_SENT		BIT(2)
#define FUSB302_MASK_TX_HARD_RST	BIT(3)
#define FUSB302_MASK_RETRY_FAIL		BIT(4)
#define FUSB302_MASK_TX_SOFT_FAIL	BIT(5)
#define FUSB302_MASK_TOG_DONE		BIT(6)
#define FUSB302_MASK_OCP_TEMP		BIT(7)

#define FUSB302_INT_MASKB_REG		0xf
#define FUSB302_MASK_GCRC_SENT		BIT(0)

#define FUSB302_STAT0A_REG		0x3c
#define FUSB302_STAT0A_RX_HARD_RST	BIT(0)
#define FUSB302_STAT0A_RX_SOFT_RST	BIT(1)
#define FUSB302_STAT0A_RETRY_FAIL	BIT(4)
#define FUSB302_STAT0A_SOFT_RST_FAIL	BIT(5)
#define FUSB302_STAT0A_TOGDONE		BIT(6)

#define FUSB302_STAT1A_REG		0x3d
#define FUSB302_STAT1A_RX_SOP		BIT(0)
#define FUSB302_STAT1A_SOP1_DBG		BIT(1)
#define FUSB302_STAT1A_SOP2_DBG		BIT(2)
#define FUSB302_STAT1A_TOG_STAT		(BIT(3)|BIT(4)|BIT(5))
#define FUSB302_STAT1A_TOG_STAT_SHIFT	3

#define FUSB302_TOG_STAT_DFP_CC1	1
#define FUSB302_TOG_STAT_DFP_CC2	2
#define FUSB302_TOG_STAT_UFP_CC1	5
#define FUSB302_TOG_STAT_UFP_CC2	6

#define FUSB302_INTA_REG		0x3e
#define FUSB302_INTA_HARD_RST		BIT(0)
#define FUSB302_INTA_SOFT_RST		BIT(1)
#define FUSB302_INTA_TX_SENT		BIT(2)
#define FUSB302_INTA_TX_HARD_RST	BIT(3)
#define FUSB302_INTA_TX_RETRY_FAIL	BIT(4)
#define FUSB302_INTA_TX_SOFT_RST_FAIL	BIT(5)
#define FUSB302_INTA_TOG_DONE		BIT(6)

#define FUSB302_INTB_REG		0x3f
#define FUSB302_INTB_GCRC_SENT		BIT(0)

#define FUSB302_TOG_STAT(x)		(((x) & FUSB302_STAT1A_TOG_STAT) >> \
						(FUSB302_STAT1A_TOG_STAT_SHIFT))


#define is_fusb300(x)			((((x) & FUSB30x_REV_ID_MASK) == 0) && \
					(((x) & FUSB30x_VER_ID_MASK) ==	\
					FUSB300_VER_ID))
#define is_fusb302(x)			((((x) & FUSB30x_REV_ID_MASK) == 0) && \
					(((x) & FUSB30x_VER_ID_MASK) ==	\
					FUSB302_VER_ID))

#define SOP1				0x12
#define SOP2				0x13
#define SOP3				0x1b
#define RESET1				0x15
#define RESET2				0x16
#define PACKSYM				0x80
#define JAMCRC				0xff
#define EOP				0x14
#define TXON				0xa1
#define TXOFF				0xfe

#define USB_TYPEC_PD_VERSION		2

#define FUSB300_MAX_INT_STAT		3
#define FUSB302_MAX_INT_STAT		7
#define MAX_FIFO_SIZE	64

static int host_cur[4] = {
	TYPEC_CURRENT_UNKNOWN,
	TYPEC_CURRENT_USB,
	TYPEC_CURRENT_1500,
	TYPEC_CURRENT_3000
};

struct fusb300_int_stat {
	u8 stat0a_reg;
	u8 stat1a_reg;
	u8 inta_reg;
	u8 intb_reg;
	u8 stat_reg;
	u8 stat1_reg;
	u8 int_reg;
} __packed;

struct fusb300_chip {
	struct i2c_client *client;
	struct device *dev;
	struct regmap *map;
	struct mutex lock;
	struct typec_phy phy;
	bool i_vbus;
	u32 stored_int_reg;
	bool transmit;
	int activity_count;
	struct completion int_complete;
	spinlock_t irqlock;
	bool process_pd;
	int is_fusb300;
};

static int fusb300_wake_on_cc_change(struct fusb300_chip *chip);
static inline int fusb302_enable_toggle(struct fusb300_chip *chip, bool en,
					int mode);
static inline int fusb300_send_pkt(struct typec_phy *phy, u8 *buf, int len);
static inline int fusb300_recv_pkt(struct typec_phy *phy, u8 *buf);
static int fusb300_flush_fifo(struct typec_phy *phy, enum typec_fifo fifo_type);
static inline int fusb300_pd_send_hard_rst(struct typec_phy *phy);
static inline int fusb302_pd_send_hard_rst(struct typec_phy *phy);

static int fusb300_get_negotiated_cur(int val)
{
	if (val >= 0 && val < 4)
		return host_cur[val];
	return TYPEC_CURRENT_UNKNOWN;
}

static int fusb300_set_host_current(struct typec_phy *phy,
					enum typec_current cur)
{
	struct fusb300_chip *chip;
	int ret;
	u8 i;
	u32 val;

	if (!phy)
		return -EINVAL;

	chip = dev_get_drvdata(phy->dev);
	for (i = 0; i < ARRAY_SIZE(host_cur); i++) {
		if (host_cur[i] == cur)
			break;
	}
	if (i >= ARRAY_SIZE(host_cur)) {
		dev_err(phy->dev, "%s: host current mismatch\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&chip->lock);
	regmap_read(chip->map, FUSB300_CONTROL0_REG, &val);
	val &= ~(FUSB300_HOST_CUR_MASK << FUSB300_HOST_CUR_SHIFT);
	val |= (i << FUSB300_HOST_CUR_SHIFT);
	dev_dbg(phy->dev, "control0 reg = %x cur = %d i = %d", val, cur, i);
	ret = regmap_write(chip->map, FUSB300_CONTROL0_REG, val);
	mutex_unlock(&chip->lock);

	return ret;
}

static enum typec_current fusb300_get_host_current(struct typec_phy *phy)
{
	struct fusb300_chip *chip;
	unsigned int val;
	int ret;

	if (!phy)
		return TYPEC_CURRENT_UNKNOWN;

	chip = dev_get_drvdata(phy->dev);
	ret = regmap_read(chip->map, FUSB300_CONTROL0_REG, &val);
	if (ret < 0)
		return TYPEC_CURRENT_UNKNOWN;
	return fusb300_get_negotiated_cur(FUSB300_HOST_CUR(val));
}


static int fusb300_en_pu(struct fusb300_chip *chip, bool en_pu, int cur)
{
	unsigned int val = 0;
	int ret;

	mutex_lock(&chip->lock);
	ret = regmap_read(chip->map, FUSB300_SWITCH0_REG, &val);
	if (ret < 0) {
		dev_err(&chip->client->dev, "error(%d) reading %x\n",
				ret, FUSB300_SWITCH0_REG);
		mutex_unlock(&chip->lock);
		return ret;
	}

	if (en_pu) {
		val &= ~FUSB300_SWITCH0_PD_EN;
		val |= FUSB300_SWITCH0_PU_EN;
	} else {
		val &= ~FUSB300_SWITCH0_PU_EN;
	}
	dev_dbg(chip->dev, "%s: switch0 %x = %x", __func__,
				FUSB300_SWITCH0_REG, val);
	mutex_unlock(&chip->lock);
	ret = fusb300_set_host_current(&chip->phy, cur);
	if (ret < 0) {
		dev_err(&chip->client->dev,
			"error setting host cur%d", ret);
		return ret;
	}
	mutex_lock(&chip->lock);
	ret = regmap_write(chip->map, FUSB300_SWITCH0_REG, val);
	mutex_unlock(&chip->lock);
	if (ret < 0)
		dev_err(&chip->client->dev, "error(%d) writing %x\n",
				ret, FUSB300_SWITCH0_REG);

	return ret;
}

static int fusb300_en_pd(struct fusb300_chip *chip, bool en_pd)
{
	unsigned int val = 0;
	int ret;

	mutex_lock(&chip->lock);
	ret = regmap_read(chip->map, FUSB300_SWITCH0_REG, &val);
	if (ret < 0) {
		mutex_unlock(&chip->lock);
		dev_err(&chip->client->dev, "error(%d) reading %x\n",
				ret, FUSB300_SWITCH0_REG);
		return ret;
	}

	if (en_pd) {
		val |= FUSB300_SWITCH0_PD_EN;
		val &= ~FUSB300_SWITCH0_PU_EN;
	} else {
		val &= ~FUSB300_SWITCH0_PD_EN;
	}
	dev_dbg(chip->dev, "%s: switch0 %x = %x", __func__,
				FUSB300_SWITCH0_REG, val);
	ret = regmap_write(chip->map, FUSB300_SWITCH0_REG, val);
	mutex_unlock(&chip->lock);
	if (ret < 0)
		dev_err(&chip->client->dev, "error(%d) write %d",
				ret, FUSB300_SWITCH0_REG);
	return ret;
}

static int fusb300_switch_mode(struct typec_phy *phy, enum typec_mode mode)
{
	struct fusb300_chip *chip;
	int cur;

	if (!phy)
		return -ENODEV;

	dev_dbg(phy->dev, "%s: %d", __func__, mode);
	chip = dev_get_drvdata(phy->dev);

	if (mode == TYPEC_MODE_UFP) {
		if (chip->is_fusb300)
			fusb300_set_host_current(phy, 0);
		fusb300_en_pd(chip, true);
		mutex_lock(&chip->lock);
		phy->state = TYPEC_STATE_UNATTACHED_UFP;
		regmap_write(chip->map, FUSB300_MEAS_REG, 0x31);
		/* for FPGA write different values
		regmap_write(chip->map, FUSB300_MEAS_REG, 0x28);*/
		mutex_unlock(&chip->lock);
	} else if (mode == TYPEC_MODE_DFP) {
		cur = TYPEC_CURRENT_USB;
		mutex_lock(&chip->lock);
		phy->state = TYPEC_STATE_UNATTACHED_DFP;
		if (cur == TYPEC_CURRENT_3000) {
			regmap_write(chip->map, FUSB300_MEAS_REG, 0x3E);
			/* for FPGA write different values
			regmap_write(chip->map, FUSB300_MEAS_REG, 0x33);*/
		} else {
			regmap_write(chip->map, FUSB300_MEAS_REG, 0x26);
			/* for FPGA write different values
			regmap_write(chip->map, FUSB300_MEAS_REG, 0x1f);*/
		}
		mutex_unlock(&chip->lock);
		fusb300_en_pu(chip, true, cur);
	} else if (mode == TYPEC_MODE_DRP)
		fusb300_wake_on_cc_change(chip);

	return 0;
}

static int fusb300_setup_cc(struct typec_phy *phy, enum typec_cc_pin cc,
				enum typec_state state)
{
	struct fusb300_chip *chip;
	unsigned int val = 0;

	if (!phy)
		return -ENODEV;

	dev_dbg(phy->dev, "%s cc: %d state: %d\n", __func__, cc, state);
	chip = dev_get_drvdata(phy->dev);

	mutex_lock(&chip->lock);
	phy->valid_cc = cc;

	switch (state) {
	case TYPEC_STATE_ATTACHED_UFP:
	case TYPEC_STATE_ATTACHED_DFP:
	case TYPEC_STATE_UNKNOWN:
	case TYPEC_STATE_POWERED:
		phy->state = state;
		break;
	default:
		break;
	}

	if (cc == TYPEC_PIN_CC1) {
		val |= FUSB300_SWITCH0_MEASURE_CC1;
		if (phy->state == TYPEC_STATE_ATTACHED_UFP)
			val |= FUSB300_SWITCH0_PD_CC1_EN;
		else if (phy->state == TYPEC_STATE_ATTACHED_DFP)
			val |= FUSB300_SWITCH0_PU_CC1_EN;
	} else if (cc == TYPEC_PIN_CC2) {
		val |= FUSB300_SWITCH0_MEASURE_CC2;
		if (phy->state == TYPEC_STATE_ATTACHED_UFP)
			val |= FUSB300_SWITCH0_PD_CC2_EN;
		else if (phy->state == TYPEC_STATE_ATTACHED_DFP)
			val |= FUSB300_SWITCH0_PU_CC2_EN;
	} else { /* cc removal */
		goto end;
	}

	regmap_write(chip->map, FUSB300_SWITCH0_REG, val);
end:
	mutex_unlock(&chip->lock);

	return 0;
}


#ifdef DEBUG
static void dump_registers(struct fusb300_chip *chip)
{
	struct regmap *regmap = chip->map;
	int ret;
	unsigned int val;

	ret = regmap_read(regmap, 1, &val);
	dev_info(chip->dev, "reg1 = %x", val);

	ret = regmap_read(regmap, 2, &val);
	dev_info(chip->dev, "reg2 = %x", val);

	ret = regmap_read(regmap, 3, &val);
	dev_info(chip->dev, "reg3 = %x", val);

	ret = regmap_read(regmap, 4, &val);
	dev_info(chip->dev, "reg4 = %x", val);

	ret = regmap_read(regmap, 5, &val);
	dev_info(chip->dev, "reg5 = %x", val);

	ret = regmap_read(regmap, 6, &val);
	dev_info(chip->dev, "reg6 = %x", val);

	ret = regmap_read(regmap, 7, &val);
	dev_info(chip->dev, "reg7 = %x", val);

	ret = regmap_read(regmap, 8, &val);
	dev_info(chip->dev, "reg8 = %x", val);

	ret = regmap_read(regmap, 9, &val);
	dev_info(chip->dev, "reg9 = %x", val);

	ret = regmap_read(regmap, 0xa, &val);
	dev_info(chip->dev, "rega = %x", val);

	ret = regmap_read(regmap, 0xb, &val);
	dev_info(chip->dev, "regb = %x", val);

	ret = regmap_read(regmap, 0xc, &val);
	dev_info(chip->dev, "regc = %x", val);

	ret = regmap_read(regmap, 0x40, &val);
	dev_info(chip->dev, "reg40 = %x", val);

	ret = regmap_read(regmap, 0x41, &val);
	dev_info(chip->dev, "reg41 = %x", val);

	ret = regmap_read(regmap, 0x42, &val);
	dev_info(chip->dev, "reg42 = %x", val);
}
#endif

static inline int fusb302_configure_pd(struct fusb300_chip *chip)
{
	unsigned int val;

	val = FUSB302_CONTROL3_AUTO_RETRY |
		(FUSB302_CONTROL3_RETRY3 << FUSB302_CONTROL3_N_RETRY_SHIFT) |
		FUSB302_CONTROL3_AUTO_SOFT_RST |
		FUSB302_CONTROL3_AUTO_HARD_RST;

	regmap_write(chip->map, FUSB302_CONTROL3_REG, val);
	return 0;
}

static int fusb300_init_chip(struct fusb300_chip *chip)
{
	struct regmap *regmap = chip->map;
	unsigned int val;
	int ret;

	ret = regmap_write(chip->map, FUSB300_SOFT_POR_REG,
		       FUSB300_SOFT_POR | FUSB300_PD_POR);
	if (ret < 0) {
		dev_err(chip->dev, "error(%d) writing to reg:%x\n",
				ret, FUSB300_SOFT_POR_REG);
		return ret;
	}
	udelay(25);

	val = (FUSB300_PWR_BG_WKUP | FUSB300_PWR_BMC |
		 FUSB300_PWR_MEAS | FUSB300_PWR_OSC);
	ret = regmap_write(regmap, FUSB300_PWR_REG, val);
	if (ret < 0) {
		dev_err(chip->dev, "error(%d) writing to reg:%x\n",
				ret, FUSB300_PWR_REG);
		return ret;
	}

#ifdef DEBUG
	dump_registers(chip);
#endif

	ret = regmap_read(regmap, FUSB300_INT_REG, &val);
	if (ret < 0) {
		dev_err(chip->dev, "error(%d) reading reg:%x\n",
				ret, FUSB300_INT_REG);
		return ret;
	}
	dev_dbg(chip->dev, "init_chip int reg = %x", val);
	ret = regmap_read(regmap, FUSB300_STAT0_REG, &val);
	if (ret < 0) {
		dev_err(chip->dev, "error(%d) reading reg:%x\n",
				ret, FUSB300_STAT0_REG);
		return ret;
	}
	dev_dbg(chip->dev, "statreg = %x = %x", FUSB300_STAT0_REG, val);

	if (val & FUSB300_STAT0_VBUS_OK) {
		chip->i_vbus = true;
		/* Enable PullDown  */
		regmap_write(regmap, FUSB300_SWITCH0_REG,
				FUSB300_SWITCH0_PD_CC1_EN |
				FUSB300_SWITCH0_PD_CC2_EN);
		regmap_write(regmap, FUSB300_MEAS_REG, 0x31);
		/* for FPGA write different values
		regmap_write(regmap, FUSB300_MEAS_REG, 0x28);*/
	}
	/* for FPGA
	regmap_write(regmap, FUSB300_SWITCH0_REG, 0xa);
	regmap_write(regmap, FUSB300_MEAS_REG, 0x31);*/

	/* enable fast i2c */
	if (!chip->is_fusb300) {
		/* regmap_update_bits(regmap, FUSB300_CONTROL1_REG, */
		/* FUSB302_CONTROL1_FAST_I2C, FUSB302_CONTROL1_FAST_I2C); */
		fusb302_configure_pd(chip);
	}
	regmap_write(chip->map, FUSB300_SLICE_REG, 0x2A);

	return 0;
}

static void fusb300_handle_toggle_done(struct fusb300_chip *chip, int stat_reg)
{
	struct typec_phy *phy = &chip->phy;
	u8 tog_stat;

	/*
	 * disable the toggle state machine and setup the PD transmitter
	 * to the identified CC
	 */
	fusb302_enable_toggle(chip, false, FUSB302_TOG_MODE_DRP);
	tog_stat = FUSB302_TOG_STAT(stat_reg);

	if ((tog_stat == FUSB302_TOG_STAT_DFP_CC1) ||
		(tog_stat == FUSB302_TOG_STAT_DFP_CC2))  {
		mutex_lock(&chip->lock);
		phy->state = TYPEC_STATE_UNATTACHED_DFP;
		mutex_unlock(&chip->lock);
		atomic_notifier_call_chain(&phy->notifier,
				TYPEC_EVENT_DFP, phy);
	}

	/* according to TYPEC new spec UFP trigger will happen
	 * after VBUS and Rp terminations are seen
	 */
	if ((tog_stat == FUSB302_TOG_STAT_UFP_CC1) ||
		(tog_stat == FUSB302_TOG_STAT_UFP_CC2)) {
		mutex_lock(&chip->lock);
		phy->state = TYPEC_STATE_UNATTACHED_UFP;
		mutex_unlock(&chip->lock);
		atomic_notifier_call_chain(&phy->notifier,
				TYPEC_EVENT_VBUS, phy);
	}

	if (tog_stat == FUSB302_TOG_STAT_DFP_CC1 ||
		tog_stat == FUSB302_TOG_STAT_UFP_CC1) {
		phy->valid_cc = TYPEC_PIN_CC1;
		regmap_update_bits(chip->map, FUSB300_SWITCH1_REG,
			FUSB300_SWITCH1_TX_MASK, FUSB300_SWITCH1_TXCC1);
	} else {
		phy->valid_cc = TYPEC_PIN_CC2;
		regmap_update_bits(chip->map, FUSB300_SWITCH1_REG,
			FUSB300_SWITCH1_TX_MASK, FUSB300_SWITCH1_TXCC2);
	}
}

static void fusb300_handle_vbus_int(struct fusb300_chip *chip, int vbus_on)
{
	struct typec_phy *phy = &chip->phy;
	if (vbus_on) {
		chip->i_vbus = true;
		if (phy->state == TYPEC_STATE_UNATTACHED_DFP)
			complete(&chip->int_complete);

		if (chip->is_fusb300)
			atomic_notifier_call_chain(&phy->notifier,
				 TYPEC_EVENT_VBUS, phy);
		/* TOG_DONE will be used with FUSB302 */
	} else {
		chip->i_vbus = false;
		phy->valid_cc = 0;
		if (chip->phy.state != TYPEC_STATE_UNATTACHED_UFP) {
			atomic_notifier_call_chain(&phy->notifier,
					TYPEC_EVENT_NONE, phy);
			fusb300_wake_on_cc_change(chip);
		}
		fusb300_flush_fifo(phy, FIFO_TYPE_TX | FIFO_TYPE_RX);
	}
}

static irqreturn_t fusb300_interrupt(int id, void *dev)
{
	struct fusb300_chip *chip = dev;
	struct typec_phy *phy = &chip->phy;
	int ret;
	struct fusb300_int_stat int_stat;
	void *int_ptr;
	unsigned int reg_start;
	size_t count;

	memset(&int_stat, 0x00, sizeof(int_stat));
	pm_runtime_get_sync(chip->dev);

	if (chip->is_fusb300) {
		int_ptr = (void *) &int_stat.stat_reg;
		reg_start = FUSB300_STAT0_REG;
		count = FUSB300_MAX_INT_STAT;
	} else {
		int_ptr = (void *)&int_stat;
		reg_start = FUSB302_STAT0A_REG;
		count = FUSB302_MAX_INT_STAT;
	}

	ret = regmap_bulk_read(chip->map, reg_start, int_ptr, count);
	if (ret < 0) {
		dev_err(phy->dev, "bulk read reg failed %d", ret);
		pm_runtime_put_sync(chip->dev);
		return IRQ_NONE;
	}

	dev_dbg(chip->dev, "int %x stat %x stat1 = %x", int_stat.int_reg,
					int_stat.stat_reg, int_stat.stat1_reg);
	dev_dbg(chip->dev, "stat0a %x stat1a = %x", int_stat.stat0a_reg,
					int_stat.stat1a_reg);

	if (!chip->is_fusb300) {
		if (int_stat.inta_reg & FUSB302_INTA_TOG_DONE) {
			dev_dbg(phy->dev, "TOG_DONE INTR");
			fusb300_handle_toggle_done(chip, int_stat.stat1a_reg);
		}
	}

	if (int_stat.int_reg & FUSB300_INT_WAKE &&
		(phy->state == TYPEC_STATE_UNATTACHED_UFP ||
		phy->state == TYPEC_STATE_UNATTACHED_DFP)) {
		unsigned int val;

		regmap_read(chip->map, FUSB300_SWITCH0_REG, &val);

		if (((val & FUSB300_SWITCH0_PD_EN) == 0) &&
			((val & FUSB300_SWITCH0_PU_EN) == 0))
			atomic_notifier_call_chain(&phy->notifier,
				TYPEC_EVENT_DRP, phy);
		complete(&chip->int_complete);
	}

	if (int_stat.int_reg & FUSB300_INT_VBUS_OK) {
		fusb300_handle_vbus_int(chip,
			((int_stat.stat_reg & FUSB300_STAT0_VBUS_OK) ==
			FUSB300_STAT0_VBUS_OK));
	}


	if (int_stat.int_reg & (FUSB300_INT_COMP | FUSB300_INT_BC_LVL))
		complete(&chip->int_complete);


	if ((int_stat.int_reg & FUSB300_INT_COMP) &&
			(int_stat.stat_reg & FUSB300_STAT0_COMP)) {
		if ((phy->state == TYPEC_STATE_ATTACHED_UFP) ||
			(phy->state == TYPEC_STATE_ATTACHED_DFP)) {
			atomic_notifier_call_chain(&phy->notifier,
				 TYPEC_EVENT_NONE, phy);
			/*fusb300_wake_on_cc_change(chip);*/
			fusb300_flush_fifo(phy, FIFO_TYPE_TX | FIFO_TYPE_RX);
		}
	}

	if (chip->process_pd && (int_stat.int_reg & FUSB300_INT_ACTIVITY) &&
		(int_stat.int_reg & FUSB300_INT_COLLISION) &&
		!(int_stat.stat_reg & FUSB300_STAT0_ACTIVITY)) {
		mutex_lock(&chip->lock);
		chip->transmit = false;
		mutex_unlock(&chip->lock);
		if (phy->notify_protocol)
			phy->notify_protocol(phy, PROT_PHY_EVENT_COLLISION);
	}

	if (chip->process_pd && (int_stat.int_reg & FUSB300_INT_CRCCHK)) {
		if (phy->notify_protocol)
			phy->notify_protocol(phy, PROT_PHY_EVENT_MSG_RCV);
	}

	if (int_stat.int_reg & FUSB300_INT_ALERT) {
		if (int_stat.stat1_reg & FUSB300_STAT1_TXFULL) {
			dev_info(phy->dev, "alert int tx fifo full");
			mutex_lock(&chip->lock);
			regmap_update_bits(chip->map, FUSB300_CONTROL0_REG,
					(1<<6), (1<<6));
			mutex_unlock(&chip->lock);
		}
		if (int_stat.stat1_reg & FUSB300_STAT1_RXFULL) {
			dev_info(phy->dev, "alert int rx fifo full");
			mutex_lock(&chip->lock);
			regmap_update_bits(chip->map, FUSB300_CONTROL1_REG,
					(1<<2), (1<<2));
			mutex_unlock(&chip->lock);
		}
	}

	if (!chip->is_fusb300 && chip->process_pd) {
		if (int_stat.inta_reg & FUSB302_INTB_GCRC_SENT) {
			dev_dbg(phy->dev, "GoodCRC sent");
			if (phy->notify_protocol)
				phy->notify_protocol(phy,
						PROT_PHY_EVENT_GOODCRC_SENT);
		}
	}
	/* indication of activity means there is some transaction on CC
	 * FUSB302 has TXSENT interrupt  for TX completion
	 */
	if (chip->is_fusb300 && int_stat.int_reg & FUSB300_INT_ACTIVITY) {
		if (!(int_stat.stat_reg & FUSB300_STAT0_ACTIVITY) &&
			!(int_stat.int_reg & FUSB300_INT_CRCCHK)) {
			dev_info(phy->dev,
				"Activity happend and bus is idle tx complete");
			if (phy->notify_protocol)
				phy->notify_protocol(phy,
						PROT_PHY_EVENT_TX_SENT);
		}
	}
	if (!chip->is_fusb300 && chip->process_pd) {
		/* handle FUSB302 specific interrupt */

		if (int_stat.inta_reg & FUSB302_INTA_HARD_RST) {
			if (phy->notify_protocol)
				phy->notify_protocol(phy,
						PROT_PHY_EVENT_HARD_RST);

			regmap_update_bits(chip->map,
				FUSB300_SOFT_POR_REG, 2, 2);
		}
		if (int_stat.inta_reg & FUSB302_INTA_SOFT_RST) {
			/* flush fifo ? */
			if (phy->notify_protocol)
				phy->notify_protocol(phy,
						PROT_PHY_EVENT_SOFT_RST);
		}

		if (int_stat.inta_reg & FUSB302_INTA_TX_SENT) {
			dev_dbg(phy->dev,
				"Activity happend and bus is idle tx complete");
			if (phy->notify_protocol)
				phy->notify_protocol(phy,
						PROT_PHY_EVENT_TX_SENT);
		}

		if (int_stat.inta_reg & FUSB302_INTA_TX_RETRY_FAIL) {
			if (phy->notify_protocol)
				phy->notify_protocol(phy,
						PROT_PHY_EVENT_TX_FAIL);
		}

		if (int_stat.inta_reg & FUSB302_INTA_TX_SOFT_RST_FAIL) {
			if (phy->notify_protocol)
				phy->notify_protocol(phy,
						PROT_PHY_EVENT_SOFT_RST_FAIL);
		}

		if (int_stat.inta_reg & FUSB302_INTA_TX_HARD_RST) {
			if (phy->notify_protocol)
				phy->notify_protocol(phy,
						PROT_PHY_EVENT_TX_HARD_RST);
		}
	}

	pm_runtime_put_sync(chip->dev);

	return IRQ_HANDLED;
}

static int fusb300_phy_reset(struct typec_phy *phy)
{
	struct fusb300_chip *chip;
	bool chip_id;

	chip = dev_get_drvdata(phy->dev);
	chip_id = chip->is_fusb300;

	mutex_lock(&chip->lock);
	chip->activity_count = 0;
	chip->transmit = 0;

	if (chip->is_fusb300)
		fusb300_pd_send_hard_rst(phy);
	else
		fusb302_pd_send_hard_rst(phy);
	mutex_unlock(&chip->lock);
	return 0;
}

static int fusb300_flush_fifo(struct typec_phy *phy, enum typec_fifo fifo_type)
{
	struct fusb300_chip *chip;

	chip = dev_get_drvdata(phy->dev);

	mutex_lock(&chip->lock);
	chip->activity_count = 0;
	chip->transmit = 0;

	if (fifo_type & FIFO_TYPE_TX)
		/* TX FLUSH */
		regmap_update_bits(chip->map, FUSB300_CONTROL0_REG,
				0x40, 0x40);

	if (fifo_type & FIFO_TYPE_RX)
		/* RX FLUSH */
		regmap_update_bits(chip->map, FUSB300_CONTROL1_REG,
				0x4, 0x4);
	mutex_unlock(&chip->lock);

	return 0;
}

static inline int fusb300_frame_pkt(struct fusb300_chip *chip, u8 *pkt, int len)
{
#define MAX_FIFO_SIZE	64
	static u8 buffer[MAX_FIFO_SIZE];
	int i, j;


	for (i = 0; i < 3; i++)
		buffer[i] = SOP1;
	buffer[i++] = SOP2;

	buffer[i++] = PACKSYM | len;

	for (j = 0; j < len; i++, j++)
		buffer[i] = pkt[j];

	buffer[i++] = JAMCRC;
	buffer[i++] = EOP;
	buffer[i++] = TXOFF;
	buffer[i++] = TXON;

	dev_dbg(chip->dev, "%s: total bytes  = %d", __func__, i);
	return regmap_bulk_write(chip->map, FUSB300_FIFO_REG, buffer, i);
}


static inline int fusb300_send_pkt(struct typec_phy *phy, u8 *buf, int len)
{
	struct fusb300_chip *chip;

	chip = dev_get_drvdata(phy->dev);

	return fusb300_frame_pkt(chip, buf, len);
}

#ifdef DEBUG
static int fusb300_dump_rx_fifo(struct fusb300_chip *chip, u8 *buf, int len)
{
	struct typec_phy *phy = &chip->phy;
	int i;
	u16 *header = (u16 *)buf;
	u32 *data;

	dev_dbg(phy->dev, "header = %x", *header);
	if (len == 0)
		goto end;

	data = (u32 *) (buf + 2);

	for (i = 0; i < len; i++)
		dev_dbg(phy->dev, "data[%d] = %x", i, *data++);
end:
	return 0;
}
#endif

static inline int fusb300_recv_pkt(struct typec_phy *phy, u8 *buf)
{
#define FUSB302_DEF_PKT_SIZE 7 /* SOP(1), HEADER(2), CRC(4) */
#define PD_HEADER_SIZE 2
	struct fusb300_chip *chip;
	int len, bytecnt;
	static u8 buffer[MAX_FIFO_SIZE];
	u8 *header;
	u8 *fifo_ptr;

	chip = dev_get_drvdata(phy->dev);

	mutex_lock(&chip->lock);
	header = &buffer[1];
	fifo_ptr = &buffer[0];
	regmap_bulk_read(chip->map, FUSB300_FIFO_REG, (void *)fifo_ptr,
					(size_t)FUSB302_DEF_PKT_SIZE);
	fifo_ptr += FUSB302_DEF_PKT_SIZE;

	len = (header[1] >> 4) & 0x7;
	bytecnt = len * 4 + PD_HEADER_SIZE;

	if (bytecnt != 2)
		regmap_bulk_read(chip->map, FUSB300_FIFO_REG, (void *)fifo_ptr,
					(size_t)(bytecnt - PD_HEADER_SIZE));
	/* copy header + data, not the CRC */
	memcpy(buf, header, bytecnt);
	mutex_unlock(&chip->lock);


	return bytecnt;
}

static int fusb300_setup_role(struct typec_phy *phy, int data_role,
							int pwr_role)
{
	struct fusb300_chip *chip;
	int val;

	chip = dev_get_drvdata(phy->dev);

	regmap_read(chip->map, FUSB300_SWITCH1_REG, &val);

	val &= ~(FUSB302_SWITCH1_DATAROLE | FUSB302_SWITCH1_PWRROLE);

	if (data_role == PD_DATA_ROLE_DFP)
		val |= FUSB302_SWITCH1_DATAROLE;

	if (pwr_role == PD_POWER_ROLE_PROVIDER ||
		pwr_role == PD_POWER_ROLE_CONSUMER_PROVIDER)
		val |= FUSB302_SWITCH1_PWRROLE;

	return regmap_write(chip->map, FUSB300_SWITCH1_REG, val);
}

static inline int fusb300_pd_send_hard_rst(struct typec_phy *phy)
{
	static const u8 buf[6] = {RESET1, RESET1, RESET1, RESET2, TXOFF, TXON};

	struct fusb300_chip *chip;

	chip = dev_get_drvdata(phy->dev);
	return regmap_bulk_write(chip->map, FUSB300_FIFO_REG, buf, 6);
}

static inline int fusb302_pd_send_hard_rst(struct typec_phy *phy)
{
	struct fusb300_chip *chip;

	chip = dev_get_drvdata(phy->dev);

	return regmap_update_bits(chip->map, FUSB302_CONTROL3_REG,
				FUSB302_CONTROL3_SEND_HARD_RST,
				FUSB302_CONTROL3_SEND_HARD_RST);

}

static int fusb300_measure_cc(struct typec_phy *phy, enum typec_cc_pin pin,
				struct typec_cc_psy *cc_psy,
				unsigned long timeout)
{
	struct fusb300_chip *chip;
	int ret, s_comp, s_bclvl;
	unsigned int val, stat_reg;

	if (!phy) {
		cc_psy->v_rd = -1;
		return -ENODEV;
	}

	chip = dev_get_drvdata(phy->dev);
	timeout = msecs_to_jiffies(250);

	pm_runtime_get_sync(chip->dev);

	mutex_lock(&chip->lock);

	if (pin == TYPEC_PIN_CC1) {
		val = FUSB300_SWITCH0_MEASURE_CC1;
		if (phy->state == TYPEC_STATE_UNATTACHED_DFP)
			val |= FUSB300_SWITCH0_PU_CC1_EN;
		else
			val |= FUSB300_SWITCH0_PD_CC1_EN;
	} else {
		val = FUSB300_SWITCH0_MEASURE_CC2;
		if (phy->state == TYPEC_STATE_UNATTACHED_DFP)
			val |= FUSB300_SWITCH0_PU_CC2_EN;
		else
			val |= FUSB300_SWITCH0_PD_CC2_EN;
	}

	dev_dbg(phy->dev,
		"%s state %d unattached_dfp: %d switch0: %x val: %x\n",
		 __func__, phy->state, TYPEC_STATE_UNATTACHED_DFP,
		FUSB300_SWITCH0_REG, val);
	reinit_completion(&chip->int_complete);
	ret = regmap_write(chip->map, FUSB300_SWITCH0_REG, val);
	if (ret < 0)
		goto err_measure;

	if (!chip->is_fusb300) {
		/* after toggle, it takes 1 to 1.2ms to measure */
		usleep_range(1000, 1200);
		regmap_read(chip->map, FUSB300_STAT0_REG, &stat_reg);
		goto do_bclvl;
	}

	mutex_unlock(&chip->lock);

	ret = wait_for_completion_timeout(&chip->int_complete, timeout);
	if (ret == 0) {
		ret = -ETIME;
		goto err_measure;
	}

	mutex_lock(&chip->lock);
	regmap_read(chip->map, FUSB300_STAT0_REG, &stat_reg);

do_bclvl:
	dev_dbg(chip->dev, "STAT0_REG = %x\n",
				stat_reg);
	if ((stat_reg & FUSB300_STAT0_VBUS_OK) &&
		phy->state == TYPEC_STATE_UNATTACHED_DFP) {
		ret = -EPROTO;
		goto err;
	}
	s_comp = stat_reg & FUSB300_STAT0_COMP;
	s_bclvl = stat_reg & FUSB300_STAT0_BC_LVL_MASK;
	mutex_unlock(&chip->lock);

	if (!s_comp) {
		switch (s_bclvl) {
		case FUSB300_BC_LVL_VRA:
			cc_psy->v_rd = USB_TYPEC_CC_VRA;
			cc_psy->cur = 0;
			break;
		case FUSB300_BC_LVL_USB:
			cc_psy->v_rd = USB_TYPEC_CC_VRD_USB;
			cc_psy->cur = host_cur[1];
			break;
		case FUSB300_BC_LVL_1500:
			cc_psy->v_rd = USB_TYPEC_CC_VRD_1500;
			cc_psy->cur = host_cur[2];
			break;
		case FUSB300_BC_LVL_3000:
			cc_psy->v_rd = USB_TYPEC_CC_VRD_3000;
			cc_psy->cur = host_cur[3];
			break;
		}
	} else {
		dev_dbg(phy->dev, "chip->stat = %x s_comp %x",
				stat_reg, s_comp);
		cc_psy->v_rd = USB_TYPEC_CC_VRD_UNKNOWN; /* illegal */
		cc_psy->cur = TYPEC_CURRENT_UNKNOWN; /* illegal */
	}
	pm_runtime_put_sync(chip->dev);
	return 0;
err:
	mutex_unlock(&chip->lock);
err_measure:
	cc_psy->cur = TYPEC_CURRENT_UNKNOWN;
	cc_psy->v_rd = USB_TYPEC_CC_VRD_UNKNOWN;
	pm_runtime_put_sync(chip->dev);
	return ret;
}

static bool fusb300_pd_capable(struct typec_phy *phy)
{
	struct fusb300_chip *chip;

	chip = dev_get_drvdata(phy->dev);
	/* set the paltform is pd capable only if the phy is fusb302 */
	if (phy->type == USB_TYPE_C && !chip->is_fusb300)
		return true;
	else
		return false;
}

static int fusb300_pd_version(struct typec_phy *phy)
{
	if (phy->type == USB_TYPE_C)
		return USB_TYPEC_PD_VERSION;
	else
		return 0;
}

static int fusb300_get_irq(struct i2c_client *client)
{
	struct gpio_desc *gpio_desc;
	int irq;
	struct device *dev = &client->dev;

	if (client->irq > 0)
		return client->irq;

	gpio_desc = devm_gpiod_get_index(dev, "fusb300", 0);

	if (IS_ERR(gpio_desc))
		return client->irq;

	irq = gpiod_to_irq(gpio_desc);

	devm_gpiod_put(&client->dev, gpio_desc);

	return irq;
}

static int fusb300_wake_on_cc_change(struct fusb300_chip *chip)
{
	int val, ret;

	mutex_lock(&chip->lock);
	if (chip->is_fusb300) {
		val = FUSB300_SWITCH0_MEASURE_CC1 |
			FUSB300_SWITCH0_MEASURE_CC2;

		ret = regmap_write(chip->map, FUSB300_SWITCH0_REG, val);
		if (ret < 0) {
			dev_err(&chip->client->dev, "error(%d) writing %x\n",
					ret, FUSB300_SWITCH0_REG);
			mutex_unlock(&chip->lock);
			return ret;
		}
	} else {
		/* enable chip level toggle */
		ret = fusb302_enable_toggle(chip, true, FUSB302_TOG_MODE_DRP);
	}

	chip->phy.state = TYPEC_STATE_UNATTACHED_UFP;
	mutex_unlock(&chip->lock);

	return ret;
}

static struct regmap_config fusb300_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.val_format_endian = REGMAP_ENDIAN_NATIVE,
};

static inline int fusb302_enable_toggle(struct fusb300_chip *chip, bool en,
					int mode)
{
	unsigned int val;
	unsigned int mask;
	int ret;

	if (en) {
		val = (mode << FUSB302_CONTROL2_TOG_MODE_SHIFT) |
				 FUSB302_CONTROL2_TOGGLE_EN;
		mask = FUSB300_INT_MASK_COMP;
	} else {
		val = (FUSB302_TOG_MODE_UFP <<
			FUSB302_CONTROL2_TOG_MODE_SHIFT);
		mask = 0;
	}

	ret = regmap_write(chip->map, FUSB302_CONTROL2_REG, val);
	if (ret < 0)
		goto end;
	ret = regmap_update_bits(chip->map, FUSB300_INT_MASK_REG,
			FUSB300_INT_MASK_COMP, mask);
end:
	return ret;
}

static int fusb300_enable_autocrc(struct typec_phy *phy, bool en)
{
	struct fusb300_chip *chip;
	unsigned int val, int_mask;
	int ret;

	chip = dev_get_drvdata(phy->dev);
	if (chip->is_fusb300)
		return -ENOTSUPP;

	mutex_lock(&chip->lock);

	regmap_read(chip->map, FUSB300_SWITCH1_REG, &val);

	val &= ~(1<<2);

	if (en) {
		val |= FUSB302_SWITCH1_AUTOCRC;
		int_mask = FUSB300_INT_MASK_COMP;
	} else {
		int_mask = (FUSB300_INT_MASK_COMP |
			FUSB300_INT_MASK_ACTIVITY |
			FUSB300_INT_MASK_CRCCHK);
	}


	regmap_update_bits(chip->map, FUSB300_SOFT_POR_REG, 2, 2);

	mutex_unlock(&chip->lock);

	fusb300_flush_fifo(phy, FIFO_TYPE_TX);
	fusb300_flush_fifo(phy, FIFO_TYPE_RX);

	mutex_lock(&chip->lock);
	ret = regmap_write(chip->map, FUSB300_SWITCH1_REG, val);

	if (ret < 0)
		goto err;

	ret = regmap_write(chip->map, FUSB300_INT_MASK_REG, int_mask);
	if (ret < 0)
		goto err;

	chip->process_pd = en;

	mutex_unlock(&chip->lock);
	return ret;
err:
	chip->process_pd = false;
	mutex_unlock(&chip->lock);
	return ret;
}

static int fusb300_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct fusb300_chip *chip;
	int ret;
	unsigned int val, stat;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_WORD_DATA))
		return -EIO;

	chip = devm_kzalloc(&client->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	dev_dbg(&client->dev, "chip addr = %x", client->addr);
	chip->client = client;
	chip->dev = &client->dev;
	chip->map = devm_regmap_init_i2c(client, &fusb300_regmap_config);
	if (IS_ERR(chip->map)) {
		dev_err(&client->dev, "Failed to initialize regmap\n");
		return -EINVAL;
	}

	if (regmap_read(chip->map, FUSB30x_DEV_ID_REG, &val) < 0) {
		dev_err(&client->dev, "I2C read failed for ID-reg:%x\n",
				FUSB30x_DEV_ID_REG);
		return -EIO;
	}

	if (is_fusb300(val)) {
		chip->phy.support_drp_toggle = false;
		chip->is_fusb300 = true;
	} else {
		chip->phy.support_drp_toggle = true;
		chip->is_fusb300 = false;
		chip->phy.support_auto_goodcrc = true;
		chip->phy.support_retry = true;
	}
	dev_info(&client->dev, "ID-reg(%x)=%x, is_fusb300:%d\n",
		       FUSB30x_DEV_ID_REG, val, chip->is_fusb300);

	spin_lock_init(&chip->irqlock);
	chip->phy.dev = &client->dev;
	chip->phy.label = "fusb300";
	chip->phy.ops.measure_cc = fusb300_measure_cc;
	chip->phy.ops.set_host_current = fusb300_set_host_current;
	chip->phy.ops.get_host_current = fusb300_get_host_current;
	chip->phy.ops.switch_mode = fusb300_switch_mode;
	chip->phy.ops.setup_cc = fusb300_setup_cc;

	chip->phy.get_pd_version = fusb300_pd_version;
	chip->phy.is_pd_capable = fusb300_pd_capable;
	chip->phy.phy_reset = fusb300_phy_reset;
	chip->phy.flush_fifo = fusb300_flush_fifo;
	chip->phy.send_packet = fusb300_send_pkt;
	chip->phy.recv_packet = fusb300_recv_pkt;
	if (!chip->is_fusb300) {
		chip->phy.setup_role = fusb300_setup_role;
		chip->phy.enable_autocrc = fusb300_enable_autocrc;
	}

	if (IS_ENABLED(CONFIG_ACPI))
		client->irq = fusb300_get_irq(client);

	mutex_init(&chip->lock);
	init_completion(&chip->int_complete);
	i2c_set_clientdata(client, chip);

	typec_add_phy(&chip->phy);

	/* typec detect binding */
	typec_bind_detect(&chip->phy);

	fusb300_init_chip(chip);
	if (client->irq > 0) {
		u8 int_mask;

		int_mask = (chip->is_fusb300) ? 0 : (FUSB300_INT_MASK_COMP |
						FUSB300_INT_MASK_ACTIVITY |
						FUSB300_INT_MASK_CRCCHK);
		regmap_write(chip->map, FUSB300_INT_MASK_REG, int_mask);

		ret = devm_request_threaded_irq(&client->dev, client->irq,
				NULL, fusb300_interrupt,
				IRQF_ONESHOT | IRQF_TRIGGER_LOW,
				client->name, chip);
		if (ret < 0) {
			dev_err(&client->dev,
				"error registering interrupt %d", ret);
			return -EIO;
		}
	}


	regmap_read(chip->map, FUSB300_CONTROL0_REG, &val);
	val &= ~FUSB300_CONTROL0_MASK_INT;
	regmap_write(chip->map, FUSB300_CONTROL0_REG, val);

	syspolicy_register_typec_phy(&chip->phy);
	if (!chip->i_vbus) {
		fusb300_wake_on_cc_change(chip);
		regmap_read(chip->map, FUSB300_STAT0_REG, &stat);

		if (stat &  FUSB300_STAT0_WAKE) {
			if (chip->is_fusb300)
				atomic_notifier_call_chain(&chip->phy.notifier,
					TYPEC_EVENT_DRP, &chip->phy);
			else
				 atomic_notifier_call_chain(&chip->phy.notifier,
					TYPEC_EVENT_DFP, &chip->phy);
		}

	} else
		atomic_notifier_call_chain(&chip->phy.notifier,
				TYPEC_EVENT_VBUS, &chip->phy);

	return 0;
}

static int fusb300_remove(struct i2c_client *client)
{
	struct fusb300_chip *chip = i2c_get_clientdata(client);
	struct typec_phy *phy = &chip->phy;

	syspolicy_unregister_typec_phy(phy);
	typec_unbind_detect(phy);
	typec_remove_phy(phy);
	return 0;
}

static int fusb300_suspend(struct device *dev)
{
	return 0;
}

static int fusb300_resume(struct device *dev)
{
	return 0;
}

static int fusb300_late_suspend(struct device *dev)
{
	struct fusb300_chip *chip;

	chip = dev_get_drvdata(dev);

	/* Disable the irq during suspend to prevent fusb300
	isr executed before the i2c controller resume.*/
	if (chip->client->irq) {
		disable_irq(chip->client->irq);
		enable_irq_wake(chip->client->irq);
	}

	return 0;
}

static int fusb300_early_resume(struct device *dev)
{
	struct fusb300_chip *chip;

	chip = dev_get_drvdata(dev);

	/* Enable the irq after resume to prevent fusb300
	isr executed before the i2c controller resume.*/
	if (chip->client->irq) {
		disable_irq_wake(chip->client->irq);
		enable_irq(chip->client->irq);
	}

	return 0;
}

static int fusb300_runtime_suspend(struct device *dev)
{
	return 0;
}

static int fusb300_runtime_idle(struct device *dev)
{
	return 0;
}

static int fusb300_runtime_resume(struct device *dev)
{
	return 0;
}

static const struct dev_pm_ops fusb300_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(fusb300_suspend,
			fusb300_resume)
	SET_RUNTIME_PM_OPS(fusb300_runtime_suspend,
			fusb300_runtime_resume,
			fusb300_runtime_idle)
	SET_LATE_SYSTEM_SLEEP_PM_OPS(fusb300_late_suspend,
			fusb300_early_resume)
};


#ifdef CONFIG_ACPI
static struct acpi_device_id fusb300_acpi_match[] = {
	{"FUSB0300", 0},
};
MODULE_DEVICE_TABLE(acpi, fusb300_acpi_match);
#endif

static const struct i2c_device_id fusb300_id[] = {
	{ "FUSB0300", 0 },
	{ "FUSB0300:00", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, fusb300_id);

static struct i2c_driver fusb300_i2c_driver = {
	.driver	= {
		.name	= "fusb300",
#ifdef CONFIG_ACPI
		.acpi_match_table = ACPI_PTR(fusb300_acpi_match),
#endif
		.pm	= &fusb300_pm_ops,
	},
	.probe		= fusb300_probe,
	.remove		= fusb300_remove,
	.id_table	= fusb300_id,
};
module_i2c_driver(fusb300_i2c_driver);

MODULE_AUTHOR("Kannappan, R r.kannappan@intel.com");
MODULE_DESCRIPTION("FUSB300 usb phy for TYPE-C & PD");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("i2c:fusb300");
