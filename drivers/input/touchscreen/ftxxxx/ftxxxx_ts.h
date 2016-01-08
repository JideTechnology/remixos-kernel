#ifndef __LINUX_ftxxxx_TS_H__
#define __LINUX_ftxxxx_TS_H__

#include <linux/i2c.h>
#include <linux/input.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/input/mt.h>
#include <linux/switch.h>
#include <linux/gpio.h>


/* -- dirver configure -- */
#define CFG_MAX_TOUCH_POINTS	10

#define PRESS_MAX	0xFF
#define FT_PRESS	0x08

#define FTXXXX_NAME	"ftxxxx"
#define Focal_input_dev_name	"focal-touchscreen"


#define FT_MAX_ID	0x0F
#define FT_TOUCH_STEP	6
#define FT_TOUCH_X_H_POS		3
#define FT_TOUCH_X_L_POS		4
#define FT_TOUCH_Y_H_POS		5
#define FT_TOUCH_Y_L_POS		6
#define FT_TOUCH_XY_POS			7
#define FT_TOUCH_MISC			8
#define FT_TOUCH_EVENT_POS		3
#define FT_TOUCH_ID_POS			5

#define POINT_READ_BUF	(3 + FT_TOUCH_STEP * CFG_MAX_TOUCH_POINTS)

/*register address*/
#define FTXXXX_REG_CHIP_ID                   0xA3    //chip ID 
#define FTXXXX_REG_FW_VER		0xA6
#define FTXXXX_REG_POINT_RATE	0x88
#define FTXXXX_REG_THGROUP	0x80
#define FTXXXX_REG_VENDOR_ID	0xA8

#define FTXXXX_ENABLE_IRQ	1
#define FTXXXX_DISABLE_IRQ	0

int ftxxxx_i2c_Read(struct i2c_client *client, char *writebuf, int writelen, char *readbuf, int readlen);
int ftxxxx_i2c_Write(struct i2c_client *client, char *writebuf, int writelen);

void ftxxxx_reset_tp(int HighOrLow);

u8 get_focal_tp_fw(void);

void ftxxxx_Enable_IRQ(struct i2c_client *client, int enable);
struct Upgrade_Info{

	u8 CHIP_ID;
        u8 FTS_NAME[20];
        u8 TPD_MAX_POINTS;
        u8 AUTO_CLB;
        u16 delay_aa;         /*delay of write FT_UPGRADE_AA */
        u16 delay_55;         /*delay of write FT_UPGRADE_55 */
        u8 upgrade_id_1;        /*upgrade id 1 */
        u8 upgrade_id_2;        /*upgrade id 2 */
        u16 delay_readid;        /*delay of read id */
        u16 delay_earse_flash; /*delay of earse flash*/
};

enum key_index_t {
	KEY_HOME_INDEX,
	KEY_BACK_INDEX,
	KEY_MENU_INDEX,
	KEY_SEARCH_INDEX,
	KEY_INDEX_MAX,
};

static int const android_key[KEY_INDEX_MAX] = {
	KEY_HOME,
	KEY_BACK,
	KEY_MENU,
	KEY_SEARCH,
};

/* The platform data for the Focaltech ftxxxx touchscreen driver */
struct ftxxxx_ts_platform_data {
	int irq_pin;
	int reset_pin;
	struct regulator *power;
	struct regulator *power2;
	bool polling_mode;
	struct device_pm_platdata *pm_platdata;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_sleep;
	struct pinctrl_state *pins_inactive;
	int x_pos_max;
	int x_pos_min;
	int y_pos_max;
	int y_pos_min;
	int screen_max_y;
	int screen_max_x;
	int key_y;
	int key_x[KEY_INDEX_MAX];
};

struct ts_event {
	u16 au16_x[CFG_MAX_TOUCH_POINTS];	/*x coordinate */
	u16 au16_y[CFG_MAX_TOUCH_POINTS];	/*y coordinate */
	u8 au8_touch_event[CFG_MAX_TOUCH_POINTS];	/*touch event:0 -- down; 1-- contact; 2 -- contact */
	u8 au8_finger_id[CFG_MAX_TOUCH_POINTS];	/*touch ID */
	u8 au8_finger_weight[CFG_MAX_TOUCH_POINTS];	/*touch weight */
	u8 pressure[CFG_MAX_TOUCH_POINTS];
	u8 area[CFG_MAX_TOUCH_POINTS];
	u8 touch_point;
	u8 touch_point_num;
};

struct ftxxxx_ts_data {
	unsigned int irq;
	unsigned int x_max;
	unsigned int y_max;
	unsigned int init_success;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct ts_event event;
	struct ftxxxx_ts_platform_data *pdata;
	struct mutex g_device_mutex;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend es;
#endif
int touchs;
};

#endif
