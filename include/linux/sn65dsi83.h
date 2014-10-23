#ifndef _sn65dsi83_TS_H_
#define _sn65dsi83_TS_H_
#include <linux/regulator/consumer.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>

#define SN65DSI83_I2C_ADDR		(0x2D)

#define SN65DSI83_I2C_NUM 0

#define SN65DSI83_DEV_NAME "sn65dsi83"
#if 0
#define SN65DSI83_DATA(_SCREENON_CALLBACK, _SCREENOFF_CALLBACK) { \
	.ssd_init_callback = _SCREENON_CALLBACK, \
	.ssd_deinit_callback = _SCREENOFF_CALLBACK, \
}

#define SN65DSI83_PDATA(_REGISTER_CALLBACK, _MIPI_EN_GPIO,_IRQ) { \
	.register_resume = _REGISTER_CALLBACK, \
	.mipi_enable_gpio = _MIPI_EN_GPIO, \
}
#endif
struct bridge_reg {
	u8 addr;
	u8 val;
};

struct sn65dsi83_data {
	void (*ssd_init_callback)(void);
	void (*ssd_deinit_callback)(void);
};

struct sn65dsi83_platform_data{
	int (*register_resume)(struct sn65dsi83_data *);
	int mipi_enable_gpio;
};

#endif				
