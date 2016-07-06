#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/dmi.h>
#include <linux/efi.h>
#include <linux/pci.h>
#include <linux/acpi.h>
#include <linux/delay.h>
#include <media/v4l2-subdev.h>
#include <linux/mfd/intel_soc_pmic.h>
#include <linux/vlv2_plat_clock.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/atomisp_platform.h>
#include <linux/atomisp_gmin_platform.h>
#include <asm/spid.h>

#define MAX_SUBDEVS 8

/* This needs to be initialized at runtime so the various
 * platform-checking macros in spid.h return the correct results.
 * Either that, or we need to fix up the usage of those macros so that
 * it's checking more appropriate runtime-detectable data. */
struct soft_platform_id spid;
EXPORT_SYMBOL(spid);

#define DEVNAME_PMIC_AXP "INT33F4:00"
#define DEVNAME_PMIC_TI  "INT33F5:00"
#define DEVNAME_PMIC_CRYSTALCOVE "INT33FD:00"

/* Should be defined in vlv2_plat_clock API, isn't: */
#define VLV2_CLK_PLL_19P2MHZ 1
#define VLV2_CLK_XTAL_19P2MHZ 0
#define VLV2_CLK_ON      1
#define VLV2_CLK_OFF     2

/* X-Powers AXP288 register hackery */
#define ALDO1_CTRL3_REG	0x13
#define ALDO1_SEL_REG	0x28
#define ALDO1_2P8V	0x15   //0x2a  default:0x16
#define ALDO1_CTRL3_SHIFT 0x05

#define ELDO_CTRL_REG   0x12
#define ELDO1_SEL_REG	0x19
#define ELDO1_1P6V	0x12
#define ELDO1_CTRL_SHIFT 0x0
/* 50mv step for following*/
#define ELDO2_SEL_REG	0x1a
#define ELDO2_1P8V	0x16
#define ELDO2_CTRL_SHIFT 0x01

#define FLDO_CTRL_REG   0x13
#define FLDO2_SEL_REG  0x1d
#define FLDO2_1P2V     0x0a
#define FLDO2_CTRL_SHIFT 0x3

/* TI SND9039 PMIC register hackery */
#define LDO9_REG	0x49
#define LDO10_REG	0x4a
#define LDO11_REG	0x4b

#define LDO_2P8V_ON	0x2f /* 0x2e selects 2.85V ...      */
#define LDO_2P8V_OFF	0x2e /* ... bottom bit is "enabled" */

#define LDO_1P8V_ON	0x59 /* 0x58 selects 1.80V ...      */
#define LDO_1P8V_OFF	0x58 /* ... bottom bit is "enabled" */

/* CRYSTAL COVE PMIC register hackery */
#define CRYSTAL_1P8V_REG        0x57
#define CRYSTAL_2P8V_REG        0x5d
#define CRYSTAL_ON      0x63
#define CRYSTAL_OFF     0x62

/*Whiskey Cove reg*/
#define WCOVE_V1P8SX_CTRL	0x57
#define WCOVE_V2P8SX_CTRL	0x5d
#define WCOVE_CTRL_MASK		0x7
#define WCOVE_CTRL_ENABLE	0x2
#define WCOVE_CTRL_DISABLE	0x0


struct gmin_subdev {
	struct v4l2_subdev *subdev;
	int clock_num;
	int clock_src;
	struct gpio_desc *gpio0;
	struct gpio_desc *gpio1;
	struct regulator *v1p8_reg;
	struct regulator *v2p8_reg;
	enum atomisp_camera_port csi_port;
	unsigned int csi_lanes;
	enum atomisp_input_format csi_fmt;
	enum atomisp_bayer_order csi_bayer;
	bool v1p8_on;
	bool v2p8_on;
	int eldo1_sel_reg, eldo1_1p8v, eldo1_ctrl_shift;
	int eldo2_sel_reg, eldo2_1p8v, eldo2_ctrl_shift;
	int fldo2_sel_reg,fldo2_1p2v,fldo2_ctrl_shift;
};

static struct gmin_subdev gmin_subdevs[MAX_SUBDEVS];

static enum { PMIC_UNSET = 0, PMIC_REGULATOR, PMIC_AXP, PMIC_TI ,
	PMIC_CRYSTALCOVE } pmic_id;

/* The atomisp uses type==0 for the end-of-list marker, so leave space. */
static struct intel_v4l2_subdev_table pdata_subdevs[MAX_SUBDEVS+1];

static const struct atomisp_platform_data pdata = {
	.subdevs = pdata_subdevs,
};

/*
 * Something of a hack.  The ECS E7 board drives camera 2.8v from an
 * external regulator instead of the PMIC.  There's a gmin_CamV2P8
 * config variable that specifies the GPIO to handle this particular
 * case, but this needs a broader architecture for handling camera
 * power.
 */
enum { V2P8_GPIO_UNSET = -2, V2P8_GPIO_NONE = -1 };
static int v2p8_gpio = V2P8_GPIO_UNSET;

/*
 * Something of a hack. The CHT RVP board drives camera 1.8v from an
 * external regulator instead of the PMIC just like ECS E7 board, see the
 * comments above.
 */
enum { V1P8_GPIO_UNSET = -2, V1P8_GPIO_NONE = -1 };
static int v1p8_gpio = V1P8_GPIO_UNSET;

static LIST_HEAD(vcm_devices);
static DEFINE_MUTEX(vcm_lock);

static struct gmin_subdev *find_gmin_subdev(struct v4l2_subdev *subdev);

/*
 * Legacy/stub behavior copied from upstream platform_camera.c.  The
 * atomisp driver relies on these values being non-NULL in a few
 * places, even though they are hard-coded in all current
 * implementations.
 */
const struct atomisp_camera_caps *atomisp_get_default_camera_caps(void)
{
	static const struct atomisp_camera_caps caps = {
		.sensor_num = 1,
		.sensor = {
			{ .stream_num = 1, },
		},
	};
	return &caps;
}
EXPORT_SYMBOL_GPL(atomisp_get_default_camera_caps);

#ifdef CONFIG_VIDEO_CAMERA_PLUG_AND_PLAY
enum {
	I2C_BUS_0 = 0,
	I2C_BUS_1,
	I2C_BUS_2,
} I2C_BUS_NR;

/* Not used yet */
static struct i2c_board_info board_info[3];

struct atomisp_camera_cht_table {
	const char   *acpi_match_name;
	const char   *subdev_match_name;
	const char   *dev_name;
	const char   *i2c_dev_name;
	unsigned char i2c_addr;
	unsigned char flags;
};

