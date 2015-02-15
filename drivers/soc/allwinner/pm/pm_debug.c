#include "pm_i.h"

//for io-measure time
#define PORT_E_CONFIG (0xf1c20890)
#define PORT_E_DATA (0xf1c208a0)
#define PORT_CONFIG PORT_E_CONFIG
#define PORT_DATA PORT_E_DATA

volatile int  print_flag = 0;

void busy_waiting(void)
{
#if 1
	volatile __u32 loop_flag = 1;
	while(1 == loop_flag);
	
#endif
	return;
}

#if defined(CONFIG_ARCH_SUN8IW6P1) || defined(CONFIG_ARCH_SUN9IW1P1)
static volatile __r_prcm_pio_pad_hold *status_reg;
static __r_prcm_pio_pad_hold status_reg_tmp;
static volatile __r_prcm_pio_pad_hold *status_reg_pa; 
static __r_prcm_pio_pad_hold status_reg_pa_tmp; 
void mem_status_init(void)
{
	status_reg	= (volatile __r_prcm_pio_pad_hold *)(STANDBY_STATUS_REG);
	status_reg_pa	= (volatile __r_prcm_pio_pad_hold *)(STANDBY_STATUS_REG_PA);

	//init spinlock for sync
	hwspinlock_init(1);
}

void mem_status_init_nommu(void)
{
	status_reg	= (volatile __r_prcm_pio_pad_hold *)(STANDBY_STATUS_REG);
	status_reg_pa	= (volatile __r_prcm_pio_pad_hold *)(STANDBY_STATUS_REG_PA);

	//init spinlock for sync
	hwspinlock_init(0);
}

void mem_status_clear(void)
{
	int i = 1;

	status_reg_tmp.dwval = (*status_reg).dwval;
	if (!hwspin_lock_timeout(MEM_RTC_REG_HWSPINLOCK, 20000)) {
		while(i < STANDBY_STATUS_REG_NUM){
			status_reg_tmp.bits.reg_sel = i;
			status_reg_tmp.bits.data_wr = 0;
			(*status_reg).dwval = status_reg_tmp.dwval;
			status_reg_tmp.bits.wr_pulse = 0;
			(*status_reg).dwval = status_reg_tmp.dwval;
			status_reg_tmp.bits.wr_pulse = 1;
			(*status_reg).dwval = status_reg_tmp.dwval;
			status_reg_tmp.bits.wr_pulse = 0;
			(*status_reg).dwval = status_reg_tmp.dwval;
			i++;
	    	}
		hwspin_unlock(MEM_RTC_REG_HWSPINLOCK);
	}
}

void mem_status_exit(void)
{
	return ;
}

void save_mem_status(volatile __u32 val)
{
	if (!hwspin_lock_timeout(MEM_RTC_REG_HWSPINLOCK, 20000)) {
		status_reg_tmp.bits.reg_sel = 1;
		status_reg_tmp.bits.data_wr = val;
		(*status_reg).dwval = status_reg_tmp.dwval;
		status_reg_tmp.bits.wr_pulse = 0;
		(*status_reg).dwval = status_reg_tmp.dwval;
		status_reg_tmp.bits.wr_pulse = 1;
		(*status_reg).dwval = status_reg_tmp.dwval;
		status_reg_tmp.bits.wr_pulse = 0;
		(*status_reg).dwval = status_reg_tmp.dwval;
		hwspin_unlock(MEM_RTC_REG_HWSPINLOCK);
	}

//	asm volatile ("dsb");
//	asm volatile ("isb");
	return;
}

__u32 get_mem_status(void)
{
	int val = 0;
	status_reg_tmp.bits.reg_sel = 1;
    
	if (!hwspin_lock_timeout(MEM_RTC_REG_HWSPINLOCK, 20000)) {
		(*status_reg).dwval = status_reg_tmp.dwval;
		//read
		status_reg_tmp.dwval = (*status_reg).dwval;
		hwspin_unlock(MEM_RTC_REG_HWSPINLOCK);
	}
    
	val = status_reg_tmp.bits.data_rd;
	return (val);
}

void show_mem_status(void)
{
	int i = 1;
	int val = 0;
	while(i < STANDBY_STATUS_REG_NUM) {
		status_reg_tmp.bits.reg_sel = i;
	    
	    //write
	    if (!hwspin_lock_timeout(MEM_RTC_REG_HWSPINLOCK, 20000)) {
		    (*status_reg).dwval = status_reg_tmp.dwval;
		    //read
		    status_reg_tmp.dwval = (*status_reg).dwval;
		    hwspin_unlock(MEM_RTC_REG_HWSPINLOCK);
	    }
	   
	    val = status_reg_tmp.bits.data_rd;
	    printk("addr %x, value = %x. \n", \
		    (i), val);
	    i++;
	}
}

void save_mem_status_nommu(volatile __u32 val)
{
	if (!hwspin_lock_timeout_nommu(MEM_RTC_REG_HWSPINLOCK, 20000)) {
		status_reg_pa_tmp.bits.reg_sel = 1;
		status_reg_pa_tmp.bits.data_wr = val;
		(*status_reg_pa).dwval = status_reg_pa_tmp.dwval;
		status_reg_pa_tmp.bits.wr_pulse = 0;
		(*status_reg_pa).dwval = status_reg_pa_tmp.dwval;
		status_reg_pa_tmp.bits.wr_pulse = 1;
		(*status_reg_pa).dwval = status_reg_pa_tmp.dwval;
		status_reg_pa_tmp.bits.wr_pulse = 0;
		(*status_reg_pa).dwval = status_reg_pa_tmp.dwval;
		hwspin_unlock_nommu(MEM_RTC_REG_HWSPINLOCK);
	}

	return;
}

#elif defined(CONFIG_ARCH_SUN8I)    || defined(CONFIG_ARCH_SUN50IW1P1)
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

#endif


/*
 * notice: dependant with perf counter to delay.
 */
void io_init(void)
{
	//config port output
	*(volatile unsigned int *)(PORT_CONFIG)  = 0x111111;
	
	return;
}

void io_init_high(void)
{
	__u32 data;
	
	//set port to high
	data = *(volatile unsigned int *)(PORT_DATA);
	data |= 0x3f;
	*(volatile unsigned int *)(PORT_DATA) = data;

	return;
}

void io_init_low(void)
{
	__u32 data;

	data = *(volatile unsigned int *)(PORT_DATA);
	//set port to low
	data &= 0xffffffc0;
	*(volatile unsigned int *)(PORT_DATA) = data;

	return;
}

/*
 * set pa port to high, num range is 0-7;	
 */
void io_high(int num)
{
	__u32 data;
	data = *(volatile unsigned int *)(PORT_DATA);
	//pull low 10ms
	data &= (~(1<<num));
	*(volatile unsigned int *)(PORT_DATA) = data;
	udelay(10000);
	//pull high
	data |= (1<<num);
	*(volatile unsigned int *)(PORT_DATA) = data;

	return;
}

