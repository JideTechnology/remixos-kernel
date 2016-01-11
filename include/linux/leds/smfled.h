/* include/linux/leds/smfled.h
 * Header of Siliconmitus Flash LED Driver
 *
 * Copyright (C) 2013 Siliconmitus Technology Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef LINUX_LEDS_SMFLED_H
#define LINUX_LEDS_SMFLED_H
#include <linux/leds/flashlight.h>

struct sm_fled_info;
typedef int (*sm_hal_fled_init)(struct sm_fled_info *info);
typedef int (*sm_hal_fled_suspend)(struct sm_fled_info *info, pm_message_t state);
typedef int (*sm_hal_fled_resume)(struct sm_fled_info *info);
typedef int (*sm_hal_fled_set_mode)(struct sm_fled_info *info, flashlight_mode_t mode);
typedef int (*sm_hal_fled_get_mode)(struct sm_fled_info *info);
typedef int (*sm_hal_fled_flash)(struct sm_fled_info *info, int turn_way);

/* Return value : -EINVAL => selector parameter is out of range, otherwise current in uA*/
typedef int (*sm_hal_fled_movie_current_list)(struct sm_fled_info *info, int selector);
typedef int (*sm_hal_fled_flash_current_list)(struct sm_fled_info *info, int selector);

typedef int (*sm_hal_fled_set_movie_current)(struct sm_fled_info *info,
                                              int min_uA, int max_uA, int *selector);
typedef int (*sm_hal_fled_set_flash_current)(struct sm_fled_info *info,
                                               int min_uA, int max_uA, int *selector);
typedef int (*sm_hal_fled_set_movie_current_sel)(struct sm_fled_info *info,
                                              int selector);
typedef int (*sm_hal_fled_set_flash_current_sel)(struct sm_fled_info *info,
                                               int selector);
typedef int (*sm_hal_fled_get_movie_current_sel)(struct sm_fled_info *info);
typedef int (*sm_hal_fled_get_flash_current_sel)(struct sm_fled_info *info);
typedef int (*sm_hal_fled_get_movie_current)(struct sm_fled_info *info);
typedef int (*sm_hal_fled_get_flash_current)(struct sm_fled_info *info);
typedef void (*sm_hal_fled_shutdown)(struct sm_fled_info *info);

struct sm_fled_hal {
    sm_hal_fled_init fled_init;
    sm_hal_fled_suspend fled_suspend;
    sm_hal_fled_resume fled_resume;
    sm_hal_fled_set_mode fled_set_mode;
    sm_hal_fled_get_mode fled_get_mode;
    sm_hal_fled_flash fled_flash;
    sm_hal_fled_movie_current_list  fled_movie_current_list;
    sm_hal_fled_flash_current_list fled_flash_current_list;
    
    /* method to set */
    sm_hal_fled_set_movie_current_sel fled_set_movie_current_sel;
    sm_hal_fled_set_flash_current_sel fled_set_flash_current_sel;

    /* method to set, optional */
    sm_hal_fled_set_movie_current fled_set_movie_current;
    sm_hal_fled_set_flash_current fled_set_flash_current;
    /* method to get */
    sm_hal_fled_get_movie_current_sel fled_get_movie_current_sel;
    sm_hal_fled_get_flash_current_sel fled_get_flash_current_sel;
    /* method to get, optional*/
    sm_hal_fled_get_movie_current fled_get_movie_current;
    sm_hal_fled_get_flash_current fled_get_flash_current;
    /* PM shutdown, optional */
    sm_hal_fled_shutdown fled_shutdown;
};

typedef struct sm_fled_info {
    struct sm_fled_hal *hal;
    struct flashlight_device *flashlight_dev;
    const struct flashlight_properties *init_props;
    char *name;
    char *chip_name;
} sm_fled_info_t;

enum {
    TURN_WAY_I2C = 0,
    TURN_WAY_GPIO = 1,
};

/* Public funtions
 * argument
 *   @name : Flash LED's name;pass NULL menas "sm-flash-led"
 */

sm_fled_info_t *sm_fled_get_info_by_name(char *name);

/* Usage :
 * fled_info = sm_fled_get_info_by_name("FlashLED1");
 * fled_info->hal->fled_set_flash_current(fled_info,
 *                                        150, 200);
 */

#endif /*LINUX_LEDS_SMFLED_H*/
