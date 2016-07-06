#ifndef __UNICAM_EXT_H__
#define __UNICAM_EXT_H__

#define UNICAM_FOCAL_LENGTH_NUM	278	/*2.78mm*/
#define UNICAM_FOCAL_LENGTH_DEM	100
#define UNICAM_F_NUMBER_DEFAULT_NUM	26
#define UNICAM_F_NUMBER_DEM	10

#define UNICAM_MAX_FOCUS_POS	1023
#define UNICAM_MAX_FOCUS_NEG	(-1023)

/*
 * focal length bits definition:
 * bits 31-16: numerator, bits 15-0: denominator
 */
#define UNICAM_FOCAL_LENGTH_DEFAULT 0x1160064

/*
 * current f-number bits definition:
 * bits 31-16: numerator, bits 15-0: denominator
 */
#define UNICAM_F_NUMBER_DEFAULT 0x1a000a

/*
 * f-number range bits definition:
 * bits 31-24: max f-number numerator
 * bits 23-16: max f-number denominator
 * bits 15-8: min f-number numerator
 * bits 7-0: min f-number denominator
 */
#define UNICAM_F_NUMBER_RANGE 0x1a0a1a0a

#define UNICAM_BIN_FACTOR_MAX 2

typedef enum _E_UNI_TOKE_TYPE_ {
	UNICAM_8BIT  = 0x0001,
	UNICAM_16BIT = 0x0002,
	UNICAM_32BIT = 0x0004,
	UNICAM_TOK_DELAY  = 0xf000,
	UNICAM_TOK_TERM   = 0xf001,
	UNICAM_TOK_MASK = 0xfff0,
}E_UNI_TOKE_TYPE;

enum _E_UNI_DATA_TYPE_ {
	UNICAM_8BIT_ADDR  = 1,
	UNICAM_16BIT_ADDR,
	UNICAM_24BIT_ADDR,
	UNICAM_32BIT_ADDR,
};

typedef struct _S_UNI_REG_ {
	E_UNI_TOKE_TYPE type;
	UINT16 reg;
	UINT32 val;//in delay for ms
}S_UNI_REG;

typedef enum _E_UNI_DEV_RES_TYPE_ {
	UNI_DEV_RES_DEFAULT=0x0001,////0 mean na, preview->default->first(no 0)
	UNI_DEV_RES_PREVIEW=0x0002,
	UNI_DEV_RES_VIDEO=0x0004,
	UNI_DEV_RES_STILL=0x0008,
	UNI_DEV_RES_AONLY=0x0010,//android only
	UNI_DEV_RES_WONLY=0x0020,//window only
}E_UNI_DEV_RES_TYPE;

#define S_UNI_RESOLUTION_RES_NAME_LEN 30
typedef struct _S_UNI_RESOLUTION_ {
	UINT8  desc[S_UNI_RESOLUTION_RES_NAME_LEN];
	INT32 res_type;
	INT32 width;
	INT32 height;
	INT32 fps;
	UINT16 pixels_per_line;
	UINT16 lines_per_frame;
	UINT8 bin_factor_x;
	UINT8 bin_factor_y;
	UINT8 bin_mode;
	UINT32 skip_frames;
	S_UNI_REG *regs;
	INT32 regs_n;// no use?

	INT32 vt_pix_clk_freq_mhz;
	INT32 crop_horizontal_start;
	INT32 crop_vertical_start;
	INT32 crop_horizontal_end;
	INT32 crop_vertical_end;
	INT32 output_width;
	INT32 output_height;
	bool used;

	INT32 vts_fix;
	INT32 vts;

	INT32	unused[5]; //for extend
}S_UNI_RESOLUTION;

typedef enum _E_UNI_DEV_FLAG_ { //mask
	UNI_DEV_FLAG_1B_ACCESS=0x0001, //default is 2B
	UNI_DEV_FLAG_LE=0x0002, //default is BE
	UNI_DEV_FLAG_I2C_DELAY_100NS=0x0004,//default no delay, 
	UNI_DEV_FLAG_USEADGAIN=0x0008,
	UNI_DEV_FLAG_1B_ADDR=0x0010, //default is 2B
	UNI_DEV_FLAG_DBGI2C=0x0020, //default is not show i2c access
}E_UNI_DEV_FLAG;

typedef enum _E_UNI_DEV_AF_TYPE_ {
	UNI_DEV_AF_NONE=0x0000,
	UNI_DEV_AF_DW9714=0x0001,
	UNI_DEV_AF_AD5823=0x0002,
	UNI_DEV_AF_VM149C=0x0003,
	UNI_DEV_AF_AD5823_APTINA=0x0004,
	UNI_DEV_AF_BU64243=0x0005,
}E_UNI_DEV_AF_TYPE;

#define S_UNI_DEVICE_VER1   0x01
#define S_UNI_DEVICE_DESC_LEN 100 //build data + commitid
typedef struct _S_UNI_DEVICE_ {
	INT32 version;
	INT32 minversion;
	UINT8 desc[S_UNI_DEVICE_DESC_LEN];
	INT32 af_type;
	INT32 i2c_addr;
	INT32 idreg;
	INT32 idval;
	INT32 idmask;
	INT32 powerdelayms;
	INT32 accessdelayms;

	INT32 flag;
	INT32 exposecoarse;
	INT32 exposeanaloggain;
	INT32 exposedigitgainR;
	INT32 exposedigitgainGR;
	INT32 exposedigitgainGB;
	INT32 exposedigitgainB;

	INT32 exposefine;
	INT32 exposefine_mask;
	INT32 exposevts;
	INT32 exposevts_mask;

	INT32 exposecoarse_mask;
	INT32 exposeanaloggain_mask;
	INT32 exposedigitgain_mask;

	//INT32 exposeadgain;//digital gain with a gain , when use, no digital gain at all
	//INT32 exposeadgain_mask;
	INT32 exposeadgain_n;
	INT32 *exposeadgain_param;

	INT32 exposeadgain_min;
	INT32 exposeadgain_max;
	INT32 exposeadgain_fra;
	INT32 exposeadgain_step;
	INT32 exposeadgain_isp1gain;
	INT32 exposeadgain_param1gain;

	S_UNI_REG * global_setting;
	INT32 global_setting_n;
	S_UNI_REG * stream_off;
	INT32 stream_off_n;
	S_UNI_REG * stream_on;
	INT32 stream_on_n;
	S_UNI_REG * group_hold_start;
	INT32 group_hold_start_n;
	S_UNI_REG * group_hold_end;
	INT32 group_hold_end_n;
	S_UNI_RESOLUTION * ress;
	INT32 ress_n;
	INT32 coarse_integration_time_min;
	INT32 coarse_integration_time_max_margin;
	INT32 fine_integration_time_min;
	INT32 fine_integration_time_max_margin;

	//hardware config
	INT32 hw_port;
	INT32 hw_lane;
	INT32 hw_format;
	INT32 hw_bayer_order;
	INT32 hw_sensor_type;

	INT32 hw_reset_gpio;
	INT32 hw_reset_gpio_polarity;
	INT32 hw_reset_delayms;
	INT32 hw_pwd_gpio;
	INT32 hw_pwd_gpio_polarity;
	INT32 hw_clksource;

	INT32	unused[10];//last for expand

}S_UNI_DEVICE;

#endif
