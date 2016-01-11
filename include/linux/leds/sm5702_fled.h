/* include/linux/leds/sm5702_fled.h
 * Header of Siliconmitus SM5702 Flash LED Driver
 *
 * Copyright (C) 2013 Siliconmitus Technology Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef LINUX_LEDS_SM5702_FLED_H
#define LINUX_LEDS_SM5702_FLED_H
#include <linux/kernel.h>
#include "smfled.h"


#define SM5702_FLASH_CURRENT(mA) mA<700?(((mA-300)/25) & 0x1f):(((((mA-700)/50)+0x0F)) & 0x1f)
#define SM5702_MOVIE_CURRENT(mA) (((mA - 10) /10) & 0x1f)

typedef struct sm5702_fled_platform_data {
    unsigned int fled_flash_current;
    unsigned int fled_movie_current;
    struct pinctrl *fled_pinctrl;
    struct pinctrl_state *gpio_state_active;
    struct pinctrl_state *gpio_state_suspend;
} sm5702_fled_platform_data_t;

#define SM5702_FLEDCNTL1			0x13
#define SM5702_FLEDCNTL2			0x14
#define SM5702_FLEDCNTL3			0x15
#define SM5702_FLEDCNTL4			0x16
#define SM5702_FLEDCNTL5			0x17
#define SM5702_FLEDCNTL6			0x18

#define SM5702_FLEDEN_MASK          0x03
#define SM5702_FLEDEN_DISABLE       0x00
#define SM5702_FLEDEN_MOVIE_MODE    0x01
#define SM5702_FLEDEN_FLASH_MODE    0x02
#define SM5702_FLEDEN_EXTERNAL      0x03

#define SM5702_IFLED_MASK           0x1F

#define SM5702_IMLED_MASK           0x1F

extern int32_t sm5702_charger_notification(struct sm_fled_info *info, int32_t on);
extern int32_t sm5702_boost_notification(struct sm_fled_info *info, int32_t on);
extern int32_t sm5702_fled_notification(struct sm_fled_info *info);

extern void sm5702_fled_lock(struct sm_fled_info *fled_info);
extern void sm5702_fled_unlock(struct sm_fled_info *fled_info);

/* If you are using external GPIO to control movie and flash led,
 * you must call sm5702_fled_flash_critial_section_lock() for camera shot,
 * and call sm5702_fled_flash_critial_section_unlock() after camera mode.
 * example code :
 * struct camera_chip* chip;
 * if (chip->fled_info == NULL)
 *      chip->fled_info = sm_fled_get_info_by_name(NULL);
 * if (chip->fled_info)
 *      sm5702_fled_flash_critial_section_lock(chip->fled_info);
 *  ...
 *  ... camera opertion
 *  ...
 *  ... when finished camera operation
 * if (chip->fled_info)
 *      sm5702_fled_flash_critial_section_unlock(chip->fled_info);
 */
extern void sm5702_fled_flash_critial_section_lock(struct sm_fled_info *fled_info);
extern void sm5702_fled_flash_critial_section_unlock(struct sm_fled_info *fled_info);

#endif /* LINUX_LEDS_SM5702_FLED_H */
