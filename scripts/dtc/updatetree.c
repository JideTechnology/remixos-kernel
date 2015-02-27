#include "updatetree.h"

extern struct script  *script;

/*public functions */
int of_prop_string_count(const char *prop_val, int prop_len)
{
	int l = 0, i = 0;
	const char *p, *end;

	p = prop_val;
	end = p + prop_len;

	for (i = 0; p < end;  i++, p += l) {
		l = strnlen(p, end - p) + 1;
		if (p + l > end)
			return -1;
	}
	return i <= 0 ? -1 : i;
}
const char *of_prop_next_string(struct property *prop, const char *cur)
{
	const char *curv = cur;
	if (!prop)
		return NULL;

	if (!cur)
		return prop->val.val;

	curv += strlen(cur) + 1;
	if (curv >= prop->val.val + prop->val.len)
		return NULL;

	return curv;
}

int sunxi_get_propval(struct node *node, const char *name)
{
	unsigned int value;
	struct property *prop;
	cell_t *info = NULL;
	prop = get_property(node, name);
	if (prop) {
		info = (cell_t *)prop->val.val;
		value = fdt32_to_cpu(*info++);
		return value;
	} else {
		return -1;
	}

}
int sunxi_gpio_to_name(int port, int port_num, char *name)
{
	int index;
	if (!name) {
		return -1;
	}
	if ((port*32+port_num) >= 1024) {
		/* axp gpio name like this : GPIO0/GPIO1/.. */
		index = (port-1)*32+port_num - 1024;
		sprintf(name, "GPIO%d", index);
	} else {
		/* sunxi gpio name like this : PA0/PA1/PB0 */
		sprintf(name, "P%c%d", ('A' + (port-1)), port_num);
	}
	return 0;
}


/*these functions mainly to process pin subkey*/
#define PL_BASE  (352)
#define AXP_BASE (1024)

void sunxi_dt_update_gpio_group(struct boot_info *bi,
				   struct node *node,
				   struct script_entry *ep,
				   struct script_gpio_entry *entry)
{
	char *pinctrl_name = NULL, *string;
	int val32, gpio_index, temp_val, phandle;

	struct data d;
	struct property *prop;
	struct node *pinctrl_node;

	prop = get_property(node, ep->name);
	if (prop) {
		delete_property(prop);
	}

	gpio_index = (entry->port - 1) * 32 + entry->port_num;
	if (gpio_index < PL_BASE) {
		pinctrl_name = "pio";
	} else if (gpio_index < AXP_BASE && gpio_index >= PL_BASE) {
		pinctrl_name = "r_pio";
	} else {
		pinctrl_name = "axp_pio";
	}
	pinctrl_node = get_node_by_label(bi->dt, pinctrl_name);

	string = malloc(7 * sizeof(unsigned int));
	/*set gpio control */
	phandle = get_node_phandle(bi->dt, pinctrl_node);
	val32 = cpu_to_fdt32(phandle);
	memcpy(string, (char *)&val32, sizeof(val32));

	/*set gpio bank */
	val32 = cpu_to_fdt32(entry->port-1);
	memcpy(string + 1 * sizeof(val32), (char *)&val32,  sizeof(val32));

	/*set gpio bank index */
	val32 = cpu_to_fdt32(entry->port_num);
	memcpy(string + 2 * sizeof(val32), (char *)&val32,  sizeof(val32));

	/*set gpio mul */
	val32 = cpu_to_fdt32(entry->data[0]);
	memcpy(string + 3 * sizeof(val32), (char *)&val32,  sizeof(val32));

	/*set gpio pull */
	temp_val = entry->data[1] < 0 ? 0 : entry->data[1];
	val32 = cpu_to_fdt32(temp_val);
	memcpy(string + 4 * sizeof(val32), (char *)&val32,  sizeof(val32));

	/*set gpio drive */
	temp_val = entry->data[2] < 0 ? 1 : entry->data[2];
	val32 = cpu_to_fdt32(temp_val);
	memcpy(string + 5 * sizeof(val32),  (char *)&val32,  sizeof(val32));

	/*set gpio data */
	temp_val = entry->data[3] < 0 ? 0 : entry->data[3];
	val32 = cpu_to_fdt32(temp_val);
	memcpy(string + 6 * sizeof(val32),  (char *)&val32,  sizeof(val32));

	d = data_copy_mem(string, 7 * sizeof(unsigned int));
	prop = build_property(ep->name, d);
	add_property(node, prop);
	free(string);

}

