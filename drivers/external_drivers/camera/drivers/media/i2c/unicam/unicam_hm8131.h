
// auto generate : Mon Jul  7 15:51:26 CST 2014
S_UNI_REG  hm8131_stream_on[] = {
//	{UNICAM_8BIT, 0x0100, 0x01},
	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG  hm8131_stream_off[] = {
//	{UNICAM_8BIT, 0x0100, 0x00},
	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG  hm8131_global_setting[] = {
	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG  hm8131_group_hold_start[] = {
//	{UNICAM_8BIT, 0x0104, 0x01},
	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG  hm8131_group_hold_end[] = {
	{UNICAM_8BIT, 0x0104, 0xff},
	{UNICAM_TOK_TERM, 0, 0}
};
static  int hm8131_adgain_p[] = {
	0,
};
S_UNI_REG hm8131_regress0[] = {
	{UNICAM_8BIT, 0x0100, 0x00},
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay		
	{UNICAM_8BIT,0x0103,0x00},// ; software reset-> was 0x22
	{UNICAM_8BIT, 0x0100, 0x02},
	{UNICAM_8BIT,0x3519,0x00},//;// PWE interval high byte
	{UNICAM_8BIT,0x351A,0x05},//;// PWE interval low byte
	{UNICAM_8BIT,0x351B,0x1E},//;// PWE interval high byte
	{UNICAM_8BIT,0x351C,0x90},//;// PWE interval low byte
	{UNICAM_8BIT,0x351E,0x15},//;// PRD high pulse byte
	{UNICAM_8BIT,0x351D,0x15},// ;// PRD interval low byte
	{UNICAM_8BIT,0x4001,0x80},//;
	{UNICAM_8BIT,0xBA93,0x01},//; Read Header
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x412A,0x8A},//;
	{UNICAM_8BIT,0xBAA2,0xC0},//;
	{UNICAM_8BIT,0xBAA2,0xC0},//;
	{UNICAM_8BIT,0xBAA2,0xC0},//;
	{UNICAM_8BIT,0xBAA2,0x40},//;
	{UNICAM_8BIT,0x412A,0x8A},//;
	{UNICAM_8BIT,0x412A,0x8A},//;
	{UNICAM_8BIT,0x412A,0x0A},//;
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0xBA93,0x03},//; Read Header
	{UNICAM_8BIT,0xBA93,0x02},//; Read Header
	{UNICAM_8BIT,0xBA90,0x01},//; BPS_OTP_EN[0]
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x0100,0x02},//; power up
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x4001,0x00},// ; ck_cfg - choose mclk_pll -> was 0025
	{UNICAM_8BIT,0x0309,0x02},// ; PLL mipi1 - enabled PLL -> was 002D
	{UNICAM_8BIT,0x0305,0x0C},// ; PLL N, mclk 24mhz
	{UNICAM_8BIT,0x0307,0x46},// ; PLL M, pclk_raw=71/2=35mhz
	{UNICAM_8BIT,0x030D,0x0C},// ; PLL N, 
	{UNICAM_8BIT,0x030F,0x62},// ; PLL M, pkt_clk=98/2=49mhz
	{UNICAM_8BIT,0x414A,0x02},// ; BLC IIR setting
	{UNICAM_8BIT,0x4147,0x03},// ;
	{UNICAM_8BIT,0x4144,0x03},// ; 
	{UNICAM_8BIT,0x4145,0x31},// ; By Willie enalbe CFPN
	{UNICAM_8BIT,0x4146,0x51},// ; 
	{UNICAM_8BIT,0x4149,0x57},// ; By Ken Wu to improve the beating (BLC dither and hysterisis enable)
	{UNICAM_8BIT,0x4260,0x00},// ; By Simon
	{UNICAM_8BIT,0x4261,0x00},// ; By Simon
	{UNICAM_8BIT,0x4264,0x14},// ; By Simon
	{UNICAM_8BIT,0x426A,0x01},// ;
	{UNICAM_8BIT,0x4270,0x08},// ; By Willie 
	{UNICAM_8BIT,0x4271,0x8A},// ; By Willie
	{UNICAM_8BIT,0x4272,0x22},// ;
	{UNICAM_8BIT,0x427D,0x00},// ; By Simon
	{UNICAM_8BIT,0x427E,0x03},// ; By Simon
	{UNICAM_8BIT,0x427F,0x00},// ;
	{UNICAM_8BIT,0x4380,0xA6},// ; 
	{UNICAM_8BIT,0x4381,0x7B},// ; By Willie to save ramp power
	{UNICAM_8BIT,0x4383,0x58},// ; By Willie (get brighter image at low light), increase VRNP voltage.
	{UNICAM_8BIT,0x4387,0x37},// ; Adjust clamping level
	{UNICAM_8BIT,0x4386,0x00},// ; 
	{UNICAM_8BIT,0x4382,0x00},// ;
	{UNICAM_8BIT,0x4388,0x9F},// ;
	{UNICAM_8BIT,0x4389,0x15},// ;
	{UNICAM_8BIT,0x438A,0x80},// ;
	{UNICAM_8BIT,0x438C,0x0F},// ;
	{UNICAM_8BIT,0x4384,0x14},// ; 
	{UNICAM_8BIT,0x438B,0x00},// ;
	{UNICAM_8BIT,0x4385,0xA5},// ; 
	{UNICAM_8BIT,0x438F,0x00},// ;
	{UNICAM_8BIT,0x438D,0xA0},// ;
	{UNICAM_8BIT,0x4B11,0x1F},// ;
	{UNICAM_8BIT,0x4B44,0x40},// ;
	{UNICAM_8BIT,0x4B46,0x03},// ;
	{UNICAM_8BIT,0x4B47,0xC9},// ;
	{UNICAM_8BIT,0x44B0,0x03},// ;
	{UNICAM_8BIT,0x44B1,0x01},// ;
	{UNICAM_8BIT,0x44B2,0x00},// ;
	{UNICAM_8BIT,0x44B3,0x04},// ;
	{UNICAM_8BIT,0x44B4,0x14},// ;
	{UNICAM_8BIT,0x44B5,0x24},// ;
	{UNICAM_8BIT,0x44B8,0x03},// ;
	{UNICAM_8BIT,0x44B9,0x01},// ;
	{UNICAM_8BIT,0x44BA,0x05},// ;
	{UNICAM_8BIT,0x44BB,0x15},// ;
	{UNICAM_8BIT,0x44BC,0x25},// ;
	{UNICAM_8BIT,0x44BD,0x35},// ;
	{UNICAM_8BIT,0x44D0,0xC0},// ;
	{UNICAM_8BIT,0x44D1,0x80},// ;
	{UNICAM_8BIT,0x44D2,0x40},// ;
	{UNICAM_8BIT,0x44D3,0x40},// ;
	{UNICAM_8BIT,0x44D4,0x40},// ;
	{UNICAM_8BIT,0x44D5,0x40},// ;
	{UNICAM_8BIT,0x4B07,0xF0},// ; by willie_cheng extend mark1 ~ 1200us
	{UNICAM_8BIT,0x4131,0x01},// ;	//BLI
	{UNICAM_8BIT,0x060B,0x01},// ;	//test pattern PN9 seed
	{UNICAM_8BIT,0x4274,0x33},// ; [5] mask out bad frame due to mode (flip/mirror) change
	{UNICAM_8BIT,0x400D,0x04},// ; [2] binning mode analog gain table -> HII binning gain table
	{UNICAM_8BIT,0x3110,0x01},// ;
	{UNICAM_8BIT,0x3111,0x01},// ;
	{UNICAM_8BIT,0x3130,0x01},// ;
	{UNICAM_8BIT,0x3131,0x26},// ;
	{UNICAM_8BIT,0x0383,0x03},// ; x odd increment -> was 01
	{UNICAM_8BIT,0x0387,0x03},// ; y odd increment -> was 01
	{UNICAM_8BIT,0x0390,0x11},// ; binning mode -> was 00	1:binning 2:summing
	{UNICAM_8BIT,0x0348,0x0C},// ; Image size X end Hb
	{UNICAM_8BIT,0x0349,0xCD},// ; Image size X end Lb
	{UNICAM_8BIT,0x034A,0x09},// ; Image size Y end Hb
	{UNICAM_8BIT,0x034B,0x9D},// ; Image size Y end Lb
	{UNICAM_8BIT,0x0340,0x05},// ; frame length lines Hb
	{UNICAM_8BIT,0x0341,0x1A},// ; frame length lines Lb
	{UNICAM_8BIT,0x4B31,0x06},// ;[2] PKTCLK_PHASE_SEL
	{UNICAM_8BIT,0x034C,0x06},// ; HSIZE (1616)
	{UNICAM_8BIT,0x034D,0x50},// ; HSIZE  
	{UNICAM_8BIT,0x034E,0x03},// ; VSIZE (916)
	{UNICAM_8BIT,0x034F,0x94},// ; VSIZE 
	{UNICAM_8BIT,0x4B18,0x06},// ; [7:0] FULL_TRIGGER_TIME 
	{UNICAM_8BIT,0x0111,0x01},// ; B5: 1'b0:2lane, 1'b1:4lane (org 4B19)
	{UNICAM_8BIT,0x4B20,0xDE},// ; clock always on(9E)
	{UNICAM_8BIT,0x4B02,0x02},// ; lpx_width
	{UNICAM_8BIT,0x4B03,0x05},// ; hs_zero_width
	{UNICAM_8BIT,0x4B04,0x02},// ; hs_trail_width
	{UNICAM_8BIT,0x4B05,0x0C},// ; clk_zero_width
	{UNICAM_8BIT,0x4B06,0x03},// ; clk_trail_width
	{UNICAM_8BIT,0x4B0f,0x07},// ; clk_back_porch
	{UNICAM_8BIT,0x4B39,0x02},// ; clk_width_exit
	{UNICAM_8BIT,0x4B3f,0x08},// ; [3:0] mipi_req_dly
	{UNICAM_8BIT,0x4B42,0x02},// ; HS_PREPARE_WIDTH
	{UNICAM_8BIT,0x4B43,0x02},// ; CLK_PREPARE_WIDTH
	{UNICAM_8BIT,0x4B0E,0x01},// ;Tclk_pre > 0, if H_SIZE/16!=0 
	{UNICAM_8BIT,0x4024,0x40},// ; enabled MIPI -> was 0024
	{UNICAM_8BIT,0x4800,0x00},// ; was CMU 0000
	{UNICAM_8BIT,0x0104,0x01},// ; was 0100
	{UNICAM_8BIT,0x0104,0x00},// ; was 0100, need an high to low edge now! (CMU_AE)
	{UNICAM_8BIT,0x4801,0x00},// ; was 0106
	{UNICAM_8BIT,0x0000,0x00},// ; was 0000
	{UNICAM_8BIT,0xBA94,0x01},//;
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0xBA94,0x00},//;
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0xBA93,0x02},//; 
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x0100,0x01},// ; was 0005 ; mode_select
	{UNICAM_TOK_DELAY,0,0x45},// 68ms
	{UNICAM_TOK_TERM, 0, 0}

};
S_UNI_REG hm8131_regress1_6M[] = {
	{UNICAM_8BIT, 0x0100, 0x00},
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay		
	{UNICAM_8BIT,0x0103,0x00},// ; software reset-> was 0x22
	{UNICAM_8BIT, 0x0100, 0x02},
	{UNICAM_TOK_DELAY,0,0x45},// 68ms
	{UNICAM_8BIT,0x3519,0x00},
	{UNICAM_8BIT,0x351A,0x05},
	{UNICAM_8BIT,0x351B,0x1E},
	{UNICAM_8BIT,0x351C,0x90},
	{UNICAM_8BIT,0x351E,0x15},
	{UNICAM_8BIT,0x351D,0x15},
	{UNICAM_8BIT,0x4001,0x80},
	{UNICAM_8BIT,0xBA93,0x01},
	{UNICAM_8BIT,0x412A,0x8A},
	{UNICAM_8BIT,0xBAA2,0xC0},
	{UNICAM_8BIT,0xBAA2,0xC0},
	{UNICAM_8BIT,0xBAA2,0xC0},
	{UNICAM_8BIT,0xBAA2,0x40},
	{UNICAM_8BIT,0x412A,0x8A},
	{UNICAM_8BIT,0x412A,0x8A},
	{UNICAM_8BIT,0x412A,0x0A},
	{UNICAM_8BIT,0xBA93,0x03},
	{UNICAM_8BIT,0xBA93,0x02},
	{UNICAM_8BIT,0xBA90,0x01},
	{UNICAM_8BIT,0x0100,0x02},
	{UNICAM_8BIT,0x4001,0x00},
	{UNICAM_8BIT,0x0305,0x0C},
	{UNICAM_8BIT,0x0307,0x36},
	{UNICAM_8BIT,0x030D,0x0C},
	{UNICAM_8BIT,0x030F,0x4B},
	{UNICAM_8BIT,0x414A,0x02},
	{UNICAM_8BIT,0x4147,0x03},
	{UNICAM_8BIT,0x4144,0x03},
	{UNICAM_8BIT,0x4145,0x31},
	{UNICAM_8BIT,0x4146,0x51},
	{UNICAM_8BIT,0x4149,0x57},
	{UNICAM_8BIT,0x4260,0x00},
	{UNICAM_8BIT,0x4261,0x00},
	{UNICAM_8BIT,0x4264,0x14},
	{UNICAM_8BIT,0x426A,0x01},
	{UNICAM_8BIT,0x4270,0x08},
	{UNICAM_8BIT,0x4271,0x8A},
	{UNICAM_8BIT,0x4272,0x22},
	{UNICAM_8BIT,0x427D,0x00},
	{UNICAM_8BIT,0x427E,0x03},
	{UNICAM_8BIT,0x427F,0x00},
	{UNICAM_8BIT,0x4380,0xA6},
	{UNICAM_8BIT,0x4381,0x7B},
	{UNICAM_8BIT,0x4383,0x58},
	{UNICAM_8BIT,0x4387,0x37},
	{UNICAM_8BIT,0x4386,0x00},
	{UNICAM_8BIT,0x4382,0x00},
	{UNICAM_8BIT,0x4388,0x9F},
	{UNICAM_8BIT,0x4389,0x15},
	{UNICAM_8BIT,0x438A,0x80},
	{UNICAM_8BIT,0x438C,0x0F},
	{UNICAM_8BIT,0x4384,0x14},
	{UNICAM_8BIT,0x438B,0x00},
	{UNICAM_8BIT,0x4385,0xA5},
	{UNICAM_8BIT,0x438F,0x00},
	{UNICAM_8BIT,0x438D,0xA0},
	{UNICAM_8BIT,0x4B11,0x1F},
	{UNICAM_8BIT,0x4B44,0x40},
	{UNICAM_8BIT,0x4B46,0x03},
	{UNICAM_8BIT,0x4B47,0xC9},
	{UNICAM_8BIT,0x44B0,0x03},
	{UNICAM_8BIT,0x44B1,0x01},
	{UNICAM_8BIT,0x44B2,0x00},
	{UNICAM_8BIT,0x44B3,0x04},
	{UNICAM_8BIT,0x44B4,0x14},
	{UNICAM_8BIT,0x44B5,0x24},
	{UNICAM_8BIT,0x44B8,0x03},
	{UNICAM_8BIT,0x44B9,0x01},
	{UNICAM_8BIT,0x44BA,0x05},
	{UNICAM_8BIT,0x44BB,0x15},
	{UNICAM_8BIT,0x44BC,0x25},
	{UNICAM_8BIT,0x44BD,0x35},
	{UNICAM_8BIT,0x44D0,0xC0},
	{UNICAM_8BIT,0x44D1,0x80},
	{UNICAM_8BIT,0x44D2,0x40},
	{UNICAM_8BIT,0x44D3,0x40},
	{UNICAM_8BIT,0x44D4,0x40},
	{UNICAM_8BIT,0x44D5,0x40},
	{UNICAM_8BIT,0x4B07,0xF0},
	{UNICAM_8BIT,0x4131,0x01},
	{UNICAM_8BIT,0x060B,0x01},
	{UNICAM_8BIT,0x4274,0x33},
	{UNICAM_8BIT,0x3110,0x23},
	{UNICAM_8BIT,0x3111,0x00},
	{UNICAM_8BIT,0x3130,0x01},
	{UNICAM_8BIT,0x3131,0x80},
	{UNICAM_8BIT,0x4B31,0x06},
	{UNICAM_8BIT,0x4002,0x23},
	{UNICAM_8BIT,0x034C,0x0C},
	{UNICAM_8BIT,0x034D,0xD0},
	{UNICAM_8BIT,0x034E,0x09},
	{UNICAM_8BIT,0x034F,0xA0},
	{UNICAM_8BIT,0x4B18,0x17},
	{UNICAM_8BIT,0x0111,0x01},
	{UNICAM_8BIT,0x4B20,0xDE},
	{UNICAM_8BIT,0x4B0E,0x03},
	{UNICAM_8BIT,0x4B42,0x05},
	{UNICAM_8BIT,0x4B04,0x05},
	{UNICAM_8BIT,0x4B06,0x06},
	{UNICAM_8BIT,0x4B3F,0x12},
	{UNICAM_8BIT,0x4024,0x40},
	{UNICAM_8BIT,0x4800,0x00},
	{UNICAM_8BIT,0x0104,0x01},
	{UNICAM_8BIT,0x0104,0x00},
	{UNICAM_8BIT,0x4801,0x00},
	{UNICAM_8BIT,0x0000,0x00},
	{UNICAM_8BIT,0xBA94,0x01},
	{UNICAM_8BIT,0xBA94,0x00},
	{UNICAM_8BIT,0xBA93,0x02},
	//{UNICAM_8BIT,0x0601,0x02}, //Test patten
	{UNICAM_8BIT,0x0100,0x00},
	{UNICAM_8BIT,0x0104,0x01},
	{UNICAM_8BIT,0x4B08,0x3C},// ;VSIZE (1852)
 	{UNICAM_8BIT,0x4B09,0x07},// ;
	{UNICAM_8BIT,0x4B0A,0xD0},// ;HSIZE (3280)
	{UNICAM_8BIT,0x4B0B,0x0C},// ;
	{UNICAM_8BIT,0x0348,0x0C},// ;X-END (3279)
	{UNICAM_8BIT,0x0349,0xCF},// ;
	{UNICAM_8BIT,0x034A,0x07},// ;Y-END (1851)
	{UNICAM_8BIT,0x034B,0x3B},// ;
	{UNICAM_8BIT,0x034C,0x0C},// ;HSIZE (3280)
	{UNICAM_8BIT,0x034D,0xD0},// ;
	{UNICAM_8BIT,0x034E,0x07},// ;VSIZE (1852)
	{UNICAM_8BIT,0x034F,0x3C},// ;
	{UNICAM_8BIT,0x0104,0x00},
	{UNICAM_8BIT,0x0100,0x01},
//	{UNICAM_TOK_DELAY,0,0x45},// 68ms

	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG hm8131_regress2_8M[] = {
	{UNICAM_8BIT, 0x0100, 0x00},
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay		
	{UNICAM_8BIT,0x0103,0x00},// ; software reset-> was 0x22
	{UNICAM_8BIT, 0x0100, 0x02},
//	{UNICAM_TOK_DELAY,0,0x45},// 68ms
	{UNICAM_8BIT,0x3519,0x00},
	{UNICAM_8BIT,0x351A,0x05},
	{UNICAM_8BIT,0x351B,0x1E},
	{UNICAM_8BIT,0x351C,0x90},
	{UNICAM_8BIT,0x351E,0x15},
	{UNICAM_8BIT,0x351D,0x15},
	{UNICAM_8BIT,0x4001,0x80},
	{UNICAM_8BIT,0xBA93,0x01},
	{UNICAM_8BIT,0x412A,0x8A},
	{UNICAM_8BIT,0xBAA2,0xC0},
	{UNICAM_8BIT,0xBAA2,0xC0},
	{UNICAM_8BIT,0xBAA2,0xC0},
	{UNICAM_8BIT,0xBAA2,0x40},
	{UNICAM_8BIT,0x412A,0x8A},
	{UNICAM_8BIT,0x412A,0x8A},
	{UNICAM_8BIT,0x412A,0x0A},
	{UNICAM_8BIT,0xBA93,0x03},
	{UNICAM_8BIT,0xBA93,0x02},
	{UNICAM_8BIT,0xBA90,0x01},
	{UNICAM_8BIT,0x0100,0x02},
	{UNICAM_8BIT,0x4001,0x00},
	{UNICAM_8BIT,0x0305,0x0C},
	{UNICAM_8BIT,0x0307,0x36},
	{UNICAM_8BIT,0x030D,0x0C},
	{UNICAM_8BIT,0x030F,0x4B},
	{UNICAM_8BIT,0x414A,0x02},
	{UNICAM_8BIT,0x4147,0x03},
	{UNICAM_8BIT,0x4144,0x03},
	{UNICAM_8BIT,0x4145,0x31},
	{UNICAM_8BIT,0x4146,0x51},
	{UNICAM_8BIT,0x4149,0x57},
	{UNICAM_8BIT,0x4260,0x00},
	{UNICAM_8BIT,0x4261,0x00},
	{UNICAM_8BIT,0x4264,0x14},
	{UNICAM_8BIT,0x426A,0x01},
	{UNICAM_8BIT,0x4270,0x08},
	{UNICAM_8BIT,0x4271,0x8A},
	{UNICAM_8BIT,0x4272,0x22},
	{UNICAM_8BIT,0x427D,0x00},
	{UNICAM_8BIT,0x427E,0x03},
	{UNICAM_8BIT,0x427F,0x00},
	{UNICAM_8BIT,0x4380,0xA6},
	{UNICAM_8BIT,0x4381,0x7B},
	{UNICAM_8BIT,0x4383,0x58},
	{UNICAM_8BIT,0x4387,0x37},
	{UNICAM_8BIT,0x4386,0x00},
	{UNICAM_8BIT,0x4382,0x00},
	{UNICAM_8BIT,0x4388,0x9F},
	{UNICAM_8BIT,0x4389,0x15},
	{UNICAM_8BIT,0x438A,0x80},
	{UNICAM_8BIT,0x438C,0x0F},
	{UNICAM_8BIT,0x4384,0x14},
	{UNICAM_8BIT,0x438B,0x00},
	{UNICAM_8BIT,0x4385,0xA5},
	{UNICAM_8BIT,0x438F,0x00},
	{UNICAM_8BIT,0x438D,0xA0},
	{UNICAM_8BIT,0x4B11,0x1F},
	{UNICAM_8BIT,0x4B44,0x40},
	{UNICAM_8BIT,0x4B46,0x03},
	{UNICAM_8BIT,0x4B47,0xC9},
	{UNICAM_8BIT,0x44B0,0x03},
	{UNICAM_8BIT,0x44B1,0x01},
	{UNICAM_8BIT,0x44B2,0x00},
	{UNICAM_8BIT,0x44B3,0x04},
	{UNICAM_8BIT,0x44B4,0x14},
	{UNICAM_8BIT,0x44B5,0x24},
	{UNICAM_8BIT,0x44B8,0x03},
	{UNICAM_8BIT,0x44B9,0x01},
	{UNICAM_8BIT,0x44BA,0x05},
	{UNICAM_8BIT,0x44BB,0x15},
	{UNICAM_8BIT,0x44BC,0x25},
	{UNICAM_8BIT,0x44BD,0x35},
	{UNICAM_8BIT,0x44D0,0xC0},
	{UNICAM_8BIT,0x44D1,0x80},
	{UNICAM_8BIT,0x44D2,0x40},
	{UNICAM_8BIT,0x44D3,0x40},
	{UNICAM_8BIT,0x44D4,0x40},
	{UNICAM_8BIT,0x44D5,0x40},
	{UNICAM_8BIT,0x4B07,0xF0},
	{UNICAM_8BIT,0x4131,0x01},
	{UNICAM_8BIT,0x060B,0x01},
	{UNICAM_8BIT,0x4274,0x33},
	{UNICAM_8BIT,0x3110,0x23},
	{UNICAM_8BIT,0x3111,0x00},
	{UNICAM_8BIT,0x3130,0x01},
	{UNICAM_8BIT,0x3131,0x80},
	{UNICAM_8BIT,0x4B31,0x06},
	{UNICAM_8BIT,0x4002,0x23},
	{UNICAM_8BIT,0x034C,0x0C},
	{UNICAM_8BIT,0x034D,0xD0},
	{UNICAM_8BIT,0x034E,0x09},
	{UNICAM_8BIT,0x034F,0xA0},
	{UNICAM_8BIT,0x4B18,0x17},
	{UNICAM_8BIT,0x0111,0x01},
	{UNICAM_8BIT,0x4B20,0xDE},
	{UNICAM_8BIT,0x4B0E,0x03},
	{UNICAM_8BIT,0x4B42,0x05},
	{UNICAM_8BIT,0x4B04,0x05},
	{UNICAM_8BIT,0x4B06,0x06},
	{UNICAM_8BIT,0x4B3F,0x12},
	{UNICAM_8BIT,0x4024,0x40},
	{UNICAM_8BIT,0x4800,0x00},
	{UNICAM_8BIT,0x0104,0x01},
	{UNICAM_8BIT,0x0104,0x00},
	{UNICAM_8BIT,0x4801,0x00},
	{UNICAM_8BIT,0x0000,0x00},
	{UNICAM_8BIT,0xBA94,0x01},
	{UNICAM_8BIT,0xBA94,0x00},
	{UNICAM_8BIT,0xBA93,0x02},
	//{UNICAM_8BIT,0x0601,0x02}, //Test patten
	{UNICAM_8BIT,0x0100,0x00},
	{UNICAM_8BIT,0x0104,0x01},
	{UNICAM_8BIT,0x4B08,0xA0},// ;VSIZE (2464)
	{UNICAM_8BIT,0x4B09,0x09},// ;
	{UNICAM_8BIT,0x4B0A,0xD0},// ;HSIZE (3280)
	{UNICAM_8BIT,0x4B0B,0x0C},// ;
	{UNICAM_8BIT,0x0348,0x0C},// ;X-END (3279)
	{UNICAM_8BIT,0x0349,0xCF},// ;
	{UNICAM_8BIT,0x034A,0x09},// ;Y-END (2463)
	{UNICAM_8BIT,0x034B,0x9F},// ;
	{UNICAM_8BIT,0x034C,0x0C},// ;HSIZE (3280)
	{UNICAM_8BIT,0x034D,0xD0},// ;
	{UNICAM_8BIT,0x034E,0x09},// ;VSIZE (2464)
	{UNICAM_8BIT,0x034F,0xA0},// ;
	{UNICAM_8BIT,0x0104,0x00},
	{UNICAM_8BIT,0x0100,0x01},
	//{UNICAM_TOK_DELAY,0,0x45},// 68ms

	{UNICAM_TOK_TERM, 0, 0}
};

S_UNI_REG hm8131_1616_916[] = {
	{UNICAM_8BIT, 0x0100, 0x00},
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay		
	{UNICAM_8BIT,0x0103,0x00},// ; software reset-> was 0x22
	{UNICAM_8BIT, 0x0100, 0x02},
	{UNICAM_8BIT,0x3519,0x00},//;// PWE interval high byte
	{UNICAM_8BIT,0x351A,0x05},//;// PWE interval low byte
	{UNICAM_8BIT,0x351B,0x1E},//;// PWE interval high byte
	{UNICAM_8BIT,0x351C,0x90},//;// PWE interval low byte
	{UNICAM_8BIT,0x351E,0x15},//;// PRD high pulse byte
	{UNICAM_8BIT,0x351D,0x15},// ;// PRD interval low byte
	{UNICAM_8BIT,0x4001,0x80},//;
	{UNICAM_8BIT,0xBA93,0x01},//; Read Header
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x412A,0x8A},//;
	{UNICAM_8BIT,0xBAA2,0xC0},//;
	{UNICAM_8BIT,0xBAA2,0xC0},//;
	{UNICAM_8BIT,0xBAA2,0xC0},//;
	{UNICAM_8BIT,0xBAA2,0x40},//;
	{UNICAM_8BIT,0x412A,0x8A},//;
	{UNICAM_8BIT,0x412A,0x8A},//;
	{UNICAM_8BIT,0x412A,0x0A},//;
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0xBA93,0x03},//; Read Header
	{UNICAM_8BIT,0xBA93,0x02},//; Read Header
	{UNICAM_8BIT,0xBA90,0x01},//; BPS_OTP_EN[0]
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x0100,0x02},//; power up
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x4001,0x00},// ; ck_cfg - choose mclk_pll -> was 0025
	{UNICAM_8BIT,0x0309,0x02},// ; PLL mipi1 - enabled PLL -> was 002D
	{UNICAM_8BIT,0x0305,0x0C},// ; PLL N, mclk 24mhz
	{UNICAM_8BIT,0x0307,0x46},// ; PLL M, pclk_raw=71/2=35mhz
	{UNICAM_8BIT,0x030D,0x0C},// ; PLL N, 
	{UNICAM_8BIT,0x030F,0x62},// ; PLL M, pkt_clk=98/2=49mhz
	{UNICAM_8BIT,0x414A,0x02},// ; BLC IIR setting
	{UNICAM_8BIT,0x4147,0x03},// ;
	{UNICAM_8BIT,0x4144,0x03},// ; 
	{UNICAM_8BIT,0x4145,0x31},// ; By Willie enalbe CFPN
	{UNICAM_8BIT,0x4146,0x51},// ; 
	{UNICAM_8BIT,0x4149,0x57},// ; By Ken Wu to improve the beating (BLC dither and hysterisis enable)
	{UNICAM_8BIT,0x4260,0x00},// ; By Simon
	{UNICAM_8BIT,0x4261,0x00},// ; By Simon
	{UNICAM_8BIT,0x4264,0x14},// ; By Simon
	{UNICAM_8BIT,0x426A,0x01},// ;
	{UNICAM_8BIT,0x4270,0x08},// ; By Willie 
	{UNICAM_8BIT,0x4271,0x8A},// ; By Willie
	{UNICAM_8BIT,0x4272,0x22},// ;
	{UNICAM_8BIT,0x427D,0x00},// ; By Simon
	{UNICAM_8BIT,0x427E,0x03},// ; By Simon
	{UNICAM_8BIT,0x427F,0x00},// ;
	{UNICAM_8BIT,0x4380,0xA6},// ; 
	{UNICAM_8BIT,0x4381,0x7B},// ; By Willie to save ramp power
	{UNICAM_8BIT,0x4383,0x58},// ; By Willie (get brighter image at low light), increase VRNP voltage.
	{UNICAM_8BIT,0x4387,0x37},// ; Adjust clamping level
	{UNICAM_8BIT,0x4386,0x00},// ; 
	{UNICAM_8BIT,0x4382,0x00},// ;
	{UNICAM_8BIT,0x4388,0x9F},// ;
	{UNICAM_8BIT,0x4389,0x15},// ;
	{UNICAM_8BIT,0x438A,0x80},// ;
	{UNICAM_8BIT,0x438C,0x0F},// ;
	{UNICAM_8BIT,0x4384,0x14},// ; 
	{UNICAM_8BIT,0x438B,0x00},// ;
	{UNICAM_8BIT,0x4385,0xA5},// ; 
	{UNICAM_8BIT,0x438F,0x00},// ;
	{UNICAM_8BIT,0x438D,0xA0},// ;
	{UNICAM_8BIT,0x4B11,0x1F},// ;
	{UNICAM_8BIT,0x4B44,0x40},// ;
	{UNICAM_8BIT,0x4B46,0x03},// ;
	{UNICAM_8BIT,0x4B47,0xC9},// ;
	{UNICAM_8BIT,0x44B0,0x03},// ;
	{UNICAM_8BIT,0x44B1,0x01},// ;
	{UNICAM_8BIT,0x44B2,0x00},// ;
	{UNICAM_8BIT,0x44B3,0x04},// ;
	{UNICAM_8BIT,0x44B4,0x14},// ;
	{UNICAM_8BIT,0x44B5,0x24},// ;
	{UNICAM_8BIT,0x44B8,0x03},// ;
	{UNICAM_8BIT,0x44B9,0x01},// ;
	{UNICAM_8BIT,0x44BA,0x05},// ;
	{UNICAM_8BIT,0x44BB,0x15},// ;
	{UNICAM_8BIT,0x44BC,0x25},// ;
	{UNICAM_8BIT,0x44BD,0x35},// ;
	{UNICAM_8BIT,0x44D0,0xC0},// ;
	{UNICAM_8BIT,0x44D1,0x80},// ;
	{UNICAM_8BIT,0x44D2,0x40},// ;
	{UNICAM_8BIT,0x44D3,0x40},// ;
	{UNICAM_8BIT,0x44D4,0x40},// ;
	{UNICAM_8BIT,0x44D5,0x40},// ;
	{UNICAM_8BIT,0x4B07,0xF0},// ; by willie_cheng extend mark1 ~ 1200us
	{UNICAM_8BIT,0x4131,0x01},// ;	//BLI
	{UNICAM_8BIT,0x060B,0x01},// ;	//test pattern PN9 seed
	{UNICAM_8BIT,0x4274,0x33},// ; [5] mask out bad frame due to mode (flip/mirror) change
	{UNICAM_8BIT,0x400D,0x04},// ; [2] binning mode analog gain table -> HII binning gain table
	{UNICAM_8BIT,0x3110,0x01},// ;
	{UNICAM_8BIT,0x3111,0x01},// ;
	{UNICAM_8BIT,0x3130,0x01},// ;
	{UNICAM_8BIT,0x3131,0x26},// ;
	{UNICAM_8BIT,0x0383,0x03},// ; x odd increment -> was 01
	{UNICAM_8BIT,0x0387,0x03},// ; y odd increment -> was 01
	{UNICAM_8BIT,0x0390,0x11},// ; binning mode -> was 00	1:binning 2:summing
	{UNICAM_8BIT,0x0348,0x0C},// ; Image size X end Hb
	{UNICAM_8BIT,0x0349,0xCD},// ; Image size X end Lb
	{UNICAM_8BIT,0x034A,0x09},// ; Image size Y end Hb
	{UNICAM_8BIT,0x034B,0x9D},// ; Image size Y end Lb
	{UNICAM_8BIT,0x0340,0x05},// ; frame length lines Hb
	{UNICAM_8BIT,0x0341,0x1A},// ; frame length lines Lb
	{UNICAM_8BIT,0x4B31,0x06},// ;[2] PKTCLK_PHASE_SEL
	{UNICAM_8BIT,0x034C,0x06},// ; HSIZE (1640)
	{UNICAM_8BIT,0x034D,0x68},// ; HSIZE (1640)
	{UNICAM_8BIT,0x034E,0x03},// ; VSIZE (1232)
	{UNICAM_8BIT,0x034F,0xA4},// ; VSIZE (1232)
	{UNICAM_8BIT,0x4B18,0x06},// ; [7:0] FULL_TRIGGER_TIME 
	{UNICAM_8BIT,0x0111,0x01},// ; B5: 1'b0:2lane, 1'b1:4lane (org 4B19)
	{UNICAM_8BIT,0x4B20,0xDE},// ; clock always on(9E)
	{UNICAM_8BIT,0x4B02,0x02},// ; lpx_width
	{UNICAM_8BIT,0x4B03,0x05},// ; hs_zero_width
	{UNICAM_8BIT,0x4B04,0x02},// ; hs_trail_width
	{UNICAM_8BIT,0x4B05,0x0C},// ; clk_zero_width
	{UNICAM_8BIT,0x4B06,0x03},// ; clk_trail_width
	{UNICAM_8BIT,0x4B0f,0x07},// ; clk_back_porch
	{UNICAM_8BIT,0x4B39,0x02},// ; clk_width_exit
	{UNICAM_8BIT,0x4B3f,0x08},// ; [3:0] mipi_req_dly
	{UNICAM_8BIT,0x4B42,0x02},// ; HS_PREPARE_WIDTH
	{UNICAM_8BIT,0x4B43,0x02},// ; CLK_PREPARE_WIDTH
	{UNICAM_8BIT,0x4B0E,0x01},// ;Tclk_pre > 0, if H_SIZE/16!=0 
	{UNICAM_8BIT,0x4024,0x40},// ; enabled MIPI -> was 0024
	{UNICAM_8BIT,0x4800,0x00},// ; was CMU 0000
	{UNICAM_8BIT,0x0104,0x01},// ; was 0100
	{UNICAM_8BIT,0x0104,0x00},// ; was 0100, need an high to low edge now! (CMU_AE)
	{UNICAM_8BIT,0x4801,0x00},// ; was 0106
	{UNICAM_8BIT,0x0000,0x00},// ; was 0000
	{UNICAM_8BIT,0xBA94,0x01},//;
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0xBA94,0x00},//;
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0xBA93,0x02},//; 
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x0100,0x01},// ; was 0005 ; mode_select

	{UNICAM_TOK_TERM, 0, 0}
};


