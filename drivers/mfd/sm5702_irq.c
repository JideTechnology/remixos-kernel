/* drivers/mfd/sm5702_irq.c
 * SM5702 Multifunction Device Driver
 * Charger / Buck / LDOs / FlashLED
 * *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */
#include <linux/mfd/sm5702.h>
#include <linux/mfd/sm5702_irq.h>
#include <linux/gpio.h>
#include <linux/device.h>

#define ALIAS_NAME "sm5702_mfd_irq"

#define SM5702_INT1_CTRL     SM5702_INTMSK1
#define SM5702_INT2_CTRL     SM5702_INTMSK2
#define SM5702_INT3_CTRL     SM5702_INTMSK3
#define SM5702_INT4_CTRL     SM5702_INTMSK4


#define IRQ_NAME(irq) [irq] = irq##_NAME
static const char *sm5702_irq_names[] = {
	IRQ_NAME(SM5702_NOBAT_IRQ),
    IRQ_NAME(SM5702_CHGRSTF_IRQ),
    IRQ_NAME(SM5702_VBUSLIMIT_IRQ),
    IRQ_NAME(SM5702_VBUSOVP_IRQ),
    IRQ_NAME(SM5702_VBUSUVLO_IRQ),
    IRQ_NAME(SM5702_VBUSPOK_IRQ),
    IRQ_NAME(SM5702_BATOVP_IRQ),
    IRQ_NAME(SM5702_AICL_IRQ),
    IRQ_NAME(SM5702_VSYSOLP_IRQ),
    IRQ_NAME(SM5702_VSYSNG_IRQ),
    IRQ_NAME(SM5702_VSYSOK_IRQ),
    IRQ_NAME(SM5702_OTGFAIL_IRQ),
    IRQ_NAME(SM5702_PRETMROFF_IRQ),
    IRQ_NAME(SM5702_FASTTMROFF_IRQ),
    IRQ_NAME(SM5702_DONE_IRQ),
    IRQ_NAME(SM5702_TOPOFF_IRQ),
    IRQ_NAME(SM5702_LOWBATT_IRQ),
    IRQ_NAME(SM5702_ABSTMRON_IRQ),
    IRQ_NAME(SM5702_BOOSTPOK_IRQ),
    IRQ_NAME(SM5702_BOOSTPOK_NG_IRQ),
    IRQ_NAME(SM5702_FLEDOPEN_IRQ),
    IRQ_NAME(SM5702_FLEDSHORT_IRQ),
    IRQ_NAME(SM5702_THEMREG_IRQ),
    IRQ_NAME(SM5702_THEMSHDN_IRQ),
    IRQ_NAME(SM5702_DISLIMIT_IRQ),
    IRQ_NAME(SM5702_DCDOUT_SDP_IRQ),
    IRQ_NAME(SM5702_SDP_IRQ),
    IRQ_NAME(SM5702_CDP_IRQ),
    IRQ_NAME(SM5702_DCP_IRQ),
    
};

const char *sm5702_get_irq_name_by_index(int index)
{
	return sm5702_irq_names[index];
}
EXPORT_SYMBOL(sm5702_get_irq_name_by_index);

enum SM5702_IRQ_OFFSET {
	SM5702_INT1_OFFSET = 0,
	SM5702_INT2_OFFSET,
	SM5702_INT3_OFFSET,
	SM5702_INT4_OFFSET,
};
	

struct sm5702_irq_data {
	int mask;
	int offset;
};

static const u8 sm5702_mask_reg[] = {
	SM5702_INT1_CTRL,
	SM5702_INT2_CTRL,
	SM5702_INT3_CTRL,
	SM5702_INT4_CTRL,
};

#define DECLARE_IRQ_CTRL(idx, _offset, _mask)		\
	[(idx)] = { .offset = (_offset), .mask = (_mask) }