static struct atomisp_camera_cht_table cht_cam_comp_tab[] = {
	/* OV8858 */
	{"i2c-OVB2680:00",  "OVB2680:00",  "rear",  "i2c-rear:36",  0x36, 0x00},
	/* OV5648 */
	{"i2c-INT5648:00",  "INT5648:00",  "rear",  "i2c-rear:36",  0x36, 0x00},
	/* OV2680 */
	{"i2c-OVTI2680:00", "OVTI2680:00", "front", "i2c-front:36", 0x36, 0x00},
	/* GC2355 */
	{"i2c-OVTI2680:01", "OVTI2680:01", "front", "i2c-front:10", 0x10, 0x00}
};


/* Using some arbitary limit for max adresses in resource */
#define MAX_CRS_ELEMENTS	20
struct i2c_resource_info {
	struct i2c_comp_address addrs[MAX_CRS_ELEMENTS];
	int count;
	int common_irq;
	unsigned long irq_flags;
};

/*
 * Copy from i2c-core.c;
 */
static int atomisp_acpi_i2c_add_resource(struct acpi_resource *ares, void *data)
{
	struct i2c_resource_info *rcs_info = data;
	struct acpi_resource_i2c_serialbus *sb;
	struct resource r;

	if (ares->type == ACPI_RESOURCE_TYPE_SERIAL_BUS) {
		sb = &ares->data.i2c_serial_bus;
		if (sb->type == ACPI_RESOURCE_SERIAL_TYPE_I2C) {
			if (rcs_info->count >= MAX_CRS_ELEMENTS)
				return 1;
			rcs_info->addrs[rcs_info->count].addr =
							sb->slave_address;
			if (sb->access_mode == ACPI_I2C_10BIT_MODE)
				rcs_info->addrs[rcs_info->count].flags =
								I2C_CLIENT_TEN;
			rcs_info->count++;
		}
	} else if (rcs_info->common_irq < 0) {
		if (acpi_dev_resource_interrupt(ares, 0, &r)) {
			rcs_info->common_irq = r.start;
			rcs_info->irq_flags = r.flags;
		}
	}

	/* Tell the ACPI core to skip this resource */
	return 1;
}

/*
 * Copy from i2c-core.c
 * Used to add i2c device and trigger the driver probe
 */
static acpi_status atomisp_acpi_i2c_add_device(acpi_handle handle,
		u32 level, void *data, void **return_value)
{
	struct i2c_adapter *adapter = data;
	struct list_head resource_list;
	struct i2c_board_info info;
	struct acpi_device *adev;
	struct i2c_resource_info rcs_info;
	struct i2c_client *i2c_client;
	struct device *dev;
	int ret = 0;
	int i;

	if (acpi_bus_get_device(handle, &adev))
		return AE_OK;
	if (acpi_bus_get_status(adev) || !adev->status.present)
		return AE_OK;

	memset(&info, 0, sizeof(info));
	info.acpi_node.companion = adev;

	memset(&rcs_info, 0, sizeof(rcs_info));
	rcs_info.common_irq = -1;

	INIT_LIST_HEAD(&resource_list);
	ret = acpi_dev_get_resources(adev, &resource_list,
				     atomisp_acpi_i2c_add_resource, &rcs_info);
	acpi_dev_free_resource_list(&resource_list);

	if (ret < 0)
		return AE_OK;

	adev->power.flags.ignore_parent = true;
	info.irq                        = rcs_info.common_irq;
	info.irq_flags                  = rcs_info.irq_flags;
	info.comp_addr_count            = rcs_info.count;

	for (i = 0; i < ARRAY_SIZE(cht_cam_comp_tab); i++) {
		if (!strncmp(dev_name(&adev->dev),
					cht_cam_comp_tab[i].subdev_match_name,
					strlen(dev_name(&adev->dev)) + 1) &&
				!cht_cam_comp_tab[i].flags) {
			info.addr  = cht_cam_comp_tab[i].i2c_addr;
			info.flags = cht_cam_comp_tab[i].flags;

			snprintf(info.type, sizeof(info.type), "%s:%.2x",
					cht_cam_comp_tab[i].dev_name,
					info.addr);

			/* Fixme */
			info.comp_addrs = kmemdup(rcs_info.addrs,
					rcs_info.count *
					sizeof(struct i2c_comp_address),
					GFP_KERNEL);

			/* Register new i2c device to trigger probe */
			i2c_client = i2c_new_device(adapter, &info);
			if (!i2c_client) {
				if (!i)
					adev->power.flags.ignore_parent = false;

				dev_err(&adapter->dev,
					"failed to add I2C device %s from ACPI\n",
						dev_name(&adev->dev));

				kfree(info.comp_addrs);
				continue;
			}

			pr_debug("%s(): i is %d,\
					adapter->name is %s,\
					client->addr is 0x%.2X,\
					client->irq is %d,\
					client->name is %s,\
					dev_name is %s.\n",
					__func__, i,
					adapter->name,
					i2c_client->addr,
					i2c_client->irq,
					i2c_client->name,
					cht_cam_comp_tab[i].dev_name);

			/* Check the result of probe() finally */
			dev = bus_find_device_by_name(&i2c_bus_type,
					NULL,
					cht_cam_comp_tab[i].i2c_dev_name);
			if (dev && dev->driver) {
				pr_info("%s(): %s has been registered successfully!\n",
						__func__,
						dev->driver->name);

				cht_cam_comp_tab[i].flags = 1;
			} else {
				pr_err("%s(): %s doesn't be registered correctly!\n",
						__func__,
						cht_cam_comp_tab[i].i2c_dev_name);

				//i2c_unregister_device(i2c_client);
			}
		}
	}

	return AE_OK;
}

/*
 * Copy from i2c-core.c
 * Call atomisp_acpi_i2c_add_device() to register i2c device
 * in acpi namespace
 */
static int atomisp_register_acpi_devices(void)
{
	int i;
	int ret = 0;
	struct i2c_adapter *adapter;

	acpi_handle handle;
	acpi_status status;

	/* Fixme, in case of diff I2C adapter;
	 * But it must be spent more time */
	for (i = I2C_BUS_0; i <= I2C_BUS_2; i++) {
		adapter = i2c_get_adapter(i);

		if (!adapter->dev.parent) {
			ret = -EINVAL;
			goto err;
		}

		handle = ACPI_HANDLE(adapter->dev.parent);
		if (!handle) {
			ret = -EINVAL;
			goto err;
		}

		status = acpi_walk_namespace(ACPI_TYPE_DEVICE, handle, 1,
				atomisp_acpi_i2c_add_device, NULL,
				adapter, NULL);
		if (ACPI_FAILURE(status)) {
			dev_err(&adapter->dev, "failed to enumerate I2C slaves!\n");

			ret = -ENODEV;
		}
	}

err:
	return ret;
}

