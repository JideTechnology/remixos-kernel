#include <linux/module.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>
#include <linux/workqueue.h>

#include <asm/gpio.h>
#include "../../../arch/arm/mach-tegra/gpio-names.h"
#include <linux/iio/consumer.h>
#include <linux/iio/types.h>
struct gpio_button_data {
    struct gpio_keys_button *button;
    struct input_dev *input;
    struct timer_list timer;
    struct work_struct work;
};

struct gpio_keys_drvdata {
	bool send_sw_lid;
	bool first_sw_lid;
    struct input_dev *input;
    struct gpio_button_data data[0];
};
struct gpio_keys_drvdata *drdate = NULL;
void gpio_keys_send_event(void)
{
	if (drdate == NULL)
		return;

	if (drdate->first_sw_lid) {
		drdate->first_sw_lid = false;
		return;
	}

	if (drdate->send_sw_lid) {
		input_event(drdate->input, EV_SW, SW_LID, 0);
		input_sync(drdate->input);
		drdate->send_sw_lid = false;
	}
}

static void gpio_keys_report_event(struct work_struct *work)
{
    struct gpio_button_data *bdata =
        container_of(work, struct gpio_button_data, work);
    struct gpio_keys_button *button = bdata->button;
    struct input_dev *input = bdata->input;
    unsigned int type = button->type ?: EV_KEY;
    int state = (gpio_get_value(button->gpio) ? 1 : 0) ^ button->active_low;
	if (state) {
		input_event(input, type, button->code, !!state);
		input_sync(input);
		drdate->send_sw_lid = true;
	}
}

static void gpio_keys_timer(unsigned long _data)
{
    struct gpio_button_data *data = (struct gpio_button_data *)_data;

    schedule_work(&data->work);
}

static irqreturn_t gpio_keys_isr(int irq, void *dev_id)
{
    struct gpio_button_data *bdata = dev_id;
    struct gpio_keys_button *button = bdata->button;

    BUG_ON(irq != gpio_to_irq(button->gpio));

    if (button->debounce_interval) 
        mod_timer(&bdata->timer,
            jiffies + msecs_to_jiffies(button->debounce_interval));
    else
        schedule_work(&bdata->work);

    return IRQ_HANDLED;
}

static int __devinit gpio_keys_probe(struct platform_device *pdev)
{
    struct gpio_keys_platform_data *pdata = pdev->dev.platform_data;
    struct gpio_keys_drvdata *ddata;
    struct input_dev *input;
    int i, error;
    int wakeup = 0;

    ddata = kzalloc(sizeof(struct gpio_keys_drvdata) +
            pdata->nbuttons * sizeof(struct gpio_button_data),
            GFP_KERNEL);
    input = input_allocate_device();
    if (!ddata || !input) {
        error = -ENOMEM;
        goto fail1;
    }

    platform_set_drvdata(pdev, ddata);

    input->name = pdev->name;
    input->phys = "sw-key/input0";
    input->dev.parent = &pdev->dev;

    input->id.bustype = BUS_HOST;
    input->id.vendor = 0x0001;
    input->id.product = 0x0001;
    input->id.version = 0x0100;

    /* Enable auto repeat feature of Linux input subsystem */
    if (pdata->rep)
        __set_bit(EV_REP, input->evbit); 

    ddata->input = input;
	drdate = ddata;

    for (i = 0; i < pdata->nbuttons; i++) {
        struct gpio_keys_button *button = &pdata->buttons[i];
        struct gpio_button_data *bdata = &ddata->data[i];
        int irq;
        unsigned int type = button->type ?: EV_KEY;

        bdata->input = input;
        bdata->button = button;
        setup_timer(&bdata->timer,
             gpio_keys_timer, (unsigned long)bdata);
        INIT_WORK(&bdata->work, gpio_keys_report_event);

		error  = gpio_request(button->gpio, "sw_key");
		if (error) {
            pr_err("failed to request GPIO%d\n", button->gpio);
            goto fail2;
		}

        error = gpio_direction_input(button->gpio);
        if (error < 0) {
            pr_err("sw-key: failed to configure input"
                " direction for GPIO %d, error %d\n",
                button->gpio, error);
            gpio_free(button->gpio);
            goto fail2;
        }

        irq = gpio_to_irq(button->gpio);
        if (irq < 0) {
            error = irq;
            pr_err("sw-key: Unable to get irq number"
                " for GPIO %d, error %d\n",
                button->gpio, error);
            gpio_free(button->gpio);
            goto fail2;
        }

        error = request_irq(irq, gpio_keys_isr,
                 IRQF_SHARED |
                 IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                 button->desc ? button->desc : "gpio_keys",
                 bdata);
        if (error) {
            pr_err("sw-key: Unable to claim irq %d; error %d\n",
                irq, error);
            gpio_free(button->gpio);
            goto fail2;
        }

        if (button->wakeup)
            wakeup = 1;

		input_set_capability(input, type, button->code);

	}

    error = input_register_device(input);
    if (error) {
        pr_err("sw-key: Unable to register input device, "
            "error: %d\n", error);
        goto fail2;
	}

	for (i = 0; i < pdata->nbuttons; i++) {
		unsigned int type = pdata->buttons[i].type ?: EV_KEY;
		int state = (gpio_get_value(pdata->buttons[i].gpio) ? 1 : 0) ^ pdata->buttons[i].active_low;
		if (pdata->buttons[i].code == SW_LID && state) {
			input_event(input, type, pdata->buttons[i].code, !!state);
			input_sync(input);
			drdate->send_sw_lid = true;
			drdate->first_sw_lid = true;
		}
	}

	device_init_wakeup(&pdev->dev, wakeup);

    return 0;

 fail2:
    while (--i >= 0) {
        free_irq(gpio_to_irq(pdata->buttons[i].gpio), &ddata->data[i]);
        if (pdata->buttons[i].debounce_interval)
            del_timer_sync(&ddata->data[i].timer);
        cancel_work_sync(&ddata->data[i].work);
        gpio_free(pdata->buttons[i].gpio);
    }

    platform_set_drvdata(pdev, NULL);
 fail1:
    input_free_device(input);
	drdate = NULL;
    kfree(ddata);

    return error;
}

