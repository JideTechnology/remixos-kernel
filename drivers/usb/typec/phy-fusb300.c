/*
 * Copyright (C) 2014 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *
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
#include <linux/of.h>
#include <linux/regmap.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/acpi.h>
#include <linux/pm_runtime.h>
#include <linux/notifier.h>
#include <linux/suspend.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/wakelock.h>
#include <linux/usb_typec_phy.h>
#include "usb_typec_detect.h"

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
#define FUSB300_HOST_CUR(x)		(((x) >> FUSB300_HOST_CUR_SHIFT) \
						& FUSB300_HOST_CUR_MASK)
#define FUSB300_HOST_CUR_DISABLE	0
#define FUSB300_HOST_CUR_USB_SDP	1
#define FUSB300_HOST_CUR_1500		2
#define FUSB300_HOST_CUR_3000		3

#define FUSB300_CONTROL1_REG		0x7
#define FUSB300_CONTROL1_RX_FLUSH	BIT(1)
#define FUSB300_CONTROL1_BIST_MODE	BIT(3)

#define FUSB300_SOFT_POR_REG		0xc
#define FUSB300_SOFT_POR		BIT(1)

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

#define FUSB300_COMP_RD_LOW		0x24
#define FUSB300_COMP_RD_HIGH		0x3e
#define FUSB300_COMP_RA_LOW		0xa
#define FUSB300_COMP_RA_HIGH		0x12

static int host_cur[4] = {
	TYPEC_CURRENT_UNKNOWN,
	TYPEC_CURRENT_500,
	TYPEC_CURRENT_1500,
	TYPEC_CURRENT_3000
};

enum {
	FUSB300_CC1_PIN = 1,
	FUSB300_CC2_PIN = 2
};

struct fusb300_chip {
	struct i2c_client *client;
	struct device *dev;
	struct regmap *map;
	struct fusb300_platform_data *pdata;
	struct mutex lock;
	struct typec_phy phy;
	bool i_comp;
	bool i_vbus;
	u32 stored_int_reg;
	unsigned int mdac;
	/* wait_queue_head_t wait_queue; */
	struct completion int_complete;
	spinlock_t irqlock;
	/* TODO: Revert me */
	struct wake_lock wakelock;
};

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
		return TYPEC_CURRENT_UNKNOWN;

	chip = dev_get_drvdata(phy->dev);
	for (i = 0; i < 4; i++) {
		if (host_cur[i] == cur)
			break;
	}
	if (i >= 4)
		i = 0;
	mutex_lock(&chip->lock);
	regmap_read(chip->map, FUSB300_CONTROL0_REG, &val);
	val &= ~(FUSB300_HOST_CUR_MASK << FUSB300_HOST_CUR_SHIFT);
	val |= (i << FUSB300_HOST_CUR_SHIFT);
	dev_dbg(phy->dev, "control0 reg = %x cur = %d i = %d", val, cur, i);
	ret = regmap_write(chip->map, FUSB300_CONTROL0_REG, val);
#if 0
	ret = regmap_update_bits(chip->map, FUSB300_CONTROL0_REG,
					FUSB300_HOST_CUR_MASK,
					(i << FUSB300_HOST_CUR_SHIFT));
#endif
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
	int ret, reg = FUSB300_SWITCH0_REG;

	if (en_pu) {
		mutex_lock(&chip->lock);
		val &= ~FUSB300_SWITCH0_PD_EN;
		val |= FUSB300_SWITCH0_PU_EN;
		mutex_unlock(&chip->lock);
	} else {
		val &= ~FUSB300_SWITCH0_PU_EN;
	}
	dev_dbg(chip->dev, "%s: switch0 %x = %x", __func__,
					FUSB300_SWITCH0_REG, val);
	ret = fusb300_set_host_current(&chip->phy, cur);
	if (ret < 0) {
		dev_err(&chip->client->dev,
			"error setting host cur%d", ret);
		return ret;
	}
	mutex_lock(&chip->lock);
	ret = regmap_write(chip->map, reg, val);
	mutex_unlock(&chip->lock);
	if (ret < 0)
		dev_err(&chip->client->dev, "error write (%d) %d", ret, reg);

	return ret;
}

static int fusb300_en_pd(struct fusb300_chip *chip, bool en_pd)
{
	unsigned int val = 0;
	int ret, reg = FUSB300_SWITCH0_REG;

	mutex_lock(&chip->lock);
	if (en_pd) {
		val |= FUSB300_SWITCH0_PD_EN;
		val &= ~FUSB300_SWITCH0_PU_EN;
	} else {
		val &= ~FUSB300_SWITCH0_PD_EN;
	}
	dev_dbg(chip->dev, "%s: switch0 %x = %x", __func__,
					FUSB300_SWITCH0_REG, val);
	ret = regmap_write(chip->map, reg, val);
	mutex_unlock(&chip->lock);
	if (ret < 0)
		dev_err(&chip->client->dev, "error write (%d) %d", ret, reg);
	return ret;
}

