#include "pm_i.h"

static u32 *base;
static u32 len;

void mem_status_init(char *name)
{
    u32 gpr_offset;
    struct device_node *np;
    
    pm_get_dev_info(name, 0, &base, &len);
    np = of_find_node_by_type(NULL, name);
    if(NULL == np){
	printk(KERN_ERR "can not find np for %s. \n", name);
    }
    else{
	printk(KERN_INFO "np name = %s. \n", np->full_name);
	of_property_read_u32(np, "gpr_offset", &gpr_offset);
	of_property_read_u32(np, "gpr_len", &len);

	base = (gpr_offset/sizeof(u32) + base);
	printk("base = %p, len = %x.\n", base, len);
    }

    
    return  ;
}

void mem_status_init_nommu(void)
{
    return  ;
}

void mem_status_clear(void)
{
	int i = 0;

	while(i < len){
		*(volatile int *)((phys_addr_t)(base + i)) = 0x0;
		i++;
	}
	return	;

}

void mem_status_exit(void)
{
	return ;
}

void save_mem_status(volatile __u32 val)
{
	*(volatile __u32 *)((phys_addr_t)(base  + 1)) = val;
//	asm volatile ("dsb");
//	asm volatile ("isb");
	return;
}

__u32 get_mem_status(void)
{
	return *(volatile __u32 *)((phys_addr_t)(base  + 1));
}

void show_mem_status(void)
{
	int i = 0;

	while(i < len){
		printk("addr %p, value = %x. \n", \
			(base + i), *(volatile int *)((phys_addr_t)(base + i)));
		i++;
	}
}



