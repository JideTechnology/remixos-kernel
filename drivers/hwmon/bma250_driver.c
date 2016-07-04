/*  Date: 2011/12/29 12:37:00
 *  Revision: 2.1
 */

/*
 * This software program is licensed subject to the GNU General Public License
 * (GPL).Version 2,June 1991, available at http://www.fsf.org/copyleft/gpl.html

 * (C) Copyright 2011 Bosch Sensortec GmbH
 * All Rights Reserved
 */


#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/kernel.h>
#include <linux/delay.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#define SENSOR_NAME 			"bma250"
#define GRAVITY_EARTH                   9806550
#define ABSMIN_2G                       (-GRAVITY_EARTH * 2)
#define ABSMAX_2G                       (GRAVITY_EARTH * 2)
#define SLOPE_THRESHOLD_VALUE 		32
#define SLOPE_DURATION_VALUE 		1
#define INTERRUPT_LATCH_MODE 		13
#define INTERRUPT_ENABLE 		1
#define INTERRUPT_DISABLE 		0
#define MAP_SLOPE_INTERRUPT 		2
#define SLOPE_X_INDEX 			5
#define SLOPE_Y_INDEX 			6
#define SLOPE_Z_INDEX 			7

#define BMA250_MIN_DELAY		1
#define BMA250_DEFAULT_DELAY	200

#define BMA250_CHIP_ID			3
#define BMA250E_CHIP_ID			0xF9
#define BMA253_CHIP_ID			0xFA
#define BMA250_RANGE_SET		0
#define BMA250_BW_SET			4


/*
 *
 *      register definitions
 *
 */

#define BMA250_CHIP_ID_REG                      0x00
#define BMA250_VERSION_REG                      0x01
#define BMA250_X_AXIS_LSB_REG                   0x02
#define BMA250_X_AXIS_MSB_REG                   0x03
#define BMA250_Y_AXIS_LSB_REG                   0x04
#define BMA250_Y_AXIS_MSB_REG                   0x05
#define BMA250_Z_AXIS_LSB_REG                   0x06
#define BMA250_Z_AXIS_MSB_REG                   0x07
#define BMA250_TEMP_RD_REG                      0x08
#define BMA250_STATUS1_REG                      0x09
#define BMA250_STATUS2_REG                      0x0A
#define BMA250_STATUS_TAP_SLOPE_REG             0x0B
#define BMA250_STATUS_ORIENT_HIGH_REG           0x0C
#define BMA250_RANGE_SEL_REG                    0x0F
#define BMA250_BW_SEL_REG                       0x10
#define BMA250_MODE_CTRL_REG                    0x11
#define BMA250_LOW_NOISE_CTRL_REG               0x12
#define BMA250_DATA_CTRL_REG                    0x13
#define BMA250_RESET_REG                        0x14
#define BMA250_INT_ENABLE1_REG                  0x16
#define BMA250_INT_ENABLE2_REG                  0x17
#define BMA250_INT1_PAD_SEL_REG                 0x19
#define BMA250_INT_DATA_SEL_REG                 0x1A
#define BMA250_INT2_PAD_SEL_REG                 0x1B
#define BMA250_INT_SRC_REG                      0x1E
#define BMA250_INT_SET_REG                      0x20
#define BMA250_INT_CTRL_REG                     0x21
#define BMA250_LOW_DURN_REG                     0x22
#define BMA250_LOW_THRES_REG                    0x23
#define BMA250_LOW_HIGH_HYST_REG                0x24
#define BMA250_HIGH_DURN_REG                    0x25
#define BMA250_HIGH_THRES_REG                   0x26
#define BMA250_SLOPE_DURN_REG                   0x27
#define BMA250_SLOPE_THRES_REG                  0x28
#define BMA250_TAP_PARAM_REG                    0x2A
#define BMA250_TAP_THRES_REG                    0x2B
#define BMA250_ORIENT_PARAM_REG                 0x2C
#define BMA250_THETA_BLOCK_REG                  0x2D
#define BMA250_THETA_FLAT_REG                   0x2E
#define BMA250_FLAT_HOLD_TIME_REG               0x2F
#define BMA250_STATUS_LOW_POWER_REG             0x31
#define BMA250_SELF_TEST_REG                    0x32
#define BMA250_EEPROM_CTRL_REG                  0x33
#define BMA250_SERIAL_CTRL_REG                  0x34
#define BMA250_CTRL_UNLOCK_REG                  0x35
#define BMA250_OFFSET_CTRL_REG                  0x36
#define BMA250_OFFSET_PARAMS_REG                0x37
#define BMA250_OFFSET_FILT_X_REG                0x38
#define BMA250_OFFSET_FILT_Y_REG                0x39
#define BMA250_OFFSET_FILT_Z_REG                0x3A
#define BMA250_OFFSET_UNFILT_X_REG              0x3B
#define BMA250_OFFSET_UNFILT_Y_REG              0x3C
#define BMA250_OFFSET_UNFILT_Z_REG              0x3D
#define BMA250_SPARE_0_REG                      0x3E
#define BMA250_SPARE_1_REG                      0x3F


//add 253 define reg begin
#define BMA2x2_X_AXIS_LSB_REG                   0x02
#define BMA2x2_X_AXIS_MSB_REG                   0x03
#define BMA2x2_Y_AXIS_LSB_REG                   0x04
#define BMA2x2_Y_AXIS_MSB_REG                   0x05
#define BMA2x2_Z_AXIS_LSB_REG                   0x06
#define BMA2x2_Z_AXIS_MSB_REG                   0x07
//add 253 define reg end 

#define BMA250_ACC_X_LSB__POS           6
#define BMA250_ACC_X_LSB__LEN           2
#define BMA250_ACC_X_LSB__MSK           0xC0
#define BMA250_ACC_X_LSB__REG           BMA250_X_AXIS_LSB_REG


#define BMA250_ACC_X_MSB__POS           0
#define BMA250_ACC_X_MSB__LEN           8
#define BMA250_ACC_X_MSB__MSK           0xFF
#define BMA250_ACC_X_MSB__REG           BMA250_X_AXIS_MSB_REG

//add 253 x   begin 
#define BMA2x2_ACC_X12_LSB__POS           4
#define BMA2x2_ACC_X12_LSB__LEN           4
#define BMA2x2_ACC_X12_LSB__MSK           0xF0
#define BMA2x2_ACC_X12_LSB__REG           BMA2x2_X_AXIS_LSB_REG

#define BMA2x2_ACC_X_MSB__POS           0
#define BMA2x2_ACC_X_MSB__LEN           8
#define BMA2x2_ACC_X_MSB__MSK           0xFF
#define BMA2x2_ACC_X_MSB__REG           BMA2x2_X_AXIS_MSB_REG
// 253 x end 

#define BMA250_ACC_Y_LSB__POS           6
#define BMA250_ACC_Y_LSB__LEN           2
#define BMA250_ACC_Y_LSB__MSK           0xC0
#define BMA250_ACC_Y_LSB__REG           BMA250_Y_AXIS_LSB_REG

#define BMA250_ACC_Y_MSB__POS           0
#define BMA250_ACC_Y_MSB__LEN           8
#define BMA250_ACC_Y_MSB__MSK           0xFF
#define BMA250_ACC_Y_MSB__REG           BMA250_Y_AXIS_MSB_REG

// add 253 y begin 
#define BMA2x2_ACC_Y12_LSB__POS           4
#define BMA2x2_ACC_Y12_LSB__LEN           4
#define BMA2x2_ACC_Y12_LSB__MSK           0xF0
#define BMA2x2_ACC_Y12_LSB__REG           BMA2x2_Y_AXIS_LSB_REG

#define BMA2x2_ACC_Y_MSB__POS           0
#define BMA2x2_ACC_Y_MSB__LEN           8
#define BMA2x2_ACC_Y_MSB__MSK           0xFF
#define BMA2x2_ACC_Y_MSB__REG           BMA2x2_Y_AXIS_MSB_REG
//add 253 y end 


#define BMA250_ACC_Z_LSB__POS           6
#define BMA250_ACC_Z_LSB__LEN           2
#define BMA250_ACC_Z_LSB__MSK           0xC0
#define BMA250_ACC_Z_LSB__REG           BMA250_Z_AXIS_LSB_REG

#define BMA250_ACC_Z_MSB__POS           0
#define BMA250_ACC_Z_MSB__LEN           8
#define BMA250_ACC_Z_MSB__MSK           0xFF
#define BMA250_ACC_Z_MSB__REG           BMA250_Z_AXIS_MSB_REG

//add 253 z begin 
#define BMA2x2_ACC_Z12_LSB__POS           4
#define BMA2x2_ACC_Z12_LSB__LEN           4
#define BMA2x2_ACC_Z12_LSB__MSK           0xF0
#define BMA2x2_ACC_Z12_LSB__REG           BMA2x2_Z_AXIS_LSB_REG

