
// auto generate : Fri Sep 26 15:49:18 CST 2014
S_UNI_REG  gc0310_stream_on[] = {
	{UNICAM_8BIT, 0xfe, 0x30},	
	{UNICAM_8BIT, 0xfe, 0x03},	
	{UNICAM_8BIT, 0x10, 0x94},
	{UNICAM_8BIT, 0xfe, 0x00},	
	{UNICAM_TOK_TERM, 0, 0},
};

S_UNI_REG  gc0310_stream_off[] = {
	{UNICAM_8BIT, 0xfe, 0x03},	
	{UNICAM_8BIT, 0x10, 0x84},	
	{UNICAM_8BIT, 0xfe, 0x00},	
	{UNICAM_TOK_TERM, 0, 0},
};

S_UNI_REG  gc0310_global_setting[] = { 
	{UNICAM_TOK_TERM, 0, 0} 
};
S_UNI_REG  gc0310_group_hold_start[] = {
	{UNICAM_8BIT, 0xfe, 0x00},
	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG  gc0310_group_hold_end[] = {
	{UNICAM_TOK_TERM, 0, 0}
};

static  int gc0310_adgain_p[] = {
	0,
};

S_UNI_REG gc0310_640x480_30fps[] = {

	{ UNICAM_8BIT, 0xfe, 0xf0 }, 
	{ UNICAM_8BIT, 0xfe, 0xf0 }, 
	{ UNICAM_8BIT, 0xfe, 0x00 }, 
	{ UNICAM_8BIT, 0xfc, 0x0e }, 
	{ UNICAM_8BIT, 0xfc, 0x0e }, 
	{ UNICAM_8BIT, 0xf2, 0x80 }, 
	{ UNICAM_8BIT, 0xf3, 0x00 }, 

	{ UNICAM_8BIT, 0xf7, 0x1b }, 
	{ UNICAM_8BIT, 0xf8, 0x05 }, 

	{ UNICAM_8BIT, 0xf9, 0x8e }, 
	{ UNICAM_8BIT, 0xfa, 0x11 }, 
	{ UNICAM_8BIT, 0xfe, 0x03 }, 
	{ UNICAM_8BIT, 0x40, 0x08 }, 
	{ UNICAM_8BIT, 0x42, 0x00 }, 
	{ UNICAM_8BIT, 0x43, 0x00 }, 
	{ UNICAM_8BIT, 0x01, 0x03 }, 
	{ UNICAM_8BIT, 0x10, 0x84 }, 

	{ UNICAM_8BIT, 0x01, 0x03 }, 
	{ UNICAM_8BIT, 0x02, 0x22 }, 
	{ UNICAM_8BIT, 0x03, 0x94 }, 
	{ UNICAM_8BIT, 0x04, 0x01 }, 
	{ UNICAM_8BIT, 0x05, 0x00 }, 
	{ UNICAM_8BIT, 0x06, 0x80 }, 
	{ UNICAM_8BIT, 0x11, 0x1e }, 
	{ UNICAM_8BIT, 0x12, 0x00 }, 
	{ UNICAM_8BIT, 0x13, 0x05 }, 
	{ UNICAM_8BIT, 0x15, 0x12 }, 
	{ UNICAM_8BIT, 0x17, 0xf0 }, 

	{ UNICAM_8BIT, 0x21, 0x02 }, 
	{ UNICAM_8BIT, 0x22, 0x02 }, 
	{ UNICAM_8BIT, 0x23, 0x01 }, 
	{ UNICAM_8BIT, 0x29, 0x02 }, 
	{ UNICAM_8BIT, 0x2a, 0x02 }, 

	{ UNICAM_8BIT, 0xfe, 0x00 }, 


	{ UNICAM_8BIT, 0x00, 0x2f }, 
	{ UNICAM_8BIT, 0x01, 0x0f }, 
	{ UNICAM_8BIT, 0x02, 0x04 }, 
	{ UNICAM_8BIT, 0x03, 0x02 }, 
	{ UNICAM_8BIT, 0x04, 0xee }, 
	{ UNICAM_8BIT, 0x09, 0x00 }, 
	{ UNICAM_8BIT, 0x0a, 0x00 }, 
	{ UNICAM_8BIT, 0x0b, 0x00 }, 
	{ UNICAM_8BIT, 0x0c, 0x04 }, 
	{ UNICAM_8BIT, 0x0d, 0x01 }, 
	{ UNICAM_8BIT, 0x0e, 0xf2 }, //e8
	{ UNICAM_8BIT, 0x0f, 0x02 }, 
	{ UNICAM_8BIT, 0x10, 0x96 }, //88
	{ UNICAM_8BIT, 0x16, 0x00 }, 
	{ UNICAM_8BIT, 0x17, 0x14 }, 
	{ UNICAM_8BIT, 0x18, 0x1a }, 
	{ UNICAM_8BIT, 0x19, 0x14 }, 
	{ UNICAM_8BIT, 0x1b, 0x48 }, 
	{ UNICAM_8BIT, 0x1e, 0x6b }, 
	{ UNICAM_8BIT, 0x1f, 0x28 }, 
	{ UNICAM_8BIT, 0x20, 0x8b }, 
	{ UNICAM_8BIT, 0x21, 0x49 }, 
	{ UNICAM_8BIT, 0x22, 0xb0 }, 
	{ UNICAM_8BIT, 0x23, 0x04 }, 
	{ UNICAM_8BIT, 0x24, 0x16 }, 
	{ UNICAM_8BIT, 0x34, 0x20 }, 

	{ UNICAM_8BIT, 0x26, 0x23 }, 
	{ UNICAM_8BIT, 0x28, 0xff }, 
	{ UNICAM_8BIT, 0x29, 0x00 }, 
	{ UNICAM_8BIT, 0x33, 0x10 }, 
	{ UNICAM_8BIT, 0x37, 0x20 }, 
	{ UNICAM_8BIT, 0x38, 0x10 }, 
	{ UNICAM_8BIT, 0x47, 0x80 }, 
	{ UNICAM_8BIT, 0x4e, 0x66 }, 
	{ UNICAM_8BIT, 0xa8, 0x02 }, 
	{ UNICAM_8BIT, 0xa9, 0x80 }, 

	{ UNICAM_8BIT, 0x40, 0xff }, 
	{ UNICAM_8BIT, 0x41, 0x25 }, 
	{ UNICAM_8BIT, 0x42, 0xcf }, 
	{ UNICAM_8BIT, 0x44, 0x00 }, 
	{ UNICAM_8BIT, 0x45, 0xaf }, 
	{ UNICAM_8BIT, 0x46, 0x02 }, 
	{ UNICAM_8BIT, 0x4a, 0x11 }, 
	{ UNICAM_8BIT, 0x4b, 0x01 }, 
	{ UNICAM_8BIT, 0x4c, 0x20 }, 
	{ UNICAM_8BIT, 0x4d, 0x05 }, 
	{ UNICAM_8BIT, 0x4f, 0x01 }, 
	{ UNICAM_8BIT, 0x50, 0x01 }, 
	{ UNICAM_8BIT, 0x55, 0x01 }, 
	{ UNICAM_8BIT, 0x56, 0xe0 }, //e0
	{ UNICAM_8BIT, 0x57, 0x02 }, 
	{ UNICAM_8BIT, 0x58, 0x80 }, //80

	{ UNICAM_8BIT, 0x70, 0x70 }, 
	{ UNICAM_8BIT, 0x5a, 0x84 }, 
	{ UNICAM_8BIT, 0x5b, 0xc9 }, 
	{ UNICAM_8BIT, 0x5c, 0xed }, 
	{ UNICAM_8BIT, 0x77, 0x71 }, 
	{ UNICAM_8BIT, 0x78, 0x4c }, 
	{ UNICAM_8BIT, 0x79, 0x50 }, 

	{ UNICAM_8BIT, 0x82, 0x14 }, 
	{ UNICAM_8BIT, 0x83, 0x0b }, 
	{ UNICAM_8BIT, 0x89, 0xf0 }, 

	{ UNICAM_8BIT, 0x8f, 0xaa }, 
	{ UNICAM_8BIT, 0x90, 0x8c }, 
	{ UNICAM_8BIT, 0x91, 0x90 }, 
	{ UNICAM_8BIT, 0x92, 0x03 }, 
	{ UNICAM_8BIT, 0x93, 0x03 }, 
	{ UNICAM_8BIT, 0x94, 0x05 }, 
	{ UNICAM_8BIT, 0x95, 0x65 }, 
	{ UNICAM_8BIT, 0x96, 0xf0 }, 

	{ UNICAM_8BIT, 0xfe, 0x00 }, 
	{ UNICAM_8BIT, 0x9a, 0x20 }, 
	{ UNICAM_8BIT, 0x9b, 0xa0 },//0x80 
	{ UNICAM_8BIT, 0x9c, 0x40 }, 
	{ UNICAM_8BIT, 0x9d, 0x80 }, 
	{ UNICAM_8BIT, 0xa1, 0x30 }, 
	{ UNICAM_8BIT, 0xa2, 0x32 }, 
	{ UNICAM_8BIT, 0xa4, 0x30 }, 
	{ UNICAM_8BIT, 0xa5, 0x30 }, 
	{ UNICAM_8BIT, 0xaa, 0x10 }, 
	{ UNICAM_8BIT, 0xac, 0x66 },//22 
	 
	{ UNICAM_8BIT, 0xfe, 0x00 }, 
	{ UNICAM_8BIT, 0xbf, 0x0b }, 
	{ UNICAM_8BIT, 0xc0, 0x17 }, 
	{ UNICAM_8BIT, 0xc1, 0x2a }, 
	{ UNICAM_8BIT, 0xc2, 0x41 }, 
	{ UNICAM_8BIT, 0xc3, 0x54 }, 
	{ UNICAM_8BIT, 0xc4, 0x66 }, 
	{ UNICAM_8BIT, 0xc5, 0x74 }, 
	{ UNICAM_8BIT, 0xc6, 0x8c }, 
	{ UNICAM_8BIT, 0xc7, 0xa3 }, 
	{ UNICAM_8BIT, 0xc8, 0xb5 }, 
	{ UNICAM_8BIT, 0xc9, 0xc4 }, 
	{ UNICAM_8BIT, 0xca, 0xd0 }, 
	{ UNICAM_8BIT, 0xcb, 0xdb }, 
	{ UNICAM_8BIT, 0xcc, 0xe5 }, 
	{ UNICAM_8BIT, 0xcd, 0xf0 }, 
	{ UNICAM_8BIT, 0xce, 0xf7 }, 
	{ UNICAM_8BIT, 0xcf, 0xff }, 

	{ UNICAM_8BIT, 0xd0, 0x40 }, 
	{ UNICAM_8BIT, 0xd1, 0x27 }, 
	{ UNICAM_8BIT, 0xd2, 0x27 }, 
	{ UNICAM_8BIT, 0xd3, 0x40 }, 
	{ UNICAM_8BIT, 0xd6, 0xf2 }, 
	{ UNICAM_8BIT, 0xd7, 0x1b }, 
	{ UNICAM_8BIT, 0xd8, 0x18 }, 
	{ UNICAM_8BIT, 0xdd, 0x03 }, 

	{ UNICAM_8BIT, 0xde, 0xe5 }, 
	{ UNICAM_8BIT, 0xfe, 0x01 }, 
	{ UNICAM_8BIT, 0x1e, 0x41 }, 	
	{ UNICAM_8BIT, 0x58, 0x04 }, 
	{ UNICAM_8BIT, 0x05, 0x30 }, 
	{ UNICAM_8BIT, 0x06, 0x75 }, 
	{ UNICAM_8BIT, 0x07, 0x40 }, 
	{ UNICAM_8BIT, 0x08, 0xb0 }, 
	{ UNICAM_8BIT, 0x0a, 0x85 }, //c5
	{ UNICAM_8BIT, 0x0b, 0x11 }, 
	{ UNICAM_8BIT, 0x0c, 0x00 }, 

	{ UNICAM_8BIT, 0x12, 0x52 }, 
	{ UNICAM_8BIT, 0x13, 0x88 }, //90
	{ UNICAM_8BIT, 0x18, 0x95 }, 
	{ UNICAM_8BIT, 0x19, 0x96 }, 
	{ UNICAM_8BIT, 0x1f, 0x30 }, 
	{ UNICAM_8BIT, 0x20, 0xc0 }, 
	{ UNICAM_8BIT, 0x3e, 0x40 }, 
	{ UNICAM_8BIT, 0x3f, 0x57 }, 
	{ UNICAM_8BIT, 0x40, 0x7d }, 
	{ UNICAM_8BIT, 0x03, 0x60 }, 
	{ UNICAM_8BIT, 0x44, 0x02 }, 

	{ UNICAM_8BIT, 0x1c, 0x91 }, 
	{ UNICAM_8BIT, 0x21, 0x15 }, 
	{ UNICAM_8BIT, 0x50, 0x80 }, 
	{ UNICAM_8BIT, 0x56, 0x00 },//04 
	{ UNICAM_8BIT, 0x59, 0x08 }, 
	{ UNICAM_8BIT, 0x5b, 0x04 },//02 
	{ UNICAM_8BIT, 0x61, 0x8d }, 
	{ UNICAM_8BIT, 0x62, 0xa7 }, 
	{ UNICAM_8BIT, 0x63, 0xd0 }, 
	{ UNICAM_8BIT, 0x65, 0x06 }, 
	{ UNICAM_8BIT, 0x66, 0x06 }, 
	{ UNICAM_8BIT, 0x67, 0x84 }, 
	{ UNICAM_8BIT, 0x69, 0x08 }, 
	{ UNICAM_8BIT, 0x6a, 0x25 }, 
	{ UNICAM_8BIT, 0x6b, 0x01 }, 
	{ UNICAM_8BIT, 0x6c, 0x00 }, 
	{ UNICAM_8BIT, 0x6d, 0x02 }, 
	{ UNICAM_8BIT, 0x6e, 0xf0 }, //f0 //e0
	{ UNICAM_8BIT, 0x6f, 0x40 }, 
	{ UNICAM_8BIT, 0x76, 0x80 }, 
	{ UNICAM_8BIT, 0x78, 0xdf }, 
	{ UNICAM_8BIT, 0x79, 0x75 }, 
	{ UNICAM_8BIT, 0x7a, 0x40 }, 
	{ UNICAM_8BIT, 0x7b, 0x70 }, 
	{ UNICAM_8BIT, 0x7c, 0x0c }, 

	{ UNICAM_8BIT, 0xfe, 0x01 }, 
	{ UNICAM_8BIT, 0x90, 0x00 }, 
	{ UNICAM_8BIT, 0x91, 0x00 }, 
	{ UNICAM_8BIT, 0x92, 0xf5 }, 
	{ UNICAM_8BIT, 0x93, 0xde }, 
	{ UNICAM_8BIT, 0x95, 0x0b }, 
	{ UNICAM_8BIT, 0x96, 0xf5 }, 
	{ UNICAM_8BIT, 0x97, 0x44 }, 
	{ UNICAM_8BIT, 0x98, 0x1f }, 
	{ UNICAM_8BIT, 0x9a, 0x4b }, 
	{ UNICAM_8BIT, 0x9b, 0x1f }, 
	{ UNICAM_8BIT, 0x9c, 0x7e }, 
	{ UNICAM_8BIT, 0x9d, 0x4b }, 
	{ UNICAM_8BIT, 0x9f, 0x8e }, 
	{ UNICAM_8BIT, 0xa0, 0x7e }, 
	{ UNICAM_8BIT, 0xa1, 0x00 }, 
	{ UNICAM_8BIT, 0xa2, 0x00 }, 
	{ UNICAM_8BIT, 0x86, 0x00 }, 
	{ UNICAM_8BIT, 0x87, 0x00 }, 
	{ UNICAM_8BIT, 0x88, 0x00 }, 
	{ UNICAM_8BIT, 0x89, 0x00 }, 
	{ UNICAM_8BIT, 0xa4, 0x00 }, 
	{ UNICAM_8BIT, 0xa5, 0x00 }, 
	{ UNICAM_8BIT, 0xa6, 0xc4 }, 
	{ UNICAM_8BIT, 0xa7, 0x99 }, 
	{ UNICAM_8BIT, 0xa9, 0xc6 }, 
	{ UNICAM_8BIT, 0xaa, 0x9a }, 
	{ UNICAM_8BIT, 0xab, 0xab }, 
	{ UNICAM_8BIT, 0xac, 0x94 }, 
	{ UNICAM_8BIT, 0xae, 0xc7 }, 
	{ UNICAM_8BIT, 0xaf, 0xa8 }, 
	{ UNICAM_8BIT, 0xb0, 0xc6 }, 
	{ UNICAM_8BIT, 0xb1, 0xab }, 
	{ UNICAM_8BIT, 0xb3, 0xca }, 
	{ UNICAM_8BIT, 0xb4, 0xac }, 
	{ UNICAM_8BIT, 0xb5, 0x00 }, 
	{ UNICAM_8BIT, 0xb6, 0x00 }, 
	{ UNICAM_8BIT, 0x8b, 0x00 }, 
	{ UNICAM_8BIT, 0x8c, 0x00 }, 
	{ UNICAM_8BIT, 0x8d, 0x00 }, 
	{ UNICAM_8BIT, 0x8e, 0x00 }, 
	{ UNICAM_8BIT, 0x94, 0x50 }, 
	{ UNICAM_8BIT, 0x99, 0xa6 }, 
	{ UNICAM_8BIT, 0x9e, 0xaa }, 
	{ UNICAM_8BIT, 0xa3, 0x0a }, 
	{ UNICAM_8BIT, 0x8a, 0x00 }, 
	{ UNICAM_8BIT, 0xa8, 0x50 }, 
	{ UNICAM_8BIT, 0xad, 0x55 }, 
	{ UNICAM_8BIT, 0xb2, 0x55 }, 
	{ UNICAM_8BIT, 0xb7, 0x05 }, 
	{ UNICAM_8BIT, 0x8f, 0x00 }, 
	{ UNICAM_8BIT, 0xb8, 0xd5 }, 
	{ UNICAM_8BIT, 0xb9, 0x8d }, 

	{ UNICAM_8BIT, 0xfe, 0x01 }, 
	{ UNICAM_8BIT, 0xd0, 0x3d }, 
	{ UNICAM_8BIT, 0xd1, 0xf0 }, //fc
	{ UNICAM_8BIT, 0xd2, 0x02 }, 
	{ UNICAM_8BIT, 0xd3, 0x02 }, 
	{ UNICAM_8BIT, 0xd4, 0x58 }, 
	{ UNICAM_8BIT, 0xd5, 0xf0 }, //ee
	{ UNICAM_8BIT, 0xd6, 0x44 }, 
	{ UNICAM_8BIT, 0xd7, 0xf0 }, 
	{ UNICAM_8BIT, 0xd8, 0xf8 }, 
	{ UNICAM_8BIT, 0xd9, 0xf8 }, 
	{ UNICAM_8BIT, 0xda, 0x46 }, 
	{ UNICAM_8BIT, 0xdb, 0xe4 }, 

	{ UNICAM_8BIT, 0xfe, 0x01 }, 
	{ UNICAM_8BIT, 0xc1, 0x3c }, 
	{ UNICAM_8BIT, 0xc2, 0x50 }, 
	{ UNICAM_8BIT, 0xc3, 0x00 }, 
	{ UNICAM_8BIT, 0xc4, 0x40 },//0x58 
	{ UNICAM_8BIT, 0xc5, 0x20 },//0x30 
	{ UNICAM_8BIT, 0xc6, 0x1b },//0x30
	{ UNICAM_8BIT, 0xc7, 0x10 }, 
	{ UNICAM_8BIT, 0xc8, 0x00 }, 
	{ UNICAM_8BIT, 0xc9, 0x00 }, 
	{ UNICAM_8BIT, 0xdc, 0x20 }, 
	{ UNICAM_8BIT, 0xdd, 0x10 }, 
	{ UNICAM_8BIT, 0xdf, 0x00 }, 
	{ UNICAM_8BIT, 0xde, 0x00 }, 

	{ UNICAM_8BIT, 0x01, 0x10 }, 
	{ UNICAM_8BIT, 0x0b, 0x31 }, 
	{ UNICAM_8BIT, 0x0e, 0x50 }, 
	{ UNICAM_8BIT, 0x0f, 0x0f }, 
	{ UNICAM_8BIT, 0x10, 0x6e }, 
	{ UNICAM_8BIT, 0x12, 0xa0 }, 
	{ UNICAM_8BIT, 0x15, 0x60 }, 
	{ UNICAM_8BIT, 0x16, 0x60 }, 
	{ UNICAM_8BIT, 0x17, 0xe0 }, 

	{ UNICAM_8BIT, 0xcc, 0x0c }, 
	{ UNICAM_8BIT, 0xcd, 0x10 }, 
	{ UNICAM_8BIT, 0xce, 0xa0 }, 
	{ UNICAM_8BIT, 0xcf, 0xe6 }, 

	{ UNICAM_8BIT, 0x45, 0xf7 }, 
	{ UNICAM_8BIT, 0x46, 0xff }, 
	{ UNICAM_8BIT, 0x47, 0x15 }, 
	{ UNICAM_8BIT, 0x48, 0x03 }, 
	{ UNICAM_8BIT, 0x4f, 0x60 }, 


	{ UNICAM_8BIT, 0xfe, 0x00 }, 

	{ UNICAM_8BIT, 0x05, 0x01 }, 
	{ UNICAM_8BIT, 0x06, 0xe2 }, 
	{ UNICAM_8BIT, 0x07, 0x00 }, 
	{ UNICAM_8BIT, 0x08, 0x6b }, 


	{ UNICAM_8BIT, 0xfe, 0x01 }, 
	{ UNICAM_8BIT, 0x25, 0x00 }, 
	{ UNICAM_8BIT, 0x26, 0x79 }, 


	{ UNICAM_8BIT, 0x27, 0x02 }, 
	{ UNICAM_8BIT, 0x28, 0x5d }, 
	{ UNICAM_8BIT, 0x29, 0x03 }, 
	{ UNICAM_8BIT, 0x2a, 0xc8 }, 
	{ UNICAM_8BIT, 0x2b, 0x05 }, //05
	{ UNICAM_8BIT, 0x2c, 0xac }, //dc
	{ UNICAM_8BIT, 0x2d, 0x07 }, 
	{ UNICAM_8BIT, 0x2e, 0x90 }, 
	{ UNICAM_8BIT, 0x3c, 0x20 }, 
	{ UNICAM_8BIT, 0xfe, 0x00 }, 

	{ UNICAM_8BIT, 0xfe, 0x00 }, 

	{ UNICAM_TOK_TERM, 0, 0}

};

#define gc0310_ANALOG_GAIN_1                   64  // 1.00x
#define gc0310_ANALOG_GAIN_2                   88  // 1.375x
#define gc0310_ANALOG_GAIN_3                   122  // 1.90x
#define gc0310_ANALOG_GAIN_4                   168  // 2.625x
#define gc0310_ANALOG_GAIN_5                   239  // 3.738x
#define gc0310_ANALOG_GAIN_6                   330  // 5.163x
#define gc0310_ANALOG_GAIN_7                   470  // 7.350x


static S_UNI_RESOLUTION gc0310_res_preview[] = {
	{
		.desc = "gc0310_VGA",
		.res_type = UNI_DEV_RES_PREVIEW | UNI_DEV_RES_VIDEO | UNI_DEV_RES_STILL,
		.width = 640,
		.height = 480,
		.fps = 30, 
		.pixels_per_line = 1190,   
 		.lines_per_frame = 605,   
		.bin_factor_x = 2,
		.bin_factor_y = 2,
		.bin_mode = 1,
		.skip_frames = 3,
		.regs = gc0310_640x480_30fps,
		.regs_n= (ARRAY_SIZE(gc0310_640x480_30fps)),

		.vt_pix_clk_freq_mhz=14400000, 
		.crop_horizontal_start=0,
		.crop_vertical_start=0,
		.crop_horizontal_end=656,
		.crop_vertical_end=496,
		.output_width=640,
		.output_height=480,
		.used = 0,

		.vts_fix=8,
		.vts=638,
	},
		
};

static S_UNI_DEVICE gc0310_unidev={
	.version=0x1,
	.minversion=125,//update if change, as buildid
	.af_type=UNI_DEV_AF_NONE,
	.i2c_addr=0x21,

	.idreg=0xF0,
	.idval=0xa310,
	.idmask=0xffff,
	.powerdelayms=1,
	.accessdelayms=20,
	.flag=UNI_DEV_FLAG_1B_ADDR,
	.exposecoarse=0,
	.exposeanaloggain=0,
	.exposedigitgainR=0,
	.exposedigitgainGR=0,
	.exposedigitgainGB=0,
	.exposedigitgainB=0,
	.exposefine=0,
	.exposefine_mask=0,
	.exposevts=0,
	.exposevts_mask=0,

	.exposecoarse_mask=0,
	.exposeanaloggain_mask=0,
	.exposedigitgain_mask=0,
	.exposeadgain_param=gc0310_adgain_p,
	.exposeadgain_n=(ARRAY_SIZE(gc0310_adgain_p)),
	.exposeadgain_min=0,
	.exposeadgain_max=0, //max 6x Gain
	.exposeadgain_fra=0,
	.exposeadgain_step=0,
	.exposeadgain_isp1gain=0,
	.exposeadgain_param1gain=0,

	.global_setting=gc0310_global_setting,
	.global_setting_n=(ARRAY_SIZE(gc0310_global_setting)),
	.stream_off=gc0310_stream_off,
	.stream_off_n=(ARRAY_SIZE(gc0310_stream_off)),
	.stream_on=gc0310_stream_on,
	.stream_on_n=(ARRAY_SIZE(gc0310_stream_on)),
	.group_hold_start=gc0310_group_hold_start,
	.group_hold_start_n=(ARRAY_SIZE(gc0310_group_hold_start)),
	.group_hold_end=gc0310_group_hold_end,
	.group_hold_end_n=(ARRAY_SIZE(gc0310_group_hold_end)),

	.ress=gc0310_res_preview,
	.ress_n=(ARRAY_SIZE(gc0310_res_preview)),
	.coarse_integration_time_min=1,
	.coarse_integration_time_max_margin=8,
	.fine_integration_time_min=0,
	.fine_integration_time_max_margin=6,
	.hw_port=ATOMISP_CAMERA_PORT_SECONDARY,
	.hw_lane=1,
	.hw_format=ATOMISP_INPUT_FORMAT_YUV422_8,
	.hw_bayer_order=atomisp_bayer_order_grbg,
	.hw_sensor_type=SOC_CAMERA,

	.hw_reset_gpio=120,
	.hw_reset_gpio_polarity=0,
	.hw_reset_delayms=20,
	.hw_pwd_gpio=124,
	.hw_pwd_gpio_polarity=1,
	.hw_clksource=1,
};
#undef PPR_DEV
#define PPR_DEV gc0310_unidev
