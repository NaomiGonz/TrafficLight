#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/uaccess.h>


// define GPIO pins
#define GPIO_RED        67
#define GPIO_YELLOW     68
#define GPIO_GREEN      44
#define GPIO_BTN0       26
#define GPIO_BTN1       46

// Structs and varibales to keep track of state
typedef enum {
    NORMAL,
    FLASH_RED,
    FLASH_YELLOW,
    ALL_ON
} traffic_mode; 

typedef enum {
    GREEN = 0,
    YELLOW = 1,
    RED = 2,
} traffic_color; 

static traffic_mode current_mode;

static bool ped_flag;
static bool red_state;
static bool yellow_state;
static int normal_color;    // current color 
static int norm_len[3] = {3, 1, 2}; // cycle length in normal mode {green, yellow, red}

static bool button0_pressed = false;
static bool button1_pressed = false;

static int cycle_rate;

struct timer_list timer;

static spinlock_t lock;

// FUNCTION PROTOTYPES ------------------------------------

static int __init mytraffic_init(void);
static void __exit mytraffic_exit(void);
static int mytraffic_release(struct inode *inode, struct file *file);
static int mytraffic_open(struct inode *inode, struct file *file);
static void set_lights(int red, int yellow, int green); // helper function to set lights
static void sw_mode(struct timer_list *t);
static ssize_t mytraffic_read(struct file *file, char __user *buffer, size_t len, loff_t *offset);
static ssize_t mytraffic_write(struct file *file, const char __user *buffer, size_t len, loff_t *offset);
static void update_mode_on_buttons(void);


struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = mytraffic_read,
    .write = mytraffic_write,
    .release = mytraffic_release,
    .open = mytraffic_open,
};


// FUNCTION IMPLEMENTATIONS ------------------------------------

// Function to check if both buttons are pressed
static void update_mode_on_buttons(void){
	bool both_pressed = button0_pressed && button1_pressed;

    if(both_pressed && current_mode != ALL_ON) {
        del_timer(&timer);
        current_mode = ALL_ON;
        set_lights(1, 1, 1);
    }
     else if(!both_pressed && current_mode == ALL_ON) {
     	// Resst to normal mode
        current_mode = NORMAL;
        norm_len[GREEN] = 3; norm_len[YELLOW] = 1; norm_len[RED] = 2;
        ped_flag = false;
        cycle_rate = 1;
        normal_color = GREEN;
        set_lights(0, 0, 1);
        mod_timer(&timer, jiffies + norm_len[GREEN] * (HZ / cycle_rate));
    }
}

// --------- BUTTON 1 interrupt ----------------
static irqreturn_t button1_isr(int irq, void *dev_id) {
    unsigned long flags;
    traffic_mode prev_mode;

    spin_lock_irqsave(&lock, flags);

    // Save prev mode 
    prev_mode = current_mode;

    // read button value
    button1_pressed = gpio_get_value(GPIO_BTN1); 
    update_mode_on_buttons();

    printk(KERN_INFO "BUTTON1_ISR- button0_pressed: %d button1_pressed: %d\n", button0_pressed, button1_pressed);

    // If prev mode ALL_ON => skip single-press logic
    if (prev_mode == ALL_ON && current_mode == NORMAL) {
        spin_unlock_irqrestore(&lock, flags);
        return IRQ_HANDLED;
    }
    
    if(current_mode != ALL_ON && button1_pressed && !button0_pressed) {
        if(current_mode == NORMAL) {
            norm_len[YELLOW] = 5;
            norm_len[RED] = 5;
            ped_flag = true;
        }
    }
    spin_unlock_irqrestore(&lock, flags);
    return IRQ_HANDLED;
}

// --------- BUTTON 0 interrupt ----------------
static irqreturn_t button0_isr(int irq, void *dev_id){
    unsigned long flags;
    traffic_mode prev_mode;

    spin_lock_irqsave(&lock, flags);

    // Save prev mode 
    prev_mode = current_mode;

    // read button value
    button0_pressed = gpio_get_value(GPIO_BTN0);
    update_mode_on_buttons();

    printk(KERN_INFO "BUTTON0_ISR- button0_pressed: %d button1_pressed: %d\n", button0_pressed, button1_pressed);

    // If prev mode ALL_ON => skip single-press logic
    if (prev_mode == ALL_ON && current_mode == NORMAL) {
        spin_unlock_irqrestore(&lock, flags);
        return IRQ_HANDLED;
    }

    // Toggle to next mode if single button pressed
    if(current_mode != ALL_ON && button0_pressed && !button1_pressed){
    	del_timer(&timer);
        switch (current_mode) {
            case NORMAL:
            	// Change to flashing red mode
                current_mode = FLASH_RED;
                red_state = true;
                // Set LED and new timer
                set_lights(1, 0, 0);
                mod_timer(&timer, jiffies + (HZ / cycle_rate));
                break;

            case FLASH_RED:
            	// Change to flashing yellow mode
                current_mode = FLASH_YELLOW;
                yellow_state = true;
                // Set LED and new timer
                set_lights(0, 1, 0);
                mod_timer(&timer, jiffies + (HZ / cycle_rate));
                break;

            case FLASH_YELLOW:
            default:
            	// Change to normal mode
                current_mode = NORMAL;
                red_state = false;
                yellow_state = false;
                // Set LED and new timer
                normal_color = GREEN;
                set_lights(0, 0, 1);
                mod_timer(&timer, jiffies + norm_len[GREEN] * (HZ / cycle_rate));
                break;
        }
    }

    spin_unlock_irqrestore(&lock, flags);
    return IRQ_HANDLED;
}