#define BMA2x2_ACC_Z_MSB__POS           0
#define BMA2x2_ACC_Z_MSB__LEN           8
#define BMA2x2_ACC_Z_MSB__MSK           0xFF
#define BMA2x2_ACC_Z_MSB__REG           BMA2x2_Z_AXIS_MSB_REG
// add 253 z end 
#define BMA250_RANGE_SEL__POS             0
#define BMA250_RANGE_SEL__LEN             4
#define BMA250_RANGE_SEL__MSK             0x0F
#define BMA250_RANGE_SEL__REG             BMA250_RANGE_SEL_REG

#define BMA250_BANDWIDTH__POS             0
#define BMA250_BANDWIDTH__LEN             5
#define BMA250_BANDWIDTH__MSK             0x1F
#define BMA250_BANDWIDTH__REG             BMA250_BW_SEL_REG

#define BMA250_EN_LOW_POWER__POS          6
#define BMA250_EN_LOW_POWER__LEN          1
#define BMA250_EN_LOW_POWER__MSK          0x40
#define BMA250_EN_LOW_POWER__REG          BMA250_MODE_CTRL_REG

#define BMA250_EN_SUSPEND__POS            7
#define BMA250_EN_SUSPEND__LEN            1
#define BMA250_EN_SUSPEND__MSK            0x80
#define BMA250_EN_SUSPEND__REG            BMA250_MODE_CTRL_REG


#define BMA250_UNLOCK_EE_WRITE_SETTING__POS     0
#define BMA250_UNLOCK_EE_WRITE_SETTING__LEN     1
#define BMA250_UNLOCK_EE_WRITE_SETTING__MSK     0x01
#define BMA250_UNLOCK_EE_WRITE_SETTING__REG     BMA250_EEPROM_CTRL_REG

#define BMA250_START_EE_WRITE_SETTING__POS      1
#define BMA250_START_EE_WRITE_SETTING__LEN      1
#define BMA250_START_EE_WRITE_SETTING__MSK      0x02
#define BMA250_START_EE_WRITE_SETTING__REG      BMA250_EEPROM_CTRL_REG

#define BMA250_EE_WRITE_SETTING_S__POS          2
#define BMA250_EE_WRITE_SETTING_S__LEN          1
#define BMA250_EE_WRITE_SETTING_S__MSK          0x04
#define BMA250_EE_WRITE_SETTING_S__REG          BMA250_EEPROM_CTRL_REG