static const struct sm5702_irq_data sm5702_irqs[] = {
	DECLARE_IRQ_CTRL(SM5702_NOBAT_IRQ, SM5702_INT1_OFFSET, 1<<7),
	DECLARE_IRQ_CTRL(SM5702_CHGRSTF_IRQ, SM5702_INT1_OFFSET, 1<<6),
	DECLARE_IRQ_CTRL(SM5702_VBUSLIMIT_IRQ, SM5702_INT1_OFFSET, 1<<5),
	DECLARE_IRQ_CTRL(SM5702_VBUSOVP_IRQ, SM5702_INT1_OFFSET, 1<<4),
	DECLARE_IRQ_CTRL(SM5702_VBUSUVLO_IRQ, SM5702_INT1_OFFSET, 1<<3),
	DECLARE_IRQ_CTRL(SM5702_VBUSPOK_IRQ, SM5702_INT1_OFFSET, 1<<2),
	DECLARE_IRQ_CTRL(SM5702_BATOVP_IRQ, SM5702_INT1_OFFSET, 1<<1),
	DECLARE_IRQ_CTRL(SM5702_AICL_IRQ, SM5702_INT1_OFFSET, 1<<0),
	
	DECLARE_IRQ_CTRL(SM5702_VSYSOLP_IRQ, SM5702_INT2_OFFSET, 1<<7),
    DECLARE_IRQ_CTRL(SM5702_VSYSNG_IRQ, SM5702_INT2_OFFSET, 1<<6),
	DECLARE_IRQ_CTRL(SM5702_VSYSOK_IRQ, SM5702_INT2_OFFSET, 1<<5),
	DECLARE_IRQ_CTRL(SM5702_OTGFAIL_IRQ, SM5702_INT2_OFFSET, 1<<4),
	DECLARE_IRQ_CTRL(SM5702_PRETMROFF_IRQ, SM5702_INT2_OFFSET, 1<<3),
	DECLARE_IRQ_CTRL(SM5702_FASTTMROFF_IRQ, SM5702_INT2_OFFSET, 1<<2),
	DECLARE_IRQ_CTRL(SM5702_DONE_IRQ, SM5702_INT2_OFFSET, 1<<1),
	DECLARE_IRQ_CTRL(SM5702_TOPOFF_IRQ, SM5702_INT2_OFFSET, 1<<0),

    DECLARE_IRQ_CTRL(SM5702_LOWBATT_IRQ, SM5702_INT3_OFFSET, 1<<7),
	DECLARE_IRQ_CTRL(SM5702_ABSTMRON_IRQ, SM5702_INT3_OFFSET, 1<<6),
	DECLARE_IRQ_CTRL(SM5702_BOOSTPOK_IRQ, SM5702_INT3_OFFSET, 1<<5),
	DECLARE_IRQ_CTRL(SM5702_BOOSTPOK_NG_IRQ, SM5702_INT3_OFFSET, 1<<4),
	DECLARE_IRQ_CTRL(SM5702_FLEDOPEN_IRQ, SM5702_INT3_OFFSET, 1<<3),
	DECLARE_IRQ_CTRL(SM5702_FLEDSHORT_IRQ, SM5702_INT3_OFFSET, 1<<2),
	DECLARE_IRQ_CTRL(SM5702_THEMREG_IRQ, SM5702_INT3_OFFSET, 1<<1),
	DECLARE_IRQ_CTRL(SM5702_THEMSHDN_IRQ, SM5702_INT3_OFFSET, 1<<0),

	DECLARE_IRQ_CTRL(SM5702_DISLIMIT_IRQ, SM5702_INT4_OFFSET, 1<<4),
	DECLARE_IRQ_CTRL(SM5702_DCDOUT_SDP_IRQ, SM5702_INT4_OFFSET, 1<<3),
	DECLARE_IRQ_CTRL(SM5702_SDP_IRQ, SM5702_INT4_OFFSET, 1<<2),
	DECLARE_IRQ_CTRL(SM5702_CDP_IRQ, SM5702_INT4_OFFSET, 1<<1),
	DECLARE_IRQ_CTRL(SM5702_DCP_IRQ, SM5702_INT4_OFFSET, 1<<0),    

};

