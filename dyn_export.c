#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/idr.h>
#include <linux/types.h>
#include <linux/sysfs.h>

#include <linux/device.h>

#define DEVICE_NAME "dyn_exportdev"
#define CLASS_NAME "dyn_export"

MODULE_AUTHOR("Adam Labbe <adamlabbe@gmail.com");
MODULE_DESCRIPTION("Template for creating a class with dynamic creation of devices");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL");

#define dbg(format, arg...) pr_info(CLASS_NAME ": %s: " format "\n", __FUNCTION__, ## arg)
#define info(format, arg...) pr_info(CLASS_NAME ": " format "\n", ## arg)
#define warn(format, arg...) pr_warn(CLASS_NAME ": " format "\n", ## arg)
#define err(format, arg...) pr_err(CLASS_NAME ": " format "\n", ## arg)


struct dyn_export_data {
    int number;
    int thing1;
    int thing2;
    /* other meta data */
};

static int match_export(struct device *dev, const void *desc)
{
    const long number = (const long)desc;
    struct dyn_export_data *data = dev_get_drvdata(dev);
    return data->number == number;
}

static ssize_t export_store(struct class *class,
                            struct class_attribute *attr,
                            const char *buf, size_t len);
static ssize_t unexport_store(struct class *class,
                            struct class_attribute *attr,
                            const char *buf, size_t len);

static struct class_attribute dyn_export_class_attrs[] = {
    __ATTR_WO(export),
    __ATTR_WO(unexport),
    __ATTR_NULL,
};

static struct class dyn_export_class = {
    .name = CLASS_NAME,
    .owner = THIS_MODULE,
    .class_attrs = dyn_export_class_attrs,
};

/* These are per-device attributes, and show up in:
 * /sys/class/dyn_export/dynN/ after /sys/class/dyn_export/export is used.
 */
static ssize_t thing1_show(struct device *dev,
                         struct device_attribute *attr, char *buf)
{
    struct dyn_export_data *data = dev_get_drvdata(dev);
    return scnprintf(buf, PAGE_SIZE, "%d", data->thing1);
}

static ssize_t thing1_store(struct device *dev,
                          struct device_attribute *attr, const char *buf, size_t size)
{
    long number;
    int status;
    struct dyn_export_data *data = dev_get_drvdata(dev);

    status = kstrtol(buf, 0, &number);
    if (status < 0) {
        err("could not parse input as a long, status=%d", status);
        return -EINVAL;
    }
    data->thing1 = number;
    return size;
}
static DEVICE_ATTR_RW(thing1);

static ssize_t thing2_show(struct device *dev,
                         struct device_attribute *attr, char *buf)
{
    struct dyn_export_data *data = dev_get_drvdata(dev);
    return scnprintf(buf, PAGE_SIZE, "%d", data->thing1);
}

static ssize_t thing2_store(struct device *dev,
                          struct device_attribute *attr, const char *buf, size_t size)
{
    long number;
    int status;
    struct dyn_export_data *data = dev_get_drvdata(dev);

    status = kstrtol(buf, 0, &number);
    if (status < 0) {
        err("could not parse input as a long, status=%d", status);
        return -EINVAL;
    }
    data->thing2 = number;
    return size;
}
static DEVICE_ATTR_RW(thing2);

static struct attribute *dyn_attrs[] = {
    &dev_attr_thing1.attr,
    &dev_attr_thing2.attr,
    NULL,
};

static const struct attribute_group dyn_group = {
    .attrs = dyn_attrs,
};

static const struct attribute_group *dyn_groups[] = {
    &dyn_group,
    NULL
};

/* top-level, "class" attributes.
 * Only one set of these ever exist, in /sys/class/dyn_export/{export,unexport}
 */
static ssize_t export_store(struct class *class,
                            struct class_attribute *attr,
                            const char *buf, size_t len)
{
    long number;
    struct dyn_export_data *data;
    int status;
    struct device *dev;

    status = kstrtol(buf, 0, &number);
    if (status < 0) {
        err("could not parse input as a long, status=%d", status);
        goto err_parse_input;
    }

    data = kzalloc(sizeof(struct dyn_export_data), GFP_KERNEL);
    if (!data) {
        err("could not allocate memory");
        status = -ENOMEM;
        goto err_alloc_data;
    }

    data->number = number;
    data->thing1 = data->thing2 = 0;
    err("starting create with groups");
    dev = device_create_with_groups(&dyn_export_class, NULL,
                                    MKDEV(0,0), data, dyn_groups,
                                    "dyn%ld", number);
    err("done create with groups");
    if (IS_ERR(dev)) {
        err("Could not create device for dyn%ld", number);
        status = PTR_ERR(dev);
        goto err_dev_create;
    }
    return len;

err_dev_create:
    kfree(data);
err_alloc_data:
err_parse_input:
    return status;
}

static ssize_t unexport_store(struct class *class,
                            struct class_attribute *attr,
                            const char *buf, size_t len)
{
    long number;
    int status;
    struct gpiod_data *data;
    struct device *dev;

    status = kstrtol(buf, 0, &number);
    if (status < 0) {
        err("could not parse input as a long, status=%d", status);
        goto err_parse_input;
    }

    dev = class_find_device(&dyn_export_class, NULL, (const void *)number, match_export);
    if (!dev) {
        err("'%ld' does not appear to be exported", number);
        status = -EINVAL;
        goto err_not_exported;
    }

    data = dev_get_drvdata(dev);

    device_unregister(dev);
    put_device(dev);
    kfree(data);

    return len;

err_not_exported:
err_parse_input:
    return status;
}


static struct file_operations fops = {};

/* Module initialization and release */
static int __init dyn_export_module_init(void)
{
    int retval;

    retval = class_register(&dyn_export_class);
    if (retval < 0) {
        err("failed to register device class '%s'\n", CLASS_NAME);
        return -EINVAL;
    }

    return 0;
}

static void __exit dyn_export_module_exit(void)
{
    class_unregister(&dyn_export_class);
    class_destroy(&dyn_export_class);
}

module_init(dyn_export_module_init);
module_exit(dyn_export_module_exit);
