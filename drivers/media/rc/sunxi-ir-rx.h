#ifndef SUNXI_IR_RX_H
#define SUNXI_IR_RX_H

#define CONFIG_FPGA_V4_PLATFORM
/* Registers */
#define IR_REG(x)        (x)

#define IR_CTRL_REG      IR_REG(0x00)     /* IR Control */
#define IR_RXCFG_REG     IR_REG(0x10)     /* Rx Config */
#define IR_RXDAT_REG     IR_REG(0x20)     /* Rx Data */
#define IR_RXINTE_REG    IR_REG(0x2C)     /* Rx Interrupt Enable */
#define IR_RXINTS_REG    IR_REG(0x30)     /* Rx Interrupt Status */
#define IR_SPLCFG_REG    IR_REG(0x34)     /* IR Sample Config */

#define IR_FIFO_SIZE     (64)             /* 64Bytes */

#if (defined CONFIG_FPGA_V4_PLATFORM) || (defined CONFIG_FPGA_V7_PLATFORM)
#define IR_SIMPLE_UNIT	 (21333)	  /* simple in ns */
#define IR_CLK		 (24000000)	  /* fpga clk output is fixed */
#define IR_CIR_MODE      (0x3<<4)         /* CIR mode enable */
#define IR_ENTIRE_ENABLE (0x3<<0)         /* IR entire enable */
#define IR_SAMPLE_DEV    (0x3<<0)         /* 24MHz/512 =46875Hz (21333ns) */
#define IR_FIFO_32       (((IR_FIFO_SIZE>>1)-1)<<8)
#define IR_IRQ_STATUS    ((0x1<<4)|0x3)
#else
#define IR_SIMPLE_UNIT	 (64000)	  /* simple in ns */
#define IR_CLK		 (4000000)
#define IR_CIR_MODE      (0x3<<4)         /* CIR mode enable */
#define IR_ENTIRE_ENABLE (0x3<<0)         /* IR entire enable */
#define IR_SAMPLE_DEV    (0x2<<0)         /* 4MHz/256 =15625Hz (64000ns) */
#define IR_FIFO_32       (((IR_FIFO_SIZE>>1)-1)<<8)
#define IR_IRQ_STATUS    ((0x1<<4)|0x3)
#endif

//Bit Definition of IR_RXINTS_REG Register
#define IR_RXINTS_RXOF   (0x1<<0)         /* Rx FIFO Overflow */
#define IR_RXINTS_RXPE   (0x1<<1)         /* Rx Packet End */
#define IR_RXINTS_RXDA   (0x1<<4)         /* Rx FIFO Data Available */

#if (defined CONFIG_FPGA_V4_PLATFORM) || (defined CONFIG_FPGA_V7_PLATFORM)
#define IR_RXFILT_VAL    (((16)&0x3f)<<2)  /* Filter Threshold = 16*21.3 = ~341us	< 500us */
#define IR_RXIDLE_VAL    (((5)&0xff)<<8)  /* Idle Threshold = (5+1)*128*21.3 = ~16.4ms > 9ms */
#define IR_ACTIVE_T      ((0&0xff)<<16)   /* Active Threshold */
#define IR_ACTIVE_T_C    ((1&0xff)<<23)   /* Active Threshold */
#else
#define IR_RXFILT_VAL    (((6)&0x3f)<<2)  /* Filter Threshold = 6*64 = ~384us	< 500us */
//#define IR_RXIDLE_VAL    (((1)&0xff)<<8)  /* Idle Threshold = (1+1)*128*64 = ~16.4ms > 9ms */
#define IR_RXIDLE_VAL    (((2)&0xff)<<8)  /* Idle Threshold = (1+1)*128*64 = ~16.4ms > 9ms */
#define IR_ACTIVE_T      ((99&0xff)<<16)   /* Active Threshold */
#define IR_ACTIVE_T_C    ((0&0xff)<<23)   /* Active Threshold */
#endif

#define IR_L1_MIN        (80)             /* 80*42.7 = ~3.4ms, Lead1(4.5ms) > IR_L1_MIN */
#define IR_L0_MIN        (40)             /* 40*42.7 = ~1.7ms, Lead0(4.5ms) Lead0R(2.25ms)> IR_L0_MIN */
#define IR_PMAX          (26)             /* 26*42.7 = ~1109us ~= 561*2, Pluse < IR_PMAX */
#define IR_DMID          (26)             /* 26*42.7 = ~1109us ~= 561*2, D1 > IR_DMID, D0 =< IR_DMID */
#define IR_DMAX          (53)             /* 53*42.7 = ~2263us ~= 561*4, D < IR_DMAX */

#define IR_ERROR_CODE    (0xffffffff)
#define IR_REPEAT_CODE   (0x00000000)
#define DRV_VERSION      "1.00"

#define RC_MAP_SUNXI "rc_map_sunxi"

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_INT = 1U << 1,
	DEBUG_DATA_INFO = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
	DEBUG_ERR = 1U << 4,
};

enum ir_mode {
	CIR_MODE_ENABLE,
	IR_MODULE_ENABLE,
};
enum ir_sample_config {
	IR_SAMPLE_REG_CLEAR,
	IR_CLK_SAMPLE,
	IR_FILTER_TH,
	IR_IDLE_TH,
	IR_ACTIVE_TH,
};
enum ir_irq_config {
	IR_IRQ_STATUS_CLEAR,
	IR_IRQ_ENABLE,
	IR_IRQ_FIFO_SIZE,
};

struct sunxi_ir_data{
	struct platform_device	*pdev;
	struct clk *mclk;
	struct clk *pclk;
	struct rc_dev *rcdev;
	void __iomem 	*reg_base;
	int	irq_num;
};

int init_rc_map_sunxi(void);
void exit_rc_map_sunxi(void);

#endif /* SUNXI_IR_RX_H */