#define BMA250_GET_BITSLICE(regvar, bitname)\
	((regvar & bitname##__MSK) >> bitname##__POS)


#define BMA250_SET_BITSLICE(regvar, bitname, val)\
	((regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK))

#define BMA2x2_GET_BITSLICE(regvar, bitname)\
                        (regvar & bitname##__MSK) >> bitname##__POS
                        
/* range and bandwidth */

#define BMA250_RANGE_2G                 3
#define BMA250_RANGE_4G                 5
#define BMA250_RANGE_8G                 8
#define BMA250_RANGE_16G                12


#define BMA250_BW_7_81HZ        0x08
#define BMA250_BW_15_63HZ       0x09
#define BMA250_BW_31_25HZ       0x0A
#define BMA250_BW_62_50HZ       0x0B
#define BMA250_BW_125HZ         0x0C
#define BMA250_BW_250HZ         0x0D
#define BMA250_BW_500HZ         0x0E
#define BMA250_BW_1000HZ        0x0F

/* mode settings */

#define BMA250_MODE_NORMAL      0
#define BMA250_MODE_LOWPOWER    1
#define BMA250_MODE_SUSPEND     2


#define BMA250_EN_SELF_TEST__POS                0
#define BMA250_EN_SELF_TEST__LEN                2
#define BMA250_EN_SELF_TEST__MSK                0x03
#define BMA250_EN_SELF_TEST__REG                BMA250_SELF_TEST_REG



#define BMA250_NEG_SELF_TEST__POS               2
#define BMA250_NEG_SELF_TEST__LEN               1
#define BMA250_NEG_SELF_TEST__MSK               0x04
#define BMA250_NEG_SELF_TEST__REG               BMA250_SELF_TEST_REG

#define BMA250_EN_FAST_COMP__POS                5
#define BMA250_EN_FAST_COMP__LEN                2
#define BMA250_EN_FAST_COMP__MSK                0x60
#define BMA250_EN_FAST_COMP__REG                BMA250_OFFSET_CTRL_REG

#define BMA250_FAST_COMP_RDY_S__POS             4
#define BMA250_FAST_COMP_RDY_S__LEN             1
#define BMA250_FAST_COMP_RDY_S__MSK             0x10
#define BMA250_FAST_COMP_RDY_S__REG             BMA250_OFFSET_CTRL_REG

#define BMA250_COMP_TARGET_OFFSET_X__POS        1
#define BMA250_COMP_TARGET_OFFSET_X__LEN        2
#define BMA250_COMP_TARGET_OFFSET_X__MSK        0x06
#define BMA250_COMP_TARGET_OFFSET_X__REG        BMA250_OFFSET_PARAMS_REG

#define BMA250_COMP_TARGET_OFFSET_Y__POS        3
#define BMA250_COMP_TARGET_OFFSET_Y__LEN        2
#define BMA250_COMP_TARGET_OFFSET_Y__MSK        0x18
#define BMA250_COMP_TARGET_OFFSET_Y__REG        BMA250_OFFSET_PARAMS_REG

#define BMA250_COMP_TARGET_OFFSET_Z__POS        5
#define BMA250_COMP_TARGET_OFFSET_Z__LEN        2
#define BMA250_COMP_TARGET_OFFSET_Z__MSK        0x60
#define BMA250_COMP_TARGET_OFFSET_Z__REG        BMA250_OFFSET_PARAMS_REG


static unsigned int chiptype = 0 ;

static const u8 bma250_valid_range[] = {
	BMA250_RANGE_2G,
	BMA250_RANGE_4G,
	BMA250_RANGE_8G,
	BMA250_RANGE_16G,
};

static const u8 bma250_valid_bw[] = {
	BMA250_BW_7_81HZ,
	BMA250_BW_15_63HZ,
	BMA250_BW_31_25HZ,
	BMA250_BW_62_50HZ,
	BMA250_BW_125HZ,
	BMA250_BW_250HZ,
	BMA250_BW_500HZ,
	BMA250_BW_1000HZ,
};

struct bma250acc{
	s16	x,
		y,
		z;
} ;

struct bma250_data {
	struct i2c_client *bma250_client;
	atomic_t delay;
	atomic_t enable;
	unsigned char mode;
	struct input_dev *input;
	struct bma250acc value;
	struct mutex enable_mutex;
	struct mutex mode_mutex;
	struct delayed_work work;
	struct work_struct irq_work;

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif

	atomic_t selftest_result;
};
#define BMA2XX_EEPROM_CTRL_REG   0x33
#define BMA2XX_OFFSET_CTRL_REG   0x36
#define BMA2XX_OFFSET_PARAMS_REG 0x37
#define BMA2XX_OFFSET_X			 0x38
#define BMA2XX_OFFSET_Y			 0x39
#define BMA2XX_OFFSET_Z			 0x3A

//#define BACKUP_OFFSET_OUTSIDE
/*
extern int bma2xx_offset_fast_cali(
	struct device *dev
	unsigned char target_x, 
	unsigned char target_y, 
	unsigned char target_z);
*/
#ifdef BACKUP_OFFSET_OUTSIDE
extern int bma2xx_restore_offset(struct device *dev);
#endif 
//#define BACKUP_OFFSET_OUTSIDE
static int bma2xx_fast_compensate(struct device *dev,const unsigned char target[3]);
static int bma2xx_backup_offset(struct device *dev);
static int bma2xx_eeprom_prog(struct device *dev);
static int bma250_set_mode(struct i2c_client *client, unsigned char Mode);
static int bma250_set_range(struct i2c_client *client, unsigned char Range);
static int bma250_set_bandwidth(struct i2c_client *client, unsigned char BW);
static int bma250_smbus_read_byte(struct i2c_client *client,unsigned char reg_addr, unsigned char *data);
static int bma250_smbus_write_byte(struct i2c_client *client,unsigned char reg_addr, unsigned char *data);
/*
  offset fast calibration
  target:
  0 => 0g; 1 => +1g; 2 => -1g; 3 => 0g
*/
int bma2xx_offset_fast_cali(struct device *dev,
                struct device_attribute *attr,
                const char *buf, size_t count)
{
	unsigned char target[3];
	struct i2c_client *client = to_i2c_client(dev);
        struct bma250_data *bma250 = i2c_get_clientdata(client);
/*
	if ((target_x > 3) || (target_y > 3) || (target_z > 3))
	{
		return -1;
	}
*/
	/* trigger offset calibration secondly */
	target[0] = 0;
	target[1] = 0;
	target[2] = 2;
	if (bma2xx_fast_compensate(dev,target) != 0)
	{
		return -2;
	}

#ifdef BACKUP_OFFSET_OUTSIDE
	/* save offset to NVRAM for backup */
	if (bma2xx_backup_offset(dev) != 0)
	{
		return -3;
	}
#else

	/* save calibration result to EEPROM finally */
	if (bma2xx_eeprom_prog(dev) != 0)
	{
		return -3;
	}
#endif
	bma250_set_bandwidth(bma250->bma250_client, BMA250_BW_31_25HZ);
	return count;
}

/*
  offset fast compensation
  target:
  0 => 0g; 1 => +1g; 2 => -1g; 3 => 0g
*/
static int bma2xx_fast_compensate(struct device *dev,
	const unsigned char target[3])
{
	static const int CALI_TIMEOUT = 100;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);	
	int res = 0, timeout = 0;
	unsigned char databuf;
  
	/*** set normal mode, make sure that lowpower_en bit is '0' ***/
	res = bma250_set_mode(bma250->bma250_client, BMA250_MODE_NORMAL);
	if (res != 0)
	{
		return -1;
	}
	/*** set +/-2g range ***/
	res = bma250_set_range(bma250->bma250_client, BMA250_RANGE_2G);
	if (res != 0)
	{
		return -2;
	}
  
	/*** set 1000 Hz bandwidth ***/
	res = bma250_set_bandwidth(bma250->bma250_client, BMA250_BW_1000HZ);
	if (res != 0 ) 
	{
		return -3;
	}
	
	/*** set offset target (x/y/z) ***/
	res = bma250_smbus_read_byte(bma250->bma250_client, BMA250_OFFSET_PARAMS_REG, &databuf);
	if (res != 0)
	{
		return -4;
	}
	/* combine three target value */
	databuf &= 0x81; //clean old value
	databuf |= (((target[0] & 0x03) << 1) | ((target[1] & 0x03) << 3) | ((target[2] & 0x03) << 5));
	res = bma250_smbus_write_byte(bma250->bma250_client, BMA250_OFFSET_PARAMS_REG, &databuf);
	if (res != 0)
	{
		return -5;
	}
	/*** trigger x-axis offset compensation ***/
	res = bma250_smbus_read_byte(bma250->bma250_client, BMA250_OFFSET_CTRL_REG, &databuf);
	if (res != 0)
	{
		return -6;
	}
	databuf &= 0x9F; //clean old value
	databuf |= 0x01 << 5;
	res = bma250_smbus_write_byte(bma250->bma250_client, BMA250_OFFSET_CTRL_REG, &databuf);
	if (res != 0)
	{
		return -7;
	}

	/*** checking status and waiting x-axis offset compensation done ***/
	timeout = 0;
	do 
	{
		mdelay(2);
		bma250_smbus_read_byte(bma250->bma250_client, BMA250_OFFSET_CTRL_REG, &databuf);
		databuf = (databuf >> 4) & 0x01;  //get cal_rdy bit
		//printk(KERN_INFO "wait 2ms and get cal_rdy = %d\n", databuf);
		if (++timeout == CALI_TIMEOUT)
		{
			//printk(KERN_ERR "check cal_rdy time out\n");
			return -8;
		}
	} while (databuf == 0);
	/*** trigger y-axis offset compensation ***/
	res = bma250_smbus_read_byte(bma250->bma250_client, BMA250_OFFSET_CTRL_REG, &databuf);
	if (res != 0)
	{
		return -9;
	}
	databuf &= 0x9F; //clean old value
	databuf |= 0x02 << 5;
	res = bma250_smbus_write_byte(bma250->bma250_client, BMA250_OFFSET_CTRL_REG, &databuf);
	if (res != 0)
	{
		return -10;
	}
	/*** checking status and waiting y-axis offset compensation done ***/
	timeout = 0;
	do 
	{
		mdelay(2);
		bma250_smbus_read_byte(bma250->bma250_client, BMA250_OFFSET_CTRL_REG, &databuf);
		databuf = (databuf >> 4) & 0x01;  //get cal_rdy bit
		//printk(KERN_INFO "wait 2ms and get cal_rdy = %d\n", databuf);
		if (++timeout == CALI_TIMEOUT)
		{
			//printk(KERN_ERR "check cal_rdy time out\n");
			return -11;
		}
	} while (databuf == 0);
	/*** trigger z-axis offset compensation ***/
	//printk(KERN_INFO "trigger z-axis offset compensation\n");
	bma250_smbus_read_byte(bma250->bma250_client, BMA250_OFFSET_CTRL_REG, &databuf);
	if (res != 0)
	{
		return -12;
	}
	databuf &= 0x9F; //clean old value
	databuf |= 0x03 << 5;
	res = bma250_smbus_write_byte(bma250->bma250_client, BMA250_OFFSET_CTRL_REG, &databuf);
	if (res != 0)
	{
	  return -13;
	}

	/*** checking status and waiting z-axis offset compensation done ***/
	timeout = 0;
	do 
	{
		mdelay(2);
		bma250_smbus_read_byte(bma250->bma250_client, BMA250_OFFSET_CTRL_REG, &databuf);
		databuf = (databuf >> 4) & 0x01;  //get cal_rdy bit
		//printk(KERN_INFO "wait 2ms and get cal_rdy = %d\n", databuf);
		if (++timeout == CALI_TIMEOUT)
		{
			//printk(KERN_ERR "check cal_rdy time out\n");
			return -14;
		}
	} while (databuf == 0);

	return 0;
}

#ifndef BACKUP_OFFSET_OUTSIDE
static int bma2xx_eeprom_prog(struct device *dev)
{
	static const int PROG_TIMEOUT = 50;
	int res = 0, timeout = 0;
	unsigned char databuf;    
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);
	/*** unlock EEPROM: write '1' to bit (0x33) nvm_prog_mode ***/
	//printk(KERN_INFO "unlock EEPROM\n");
	res = bma250_smbus_read_byte(bma250->bma250_client, BMA250_EEPROM_CTRL_REG, &databuf);
	if (res != 0)
	{
		return -1;
	}
	databuf |= 0x01;
	res = bma250_smbus_write_byte(bma250->bma250_client, BMA250_EEPROM_CTRL_REG, &databuf);
	if (res != 0)
	{
		return -2;
	}

	/*** need to delay ??? ***/

	/*** trigger the write process: write '1' to bit (0x33) nvm_prog_trig and keep '1' in bit (0x33) nvm_prog_mode ***/
	//printk(KERN_INFO "trigger EEPROM write process\n");
	res = bma250_smbus_read_byte(bma250->bma250_client, BMA250_EEPROM_CTRL_REG, &databuf);
	if (res != 0)
	{
		res = -3;
		goto __lock_eeprom__;
	}
	databuf |= 0x02;
	res = bma250_smbus_write_byte(bma250->bma250_client, BMA250_EEPROM_CTRL_REG, &databuf);
	if (res != 0)
	{
		res = -4;
		goto __lock_eeprom__;
	}

	/*** check the write status by reading bit (0x33) nvm_rdy
	while nvm_rdy = '0', the write process is still enduring; if nvm_rdy = '1', then writing is completed ***/
	do 
	{
		mdelay(2);
		bma250_smbus_read_byte(bma250->bma250_client, BMA250_EEPROM_CTRL_REG, &databuf);
		databuf = (databuf >> 2) & 0x01;  //get nvm_rdy bit
		//printk(KERN_INFO "wait 2ms and get nvm_rdy = %d\n", databuf);
		if (++timeout == PROG_TIMEOUT)
		{
			//printk(KERN_ERR "check nvm_rdy time out\n");
			res = -5;
			goto __lock_eeprom__;
		}
	} while (databuf == 0);

	/*** lock EEPROM: write '0' to nvm_prog_mode ***/
__lock_eeprom__:
	//printk(KERN_INFO "lock EEPROM\n");
	bma250_smbus_read_byte(bma250->bma250_client, BMA250_EEPROM_CTRL_REG, &databuf);
	if(res != 0)
	{
		return -6;
	}
	databuf &= 0xFE;
	res = bma250_smbus_write_byte(bma250->bma250_client, BMA250_EEPROM_CTRL_REG, &databuf);
	if(res != 0)
	{
		return -7;
	}

	return res;
}
#endif 

#ifdef BACKUP_OFFSET_OUTSIDE
static int bma2xx_backup_offset(struct device *dev)
{
	unsigned char offset[3];
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);
	int res = 0;
	
	/* read offset out for backup */
	res = bma250_smbus_read_byte(bma250->bma250_client, BMA250_OFFSET_FILT_X_REG, &(offset[0]));
	if (res != 0)
	{
		return -1;
	}
	res = bma250_smbus_read_byte(bma250->bma250_client, BMA250_OFFSET_FILT_Y_REG, &(offset[1]));
	if (res != 0)
	{
		return -2;
	}
	res = bma250_smbus_read_byte(bma250->bma250_client, BMA250_OFFSET_FILT_Z_REG, &(offset[2]));
	if (res != 0)
	{
		return -3;
	}
	/* save offset into NVRAM */
	
	//TO BE IMPLEMENTED BY CUSTOMER


	return 0;
}
int bma2xx_restore_offset(struct device *dev)
{
	unsigned char offset[3];
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);
	int res = 0;

	/* load offset from NVRAM */
	
	//TO BE IMPLEMENTED BY CUSTOMER
	

	/* write offset back to sensor */
	res = bma250_smbus_read_byte(bma250->bma250_client, BMA250_OFFSET_FILT_X_REG, &offset[0]);
	if (res != 0)
	{
		return -10;
	}
	res = bma250_smbus_read_byte(bma250->bma250_client, BMA250_OFFSET_FILT_Y_REG, &offset[1]);
	if (res != 0)
	{
		return -11;
	}
	res = bma250_smbus_read_byte(bma250->bma250_client, BMA250_OFFSET_FILT_Z_REG, &offset[2]);
	if (res != 0)
	{
		return -12;
	}

	return 0;
}
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
static void bma250_early_suspend(struct early_suspend *h);
static void bma250_late_resume(struct early_suspend *h);
#endif

