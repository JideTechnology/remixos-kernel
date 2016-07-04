
// auto generate : Fri Sep 26 15:49:18 CST 2014
S_UNI_REG  hm2051_stream_on[] = {
	{UNICAM_8BIT, 0x0000, 0x01},
	{UNICAM_8BIT, 0x0100, 0x01},
	{UNICAM_8BIT, 0x0101, 0x01},
	{UNICAM_8BIT, 0x4B20, 0x9E},
	{UNICAM_8BIT, 0x0005, 0x03},	//Turn on rolling shutter
	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG  hm2051_stream_off[] = {
	{UNICAM_8BIT, 0x0005, 0x02},	//Turn on rolling shutter
	{UNICAM_8BIT, 0x4B20, 0xDE},
	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG  hm2051_global_setting[] = {
	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG  hm2051_group_hold_start[] = {
	{UNICAM_8BIT, 0x0100, 0x00},		
	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG  hm2051_group_hold_end[] = {
	{UNICAM_8BIT, 0x0100, 0x01},	
	{UNICAM_TOK_TERM, 0, 0}
};
static  int hm2051_adgain_p[] = {
	0,100,
	1,200,
	2,400,
	3,800,
};
S_UNI_REG hm2051_regress0[] = {
	//---------------------------------------------------
	// Initial
	//---------------------------------------------------
//	{UNICAM_8BIT,0x0022,0x00},// RESET
	// ---------------------------------------------------
	// CMU update
	// ---------------------------------------------------
	{UNICAM_TOK_DELAY,0,40},
	{UNICAM_8BIT,0x0000,0x00},//
	{UNICAM_8BIT,0x0100,0x00},//
	{UNICAM_8BIT,0x0101,0x00},//
	{UNICAM_TOK_DELAY,0,50},
	{UNICAM_8BIT,0x0005,0x02},// power up
	{UNICAM_TOK_DELAY,0,100},

	{UNICAM_8BIT,0x0010,0x02},//BLNKRH[7:0]
	{UNICAM_8BIT,0x0011,0x02},//BLNKRL[7:0]

	{UNICAM_8BIT,0x0026,0x08},// PLL1, mipi pll_pre_div
	{UNICAM_8BIT,0x002A,0x52},// PLL1, mipi pll_div (pclk=pktclk= 002A + 1)

	//---------------------------------------------------
	// Digital function
	//---------------------------------------------------
	{UNICAM_8BIT,0x0006,0x04},// [1] hmirror, [0] vflip
	{UNICAM_8BIT,0x000F,0x00},// [1] fixed frame rate mode, [0] non fixed frame rate mode
	{UNICAM_8BIT,0x0024,0x40},// mipi enable
	{UNICAM_8BIT,0x0027,0x23},// OUTFMT, after BPC, [7] pre_vsync_en
	{UNICAM_8BIT,0x0065,0x01},// Hscan_delay (default=1)
	{UNICAM_8BIT,0x0074,0x1B},// disable output black rows
	//---------------------------------------------------
	// Analog
	//---------------------------------------------------

	{UNICAM_8BIT,0x002B,0x00},//  clk divider
	{UNICAM_8BIT,0x002C,0x06},// PLL cfg: CP, LPF, use PLL clk

	{UNICAM_8BIT,0x0040,0x00}, // BLC  //0A->00
	{UNICAM_8BIT,0x0044,0x03},// enable BLC, enable BLC IIR
	{UNICAM_8BIT,0x0045,0x63},// CFPN cfg, 0x65=repeat 32 times, 0x55=repeat 16 times
	{UNICAM_8BIT,0x0046,0x5F},// CFPN cfg, enable IIR, weight=1/4 new, CFPN applied to BLC
	{UNICAM_8BIT,0x0049,0xC0},//improve BLC_hunting
	{UNICAM_8BIT,0x004B,0x03},//improve BLC_hunting

	{UNICAM_8BIT,0x0070,0x2F},// ADD  0923
	{UNICAM_8BIT,0x0072,0xFB},//8F -> FB  0923
	{UNICAM_8BIT,0x0073,0x77},// ADD  0923 for 30 fps
	{UNICAM_8BIT,0x0075,0x40},// Negative CP is controlled by INT
	{UNICAM_8BIT,0x0078,0x65},// ADD  0923 for 30 fps

	{UNICAM_8BIT,0x0080,0x98},// fd_clamp_en_d=1, tg_boost_en_d=1  //90 -> 98  0923
	{UNICAM_8BIT,0x0082,0x09},// fd_clamp_en_d=1, tg_boost_en_d=1
	{UNICAM_8BIT,0x0083,0x3C},// VRPP=avdd+1.36V, VRNP=-1V, w/ lowest pump clk freq.
	{UNICAM_8BIT,0x0087,0x41},// disable dsun clamp first  31 -> 41 0923
	{UNICAM_8BIT,0x008D,0x20},// pix_disc_diff_en_d=1
	{UNICAM_8BIT,0x008E,0x30},//

	{UNICAM_8BIT,0x009D,0x11},// Nramp_rst1,2
	{UNICAM_8BIT,0x009E,0x12},// Nramp_rst3,4

	{UNICAM_8BIT,0x0090,0x00},// gain table
	{UNICAM_8BIT,0x0091,0x01},// gain table
	{UNICAM_8BIT,0x0092,0x02},// gain table
	{UNICAM_8BIT,0x0093,0x03},// gain table

	{UNICAM_8BIT,0x00C0,0x64},// col_ldo setting
	{UNICAM_8BIT,0x00C1,0x15},// pvdd_refsel=5h(for power noise), pvdd_lorefsel
	{UNICAM_8BIT,0x00C2,0x00},// ramp power consumption

	{UNICAM_8BIT,0x00C3,0x02},// comp1,2,3_bias_sel
	{UNICAM_8BIT,0x00C4,0x0B},// column ADC cfg
	{UNICAM_8BIT,0x00C6,0x83},// sf_srcdr_shrt_en_right_d=1, sf_always_on_d=0(improve noise)  93 -> 83 0923
	{UNICAM_8BIT,0x00C7,0x02},// sr_sel_sh_d (reduce CFPN @ high AVDD) ADD 0923
	{UNICAM_8BIT,0x00CC,0x00},// mipi skew[5:0]

	{UNICAM_8BIT,0x4B3B,0x12},// MIPI analog setting
	{UNICAM_8BIT,0x4B41,0x10},// HS0_D=1, prevent enter test mode(clk lane=0)

	//Star of BPC setting
	{UNICAM_8BIT,0x0165,0x03},//[1:0]0:24 1:32, 2:48, 3:80
	{UNICAM_8BIT,0x0183,0xF0},//20150109 for 720P BPC
	{UNICAM_8BIT,0x018C,0x00},//[7:6]Dark_raw_enable

	{UNICAM_8BIT,0x0195,0x06},//X_offset[2:0]
	{UNICAM_8BIT,0x0196,0x4F},//X_offset[7:0]
	{UNICAM_8BIT,0x0197,0x04},//Y_offset[2:0]
	{UNICAM_8BIT,0x0198,0xBF},//Y_offset[7:0]

	{UNICAM_8BIT,0x0144,0x12},//BPC_HOT_TH[8],[1]Median Filter with current pixel
	{UNICAM_8BIT,0x0140,0x20},//BPC_HOT_TH[7:0]
	{UNICAM_8BIT,0x015A,0x80},//BPC_HOT_2
	{UNICAM_8BIT,0x015D,0x20},//BPC_HOT_3
	{UNICAM_8BIT,0x0160,0x65},//[0]hot_replace[1]cold_replace[3:2]Max1_Max2[4]correct_all[5]Dynamic[6]Static[7]no write back

	// ---------------------------------------------------/
	// mipi-tx settings
	// ---------------------------------------------------
	// window off (1616x1216)
	//{UNICAM_8BIT,0x0123,0xC5},// [4] digital window off

	{UNICAM_8BIT,0x0123,0xD5},
	{UNICAM_8BIT,0x0660,0x00},  //win x_st Hb
	{UNICAM_8BIT,0x0661,0xA0},  //win x_st Lb
	{UNICAM_8BIT,0x0662,0x05},  //win x_ed Hb
	{UNICAM_8BIT,0x0663,0xAF},  //win x_ed Lb
	{UNICAM_8BIT,0x0664,0x00},  //win y_st Hb
	{UNICAM_8BIT,0x0665,0x2E},  //win y_st Lb
	{UNICAM_8BIT,0x0666,0x03},  //win y_ed Hb
	{UNICAM_8BIT,0x0667,0x0D},  //win y_ed Lb

	{UNICAM_8BIT,0x4B50,0x00},// pre_h Hb
	{UNICAM_8BIT,0x4B51,0xAF},// pre_h Lb  CD->AF
	{UNICAM_8BIT,0x4B0A,0x05},
	{UNICAM_8BIT,0x4B0B,0x10},

	// window on (default 1600x1200)
	{UNICAM_8BIT,0x4B20,0x9E},//
	{UNICAM_8BIT,0x4B07,0xBD},// MARK1 width

	// ---------------------------------------------------
	// CMU update
	// ---------------------------------------------------
	{UNICAM_8BIT,0x0000,0x00},//
	{UNICAM_8BIT,0x0100,0x00},//
	{UNICAM_8BIT,0x0101,0x00},//

	//---------------------------------------------------
	// Turn on rolling shutter
	//---------------------------------------------------
	{UNICAM_8BIT,0x0005,0x03},//
	{UNICAM_8BIT,0x0025,0x00},//
	//{UNICAM_8BIT,0x0028,0x84},//test pattern

	{UNICAM_TOK_TERM, 0, 0}
};

S_UNI_REG hm2051_regress1[] = {
	//---------------------------------------------------
	// Initial
	//---------------------------------------------------
//	{UNICAM_8BIT,0x0022,0x00},// RESET
	{UNICAM_8BIT,0x0005,0x02},// power up
	{UNICAM_TOK_DELAY,0,100},

	{UNICAM_8BIT,0x0026,0x08},// PLL1, mipi pll_pre_div
	{UNICAM_8BIT,0x002A,0x52},// PLL1, mipi pll_div (pclk=pktclk= 002A + 1)

	//---------------------------------------------------
	// Digital function
	//---------------------------------------------------
	{UNICAM_8BIT,0x0006,0x00},// [1] hmirror, [0] vflip
	{UNICAM_8BIT,0x000F,0x00},// [1] fixed frame rate mode, [0] non fixed frame rate mode
	{UNICAM_8BIT,0x0024,0x40},// mipi enable
	{UNICAM_8BIT,0x0027,0x23},// OUTFMT, after BPC, [7] pre_vsync_en
	{UNICAM_8BIT,0x0065,0x01},// Hscan_delay (default=1)
	{UNICAM_8BIT,0x0074,0x13},// disable output black rows
	//---------------------------------------------------
	// Analog
	//---------------------------------------------------
	{UNICAM_8BIT,0x002B,0x00},//  clk divider
	{UNICAM_8BIT,0x002C,0x06},// PLL cfg: CP, LPF, use PLL clk

	{UNICAM_8BIT,0x0040,0x00}, // BLC  //0A->00
	{UNICAM_8BIT,0x0044,0x03},// enable BLC, enable BLC IIR
	{UNICAM_8BIT,0x0045,0x63},// CFPN cfg, 0x65=repeat 32 times, 0x55=repeat 16 times
	{UNICAM_8BIT,0x0046,0x5F},// CFPN cfg, enable IIR, weight=1/4 new, CFPN applied to BLC
	{UNICAM_8BIT,0x0049,0xC0},//improve BLC_hunting
	{UNICAM_8BIT,0x004B,0x03},//improve BLC_hunting

	{UNICAM_8BIT,0x0072,0x89},//for CFPN
	{UNICAM_8BIT,0x0073,0x77}, // for 30 fps
	{UNICAM_8BIT,0x0075,0x40},// Negative CP is controlled by INT
	{UNICAM_8BIT,0x0078,0x65}, // for 30 fps

	{UNICAM_8BIT,0x0080,0x90},// fd_clamp_en_d=1, tg_boost_en_d=1
	{UNICAM_8BIT,0x0082,0x09},// fd_clamp_en_d=1, tg_boost_en_d=1
	{UNICAM_8BIT,0x0083,0x3C},// VRPP=avdd+1.36V, VRNP=-1V, w/ lowest pump clk freq.
	{UNICAM_8BIT,0x0087,0x31},// disable dsun clamp first
	{UNICAM_8BIT,0x008D,0x20},// pix_disc_diff_en_d=1
	{UNICAM_8BIT,0x008E,0x30},//

	{UNICAM_8BIT,0x009D,0x11},// Nramp_rst1,2
	{UNICAM_8BIT,0x009E,0x12},// Nramp_rst3,4

	{UNICAM_8BIT,0x0090,0x00},// gain table
	{UNICAM_8BIT,0x0091,0x01},// gain table
	{UNICAM_8BIT,0x0092,0x02},// gain table
	{UNICAM_8BIT,0x0093,0x03},// gain table

	{UNICAM_8BIT,0x00C0,0x64},// col_ldo setting
	{UNICAM_8BIT,0x00C1,0x15},// pvdd_refsel=5h(for power noise), pvdd_lorefsel
	{UNICAM_8BIT,0x00C2,0x00},// ramp power consumption

	{UNICAM_8BIT,0x00C3,0x02},// comp1,2,3_bias_sel
	{UNICAM_8BIT,0x00C4,0x0B},// column ADC cfg
	{UNICAM_8BIT,0x00C6,0x93},// sf_srcdr_shrt_en_right_d=1, sf_always_on_d=0(improve noise)
	{UNICAM_8BIT,0x00CC,0x00},// mipi skew[5:0]

	{UNICAM_8BIT,0x4B3B,0x12},// MIPI analog setting
	{UNICAM_8BIT,0x4B41,0x10},// HS0_D=1, prevent enter test mode(clk lane=0)

	//Star of BPC setting
	{UNICAM_8BIT,0x0165,0x03},//[1:0]0:24 1:32, 2:48, 3:80
	{UNICAM_8BIT,0x018C,0x00},//[7:6]Dark_raw_enable

	{UNICAM_8BIT,0x0195,0x06},//X_offset[2:0]
	{UNICAM_8BIT,0x0196,0x4F},//X_offset[7:0]
	{UNICAM_8BIT,0x0197,0x04},//Y_offset[2:0]
	{UNICAM_8BIT,0x0198,0xBF},//Y_offset[7:0]

	{UNICAM_8BIT,0x0144,0x12},//BPC_HOT_TH[8],[1]Median Filter with current pixel
	{UNICAM_8BIT,0x0140,0x20},//BPC_HOT_TH[7:0]
	{UNICAM_8BIT,0x015A,0x80},//BPC_HOT_2
	{UNICAM_8BIT,0x015D,0x20},//BPC_HOT_3
	{UNICAM_8BIT,0x0160,0x65},//[0]hot_replace[1]cold_replace[3:2]Max1_Max2[4]correct_all[5]Dynamic[6]Static[7]no write back 

	// ---------------------------------------------------/
	// mipi-tx settings
	// ---------------------------------------------------
	// window off (1616x1216)
	{UNICAM_8BIT,0x0123,0xC5},// [4] digital window off
	{UNICAM_8BIT,0x4B50,0x08},// pre_h Hb 09->08
	{UNICAM_8BIT,0x4B51,0xB2},// pre_h Lb 22->B2

	// window on (default 1600x1200)
	{UNICAM_8BIT,0x4B20,0x9E},//
	{UNICAM_8BIT,0x4B07,0xBD},// MARK1 width

	// ---------------------------------------------------
	// CMU update
	// ---------------------------------------------------
	{UNICAM_8BIT,0x0000,0x00},//
	{UNICAM_8BIT,0x0100,0x00},//
	{UNICAM_8BIT,0x0101,0x00},//

	//---------------------------------------------------
	// Turn on rolling shutter
	//---------------------------------------------------
	{UNICAM_8BIT,0x0005,0x03},//
	{UNICAM_8BIT,0x0025,0x00},//
	//{UNICAM_8BIT,0x0028,0x84},//test pattern
	
	{UNICAM_TOK_TERM, 0, 0}
};

S_UNI_REG hm2051_regress2[] = {
	{UNICAM_TOK_TERM, 0, 0}
};
#if 0
S_UNI_REG hm2051_regress[][] = {
	hm2051_regress0,
	hm2051_regress1,
	hm2051_regress2,
};
#endif

static S_UNI_RESOLUTION hm2051_res_preview[] = {
	/*{
		.desc = "hm2051_720p",
		.res_type = UNI_DEV_RES_PREVIEW | UNI_DEV_RES_VIDEO | UNI_DEV_RES_STILL,
		.width = 1296,
		.height = 736,
		.fps = 30,
		.pixels_per_line = 1913,
		.lines_per_frame = 786,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 3,
		.regs = hm2051_regress0,
		.regs_n= (ARRAY_SIZE(hm2051_regress0)),

		.vt_pix_clk_freq_mhz=70400000,
		.crop_horizontal_start=0,
		.crop_vertical_start=0,
		.crop_horizontal_end=1295,
		.crop_vertical_end=735,
		.output_width=1296,
		.output_height=736,
		.vts_fix=8,
		.vts=786,
	},*/{
		.desc = "hm2051_1200p",
		.res_type = UNI_DEV_RES_PREVIEW | UNI_DEV_RES_VIDEO | UNI_DEV_RES_STILL,
		.width = 1616,
		.height = 1216,
		.fps = 30,
		.pixels_per_line = 1913,
		.lines_per_frame = 1266,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 3,
		.regs = hm2051_regress1,
		.regs_n= (ARRAY_SIZE(hm2051_regress1)),

		.vt_pix_clk_freq_mhz=70400000,
		.crop_horizontal_start=0,
		.crop_vertical_start=0,
		.crop_horizontal_end=1615,
		.crop_vertical_end=1211,
		.output_width=1616,
		.output_height=1216,
		.used = 0,

		.vts_fix=8,
		.vts=1266,
	}, 
};

static S_UNI_DEVICE hm2051_unidev={
	.version=0x1,
	.minversion=125,//update if change, as buildid
	.af_type=UNI_DEV_AF_NONE,
	.i2c_addr=0x3C,

	.idreg=0x0000,
	.idval=0x2355,
	.idmask=0xffff,
	.powerdelayms=1,
	.accessdelayms=20,
	.flag=UNI_DEV_FLAG_1B_ACCESS|UNI_DEV_FLAG_USEADGAIN,
	.exposecoarse=0x15,
	.exposeanaloggain=0x18,
	.exposedigitgainR=0x1d,
	.exposedigitgainGR=0,
	.exposedigitgainGB=0,
	.exposedigitgainB=0,
	.exposefine=0,
	.exposefine_mask=0,
	.exposevts=0,
	.exposevts_mask=0,

	.exposecoarse_mask=0xffff,
	.exposeanaloggain_mask=0x03,
	.exposedigitgain_mask=0xff,
	.exposeadgain_param=hm2051_adgain_p,
	.exposeadgain_n=(ARRAY_SIZE(hm2051_adgain_p)),
	.exposeadgain_min=64,
	.exposeadgain_max=127,
	.exposeadgain_fra=64,
	.exposeadgain_step=1,
	.exposeadgain_isp1gain=64,
	.exposeadgain_param1gain=100,

	.global_setting=hm2051_global_setting,
	.global_setting_n=(ARRAY_SIZE(hm2051_global_setting)),
	.stream_off=hm2051_stream_off,
	.stream_off_n=(ARRAY_SIZE(hm2051_stream_off)),
	.stream_on=hm2051_stream_on,
	.stream_on_n=(ARRAY_SIZE(hm2051_stream_on)),
	.group_hold_start=hm2051_group_hold_start,
	.group_hold_start_n=(ARRAY_SIZE(hm2051_group_hold_start)),
	.group_hold_end=hm2051_group_hold_end,
	.group_hold_end_n=(ARRAY_SIZE(hm2051_group_hold_end)),

	.ress=hm2051_res_preview,
	.ress_n=(ARRAY_SIZE(hm2051_res_preview)),
	.coarse_integration_time_min=1,
	.coarse_integration_time_max_margin=8,
	.fine_integration_time_min=0,
	.fine_integration_time_max_margin=6,
	.hw_port=ATOMISP_CAMERA_PORT_SECONDARY,
	.hw_lane=1,
	.hw_format=ATOMISP_INPUT_FORMAT_RAW_10,
	.hw_bayer_order=atomisp_bayer_order_bggr,
	.hw_sensor_type=RAW_CAMERA,

	.hw_reset_gpio=120,
	.hw_reset_gpio_polarity=0,
	.hw_reset_delayms=20,
	.hw_pwd_gpio=124,
	.hw_pwd_gpio_polarity=0,
	.hw_clksource=1,
};
#undef PPR_DEV
#define PPR_DEV hm2051_unidev