static void sm5702_irq_lock(struct irq_data *data)
{
	sm5702_mfd_chip_t *chip = irq_get_chip_data(data->irq);
	mutex_lock(&chip->irq_lock);
}

static void sm5702_irq_sync_unlock(struct irq_data *data)
{
	sm5702_mfd_chip_t *chip = irq_get_chip_data(data->irq);
	if (chip != NULL) {
		sm5702_block_write_device(chip->i2c_client,
				SM5702_INT1_CTRL,
				SM5702_INT1_IRQ_REGS_NR,
				chip->irq_masks_cache +
				SM5702_INT1_OFFSET);

		sm5702_block_write_device(chip->i2c_client,
				SM5702_INT2_CTRL,
				SM5702_INT2_IRQ_REGS_NR,
				chip->irq_masks_cache +
				SM5702_INT2_OFFSET);

		sm5702_block_write_device(chip->i2c_client,
				SM5702_INT3_CTRL,
				SM5702_INT3_IRQ_REGS_NR,
				chip->irq_masks_cache +
				SM5702_INT3_OFFSET);

		sm5702_block_write_device(chip->i2c_client,
				SM5702_INT4_CTRL,
				SM5702_INT4_IRQ_REGS_NR,
				chip->irq_masks_cache +
				SM5702_INT4_OFFSET);
	    //FG
				
		mutex_unlock(&chip->irq_lock);
	}
}

static const inline struct sm5702_irq_data *
	irq_to_sm5702_irq(sm5702_mfd_chip_t *chip, int irq)
{
	return &sm5702_irqs[irq - chip->irq_base];
}

static void sm5702_irq_mask(struct irq_data *data)
{
	sm5702_mfd_chip_t *chip = irq_get_chip_data(data->irq);
	if (chip != NULL) {
		const struct sm5702_irq_data *irq_data = irq_to_sm5702_irq(chip,
				data->irq);

		chip->irq_masks_cache[irq_data->offset] |= irq_data->mask;
	}
}


static void sm5702_irq_unmask(struct irq_data *data)
{
	sm5702_mfd_chip_t *chip = irq_get_chip_data(data->irq);
	if (chip != NULL) {
		const struct sm5702_irq_data *irq_data = irq_to_sm5702_irq(chip,
				data->irq);

		chip->irq_masks_cache[irq_data->offset] &= ~irq_data->mask;
	}
}

static struct irq_chip sm5702_irq_chip = {
	.name			= "sm5702",
	.irq_bus_lock		= sm5702_irq_lock,
	.irq_bus_sync_unlock	= sm5702_irq_sync_unlock,
	.irq_mask		= sm5702_irq_mask,
	.irq_unmask		= sm5702_irq_unmask,
};

sm5702_irq_status_t *sm5702_get_irq_status(sm5702_mfd_chip_t *mfd_chip,
		sm5702_irq_status_sel_t sel)
{

	int index;

	switch(sel) {
	case SM5702_PREV_STATUS:
		index = mfd_chip->irq_status_index^0x01;
		break;
	case SM5702_NOW_STATUS:
	default:
		index = mfd_chip->irq_status_index;
	}

	return &mfd_chip->irq_status[index];
}
EXPORT_SYMBOL(sm5702_get_irq_status);