static int bma250_smbus_read_byte(struct i2c_client *client,
		unsigned char reg_addr, unsigned char *data)
{
	s32 dummy;
	dummy = i2c_smbus_read_byte_data(client, reg_addr);
	if (dummy < 0)
		return -1;
	*data = dummy & 0x000000ff;

	return 0;
}

static int bma250_smbus_write_byte(struct i2c_client *client,
		unsigned char reg_addr, unsigned char *data)
{
	s32 dummy;
	dummy = i2c_smbus_write_byte_data(client, reg_addr, *data);
	if (dummy < 0)
		return -1;
	return 0;
}

static int bma250_smbus_read_byte_block(struct i2c_client *client,
		unsigned char reg_addr, unsigned char *data, unsigned char len)
{
	s32 dummy;
	dummy = i2c_smbus_read_i2c_block_data(client, reg_addr, len, data);
	if (dummy < 0)
		return -1;
	return 0;
}

static int bma250_set_mode(struct i2c_client *client, unsigned char Mode)
{
	int comres = 0;
	unsigned char data1;

	if (client == NULL) {
		comres = -1;
	} else{
		if (Mode < 3) {
			comres = bma250_smbus_read_byte(client,
					BMA250_EN_LOW_POWER__REG, &data1);
			switch (Mode) {
			case BMA250_MODE_NORMAL:
				data1  = BMA250_SET_BITSLICE(data1,
						BMA250_EN_LOW_POWER, 0);
				data1  = BMA250_SET_BITSLICE(data1,
						BMA250_EN_SUSPEND, 0);
				break;
			case BMA250_MODE_LOWPOWER:
				data1  = BMA250_SET_BITSLICE(data1,
						BMA250_EN_LOW_POWER, 1);
				data1  = BMA250_SET_BITSLICE(data1,
						BMA250_EN_SUSPEND, 0);
				break;
			case BMA250_MODE_SUSPEND:
				data1  = BMA250_SET_BITSLICE(data1,
						BMA250_EN_LOW_POWER, 0);
				data1  = BMA250_SET_BITSLICE(data1,
						BMA250_EN_SUSPEND, 1);
				break;
			default:
				break;
			}

			comres += bma250_smbus_write_byte(client,
					BMA250_EN_LOW_POWER__REG, &data1);
		} else{
			comres = -1;
		}
	}

	return comres;
}

static int bma250_get_mode(struct i2c_client *client, unsigned char *Mode)
{
	int comres = 0;

	if (client == NULL) {
		comres = -1;
	} else{
		comres = bma250_smbus_read_byte(client,
				BMA250_EN_LOW_POWER__REG, Mode);
		*Mode  = (*Mode) >> 6;
	}

	return comres;
}

static int bma250_set_range(struct i2c_client *client, unsigned char Range)
{
	int comres = 0;
	unsigned char data1;
	int i;

	if (client == NULL) {
		comres = -1;
	} else{
		for (i = 0; i < ARRAY_SIZE(bma250_valid_range); i++) {
			if (bma250_valid_range[i] == Range)
				break;
		}

		if (ARRAY_SIZE(bma250_valid_range) > i) {
			comres = bma250_smbus_read_byte(client,
					BMA250_RANGE_SEL_REG, &data1);

			data1  = BMA250_SET_BITSLICE(data1,
					BMA250_RANGE_SEL, Range);

			comres += bma250_smbus_write_byte(client,
					BMA250_RANGE_SEL_REG, &data1);
		} else {
			comres = -EINVAL;
		}
	}

	return comres;
}

static int bma250_get_range(struct i2c_client *client, unsigned char *Range)
{
	int comres = 0;
	unsigned char data;

	if (client == NULL) {
		comres = -1;
	} else{
		comres = bma250_smbus_read_byte(client, BMA250_RANGE_SEL__REG,
				&data);
		data = BMA250_GET_BITSLICE(data, BMA250_RANGE_SEL);
		*Range = data;
	}

	return comres;
}


static int bma250_set_bandwidth(struct i2c_client *client, unsigned char BW)
{
	int comres = 0;
	unsigned char data;
	int i = 0;

	if (client == NULL) {
		comres = -1;
	} else {

		for (i = 0; i < ARRAY_SIZE(bma250_valid_bw); i++) {
			if (bma250_valid_bw[i] == BW)
				break;
		}

		if (ARRAY_SIZE(bma250_valid_bw) > i) {
			comres = bma250_smbus_read_byte(client,
					BMA250_BANDWIDTH__REG, &data);
			data = BMA250_SET_BITSLICE(data, BMA250_BANDWIDTH, BW);
			comres += bma250_smbus_write_byte(client,
					BMA250_BANDWIDTH__REG, &data);
		} else {
			comres = -EINVAL;
		}
	}

	return comres;
}

static int bma250_get_bandwidth(struct i2c_client *client, unsigned char *BW)
{
	int comres = 0;
	unsigned char data;

	if (client == NULL) {
		comres = -1;
	} else{
		comres = bma250_smbus_read_byte(client, BMA250_BANDWIDTH__REG,
				&data);
		data = BMA250_GET_BITSLICE(data, BMA250_BANDWIDTH);
		if (data < BMA250_BW_7_81HZ)
			*BW = BMA250_BW_7_81HZ;
		else if (data > BMA250_BW_1000HZ)
			*BW = BMA250_BW_1000HZ;
		else
			*BW = data;
	}

	return comres;
}

static int bma250_read_accel_xyz(struct i2c_client *client,
		struct bma250acc *acc)
{
	int comres;
	unsigned char data[6]= {0};
	if (client == NULL) {
		comres = -1;
	} else{
		comres = bma250_smbus_read_byte_block(client,
				BMA250_ACC_X_LSB__REG, data, 6);
//add 253 start
   if(chiptype == BMA253_CHIP_ID)
   	{
	    acc->x = BMA2x2_GET_BITSLICE(data[0],BMA2x2_ACC_X12_LSB)| (BMA2x2_GET_BITSLICE(data[1],BMA2x2_ACC_X_MSB)<<(BMA2x2_ACC_X12_LSB__LEN));
	    acc->x = acc->x << (sizeof(short)*8-(BMA2x2_ACC_X12_LSB__LEN + BMA2x2_ACC_X_MSB__LEN));
	    acc->x = acc->x >> (sizeof(short)*8-(BMA2x2_ACC_X12_LSB__LEN + BMA2x2_ACC_X_MSB__LEN));
	
	    acc->y = BMA2x2_GET_BITSLICE(data[2],BMA2x2_ACC_Y12_LSB)| (BMA2x2_GET_BITSLICE(data[3],BMA2x2_ACC_Y_MSB)<<(BMA2x2_ACC_Y12_LSB__LEN ));
	    acc->y = acc->y << (sizeof(short)*8-(BMA2x2_ACC_Y12_LSB__LEN + BMA2x2_ACC_Y_MSB__LEN));
	    acc->y = acc->y >> (sizeof(short)*8-(BMA2x2_ACC_Y12_LSB__LEN + BMA2x2_ACC_Y_MSB__LEN));
	
	    acc->z = BMA2x2_GET_BITSLICE(data[4],BMA2x2_ACC_Z12_LSB)| (BMA2x2_GET_BITSLICE(data[5],BMA2x2_ACC_Z_MSB)<<(BMA2x2_ACC_Z12_LSB__LEN));
	    acc->z = acc->z << (sizeof(short)*8-(BMA2x2_ACC_Z12_LSB__LEN + BMA2x2_ACC_Z_MSB__LEN));
	    acc->z = acc->z >> (sizeof(short)*8-(BMA2x2_ACC_Z12_LSB__LEN + BMA2x2_ACC_Z_MSB__LEN));
	}
   else
    {
		acc->x = BMA250_GET_BITSLICE(data[0], BMA250_ACC_X_LSB)
			|(BMA250_GET_BITSLICE(data[1], BMA250_ACC_X_MSB)
					<<BMA250_ACC_X_LSB__LEN);
		acc->x = acc->x << (sizeof(short)*8-(BMA250_ACC_X_LSB__LEN
					+ BMA250_ACC_X_MSB__LEN));
		acc->x = acc->x >> (sizeof(short)*8-(BMA250_ACC_X_LSB__LEN
					+ BMA250_ACC_X_MSB__LEN));
		acc->y = BMA250_GET_BITSLICE(data[2], BMA250_ACC_Y_LSB)
			| (BMA250_GET_BITSLICE(data[3], BMA250_ACC_Y_MSB)
					<<BMA250_ACC_Y_LSB__LEN);
		acc->y = acc->y << (sizeof(short)*8-(BMA250_ACC_Y_LSB__LEN
					+ BMA250_ACC_Y_MSB__LEN));
		acc->y = acc->y >> (sizeof(short)*8-(BMA250_ACC_Y_LSB__LEN
					+ BMA250_ACC_Y_MSB__LEN));

		acc->z = BMA250_GET_BITSLICE(data[4], BMA250_ACC_Z_LSB)
			| (BMA250_GET_BITSLICE(data[5], BMA250_ACC_Z_MSB)
					<<BMA250_ACC_Z_LSB__LEN);
		acc->z = acc->z << (sizeof(short)*8-(BMA250_ACC_Z_LSB__LEN
					+ BMA250_ACC_Z_MSB__LEN));
		acc->z = acc->z >> (sizeof(short)*8-(BMA250_ACC_Z_LSB__LEN
					+ BMA250_ACC_Z_MSB__LEN));
	  }			
	}

