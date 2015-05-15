#ifndef __WIRELESS_REZENCE_H__
#define __WIRELESS_REZENCE_H__

#define PSY_MAX_STR_PROP_LEN	33

#define PRU_ALERT_OVERVOLTAGE	(1 << 7)
#define PRU_ALERT_OVERCURRENT	(1 << 6)
#define PRU_ALERT_OVERTEMP	(1 << 5)
#define PRU_ALERT_CHARGE_COMPLETE	(1 << 3)
#define PRU_ALERT_WIRECHARGER_DET	(1 << 2)
#define PRU_ALERT_CHARGEPORT_EN	(1 << 1)
#define PRU_ALERT_CHARGING	0xF7
#define PRU_ALERT_ERROR_RECOVERED	0x1F

struct rezence_status {
	unsigned long ptu_stat;
	unsigned long pru_stat;
	unsigned long battery_stat;
	unsigned long charger_health;
	unsigned long pru_health;
	unsigned long battery_health;
	char ptu_static_param[PSY_MAX_STR_PROP_LEN];
	bool is_init;
	bool is_charging_enabled;
	bool is_deselected;
	bool is_wired_mode;
};

struct ptu_static_param {
	u8 optional_fields;
	u8 ptu_power;
	u8 ptu_max_src_impedance;
	u8 ptu_max_load_res;
	u16 rfu1;
	u8 ptu_class;
	u8 hw_rev;
	u8 fw_rev;
	u8 proto_rev;
	u8 num_devs;
	u32 rfu2;
	u16 rfu3;
} __packed;

struct wireless_pru {
	int (*do_measurements)(struct wireless_pru *pru);
	int (*hide_present)(struct wireless_pru *pru, bool hide,
			bool maintenance);

	struct power_supply *psy_rez;

	/* private */
	struct power_supply *to_charger;
	struct notifier_block psy_rez_nb;
	struct work_struct  notifier_work;
	struct delayed_work dynamic_param_work;
	struct workqueue_struct *notifier_wq;
	struct list_head evt_queue;
	struct rezence_status stat;
	struct rezence_status prev_stat;
	struct ptu_static_param ptu_stat;
	char pru_dynamic_str[PSY_MAX_STR_PROP_LEN];
	char pru_static_str[PSY_MAX_STR_PROP_LEN];
	spinlock_t evt_queue_lock;
	bool is_charge_complete;
};

extern int wireless_rezence_register(struct wireless_pru *pru);
extern int wireless_rezence_unregister(struct wireless_pru *pru);

#endif
