/* Lite-On LTR-303ALS Android / Linux Driver
 *
 * Copyright (C) 2013 Lite-On Technology Corp (Singapore)
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */


#include <linux/delay.h>
//#include <linux/earlysuspend.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/gfp.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>
#include <asm/uaccess.h>
//#include <asm/mach-types.h>
#include <asm/setup.h>
#include <linux/version.h>
#include <linux/acpi.h>


/* LTR-303 Registers */
#define LTR303_ALS_CONTR	0x80
#define LTR303_ALS_MEAS_RATE	0x85
#define LTR303_PART_ID		0x86
#define LTR303_MANUFACTURER_ID	0x87
#define LTR303_ALS_DATA_CH1_0	0x88
#define LTR303_ALS_DATA_CH1_1	0x89
#define LTR303_ALS_DATA_CH0_0	0x8A
#define LTR303_ALS_DATA_CH0_1	0x8B
#define LTR303_ALS_STATUS	0x8C
#define LTR303_INTERRUPT	0x8F
#define LTR303_ALS_THRES_UP_0	0x97
#define LTR303_ALS_THRES_UP_1	0x98
#define LTR303_ALS_THRES_LOW_0	0x99
#define LTR303_ALS_THRES_LOW_1	0x9A
#define LTR303_INTERRUPT_PRST	0x9E
/* LTR-303 Registers */

#define SET_BIT 1
#define CLR_BIT 0

/* address 0x80 */
#define ALS_MODE_ACTIVE	(1 << 0)
#define ALS_MODE_STDBY		(0 << 0)
#define ALS_SW_RESET		(1 << 1)
#define ALS_SW_NRESET		(0 << 1)
#define ALS_GAIN_1x		(0 << 2)
#define ALS_GAIN_2x		(1 << 2)
#define ALS_GAIN_4x		(2 << 2)
#define ALS_GAIN_8x		(3 << 2)
#define ALS_GAIN_48x	(6 << 2)
#define ALS_GAIN_96x	(7 << 2)
#define ALS_MODE_RDBCK			0
#define ALS_SWRT_RDBCK			1
#define ALS_GAIN_RDBCK			2
#define ALS_CONTR_RDBCK		3

/* address 0x85 */
#define ALS_MEAS_RPT_RATE_50MS	(0 << 0)
#define ALS_MEAS_RPT_RATE_100MS	(1 << 0)
#define ALS_MEAS_RPT_RATE_200MS	(2 << 0)
#define ALS_MEAS_RPT_RATE_500MS	(3 << 0)
#define ALS_MEAS_RPT_RATE_1000MS	(4 << 0)
#define ALS_MEAS_RPT_RATE_2000MS	(5 << 0)
#define ALS_INTEG_TM_100MS		(0 << 3)
#define ALS_INTEG_TM_50MS			(1 << 3)
#define ALS_INTEG_TM_200MS		(2 << 3)
#define ALS_INTEG_TM_400MS		(3 << 3)
#define ALS_INTEG_TM_150MS		(4 << 3)
#define ALS_INTEG_TM_250MS		(5 << 3)
#define ALS_INTEG_TM_300MS		(6 << 3)
#define ALS_INTEG_TM_350MS		(7 << 3)
#define ALS_MEAS_RPT_RATE_RDBCK	0
#define ALS_INTEG_TM_RDBCK			1
#define ALS_MEAS_RATE_RDBCK		2

/* address 0x86 */
#define PART_NUM_ID_RDBCK		0
#define REVISION_ID_RDBCK		1
#define PART_ID_REG_RDBCK		2

/* address 0x8C */
#define ALS_DATA_STATUS_RDBCK		0
#define ALS_INTERR_STATUS_RDBCK	1
#define ALS_GAIN_STATUS_RDBCK		2
#define ALS_VALID_STATUS_RDBCK	3
#define ALS_STATUS_RDBCK		4

/* address 0x8F */
#define INT_MODE_0					(0 << 1)
#define INT_MODE_ALS_TRIG			(1 << 1)
#define INT_POLAR_ACT_LO			(0 << 2)
#define INT_POLAR_ACT_HI			(1 << 2)
#define INT_MODE_RDBCK				0
#define INT_POLAR_RDBCK			1
#define INT_INTERRUPT_RDBCK		2

/* address 0x9E */
#define ALS_PERSIST_SHIFT	0
#define ALS_PRST_RDBCK		0

#define PON_DELAY		600

#define ALS_MIN_MEASURE_VAL	0
#define ALS_MAX_MEASURE_VAL	65535
#define ALS_VALID_MEASURE_MASK	ALS_MAX_MEASURE_VAL
#define LO_LIMIT			0
#define HI_LIMIT			1
#define LO_N_HI_LIMIT	2

#define DRIVER_VERSION "1.11"
#define PARTID 0xA0
#define MANUID 0x05

#define I2C_RETRY 5

#define DEVICE_NAME "LTR303ALS"

#define ACT_INTERRUPT 1

/*
 * Magic Number
 * ============
 * Refer to file ioctl-number.txt for allocation
 */
#define LTR303_IOCTL_MAGIC      'c'

/* IOCTLs for ltr303 device */
#define LTR303_IOCTL_ALS_ENABLE		_IOR(LTR303_IOCTL_MAGIC, 1, int *)
#define LTR303_IOCTL_ALS_GET_ENABLED	_IOW(LTR303_IOCTL_MAGIC, 2, int *)

//(Linux RTOS)>
#if 1
struct ltr303_platform_data {
	/* ALS */
	uint16_t pfd_levels[5];
	uint16_t pfd_als_lowthresh;
	uint16_t pfd_als_highthresh;
	int pfd_disable_als_on_suspend;

	/* Interrupt */
	int pfd_gpio_int_no;
};
#endif
//(Linux RTOS)<


struct ltr303_data {
	/* Device */
	struct i2c_client *i2c_client;
	struct input_dev *als_input_dev;
	struct workqueue_struct *workqueue;
	//struct early_suspend early_suspend;
	struct mutex bus_lock;

	/* Device mode
	 * 0 = ALS
	 */
	uint8_t mode;

	/* ALS */
	uint8_t als_enable_flag;
	uint8_t als_suspend_enable_flag;
	uint8_t als_irq_flag;
	uint8_t als_opened;
	uint16_t als_lowthresh;
	uint16_t als_highthresh;
	uint16_t default_als_lowthresh;
	uint16_t default_als_highthresh;
	uint16_t *adc_levels;
	/* Flag to suspend ALS on suspend or not */
	uint8_t disable_als_on_suspend;

	/* Interrupt */
	int irq;
	int gpio_int_no;
	int is_suspend;
};

struct ltr303_data *sensor_info;

uint16_t winfac1 = 100;
uint16_t winfac2 = 80;
uint16_t winfac3 = 44;
uint8_t eqn_prev = 0;
uint8_t ratio_old = 0;
uint16_t lux_val_prev = 0;


/* I2C Read */
// take note --------------------------------------- 
// for i2c read, need to send the register address follwed by buffer over to register.
// There should not be a stop in between register address and buffer.  
// There should not be release of lock in between register address and buffer. 
// take note ---------------------------------------
static int8_t I2C_Read(uint8_t *rxData, uint8_t length)
{
	int8_t index;
	struct i2c_msg data[] = {
		{
			.addr = sensor_info->i2c_client->addr,
			.flags = 0,
			.len = 1,
			.buf = rxData,
		},
		{
			.addr = sensor_info->i2c_client->addr,
			.flags = I2C_M_RD,
			.len = length,
			.buf = rxData,
		},
	};

	for (index = 0; index < I2C_RETRY; index++) {
		if (i2c_transfer(sensor_info->i2c_client->adapter, data, 2) > 0)
			break;

		mdelay(10);
	}

	if (index >= I2C_RETRY) {
		pr_alert("%s I2C Read Fail !!!!\n",__func__);
		return -EIO;
	}

	return 0;
}


/* I2C Write */
static int8_t I2C_Write(uint8_t *txData, uint8_t length)
{
	int8_t index;
	struct i2c_msg data[] = {
		{
			.addr = sensor_info->i2c_client->addr,
			.flags = 0,
			.len = length,
			.buf = txData,
		},
	};

	for (index = 0; index < I2C_RETRY; index++) {
		if (i2c_transfer(sensor_info->i2c_client->adapter, data, 1) > 0)
			break;

		mdelay(10);
	}

	if (index >= I2C_RETRY) {
		pr_alert("%s I2C Write Fail !!!!\n", __func__);
		return -EIO;
	}

	return 0;
}


