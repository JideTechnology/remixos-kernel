
// auto generate : Thu Aug 14 03:52:28 CST 2014
S_UNI_REG  ov5648_stream_on[] = {
//	{UNICAM_8BIT, 0x0103, 0x01},
	{UNICAM_8BIT, 0x0100, 0x01},
	{UNICAM_TOK_DELAY, 0, 50}, 	//DELAY=50		 //Initialization Time
	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG  ov5648_stream_off[] = {
	{UNICAM_8BIT, 0x0100, 0x00},
	{UNICAM_TOK_DELAY, 0, 50}, 	//DELAY=50		 //Initialization Time
	{UNICAM_TOK_TERM, 0, 0}
};

S_UNI_REG  ov5648_global_setting[] = {
	{ UNICAM_8BIT, 0x0103, 0x01 },
	{ UNICAM_8BIT, 0x3001, 0x00 },
	{ UNICAM_8BIT, 0x3002, 0x00 },
	{ UNICAM_8BIT, 0x3011, 0x02 },
	{ UNICAM_8BIT, 0x3017, 0x05 },
	{ UNICAM_8BIT, 0x3018, 0x4c },
	{ UNICAM_8BIT, 0x301c, 0xd2 },
	{ UNICAM_8BIT, 0x3022, 0x00 },
	{ UNICAM_8BIT, 0x3034, 0x1a },
	{ UNICAM_8BIT, 0x3035, 0x21 },
	{ UNICAM_8BIT, 0x3036, 0x69 },
	{ UNICAM_8BIT, 0x3037, 0x03 },
	{ UNICAM_8BIT, 0x3038, 0x00 },
	{ UNICAM_8BIT, 0x3039, 0x00 },
	{ UNICAM_8BIT, 0x303a, 0x00 },
	{ UNICAM_8BIT, 0x303b, 0x19 },
	{ UNICAM_8BIT, 0x303c, 0x11 },
	{ UNICAM_8BIT, 0x303d, 0x30 },
	{ UNICAM_8BIT, 0x3105, 0x11 },
	{ UNICAM_8BIT, 0x3106, 0x05 },
	{ UNICAM_8BIT, 0x3304, 0x28 },
	{ UNICAM_8BIT, 0x3305, 0x41 },
	{ UNICAM_8BIT, 0x3306, 0x30 },
	{ UNICAM_8BIT, 0x3308, 0x00 },
	{ UNICAM_8BIT, 0x3309, 0xc8 },
	{ UNICAM_8BIT, 0x330a, 0x01 },
	{ UNICAM_8BIT, 0x330b, 0x90 },
	{ UNICAM_8BIT, 0x330c, 0x02 },
	{ UNICAM_8BIT, 0x330d, 0x58 },
	{ UNICAM_8BIT, 0x330e, 0x03 },
	{ UNICAM_8BIT, 0x330f, 0x20 },
	{ UNICAM_8BIT, 0x3300, 0x00 },
	{ UNICAM_8BIT, 0x3500, 0x00 },
	{ UNICAM_8BIT, 0x3501, 0x7b },
	{ UNICAM_8BIT, 0x3502, 0x00 },
	{ UNICAM_8BIT, 0x3503, 0x07 },
	{ UNICAM_8BIT, 0x350a, 0x00 },
	{ UNICAM_8BIT, 0x350b, 0x40 },
	{ UNICAM_8BIT, 0x3601, 0x33 },
	{ UNICAM_8BIT, 0x3602, 0x00 },
	{ UNICAM_8BIT, 0x3611, 0x0e },
	{ UNICAM_8BIT, 0x3612, 0x2b },
	{ UNICAM_8BIT, 0x3614, 0x50 },
	{ UNICAM_8BIT, 0x3620, 0x33 },
	{ UNICAM_8BIT, 0x3622, 0x00 },
	{ UNICAM_8BIT, 0x3630, 0xad },
	{ UNICAM_8BIT, 0x3631, 0x00 },
	{ UNICAM_8BIT, 0x3632, 0x94 },
	{ UNICAM_8BIT, 0x3633, 0x17 },
	{ UNICAM_8BIT, 0x3634, 0x14 },
	{ UNICAM_8BIT, 0x3704, 0xc0 },
	{ UNICAM_8BIT, 0x3705, 0x2a },
	{ UNICAM_8BIT, 0x3708, 0x63 },
	{ UNICAM_8BIT, 0x3709, 0x12 },
	{ UNICAM_8BIT, 0x370b, 0x23 },
	{ UNICAM_8BIT, 0x370c, 0xc0 },
	{ UNICAM_8BIT, 0x370d, 0x00 },
	{ UNICAM_8BIT, 0x370e, 0x00 },
	{ UNICAM_8BIT, 0x371c, 0x07 },
	{ UNICAM_8BIT, 0x3739, 0xd2 },
	{ UNICAM_8BIT, 0x373c, 0x00 },
	{ UNICAM_8BIT, 0x3800, 0x00 },
	{ UNICAM_8BIT, 0x3801, 0x00 },
	{ UNICAM_8BIT, 0x3802, 0x00 },
	{ UNICAM_8BIT, 0x3803, 0x00 },
	{ UNICAM_8BIT, 0x3804, 0x0a },
	{ UNICAM_8BIT, 0x3805, 0x3f },
	{ UNICAM_8BIT, 0x3806, 0x07 },
	{ UNICAM_8BIT, 0x3807, 0xa3 },
	{ UNICAM_8BIT, 0x3808, 0x0a },
	{ UNICAM_8BIT, 0x3809, 0x20 },
	{ UNICAM_8BIT, 0x380a, 0x07 },
	{ UNICAM_8BIT, 0x380b, 0x98 },
	{ UNICAM_8BIT, 0x380c, 0x0b },
	{ UNICAM_8BIT, 0x380d, 0x00 },
	{ UNICAM_8BIT, 0x380e, 0x07 },
	{ UNICAM_8BIT, 0x380f, 0xc0 },
	{ UNICAM_8BIT, 0x3810, 0x00 },
	{ UNICAM_8BIT, 0x3811, 0x10 },
	{ UNICAM_8BIT, 0x3812, 0x00 },
	{ UNICAM_8BIT, 0x3813, 0x06 },
	{ UNICAM_8BIT, 0x3814, 0x11 },
	{ UNICAM_8BIT, 0x3815, 0x11 },
	{ UNICAM_8BIT, 0x3817, 0x00 },
	{ UNICAM_8BIT, 0x3820, 0x40 },///
	{ UNICAM_8BIT, 0x3821, 0x06 },
	{ UNICAM_8BIT, 0x3826, 0x03 },
	{ UNICAM_8BIT, 0x3829, 0x00 },
	{ UNICAM_8BIT, 0x382b, 0x0b },
	{ UNICAM_8BIT, 0x3830, 0x00 },
	{ UNICAM_8BIT, 0x3836, 0x00 },
	{ UNICAM_8BIT, 0x3837, 0x00 },
	{ UNICAM_8BIT, 0x3838, 0x00 },
	{ UNICAM_8BIT, 0x3839, 0x04 },
	{ UNICAM_8BIT, 0x383a, 0x00 },
	{ UNICAM_8BIT, 0x383b, 0x01 },
	{ UNICAM_8BIT, 0x3b00, 0x00 },
	{ UNICAM_8BIT, 0x3b02, 0x08 },
	{ UNICAM_8BIT, 0x3b03, 0x00 },
	{ UNICAM_8BIT, 0x3b04, 0x04 },
	{ UNICAM_8BIT, 0x3b05, 0x00 },
	{ UNICAM_8BIT, 0x3b06, 0x04 },
	{ UNICAM_8BIT, 0x3b07, 0x08 },
	{ UNICAM_8BIT, 0x3b08, 0x00 },
	{ UNICAM_8BIT, 0x3b09, 0x02 },
	{ UNICAM_8BIT, 0x3b0a, 0x04 },
	{ UNICAM_8BIT, 0x3b0b, 0x00 },
	{ UNICAM_8BIT, 0x3b0c, 0x3d },
	{ UNICAM_8BIT, 0x3f01, 0x0d },
	{ UNICAM_8BIT, 0x3f0f, 0xf5 },
	{ UNICAM_8BIT, 0x4000, 0x89 },
	{ UNICAM_8BIT, 0x4001, 0x02 },
	{ UNICAM_8BIT, 0x4002, 0x45 },
	{ UNICAM_8BIT, 0x4004, 0x04 },
	{ UNICAM_8BIT, 0x4005, 0x18 },
	{ UNICAM_8BIT, 0x4006, 0x08 },
	{ UNICAM_8BIT, 0x4007, 0x10 },
	{ UNICAM_8BIT, 0x4008, 0x00 },
	{ UNICAM_8BIT, 0x4050, 0x6e },
	{ UNICAM_8BIT, 0x4051, 0x8f },
	{ UNICAM_8BIT, 0x4300, 0xf8 },
	{ UNICAM_8BIT, 0x4303, 0xff },
	{ UNICAM_8BIT, 0x4304, 0x00 },
	{ UNICAM_8BIT, 0x4307, 0xff },
	{ UNICAM_8BIT, 0x4520, 0x00 },
	{ UNICAM_8BIT, 0x4521, 0x00 },
	{ UNICAM_8BIT, 0x4511, 0x22 },
	{ UNICAM_8BIT, 0x4801, 0x0f },
	{ UNICAM_8BIT, 0x4814, 0x2a },
	{ UNICAM_8BIT, 0x481f, 0x3c },
	{ UNICAM_8BIT, 0x4823, 0x3c },
	{ UNICAM_8BIT, 0x4826, 0x00 },
	{ UNICAM_8BIT, 0x481b, 0x3c },
	{ UNICAM_8BIT, 0x4827, 0x32 },
	{ UNICAM_8BIT, 0x4837, 0x17 },
	{ UNICAM_8BIT, 0x4b00, 0x06 },
	{ UNICAM_8BIT, 0x4b01, 0x0a },
	{ UNICAM_8BIT, 0x4b04, 0x10 },
	{ UNICAM_8BIT, 0x5000, 0xff },
	{ UNICAM_8BIT, 0x5001, 0x00 },
	{ UNICAM_8BIT, 0x5002, 0x41 },
	{ UNICAM_8BIT, 0x5003, 0x0a },
	{ UNICAM_8BIT, 0x5004, 0x00 },
	{ UNICAM_8BIT, 0x5043, 0x00 },
	{ UNICAM_8BIT, 0x5013, 0x00 },
	{ UNICAM_8BIT, 0x501f, 0x03 },
	{ UNICAM_8BIT, 0x503d, 0x00 },
	{ UNICAM_8BIT, 0x5a00, 0x08 },
	{ UNICAM_8BIT, 0x5b00, 0x01 },
	{ UNICAM_8BIT, 0x5b01, 0x40 },
	{UNICAM_8BIT, 0x5b02, 0x00},
	{UNICAM_8BIT, 0x5b03, 0xf0},
//	{UNICAM_8BIT, 0x503d, 0x80}, // test pattern

	{UNICAM_TOK_TERM, 0, 0}
};


S_UNI_REG  ov5648_group_hold_start[] = {
	{UNICAM_8BIT, 0x3208, 0x00},
	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG  ov5648_group_hold_end[] = {
	{UNICAM_8BIT, 0x3208, 0x10},
	{UNICAM_8BIT, 0x3208, 0xA0},
	{UNICAM_TOK_TERM, 0, 0}
};
static  int ov5648_adgain_p[] = {
	0,
};

/*
 * Register settings for various resolution
 */
/*Quarter Size(1296X972) 30fps 2lane 10Bit (Binning)*/
S_UNI_REG ov5648_1296_972_30fps_2lanes[] = {
	// 1296x972 30fps 2 lane
	// 19.2 M, PCLK84.48 MIPI 422.4Mbps/lane

	{UNICAM_8BIT, 0x0100, 0x00},
	{UNICAM_8BIT, 0x3501, 0x3d}, // exposure
	{UNICAM_8BIT, 0x3502, 0x00}, // exposure
	{UNICAM_8BIT, 0x3708, 0x66},
	{UNICAM_8BIT, 0x3709, 0x52},
	{UNICAM_8BIT, 0x370c, 0xcf},
	{UNICAM_8BIT, 0x3800, 0x00}, // xstart = 0
	{UNICAM_8BIT, 0x3801, 0x00}, // x start
	{UNICAM_8BIT, 0x3802, 0x00}, // y start = 0
	{UNICAM_8BIT, 0x3803, 0x00}, // y start
	{UNICAM_8BIT, 0x3804, 0x0a}, // xend = 2623
	{UNICAM_8BIT, 0x3805, 0x3f}, // xend
	{UNICAM_8BIT, 0x3806, 0x07}, // yend = 1955
	{UNICAM_8BIT, 0x3807, 0xa3}, // yend
	{UNICAM_8BIT, 0x3808, 0x05}, // x output size = 1296
	// 23 Company Confidential
	// For OmniVision Internal Only
	// Camera Module Application Notes
	{UNICAM_8BIT, 0x3809, 0x10}, // x output size
	{UNICAM_8BIT, 0x380a, 0x03}, // y output size = 972
	{UNICAM_8BIT, 0x380b, 0xcc}, // y output size
	{UNICAM_8BIT, 0x380c, 0x09},
	{UNICAM_8BIT, 0x380d, 0x60},
	{UNICAM_8BIT, 0x380e, 0x04},
	{UNICAM_8BIT, 0x380f, 0x60},
	{UNICAM_8BIT, 0x3810, 0x00}, // isp x win = 8
	{UNICAM_8BIT, 0x3811, 0x08}, // isp x win
	{UNICAM_8BIT, 0x3812, 0x00}, // isp y win = 4
	{UNICAM_8BIT, 0x3813, 0x04}, // isp y win
	{UNICAM_8BIT, 0x3814, 0x31}, // x inc
	{UNICAM_8BIT, 0x3815, 0x31}, // y inc
	{UNICAM_8BIT, 0x3817, 0x00}, // hsync start
	{UNICAM_8BIT, 0x3820, 0x08}, // flip off, v bin off
	{UNICAM_8BIT, 0x3821, 0x07}, // mirror on, h bin on
	{UNICAM_8BIT, 0x4004, 0x02}, // black line number
	{UNICAM_8BIT, 0x4005, 0x18}, // blc level trigger
	{UNICAM_8BIT, 0x350b, 0x80}, // gain = 8x
	{UNICAM_8BIT, 0x4837, 0x17}, // MIPI global timing
	{UNICAM_8BIT, 0x0100, 0x01},
	//PLLfor 19.2M
	{UNICAM_8BIT, 0x380e, 0x03}, // 0x02,
	{UNICAM_8BIT, 0x380f, 0xe8}, // 0xf2,
	{UNICAM_8BIT, 0x3034, 0x1a}, /* mipi 10bit mode */
	{UNICAM_8BIT, 0x3035, 0x21},
	{UNICAM_8BIT, 0x3036, 0x58},
	{UNICAM_8BIT, 0x3037, 0x02},
	{UNICAM_8BIT, 0x3038, 0x00},
	{UNICAM_8BIT, 0x3039, 0x00},
	{UNICAM_8BIT, 0x3106, 0x05},
	{UNICAM_8BIT, 0x3105, 0x11},
	{UNICAM_8BIT, 0x303a, 0x00},
	{UNICAM_8BIT, 0x303b, 0x16},
	{UNICAM_8BIT, 0x303c, 0x11},
	{UNICAM_8BIT, 0x303d, 0x20},

	{UNICAM_TOK_TERM, 0, 0}
};

S_UNI_REG ov5648_1080p_30fps_2lanes[] = {
	{UNICAM_8BIT, 0x3708, 0x63},
	{UNICAM_8BIT, 0x3709, 0x12},
	{UNICAM_8BIT, 0x370c, 0xc0},
	{UNICAM_8BIT, 0x3800, 0x01},/* xstart = 320 */
	{UNICAM_8BIT, 0x3801, 0x40},/* xstart */
	{UNICAM_8BIT, 0x3802, 0x01},/* ystart = 418 */
	{UNICAM_8BIT, 0x3803, 0xa2},/* ystart */
	{UNICAM_8BIT, 0x3804, 0x08},/* xend = 2287 */
	{UNICAM_8BIT, 0x3805, 0xef},/* xend */
	{UNICAM_8BIT, 0x3806, 0x05},/* yend = 1521 */
	{UNICAM_8BIT, 0x3807, 0xf1},/* yend */
	{UNICAM_8BIT, 0x3808, 0x07},/* x output size = 1940 */
	{UNICAM_8BIT, 0x3809, 0x94},/* x output size */
	{UNICAM_8BIT, 0x380a, 0x04},/* y output size = 1096 */
	{UNICAM_8BIT, 0x380b, 0x48},/* y output size */
	{UNICAM_8BIT, 0x380c, 0x09},/* hts = 2500 */
	{UNICAM_8BIT, 0x380d, 0xc4},/* hts */
	{UNICAM_8BIT, 0x380e, 0x04},/* vts = 1120 */
	{UNICAM_8BIT, 0x380f, 0x60},/* vts */
	{UNICAM_8BIT, 0x3810, 0x00},/* isp x win = 16 */
	{UNICAM_8BIT, 0x3811, 0x10},/* isp x win */
	{UNICAM_8BIT, 0x3812, 0x00},/* isp y win = 4 */
	{UNICAM_8BIT, 0x3813, 0x04},/* isp y win */
	{UNICAM_8BIT, 0x3814, 0x11},/* x inc */
	{UNICAM_8BIT, 0x3815, 0x11},/* y inc */
	{UNICAM_8BIT, 0x3817, 0x00},/* hsync start */
	{UNICAM_8BIT, 0x3820, 0x40},/* flip off; v bin off */
	{UNICAM_8BIT, 0x3821, 0x06},/* mirror off; v bin off */
	{UNICAM_8BIT, 0x4004, 0x04},/* black line number */
	{UNICAM_8BIT, 0x4005, 0x18},/* blc always update */
	{UNICAM_8BIT, 0x4837, 0x18},/* MIPI global timing */

	{UNICAM_8BIT, 0x350b, 0x40},/* gain 4x */
	{UNICAM_8BIT, 0x3501, 0x45},/* exposure */
	{UNICAM_8BIT, 0x3502, 0x80},/* exposure */
	/*;add 19.2MHz 30fps */

	{UNICAM_8BIT, 0x3034, 0x1a},/* mipi 10bit mode */
	{UNICAM_8BIT, 0x3035, 0x21},
	{UNICAM_8BIT, 0x3036, 0x58},
	{UNICAM_8BIT, 0x3037, 0x02},
	{UNICAM_8BIT, 0x3038, 0x00},
	{UNICAM_8BIT, 0x3039, 0x00},
	{UNICAM_8BIT, 0x3106, 0x05},
	{UNICAM_8BIT, 0x3105, 0x11},
	{UNICAM_8BIT, 0x303a, 0x00},
	{UNICAM_8BIT, 0x303b, 0x16},
	{UNICAM_8BIT, 0x303c, 0x11},
	{UNICAM_8BIT, 0x303d, 0x20},

	{UNICAM_TOK_TERM, 0, 0}
};

S_UNI_REG ov5648_5M_15fps_2lanes[] = {
	//{UNICAM_8BIT, 0x0103, 0x01},//software reset ## delay 5ms
	{ UNICAM_8BIT, 0x3001, 0x00 },
	{ UNICAM_8BIT, 0x3002, 0x00 },
	{ UNICAM_8BIT, 0x3011, 0x02 },
	{ UNICAM_8BIT, 0x3017, 0x05 },
	{ UNICAM_8BIT, 0x3018, 0x4c },
	{ UNICAM_8BIT, 0x301c, 0xd2 },
	{ UNICAM_8BIT, 0x3022, 0x00 },
	{ UNICAM_8BIT, 0x3034, 0x1a },
	{ UNICAM_8BIT, 0x3035, 0x21 },
	{ UNICAM_8BIT, 0x3036, 0x58 },//69
	{ UNICAM_8BIT, 0x3037, 0x02 },//03
	{ UNICAM_8BIT, 0x3038, 0x00 },
	{ UNICAM_8BIT, 0x3039, 0x00 },
	{ UNICAM_8BIT, 0x303a, 0x00 },
	{ UNICAM_8BIT, 0x303b, 0x16 },//19
	{ UNICAM_8BIT, 0x303c, 0x11 },
	{ UNICAM_8BIT, 0x303d, 0x20 },//30
	{ UNICAM_8BIT, 0x3105, 0x11 },
	{ UNICAM_8BIT, 0x3106, 0x05 },
	{ UNICAM_8BIT, 0x3304, 0x28 },
	{ UNICAM_8BIT, 0x3305, 0x41 },
	{ UNICAM_8BIT, 0x3306, 0x30 },
	{ UNICAM_8BIT, 0x3308, 0x00 },
	{ UNICAM_8BIT, 0x3309, 0xc8 },
	{ UNICAM_8BIT, 0x330a, 0x01 },
	{ UNICAM_8BIT, 0x330b, 0x90 },
	{ UNICAM_8BIT, 0x330c, 0x02 },
	{ UNICAM_8BIT, 0x330d, 0x58 },
	{ UNICAM_8BIT, 0x330e, 0x03 },
	{ UNICAM_8BIT, 0x330f, 0x20 },
	{ UNICAM_8BIT, 0x3300, 0x00 },
	{ UNICAM_8BIT, 0x3500, 0x00 },
	{ UNICAM_8BIT, 0x3501, 0x7b },
	{ UNICAM_8BIT, 0x3502, 0x00 },
	{ UNICAM_8BIT, 0x3503, 0x07 },
	{ UNICAM_8BIT, 0x350a, 0x00 },
	{ UNICAM_8BIT, 0x350b, 0x40 },
	{ UNICAM_8BIT, 0x3601, 0x33 },
	{ UNICAM_8BIT, 0x3602, 0x00 },
	{ UNICAM_8BIT, 0x3611, 0x0e },
	{ UNICAM_8BIT, 0x3612, 0x2b },
	{ UNICAM_8BIT, 0x3614, 0x50 },
	{ UNICAM_8BIT, 0x3620, 0x33 },
	{ UNICAM_8BIT, 0x3622, 0x00 },
	{ UNICAM_8BIT, 0x3630, 0xad },
	{ UNICAM_8BIT, 0x3631, 0x00 },
	{ UNICAM_8BIT, 0x3632, 0x94 },
	{ UNICAM_8BIT, 0x3633, 0x17 },
	{ UNICAM_8BIT, 0x3634, 0x14 },
	{ UNICAM_8BIT, 0x3704, 0xc0 },
	{ UNICAM_8BIT, 0x3705, 0x2a },
	{ UNICAM_8BIT, 0x3708, 0x63 },
	{ UNICAM_8BIT, 0x3709, 0x12 },
	{ UNICAM_8BIT, 0x370b, 0x23 },
	{ UNICAM_8BIT, 0x370c, 0xcc },//0xc0-->0xcc based on OV5648R1A_AM05e.ovd
	{ UNICAM_8BIT, 0x370d, 0x00 },
	{ UNICAM_8BIT, 0x370e, 0x00 },
	{ UNICAM_8BIT, 0x371c, 0x07 },
	{ UNICAM_8BIT, 0x3739, 0xd2 },
	{ UNICAM_8BIT, 0x373c, 0x00 },
	{ UNICAM_8BIT, 0x3800, 0x00 },
	{ UNICAM_8BIT, 0x3801, 0x00 },
	{ UNICAM_8BIT, 0x3802, 0x00 },
	{ UNICAM_8BIT, 0x3803, 0x00 },
	{ UNICAM_8BIT, 0x3804, 0x0a },
	{ UNICAM_8BIT, 0x3805, 0x3f },
	{ UNICAM_8BIT, 0x3806, 0x07 },
	{ UNICAM_8BIT, 0x3807, 0xa3 },
	{ UNICAM_8BIT, 0x3808, 0x0a },
	{ UNICAM_8BIT, 0x3809, 0x20 },
	{ UNICAM_8BIT, 0x380a, 0x07 },
	{ UNICAM_8BIT, 0x380b, 0x98 },
	{ UNICAM_8BIT, 0x380c, 0x0b }, // hts 2838
	{ UNICAM_8BIT, 0x380d, 0x16 },//00
	{ UNICAM_8BIT, 0x380e, 0x07 },//vts 1984
	{ UNICAM_8BIT, 0x380f, 0xc0 },
	{ UNICAM_8BIT, 0x3810, 0x00 },
	{ UNICAM_8BIT, 0x3811, 0x10 },
	{ UNICAM_8BIT, 0x3812, 0x00 },
	{ UNICAM_8BIT, 0x3813, 0x06 },
	{ UNICAM_8BIT, 0x3814, 0x11 },
	{ UNICAM_8BIT, 0x3815, 0x11 },
	{ UNICAM_8BIT, 0x3817, 0x00 },
	{ UNICAM_8BIT, 0x3820, 0x40 },//40
	{ UNICAM_8BIT, 0x3821, 0x06 },
	{ UNICAM_8BIT, 0x3826, 0x03 },
	{ UNICAM_8BIT, 0x3829, 0x00 },
	{ UNICAM_8BIT, 0x382b, 0x0b },
	{ UNICAM_8BIT, 0x3830, 0x00 },
	{ UNICAM_8BIT, 0x3836, 0x00 },
	{ UNICAM_8BIT, 0x3837, 0x00 },
	{ UNICAM_8BIT, 0x3838, 0x00 },
	{ UNICAM_8BIT, 0x3839, 0x04 },
	{ UNICAM_8BIT, 0x383a, 0x00 },
	{ UNICAM_8BIT, 0x383b, 0x01 },
	{ UNICAM_8BIT, 0x3b00, 0x00 },
	{ UNICAM_8BIT, 0x3b02, 0x08 },
	{ UNICAM_8BIT, 0x3b03, 0x00 },
	{ UNICAM_8BIT, 0x3b04, 0x04 },
	{ UNICAM_8BIT, 0x3b05, 0x00 },
	{ UNICAM_8BIT, 0x3b06, 0x04 },
	{ UNICAM_8BIT, 0x3b07, 0x08 },
	{ UNICAM_8BIT, 0x3b08, 0x00 },
	{ UNICAM_8BIT, 0x3b09, 0x02 },
	{ UNICAM_8BIT, 0x3b0a, 0x04 },
	{ UNICAM_8BIT, 0x3b0b, 0x00 },
	{ UNICAM_8BIT, 0x3b0c, 0x3d },
	{ UNICAM_8BIT, 0x3f01, 0x0d },
	{ UNICAM_8BIT, 0x3f0f, 0xf5 },
	{ UNICAM_8BIT, 0x4000, 0x89 },
	{ UNICAM_8BIT, 0x4001, 0x02 },
	{ UNICAM_8BIT, 0x4002, 0x45 },
	{ UNICAM_8BIT, 0x4004, 0x04 },
	{ UNICAM_8BIT, 0x4005, 0x1a },//18
	{ UNICAM_8BIT, 0x4006, 0x08 },
	{ UNICAM_8BIT, 0x4007, 0x10 },
	{ UNICAM_8BIT, 0x4008, 0x00 },
	{ UNICAM_8BIT, 0x4050, 0x6e },
	{ UNICAM_8BIT, 0x4051, 0x8f },
	{ UNICAM_8BIT, 0x4300, 0xf8 },
	{ UNICAM_8BIT, 0x4303, 0xff },
	{ UNICAM_8BIT, 0x4304, 0x00 },
	{ UNICAM_8BIT, 0x4307, 0xff },
	{ UNICAM_8BIT, 0x4520, 0x00 },
	{ UNICAM_8BIT, 0x4521, 0x00 },
	{ UNICAM_8BIT, 0x4511, 0x22 },
	{ UNICAM_8BIT, 0x4801, 0x0f },
	{ UNICAM_8BIT, 0x4814, 0x2a },
	{ UNICAM_8BIT, 0x481f, 0x3c },
	{ UNICAM_8BIT, 0x4823, 0x3c },
	{ UNICAM_8BIT, 0x4826, 0x00 },
	{ UNICAM_8BIT, 0x481b, 0x3c },
	{ UNICAM_8BIT, 0x4827, 0x32 },
	{ UNICAM_8BIT, 0x4837, 0x17 },
	{ UNICAM_8BIT, 0x4b00, 0x06 },
	{ UNICAM_8BIT, 0x4b01, 0x0a },
	{ UNICAM_8BIT, 0x4b04, 0x10 },
	{ UNICAM_8BIT, 0x5000, 0xff },
	{ UNICAM_8BIT, 0x5001, 0x00 },
	{ UNICAM_8BIT, 0x5002, 0x41 },
	{ UNICAM_8BIT, 0x5003, 0x0a },
	{ UNICAM_8BIT, 0x5004, 0x00 },
	{ UNICAM_8BIT, 0x5043, 0x00 },
	{ UNICAM_8BIT, 0x5013, 0x00 },
	{ UNICAM_8BIT, 0x501f, 0x03 },
	{ UNICAM_8BIT, 0x503d, 0x00 },
	{ UNICAM_8BIT, 0x5a00, 0x08 },
	{ UNICAM_8BIT, 0x5b00, 0x01 },
	{ UNICAM_8BIT, 0x5b01, 0x40 },
	{ UNICAM_8BIT, 0x5b02, 0x00 },
	{ UNICAM_8BIT, 0x5b03, 0xf0 },
	{ UNICAM_TOK_TERM, 0, 0 }
};

static S_UNI_RESOLUTION ov5648_res_preview[] = {
    /* Add for CTS test only, begin */
	{
	 .desc = "ov5648_1296_972_30fps",
	 .res_type = UNI_DEV_RES_PREVIEW|UNI_DEV_RES_VIDEO|UNI_DEV_RES_STILL,
	 .width = 1296,
	 .height = 972,
	 .fps = 30,
	 .pixels_per_line = 2400,
	 .lines_per_frame = 1120,
	 .bin_factor_x = 2,
	 .bin_factor_y = 2,
	 .bin_mode = 1,
	 .skip_frames = 3,
	 .regs = ov5648_1296_972_30fps_2lanes,
	 .regs_n= (ARRAY_SIZE(ov5648_1296_972_30fps_2lanes)),
	 .vt_pix_clk_freq_mhz = 84480000,

	 .crop_horizontal_start=0,
	 .crop_vertical_start=0,
	 .crop_horizontal_end=2623,
	 .crop_vertical_end=1955,
	 .output_width=1296,
	 .output_height=972,
	 .used = 0,
	 .vts_fix=8,
	 .vts=1120,
	},
	{
		.desc = "ov5648_1080p_30fps",
		.res_type = UNI_DEV_RES_PREVIEW|UNI_DEV_RES_VIDEO|UNI_DEV_RES_STILL,
		.width = 1936,
		.height = 1096,
		.fps = 30,
		.pixels_per_line = 2500,
		.lines_per_frame = 1120,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 1,
		.regs = ov5648_1080p_30fps_2lanes,
		.regs_n= (ARRAY_SIZE(ov5648_1080p_30fps_2lanes)),

		.vt_pix_clk_freq_mhz=84480000,
		.crop_horizontal_start=320,
		.crop_vertical_start=418,
		.crop_horizontal_end=2287,
		.crop_vertical_end=1521,
		.output_width=1920,
		.output_height=1080,
		.used = 0,

		.vts_fix=8,
		.vts=1120,

	},
	{
		.desc = "ov5648_5M_15fps",
		.res_type = UNI_DEV_RES_PREVIEW|UNI_DEV_RES_VIDEO|UNI_DEV_RES_STILL,
		.width = 2592,
		.height = 1944,
		.fps = 15,
		.pixels_per_line = 2838,
		.lines_per_frame = 1984,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 3,
		.regs = ov5648_5M_15fps_2lanes,
		.regs_n= (ARRAY_SIZE(ov5648_5M_15fps_2lanes)),

		.vt_pix_clk_freq_mhz=84480000,
		.crop_horizontal_start=0,
		.crop_vertical_start=0,
		.crop_horizontal_end=2623,
		.crop_vertical_end=1955,
		.output_width=2592,
		.output_height=1944,
		.used = 0,

		.vts_fix=8,
		.vts=1984,

	},
};
static S_UNI_DEVICE ov5648_unidev= {
	.desc = "ov5648",
	.version=0x1,
	.minversion=125,//update if change, as buildid
	.af_type=UNI_DEV_AF_DW9714,
	.i2c_addr=0x36,

	.idreg=0x300A,
	.idval=0x5648,
	.idmask=0xffff,
	.powerdelayms=1,
	.accessdelayms=20,
	.flag=0,
	.exposecoarse=0x3500,
	.exposeanaloggain=0x350A,
	.exposedigitgainR=0,
	.exposedigitgainGR=0,
	.exposedigitgainGB=0,
	.exposedigitgainB=0,
	.exposefine=0,
	.exposefine_mask=0,
	.exposevts=0x380e,
	.exposevts_mask=0xffff,

	.exposecoarse_mask=0xfffff,
	.exposeanaloggain_mask=0x4ff,
	.exposedigitgain_mask=0,
	.exposeadgain_param=ov5648_adgain_p,
	.exposeadgain_n=(ARRAY_SIZE(ov5648_adgain_p)),
	.exposeadgain_min=0,
	.exposeadgain_max=0,
	.exposeadgain_fra=0,
	.exposeadgain_step=0,
	.exposeadgain_isp1gain=0,
	.exposeadgain_param1gain=0,


	.global_setting=ov5648_global_setting,
	.global_setting_n=(ARRAY_SIZE(ov5648_global_setting)),
	.stream_off=ov5648_stream_off,
	.stream_off_n=(ARRAY_SIZE(ov5648_stream_off)),
	.stream_on=ov5648_stream_on,
	.stream_on_n=(ARRAY_SIZE(ov5648_stream_on)),
	.group_hold_start=ov5648_group_hold_start,
	.group_hold_start_n=(ARRAY_SIZE(ov5648_group_hold_start)),
	.group_hold_end=ov5648_group_hold_end,
	.group_hold_end_n=(ARRAY_SIZE(ov5648_group_hold_end)),


	.ress=ov5648_res_preview,
	.ress_n=(ARRAY_SIZE(ov5648_res_preview)),
	.coarse_integration_time_min=1,
	.coarse_integration_time_max_margin=8,
	.fine_integration_time_min=0,
	.fine_integration_time_max_margin=0,
	.hw_port=ATOMISP_CAMERA_PORT_PRIMARY,
	.hw_lane=2,
	.hw_format=ATOMISP_INPUT_FORMAT_RAW_10,
	.hw_bayer_order=atomisp_bayer_order_bggr,

	.hw_reset_gpio=119,
	.hw_reset_gpio_polarity=0,
	.hw_reset_delayms=20,
	.hw_pwd_gpio=123,
	.hw_pwd_gpio_polarity=0,
	.hw_clksource=0,

};

#undef PPR_DEV
#define PPR_DEV ov5648_unidev
