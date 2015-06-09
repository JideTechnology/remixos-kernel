/*
 * drivers/input/tablet/wacom.h
 *
 *  USB Wacom tablet support
 *
 *  Copyright (c) 2000-2004 Vojtech Pavlik	<vojtech@ucw.cz>
 *  Copyright (c) 2000 Andreas Bach Aaen	<abach@stofanet.dk>
 *  Copyright (c) 2000 Clifford Wolf		<clifford@clifford.at>
 *  Copyright (c) 2000 Sam Mosel		<sam.mosel@computer.org>
 *  Copyright (c) 2000 James E. Blair		<corvus@gnu.org>
 *  Copyright (c) 2000 Daniel Egger		<egger@suse.de>
 *  Copyright (c) 2001 Frederic Lepied		<flepied@mandrakesoft.com>
 *  Copyright (c) 2004 Panagiotis Issaris	<panagiotis.issaris@mech.kuleuven.ac.be>
 *  Copyright (c) 2002-2009 Ping Cheng		<pingc@wacom.com>
 *
 *  ChangeLog:
 *      v0.1 (vp)  - Initial release
 *      v0.2 (aba) - Support for all buttons / combinations
 *      v0.3 (vp)  - Support for Intuos added
 *	v0.4 (sm)  - Support for more Intuos models, menustrip
 *			relative mode, proximity.
 *	v0.5 (vp)  - Big cleanup, nifty features removed,
 *			they belong in userspace
 *	v1.8 (vp)  - Submit URB only when operating, moved to CVS,
 *			use input_report_key instead of report_btn and
 *			other cleanups
 *	v1.11 (vp) - Add URB ->dev setting for new kernels
 *	v1.11 (jb) - Add support for the 4D Mouse & Lens
 *	v1.12 (de) - Add support for two more inking pen IDs
 *	v1.14 (vp) - Use new USB device id probing scheme.
 *		     Fix Wacom Graphire mouse wheel
 *	v1.18 (vp) - Fix mouse wheel direction
 *		     Make mouse relative
 *      v1.20 (fl) - Report tool id for Intuos devices
 *                 - Multi tools support
 *                 - Corrected Intuos protocol decoding (airbrush, 4D mouse, lens cursor...)
 *                 - Add PL models support
 *		   - Fix Wacom Graphire mouse wheel again
 *	v1.21 (vp) - Removed protocol descriptions
 *		   - Added MISC_SERIAL for tool serial numbers
 *	      (gb) - Identify version on module load.
 *    v1.21.1 (fl) - added Graphire2 support
 *    v1.21.2 (fl) - added Intuos2 support
 *                 - added all the PL ids
 *    v1.21.3 (fl) - added another eraser id from Neil Okamoto
 *                 - added smooth filter for Graphire from Peri Hankey
 *                 - added PenPartner support from Olaf van Es
 *                 - new tool ids from Ole Martin Bjoerndalen
 *	v1.29 (pc) - Add support for more tablets
 *		   - Fix pressure reporting
 *	v1.30 (vp) - Merge 2.4 and 2.5 drivers
 *		   - Since 2.5 now has input_sync(), remove MSC_SERIAL abuse
 *		   - Cleanups here and there
 *    v1.30.1 (pi) - Added Graphire3 support
 *	v1.40 (pc) - Add support for several new devices, fix eraser reporting, ...
 *	v1.43 (pc) - Added support for Cintiq 21UX
 *		   - Fixed a Graphire bug
 *		   - Merged wacom_intuos3_irq into wacom_intuos_irq
 *	v1.44 (pc) - Added support for Graphire4, Cintiq 710, Intuos3 6x11, etc.
 *		   - Report Device IDs
 *      v1.45 (pc) - Added support for DTF 521, Intuos3 12x12 and 12x19
 *                 - Minor data report fix
 *      v1.46 (pc) - Split wacom.c into wacom_sys.c and wacom_wac.c,
 *		   - where wacom_sys.c deals with system specific code,
 *		   - and wacom_wac.c deals with Wacom specific code
 *		   - Support Intuos3 4x6
 *      v1.47 (pc) - Added support for Bamboo
 *      v1.48 (pc) - Added support for Bamboo1, BambooFun, and Cintiq 12WX
 *      v1.49 (pc) - Added support for USB Tablet PC (0x90, 0x93, and 0x9A)
 *      v1.50 (pc) - Fixed a TabletPC touch bug in 2.6.28
 *      v1.51 (pc) - Added support for Intuos4
 *      v1.52 (pc) - Query Wacom data upon system resume
 *                 - add defines for features->type
 *                 - add new devices (0x9F, 0xE2, and 0XE3)
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef WACOM_H
#define WACOM_H
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/init.h>
#include <linux/usb/input.h>
#include <asm/unaligned.h>

/* maximum packet length for USB devices */
#define WACOM_PKGLEN_MAX	32

/* packet length for individual models */
#define WACOM_PKGLEN_PENPRTN	 7
#define WACOM_PKGLEN_GRAPHIRE	 8
#define WACOM_PKGLEN_BBFUN	 9
#define WACOM_PKGLEN_INTUOS	10
#define WACOM_PKGLEN_TPC1FG	 5
#define WACOM_PKGLEN_TPC2FG	14
#define WACOM_PKGLEN_BBTOUCH	20

/* device IDs */
#define STYLUS_DEVICE_ID	0x02
#define TOUCH_DEVICE_ID		0x03
#define CURSOR_DEVICE_ID	0x06
#define ERASER_DEVICE_ID	0x0A
#define PAD_DEVICE_ID		0x0F

