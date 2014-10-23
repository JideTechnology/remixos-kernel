/*
 * linux/sound/soc/codecs/aic325x_tiload.h
 *
 *
 * Copyright (C) 2012 Texas Instruments, Inc.
 *
 *
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * History:
 *
 * 
 *
 * 
 */

#ifndef _aic325x_TILOAD_H
#define _aic325x_TILOAD_H

/* typedefs required for the included header files */
typedef char *string;

/* defines */
#define DEVICE_NAME		"tiload_node"
#define aic325x_IOC_MAGIC	0xE0
#define aic325x_IOMAGICNUM_GET	_IOR(aic325x_IOC_MAGIC, 1, int)
#define aic325x_IOMAGICNUM_SET	_IOW(aic325x_IOC_MAGIC, 2, int)

#endif
