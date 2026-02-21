#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "char_driver.h"

static struct char_dev g_dev;

static int char_driver_open(struct inode *inode, struct file *file)
{
	file->private_data = &g_dev;
	pr_info("char: device opened\n");
	return 0;
}

static int char_driver_release(struct inode *inode, struct file *file)
{
	pr_info("char: device closed\n");
	return 0;
}

static ssize_t char_driver_read(struct file *file, char __user *buf, size_t count,
				loff_t *ppos)
{
	struct char_dev *dev = file->private_data;
	size_t bytes_to_copy;

	if (!buf)
		return -EINVAL;

	mutex_lock(&dev->lock);

	if (*ppos >= dev->data_size) {
		mutex_unlock(&dev->lock);
		return 0;
	}

	bytes_to_copy = min(count, dev->data_size - (size_t)*ppos);
	if (copy_to_user(buf, dev->buffer + *ppos, bytes_to_copy)) {
		mutex_unlock(&dev->lock);
		pr_err("char: copy_to_user failed\n");
		return -EFAULT;
	}

	*ppos += bytes_to_copy;
	mutex_unlock(&dev->lock);

	pr_info("char: read %zu bytes\n", bytes_to_copy);
	return bytes_to_copy;
}

static ssize_t char_driver_write(struct file *file, const char __user *buf,
				 size_t count, loff_t *ppos)
{
	struct char_dev *dev = file->private_data;
	size_t bytes_to_copy;

	if (!buf)
		return -EINVAL;

	if (*ppos >= CHAR_BUFFER_SIZE)
		return -ENOSPC;

	mutex_lock(&dev->lock);

	bytes_to_copy = min(count, (size_t)(CHAR_BUFFER_SIZE - *ppos));
	if (copy_from_user(dev->buffer + *ppos, buf, bytes_to_copy)) {
		mutex_unlock(&dev->lock);
		pr_err("char: copy_from_user failed\n");
		return -EFAULT;
	}

	*ppos += bytes_to_copy;
	dev->data_size = max(dev->data_size, (size_t)*ppos);
	mutex_unlock(&dev->lock);

	pr_info("char: wrote %zu bytes\n", bytes_to_copy);
	return bytes_to_copy;
}

static loff_t char_driver_llseek(struct file *file, loff_t off, int whence)
{
	struct char_dev *dev = file->private_data;
	loff_t new_pos;

	mutex_lock(&dev->lock);

	switch (whence) {
	case SEEK_SET:
		new_pos = off;
		break;
	case SEEK_CUR:
		new_pos = file->f_pos + off;
		break;
	case SEEK_END:
		new_pos = (loff_t)dev->data_size + off;
		break;
	default:
		mutex_unlock(&dev->lock);
		return -EINVAL;
	}

	if (new_pos < 0 || new_pos > CHAR_BUFFER_SIZE) {
		mutex_unlock(&dev->lock);
		return -EINVAL;
	}

	file->f_pos = new_pos;
	mutex_unlock(&dev->lock);
	return new_pos;
}

static const struct file_operations char_driver_fops = {
	.owner = THIS_MODULE,
	.open = char_driver_open,
	.release = char_driver_release,
	.read = char_driver_read,
	.write = char_driver_write,
	.llseek = char_driver_llseek,
};

static int __init char_driver_init(void)
{
	int ret;

	memset(&g_dev, 0, sizeof(g_dev));

	ret = alloc_chrdev_region(&g_dev.devt, 0, 1, CHAR_DEVICE_NAME);
	if (ret) {
		pr_err("char: alloc_chrdev_region failed (%d)\n", ret);
		return ret;
	}

	cdev_init(&g_dev.cdev, &char_driver_fops);
	g_dev.cdev.owner = THIS_MODULE;

	ret = cdev_add(&g_dev.cdev, g_dev.devt, 1);
	if (ret) {
		pr_err("char: cdev_add failed (%d)\n", ret);
		goto err_unregister_chrdev;
	}

	g_dev.class = class_create(CHAR_CLASS_NAME);
	if (IS_ERR(g_dev.class)) {
		ret = PTR_ERR(g_dev.class);
		pr_err("char: class_create failed (%d)\n", ret);
		goto err_cdev_del;
	}

	g_dev.device = device_create(g_dev.class, NULL, g_dev.devt, NULL,
				     CHAR_NODE_NAME);
	if (IS_ERR(g_dev.device)) {
		ret = PTR_ERR(g_dev.device);
		pr_err("char: device_create failed (%d)\n", ret);
		goto err_class_destroy;
	}

	g_dev.buffer = kmalloc(CHAR_BUFFER_SIZE, GFP_KERNEL);
	if (!g_dev.buffer) {
		ret = -ENOMEM;
		pr_err("char: kmalloc failed\n");
		goto err_device_destroy;
	}

	mutex_init(&g_dev.lock);
	g_dev.data_size = 0;

	pr_info("char: module loaded major=%d minor=%d node=/dev/%s\n",
		MAJOR(g_dev.devt), MINOR(g_dev.devt), CHAR_NODE_NAME);
	return 0;

err_device_destroy:
	device_destroy(g_dev.class, g_dev.devt);
err_class_destroy:
	class_destroy(g_dev.class);
err_cdev_del:
	cdev_del(&g_dev.cdev);
err_unregister_chrdev:
	unregister_chrdev_region(g_dev.devt, 1);
	return ret;
}

static void __exit char_driver_exit(void)
{
	kfree(g_dev.buffer);
	device_destroy(g_dev.class, g_dev.devt);
	class_destroy(g_dev.class);
	cdev_del(&g_dev.cdev);
	unregister_chrdev_region(g_dev.devt, 1);

	pr_info("char: module unloaded\n");
}

module_init(char_driver_init);
module_exit(char_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("linux-char-driver");
MODULE_DESCRIPTION("Production-style Linux character device driver example");
MODULE_VERSION("0.1.0");