static int fusb300_switch_mode(struct typec_phy *phy, enum typec_mode mode)
{
	struct fusb300_chip *chip;
	unsigned int val;
	int cur;

	if (!phy)
		return -ENODEV;

	dev_dbg(phy->dev, "%s: %d", __func__, mode);

	chip = dev_get_drvdata(phy->dev);

	if (mode == TYPEC_MODE_UFP) {
		fusb300_set_host_current(phy, 0);
		fusb300_en_pd(chip, true);
		mutex_lock(&chip->lock);
		chip->mdac = 0x31;
		phy->state = TYPEC_STATE_UNATTACHED_UFP;
		regmap_write(chip->map, FUSB300_MEAS_REG, 0x31);
		mutex_unlock(&chip->lock);
	} else if (mode == TYPEC_MODE_DFP) {
		cur = TYPEC_CURRENT_500;
		mutex_lock(&chip->lock);
		phy->state = TYPEC_STATE_UNATTACHED_DFP;
		if (cur == TYPEC_CURRENT_3000) {
			chip->mdac = 0x3E;
			regmap_write(chip->map, FUSB300_MEAS_REG, 0x3E);
		} else {
			chip->mdac = 0x26;
			regmap_write(chip->map, FUSB300_MEAS_REG, 0x26);
		}
		mutex_unlock(&chip->lock);
		fusb300_en_pu(chip, false, cur);
	} else if (mode == TYPEC_MODE_DRP) {
		fusb300_en_pd(chip, false);
		fusb300_en_pu(chip, false, 0);
		mutex_lock(&chip->lock);
		regmap_read(chip->map, FUSB300_SWITCH0_REG, &val);
		val |= (FUSB300_SWITCH0_MEASURE_CC1 |
				FUSB300_SWITCH0_MEASURE_CC2);
		regmap_write(chip->map, FUSB300_SWITCH0_REG, val);
		mutex_unlock(&chip->lock);
	}
	return 0;
}

static int fusb300_enable_pullup(struct typec_phy *phy)
{
	struct fusb300_chip *chip;
	int cur;

	if (!phy)
		return -ENODEV;

	chip = dev_get_drvdata(phy->dev);
	mutex_lock(&chip->lock);
	cur = fusb300_get_host_current(phy);
	if (cur == 3000) {
		chip->mdac = 0x3E;
		regmap_write(chip->map, FUSB300_MEAS_REG, 0x3E);
	} else {
		chip->mdac = 0x26;
		regmap_write(chip->map, FUSB300_MEAS_REG, 0x26);
	}
	mutex_unlock(&chip->lock);
	fusb300_en_pu(chip, false, cur);
	return 0;
}

