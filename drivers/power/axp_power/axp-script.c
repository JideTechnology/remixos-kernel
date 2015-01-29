#include <linux/module.h>
#include <linux/sys_config.h>
#include "axp-cfg.h"

static s32 axp_script_parser_fetch(char *main, char *sub, u32 *val, u32 size)
{
	script_item_u script_val;
	script_item_value_type_e type;

	type = script_get_item(main, sub, &script_val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		printk(KERN_INFO "%s: get %s err.\n", __func__, sub);
		return -1;
	}
	*val = script_val.val;
	return 0;
}

s32 axp_fetch_sysconfig_para(char * pmu_type, struct axp_config_info *axp_config)
{
	s32 ret;

	ret = axp_script_parser_fetch(pmu_type, "used", &axp_config->pmu_used, sizeof(u32));
	if (ret)
		return -1;
	if (axp_config->pmu_used) {
		ret = axp_script_parser_fetch(pmu_type, "pmu_twi_id", &axp_config->pmu_twi_id, sizeof(u32));
		if (ret)
			axp_config->pmu_twi_id = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_twi_addr", &axp_config->pmu_twi_addr, sizeof(u32));
		if (ret)
			axp_config->pmu_twi_addr = 34;

		ret = axp_script_parser_fetch(pmu_type, "pmu_irq_id", &axp_config->pmu_irq_id, sizeof(u32));
		if (ret)
			axp_config->pmu_irq_id = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_battery_rdc", &axp_config->pmu_battery_rdc, sizeof(u32));
		if (ret)
			axp_config->pmu_battery_rdc = BATRDC;

		ret = axp_script_parser_fetch(pmu_type, "pmu_battery_cap", &axp_config->pmu_battery_cap, sizeof(u32));
		if (ret)
			axp_config->pmu_battery_cap = 4000;

		ret = axp_script_parser_fetch(pmu_type, "pmu_batdeten", &axp_config->pmu_batdeten, sizeof(u32));
		if (ret)
			axp_config->pmu_batdeten = 1;

		ret = axp_script_parser_fetch(pmu_type, "pmu_chg_ic_temp", &axp_config->pmu_chg_ic_temp, sizeof(u32));
		if (ret)
			axp_config->pmu_chg_ic_temp = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_runtime_chgcur", &axp_config->pmu_runtime_chgcur, sizeof(u32));
		if (ret)
			axp_config->pmu_runtime_chgcur = INTCHGCUR / 1000;
		axp_config->pmu_runtime_chgcur = axp_config->pmu_runtime_chgcur * 1000;

		ret = axp_script_parser_fetch(pmu_type, "pmu_suspend_chgcur", &axp_config->pmu_suspend_chgcur, sizeof(u32));
		if (ret)
			axp_config->pmu_suspend_chgcur = 1200;
		axp_config->pmu_suspend_chgcur = axp_config->pmu_suspend_chgcur * 1000;

		ret = axp_script_parser_fetch(pmu_type, "pmu_shutdown_chgcur", &axp_config->pmu_shutdown_chgcur, sizeof(u32));
		if (ret)
			axp_config->pmu_shutdown_chgcur = 1200;
		axp_config->pmu_shutdown_chgcur = axp_config->pmu_shutdown_chgcur *1000;

		ret = axp_script_parser_fetch(pmu_type, "pmu_init_chgvol", &axp_config->pmu_init_chgvol, sizeof(u32));
		if (ret)
			axp_config->pmu_init_chgvol = INTCHGVOL / 1000;
		axp_config->pmu_init_chgvol = axp_config->pmu_init_chgvol * 1000;

		ret = axp_script_parser_fetch(pmu_type, "pmu_init_chgend_rate", &axp_config->pmu_init_chgend_rate, sizeof(u32));
		if (ret)
			axp_config->pmu_init_chgend_rate = INTCHGENDRATE;

		ret = axp_script_parser_fetch(pmu_type, "pmu_init_chg_enabled", &axp_config->pmu_init_chg_enabled, sizeof(u32));
		if (ret)
			axp_config->pmu_init_chg_enabled = 1;

		ret = axp_script_parser_fetch(pmu_type, "pmu_init_bc_en", &axp_config->pmu_init_bc_en, sizeof(u32));
		if (ret)
			axp_config->pmu_init_bc_en = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_init_adc_freq", &axp_config->pmu_init_adc_freq, sizeof(u32));
		if (ret)
			axp_config->pmu_init_adc_freq = INTADCFREQ;

		ret = axp_script_parser_fetch(pmu_type, "pmu_init_adcts_freq", &axp_config->pmu_init_adcts_freq, sizeof(u32));
		if (ret)
			axp_config->pmu_init_adcts_freq = INTADCFREQC;

		ret = axp_script_parser_fetch(pmu_type, "pmu_init_chg_pretime", &axp_config->pmu_init_chg_pretime, sizeof(u32));
		if (ret)
			axp_config->pmu_init_chg_pretime = INTCHGPRETIME;

		ret = axp_script_parser_fetch(pmu_type, "pmu_init_chg_csttime", &axp_config->pmu_init_chg_csttime, sizeof(u32));
		if (ret)
			axp_config->pmu_init_chg_csttime = INTCHGCSTTIME;

		ret = axp_script_parser_fetch(pmu_type, "pmu_batt_cap_correct", &axp_config->pmu_batt_cap_correct, sizeof(u32));
		if (ret)
			axp_config->pmu_batt_cap_correct = 1;

		ret = axp_script_parser_fetch(pmu_type, "pmu_chg_end_on_en", &axp_config->pmu_chg_end_on_en, sizeof(u32));
		if (ret)
			axp_config->pmu_chg_end_on_en = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para1", &axp_config->pmu_bat_para1, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para1 = OCVREG0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para2", &axp_config->pmu_bat_para2, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para2 = OCVREG1;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para3", &axp_config->pmu_bat_para3, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para3 = OCVREG2;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para4", &axp_config->pmu_bat_para4, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para4 = OCVREG3;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para5", &axp_config->pmu_bat_para5, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para5 = OCVREG4;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para6", &axp_config->pmu_bat_para6, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para6 = OCVREG5;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para7", &axp_config->pmu_bat_para7, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para7 = OCVREG6;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para8", &axp_config->pmu_bat_para8, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para8 = OCVREG7;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para9", &axp_config->pmu_bat_para9, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para9 = OCVREG8;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para10", &axp_config->pmu_bat_para10, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para10 = OCVREG9;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para11", &axp_config->pmu_bat_para11, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para11 = OCVREGA;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para12", &axp_config->pmu_bat_para12, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para12 = OCVREGB;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para13", &axp_config->pmu_bat_para13, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para13 = OCVREGC;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para14", &axp_config->pmu_bat_para14, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para14 = OCVREGD;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para15", &axp_config->pmu_bat_para15, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para15 = OCVREGE;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para16", &axp_config->pmu_bat_para16, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para16 = OCVREGF;

		//Add 32 Level OCV para 20121128 by evan
		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para17", &axp_config->pmu_bat_para17, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para17 = OCVREG10;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para18", &axp_config->pmu_bat_para18, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para18 = OCVREG11;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para19", &axp_config->pmu_bat_para19, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para19 = OCVREG12;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para20", &axp_config->pmu_bat_para20, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para20 = OCVREG13;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para21", &axp_config->pmu_bat_para21, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para21 = OCVREG14;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para22", &axp_config->pmu_bat_para22, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para22 = OCVREG15;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para23", &axp_config->pmu_bat_para23, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para23 = OCVREG16;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para24", &axp_config->pmu_bat_para24, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para24 = OCVREG17;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para25", &axp_config->pmu_bat_para25, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para25 = OCVREG18;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para26", &axp_config->pmu_bat_para26, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para26 = OCVREG19;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para27", &axp_config->pmu_bat_para27, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para27 = OCVREG1A;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para28", &axp_config->pmu_bat_para28, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para28 = OCVREG1B;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para29", &axp_config->pmu_bat_para29, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para29 = OCVREG1C;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para30", &axp_config->pmu_bat_para30, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para30 = OCVREG1D;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para31", &axp_config->pmu_bat_para31, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para31 = OCVREG1E;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_para32", &axp_config->pmu_bat_para32, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_para32 = OCVREG1F;

		ret = axp_script_parser_fetch(pmu_type, "pmu_ac_vol", &axp_config->pmu_ac_vol, sizeof(u32));
		if (ret)
			axp_config->pmu_ac_vol = 4400;

		ret = axp_script_parser_fetch(pmu_type, "pmu_usbpc_vol", &axp_config->pmu_usbpc_vol, sizeof(u32));
		if (ret)
			axp_config->pmu_usbpc_vol = 4400;

		ret = axp_script_parser_fetch(pmu_type, "pmu_ac_cur", &axp_config->pmu_ac_cur, sizeof(u32));
		if (ret)
			axp_config->pmu_ac_cur = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_usbpc_cur", &axp_config->pmu_usbpc_cur, sizeof(u32));
		if (ret)
			axp_config->pmu_usbpc_cur = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_pwroff_vol", &axp_config->pmu_pwroff_vol, sizeof(u32));
		if (ret)
			axp_config->pmu_pwroff_vol = 3300;

		ret = axp_script_parser_fetch(pmu_type, "pmu_pwron_vol", &axp_config->pmu_pwron_vol, sizeof(u32));
		if (ret)
			axp_config->pmu_pwron_vol = 2900;
		ret = axp_script_parser_fetch(pmu_type, "pmu_powkey_off_time", &axp_config->pmu_powkey_off_time, sizeof(u32));
		if (ret)
			axp_config->pmu_powkey_off_time = 6000;

		//offlevel restart or not 0:not restart 1:restart
		ret = axp_script_parser_fetch(pmu_type, "pmu_powkey_off_func", &axp_config->pmu_powkey_off_func, sizeof(u32));
		if (ret)
			axp_config->pmu_powkey_off_func = 0;

		//16's power restart or not 0:not restart 1:restart
		ret = axp_script_parser_fetch(pmu_type, "pmu_powkey_off_en", &axp_config->pmu_powkey_off_en, sizeof(u32));
		if (ret)
			axp_config->pmu_powkey_off_en   = 1;

		ret = axp_script_parser_fetch(pmu_type, "pmu_powkey_off_delay_time", &axp_config->pmu_powkey_off_delay_time, sizeof(u32));
		if (ret)
			axp_config->pmu_powkey_off_delay_time   = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_powkey_long_time", &axp_config->pmu_powkey_long_time, sizeof(u32));
		if (ret)
			axp_config->pmu_powkey_long_time = 1500;

		ret = axp_script_parser_fetch(pmu_type, "pmu_pwrok_time", &axp_config->pmu_pwrok_time, sizeof(u32));
		if (ret)
			axp_config->pmu_pwrok_time    = 64;

		ret = axp_script_parser_fetch(pmu_type, "pmu_powkey_on_time", &axp_config->pmu_powkey_on_time, sizeof(u32));
		if (ret)
			axp_config->pmu_powkey_on_time = 1000;

		ret = axp_script_parser_fetch(pmu_type, "pmu_reset_shutdown_en", &axp_config->pmu_reset_shutdown_en, sizeof(u32));
		if (ret)
			axp_config->pmu_reset_shutdown_en = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_battery_warning_level1", &axp_config->pmu_battery_warning_level1, sizeof(u32));
		if (ret)
			axp_config->pmu_battery_warning_level1 = 15;

		ret = axp_script_parser_fetch(pmu_type, "pmu_battery_warning_level2", &axp_config->pmu_battery_warning_level2, sizeof(u32));
		if (ret)
			axp_config->pmu_battery_warning_level2 = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_restvol_adjust_time", &axp_config->pmu_restvol_adjust_time, sizeof(u32));
		if (ret)
			axp_config->pmu_restvol_adjust_time = 30;

		ret = axp_script_parser_fetch(pmu_type, "pmu_ocv_cou_adjust_time", &axp_config->pmu_ocv_cou_adjust_time, sizeof(u32));
		if (ret)
			axp_config->pmu_ocv_cou_adjust_time = 60;

		ret = axp_script_parser_fetch(pmu_type, "pmu_chgled_func", &axp_config->pmu_chgled_func, sizeof(u32));
		if (ret)
			axp_config->pmu_chgled_func = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_chgled_type", &axp_config->pmu_chgled_type, sizeof(u32));
		if (ret)
			axp_config->pmu_chgled_type = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_vbusen_func", &axp_config->pmu_vbusen_func, sizeof(u32));
		if (ret)
			axp_config->pmu_vbusen_func = 1;

		ret = axp_script_parser_fetch(pmu_type, "pmu_reset", &axp_config->pmu_reset, sizeof(u32));
		if (ret)
			axp_config->pmu_reset = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_IRQ_wakeup", &axp_config->pmu_IRQ_wakeup, sizeof(u32));
		if (ret)
			axp_config->pmu_IRQ_wakeup = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_hot_shutdown", &axp_config->pmu_hot_shutdown, sizeof(u32));
		if (ret)
			axp_config->pmu_hot_shutdown = 1;

		ret = axp_script_parser_fetch(pmu_type, "pmu_inshort", &axp_config->pmu_inshort, sizeof(u32));
		if (ret)
			axp_config->pmu_inshort = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_temp_enable", &axp_config->pmu_bat_temp_enable, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_temp_enable = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_charge_ltf", &axp_config->pmu_bat_charge_ltf, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_charge_ltf = 0xA5;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_charge_htf", &axp_config->pmu_bat_charge_htf, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_charge_htf = 0x1F;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_shutdown_ltf", &axp_config->pmu_bat_shutdown_ltf, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_shutdown_ltf = 0xFC;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_shutdown_htf", &axp_config->pmu_bat_shutdown_htf, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_shutdown_htf = 0x16;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_temp_para1", &axp_config->pmu_bat_temp_para1, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_temp_para1 = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_temp_para2", &axp_config->pmu_bat_temp_para2, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_temp_para2 = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_temp_para3", &axp_config->pmu_bat_temp_para3, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_temp_para3 = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_temp_para4", &axp_config->pmu_bat_temp_para4, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_temp_para4 = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_temp_para5", &axp_config->pmu_bat_temp_para5, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_temp_para5 = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_temp_para6", &axp_config->pmu_bat_temp_para6, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_temp_para6 = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_temp_para7", &axp_config->pmu_bat_temp_para7, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_temp_para7 = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_temp_para8", &axp_config->pmu_bat_temp_para8, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_temp_para8 = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_temp_para9", &axp_config->pmu_bat_temp_para9, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_temp_para9 = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_temp_para10", &axp_config->pmu_bat_temp_para10, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_temp_para10 = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_temp_para11", &axp_config->pmu_bat_temp_para11, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_temp_para11 = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_temp_para12", &axp_config->pmu_bat_temp_para12, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_temp_para12 = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_temp_para13", &axp_config->pmu_bat_temp_para13, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_temp_para13 = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_temp_para14", &axp_config->pmu_bat_temp_para14, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_temp_para14 = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_temp_para15", &axp_config->pmu_bat_temp_para15, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_temp_para15 = 0;

		ret = axp_script_parser_fetch(pmu_type, "pmu_bat_temp_para16", &axp_config->pmu_bat_temp_para16, sizeof(u32));
		if (ret)
			axp_config->pmu_bat_temp_para16 = 0;

	} else {
		return -1;
	}

	return 0;
}

MODULE_DESCRIPTION("X-POWERS axp script");
MODULE_AUTHOR("Ming Li");
MODULE_LICENSE("GPL");