/*
 * Called by atomisp_subdev_probe() the beginning
 * Register i2c device
 */
int camera_init_device(void)
{
	return atomisp_register_acpi_devices();
}
EXPORT_SYMBOL_GPL(camera_init_device);
#endif /* CONFIG_VIDEO_CAMERA_PLUG_AND_PLAY */

const struct atomisp_platform_data *atomisp_get_platform_data(void)
{
	return &pdata;
}
EXPORT_SYMBOL_GPL(atomisp_get_platform_data);

static int af_power_ctrl(struct v4l2_subdev *subdev, int flag)
{
	return 0;
}

/*
 * Used in a handful of modules.  Focus motor control, I think.  Note
 * that there is no configurability in the API, so this needs to be
 * fixed where it is used.
 *
 * struct camera_af_platform_data {
 *     int (*power_ctrl)(struct v4l2_subdev *subdev, int flag);
 * };
 *
 * Note that the implementation in MCG platform_camera.c is stubbed
 * out anyway (i.e. returns zero from the callback) on BYT.  So
 * neither needed on gmin platforms or supported upstream.
 */
const struct camera_af_platform_data *camera_get_af_platform_data(void)
{
	static struct camera_af_platform_data afpd = {
		.power_ctrl = af_power_ctrl,
	};
	return &afpd;
}
EXPORT_SYMBOL_GPL(camera_get_af_platform_data);

int atomisp_register_i2c_module(struct v4l2_subdev *subdev,
                                struct camera_sensor_platform_data *plat_data,
                                enum intel_v4l2_subdev_type type)
{
	int i;
	struct i2c_board_info *bi;
	struct gmin_subdev *gs;
        struct i2c_client *client = v4l2_get_subdevdata(subdev);
	struct acpi_device *adev;

	dev_info(&client->dev, "register atomisp i2c module type %d\n", type);

	/* The windows driver model (and thus most BIOSes by default)
	 * uses ACPI runtime power management for camera devices, but
	 * we don't.  Disable it, or else the rails will be needlessly
	 * tickled during suspend/resume.  This has caused power and
	 * performance issues on multiple devices. */
	adev = ACPI_COMPANION(&client->dev);
	if (adev)
		adev->power.flags.power_resources = 0;

	for (i=0; i < MAX_SUBDEVS; i++)
		if (!pdata.subdevs[i].type)
			break;

	if (pdata.subdevs[i].type)
		return -ENOMEM;

	/* Note subtlety of initialization order: at the point where
	 * this registration API gets called, the platform data
	 * callbacks have probably already been invoked, so the
	 * gmin_subdev struct is already initialized for us. */
	gs = find_gmin_subdev(subdev);

	pdata.subdevs[i].type = type;
	pdata.subdevs[i].port = gs->csi_port;
	pdata.subdevs[i].subdev = subdev;
	pdata.subdevs[i].v4l2_subdev.i2c_adapter_id = client->adapter->nr;

	/* Convert i2c_client to i2c_board_info */
	bi = &pdata.subdevs[i].v4l2_subdev.board_info;
	memcpy(bi->type, client->name, I2C_NAME_SIZE);
	bi->flags = client->flags;
	bi->addr = client->addr;
	bi->irq = client->irq;
	bi->comp_addr_count = client->comp_addr_count;
	bi->comp_addrs = client->comp_addrs;
	bi->irq_flags = client->irq_flags;
	bi->platform_data = plat_data;

	return 0;
}
EXPORT_SYMBOL_GPL(atomisp_register_i2c_module);

struct v4l2_subdev *atomisp_gmin_find_subdev(struct i2c_adapter *adapter,
					     struct i2c_board_info *board_info)
{
	int i;
	for (i=0; i < MAX_SUBDEVS && pdata.subdevs[i].type; i++) {
		struct intel_v4l2_subdev_table *sd = &pdata.subdevs[i];
		if (sd->v4l2_subdev.i2c_adapter_id == adapter->nr &&
		    sd->v4l2_subdev.board_info.addr == board_info->addr)
			return sd->subdev;
	}
	return NULL;
}
EXPORT_SYMBOL_GPL(atomisp_gmin_find_subdev);

int atomisp_gmin_remove_subdev(struct v4l2_subdev *sd)
{
	int i, j;

	if (!sd)
		return 0;

	for (i = 0; i < MAX_SUBDEVS; i++) {
		if (pdata.subdevs[i].subdev == sd) {
			for (j = i + 1; j <= MAX_SUBDEVS; j++)
				pdata.subdevs[j - 1] = pdata.subdevs[j];
		}
		if (gmin_subdevs[i].subdev == sd) {
			if (gmin_subdevs[i].gpio0)
				gpiod_put(gmin_subdevs[i].gpio0);
			gmin_subdevs[i].gpio0 = NULL;
			if (gmin_subdevs[i].gpio1)
				gpiod_put(gmin_subdevs[i].gpio1);
			gmin_subdevs[i].gpio1 = NULL;
			if (pmic_id == PMIC_REGULATOR) {
				regulator_put(gmin_subdevs[i].v1p8_reg);
				regulator_put(gmin_subdevs[i].v2p8_reg);
			}
			gmin_subdevs[i].subdev = NULL;
		}
	}
	return 0;
}
EXPORT_SYMBOL_GPL(atomisp_gmin_remove_subdev);

#ifdef CONFIG_VIDEO_CAMERA_PLUG_AND_PLAY
struct gmin_cfg_var {
	const char *name, *val, *drv_name;
};

static const struct gmin_cfg_var ffrd8_vars[] = {
	{ "INTCF1B:00_ImxId",    "0x134", NULL },
	{ "INTCF1B:00_CsiPort",  "1",     NULL },
	{ "INTCF1B:00_CsiLanes", "4",     NULL },
	{ "INTCF1B:00_CamClk",   "0",     NULL },
	{},
};

/* Cribbed from MCG defaults in the mt9m114 driver, not actually verified
 * vs. T100 hardware */
static const struct gmin_cfg_var t100_vars[] = {
	{ "INT33F0:00_CsiPort",  "0", NULL },
	{ "INT33F0:00_CsiLanes", "1", NULL },
	{ "INT33F0:00_CamClk",   "1", NULL },
	{},
};