/* Set register bit */
static int8_t _ltr303_set_bit(struct i2c_client *client, uint8_t set, uint8_t cmd, uint8_t data)
{
	uint8_t buffer[2];
	uint8_t value;
	int8_t ret = 0;

	buffer[0] = cmd;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		dev_err(&client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	value = buffer[0];

	if (set)
		value |= data;
	else
		value &= ~data;

	buffer[0] = cmd;
	buffer[1] = value;
	ret = I2C_Write(buffer, 2);
	if (ret < 0) {
		dev_err(&client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return -EIO;
	}

	return ret;
}


static uint16_t lux_formula(uint16_t ch0_adc, uint16_t ch1_adc, uint8_t eqtn)
{
	uint32_t luxval = 0;
	uint32_t luxval_i = 0;
	uint32_t luxval_f = 0;
	uint16_t ch0_coeff_i = 0;
	uint16_t ch1_coeff_i = 0;
	uint16_t ch0_coeff_f = 0;
	uint16_t ch1_coeff_f = 0;
	int8_t ret;
	uint8_t gain = 1, als_int_fac;
	uint8_t buffer[2];
	uint16_t win_fac = 0;
	int8_t fac = 1;

	buffer[0] = LTR303_ALS_STATUS;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		pr_alert("%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	gain = (buffer[0] & 0x70);
	gain >>= 4;

	if (gain == 0) {			//gain 1
		gain = 1;
	} else if (gain == 1) {		//gain 2
		gain = 2;
	} else if (gain == 2) {		//gain 4
		gain = 4;
	} else if (gain == 3) {		//gain 8
		gain = 8;
	} else if (gain == 6) {		//gain 48
		gain = 48;
	} else if (gain == 7) {		//gain 96
		gain = 96;
	}

	buffer[0] = LTR303_ALS_MEAS_RATE;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		pr_alert("%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}
	als_int_fac = buffer[0] & 0x38;
	als_int_fac >>= 3;

	if (als_int_fac == 0) {
		als_int_fac = 10;
	} else if (als_int_fac == 1) {
		als_int_fac = 5;
	} else if (als_int_fac == 2) {
		als_int_fac = 20;
	} else if (als_int_fac == 3) {
		als_int_fac = 40;
	} else if (als_int_fac == 4) {
		als_int_fac = 15;
	} else if (als_int_fac == 5) {
		als_int_fac = 25;
	} else if (als_int_fac == 6) {
		als_int_fac = 30;
	} else if (als_int_fac == 7) {
		als_int_fac = 35;
	}

	if (eqtn == 1) {
		ch0_coeff_i = 1;
		ch1_coeff_i = 1;
		ch0_coeff_f = 7743;
		ch1_coeff_f = 1059;
		fac = 1;
		win_fac = winfac1;
		luxval_i = ((ch0_adc * ch0_coeff_i) + (ch1_adc * ch1_coeff_i)) * win_fac;
		luxval_f = (((ch0_adc * ch0_coeff_f) + (ch1_adc * ch1_coeff_f)) / 100) * win_fac;
		//luxval = ((17743 * ch0_calc) + (11059 * ch1_adc));
		//luxval = ((1.7743 * ch0_calc) + (1.1059 * ch1_adc)) / (gain * (als_int_fac / 10));
	} else if (eqtn == 2) {
		ch0_coeff_i = 4;
		ch1_coeff_i = 1;
		ch0_coeff_f = 2785;
		ch1_coeff_f = 696;
		win_fac = winfac2;
		if ((ch1_coeff_f * ch1_adc) < (ch0_adc * ch0_coeff_f)) {
			fac = 1;
			luxval_f = (((ch0_adc * ch0_coeff_f) - (ch1_adc * ch1_coeff_f)) / 100) * win_fac;
		} else {
			fac = -1;
			luxval_f = (((ch1_adc * ch1_coeff_f) - (ch0_adc * ch0_coeff_f)) / 100) * win_fac;
		}
		luxval_i = ((ch0_adc * ch0_coeff_i) - (ch1_adc * ch1_coeff_i)) * win_fac;
		//luxval = ((42785 * ch0_calc) - (10696 * ch1_adc));
		//luxval = ((4.2785 * ch0_calc) - (1.9548 * ch1_adc)) / (gain * (als_int_fac / 10));
	} else if (eqtn == 3) {
		ch0_coeff_i = 0;
		ch1_coeff_i = 0;
		ch0_coeff_f = 5926;
		ch1_coeff_f = 1300;
		fac = 1;
		win_fac = winfac3;
		luxval_i = ((ch0_adc * ch0_coeff_i) + (ch1_adc * ch1_coeff_i)) * win_fac;
		luxval_f = (((ch0_adc * ch0_coeff_f) + (ch1_adc * ch1_coeff_f)) / 100) * win_fac;
		//luxval = ((5926 * ch0_calc) + (1185 * ch1_adc));
		//luxval = ((0.5926 * ch0_calc) + (0.1185 * ch1_adc)) / (gain * (als_int_fac / 10));
	} else if (eqtn == 4) {
		ch0_coeff_i = 0;
		ch1_coeff_i = 0;
		ch0_coeff_f = 0;
		ch1_coeff_f = 0;
		fac = 1;
		luxval_i = 0;
		luxval_f = 0;
		//luxval = 0;
	}

	luxval = (luxval_i  + ((fac) * luxval_f) / 100) / (gain * als_int_fac);

	return luxval;
}


static uint16_t ratioHysterisis (uint16_t ch0_adc, uint16_t ch1_adc)
{
#define	RATIO_HYSVAL	10
	int ratio;
	uint8_t buffer[2], eqn_now;
	int8_t ret;
	uint16_t ch0_calc;
	uint32_t luxval = 0;
	int abs_ratio_now_old;

	buffer[0] = LTR303_ALS_CONTR;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		pr_alert("%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	ch0_calc = ch0_adc;
	if ((buffer[0] & 0x20) == 0x20) {
		ch0_calc = ch0_adc - ch1_adc;
	}

	if ((ch1_adc + ch0_calc) == 0) {
		ratio = 100;
	} else {
		ratio = (ch1_adc*100) / (ch1_adc + ch0_calc);
	}

	if (ratio < 45) {
		eqn_now = 1;
	} else if ((ratio >= 45) && (ratio < 68)) {
		eqn_now = 2;
	} else if ((ratio >= 68) && (ratio < 99)) {
		eqn_now = 3;
	} else if (ratio >= 99) {
		eqn_now = 4;
	}

	if (eqn_prev == 0) {
		luxval = lux_formula(ch0_calc, ch1_adc, eqn_now);
		ratio_old = ratio;
		eqn_prev = eqn_now;
	} else {
		if (eqn_now == eqn_prev) {
			luxval = lux_formula(ch0_calc, ch1_adc, eqn_now);
			ratio_old = ratio;
			eqn_prev = eqn_now;
		} else {
			abs_ratio_now_old = ratio - ratio_old;
			if (abs_ratio_now_old < 0) {
				abs_ratio_now_old *= (-1);
			}
			if (abs_ratio_now_old > RATIO_HYSVAL) {
				luxval = lux_formula(ch0_calc, ch1_adc, eqn_now);
				ratio_old = ratio;
				eqn_prev = eqn_now;
			} else {
				luxval = lux_formula(ch0_calc, ch1_adc, eqn_prev);
			}
		}
	}

	return luxval;
}


/* Read ADC Value */
static uint16_t read_als_adc_value(struct ltr303_data *ltr303)
{

	int8_t ret = -99;
	uint16_t value = -99;
	int ch0_val;
	int ch1_val;
	uint8_t gain, value_temp, gain_chg_req = 0;
	uint8_t buffer[4], temp;
	
#define AGC_UP_THRESHOLD		40000
#define AGC_DOWN_THRESHOLD  	5000
#define AGC_HYS					15
#define MAX_VAL					50000
		
	/* ALS */
	buffer[0] = LTR303_ALS_DATA_CH1_0;

	/* read data bytes from data regs */
	ret = I2C_Read(buffer, 4);	

	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	/* ALS Ch0 */
 	ch0_val = (uint16_t)buffer[2] | ((uint16_t)buffer[3] << 8);
		dev_dbg(&ltr303->i2c_client->dev, 
			"%s | als_ch0 value = 0x%04X\n", __func__, 
			ch0_val);

	if (ch0_val > ALS_MAX_MEASURE_VAL) {
		dev_err(&ltr303->i2c_client->dev,
		        "%s: ALS Value Error: 0x%X\n", __func__,
		        ch0_val);
	}
	ch0_val &= ALS_VALID_MEASURE_MASK;
	input_report_abs(ltr303->als_input_dev, ABS_MISC, ch0_val);
	input_sync(ltr303->als_input_dev);

	/* ALS Ch1 */
 	ch1_val = (uint16_t)buffer[0] | ((uint16_t)buffer[1] << 8);
		dev_dbg(&ltr303->i2c_client->dev, 
			"%s | als_ch1 value = 0x%04X\n", __func__, 
			ch1_val);

	if (ch1_val > ALS_MAX_MEASURE_VAL) {
		dev_err(&ltr303->i2c_client->dev,
		        "%s: ALS Value Error: 0x%X\n", __func__,
		        ch1_val);
	}
	ch1_val &= ALS_VALID_MEASURE_MASK;
	input_report_abs(ltr303->als_input_dev, ABS_MISC, ch1_val);
	input_sync(ltr303->als_input_dev);

	buffer[0] = LTR303_ALS_STATUS;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	value_temp = buffer[0];
	temp = buffer[0];
	gain = (value_temp & 0x70);	
	gain >>= 4;

	if (gain == 0) {			//gain 1
		gain = 1;
	} else if (gain == 1) {		//gain 2
		gain = 2;
	} else if (gain == 2) {		//gain 4
		gain = 4;
	} else if (gain == 3) {		//gain 8
		gain = 8;
	} else if (gain == 6) {		//gain 48
		gain = 48;
	} else if (gain == 7) {		//gain 96
		gain = 96;
	}

	buffer[0] = LTR303_ALS_CONTR;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}
	value_temp = buffer[0];
	value_temp &= 0xE3;

	if ((ch0_val == 0) && (ch1_val > 50 )) {
		value = lux_val_prev;
	} else {
		if (gain == 1) {
			if ((ch0_val + ch1_val) < ((AGC_DOWN_THRESHOLD * 10) / AGC_HYS)) {
				value = ratioHysterisis(ch0_val, ch1_val);
				value_temp |= ALS_GAIN_8x;
				gain_chg_req = 1;
			} else {
				value = ratioHysterisis(ch0_val, ch1_val);
			}
		} else if (gain == 8) {
			if ((ch0_val + ch1_val) > AGC_UP_THRESHOLD) {
				value = ratioHysterisis(ch0_val, ch1_val);
				value_temp |= ALS_GAIN_1x;
				gain_chg_req = 1;
			} else {
				value = ratioHysterisis(ch0_val, ch1_val);
			}
		} else {
			value = ratioHysterisis(ch0_val, ch1_val);
		}
		if (gain_chg_req) {
			buffer[0] = LTR303_ALS_CONTR;
			buffer[1] = value_temp;
			ret = I2C_Write(buffer, 2);
			if (ret < 0) {
				dev_err(&ltr303->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
				return ret;
			}
		}
		
	}

	if ((value > MAX_VAL) || (((ch0_val + ch1_val) > MAX_VAL) && (temp & 0x80))) {
		value = MAX_VAL;
	}
	lux_val_prev = value;	
			
	return value;
}


static int8_t als_mode_setup (uint8_t alsMode_set_reset, struct ltr303_data *ltr303)
{
	int8_t ret = 0;

	ret = _ltr303_set_bit(ltr303->i2c_client, alsMode_set_reset, LTR303_ALS_CONTR, ALS_MODE_ACTIVE);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s ALS mode setup fail...\n", __func__);
		return ret;
	}

	return ret;
}


static int8_t als_sw_reset_setup(uint8_t alsSWReset_set_reset, struct ltr303_data *ltr303)
{
	int8_t ret = 0;

	ret = _ltr303_set_bit(ltr303->i2c_client, alsSWReset_set_reset, LTR303_ALS_CONTR, ALS_SW_RESET);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s ALS sw reset setup fail...\n", __func__);
		return ret;
	}

	return ret;
}


static int8_t als_gain_setup (uint8_t alsgain_range, struct ltr303_data *ltr303)
{
	int8_t ret = 0;
	uint8_t buffer[2], value;

	buffer[0] = LTR303_ALS_CONTR;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	value = buffer[0];
	value &= 0xE3;

	if (alsgain_range == 1) {
		value |= ALS_GAIN_1x;
	} else if (alsgain_range == 2) {
		value |= ALS_GAIN_2x;
	} else if (alsgain_range == 4) {
		value |= ALS_GAIN_4x;
	} else if (alsgain_range == 8) {
		value |= ALS_GAIN_8x;
	} else if (alsgain_range == 48) {
		value |= ALS_GAIN_48x;
	} else if (alsgain_range == 96) {
		value |= ALS_GAIN_96x;
	}

	buffer[0] = LTR303_ALS_CONTR;
	buffer[1] = value;
	ret = I2C_Write(buffer, 2);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s ALS gain setup fail...\n", __func__);
		return ret;
	}

	return ret;
}


static int8_t als_contr_setup(uint8_t als_contr_val, struct ltr303_data *ltr303)
{
	int8_t ret = 0;
	uint8_t buffer[3];

	buffer[0] = LTR303_ALS_CONTR;

	/* Default settings used for now. */
	buffer[1] = als_contr_val;
	buffer[1] &= 0x1F;
	ret = I2C_Write(buffer, 2);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s | ALS_CONTR (0x%02X) setup fail...", __func__, buffer[0]);
	}

	return ret;
}


static int8_t als_contr_readback (uint8_t rdbck_type, uint8_t *retVal, struct ltr303_data *ltr303)
{
	int8_t ret = 0;
	uint8_t buffer[2], value;

	buffer[0] = LTR303_ALS_CONTR;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	value = buffer[0];

	if (rdbck_type == ALS_MODE_RDBCK) {
		*retVal = value & 0x01;
	} else if (rdbck_type == ALS_SWRT_RDBCK) {
		*retVal = (value & 0x02) >> 1;
	} else if (rdbck_type == ALS_GAIN_RDBCK) {
		*retVal = (value & 0x1C) >> 2;
	} else if (rdbck_type == ALS_CONTR_RDBCK) {
		*retVal = value & 0x1F;
	}

	return ret;
}


static int8_t als_meas_rate_setup(uint16_t meas_rate_val, struct ltr303_data *ltr303)
{
	int8_t ret = 0;
	uint8_t buffer[2], value;

	buffer[0] = LTR303_ALS_MEAS_RATE;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	value = buffer[0];
	value &= 0xF8;

	if (meas_rate_val == 50) {
		value |= ALS_MEAS_RPT_RATE_50MS;
	} else if (meas_rate_val == 100) {
		value |= ALS_MEAS_RPT_RATE_100MS;
	} else if (meas_rate_val == 200) {
		value |= ALS_MEAS_RPT_RATE_200MS;
	} else if (meas_rate_val == 500) {
		value |= ALS_MEAS_RPT_RATE_500MS;
	} else if (meas_rate_val == 1000) {
		value |= ALS_MEAS_RPT_RATE_1000MS;
	} else if (meas_rate_val == 2000) {
		value |= ALS_MEAS_RPT_RATE_2000MS;
	}

	buffer[0] = LTR303_ALS_MEAS_RATE;
	buffer[1] = value;
	ret = I2C_Write(buffer, 2);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s ALS measurement rate setup fail...\n", __func__);
		return ret;
	}

	return ret;
}