static int __devexit gpio_keys_remove(struct platform_device *pdev)
{
    struct gpio_keys_platform_data *pdata = pdev->dev.platform_data;
    struct gpio_keys_drvdata *ddata = platform_get_drvdata(pdev);
    struct input_dev *input = ddata->input;
    int i;

    device_init_wakeup(&pdev->dev, 0);

    for (i = 0; i < pdata->nbuttons; i++) {
        int irq = gpio_to_irq(pdata->buttons[i].gpio);
        free_irq(irq, &ddata->data[i]);
        if (pdata->buttons[i].debounce_interval)
            del_timer_sync(&ddata->data[i].timer);
        cancel_work_sync(&ddata->data[i].work);
        gpio_free(pdata->buttons[i].gpio);
    }

    input_unregister_device(input);

    return 0;
}


#ifdef CONFIG_PM
static int gpio_keys_suspend(struct device *dev)
{
    struct platform_device *pdev = to_platform_device(dev);
    struct gpio_keys_platform_data *pdata = pdev->dev.platform_data;
    int i;

    if (device_may_wakeup(&pdev->dev)) {
        for (i = 0; i < pdata->nbuttons; i++) {
            struct gpio_keys_button *button = &pdata->buttons[i];
            if (button->wakeup) {
                int irq = gpio_to_irq(button->gpio);
                enable_irq_wake(irq);
            }
        }
    }

    return 0;
}

static int gpio_keys_resume(struct device *dev)
{
    struct platform_device *pdev = to_platform_device(dev);
    struct gpio_keys_platform_data *pdata = pdev->dev.platform_data;
    int i;

    if (device_may_wakeup(&pdev->dev)) {
        for (i = 0; i < pdata->nbuttons; i++) {
            struct gpio_keys_button *button = &pdata->buttons[i];
            if (button->wakeup) {
                int irq = gpio_to_irq(button->gpio);
                disable_irq_wake(irq);
            }
        }
    }

    return 0;
}

static const struct dev_pm_ops gpio_keys_pm_ops = {
    .suspend = gpio_keys_suspend,
    .resume = gpio_keys_resume,
};
#endif

static struct platform_driver gpio_keys_device_driver = {
    .probe = gpio_keys_probe,
    .remove = __devexit_p(gpio_keys_remove),
    .driver = {
        .name = "sw-key",
        .owner = THIS_MODULE,
#ifdef CONFIG_PM
        .pm = &gpio_keys_pm_ops,
#endif
    }
};

static int __init gpio_keys_init(void)
{
    return platform_driver_register(&gpio_keys_device_driver);
}

static void __exit gpio_keys_exit(void)
{
    platform_driver_unregister(&gpio_keys_device_driver);
}

late_initcall(gpio_keys_init);
module_exit(gpio_keys_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Phil Blundell <pb@handhelds.org>");
MODULE_DESCRIPTION("Keyboard driver for CPU GPIOs");
MODULE_ALIAS("platform:sw-key");