static int fusb300_enable_pulldown(struct typec_phy *phy)
{
	struct fusb300_chip *chip;

	if (!phy)
		return -ENODEV;

	chip = dev_get_drvdata(phy->dev);
	fusb300_en_pd(chip, true);
	mutex_lock(&chip->lock);
	chip->mdac = 0x31;
	regmap_write(chip->map, FUSB300_MEAS_REG, 0x31);
	mutex_unlock(&chip->lock);
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
		phy->state = state;
		break;
	case TYPEC_STATE_UNKNOWN:
	case TYPEC_STATE_POWERED:
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

static int fusb300_init_chip(struct fusb300_chip *chip)
{
	struct regmap *regmap = chip->map;
	unsigned int val, ret;
	struct typec_phy *phy = &chip->phy;

	regmap_write(chip->map, FUSB300_SOFT_POR_REG, 1);
	udelay(25);

	ret = regmap_write(regmap, FUSB300_PWR_REG, 7);
	if (ret < 0) {
		dev_err(chip->dev, "error writing to pwr reg");
		return -1;
	}

#ifdef DEBUG
	dump_registers(chip);
#endif

	ret = regmap_read(regmap, FUSB300_INT_REG, &val);
	dev_info(chip->dev, "init_chip int reg = %x", val);
	ret = regmap_read(regmap, FUSB300_STAT0_REG, &val);
	if (ret < 0) {
		dev_err(chip->dev, "error writing to pwr reg");
		return -1;
	}
	dev_info(chip->dev, "statreg = %x = %x", FUSB300_STAT0_REG, val);

	if (val & FUSB300_STAT0_VBUS_OK) {
		chip->i_vbus = true;
		chip->mdac = 0x31;
		regmap_write(regmap, FUSB300_SWITCH0_REG, 3); /* Enable PD  */
		regmap_write(regmap, FUSB300_MEAS_REG, 0x31);
	}

	return 0;
}


static int fusb300_read_reg(struct fusb300_chip *chip, u8 reg, u32 *val)
{
	int i, ret;

	for (i = 0; i < 3; i++) {
		ret = regmap_read(chip->map, reg, val);
		if (ret < 0) {
			*val = -1;
			continue;
		} else
			break;
	}
	return ret;
}

static irqreturn_t fusb300_interrupt(int id, void *dev)
{
	struct fusb300_chip *chip = dev;
	struct typec_phy *phy = &chip->phy;
	unsigned int int_reg, stat_reg;
	unsigned long flags;
	unsigned int mdac;
	int ret;

	ret = fusb300_read_reg(chip, FUSB300_INT_REG, &int_reg);
	if (ret < 0) {
		dev_err(phy->dev, "read reg %x failed %d",
					FUSB300_INT_REG, ret);
		return IRQ_HANDLED;
	}
#if 0
	regmap_read(chip->map, FUSB300_INT_REG, &int_reg);
#endif
	regmap_read(chip->map, FUSB300_STAT0_REG, &stat_reg);

	dev_dbg(chip->dev, "int %x stat %x", int_reg, stat_reg);

	if (int_reg & FUSB300_INT_VBUS_OK) {
		if (stat_reg & FUSB300_STAT0_VBUS_OK) {
			chip->i_vbus = true;
			if (phy->state == TYPEC_STATE_UNATTACHED_DFP) {
				mutex_lock(&chip->lock);
				chip->i_comp = true;
				mutex_unlock(&chip->lock);
				/* wake_up(&chip->wait_queue); */
				complete(&chip->int_complete);
			}
			atomic_notifier_call_chain(&phy->notifier,
				 TYPEC_EVENT_VBUS, phy);
		} else {
			chip->i_vbus = false;
			atomic_notifier_call_chain(&phy->notifier,
				 TYPEC_EVENT_NONE, phy);
		}
	}

	if (int_reg & FUSB300_INT_WAKE &&
				(phy->state == TYPEC_STATE_UNATTACHED_UFP ||
				phy->state == TYPEC_STATE_UNATTACHED_DFP)) {
		/* something is trying to attach. if measurement is */
		/* happening finish it */
		/* TODO: change to completion instead of wait event */
		mutex_lock(&chip->lock);
		chip->i_comp = true;
		mutex_unlock(&chip->lock);
		complete(&chip->int_complete);
		/* wake_up(&chip->wait_queue); */
	}

	if (int_reg & (FUSB300_INT_COMP | FUSB300_INT_BC_LVL)) {
		mutex_lock(&chip->lock);
		chip->i_comp = true;
		mutex_unlock(&chip->lock);
		complete(&chip->int_complete);
		/* wake_up(&chip->wait_queue); */
	}


	if ((int_reg & FUSB300_INT_COMP) &&
			(stat_reg & FUSB300_STAT0_COMP)) {
		if ((phy->state == TYPEC_STATE_ATTACHED_UFP) ||
			(phy->state == TYPEC_STATE_ATTACHED_DFP)) {
			atomic_notifier_call_chain(&phy->notifier,
				 TYPEC_EVENT_NONE, phy);
		}
	}

	if (int_reg & (FUSB300_INT_COMP | FUSB300_INT_BC_LVL)) {
		mutex_lock(&chip->lock);
		chip->i_comp = true;
		mutex_unlock(&chip->lock);
		complete(&chip->int_complete);
		/* wake_up(&chip->wait_queue); */
	}

	return IRQ_HANDLED;
}

static int fusb300_measure_cc(struct typec_phy *phy, int pin,
				struct typec_cc_psy *cc_psy,
				unsigned long timeout)
{
	struct fusb300_chip *chip;
	int ret, s_comp, s_bclvl;
	unsigned int val, stat_reg;

	if (!phy) {
		cc_psy->v_rd = -1;
		return -1;
	}

	chip = dev_get_drvdata(phy->dev);
	timeout = msecs_to_jiffies(150);

	pm_runtime_get_sync(chip->dev);

	mutex_lock(&chip->lock);
	chip->i_comp = false;

	regmap_read(chip->map, FUSB300_SWITCH0_REG, &val);

	if (pin == TYPEC_PIN_CC1) {
		val |= FUSB300_SWITCH0_MEASURE_CC1;
		val &= ~FUSB300_SWITCH0_MEASURE_CC2;
		if (phy->state == TYPEC_STATE_UNATTACHED_DFP) {
			val &= ~FUSB300_SWITCH0_PU_CC2_EN;
			val |= FUSB300_SWITCH0_PU_CC1_EN;
		}
	} else {
		val |= FUSB300_SWITCH0_MEASURE_CC2;
		val &= ~FUSB300_SWITCH0_MEASURE_CC1;
		if (phy->state == TYPEC_STATE_UNATTACHED_DFP) {
			val &= ~FUSB300_SWITCH0_PU_CC1_EN;
			val |= FUSB300_SWITCH0_PU_CC2_EN;
		}
	}

	dev_dbg(phy->dev,
	"%s: state = %d unattached_dfp = %d switch0_reg = %x val = %x i_comp = %d",
					 __func__,
					phy->state, TYPEC_STATE_UNATTACHED_DFP,
					FUSB300_SWITCH0_REG, val, chip->i_comp);
	ret = regmap_write(chip->map, FUSB300_SWITCH0_REG, val);
	if (ret < 0)
		goto err_measure;
	reinit_completion(&chip->int_complete);

	mutex_unlock(&chip->lock);

#if 0
	usleep_range(3000, 200000);
	wait_event(chip->wait_queue, (chip->i_comp == true));
#endif
#if 1
	ret = wait_for_completion_timeout(&chip->int_complete, timeout);
#if 0
	ret = wait_event_timeout(chip->wait_queue, (chip->i_comp == true),
						timeout);
#endif

	if (ret == 0)
		goto err_measure;
#endif


	mutex_lock(&chip->lock);
	regmap_read(chip->map, FUSB300_STAT0_REG, &stat_reg);
	dev_dbg(chip->dev, "STAT0_REG = %x i_comp  = %d",
					stat_reg, chip->i_comp);
	if ((stat_reg & FUSB300_STAT0_VBUS_OK) &&
			phy->state == TYPEC_STATE_UNATTACHED_DFP)
		goto err_measure;
	s_comp = stat_reg & FUSB300_STAT0_COMP;
	s_bclvl = stat_reg & FUSB300_STAT0_BC_LVL_MASK;
	mutex_unlock(&chip->lock);

	if (!s_comp) {
		switch (s_bclvl) {
		case FUSB300_BC_LVL_VRA:
			cc_psy->v_rd = TYPEC_CC_VRA;
			cc_psy->cur = 0;
			break;
		case FUSB300_BC_LVL_USB:
			cc_psy->v_rd = TYPEC_CC_VRD_USB;
			cc_psy->cur = host_cur[1];
			break;
		case FUSB300_BC_LVL_1500:
			cc_psy->v_rd = TYPEC_CC_VRD_1500;
			cc_psy->cur = host_cur[2];
			break;
		case FUSB300_BC_LVL_3000:
			cc_psy->v_rd = TYPEC_CC_VRD_3000;
			cc_psy->cur = host_cur[3];
			break;
		}
	} else {
		dev_dbg(phy->dev, "chip->stat = %x s_comp %x",
				stat_reg, s_comp);
		cc_psy->v_rd = -1; /* illegal */
		cc_psy->cur = -1; /* illegal */
	}
	pm_runtime_put_sync(chip->dev);
	return 1;

err_measure:
	cc_psy->cur = -1;
	cc_psy->v_rd = -1;
	mutex_unlock(&chip->lock);
	pm_runtime_put_sync(chip->dev);
	return -1;
}

static int fusb300_pd_capable(struct typec_phy *phy)
{
	return 1;
}

static int fusb300_pd_version(struct typec_phy *phy)
{
	return 2;
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

	gpiod_put(gpio_desc);

	return irq;
}

static struct regmap_config fusb300_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.val_format_endian = REGMAP_ENDIAN_NATIVE,
};