static int sm5702_read_irq_status(sm5702_mfd_chip_t *chip)
{
	int ret;
	struct i2c_client *iic = chip->i2c_client;
	sm5702_irq_status_t *now_irq_status;

	now_irq_status = sm5702_get_irq_status(chip, SM5702_NOW_STATUS);


	ret = sm5702_block_read_device(iic, SM5702_INT1,
			sizeof(now_irq_status->int1_irq_status),
			now_irq_status->int1_irq_status);
	if (ret < 0) {
		pr_err("Failed on reading irq1 status\n");
		return ret;
	}

	pr_err("charger irq1 = 0x%x\n", (int)now_irq_status->int1_irq_status[0]);

	ret = sm5702_block_read_device(iic, SM5702_INT2,
			sizeof(now_irq_status->int2_irq_status),
			now_irq_status->int2_irq_status);
	if (ret < 0) {
		pr_err("Failed on reading irq2 status\n");
		return ret;
	}
	pr_err("irq2 = 0x%x\n", (int)now_irq_status->int2_irq_status[0]);

	ret = sm5702_block_read_device(iic, SM5702_INT3,
			sizeof(now_irq_status->int3_irq_status),
			now_irq_status->int3_irq_status);
	if (ret < 0) {
		pr_err("Failed on reading irq3 status\n");
		return ret;
	}
	pr_err("irq3 = 0x%x\n", (int)now_irq_status->int3_irq_status[0]);

	ret = sm5702_block_read_device(iic, SM5702_INT4,
			sizeof(now_irq_status->int4_irq_status),
			now_irq_status->int4_irq_status);
	if (ret < 0) {
		pr_err("Failed on reading irq4 status\n");
		return ret;
	}
	pr_err("irq4 = 0x%x\n", (int)now_irq_status->int4_irq_status[0]);


	return 0;
}

static void sm5702_irq_work(sm5702_mfd_chip_t *chip)
{
	int ret;
	int i;
	sm5702_irq_status_t *status;
	pr_info("%s : handle IRQ event\n", __func__);
	ret = sm5702_read_irq_status(chip);
	if (ret < 0) {
		pr_err("%s :Error : can't read irq status (%d)\n",
		       __func__, ret);
		return;
	}
	status = sm5702_get_irq_status(chip, SM5702_NOW_STATUS);

	for (i = 0; i < SM5702_IRQ_REGS_NR; i++)
		status->regs[i] &= ~chip->irq_masks_cache[i];

	for (i = 0; i < SM5702_IRQS_NR; i++) {
		if (status->regs[sm5702_irqs[i].offset] & sm5702_irqs[i].mask) {
			pr_info("%s : Trigger IRQ %s, irq : %d \n",
				__func__, sm5702_get_irq_name_by_index(i),
				chip->irq_base + i);

			handle_nested_irq(chip->irq_base + i);
		}
	}

	chip->irq_status_index ^= 0x01; // exchange irq index;
}
static irqreturn_t sm5702_irq_handler(int irq, void *data)
{
	sm5702_mfd_chip_t *chip = data;


	printk("SM5702 IRQ triggered\n");
	wake_lock_timeout(&chip->irq_wake_lock, msecs_to_jiffies(500));
	mutex_lock(&chip->suspend_flag_lock);
	if (chip->suspend_flag) {
		printk("I2C host controller might not be ready,"
			" pend this IRQ event...\n");
		chip->pending_irq = true;
	}
	else
		sm5702_irq_work(chip);
	mutex_unlock(&chip->suspend_flag_lock);;
	return IRQ_HANDLED;
}

static int sm5702_irq_ctrl_regs[] = {
	SM5702_INT1_CTRL,
	SM5702_INT2_CTRL,
	SM5702_INT3_CTRL,
	SM5702_INT4_CTRL,
};

static uint8_t sm5702_irqs_ctrl_mask_all_val[] = {
	0xFF,
	0xFF,
	0xFF,
	0xFF,
};



static int sm5702_mask_all_irqs(struct i2c_client *iic)
{
	int rc;
	int i;

	sm5702_mfd_chip_t *chip = i2c_get_clientdata(iic);

	for (i=0;i<ARRAY_SIZE(sm5702_irq_ctrl_regs);i++) {
    		rc = sm5702_reg_write(iic, sm5702_irq_ctrl_regs[i],
    				sm5702_irqs_ctrl_mask_all_val[i]);
    		chip->irq_masks_cache[i] = sm5702_irqs_ctrl_mask_all_val[i];
    		if (rc<0) {
    			pr_err("Error : can't write reg[0x%x] = 0x%x\n",
    					sm5702_irq_ctrl_regs[i],
    					sm5702_irqs_ctrl_mask_all_val[i]);
    			return rc;
    		}
        pr_err("%s :[%d] write reg[0x%x] = 0x%x\n", __func__, i, sm5702_irq_ctrl_regs[i], sm5702_irqs_ctrl_mask_all_val[i]);
	}

	return 0;
}