S_UNI_REG hm8131_1640_1232[] = {
	{UNICAM_8BIT, 0x0100, 0x00},
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay		
	{UNICAM_8BIT,0x0103,0x00},// ; software reset-> was 0x22
	{UNICAM_8BIT, 0x0100, 0x02},
	{UNICAM_8BIT,0x3519,0x00},//;// PWE interval high byte
	{UNICAM_8BIT,0x351A,0x05},//;// PWE interval low byte
	{UNICAM_8BIT,0x351B,0x1E},//;// PWE interval high byte
	{UNICAM_8BIT,0x351C,0x90},//;// PWE interval low byte
	{UNICAM_8BIT,0x351E,0x15},//;// PRD high pulse byte
	{UNICAM_8BIT,0x351D,0x15},// ;// PRD interval low byte
	{UNICAM_8BIT,0x4001,0x80},//;
	{UNICAM_8BIT,0xBA93,0x01},//; Read Header
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x412A,0x8A},//;
	{UNICAM_8BIT,0xBAA2,0xC0},//;
	{UNICAM_8BIT,0xBAA2,0xC0},//;
	{UNICAM_8BIT,0xBAA2,0xC0},//;
	{UNICAM_8BIT,0xBAA2,0x40},//;
	{UNICAM_8BIT,0x412A,0x8A},//;
	{UNICAM_8BIT,0x412A,0x8A},//;
	{UNICAM_8BIT,0x412A,0x0A},//;
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0xBA93,0x03},//; Read Header
	{UNICAM_8BIT,0xBA93,0x02},//; Read Header
	{UNICAM_8BIT,0xBA90,0x01},//; BPS_OTP_EN[0]
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x0100,0x02},//; power up
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x4001,0x00},// ; ck_cfg - choose mclk_pll -> was 0025
	{UNICAM_8BIT,0x0309,0x02},// ; PLL mipi1 - enabled PLL -> was 002D
	{UNICAM_8BIT,0x0305,0x0C},// ; PLL N, mclk 24mhz
	{UNICAM_8BIT,0x0307,0x46},// ; PLL M, pclk_raw=71/2=35mhz
	{UNICAM_8BIT,0x030D,0x0C},// ; PLL N, 
	{UNICAM_8BIT,0x030F,0x62},// ; PLL M, pkt_clk=98/2=49mhz
	{UNICAM_8BIT,0x414A,0x02},// ; BLC IIR setting
	{UNICAM_8BIT,0x4147,0x03},// ;
	{UNICAM_8BIT,0x4144,0x03},// ; 
	{UNICAM_8BIT,0x4145,0x31},// ; By Willie enalbe CFPN
	{UNICAM_8BIT,0x4146,0x51},// ; 
	{UNICAM_8BIT,0x4149,0x57},// ; By Ken Wu to improve the beating (BLC dither and hysterisis enable)
	{UNICAM_8BIT,0x4260,0x00},// ; By Simon
	{UNICAM_8BIT,0x4261,0x00},// ; By Simon
	{UNICAM_8BIT,0x4264,0x14},// ; By Simon
	{UNICAM_8BIT,0x426A,0x01},// ;
	{UNICAM_8BIT,0x4270,0x08},// ; By Willie 
	{UNICAM_8BIT,0x4271,0x8A},// ; By Willie
	{UNICAM_8BIT,0x4272,0x22},// ;
	{UNICAM_8BIT,0x427D,0x00},// ; By Simon
	{UNICAM_8BIT,0x427E,0x03},// ; By Simon
	{UNICAM_8BIT,0x427F,0x00},// ;
	{UNICAM_8BIT,0x4380,0xA6},// ; 
	{UNICAM_8BIT,0x4381,0x7B},// ; By Willie to save ramp power
	{UNICAM_8BIT,0x4383,0x58},// ; By Willie (get brighter image at low light), increase VRNP voltage.
	{UNICAM_8BIT,0x4387,0x37},// ; Adjust clamping level
	{UNICAM_8BIT,0x4386,0x00},// ; 
	{UNICAM_8BIT,0x4382,0x00},// ;
	{UNICAM_8BIT,0x4388,0x9F},// ;
	{UNICAM_8BIT,0x4389,0x15},// ;
	{UNICAM_8BIT,0x438A,0x80},// ;
	{UNICAM_8BIT,0x438C,0x0F},// ;
	{UNICAM_8BIT,0x4384,0x14},// ; 
	{UNICAM_8BIT,0x438B,0x00},// ;
	{UNICAM_8BIT,0x4385,0xA5},// ; 
	{UNICAM_8BIT,0x438F,0x00},// ;
	{UNICAM_8BIT,0x438D,0xA0},// ;
	{UNICAM_8BIT,0x4B11,0x1F},// ;
	{UNICAM_8BIT,0x4B44,0x40},// ;
	{UNICAM_8BIT,0x4B46,0x03},// ;
	{UNICAM_8BIT,0x4B47,0xC9},// ;
	{UNICAM_8BIT,0x44B0,0x03},// ;
	{UNICAM_8BIT,0x44B1,0x01},// ;
	{UNICAM_8BIT,0x44B2,0x00},// ;
	{UNICAM_8BIT,0x44B3,0x04},// ;
	{UNICAM_8BIT,0x44B4,0x14},// ;
	{UNICAM_8BIT,0x44B5,0x24},// ;
	{UNICAM_8BIT,0x44B8,0x03},// ;
	{UNICAM_8BIT,0x44B9,0x01},// ;
	{UNICAM_8BIT,0x44BA,0x05},// ;
	{UNICAM_8BIT,0x44BB,0x15},// ;
	{UNICAM_8BIT,0x44BC,0x25},// ;
	{UNICAM_8BIT,0x44BD,0x35},// ;
	{UNICAM_8BIT,0x44D0,0xC0},// ;
	{UNICAM_8BIT,0x44D1,0x80},// ;
	{UNICAM_8BIT,0x44D2,0x40},// ;
	{UNICAM_8BIT,0x44D3,0x40},// ;
	{UNICAM_8BIT,0x44D4,0x40},// ;
	{UNICAM_8BIT,0x44D5,0x40},// ;
	{UNICAM_8BIT,0x4B07,0xF0},// ; by willie_cheng extend mark1 ~ 1200us
	{UNICAM_8BIT,0x4131,0x01},// ;	//BLI
	{UNICAM_8BIT,0x060B,0x01},// ;	//test pattern PN9 seed
	{UNICAM_8BIT,0x4274,0x33},// ; [5] mask out bad frame due to mode (flip/mirror) change
	{UNICAM_8BIT,0x400D,0x04},// ; [2] binning mode analog gain table -> HII binning gain table
	{UNICAM_8BIT,0x3110,0x01},// ;
	{UNICAM_8BIT,0x3111,0x01},// ;
	{UNICAM_8BIT,0x3130,0x01},// ;
	{UNICAM_8BIT,0x3131,0x26},// ;
	{UNICAM_8BIT,0x0383,0x03},// ; x odd increment -> was 01
	{UNICAM_8BIT,0x0387,0x03},// ; y odd increment -> was 01
	{UNICAM_8BIT,0x0390,0x11},// ; binning mode -> was 00	1:binning 2:summing
	{UNICAM_8BIT,0x0348,0x0C},// ; Image size X end Hb
	{UNICAM_8BIT,0x0349,0xCD},// ; Image size X end Lb
	{UNICAM_8BIT,0x034A,0x09},// ; Image size Y end Hb
	{UNICAM_8BIT,0x034B,0x9D},// ; Image size Y end Lb
	{UNICAM_8BIT,0x0340,0x05},// ; frame length lines Hb
	{UNICAM_8BIT,0x0341,0x1A},// ; frame length lines Lb
	{UNICAM_8BIT,0x4B31,0x06},// ;[2] PKTCLK_PHASE_SEL
	{UNICAM_8BIT,0x034C,0x06},// ; HSIZE (1640)
	{UNICAM_8BIT,0x034D,0x68},// ; HSIZE (1640)
	{UNICAM_8BIT,0x034E,0x04},// ; VSIZE (1232)
	{UNICAM_8BIT,0x034F,0xD0},// ; VSIZE (1232)
	{UNICAM_8BIT,0x4B18,0x06},// ; [7:0] FULL_TRIGGER_TIME 
	{UNICAM_8BIT,0x0111,0x01},// ; B5: 1'b0:2lane, 1'b1:4lane (org 4B19)
	{UNICAM_8BIT,0x4B20,0xDE},// ; clock always on(9E)
	{UNICAM_8BIT,0x4B02,0x02},// ; lpx_width
	{UNICAM_8BIT,0x4B03,0x05},// ; hs_zero_width
	{UNICAM_8BIT,0x4B04,0x02},// ; hs_trail_width
	{UNICAM_8BIT,0x4B05,0x0C},// ; clk_zero_width
	{UNICAM_8BIT,0x4B06,0x03},// ; clk_trail_width
	{UNICAM_8BIT,0x4B0f,0x07},// ; clk_back_porch
	{UNICAM_8BIT,0x4B39,0x02},// ; clk_width_exit
	{UNICAM_8BIT,0x4B3f,0x08},// ; [3:0] mipi_req_dly
	{UNICAM_8BIT,0x4B42,0x02},// ; HS_PREPARE_WIDTH
	{UNICAM_8BIT,0x4B43,0x02},// ; CLK_PREPARE_WIDTH
	{UNICAM_8BIT,0x4B0E,0x01},// ;Tclk_pre > 0, if H_SIZE/16!=0 
	{UNICAM_8BIT,0x4024,0x40},// ; enabled MIPI -> was 0024
	{UNICAM_8BIT,0x4800,0x00},// ; was CMU 0000
	{UNICAM_8BIT,0x0104,0x01},// ; was 0100
	{UNICAM_8BIT,0x0104,0x00},// ; was 0100, need an high to low edge now! (CMU_AE)
	{UNICAM_8BIT,0x4801,0x00},// ; was 0106
	{UNICAM_8BIT,0x0000,0x00},// ; was 0000
	{UNICAM_8BIT,0xBA94,0x01},//;
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0xBA94,0x00},//;
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0xBA93,0x02},//; 
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x0100,0x01},// ; was 0005 ; mode_select

	{UNICAM_TOK_TERM, 0, 0}
};