	return comres;
}

static int bma250_set_ee_w(struct i2c_client *client, unsigned char eew)
{
	int comres = 0;
	unsigned char data;

	comres = bma250_smbus_read_byte(client,
			BMA250_UNLOCK_EE_WRITE_SETTING__REG, &data);
	data = BMA250_SET_BITSLICE(data, BMA250_UNLOCK_EE_WRITE_SETTING, eew);
	comres = bma250_smbus_write_byte(client,
			BMA250_UNLOCK_EE_WRITE_SETTING__REG, &data);
	return comres;
}


static int bma250_set_ee_prog_trig(struct i2c_client *client)
{
	int comres = 0;
	unsigned char data;
	unsigned char eeprog;
	eeprog = 0x01;

	comres = bma250_smbus_read_byte(client,
			BMA250_START_EE_WRITE_SETTING__REG, &data);
	data = BMA250_SET_BITSLICE(data,
				BMA250_START_EE_WRITE_SETTING, eeprog);
	comres = bma250_smbus_write_byte(client,
			BMA250_START_EE_WRITE_SETTING__REG, &data);
	return comres;
}

static int bma250_get_eeprom_writing_status(struct i2c_client *client,
							unsigned char *eewrite)
{
	int comres = 0;
	unsigned char data;

	comres = bma250_smbus_read_byte(client,
				BMA250_EEPROM_CTRL_REG, &data);
	data = BMA250_GET_BITSLICE(data, BMA250_EE_WRITE_SETTING_S);
	*eewrite = data;

	return comres;
}


static void bma250_work_func(struct work_struct *work)
{
	struct bma250_data *bma250 = container_of((struct delayed_work *)work,
			struct bma250_data, work);
	static struct bma250acc acc;
	unsigned long delay = msecs_to_jiffies(atomic_read(&bma250->delay));

	bma250_read_accel_xyz(bma250->bma250_client, &acc);
	if(chiptype == BMA253_CHIP_ID)
	{
		input_report_abs(bma250->input, ABS_X, -(acc.x/4));
		input_report_abs(bma250->input, ABS_Y, -(acc.y/4));
		input_report_abs(bma250->input, ABS_Z, (acc.z/4));
	}
	else
	{
		input_report_abs(bma250->input, ABS_X, -acc.x);
		input_report_abs(bma250->input, ABS_Y, -acc.y);
		input_report_abs(bma250->input, ABS_Z, acc.z);
	}
	//printk(KERN_ERR "MIN****X=%d***Y=%d**Z=%d\n",acc.x,acc.y,acc.z);
	input_sync(bma250->input);
	bma250->value = acc;
	schedule_delayed_work(&bma250->work, delay);
}

static ssize_t bma250_range_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned char data;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);

	if (bma250_get_range(bma250->bma250_client, &data) < 0)
		return sprintf(buf, "Read error\n");

	return sprintf(buf, "%d\n", data);
}

static ssize_t bma250_range_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;

	if (bma250_set_range(bma250->bma250_client, (unsigned char) data) < 0)
		return -EINVAL;

	return count;
}

static ssize_t bma250_bandwidth_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned char data;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);

	if (bma250_get_bandwidth(bma250->bma250_client, &data) < 0)
		return sprintf(buf, "Read error\n");

	return sprintf(buf, "%d\n", data);

}

static ssize_t bma250_bandwidth_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if (bma250_set_bandwidth(bma250->bma250_client,
				(unsigned char) data) < 0)
		return -EINVAL;

	return count;
}

static ssize_t bma250_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned char data;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);

	if (bma250_get_mode(bma250->bma250_client, &data) < 0)
		return sprintf(buf, "Read error\n");

	return sprintf(buf, "%d\n", data);
}

static ssize_t bma250_mode_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if (bma250_set_mode(bma250->bma250_client, (unsigned char) data) < 0)
		return -EINVAL;

	return count;
}


static ssize_t bma250_value_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct bma250_data *bma250 = input_get_drvdata(input);
	int err;

	err = sprintf(buf, "%d %d %d\n", bma250->value.x, bma250->value.y,
			bma250->value.z);

	return err;
}

static ssize_t bma250_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);

	return sprintf(buf, "%d\n", atomic_read(&bma250->delay));

}

static ssize_t bma250_delay_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;

	if (data <= 0)
		return -EINVAL;

	if (data < BMA250_MIN_DELAY)
		data = BMA250_MIN_DELAY;

	atomic_set(&bma250->delay, (unsigned int) data);

	return count;
}


static ssize_t bma250_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);

	return sprintf(buf, "%d\n", atomic_read(&bma250->enable));

}

static void bma250_set_enable(struct device *dev, int enable)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);
	int pre_enable = atomic_read(&bma250->enable);

	mutex_lock(&bma250->enable_mutex);
	if (enable) {
		if (pre_enable == 0) {
			bma250_set_mode(bma250->bma250_client,
						BMA250_MODE_NORMAL);
			schedule_delayed_work(&bma250->work,
					msecs_to_jiffies(
						atomic_read(&bma250->delay)));
			atomic_set(&bma250->enable, 1);
		}

	} else {
		if (pre_enable == 1) {
			bma250_set_mode(bma250->bma250_client,
						BMA250_MODE_SUSPEND);
			cancel_delayed_work_sync(&bma250->work);
			atomic_set(&bma250->enable, 0);
		}
	}
	mutex_unlock(&bma250->enable_mutex);

}

static ssize_t bma250_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if ((data == 0) || (data == 1))
		bma250_set_enable(dev, data);

	return count;
}

static ssize_t bma250_update_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);

	bma250_read_accel_xyz(bma250->bma250_client, &bma250->value);
	return count;
}

static int bma250_set_selftest_st(struct i2c_client *client,
		unsigned char selftest)
{
	int comres = 0;
	unsigned char data;

	comres = bma250_smbus_read_byte(client,
			BMA250_EN_SELF_TEST__REG, &data);
	data = BMA250_SET_BITSLICE(data,
			BMA250_EN_SELF_TEST, selftest);
	comres = bma250_smbus_write_byte(client,
			BMA250_EN_SELF_TEST__REG, &data);

	return comres;
}

static int bma250_set_selftest_stn(struct i2c_client *client, unsigned char stn)
{
	int comres = 0;
	unsigned char data;

	comres = bma250_smbus_read_byte(client,
			BMA250_NEG_SELF_TEST__REG, &data);
	data = BMA250_SET_BITSLICE(data, BMA250_NEG_SELF_TEST, stn);
	comres = bma250_smbus_write_byte(client,
			BMA250_NEG_SELF_TEST__REG, &data);

	return comres;
}