int sunxi_dt_init_pinconf_prop(struct script_section *section,
				  struct boot_info *bi,
				  struct node *node,
				  int sleep_state)
{

	int i, len, phandle;
	cell_t *cp = NULL;
	void *propend;

	struct list_entry *o;
	struct script_entry *ep;
	struct script_gpio_entry *entry;

	struct data d0;
	struct node *p_node = NULL;
	struct property *prop;
	struct property *pinctrl;
	/*remove pin config */
	for_each_entry_in_section(section->entries, o) {
		ep = container_of(o, struct script_entry,  entries);
		entry = container_of(ep, struct script_gpio_entry, entry);
		if (entry->data[0] == 2 || entry->data[0] == 3 ||
		    entry->data[0] == 4 || entry->data[0] == 5) {
			if (!sleep_state) {
				prop = get_property(node, "pinctrl-0");
			} else {
				prop = get_property(node, "pinctrl-1");
			}
			if (prop) {
				len = prop->val.len/sizeof(unsigned int);
				cp = (cell_t *)prop->val.val;
				propend = prop->val.val + prop->val.len;
				for (i = 0 ; i < len; i++) {
					phandle = fdt32_to_cpu(*cp++);
					p_node = get_node_by_phandle(bi->dt, phandle);
					delete_node(p_node);
				}
				delete_property(prop);
			}
			/*build pinctrl-0 property */
			d0 = data_copy_mem(NULL, 0);
			if (!sleep_state) {
				pinctrl = build_property("pinctrl-0", d0);
			} else {
				pinctrl = build_property("pinctrl-1", d0);
			}
			add_property(node, pinctrl);

			break;
		}
	}

	return 0;

}

cell_t sunxi_dt_add_new_node_to_pinctrl(struct node *pinctrl_node,
					   const char *dev_name,
					   const char *pname,
					   char *gpio_name,
					   int gpio_value[],
					   struct boot_info *bi)
{
	cell_t  phandle;
	int  i, value_32, len, str_len;
	char *label, *node_name;
	struct  data d;
	struct  node *child, *temp_node;
	struct  property *prop;
	child = build_node(NULL, NULL);

	/* set label */
	str_len = strlen(dev_name)+strlen("_pins_")+2;
	label = malloc(str_len);
	strcpy(label, dev_name);
	strcat(label, "_pins_");

	for (i = 0;i < 8; i++) {
		label[str_len-2] = (char)(i+'a');
		label[str_len-1] = '\0';
		temp_node = get_node_by_label(bi->dt, label);
		if (temp_node) {
			continue;
		} else {
			break;
		}
	}
	add_label(&child->labels, label);

	/*set node name*/
	str_len = strlen(dev_name)+2;
	node_name = malloc(str_len);
	strcpy(node_name, dev_name);
	strcat(node_name, "@");
	for (i = 0; i < 8; i++) {
		node_name[str_len-1] = (char)(i+'0');
		node_name[str_len] = '\0';
		temp_node = get_subnode(pinctrl_node, node_name);
		if (temp_node) {
			continue;
		} else {
			break;
		}
	}

	/* set node name and phandle*/
	child->name = xstrdup(node_name);
	free(node_name);

	/* set fullpath */
	child->fullpath = join_path(pinctrl_node->fullpath, child->name);

	/*set phandle*/
	phandle = get_node_phandle(bi->dt, child);
	add_child(pinctrl_node, child);

	/*add property of pins */
	d = data_copy_mem(gpio_name, strlen(gpio_name) + 1);
	prop = build_property("allwinner,pins", d);
	add_property(child, prop);

	d = data_copy_mem(dev_name, strlen(dev_name) + 1);
	prop = build_property("allwinner,function", d);
	add_property(child, prop);

	d = data_copy_mem(pname, strlen(pname) + 1);
	prop = build_property("allwinner,pname", d);
	add_property(child, prop);

	value_32 = cpu_to_fdt32(gpio_value[1]);
	len = sizeof(unsigned int);
	d = data_copy_mem((char *)&value_32, len);
	prop = build_property("allwinner,pull", d);
	add_property(child, prop);

	value_32 = cpu_to_fdt32(gpio_value[2]);
	len = sizeof(unsigned int);
	d = data_copy_mem((char *)&value_32, len);
	prop = build_property("allwinner,drive", d);
	add_property(child, prop);

