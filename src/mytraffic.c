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
#define GPIO_RED		67
#define GPIO_YELLOW		68
#define GPIO_GREEN		44
#define GPIO_BTN0		26
#define GPIO_BTN1		46

// Structs and varibales to keep track of state
typedef enum {
	NORMAL,
	FLASH_RED,
	FLASH_YELLOW,
} traffic_mode; 

typedef enum {
	GREEN,
	YELLOW,
	RED,
} traffic_color; 

// Node to keep normal mode state
typedef struct node{
	traffic_color color;
	int cycle_len;
} node;

static traffic_mode current_mode;

static bool ped_flag;
static bool red_state;
static bool yellow_state;

static int cycle_rate;

struct timer_list timer;

struct file_operations fops = {
	.owner = THIS_MODULE;
	.read = mytraffic_read,
	.write = mytraffic_write,
};

// Function Prototypes

// --------- BUTTON 1 inturrept ----------------
/*
- sets the cycle length longer for yellow and red (IN NORMAL MODE ONLY)
- sets ped_flag to high 
- NOTE: need to inverent the cycle length changes once hit green again and set ped_flag to low
*/

// --------- BUTTON 0 inturrept ----------------
/*
- switches mode 
- if we have pointer set to green and make sure cycle length is default 
-  yellow_state and red_state variables start with them on
- if we have timer set, delete it (set new one)
*/

// -------- Function with Switch Statement (catch timer interupt) --------
/*
- called after every timer interrupt done
- reset timer and puts current led on or off (based on state and mode)
*/

// read function
static ssize_t mytraffic_read(struct file *file, char __user *buffer, size_t len, loff_t *offset);

// write function
static ssize_t mytraffic_write(struct file *file, const char __user *buffer, size_t len, loff_t *offset);

static int mytraffic_release(struct inode *inode, struct file *file)
{
    return 0;
}

static int mytraffic_open(struct inode *inode, struct file *file)
{
    return 0;
}

//set lights helper function
static void set_lights(int red, int yellow, int green);

// file operations structure

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
// free any memory to make linked list


module_init(traffic_init);
module_exit(traffic_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR();
MODULE_DESCRIPTION();