// Function that runs when timer expires
void sw_mode(struct timer_list *t){
    unsigned long new_time;
    unsigned long flags;

    spin_lock_irqsave(&lock, flags);
    // Move to next state based on what mode the device is in
    switch (current_mode) {

    case NORMAL: {
        normal_color = (normal_color < 2) ? (normal_color + 1) : GREEN;

        // Turn on next LED
        set_lights((normal_color == RED), (normal_color == YELLOW), (normal_color == GREEN));

        // Set new timer expiration time
        mod_timer(&timer, jiffies + norm_len[normal_color] * (HZ / cycle_rate));

        // Reset pedestrian settings
        if (normal_color == GREEN && ped_flag) { 
                norm_len[YELLOW] = 1;
                norm_len[RED] = 2;
                ped_flag = false;
         }

        break;
    }
    case FLASH_RED:
        // toggle LED
        red_state = !red_state;
        set_lights(red_state, 0, 0);

        // Set new timer expiration time 
        mod_timer(&timer, jiffies + (HZ / cycle_rate));
        break;

    case FLASH_YELLOW:
        // toggle LED
        yellow_state = !yellow_state;
        set_lights(0, yellow_state, 0);

        // Set new timer expiration time 
        mod_timer(&timer, jiffies + (HZ / cycle_rate));
        break;

    case ALL_ON:
        break;
    default:
        printk(KERN_INFO "ERROR: unknown mode\n");
        // fallback
        set_lights(0, 0, 1);
        new_time = jiffies + norm_len[GREEN] * (HZ / cycle_rate);
    }
    
    spin_unlock_irqrestore(&lock, flags);

}

// helper function for read functionality (converts mode to a string)
static const char* mode_to_str(traffic_mode mode){
    switch(mode) {
        case NORMAL: return "Normal";
        case FLASH_RED: return "Flashing-red";
        case FLASH_YELLOW: return "Flashing-yellow";
        case ALL_ON: return "All-on";
        default: return "unknown state";
    }
}

// read function
static ssize_t mytraffic_read(struct file *file, char __user *buffer, size_t len, loff_t *offset){
    char kbuf[128];
    int written = 0;

    // limit reading to avoid infinite reads
    if(*offset > 0) return 0;

    spin_lock(&lock);

    // format and generate output

    //mode
    written += snprintf(kbuf + written, sizeof(kbuf) - written,
            "Mode: %s\n", mode_to_str(current_mode));

    // cycle rate
    written += snprintf(kbuf + written, sizeof(kbuf) - written,
            "Cycle rate: %d\n", cycle_rate);

    // LED status
    switch(current_mode){
        case FLASH_RED:
        written += snprintf(kbuf + written, sizeof(kbuf) - written,
                    "Light status: Red %s, Yellow %s, Green %s\n",
                    red_state ? "on" : "off",
                    gpio_get_value(GPIO_YELLOW) ? "on" : "off",
                    gpio_get_value(GPIO_GREEN) ? "on" : "off"
        ); break;
        case FLASH_YELLOW:
        written += snprintf(kbuf + written, sizeof(kbuf) - written,
            "Light status: Red %s, Yellow %s, Green %s\n",
            gpio_get_value(GPIO_RED) ? "on" : "off",
            yellow_state ? "on" : "off",
            gpio_get_value(GPIO_GREEN) ? "on" : "off"
        ); break;
        default:
        written += snprintf(kbuf + written, sizeof(kbuf) - written,
            "Light status: Red %s, Yellow %s, Green %s\n",
            gpio_get_value(GPIO_RED) ? "on" : "off",
            gpio_get_value(GPIO_YELLOW) ? "on" : "off",
            gpio_get_value(GPIO_GREEN) ? "on" : "off"
        ); break;
    }
    
    // pedestrian waiting?
    if(ped_flag) {
        written += snprintf(kbuf + written, sizeof(kbuf) - written,
                "Pedestrian: Present\n");
    } else {
        written += snprintf(kbuf + written, sizeof(kbuf) - written,
                "Pedestrian: Not present\n");
    }

    spin_unlock(&lock);

    if(copy_to_user(buffer, kbuf, written)) return -EFAULT;

    *offset += written;
    return written;
}