static int8_t als_integ_time_setup(uint16_t integ_time_val, struct ltr303_data *ltr303)
{
	int8_t ret = 0;
	uint8_t buffer[2], value;

	buffer[0] = LTR303_ALS_MEAS_RATE;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	value = buffer[0];
	value &= 0xC7;

	if (integ_time_val == 100) {
		value |= ALS_INTEG_TM_100MS;
	} else if (integ_time_val == 50) {
		value |= ALS_INTEG_TM_50MS;
	} else if (integ_time_val == 200) {
		value |= ALS_INTEG_TM_200MS;
	} else if (integ_time_val == 400) {
		value |= ALS_INTEG_TM_400MS;
	} else if (integ_time_val == 150) {
		value |= ALS_INTEG_TM_150MS;
	} else if (integ_time_val == 250) {
		value |= ALS_INTEG_TM_250MS;
	} else if (integ_time_val == 300) {
		value |= ALS_INTEG_TM_300MS;
	} else if (integ_time_val == 350) {
		value |= ALS_INTEG_TM_350MS;
	}

	buffer[0] = LTR303_ALS_MEAS_RATE;
	buffer[1] = value;
	ret = I2C_Write(buffer, 2);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s ALS integration time setup fail...\n", __func__);
		return ret;
	}

	return ret;
}


static int8_t als_meas_rate_reg_setup(uint8_t als_meas_rate_reg_val, struct ltr303_data *ltr303)
{
	int8_t ret = 0;
	uint8_t buffer[3];

	buffer[0] = LTR303_ALS_MEAS_RATE;

	buffer[1] = als_meas_rate_reg_val;
	buffer[1] &= 0x3F;
	ret = I2C_Write(buffer, 2);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s | ALS_MEAS_RATE (0x%02X) setup fail...", __func__, buffer[0]);
	}

	return ret;
}


static int8_t als_meas_rate_readback (uint8_t rdbck_type, uint8_t *retVal, struct ltr303_data *ltr303)
{
	int8_t ret = 0;
	uint8_t buffer[2], value;

	buffer[0] = LTR303_ALS_MEAS_RATE;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	value = buffer[0];

	if (rdbck_type == ALS_MEAS_RPT_RATE_RDBCK) {
		*retVal = (value & 0x07);
	} else if (rdbck_type == ALS_INTEG_TM_RDBCK) {
		*retVal = (value & 0x38) >> 3;
	} else if (rdbck_type == ALS_MEAS_RATE_RDBCK) {
		*retVal = (value & 0x3F);
	}

	return ret;
}


static int8_t part_ID_reg_readback (uint8_t rdbck_type, uint8_t *retVal, struct ltr303_data *ltr303)
{
	int8_t ret = 0;
	uint8_t buffer[1], value;

	buffer[0] = LTR303_PART_ID;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	value = buffer[0];

	if (rdbck_type == PART_NUM_ID_RDBCK) {
		*retVal = (value & 0xF0) >> 4;
	} else if (rdbck_type == REVISION_ID_RDBCK) {
		*retVal = value & 0x0F;
	} else if (rdbck_type == PART_ID_REG_RDBCK) {
		*retVal = value;
	}

	return ret;
}


static int8_t manu_ID_reg_readback (uint8_t *retVal, struct ltr303_data *ltr303)
{
	int8_t ret = 0;
	uint8_t buffer[1], value;

	buffer[0] = LTR303_MANUFACTURER_ID;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	value = buffer[0];
	*retVal = value;

	return ret;
}


static int8_t als_status_reg (uint8_t data_status_type, uint8_t *retVal, struct ltr303_data *ltr303)
{
	int8_t ret = 0;
	uint8_t buffer[2], value;

	buffer[0] = LTR303_ALS_STATUS;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	value = buffer[0];

	if (data_status_type == ALS_DATA_STATUS_RDBCK) {
		*retVal = (value & 0x04) >> 2;
	} else if (data_status_type == ALS_INTERR_STATUS_RDBCK) {
		*retVal = (value & 0x08) >> 3;
	} else if (data_status_type == ALS_GAIN_STATUS_RDBCK) {
		*retVal = (value & 0x70) >> 4;
	} else if (data_status_type == ALS_VALID_STATUS_RDBCK) {
		*retVal = (value & 0x80) >> 7;
	} else if (data_status_type == ALS_STATUS_RDBCK) {
		*retVal = (value & 0xFC);
	}

	return ret;
}


static int8_t interrupt_mode_setup (uint8_t interr_mode_val, struct ltr303_data *ltr303)
{
	int8_t ret = 0;
	uint8_t buffer[2], value;

	buffer[0] = LTR303_INTERRUPT;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	value = buffer[0];
	value &= 0xFD;

	if (interr_mode_val == 0) {
		value |= INT_MODE_0;
	} else if (interr_mode_val == 1) {
		value |= INT_MODE_ALS_TRIG;
	}

	buffer[0] = LTR303_INTERRUPT;
	buffer[1] = value;
	ret = I2C_Write(buffer, 2);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s Interrupt mode setup fail...\n", __func__);
		return ret;
	}

	return ret;
}


static int8_t interrupt_polarity_setup (uint8_t interr_polar_val, struct ltr303_data *ltr303)
{
	int8_t ret = 0;
	uint8_t buffer[2], value;

	buffer[0] = LTR303_INTERRUPT;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	value = buffer[0];
	value &= 0xFB;

	if (interr_polar_val == 0) {
		value |= INT_POLAR_ACT_LO;
	} else if (interr_polar_val == 1) {
		value |= INT_POLAR_ACT_HI;
	}

	buffer[0] = LTR303_INTERRUPT;
	buffer[1] = value;
	ret = I2C_Write(buffer, 2);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s Interrupt polarity setup fail...\n", __func__);
		return ret;
	}

	return ret;
}


static int8_t interrupt_setup(uint8_t interrupt_val, struct ltr303_data *ltr303)
{
	int8_t ret = 0;
	uint8_t buffer[3];

	buffer[0] = LTR303_INTERRUPT;

	/* Default settings used for now. */
	buffer[1] = interrupt_val;
	buffer[1] &= 0x06;
	ret = I2C_Write(buffer, 2);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s |Interrupt (0x%02X) setup fail...", __func__, buffer[0]);
	}

	return ret;
}


static int8_t interrupt_readback (uint8_t rdbck_type, uint8_t *retVal, struct ltr303_data *ltr303)
{
	int8_t ret = 0;
	uint8_t buffer[2], value;

	buffer[0] = LTR303_INTERRUPT;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	value = buffer[0];

	if (rdbck_type == INT_MODE_RDBCK) {
		*retVal = (value & 0x02) >> 1;
	} else if (rdbck_type == INT_POLAR_RDBCK) {
		*retVal = (value & 0x04) >> 2;
	} else if (rdbck_type == INT_INTERRUPT_RDBCK) {
		*retVal = (value & 0x06);
	}

	return ret;
}


static int8_t interrupt_persist_setup (uint8_t interr_persist_val, struct ltr303_data *ltr303)
{
	int8_t ret = 0;
	uint8_t buffer[2], value;

	value = interr_persist_val;

	buffer[0] = LTR303_INTERRUPT_PRST;
	buffer[1] = value;
	buffer[1] &= 0x0F;
	ret = I2C_Write(buffer, 2);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s Interrupt persist setup fail...\n", __func__);
		return ret;
	}

	return ret;
}


static int8_t interrupt_prst_readback (uint8_t *retVal, struct ltr303_data *ltr303)
{
	int8_t ret = 0;
	uint8_t buffer[2], value;

	buffer[0] = LTR303_INTERRUPT_PRST;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	value = buffer[0];

	*retVal = value;

	return ret;
}


