#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <mach/irqs.h>
#include <linux/mfd/palmas.h>
#include "board-keenhi-t40.h"


#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>


#define PWRBUTTON	PALMAS_BASE_TO_REG(PALMAS_INTERRUPT_BASE, PALMAS_INT1_LINE_STATE)
#define PWRBUTTON_SLAVE		PALMAS_BASE_TO_SLAVE(PALMAS_INTERRUPT_BASE)

struct palmas_pwrbutton {
	unsigned int code;	/* input event code (KEY_*, SW_*) */
	unsigned int type;	/* input event type (EV_KEY, EV_SW, EV_ABS) */
	int debounce_interval;
	unsigned int irq;
	bool keypress;
	struct timer_list timer;
	struct work_struct work;
	struct input_dev *input;
};

static ssize_t powerkey_status_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	int ret;
	
	ret = palmas_powerkey_state();
//	printk("powerkey_status_show ret = %d\n",ret);
		
	 return sprintf(buf,"%d\n",ret);
}



static struct device_attribute powerkey_status_attributes[] = {
	__ATTR(power_status_control, S_IROTH, powerkey_status_show, NULL),
	__ATTR_NULL,
};

static void palmas_pwrbutton_work_func(struct work_struct *work)
{
	struct palmas_pwrbutton *palmas_pwrbutton =
		container_of(work, struct palmas_pwrbutton, work);
	int ret;

	if(palmas_pwrbutton->keypress)	{
		ret = palmas_powerkey_state();
		if(ret) {
			input_event(palmas_pwrbutton->input, palmas_pwrbutton->type, palmas_pwrbutton->code, 0);
			input_sync(palmas_pwrbutton->input);
			palmas_pwrbutton->keypress = false;
		} else {
			if (palmas_pwrbutton->debounce_interval)
				mod_timer(&palmas_pwrbutton->timer,
					jiffies + msecs_to_jiffies(palmas_pwrbutton->debounce_interval));
		}
	}
}

static void palmas_pwrbutton_timer(unsigned long _data)
{
	struct palmas_pwrbutton *palmas_pwrbutton = (struct palmas_pwrbutton *)_data;
	schedule_work(&palmas_pwrbutton->work);
}

static irqreturn_t palmas_pwrbutton_interrupt(int irq, void *pwrbutton)
{
	struct device *dev = pwrbutton;
	//struct palmas *palmas = dev_get_drvdata(dev->parent);
	struct palmas_pwrbutton *palmas_pwrbutton = dev_get_drvdata(dev);
	int ret;
	
	if(!palmas_pwrbutton->keypress) {
		ret = palmas_powerkey_state();
		if(ret == 0)
		{
			input_event(palmas_pwrbutton->input, palmas_pwrbutton->type, palmas_pwrbutton->code, 1);
			input_sync(palmas_pwrbutton->input);
			palmas_pwrbutton->keypress = true;
		}
		if (palmas_pwrbutton->debounce_interval)
			mod_timer(&palmas_pwrbutton->timer,
				jiffies + msecs_to_jiffies(palmas_pwrbutton->debounce_interval));
	}

	return IRQ_HANDLED;
}



static int __devinit irq_keys_probe(struct platform_device *pdev)
{
	struct palmas *palmas = dev_get_drvdata(pdev->dev.parent);
	struct palmas_pwrbutton *palmas_pwrbutton = NULL;
	struct palmas_platform_data *palmas_pdata = dev_get_platdata(pdev->dev.parent);
	struct palmas_pwrbutton_platform_data *pwrbutton_pdata = NULL;
	struct input_dev *input;
	int ret=0;

	if (!palmas_pdata || !palmas_pdata->pwrbutton_pdata)
		return -EINVAL;

	pwrbutton_pdata = palmas_pdata->pwrbutton_pdata;

	palmas_pwrbutton = devm_kzalloc(&pdev->dev, sizeof(struct palmas_pwrbutton),
			GFP_KERNEL);
	if (!palmas_pwrbutton) {
		return -ENOMEM;
	}

	input = input_allocate_device();
	if (!input) {
		dev_err(&pdev->dev, "error for allocate device\n");
		ret = -ENOMEM;
		goto fail1;
	}

	palmas->pwrbutton = palmas_pwrbutton;
	palmas_pwrbutton->irq= platform_get_irq(pdev, 0);
	palmas_pwrbutton->code = pwrbutton_pdata->code;
	palmas_pwrbutton->type = pwrbutton_pdata->type;
	palmas_pwrbutton->debounce_interval = pwrbutton_pdata->debounce_interval;
	palmas_pwrbutton->keypress= false;

	input->name = pdev->name;
	input->phys = "palmas-pwrbutton/input0";
	input->dev.parent = &pdev->dev;
	//input->open = gpio_keys_open;
	//input->close = gpio_keys_close;
	input->id.bustype = BUS_I2C;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0100;
	palmas_pwrbutton->input = input;
	INIT_WORK(&palmas_pwrbutton->work, palmas_pwrbutton_work_func);
	setup_timer(&palmas_pwrbutton->timer,
			    palmas_pwrbutton_timer, (unsigned long)palmas_pwrbutton);

	ret = request_threaded_irq(palmas_pwrbutton->irq, NULL,
		palmas_pwrbutton_interrupt, IRQF_TRIGGER_LOW | IRQF_ONESHOT |
		IRQF_EARLY_RESUME,
		"palmas-pwrbutton", &pdev->dev);
	if (ret < 0) {
		ret = -ENOMEM;
		goto fail1;
	}
	device_init_wakeup(&pdev->dev, 1);
	platform_set_drvdata(pdev, palmas_pwrbutton);
	input_set_capability(input, palmas_pwrbutton->type ?: EV_KEY, palmas_pwrbutton->code);


	device_create_file(palmas->dev,powerkey_status_attributes);

	
	ret = input_register_device(input);
	if (ret) {
		dev_err(&pdev->dev, "error register device\n");
		ret = -EINVAL;
		goto fail2;
	}
	return 0;

fail2:
	platform_set_drvdata(pdev, NULL);
fail1:
	input_free_device(input);
	kfree(palmas_pwrbutton);
	return ret;
}

static int __devexit irq_keys_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver irq_keys_device_driver = {
	.probe		= irq_keys_probe,
	.remove		= __devexit_p(irq_keys_remove),
	.driver		= {
		.name	= "palmas-pwrbutton",
		.owner	= THIS_MODULE,
		//.pm	= &irq_keys_pm_ops,
	}
};

static int __init keenhi_t40_irq_keys_init(void)
{
	return platform_driver_register(&irq_keys_device_driver);
}

static void __exit keenhi_t40_irq_keys_exit(void)
{
	platform_driver_unregister(&irq_keys_device_driver);
}

module_init(keenhi_t40_irq_keys_init);
module_exit(keenhi_t40_irq_keys_exit);

MODULE_AUTHOR("jimmy.tang");
MODULE_DESCRIPTION("Module for power key handler");
MODULE_LICENSE("GPL");
MODULE_ALIAS("irq_keys");
