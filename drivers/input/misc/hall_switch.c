

#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/pm_runtime.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/async.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>



#define IRQ_GPIO_NUM     336       //east-22
#define TP_RST_GPIO_NUM  360//366
#define IRQ              gpio_to_irq(IRQ_GPIO_NUM)

struct hall_switch_data {
	struct input_dev *input_dev;
	struct work_struct work;
	struct workqueue_struct *wq;
}*ghall_switch_data;

int hall_switch_flag = 0;
EXPORT_SYMBOL(hall_switch_flag);

static void hall_switch_worker(struct work_struct *work)
{
	int value = gpio_get_value(IRQ_GPIO_NUM);
	int tp_state = gpio_get_value(TP_RST_GPIO_NUM);    

	msleep(500);
	int new_value = gpio_get_value(IRQ_GPIO_NUM);

	printk("[%s] irq_value = %d, irq_new_value:%d, tp_status:%d\n", __func__, value, new_value,tp_state);

	printk("hampoo the value =%d,tp_state=%d\n",value,tp_state);


	if (new_value != value)
		return;

	if (!value && !tp_state)
		return;



	if (!value) {
		hall_switch_flag = 0;
		printk("set hall_switch_flag %d!\n", hall_switch_flag);
		msleep(10);

		input_report_key(ghall_switch_data->input_dev, KEY_SLEEP, 1);
		input_sync(ghall_switch_data->input_dev);
		input_report_key(ghall_switch_data->input_dev, KEY_SLEEP, 0);
		input_sync(ghall_switch_data->input_dev);
	} else {
		input_report_key(ghall_switch_data->input_dev, KEY_WAKEUP, 1);
		input_sync(ghall_switch_data->input_dev);
		input_report_key(ghall_switch_data->input_dev, KEY_WAKEUP, 0);
		input_sync(ghall_switch_data->input_dev);

		msleep(10);
		hall_switch_flag = 1;
		printk("set hall_switch_flag %d!\n", hall_switch_flag);
	}
	//enable_irq(IRQ);
}


static irqreturn_t hall_switch_irq_handler(int irq, void *dev_id)
{
	printk("[%s]\n", __func__);

	struct hall_switch_data *hall_switch_data = dev_id;

	//disable_irq_nosync(IRQ);

	if (!work_pending(&hall_switch_data->work)) 
	{
		queue_work(hall_switch_data->wq, &hall_switch_data->work);
	}

	return IRQ_HANDLED;
}


//extern set_gpio_pullup(int gpio, int strength);
static int __init hall_switch_init(void)
{
	printk("[%s]\n", __func__);

	int ret = -1;
	int irq = -1;
	struct input_dev *input_dev;
	
	ret = gpio_request(IRQ_GPIO_NUM, "hall_switch irq");
	if (ret < 0) {
		printk("[%s] gpio[%d] request failed", __func__, IRQ_GPIO_NUM);
		return ret;
	}
	gpio_direction_input(IRQ_GPIO_NUM);

	ghall_switch_data = kzalloc(sizeof(*ghall_switch_data), GFP_KERNEL);
	if (!ghall_switch_data)
		return -ENOMEM;

	input_dev = input_allocate_device();
	if (!input_dev) {
		ret = -ENOMEM;
		goto err_kzalloc;
	}
	ghall_switch_data->input_dev = input_dev;


	input_dev->name = "hall_switch";
	input_set_capability(input_dev, EV_KEY, KEY_POWER);
	input_set_capability(input_dev, EV_KEY, KEY_SLEEP);
	input_set_capability(input_dev, EV_KEY, KEY_WAKEUP);

	ret = input_register_device(input_dev);
	if (ret < 0) {
		ret = -EINVAL;
		goto err_input_register_device;
	}

	ghall_switch_data->wq = create_singlethread_workqueue("hall_switch_worker");
	flush_workqueue(ghall_switch_data->wq);	
	INIT_WORK(&ghall_switch_data->work, hall_switch_worker);


	//irq = gpio_to_irq(IRQ_GPIO_NUM);
	ret = request_irq(IRQ, hall_switch_irq_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "hall_switch", ghall_switch_data);
	if (ret < 0) {
		printk("[%s] irq[%d] request failed", __func__, irq);
		ret = -EINVAL;
		goto err_request_irq;
	}

	return 0;

err_request_irq:
	gpio_free(IRQ_GPIO_NUM);    
err_input_register_device:
	input_free_device(input_dev);
err_kzalloc:
	free_irq(IRQ, input_dev);


	return ret;
}


static void __exit hall_switch_exit(void)
{
	printk("[%s]\n", __func__);

	gpio_free(IRQ_GPIO_NUM);
	input_unregister_device(ghall_switch_data->input_dev);
	input_free_device(ghall_switch_data->input_dev);
	free_irq(IRQ, ghall_switch_data->input_dev);
}


module_init(hall_switch_init);
module_exit(hall_switch_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("hall_switch controller driver");
MODULE_AUTHOR("zengmin");