	value_32 = cpu_to_fdt32(gpio_value[3]);
	len = sizeof(unsigned int);
	d = data_copy_mem((char *)&value_32, len);
	prop = build_property("allwinner,data", d);
	add_property(child, prop);

	return phandle;

}
int insert_pinconf_node(const char *section_name,
			 struct boot_info *bi,
			 struct node *node,
			 struct script_entry *ep,
			 const char *prop_name)
{
	char gpio_name[32], *string;
	cell_t *cp, *info;
	int pull = 0, data = 0, drive = 0;
	int i, ret, len, gpio_value[4], phandle, phandle_count;

	struct node *pin_node;
	struct property *prop, *prop_temp;
	struct script_gpio_entry *entry;

	entry = container_of(ep, struct script_gpio_entry, entry);
	ret = sunxi_gpio_to_name(entry->port, entry->port_num, gpio_name);
	gpio_value[0] = entry->data[0];
	gpio_value[1] = entry->data[1] < 0 ? 0 : entry->data[1];
	gpio_value[2] = entry->data[2] < 0 ? 1 : entry->data[2];
	gpio_value[3] = entry->data[3] < 0 ? 0 : entry->data[3];


	prop = get_property(node, prop_name);
	phandle_count = prop->val.len/sizeof(unsigned int);
	cp = (cell_t *)prop->val.val;
	for(i = 0; i < phandle_count; i++) {
		phandle = fdt32_to_cpu(*cp++);
		pin_node = get_node_by_phandle(bi->dt, phandle);
		if (pin_node) {

			/* if find a pinctrl node, check config*/
			prop_temp = get_property(pin_node, "allwinner,pull");
			if (prop_temp) {
				info = (cell_t *)prop_temp->val.val;
				pull = fdt32_to_cpu(*info++);
			}
			prop_temp = get_property(pin_node, "allwinner,drive");
			if (prop_temp) {
				info = (cell_t *)prop_temp->val.val;
				drive = fdt32_to_cpu(*info++);
			}
			prop_temp = get_property(pin_node, "allwinner,data");
			if (prop_temp) {
				info = (cell_t *)prop_temp->val.val;
				data = fdt32_to_cpu(*info++);
			}
			if (gpio_value[1] == pull && gpio_value[2] == drive && gpio_value[3] == data) {
				prop_temp = get_property(pin_node, "allwinner,pins");
				if (prop_temp) {
					len = prop_temp->val.len + strlen(gpio_name) + 1;
					string = malloc(len);
					memcpy(string, prop_temp->val.val, prop_temp->val.len);
					memcpy(string + prop_temp->val.len, gpio_name, strlen(gpio_name));
					string[len-1] = '\0';
					free(prop_temp->val.val);
					prop_temp->val.val = malloc(len);
					memcpy(prop_temp->val.val, string, len);
					prop_temp->val.len = len;
					free(string);
				}
				prop_temp = get_property(pin_node, "allwinner,pname");
				if(prop_temp){
					len = prop_temp->val.len + strlen(ep->name) + 1;
					string = malloc(len);
					memcpy(string, prop_temp->val.val, prop_temp->val.len);
					memcpy(string + prop_temp->val.len, ep->name, strlen(ep->name));
					string[len-1] = '\0';
					free(prop_temp->val.val);
					prop_temp->val.val = malloc(len);
					memcpy(prop_temp->val.val, string, len);
					prop_temp->val.len = len;
					free(string);
					return 0;
				}
			}
		}
	}
	return -1;

}
void create_pinconf_node(const char *section_name,
			   struct boot_info *bi,
			   struct node *node,
			   struct script_entry *ep,
			   struct property *prop)
{
	int value[4], phandle;
	char gpio_name[32], *propname;
	struct node *pinctrl = NULL;
	struct data d_pinctrl_x;
	struct property *pinctrl_x;
	struct script_gpio_entry *entry;

	entry = container_of(ep, struct script_gpio_entry, entry);
	sunxi_gpio_to_name(entry->port, entry->port_num, gpio_name);
	value[0] = entry->data[0];
	value[1] = entry->data[1] < 0 ? 0 : entry->data[1];
	value[2] = entry->data[2] < 0 ? 1 : entry->data[2];
	value[3] = entry->data[3] < 0 ? 0 : entry->data[3];