#if 0
S_UNI_REG hm8131_regress[][] = {
	hm8131_regress0,
	hm8131_regress1,
	hm8131_regress2,
};
#endif

S_UNI_REG hm8131_368_304[] = {
	{UNICAM_8BIT, 0x0100, 0x00},
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay		
	{UNICAM_8BIT,0x0103,0x00},// ; software reset-> was 0x22
	{UNICAM_8BIT, 0x0100, 0x02},
	{UNICAM_8BIT,0x3519,0x00},//;// PWE interval high byte
	{UNICAM_8BIT,0x351A,0x05},//;// PWE interval low byte
	{UNICAM_8BIT,0x351B,0x1E},//;// PWE interval high byte
	{UNICAM_8BIT,0x351C,0x90},//;// PWE interval low byte
	{UNICAM_8BIT,0x351E,0x15},//;// PRD high pulse byte
	{UNICAM_8BIT,0x351D,0x15},// ;// PRD interval low byte
	{UNICAM_8BIT,0x4001,0x80},//;
	{UNICAM_8BIT,0xBA93,0x01},//; Read Header
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x412A,0x8A},//;
	{UNICAM_8BIT,0xBAA2,0xC0},//;
	{UNICAM_8BIT,0xBAA2,0xC0},//;
	{UNICAM_8BIT,0xBAA2,0xC0},//;
	{UNICAM_8BIT,0xBAA2,0x40},//;
	{UNICAM_8BIT,0x412A,0x8A},//;
	{UNICAM_8BIT,0x412A,0x8A},//;
	{UNICAM_8BIT,0x412A,0x0A},//;
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0xBA93,0x03},//; Read Header
	{UNICAM_8BIT,0xBA93,0x02},//; Read Header
	{UNICAM_8BIT,0xBA90,0x01},//; BPS_OTP_EN[0]
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x0100,0x02},//; power up
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x4001,0x00},// ; ck_cfg - choose mclk_pll -> was 0025
	{UNICAM_8BIT,0x0309,0x02},// ; PLL mipi1 - enabled PLL -> was 002D
	{UNICAM_8BIT,0x0305,0x0C},// ; PLL N, mclk 24mhz
	{UNICAM_8BIT,0x0307,0x46},// ; PLL M, pclk_raw=71/2=35mhz
	{UNICAM_8BIT,0x030D,0x0C},// ; PLL N, 
	{UNICAM_8BIT,0x030F,0x62},// ; PLL M, pkt_clk=98/2=49mhz
	{UNICAM_8BIT,0x414A,0x02},// ; BLC IIR setting
	{UNICAM_8BIT,0x4147,0x03},// ;
	{UNICAM_8BIT,0x4144,0x03},// ; 
	{UNICAM_8BIT,0x4145,0x31},// ; By Willie enalbe CFPN
	{UNICAM_8BIT,0x4146,0x51},// ; 
	{UNICAM_8BIT,0x4149,0x57},// ; By Ken Wu to improve the beating (BLC dither and hysterisis enable)
	{UNICAM_8BIT,0x4260,0x00},// ; By Simon
	{UNICAM_8BIT,0x4261,0x00},// ; By Simon
	{UNICAM_8BIT,0x4264,0x14},// ; By Simon
	{UNICAM_8BIT,0x426A,0x01},// ;
	{UNICAM_8BIT,0x4270,0x08},// ; By Willie 
	{UNICAM_8BIT,0x4271,0x8A},// ; By Willie
	{UNICAM_8BIT,0x4272,0x22},// ;
	{UNICAM_8BIT,0x427D,0x00},// ; By Simon
	{UNICAM_8BIT,0x427E,0x03},// ; By Simon
	{UNICAM_8BIT,0x427F,0x00},// ;
	{UNICAM_8BIT,0x4380,0xA6},// ; 
	{UNICAM_8BIT,0x4381,0x7B},// ; By Willie to save ramp power
	{UNICAM_8BIT,0x4383,0x58},// ; By Willie (get brighter image at low light), increase VRNP voltage.
	{UNICAM_8BIT,0x4387,0x37},// ; Adjust clamping level
	{UNICAM_8BIT,0x4386,0x00},// ; 
	{UNICAM_8BIT,0x4382,0x00},// ;
	{UNICAM_8BIT,0x4388,0x9F},// ;
	{UNICAM_8BIT,0x4389,0x15},// ;
	{UNICAM_8BIT,0x438A,0x80},// ;
	{UNICAM_8BIT,0x438C,0x0F},// ;
	{UNICAM_8BIT,0x4384,0x14},// ; 
	{UNICAM_8BIT,0x438B,0x00},// ;
	{UNICAM_8BIT,0x4385,0xA5},// ; 
	{UNICAM_8BIT,0x438F,0x00},// ;
	{UNICAM_8BIT,0x438D,0xA0},// ;
	{UNICAM_8BIT,0x4B11,0x1F},// ;
	{UNICAM_8BIT,0x4B44,0x40},// ;
	{UNICAM_8BIT,0x4B46,0x03},// ;
	{UNICAM_8BIT,0x4B47,0xC9},// ;
	{UNICAM_8BIT,0x44B0,0x03},// ;
	{UNICAM_8BIT,0x44B1,0x01},// ;
	{UNICAM_8BIT,0x44B2,0x00},// ;
	{UNICAM_8BIT,0x44B3,0x04},// ;
	{UNICAM_8BIT,0x44B4,0x14},// ;
	{UNICAM_8BIT,0x44B5,0x24},// ;
	{UNICAM_8BIT,0x44B8,0x03},// ;
	{UNICAM_8BIT,0x44B9,0x01},// ;
	{UNICAM_8BIT,0x44BA,0x05},// ;
	{UNICAM_8BIT,0x44BB,0x15},// ;
	{UNICAM_8BIT,0x44BC,0x25},// ;
	{UNICAM_8BIT,0x44BD,0x35},// ;
	{UNICAM_8BIT,0x44D0,0xC0},// ;
	{UNICAM_8BIT,0x44D1,0x80},// ;
	{UNICAM_8BIT,0x44D2,0x40},// ;
	{UNICAM_8BIT,0x44D3,0x40},// ;
	{UNICAM_8BIT,0x44D4,0x40},// ;
	{UNICAM_8BIT,0x44D5,0x40},// ;
	{UNICAM_8BIT,0x4B07,0xF0},// ; by willie_cheng extend mark1 ~ 1200us
	{UNICAM_8BIT,0x4131,0x01},// ;	//BLI
	{UNICAM_8BIT,0x060B,0x01},// ;	//test pattern PN9 seed
	{UNICAM_8BIT,0x4274,0x33},// ; [5] mask out bad frame due to mode (flip/mirror) change
	{UNICAM_8BIT,0x400D,0x04},// ; [2] binning mode analog gain table -> HII binning gain table
	{UNICAM_8BIT,0x3110,0x01},// ;
	{UNICAM_8BIT,0x3111,0x01},// ;
	{UNICAM_8BIT,0x3130,0x01},// ;
	{UNICAM_8BIT,0x3131,0x26},// ;
	{UNICAM_8BIT,0x0383,0x03},// ; x odd increment -> was 01
	{UNICAM_8BIT,0x0387,0x03},// ; y odd increment -> was 01
	{UNICAM_8BIT,0x0390,0x11},// ; binning mode -> was 00	1:binning 2:summing
	{UNICAM_8BIT,0x0348,0x0C},// ; Image size X end Hb
	{UNICAM_8BIT,0x0349,0xCD},// ; Image size X end Lb
	{UNICAM_8BIT,0x034A,0x09},// ; Image size Y end Hb
	{UNICAM_8BIT,0x034B,0x9D},// ; Image size Y end Lb
	{UNICAM_8BIT,0x0340,0x05},// ; frame length lines Hb
	{UNICAM_8BIT,0x0341,0x1A},// ; frame length lines Lb
	{UNICAM_8BIT,0x4B31,0x06},// ;[2] PKTCLK_PHASE_SEL
	{UNICAM_8BIT,0x034C,0x01},// ; HSIZE (368)
	{UNICAM_8BIT,0x034D,0x70},// ;
	{UNICAM_8BIT,0x034E,0x01},// ; VSIZE (304)
	{UNICAM_8BIT,0x034F,0x30},// ;
	{UNICAM_8BIT,0x4B18,0x06},// ; [7:0] FULL_TRIGGER_TIME 
	{UNICAM_8BIT,0x0111,0x01},// ; B5: 1'b0:2lane, 1'b1:4lane (org 4B19)
	{UNICAM_8BIT,0x4B20,0xDE},// ; clock always on(9E)
	{UNICAM_8BIT,0x4B02,0x02},// ; lpx_width
	{UNICAM_8BIT,0x4B03,0x05},// ; hs_zero_width
	{UNICAM_8BIT,0x4B04,0x02},// ; hs_trail_width
	{UNICAM_8BIT,0x4B05,0x0C},// ; clk_zero_width
	{UNICAM_8BIT,0x4B06,0x03},// ; clk_trail_width
	{UNICAM_8BIT,0x4B0f,0x07},// ; clk_back_porch
	{UNICAM_8BIT,0x4B39,0x02},// ; clk_width_exit
	{UNICAM_8BIT,0x4B3f,0x08},// ; [3:0] mipi_req_dly
	{UNICAM_8BIT,0x4B42,0x02},// ; HS_PREPARE_WIDTH
	{UNICAM_8BIT,0x4B43,0x02},// ; CLK_PREPARE_WIDTH
	{UNICAM_8BIT,0x4B0E,0x01},// ;Tclk_pre > 0, if H_SIZE/16!=0 
	{UNICAM_8BIT,0x4024,0x40},// ; enabled MIPI -> was 0024
	{UNICAM_8BIT,0x4800,0x00},// ; was CMU 0000
	{UNICAM_8BIT,0x0104,0x01},// ; was 0100
	{UNICAM_8BIT,0x0104,0x00},// ; was 0100, need an high to low edge now! (CMU_AE)
	{UNICAM_8BIT,0x4801,0x00},// ; was 0106
	{UNICAM_8BIT,0x0000,0x00},// ; was 0000
	{UNICAM_8BIT,0xBA94,0x01},//;
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0xBA94,0x00},//;
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0xBA93,0x02},//; 
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x0100,0x01},// ; was 0005 ; mode_select
	{UNICAM_TOK_TERM, 0, 0}
};