static int bma250_read_accel_x(struct i2c_client *client, short *a_x)
{
	int comres;
	unsigned char data[2] = {0};

	comres = bma250_smbus_read_byte_block(client,
			BMA250_ACC_X_LSB__REG, data, 2);
	*a_x = BMA250_GET_BITSLICE(data[0], BMA250_ACC_X_LSB)
		| (BMA250_GET_BITSLICE(data[1], BMA250_ACC_X_MSB)
				<<BMA250_ACC_X_LSB__LEN);
	*a_x = *a_x << (sizeof(short)*8
			-(BMA250_ACC_X_LSB__LEN+BMA250_ACC_X_MSB__LEN));
	*a_x = *a_x >> (sizeof(short)*8
			-(BMA250_ACC_X_LSB__LEN+BMA250_ACC_X_MSB__LEN));

	return comres;
}
static int bma250_read_accel_y(struct i2c_client *client, short *a_y)
{
	int comres;
	unsigned char data[2] = {0};

	comres = bma250_smbus_read_byte_block(client,
			BMA250_ACC_Y_LSB__REG, data, 2);
	*a_y = BMA250_GET_BITSLICE(data[0], BMA250_ACC_Y_LSB)
		| (BMA250_GET_BITSLICE(data[1], BMA250_ACC_Y_MSB)
				<<BMA250_ACC_Y_LSB__LEN);
	*a_y = *a_y << (sizeof(short)*8
			-(BMA250_ACC_Y_LSB__LEN+BMA250_ACC_Y_MSB__LEN));
	*a_y = *a_y >> (sizeof(short)*8
			-(BMA250_ACC_Y_LSB__LEN+BMA250_ACC_Y_MSB__LEN));

	return comres;
}

static int bma250_read_accel_z(struct i2c_client *client, short *a_z)
{
	int comres;
	unsigned char data[2] = {0};

	comres = bma250_smbus_read_byte_block(client,
			BMA250_ACC_Z_LSB__REG, data, 2);
	*a_z = BMA250_GET_BITSLICE(data[0], BMA250_ACC_Z_LSB)
		| BMA250_GET_BITSLICE(data[1], BMA250_ACC_Z_MSB)
		<<BMA250_ACC_Z_LSB__LEN;
	*a_z = *a_z << (sizeof(short)*8
			-(BMA250_ACC_Z_LSB__LEN+BMA250_ACC_Z_MSB__LEN));
	*a_z = *a_z >> (sizeof(short)*8
			-(BMA250_ACC_Z_LSB__LEN+BMA250_ACC_Z_MSB__LEN));

	return comres;
}

static ssize_t bma250_selftest_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);

	return sprintf(buf, "%d\n", atomic_read(&bma250->selftest_result));
}

static ssize_t bma250_selftest_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	unsigned char clear_value = 0;
	int error;
	short value1 = 0;
	short value2 = 0;
	short diff = 0;
	unsigned long result = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;

	if (data != 1)
		return -EINVAL;
	/* set to 2 G range */
	if (bma250_set_range(bma250->bma250_client, 0) < 0)
		return -EINVAL;

	bma250_smbus_write_byte(bma250->bma250_client, 0x32, &clear_value);
	/* 1 for x-axis*/
	bma250_set_selftest_st(bma250->bma250_client, 1);
	/* positive direction*/
	bma250_set_selftest_stn(bma250->bma250_client, 0);
	mdelay(10);
	bma250_read_accel_x(bma250->bma250_client, &value1);
	/* negative direction*/
	bma250_set_selftest_stn(bma250->bma250_client, 1);
	mdelay(10);
	bma250_read_accel_x(bma250->bma250_client, &value2);
	diff = value1-value2;

	printk(KERN_ERR "diff x is %d, value1 is %d, value2 is %d\n",
			diff, value1, value2);

	if (abs(diff) < 204)
		result |= 1;

	/* 2 for y-axis*/
	bma250_set_selftest_st(bma250->bma250_client, 2);
	/* positive direction*/
	bma250_set_selftest_stn(bma250->bma250_client, 0);
	mdelay(10);
	bma250_read_accel_y(bma250->bma250_client, &value1);
	/* negative direction*/
	bma250_set_selftest_stn(bma250->bma250_client, 1);
	mdelay(10);
	bma250_read_accel_y(bma250->bma250_client, &value2);
	diff = value1-value2;
	printk(KERN_ERR "diff y is %d, value1 is %d, value2 is %d\n",
			diff, value1, value2);
	if (abs(diff) < 204)
		result |= 2;

	/* 3 for z-axis*/
	bma250_set_selftest_st(bma250->bma250_client, 3);
	/* positive direction*/
	bma250_set_selftest_stn(bma250->bma250_client, 0);
	mdelay(10);
	bma250_read_accel_z(bma250->bma250_client, &value1);
	/* negative direction*/
	bma250_set_selftest_stn(bma250->bma250_client, 1);
	mdelay(10);
	bma250_read_accel_z(bma250->bma250_client, &value2);
	diff = value1 - value2;

	printk(KERN_ERR "diff z is %d, value1 is %d, value2 is %d\n",
			diff, value1, value2);
	if (abs(diff) < 102)
		result |= 4;

	atomic_set(&bma250->selftest_result, (unsigned int) result);

	printk(KERN_ERR "self test finished\n");


	return count;
}

static int bma250_set_offset_target_x(struct i2c_client *client,
		unsigned char offsettarget)
{
	int comres = 0;
	unsigned char data;

	comres = bma250_smbus_read_byte(client,
			BMA250_COMP_TARGET_OFFSET_X__REG, &data);
	data = BMA250_SET_BITSLICE(data,
			BMA250_COMP_TARGET_OFFSET_X, offsettarget);
	comres = bma250_smbus_write_byte(client,
			BMA250_COMP_TARGET_OFFSET_X__REG, &data);

	return comres;
}

static int bma250_get_offset_target_x(struct i2c_client *client,
		unsigned char *offsettarget)
{
	int comres = 0 ;
	unsigned char data;

	comres = bma250_smbus_read_byte(client,
			BMA250_OFFSET_PARAMS_REG, &data);
	data = BMA250_GET_BITSLICE(data, BMA250_COMP_TARGET_OFFSET_X);
	*offsettarget = data;

	return comres;
}

static int bma250_set_offset_target_y(struct i2c_client *client,
		unsigned char offsettarget)
{
	int comres = 0;
	unsigned char data;

	comres = bma250_smbus_read_byte(client,
			BMA250_COMP_TARGET_OFFSET_Y__REG, &data);
	data = BMA250_SET_BITSLICE(data,
			BMA250_COMP_TARGET_OFFSET_Y, offsettarget);
	comres = bma250_smbus_write_byte(client,
			BMA250_COMP_TARGET_OFFSET_Y__REG, &data);

	return comres;
}

static int bma250_get_offset_target_y(struct i2c_client *client,
		unsigned char *offsettarget)
{
	int comres = 0 ;
	unsigned char data;

	comres = bma250_smbus_read_byte(client,
			BMA250_OFFSET_PARAMS_REG, &data);
	data = BMA250_GET_BITSLICE(data, BMA250_COMP_TARGET_OFFSET_Y);
	*offsettarget = data;

	return comres;
}

static int bma250_set_offset_target_z(struct i2c_client *client,
		unsigned char offsettarget)
{
	int comres = 0;
	unsigned char data;

	comres = bma250_smbus_read_byte(client,
			BMA250_COMP_TARGET_OFFSET_Z__REG, &data);
	data = BMA250_SET_BITSLICE(data,
			BMA250_COMP_TARGET_OFFSET_Z, offsettarget);
	comres = bma250_smbus_write_byte(client,
			BMA250_COMP_TARGET_OFFSET_Z__REG, &data);

	return comres;
}

static int bma250_get_offset_target_z(struct i2c_client *client,
		unsigned char *offsettarget)
{
	int comres = 0 ;
	unsigned char data;

	comres = bma250_smbus_read_byte(client,
			BMA250_OFFSET_PARAMS_REG, &data);
	data = BMA250_GET_BITSLICE(data, BMA250_COMP_TARGET_OFFSET_Z);
	*offsettarget = data;

	return comres;
}


static int bma250_set_offset_filt_x(struct i2c_client *client, unsigned char
		offsetfilt)
{
	int comres = 0;
	unsigned char data;

	data =  offsetfilt;
	comres = bma250_smbus_write_byte(client, BMA250_OFFSET_FILT_X_REG,
						&data);

	return comres;
}


static int bma250_get_offset_filt_x(struct i2c_client *client, unsigned char
						*offsetfilt)
{
	int comres = 0;
	unsigned char data;

	comres = bma250_smbus_read_byte(client, BMA250_OFFSET_FILT_X_REG,
						&data);
	*offsetfilt = data;

	return comres;
}

static int bma250_set_offset_filt_y(struct i2c_client *client, unsigned char
						offsetfilt)
{
	int comres = 0;
	unsigned char data;

	data =  offsetfilt;
	comres = bma250_smbus_write_byte(client, BMA250_OFFSET_FILT_Y_REG,
						&data);

	return comres;
}