static int fusb300_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct fusb300_chip *chip;
	int ret;
	unsigned int val;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_WORD_DATA))
		return -EIO;

	chip = devm_kzalloc(&client->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	dev_info(&client->dev, "chip addr = %x", client->addr);

	chip->client = client;
	chip->dev = &client->dev;
	chip->map = devm_regmap_init_i2c(client, &fusb300_regmap_config);
	if (IS_ERR(chip->map)) {
		dev_err(&client->dev, "Failed to initialize regmap\n");
		return -EINVAL;
	}

	spin_lock_init(&chip->irqlock);
	chip->phy.dev = &client->dev;
	chip->phy.label = "fusb300";
	chip->phy.ops.measure_cc = fusb300_measure_cc;
	chip->phy.ops.set_host_current = fusb300_set_host_current;
	chip->phy.ops.get_host_current = fusb300_get_host_current;
	chip->phy.ops.switch_mode = fusb300_switch_mode;
	chip->phy.ops.enable_cc_pullup = fusb300_enable_pullup;
	chip->phy.ops.enable_cc_pulldown = fusb300_enable_pulldown;
	chip->phy.ops.setup_cc = fusb300_setup_cc;

	chip->phy.is_pd_capable = fusb300_pd_capable;
	chip->phy.get_pd_version = fusb300_pd_version;

	if (IS_ENABLED(CONFIG_ACPI))
		client->irq = fusb300_get_irq(client);

	mutex_init(&chip->lock);
	wake_lock_init(&chip->wakelock, WAKE_LOCK_SUSPEND, "typc_wakelock");
	/* init_waitqueue_head(&chip->wait_queue);*/
	init_completion(&chip->int_complete);
	i2c_set_clientdata(client, chip);

	wake_lock(&chip->wakelock);

	typec_add_phy(&chip->phy);
	typec_bind_detect(&chip->phy);


	fusb300_init_chip(chip);

	if (client->irq > 0) {
		ret = request_threaded_irq(client->irq,
				NULL, fusb300_interrupt,
				IRQF_ONESHOT | IRQF_TRIGGER_LOW |
				IRQF_NO_SUSPEND,
				client->name, chip);
		if (ret < 0) {
			dev_err(&client->dev,
				"error registering interrupt %d", ret);
			wake_lock_destroy(&chip->wakelock);
			return -EIO;
		}
		regmap_write(chip->map, FUSB300_INT_MASK_REG, 0);
	}

	regmap_read(chip->map, FUSB300_CONTROL0_REG, &val);
	val &= ~FUSB300_CONTROL0_MASK_INT;
	regmap_write(chip->map, FUSB300_CONTROL0_REG, val);

	if (!chip->i_vbus)
		atomic_notifier_call_chain(&chip->phy.notifier,
					TYPEC_EVENT_DRP, &chip->phy);
	else
		atomic_notifier_call_chain(&chip->phy.notifier,
					TYPEC_EVENT_VBUS, &chip->phy);

	return 0;
}