S_UNI_REG hm8131_1296_736[] = {
	{UNICAM_8BIT, 0x0100, 0x00},
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay		
	{UNICAM_8BIT,0x0103,0x00},// ; software reset-> was 0x22
	{UNICAM_8BIT, 0x0100, 0x02},
	{UNICAM_8BIT,0x3519,0x00},//;// PWE interval high byte
	{UNICAM_8BIT,0x351A,0x05},//;// PWE interval low byte
	{UNICAM_8BIT,0x351B,0x1E},//;// PWE interval high byte
	{UNICAM_8BIT,0x351C,0x90},//;// PWE interval low byte
	{UNICAM_8BIT,0x351E,0x15},//;// PRD high pulse byte
	{UNICAM_8BIT,0x351D,0x15},// ;// PRD interval low byte
	{UNICAM_8BIT,0x4001,0x80},//;
	{UNICAM_8BIT,0xBA93,0x01},//; Read Header
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x412A,0x8A},//;
	{UNICAM_8BIT,0xBAA2,0xC0},//;
	{UNICAM_8BIT,0xBAA2,0xC0},//;
	{UNICAM_8BIT,0xBAA2,0xC0},//;
	{UNICAM_8BIT,0xBAA2,0x40},//;
	{UNICAM_8BIT,0x412A,0x8A},//;
	{UNICAM_8BIT,0x412A,0x8A},//;
	{UNICAM_8BIT,0x412A,0x0A},//;
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0xBA93,0x03},//; Read Header
	{UNICAM_8BIT,0xBA93,0x02},//; Read Header
	{UNICAM_8BIT,0xBA90,0x01},//; BPS_OTP_EN[0]
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x0100,0x02},//; power up
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x4001,0x00},// ; ck_cfg - choose mclk_pll -> was 0025
	{UNICAM_8BIT,0x0309,0x02},// ; PLL mipi1 - enabled PLL -> was 002D
	{UNICAM_8BIT,0x0305,0x0C},// ; PLL N, mclk 24mhz
	{UNICAM_8BIT,0x0307,0x46},// ; PLL M, pclk_raw=71/2=35mhz
	{UNICAM_8BIT,0x030D,0x0C},// ; PLL N, 
	{UNICAM_8BIT,0x030F,0x62},// ; PLL M, pkt_clk=98/2=49mhz
	{UNICAM_8BIT,0x414A,0x02},// ; BLC IIR setting
	{UNICAM_8BIT,0x4147,0x03},// ;
	{UNICAM_8BIT,0x4144,0x03},// ; 
	{UNICAM_8BIT,0x4145,0x31},// ; By Willie enalbe CFPN
	{UNICAM_8BIT,0x4146,0x51},// ; 
	{UNICAM_8BIT,0x4149,0x57},// ; By Ken Wu to improve the beating (BLC dither and hysterisis enable)
	{UNICAM_8BIT,0x4260,0x00},// ; By Simon
	{UNICAM_8BIT,0x4261,0x00},// ; By Simon
	{UNICAM_8BIT,0x4264,0x14},// ; By Simon
	{UNICAM_8BIT,0x426A,0x01},// ;
	{UNICAM_8BIT,0x4270,0x08},// ; By Willie 
	{UNICAM_8BIT,0x4271,0x8A},// ; By Willie
	{UNICAM_8BIT,0x4272,0x22},// ;
	{UNICAM_8BIT,0x427D,0x00},// ; By Simon
	{UNICAM_8BIT,0x427E,0x03},// ; By Simon
	{UNICAM_8BIT,0x427F,0x00},// ;
	{UNICAM_8BIT,0x4380,0xA6},// ; 
	{UNICAM_8BIT,0x4381,0x7B},// ; By Willie to save ramp power
	{UNICAM_8BIT,0x4383,0x58},// ; By Willie (get brighter image at low light), increase VRNP voltage.
	{UNICAM_8BIT,0x4387,0x37},// ; Adjust clamping level
	{UNICAM_8BIT,0x4386,0x00},// ; 
	{UNICAM_8BIT,0x4382,0x00},// ;
	{UNICAM_8BIT,0x4388,0x9F},// ;
	{UNICAM_8BIT,0x4389,0x15},// ;
	{UNICAM_8BIT,0x438A,0x80},// ;
	{UNICAM_8BIT,0x438C,0x0F},// ;
	{UNICAM_8BIT,0x4384,0x14},// ; 
	{UNICAM_8BIT,0x438B,0x00},// ;
	{UNICAM_8BIT,0x4385,0xA5},// ; 
	{UNICAM_8BIT,0x438F,0x00},// ;
	{UNICAM_8BIT,0x438D,0xA0},// ;
	{UNICAM_8BIT,0x4B11,0x1F},// ;
	{UNICAM_8BIT,0x4B44,0x40},// ;
	{UNICAM_8BIT,0x4B46,0x03},// ;
	{UNICAM_8BIT,0x4B47,0xC9},// ;
	{UNICAM_8BIT,0x44B0,0x03},// ;
	{UNICAM_8BIT,0x44B1,0x01},// ;
	{UNICAM_8BIT,0x44B2,0x00},// ;
	{UNICAM_8BIT,0x44B3,0x04},// ;
	{UNICAM_8BIT,0x44B4,0x14},// ;
	{UNICAM_8BIT,0x44B5,0x24},// ;
	{UNICAM_8BIT,0x44B8,0x03},// ;
	{UNICAM_8BIT,0x44B9,0x01},// ;
	{UNICAM_8BIT,0x44BA,0x05},// ;
	{UNICAM_8BIT,0x44BB,0x15},// ;
	{UNICAM_8BIT,0x44BC,0x25},// ;
	{UNICAM_8BIT,0x44BD,0x35},// ;
	{UNICAM_8BIT,0x44D0,0xC0},// ;
	{UNICAM_8BIT,0x44D1,0x80},// ;
	{UNICAM_8BIT,0x44D2,0x40},// ;
	{UNICAM_8BIT,0x44D3,0x40},// ;
	{UNICAM_8BIT,0x44D4,0x40},// ;
	{UNICAM_8BIT,0x44D5,0x40},// ;
	{UNICAM_8BIT,0x4B07,0xF0},// ; by willie_cheng extend mark1 ~ 1200us
	{UNICAM_8BIT,0x4131,0x01},// ;	//BLI
	{UNICAM_8BIT,0x060B,0x01},// ;	//test pattern PN9 seed
	{UNICAM_8BIT,0x4274,0x33},// ; [5] mask out bad frame due to mode (flip/mirror) change
	{UNICAM_8BIT,0x400D,0x04},// ; [2] binning mode analog gain table -> HII binning gain table
	{UNICAM_8BIT,0x3110,0x01},// ;
	{UNICAM_8BIT,0x3111,0x01},// ;
	{UNICAM_8BIT,0x3130,0x01},// ;
	{UNICAM_8BIT,0x3131,0x26},// ;
	{UNICAM_8BIT,0x0383,0x03},// ; x odd increment -> was 01
	{UNICAM_8BIT,0x0387,0x03},// ; y odd increment -> was 01
	{UNICAM_8BIT,0x0390,0x11},// ; binning mode -> was 00	1:binning 2:summing
	{UNICAM_8BIT,0x0348,0x0C},// ; Image size X end Hb
	{UNICAM_8BIT,0x0349,0xCD},// ; Image size X end Lb
	{UNICAM_8BIT,0x034A,0x09},// ; Image size Y end Hb
	{UNICAM_8BIT,0x034B,0x9D},// ; Image size Y end Lb
	{UNICAM_8BIT,0x0340,0x05},// ; frame length lines Hb
	{UNICAM_8BIT,0x0341,0x1A},// ; frame length lines Lb
	{UNICAM_8BIT,0x4B31,0x06},// ;[2] PKTCLK_PHASE_SEL
	{UNICAM_8BIT,0x034C,0x05},// ; HSIZE (1296)
	{UNICAM_8BIT,0x034D,0x10},// ; HSIZE (1296)
	{UNICAM_8BIT,0x034E,0x02},// ; VSIZE (736)
	{UNICAM_8BIT,0x034F,0xE0},// ; VSIZE (736)
	{UNICAM_8BIT,0x4B18,0x06},// ; [7:0] FULL_TRIGGER_TIME 
	{UNICAM_8BIT,0x0111,0x01},// ; B5: 1'b0:2lane, 1'b1:4lane (org 4B19)
	{UNICAM_8BIT,0x4B20,0xDE},// ; clock always on(9E)
	{UNICAM_8BIT,0x4B02,0x02},// ; lpx_width
	{UNICAM_8BIT,0x4B03,0x05},// ; hs_zero_width
	{UNICAM_8BIT,0x4B04,0x02},// ; hs_trail_width
	{UNICAM_8BIT,0x4B05,0x0C},// ; clk_zero_width
	{UNICAM_8BIT,0x4B06,0x03},// ; clk_trail_width
	{UNICAM_8BIT,0x4B0f,0x07},// ; clk_back_porch
	{UNICAM_8BIT,0x4B39,0x02},// ; clk_width_exit
	{UNICAM_8BIT,0x4B3f,0x08},// ; [3:0] mipi_req_dly
	{UNICAM_8BIT,0x4B42,0x02},// ; HS_PREPARE_WIDTH
	{UNICAM_8BIT,0x4B43,0x02},// ; CLK_PREPARE_WIDTH
	{UNICAM_8BIT,0x4B0E,0x01},// ;Tclk_pre > 0, if H_SIZE/16!=0 
	{UNICAM_8BIT,0x4024,0x40},// ; enabled MIPI -> was 0024
	{UNICAM_8BIT,0x4800,0x00},// ; was CMU 0000
	{UNICAM_8BIT,0x0104,0x01},// ; was 0100
	{UNICAM_8BIT,0x0104,0x00},// ; was 0100, need an high to low edge now! (CMU_AE)
	{UNICAM_8BIT,0x4801,0x00},// ; was 0106
	{UNICAM_8BIT,0x0000,0x00},// ; was 0000
	{UNICAM_8BIT,0xBA94,0x01},//;
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0xBA94,0x00},//;
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0xBA93,0x02},//; 
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x0100,0x01},// ; was 0005 ; mode_select
	{UNICAM_TOK_TERM, 0, 0}
};