/* Set ALS range */
static int8_t set_als_range(uint16_t lt, uint16_t ht, uint8_t lo_hi)
{
	int8_t ret;
	uint8_t buffer[5], num_data = 0;

	if (lo_hi == LO_LIMIT) {
		buffer[0] = LTR303_ALS_THRES_LOW_0;
		buffer[1] = lt & 0xFF;
		buffer[2] = (lt >> 8) & 0xFF;
		num_data = 3;
	} else if (lo_hi == HI_LIMIT) {
		buffer[0] = LTR303_ALS_THRES_UP_0;
		buffer[1] = ht & 0xFF;
		buffer[2] = (ht >> 8) & 0xFF;
		num_data = 3;
	} else if (lo_hi == LO_N_HI_LIMIT) {
		buffer[0] = LTR303_ALS_THRES_UP_0;
		buffer[1] = ht & 0xFF;
		buffer[2] = (ht >> 8) & 0xFF;
		buffer[3] = lt & 0xFF;
		buffer[4] = (lt >> 8) & 0xFF;
		num_data = 5;
	}	

	ret = I2C_Write(buffer, num_data);
	if (ret <0) {
		pr_alert("%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}
	dev_dbg(&sensor_info->i2c_client->dev, "%s Set als range:0x%04x"
	                                       " - 0x%04x\n", __func__, lt, ht);

	return ret;
}


static int8_t als_range_readback (uint16_t *lt, uint16_t *ht, struct ltr303_data *ltr303)
{
	int8_t ret = 0;
	uint8_t buffer[5];
	uint16_t value_lo, value_hi;

	buffer[0] = LTR303_ALS_THRES_UP_0;
	ret = I2C_Read(buffer, 4);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	value_lo = buffer[3];
	value_lo <<= 8;
	value_lo += buffer[2];
	*lt = value_lo;

	value_hi = buffer[1];
	value_hi <<= 8;
	value_hi += buffer[0];
	*ht = value_hi;

	return ret;
}


//(Linux RTOS)>
/* Report ALS input event */
static void report_als_input_event(struct ltr303_data *ltr303)
{
	//int8_t ret;
	uint16_t adc_value;
	//int thresh_hi, thresh_lo, thresh_delta;

	adc_value = read_als_adc_value (ltr303);

	input_report_abs(ltr303->als_input_dev, ABS_MISC, adc_value);
	input_sync(ltr303->als_input_dev);	
}


/* Work when interrupt */
static void ltr303_schedwork(struct work_struct *work)
{
	int8_t ret;
	uint8_t status;
	uint8_t	interrupt_stat, newdata;
	struct ltr303_data *ltr303 = sensor_info;
	uint8_t buffer[2];

	buffer[0] = LTR303_ALS_STATUS;	
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return;
	}
	status = buffer[0];
	interrupt_stat = status & 0x08;
	newdata = status & 0x04;
	
	// ALS interrupt and ALS with new data
	if ((interrupt_stat & 0x08) && (newdata & 0x04)) {
		ltr303->als_irq_flag = 1;
		report_als_input_event(ltr303);
		ltr303->als_irq_flag = 0;
	}
	
	enable_irq(ltr303->irq);
}

static DECLARE_WORK(irq_workqueue, ltr303_schedwork);


/* IRQ Handler */
static irqreturn_t ltr303_irq_handler(int irq, void *data)
{
	struct ltr303_data *ltr303 = data;

	/* disable an irq without waiting */
	disable_irq_nosync(ltr303->irq);

	schedule_work(&irq_workqueue);

	return IRQ_HANDLED;
}


#if 1
static int ltr303_gpio_irq(struct ltr303_data *ltr303)
{
	int rc = 0;

	rc = gpio_direction_input(ltr303->gpio_int_no);
	if (rc < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: Set GPIO %d as Input Fail (%d)\n", __func__, ltr303->gpio_int_no, rc);
		goto out1;
	}

	/* Configure an active low trigger interrupt for the device */
	rc = request_irq(ltr303->irq, ltr303_irq_handler, IRQF_TRIGGER_LOW, DEVICE_NAME, ltr303);
	if (rc < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: Request IRQ (%d) for GPIO %d Fail (%d)\n", __func__, ltr303->irq,
		        ltr303->gpio_int_no, rc);
		goto out1;
	}

	return rc;

out1:
	gpio_free(ltr303->gpio_int_no);

	return rc;
}
#endif
//(Linux RTOS)<


static int8_t als_enable_init(struct ltr303_data *ltr303)
{
	int8_t rc = 0;
	uint8_t buffer[1]; // for dummy read

	/* if device not enabled, enable it */
	if (ltr303->als_enable_flag) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS already enabled...\n", __func__);
		return rc;
	}

	rc = als_meas_rate_reg_setup(0x03, ltr303);
	if (rc < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS_Meas_Rate register Setup Fail...\n", __func__);
		return rc;
	}

	/* Set minimummax thresholds where interrupt will *not* be generated */	
#if ACT_INTERRUPT
	rc = set_als_range(ALS_MIN_MEASURE_VAL, ALS_MIN_MEASURE_VAL, LO_N_HI_LIMIT);
#else
	rc = set_als_range(ALS_MIN_MEASURE_VAL, ALS_MAX_MEASURE_VAL, LO_N_HI_LIMIT);
#endif
	if (rc < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s : ALS Thresholds Write Fail...\n", __func__);
		return rc;
	}

	rc = als_contr_setup(0x0D, ltr303);
	if (rc < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS Enable Fail...\n", __func__);
		return rc;
	}

	// dummy read
	buffer[0] = LTR303_ALS_CONTR;
	I2C_Read(buffer, 1);
	// dumy read
	
	ltr303->als_enable_flag = 1;

	return rc;
}


static int8_t als_disable(struct ltr303_data *ltr303)
{
	int8_t rc = 0;

	if (ltr303->als_enable_flag == 0) {
		dev_err(&ltr303->i2c_client->dev, "%s : ALS already disabled...\n", __func__);
		return rc;
	}

	//rc = _ltr303_set_bit(ltr303->i2c_client, CLR_BIT, LTR303_ALS_CONTR, ALS_MODE);
	rc = als_mode_setup(CLR_BIT, ltr303);
	if (rc < 0) {
		dev_err(&ltr303->i2c_client->dev,"%s: ALS Disable Fail...\n", __func__);
		return rc;
	}
	ltr303->als_enable_flag = 0;

	return rc;
}


ssize_t als_open(struct inode *inode, struct file *file)
{
	struct ltr303_data *ltr303 = sensor_info;
	int8_t rc = 0;

	if (ltr303->als_opened) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS already Opened...\n", __func__);
		rc = -EBUSY;
	}
	ltr303->als_opened = 1;

	return rc;
}


ssize_t als_release(struct inode *inode, struct file *file)
{
	struct ltr303_data *ltr303 = sensor_info;

	ltr303->als_opened = 0;

	//return 0;
	return als_disable(ltr303);
}


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
static int als_ioctl (struct inode *ino, struct file *file, unsigned int cmd, unsigned long arg)
#else
static long als_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
#endif
{
	int rc = 0, val = 0;
	struct ltr303_data *ltr303 = sensor_info;

	pr_debug("%s cmd %d\n", __func__, _IOC_NR(cmd));

	switch (cmd) {
		case LTR303_IOCTL_ALS_ENABLE:
			if (get_user(val, (unsigned long __user *)arg)) {
				rc = -EFAULT;
				break;
			}
			/*pr_info("%s value = %d\n", __func__, val);*/
			rc = val ? als_enable_init(ltr303) : als_disable(ltr303);

			break;
		case LTR303_IOCTL_ALS_GET_ENABLED:
			val = ltr303->als_enable_flag;
			/*pr_info("%s enabled %d\n", __func__, val);*/
			rc = put_user(val, (unsigned long __user *)arg);

			break;
		default:
			pr_err("%s: INVALID COMMAND %d\n", __func__, _IOC_NR(cmd));
			rc = -EINVAL;
	}

	return rc;
}


static const struct file_operations als_fops = {
	.owner = THIS_MODULE,
	.open = als_open,
	.release = als_release,
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
	.ioctl = als_ioctl
	#else
	.unlocked_ioctl = als_ioctl
	#endif
};

static struct miscdevice als_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "ltr303_ls",
	.fops = &als_fops
};


static ssize_t als_adc_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint16_t value;
	int ret;
	struct ltr303_data *ltr303 = sensor_info;

	//ltr303->mode = ALS;
	value = read_als_adc_value(ltr303);
	input_report_abs(ltr303->als_input_dev, ABS_MISC, value);
	input_sync(ltr303->als_input_dev);
	//ret = sprintf(buf, "%d\n", value);
	ret = sprintf(buf, "%d", value);

	return ret;
}

static DEVICE_ATTR(als_adc, 0666, als_adc_show, NULL);


static ssize_t ltr303help_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk("To show ALS value : cat als_adc\n");
	
	// address 0x80
	printk("Address 0x80 (ALS_CONTR)\n");
	printk("ALS active mode : echo 1 > alsmodesetup\n");
	printk("ALS standby mode : echo 0 > alsmodesetup\n");
	printk("To read ALS mode : cat alsmodesetup\n\n");

	printk("ALS SW reset : echo 1 > alsswresetsetup\n");
	printk("ALS SW not reset : echo 0 > alsswresetsetup\n");
	printk("To read ALS SW reset bit : cat alsswresetsetup\n\n");

	printk("ALS gain 1x : echo 1 > alsgainsetup\n");
	printk("ALS gain 2x : echo 2 > alsgainsetup\n");
	printk("ALS gain 4x : echo 4 > alsgainsetup\n");
	printk("ALS gain 8x : echo 8 > alsgainsetup\n");
	printk("ALS gain 48x : echo 48 > alsgainsetup\n");
	printk("ALS gain 96x : echo 96 > alsgainsetup\n");
	printk("To read ALS gain : cat alsgainsetup\n\n");

	printk("Write value to ALS_CONTR register (0x80) : echo [hexcode value] > alscontrsetup\n");
	printk("Example...to write 0x0B : echo B > alscontrsetup or echo b > alscontrsetup\n");
	printk("Example...to write 0x13 : echo 13 > alscontrsetup\n");
	printk("To read register ALS_CONTR (0x80) : cat alscontrsetup\n\n");
	// address 0x80

	// address 0x85
	printk("Address 0x85 (ALS_MEAS_RATE)\n");
	printk("ALS meas repeat rate 50ms : echo 50 > alsmeasratesetup\n");
	printk("ALS meas repeat rate 100ms : echo 100 > alsmeasratesetup\n");
	printk("ALS meas repeat rate 200ms : echo 200 > alsmeasratesetup\n");
	printk("ALS meas repeat rate 500ms : echo 500 > alsmeasratesetup\n");
	printk("ALS meas repeat rate 1000ms : echo 1000 > alsmeasratesetup\n");
	printk("ALS meas repeat rate 2000ms : echo 2000 > alsmeasratesetup\n");
	printk("To read ALS meas repeat rate : cat alsmeasratesetup\n\n");

	printk("ALS integration time 100ms : echo 100 > alsintegtimesetup\n");
	printk("ALS integration time 50ms : echo 50 > alsintegtimesetup\n");
	printk("ALS integration time 200ms : echo 200 > alsintegtimesetup\n");
	printk("ALS integration time 400ms : echo 400 > alsintegtimesetup\n");
	printk("ALS integration time 150ms : echo 150 > alsintegtimesetup\n");
	printk("ALS integration time 250ms : echo 250 > alsintegtimesetup\n");
	printk("ALS integration time 300ms : echo 300 > alsintegtimesetup\n");
	printk("ALS integration time 350ms : echo 350 > alsintegtimesetup\n");
	printk("To read ALS integration time : cat alsintegtimesetup\n\n");

	printk("Write value to ALS_MEAS register (0x85) : echo [hexcode value] > alsmeasrateregsetup\n");
	printk("Example...to write 0x0B : echo B > alsmeasrateregsetup or echo b > alsmeasrateregsetup\n");
	printk("Example...to write 0x13 : echo 13 > alsmeasrateregsetup\n");
	printk("To read register ALS_MEAS (0x85) : cat alsmeasrateregsetup\n\n");
	// address 0x85

	// address 0x86
	printk("To read part ID : cat partid\n");
	printk("To read revision ID : cat revid\n");
	printk("To read PART_ID register (0x86) : cat partidreg\n\n");
	// address 0x86

	// address 0x87
	printk("To read manufacturing ID : cat manuid\n\n");
	// address 0x87

	// address 0x8C
	printk("Address 0x8C (ALS_STATUS)\n");
	printk("To read ALS data status : cat alsdatastatus\n");
	printk("To read ALS interrupt status : cat alsinterruptstatus\n");
	printk("To read ALS gain status : cat alsgainstatus\n");
	printk("To read ALS validity status : cat alsdatavaliditystatus\n");
	printk("To read register ALS_STATUS (0x8C) : cat alsstatusreg\n\n");
	// address 0x8C

	// address 0x8F
	printk("Address 0x8F (INTERRUPT)\n");
	printk("INT output pin inactive : echo 0 > interruptmodesetup\n");
	printk("Only ALS triggers interrupt : echo 1 > interruptmodesetup\n");
	printk("To read interrupt mode : cat interruptmodesetup\n\n");

	printk("INT output pin active low : echo 0 > interruptpolarsetup\n");
	printk("INT output pin active high : echo 1 > interruptpolarsetup\n");
	printk("To read interrupt pin polarity : cat interruptpolarsetup\n\n");

	printk("Write value to INTERRUPT register (0x8F) : echo [hexcode value] > interruptsetup\n");
	printk("Example...to write 0x0B : echo B > interruptsetup or echo b > interruptsetup\n");
	printk("Example...to write 0x13 : echo 13 > interruptsetup\n");
	printk("To read register INTERRUPT (0x8F) : cat interruptsetup\n\n");
	// address 0x8F

	// address 0x9E
	printk("Address 0x9E (INTERRUPT PERSIST)\n");
	printk("Write value to INTERRUPT register (0x9E) : echo [hexcode value] > interruptpersistsetup\n");
	printk("Example...to write 0x0B : echo B > interruptpersistsetup or echo b > interruptpersistsetup\n");
	printk("Example...to write 0x13 : echo 13 > interruptpersistsetup\n");
	printk("To read register INTERRUPT PERSIST (0x9E) : cat interruptpersistsetup\n\n");
	// address 0x9E

	// ALS threshold setting
	printk("ALS threshold setting 0x97, 0x98, 0x99, 0x9A\n");
	printk("To set ALS lo threshold : echo [lo limit in decimal] > setalslothrerange\n");
	printk("Example...To set 20 to lo threshold : echo 20 > setalslothrerange\n");
	printk("To set ALS hi threshold : echo [hi limit in decimal] > setalshithrerange\n");
	printk("Example...To set 999 to hi threshold : echo 999 > setalshithrerange\n");
	printk("To read the threshold values : cat dispalsthrerange\n\n");	
	// ALS threshold setting
	
	return 0;
}

