#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/mutex.h>

// define GPIO pins
#define

// define operational modes - use enum?
// e.g., 
enum traffic_mode{}; 
// and then 
static enum traffic_mode current_mode = ...;

// init function
static int __init traffic_init(void){}
// configure LEDs and buttons using gpio_request
// to set as output: gpio_direction_output(gpio, value)
// to set as an input: gpio_direction_input(gpio

// configure interrupt requests for buttons
// gpio_to_irq(), request_irq()

// exit function
static int __exit traffic_exit(void){}
// to set value: gpio_set_value(gpio, value)
// to free: gpio_free(gpio)




// function(s?) to handle interrupts

// helper function to set light values? e.g. set_lights(0,1,1)

// function to handle modes - can use switch case?
// use kernel thread - easy to use, can just use msleep() for timing
// you define the thread as a function, then call it in the init function using kthread_run()

module_init(traffic_init);
module_exit(traffic_exit);

MODULE_LICENSE();
MODULE_AUTHOR();
MODULE_DESCRIPTION();