	if (entry->port * 32 < PL_BASE) {
		pinctrl = get_node_by_label(bi->dt, "pio");
	} else if (entry->port * 32 >= PL_BASE && entry->port < AXP_BASE) {
		pinctrl = get_node_by_label(bi->dt, "r_pio");
	}
	phandle = sunxi_dt_add_new_node_to_pinctrl(pinctrl, section_name, ep->name, gpio_name, value, bi);
	if (phandle) {
		d_pinctrl_x = data_append_cell(prop->val, phandle);
		propname = malloc(strlen(prop->name));
		strcpy(propname, prop->name);
		delete_property(prop);
		pinctrl_x = build_property(propname, d_pinctrl_x);
		add_property(node, pinctrl_x);
	}
}
void sunxi_dt_update_pin_group_sleep(const char *section_name,
				  struct boot_info *bi,
				  struct node *node,
				  struct script_entry *ep)
{
	int ret = 0;
	char gpio_name[32] = {0};
	struct property *prop;
	struct script_gpio_entry *entry;

	entry = container_of(ep, struct script_gpio_entry, entry);
	sunxi_gpio_to_name(entry->port, entry->port_num, gpio_name);
	prop = get_property(node, "pinctrl-1");
	ret = insert_pinconf_node(section_name, bi, node, ep, "pinctrl-1");
	if (ret) {
		create_pinconf_node(section_name, bi, node, ep, prop);
	}


}


void sunxi_dt_update_pin_group_default(const char *section_name,
				  struct boot_info *bi,
				  struct node *node,
				  struct script_entry *ep)
{
	int ret = 0;
	char gpio_name[32] = {0};

	struct property *prop;
	struct script_gpio_entry *entry;

	entry = container_of(ep, struct script_gpio_entry, entry);
	sunxi_gpio_to_name(entry->port, entry->port_num, gpio_name);
	prop = get_property(node, "pinctrl-0");
	ret = insert_pinconf_node(section_name, bi, node, ep, "pinctrl-0");
	if (ret) {
		create_pinconf_node(section_name, bi, node, ep, prop);
	}

}

/*end process pinconf */

void sunxi_dt_update_propval_cells(const char *section_name,
				     struct script_entry *ep,
				     struct node *node)
{
	int  len;
	unsigned int value_32;
	char	temp[32], *string;
	struct data d;
	struct property *prop;
	struct script_single_entry *entry;

	entry = container_of(ep, struct script_single_entry, entry);
	strcpy(temp, section_name);
	strcat(temp, "_used");
	if (strcmp(ep->name, "used") == 0 || strcmp(ep->name, temp) == 0) {

		string = entry->value ? "okay" : "disabled";
		len = strlen(string) + 1;
		prop = get_property(node, "status");

		if (prop) {
			free(prop->val.val);
			prop->val.val = malloc(len);
			strcpy(prop->val.val, string);
			prop->val.len = len;
		} else {
			d = data_copy_mem(string, len);
			prop = build_property("status", d);
			add_property(node, prop);
		}
	} else {
		value_32 = cpu_to_fdt32(entry->value);
		len = sizeof(unsigned int);
		prop = get_property(node, ep->name);

		if (prop) {
			free(prop->val.val);
			prop->val.val = malloc(len);
			memcpy(prop->val.val, &value_32, len);
			prop->val.len = len;
		} else {
			d = data_copy_mem((char *)&value_32, len);
			prop = build_property(ep->name, d);
			add_property(node, prop);
		}
	}
}

void sunxi_dt_update_propval_string(const char *section_name,
				      struct script_entry *ep,
				      struct node *node)
{
	int  len;
	char *string;

	struct data d;
	struct property *prop;
	struct script_string_entry *entry;

	/*SCRIPT_VALUE_TYPE_STRING*/
	entry = container_of(ep, struct script_string_entry, entry);
	string = entry->string;
	len = entry->l;

	prop = get_property(node, ep->name);
	if (prop) {
		free(prop->val.val);
		prop->val.val = malloc(len);
		strcpy(prop->val.val, string);
		prop->val.len = len;
	} else {
		d = data_copy_mem(string, len);
		prop = build_property(ep->name, d);
		add_property(node, prop);
	}
}
void sunxi_dt_update_propval_empty(const char *section_name,
				      struct script_entry *ep,
				      struct node *node)
{
	struct data d={0};
	struct property *prop, *temp;
	struct script_null_entry *entry;

	/*SCRIPT_VALUE_TYPE_STRING*/
	entry = container_of(ep, struct script_null_entry, entry);