static DEVICE_ATTR(ltr303help, 0666, ltr303help_show, NULL);


static ssize_t alsmodesetup_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint8_t rdback_val = 0;
	struct ltr303_data *ltr303 = sensor_info;	

	ret = als_contr_readback(ALS_MODE_RDBCK, &rdback_val, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS_MODE_RDBCK Fail...\n", __func__);
		return (-1);
	}

	ret = sprintf(buf, "%d\n", rdback_val);

	return ret;
}


static ssize_t alsmodesetup_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int param;
	int8_t ret;
	struct ltr303_data *ltr303 = sensor_info;

	sscanf(buf, "%d", &param);
	dev_dbg(&ltr303->i2c_client->dev, "%s: store value = %d\n", __func__, param);

	ret = als_mode_setup((uint8_t)param, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS mode setup Fail...\n", __func__);
		return (-1);
	}

	return count;	
}

static DEVICE_ATTR(alsmodesetup, 0666, alsmodesetup_show, alsmodesetup_store);


static ssize_t alsswresetsetup_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint8_t rdback_val = 0;
	struct ltr303_data *ltr303 = sensor_info;	

	ret = als_contr_readback(ALS_SWRT_RDBCK, &rdback_val, ltr303);

	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS_SWRT_RDBCK Fail...\n", __func__);
		return (-1);
	}

	ret = sprintf(buf, "%d\n", rdback_val);

	return ret;
}


static ssize_t alsswresetsetup_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int param;
	int8_t ret;

	struct ltr303_data *ltr303 = sensor_info;

	sscanf(buf, "%d", &param);
	dev_dbg(&ltr303->i2c_client->dev, "%s: store value = %d\n", __func__, param);

	ret = als_sw_reset_setup((uint8_t)param, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS sw reset setup Fail...\n", __func__);
		return (-1);
	}

	return count;
}

static DEVICE_ATTR(alsswresetsetup, 0666, alsswresetsetup_show, alsswresetsetup_store);


static ssize_t alsgainsetup_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint8_t rdback_val = 0;
	struct ltr303_data *ltr303 = sensor_info;	

	ret = als_contr_readback(ALS_GAIN_RDBCK, &rdback_val, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS_GAIN_RDBCK Fail...\n", __func__);
		return (-1);
	}

	ret = sprintf(buf, "%d\n", rdback_val);

	return ret;
}


static ssize_t alsgainsetup_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int8_t ret;
	uint8_t param;
	//int *param_temp = buf;
	int param_temp[2];

	struct ltr303_data *ltr303 = sensor_info;

	//sscanf(buf, "%d", param_temp);
	param_temp[0] = buf[0];
	param_temp[1] = buf[1];

	if (count <= 1) {
		param_temp[0] = 0;
		param_temp[1] = 0;
	} else if (count == 2) {
		param_temp[0] -= 48;
		param_temp[1] = 0;

		param_temp[1] = param_temp[0];
		param_temp[0] = 0;
	} else if (count >= 3) {
		param_temp[0] -= 48;
		param_temp[1] -= 48;
	}

	param = ((param_temp[0] * 10) + param_temp[1]);
	dev_dbg(&ltr303->i2c_client->dev, "%s: store value = %d\n", __func__, param);

	ret = als_gain_setup((uint8_t)param, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS gain setup Fail...\n", __func__);
		return (-1);
	}

	return count;

}

static DEVICE_ATTR(alsgainsetup, 0666, alsgainsetup_show, alsgainsetup_store);


static ssize_t alscontrsetup_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint8_t rdback_val = 0;
	struct ltr303_data *ltr303 = sensor_info;	

	ret = als_contr_readback(ALS_CONTR_RDBCK, &rdback_val, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS_CONTR_RDBCK Fail...\n", __func__);
		return (-1);
	}

	ret = sprintf(buf, "%d\n", rdback_val);

	return ret;
}

static ssize_t alscontrsetup_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int8_t ret;
	uint8_t param;
	//int *param_temp = buf;
	int param_temp[2];

	struct ltr303_data *ltr303 = sensor_info;

	//sscanf(buf, "%d", param_temp);
	param_temp[0] = buf[0];
	param_temp[1] = buf[1];

	if (count <= 1) {
		param_temp[0] = 48;
		param_temp[1] = 48;
	} else if (count == 2) {
		param_temp[1] = param_temp[0];
		param_temp[0] = 48;
	}


	if (param_temp[0] >= 65 && param_temp[0] <= 70) {
		param_temp[0] -= 55;
	} else if (param_temp[0] >= 97 && param_temp[0] <= 102) {
		param_temp[0] -= 87;
	} else if (param_temp[0] >= 48 && param_temp[0] <= 57) {
		param_temp[0] -= 48;
	} else {
		param_temp[0] = 0;
	}

	if (param_temp[1] >= 65 && param_temp[1] <= 70) {
		param_temp[1] -= 55;
	} else if (param_temp[1] >= 97 && param_temp[1] <= 102) {
		param_temp[1] -= 87;
	} else if (param_temp[1] >= 48 && param_temp[1] <= 57) {
		param_temp[1] -= 48;
	} else {
		param_temp[1] = 0;
	}

	param = ((param_temp[0] << 4) + (param_temp[1]));
	dev_dbg(&ltr303->i2c_client->dev, "%s: store value = %d\n", __func__, param);

	ret = als_contr_setup(param, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS contr setup Fail...\n", __func__);
		return (-1);
	}

	return count;	
}

static DEVICE_ATTR(alscontrsetup, 0666, alscontrsetup_show, alscontrsetup_store);


static ssize_t alsmeasratesetup_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint8_t rdback_val = 0;
	struct ltr303_data *ltr303 = sensor_info;	

	ret = als_meas_rate_readback(ALS_MEAS_RPT_RATE_RDBCK, &rdback_val, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS_MEAS_RPT_RATE_RDBCK Fail...\n", __func__);
		return (-1);
	}

	ret = sprintf(buf, "%d\n", rdback_val);

	return ret;

}


static ssize_t alsmeasratesetup_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int8_t ret;
	uint16_t param;
	//int *param_temp = buf;
	int param_temp[4];
	
	struct ltr303_data *ltr303 = sensor_info;

	//sscanf(buf, "%d", param_temp);
	param_temp[0] = buf[0];
	param_temp[1] = buf[1];
	param_temp[2] = buf[2];
	param_temp[3] = buf[3];

	if (count <= 2) {
		param_temp[0] = 0;
		param_temp[1] = 0;
		param_temp[2] = 0;
		param_temp[3] = 0;
	} else if (count == 3) {		
		param_temp[0] -= 48;
		param_temp[1] -= 48;
		param_temp[2] = 0;
		param_temp[3] = 0;

		param_temp[3] = param_temp[1];
		param_temp[2] = param_temp[0];
		param_temp[1] = 0;
		param_temp[0] = 0;
	} else if (count == 4) {
		param_temp[0] -= 48;
		param_temp[1] -= 48;
		param_temp[2] -= 48;
		param_temp[3] = 0;

		param_temp[3] = param_temp[2];
		param_temp[2] = param_temp[1];
		param_temp[1] = param_temp[0];
		param_temp[0] = 0;
	} else if (count > 4) {
		param_temp[0] -= 48;
		param_temp[1] -= 48;
		param_temp[2] -= 48;
		param_temp[3] -= 48;
	}

	//for (i_ctr = 0; i_ctr < sizeof(param_temp); i_ctr++) {
	//	if ((param_temp[i_ctr] < 0) ||(param_temp[i_ctr] > 9)) {
	//		param_temp[i_ctr] = 0;
	//	}
	//}

	param = ((param_temp[0] * 1000) + (param_temp[1] * 100) + (param_temp[2] * 10) + param_temp[3]);
	dev_dbg(&ltr303->i2c_client->dev, "%s: store value = %d\n", __func__, param);

	ret = als_meas_rate_setup(param, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS measurement rate setup Fail...\n", __func__);
		return (-1);
	}

	return count;
}

static DEVICE_ATTR(alsmeasratesetup, 0666, alsmeasratesetup_show, alsmeasratesetup_store);


static ssize_t alsintegtimesetup_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint8_t rdback_val = 0;
	struct ltr303_data *ltr303 = sensor_info;	

	ret = als_meas_rate_readback(ALS_INTEG_TM_RDBCK, &rdback_val, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS_INTEG_TM_RDBCK Fail...\n", __func__);
		return (-1);
	}

	ret = sprintf(buf, "%d\n", rdback_val);

	return ret;
	
}


static ssize_t alsintegtimesetup_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int8_t ret;
	uint16_t param;
	//int *param_temp = buf;

	int param_temp[3];
	
	struct ltr303_data *ltr303 = sensor_info;

	//sscanf(buf, "%d", param_temp);
	param_temp[0] = buf[0];
	param_temp[1] = buf[1];
	param_temp[2] = buf[2];

	if (count <= 2) {
		param_temp[0] = 0;
		param_temp[1] = 0;
		param_temp[2] = 0;
	} else if (count == 3) {
		param_temp[0] -= 48;
		param_temp[1] -= 48;
		param_temp[2] = 0;

		param_temp[2] = param_temp[1];
		param_temp[1] = param_temp[0];
		param_temp[0] = 0;
	} else if (count > 3) {
		param_temp[0] -= 48;
		param_temp[1] -= 48;
		param_temp[2] -= 48;
	}

	//for (i_ctr = 0; i_ctr < sizeof(param_temp); i_ctr++) {
	//	if ((param_temp[i_ctr] < 0) ||(param_temp[i_ctr] > 9)) {
	//		param_temp[i_ctr] = 0;
	//	}
	//}

	param = ((param_temp[0] * 100) + (param_temp[1] * 10) + param_temp[2]);
	dev_dbg(&ltr303->i2c_client->dev, "%s: store value = %d\n", __func__, param);

	ret = als_integ_time_setup(param, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS integration time setup Fail...\n", __func__);
		return (-1);
	}

	return count;
}