/* wacom data packet report IDs */
#define WACOM_REPORT_PENABLED		2
#define WACOM_REPORT_INTUOSREAD		5
#define WACOM_REPORT_INTUOSWRITE	6
#define WACOM_REPORT_INTUOSPAD		12
#define WACOM_REPORT_TPC1FG		6
#define WACOM_REPORT_TPC2FG		13

/* device quirks */
#define WACOM_QUIRK_MULTI_INPUT		0x0001
#define WACOM_QUIRK_BBTOUCH_LOWRES	0x0002

enum {
	PENPARTNER = 0,
	GRAPHIRE,
	WACOM_G4,
	PTU,
	PL,
	DTU,
	BAMBOO_PT,
	INTUOS,
	INTUOS3S,
	INTUOS3,
	INTUOS3L,
	INTUOS4S,
	INTUOS4,
	INTUOS4L,
	WACOM_21UX2,
	CINTIQ,
	WACOM_BEE,
	WACOM_MO,
	TABLETPC,
	TABLETPC2FG,
	MAX_TYPE
};

struct wacom_features {
	const char *name;
	int pktlen;
	int x_max;
	int y_max;
	int pressure_max;
	int distance_max;
	int type;
	int x_resolution;
	int y_resolution;
	unsigned int pid;
	int device_type;
	int x_phy;
	int y_phy;
	unsigned char unit;
	unsigned char unitExpo;
	int x_fuzz;
	int y_fuzz;
	int pressure_fuzz;
	int distance_fuzz;
	unsigned quirks;
};

struct wacom_shared {
	bool stylus_in_proximity;
	bool touch_down;
};

struct wacom_wac {
	char name[64];
	unsigned char *data;
	int tool[2];
	int id[2];
	__u32 serial[2];
	struct wacom_features features;
	struct wacom_shared *shared;
	struct input_dev *input;
};

/*
 * Version Information
 */
#define DRIVER_VERSION "v1.52"
#define DRIVER_AUTHOR "Vojtech Pavlik <vojech@ucw.cz>"
#define DRIVER_DESC "USB Wacom tablet driver"
#define DRIVER_LICENSE "GPL"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE(DRIVER_LICENSE);

#define USB_VENDOR_ID_WACOM	0x056a
#define USB_VENDOR_ID_LENOVO	0x17ef

/* defines to get HID report descriptor */
#define HID_DEVICET_HID		(USB_TYPE_CLASS | 0x01)
#define HID_DEVICET_REPORT	(USB_TYPE_CLASS | 0x02)
#define HID_USAGE_UNDEFINED		0x00
#define HID_USAGE_PAGE			0x05
#define HID_USAGE_PAGE_DIGITIZER	0x0d
#define HID_USAGE_PAGE_DESKTOP		0x01
#define HID_USAGE			0x09
#define HID_USAGE_X			0x30
#define HID_USAGE_Y			0x31
#define HID_USAGE_X_TILT		0x3d
#define HID_USAGE_Y_TILT		0x3e
#define HID_USAGE_FINGER		0x22
#define HID_USAGE_STYLUS		0x20
#define HID_COLLECTION			0xc0

enum {
	WCM_UNDEFINED = 0,
	WCM_DESKTOP,
	WCM_DIGITIZER,
};

struct hid_descriptor {
	struct usb_descriptor_header header;
	__le16   bcdHID;
	u8       bCountryCode;
	u8       bNumDescriptors;
	u8       bDescriptorType;
	__le16   wDescriptorLength;
} __attribute__ ((packed));

/* defines to get/set USB message */
#define USB_REQ_GET_REPORT	0x01
#define USB_REQ_SET_REPORT	0x09
#define WAC_HID_FEATURE_REPORT	0x03

#define MAX_SYSFS_SIZE 4095
#define MAX_ELEMENT_SIZE 10
#define DATASIZE_IN_ONESHOT (MAX_SYSFS_SIZE - MAX_ELEMENT_SIZE)
#define MAX_DATASIZE  (65536 * 5)
#define REST_OF_DATA (MAX_DATASIZE % MAX_SYSFS_SIZE)

struct wacom {
	dma_addr_t data_dma;
	struct usb_device *usbdev;
	struct usb_interface *intf;
	struct urb *irq;
	struct wacom_wac wacom_wac;
	struct mutex lock;
	bool open;
	char phys[32];
};

struct wacom_boot {
	dma_addr_t data_dma;
	struct usb_device *usbdev;
	struct usb_interface *intf;
	struct urb *irq;
};

struct wacom_sysfs {
	struct device *dev;
	struct class *class;
	struct usb_interface intf[127];
	struct mutex lock;
	unsigned long data_count;
	unsigned int pid[127];
	dev_t dev_t;
	u8 *data;
	bool storing_started;
};

extern bool sysfs_enabled;
extern unsigned char counter;
extern struct wacom_sysfs *wacom_sysfs;
extern const struct usb_device_id wacom_ids[];

void wacom_wac_irq(struct wacom_wac *wacom_wac, size_t len);
void wacom_setup_device_quirks(struct wacom_features *features);
void wacom_setup_input_capabilities(struct input_dev *input_dev,
				    struct wacom_wac *wacom_wac);
int wacom_enter(struct wacom_sysfs *wac_boot, int intf_index);
int wacom_out(struct wacom_sysfs *wac_boot, int intf_index);
int wacom_flash(struct wacom_sysfs *wac_boot, int index, u8 *data);
int register_sysfs(struct wacom_sysfs *wacom);
void remove_sysfs(struct wacom_sysfs *wacom);
#define SYSFS_ENABLED 1
#define ERR_REGISTER_SYSFS 2
#define NOBOOTDEV 3
#endif
