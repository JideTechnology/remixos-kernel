
// auto generate : Fri Sep 26 15:49:18 CST 2014
S_UNI_REG  gc2355_stream_on[] = {
	{UNICAM_8BIT, 0xfe, 0x03},	//Turn on rolling shutter
	{UNICAM_8BIT, 0x10, 0x90},
	{UNICAM_8BIT, 0xfe, 0x00},
	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG  gc2355_stream_off[] = {
	{UNICAM_8BIT, 0xfe, 0x03},	//Turn off rolling shutter
	{UNICAM_8BIT, 0x10, 0x80},
	{UNICAM_8BIT, 0xfe, 0x00},
	{UNICAM_TOK_TERM, 0, 0}
};

S_UNI_REG  gc2355_global_setting[] = {

	{ UNICAM_8BIT, 0xfe, 0x80 },
	{ UNICAM_8BIT, 0xfe, 0x80 },
	{ UNICAM_8BIT, 0xfe, 0x80 },
	{ UNICAM_8BIT, 0xf2, 0x00 },
	{ UNICAM_8BIT, 0xf6, 0x00 },
	{ UNICAM_8BIT, 0xf7, 0x19 },
	{ UNICAM_8BIT, 0xf8, 0x08 },//06
	{ UNICAM_8BIT, 0xf9, 0x0e },
	{ UNICAM_8BIT, 0xfa, 0x00 },
	{ UNICAM_8BIT, 0xfc, 0x06 },
	{ UNICAM_8BIT, 0xfe, 0x00 },

	{ UNICAM_8BIT, 0x03, 0x07 },
	{ UNICAM_8BIT, 0x04, 0xd0 },
	{ UNICAM_8BIT, 0x05, 0x01 },//03
	{ UNICAM_8BIT, 0x06, 0x22 },//4c
	{ UNICAM_8BIT, 0x07, 0x00 },
	{ UNICAM_8BIT, 0x08, 0x0b },//12
	{ UNICAM_8BIT, 0x0a, 0x00 },
	{ UNICAM_8BIT, 0x0c, 0x04 },
	{ UNICAM_8BIT, 0x0d, 0x04 },
	{ UNICAM_8BIT, 0x0e, 0xc0 },
	{ UNICAM_8BIT, 0x0f, 0x06 },
	{ UNICAM_8BIT, 0x10, 0x50 },
	{ UNICAM_8BIT, 0x17, 0x14 },
	{ UNICAM_8BIT, 0x19, 0x0b },
	{ UNICAM_8BIT, 0x1b, 0x48 },
	{ UNICAM_8BIT, 0x1c, 0x12 },
	{ UNICAM_8BIT, 0x1d, 0x10 },
	{ UNICAM_8BIT, 0x1e, 0xbc },
	{ UNICAM_8BIT, 0x1f, 0xc9 },
	{ UNICAM_8BIT, 0x20, 0x71 },
	{ UNICAM_8BIT, 0x21, 0x20 },
	{ UNICAM_8BIT, 0x22, 0xa0 },
	{ UNICAM_8BIT, 0x23, 0x51 },
	{ UNICAM_8BIT, 0x24, 0x19 },
	{ UNICAM_8BIT, 0x27, 0x20 },
	{ UNICAM_8BIT, 0x28, 0x00 },
	{ UNICAM_8BIT, 0x2b, 0x80 },
	{ UNICAM_8BIT, 0x2c, 0x38 },
	{ UNICAM_8BIT, 0x2e, 0x16 },
	{ UNICAM_8BIT, 0x2f, 0x14 },
	{ UNICAM_8BIT, 0x30, 0x00 },
	{ UNICAM_8BIT, 0x31, 0x01 },
	{ UNICAM_8BIT, 0x32, 0x02 },
	{ UNICAM_8BIT, 0x33, 0x03 },
	{ UNICAM_8BIT, 0x34, 0x07 },
	{ UNICAM_8BIT, 0x35, 0x0b },
	{ UNICAM_8BIT, 0x36, 0x0f },

	{ UNICAM_8BIT, 0xfe, 0x03 },
	{ UNICAM_8BIT, 0x10, 0x80 },
	{ UNICAM_8BIT, 0x01, 0x83 },
	{ UNICAM_8BIT, 0x22, 0x05 },
	{ UNICAM_8BIT, 0x23, 0x30 },
	{ UNICAM_8BIT, 0x25, 0x15 },
	{ UNICAM_8BIT, 0x29, 0x06 },
	{ UNICAM_8BIT, 0x02, 0x00 },
	{ UNICAM_8BIT, 0x03, 0x90 },
	{ UNICAM_8BIT, 0x04, 0x01 },
	{ UNICAM_8BIT, 0x05, 0x00 },
	{ UNICAM_8BIT, 0x06, 0xa2 },
	{ UNICAM_8BIT, 0x11, 0x2b },
	{ UNICAM_8BIT, 0x12, 0xe4 },
	{ UNICAM_8BIT, 0x13, 0x07 },
	{ UNICAM_8BIT, 0x15, 0x60 },

	{ UNICAM_8BIT, 0x21, 0x10},
	{ UNICAM_8BIT, 0x24, 0x02 },
	{ UNICAM_8BIT, 0x26, 0x08 },
	{ UNICAM_8BIT, 0x27, 0x06 },
	{ UNICAM_8BIT, 0x2a, 0x0a },
	{ UNICAM_8BIT, 0x2b, 0x08 },

	{ UNICAM_8BIT, 0x40, 0x00 },
	{ UNICAM_8BIT, 0x41, 0x00 },
	{ UNICAM_8BIT, 0x42, 0x40 },
	{ UNICAM_8BIT, 0x43, 0x06 },
	{ UNICAM_8BIT, 0xfe, 0x00 },

	{ UNICAM_8BIT, 0xb0, 0x50 },
	{ UNICAM_8BIT, 0xb1, 0x01 },
	{ UNICAM_8BIT, 0xb2, 0x00 },
	{ UNICAM_8BIT, 0xb3, 0x40 },
	{ UNICAM_8BIT, 0xb4, 0x40 },
	{ UNICAM_8BIT, 0xb5, 0x40 },
	{ UNICAM_8BIT, 0xb6, 0x00 },

	{ UNICAM_8BIT, 0x92, 0x02 },
	{ UNICAM_8BIT, 0x94, 0x00 },
	{ UNICAM_8BIT, 0x95, 0x04 },
	{ UNICAM_8BIT, 0x96, 0xc0 },
	{ UNICAM_8BIT, 0x97, 0x06 },
	{ UNICAM_8BIT, 0x98, 0x50 },

	{ UNICAM_8BIT, 0x18, 0x02 },
	{ UNICAM_8BIT, 0x1a, 0x01 },
	{ UNICAM_8BIT, 0x40, 0x42 },
	{ UNICAM_8BIT, 0x41, 0x00 },
	{ UNICAM_8BIT, 0x44, 0x00 },
	{ UNICAM_8BIT, 0x45, 0x00 },
	{ UNICAM_8BIT, 0x46, 0x00 },
	{ UNICAM_8BIT, 0x47, 0x00 },
	{ UNICAM_8BIT, 0x48, 0x00 },
	{ UNICAM_8BIT, 0x49, 0x00 },
	{ UNICAM_8BIT, 0x4a, 0x00 },
	{ UNICAM_8BIT, 0x4b, 0x00 },
	{ UNICAM_8BIT, 0x4e, 0x3c },
	{ UNICAM_8BIT, 0x4f, 0x00 },
	{ UNICAM_8BIT, 0x5e, 0x00 },
	{ UNICAM_8BIT, 0x66, 0x20 },
	{ UNICAM_8BIT, 0x6a, 0x02 },
	{ UNICAM_8BIT, 0x6b, 0x02 },
	{ UNICAM_8BIT, 0x6c, 0x02 },
	{ UNICAM_8BIT, 0x6d, 0x02 },
	{ UNICAM_8BIT, 0x6e, 0x02 },
	{ UNICAM_8BIT, 0x6f, 0x02 },
	{ UNICAM_8BIT, 0x70, 0x02 },
	{ UNICAM_8BIT, 0x71, 0x02 },

	{ UNICAM_8BIT, 0x87, 0x03 },
	{ UNICAM_8BIT, 0xe0, 0xe7 },
	{ UNICAM_8BIT, 0xe3, 0xc0 },
    {UNICAM_TOK_TERM, 0, 0},
};
S_UNI_REG  gc2355_group_hold_start[] = {
	{UNICAM_8BIT, 0xfe, 0x00},
	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG  gc2355_group_hold_end[] = {
	{UNICAM_8BIT, 0xfe, 0x00},
	{UNICAM_TOK_TERM, 0, 0}
};

static  int gc2355_adgain_p[] = {
	0,
};

/*
static  int gc2355_adgain_p[] = {
	0,64,
	1,88,
	2,122,
	3,168,
	4,239,
	5,330,
	6,470,
};
*/

S_UNI_REG gc2355_720p_30fps[] = {
	{UNICAM_8BIT, 0x90, 0x01},
	{UNICAM_8BIT, 0x92, 0xf0},
	{UNICAM_8BIT, 0x94, 0xa0},
	{UNICAM_8BIT, 0x95, 0x02},
	{UNICAM_8BIT, 0x96, 0xe0},
	{UNICAM_8BIT, 0x97, 0x05},
	{UNICAM_8BIT, 0x98, 0x10},

	{UNICAM_8BIT, 0xfe, 0x03},
	{UNICAM_8BIT, 0x12, 0x54},
	{UNICAM_8BIT, 0x13, 0x06},
	{UNICAM_8BIT, 0x04, 0x20},
	{UNICAM_8BIT, 0x05, 0x00},
	{UNICAM_8BIT, 0xfe, 0x00},
	{UNICAM_TOK_TERM, 0, 0},

	
};

S_UNI_REG gc2355_still_1600x1200_30fps[] = {
	{UNICAM_8BIT, 0xfe, 0x00},
	{UNICAM_8BIT, 0x0a, 0x02},
	{UNICAM_8BIT, 0x0c, 0x00},
	{UNICAM_8BIT, 0x0d, 0x04},
	{UNICAM_8BIT, 0x0e, 0xd0},
	{UNICAM_8BIT, 0x0f, 0x06},
	{UNICAM_8BIT, 0x10, 0x58},

	{UNICAM_8BIT, 0x90, 0x01},
	{UNICAM_8BIT, 0x92, 0x01},
	{UNICAM_8BIT, 0x94, 0x00},
	{UNICAM_8BIT, 0x95, 0x04},
	{UNICAM_8BIT, 0x96, 0xc0},
	{UNICAM_8BIT, 0x97, 0x06},
	{UNICAM_8BIT, 0x98, 0x50},

	{UNICAM_8BIT, 0xfe, 0x03},
	{UNICAM_8BIT, 0x12, 0xe4},
	{UNICAM_8BIT, 0x13, 0x07},
	{UNICAM_8BIT, 0x04, 0x01},
	{UNICAM_8BIT, 0x05, 0x00},
	{UNICAM_8BIT, 0xfe, 0x00},
	{UNICAM_TOK_TERM, 0, 0},
};

#define GC2355_ANALOG_GAIN_1                   64  // 1.00x
#define GC2355_ANALOG_GAIN_2                   88  // 1.375x
#define GC2355_ANALOG_GAIN_3                   122  // 1.90x
#define GC2355_ANALOG_GAIN_4                   168  // 2.625x
#define GC2355_ANALOG_GAIN_5                   239  // 3.738x
#define GC2355_ANALOG_GAIN_6                   330  // 5.163x
#define GC2355_ANALOG_GAIN_7                   470  // 7.350x


static S_UNI_RESOLUTION gc2355_res_preview[] = {
	/*{
		.desc = "gc2355_720p",
		.res_type = UNI_DEV_RES_PREVIEW | UNI_DEV_RES_VIDEO | UNI_DEV_RES_STILL,
		.width = 1296,
		.height = 736,
		.fps = 30,
		.pixels_per_line = 1913,
		.lines_per_frame = 786,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 5,
		.regs = gc2355_720p_30fps,
		.regs_n= (ARRAY_SIZE(gc2355_720p_30fps)),

		.vt_pix_clk_freq_mhz=86400000,
		.crop_horizontal_start=0,
		.crop_vertical_start=0,
		.crop_horizontal_end=1295,
		.crop_vertical_end=735,
		.output_width=1296,
		.output_height=736,
		.vts_fix=8,
		.vts=786,
	},*/
	{
		.desc = "gc2355_1200p",
		.res_type = UNI_DEV_RES_PREVIEW | UNI_DEV_RES_VIDEO | UNI_DEV_RES_STILL,
		.width = 1616,
		.height = 1216,
		.fps = 30, 
		.pixels_per_line = 2190,   
 		.lines_per_frame = 1252,   
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 5,
		.regs = gc2355_still_1600x1200_30fps,
		.regs_n= (ARRAY_SIZE(gc2355_still_1600x1200_30fps)),

		.vt_pix_clk_freq_mhz=82000000, 
		.crop_horizontal_start=0,
		.crop_vertical_start=0,
		.crop_horizontal_end=1615,
		.crop_vertical_end=1215,
		.output_width=1616,
		.output_height=1216,
		.used = 0,

		.vts_fix=8,
		.vts=1252,
	},
};

static S_UNI_DEVICE gc2355_unidev={
	.desc = "gc2355",
	.version=0x1,
	.minversion=125,//update if change, as buildid
	.af_type=UNI_DEV_AF_NONE,
	.i2c_addr=0x3C,

	.idreg=0xF0,
	.idval=0x2355,
	.idmask=0xffff,
	.powerdelayms=1,
	.accessdelayms=20,
	.flag=UNI_DEV_FLAG_1B_ADDR,
	.exposecoarse=0x03,
	.exposeanaloggain=0xb6,
	.exposedigitgainR=0,
	.exposedigitgainGR=0,
	.exposedigitgainGB=0,
	.exposedigitgainB=0,
	.exposefine=0,
	.exposefine_mask=0,
	.exposevts=0,
	.exposevts_mask=0,

	.exposecoarse_mask=0xffff,
	.exposeanaloggain_mask=0xff,
	.exposedigitgain_mask=0,
	.exposeadgain_param=gc2355_adgain_p,
	.exposeadgain_n=(ARRAY_SIZE(gc2355_adgain_p)),
	.exposeadgain_min=0,
	.exposeadgain_max=0, //max 6x Gain
	.exposeadgain_fra=0,
	.exposeadgain_step=0,
	.exposeadgain_isp1gain=0,
	.exposeadgain_param1gain=0,

	.global_setting=gc2355_global_setting,
	.global_setting_n=(ARRAY_SIZE(gc2355_global_setting)),
	.stream_off=gc2355_stream_off,
	.stream_off_n=(ARRAY_SIZE(gc2355_stream_off)),
	.stream_on=gc2355_stream_on,
	.stream_on_n=(ARRAY_SIZE(gc2355_stream_on)),
	.group_hold_start=gc2355_group_hold_start,
	.group_hold_start_n=(ARRAY_SIZE(gc2355_group_hold_start)),
	.group_hold_end=gc2355_group_hold_end,
	.group_hold_end_n=(ARRAY_SIZE(gc2355_group_hold_end)),

	.ress=gc2355_res_preview,
	.ress_n=(ARRAY_SIZE(gc2355_res_preview)),
	.coarse_integration_time_min=1,
	.coarse_integration_time_max_margin=8,
	.fine_integration_time_min=0,
	.fine_integration_time_max_margin=6,
	.hw_port=ATOMISP_CAMERA_PORT_SECONDARY,
	.hw_lane=1,
	.hw_format=ATOMISP_INPUT_FORMAT_RAW_10,
	.hw_bayer_order=atomisp_bayer_order_gbrg,
	.hw_sensor_type = RAW_CAMERA,

	.hw_reset_gpio=120,
	.hw_reset_gpio_polarity=0,
	.hw_reset_delayms=20,
	.hw_pwd_gpio=124,
	.hw_pwd_gpio_polarity=1,
	.hw_clksource=1,
};
#undef PPR_DEV
#define PPR_DEV gc2355_unidev