static int fusb300_remove(struct i2c_client *client)
{
	struct fusb300_chip *chip = i2c_get_clientdata(client);
	struct typec_phy *phy = &chip->phy;

	typec_unbind_detect(&chip->phy);
	typec_remove_phy(phy);
	if (client->irq)
		free_irq(client->irq, chip);
	wake_lock_destroy(&chip->wakelock);
	return 0;
}

static int fusb300_suspend(struct device *dev)
{
	struct fusb300_chip *chip;
	struct typec_phy *phy;
	unsigned int val;

	dev_info(dev, "going to suspend %s", __func__);
	chip = dev_get_drvdata(dev);
	phy = &chip->phy;

	atomic_notifier_call_chain(&phy->notifier,
			TYPEC_EVENT_DEV_SUSPEND, phy);
	if (phy->state == TYPEC_STATE_UNKNOWN ||
		phy->state == TYPEC_STATE_UNATTACHED_UFP ||
		phy->state == TYPEC_STATE_UNATTACHED_DFP) { /* unattached */
		fusb300_en_pd(chip, false);
		fusb300_en_pu(chip, false, 0);
		mutex_lock(&chip->lock);
		regmap_read(chip->map, FUSB300_SWITCH0_REG, &val);
		val |= (FUSB300_SWITCH0_MEASURE_CC1 |
				FUSB300_SWITCH0_MEASURE_CC2);
		regmap_write(chip->map, FUSB300_SWITCH0_REG, val);
		mutex_unlock(&chip->lock);
	}
	return 0;
}

static int fusb300_resume(struct device *dev)
{
	struct fusb300_chip *chip;
	struct typec_phy *phy;
	unsigned int stat_reg;

	chip = dev_get_drvdata(dev);
	phy = &chip->phy;
	regmap_read(chip->map, FUSB300_STAT0_REG, &stat_reg);
	dev_info(phy->dev, "%s: stat = %x", stat_reg);
	if (!(stat_reg & FUSB300_STAT0_WAKE))
		atomic_notifier_call_chain(&phy->notifier,
			TYPEC_EVENT_DEV_RESUME, phy);
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
MODULE_DESCRIPTION("FUSB300 TYPE C PD");
MODULE_LICENSE("GPL");
MODULE_ALIAS("i2c:fusb300");
