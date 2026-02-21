#ifndef CHAR_DRIVER_H
#define CHAR_DRIVER_H

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>

#define CHAR_DEVICE_NAME "char"
#define CHAR_CLASS_NAME "char_class"
#define CHAR_NODE_NAME "char0"
#define CHAR_BUFFER_SIZE 1024

struct char_dev {
	struct cdev cdev;
	dev_t devt;
	struct class *class;
	struct device *device;
	char *buffer;
	size_t data_size;
	struct mutex lock;
};

#endif /* CHAR_DRIVER_H */