static DEVICE_ATTR(alsintegtimesetup, 0666, alsintegtimesetup_show, alsintegtimesetup_store);


static ssize_t alsmeasrateregsetup_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint8_t rdback_val = 0;
	struct ltr303_data *ltr303 = sensor_info;	

	ret = als_meas_rate_readback(ALS_MEAS_RATE_RDBCK, &rdback_val, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS_MEAS_RATE_RDBCK Fail...\n", __func__);
		return (-1);
	}

	ret = sprintf(buf, "%d\n", rdback_val);

	return ret;
	
}


static ssize_t alsmeasrateregsetup_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int8_t ret;
	uint8_t param;
	//int *param_temp = buf;
	int param_temp[2];

	struct ltr303_data *ltr303 = sensor_info;

	//sscanf(buf, "%d", param_temp);
	param_temp[0] = buf[0];
	param_temp[1] = buf[1];

	if (count <= 1) {
		param_temp[0] = 48;
		param_temp[1] = 48;
	} else if (count == 2) {
		param_temp[1] = param_temp[0];
		param_temp[0] = 48;
	}

	if (param_temp[0] >= 65 && param_temp[0] <= 70) {
		param_temp[0] -= 55;
	} else if (param_temp[0] >= 97 && param_temp[0] <= 102) {
		param_temp[0] -= 87;
	} else if (param_temp[0] >= 48 && param_temp[0] <= 57) {
		param_temp[0] -= 48;
	} else {
		param_temp[0] = 0;
	}

	if (param_temp[1] >= 65 && param_temp[1] <= 70) {
		param_temp[1] -= 55;
	} else if (param_temp[1] >= 97 && param_temp[1] <= 102) {
		param_temp[1] -= 87;
	} else if (param_temp[1] >= 48 && param_temp[1] <= 57) {
		param_temp[1] -= 48;
	} else {
		param_temp[1] = 0;
	}

	param = ((param_temp[0] << 4) + (param_temp[1]));
	dev_dbg(&ltr303->i2c_client->dev, "%s: store value = %d\n", __func__, param);

	ret = als_meas_rate_reg_setup(param, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS meas rate register setup Fail...\n", __func__);
		return (-1);
	}

	return count;
}

static DEVICE_ATTR(alsmeasrateregsetup, 0666, alsmeasrateregsetup_show, alsmeasrateregsetup_store);


static ssize_t partid_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint8_t rdback_val = 0;
	struct ltr303_data *ltr303 = sensor_info;

	ret = part_ID_reg_readback(PART_NUM_ID_RDBCK, &rdback_val, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: PART_NUM_ID_RDBCK Fail...\n", __func__);
		return (-1);
	}

	ret = sprintf(buf, "%d\n", rdback_val);

	return ret;
}

static DEVICE_ATTR(partid, 0666, partid_show, NULL);


static ssize_t revid_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint8_t rdback_val = 0;
	struct ltr303_data *ltr303 = sensor_info;

	ret = part_ID_reg_readback(REVISION_ID_RDBCK, &rdback_val, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: REVISION_ID_RDBCK Fail...\n", __func__);
		return (-1);
	}

	ret = sprintf(buf, "%d\n", rdback_val);

	return ret;
}

static DEVICE_ATTR(revid, 0666, revid_show, NULL);


static ssize_t partidreg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint8_t rdback_val = 0;
	struct ltr303_data *ltr303 = sensor_info;

	ret = part_ID_reg_readback(PART_ID_REG_RDBCK, &rdback_val, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: PART_ID_REG_RDBCK Fail...\n", __func__);
		return (-1);
	}

	ret = sprintf(buf, "%d\n", rdback_val);

	return ret;
}

static DEVICE_ATTR(partidreg, 0666, partidreg_show, NULL);


static ssize_t manuid_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint8_t rdback_val = 0;
	struct ltr303_data *ltr303 = sensor_info;

	ret = manu_ID_reg_readback(&rdback_val, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: Manufacturing ID readback Fail...\n", __func__);
		return (-1);
	}

	ret = sprintf(buf, "%d\n", rdback_val);

	return ret;
}

static DEVICE_ATTR(manuid, 0666, manuid_show, NULL);


static ssize_t alsdatastatus_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint8_t rdback_val = 0;
	struct ltr303_data *ltr303 = sensor_info;

	ret = als_status_reg(ALS_DATA_STATUS_RDBCK, &rdback_val, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS_DATA_STATUS_RDBCK Fail...\n", __func__);
		return (-1);
	}

	ret = sprintf(buf, "%d\n", rdback_val);

	return ret;
}

static DEVICE_ATTR(alsdatastatus, 0666, alsdatastatus_show, NULL);


static ssize_t alsinterruptstatus_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint8_t rdback_val = 0;
	struct ltr303_data *ltr303 = sensor_info;

	ret = als_status_reg(ALS_INTERR_STATUS_RDBCK, &rdback_val, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS_INTERR_STATUS_RDBCK Fail...\n", __func__);
		return (-1);
	}

	ret = sprintf(buf, "%d\n", rdback_val);

	return ret;
}

static DEVICE_ATTR(alsinterruptstatus, 0666, alsinterruptstatus_show, NULL);


static ssize_t alsgainstatus_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint8_t rdback_val = 0;
	struct ltr303_data *ltr303 = sensor_info;

	ret = als_status_reg(ALS_GAIN_STATUS_RDBCK, &rdback_val, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS_GAIN_STATUS_RDBCK Fail...\n", __func__);
		return (-1);
	}

	ret = sprintf(buf, "%d\n", rdback_val);

	return ret;
}

static DEVICE_ATTR(alsgainstatus, 0666, alsgainstatus_show, NULL);


static ssize_t alsdatavaliditystatus_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint8_t rdback_val = 0;
	struct ltr303_data *ltr303 = sensor_info;

	ret = als_status_reg(ALS_VALID_STATUS_RDBCK, &rdback_val, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS_VALID_STATUS_RDBCK Fail...\n", __func__);
		return (-1);
	}

	ret = sprintf(buf, "%d\n", rdback_val);

	return ret;
}

static DEVICE_ATTR(alsdatavaliditystatus, 0666, alsdatavaliditystatus_show, NULL);


static ssize_t alsstatusreg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint8_t rdback_val = 0;
	struct ltr303_data *ltr303 = sensor_info;	

	ret = als_status_reg(ALS_STATUS_RDBCK, &rdback_val, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS_STATUS_RDBCK Fail...\n", __func__);
		return (-1);
	}

	ret = sprintf(buf, "%d\n", rdback_val);

	return ret;
	
}

static DEVICE_ATTR(alsstatusreg, 0666, alsstatusreg_show, NULL);


static ssize_t interruptmodesetup_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint8_t rdback_val = 0;
	struct ltr303_data *ltr303 = sensor_info;	

	ret = interrupt_readback(INT_MODE_RDBCK, &rdback_val, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: INT_MODE_RDBCK Fail...\n", __func__);
		return (-1);
	}

	ret = sprintf(buf, "%d\n", rdback_val);

	return ret;
}


static ssize_t interruptmodesetup_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int param;
	int8_t ret;

	struct ltr303_data *ltr303 = sensor_info;

	sscanf(buf, "%d", &param);
	dev_dbg(&ltr303->i2c_client->dev, "%s: store value = %d\n", __func__, param);

	ret = interrupt_mode_setup((uint8_t)param, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: interrupt mode setup Fail...\n", __func__);
		return (-1);
	}

	return count;
}

static DEVICE_ATTR(interruptmodesetup, 0666, interruptmodesetup_show, interruptmodesetup_store);


static ssize_t interruptpolarsetup_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint8_t rdback_val = 0;
	struct ltr303_data *ltr303 = sensor_info;	

	ret = interrupt_readback(INT_POLAR_RDBCK, &rdback_val, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: INT_POLAR_RDBCK Fail...\n", __func__);
		return (-1);
	}

	ret = sprintf(buf, "%d\n", rdback_val);

	return ret;	
}


static ssize_t interruptpolarsetup_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int param;
	int8_t ret;

	struct ltr303_data *ltr303 = sensor_info;

	sscanf(buf, "%d", &param);
	dev_dbg(&ltr303->i2c_client->dev, "%s: store value = %d\n", __func__, param);

	ret = interrupt_polarity_setup((uint8_t)param, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: interrupt polarity setup Fail...\n", __func__);
		return (-1);
	}

	return count;
}

static DEVICE_ATTR(interruptpolarsetup, 0666, interruptpolarsetup_show, interruptpolarsetup_store);


static ssize_t interruptsetup_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint8_t rdback_val = 0;
	struct ltr303_data *ltr303 = sensor_info;	

	ret = interrupt_readback(INT_INTERRUPT_RDBCK, &rdback_val, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: INT_INTERRUPT_RDBCK Fail...\n", __func__);
		return (-1);
	}

	ret = sprintf(buf, "%d\n", rdback_val);

	return ret;	
}


static ssize_t interruptsetup_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int8_t ret;
	uint8_t param;
	//int *param_temp = buf;
	int param_temp[2];

	struct ltr303_data *ltr303 = sensor_info;

	//sscanf(buf, "%d", param_temp);
	param_temp[0] = buf[0];
	param_temp[1] = buf[1];

	if (count <= 1) {
		param_temp[0] = 48;
		param_temp[1] = 48;
	} else if (count == 2) {
		param_temp[1] = param_temp[0];
		param_temp[0] = 48;
	}

	if (param_temp[0] >= 65 && param_temp[0] <= 70) {
		param_temp[0] -= 55;
	} else if (param_temp[0] >= 97 && param_temp[0] <= 102) {
		param_temp[0] -= 87;
	} else if (param_temp[0] >= 48 && param_temp[0] <= 57) {
		param_temp[0] -= 48;
	} else {
		param_temp[0] = 0;
	}

	if (param_temp[1] >= 65 && param_temp[1] <= 70) {
		param_temp[1] -= 55;
	} else if (param_temp[1] >= 97 && param_temp[1] <= 102) {
		param_temp[1] -= 87;
	} else if (param_temp[1] >= 48 && param_temp[1] <= 57) {
		param_temp[1] -= 48;
	} else {
		param_temp[1] = 0;
	}

	param = ((param_temp[0] << 4) + (param_temp[1]));
	dev_dbg(&ltr303->i2c_client->dev, "%s: store value = %d\n", __func__, param);

	ret = interrupt_setup(param, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: interrupt setup Fail...\n", __func__);
		return (-1);
	}

	return count;
}

static DEVICE_ATTR(interruptsetup, 0666, interruptsetup_show, interruptsetup_store);


static ssize_t interruptpersistsetup_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint8_t rdback_val = 0;
	struct ltr303_data *ltr303 = sensor_info;	

	ret = interrupt_prst_readback(&rdback_val, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: Interrupt persist readback Fail...\n", __func__);
		return (-1);
	}

	ret = sprintf(buf, "%d\n", rdback_val);

	return ret;
}