// write function
static ssize_t mytraffic_write(struct file *file, const char __user *buffer, size_t len, loff_t *offset){
    char kbuf[10];
    int new_rate;

    if(copy_from_user(kbuf, buffer, len)) return -EFAULT;

    kbuf[len] = '\0'; // null terminate user input string

    if(kstrtoint(kbuf, 10, &new_rate) == 0){
        if(new_rate >= 1 && new_rate <= 9){
            unsigned long flag;
            spin_lock_irqsave(&lock, flag);
            cycle_rate = new_rate;
            spin_unlock_irqrestore(&lock, flag);
        } else {
            printk(KERN_ERR "ERROR: invalid new cycle rate\n");
        }
    } else {
        printk(KERN_ERR "ERROR: invalid new cycle rate\n");
    }

    return len;
}


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
    if((red != 0 && red != 1) || (yellow != 0 && yellow != 1) || (green != 0 && green != 1)){
        printk(KERN_ERR "invalid values for set_lights\n");
        return;
    }

    gpio_set_value(GPIO_RED,red);
    gpio_set_value(GPIO_YELLOW,yellow);
    gpio_set_value(GPIO_GREEN,green);
}

// init function
static int __init mytraffic_init(void){

    int retcheck;

    // register device
    retcheck = register_chrdev(61, "mytraffic", &fops);
    if(retcheck < 0){
        printk(KERN_INFO "ERROR: failed to register character device\n");
        return -1;
    }

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

    // init lock 
    spin_lock_init(&lock);
    
    // initialize interrupt requests
    retcheck = request_irq(gpio_to_irq(GPIO_BTN0), button0_isr, 
            IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "btn0", NULL);
    if (retcheck) {
        printk(KERN_ERR "ERROR: Failed to register IRQ for BTN0\n");
        goto irq_fail;
    }

    retcheck = request_irq(gpio_to_irq(GPIO_BTN1), button1_isr, 
            IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "btn1", NULL);
    if (retcheck) {
        printk(KERN_ERR "ERROR: Failed to register IRQ for BTN1\n");
        free_irq(gpio_to_irq(GPIO_BTN0), NULL);
        goto irq_fail;
    }

    // set the default cycle rate of 1
    cycle_rate = 1;

    // set normal_color to GREEN and mode to NORMAL
    normal_color = GREEN;
    current_mode = NORMAL;

    // Set other states/flags to false
    ped_flag = false;
    red_state = false;
    yellow_state = false;
    button0_pressed = false;
    button1_pressed = false;

    // set the green LED to be on
    set_lights(0,0,1);

    // set a timer for correct number of cycles (timer_setup and mod_timer)
    timer_setup(&timer, sw_mode, 0);
    mod_timer(&timer, jiffies + norm_len[GREEN] * (HZ / cycle_rate));

    return 0;

    // initialization failure handling

green_fail:
    printk(KERN_ERR "Green LED failed to initialize\n");
    gpio_free(GPIO_GREEN);
    return -1;
yellow_fail:
    printk(KERN_ERR "Yellow LED failed to initialize\n");
    gpio_free(GPIO_YELLOW);
    gpio_free(GPIO_GREEN);
    return -1;
red_fail:
    printk(KERN_ERR "Red LED failed to initialize\n");
    gpio_free(GPIO_RED);
    gpio_free(GPIO_YELLOW);
    gpio_free(GPIO_GREEN);
    return -1;
btn0_fail:
    printk(KERN_ERR "Btn 0 failed to initialize\n");
    gpio_free(GPIO_BTN0);
    gpio_free(GPIO_RED);
    gpio_free(GPIO_YELLOW);
    gpio_free(GPIO_GREEN);
    return -1;
btn1_fail:
    printk(KERN_ERR "Btn 1 failed to initialize\n");
    gpio_free(GPIO_BTN1);
    gpio_free(GPIO_BTN0);
    gpio_free(GPIO_RED);
    gpio_free(GPIO_YELLOW);
    gpio_free(GPIO_GREEN);
    return -1;
irq_fail:
    gpio_free(GPIO_BTN1);
    gpio_free(GPIO_BTN0);
    gpio_free(GPIO_RED);
    gpio_free(GPIO_YELLOW);
    gpio_free(GPIO_GREEN);
    free_irq(gpio_to_irq(GPIO_BTN0), NULL);
    return -1;
}


// exit function
static void __exit mytraffic_exit(void){

    //turn off all lights
    set_lights(0,0,0);

    // delete timer
    del_timer_sync(&timer);

    // free GPIO devices
    gpio_free(GPIO_BTN0);
    gpio_free(GPIO_BTN1);
    gpio_free(GPIO_GREEN);
    gpio_free(GPIO_YELLOW);
    gpio_free(GPIO_RED);

    free_irq(gpio_to_irq(GPIO_BTN0), NULL);
    free_irq(gpio_to_irq(GPIO_BTN1), NULL);

    //unregister character device
    unregister_chrdev(61,"mytraffic");
}
// to set value: gpio_set_value(gpio, value)
// to free: gpio_free(gpio)
// free any memory 
// delete timer 


module_init(mytraffic_init);
module_exit(mytraffic_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Naomi Gonzalez, Gidon Gautel");
MODULE_DESCRIPTION("traffic light controller");


// watch cat /dev/mytraffic
// echo 2 > /dev/mytraffic
// cat /dev/mytraffic
// for f in 1 3 5 7 9 1; do echo $f | tee /dev/mytraffic; sleep 3; done