static const struct gmin_cfg_var cht_camera_config_vars[] = {
	/* For MRD rear camera ov8858 with MRD default BIOS */
	{ "INT3477:00_CamClk",           "4",    "ov8858" },
	{ "INT3477:00_ClkSrc",           "0",    "ov8858" },
	{ "INT3477:00_CsiPort",          "1",    "ov8858" },
	{ "INT3477:00_CsiLanes",         "4",    "ov8858" },

	/* For MRD front camera ov2680 with MRD default BIOS */
	{ "OVTI2680:00_CamClk",          "2",    "ov2680" },
	{ "OVTI2680:00_ClkSrc",          "0",    "ov2680" },
	{ "OVTI2680:00_CsiPort",         "0",    "ov2680" },
	{ "OVTI2680:00_CsiLanes",        "1",    "ov2680" },

	/* For Catalog camera ov5648 with BIOS change */
	{ "INT5648:00_CamClk",          "4",    "ppr_cam" },
	{ "INT5648:00_ClkSrc",          "0",    "ppr_cam" },
	{ "INT5648:00_CsiPort",         "1",    "ppr_cam" },
	{ "INT5648:00_CsiLanes",        "2",    "ppr_cam" },

	/* For Catalog front camera ov2680 with MRD default BIOS */
	{ "OVB2680:00_CamClk",         "4",    "ov2680b" },
	{ "OVB2680:00_ClkSrc",         "0",    "ov2680b" },
	{ "OVB2680:00_CsiPort",        "1",    "ov2680b" },
	{ "OVB2680:00_CsiLanes",       "1",    "ov2680b" },

	{ "OVTI2680:01_CamClk", 		"2",	"ov2680" },
	{ "OVTI2680:01_ClkSrc", 		"0",	"ov2680" },
	{ "OVTI2680:01_CsiPort",		"0",	"ov2680" },
	{ "OVTI2680:01_CsiLanes",		"1",	"ov2680" },

	{},
};
#else
struct gmin_cfg_var {
	const char *name, *val;
};

static const struct gmin_cfg_var ffrd8_vars[] = {
	{ "INTCF1B:00_ImxId",    "0x134" },
	{ "INTCF1B:00_CsiPort",  "1" },
	{ "INTCF1B:00_CsiLanes", "4" },
	{ "INTCF1B:00_CamClk", "0" },
	{},
};

/* Cribbed from MCG defaults in the mt9m114 driver, not actually verified
 * vs. T100 hardware */
static const struct gmin_cfg_var t100_vars[] = {
	{ "INT33F0:00_CsiPort",  "0" },
	{ "INT33F0:00_CsiLanes", "1" },
	{ "INT33F0:00_CamClk",   "1" },
	{},
};
#endif /* CONFIG_VIDEO_CAMERA_PLUG_AND_PLAY */

static const struct {
	const char *dmi_board_name;
	const struct gmin_cfg_var *vars;
} hard_vars[] = {
	//{ "BYT-T FFD8", ffrd8_vars },
	//{ "T100TA", t100_vars },
#ifdef CONFIG_VIDEO_CAMERA_PLUG_AND_PLAY
	{ "CAMERA", cht_camera_config_vars },
#endif /* CONFIG_VIDEO_CAMERA_PLUG_AND_PLAY */
};


#define GMIN_CFG_VAR_EFI_GUID EFI_GUID(0xecb54cd9, 0xe5ae, 0x4fdc, \
				       0xa9, 0x71, 0xe8, 0x77,	   \
				       0x75, 0x60, 0x68, 0xf7)

#define CFG_VAR_NAME_MAX 64

static int gmin_platform_init(struct i2c_client *client)
{
	return 0;
}

static int gmin_platform_deinit(void)
{
	return 0;
}

static int match_i2c_name(struct device *dev, void *name)
{
	return !strcmp(to_i2c_client(dev)->name, (char *)name);
}

static bool i2c_dev_exists(char *name)
{
	return !!bus_find_device(&i2c_bus_type, NULL, name, match_i2c_name);
}

static struct gmin_subdev *gmin_subdev_add(struct v4l2_subdev *subdev)
{
	int i, ret;
	struct device *dev;
        struct i2c_client *client = v4l2_get_subdevdata(subdev);

	if (!pmic_id) {
		if (i2c_dev_exists(DEVNAME_PMIC_AXP))
			pmic_id = PMIC_AXP;
		else if (i2c_dev_exists(DEVNAME_PMIC_TI))
			pmic_id = PMIC_TI;
		else if (i2c_dev_exists(DEVNAME_PMIC_CRYSTALCOVE))
			pmic_id = PMIC_CRYSTALCOVE;
		else
			pmic_id = PMIC_REGULATOR;
	}

	if (!client)
		return NULL;

	dev = client ? &client->dev : NULL;

	for (i=0; i < MAX_SUBDEVS && gmin_subdevs[i].subdev; i++)
		;
	if (i >= MAX_SUBDEVS)
		return NULL;

	dev_info(dev,
		"gmin: initializing atomisp module subdev data.PMIC ID %d\n",
		pmic_id);


	dev_info(dev, "suddev name = %s", subdev->name);

	gmin_subdevs[i].subdev = subdev;

#ifdef CONFIG_VIDEO_CAMERA_PLUG_AND_PLAY
	/* Clk Num:
	 * 4 -> CLK4_PLT_CAM1_19P2MHz(rear)
	 * 2 -> CLK2_PLT_CAM2_19P2MHz(front)
	 */
	gmin_subdevs[i].clock_num        = gmin_get_var_int(dev, "CamClk",
			2, dev->driver->name);
	/* Clk Src:
	 * 0 -> CLK_FREQ_TYPE_XTAL
	 */
	gmin_subdevs[i].clock_src        = gmin_get_var_int(dev, "ClkSrc",
			0, dev->driver->name);
	/* CSI port:
	 * 0 -> front;
	 * 1 -> rear;
	 */
	gmin_subdevs[i].csi_port         = gmin_get_var_int(dev, "CsiPort",
			1, dev->driver->name);
	/* CSI Lanes: lane's number */
	gmin_subdevs[i].csi_lanes        = gmin_get_var_int(dev, "CsiLanes",
			2, dev->driver->name);
#else
	if (!strcmp(dev->driver->acpi_match_table->id,
				"INT3477")) {
		gmin_subdevs[i].clock_num = 4;
		gmin_subdevs[i].clock_src = 0;
		gmin_subdevs[i].csi_port  = 1;
		gmin_subdevs[i].csi_lanes = 4;
	} else if (!strcmp(dev->driver->acpi_match_table->id,
				"OVB2680")) {
		gmin_subdevs[i].clock_num = 4;
		gmin_subdevs[i].clock_src = 0;
		gmin_subdevs[i].csi_port  = 1;
		gmin_subdevs[i].csi_lanes = 1;
	} else if (!strcmp(dev->driver->acpi_match_table->id,
				"INT5648")) {
		gmin_subdevs[i].clock_num = 4;
		gmin_subdevs[i].clock_src = 0;
		gmin_subdevs[i].csi_port  = 1;
		gmin_subdevs[i].csi_lanes = 2;
	} else {
		gmin_subdevs[i].clock_num = 2;
		gmin_subdevs[i].clock_src = 0;
		gmin_subdevs[i].csi_port  = 0;
		gmin_subdevs[i].csi_lanes = 1;
	}
#endif