static ssize_t interruptpersistsetup_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int8_t ret = 0;
	uint8_t prst_val;
	//int *param_temp = buf;
	int param_temp[2];

	struct ltr303_data *ltr303 = sensor_info;

	param_temp[0] = buf[0];
	param_temp[1] = buf[1];

	if (count <= 1) {
		param_temp[0] = 48;
		param_temp[1] = 48;
	} else if (count == 2) {
		param_temp[1] = param_temp[0];
		param_temp[0] = 48;
	}

	if (param_temp[0] >= 65 && param_temp[0] <= 70) {
		param_temp[0] -= 55;
	} else if (param_temp[0] >= 97 && param_temp[0] <= 102) {
		param_temp[0] -= 87;
	} else if (param_temp[0] >= 48 && param_temp[0] <= 57) {
		param_temp[0] -= 48;
	} else {
		param_temp[0] = 0;
	}

	if (param_temp[1] >= 65 && param_temp[1] <= 70) {
		param_temp[1] -= 55;
	} else if (param_temp[1] >= 97 && param_temp[1] <= 102) {
		param_temp[1] -= 87;
	} else if (param_temp[1] >= 48 && param_temp[1] <= 57) {
		param_temp[1] -= 48;
	} else {
		param_temp[1] = 0;
	}

	prst_val = ((param_temp[0] << 4) + (param_temp[1]));
	dev_dbg(&ltr303->i2c_client->dev, "%s: store value = %d\n", __func__, prst_val);

	ret = interrupt_persist_setup(prst_val, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: Interrupt persist setup Fail...\n", __func__);
		return (-1);
	}

	return count;
		
}

static DEVICE_ATTR(interruptpersistsetup, 0666, interruptpersistsetup_show, interruptpersistsetup_store);


static ssize_t setalslothrerange_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int8_t ret;
	int lo_thr = 0;
	uint8_t param_temp[5];
	struct ltr303_data *ltr303 = sensor_info;

	param_temp[0] = buf[0];
	param_temp[1] = buf[1];
	param_temp[2] = buf[2];
	param_temp[3] = buf[3];
	param_temp[4] = buf[4];

	if (count <= 1) {
		param_temp[0] = 0;
		param_temp[1] = 0;
		param_temp[2] = 0;
		param_temp[3] = 0;
		param_temp[4] = 0;
	} else if (count == 2) { // 1 digit
		param_temp[0] -= 48;
		param_temp[1] = 0;
		param_temp[2] = 0;
		param_temp[3] = 0;
		param_temp[4] = 0;

		param_temp[4] = param_temp[0];
		param_temp[3] = 0;
		param_temp[2] = 0;
		param_temp[1] = 0;
		param_temp[0] = 0;
	} else if (count == 3) { // 2 digits
		param_temp[0] -= 48;
		param_temp[1] -= 48;
		param_temp[2] = 0;
		param_temp[3] = 0;
		param_temp[4] = 0;

		param_temp[4] = param_temp[1];
		param_temp[3] = param_temp[0];
		param_temp[2] = 0;
		param_temp[1] = 0;
		param_temp[0] = 0;
	} else if (count == 4) { // 3 digits
		param_temp[0] -= 48;
		param_temp[1] -= 48;
		param_temp[2] -= 48;
		param_temp[3] = 0;
		param_temp[4] = 0;

		param_temp[4] = param_temp[2];
		param_temp[3] = param_temp[1];
		param_temp[2] = param_temp[0];
		param_temp[1] = 0;
		param_temp[0] = 0;
	} else if (count == 5) { // 4 digits
		param_temp[0] -= 48;
		param_temp[1] -= 48;
		param_temp[2] -= 48;
		param_temp[3] -= 48;
		param_temp[4] = 0;

		param_temp[4] = param_temp[3];
		param_temp[3] = param_temp[2];
		param_temp[2] = param_temp[1];
		param_temp[1] = param_temp[0];
		param_temp[0] = 0;
	} else if (count >= 6) { // 5 digits
		param_temp[0] -= 48;
		param_temp[1] -= 48;
		param_temp[2] -= 48;
		param_temp[3] -= 48;
		param_temp[4] -= 48;
	}

	//for (i_ctr = 0; i_ctr < sizeof(param_temp); i_ctr++) {
	//	if ((param_temp[i_ctr] < 0) ||(param_temp[i_ctr] > 9)) {
	//		param_temp[i_ctr] = 0;
	//	}
	//}

	lo_thr = ((param_temp[0] * 10000) + (param_temp[1] * 1000) + (param_temp[2] * 100) + (param_temp[3] * 10) + param_temp[4]);
	if (lo_thr > 65535) {
		lo_thr = 65535;
	}
	dev_dbg(&ltr303->i2c_client->dev, "%s: store value = %d\n", __func__, lo_thr);

	ret = set_als_range((uint16_t)lo_thr, 0, LO_LIMIT);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: set ALS lo threshold Fail...\n", __func__);
		return (-1);
	}
	
	return count;
}

static DEVICE_ATTR(setalslothrerange, 0666, NULL, setalslothrerange_store);


static ssize_t setalshithrerange_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int8_t ret;
	int hi_thr = 0;
	uint8_t param_temp[5];
	struct ltr303_data *ltr303 = sensor_info;

	param_temp[0] = buf[0];
	param_temp[1] = buf[1];
	param_temp[2] = buf[2];
	param_temp[3] = buf[3];
	param_temp[4] = buf[4];

	if (count <= 1) {
		param_temp[0] = 0;
		param_temp[1] = 0;
		param_temp[2] = 0;
		param_temp[3] = 0;
		param_temp[4] = 0;
	} else if (count == 2) { // 1 digit
		param_temp[0] -= 48;
		param_temp[1] = 0;
		param_temp[2] = 0;
		param_temp[3] = 0;
		param_temp[4] = 0;

		param_temp[4] = param_temp[0];
		param_temp[3] = 0;
		param_temp[2] = 0;
		param_temp[1] = 0;
		param_temp[0] = 0;
	} else if (count == 3) { // 2 digits
		param_temp[0] -= 48;
		param_temp[1] -= 48;
		param_temp[2] = 0;
		param_temp[3] = 0;
		param_temp[4] = 0;

		param_temp[4] = param_temp[1];
		param_temp[3] = param_temp[0];
		param_temp[2] = 0;
		param_temp[1] = 0;
		param_temp[0] = 0;
	} else if (count == 4) { // 3 digits
		param_temp[0] -= 48;
		param_temp[1] -= 48;
		param_temp[2] -= 48;
		param_temp[3] = 0;
		param_temp[4] = 0;

		param_temp[4] = param_temp[2];
		param_temp[3] = param_temp[1];
		param_temp[2] = param_temp[0];
		param_temp[1] = 0;
		param_temp[0] = 0;
	} else if (count == 5) { // 4 digits
		param_temp[0] -= 48;
		param_temp[1] -= 48;
		param_temp[2] -= 48;
		param_temp[3] -= 48;
		param_temp[4] = 0;

		param_temp[4] = param_temp[3];
		param_temp[3] = param_temp[2];
		param_temp[2] = param_temp[1];
		param_temp[1] = param_temp[0];
		param_temp[0] = 0;
	} else if (count >= 6) { // 5 digits
		param_temp[0] -= 48;
		param_temp[1] -= 48;
		param_temp[2] -= 48;
		param_temp[3] -= 48;
		param_temp[4] -= 48;
	}

	//for (i_ctr = 0; i_ctr < sizeof(param_temp); i_ctr++) {
	//	if ((param_temp[i_ctr] < 0) ||(param_temp[i_ctr] > 9)) {
	//		param_temp[i_ctr] = 0;
	//	}
	//}

	hi_thr = ((param_temp[0] * 10000) + (param_temp[1] * 1000) + (param_temp[2] * 100) + (param_temp[3] * 10) + param_temp[4]);
	if (hi_thr > 65535) {
		hi_thr = 65535;
	}
	dev_dbg(&ltr303->i2c_client->dev, "%s: store value = %d\n", __func__, hi_thr);

	ret = set_als_range(0, (uint16_t)hi_thr, HI_LIMIT);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: set ALS hi threshold Fail...\n", __func__);
		return (-1);
	}
	
	return count;
}

static DEVICE_ATTR(setalshithrerange, 0666, NULL, setalshithrerange_store);


static ssize_t dispalsthrerange_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint16_t rdback_lo, rdback_hi;
	struct ltr303_data *ltr303 = sensor_info;

	ret = als_range_readback(&rdback_lo, &rdback_hi, ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS threshold range readback Fail...\n", __func__);
		return (-1);
	}
	
	ret = sprintf(buf, "%d %d\n", rdback_lo, rdback_hi);

	return ret;
}

static DEVICE_ATTR(dispalsthrerange, 0666, dispalsthrerange_show, NULL);


static void sysfs_register_device(struct i2c_client *client) 
{
	int rc = 0;

	rc += device_create_file(&client->dev, &dev_attr_als_adc);
	rc += device_create_file(&client->dev, &dev_attr_ltr303help);
	rc += device_create_file(&client->dev, &dev_attr_alsmodesetup);
	rc += device_create_file(&client->dev, &dev_attr_alsswresetsetup);
	rc += device_create_file(&client->dev, &dev_attr_alsgainsetup);
	rc += device_create_file(&client->dev, &dev_attr_alscontrsetup);
	rc += device_create_file(&client->dev, &dev_attr_alsmeasratesetup);
	rc += device_create_file(&client->dev, &dev_attr_alsintegtimesetup);
	rc += device_create_file(&client->dev, &dev_attr_alsmeasrateregsetup);
	rc += device_create_file(&client->dev, &dev_attr_partid);
	rc += device_create_file(&client->dev, &dev_attr_revid);
	rc += device_create_file(&client->dev, &dev_attr_partidreg);
	rc += device_create_file(&client->dev, &dev_attr_manuid);
	rc += device_create_file(&client->dev, &dev_attr_alsdatastatus);
	rc += device_create_file(&client->dev, &dev_attr_alsinterruptstatus);
	rc += device_create_file(&client->dev, &dev_attr_alsgainstatus);
	rc += device_create_file(&client->dev, &dev_attr_alsdatavaliditystatus);
	rc += device_create_file(&client->dev, &dev_attr_alsstatusreg);
	rc += device_create_file(&client->dev, &dev_attr_interruptmodesetup);
	rc += device_create_file(&client->dev, &dev_attr_interruptpolarsetup);
	rc += device_create_file(&client->dev, &dev_attr_interruptsetup);
	rc += device_create_file(&client->dev, &dev_attr_interruptpersistsetup);
	rc += device_create_file(&client->dev, &dev_attr_setalslothrerange);
	rc += device_create_file(&client->dev, &dev_attr_setalshithrerange);
	rc += device_create_file(&client->dev, &dev_attr_dispalsthrerange);
	
	if (rc) {
		dev_err(&client->dev, "%s Unable to create sysfs files\n", __func__);
	} else {
		dev_dbg(&client->dev, "%s Created sysfs files\n", __func__);
	}
}