S_UNI_REG hm8131_1296_976[] = {
	{UNICAM_8BIT, 0x0100, 0x00},
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay		
	{UNICAM_8BIT,0x0103,0x00},// ; software reset-> was 0x22
	{UNICAM_8BIT, 0x0100, 0x02},
	{UNICAM_8BIT,0x0103,0x00},// ; software reset-> was 0x22
	{UNICAM_8BIT,0x3519,0x00},//;// PWE interval high byte
	{UNICAM_8BIT,0x351A,0x05},//;// PWE interval low byte
	{UNICAM_8BIT,0x351B,0x1E},//;// PWE interval high byte
	{UNICAM_8BIT,0x351C,0x90},//;// PWE interval low byte
	{UNICAM_8BIT,0x351E,0x15},//;// PRD high pulse byte
	{UNICAM_8BIT,0x351D,0x15},// ;// PRD interval low byte
	{UNICAM_8BIT,0x4001,0x80},//;
	{UNICAM_8BIT,0xBA93,0x01},//; Read Header
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x412A,0x8A},//;
	{UNICAM_8BIT,0xBAA2,0xC0},//;
	{UNICAM_8BIT,0xBAA2,0xC0},//;
	{UNICAM_8BIT,0xBAA2,0xC0},//;
	{UNICAM_8BIT,0xBAA2,0x40},//;
	{UNICAM_8BIT,0x412A,0x8A},//;
	{UNICAM_8BIT,0x412A,0x8A},//;
	{UNICAM_8BIT,0x412A,0x0A},//;
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0xBA93,0x03},//; Read Header
	{UNICAM_8BIT,0xBA93,0x02},//; Read Header
	{UNICAM_8BIT,0xBA90,0x01},//; BPS_OTP_EN[0]
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x0100,0x02},//; power up
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x4001,0x00},// ; ck_cfg - choose mclk_pll -> was 0025
	{UNICAM_8BIT,0x0309,0x02},// ; PLL mipi1 - enabled PLL -> was 002D
	{UNICAM_8BIT,0x0305,0x0C},// ; PLL N, mclk 24mhz
	{UNICAM_8BIT,0x0307,0x46},// ; PLL M, pclk_raw=71/2=35mhz
	{UNICAM_8BIT,0x030D,0x0C},// ; PLL N, 
	{UNICAM_8BIT,0x030F,0x62},// ; PLL M, pkt_clk=98/2=49mhz
	{UNICAM_8BIT,0x414A,0x02},// ; BLC IIR setting
	{UNICAM_8BIT,0x4147,0x03},// ;
	{UNICAM_8BIT,0x4144,0x03},// ; 
	{UNICAM_8BIT,0x4145,0x31},// ; By Willie enalbe CFPN
	{UNICAM_8BIT,0x4146,0x51},// ; 
	{UNICAM_8BIT,0x4149,0x57},// ; By Ken Wu to improve the beating (BLC dither and hysterisis enable)
	{UNICAM_8BIT,0x4260,0x00},// ; By Simon
	{UNICAM_8BIT,0x4261,0x00},// ; By Simon
	{UNICAM_8BIT,0x4264,0x14},// ; By Simon
	{UNICAM_8BIT,0x426A,0x01},// ;
	{UNICAM_8BIT,0x4270,0x08},// ; By Willie 
	{UNICAM_8BIT,0x4271,0x8A},// ; By Willie
	{UNICAM_8BIT,0x4272,0x22},// ;
	{UNICAM_8BIT,0x427D,0x00},// ; By Simon
	{UNICAM_8BIT,0x427E,0x03},// ; By Simon
	{UNICAM_8BIT,0x427F,0x00},// ;
	{UNICAM_8BIT,0x4380,0xA6},// ; 
	{UNICAM_8BIT,0x4381,0x7B},// ; By Willie to save ramp power
	{UNICAM_8BIT,0x4383,0x58},// ; By Willie (get brighter image at low light), increase VRNP voltage.
	{UNICAM_8BIT,0x4387,0x37},// ; Adjust clamping level
	{UNICAM_8BIT,0x4386,0x00},// ; 
	{UNICAM_8BIT,0x4382,0x00},// ;
	{UNICAM_8BIT,0x4388,0x9F},// ;
	{UNICAM_8BIT,0x4389,0x15},// ;
	{UNICAM_8BIT,0x438A,0x80},// ;
	{UNICAM_8BIT,0x438C,0x0F},// ;
	{UNICAM_8BIT,0x4384,0x14},// ; 
	{UNICAM_8BIT,0x438B,0x00},// ;
	{UNICAM_8BIT,0x4385,0xA5},// ; 
	{UNICAM_8BIT,0x438F,0x00},// ;
	{UNICAM_8BIT,0x438D,0xA0},// ;
	{UNICAM_8BIT,0x4B11,0x1F},// ;
	{UNICAM_8BIT,0x4B44,0x40},// ;
	{UNICAM_8BIT,0x4B46,0x03},// ;
	{UNICAM_8BIT,0x4B47,0xC9},// ;
	{UNICAM_8BIT,0x44B0,0x03},// ;
	{UNICAM_8BIT,0x44B1,0x01},// ;
	{UNICAM_8BIT,0x44B2,0x00},// ;
	{UNICAM_8BIT,0x44B3,0x04},// ;
	{UNICAM_8BIT,0x44B4,0x14},// ;
	{UNICAM_8BIT,0x44B5,0x24},// ;
	{UNICAM_8BIT,0x44B8,0x03},// ;
	{UNICAM_8BIT,0x44B9,0x01},// ;
	{UNICAM_8BIT,0x44BA,0x05},// ;
	{UNICAM_8BIT,0x44BB,0x15},// ;
	{UNICAM_8BIT,0x44BC,0x25},// ;
	{UNICAM_8BIT,0x44BD,0x35},// ;
	{UNICAM_8BIT,0x44D0,0xC0},// ;
	{UNICAM_8BIT,0x44D1,0x80},// ;
	{UNICAM_8BIT,0x44D2,0x40},// ;
	{UNICAM_8BIT,0x44D3,0x40},// ;
	{UNICAM_8BIT,0x44D4,0x40},// ;
	{UNICAM_8BIT,0x44D5,0x40},// ;
	{UNICAM_8BIT,0x4B07,0xF0},// ; by willie_cheng extend mark1 ~ 1200us
	{UNICAM_8BIT,0x4131,0x01},// ;	//BLI
	{UNICAM_8BIT,0x060B,0x01},// ;	//test pattern PN9 seed
	{UNICAM_8BIT,0x4274,0x33},// ; [5] mask out bad frame due to mode (flip/mirror) change
	{UNICAM_8BIT,0x400D,0x04},// ; [2] binning mode analog gain table -> HII binning gain table
	{UNICAM_8BIT,0x3110,0x01},// ;
	{UNICAM_8BIT,0x3111,0x01},// ;
	{UNICAM_8BIT,0x3130,0x01},// ;
	{UNICAM_8BIT,0x3131,0x26},// ;
	{UNICAM_8BIT,0x0383,0x03},// ; x odd increment -> was 01
	{UNICAM_8BIT,0x0387,0x03},// ; y odd increment -> was 01
	{UNICAM_8BIT,0x0390,0x11},// ; binning mode -> was 00	1:binning 2:summing
	{UNICAM_8BIT,0x0348,0x0C},// ; Image size X end Hb
	{UNICAM_8BIT,0x0349,0xCD},// ; Image size X end Lb
	{UNICAM_8BIT,0x034A,0x09},// ; Image size Y end Hb
	{UNICAM_8BIT,0x034B,0x9D},// ; Image size Y end Lb
	{UNICAM_8BIT,0x0340,0x05},// ; frame length lines Hb
	{UNICAM_8BIT,0x0341,0x1A},// ; frame length lines Lb
	{UNICAM_8BIT,0x4B31,0x06},// ;[2] PKTCLK_PHASE_SEL
	{UNICAM_8BIT,0x034C,0x05},// ; HSIZE (1296)
	{UNICAM_8BIT,0x034D,0x10},// ;
	{UNICAM_8BIT,0x034E,0x03},// ; VSIZE (976)
	{UNICAM_8BIT,0x034F,0xD0},// ;
	{UNICAM_8BIT,0x4B18,0x06},// ; [7:0] FULL_TRIGGER_TIME 
	{UNICAM_8BIT,0x0111,0x01},// ; B5: 1'b0:2lane, 1'b1:4lane (org 4B19)
	{UNICAM_8BIT,0x4B20,0xDE},// ; clock always on(9E)
	{UNICAM_8BIT,0x4B02,0x02},// ; lpx_width
	{UNICAM_8BIT,0x4B03,0x05},// ; hs_zero_width
	{UNICAM_8BIT,0x4B04,0x02},// ; hs_trail_width
	{UNICAM_8BIT,0x4B05,0x0C},// ; clk_zero_width
	{UNICAM_8BIT,0x4B06,0x03},// ; clk_trail_width
	{UNICAM_8BIT,0x4B0f,0x07},// ; clk_back_porch
	{UNICAM_8BIT,0x4B39,0x02},// ; clk_width_exit
	{UNICAM_8BIT,0x4B3f,0x08},// ; [3:0] mipi_req_dly
	{UNICAM_8BIT,0x4B42,0x02},// ; HS_PREPARE_WIDTH
	{UNICAM_8BIT,0x4B43,0x02},// ; CLK_PREPARE_WIDTH
	{UNICAM_8BIT,0x4B0E,0x01},// ;Tclk_pre > 0, if H_SIZE/16!=0 
	{UNICAM_8BIT,0x4024,0x40},// ; enabled MIPI -> was 0024
	{UNICAM_8BIT,0x4800,0x00},// ; was CMU 0000
	{UNICAM_8BIT,0x0104,0x01},// ; was 0100
	{UNICAM_8BIT,0x0104,0x00},// ; was 0100, need an high to low edge now! (CMU_AE)
	{UNICAM_8BIT,0x4801,0x00},// ; was 0106
	{UNICAM_8BIT,0x0000,0x00},// ; was 0000
	{UNICAM_8BIT,0xBA94,0x01},//;
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0xBA94,0x00},//;
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0xBA93,0x02},//; 
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x0100,0x01},// ; was 0005 ; mode_select
		{UNICAM_TOK_TERM, 0, 0}

};

