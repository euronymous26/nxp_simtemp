#ifndef NXP_SIMTEMP_IOCTL_H
#define NXP_SIMTEMP_IOCTL_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>

static int dev_open(struct inode *inodep, struct file *filep);
static int dev_release(struct inode *inodep, struct file *filep);
static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset);

#endif // NXP_SIMTEMP_IOCTL_H
