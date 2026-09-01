#ifndef TEMPSENSOR_IOCTL_H
#define TEMPSENSOR_IOCTL_H

#ifdef __KERNEL__
#include <linux/ioctl.h>
#else
#include <sys/ioctl.h>
#endif

#define TEMP_IOC_MAGIC 't'

// reset to 25.0
#define TEMP_IOC_RESET _IO(TEMP_IOC_MAGIC, 1)

// drift config
#define TEMP_IOC_SET_DRIFT _IOW(TEMP_IOC_MAGIC, 2, int)

#endif
