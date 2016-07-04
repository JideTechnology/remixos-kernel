
// auto generate : Fri Sep 26 15:49:18 CST 2014
S_UNI_REG  hm2056_stream_on[] = {
	{UNICAM_8BIT, 0x0000, 0x01},
	{UNICAM_8BIT, 0x0100, 0x01},
	{UNICAM_8BIT, 0x0101, 0x01},
	{UNICAM_8BIT, 0x0005, 0x01},	//Turn on rolling shutter
	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG  hm2056_stream_off[] = {
	{UNICAM_8BIT, 0x0005, 0x00},	//Turn on rolling shutter
	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG  hm2056_global_setting[] = {
	{UNICAM_8BIT, 0x0022, 0x00},	// Reset
	{UNICAM_8BIT, 0x0020, 0x00},
	{UNICAM_8BIT, 0x0025, 0x00},	//CKCFG 80 from system clock, 00 from PLL
	{UNICAM_8BIT, 0x0026, 0x85},	//PLL1CFG should be 07 when system clock, should be 87 when PLL
	{UNICAM_8BIT, 0x0027, 0x03},	//Raw output
	{UNICAM_8BIT, 0x0028, 0xC0},	//Raw output
	{UNICAM_8BIT, 0x002A, 0x2F},	// CLK - 20131106
	{UNICAM_8BIT, 0x002B, 0x04},	// CLK - 20131106
	{UNICAM_8BIT, 0x002C, 0x0A},	//Set default vaule for CP and resistance of LPF to 1010
	{UNICAM_8BIT, 0x0004, 0x10},
	{UNICAM_8BIT, 0x0006, 0x00},	// Flip/Mirror
	{UNICAM_8BIT, 0x000D, 0x00},	// 20120220 to fix morie
	{UNICAM_8BIT, 0x000E, 0x00},	// Binning ON
	{UNICAM_8BIT, 0x000F, 0x00},	// IMGCFG
	{UNICAM_8BIT, 0x0010, 0x00},	// VBI - 20131106
	{UNICAM_8BIT, 0x0011, 0x5F},	// VBI - 20131119
	{UNICAM_8BIT, 0x0012, 0x04},	//2012.02.08
	{UNICAM_8BIT, 0x0013, 0x00},
	{UNICAM_8BIT, 0x0040, 0x20},	//20120224 for BLC stable
	{UNICAM_8BIT, 0x0053, 0x0A},
	{UNICAM_8BIT, 0x0044, 0x06},	//enable BLC_phase2
	{UNICAM_8BIT, 0x0046, 0xD8},	//enable BLC_phase1, disable BLC_phase2 dithering
	{UNICAM_8BIT, 0x004A, 0x0A},	//disable BLC_phase2 hot pixel filter
	{UNICAM_8BIT, 0x004B, 0x72},
	{UNICAM_8BIT, 0x0075, 0x01},	//in OMUX data swap for debug usage
	{UNICAM_8BIT, 0x0070, 0x5F},	// HBlank related - 20131106
	{UNICAM_8BIT, 0x0071, 0xAB},	// HBlank related - 20131106
	{UNICAM_8BIT, 0x0072, 0x55},	// HBlank related - 20131106
	{UNICAM_8BIT, 0x0073, 0x50},
	{UNICAM_8BIT, 0x0077, 0x04},
	{UNICAM_8BIT, 0x0080, 0xC8},	//2012.02.08
	{UNICAM_8BIT, 0x0082, 0xE2},
	{UNICAM_8BIT, 0x0083, 0xF0},
	{UNICAM_8BIT, 0x0085, 0x11},	//Enable Thin-Oxide Case 
	{UNICAM_8BIT, 0x0086, 0x02},	//K.Kim, 0x2011.12.09
	{UNICAM_8BIT, 0x0087, 0x80},	//K.Kim, 0x2011.12.09
	{UNICAM_8BIT, 0x0088, 0x6C},
	{UNICAM_8BIT, 0x0089, 0x2E},
	{UNICAM_8BIT, 0x008A, 0x7D},	//20120224 for BLC stable
	{UNICAM_8BIT, 0x008D, 0x20},
	{UNICAM_8BIT, 0x0090, 0x00},	//1.5x(Change Gain Table )
	{UNICAM_8BIT, 0x0091, 0x10},	//3x  (3x CTIA)
	{UNICAM_8BIT, 0x0092, 0x11},	//6x  (3x CTIA + 2x PGA)
	{UNICAM_8BIT, 0x0093, 0x12},	//12x (3x CTIA + 4x PGA)
	{UNICAM_8BIT, 0x0094, 0x16},	//24x (3x CTIA + 8x PGA)
	{UNICAM_8BIT, 0x0095, 0x08},	//1.5x  20120217 for color shift
	{UNICAM_8BIT, 0x0096, 0x00},	//3x    20120217 for color shift
	{UNICAM_8BIT, 0x0097, 0x10},	//6x    20120217 for color shift
	{UNICAM_8BIT, 0x0098, 0x11},	//12x   20120217 for color shift
	{UNICAM_8BIT, 0x0099, 0x12},	//24x   20120217 for color shift
	{UNICAM_8BIT, 0x009A, 0x16},	//24x
	{UNICAM_8BIT, 0x009B, 0x34},
	{UNICAM_8BIT, 0x00A0, 0x00},
	{UNICAM_8BIT, 0x00A1, 0x04},	//2012.02.06(for Ver.C)
	{UNICAM_8BIT, 0x011F, 0xFF},	//simple bpc P31 & P33[4] P40 P42 P44[5]
	{UNICAM_8BIT, 0x0120, 0x13},	//36:50Hz, 37:60Hz, BV_Win_Weight_En=1
	{UNICAM_8BIT, 0x0121, 0x01},	//NSatScale_En=0, NSatScale=0
	{UNICAM_8BIT, 0x0122, 0x39},
	{UNICAM_8BIT, 0x0123, 0xC2},
	{UNICAM_8BIT, 0x0124, 0xCE},
	{UNICAM_8BIT, 0x0125, 0x20},
	{UNICAM_8BIT, 0x0126, 0x50},
	{UNICAM_8BIT, 0x0128, 0x1F},
	{UNICAM_8BIT, 0x0132, 0x10},
	{UNICAM_8BIT, 0x0136, 0x0A},
	{UNICAM_8BIT, 0x0131, 0xB8},	//simle bpc enable[4]
	{UNICAM_8BIT, 0x0140, 0x14},
	{UNICAM_8BIT, 0x0141, 0x0A},
	{UNICAM_8BIT, 0x0142, 0x14},
	{UNICAM_8BIT, 0x0143, 0x0A},
	{UNICAM_8BIT, 0x0144, 0x06},	//Sort bpc hot pixel ratio
	{UNICAM_8BIT, 0x0145, 0x00},
	{UNICAM_8BIT, 0x0146, 0x20},
	{UNICAM_8BIT, 0x0147, 0x0A},
	{UNICAM_8BIT, 0x0148, 0x10},
	{UNICAM_8BIT, 0x0149, 0x0C},
	{UNICAM_8BIT, 0x014A, 0x80},
	{UNICAM_8BIT, 0x014B, 0x80},
	{UNICAM_8BIT, 0x014C, 0x2E},
	{UNICAM_8BIT, 0x014D, 0x2E},
	{UNICAM_8BIT, 0x014E, 0x05},
	{UNICAM_8BIT, 0x014F, 0x05},
	{UNICAM_8BIT, 0x0150, 0x0D},
	{UNICAM_8BIT, 0x0155, 0x00},
	{UNICAM_8BIT, 0x0156, 0x10},
	{UNICAM_8BIT, 0x0157, 0x0A},
	{UNICAM_8BIT, 0x0158, 0x0A},
	{UNICAM_8BIT, 0x0159, 0x0A},
	{UNICAM_8BIT, 0x015A, 0x05},
	{UNICAM_8BIT, 0x015B, 0x05},
	{UNICAM_8BIT, 0x015C, 0x05},
	{UNICAM_8BIT, 0x015D, 0x05},
	{UNICAM_8BIT, 0x015E, 0x08},
	{UNICAM_8BIT, 0x015F, 0xFF},
	{UNICAM_8BIT, 0x0160, 0x50},	// OTP BPC 2line & 4line enable
	{UNICAM_8BIT, 0x0161, 0x20},
	{UNICAM_8BIT, 0x0162, 0x14},
	{UNICAM_8BIT, 0x0163, 0x0A},
	{UNICAM_8BIT, 0x0164, 0x10},	// OTP 4line Strength
	{UNICAM_8BIT, 0x0165, 0x08},
	{UNICAM_8BIT, 0x0166, 0x0A},
	{UNICAM_8BIT, 0x018C, 0x24},
	{UNICAM_8BIT, 0x018D, 0x04},	//Cluster correction enable singal from thermal sensor (YL 2012 02 13)
	{UNICAM_8BIT, 0x018E, 0x00},	//Enable Thermal sensor control bit[7] (YL 2012 02 13)
	{UNICAM_8BIT, 0x018F, 0x11},	//Cluster Pulse enable T1[0] T2[1] T3[2] T4[3]
	{UNICAM_8BIT, 0x0190, 0x80},	//A11 BPC Strength[7:3], cluster correct P11[0]P12[1]P13[2]
	{UNICAM_8BIT, 0x0191, 0x47},	//A11[0],A7[1],Sort[3],A13 AVG[6]
	{UNICAM_8BIT, 0x0192, 0x48},	//A13 Strength[4:0],hot pixel detect for cluster[6]
	{UNICAM_8BIT, 0x0193, 0x64},
	{UNICAM_8BIT, 0x0194, 0x32},
	{UNICAM_8BIT, 0x0195, 0xc8},
	{UNICAM_8BIT, 0x0196, 0x96},
	{UNICAM_8BIT, 0x0197, 0x64},
	{UNICAM_8BIT, 0x0198, 0x32},
	{UNICAM_8BIT, 0x0199, 0x14},	//A13 hot pixel th
	{UNICAM_8BIT, 0x019A, 0x20},	// A13 edge detect th
	{UNICAM_8BIT, 0x019B, 0x14},
	{UNICAM_8BIT, 0x01BA, 0x10},	//BD
	{UNICAM_8BIT, 0x01BB, 0x04},
	{UNICAM_8BIT, 0x01D8, 0x40},
	{UNICAM_8BIT, 0x01DE, 0x60},
	{UNICAM_8BIT, 0x01E4, 0x04},
	{UNICAM_8BIT, 0x01E5, 0x04},
	{UNICAM_8BIT, 0x01E6, 0x04},
	{UNICAM_8BIT, 0x01F2, 0x0C},
	{UNICAM_8BIT, 0x01F3, 0x14},
	{UNICAM_8BIT, 0x01F8, 0x04},
	{UNICAM_8BIT, 0x01F9, 0x0C},
	{UNICAM_8BIT, 0x01FE, 0x02},
	{UNICAM_8BIT, 0x01FF, 0x04},
	{UNICAM_8BIT, 0x0380, 0xFC},
	{UNICAM_8BIT, 0x0381, 0x4A},
	{UNICAM_8BIT, 0x0382, 0x36},
	{UNICAM_8BIT, 0x038A, 0x40},
	{UNICAM_8BIT, 0x038B, 0x08},
	{UNICAM_8BIT, 0x038C, 0xC1},
	{UNICAM_8BIT, 0x038E, 0x40},
	{UNICAM_8BIT, 0x038F, 0x09},
	{UNICAM_8BIT, 0x0390, 0xD0},
	{UNICAM_8BIT, 0x0391, 0x05},
	{UNICAM_8BIT, 0x0393, 0x80},
	{UNICAM_8BIT, 0x0395, 0x21},	//AEAWB skip count
	{UNICAM_8BIT, 0x0420, 0x84},	//Digital Gain offset
	{UNICAM_8BIT, 0x0421, 0x00},
	{UNICAM_8BIT, 0x0422, 0x00},
	{UNICAM_8BIT, 0x0423, 0x83},
	{UNICAM_8BIT, 0x0466, 0x14},
	{UNICAM_8BIT, 0x0460, 0x01},
	{UNICAM_8BIT, 0x0461, 0xFF},
	{UNICAM_8BIT, 0x0462, 0xFF},
	{UNICAM_8BIT, 0x0478, 0x01},
	{UNICAM_8BIT, 0x047A, 0x00},	//ELOFFNRB
	{UNICAM_8BIT, 0x047B, 0x00},	//ELOFFNRY
	{UNICAM_8BIT, 0x0540, 0x00},
	{UNICAM_8BIT, 0x0541, 0x9D},	//60Hz Flicker
	{UNICAM_8BIT, 0x0542, 0x00},
	{UNICAM_8BIT, 0x0543, 0xBC},	//50Hz Flicker
	{UNICAM_8BIT, 0x05E4, 0x00},	//Windowing
	{UNICAM_8BIT, 0x05E5, 0x00},
	{UNICAM_8BIT, 0x05E6, 0x53},
	{UNICAM_8BIT, 0x05E7, 0x06},
	{UNICAM_8BIT, 0x0698, 0x00},
	{UNICAM_8BIT, 0x0699, 0x00},
	{UNICAM_8BIT, 0x0B20, 0xAE},	//Set clock lane is on at sending packet, Patrick: 0xAE
	{UNICAM_8BIT, 0x0078, 0x80},	//Set clock lane is on at sending packet, Patrick: new setting
	{UNICAM_8BIT, 0x007C, 0x09},	//pre-hsync setting, Patrick: 0x09
	{UNICAM_8BIT, 0x007D, 0x3E},	//pre-vsync setting
	{UNICAM_8BIT, 0x0B02, 0x01},	// TLPX WIDTH, Add by Wilson, 20111114
	{UNICAM_8BIT, 0x0B03, 0x03},
	{UNICAM_8BIT, 0x0B04, 0x01},
	{UNICAM_8BIT, 0x0B05, 0x08},
	{UNICAM_8BIT, 0x0B06, 0x02},	
	{UNICAM_8BIT, 0x0B07, 0x28},	//MARK1 WIDTH
	{UNICAM_8BIT, 0x0B0E, 0x0B},	//CLK FRONT PORCH WIDTH
	{UNICAM_8BIT, 0x0B0F, 0x04},	//CLK BACK PORCH WIDTH
	{UNICAM_8BIT, 0x0B22, 0x02},	//HS_EXIT Eanble
	{UNICAM_8BIT, 0x0B39, 0x03},	//Clock Lane HS_EXIT WIDTH(at least 100ns)
	{UNICAM_8BIT, 0x0B11, 0x7F},	//Clock Lane LP Driving Strength
	{UNICAM_8BIT, 0x0B12, 0x7F},	//Data Lane LP Driving Strength
	{UNICAM_8BIT, 0x0B17, 0xE0},	//D-PHY Power Down Control
	{UNICAM_8BIT, 0x0B22, 0x02},
	{UNICAM_8BIT, 0x0B30, 0x0F},	//D-PHY Reset, set to 1 for normal operation
	{UNICAM_8BIT, 0x0B31, 0x02},	//[1]: PHASE_SEL = 1 First Data at rising edge
	{UNICAM_8BIT, 0x0B32, 0x00},	//[4]: DBG_ULPM 
	{UNICAM_8BIT, 0x0B33, 0x00},	//DBG_SEL
	{UNICAM_8BIT, 0x0B35, 0x00},
	{UNICAM_8BIT, 0x0B36, 0x00},
	{UNICAM_8BIT, 0x0B37, 0x00},
	{UNICAM_8BIT, 0x0B38, 0x00},
	{UNICAM_8BIT, 0x0B39, 0x03},
	{UNICAM_8BIT, 0x0B3A, 0x00},	//CLK_HS_EXIT, Add by Wilson, 20111114
	{UNICAM_8BIT, 0x0B3B, 0x12},	//Turn on PHY LDO
	{UNICAM_8BIT, 0x0B3F, 0x01},	//MIPI reg delay, Add by Wilson, 20111114
	{UNICAM_8BIT, 0x0024, 0x40},	//[6]: MIPI Enable
	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG  hm2056_group_hold_start[] = {
	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG  hm2056_group_hold_end[] = {
	{UNICAM_8BIT, 0x0100, 0x01},
	{UNICAM_TOK_TERM, 0, 0}
};
static  int hm2056_adgain_p[] = {
	0,100,
	1,200,
	2,400,
	3,800,
};
S_UNI_REG hm2056_regress0[] = {
	{UNICAM_8BIT, 0x0024, 0x00},
	{UNICAM_8BIT, 0x0006, 0x14},
	{UNICAM_8BIT, 0x000D, 0x00},
	{UNICAM_8BIT, 0x000E, 0x00},
	{UNICAM_8BIT, 0x0010, 0x00},
	{UNICAM_8BIT, 0x0011, 0x64},
	{UNICAM_8BIT, 0x0012, 0x04},
	{UNICAM_8BIT, 0x0013, 0x00},
	{UNICAM_8BIT, 0x002A, 0x2F},
	{UNICAM_8BIT, 0x0071, 0xAB},
	{UNICAM_8BIT, 0x0074, 0x13},
	{UNICAM_8BIT, 0x0082, 0xE2},
	{UNICAM_8BIT, 0x0131, 0xB8},
	{UNICAM_8BIT, 0x0144, 0x06},
	{UNICAM_8BIT, 0x0190, 0x80},
	{UNICAM_8BIT, 0x0192, 0x48},
	{UNICAM_8BIT, 0x05E4, 0x00},
	{UNICAM_8BIT, 0x05E5, 0x00},
	{UNICAM_8BIT, 0x05E6, 0x53},
	{UNICAM_8BIT, 0x05E7, 0x06},
	{UNICAM_8BIT, 0x05E8, 0x00},
	{UNICAM_8BIT, 0x05E9, 0x00},
	{UNICAM_8BIT, 0x05EA, 0x8F},
	{UNICAM_8BIT, 0x05EB, 0x03},
	{UNICAM_8BIT, 0x0024, 0x40},
	{UNICAM_TOK_DELAY, 0, 60}, // Delay longer than 1 frame time ~52ms
	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG hm2056_regress1[] = {
	{UNICAM_8BIT, 0x0024, 0x00},
	{UNICAM_8BIT, 0x0006, 0x00},
	{UNICAM_8BIT, 0x000D, 0x00},
	{UNICAM_8BIT, 0x000E, 0x00},
	{UNICAM_8BIT, 0x0010, 0x00},
	{UNICAM_8BIT, 0x0011, 0x8A},
	{UNICAM_8BIT, 0x0012, 0x04},
	{UNICAM_8BIT, 0x0013, 0x00},
	{UNICAM_8BIT, 0x002A, 0x2F},
	{UNICAM_8BIT, 0x0071, 0xAB},
	{UNICAM_8BIT, 0x0074, 0x13},
	{UNICAM_8BIT, 0x0082, 0xE2},
	{UNICAM_8BIT, 0x0131, 0xB8},
	{UNICAM_8BIT, 0x0144, 0x06},
	{UNICAM_8BIT, 0x0190, 0x80},
	{UNICAM_8BIT, 0x0192, 0x48},
	{UNICAM_8BIT, 0x05E6, 0x53},
	{UNICAM_8BIT, 0x05E7, 0x06},
	{UNICAM_8BIT, 0x05EA, 0xC3},
	{UNICAM_8BIT, 0x05EB, 0x04},
	{UNICAM_8BIT, 0x0024, 0x40},
	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG hm2056_regress2[] = {
	{UNICAM_8BIT, 0x0024, 0x00},
	{UNICAM_8BIT, 0x0006, 0x10},
	{UNICAM_8BIT, 0x000D, 0x00},
	{UNICAM_8BIT, 0x000E, 0x00},
	{UNICAM_8BIT, 0x0010, 0x00},
	{UNICAM_8BIT, 0x0011, 0x84},
	{UNICAM_8BIT, 0x0012, 0x04},
	{UNICAM_8BIT, 0x0013, 0x00},
	{UNICAM_8BIT, 0x002A, 0x2F},
	{UNICAM_8BIT, 0x0071, 0xAB},
	{UNICAM_8BIT, 0x0074, 0x13},
	{UNICAM_8BIT, 0x0082, 0xE2},
	{UNICAM_8BIT, 0x0131, 0xB8},
	{UNICAM_8BIT, 0x0144, 0x06},
	{UNICAM_8BIT, 0x0190, 0x80},
	{UNICAM_8BIT, 0x0192, 0x48},
	{UNICAM_8BIT, 0x05E4, 0x00},
	{UNICAM_8BIT, 0x05E5, 0x00},
	{UNICAM_8BIT, 0x05E6, 0x6F},
	{UNICAM_8BIT, 0x05E7, 0x05},
	{UNICAM_8BIT, 0x05E8, 0x00},
	{UNICAM_8BIT, 0x05E9, 0x00},
	{UNICAM_8BIT, 0x05EA, 0x0b},
	{UNICAM_8BIT, 0x05EB, 0x03},
	{UNICAM_8BIT, 0x0024, 0x40},
	{UNICAM_TOK_DELAY, 0, 40}, // Delay longer than 1 frame time ~35ms
	{UNICAM_TOK_TERM, 0, 0}
};
#if 0
S_UNI_REG hm2056_regress[][] = {
	hm2056_regress0,
	hm2056_regress1,
	hm2056_regress2,
};
#endif
static S_UNI_RESOLUTION hm2056_res_preview[] = {
	{
		.desc = "hm2056_hm2056_hdp_19fps",
		.res_type = 0,
		.width = 1620,
		.height = 912,
		.fps = 19,
		.pixels_per_line = 1919,
		.lines_per_frame = 1050,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 1,
		.regs = hm2056_regress0,
		.regs_n= (ARRAY_SIZE(hm2056_regress0)),

		.vt_pix_clk_freq_mhz=39000000,
		.crop_horizontal_start=0,
		.crop_vertical_start=1619,
		.crop_horizontal_end=0,
		.crop_vertical_end=911,
		.output_width=1620,
		.output_height=912,
		.used = 0,

		.vts_fix=8,
		.vts=1050,
	},
	{
		.desc = "hm2056_hm2056_uxga_14fps",
		.res_type = 0,
		.width = 1620,
		.height = 1220,
		.fps = 14,
		.pixels_per_line = 1919,
		.lines_per_frame = 1358,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 1,
		.regs = hm2056_regress1,
		.regs_n= (ARRAY_SIZE(hm2056_regress1)),

		.vt_pix_clk_freq_mhz=39000000,
		.crop_horizontal_start=0,
		.crop_vertical_start=0,
		.crop_horizontal_end=1619,
		.crop_vertical_end=1219,
		.output_width=1620,
		.output_height=1220,
		.used = 0,

		.vts_fix=8,
		.vts=1358,
	},
	{
		.desc = "hm2056_HDH_28fps",
		.res_type = UNI_DEV_RES_DEFAULT,
		.width = 1392,
		.height = 780,
		.fps = 28,
		.pixels_per_line = 1711,
		.lines_per_frame = 912,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 3,
		.regs = hm2056_regress2,
		.regs_n= (ARRAY_SIZE(hm2056_regress2)),

		.vt_pix_clk_freq_mhz=39000000,
		.crop_horizontal_start=0,
		.crop_vertical_start=0,
		.crop_horizontal_end=1391,
		.crop_vertical_end=779,
		.output_width=1392,
		.output_height=780,
		.used = 0,

		.vts_fix=5,
		.vts=912,
	},
};
static S_UNI_DEVICE hm2056_unidev={
	.version=0x1,
	.minversion=125,//update if change, as buildid
	.af_type=UNI_DEV_AF_NONE,
	.i2c_addr=0x24,

	.idreg=0x0001,
	.idval=0x2056,
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
	.exposevts=0x0010,
	.exposevts_mask=0xffff,

	.exposecoarse_mask=0xffff,
	.exposeanaloggain_mask=0x03,
	.exposedigitgain_mask=0xff,
	.exposeadgain_param=hm2056_adgain_p,
	.exposeadgain_n=(ARRAY_SIZE(hm2056_adgain_p)),
	.exposeadgain_min=64,
	.exposeadgain_max=127,
	.exposeadgain_fra=64,
	.exposeadgain_step=1,
	.exposeadgain_isp1gain=16,
	.exposeadgain_param1gain=100,

	.global_setting=hm2056_global_setting,
	.global_setting_n=(ARRAY_SIZE(hm2056_global_setting)),
	.stream_off=hm2056_stream_off,
	.stream_off_n=(ARRAY_SIZE(hm2056_stream_off)),
	.stream_on=hm2056_stream_on,
	.stream_on_n=(ARRAY_SIZE(hm2056_stream_on)),
	.group_hold_start=hm2056_group_hold_start,
	.group_hold_start_n=(ARRAY_SIZE(hm2056_group_hold_start)),
	.group_hold_end=hm2056_group_hold_end,
	.group_hold_end_n=(ARRAY_SIZE(hm2056_group_hold_end)),

	.ress=hm2056_res_preview,
	.ress_n=(ARRAY_SIZE(hm2056_res_preview)),
	.coarse_integration_time_min=0,
	.coarse_integration_time_max_margin=8,
	.fine_integration_time_min=0,
	.fine_integration_time_max_margin=65535,
	.hw_port=ATOMISP_CAMERA_PORT_SECONDARY,
	.hw_lane=1,
	.hw_format=ATOMISP_INPUT_FORMAT_RAW_8,
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
#define PPR_DEV hm2056_unidev