S_UNI_REG hm8131_1640_1024[] = {
	{UNICAM_8BIT, 0x0100, 0x00},
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay		
	{UNICAM_8BIT,0x0103,0x00},// ; software reset-> was 0x22
	{UNICAM_8BIT, 0x0100, 0x02},
	{UNICAM_8BIT,0x3519,0x00},//;// PWE interval high byte
	{UNICAM_8BIT,0x351A,0x05},//;// PWE interval low byte
	{UNICAM_8BIT,0x351B,0x1E},//;// PWE interval high byte
	{UNICAM_8BIT,0x351C,0x90},//;// PWE interval low byte
	{UNICAM_8BIT,0x351E,0x15},//;// PRD high pulse byte
	{UNICAM_8BIT,0x351D,0x15},// ;// PRD interval low byte
	{UNICAM_8BIT,0x4001,0x80},//;
	{UNICAM_8BIT,0xBA93,0x01},//; Read Header
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x412A,0x8A},//;
	{UNICAM_8BIT,0xBAA2,0xC0},//;
	{UNICAM_8BIT,0xBAA2,0xC0},//;
	{UNICAM_8BIT,0xBAA2,0xC0},//;
	{UNICAM_8BIT,0xBAA2,0x40},//;
	{UNICAM_8BIT,0x412A,0x8A},//;
	{UNICAM_8BIT,0x412A,0x8A},//;
	{UNICAM_8BIT,0x412A,0x0A},//;
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0xBA93,0x03},//; Read Header
	{UNICAM_8BIT,0xBA93,0x02},//; Read Header
	{UNICAM_8BIT,0xBA90,0x01},//; BPS_OTP_EN[0]
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x0100,0x02},//; power up
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x4001,0x00},// ; ck_cfg - choose mclk_pll -> was 0025
	{UNICAM_8BIT,0x0309,0x02},// ; PLL mipi1 - enabled PLL -> was 002D
	{UNICAM_8BIT,0x0305,0x0C},// ; PLL N, mclk 24mhz
	{UNICAM_8BIT,0x0307,0x46},// ; PLL M, pclk_raw=71/2=35mhz
	{UNICAM_8BIT,0x030D,0x0C},// ; PLL N, 
	{UNICAM_8BIT,0x030F,0x62},// ; PLL M, pkt_clk=98/2=49mhz
	{UNICAM_8BIT,0x414A,0x02},// ; BLC IIR setting
	{UNICAM_8BIT,0x4147,0x03},// ;
	{UNICAM_8BIT,0x4144,0x03},// ; 
	{UNICAM_8BIT,0x4145,0x31},// ; By Willie enalbe CFPN
	{UNICAM_8BIT,0x4146,0x51},// ; 
	{UNICAM_8BIT,0x4149,0x57},// ; By Ken Wu to improve the beating (BLC dither and hysterisis enable)
	{UNICAM_8BIT,0x4260,0x00},// ; By Simon
	{UNICAM_8BIT,0x4261,0x00},// ; By Simon
	{UNICAM_8BIT,0x4264,0x14},// ; By Simon
	{UNICAM_8BIT,0x426A,0x01},// ;
	{UNICAM_8BIT,0x4270,0x08},// ; By Willie 
	{UNICAM_8BIT,0x4271,0x8A},// ; By Willie
	{UNICAM_8BIT,0x4272,0x22},// ;
	{UNICAM_8BIT,0x427D,0x00},// ; By Simon
	{UNICAM_8BIT,0x427E,0x03},// ; By Simon
	{UNICAM_8BIT,0x427F,0x00},// ;
	{UNICAM_8BIT,0x4380,0xA6},// ; 
	{UNICAM_8BIT,0x4381,0x7B},// ; By Willie to save ramp power
	{UNICAM_8BIT,0x4383,0x58},// ; By Willie (get brighter image at low light), increase VRNP voltage.
	{UNICAM_8BIT,0x4387,0x37},// ; Adjust clamping level
	{UNICAM_8BIT,0x4386,0x00},// ; 
	{UNICAM_8BIT,0x4382,0x00},// ;
	{UNICAM_8BIT,0x4388,0x9F},// ;
	{UNICAM_8BIT,0x4389,0x15},// ;
	{UNICAM_8BIT,0x438A,0x80},// ;
	{UNICAM_8BIT,0x438C,0x0F},// ;
	{UNICAM_8BIT,0x4384,0x14},// ; 
	{UNICAM_8BIT,0x438B,0x00},// ;
	{UNICAM_8BIT,0x4385,0xA5},// ; 
	{UNICAM_8BIT,0x438F,0x00},// ;
	{UNICAM_8BIT,0x438D,0xA0},// ;
	{UNICAM_8BIT,0x4B11,0x1F},// ;
	{UNICAM_8BIT,0x4B44,0x40},// ;
	{UNICAM_8BIT,0x4B46,0x03},// ;
	{UNICAM_8BIT,0x4B47,0xC9},// ;
	{UNICAM_8BIT,0x44B0,0x03},// ;
	{UNICAM_8BIT,0x44B1,0x01},// ;
	{UNICAM_8BIT,0x44B2,0x00},// ;
	{UNICAM_8BIT,0x44B3,0x04},// ;
	{UNICAM_8BIT,0x44B4,0x14},// ;
	{UNICAM_8BIT,0x44B5,0x24},// ;
	{UNICAM_8BIT,0x44B8,0x03},// ;
	{UNICAM_8BIT,0x44B9,0x01},// ;
	{UNICAM_8BIT,0x44BA,0x05},// ;
	{UNICAM_8BIT,0x44BB,0x15},// ;
	{UNICAM_8BIT,0x44BC,0x25},// ;
	{UNICAM_8BIT,0x44BD,0x35},// ;
	{UNICAM_8BIT,0x44D0,0xC0},// ;
	{UNICAM_8BIT,0x44D1,0x80},// ;
	{UNICAM_8BIT,0x44D2,0x40},// ;
	{UNICAM_8BIT,0x44D3,0x40},// ;
	{UNICAM_8BIT,0x44D4,0x40},// ;
	{UNICAM_8BIT,0x44D5,0x40},// ;
	{UNICAM_8BIT,0x4B07,0xF0},// ; by willie_cheng extend mark1 ~ 1200us
	{UNICAM_8BIT,0x4131,0x01},// ;	//BLI
	{UNICAM_8BIT,0x060B,0x01},// ;	//test pattern PN9 seed
	{UNICAM_8BIT,0x4274,0x33},// ; [5] mask out bad frame due to mode (flip/mirror) change
	{UNICAM_8BIT,0x400D,0x04},// ; [2] binning mode analog gain table -> HII binning gain table
	{UNICAM_8BIT,0x3110,0x01},// ;
	{UNICAM_8BIT,0x3111,0x01},// ;
	{UNICAM_8BIT,0x3130,0x01},// ;
	{UNICAM_8BIT,0x3131,0x26},// ;
	{UNICAM_8BIT,0x0383,0x03},// ; x odd increment -> was 01
	{UNICAM_8BIT,0x0387,0x03},// ; y odd increment -> was 01
	{UNICAM_8BIT,0x0390,0x11},// ; binning mode -> was 00	1:binning 2:summing
	{UNICAM_8BIT,0x0348,0x0C},// ; Image size X end Hb
	{UNICAM_8BIT,0x0349,0xCD},// ; Image size X end Lb
	{UNICAM_8BIT,0x034A,0x09},// ; Image size Y end Hb
	{UNICAM_8BIT,0x034B,0x9D},// ; Image size Y end Lb
	{UNICAM_8BIT,0x0340,0x05},// ; frame length lines Hb
	{UNICAM_8BIT,0x0341,0x1A},// ; frame length lines Lb
	{UNICAM_8BIT,0x4B31,0x06},// ;[2] PKTCLK_PHASE_SEL
	{UNICAM_8BIT,0x034C,0x06},// ; HSIZE (1640)
	{UNICAM_8BIT,0x034D,0x68},// ; HSIZE (1640)
	{UNICAM_8BIT,0x034E,0x04},// ; VSIZE (1024)
	{UNICAM_8BIT,0x034F,0x00},// ; VSIZE (1024)
	{UNICAM_8BIT,0x4B18,0x06},// ; [7:0] FULL_TRIGGER_TIME 
	{UNICAM_8BIT,0x0111,0x01},// ; B5: 1'b0:2lane, 1'b1:4lane (org 4B19)
	{UNICAM_8BIT,0x4B20,0xDE},// ; clock always on(9E)
	{UNICAM_8BIT,0x4B02,0x02},// ; lpx_width
	{UNICAM_8BIT,0x4B03,0x05},// ; hs_zero_width
	{UNICAM_8BIT,0x4B04,0x02},// ; hs_trail_width
	{UNICAM_8BIT,0x4B05,0x0C},// ; clk_zero_width
	{UNICAM_8BIT,0x4B06,0x03},// ; clk_trail_width
	{UNICAM_8BIT,0x4B0f,0x07},// ; clk_back_porch
	{UNICAM_8BIT,0x4B39,0x02},// ; clk_width_exit
	{UNICAM_8BIT,0x4B3f,0x08},// ; [3:0] mipi_req_dly
	{UNICAM_8BIT,0x4B42,0x02},// ; HS_PREPARE_WIDTH
	{UNICAM_8BIT,0x4B43,0x02},// ; CLK_PREPARE_WIDTH
	{UNICAM_8BIT,0x4B0E,0x01},// ;Tclk_pre > 0, if H_SIZE/16!=0 
	{UNICAM_8BIT,0x4024,0x40},// ; enabled MIPI -> was 0024
	{UNICAM_8BIT,0x4800,0x00},// ; was CMU 0000
	{UNICAM_8BIT,0x0104,0x01},// ; was 0100
	{UNICAM_8BIT,0x0104,0x00},// ; was 0100, need an high to low edge now! (CMU_AE)
	{UNICAM_8BIT,0x4801,0x00},// ; was 0106
	{UNICAM_8BIT,0x0000,0x00},// ; was 0000
	{UNICAM_8BIT,0xBA94,0x01},//;
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0xBA94,0x00},//;
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0xBA93,0x02},//; 
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay
	{UNICAM_8BIT,0x0100,0x01},// ; was 0005 ; mode_select

	{UNICAM_TOK_TERM, 0, 0}
};


