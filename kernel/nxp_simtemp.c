#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/random.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/cdev.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include "nxp_simtemp_ioctl.h"
#include "nxp_simtemp.h"

// struct used to point every call system driver function (fops).
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .read = dev_read,
    .release = dev_release,
    .unlocked_ioctl = dev_ioctl
};

// struct used save values to be share with user-space.
static struct simtemp_sample current_sample = 
{
    .timestamp_ns = 0,
    .temp_mC = 0,
    .threshold_flag = 0
};

static int sampling_ms = 100;
static int threshold_mC = 45000;
static struct class *temp_class = NULL;
static struct device *temp_device = NULL;
static struct cdev simtemp_cdev;
static dev_t dev_num;

static struct timer_list random_timer;
static DEFINE_MUTEX(random_mutex);

static DEVICE_ATTR_RW(sampling_ms);
static DEVICE_ATTR_RW(threshold_mC);

module_param(sampling_ms, int, S_IRUGO | S_IWUSR);
module_param(threshold_mC, int, S_IRUGO | S_IWUSR);

MODULE_PARM_DESC(sampling_ms, "Temperature period ms.");
MODULE_PARM_DESC(threshold_mC, "Temperature threshold mC.");

//// sys fs
static ssize_t sampling_ms_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", sampling_ms);
}

static ssize_t sampling_ms_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int val;
    if (kstrtoint(buf, 10, &val))
        return -EINVAL;
    mutex_lock(&random_mutex);
    sampling_ms = val;
    mutex_unlock(&random_mutex);
    return count;
}

static ssize_t threshold_mC_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", threshold_mC);
}

static ssize_t threshold_mC_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int val;
    if (kstrtoint(buf, 10, &val))
        return -EINVAL;
    mutex_lock(&random_mutex);
    threshold_mC = val;
    mutex_unlock(&random_mutex);
    return count;
}



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
    temp_len = snprintf(temp_str, sizeof(temp_str), " %llu temp=%d °C alert=%d\n", aux_sample.timestamp_ns, aux_sample.temp_mC, aux_sample.threshold_flag);
    if (*offset >= temp_len)
        return 0;
    if (copy_to_user(buffer, temp_str, temp_len)) {
        return -EFAULT;
    }
    *offset += temp_len;
    return temp_len;
}

static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int val;
    switch (cmd) {
    case SIMTEMP_IOC_SET_PERIOD:
        if (copy_from_user(&val, (int __user *)arg, sizeof(int)))
            return -EFAULT;
        sampling_ms = val;
        pr_info("simtemp: period_ms updated to %d ms\n", sampling_ms);
        break;
    case SIMTEMP_IOC_SET_THRESHOLD:
        if (copy_from_user(&val, (int __user *)arg, sizeof(int)))
            return -EFAULT;
        threshold_mC = val;
        pr_info("simtemp: temperature threshold updated to %d°C\n", threshold_mC);
        break;
    default:
        return -EINVAL;
    }
    return 0;
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
    if (sample->temp_mC > threshold_mC)
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
    mod_timer(&random_timer, jiffies + msecs_to_jiffies(sampling_ms));
    printk(KERN_INFO "Time: %llu temp=%d °C alert=%d, period=%d threshold=%d\n", current_sample.timestamp_ns, current_sample.temp_mC, current_sample.threshold_flag, sampling_ms, threshold_mC);
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
	int ret;
	// Allocate a major number
    /*major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        printk(KERN_ALERT "temp_sensor: Failed to register major number\n");
        return major;
    }*/
    
    mutex_init(&random_mutex);
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) return ret;
    
    ret = register_chrdev(0, DEVICE_NAME, &fops);
    if (ret < 0) {
        printk(KERN_ALERT "temp_sensor: Failed to register major number\n");
        return ret;
    }
    
    cdev_init(&simtemp_cdev, &fops);
    cdev_add(&simtemp_cdev, dev_num, 1);
    
    // Register device class
    temp_class = class_create(CLASS_NAME);
    if (IS_ERR(temp_class)) {
        unregister_chrdev(dev_num, DEVICE_NAME);
        printk(KERN_ALERT "temp_sensor: Failed to register device class\n");
        return PTR_ERR(temp_class);
    }

    // Register device driver
    temp_device = device_create(temp_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(temp_device)) {
        class_destroy(temp_class);
        unregister_chrdev(dev_num, DEVICE_NAME);
        printk(KERN_ALERT "temp_sensor: Failed to create device\n");
        return PTR_ERR(temp_device);
    }
    device_create_file(temp_device, &dev_attr_sampling_ms);
    device_create_file(temp_device, &dev_attr_threshold_mC);
    
    timer_setup(&random_timer, tempsensor_callback, 0);
    mod_timer(&random_timer, jiffies + msecs_to_jiffies(sampling_ms));
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
    device_remove_file(temp_device, &dev_attr_sampling_ms);
    device_remove_file(temp_device, &dev_attr_threshold_mC);
    device_destroy(temp_class, dev_num);
    class_unregister(temp_class);
    class_destroy(temp_class);
    cdev_del(&simtemp_cdev);
    unregister_chrdev(dev_num, DEVICE_NAME);	
    printk(KERN_INFO "NXP temperature sensor removed.\n");
}


module_init(nxp_simtemp_init);
module_exit(nxp_simtemp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Enrique Mireles Rodriguez");
MODULE_DESCRIPTION("NXP system that simulates a hardware sensor in the linux kernel and exposes it to user-space using an app to configure/read it.");
MODULE_VERSION("1.0");