static int als_setup(struct ltr303_data *ltr303)
{
	int ret;

	ltr303->als_input_dev = input_allocate_device();
	if (!ltr303->als_input_dev) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS Input Allocate Device Fail...\n", __func__);
		return -ENOMEM;
	}
	ltr303->als_input_dev->name = "ltr303_als";
	set_bit(EV_ABS, ltr303->als_input_dev->evbit);
	input_set_abs_params(ltr303->als_input_dev, ABS_MISC, ALS_MIN_MEASURE_VAL, ALS_MAX_MEASURE_VAL, 0, 0);

	ret = input_register_device(ltr303->als_input_dev);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS Register Input Device Fail...\n", __func__);
		goto err_als_register_input_device;
	}

	ret = misc_register(&als_misc);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS Register Misc Device Fail...\n", __func__);
		goto err_als_register_misc_device;
	}

	return ret;

err_als_register_misc_device:
	input_unregister_device(ltr303->als_input_dev);
err_als_register_input_device:
	input_free_device(ltr303->als_input_dev);

	return ret;
}


static uint8_t _check_part_id(struct ltr303_data *ltr303)
{
	uint8_t ret;
	uint8_t buffer[2];

	buffer[0] = LTR303_PART_ID;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: Read failure :0x%02X",
		        __func__, buffer[0]);
		return -1;
	}

	if (buffer[0] != PARTID) {
		dev_err(&ltr303->i2c_client->dev, "%s: Part failure miscompare"
		        " act:0x%02x exp:0x%02x\n", __func__, buffer[0], PARTID);
		return -2;
	}

	return 0;
}


static int ltr303_setup(struct ltr303_data *ltr303)
{
	int ret = 0;

	/* Reset the devices */
	ret = _ltr303_set_bit(ltr303->i2c_client, SET_BIT, LTR303_ALS_CONTR, ALS_SW_RESET);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS reset fail...\n", __func__);
		goto err_out1;
	}

	msleep(PON_DELAY);
	dev_dbg(&ltr303->i2c_client->dev, "%s: Reset ltr303 device\n", __func__);

	/* Do another part read to ensure we have exited reset */
	if (_check_part_id(ltr303) < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: Part ID Read Fail after reset...\n", __func__);
		goto err_out1;
	}

	//(Linux RTOS)>
#if 1
	ret = ltr303_gpio_irq(ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: GPIO Request Fail...\n", __func__);
		goto err_out1;
	}
	dev_dbg(&ltr303->i2c_client->dev, "%s Requested interrupt\n", __func__);
#endif
	//(Linux RTOS)<

	/* Set count of measurements outside data range before interrupt is generated */
	ret = _ltr303_set_bit(ltr303->i2c_client, SET_BIT, LTR303_INTERRUPT_PRST, 0x01);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: ALS Set Persist Fail...\n", __func__);
		goto err_out2;
	}

	dev_dbg(&ltr303->i2c_client->dev, "%s: Set ltr303 persists\n", __func__);

	/* Enable interrupts on the device and clear only when status is read */
#if ACT_INTERRUPT
	ret = _ltr303_set_bit(ltr303->i2c_client, SET_BIT, LTR303_INTERRUPT, INT_MODE_ALS_TRIG);
#else
	ret = _ltr303_set_bit(ltr303->i2c_client, SET_BIT, LTR303_INTERRUPT, INT_MODE_0);
#endif
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: Enabled interrupts failed...\n", __func__);
		goto err_out2;
	}
	dev_dbg(&ltr303->i2c_client->dev, "%s Enabled interrupt to device\n", __func__);

	/* Turn on ALS */
	ret = als_enable_init(ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s Unable to enable ALS", __func__);
		goto err_out2;
	}
	dev_info(&ltr303->i2c_client->dev, "%s Turned on ambient light sensor\n", __func__);

	return ret;

err_out2:
	free_irq(ltr303->irq, ltr303);
	gpio_free(ltr303->gpio_int_no);

err_out1:
	dev_err(&ltr303->i2c_client->dev, "%s Unable to setup device\n", __func__);

	return ret;
}


//(Linux RTOS)>
#if 0
static void ltr303_early_suspend(struct early_suspend *h)
{
	int ret = 0;
	struct ltr303_data *ltr303 = sensor_info;

	if (ltr303->is_suspend != 0) {
		dev_err(&ltr303->i2c_client->dev, "%s Asked to suspend when already suspended\n", __func__);
		return;
	}
	ltr303->is_suspend = 1;

	/* Save away the state of the devices at suspend point */
	ltr303->als_suspend_enable_flag = ltr303->als_enable_flag;

	/* Disable the devices for suspend if configured */
	if (ltr303->disable_als_on_suspend && ltr303->als_enable_flag) {
		ret += als_disable(ltr303);
	}

	if (ret) {
		dev_err(&ltr303->i2c_client->dev, "%s Unable to complete suspend\n", __func__);
	} else {
		dev_info(&ltr303->i2c_client->dev, "%s Suspend completed\n", __func__);
	}
}


static void ltr303_late_resume(struct early_suspend *h)
{
	struct ltr303_data *ltr303 = sensor_info;
	int ret = 0;

	if (ltr303->is_suspend != 1) {
		dev_err(&ltr303->i2c_client->dev, "%s Asked to resume when not suspended\n", __func__);
		return;
	}
	ltr303->is_suspend = 0;

	/* If ALS was enbled before suspend, enable during resume */
	if (ltr303->als_suspend_enable_flag) {
		ret += als_enable_init(ltr303);
		ltr303->als_suspend_enable_flag = 0;
	}	

	if (ret) {
		dev_err(&ltr303->i2c_client->dev, "%s Unable to complete resume\n", __func__);
	} else {
		dev_info(&ltr303->i2c_client->dev, "%s Resume completed\n", __func__);
	}
}
#endif
//(Linux RTOS)<


static int ltr303_irq_init(struct ltr303_data *ltr303)
{
	struct i2c_client *client;
        struct device *dev;
        int ret;

        if (!ltr303)
                return -EINVAL;

        client = ltr303->i2c_client;
        dev = &client->dev;

        ret = gpio_to_irq(ltr303->gpio_int_no);
        if (ret < 0) {
                dev_err(dev, "gpio %d irq failed\n", ltr303->gpio_int_no);
                return ret;
        }

        ltr303->irq = ret;

        /* Update client irq if its invalid */
        if (client->irq < 0)
                client->irq = ret;

        dev_dbg(dev, "gpio no:%d irq:%d\n", ltr303->gpio_int_no, ltr303->irq);

        return 0;
}

static int ltr303_acpi_probe(struct ltr303_data *ltr303)
{
        struct i2c_client *client = ltr303->i2c_client;
        struct device *dev;
        struct gpio_desc *gpio;
        int ret, index = 0;


        printk( "### MyDebug gsl_acpi_probe ... \n" );

        if (!client)
                return -EINVAL;

        dev = &client->dev;
        if (!ACPI_HANDLE(dev))
                return -ENODEV;

        printk( "### MyDebug  ACPI_HANDLE ok \n" );

        /* touch gpio interrupt pin */
        gpio = devm_gpiod_get_index(dev, "ltr303_intr_gpio", index);
        if (IS_ERR(gpio)) {
                dev_err(dev, "acpi gpio get index %d failed\n", index);
                return PTR_ERR(gpio);
        }

        ltr303->gpio_int_no = desc_to_gpio(gpio);
	ret = ltr303_irq_init(ltr303);
	if (ret < 0) {
                dev_err(dev, "irq init failed\n");
                return ret;
        }

        printk( "gpio reset resource %d, no %d\n", index, ltr303->gpio_int_no);
	return 0;
}
static int ltr303_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	struct ltr303_data *ltr303;
//(Linux RTOS)>
#if 1
	struct ltr303_platform_data *platdata;
#endif
//(Linux RTOS)<

	ltr303 = kzalloc(sizeof(struct ltr303_data), GFP_KERNEL);
	if (!ltr303)
	{
		dev_err(&ltr303->i2c_client->dev, "%s: Mem Alloc Fail...\n", __func__);
		return -ENOMEM;
	}

	/* Global pointer for this device */
	sensor_info = ltr303;

	/* Set initial defaults */
	ltr303->als_enable_flag = 0;
	
	ltr303->i2c_client = client;
	//ltr303->irq = client->irq;
	ret = ltr303_acpi_probe(ltr303);
	if(ret < 0) {
             printk(" ltr303_acpi_probe error \n");
             goto err_out;
        }
	i2c_set_clientdata(client, ltr303);

	/* Parse the platform data */
	//(Linux RTOS)>
#if 0
	platdata = client->dev.platform_data;
	if (!platdata) {
		dev_err(&ltr303->i2c_client->dev, "%s: Platform Data assign Fail...\n", __func__);
		ret = -EBUSY;
		goto err_out;
	}

	ltr303->gpio_int_no = platdata->pfd_gpio_int_no;
	//ltr303->adc_levels = platdata->pfd_levels;
	
	/* Configuration to set or disable devices upon suspend */
	//ltr303->disable_als_on_suspend = platdata->pfd_disable_als_on_suspend;	
#endif
	//(Linux RTOS)<

	if (_check_part_id(ltr303) < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: Part ID Read Fail...\n", __func__);
		goto err_out;
	}

	/* Setup the input subsystem for the ALS */
	ret = als_setup(ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev,"%s: ALS Setup Fail...\n", __func__);
		goto err_out;
	}

	/* Create the workqueue for the interrup handler */
	//(Linux RTOS)>
#if 1
	ltr303->workqueue = create_singlethread_workqueue("ltr303_wq");
	if (!ltr303->workqueue) {
		dev_err(&ltr303->i2c_client->dev, "%s: Create WorkQueue Fail...\n", __func__);
		ret = -ENOMEM;
		goto err_out;
	}	
#endif
	//(Linux RTOS)<

	/* Setup and configure the ALS on the ltr303 device */
	ret = ltr303_setup(ltr303);
	if (ret < 0) {
		dev_err(&ltr303->i2c_client->dev, "%s: Setup Fail...\n", __func__);
		goto err_ltr303_setup;
	}

	/* Setup the suspend and resume functionality */
	//(Linux RTOS)>
#if 0
	ltr303->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ltr303->early_suspend.suspend = ltr303_early_suspend;
	ltr303->early_suspend.resume = ltr303_late_resume;
	register_early_suspend(&ltr303->early_suspend);
#endif
	//(Linux RTOS)<

	/* Register the sysfs files */
	sysfs_register_device(client);
	//sysfs_register_als_device(client, &ltr303->als_input_dev->dev);

	dev_dbg(&ltr303->i2c_client->dev, "%s: probe complete\n", __func__);

	return ret;

err_ltr303_setup:
	destroy_workqueue(ltr303->workqueue);
err_out:
	kfree(ltr303);

	return ret;
}


static const struct i2c_device_id ltr303_id[] = {
	{ "LTER0303:00", 0 },
	//{ DEVICE_NAME, 0 },
	{}
};

static struct i2c_driver ltr303_driver = {
	.probe = ltr303_probe,
	.id_table = ltr303_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = DEVICE_NAME,
	},
};


static int __init ltr303_init(void)
{
	return i2c_add_driver(&ltr303_driver);
}

static void __exit ltr303_exit(void)
{
	i2c_del_driver(&ltr303_driver);
}


module_init(ltr303_init)
module_exit(ltr303_exit)

MODULE_AUTHOR("Lite-On Technology Corp");
MODULE_DESCRIPTION("LTR-303ALS Driver");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION(DRIVER_VERSION);


