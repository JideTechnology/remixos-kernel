#define HM5040_PCLK         163200000 //70400000
#define HM5040_PCLK_BINNING 120000000
#define HM5040_BINNING_MODE 1

// auto generate : Thu Aug 14 03:52:28 CST 2014
S_UNI_REG  hm5040_stream_on[] = {
	{UNICAM_8BIT, 0x0100, 0x01},
	//{UNICAM_8BIT, 0x0104, 0x01},
//	{UNICAM_TOK_DELAY, 0, 50}, 	//DELAY=50		 //Initialization Time
	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG  hm5040_stream_off[] = {
	{UNICAM_8BIT, 0x0100, 0x00},
	//{UNICAM_8BIT, 0x0104, 0x01},
//	{UNICAM_TOK_DELAY, 0, 50}, 	//DELAY=50		 //Initialization Time
	{UNICAM_TOK_TERM, 0, 0}
};

S_UNI_REG  hm5040_global_setting[] = {
	{UNICAM_8BIT, 0x0100, 0x00},
	{UNICAM_8BIT, 0x0103, 0x01},      
	//-------------------------------------
	//	mode select
	//-------------------------------------
	{UNICAM_8BIT, 0x0100, 0x00},
	{UNICAM_8BIT, 0x3002, 0x32},
	{UNICAM_8BIT, 0x3016, 0x46},
	{UNICAM_8BIT, 0x3017, 0x29},
	{UNICAM_8BIT, 0x3003, 0x03},
	{UNICAM_8BIT, 0x3045, 0x03},
	{UNICAM_8BIT, 0xFBD7, 0x4C},
	{UNICAM_8BIT, 0xFBD8, 0x89},
	{UNICAM_8BIT, 0xFBD9, 0x40},
	{UNICAM_8BIT, 0xFBDA, 0xE1},
	{UNICAM_8BIT, 0xFBDB, 0x4C},
	{UNICAM_8BIT, 0xFBDC, 0x73},
	{UNICAM_8BIT, 0xFBDD, 0x40},
	{UNICAM_8BIT, 0xFBDE, 0xD9},
	{UNICAM_8BIT, 0xFBDF, 0x4C},
	{UNICAM_8BIT, 0xFBE0, 0x74},
	{UNICAM_8BIT, 0xFBE1, 0x40},
	{UNICAM_8BIT, 0xFBE2, 0xD9},
	{UNICAM_8BIT, 0xFBE3, 0x4C},
	{UNICAM_8BIT, 0xFBE4, 0x87},
	{UNICAM_8BIT, 0xFBE5, 0x40},
	{UNICAM_8BIT, 0xFBE6, 0xD3},
	{UNICAM_8BIT, 0xFBE7, 0x4C},
	{UNICAM_8BIT, 0xFBE8, 0xB1},
	{UNICAM_8BIT, 0xFBE9, 0x40},
	{UNICAM_8BIT, 0xFBEA, 0xC7},
	{UNICAM_8BIT, 0xFBEB, 0x4C},
	{UNICAM_8BIT, 0xFBEC, 0xCC},
	{UNICAM_8BIT, 0xFBED, 0x41},
	{UNICAM_8BIT, 0xFBEE, 0x13},
	{UNICAM_8BIT, 0xFBEF, 0x4C},
	{UNICAM_8BIT, 0xFBF0, 0xB9},
	{UNICAM_8BIT, 0xFBF1, 0x41},
	{UNICAM_8BIT, 0xFBF2, 0x11},
	{UNICAM_8BIT, 0xFBF3, 0x4C},
	{UNICAM_8BIT, 0xFBF4, 0xB9},
	{UNICAM_8BIT, 0xFBF5, 0x41},
	{UNICAM_8BIT, 0xFBF6, 0x12},
	{UNICAM_8BIT, 0xFBF7, 0x4C},
	{UNICAM_8BIT, 0xFBF8, 0xBA},
	{UNICAM_8BIT, 0xFBF9, 0x41},
	{UNICAM_8BIT, 0xFBFA, 0x40},
	{UNICAM_8BIT, 0xFBFB, 0x4C},
	{UNICAM_8BIT, 0xFBFC, 0xD3},
	{UNICAM_8BIT, 0xFBFD, 0x41},
	{UNICAM_8BIT, 0xFBFE, 0x44},
	{UNICAM_8BIT, 0xFB00, 0x51},
	{UNICAM_8BIT, 0xF800, 0xC0},
	{UNICAM_8BIT, 0xF801, 0x24},
	{UNICAM_8BIT, 0xF802, 0x7C},
	{UNICAM_8BIT, 0xF803, 0xFB},
	{UNICAM_8BIT, 0xF804, 0x7D},
	{UNICAM_8BIT, 0xF805, 0xC7},
	{UNICAM_8BIT, 0xF806, 0x7B},
	{UNICAM_8BIT, 0xF807, 0x10},
	{UNICAM_8BIT, 0xF808, 0x7F},
	{UNICAM_8BIT, 0xF809, 0x72},
	{UNICAM_8BIT, 0xF80A, 0x7E},
	{UNICAM_8BIT, 0xF80B, 0x30},
	{UNICAM_8BIT, 0xF80C, 0x12},
	{UNICAM_8BIT, 0xF80D, 0x09},
	{UNICAM_8BIT, 0xF80E, 0x47},
	{UNICAM_8BIT, 0xF80F, 0xD0},
	{UNICAM_8BIT, 0xF810, 0x24},
	{UNICAM_8BIT, 0xF811, 0x90},
	{UNICAM_8BIT, 0xF812, 0x02},
	{UNICAM_8BIT, 0xF813, 0x05},
	{UNICAM_8BIT, 0xF814, 0xE0},
	{UNICAM_8BIT, 0xF815, 0xF5},
	{UNICAM_8BIT, 0xF816, 0x77},
	{UNICAM_8BIT, 0xF817, 0xE5},
	{UNICAM_8BIT, 0xF818, 0x77},
	{UNICAM_8BIT, 0xF819, 0xC3},
	{UNICAM_8BIT, 0xF81A, 0x94},
	{UNICAM_8BIT, 0xF81B, 0x80},
	{UNICAM_8BIT, 0xF81C, 0x50},
	{UNICAM_8BIT, 0xF81D, 0x08},
	{UNICAM_8BIT, 0xF81E, 0x75},
	{UNICAM_8BIT, 0xF81F, 0x7A},
	{UNICAM_8BIT, 0xF820, 0xFB},
	{UNICAM_8BIT, 0xF821, 0x75},
	{UNICAM_8BIT, 0xF822, 0x7B},
	{UNICAM_8BIT, 0xF823, 0xD7},
	{UNICAM_8BIT, 0xF824, 0x80},
	{UNICAM_8BIT, 0xF825, 0x33},
	{UNICAM_8BIT, 0xF826, 0xE5},
	{UNICAM_8BIT, 0xF827, 0x77},
	{UNICAM_8BIT, 0xF828, 0xC3},
	{UNICAM_8BIT, 0xF829, 0x94},
	{UNICAM_8BIT, 0xF82A, 0xC0},
	{UNICAM_8BIT, 0xF82B, 0x50},
	{UNICAM_8BIT, 0xF82C, 0x08},
	{UNICAM_8BIT, 0xF82D, 0x75},
	{UNICAM_8BIT, 0xF82E, 0x7A},
	{UNICAM_8BIT, 0xF82F, 0xFB},
	{UNICAM_8BIT, 0xF830, 0x75},
	{UNICAM_8BIT, 0xF831, 0x7B},
	{UNICAM_8BIT, 0xF832, 0xDB},
	{UNICAM_8BIT, 0xF833, 0x80},
	{UNICAM_8BIT, 0xF834, 0x24},
	{UNICAM_8BIT, 0xF835, 0xE5},
	{UNICAM_8BIT, 0xF836, 0x77},
	{UNICAM_8BIT, 0xF837, 0xC3},
	{UNICAM_8BIT, 0xF838, 0x94},
	{UNICAM_8BIT, 0xF839, 0xE0},
	{UNICAM_8BIT, 0xF83A, 0x50},
	{UNICAM_8BIT, 0xF83B, 0x08},
	{UNICAM_8BIT, 0xF83C, 0x75},
	{UNICAM_8BIT, 0xF83D, 0x7A},
	{UNICAM_8BIT, 0xF83E, 0xFB},
	{UNICAM_8BIT, 0xF83F, 0x75},
	{UNICAM_8BIT, 0xF840, 0x7B},
	{UNICAM_8BIT, 0xF841, 0xDF},
	{UNICAM_8BIT, 0xF842, 0x80},
	{UNICAM_8BIT, 0xF843, 0x15},
	{UNICAM_8BIT, 0xF844, 0xE5},
	{UNICAM_8BIT, 0xF845, 0x77},
	{UNICAM_8BIT, 0xF846, 0xC3},
	{UNICAM_8BIT, 0xF847, 0x94},
	{UNICAM_8BIT, 0xF848, 0xF0},
	{UNICAM_8BIT, 0xF849, 0x50},
	{UNICAM_8BIT, 0xF84A, 0x08},
	{UNICAM_8BIT, 0xF84B, 0x75},
	{UNICAM_8BIT, 0xF84C, 0x7A},
	{UNICAM_8BIT, 0xF84D, 0xFB},
	{UNICAM_8BIT, 0xF84E, 0x75},
	{UNICAM_8BIT, 0xF84F, 0x7B},
	{UNICAM_8BIT, 0xF850, 0xE3},
	{UNICAM_8BIT, 0xF851, 0x80},
	{UNICAM_8BIT, 0xF852, 0x06},
	{UNICAM_8BIT, 0xF853, 0x75},
	{UNICAM_8BIT, 0xF854, 0x7A},
	{UNICAM_8BIT, 0xF855, 0xFB},
	{UNICAM_8BIT, 0xF856, 0x75},
	{UNICAM_8BIT, 0xF857, 0x7B},
	{UNICAM_8BIT, 0xF858, 0xE7},
	{UNICAM_8BIT, 0xF859, 0xE5},
	{UNICAM_8BIT, 0xF85A, 0x55},
	{UNICAM_8BIT, 0xF85B, 0x7F},
	{UNICAM_8BIT, 0xF85C, 0x00},
	{UNICAM_8BIT, 0xF85D, 0xB4},
	{UNICAM_8BIT, 0xF85E, 0x22},
	{UNICAM_8BIT, 0xF85F, 0x02},
	{UNICAM_8BIT, 0xF860, 0x7F},
	{UNICAM_8BIT, 0xF861, 0x01},
	{UNICAM_8BIT, 0xF862, 0xE5},
	{UNICAM_8BIT, 0xF863, 0x53},
	{UNICAM_8BIT, 0xF864, 0x5F},
	{UNICAM_8BIT, 0xF865, 0x60},
	{UNICAM_8BIT, 0xF866, 0x05},
	{UNICAM_8BIT, 0xF867, 0x74},
	{UNICAM_8BIT, 0xF868, 0x14},
	{UNICAM_8BIT, 0xF869, 0x12},
	{UNICAM_8BIT, 0xF86A, 0xFA},
	{UNICAM_8BIT, 0xF86B, 0x4C},
	{UNICAM_8BIT, 0xF86C, 0x75},
	{UNICAM_8BIT, 0xF86D, 0x7C},
	{UNICAM_8BIT, 0xF86E, 0xFB},
	{UNICAM_8BIT, 0xF86F, 0x75},
	{UNICAM_8BIT, 0xF870, 0x7D},
	{UNICAM_8BIT, 0xF871, 0xC7},
	{UNICAM_8BIT, 0xF872, 0x75},
	{UNICAM_8BIT, 0xF873, 0x7E},
	{UNICAM_8BIT, 0xF874, 0x30},
	{UNICAM_8BIT, 0xF875, 0x75},
	{UNICAM_8BIT, 0xF876, 0x7F},
	{UNICAM_8BIT, 0xF877, 0x62},
	{UNICAM_8BIT, 0xF878, 0xE4},
	{UNICAM_8BIT, 0xF879, 0xF5},
	{UNICAM_8BIT, 0xF87A, 0x77},
	{UNICAM_8BIT, 0xF87B, 0xE5},
	{UNICAM_8BIT, 0xF87C, 0x77},
	{UNICAM_8BIT, 0xF87D, 0xC3},
	{UNICAM_8BIT, 0xF87E, 0x94},
	{UNICAM_8BIT, 0xF87F, 0x08},
	{UNICAM_8BIT, 0xF880, 0x40},
	{UNICAM_8BIT, 0xF881, 0x03},
	{UNICAM_8BIT, 0xF882, 0x02},
	{UNICAM_8BIT, 0xF883, 0xF9},
	{UNICAM_8BIT, 0xF884, 0x0E},
	{UNICAM_8BIT, 0xF885, 0x85},
	{UNICAM_8BIT, 0xF886, 0x7D},
	{UNICAM_8BIT, 0xF887, 0x82},
	{UNICAM_8BIT, 0xF888, 0x85},
	{UNICAM_8BIT, 0xF889, 0x7C},
	{UNICAM_8BIT, 0xF88A, 0x83},
	{UNICAM_8BIT, 0xF88B, 0xE0},
	{UNICAM_8BIT, 0xF88C, 0xFE},
	{UNICAM_8BIT, 0xF88D, 0xA3},
	{UNICAM_8BIT, 0xF88E, 0xE0},
	{UNICAM_8BIT, 0xF88F, 0xFF},
	{UNICAM_8BIT, 0xF890, 0x12},
	{UNICAM_8BIT, 0xF891, 0x21},
	{UNICAM_8BIT, 0xF892, 0x22},
	{UNICAM_8BIT, 0xF893, 0x8E},
	{UNICAM_8BIT, 0xF894, 0x78},
	{UNICAM_8BIT, 0xF895, 0x8F},
	{UNICAM_8BIT, 0xF896, 0x79},
	{UNICAM_8BIT, 0xF897, 0x12},
	{UNICAM_8BIT, 0xF898, 0xFA},
	{UNICAM_8BIT, 0xF899, 0x40},
	{UNICAM_8BIT, 0xF89A, 0x12},
	{UNICAM_8BIT, 0xF89B, 0x22},
	{UNICAM_8BIT, 0xF89C, 0x93},
	{UNICAM_8BIT, 0xF89D, 0x50},
	{UNICAM_8BIT, 0xF89E, 0x07},
	{UNICAM_8BIT, 0xF89F, 0xE4},
	{UNICAM_8BIT, 0xF8A0, 0xF5},
	{UNICAM_8BIT, 0xF8A1, 0x78},
	{UNICAM_8BIT, 0xF8A2, 0xF5},
	{UNICAM_8BIT, 0xF8A3, 0x79},
	{UNICAM_8BIT, 0xF8A4, 0x80},
	{UNICAM_8BIT, 0xF8A5, 0x33},
	{UNICAM_8BIT, 0xF8A6, 0x12},
	{UNICAM_8BIT, 0xF8A7, 0xFA},
	{UNICAM_8BIT, 0xF8A8, 0x40},
	{UNICAM_8BIT, 0xF8A9, 0x7B},
	{UNICAM_8BIT, 0xF8AA, 0x01},
	{UNICAM_8BIT, 0xF8AB, 0xAF},
	{UNICAM_8BIT, 0xF8AC, 0x79},
	{UNICAM_8BIT, 0xF8AD, 0xAE},
	{UNICAM_8BIT, 0xF8AE, 0x78},
	{UNICAM_8BIT, 0xF8AF, 0x12},
	{UNICAM_8BIT, 0xF8B0, 0x22},
	{UNICAM_8BIT, 0xF8B1, 0x4F},
	{UNICAM_8BIT, 0xF8B2, 0x74},
	{UNICAM_8BIT, 0xF8B3, 0x02},
	{UNICAM_8BIT, 0xF8B4, 0x12},
	{UNICAM_8BIT, 0xF8B5, 0xFA},
	{UNICAM_8BIT, 0xF8B6, 0x4C},
	{UNICAM_8BIT, 0xF8B7, 0x85},
	{UNICAM_8BIT, 0xF8B8, 0x7B},
	{UNICAM_8BIT, 0xF8B9, 0x82},
	{UNICAM_8BIT, 0xF8BA, 0xF5},
	{UNICAM_8BIT, 0xF8BB, 0x83},
	{UNICAM_8BIT, 0xF8BC, 0xE0},
	{UNICAM_8BIT, 0xF8BD, 0xFE},
	{UNICAM_8BIT, 0xF8BE, 0xA3},
	{UNICAM_8BIT, 0xF8BF, 0xE0},
	{UNICAM_8BIT, 0xF8C0, 0xFF},
	{UNICAM_8BIT, 0xF8C1, 0x7D},
	{UNICAM_8BIT, 0xF8C2, 0x03},
	{UNICAM_8BIT, 0xF8C3, 0x12},
	{UNICAM_8BIT, 0xF8C4, 0x17},
	{UNICAM_8BIT, 0xF8C5, 0xD8},
	{UNICAM_8BIT, 0xF8C6, 0x12},
	{UNICAM_8BIT, 0xF8C7, 0x1B},
	{UNICAM_8BIT, 0xF8C8, 0x9B},
	{UNICAM_8BIT, 0xF8C9, 0x8E},
	{UNICAM_8BIT, 0xF8CA, 0x78},
	{UNICAM_8BIT, 0xF8CB, 0x8F},
	{UNICAM_8BIT, 0xF8CC, 0x79},
	{UNICAM_8BIT, 0xF8CD, 0x74},
	{UNICAM_8BIT, 0xF8CE, 0xFE},
	{UNICAM_8BIT, 0xF8CF, 0x25},
	{UNICAM_8BIT, 0xF8D0, 0x7B},
	{UNICAM_8BIT, 0xF8D1, 0xF5},
	{UNICAM_8BIT, 0xF8D2, 0x7B},
	{UNICAM_8BIT, 0xF8D3, 0x74},
	{UNICAM_8BIT, 0xF8D4, 0xFF},
	{UNICAM_8BIT, 0xF8D5, 0x35},
	{UNICAM_8BIT, 0xF8D6, 0x7A},
	{UNICAM_8BIT, 0xF8D7, 0xF5},
	{UNICAM_8BIT, 0xF8D8, 0x7A},
	{UNICAM_8BIT, 0xF8D9, 0x78},
	{UNICAM_8BIT, 0xF8DA, 0x24},
	{UNICAM_8BIT, 0xF8DB, 0xE6},
	{UNICAM_8BIT, 0xF8DC, 0xFF},
	{UNICAM_8BIT, 0xF8DD, 0xC3},
	{UNICAM_8BIT, 0xF8DE, 0x74},
	{UNICAM_8BIT, 0xF8DF, 0x20},
	{UNICAM_8BIT, 0xF8E0, 0x9F},
	{UNICAM_8BIT, 0xF8E1, 0x7E},
	{UNICAM_8BIT, 0xF8E2, 0x00},
	{UNICAM_8BIT, 0xF8E3, 0x25},
	{UNICAM_8BIT, 0xF8E4, 0x79},
	{UNICAM_8BIT, 0xF8E5, 0xFF},
	{UNICAM_8BIT, 0xF8E6, 0xEE},
	{UNICAM_8BIT, 0xF8E7, 0x35},
	{UNICAM_8BIT, 0xF8E8, 0x78},
	{UNICAM_8BIT, 0xF8E9, 0x85},
	{UNICAM_8BIT, 0xF8EA, 0x7F},
	{UNICAM_8BIT, 0xF8EB, 0x82},
	{UNICAM_8BIT, 0xF8EC, 0x85},
	{UNICAM_8BIT, 0xF8ED, 0x7E},
	{UNICAM_8BIT, 0xF8EE, 0x83},
	{UNICAM_8BIT, 0xF8EF, 0xF0},
	{UNICAM_8BIT, 0xF8F0, 0xA3},
	{UNICAM_8BIT, 0xF8F1, 0xEF},
	{UNICAM_8BIT, 0xF8F2, 0xF0},
	{UNICAM_8BIT, 0xF8F3, 0x05},
	{UNICAM_8BIT, 0xF8F4, 0x77},
	{UNICAM_8BIT, 0xF8F5, 0x74},
	{UNICAM_8BIT, 0xF8F6, 0x02},
	{UNICAM_8BIT, 0xF8F7, 0x25},
	{UNICAM_8BIT, 0xF8F8, 0x7D},
	{UNICAM_8BIT, 0xF8F9, 0xF5},
	{UNICAM_8BIT, 0xF8FA, 0x7D},
	{UNICAM_8BIT, 0xF8FB, 0xE4},
	{UNICAM_8BIT, 0xF8FC, 0x35},
	{UNICAM_8BIT, 0xF8FD, 0x7C},
	{UNICAM_8BIT, 0xF8FE, 0xF5},
	{UNICAM_8BIT, 0xF8FF, 0x7C},
	{UNICAM_8BIT, 0xF900, 0x74},
	{UNICAM_8BIT, 0xF901, 0x02},
	{UNICAM_8BIT, 0xF902, 0x25},
	{UNICAM_8BIT, 0xF903, 0x7F},
	{UNICAM_8BIT, 0xF904, 0xF5},
	{UNICAM_8BIT, 0xF905, 0x7F},
	{UNICAM_8BIT, 0xF906, 0xE4},
	{UNICAM_8BIT, 0xF907, 0x35},
	{UNICAM_8BIT, 0xF908, 0x7E},
	{UNICAM_8BIT, 0xF909, 0xF5},
	{UNICAM_8BIT, 0xF90A, 0x7E},
	{UNICAM_8BIT, 0xF90B, 0x02},
	{UNICAM_8BIT, 0xF90C, 0xF8},
	{UNICAM_8BIT, 0xF90D, 0x7B},
	{UNICAM_8BIT, 0xF90E, 0x22},
	{UNICAM_8BIT, 0xF90F, 0x90},
	{UNICAM_8BIT, 0xF910, 0x30},
	{UNICAM_8BIT, 0xF911, 0x47},
	{UNICAM_8BIT, 0xF912, 0x74},
	{UNICAM_8BIT, 0xF913, 0x98},
	{UNICAM_8BIT, 0xF914, 0xF0},
	{UNICAM_8BIT, 0xF915, 0x90},
	{UNICAM_8BIT, 0xF916, 0x30},
	{UNICAM_8BIT, 0xF917, 0x36},
	{UNICAM_8BIT, 0xF918, 0x74},
	{UNICAM_8BIT, 0xF919, 0x1E},
	{UNICAM_8BIT, 0xF91A, 0xF0},
	{UNICAM_8BIT, 0xF91B, 0x90},
	{UNICAM_8BIT, 0xF91C, 0x30},
	{UNICAM_8BIT, 0xF91D, 0x42},
	{UNICAM_8BIT, 0xF91E, 0x74},
	{UNICAM_8BIT, 0xF91F, 0x24},
	{UNICAM_8BIT, 0xF920, 0xF0},
	{UNICAM_8BIT, 0xF921, 0xE5},
	{UNICAM_8BIT, 0xF922, 0x53},
	{UNICAM_8BIT, 0xF923, 0x60},
	{UNICAM_8BIT, 0xF924, 0x42},
	{UNICAM_8BIT, 0xF925, 0x78},
	{UNICAM_8BIT, 0xF926, 0x2B},
	{UNICAM_8BIT, 0xF927, 0x76},
	{UNICAM_8BIT, 0xF928, 0x01},
	{UNICAM_8BIT, 0xF929, 0xE5},
	{UNICAM_8BIT, 0xF92A, 0x55},
	{UNICAM_8BIT, 0xF92B, 0xB4},
	{UNICAM_8BIT, 0xF92C, 0x22},
	{UNICAM_8BIT, 0xF92D, 0x17},
	{UNICAM_8BIT, 0xF92E, 0x90},
	{UNICAM_8BIT, 0xF92F, 0x30},
	{UNICAM_8BIT, 0xF930, 0x36},
	{UNICAM_8BIT, 0xF931, 0x74},
	{UNICAM_8BIT, 0xF932, 0x46},
	{UNICAM_8BIT, 0xF933, 0xF0},
	{UNICAM_8BIT, 0xF934, 0x78},
	{UNICAM_8BIT, 0xF935, 0x28},
	{UNICAM_8BIT, 0xF936, 0x76},
	{UNICAM_8BIT, 0xF937, 0x31},
	{UNICAM_8BIT, 0xF938, 0x90},
	{UNICAM_8BIT, 0xF939, 0x30},
	{UNICAM_8BIT, 0xF93A, 0x0E},
	{UNICAM_8BIT, 0xF93B, 0xE0},
	{UNICAM_8BIT, 0xF93C, 0xC3},
	{UNICAM_8BIT, 0xF93D, 0x13},
	{UNICAM_8BIT, 0xF93E, 0x30},
	{UNICAM_8BIT, 0xF93F, 0xE0},
	{UNICAM_8BIT, 0xF940, 0x04},
	{UNICAM_8BIT, 0xF941, 0x78},
	{UNICAM_8BIT, 0xF942, 0x26},
	{UNICAM_8BIT, 0xF943, 0x76},
	{UNICAM_8BIT, 0xF944, 0x40},
	{UNICAM_8BIT, 0xF945, 0xE5},
	{UNICAM_8BIT, 0xF946, 0x55},
	{UNICAM_8BIT, 0xF947, 0xB4},
	{UNICAM_8BIT, 0xF948, 0x44},
	{UNICAM_8BIT, 0xF949, 0x21},
	{UNICAM_8BIT, 0xF94A, 0x90},
	{UNICAM_8BIT, 0xF94B, 0x30},
	{UNICAM_8BIT, 0xF94C, 0x47},
	{UNICAM_8BIT, 0xF94D, 0x74},
	{UNICAM_8BIT, 0xF94E, 0x9A},
	{UNICAM_8BIT, 0xF94F, 0xF0},
	{UNICAM_8BIT, 0xF950, 0x90},
	{UNICAM_8BIT, 0xF951, 0x30},
	{UNICAM_8BIT, 0xF952, 0x42},
	{UNICAM_8BIT, 0xF953, 0x74},
	{UNICAM_8BIT, 0xF954, 0x64},
	{UNICAM_8BIT, 0xF955, 0xF0},
	{UNICAM_8BIT, 0xF956, 0x90},
	{UNICAM_8BIT, 0xF957, 0x30},
	{UNICAM_8BIT, 0xF958, 0x0E},
	{UNICAM_8BIT, 0xF959, 0xE0},
	{UNICAM_8BIT, 0xF95A, 0x13},
	{UNICAM_8BIT, 0xF95B, 0x13},
	{UNICAM_8BIT, 0xF95C, 0x54},
	{UNICAM_8BIT, 0xF95D, 0x3F},
	{UNICAM_8BIT, 0xF95E, 0x30},
	{UNICAM_8BIT, 0xF95F, 0xE0},
	{UNICAM_8BIT, 0xF960, 0x0A},
	{UNICAM_8BIT, 0xF961, 0x78},
	{UNICAM_8BIT, 0xF962, 0x24},
	{UNICAM_8BIT, 0xF963, 0xE4},
	{UNICAM_8BIT, 0xF964, 0xF6},
	{UNICAM_8BIT, 0xF965, 0x80},
	{UNICAM_8BIT, 0xF966, 0x04},
	{UNICAM_8BIT, 0xF967, 0x78},
	{UNICAM_8BIT, 0xF968, 0x2B},
	{UNICAM_8BIT, 0xF969, 0xE4},
	{UNICAM_8BIT, 0xF96A, 0xF6},
	{UNICAM_8BIT, 0xF96B, 0x90},
	{UNICAM_8BIT, 0xF96C, 0x30},
	{UNICAM_8BIT, 0xF96D, 0x88},
	{UNICAM_8BIT, 0xF96E, 0x02},
	{UNICAM_8BIT, 0xF96F, 0x1D},
	{UNICAM_8BIT, 0xF970, 0x4F},
	{UNICAM_8BIT, 0xF971, 0x22},
	{UNICAM_8BIT, 0xF972, 0x90},
	{UNICAM_8BIT, 0xF973, 0x0C},
	{UNICAM_8BIT, 0xF974, 0x1A},
	{UNICAM_8BIT, 0xF975, 0xE0},
	{UNICAM_8BIT, 0xF976, 0x30},
	{UNICAM_8BIT, 0xF977, 0xE2},
	{UNICAM_8BIT, 0xF978, 0x18},
	{UNICAM_8BIT, 0xF979, 0x90},
	{UNICAM_8BIT, 0xF97A, 0x33},
	{UNICAM_8BIT, 0xF97B, 0x68},
	{UNICAM_8BIT, 0xF97C, 0xE0},
	{UNICAM_8BIT, 0xF97D, 0x64},
	{UNICAM_8BIT, 0xF97E, 0x05},
	{UNICAM_8BIT, 0xF97F, 0x70},
	{UNICAM_8BIT, 0xF980, 0x2F},
	{UNICAM_8BIT, 0xF981, 0x90},
	{UNICAM_8BIT, 0xF982, 0x30},
	{UNICAM_8BIT, 0xF983, 0x38},
	{UNICAM_8BIT, 0xF984, 0xE0},
	{UNICAM_8BIT, 0xF985, 0x70},
	{UNICAM_8BIT, 0xF986, 0x02},
	{UNICAM_8BIT, 0xF987, 0xA3},
	{UNICAM_8BIT, 0xF988, 0xE0},
	{UNICAM_8BIT, 0xF989, 0xC3},
	{UNICAM_8BIT, 0xF98A, 0x70},
	{UNICAM_8BIT, 0xF98B, 0x01},
	{UNICAM_8BIT, 0xF98C, 0xD3},
	{UNICAM_8BIT, 0xF98D, 0x40},
	{UNICAM_8BIT, 0xF98E, 0x21},
	{UNICAM_8BIT, 0xF98F, 0x80},
	{UNICAM_8BIT, 0xF990, 0x1B},
	{UNICAM_8BIT, 0xF991, 0x90},
	{UNICAM_8BIT, 0xF992, 0x33},
	{UNICAM_8BIT, 0xF993, 0x68},
	{UNICAM_8BIT, 0xF994, 0xE0},
	{UNICAM_8BIT, 0xF995, 0xB4},
	{UNICAM_8BIT, 0xF996, 0x05},
	{UNICAM_8BIT, 0xF997, 0x18},
	{UNICAM_8BIT, 0xF998, 0xC3},
	{UNICAM_8BIT, 0xF999, 0x90},
	{UNICAM_8BIT, 0xF99A, 0x30},
	{UNICAM_8BIT, 0xF99B, 0x3B},
	{UNICAM_8BIT, 0xF99C, 0xE0},
	{UNICAM_8BIT, 0xF99D, 0x94},
	{UNICAM_8BIT, 0xF99E, 0x0D},
	{UNICAM_8BIT, 0xF99F, 0x90},
	{UNICAM_8BIT, 0xF9A0, 0x30},
	{UNICAM_8BIT, 0xF9A1, 0x3A},
	{UNICAM_8BIT, 0xF9A2, 0xE0},
	{UNICAM_8BIT, 0xF9A3, 0x94},
	{UNICAM_8BIT, 0xF9A4, 0x00},
	{UNICAM_8BIT, 0xF9A5, 0x50},
	{UNICAM_8BIT, 0xF9A6, 0x02},
	{UNICAM_8BIT, 0xF9A7, 0x80},
	{UNICAM_8BIT, 0xF9A8, 0x01},
	{UNICAM_8BIT, 0xF9A9, 0xC3},
	{UNICAM_8BIT, 0xF9AA, 0x40},
	{UNICAM_8BIT, 0xF9AB, 0x04},
	{UNICAM_8BIT, 0xF9AC, 0x75},
	{UNICAM_8BIT, 0xF9AD, 0x10},
	{UNICAM_8BIT, 0xF9AE, 0x01},
	{UNICAM_8BIT, 0xF9AF, 0x22},
	{UNICAM_8BIT, 0xF9B0, 0x02},
	{UNICAM_8BIT, 0xF9B1, 0x16},
	{UNICAM_8BIT, 0xF9B2, 0xE1},
	{UNICAM_8BIT, 0xF9B3, 0x22},
	{UNICAM_8BIT, 0xF9B4, 0x90},
	{UNICAM_8BIT, 0xF9B5, 0xFF},
	{UNICAM_8BIT, 0xF9B6, 0x33},
	{UNICAM_8BIT, 0xF9B7, 0xE0},
	{UNICAM_8BIT, 0xF9B8, 0x90},
	{UNICAM_8BIT, 0xF9B9, 0xFF},
	{UNICAM_8BIT, 0xF9BA, 0x34},
	{UNICAM_8BIT, 0xF9BB, 0xE0},
	{UNICAM_8BIT, 0xF9BC, 0x60},
	{UNICAM_8BIT, 0xF9BD, 0x0D},
	{UNICAM_8BIT, 0xF9BE, 0x7C},
	{UNICAM_8BIT, 0xF9BF, 0xFB},
	{UNICAM_8BIT, 0xF9C0, 0x7D},
	{UNICAM_8BIT, 0xF9C1, 0xD7},
	{UNICAM_8BIT, 0xF9C2, 0x7B},
	{UNICAM_8BIT, 0xF9C3, 0x28},
	{UNICAM_8BIT, 0xF9C4, 0x7F},
	{UNICAM_8BIT, 0xF9C5, 0x34},
	{UNICAM_8BIT, 0xF9C6, 0x7E},
	{UNICAM_8BIT, 0xF9C7, 0xFF},
	{UNICAM_8BIT, 0xF9C8, 0x12},
	{UNICAM_8BIT, 0xF9C9, 0x09},
	{UNICAM_8BIT, 0xF9CA, 0x47},
	{UNICAM_8BIT, 0xF9CB, 0x7F},
	{UNICAM_8BIT, 0xF9CC, 0x20},
	{UNICAM_8BIT, 0xF9CD, 0x7E},
	{UNICAM_8BIT, 0xF9CE, 0x01},
	{UNICAM_8BIT, 0xF9CF, 0x7D},
	{UNICAM_8BIT, 0xF9D0, 0x00},
	{UNICAM_8BIT, 0xF9D1, 0x7C},
	{UNICAM_8BIT, 0xF9D2, 0x00},
	{UNICAM_8BIT, 0xF9D3, 0x12},
	{UNICAM_8BIT, 0xF9D4, 0x12},
	{UNICAM_8BIT, 0xF9D5, 0xA4},
	{UNICAM_8BIT, 0xF9D6, 0xE4},
	{UNICAM_8BIT, 0xF9D7, 0x90},
	{UNICAM_8BIT, 0xF9D8, 0x3E},
	{UNICAM_8BIT, 0xF9D9, 0x44},
	{UNICAM_8BIT, 0xF9DA, 0xF0},
	{UNICAM_8BIT, 0xF9DB, 0x02},
	{UNICAM_8BIT, 0xF9DC, 0x16},
	{UNICAM_8BIT, 0xF9DD, 0x7E},
	{UNICAM_8BIT, 0xF9DE, 0x22},
	{UNICAM_8BIT, 0xF9DF, 0xE5},
	{UNICAM_8BIT, 0xF9E0, 0x44},
	{UNICAM_8BIT, 0xF9E1, 0x60},
	{UNICAM_8BIT, 0xF9E2, 0x10},
	{UNICAM_8BIT, 0xF9E3, 0x90},
	{UNICAM_8BIT, 0xF9E4, 0xF6},
	{UNICAM_8BIT, 0xF9E5, 0x2C},
	{UNICAM_8BIT, 0xF9E6, 0x74},
	{UNICAM_8BIT, 0xF9E7, 0x04},
	{UNICAM_8BIT, 0xF9E8, 0xF0},
	{UNICAM_8BIT, 0xF9E9, 0x90},
	{UNICAM_8BIT, 0xF9EA, 0xF6},
	{UNICAM_8BIT, 0xF9EB, 0x34},
	{UNICAM_8BIT, 0xF9EC, 0xF0},
	{UNICAM_8BIT, 0xF9ED, 0x90},
	{UNICAM_8BIT, 0xF9EE, 0xF6},
	{UNICAM_8BIT, 0xF9EF, 0x3C},
	{UNICAM_8BIT, 0xF9F0, 0xF0},
	{UNICAM_8BIT, 0xF9F1, 0x80},
	{UNICAM_8BIT, 0xF9F2, 0x0E},
	{UNICAM_8BIT, 0xF9F3, 0x90},
	{UNICAM_8BIT, 0xF9F4, 0xF5},
	{UNICAM_8BIT, 0xF9F5, 0xC0},
	{UNICAM_8BIT, 0xF9F6, 0x74},
	{UNICAM_8BIT, 0xF9F7, 0x04},
	{UNICAM_8BIT, 0xF9F8, 0xF0},
	{UNICAM_8BIT, 0xF9F9, 0x90},
	{UNICAM_8BIT, 0xF9FA, 0xF5},
	{UNICAM_8BIT, 0xF9FB, 0xC8},
	{UNICAM_8BIT, 0xF9FC, 0xF0},
	{UNICAM_8BIT, 0xF9FD, 0x90},
	{UNICAM_8BIT, 0xF9FE, 0xF5},
	{UNICAM_8BIT, 0xF9FF, 0xD0},
	{UNICAM_8BIT, 0xFA00, 0xF0},
	{UNICAM_8BIT, 0xFA01, 0x90},
	{UNICAM_8BIT, 0xFA02, 0xFB},
	{UNICAM_8BIT, 0xFA03, 0x7F},
	{UNICAM_8BIT, 0xFA04, 0x02},
	{UNICAM_8BIT, 0xFA05, 0x19},
	{UNICAM_8BIT, 0xFA06, 0x0B},
	{UNICAM_8BIT, 0xFA07, 0x22},
	{UNICAM_8BIT, 0xFA08, 0x90},
	{UNICAM_8BIT, 0xFA09, 0x0C},
	{UNICAM_8BIT, 0xFA0A, 0x1A},
	{UNICAM_8BIT, 0xFA0B, 0xE0},
	{UNICAM_8BIT, 0xFA0C, 0x20},
	{UNICAM_8BIT, 0xFA0D, 0xE2},
	{UNICAM_8BIT, 0xFA0E, 0x15},
	{UNICAM_8BIT, 0xFA0F, 0xE4},
	{UNICAM_8BIT, 0xFA10, 0x90},
	{UNICAM_8BIT, 0xFA11, 0x30},
	{UNICAM_8BIT, 0xFA12, 0xF8},
	{UNICAM_8BIT, 0xFA13, 0xF0},
	{UNICAM_8BIT, 0xFA14, 0xA3},
	{UNICAM_8BIT, 0xFA15, 0xF0},
	{UNICAM_8BIT, 0xFA16, 0x90},
	{UNICAM_8BIT, 0xFA17, 0x30},
	{UNICAM_8BIT, 0xFA18, 0xF1},
	{UNICAM_8BIT, 0xFA19, 0xE0},
	{UNICAM_8BIT, 0xFA1A, 0x44},
	{UNICAM_8BIT, 0xFA1B, 0x08},
	{UNICAM_8BIT, 0xFA1C, 0xF0},
	{UNICAM_8BIT, 0xFA1D, 0x90},
	{UNICAM_8BIT, 0xFA1E, 0x30},
	{UNICAM_8BIT, 0xFA1F, 0xF0},
	{UNICAM_8BIT, 0xFA20, 0xE0},
	{UNICAM_8BIT, 0xFA21, 0x44},
	{UNICAM_8BIT, 0xFA22, 0x08},
	{UNICAM_8BIT, 0xFA23, 0xF0},
	{UNICAM_8BIT, 0xFA24, 0x02},
	{UNICAM_8BIT, 0xFA25, 0x03},
	{UNICAM_8BIT, 0xFA26, 0xDE},
	{UNICAM_8BIT, 0xFA27, 0x22},
	{UNICAM_8BIT, 0xFA28, 0x90},
	{UNICAM_8BIT, 0xFA29, 0x0C},
	{UNICAM_8BIT, 0xFA2A, 0x1A},
	{UNICAM_8BIT, 0xFA2B, 0xE0},
	{UNICAM_8BIT, 0xFA2C, 0x30},
	{UNICAM_8BIT, 0xFA2D, 0xE2},
	{UNICAM_8BIT, 0xFA2E, 0x0D},
	{UNICAM_8BIT, 0xFA2F, 0xE0},
	{UNICAM_8BIT, 0xFA30, 0x20},
	{UNICAM_8BIT, 0xFA31, 0xE0},
	{UNICAM_8BIT, 0xFA32, 0x06},
	{UNICAM_8BIT, 0xFA33, 0x90},
	{UNICAM_8BIT, 0xFA34, 0xFB},
	{UNICAM_8BIT, 0xFA35, 0x85},
	{UNICAM_8BIT, 0xFA36, 0x74},
	{UNICAM_8BIT, 0xFA37, 0x00},
	{UNICAM_8BIT, 0xFA38, 0xA5},
	{UNICAM_8BIT, 0xFA39, 0x12},
	{UNICAM_8BIT, 0xFA3A, 0x16},
	{UNICAM_8BIT, 0xFA3B, 0xA0},
	{UNICAM_8BIT, 0xFA3C, 0x02},
	{UNICAM_8BIT, 0xFA3D, 0x18},
	{UNICAM_8BIT, 0xFA3E, 0xAC},
	{UNICAM_8BIT, 0xFA3F, 0x22},
	{UNICAM_8BIT, 0xFA40, 0x85},
	{UNICAM_8BIT, 0xFA41, 0x7B},
	{UNICAM_8BIT, 0xFA42, 0x82},
	{UNICAM_8BIT, 0xFA43, 0x85},
	{UNICAM_8BIT, 0xFA44, 0x7A},
	{UNICAM_8BIT, 0xFA45, 0x83},
	{UNICAM_8BIT, 0xFA46, 0xE0},
	{UNICAM_8BIT, 0xFA47, 0xFC},
	{UNICAM_8BIT, 0xFA48, 0xA3},
	{UNICAM_8BIT, 0xFA49, 0xE0},
	{UNICAM_8BIT, 0xFA4A, 0xFD},
	{UNICAM_8BIT, 0xFA4B, 0x22},
	{UNICAM_8BIT, 0xFA4C, 0x25},
	{UNICAM_8BIT, 0xFA4D, 0x7B},
	{UNICAM_8BIT, 0xFA4E, 0xF5},
	{UNICAM_8BIT, 0xFA4F, 0x7B},
	{UNICAM_8BIT, 0xFA50, 0xE4},
	{UNICAM_8BIT, 0xFA51, 0x35},
	{UNICAM_8BIT, 0xFA52, 0x7A},
	{UNICAM_8BIT, 0xFA53, 0xF5},
	{UNICAM_8BIT, 0xFA54, 0x7A},
	{UNICAM_8BIT, 0xFA55, 0x22},
	{UNICAM_8BIT, 0xFA56, 0xC0},
	{UNICAM_8BIT, 0xFA57, 0xD0},
	{UNICAM_8BIT, 0xFA58, 0x90},
	{UNICAM_8BIT, 0xFA59, 0x35},
	{UNICAM_8BIT, 0xFA5A, 0xB5},
	{UNICAM_8BIT, 0xFA5B, 0xE0},
	{UNICAM_8BIT, 0xFA5C, 0x54},
	{UNICAM_8BIT, 0xFA5D, 0xFC},
	{UNICAM_8BIT, 0xFA5E, 0x44},
	{UNICAM_8BIT, 0xFA5F, 0x01},
	{UNICAM_8BIT, 0xFA60, 0xF0},
	{UNICAM_8BIT, 0xFA61, 0x12},
	{UNICAM_8BIT, 0xFA62, 0x1F},
	{UNICAM_8BIT, 0xFA63, 0x5F},
	{UNICAM_8BIT, 0xFA64, 0xD0},
	{UNICAM_8BIT, 0xFA65, 0xD0},
	{UNICAM_8BIT, 0xFA66, 0x02},
	{UNICAM_8BIT, 0xFA67, 0x0A},
	{UNICAM_8BIT, 0xFA68, 0x16},
	{UNICAM_8BIT, 0xFA69, 0x22},
	{UNICAM_8BIT, 0xFA6A, 0x90},
	{UNICAM_8BIT, 0xFA6B, 0x0C},
	{UNICAM_8BIT, 0xFA6C, 0x1A},
	{UNICAM_8BIT, 0xFA6D, 0xE0},
	{UNICAM_8BIT, 0xFA6E, 0x20},
	{UNICAM_8BIT, 0xFA6F, 0xE0},
	{UNICAM_8BIT, 0xFA70, 0x06},
	{UNICAM_8BIT, 0xFA71, 0x90},
	{UNICAM_8BIT, 0xFA72, 0xFB},
	{UNICAM_8BIT, 0xFA73, 0x85},
	{UNICAM_8BIT, 0xFA74, 0x74},
	{UNICAM_8BIT, 0xFA75, 0x00},
	{UNICAM_8BIT, 0xFA76, 0xA5},
	{UNICAM_8BIT, 0xFA77, 0xE5},
	{UNICAM_8BIT, 0xFA78, 0x10},
	{UNICAM_8BIT, 0xFA79, 0x02},
	{UNICAM_8BIT, 0xFA7A, 0x1E},
	{UNICAM_8BIT, 0xFA7B, 0x8F},
	{UNICAM_8BIT, 0xFA7C, 0x22},
	{UNICAM_8BIT, 0xFA7D, 0x90},
	{UNICAM_8BIT, 0xFA7E, 0xFB},
	{UNICAM_8BIT, 0xFA7F, 0x85},
	{UNICAM_8BIT, 0xFA80, 0x74},
	{UNICAM_8BIT, 0xFA81, 0x00},
	{UNICAM_8BIT, 0xFA82, 0xA5},
	{UNICAM_8BIT, 0xFA83, 0xE5},
	{UNICAM_8BIT, 0xFA84, 0x1A},
	{UNICAM_8BIT, 0xFA85, 0x60},
	{UNICAM_8BIT, 0xFA86, 0x03},
	{UNICAM_8BIT, 0xFA87, 0x02},
	{UNICAM_8BIT, 0xFA88, 0x17},
	{UNICAM_8BIT, 0xFA89, 0x47},
	{UNICAM_8BIT, 0xFA8A, 0x22},
	{UNICAM_8BIT, 0xFA8B, 0x90},
	{UNICAM_8BIT, 0xFA8C, 0xFB},
	{UNICAM_8BIT, 0xFA8D, 0x84},
	{UNICAM_8BIT, 0xFA8E, 0x02},
	{UNICAM_8BIT, 0xFA8F, 0x18},
	{UNICAM_8BIT, 0xFA90, 0xD9},
	{UNICAM_8BIT, 0xFA91, 0x22},
	{UNICAM_8BIT, 0xFA92, 0x02},
	{UNICAM_8BIT, 0xFA93, 0x1F},
	{UNICAM_8BIT, 0xFA94, 0xB1},
	{UNICAM_8BIT, 0xFA95, 0x22},
	{UNICAM_8BIT, 0x35D8, 0x01},
	{UNICAM_8BIT, 0x35D9, 0x0F},
	{UNICAM_8BIT, 0x35DA, 0x01},
	{UNICAM_8BIT, 0x35DB, 0x72},
	{UNICAM_8BIT, 0x35DC, 0x01},
	{UNICAM_8BIT, 0x35DD, 0xB4},
	{UNICAM_8BIT, 0x35DE, 0x01},
	{UNICAM_8BIT, 0x35DF, 0xDF},
	{UNICAM_8BIT, 0x35E0, 0x02},
	{UNICAM_8BIT, 0x35E1, 0x08},
	{UNICAM_8BIT, 0x35E2, 0x02},
	{UNICAM_8BIT, 0x35E3, 0x28},
	{UNICAM_8BIT, 0x35E4, 0x02},
	{UNICAM_8BIT, 0x35E5, 0x56},
	{UNICAM_8BIT, 0x35E6, 0x02},
	{UNICAM_8BIT, 0x35E7, 0x6A},
	{UNICAM_8BIT, 0x35E8, 0x02},
	{UNICAM_8BIT, 0x35E9, 0x7D},
	{UNICAM_8BIT, 0x35EA, 0x02},
	{UNICAM_8BIT, 0x35EB, 0x8B},
	{UNICAM_8BIT, 0x35EC, 0x02},
	{UNICAM_8BIT, 0x35ED, 0x92},
	{UNICAM_8BIT, 0x35EF, 0x22},
	{UNICAM_8BIT, 0x35F1, 0x23},
	{UNICAM_8BIT, 0x35F3, 0x22},
	{UNICAM_8BIT, 0x35F6, 0x19},
	{UNICAM_8BIT, 0x35F7, 0x55},
	{UNICAM_8BIT, 0x35F8, 0x1D},
	{UNICAM_8BIT, 0x35F9, 0x4C},
	{UNICAM_8BIT, 0x35FA, 0x16},
	{UNICAM_8BIT, 0x35FB, 0xC7},
	{UNICAM_8BIT, 0x35FC, 0x1A},
	{UNICAM_8BIT, 0x35FD, 0xA0},
	{UNICAM_8BIT, 0x35FE, 0x18},
	{UNICAM_8BIT, 0x35FF, 0xD6},
	{UNICAM_8BIT, 0x3600, 0x03},
	{UNICAM_8BIT, 0x3601, 0xD4},
	{UNICAM_8BIT, 0x3602, 0x18},
	{UNICAM_8BIT, 0x3603, 0x8A},
	{UNICAM_8BIT, 0x3604, 0x0A},
	{UNICAM_8BIT, 0x3605, 0x0D},
	{UNICAM_8BIT, 0x3606, 0x1E},
	{UNICAM_8BIT, 0x3607, 0x8D},
	{UNICAM_8BIT, 0x3608, 0x17},
	{UNICAM_8BIT, 0x3609, 0x43},
	{UNICAM_8BIT, 0x360A, 0x19},
	{UNICAM_8BIT, 0x360B, 0x16},
	{UNICAM_8BIT, 0x360C, 0x1F},
	{UNICAM_8BIT, 0x360D, 0xAD},
	{UNICAM_8BIT, 0x360E, 0x19},
	{UNICAM_8BIT, 0x360F, 0x08},
	{UNICAM_8BIT, 0x3610, 0x14},
	{UNICAM_8BIT, 0x3611, 0x26},
	{UNICAM_8BIT, 0x3612, 0x1A},
	{UNICAM_8BIT, 0x3613, 0xB3},
	{UNICAM_8BIT, 0x35D2, 0x7F},
	{UNICAM_8BIT, 0x35D3, 0xFF},
	{UNICAM_8BIT, 0x35D4, 0x70},
	{UNICAM_8BIT, 0x35D0, 0x01},
	{UNICAM_8BIT, 0x3E44, 0x01},
	{UNICAM_8BIT, 0x3570, 0x01},

	/*
	//-------------------------------------
	//	CSI mode
	//-------------------------------------
	{UNICAM_8BIT, 0x0111, 0x02},
	{UNICAM_8BIT, 0x0114, 0x01},	
	{UNICAM_8BIT, 0x2136, 0x0C},
	{UNICAM_8BIT, 0x2137, 0x00},
	//------------------
	// //flip+mirror
	//--------------------
	{UNICAM_8BIT, 0x0101, 0x03}, 

	//-------------------------------------
	//	CSI data format
	//-------------------------------------
	{UNICAM_8BIT, 0x0112, 0x0A},
	{UNICAM_8BIT, 0x0113, 0x0A},	
	{UNICAM_8BIT, 0x3016, 0x46},
	{UNICAM_8BIT, 0x3017, 0x29},
	{UNICAM_8BIT, 0x3003, 0x03},
	{UNICAM_8BIT, 0x3045, 0x03},
	{UNICAM_8BIT, 0x3047, 0x98},

	//-------------------------------------
	//	mode select
	//-------------------------------------
	{UNICAM_8BIT, 0x0100, 0x00},
	*/
	{UNICAM_TOK_TERM, 0, 0}
};


S_UNI_REG  hm5040_group_hold_start[] = {
	{UNICAM_8BIT, 0x0104, 0x01},
	{UNICAM_TOK_TERM, 0, 0}
};
S_UNI_REG  hm5040_group_hold_end[] = {
	{UNICAM_8BIT, 0x0104, 0x00},
	{UNICAM_TOK_TERM, 0, 0}
};
static  int hm5040_adgain_p[] = {
	0,
};

#ifdef HM5040_BINNING_MODE 
S_UNI_REG  hm5040_AB2_1304x976_30fps[] = {
	//-------------------------------------
	//	CSI mode
	//-------------------------------------
	{UNICAM_8BIT, 0x0111, 0x02},
	{UNICAM_8BIT, 0x0114, 0x01},	
	{UNICAM_8BIT, 0x2136, 0x0C}, //mipi timmng
	{UNICAM_8BIT, 0x2137, 0x00}, //mipi timmng
	//------------------
	// //flip+mirror
	//--------------------
//	{UNICAM_8BIT, 0x0101, 0x03}, 

	//-------------------------------------
	//	CSI data format
	//-------------------------------------
	{UNICAM_8BIT, 0x0112, 0x0A},
	{UNICAM_8BIT, 0x0113, 0x0A},	
	{UNICAM_8BIT, 0x3016, 0x46},
	{UNICAM_8BIT, 0x3017, 0x29},
	{UNICAM_8BIT, 0x3003, 0x03},
	{UNICAM_8BIT, 0x3045, 0x03},
	{UNICAM_8BIT, 0x3047, 0x98},

	//-------------------------------------
	//	mode select
	//-------------------------------------
	{UNICAM_8BIT, 0x0100, 0x00},



	//-------------------------------------
	//	PLL
	//-------------------------------------
	{UNICAM_8BIT, 0x0305, 0x02},
	{UNICAM_8BIT, 0x0306, 0x00},
	{UNICAM_8BIT, 0x0307, 0x7D},  //AB->AA
	{UNICAM_8BIT, 0x0301, 0x0A},
	{UNICAM_8BIT, 0x0309, 0x0A},   



	//frame Length

	{UNICAM_8BIT, 0x0340, 0x05},  //AB->AA
	{UNICAM_8BIT, 0x0341, 0xAE},  //AB->AA

	//{UNICAM_8BIT, 0x0340, 0x07},
	//{UNICAM_8BIT, 0x0341, 0xB6},  //C4
	{UNICAM_8BIT, 0x0342, 0x0A},
	{UNICAM_8BIT, 0x0343, 0xBE},  

	//-------------------------------------
	//	address increment for odd pixel 
	//-------------------------------------
	{UNICAM_8BIT, 0x0383, 0x01},
	{UNICAM_8BIT, 0x0387, 0x01},

	//-------------------------------------
	//	integration time
	//-------------------------------------
	{UNICAM_8BIT, 0x0202, 0x01}, //0x08 
	{UNICAM_8BIT, 0x0203, 0x00},



	//-------------------------------------
	//	Gain
	//-------------------------------------
	{UNICAM_8BIT, 0x0205, 0xC0},  //c0->00

	//-------------------------------------
	//	Binning
	//-------------------------------------
	{UNICAM_8BIT, 0x0900, 0x01},
	{UNICAM_8BIT, 0x0901, 0x22},
	{UNICAM_8BIT, 0x0902, 0x00},

	//-------------------------------------
	//	Crop image
	//-------------------------------------
	{UNICAM_8BIT, 0x040C, 0x05},
	{UNICAM_8BIT, 0x040D, 0x18},
	{UNICAM_8BIT, 0x040E, 0x03},
	{UNICAM_8BIT, 0x040F, 0xD0}, 

	//	analog start_end address
	//-------------------------------------
	{UNICAM_8BIT, 0x0344, 0x00},
	{UNICAM_8BIT, 0x0345, 0x00},	//x_start-0
	{UNICAM_8BIT, 0x0346, 0x00},
	{UNICAM_8BIT, 0x0347, 0x00},	//y_start-0
	{UNICAM_8BIT, 0x0348, 0x0A},
	{UNICAM_8BIT, 0x0349, 0x27},	//x_end-2599
	{UNICAM_8BIT, 0x034A, 0x07},
	{UNICAM_8BIT, 0x034B, 0x9F},	//y_end-1951

	//-------------------------------------
	//	Data output size
	//-------------------------------------
	{UNICAM_8BIT, 0x034C, 0x05},
	{UNICAM_8BIT, 0x034D, 0x18},	//x_output_size -1304
	{UNICAM_8BIT, 0x034E, 0x03},
	{UNICAM_8BIT, 0x034F, 0xD0},	//Y_output_size -976

	//-------------------------------------
	//	digital_crop_offset
	//-------------------------------------
	{UNICAM_8BIT, 0x0408, 0x00},
	{UNICAM_8BIT, 0x0409, 0x01},	//X_offset_0
	{UNICAM_8BIT, 0x040A, 0x00},
	{UNICAM_8BIT, 0x040B, 0x00},	//Y_offset_0

	//-------------------------------------
	//	Crop image
	//-------------------------------------
	{UNICAM_8BIT, 0x040C, 0x05},
	{UNICAM_8BIT, 0x040D, 0x18},
	{UNICAM_8BIT, 0x040E, 0x03},
	{UNICAM_8BIT, 0x040F, 0xD0},

	//-------------------------------------
	//	mode select
	//-------------------------------------
	{UNICAM_8BIT, 0x0100, 0x01},
	{UNICAM_TOK_TERM, 0, 0}
	};
S_UNI_REG  hm5040_AB2_1288x728_30fps[] = {
	//-------------------------------------
	//	CSI mode
	//-------------------------------------
	{UNICAM_8BIT, 0x0111, 0x02},
	{UNICAM_8BIT, 0x0114, 0x01},	
	{UNICAM_8BIT, 0x2136, 0x0C}, //mipi timmng
	{UNICAM_8BIT, 0x2137, 0x00}, //mipi timmng
	//------------------
	// //flip+mirror
	//--------------------
//	{UNICAM_8BIT, 0x0101, 0x03}, 

	//-------------------------------------
	//	CSI data format
	//-------------------------------------
	{UNICAM_8BIT, 0x0112, 0x0A},
	{UNICAM_8BIT, 0x0113, 0x0A},	
	{UNICAM_8BIT, 0x3016, 0x46},
	{UNICAM_8BIT, 0x3017, 0x29},
	{UNICAM_8BIT, 0x3003, 0x03},
	{UNICAM_8BIT, 0x3045, 0x03},
	{UNICAM_8BIT, 0x3047, 0x98},

	//-------------------------------------
	//	mode select
	//-------------------------------------
	{UNICAM_8BIT, 0x0100, 0x00},



	//-------------------------------------
	//	PLL
	//-------------------------------------
	{UNICAM_8BIT, 0x0305, 0x02},
	{UNICAM_8BIT, 0x0306, 0x00},
	{UNICAM_8BIT, 0x0307, 0x7D},  //AB->AA
	{UNICAM_8BIT, 0x0301, 0x0A},
	{UNICAM_8BIT, 0x0309, 0x0A},   



	//frame Length

	{UNICAM_8BIT, 0x0340, 0x05},  //AB->AA
	{UNICAM_8BIT, 0x0341, 0xAE},  //AB->AA

	//{UNICAM_8BIT, 0x0340, 0x07},
	//{UNICAM_8BIT, 0x0341, 0xB6},  //C4
	{UNICAM_8BIT, 0x0342, 0x0A},
	{UNICAM_8BIT, 0x0343, 0xBE},  

	//-------------------------------------
	//	address increment for odd pixel 
	//-------------------------------------
	{UNICAM_8BIT, 0x0383, 0x01},
	{UNICAM_8BIT, 0x0387, 0x01},

	//-------------------------------------
	//	integration time
	//-------------------------------------
	{UNICAM_8BIT, 0x0202, 0x01}, //0x08 
	{UNICAM_8BIT, 0x0203, 0x00},



	//-------------------------------------
	//	Gain
	//-------------------------------------
	{UNICAM_8BIT, 0x0205, 0xC0},  //c0->00

	//-------------------------------------
	//	Binning
	//-------------------------------------
	{UNICAM_8BIT, 0x0900, 0x01},
	{UNICAM_8BIT, 0x0901, 0x22},
	{UNICAM_8BIT, 0x0902, 0x00},

	//-------------------------------------
	//	Crop image
	//-------------------------------------
	{UNICAM_8BIT, 0x040C, 0x05},
	{UNICAM_8BIT, 0x040D, 0x18},
	{UNICAM_8BIT, 0x040E, 0x03},
	{UNICAM_8BIT, 0x040F, 0xD0}, 

	//	analog start_end address
	//-------------------------------------
	{UNICAM_8BIT, 0x0344, 0x00},
	{UNICAM_8BIT, 0x0345, 0x00},	//x_start-0
	{UNICAM_8BIT, 0x0346, 0x00},
	{UNICAM_8BIT, 0x0347, 0x00},	//y_start-0
	{UNICAM_8BIT, 0x0348, 0x0A},
	{UNICAM_8BIT, 0x0349, 0x27},	//x_end-2599
	{UNICAM_8BIT, 0x034A, 0x07},
	{UNICAM_8BIT, 0x034B, 0x9F},	//y_end-1951

	//-------------------------------------
	//	Data output size
	//-------------------------------------
	{UNICAM_8BIT, 0x034C, 0x05},
	{UNICAM_8BIT, 0x034D, 0x08},	//x_output_size -1304
	{UNICAM_8BIT, 0x034E, 0x02},
	{UNICAM_8BIT, 0x034F, 0xD8},	//Y_output_size -976

	//-------------------------------------
	//	digital_crop_offset
	//-------------------------------------
	{UNICAM_8BIT, 0x0408, 0x00},
	{UNICAM_8BIT, 0x0409, 0x06},	//X_offset_0
	{UNICAM_8BIT, 0x040A, 0x00},
	{UNICAM_8BIT, 0x040B, 0x7C},	//Y_offset_0

	//-------------------------------------
	//	Crop image
	//-------------------------------------
	{UNICAM_8BIT, 0x040C, 0x05},
	{UNICAM_8BIT, 0x040D, 0x08},
	{UNICAM_8BIT, 0x040E, 0x02},
	{UNICAM_8BIT, 0x040F, 0xD8},

	//-------------------------------------
	//	mode select
	//-------------------------------------
	{UNICAM_8BIT, 0x0100, 0x01},
	{UNICAM_TOK_TERM, 0, 0}
};

#endif

S_UNI_REG hm5040_5M_30fps_2lanes[] = {

	//-------------------------------------
	//	CSI mode workaround -> 0c 00 ok
	//                      -> 0f 00
	//                      -> 13 33 fail
	//-------------------------------------
	{UNICAM_8BIT, 0x0111, 0x02},
	{UNICAM_8BIT, 0x0114, 0x01},	
	{UNICAM_8BIT, 0x2136, 0x0C}, //mipi timmng 19.2->19(0x13) + 0.2( 0x31/256)
	{UNICAM_8BIT, 0x2137, 0x00}, //mipi timmng
	//------------------
	// //flip+mirror
	//--------------------
	//{UNICAM_8BIT, 0x0101, 0x03}, 

	//-------------------------------------
	//	CSI data format
	//-------------------------------------
	{UNICAM_8BIT, 0x0112, 0x0A},
	{UNICAM_8BIT, 0x0113, 0x0A},	
	{UNICAM_8BIT, 0x3016, 0x46},
	{UNICAM_8BIT, 0x3017, 0x29},
	{UNICAM_8BIT, 0x3003, 0x03},
	{UNICAM_8BIT, 0x3045, 0x03},
	{UNICAM_8BIT, 0x3047, 0x98},

	//-------------------------------------
	//	mode select
	//-------------------------------------
	{UNICAM_8BIT, 0x0100, 0x00},
	//-------------------------------------
	//	PLL
	//-------------------------------------
	{UNICAM_8BIT, 0x0305, 0x02},
	{UNICAM_8BIT, 0x0306, 0x00},
	{UNICAM_8BIT, 0x0307, 0xAA},  //AB->AA
	{UNICAM_8BIT, 0x0301, 0x0A},
	{UNICAM_8BIT, 0x0309, 0x0A},

	//-------------------------------------
	//	frame length
	//-------------------------------------
	{UNICAM_8BIT, 0x0340, 0x07},
	{UNICAM_8BIT, 0x0341, 0xB6},  //C4
	{UNICAM_8BIT, 0x0342, 0x0A},
	{UNICAM_8BIT, 0x0343, 0xBE},      

	//-------------------------------------
	//	address increment for odd pixel 
	//-------------------------------------
	{UNICAM_8BIT, 0x0383, 0x01},
	{UNICAM_8BIT, 0x0387, 0x01},

	//-------------------------------------
	//	integration time
	//-------------------------------------
	{UNICAM_8BIT, 0x0202, 0x01}, //0x08 
	{UNICAM_8BIT, 0x0203, 0x00},

	//-------------------------------------
	//	Gain
	//-------------------------------------
	{UNICAM_8BIT, 0x0205, 0xC0},

	//-------------------------------------
	//	Binning
	//-------------------------------------
	{UNICAM_8BIT, 0x0900, 0x00},
	{UNICAM_8BIT, 0x0901, 0x00},
	{UNICAM_8BIT, 0x0902, 0x00},

	//-------------------------------------
	//	Crop image
	//-------------------------------------
	{UNICAM_8BIT, 0x040C, 0x0A},
	{UNICAM_8BIT, 0x040D, 0x28},
	{UNICAM_8BIT, 0x040E, 0x07},
	{UNICAM_8BIT, 0x040F, 0xA0},
	//-------------------------------------
	//	start_end address
	//-------------------------------------
	{UNICAM_8BIT, 0x0344, 0x00},
	{UNICAM_8BIT, 0x0345, 0x00},	//x_start-0
	{UNICAM_8BIT, 0x0346, 0x00},
	{UNICAM_8BIT, 0x0347, 0x00},	//y_start-0
	{UNICAM_8BIT, 0x0348, 0x0A},
	{UNICAM_8BIT, 0x0349, 0x27},	//x_end-2599
	{UNICAM_8BIT, 0x034A, 0x07},
	{UNICAM_8BIT, 0x034B, 0x9F},	//y_end-1951

	//-------------------------------------
	//	Data output size
	//-------------------------------------
	{UNICAM_8BIT, 0x034C, 0x0A},
	{UNICAM_8BIT, 0x034D, 0x28},	//x_output_size -2600
	{UNICAM_8BIT, 0x034E, 0x07},
	{UNICAM_8BIT, 0x034F, 0xA0},	//Y_output_size -1952

	//-------------------------------------
	//	digital_crop_offset
	//-------------------------------------
	{UNICAM_8BIT, 0x0408, 0x00},
	{UNICAM_8BIT, 0x0409, 0x01},	//X_offset_0
	{UNICAM_8BIT, 0x040A, 0x00},
	{UNICAM_8BIT, 0x040B, 0x00},	//Y_offset_0

	//-------------------------------------
	//	Crop image
	//-------------------------------------
	{UNICAM_8BIT, 0x040C, 0x0A},
	{UNICAM_8BIT, 0x040D, 0x28},	//X_output_size -2600
	{UNICAM_8BIT, 0x040E, 0x07},
	{UNICAM_8BIT, 0x040F, 0xA0},	//Y_output_size -1952

	//-------------------------------------
	//	mode select
	//-------------------------------------
	{UNICAM_8BIT, 0x0100, 0x01},
	{UNICAM_TOK_TERM, 0, 0}
};


static S_UNI_RESOLUTION hm5040_res_preview[] = {
    /* Add for CTS test only, begin */
    /* Add for CTS test only, end */

#ifdef HM5040_BINNING_MODE
	{
		.desc = "hm5040_1304_976_30fps",
		.res_type = UNI_DEV_RES_PREVIEW|UNI_DEV_RES_VIDEO|UNI_DEV_RES_STILL,
		.width = 1304,
		.height = 976,
		.fps = 30,
		.pixels_per_line = 2750,
		.lines_per_frame = 1454,
		.bin_factor_x = 2,
		.bin_factor_y = 2,
		.bin_mode = 1,
		.skip_frames = 0,
		.regs = hm5040_AB2_1304x976_30fps,
		.regs_n= (ARRAY_SIZE(hm5040_AB2_1304x976_30fps)),
		
		.vt_pix_clk_freq_mhz=HM5040_PCLK_BINNING,
		.crop_horizontal_start=0,
		.crop_vertical_start=0,
		.crop_horizontal_end=2599,
		.crop_vertical_end=1952,
		.output_width=1304,
		.output_height=976,
		.used = 0,

		.vts_fix=10,
		.vts=1454,
		
	},
#endif
	{
		.desc = "hm5040_5M_30fps",
		.res_type = UNI_DEV_RES_PREVIEW|UNI_DEV_RES_VIDEO|UNI_DEV_RES_STILL,
		.width = 2600,
		.height = 1952,
		.fps = 30,
		.pixels_per_line = 2750,
		.lines_per_frame = 1974,
		.bin_factor_x = 1,
		.bin_factor_y = 1,
		.bin_mode = 0,
		.skip_frames = 3,
		.regs = hm5040_5M_30fps_2lanes,
		.regs_n= (ARRAY_SIZE(hm5040_5M_30fps_2lanes)),
	
		.vt_pix_clk_freq_mhz=HM5040_PCLK,
		.crop_horizontal_start=0,
		.crop_vertical_start=0,
		.crop_horizontal_end=2600,
		.crop_vertical_end=1952,
		.output_width=2600,
		.output_height=1952,
		.used = 0,

		.vts_fix=10,
		.vts=1974,

	},
};
static S_UNI_DEVICE hm5040_unidev= {
	.desc = "hm5040",
	.version=0x1,
	.minversion=125,//update if change, as buildid
	.af_type=UNI_DEV_AF_DW9714,
	.i2c_addr=0x1B,

	.idreg=0x2016,
	.idval=0x03BB,
	.idmask=0xffff,
	.powerdelayms=1,
	.accessdelayms=20,
	.flag=0,
	.exposecoarse=0x0202,
	.exposeanaloggain=0x0204,
	.exposedigitgainR=0,
	.exposedigitgainGR=0,
	.exposedigitgainGB=0,
	.exposedigitgainB=0,
	.exposefine=0,
	.exposefine_mask=0,
	.exposevts=0x0340,
	.exposevts_mask=0xffff,
			
	.exposecoarse_mask=0xffff,
	.exposeanaloggain_mask=0x1ff,
	.exposedigitgain_mask=0,
	.exposeadgain_param=hm5040_adgain_p,
	.exposeadgain_n=(ARRAY_SIZE(hm5040_adgain_p)),
	.exposeadgain_min=0,
	.exposeadgain_max=0,
	.exposeadgain_fra=0,
	.exposeadgain_step=0,
	.exposeadgain_isp1gain=0,
	.exposeadgain_param1gain=0,
 
	
	.global_setting=hm5040_global_setting,
	.global_setting_n=(ARRAY_SIZE(hm5040_global_setting)),
	.stream_off=hm5040_stream_off,
	.stream_off_n=(ARRAY_SIZE(hm5040_stream_off)),
	.stream_on=hm5040_stream_on,
	.stream_on_n=(ARRAY_SIZE(hm5040_stream_on)),
	.group_hold_start=hm5040_group_hold_start,
	.group_hold_start_n=(ARRAY_SIZE(hm5040_group_hold_start)),
	.group_hold_end=hm5040_group_hold_end,
	.group_hold_end_n=(ARRAY_SIZE(hm5040_group_hold_end)),
	
	
	.ress=hm5040_res_preview,
	.ress_n=(ARRAY_SIZE(hm5040_res_preview)),
	.coarse_integration_time_min=1,
	.coarse_integration_time_max_margin=8,
	.fine_integration_time_min=0,
	.fine_integration_time_max_margin=0,
	.hw_port=ATOMISP_CAMERA_PORT_PRIMARY,
	.hw_lane=2,
	.hw_format=ATOMISP_INPUT_FORMAT_RAW_10,
	.hw_bayer_order=atomisp_bayer_order_rggb,
	.hw_sensor_type = RAW_CAMERA,
	
	.hw_reset_gpio=119,
	.hw_reset_gpio_polarity=0,
	.hw_reset_delayms=20,
	.hw_pwd_gpio=123,
	.hw_pwd_gpio_polarity=0,
	.hw_clksource=0,

};

#undef PPR_DEV
#define PPR_DEV hm5040_unidev