static int bma250_get_offset_filt_y(struct i2c_client *client, unsigned char
						*offsetfilt)
{
	int comres = 0;
	unsigned char data;

	comres = bma250_smbus_read_byte(client, BMA250_OFFSET_FILT_Y_REG,
						&data);
	*offsetfilt = data;

	return comres;
}

static int bma250_set_offset_filt_z(struct i2c_client *client, unsigned char
						offsetfilt)
{
	int comres = 0;
	unsigned char data;

	data =  offsetfilt;
	comres = bma250_smbus_write_byte(client, BMA250_OFFSET_FILT_Z_REG,
						&data);

	return comres;
}

static int bma250_get_offset_filt_z(struct i2c_client *client, unsigned char
						*offsetfilt)
{
	int comres = 0;
	unsigned char data;

	comres = bma250_smbus_read_byte(client, BMA250_OFFSET_FILT_Z_REG,
						&data);
	*offsetfilt = data;

	return comres;
}


static int bma250_get_cal_ready(struct i2c_client *client,
		unsigned char *calrdy)
{
	int comres = 0 ;
	unsigned char data;

	comres = bma250_smbus_read_byte(client,
			BMA250_OFFSET_CTRL_REG, &data);
	data = BMA250_GET_BITSLICE(data, BMA250_FAST_COMP_RDY_S);
	*calrdy = data;

	return comres;
}

static int bma250_set_cal_trigger(struct i2c_client *client,
		unsigned char caltrigger)
{
	int comres = 0;
	unsigned char data;

	comres = bma250_smbus_read_byte(client,
			BMA250_EN_FAST_COMP__REG, &data);
	data = BMA250_SET_BITSLICE(data,
			BMA250_EN_FAST_COMP, caltrigger);
	comres = bma250_smbus_write_byte(client,
			BMA250_EN_FAST_COMP__REG, &data);

	return comres;
}


static ssize_t bma250_fast_calibration_x_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned char data;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);

	if (bma250_get_offset_target_x(bma250->bma250_client, &data) < 0)
		return sprintf(buf, "Read error\n");

	return sprintf(buf, "%d\n", data);
}

static ssize_t bma250_fast_calibration_x_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	signed char tmp;
	unsigned int timeout = 0;
	int error;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;

	if (bma250_set_offset_target_x(bma250->bma250_client,
				(unsigned char)data) < 0)
		return -EINVAL;

	if (bma250_set_cal_trigger(bma250->bma250_client, 1) < 0)
		return -EINVAL;

	do {
		mdelay(2);
		bma250_get_cal_ready(bma250->bma250_client, &tmp);

		printk(KERN_ERR "wait 2ms and got cal ready flag is %d\n",
				tmp);
		timeout++;
		if (timeout == 1000) {
			printk(KERN_ERR "get fast calibration ready error\n");
			return -EINVAL;
		};

	} while (tmp == 0);

	printk(KERN_ERR "x axis fast calibration finished\n");
	return count;
}

static ssize_t bma250_fast_calibration_y_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{


	unsigned char data;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);

	if (bma250_get_offset_target_y(bma250->bma250_client, &data) < 0)
		return sprintf(buf, "Read error\n");

	return sprintf(buf, "%d\n", data);

}

static ssize_t bma250_fast_calibration_y_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	signed char tmp;
	unsigned int timeout = 0;
	int error;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;

	if (bma250_set_offset_target_y(bma250->bma250_client,
				(unsigned char)data) < 0)
		return -EINVAL;

	if (bma250_set_cal_trigger(bma250->bma250_client, 2) < 0)
		return -EINVAL;

	do {
		mdelay(2);
		bma250_get_cal_ready(bma250->bma250_client, &tmp);

		printk(KERN_ERR "wait 2ms and got cal ready flag is %d\n",
				tmp);
		timeout++;
		if (timeout == 1000) {
			printk(KERN_ERR "get fast calibration ready error\n");
			return -EINVAL;
		};

	} while (tmp == 0);

	printk(KERN_ERR "y axis fast calibration finished\n");
	return count;
}

static ssize_t bma250_fast_calibration_z_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{


	unsigned char data;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);

	if (bma250_get_offset_target_z(bma250->bma250_client, &data) < 0)
		return sprintf(buf, "Read error\n");

	return sprintf(buf, "%d\n", data);

}

static ssize_t bma250_fast_calibration_z_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	signed char tmp;
	unsigned int timeout = 0;
	int error;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;

	if (bma250_set_offset_target_z(bma250->bma250_client,
				(unsigned char)data) < 0)
		return -EINVAL;

	if (bma250_set_cal_trigger(bma250->bma250_client, 3) < 0)
		return -EINVAL;

	do {
		mdelay(2);
		bma250_get_cal_ready(bma250->bma250_client, &tmp);

		printk(KERN_ERR "wait 2ms and got cal ready flag is %d\n",
				tmp);
		timeout++;
		if (timeout == 1000) {
			printk(KERN_ERR "get fast calibration ready error\n");
			return -EINVAL;
		}

	} while (tmp == 0);

	printk(KERN_ERR "z axis fast calibration finished\n");
	return count;
}

static ssize_t bma250_eeprom_writing_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	signed char tmp;
	int timeout = 0;
	int error;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;



	if (data != 1)
		return -EINVAL;

	/* unlock eeprom */
	if (bma250_set_ee_w(bma250->bma250_client, 1) < 0)
		return -EINVAL;

	printk(KERN_ERR "unlock eeprom successful\n");

	if (bma250_set_ee_prog_trig(bma250->bma250_client) < 0)
		return -EINVAL;
	printk(KERN_ERR "start update eeprom\n");

	do {
		mdelay(2);
		bma250_get_eeprom_writing_status(bma250->bma250_client, &tmp);

		printk(KERN_ERR "wait 2ms eeprom write status is %d\n", tmp);
		timeout++;
		if (timeout == 1000) {
			printk(KERN_ERR "get eeprom writing status error\n");
			return -EINVAL;
		};

	} while (tmp == 0);

	printk(KERN_ERR "eeprom writing is finished\n");

	/* unlock eeprom */
	if (bma250_set_ee_w(bma250->bma250_client, 0) < 0)
		return -EINVAL;

	printk(KERN_ERR "lock eeprom successful\n");
	return count;
}


static ssize_t bma250_eeprom_writing_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned char data;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);

	if (bma250_get_eeprom_writing_status(bma250->bma250_client, &data) < 0)
		return sprintf(buf, "Read error\n");

	return sprintf(buf, "%d\n", data);

}


static ssize_t bma250_offset_filt_x_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned char data;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);

	if (bma250_get_offset_filt_x(bma250->bma250_client, &data) < 0)
		return sprintf(buf, "Read error\n");

	return sprintf(buf, "%d\n", data);

}

static ssize_t bma250_offset_filt_x_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;

	if (bma250_set_offset_filt_x(bma250->bma250_client, (unsigned
					char)data) < 0)
		return -EINVAL;

	return count;
}

static ssize_t bma250_offset_filt_y_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned char data;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);

	if (bma250_get_offset_filt_y(bma250->bma250_client, &data) < 0)
		return sprintf(buf, "Read error\n");

	return sprintf(buf, "%d\n", data);

}

static ssize_t bma250_offset_filt_y_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;

	if (bma250_set_offset_filt_y(bma250->bma250_client, (unsigned
					char)data) < 0)
		return -EINVAL;

	return count;
}

static ssize_t bma250_offset_filt_z_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned char data;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);

	if (bma250_get_offset_filt_z(bma250->bma250_client, &data) < 0)
		return sprintf(buf, "Read error\n");

	return sprintf(buf, "%d\n", data);

}

static ssize_t bma250_offset_filt_z_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;

	if (bma250_set_offset_filt_z(bma250->bma250_client, (unsigned
					char)data) < 0)
		return -EINVAL;

	return count;
}



static DEVICE_ATTR(range, S_IRUGO|S_IWUSR|S_IWGRP,
		bma250_range_show, bma250_range_store);
static DEVICE_ATTR(bandwidth, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		bma250_bandwidth_show, bma250_bandwidth_store);
static DEVICE_ATTR(mode, S_IRUGO|S_IWUSR|S_IWGRP,
		bma250_mode_show, bma250_mode_store);
static DEVICE_ATTR(value, S_IRUGO,
		bma250_value_show, NULL);
static DEVICE_ATTR(calibration, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
                NULL, bma2xx_offset_fast_cali);
static DEVICE_ATTR(delay, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		bma250_delay_show, bma250_delay_store);
static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP,
		bma250_enable_show, bma250_enable_store);
static DEVICE_ATTR(update, S_IRUGO|S_IWUSR|S_IWGRP,
		NULL, bma250_update_store);
static DEVICE_ATTR(selftest, S_IRUGO|S_IWUSR|S_IWGRP,
		bma250_selftest_show, bma250_selftest_store);