static int sm5702_irq_init_read(sm5702_mfd_chip_t *chip)
{
	int ret;
	ret = sm5702_read_irq_status(chip);
	//chip->irq_status_index ^= 0x01;

	return ret;
}

int sm5702_init_irq(sm5702_mfd_chip_t *chip)
{
	int i, ret, curr_irq;
	ret = sm5702_mask_all_irqs(chip->i2c_client);

	if (ret < 0) {
		pr_err("%s : Can't mask all irqs(%d)\n", __func__, ret);
		goto err_mask_all_irqs;
	}

	sm5702_irq_init_read(chip);
	mutex_init(&chip->irq_lock);

	/* Register with genirq */
	for (i = 0; i < SM5702_IRQS_NR; i++) {
		curr_irq = i + chip->irq_base;
		irq_set_chip_data(curr_irq, chip);
		irq_set_chip_and_handler(curr_irq, &sm5702_irq_chip,
				handle_simple_irq);
		irq_set_nested_thread(curr_irq, 1);
#ifdef CONFIG_ARM
		set_irq_flags(curr_irq, IRQF_VALID);
#else
		irq_set_noprobe(curr_irq);
#endif
	}
    
	ret = request_threaded_irq(chip->irq, NULL, sm5702_irq_handler,
			/* IRQF_TRIGGER_FALLING */
			IRQF_TRIGGER_FALLING | IRQF_ONESHOT | IRQF_NO_SUSPEND,
			"sm5702-mfd", chip);
	if (ret <0) {
		pr_err("%s : Failed : request IRQ (%d)\n", __func__, ret);
		goto err_request_irq;

	}

	ret = enable_irq_wake(chip->irq); 
	if (ret < 0) {
		pr_err("%s : enable_irq_wake(%d) failed for (%d)\n",
				__func__, chip->irq, ret);
	}


	return ret;

err_request_irq:
	for (curr_irq = chip->irq_base;
			curr_irq < chip->irq_base + SM5702_IRQS_NR;
			curr_irq++) {
#ifdef CONFIG_ARM
		set_irq_flags(curr_irq, 0);
#endif
		irq_set_chip_and_handler(curr_irq, NULL, NULL);
		irq_set_chip_data(curr_irq, NULL);
	}

	mutex_destroy(&chip->irq_lock);
err_mask_all_irqs:
	return ret;

}

int sm5702_exit_irq(sm5702_mfd_chip_t *chip)
{
	int curr_irq;

	for (curr_irq = chip->irq_base;
			curr_irq < chip->irq_base + SM5702_IRQS_NR;
			curr_irq++) {
#ifdef CONFIG_ARM
		set_irq_flags(curr_irq, 0);
#endif
		irq_set_chip_and_handler(curr_irq, NULL, NULL);
		irq_set_chip_data(curr_irq, NULL);
	}

	if (chip->irq)
		free_irq(chip->irq, chip);
	mutex_destroy(&chip->irq_lock);
	return 0;
}
#ifdef CONFIG_PM
int sm5702_irq_suspend(sm5702_mfd_chip_t *chip)
{
	mutex_lock(&chip->suspend_flag_lock);
	chip->suspend_flag = true;
	mutex_unlock(&chip->suspend_flag_lock);
	return 0;
}

int sm5702_irq_resume(sm5702_mfd_chip_t *chip)
{
	mutex_lock(&chip->suspend_flag_lock);
	if (chip->pending_irq) {
		pr_info("%s : there is pending IRQ event\n",
					__func__);
		sm5702_irq_work(chip);
		chip->pending_irq = false;
	}
	chip->suspend_flag = false;
	mutex_unlock(&chip->suspend_flag_lock);
	return 0;
}
#endif /* CONFIG_PM */
