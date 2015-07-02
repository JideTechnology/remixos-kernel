#ifndef __PI3USB_MUX_H__
#define __PI3USB_MUX_H__

#define CHV_HPD_GPIO	409
#define CHV_HPD_INV_BIT		(1 << 6)

void chv_gpio_cfg_inv(int gpio, int inv, int en);

#endif