static DEVICE_ATTR(fast_calibration_x, S_IRUGO|S_IWUSR|S_IWGRP,
		bma250_fast_calibration_x_show,
		bma250_fast_calibration_x_store);
static DEVICE_ATTR(fast_calibration_y, S_IRUGO|S_IWUSR|S_IWGRP,
		bma250_fast_calibration_y_show,
		bma250_fast_calibration_y_store);
static DEVICE_ATTR(fast_calibration_z, S_IRUGO|S_IWUSR|S_IWGRP,
		bma250_fast_calibration_z_show,
		bma250_fast_calibration_z_store);

static DEVICE_ATTR(eeprom_writing, S_IRUGO|S_IWUSR|S_IWGRP,
		bma250_eeprom_writing_show, bma250_eeprom_writing_store);

static DEVICE_ATTR(offset_filt_x, S_IRUGO|S_IWUSR|S_IWGRP,
		bma250_offset_filt_x_show,
		bma250_offset_filt_x_store);

static DEVICE_ATTR(offset_filt_y, S_IRUGO|S_IWUSR|S_IWGRP,
		bma250_offset_filt_y_show,
		bma250_offset_filt_y_store);

static DEVICE_ATTR(offset_filt_z, S_IRUGO|S_IWUSR|S_IWGRP,
		bma250_offset_filt_z_show,
		bma250_offset_filt_z_store);


static struct attribute *bma250_attributes[] = {
	&dev_attr_range.attr,
	&dev_attr_bandwidth.attr,
	&dev_attr_mode.attr,
	&dev_attr_value.attr,
	&dev_attr_calibration.attr,
	&dev_attr_delay.attr,
	&dev_attr_enable.attr,
	&dev_attr_update.attr,
	&dev_attr_selftest.attr,
	&dev_attr_fast_calibration_x.attr,
	&dev_attr_fast_calibration_y.attr,
	&dev_attr_fast_calibration_z.attr,
	&dev_attr_eeprom_writing.attr,
	&dev_attr_offset_filt_x.attr,
	&dev_attr_offset_filt_y.attr,
	&dev_attr_offset_filt_z.attr,
	NULL
};

static struct attribute_group bma250_attribute_group = {
	.attrs = bma250_attributes
};

static int bma250_input_init(struct bma250_data *bma250)
{
	struct input_dev *dev;
	int err;

	dev = input_allocate_device();
	if (!dev)
		return -ENOMEM;
	dev->name = SENSOR_NAME;
	dev->id.bustype = BUS_I2C;

	input_set_capability(dev, EV_ABS, ABS_MISC);
	input_set_abs_params(dev, ABS_X, ABSMIN_2G, ABSMAX_2G, 0, 0);
	input_set_abs_params(dev, ABS_Y, ABSMIN_2G, ABSMAX_2G, 0, 0);
	input_set_abs_params(dev, ABS_Z, ABSMIN_2G, ABSMAX_2G, 0, 0);
	input_set_drvdata(dev, bma250);

	err = input_register_device(dev);
	if (err < 0) {
		input_free_device(dev);
		return err;
	}
	bma250->input = dev;

	return 0;
}

static void bma250_input_delete(struct bma250_data *bma250)
{
	struct input_dev *dev = bma250->input;

	input_unregister_device(dev);
	input_free_device(dev);
}

static int bma250_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int err = 0;
	int tempvalue;
	struct bma250_data *data;
	client->addr = 0x18;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk(KERN_ERR "i2c_check_functionality error\n");
		goto exit;
	}
	data = kzalloc(sizeof(struct bma250_data), GFP_KERNEL);
	if (!data) {
		err = -ENOMEM;
		goto exit;
	}
	/* read chip id */
	chiptype = 0;
	tempvalue = 0;
	tempvalue = i2c_smbus_read_word_data(client, BMA250_CHIP_ID_REG);
	if ((tempvalue&0x00FF) == BMA250_CHIP_ID) {
		chiptype = BMA250_CHIP_ID;
		printk(KERN_ERR "Bosch Sensortec Device detected!\n" \
				"BMA250 registered I2C driver!\n");
	} else if ((tempvalue&0x00FF) == BMA250E_CHIP_ID) {
		chiptype = BMA250E_CHIP_ID;
		printk(KERN_INFO "Bosch SensortecE Device detected!\n" \
				"BMA250E registered I2C driver!\n");
	}else if((tempvalue&0x00FF) == BMA253_CHIP_ID)
	{
		chiptype = BMA253_CHIP_ID;
		printk(KERN_INFO "Bosch SensortecE Device detected!\n" \
				"BMA253 registered I2C driver!\n");		
	}
	 else{
		printk(KERN_ERR "Bosch Sensortec Device not found, \
				i2c error %d \n", tempvalue);
		err = -1;
		goto kfree_exit;
	}
	printk(KERN_ERR "chiptype  ======aaaaaa======->%x!\n",chiptype);	
	i2c_set_clientdata(client, data);
	data->bma250_client = client;
	mutex_init(&data->mode_mutex);
	mutex_init(&data->enable_mutex);
	bma250_set_bandwidth(client, BMA250_BW_SET);
	bma250_set_range(client, BMA250_RANGE_SET);

	INIT_DELAYED_WORK(&data->work, bma250_work_func);
	atomic_set(&data->delay, BMA250_DEFAULT_DELAY);
	atomic_set(&data->enable, 0);
	err = bma250_input_init(data);
	if (err < 0)
		goto kfree_exit;

	err = sysfs_create_group(&data->bma250_client->dev.kobj,
			&bma250_attribute_group);
	if (err < 0)
		goto error_sysfs;

#ifdef CONFIG_HAS_EARLYSUSPEND
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	data->early_suspend.suspend = bma250_early_suspend;
	data->early_suspend.resume = bma250_late_resume;
	register_early_suspend(&data->early_suspend);
#endif
	err = bma250_set_mode(client, BMA250_MODE_SUSPEND);
	if (err)
		goto error_sysfs;

	return 0;

error_sysfs:
	bma250_input_delete(data);

kfree_exit:
	kfree(data);
exit:
	return err;
}


#ifdef CONFIG_HAS_EARLYSUSPEND
static void bma250_early_suspend(struct early_suspend *h)
{
	struct bma250_data *data =
		container_of(h, struct bma250_data, early_suspend);

	mutex_lock(&data->enable_mutex);
	if (atomic_read(&data->enable) == 1) {
		bma250_set_mode(data->bma250_client, BMA250_MODE_SUSPEND);
		cancel_delayed_work_sync(&data->work);
	}
	mutex_unlock(&data->enable_mutex);
}


static void bma250_late_resume(struct early_suspend *h)
{
	struct bma250_data *data =
		container_of(h, struct bma250_data, early_suspend);

	mutex_lock(&data->enable_mutex);
	if (atomic_read(&data->enable) == 1) {
		bma250_set_mode(data->bma250_client, BMA250_MODE_NORMAL);
		schedule_delayed_work(&data->work,
				msecs_to_jiffies(atomic_read(&data->delay)));
	}
	mutex_unlock(&data->enable_mutex);
}
#endif

static int bma250_remove(struct i2c_client *client)
{
	struct bma250_data *data = i2c_get_clientdata(client);

	if (&client->dev)
	bma250_set_enable(&client->dev, 0);
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&data->early_suspend);
#endif
	sysfs_remove_group(&data->input->dev.kobj, &bma250_attribute_group);
	bma250_input_delete(data);
	kfree(data);
	return 0;
}


static const struct i2c_device_id bma250_id[] = {
	{ SENSOR_NAME, 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, bma250_id);

static struct i2c_driver bma250_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= SENSOR_NAME,
	},
	.id_table	= bma250_id,
	.probe		= bma250_probe,
	.remove		= bma250_remove,

};
static int register_bma250_client(void)
{
        
    int ret = 0;
    int i2c_bus_num = 2;
    
    struct i2c_adapter *adapter;
    struct i2c_client *client;

    struct i2c_board_info info = {
        I2C_BOARD_INFO(SENSOR_NAME, 0x18),        
    };

    adapter = i2c_get_adapter(2);
    if(!adapter) {
        printk("Can't get adapter form i2c_bus_num[%d]",i2c_bus_num);
        return -ENODEV;
    }

    client = i2c_new_device(adapter, &info);
    if(!client) {
        printk("Can't register i2c client!");
        kfree(adapter);
        return -ENODEV;
    }

    return ret;
}

static int __init BMA250_init(void)
{

	register_bma250_client();
	return i2c_add_driver(&bma250_driver);
}

static void __exit BMA250_exit(void)
{
	i2c_del_driver(&bma250_driver);
}

MODULE_AUTHOR("Albert Zhang <xu.zhang@bosch-sensortec.com>");
MODULE_DESCRIPTION("BMA250 driver");
MODULE_LICENSE("GPL");

module_init(BMA250_init);
module_exit(BMA250_exit);

