#ifndef NXP_SIMTEMP_IOCTL_H
#define NXP_SIMTEMP_IOCTL_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/types.h>

#define SIMTEMP_IOC_MAGIC 			't'
#define SIMTEMP_IOC_SET_PERIOD 		_IOW(SIMTEMP_IOC_MAGIC, 1, int)
#define SIMTEMP_IOC_SET_THRESHOLD 	_IOW(SIMTEMP_IOC_MAGIC, 2, int)

static int dev_open(struct inode *inodep, struct file *filep);
static int dev_release(struct inode *inodep, struct file *filep);
static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset);
static ssize_t sampling_ms_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t threshold_mC_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t sampling_ms_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t threshold_mC_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

#endif // NXP_SIMTEMP_IOCTL_H