S_UNI_REG hm8131_1940_1096[] = {
	{UNICAM_8BIT, 0x0100, 0x00},
	{UNICAM_TOK_DELAY, 0x0000, 0x48}, //delay		
	{UNICAM_8BIT,0x0103,0x00},// ; software reset-> was 0x22
	{UNICAM_8BIT, 0x0100, 0x02},
//	{UNICAM_TOK_DELAY,0,0x45},// 68ms
	{UNICAM_8BIT,0x3519,0x00},
	{UNICAM_8BIT,0x351A,0x05},
	{UNICAM_8BIT,0x351B,0x1E},
	{UNICAM_8BIT,0x351C,0x90},
	{UNICAM_8BIT,0x351E,0x15},
	{UNICAM_8BIT,0x351D,0x15},
	{UNICAM_8BIT,0x4001,0x80},
	{UNICAM_8BIT,0xBA93,0x01},
	{UNICAM_8BIT,0x412A,0x8A},
	{UNICAM_8BIT,0xBAA2,0xC0},
	{UNICAM_8BIT,0xBAA2,0xC0},
	{UNICAM_8BIT,0xBAA2,0xC0},
	{UNICAM_8BIT,0xBAA2,0x40},
	{UNICAM_8BIT,0x412A,0x8A},
	{UNICAM_8BIT,0x412A,0x8A},
	{UNICAM_8BIT,0x412A,0x0A},
	{UNICAM_8BIT,0xBA93,0x03},
	{UNICAM_8BIT,0xBA93,0x02},
	{UNICAM_8BIT,0xBA90,0x01},
	{UNICAM_8BIT,0x0100,0x02},
	{UNICAM_8BIT,0x4001,0x00},
	{UNICAM_8BIT,0x0305,0x0C},
	{UNICAM_8BIT,0x0307,0x36},
	{UNICAM_8BIT,0x030D,0x0C},
	{UNICAM_8BIT,0x030F,0x4B},
	{UNICAM_8BIT,0x414A,0x02},
	{UNICAM_8BIT,0x4147,0x03},
	{UNICAM_8BIT,0x4144,0x03},
	{UNICAM_8BIT,0x4145,0x31},
	{UNICAM_8BIT,0x4146,0x51},
	{UNICAM_8BIT,0x4149,0x57},
	{UNICAM_8BIT,0x4260,0x00},
	{UNICAM_8BIT,0x4261,0x00},
	{UNICAM_8BIT,0x4264,0x14},
	{UNICAM_8BIT,0x426A,0x01},
	{UNICAM_8BIT,0x4270,0x08},
	{UNICAM_8BIT,0x4271,0x8A},
	{UNICAM_8BIT,0x4272,0x22},
	{UNICAM_8BIT,0x427D,0x00},
	{UNICAM_8BIT,0x427E,0x03},
	{UNICAM_8BIT,0x427F,0x00},
	{UNICAM_8BIT,0x4380,0xA6},
	{UNICAM_8BIT,0x4381,0x7B},
	{UNICAM_8BIT,0x4383,0x58},
	{UNICAM_8BIT,0x4387,0x37},
	{UNICAM_8BIT,0x4386,0x00},
	{UNICAM_8BIT,0x4382,0x00},
	{UNICAM_8BIT,0x4388,0x9F},
	{UNICAM_8BIT,0x4389,0x15},
	{UNICAM_8BIT,0x438A,0x80},
	{UNICAM_8BIT,0x438C,0x0F},
	{UNICAM_8BIT,0x4384,0x14},
	{UNICAM_8BIT,0x438B,0x00},
	{UNICAM_8BIT,0x4385,0xA5},
	{UNICAM_8BIT,0x438F,0x00},
	{UNICAM_8BIT,0x438D,0xA0},
	{UNICAM_8BIT,0x4B11,0x1F},
	{UNICAM_8BIT,0x4B44,0x40},
	{UNICAM_8BIT,0x4B46,0x03},
	{UNICAM_8BIT,0x4B47,0xC9},
	{UNICAM_8BIT,0x44B0,0x03},
	{UNICAM_8BIT,0x44B1,0x01},
	{UNICAM_8BIT,0x44B2,0x00},
	{UNICAM_8BIT,0x44B3,0x04},
	{UNICAM_8BIT,0x44B4,0x14},
	{UNICAM_8BIT,0x44B5,0x24},
	{UNICAM_8BIT,0x44B8,0x03},
	{UNICAM_8BIT,0x44B9,0x01},
	{UNICAM_8BIT,0x44BA,0x05},
	{UNICAM_8BIT,0x44BB,0x15},
	{UNICAM_8BIT,0x44BC,0x25},
	{UNICAM_8BIT,0x44BD,0x35},
	{UNICAM_8BIT,0x44D0,0xC0},
	{UNICAM_8BIT,0x44D1,0x80},
	{UNICAM_8BIT,0x44D2,0x40},
	{UNICAM_8BIT,0x44D3,0x40},
	{UNICAM_8BIT,0x44D4,0x40},
	{UNICAM_8BIT,0x44D5,0x40},
	{UNICAM_8BIT,0x4B07,0xF0},
	{UNICAM_8BIT,0x4131,0x01},
	{UNICAM_8BIT,0x060B,0x01},
	{UNICAM_8BIT,0x4274,0x33},
	{UNICAM_8BIT,0x3110,0x23},
	{UNICAM_8BIT,0x3111,0x00},
	{UNICAM_8BIT,0x3130,0x01},
	{UNICAM_8BIT,0x3131,0x80},
	{UNICAM_8BIT,0x4B31,0x06},
	{UNICAM_8BIT,0x4002,0x23},
	{UNICAM_8BIT,0x034C,0x0C},
	{UNICAM_8BIT,0x034D,0xD0},
	{UNICAM_8BIT,0x034E,0x09},
	{UNICAM_8BIT,0x034F,0xA0},
	{UNICAM_8BIT,0x4B18,0x17},
	{UNICAM_8BIT,0x0111,0x01},
	{UNICAM_8BIT,0x4B20,0xDE},
	{UNICAM_8BIT,0x4B0E,0x03},
	{UNICAM_8BIT,0x4B42,0x05},
	{UNICAM_8BIT,0x4B04,0x05},
	{UNICAM_8BIT,0x4B06,0x06},
	{UNICAM_8BIT,0x4B3F,0x12},
	{UNICAM_8BIT,0x4024,0x40},
	{UNICAM_8BIT,0x4800,0x00},
	{UNICAM_8BIT,0x0104,0x01},
	{UNICAM_8BIT,0x0104,0x00},
	{UNICAM_8BIT,0x4801,0x00},
	{UNICAM_8BIT,0x0000,0x00},
	{UNICAM_8BIT,0xBA94,0x01},
	{UNICAM_8BIT,0xBA94,0x00},
	{UNICAM_8BIT,0xBA93,0x02},
	{UNICAM_8BIT,0x0601,0x02}, //Test patten
	{UNICAM_8BIT,0x0100,0x00},
	{UNICAM_8BIT,0x0104,0x01},
	{UNICAM_8BIT,0x4B08,0x48},// ;VSIZE (2464)
	{UNICAM_8BIT,0x4B09,0x04},// ;
	{UNICAM_8BIT,0x4B0A,0x94},// ;HSIZE (3280)
	{UNICAM_8BIT,0x4B0B,0x07},// ;
	{UNICAM_8BIT,0x0348,0x07},// ;X-END (3279)
	{UNICAM_8BIT,0x0349,0x93},// ;
	{UNICAM_8BIT,0x034A,0x04},// ;Y-END (2463)
	{UNICAM_8BIT,0x034B,0x47},// ;
	{UNICAM_8BIT,0x034C,0x07},// ;HSIZE (3280)
	{UNICAM_8BIT,0x034D,0x94},// ;
	{UNICAM_8BIT,0x034E,0x04},// ;VSIZE (2464)
	{UNICAM_8BIT,0x034F,0x48},// ;
	{UNICAM_8BIT,0x0104,0x00},
	{UNICAM_8BIT,0x0100,0x01},
	{UNICAM_TOK_DELAY,0,0x45},// 68ms

	{UNICAM_TOK_TERM, 0, 0}
};

