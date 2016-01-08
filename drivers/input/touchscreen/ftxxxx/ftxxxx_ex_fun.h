#ifndef __LINUX_FTXXXX_EX_FUN_H__
#define __LINUX_FTXXXX_EX_FUN_H__

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#include <linux/syscalls.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/string.h>
/*
 * #define IC_FT6208	0
 * #define IC_FT5X06	1
 * #define IC_FT5606	2
 * #define IC_FT5316	3
 * #define IC_FT5X36	4
*/
#define IC_FT5X46	5

#define FT_UPGRADE_AA					0xAA
#define FT_UPGRADE_55					0x55
#define FT_APP_INFO_ADDR				0xd7f8
#define FT_UPGRADE_EARSE_DELAY			1500

/*upgrade config of FT5X46*/
/*
#define FT5X46_UPGRADE_AA_DELAY 		2
#define FT5X46_UPGRADE_55_DELAY 		2
#define FT5X46_UPGRADE_ID_1				0x54
#define FT5X46_UPGRADE_ID_2				0x2c
#define FT5X46_UPGRADE_READID_DELAY 	20
*/
#define	DEVICE_IC_TYPE	IC_FT5X46

#define	FTS_PACKET_LENGTH			128
#define	FTS_SETTING_BUF_LEN			128

/***********************0705 mshl*/
#define	BL_VERSION_LZ4			0
#define	BL_VERSION_Z7			1
#define	BL_VERSION_GZF			2

/*#define    AUTO_CLB*/

#define _FTS_DBG
#ifdef _FTS_DBG
#define FTS_DBG(fmt, args...) printk("[FTS]" fmt, ## args)
#else
#define FTS_DBG(fmt, args...) do{}while(0)
#endif

int fts_ctpm_auto_upgrade(struct i2c_client *client);
/*create sysfs for debug*/
int ftxxxx_create_sysfs(struct i2c_client * client);
/*remove sysfs*/
int ftxxxx_remove_sysfs(struct i2c_client * client);

int ftxxxx_create_apk_debug_channel(struct i2c_client *client);
void ftxxxx_release_apk_debug_channel(void);

int fts_ctpm_fw_preupgrade_hwreset(struct i2c_client * client, u8* pbt_buf, u32 dw_lenth);
int fts_ctpm_fw_preupgrade(struct i2c_client * client, u8* pbt_buf, u32 dw_lenth);
int fts_5x46_ctpm_fw_upgrade(struct i2c_client * client, u8* pbt_buf, u32 dw_lenth);

/*
*ftxxxx_write_reg- write register
*@client: handle of i2c
*@regaddr: register address
*@regvalue: register value

*
*/
int ftxxxx_write_reg(struct i2c_client * client,u8 regaddr, u8 regvalue);
int ftxxxx_read_reg(struct i2c_client * client,u8 regaddr, u8 * regvalue);

int FTS_I2c_Read(unsigned char * wBuf, int wLen, unsigned char *rBuf, int rLen);
int FTS_I2c_Write(unsigned char * wBuf, int wLen);

#endif
