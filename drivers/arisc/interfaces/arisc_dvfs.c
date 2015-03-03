/*
 *  drivers/arisc/interfaces/arisc_dvfs.c
 *
 * Copyright (c) 2012 Allwinner.
 * 2012-05-01 Written by sunny (sunny@allwinnertech.com).
 * 2012-10-01 Written by superm (superm@allwinnertech.com).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "../arisc_i.h"

static struct arisc_freq_voltage arisc_vf_table[ARISC_DVFS_VF_TABLE_MAX] =
{
	//freq          //voltage   //axi_div
	{900000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (600Mhz, 1008Mhz]
	{600000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (420Mhz, 600Mhz]
	{420000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (360Mhz, 420Mhz]
	{360000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (300Mhz, 360Mhz]
	{300000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (240Mhz, 300Mhz]
	{240000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (120Mhz, 240Mhz]
	{120000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (60Mhz,  120Mhz]
	{60000000,      1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (0Mhz,   60Mhz]
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
};

int arisc_dvfs_cfg_vf_table(void)
{
	u32    value = 0;
	int    index = 0;
	int    vf_table_size = 0;
	char   vf_table_main_key[64];
	char   vf_table_sub_key[64];
	int    vf_table_count = 0;
	int    vf_table_type = 0;

	arisc_cfg.dvfs.np = of_find_compatible_node(NULL, NULL, "allwinner,dvfs_table");
	if (IS_ERR(arisc_cfg.dvfs.np)) {
		ARISC_ERR("get [allwinner,dvfs_table] device node error\n");
		return -EINVAL;
	}

	if (of_property_read_u32(arisc_cfg.dvfs.np, "vf_table_count", &vf_table_count)) {
		ARISC_LOG("%s: support only one vf_table\n", __func__);
		sprintf(vf_table_main_key, "%s", "allwinner,dvfs_table");
	} else {
		//vf_table_type = sunxi_get_soc_bin();
		sprintf(vf_table_main_key, "%s%d", "allwinner,vf_table", vf_table_type);
	}
	ARISC_INF("%s: vf table type [%d=%s]\n", __func__, vf_table_type, vf_table_main_key);

	arisc_cfg.dvfs.np = of_find_compatible_node(NULL, NULL, vf_table_main_key);
	if (IS_ERR(arisc_cfg.dvfs.np)) {
		ARISC_ERR("get [%s] device node error\n", vf_table_main_key);
		return -EINVAL;
	}

	/* parse system config v-f table information */
	if (of_property_read_u32(arisc_cfg.dvfs.np, "lv_count", &vf_table_size)) {
		ARISC_ERR("parse system config dvfs_table size fail\n");
		return -EINVAL;
	}
	for (index = 0; index < vf_table_size; index++) {
		sprintf(vf_table_sub_key, "lv%d_freq", index + 1);
		if (of_property_read_u32(arisc_cfg.dvfs.np, vf_table_sub_key, &value) == 0) {
			arisc_vf_table[index].freq = value;
		}
		ARISC_INF("%s: freq [%s-%d=%d]\n", __func__, vf_table_sub_key, index, value);
		sprintf(vf_table_sub_key, "lv%d_volt", index + 1);
		if (of_property_read_u32(arisc_cfg.dvfs.np, vf_table_sub_key, &value) == 0) {
			arisc_vf_table[index].voltage = value;
		}
		ARISC_INF("%s: volt [%s-%d=%d]\n", __func__, vf_table_sub_key, index, value);
	}

	arisc_cfg.dvfs.vf = arisc_vf_table;

	return 0;
}

/*
 * set specific pll target frequency.
 * @freq:    target frequency to be set, based on KHZ;
 * @pll:     which pll will be set
 * @mode:    the attribute of message, whether syn or asyn;
 * @cb:      callback handler;
 * @cb_arg:  callback handler arguments;
 *
 * return: result, 0 - set frequency successed,
 *                !0 - set frequency failed;
 */
int arisc_dvfs_set_cpufreq(unsigned int freq, unsigned int pll, unsigned long mode, arisc_cb_t cb, void *cb_arg)
{
	unsigned int          msg_attr = 0;
	struct arisc_message *pmessage;
	int                   result = 0;

	if (mode & ARISC_DVFS_SYN) {
		msg_attr |= ARISC_MESSAGE_ATTR_HARDSYN;
	}

	/* allocate a message frame */
	pmessage = arisc_message_allocate(msg_attr);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message
	 *
	 * |paras[0]|paras[1]|
	 * |freq    |pll     |
	 */
	pmessage->type       = ARISC_CPUX_DVFS_REQ;
	pmessage->paras[0]   = freq;
	pmessage->paras[1]   = pll;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = cb;
	pmessage->cb.arg     = cb_arg;

	ARISC_INF("arisc dvfs request : %d\n", freq);
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	/* dvfs mode : syn or not */
	if (mode & ARISC_DVFS_SYN) {
		result = pmessage->result;
		arisc_message_free(pmessage);
	}

	return result;
}
EXPORT_SYMBOL(arisc_dvfs_set_cpufreq);