static S_UNI_RESOLUTION hm8131_res_preview[] = {
	/*{
		.desc = "hm8131_368_304",
		.res_type = UNI_DEV_RES_PREVIEW|UNI_DEV_RES_VIDEO|UNI_DEV_RES_STILL,
		.width = 368,
		.height = 304,
		.fps = 30,
		.pixels_per_line = 888,
		.lines_per_frame = 1286,
		.bin_factor_x = 2,
		.bin_factor_y = 2,
		.bin_mode = 1,
		.skip_frames = 1,
		.regs = hm8131_368_304,
		.regs_n= (ARRAY_SIZE(hm8131_368_304)),

		.vt_pix_clk_freq_mhz=56660000,
		.crop_horizontal_start=0,
		.crop_vertical_start=0,
		.crop_horizontal_end=367,
		.crop_vertical_end=303,
		.output_width=368,
		.output_height=304,
		.vts_fix=8,
		.vts=1286,
	},{
		.desc = "hm8131_900P_30fps",
		.res_type = UNI_DEV_RES_PREVIEW|UNI_DEV_RES_VIDEO|UNI_DEV_RES_STILL,
		.width = 1640,
		.height = 932,
		.fps = 48,
		.pixels_per_line = 888,
		.lines_per_frame = 1922,
		.bin_factor_x = 2,
		.bin_factor_y = 2,
		.bin_mode = 1,
		.skip_frames = 1,
		.regs = hm8131_1616_916,
		.regs_n= (ARRAY_SIZE(hm8131_1616_916)),

		.vt_pix_clk_freq_mhz=56660000,
		.crop_horizontal_start=0,
		.crop_vertical_start=0,
		.crop_horizontal_end=1639,
		.crop_vertical_end=931,
		.output_width=1640,
		.output_height=932,
		.vts_fix=8,
		.vts=1922,
	},{
		.desc = "hm8131_1296_736",
		.res_type = UNI_DEV_RES_PREVIEW|UNI_DEV_RES_VIDEO|UNI_DEV_RES_STILL,
		.width = 1296,
		.height = 736,
		.fps = 30,
		.pixels_per_line = 888,
		.lines_per_frame = 1286,
		.bin_factor_x = 2,
		.bin_factor_y = 2,
		.bin_mode = 1,
		.skip_frames = 1,
		.regs = hm8131_1296_736,
		.regs_n= (ARRAY_SIZE(hm8131_1296_736)),

		.vt_pix_clk_freq_mhz=56660000,
		.crop_horizontal_start=0,
		.crop_vertical_start=0,
		.crop_horizontal_end=1295,
		.crop_vertical_end=735,
		.output_width=1296,
		.output_height=736,
		.vts_fix=8,
		.vts=1286,
	},{
		.desc = "hm8131_1940_1096",
		.res_type = UNI_DEV_RES_PREVIEW|UNI_DEV_RES_VIDEO|UNI_DEV_RES_STILL,
		.width = 1940,
		.height = 1096,
		.fps = 30,
		.pixels_per_line = 888,
		.lines_per_frame = 2534,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 1,
		.regs = hm8131_1940_1096,
		.regs_n= (ARRAY_SIZE(hm8131_1940_1096)),

		.vt_pix_clk_freq_mhz=56660000,
		.crop_horizontal_start=0,
		.crop_vertical_start=0,
		.crop_horizontal_end=1939,
		.crop_vertical_end=1095,
		.output_width=1940,
		.output_height=1096,
		.vts_fix=8,
		.vts=2534,
	},{
		.desc = "hm8131_2M_30fps",
		.res_type = UNI_DEV_RES_PREVIEW|UNI_DEV_RES_VIDEO|UNI_DEV_RES_STILL,
		.width = 1640,
		.height = 1232,
		.fps = 30,
		.pixels_per_line = 888,
		.lines_per_frame = 1286,
		.bin_factor_x = 2,
		.bin_factor_y = 2,
		.bin_mode = 1,
		.skip_frames = 1,
		.regs = hm8131_1640_1232,
		.regs_n= (ARRAY_SIZE(hm8131_1640_1232)),

		.vt_pix_clk_freq_mhz=56660000,
		.crop_horizontal_start=0,
		.crop_vertical_start=0,
		.crop_horizontal_end=1639,
		.crop_vertical_end=1231,
		.output_width=1640,
		.output_height=1232,
		.vts_fix=8,
		.vts=1286,
	},{
		.desc = "hm8131_6M_30fps",
		.res_type = UNI_DEV_RES_PREVIEW|UNI_DEV_RES_VIDEO|UNI_DEV_RES_STILL,
		.width = 3280,
		.height = 1852,
		.fps = 30,
		.pixels_per_line = 888,
		.lines_per_frame = 1922,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 1,
		.regs = hm8131_regress1_6M,
		.regs_n= (ARRAY_SIZE(hm8131_regress1_6M)),

		.vt_pix_clk_freq_mhz=68000000,
		.crop_horizontal_start=0,
		.crop_vertical_start=0,
		.crop_horizontal_end=3279,
		.crop_vertical_end=1851,
		.output_width=3280,
		.output_height=1852,
		.vts_fix=8,
		.vts=1922,
	},*/
	/*{
		.desc = "hm8131_6M_30fps",
		.res_type = UNI_DEV_RES_PREVIEW|UNI_DEV_RES_VIDEO|UNI_DEV_RES_STILL,
		.width = 3280,
		.height = 1852,
		.fps = 30,
		.pixels_per_line = 888,
		.lines_per_frame = 1922,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 1,
		.regs = hm8131_regress1_6M,
		.regs_n= (ARRAY_SIZE(hm8131_regress1_6M)),

		.vt_pix_clk_freq_mhz=43200000,//68000000
		.crop_horizontal_start=0,
		.crop_vertical_start=0,
		.crop_horizontal_end=3279,
		.crop_vertical_end=1851,
		.output_width=3280,
		.output_height=1852,
		.used = 0,

		.vts_fix=8,
		.vts=1922,
	},*/{
		.desc = "hm8131_8M_30fps",
		.res_type = UNI_DEV_RES_PREVIEW|UNI_DEV_RES_VIDEO|UNI_DEV_RES_STILL,
		.width = 3280,
		.height = 2464,
		.fps = 20,
		.pixels_per_line = 888,
		.lines_per_frame = 2534,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 3,
		.regs = hm8131_regress2_8M,
		.regs_n= (ARRAY_SIZE(hm8131_regress2_8M)),

		.vt_pix_clk_freq_mhz=43200000,//68000000
		.crop_horizontal_start=0,
		.crop_vertical_start=0,
		.crop_horizontal_end=3279,
		.crop_vertical_end=2463,
		.output_width=3280,
		.output_height=2464,
		.used = 0,

		.vts_fix=8,
		.vts=2534,
	},
};
static S_UNI_DEVICE hm8131_unidev={
	.version=0x1,
	.minversion=125,//update if change, as buildid
	.af_type=UNI_DEV_AF_DW9714,
	.i2c_addr=0x34,

	.idreg=0x0000,
	.idval=0x8131,
	.idmask=0xffff,
	.powerdelayms=1,
	.accessdelayms=20,
	.flag=UNI_DEV_FLAG_1B_ACCESS,
	.exposecoarse=0x0202,
	.exposeanaloggain=0x0205,
	.exposedigitgainR=0,
	.exposedigitgainGR=0,
	.exposedigitgainGB=0,
	.exposedigitgainB=0,
	.exposefine=0,
	.exposefine_mask=0,
	.exposevts=0x340,
	.exposevts_mask=0xffff,

	.exposecoarse_mask=0xffff,
	.exposeanaloggain_mask=0xff,
	.exposedigitgain_mask=0,
	.exposeadgain_param=hm8131_adgain_p,
	.exposeadgain_n=(ARRAY_SIZE(hm8131_adgain_p)),
	.exposeadgain_min=0,
	.exposeadgain_max=0,
	.exposeadgain_fra=0,
	.exposeadgain_step=0,
	.exposeadgain_isp1gain=0,
	.exposeadgain_param1gain=0,
 
	.global_setting=hm8131_global_setting,
	.global_setting_n=(ARRAY_SIZE(hm8131_global_setting)),
	.stream_off=hm8131_stream_off,
	.stream_off_n=(ARRAY_SIZE(hm8131_stream_off)),
	.stream_on=hm8131_stream_on,
	.stream_on_n=(ARRAY_SIZE(hm8131_stream_on)),
	.group_hold_start=hm8131_group_hold_start,
	.group_hold_start_n=(ARRAY_SIZE(hm8131_group_hold_start)),
	.group_hold_end=hm8131_group_hold_end,
	.group_hold_end_n=(ARRAY_SIZE(hm8131_group_hold_end)),

	.ress=hm8131_res_preview,
	.ress_n=(ARRAY_SIZE(hm8131_res_preview)),
	.coarse_integration_time_min=1,
	.coarse_integration_time_max_margin=8,
	.fine_integration_time_min=0,
	.fine_integration_time_max_margin=0,
	.hw_port=ATOMISP_CAMERA_PORT_PRIMARY,
	.hw_lane=4,
	.hw_format=ATOMISP_INPUT_FORMAT_RAW_10,
	.hw_bayer_order=atomisp_bayer_order_bggr,
	.hw_sensor_type=RAW_CAMERA,

	.hw_reset_gpio=119,
	.hw_reset_gpio_polarity=0,
	.hw_reset_delayms=20,
	.hw_pwd_gpio=123,
	.hw_pwd_gpio_polarity=0,
	.hw_clksource=0,
};

#undef PPR_DEV
#define PPR_DEV hm8131_unidev
