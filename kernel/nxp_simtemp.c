#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/random.h>
#include <linux/timer.h>
#include <linux/cdev.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include "nxp_simtemp_ioctl.h"
#include "nxp_simtemp.h"

#define DEVICE_NAME "simtemp"
#define CLASS_NAME "temp"

// struct used to point every call system driver function (fops).
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .read = dev_read,
    .release = dev_release,
};

// struct used save values to be share with user-space.
static struct simtemp_sample current_sample = 
{
    .timestamp_ns = 0,
    .temp_mC = 0,
    .threshold_flag = 0
};

static int major;
static struct class* temp_class = NULL;
static struct device* temp_device = NULL;

static struct timer_list random_timer;
static DEFINE_MUTEX(random_mutex);

static int input_param[2] = {100, 45000};
static int array_size = 0;
module_param_array(input_param, int, &array_size, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(input_param, "Temperature period, and temperature threshold.");

/*
 * dev_read -- Function called when a process tries to open the device
 *             file, like "sudo cat /dev/simtemp".
 * 
 * @param: **struct inode *inodep** needed for function performance, 
 *                                  please check "include/linux/fs.h" 
 *                                  for more details.
 * 
 * @param: **struct file *filep** needed for function performance, 
 *                                please check "include/linux/fs.h" 
 *                                for more details.
 * 
 * @return: **int** error state feedback.
 * 
 */
static int dev_open(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "simtemp: Device opened\n");
    return 0;
}

/*
 * dev_read -- Function called when a process closes the device file.
 * 
 * @param: **struct inode *inodep** needed for function performance, 
 *                                  please check "include/linux/fs.h" 
 *                                  for more details.
 * 
 * @param: **struct file *filep** needed for function performance, 
 *                                please check "include/linux/fs.h" 
 *                                for more details.
 * 
 * @return: **int** error state feedback.
 * 
 */
static int dev_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "simtemp: Device closed\n");    
    return 0;
}

/*
 * dev_read -- Function called when a process, which already opened the 
 *             dev file, attempts to read from it.
 * 
 * @param: **struct file *filep** needed for function performance, 
 *                                please check "include/linux/fs.h" for 
 *                                more details.
 * 
 * @param: **char __user *buffer** buffer to fill with data.
 * 
 * @param: **size_t len** buffer length.
 * 
 * @param: **loff_t *offset** needed for function performance, please 
 *                            check "include/linux/fs.h" for more 
 *                            details.
 * 
 * @return **ssize_t** number of bytes into the buffer.
 * 
 */
static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset) {    
    char temp_str[100];
    int temp_len;
    struct simtemp_sample aux_sample;
    mutex_lock(&random_mutex);
    aux_sample = current_sample;
    mutex_unlock(&random_mutex);
    /* here sensor lectures */
    temp_len = snprintf(temp_str, sizeof(temp_str), "Time: %llu temp=%d °C alert=%d\n", aux_sample.timestamp_ns, aux_sample.temp_mC, aux_sample.threshold_flag);
    if (*offset >= temp_len)
        return 0;
    if (copy_to_user(buffer, temp_str, temp_len)) {
        return -EFAULT;
    }
    *offset += temp_len;
    return temp_len;
}

/*
 * temp_sensor_lectures -- Function used to generate a random temperature 
 *                         value between MIN_SIMTEMP and MAX_SIMTEMP 
 *                         limits.
 * 
 * @param: **struct simtemp_sample *sample** pointer input argument to 
 *                                           save temperature value. 
 * 
 * @return: No
 * 
 */
void temp_sensor_lectures(struct simtemp_sample *sample)
{
    unsigned int rand_val;
    get_random_bytes(&rand_val, sizeof(rand_val));
    sample->temp_mC = (rand_val % (MAX_SIMTEMP - MIN_SIMTEMP)) + MIN_SIMTEMP;
}

/*
 * threshold_evaluation -- Function used to evaluate temperature 
 *                         threshold provided by user.
 * 
 * @param: **struct simtemp_sample *sample** pointer input argument to 
 *                                           save current alarm value.
 * 
 * @return: No
 * 
 */
