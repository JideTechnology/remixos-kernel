#include "clk-sun8iw11.h"
/*
 * freq table from hardware, need follow rules
 * 1)   each table  named as
 *      factor_pll1_tbl
 *      factor_pll2_tbl
 *      ...
 * 2) for each table line
 *      a) follow the format PLLx(n,k,m,p,d1,d2,freq), and keep the factors order
 *      b) if any factor not used, skip it
 *      c) the factor is the value to write registers, not means factor+1
 *
 *      example
 *      PLL1(9, 0, 0, 2, 60000000) means PLL1(n,k,m,p,freq)
 *      PLLVIDEO0(3, 0, 96000000)       means PLLVIDEO0(n,m,freq)
 *
 */

//PLLCPU(n,k,m,p,freq)	F_N8X5_K4X2_M0X2_P16x2
struct sunxi_clk_factor_freq factor_pllcpu_tbl[] = {
PLLCPU(    9  ,    0  ,    0  ,    2  ,  0,    60000000U),
PLLCPU(    25 ,    2  ,    0  ,    0  ,  0,  1872000000U),

};
//PLLVIDEO0(n,m,freq)	F_N8X7_M0X4
struct sunxi_clk_factor_freq factor_pllvideo0_tbl[] = {
PLLVIDEO0(    6  ,    0  ,     168000000U),
PLLVIDEO0(    41 ,    0  ,    1008000000U),

};
//PLLVE(n,m,freq)		F_N8X7_M0X4
struct sunxi_clk_factor_freq factor_pllve_tbl[] = {
PLLVE(    11 ,    1  ,     144000000U),
PLLVE(    41 ,    0  ,    1008000000U),

};
//PLLDDR0(n,k,m,freq)	F_N8X5_K4X2_M0X2
struct sunxi_clk_factor_freq factor_pllddr0_tbl[] = {
PLLDDR0(    4  ,    1  ,    3  ,      60000000U),
PLLDDR0(    29 ,    2  ,    0  ,    2160000000U),

};
//PLLPERIPH0(n,k,freq)	F_N8X5_K4X2
struct sunxi_clk_factor_freq factor_pllperiph0_tbl[] = {
PLLPERIPH0(    7  ,    0  ,    0,  96000000U),
PLLPERIPH0(    29 ,    2  ,    0,  1080000000U),

};
//PLLPERIPH1(n,k,freq)	F_N8X5_K4X2
struct sunxi_clk_factor_freq factor_pllperiph1_tbl[] = {
PLLPERIPH1(    6  ,    0  ,      0,     84000000U),
PLLPERIPH1(    28 ,    2  ,      0,   1044000000U),
};
//PLLVIDEO1(n,m,freq)	F_N8X7_M0X4
struct sunxi_clk_factor_freq factor_pllvideo1_tbl[] = {
PLLVIDEO1(    5  ,    0  ,     144000000U),
PLLVIDEO1(    41 ,    0  ,    1008000000U),
};
//PLLGPU(n,m,freq) 		F_N8X7_M0X4
struct sunxi_clk_factor_freq factor_pllgpu_tbl[] = {
PLLGPU(    5  ,    0  ,     144000000U),
PLLGPU(    41 ,    0  ,    1008000000U),
};
//PLLSATA(n,k,m,freq)		F_N8X7_K4x2_M0X4
struct sunxi_clk_factor_freq factor_pllsata_tbl[] = {
PLLSATA(    1  ,    0  ,     3  ,    8000000U),
PLLSATA(    24 ,    2  ,     0  ,    300000000U),
};
//PLLDE(n,m,freq)		F_N8X7_M0X4
struct sunxi_clk_factor_freq factor_pllde_tbl[] = {
PLLDE(    4  ,    0  ,     120000000U),
PLLDE(    41 ,    0  ,    1008000000U),
};
//PLLDDR1(n,m,freq)		F_N8X7_M0X2
struct sunxi_clk_factor_freq factor_pllddr1_tbl[] = {
PLLDDR1(    9  ,    3  ,      60000000U),
PLLDDR1(    127,    0  ,    3072000000U),
};


static unsigned int pllcpu_max,pllvideo0_max,pllve_max,pllddr0_max ,
					pllperiph0_max,pllperiph1_max ,pllvideo1_max,
					pllgpu_max,pllsata_max,pllde_max,pllddr1_max;

#define PLL_MAX_ASSIGN(name)	pll##name##_max=factor_pll##name##_tbl[ARRAY_SIZE(factor_pll##name##_tbl)-1].freq

void sunxi_clk_factor_initlimits(void)
{
	PLL_MAX_ASSIGN(cpu);PLL_MAX_ASSIGN(video0);PLL_MAX_ASSIGN(ve);PLL_MAX_ASSIGN(ddr0);
	PLL_MAX_ASSIGN(periph0);PLL_MAX_ASSIGN(periph1);PLL_MAX_ASSIGN(video1);
	PLL_MAX_ASSIGN(gpu);PLL_MAX_ASSIGN(sata);PLL_MAX_ASSIGN(de);PLL_MAX_ASSIGN(ddr1);
}
