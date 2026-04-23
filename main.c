#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include "address_map_arm.h"

#define DEV_SUCCESS 0
#define DEVICE_KEY "KEY"
#define DEVICE_SW  "SW"
#define INTEL_CLASS_NAME "IntelFPGAUP"

// -----------------------------
// Function declarations
// -----------------------------
static int  key_open(struct inode *, struct file *);
static int  key_release(struct inode *, struct file *);
static ssize_t key_read(struct file *, char *, size_t, loff_t *);

static int  sw_open(struct inode *, struct file *);
static int  sw_release(struct inode *, struct file *);
static ssize_t sw_read(struct file *, char *, size_t, loff_t *);

// -----------------------------
// Global variables
// -----------------------------
static dev_t key_dev_num = 0;
static struct cdev *key_cdev = NULL;

static dev_t sw_dev_num = 0;
static struct cdev *sw_cdev = NULL;

static struct class *intel_class = NULL;

static void *lw_bridge_base = NULL;
volatile int *key_reg;
volatile int *sw_reg;

// -----------------------------
// File operations
// -----------------------------
static struct file_operations key_fops = {
    .owner   = THIS_MODULE,
    .read    = key_read,
    .open    = key_open,
    .release = key_release,
};

static struct file_operations sw_fops = {
    .owner   = THIS_MODULE,
    .read    = sw_read,
    .open    = sw_open,
    .release = sw_release,
};

// -----------------------------
// Device read functions (UPDATED to return TEXT)
// -----------------------------

/**
 * @brief Reads KEY state and returns it as a string (e.g., "1\n").
 */
static ssize_t key_read(struct file *filp, char *buf, size_t len, loff_t *offset)
{
    int key_val;
    char temp_buf[4];
    int bytes_to_copy;

    key_val = *key_reg & 0xF;
    bytes_to_copy = snprintf(temp_buf, sizeof(temp_buf), "%X", key_val);

    if (len < bytes_to_copy)
        return -EINVAL;

    if (copy_to_user(buf, temp_buf, bytes_to_copy))
        return -EFAULT;

    *offset = 0;
    return bytes_to_copy;
}

/**
 * @brief Reads SW state and returns it as a string (e.g., "2\n").
 */
static ssize_t sw_read(struct file *filp, char *buf, size_t len, loff_t *offset)
{
    int sw_val;
    char temp_buf[8];
    int bytes_to_copy;

    sw_val = *sw_reg & 0x3FF;
    bytes_to_copy = snprintf(temp_buf, sizeof(temp_buf), "%X", sw_val);

    if (len < bytes_to_copy)
        return -EINVAL;

    if (copy_to_user(buf, temp_buf, bytes_to_copy))
        return -EFAULT;

    *offset = 0;
    return bytes_to_copy;
}

// -----------------------------
// Open/Release (Boilerplate)
// -----------------------------
static int key_open(struct inode *inode, struct file *file) { return DEV_SUCCESS; }
static int key_release(struct inode *inode, struct file *file) { return 0; }
static int sw_open(struct inode *inode, struct file *file) { return DEV_SUCCESS; }
static int sw_release(struct inode *inode, struct file *file) { return 0; }

// -----------------------------
// Init and Exit
// -----------------------------
static int __init key_sw_init(void)
{
    int err;

    if ((err = alloc_chrdev_region(&key_dev_num, 0, 1, DEVICE_KEY)) < 0)
        return err;

    key_cdev = cdev_alloc();
    if (!key_cdev) {
        err = -ENOMEM;
        goto unregister_key_region;
    }
    key_cdev->owner = THIS_MODULE;
    key_cdev->ops = &key_fops;
    if ((err = cdev_add(key_cdev, key_dev_num, 1)) < 0)
        goto free_key_cdev;

    if ((err = alloc_chrdev_region(&sw_dev_num, 0, 1, DEVICE_SW)) < 0)
        goto del_key_cdev;

    sw_cdev = cdev_alloc();
    if (!sw_cdev) {
        err = -ENOMEM;
        goto unregister_sw_region;
    }
    sw_cdev->owner = THIS_MODULE;
    sw_cdev->ops = &sw_fops;
    if ((err = cdev_add(sw_cdev, sw_dev_num, 1)) < 0)
        goto free_sw_cdev;

    intel_class = class_create(THIS_MODULE, INTEL_CLASS_NAME);
    if (IS_ERR(intel_class)) {
        err = PTR_ERR(intel_class);
        intel_class = NULL;
        goto del_sw_cdev;
    }

    if (IS_ERR(device_create(intel_class, NULL, key_dev_num, NULL, DEVICE_KEY))) {
        err = -EIO;
        goto destroy_class;
    }

    if (IS_ERR(device_create(intel_class, NULL, sw_dev_num, NULL, DEVICE_SW))) {
        err = -EIO;
        goto destroy_key_device;
    }

    lw_bridge_base = ioremap_nocache(LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
    if (!lw_bridge_base) {
        err = -ENOMEM;
        goto destroy_sw_device;
    }

    key_reg = (volatile int *)((char *)lw_bridge_base + KEY_BASE);
    sw_reg  = (volatile int *)((char *)lw_bridge_base + SW_BASE);

    printk(KERN_INFO "[KEY_SW] IntelFPGAUP-style drivers loaded.\n");
    return 0;

destroy_sw_device:
    device_destroy(intel_class, sw_dev_num);
destroy_key_device:
    device_destroy(intel_class, key_dev_num);
destroy_class:
    class_destroy(intel_class);
    intel_class = NULL;
del_sw_cdev:
    cdev_del(sw_cdev);
free_sw_cdev:
    kfree(sw_cdev);
    sw_cdev = NULL;
unregister_sw_region:
    unregister_chrdev_region(sw_dev_num, 1);
del_key_cdev:
    cdev_del(key_cdev);
free_key_cdev:
    kfree(key_cdev);
    key_cdev = NULL;
unregister_key_region:
    unregister_chrdev_region(key_dev_num, 1);
    return err;
}

static void __exit key_sw_exit(void)
{
    if (lw_bridge_base)
        iounmap(lw_bridge_base);

    if (intel_class) {
        device_destroy(intel_class, key_dev_num);
        device_destroy(intel_class, sw_dev_num);
        class_destroy(intel_class);
    }

    if (key_cdev)
        cdev_del(key_cdev);
    unregister_chrdev_region(key_dev_num, 1);

    if (sw_cdev)
        cdev_del(sw_cdev);
    unregister_chrdev_region(sw_dev_num, 1);

    printk(KERN_INFO "[KEY_SW] Drivers unloaded.\n");
}

MODULE_LICENSE("GPL");
module_init(key_sw_init);
module_exit(key_sw_exit);