/*
*
* Copyright (c) 2012 Zhimin Tian <zmtian@chiponeic.com>
* Copyright (c) 2012 ChipOne Technology (Beijing) Co., Ltd.
* Copyright (C) 2015 ROCKCHIP, Inc.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 2 as published by the Free Software Foundation.
*/

#ifndef __LINUX_ICN85XX_H__
#define __LINUX_ICN85XX_H__

#include <linux/i2c.h>
#include <linux/input.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
    #include <linux/pm.h>
    #include <linux/earlysuspend.h>
#endif
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/async.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/hrtimer.h>
#include <linux/proc_fs.h>
#include <linux/init.h>  
#include <linux/kernel.h>  
#include <linux/fs.h>  
#include <linux/semaphore.h>  
#include <linux/cdev.h>  
#include <linux/device.h>  
#include <linux/ioctl.h>  
#include <linux/slab.h>  
#include <linux/errno.h>  
#include <linux/string.h>  
#include <linux/spinlock_types.h> 
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/consumer.h>
#include <linux/wakelock.h>
//#include "tp_suspend.h"
#include <linux/kthread.h>

//-----------------------------------------------------------------------------
// Pin Declarations
//-----------------------------------------------------------------------------


     // action platform
     #define CTP_POWER_ID           ("sensor28")//("touchPannel_power")
     #define CTP_POWER_MIN_VOL       (3300000)
     #define CTP_POWER_MAX_VOL        (3300000)   
     
     #define CTP_IRQ_PORT             IRQ_SIRQ0
     #define CTP_IRQ_MODE             0
     #define CTP_RST_PORT             ASOC_GPIO_PORTB(3)
     #define CTP_WAKEUP_PORT          0
     #define CTP_REPORT_PROTOCOL      1   //0: A protocol
                                          //1: B protocol
          
     #define ICN83XX_I2C_SCL          400*1000
     //-----------------------------------------------------------------------------
     // Global CONSTANTS
     //-----------------------------------------------------------------------------
     
     
     //tp config
     #define TOUCH_VIRTUAL_KEYS          1
     #define SUPPORT_PROC_FS             0
     #define SUPPORT_SYSFS               0
     #define SUPPORT_FW_UPDATE           1
     #define COMPILE_FW_WITH_DRIVER      1
     #define FORCE_UPDATA_FW             0
     #define SUSPEND_POWER_OFF           0 
     #define SUPPORT_SENSOR_ID           1  
     
     #define ICN85XX_NAME                "chipone-ts"//"chipone-ts"
     
     #define ICN85XX_NORMAL_IIC_ADDR     (0x90>>1)// (0x80>>1)//(0x90>>1)
     #define ICN85XX_PROG_IIC_ADDR       (0x60>>1)
     #define CTP_NAME                    ICN85XX_NAME
     
     #define CTP_RESET_LOW_PERIOD        (5)
     #define CTP_RESET_HIGH_PERIOD       (100)
     #define CTP_WAKEUP_LOW_PERIOD       (20)
     #define CTP_WAKEUP_HIGH_PERIOD      (50)
     #define CTP_POLL_TIMER              (16)    /* ms delay between samples */
     #define CTP_START_TIMER             (100)    /* ms delay between samples */
     
     #define POINT_NUM                   10
     #define POINT_SIZE                  7
     
     #define TS_KEY_HOME                 102
     #define TS_KEY_MENU                 139
     #define TS_KEY_BACK                 158
     #define TS_KEY_SEARCH               217
     
     #define ICN_VIRTUAL_BUTTON_HOME     0x01
     #define ICN_VIRTUAL_BUTTON_MENU     0x02
     #define ICN_VIRTUAL_BUTTON_BACK     0x04
     #define ICN_VIRTUAL_BUTTON_SEARCH   0x08
     
     #define IIC_RETRY_NUM               3
     
     //ICN85XX_REG_PMODE
     #define PMODE_ACTIVE                0x00
     #define PMODE_MONITOR               0x01
     #define PMODE_HIBERNATE             0x02
     
     #define MAX_LENGTH_PER_TRANSFER           128
     #define B_SIZE                            120//96//32 //128//64//32
     //#define ENABLE_BYTE_CHECK
     
     #define COL_NUM                          128//24
     #define ROW_NUM                          128//36

     #define MD25D40_ID1                      0x514013
     #define MD25D40_ID2                      0xC84013
     #define MD25D20_ID1                      0x514012
     #define MD25D20_ID2                      0xC84012
     #define GD25Q10_ID                       0xC84011
     #define MX25L512E_ID                     0xC22010
     #define MD25D05_ID                       0x514010
     #define MD25D10_ID                       0x514011

     #define ICN85XX_WITHOUT_FLASH           0x11
     #define ICN85XX_WITH_FLASH_85              0x22
     #define ICN85XX_WITH_FLASH_86              0x33

     #define FLASH_TOTAL_SIZE                0x00010000     
     #define FLASH_PAGE_SIZE                 0x1000 
     #define FLASH_AHB_BASE_ADDR             0x00100000 
     #define FLASH_PATCH_PARA_BASE_ADDR      (FLASH_TOTAL_SIZE - FLASH_PAGE_SIZE)          //  allocate 1 page for patch para, 0xff00
     #define FLASH_CODE_INFO_BASE_ADDR       (FLASH_PATCH_PARA_BASE_ADDR - FLASH_PAGE_SIZE)        //  0xfe00,allocate 1 page for system para
     #define FLASH_CRC_ADDR                  (FLASH_AHB_BASE_ADDR + FLASH_CODE_INFO_BASE_ADDR + 0x00) //  0xfe00
     #define FLASH_CODE_LENGTH_ADDR          (FLASH_AHB_BASE_ADDR + FLASH_CODE_INFO_BASE_ADDR + 0x04) //  0xfe04
     
     //-----------------------------------------------------------------------------
     // Macro DEFINITIONS
     //-----------------------------------------------------------------------------
     //#define DBG_ICN85XX_TRACE
     //#define DBG_ICN85XX_POINT
     #define DBG_ICN85XX_INFO
     #define DBG_ICN85XX_ERROR
     #define DBG_FLASH_INFO
     #define DBG_FLASH_ERROR
     #define DBG_OP_INFO
     #define DBG_OP_ERROR
     #define DBG_CALIB_INFO
     #define DBG_CALIB_ERROR
     //#define DBG_PROC_INFO
     #define DBG_PROC_ERROR
     
     
     #ifdef DBG_ICN85XX_TRACE
     #define icn85xx_trace(fmt, args...)   \
             do{                              \
                     printk(fmt, ##args);     \
             }while(0)
     #else
     #define icn85xx_trace(fmt, args...)   //
     #endif
     
     
     #ifdef DBG_ICN85XX_POINT
     #define icn85xx_point_info(fmt, args...)   \
             do{                              \
                     printk(fmt, ##args);     \
             }while(0)
     #else
     #define icn85xx_point_info(fmt, args...)   //
     #endif
     
     #ifdef DBG_ICN85XX_INFO
     #define icn85xx_info(fmt, args...)   \
             do{                              \
                     printk(fmt, ##args);     \
             }while(0)
     #else
     #define icn85xx_info(fmt, args...)   //
     #endif
     
     #ifdef DBG_ICN85XX_ERROR
     #define icn85xx_error(fmt, args...)   \
             do{                              \
                     printk(fmt, ##args);     \
             }while(0)
     #else
     #define icn85xx_error(fmt, args...)   //
     #endif
     
     #ifdef DBG_FLASH_INFO
     #define flash_info(fmt, args...)   \
             do{                              \
                     printk(fmt, ##args);     \
             }while(0)
     #else
     #define flash_info(fmt, args...)   //
     #endif
     
     #ifdef DBG_FLASH_ERROR
     #define flash_error(fmt, args...)   \
             do{                              \
                     printk(fmt, ##args);     \
             }while(0)
     #else
     #define flash_error(fmt, args...)   //
     #endif
     
     
     #ifdef DBG_OP_INFO
     #define op_info(fmt, args...)   \
             do{                              \
                     printk(fmt, ##args);     \
             }while(0)
     #else
     #define op_info(fmt, args...)   //
     #endif
     #ifdef DBG_OP_ERROR
     #define op_error(fmt, args...)   \
             do{                              \
                     printk(fmt, ##args);     \
             }while(0)
     #else
     #define op_error(fmt, args...)   //
     #endif
     
     
     #ifdef DBG_CALIB_INFO
     #define calib_info(fmt, args...)   \
             do{                              \
                     printk(fmt, ##args);     \
             }while(0)
     #else
     #define calib_info(fmt, args...)   //
     #endif
     
     #ifdef DBG_CALIB_ERROR
     #define calib_error(fmt, args...)   \
             do{                              \
                     printk(fmt, ##args);     \
             }while(0)
     #else
     #define calib_error(fmt, args...)   //
     #endif
     
     
     #ifdef DBG_PROC_INFO
     #define proc_info(fmt, args...)   \
             do{                              \
                     printk(fmt, ##args);     \
             }while(0)
     #else
     #define proc_info(fmt, args...)   //
     #endif
     
     #ifdef DBG_PROC_ERROR
     #define proc_error(fmt, args...)   \
             do{                              \
                     printk(fmt, ##args);     \
             }while(0)
     #else
     #define proc_error(fmt, args...)   //
     #endif
     
     #define swap_ab(a,b)       {char temp;temp=a;a=b;b=temp;}
     #define U16LOBYTE(var)     (*(unsigned char *) &var) 
     #define U16HIBYTE(var)     (*(unsigned char *)((unsigned char *) &var + 1))     
     
#define STRUCT_OFFSET(StructName,MemberName) ((int)(&(((StructName*)0)->MemberName)))
     

//-----------------------------------------------------------------------------
// Struct, Union and Enum DEFINITIONS
//-----------------------------------------------------------------------------
typedef struct _POINT_INFO
{
    unsigned char  u8ID;
    unsigned short u16PosX;     // coordinate X, plus 4 LSBs for precision extension
    unsigned short u16PosY;     // coordinate Y, plus 4 LSBs for precision extension
    unsigned char  u8Pressure;
    unsigned char  u8EventId;
}POINT_INFO;

struct tp_board_info {
	struct regulator *power;
	struct regulator *power2;
	struct device_pm_platdata *pm_platdata;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_sleep;
	struct pinctrl_state *pins_inactive;
    int         screen_max_x;
    int         screen_max_y;
    int         revert_x_flag;
    int         revert_y_flag;
	int irq_pin;
	int reset_pin;
};

struct icn85xx_ts_data {
    struct i2c_client        *client;
    struct input_dev         *input_dev;
    struct work_struct       pen_event_work;
    struct workqueue_struct  *ts_workqueue;
#ifdef CONFIG_HAS_EARLYSUSPEND
    struct early_suspend     early_suspend;
#endif
    struct hrtimer           timer;
    spinlock_t               irq_lock;
    struct semaphore         sem;  
//	struct	tp_device  tp;
    int         ictype;
    int         code_loaded_flag; 
    POINT_INFO  point_info[POINT_NUM+1];
    int         point_num;
    int         irq;
    int         irq_is_disable;
    int         use_irq;
    int         work_mode;
    int         screen_max_x;
    int         screen_max_y;
    int         revert_x_flag;
    int         revert_y_flag;
    int         exchange_x_y_flag;    
	int is_suspend;
    struct tp_board_info *board_info;
};

#pragma pack(1)
typedef struct{
    unsigned char  wr;         //write read flag£¬0:R  1:W
    unsigned char  flag;       //0:
    unsigned char  circle;     //polling cycle 
    unsigned char  times;      //plling times
    unsigned char  retry;      //I2C retry times
    unsigned int   data_len;   //data length
    unsigned char  addr_len;   //address length
    unsigned char  addr[2];    //address    
    unsigned char* data;       //data pointer
}pack_head;
typedef struct _STRUCT_PANEL_PARA
{
    unsigned short  u16ResX;                                     //   Row of resolution
    unsigned short  u16ResY;                                     //   Col of resolution
     
    unsigned char   u8RowNum;                                   //    Row total number (Tp + vk)
    unsigned char   u8ColNum;                                   //    Column total number (Tp + vk)
    unsigned char   u8TXOrder[36];            //   TX Order, start from zero
    unsigned char   u8RXOrder[24];            //   TX Order, start from zero

    unsigned char   u8NumVKey;                                //  Virtual Key setting
    unsigned char   u8VKeyMode;
    unsigned char   u8TpVkOrder[4];
    unsigned char   u8VKDownThreshold;
    unsigned char   u8VKUpThreshold;
    
    unsigned char   u8MaxTouchNum;                               //   max touch  support

    unsigned char   u8ScanMode;                // scan mode
    unsigned short  u16BitFreq;
    unsigned short  u16FreqCycleNum[2];
    unsigned char   u8MultiDrvNum;
    unsigned char   u8WindowType;
    
    unsigned char   u8FreHopMode;                                //  freq hopping
    unsigned short  u16FreHopBitFreq[5];       //   Bit Freq
    unsigned short  u16FreqHopCycleNum[5];      //   Cycle Num
    unsigned short  u16FreHopThreshold;                          //   Threshold of Freq Hop

    unsigned char   u8ShiftNum;                                  //  rawdata level
    unsigned char   u8DrvOutPutR;
    unsigned char   u8PgaC;
    unsigned char   u8RxVcmi;
    unsigned char   u8DacGain;
    unsigned char   u8PgaGain;
    unsigned char   u8PgaR;
    unsigned char   u8SpaceHolder[300];
}STRUCT_PANEL_PARA_H;


typedef struct{
    unsigned short u16PosX;     // coordinate X, plus 4 LSBs for precision extension
    unsigned short u16PosY;     // coordinate Y, plus 4 LSBs for precision extension
    unsigned char  u8EventId;
}GESTURE_INFO;

typedef struct{
    unsigned char  u8Status;        //set 1 when gesture valid
    unsigned char  u8Gesture;       //gesture id
    unsigned char  u8GestureNum;       //gesture num
    GESTURE_INFO  point_info[64];
}GESTURE_DATA;

#pragma pack()

#define DATA_LENGTH_UINT            512
#define CMD_HEAD_LENGTH             (sizeof(pack_head) - sizeof(unsigned char *))
#define ICN85XX_ENTRY_NAME          "icn85xx_tool"


enum icn85xx_ts_regs {
    ICN85XX_REG_PMODE   = 0x04, /* Power Consume Mode       */  
};

typedef enum
{
    R_OK = 100,
    R_FILE_ERR,
    R_STATE_ERR,
    R_ERASE_ERR,
    R_PROGRAM_ERR,
    R_VERIFY_ERR,
}E_UPGRADE_ERR_TYPE;

//-----------------------------------------------------------------------------
// Global VARIABLES
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Function PROTOTYPES
//-----------------------------------------------------------------------------

void icn85xx_ts_reset(void);
int  icn85xx_i2c_rxdata(unsigned short addr, char *rxdata, int length);
int  icn85xx_i2c_txdata(unsigned short addr, char *txdata, int length);
int  icn85xx_write_reg(unsigned short addr, char para);
int  icn85xx_read_reg(unsigned short addr, char *pdata);
int  icn85xx_prog_i2c_rxdata(unsigned int addr, char *rxdata, int length);
int  icn85xx_prog_i2c_txdata(unsigned int addr, char *txdata, int length);
int  icn85xx_prog_write_reg(unsigned int addr, char para);
int  icn85xx_prog_read_reg(unsigned int addr, char *pdata);
int icn85xx_prog_read_page(unsigned int Addr,unsigned char *Buffer, unsigned int Length);

int  icn85xx_readVersion(void);
void icn85xx_rawdatadump(short *mem, int size, char br);
void icn85xx_set_fw(int size, unsigned char *buf);
void icn85xx_memdump(char *mem, int size);
int  icn85xx_checksum(int sum, char *buf, unsigned int size);
int  icn85xx_update_status(int status);
int  icn85xx_get_status(void);
int  icn85xx_open_fw( char *fw);
int  icn85xx_read_fw(int offset, int length, char *buf);
int  icn85xx_close_fw(void);
int  icn85xx_goto_progmode(void);
int  icn85xx_check_progmod(void);
int  icn85xx_read_flashid(void);
int  icn85xx_erase_flash(void);
int  icn85xx_prog_buffer(unsigned int flash_addr,unsigned int sram_addr,unsigned int copy_length,unsigned char program_type);
int  icn85xx_prog_data(unsigned int flash_addr, unsigned int data);
void  icn85xx_read_flash(unsigned int sram_address,unsigned int flash_address,unsigned long copy_length,unsigned char i2c_wire_num);
int  icn85xx_fw_download(unsigned int offset, unsigned char * buffer, unsigned int size);
int  icn85xx_bootfrom_flash(int ictype);
int  icn85xx_bootfrom_sram(void);
int  icn85xx_crc_enable(unsigned char enable);
unsigned int icn85xx_crc_calc(unsigned crc_in, char *buf, int len);

short  icn85xx_read_fw_Ver(char *fw);
int  icn85xx_fw_update(void *fw);
//E_UPGRADE_ERR_TYPE  icn85xx_fw_update(void *fw);
#endif