	gmin_subdevs[i].gpio0            = gpiod_get_index(dev,
			"cam_gpio0", 0);
	gmin_subdevs[i].gpio1            = gpiod_get_index(dev,
			"cam_gpio1", 1);

	/* DVDD of front camera  */
	gmin_subdevs[i].eldo1_1p8v       = ELDO1_1P6V;
	gmin_subdevs[i].eldo1_sel_reg    = ELDO1_SEL_REG;
	gmin_subdevs[i].eldo1_ctrl_shift = ELDO1_CTRL_SHIFT;

	/* DOVDD of both cameras */
	gmin_subdevs[i].eldo2_1p8v       = ELDO2_1P8V;
	gmin_subdevs[i].eldo2_sel_reg    = ELDO2_SEL_REG;
	gmin_subdevs[i].eldo2_ctrl_shift = ELDO2_CTRL_SHIFT;

#if 0
	/* AVDD of both cameras */
	gmin_subdevs[i].aldo1_2p8v       = ALDO1_2P8V;
	gmin_subdevs[i].aldo1_sel_reg    = ALDO1_SEL_REG;
	gmin_subdevs[i].aldo1_ctrl_shift = ALDO1_CTRL_SHIFT;
#endif

	/* DVDD of rear camera */
	gmin_subdevs[i].fldo2_1p2v       = FLDO2_1P2V;
	gmin_subdevs[i].fldo2_sel_reg    = FLDO2_SEL_REG;
	gmin_subdevs[i].fldo2_ctrl_shift = FLDO2_CTRL_SHIFT;

	if (!IS_ERR(gmin_subdevs[i].gpio0)) {
		dev_err(dev, "gmin_subdev_add gpio0 is ok.\n");
		ret = gpiod_direction_output(gmin_subdevs[i].gpio0, 0);
		if (ret)
			dev_err(dev, "gpio0 set output failed: %d\n", ret);
	} else {
		dev_err(dev, "gmin_subdev_add gpio0 is NULL.\n");
		gmin_subdevs[i].gpio0 = NULL;
	}

	if (!IS_ERR(gmin_subdevs[i].gpio1)) {
		dev_err(dev, "gmin_subdev_add gpio1 is ok.\n");
		ret = gpiod_direction_output(gmin_subdevs[i].gpio1, 0);
		if (ret)
			dev_err(dev, "gpio1 set output failed: %d\n", ret);
	} else {
		gmin_subdevs[i].gpio1 = NULL;
		dev_err(dev, "gmin_subdev_add gpio1 is NULL.\n");
	}

	if (pmic_id == PMIC_REGULATOR) {
		gmin_subdevs[i].v1p8_reg = regulator_get(dev, "V1P8SX");
		gmin_subdevs[i].v2p8_reg = regulator_get(dev, "V2P8SX");

		/* Note: ideally we would initialize v[12]p8_on to the
		 * output of regulator_is_enabled(), but sadly that
		 * API is broken with the current drivers, returning
		 * "1" for a regulator that will then emit a
		 * "unbalanced disable" WARNing if we try to disable
		 * it. */
	}

	return &gmin_subdevs[i];
}

static struct gmin_subdev *find_gmin_subdev(struct v4l2_subdev *subdev)
{
	int i;

	for (i=0; i < MAX_SUBDEVS; i++)
		if (gmin_subdevs[i].subdev == subdev)
			return &gmin_subdevs[i];

	return gmin_subdev_add(subdev);
}

static int gmin_gpio0_ctrl(struct v4l2_subdev *subdev, int on)
{
	struct gmin_subdev *gs = find_gmin_subdev(subdev);

	pr_err("gmin_gpio0_ctrl.\n");
	if (gs && gs->gpio0) {
		pr_err("gmin_gpio0_ctrl enter.\n");
		gpiod_set_value(gs->gpio0, on);
		return 0;
	}
	return -EINVAL;
}

static int gmin_gpio1_ctrl(struct v4l2_subdev *subdev, int on)
{
	struct gmin_subdev *gs = find_gmin_subdev(subdev);

	pr_err("gmin_gpio1_ctrl.\n");
	if (gs && gs->gpio1) {
		pr_err("gmin_gpio1_ctrl enter.\n");
		gpiod_set_value(gs->gpio1, on);
		return 0;
	}
	return -EINVAL;
}

static int axp_regulator_set(int sel_reg, u8 setting, int ctrl_reg, int shift, bool on)
{
	int ret;
	int val;

	ret = intel_soc_pmic_writeb(sel_reg, setting);
	if (ret)
		return ret;
	val = intel_soc_pmic_readb(ctrl_reg);
	if (val < 0)
		return val;

	if (on)
		val |= (1 << shift);
	else
		val &= ~(1 << shift);

	ret = intel_soc_pmic_writeb(ctrl_reg, (u8)val);
	if (ret)
		return ret;

	pr_err("axp_regulator_set success.\n");
	return 0;
}

