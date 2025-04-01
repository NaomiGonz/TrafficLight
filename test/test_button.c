#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>

#define GPIO_RED    67
#define GPIO_BTN0   26

static int irq_number;

static irqreturn_t btn0_isr(int irq, void *dev_id)
{
    int value = gpio_get_value(GPIO_BTN0);
    gpio_set_value(GPIO_RED, value);  // LED mirrors button state

    printk(KERN_INFO "Button state: %d\n", value);
    return IRQ_HANDLED;
}

static int __init test_init(void)
{
    int ret;

    // Request and configure red LED
    ret = gpio_request(GPIO_RED, "red_led");
    if (ret) return ret;
    gpio_direction_output(GPIO_RED, 0);

    // Request and configure button
    ret = gpio_request(GPIO_BTN0, "btn0");
    if (ret) {
        gpio_free(GPIO_RED);
        return ret;
    }
    gpio_direction_input(GPIO_BTN0);

    // Request IRQ
    irq_number = gpio_to_irq(GPIO_BTN0);
    ret = request_irq(irq_number, btn0_isr,
                      IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
                      "btn0_irq", NULL);
    if (ret) {
        gpio_free(GPIO_BTN0);
        gpio_free(GPIO_RED);
        return ret;
    }

    printk(KERN_INFO "Test module loaded\n");
    return 0;
}

static void __exit test_exit(void)
{
    free_irq(irq_number, NULL);
    gpio_set_value(GPIO_RED, 0);
    gpio_free(GPIO_BTN0);
    gpio_free(GPIO_RED);

    printk(KERN_INFO "Test module unloaded\n");
}

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gidon Test");
MODULE_DESCRIPTION("Simple button-to-LED test module");
