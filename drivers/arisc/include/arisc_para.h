#ifndef __ARISC_PARA_H__
#define __ARISC_PARA_H__

#define ARISC_MACHINE_PAD    0
#define ARISC_MACHINE_HOMLET 1

/* arisc parameter size: 128byte */
/*
 * machine: machine id, pad = 0, homlet = 1;
 * message_pool_phys: message pool physical address;
 * message_pool_size: message pool size;
 */
#define SERVICES_DVFS_USED (1<<0)

typedef struct arisc_freq_voltage
{
	u32 freq;       //cpu frequency
	u32 voltage;    //voltage for the frequency
	u32 axi_div;    //the divide ratio of axi bus
} arisc_freq_voltage_t;

typedef struct dram_para
{
	//normal configuration
	u32 dram_clk;
	u32 dram_type; //dram_type DDR2: 2 DDR3: 3 LPDDR2: 6 DDR3L: 31
	u32 dram_zq;
	u32 dram_odt_en;

	//control configuration
	u32 dram_para1;
	u32 dram_para2;

	//timing configuration
	u32 dram_mr0;
	u32 dram_mr1;
	u32 dram_mr2;
	u32 dram_mr3;
	u32 dram_tpr0;
	u32 dram_tpr1;
	u32 dram_tpr2;
	u32 dram_tpr3;
	u32 dram_tpr4;
	u32 dram_tpr5;
	u32 dram_tpr6;

	//reserved for future use
	u32 dram_tpr7;
	u32 dram_tpr8;
	u32 dram_tpr9;
	u32 dram_tpr10;
	u32 dram_tpr11;
	u32 dram_tpr12;
	u32 dram_tpr13;

}dram_para_t;

/* arisc clocksoure */
typedef enum arisc_clksrc
{
	CLKSRC_LOSC      = 0,
	CLKSRC_IOSC      = 1,
	CLKSRC_HOSC      = 2,
	CLKSRC_PERIPH0   = 3,
	CLKSRC_ARISC_MAX = 4,
} arisc_clksrc_e;

/* ddr clocksoure */
typedef enum ddr_clksrc
{
	CLKSRC_DDR0    = 0,
	CLKSRC_DDR1    = 1,
	CLKSRC_DDR_MAX = 2,
} ddr_clksrc_e;

typedef struct core_cfg
{
	struct device_node *np;
	struct clk *losc;
	struct clk *iosc;
	struct clk *hosc;
	struct clk *pllperiph0;
	u32 powchk_used;
	u32 power_reg;
	u32 system_power;
} core_cfg_t;

typedef struct mem_cfg
{
	struct device_node *np;
	void __iomem *vbase;
	phys_addr_t pbase;
	size_t size;
} mem_cfg_t;

typedef struct space_cfg
{
	struct device_node *np;
	phys_addr_t sram_dst;
	phys_addr_t sram_offset;
	size_t sram_size;
	phys_addr_t dram_dst;
	phys_addr_t dram_offset;
	size_t dram_size;
	phys_addr_t para_dst;
	phys_addr_t para_offset;
	size_t para_size;
	phys_addr_t msgpool_dst;
	phys_addr_t msgpool_offset;
	size_t msgpool_size;
} space_cfg_t;

typedef struct dev_cfg
{
	struct device_node *np;
	struct clk *clk[ARISC_DEV_CLKSRC_NUM];
	void __iomem *vbase;
	phys_addr_t pbase;
	size_t size;
	u32 irq;
	int status;
} dev_cfg_t;

typedef struct dvfs_cfg
{
	struct device_node *np;
	struct arisc_freq_voltage *vf;
} dvfs_cfg_t;

typedef struct cir_cfg
{
	struct device_node *np;
	struct clk *clk[ARISC_DEV_CLKSRC_NUM];
	void __iomem *vbase;
	phys_addr_t pbase;
	size_t size;
	u32 irq;
	u32 power_key_code;
	u32 addr_code;
	int status;
} cir_cfg_t;

typedef struct pmu_cfg
{
	struct device_node *np;
	u32 pmu_bat_shutdown_ltf;
	u32 pmu_bat_shutdown_htf;
} pmu_cfg_t;

typedef struct dram_cfg
{
	struct device_node *np;
	struct clk *pllddr0;
	struct clk *pllddr1;
	struct dram_para para;
} dram_cfg_t;

typedef struct arisc_cfg
{
	struct core_cfg core;
	struct mem_cfg srama2;
	struct dram_cfg dram;
	struct space_cfg space;
	struct mem_cfg prcm;
	struct mem_cfg cpuscfg;
	struct dev_cfg msgbox;
	struct cir_cfg scir;
	struct dev_cfg suart;
	struct dev_cfg srsb;
	struct dev_cfg sjtag;
	struct dvfs_cfg dvfs;
	struct pmu_cfg pmu;
} arisc_cfg_t;

typedef struct arisc_para
{
	u32 message_pool_phys;
	u32 message_pool_size;
	u32 power_key_code;
	u32 addr_code;
	u32 suart_status;
	u32 pmu_bat_shutdown_ltf;
	u32 pmu_bat_shutdown_htf;
	struct dram_para dram_para;
	struct arisc_freq_voltage vf[ARISC_DVFS_VF_TABLE_MAX];
} arisc_para_t;

#define ARISC_PARA_SIZE (sizeof(struct arisc_para))

#endif /* __ARISC_PARA_H__ */