static int axp_v1p8_on(struct gmin_subdev *gs)
{
	int ret;
	struct i2c_client *client = v4l2_get_subdevdata(gs->subdev);
	struct device *dev = client ? &client->dev : NULL;

	if (!dev) {
		pr_err("%s(): client and dev are NULL!\n",
				__func__);
		ret = -ENODEV;
		return ret;
	}

	/* DOVDD for both camera*/
	ret = axp_regulator_set(gs->eldo2_sel_reg,
			gs->eldo2_1p8v,
			ELDO_CTRL_REG,
			gs->eldo2_ctrl_shift,
			true);

	/* This sleep comes out of the gc2235 driver, which is the
	 * only one I currently see that wants to set both 1.8v rails. */
	usleep_range(110, 150);

	/* DVDD of rear camera */
	if (!strcmp(dev->driver->acpi_match_table->id,
				"OVB2680")) {
	#ifdef CONFIG_VIDEO_CAMERA_PLUG_AND_PLAY
		if (!strcmp(dev->driver->name, "ov8858")) {
	#endif
			ret |= axp_regulator_set(gs->fldo2_sel_reg,
					gs->fldo2_1p2v,
					FLDO_CTRL_REG,
					gs->fldo2_ctrl_shift,
					true);
	#ifdef CONFIG_VIDEO_CAMERA_PLUG_AND_PLAY
		}
	#endif
	}

	/* DVDD of front camera */
	if (!strcmp(dev->driver->acpi_match_table->id,
				"OVTI2680") ||
			!strcmp(dev->driver->acpi_match_table->id,
				"GCTI2355"))
		ret |= axp_regulator_set(gs->eldo1_sel_reg,
				gs->eldo1_1p8v,
				ELDO_CTRL_REG,
				gs->eldo1_ctrl_shift,
				true);

	return ret;
}

static int axp_v1p8_off(struct gmin_subdev *gs)
{
	int ret;
	struct i2c_client *client = v4l2_get_subdevdata(gs->subdev);
	struct device *dev = client ? &client->dev : NULL;

	if (!dev) {
		pr_err("%s(): client and dev are NULL!\n",
				__func__);
		ret = -ENODEV;
		return ret;
	}

	/* DOVDD for both camera*/
	ret = axp_regulator_set(gs->eldo2_sel_reg,
			gs->eldo2_1p8v,
			ELDO_CTRL_REG,
			gs->eldo2_ctrl_shift,
			false);

	/* DVDD of front camera */
	if (!strcmp(dev->driver->acpi_match_table->id,
				"OVTI2680") ||
			!strcmp(dev->driver->acpi_match_table->id,
				"GCTI2355"))
		ret |= axp_regulator_set(gs->eldo1_sel_reg,
				gs->eldo1_1p8v,
				ELDO_CTRL_REG,
				gs->eldo1_ctrl_shift,
				false);

	/* DVDD of rear camera */
	if (!strcmp(dev->driver->acpi_match_table->id,
				"OVB2680")) {
	#ifdef CONFIG_VIDEO_CAMERA_PLUG_AND_PLAY
		if (!strcmp(dev->driver->name, "ov8858")) {
	#endif
			ret |= axp_regulator_set(gs->fldo2_sel_reg,
					gs->fldo2_1p2v,
					FLDO_CTRL_REG,
					gs->fldo2_ctrl_shift,
					true);
	#ifdef CONFIG_VIDEO_CAMERA_PLUG_AND_PLAY
		}
	#endif
	}

	return ret;
}


static int axp_v2p8_on(void)
{
	int ret;
	/* AVDD */
	ret = axp_regulator_set(ALDO1_SEL_REG, ALDO1_2P8V, ALDO1_CTRL3_REG,
		ALDO1_CTRL3_SHIFT, true);
	return ret;
}

static int axp_v2p8_off(void)
{
	/* AVDD */
	return axp_regulator_set(ALDO1_SEL_REG, ALDO1_2P8V, ALDO1_CTRL3_REG,
				 ALDO1_CTRL3_SHIFT, false);
}

int gmin_v1p8_ctrl(struct v4l2_subdev *subdev, int on)
{
	struct gmin_subdev *gs = find_gmin_subdev(subdev);
	int ret;

	if (v1p8_gpio == V1P8_GPIO_UNSET) {
	#ifdef CONFIG_VIDEO_CAMERA_PLUG_AND_PLAY
		v1p8_gpio = gmin_get_var_int(NULL,
				"V1P8GPIO",
				V1P8_GPIO_NONE,
				NULL);
	#else
		v1p8_gpio = gmin_get_var_int(NULL,
				"V1P8GPIO",
				V1P8_GPIO_NONE);
	#endif /* CONFIG_VIDEO_CAMERA_PLUG_AND_PLAY */
		if (v1p8_gpio != V1P8_GPIO_NONE) {
			pr_info("atomisp_gmin_platform: 1.8v power on GPIO %d\n",
				v1p8_gpio);
			ret = devm_gpio_request(gs->subdev->dev,
					v1p8_gpio,
					"camera_v1p8_en");
			if (!ret)
				ret = gpio_direction_output(v1p8_gpio, 0);
			if (ret)
				pr_err("V1P8 GPIO initialization failed\n");
		}
	}

	if (gs && gs->v1p8_on == on)
		return 0;
	gs->v1p8_on = on;

	if (v1p8_gpio >= 0)
		gpio_set_value(v1p8_gpio, on);

	if (gs->v1p8_reg) {
		if (on)
			return regulator_enable(gs->v1p8_reg);
		else
			return regulator_disable(gs->v1p8_reg);
	}

	if (pmic_id == PMIC_AXP) {
		pr_err("gmin_v1p8_ctrl PMIC_AXP.\n");
		if (on)
			return axp_v1p8_on(gs);
		else
			return axp_v1p8_off(gs);
	}

	if (pmic_id == PMIC_TI) {
		int val = on ? LDO_1P8V_ON : LDO_1P8V_OFF;
		int ret = intel_soc_pmic_writeb(LDO10_REG, val);
		if (!ret)
			ret = intel_soc_pmic_writeb(LDO11_REG, val);
		return ret;
	}

	if (pmic_id == PMIC_CRYSTALCOVE) {
		if (on)
			return intel_soc_pmic_writeb(CRYSTAL_1P8V_REG,
								CRYSTAL_ON);
		else
			return intel_soc_pmic_writeb(CRYSTAL_1P8V_REG,
								CRYSTAL_OFF);
	}

	pr_err("gmin_v1p8_ctrl failed.\n");
	return -EINVAL;
}

