
// auto generate : Thu Aug 14 05:26:49 CST 2014
S_UNI_REG  m1040_stream_on[] = {
	{UNICAM_16BIT,  0x098E, 0xDC00},
	{UNICAM_8BIT,  0xDC00, 0x54},
	{UNICAM_16BIT,  0x0080, 0x8002},
	{UNICAM_TOK_DELAY, 0, 35},
	{UNICAM_16BIT,  0x098E, 0xDC00},
	{UNICAM_8BIT,  0xDC00, 0x28},
	{UNICAM_16BIT,	0x0080, 0x8002},
	{UNICAM_TOK_DELAY, 0, 35},
	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG  m1040_stream_off[] = {
	{UNICAM_16BIT,  0x098E, 0xDC00},
	{UNICAM_8BIT,  0xDC00, 0x50},
	{UNICAM_16BIT,  0x0080, 0x8002},
	{UNICAM_TOK_DELAY, 0, 35},
	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG  m1040_global_setting[] = {
		/* reset */
	{UNICAM_TOK_DELAY, 0, 50},
	{UNICAM_16BIT,  0x301A, 0x0234},
	{UNICAM_TOK_DELAY, 0, 50},
	{UNICAM_16BIT, 0x098E, 0x1000}, // LOGICAL_ADDRESS_ACCESS
	{UNICAM_8BIT, 0xC97E, 0x01},    //cam_sysctl_pll_enable = 1
	{UNICAM_16BIT, 0xC980, 0x0128}, //cam_sysctl_pll_divider_m_n = 276
	{UNICAM_16BIT, 0xC982, 0x0700}, //cam_sysctl_pll_divider_p = 1792
	{UNICAM_16BIT, 0xC800, 0x0000}, //cam_sensor_cfg_y_addr_start = 216
	{UNICAM_16BIT, 0xC802, 0x0000}, //cam_sensor_cfg_x_addr_start = 168
	{UNICAM_16BIT, 0xC804, 0x03CD}, //cam_sensor_cfg_y_addr_end = 761
	{UNICAM_16BIT, 0xC806, 0x050D}, //cam_sensor_cfg_x_addr_end = 1127
	{UNICAM_16BIT, 0xC808, 0x02DC}, //cam_sensor_cfg_pixclk = 24000000
	{UNICAM_16BIT, 0xC80A, 0x6C00},
	{UNICAM_16BIT, 0xC80C, 0x0001}, //cam_sensor_cfg_row_speed = 1
	{UNICAM_16BIT, 0xC80E, 0x01C3}, //cam_sensor_cfg_fine_integ_time_min = 219
	{UNICAM_16BIT, 0xC810, 0x03F7}, //cam_sensor_cfg_fine_integ_time_max = 1149
	{UNICAM_16BIT, 0xC812, 0x0500}, //cam_sensor_cfg_frame_length_lines = 625
	{UNICAM_16BIT, 0xC814, 0x04E2}, //cam_sensor_cfg_line_length_pck = 1280
	{UNICAM_16BIT, 0xC816, 0x00E0}, //cam_sensor_cfg_fine_correction = 96
	{UNICAM_16BIT, 0xC818, 0x01E3}, //cam_sensor_cfg_cpipe_last_row = 541
	{UNICAM_16BIT, 0xC826, 0x0020}, //cam_sensor_cfg_reg_0_data = 32
	{UNICAM_16BIT, 0xC834, 0x0330}, //cam_sensor_control_read_mode = 0
	{UNICAM_16BIT, 0xC854, 0x0000}, //cam_crop_window_xoffset = 0
	{UNICAM_16BIT, 0xC856, 0x0000}, //cam_crop_window_yoffset = 0
	{UNICAM_16BIT, 0xC858, 0x0280}, //cam_crop_window_width = 952
	{UNICAM_16BIT, 0xC85A, 0x01E0}, //cam_crop_window_height = 538
	{UNICAM_8BIT, 0xC85C, 0x03},    //cam_crop_cropmode = 3
	{UNICAM_16BIT, 0xC868, 0x0280}, //cam_output_width = 952
	{UNICAM_16BIT, 0xC86A, 0x01E0}, //cam_output_height = 538
	//LOAD = Step3-Recommended     //Patch,Errata and Sensor optimization Setting
	{UNICAM_16BIT, 0x316A, 0x8270}, // DAC_TXLO_ROW
	{UNICAM_16BIT, 0x316C, 0x8270}, // DAC_TXLO
	{UNICAM_16BIT, 0x3ED0, 0x2305}, // DAC_LD_4_5
	{UNICAM_16BIT, 0x3ED2, 0x77CF}, // DAC_LD_6_7
	{UNICAM_16BIT, 0x316E, 0x8202}, // DAC_ECL
	{UNICAM_16BIT, 0x3180, 0x87FF}, // DELTA_DK_CONTROL
	{UNICAM_16BIT, 0x30D4, 0x6080}, // COLUMN_CORRECTION
	{UNICAM_16BIT, 0xA802, 0x0008}, // AE_TRACK_MODE
	{UNICAM_16BIT, 0x3E14, 0xFF39}, // SAMP_COL_PUP2
	{UNICAM_16BIT, 0x31E0, 0x0003}, // PIX_DEF_ID
	//LOAD = Step8-Features		//Ports, special features, etc.
	{UNICAM_16BIT, 0x098E, 0x0000}, // LOGICAL_ADDRESS_ACCESS
	{UNICAM_16BIT, 0x001E, 0x0777}, // PAD_SLEW
	{UNICAM_16BIT, 0x098E, 0x0000}, // LOGICAL_ADDRESS_ACCESS
	{UNICAM_16BIT, 0xC984, 0x8001}, // CAM_PORT_OUTPUT_CONTROL
	{UNICAM_16BIT, 0xC988, 0x0F00}, // CAM_PORT_MIPI_TIMING_T_HS_ZERO
	{UNICAM_16BIT, 0xC98A, 0x0B07}, // CAM_PORT_MIPI_TIMING_T_HS_EXIT_HS_TRAIL
	{UNICAM_16BIT, 0xC98C, 0x0D01}, // CAM_PORT_MIPI_TIMING_T_CLK_POST_CLK_PRE
	{UNICAM_16BIT, 0xC98E, 0x071D}, // CAM_PORT_MIPI_TIMING_T_CLK_TRAIL_CLK_ZERO
	{UNICAM_16BIT, 0xC990, 0x0006}, // CAM_PORT_MIPI_TIMING_T_LPX
	{UNICAM_16BIT, 0xC992, 0x0A0C}, // CAM_PORT_MIPI_TIMING_INIT_TIMING
	{UNICAM_16BIT, 0x3C5A, 0x0009}, // MIPI_DELAY_TRIM
	{UNICAM_16BIT, 0xC86C, 0x0210}, // CAM_OUTPUT_FORMAT
	{UNICAM_16BIT, 0xA804, 0x0000}, // AE_TRACK_ALGO
	//default exposure
	{UNICAM_16BIT, 0x3012, 0x0110}, // COMMAND_REGISTER
	{UNICAM_16BIT,	0x098E, 0xDC00},
	{UNICAM_8BIT,  0xDC00, 0x28},
	{UNICAM_16BIT,	0x0080, 0x8002},
	{UNICAM_TOK_DELAY, 0, 40},
	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG  m1040_group_hold_start[] = {
	{UNICAM_16BIT, 0x301A, 0x8234}, //group start
	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG  m1040_group_hold_end[] = {
	{UNICAM_16BIT, 0x301A, 0x0234}, //group end
	{UNICAM_TOK_TERM, 0, 0}
};
static  int m1040_adgain_p[] = {
	0,
};
S_UNI_REG m1040_976P[] = {
	{UNICAM_16BIT,  0x098E, 0xDC00},
	{UNICAM_8BIT,  0xDC00, 0x50},
	{UNICAM_16BIT,  0x0080, 0x8002},
	{UNICAM_TOK_DELAY, 0, 10},
	{UNICAM_16BIT, 0x98E, 0x1000},
	{UNICAM_16BIT, 0x300A, 0x03EE}, //vts
	{UNICAM_16BIT, 0xC800, 0x0000}, //cam_sensor_cfg_y_addr_start = 0
	{UNICAM_16BIT, 0xC802, 0x0000}, //cam_sensor_cfg_x_addr_start = 0
	{UNICAM_16BIT, 0xC804, 0x03CF}, //cam_sensor_cfg_y_addr_end = 975
	{UNICAM_16BIT, 0xC806, 0x050F}, //cam_sensor_cfg_x_addr_end = 1295
	{UNICAM_16BIT, 0xC808, 0x02DC}, //cam_sensor_cfg_pixclk = 480000
	{UNICAM_16BIT, 0xC80A, 0x6C00}, //cam_sensor_cfg_pixclk = 480000
	{UNICAM_16BIT, 0xC80C, 0x0001}, //cam_sensor_cfg_row_speed = 1
	{UNICAM_16BIT, 0xC80E, 0x00DB}, //cam_sensor_cfg_fine_integ_time_min = 219
	{UNICAM_16BIT, 0xC810, 0x05B3}, //0x062E //cam_sensor_cfg_fine_integ_time_max = 1459
	{UNICAM_16BIT, 0xC812, 0x03EE}, //0x074C //cam_sensor_cfg_frame_length_lines = 1006
	{UNICAM_16BIT, 0xC814, 0x0644}, //0x06B1 /cam_sensor_cfg_line_length_pck = 1590
	{UNICAM_16BIT, 0xC816, 0x0060}, //cam_sensor_cfg_fine_correction = 96
	{UNICAM_16BIT, 0xC818, 0x03C3}, //cam_sensor_cfg_cpipe_last_row = 963
	{UNICAM_16BIT, 0xC826, 0x0020}, //cam_sensor_cfg_reg_0_data = 32
	{UNICAM_16BIT, 0xC834, 0x0000}, //cam_sensor_control_read_mode = 0
	{UNICAM_16BIT, 0xC854, 0x0000}, //cam_crop_window_xoffset = 0
	{UNICAM_16BIT, 0xC856, 0x0000}, //cam_crop_window_yoffset = 0
	{UNICAM_16BIT, 0xC858, 0x0508}, //cam_crop_window_width = 1288
	{UNICAM_16BIT, 0xC85A, 0x03C8}, //cam_crop_window_height = 968
	{UNICAM_8BIT, 0xC85C, 0x03}, //cam_crop_cropmode = 3
	{UNICAM_16BIT, 0xC868, 0x0508}, //cam_output_width = 1288
	{UNICAM_16BIT, 0xC86A, 0x03C8}, //cam_output_height = 968
	{UNICAM_8BIT, 0xC878, 0x00}, //0x0E //cam_aet_aemode = 0
	{UNICAM_16BIT,	0x098E, 0xDC00},
	{UNICAM_8BIT,  0xDC00, 0x54},
	{UNICAM_16BIT,	0x0080, 0x8002},
	{UNICAM_TOK_DELAY, 0, 35},
	{UNICAM_16BIT,  0x098E, 0xDC00},
	{UNICAM_8BIT,  0xDC00, 0x28},
	{UNICAM_16BIT,	0x0080, 0x8002},
	{UNICAM_TOK_DELAY, 0, 35},
	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG m1040_736P[] = {
	{UNICAM_16BIT,  0x098E, 0xDC00},
	{UNICAM_8BIT,  0xDC00, 0x50},
	{UNICAM_16BIT,  0x0080, 0x8002},
	{UNICAM_TOK_DELAY, 0, 10},
	{UNICAM_16BIT, 0x98E, 0x1000},
	{UNICAM_16BIT, 0x300A, 0x03EE}, //cam_sensor_cfg_y_addr_start = 120
	{UNICAM_16BIT, 0xC800, 0x0078}, //cam_sensor_cfg_y_addr_start = 120
	{UNICAM_16BIT, 0xC802, 0x0000}, //cam_sensor_cfg_x_addr_start = 0
	{UNICAM_16BIT, 0xC804, 0x0357}, //cam_sensor_cfg_y_addr_end = 855
	{UNICAM_16BIT, 0xC806, 0x050F}, //cam_sensor_cfg_x_addr_end = 1295
	{UNICAM_16BIT, 0xC808, 0x0237}, //cam_sensor_cfg_pixclk = 37199999
	{UNICAM_16BIT, 0xC80A, 0xA07F}, //cam_sensor_cfg_pixclk = 37199999
	{UNICAM_16BIT, 0xC80C, 0x0001}, //cam_sensor_cfg_row_speed = 1
	{UNICAM_16BIT, 0xC80E, 0x00DB}, //cam_sensor_cfg_fine_integ_time_min = 219
	{UNICAM_16BIT, 0xC810, 0x05BD}, //0x062E //cam_sensor_cfg_fine_integ_time_max = 1469
	{UNICAM_16BIT, 0xC812, 0x03EE}, //0x074C //cam_sensor_cfg_frame_length_lines = 775
	{UNICAM_16BIT, 0xC814, 0x0640}, //0x06B1 /cam_sensor_cfg_line_length_pck = 1600
	{UNICAM_16BIT, 0xC816, 0x0060}, //cam_sensor_cfg_fine_correction = 96
	{UNICAM_16BIT, 0xC818, 0x02DB}, //cam_sensor_cfg_cpipe_last_row = 731
	{UNICAM_16BIT, 0xC826, 0x0020}, //cam_sensor_cfg_reg_0_data = 32
	{UNICAM_16BIT, 0xC834, 0x0000}, //cam_sensor_control_read_mode = 0
	{UNICAM_16BIT, 0xC854, 0x0000}, //cam_crop_window_xoffset = 0
	{UNICAM_16BIT, 0xC856, 0x0000}, //cam_crop_window_yoffset = 0
	{UNICAM_16BIT, 0xC858, 0x0508}, //cam_crop_window_width = 1288
	{UNICAM_16BIT, 0xC85A, 0x02D8}, //cam_crop_window_height = 728
	{UNICAM_8BIT, 0xC85C, 0x03}, //cam_crop_cropmode = 3
	{UNICAM_16BIT, 0xC868, 0x0508}, //cam_output_width = 1288
	{UNICAM_16BIT, 0xC86A, 0x02D8}, //cam_output_height = 728
	{UNICAM_8BIT, 0xC878, 0x00}, //0x0E //cam_aet_aemode = 0
	{UNICAM_16BIT,  0x098E, 0xDC00},
	{UNICAM_8BIT,  0xDC00, 0x54},
	{UNICAM_16BIT,  0x0080, 0x8002},
	{UNICAM_TOK_DELAY, 0, 10},
	{UNICAM_16BIT,  0x098E, 0xDC00},
	{UNICAM_8BIT,  0xDC00, 0x28},
	{UNICAM_16BIT,	0x0080, 0x8002},
	{UNICAM_TOK_DELAY, 0, 10},
	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG m1040_regress2[] = {
	
	{UNICAM_TOK_TERM, 0, 0}
};
#if 0
S_UNI_REG m1040_regress[][] = {
	m1040_regress0,
	m1040_regress1,
	m1040_regress2,
};
#endif
static S_UNI_RESOLUTION m1040_res_preview[] = {
	{
		.desc = "m1040_736P",
		.res_type = UNI_DEV_RES_PREVIEW|UNI_DEV_RES_VIDEO|UNI_DEV_RES_STILL,
		.width = 1296,
		.height = 736,
		.fps = 0,
		.pixels_per_line = 0x640,
		.lines_per_frame = 0x03EE,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 3,
		.regs = m1040_736P,
		.regs_n= (ARRAY_SIZE(m1040_736P)),
		
		.vt_pix_clk_freq_mhz=37199999,
		.crop_horizontal_start=0,
		.crop_vertical_start=0,
		.crop_horizontal_end=1295,
		.crop_vertical_end=735,
		.output_width=1296,
		.output_height=736,
		.used = 0,

		.vts_fix=2,
		.vts=0x03EE,
		
	},
	{
		.desc = "m1040_976P",
		.res_type = UNI_DEV_RES_PREVIEW|UNI_DEV_RES_VIDEO|UNI_DEV_RES_STILL,
		.width = 1296,
		.height = 976,
		.fps = 30,
		.pixels_per_line = 0x0644,
		.lines_per_frame = 0x03EE,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 3,
		.regs = m1040_976P,
		.regs_n= (ARRAY_SIZE(m1040_976P)),

		.vt_pix_clk_freq_mhz=48000000,
		.crop_horizontal_start=0,
		.crop_vertical_start=0,
		.crop_horizontal_end=1295,
		.crop_vertical_end=975,
		.output_width=1296,
		.output_height=976,
		.used = 0,

		.vts_fix=2,
		.vts=0x03EE,
		
	},
};
static S_UNI_DEVICE m1040_unidev={
	.version=0x1,
	.minversion=125,//update if change, as buildid
	.af_type=UNI_DEV_AF_NONE,
	.i2c_addr=0x48,

	.idreg=0,
	.idval=0x2481,
	.idmask=0xffff,
	.powerdelayms=1,
	.accessdelayms=20,
	.flag=0,
	.exposecoarse=0x3012,
	.exposeanaloggain=0x305E,
	.exposedigitgainR=0,
	.exposedigitgainGR=0,
	.exposedigitgainGB=0,
	.exposedigitgainB=0,
	.exposefine=0,
	.exposefine_mask=0,
	.exposevts=0x300A,
	.exposevts_mask=0xffff,

	.exposecoarse_mask=0xffff,
	.exposeanaloggain_mask=0x1ff,
	.exposedigitgain_mask=0,
	.exposeadgain_param=m1040_adgain_p,
	.exposeadgain_n=(ARRAY_SIZE(m1040_adgain_p)),
	.exposeadgain_min=0,
	.exposeadgain_max=0,
	.exposeadgain_fra=0,
	.exposeadgain_step=0,
	.exposeadgain_isp1gain=0,
	.exposeadgain_param1gain=0,
 

	.global_setting=m1040_global_setting,
	.global_setting_n=(ARRAY_SIZE(m1040_global_setting)),
	.stream_off=m1040_stream_off,
	.stream_off_n=(ARRAY_SIZE(m1040_stream_off)),
	.stream_on=m1040_stream_on,
	.stream_on_n=(ARRAY_SIZE(m1040_stream_on)),
	.group_hold_start=m1040_group_hold_start,
	.group_hold_start_n=(ARRAY_SIZE(m1040_group_hold_start)),
	.group_hold_end=m1040_group_hold_end,
	.group_hold_end_n=(ARRAY_SIZE(m1040_group_hold_end)),


	.ress=m1040_res_preview,
	.ress_n=(ARRAY_SIZE(m1040_res_preview)),
	.coarse_integration_time_min=0,
	.coarse_integration_time_max_margin=8,
	.fine_integration_time_min=0,
	.fine_integration_time_max_margin=65535,
	.hw_port=ATOMISP_CAMERA_PORT_SECONDARY,
	.hw_lane=1,
	.hw_format=ATOMISP_INPUT_FORMAT_RAW_10,
	.hw_bayer_order=atomisp_bayer_order_grbg,
	.hw_sensor_type=RAW_CAMERA,

	.hw_reset_gpio=120,
	.hw_reset_gpio_polarity=0,
	.hw_reset_delayms=20,
	.hw_pwd_gpio=124,
	.hw_pwd_gpio_polarity=1,
	.hw_clksource=1,
	
};
#undef PPR_DEV
#define PPR_DEV m1040_unidev
