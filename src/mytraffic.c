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
	.release = mytraffic_release,
	.open = mytraffic_open,
};

// FUNCTION PROTOTYPES ------------------------------------

static int __init mytraffic_init(void);
static int __exit mytraffic_exit(void);
static int mytraffic_release(struct inode *inode, struct file *file);
static int mytraffic_open(struct inode *inode, struct file *file);
static void set_lights(int red, int yellow, int green); // helper function to set lights



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



// FUNCTION IMPLEMENTATIONS ------------------------------------

static int mytraffic_release(struct inode *inode, struct file *file)
{
    return 0;
}

static int mytraffic_open(struct inode *inode, struct file *file)
{
    return 0;
}

// set lights helper function
// example usage: set_lights(1,0,0) would light up the red LED, and turn off the green and yellow LED
static void set_lights(int red, int yellow, int green){
	if((red != 0 | red != 1) | (yellow != 0 | yellow != 1) (green != 0 | green != 1)){
		printk("invalid values for set_lights");
		return -EINVAL;
	}

	gpio_set_value(GPIO_RED,red);
	gpio_set_value(GPIO_YELLOW,yellow);
	gpio_set_value(GPIO_GREEN,green);
}

// init function
static int __init mytraffic_init(void){

	int retcheck;

	// initialize LED gpio pins

	retcheck = gpio_request(GPIO_GREEN, "green_led");
	if(retcheck) goto green_fail;
	gpio_direction_output(GPIO_GREEN,0);

	retcheck = gpio_request(GPIO_YELLOW, "yellow_led");
	if(retcheck) goto yellow_fail;
	gpio_direction_output(GPIO_YELLOW,0);

	retcheck = gpio_request(GPIO_RED, "red_led");
	if(retcheck) goto red_fail;
	gpio_direction_output(GPIO_RED,0);

	// initialized button gpio pins

	retcheck = gpio_request(GPIO_BTN0, "btn0");
	if(retcheck) goto btn0_fail;
	gpio_direction_input(GPIO_BTN0);

	retcheck = gpio_request(GPIO_BTN1, "btn1");
	if(retcheck) goto btn1_fail;
	gpio_direction_input(GPIO_BTN1);

	// initialize states



	// initialization failure handling

	green_fail:
		gpio_free(GPIO_GREEN);
	yellow_fail:
		gpio_free(GPIO_YELLOW);
	red_fail:
		gpio_free(GPIO_RED);
	btn0_fail:
		gpio_free(GPIO_BTN0);
	btn1_fail:
		gpio_free(BTN1);

}


// exit function
static int __exit mytraffic_exit(void){
	set_lights(0,0,0);
	gpio_free(GPIO_BTN0);
	gpio_free(GPIO_BTN1);
	gpio_free(GPIO_GREEN);
	gpio_free(GPIO_YELLOW);
	gpio_free(GPIO_RED);
}
// to set value: gpio_set_value(gpio, value)
// to free: gpio_free(gpio)
// free any memory to make linked list


module_init(mytraffic_init);
module_exit(mytraffic_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR();
MODULE_DESCRIPTION();