int gmin_v2p8_ctrl(struct v4l2_subdev *subdev, int on)
{
	struct gmin_subdev *gs = find_gmin_subdev(subdev);
	int ret;

	if (v2p8_gpio == V2P8_GPIO_UNSET) {
	#ifdef CONFIG_VIDEO_CAMERA_PLUG_AND_PLAY
		v2p8_gpio = gmin_get_var_int(NULL,
				"V2P8GPIO",
				V2P8_GPIO_NONE,
				NULL);
	#else
		v2p8_gpio = gmin_get_var_int(NULL,
				"V2P8GPIO",
				V2P8_GPIO_NONE);
	#endif /* CONFIG_VIDEO_CAMERA_PLUG_AND_PLAY */
		if (v2p8_gpio != V2P8_GPIO_NONE) {
			pr_info("atomisp_gmin_platform: 2.8v power on GPIO %d\n",
				v2p8_gpio);
			ret = devm_gpio_request(gs->subdev->dev,
					v2p8_gpio,
					"camera_v2p8");
			if (!ret)
				ret = gpio_direction_output(v2p8_gpio, 0);
			if (ret)
				pr_err("V2P8 GPIO initialization failed\n");
		}
	}

	if (gs && gs->v2p8_on == on)
		return 0;
	gs->v2p8_on = on;

	if (v2p8_gpio >= 0)
		gpio_set_value(v2p8_gpio, on);

	if (gs->v2p8_reg) {
		if (on)
			return regulator_enable(gs->v2p8_reg);
		else
			return regulator_disable(gs->v2p8_reg);
	}

	if (pmic_id == PMIC_AXP) {
		if (on)
			return axp_v2p8_on();
		else
			return axp_v2p8_off();
	}

	if (pmic_id == PMIC_TI) {
		if (on)
			return intel_soc_pmic_writeb(LDO9_REG, LDO_2P8V_ON);
		else
			return intel_soc_pmic_writeb(LDO9_REG, LDO_2P8V_OFF);
	}

	if (pmic_id == PMIC_CRYSTALCOVE) {
		if (on)
			return intel_soc_pmic_writeb(CRYSTAL_2P8V_REG, CRYSTAL_ON);
		else
			return intel_soc_pmic_writeb(CRYSTAL_2P8V_REG, CRYSTAL_OFF);
	}

	pr_err("gmin_v2p8_ctrl failed.\n");
	return -EINVAL;
}

int gmin_flisclk_ctrl(struct v4l2_subdev *subdev, int on)
{
	int ret = 0;
	struct gmin_subdev *gs = find_gmin_subdev(subdev);

	if (on)
		ret = vlv2_plat_set_clock_freq(gs->clock_num, gs->clock_src);
	if (ret)
		return ret;

	return vlv2_plat_configure_clock(gs->clock_num,
					 on ? VLV2_CLK_ON : VLV2_CLK_OFF);
}

static int gmin_csi_cfg(struct v4l2_subdev *sd, int flag)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct gmin_subdev *gs = find_gmin_subdev(sd);

	if (!client || !gs)
		return -ENODEV;

	return camera_sensor_csi(sd, gs->csi_port, gs->csi_lanes,
				 gs->csi_fmt, gs->csi_bayer, flag);
}

static struct camera_vcm_control *gmin_get_vcm_ctrl(struct v4l2_subdev *subdev,
						char *camera_module)
{
	struct i2c_client *client = v4l2_get_subdevdata(subdev);
	struct gmin_subdev *gs = find_gmin_subdev(subdev);
	struct camera_vcm_control *vcm;

	if (client == NULL || gs == NULL)
		return NULL;

	if (!camera_module)
		return NULL;

	mutex_lock(&vcm_lock);
	list_for_each_entry(vcm, &vcm_devices, list) {
		if (!strcmp(camera_module, vcm->camera_module)) {
			mutex_unlock(&vcm_lock);
			return vcm;
		}
	}

	mutex_unlock(&vcm_lock);
	return NULL;
}

static struct camera_sensor_platform_data gmin_plat = {
	.gpio0_ctrl = gmin_gpio0_ctrl,
	.gpio1_ctrl = gmin_gpio1_ctrl,
	.v1p8_ctrl = gmin_v1p8_ctrl,
	.v2p8_ctrl = gmin_v2p8_ctrl,
	.flisclk_ctrl = gmin_flisclk_ctrl,
	.platform_init = gmin_platform_init,
	.platform_deinit = gmin_platform_deinit,
	.csi_cfg = gmin_csi_cfg,
	.get_vcm_ctrl = gmin_get_vcm_ctrl,
};

struct camera_sensor_platform_data *gmin_camera_platform_data(
		struct v4l2_subdev *subdev,
		enum atomisp_input_format csi_format,
		enum atomisp_bayer_order csi_bayer)
{
	struct gmin_subdev *gs = find_gmin_subdev(subdev);

	gs->csi_fmt   = csi_format;
	gs->csi_bayer = csi_bayer;

	return &gmin_plat;
}
EXPORT_SYMBOL_GPL(gmin_camera_platform_data);

int atomisp_gmin_register_vcm_control(struct camera_vcm_control *vcmCtrl)
{
	if (!vcmCtrl)
		return -EINVAL;

	mutex_lock(&vcm_lock);
	list_add_tail(&vcmCtrl->list, &vcm_devices);
	mutex_unlock(&vcm_lock);

	return 0;
}
EXPORT_SYMBOL_GPL(atomisp_gmin_register_vcm_control);

/* Retrieves a device-specific configuration variable.  The dev
 * argument should be a device with an ACPI companion, as all
 * configuration is based on firmware ID. */
#ifdef CONFIG_VIDEO_CAMERA_PLUG_AND_PLAY
int gmin_get_config_var(struct device *dev, const char *var,
		char *out, size_t *out_len, const char *drv_name)
#else
int gmin_get_config_var(struct device *dev, const char *var,
		char *out, size_t *out_len)
