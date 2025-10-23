#ifndef NXP_SIMTEMP_H
#define NXP_SIMTEMP_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>

#define MIN_SIMTEMP -40000
#define MAX_SIMTEMP 150000

struct simtemp_sample{
	__u64 timestamp_ns;
	__s32 temp_mC;
	__u32 threshold_flag : 1;
} __attribute__((packed));

static void tempsensor_callback(struct timer_list *t);
void temp_sensor_lectures(struct simtemp_sample *sample);
void threshold_evaluation(struct simtemp_sample *sample);
void monotonic_timestamp(struct simtemp_sample *sample);

#endif // NXP_SIMTEMP_H
