/* ========================================================================== */
/*                                                                            */
/*   O2_EC_ADAPTER.h                                                         */
/*   (c) 2001 Author                                                          */
/*                                                                            */
/*   Description                                                              */
/* This program is free software; you can redistribute it and/or modify it    */
/* under the terms of the GNU General Public License version 2 as published   */
/* by the Free Software Foundation.											  */
/*																			  */
/* This program is distributed in the hope that it will be useful, but        */
/* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY */
/* or FITNESS FOR A PARTICULAR PURPOSE.										  */
/* See the GNU General Public License for more details.						  */
/*																			  */
/* You should have received a copy of the GNU General Public License along	  */
/* with this program.  If not, see <http://www.gnu.org/licenses/>.			  */
/* ========================================================================== */

#ifndef	__O2_MTK_ADAPTER_H
#define	__O2_MTK_ADAPTER_H

#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/power_supply.h>






#define 	ADAPTER_5V          	5000
#define 	ADAPTER_7V          	7000
#define 	ADAPTER_9V          	9000
#define 	ADAPTER_12V          	12000



extern uint8_t mtk_adapter;

/////////////////////////////////////////////////////////////////////////////////////


 void o2_adapter_mtk_init(void);
bool o2_set_mtk_voltage(int voltage);


#endif