#endif /* CONFIG_VIDEO_CAMERA_PLUG_AND_PLAY */
{
	char var8[CFG_VAR_NAME_MAX];
	efi_char16_t var16[CFG_VAR_NAME_MAX];
	struct efivar_entry *ev;
	u32 efiattr_dummy;
	int i, j, ret;
	unsigned long efilen;

	if (dev && ACPI_COMPANION(dev))
		dev = &ACPI_COMPANION(dev)->dev;

	if (dev)
		ret = snprintf(var8, sizeof(var8), "%s_%s", dev_name(dev), var);
	else
		ret = snprintf(var8, sizeof(var8), "gmin_%s", var);

	if (ret < 0 || ret >= sizeof(var8)-1)
		return -EINVAL;

	/* First check a hard-coded list of board-specific variables.
	 * Some device firmwares lack the ability to set EFI variables at
	 * runtime. */
	for (i = 0; i < ARRAY_SIZE(hard_vars); i++) {
	#ifdef CONFIG_VIDEO_CAMERA_PLUG_AND_PLAY
		if (!strcmp(hard_vars[i].dmi_board_name, "CAMERA")) {
	#else
		if (dmi_match(DMI_BOARD_NAME, hard_vars[i].dmi_board_name)) {
	#endif /* CONFIG_VIDEO_CAMERA_PLUG_AND_PLAY */
			for (j = 0; hard_vars[i].vars[j].name; j++) {
				size_t vl;
				const struct gmin_cfg_var *gv;

			#ifdef CONFIG_VIDEO_CAMERA_PLUG_AND_PLAY
				if (drv_name && hard_vars[i].vars[j].drv_name) {
					if (strcmp(drv_name, hard_vars[i].vars[j].drv_name))
						continue;
				}
			#endif /* CONFIG_VIDEO_CAMERA_PLUG_AND_PLAY */

				gv = &hard_vars[i].vars[j];
				vl = strlen(gv->val);

				if (strcmp(var8, gv->name))
					continue;
				if (vl > *out_len-1)
					return -ENOSPC;

				memcpy(out, gv->val, min(*out_len, vl+1));
				out[*out_len-1] = 0;
				*out_len = vl;

				return 0;
			}
		}
	}

	/* Our variable names are ASCII by construction, but EFI names
	 * are wide chars.  Convert and zero-pad. */
	memset(var16, 0, sizeof(var16));
	for (i=0; var8[i] && i < sizeof(var8); i++)
		var16[i] = var8[i];

	/* To avoid owerflows when calling the efivar API */
	if (*out_len > ULONG_MAX)
		return -EINVAL;

	/* Not sure this API usage is kosher; efivar_entry_get()'s
	 * implementation simply uses VariableName and VendorGuid from
	 * the struct and ignores the rest, but it seems like there
	 * ought to be an "official" efivar_entry registered
	 * somewhere? */
	ev = kzalloc(sizeof(*ev), GFP_KERNEL);
	if (!ev)
		return -ENOMEM;
	memcpy(&ev->var.VariableName, var16, sizeof(var16));
	ev->var.VendorGuid = GMIN_CFG_VAR_EFI_GUID;

	efilen = *out_len;
	ret = efivar_entry_get(ev, &efiattr_dummy, &efilen, out);

	kfree(ev);
	*out_len = efilen;

	if (ret)
 		dev_warn(dev, "Failed to find gmin variable %s\n", var8);

	return ret;
}
EXPORT_SYMBOL_GPL(gmin_get_config_var);

#ifdef CONFIG_VIDEO_CAMERA_PLUG_AND_PLAY
int gmin_get_var_int(struct device *dev, const char *var,
		int def, const char *drv_name)
#else
int gmin_get_var_int(struct device *dev, const char *var,
		int def)
#endif /* CONFIG_VIDEO_CAMERA_PLUG_AND_PLAY */
{
	char val[CFG_VAR_NAME_MAX];
	size_t len = sizeof(val);
	long result;
	int ret;

#ifdef CONFIG_VIDEO_CAMERA_PLUG_AND_PLAY
	ret = gmin_get_config_var(dev, var, val, &len, drv_name);
#else
	ret = gmin_get_config_var(dev, var, val, &len);
#endif /* CONFIG_VIDEO_CAMERA_PLUG_AND_PLAY */
	if (!ret) {
		val[len] = 0;
		ret = kstrtol(val, 0, &result);
	}

	pr_err("gmin_get_var_int var: %s, ret: %x.\n",
			var,
			ret ? def :
			(unsigned int)result);

	return ret ? def : result;
}
EXPORT_SYMBOL_GPL(gmin_get_var_int);

int camera_sensor_csi(struct v4l2_subdev *sd, u32 port,
		      u32 lanes, u32 format, u32 bayer_order, int flag)
{
        struct i2c_client *client = v4l2_get_subdevdata(sd);
        struct camera_mipi_info *csi = NULL;

        if (flag) {
                csi = kzalloc(sizeof(*csi), GFP_KERNEL);
                if (!csi) {
                        dev_err(&client->dev, "out of memory\n");
                        return -ENOMEM;
                }
                csi->port = port;
                csi->num_lanes = lanes;
                csi->input_format = format;
                csi->raw_bayer_order = bayer_order;
                v4l2_set_subdev_hostdata(sd, (void *)csi);
                csi->metadata_format = ATOMISP_INPUT_FORMAT_EMBEDDED;
                csi->metadata_effective_width = NULL;
                dev_info(&client->dev,
                         "camera pdata: port: %d lanes: %d order: %8.8x\n",
                         port, lanes, bayer_order);
        } else {
                csi = v4l2_get_subdev_hostdata(sd);
                kfree(csi);
        }

        return 0;
}
EXPORT_SYMBOL_GPL(camera_sensor_csi);

#ifdef CONFIG_GMIN_INTEL_MID
/* PCI quirk: The BYT ISP advertises PCI runtime PM but it doesn't
 * work.  Disable so the kernel framework doesn't hang the device
 * trying.  The driver itself does direct calls to the PUNIT to manage
 * ISP power. */
static void isp_pm_cap_fixup(struct pci_dev *dev)
{
	dev_info(&dev->dev, "Disabling PCI power management on camera ISP\n");
	dev->pm_cap = 0;
}
DECLARE_PCI_FIXUP_FINAL(PCI_VENDOR_ID_INTEL, 0x0f38, isp_pm_cap_fixup);
#endif

#ifdef CONFIG_VIDEO_CAMERA_PLUG_AND_PLAY
/*
 * Here we unregister the devices registered by ACPI
 *
 * Call before insmod driver.ko;
 */
 #if 0
int __init atomisp_unregister_acpi_devices(void)
{
	int i;
	struct device *dev;
	struct i2c_client *client;

	/* Why should we unregister both rear/front camera at the same time? */
	for (i = 0; i < ARRAY_SIZE(cht_cam_comp_tab); i++) {
		dev = bus_find_device_by_name(&i2c_bus_type, NULL,
					      cht_cam_comp_tab[i].acpi_match_name);
		if (dev) {
			client              = to_i2c_client(dev);
			board_info[i].flags = client->flags;
			board_info[i].addr  = client->addr;
			board_info[i].irq   = client->irq;

			strlcpy(board_info[i].type, client->name,
				sizeof(client->name));

			pr_info("%s(%d): unregister i2c_device: %s! \
					\nclient->flags is 0x%.2X,\
					client->addr is 0x%.2X, \
					client->irq is %d.\n",
					__func__,
					__LINE__,
					client->name,
					client->flags,
					client->addr,
					client->irq);
			cht_cam_comp_tab[i].i2c_addr=client->addr;
			i2c_unregister_device(client);
		}
	}

	return 0;
}
device_initcall(atomisp_unregister_acpi_devices);
#endif
#endif /* CONFIG_VIDEO_CAMERA_PLUG_AND_PLAY */