	prop = get_property(node, entry->entry.name);
	if (prop) {
		delete_property(prop);
	}
	temp = build_property(entry->entry.name, d);
	add_property(node, temp);
}

void sunxi_dt_update_propval_gpio(const char *section_name,
				     struct script_entry *ep,
				     struct node *node,
				     struct boot_info *bi,
				     int sleep_state)
{
	struct script_gpio_entry *entry;
	entry = container_of(ep, struct script_gpio_entry, entry);

	switch (entry->data[0]) {
	case 0:
	case 1:
	case 6:
		sunxi_dt_update_gpio_group(bi, node, ep, entry);
		break;
	case 2:
	case 3:
	case 4:
	case 5:
		if (sleep_state) {
			sunxi_dt_update_pin_group_sleep(section_name, bi, node, ep);
		} else {
			sunxi_dt_update_pin_group_default(section_name, bi, node, ep);
		}
		break;
	}

}

int dt_update_source(const char *fexname, FILE *f, struct boot_info *bi)
{

	int ret = 0;
	int slen = 0;
	int sleep_state = 0;
	char *p_key = NULL;
	char *c_key = NULL;
	char *equals = NULL;
	struct node *node;
	struct list_entry *sec_list, *o;
	struct script_section *section;
	struct script_entry *ep;

	ret = script_parse(fexname);
	if (ret) {
		printf(PRINTF_RED 				\
			"[%s][%d]Parse Sys_config Failed.\n" 	\
			PRINTF_NONE, 				\
			__func__, __LINE__);
		return -1;
	}

	for_each_section_in_list(script->sections, sec_list){
		section = container_of(sec_list,
			struct script_section, sections);
	       /*
		* here mainly deal with section name like parent_key/child_key
		*/
		equals = strchr(section->name, '/');
		if (equals) {
			slen = (strlen(section->name) + section->name) - equals + 1;
			p_key = malloc(slen);
			memset(p_key, 0, slen);
			c_key = malloc(equals - section->name + 1);
			memset(c_key, 0, equals - section->name + 1);
			if (sscanf(section->name, "%[^/]/%[^@]", p_key, c_key) == 2) {
				node = get_node_by_label(bi->dt, p_key);
				if(!node){
					printf(PRINTF_RED 				\
						"[%s]Should defind behind[%s] or" 	\
						"Parent_key[%s]May Not Conver To Dts."	\
						PRINTF_NONE, 				\
						section->name, p_key, p_key);
					return -1;
				}
			} else {
				printf(PRINTF_RED
					"[%s]section name defined error.\n"
					PRINTF_NONE,
					section->name);
				return -1;
			}
		} else {
			c_key = malloc(strlen(section->name) + 1);
			memset(c_key, 0, strlen(section->name) + 1);
			memcpy(c_key, section->name, strlen(section->name));
		}

		/*deal with section name like xxx_suspend.*/
		if (strstr(c_key, "_suspend")) {
			sleep_state = 1;
			c_key = strsep(&c_key, "_");
		}
		node = get_node_by_label(bi->dt, c_key);
		if (!node) {
			{
				char *label;
				struct  node *child, *parent;
				parent = get_node_by_label(bi->dt, "soc");
				child = build_node(NULL, NULL);
				if (!child) {
					printf("build node faile[%s]\n", c_key);
				}
				label = xstrdup(c_key);
				add_label(&child->labels, label);
				child->name = xstrdup(c_key);
				child->fullpath = join_path(parent->fullpath, child->name);
				add_child(parent, child);
				node = get_node_by_label(bi->dt, c_key);
			}

		}
		sunxi_dt_init_pinconf_prop(section, bi, node, sleep_state);
		for_each_entry_in_section(section->entries, o){
			ep = container_of(o, struct script_entry,  entries);
			switch (ep->type) {
			case 1:
			case 2:
				sunxi_dt_update_propval_cells(c_key, ep, node);
				break;
			case 3:
				sunxi_dt_update_propval_string(c_key, ep, node);
				break;
			case 5:
				sunxi_dt_update_propval_gpio(c_key, ep, node, bi, sleep_state);
				break;
			case 6:
				/*empty property */
				sunxi_dt_update_propval_empty(c_key, ep, node);
				break;
			default:
				printf("input type error.\n");
				break;
			}
		}
		if (equals) {
			free(p_key);
			free(c_key);
		} else {
			free(c_key);
		}
	}
	return 0;
}

