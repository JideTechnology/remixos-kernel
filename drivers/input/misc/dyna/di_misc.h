
#ifndef __DI_MISC_H__
#define __DI_MISC_H__

#define I2C_RETRY_DELAY	(5)
#define I2C_RETRIES	(3)

#define NOT_EXIST_COMMAND	{~0x00, 0, 0}
#define NOT_EXIST_ADDR		0xFF

#define FORM_REG_VALUE(temp, value, reg_cmd) { temp &= reg_cmd.mask; temp |= (value<<reg_cmd.lsb); }

struct command
{
	unsigned mask;	//register mask of the command
	unsigned lsb;	//least significant bit of the command 
	unsigned max;	//max value of the command
};

int i2c_read(struct i2c_client *client, u8 reg, u8* data, int len);
int i2c_single_write(struct i2c_client *client, u8 reg, u8 data);
int i2c_single_write_ignore_ack(struct i2c_client *client, u8 reg, u8 data);

struct class* get_DI_class(void);
s64 get_time_ns(void);

#endif	/* __DI_MISC_H__ */





