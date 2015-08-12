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
PLLCPU(    9  ,    0  ,    0  ,    2  ,      60000000U),
PLLCPU(    10 ,    0  ,    0  ,    2  ,      66000000U),
PLLCPU(    11 ,    0  ,    0  ,    2  ,      72000000U),
PLLCPU(    12 ,    0  ,    0  ,    2  ,      78000000U),
PLLCPU(    13 ,    0  ,    0  ,    2  ,      84000000U),
PLLCPU(    14 ,    0  ,    0  ,    2  ,      90000000U),
PLLCPU(    15 ,    0  ,    0  ,    2  ,      96000000U),
PLLCPU(    16 ,    0  ,    0  ,    2  ,     102000000U),
PLLCPU(    17 ,    0  ,    0  ,    2  ,     108000000U),
PLLCPU(    18 ,    0  ,    0  ,    2  ,     114000000U),
PLLCPU(    9  ,    0  ,    0  ,    1  ,     120000000U),
PLLCPU(    10 ,    0  ,    0  ,    1  ,     132000000U),
PLLCPU(    11 ,    0  ,    0  ,    1  ,     144000000U),
PLLCPU(    12 ,    0  ,    0  ,    1  ,     156000000U),
PLLCPU(    13 ,    0  ,    0  ,    1  ,     168000000U),
PLLCPU(    14 ,    0  ,    0  ,    1  ,     180000000U),
PLLCPU(    15 ,    0  ,    0  ,    1  ,     192000000U),
PLLCPU(    16 ,    0  ,    0  ,    1  ,     204000000U),
PLLCPU(    17 ,    0  ,    0  ,    1  ,     216000000U),
PLLCPU(    18 ,    0  ,    0  ,    1  ,     228000000U),
PLLCPU(    9  ,    0  ,    0  ,    0  ,     240000000U),
PLLCPU(    10 ,    0  ,    0  ,    0  ,     264000000U),
PLLCPU(    11 ,    0  ,    0  ,    0  ,     288000000U),
PLLCPU(    12 ,    0  ,    0  ,    0  ,     312000000U),
PLLCPU(    13 ,    0  ,    0  ,    0  ,     336000000U),
PLLCPU(    14 ,    0  ,    0  ,    0  ,     360000000U),
PLLCPU(    15 ,    0  ,    0  ,    0  ,     384000000U),
PLLCPU(    16 ,    0  ,    0  ,    0  ,     408000000U),
PLLCPU(    17 ,    0  ,    0  ,    0  ,     432000000U),
PLLCPU(    18 ,    0  ,    0  ,    0  ,     456000000U),
PLLCPU(    19 ,    0  ,    0  ,    0  ,     480000000U),
PLLCPU(    20 ,    0  ,    0  ,    0  ,     504000000U),
PLLCPU(    21 ,    0  ,    0  ,    0  ,     528000000U),
PLLCPU(    22 ,    0  ,    0  ,    0  ,     552000000U),
PLLCPU(    23 ,    0  ,    0  ,    0  ,     576000000U),
PLLCPU(    24 ,    0  ,    0  ,    0  ,     600000000U),
PLLCPU(    25 ,    0  ,    0  ,    0  ,     624000000U),
PLLCPU(    26 ,    0  ,    0  ,    0  ,     648000000U),
PLLCPU(    27 ,    0  ,    0  ,    0  ,     672000000U),
PLLCPU(    28 ,    0  ,    0  ,    0  ,     696000000U),
PLLCPU(    29 ,    0  ,    0  ,    0  ,     720000000U),
PLLCPU(    15 ,    1  ,    0  ,    0  ,     768000000U),
PLLCPU(    10 ,    2  ,    0  ,    0  ,     792000000U),
PLLCPU(    16 ,    1  ,    0  ,    0  ,     816000000U),
PLLCPU(    17 ,    1  ,    0  ,    0  ,     864000000U),
PLLCPU(    18 ,    1  ,    0  ,    0  ,     912000000U),
PLLCPU(    12 ,    2  ,    0  ,    0  ,     936000000U),
PLLCPU(    19 ,    1  ,    0  ,    0  ,     960000000U),
PLLCPU(    20 ,    1  ,    0  ,    0  ,    1008000000U),
PLLCPU(    21 ,    1  ,    0  ,    0  ,    1056000000U),
PLLCPU(    14 ,    2  ,    0  ,    0  ,    1080000000U),
PLLCPU(    22 ,    1  ,    0  ,    0  ,    1104000000U),
PLLCPU(    23 ,    1  ,    0  ,    0  ,    1152000000U),
PLLCPU(    24 ,    1  ,    0  ,    0  ,    1200000000U),
PLLCPU(    16 ,    2  ,    0  ,    0  ,    1224000000U),
PLLCPU(    25 ,    1  ,    0  ,    0  ,    1248000000U),
PLLCPU(    26 ,    1  ,    0  ,    0  ,    1296000000U),
PLLCPU(    27 ,    1  ,    0  ,    0  ,    1344000000U),
PLLCPU(    18 ,    2  ,    0  ,    0  ,    1368000000U),
PLLCPU(    19 ,    2  ,    0  ,    0  ,    1440000000U),
PLLCPU(    20 ,    2  ,    0  ,    0  ,    1512000000U),
PLLCPU(    15 ,    3  ,    0  ,    0  ,    1536000000U),
PLLCPU(    21 ,    2  ,    0  ,    0  ,    1584000000U),
PLLCPU(    16 ,    3  ,    0  ,    0  ,    1632000000U),
PLLCPU(    22 ,    2  ,    0  ,    0  ,    1656000000U),
PLLCPU(    23 ,    2  ,    0  ,    0  ,    1728000000U),
PLLCPU(    24 ,    2  ,    0  ,    0  ,    1800000000U),
PLLCPU(    25 ,    2  ,    0  ,    0  ,    1872000000U),

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