void threshold_evaluation(struct simtemp_sample *sample)
{
    if (sample->temp_mC > input_param[1])
    {
	    sample->threshold_flag = 1;
    }
    else
    {
	    sample->threshold_flag = 0;
    }
}

/*
 * monotonic_timestamp -- Function used to get monotonic timestamp every
 *                        period ms.
 * 
 * @param: **struct simtemp_sample *sample** pointer input argument to 
 *                                           save monotonic value.
 * 
 * @return: No 
 * 
 */
void monotonic_timestamp(struct simtemp_sample *sample)
{
    u64 monotonicts_ns;
    monotonicts_ns = ktime_get_ns();
    sample->timestamp_ns = monotonicts_ns;
}

/*
 * tempsensor_callback -- Function that is called every period (ms) 
 *                        provided by user, and generates temperature 
 *                        signal and execute its threshold evaluation.
 * 
 * @param: **struct timer_list *t** pointer input argument needed to 
 *         manage dynamic timers, and is crusial for scheduling events.
 * 
 * @return: No
 * 
 */
static void tempsensor_callback(struct timer_list *t)
{
    mutex_lock(&random_mutex);
     monotonic_timestamp(&current_sample);
     temp_sensor_lectures(&current_sample);
     threshold_evaluation(&current_sample);
    mutex_unlock(&random_mutex);
    // Timer reset for next period.
    mod_timer(&random_timer, jiffies + msecs_to_jiffies(input_param[0]));
    printk(KERN_INFO "Time: %llu temp=%d °C alert=%d, period=%d threshold=%d\n", current_sample.timestamp_ns, current_sample.temp_mC, current_sample.threshold_flag, input_param[0], input_param[1]);
}

/*
 * nxp_simtemp_init -- Function to initialize all internal dependencies 
 *                     for kernel module nxp_simtemp.
 * 
 * @param: No
 * 
 * @return: **int** value to be returned from function to indicate if 
 *                  an error occurred during initialization process.
 * 
 */
static int __init nxp_simtemp_init(void)
{    
	// Allocate a major number
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        printk(KERN_ALERT "temp_sensor: Failed to register major number\n");
        return major;
    }
    // Register device class
    temp_class = class_create(CLASS_NAME);
    if (IS_ERR(temp_class)) {
        unregister_chrdev(major, DEVICE_NAME);
        printk(KERN_ALERT "temp_sensor: Failed to register device class\n");
        return PTR_ERR(temp_class);
    }

    // Register device driver
    temp_device = device_create(temp_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(temp_device)) {
        class_destroy(temp_class);
        unregister_chrdev(major, DEVICE_NAME);
        printk(KERN_ALERT "temp_sensor: Failed to create device\n");
        return PTR_ERR(temp_device);
    }    
    timer_setup(&random_timer, tempsensor_callback, 0);
    mod_timer(&random_timer, jiffies + msecs_to_jiffies(input_param[0]));
    printk(KERN_INFO "NXP temperature driver sensor initialized.\n");
    return 0;
}

/*
 * nxp_simtemp_exit -- Function to destroy all internal depdenecies 
 *                     used for kernel module nxp_simtemp.
 * 
 * @param: No
 * 
 * @return: No
 * 
 */
static void __exit nxp_simtemp_exit(void)
{
    del_timer(&random_timer);
    device_destroy(temp_class, MKDEV(major, 0));
    class_unregister(temp_class);
    class_destroy(temp_class);
    unregister_chrdev(major, DEVICE_NAME);	
    printk(KERN_INFO "NXP temperature sensor removed.\n");
}


module_init(nxp_simtemp_init);
module_exit(nxp_simtemp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Enrique Mireles Rodriguez");
MODULE_DESCRIPTION("NXP system that simulates a hardware sensor in the linux kernel and exposes it to user-space using an app to configure/read it.");
MODULE_VERSION("1.0");
