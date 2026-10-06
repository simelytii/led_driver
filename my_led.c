#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/of.h>

struct my_led_data {
	struct gpio_desc *led_gpio;
	const char *label;
	bool state;
	unsigned int interval;
	struct delayed_work blink_work;
	struct class *class;
	struct device *device;
};

static void my_led_blink_work(struct work_struct *work)
{
	struct my_led_data *data =
		container_of(to_delayed_work(work),
			     struct my_led_data, blink_work);

	if (!data->state || data->interval == 0)
		return;

	gpiod_set_value(data->led_gpio,
			!gpiod_get_value(data->led_gpio));

	schedule_delayed_work(&data->blink_work,
			      msecs_to_jiffies(data->interval * 1000));
}

static ssize_t state_show(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	struct my_led_data *data = dev_get_drvdata(dev);

	return sysfs_emit(buf, "%d\n", data->state);
}

static ssize_t state_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct my_led_data *data = dev_get_drvdata(dev);
	bool value;

	if (kstrtobool(buf, &value))
		return -EINVAL;

	data->state = value;
	gpiod_set_value(data->led_gpio, data->state);

	return count;
}

static DEVICE_ATTR_RW(state);

static ssize_t interval_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct my_led_data *data = dev_get_drvdata(dev);

	return sysfs_emit(buf, "%u\n", data->interval);
}

static ssize_t interval_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct my_led_data *data = dev_get_drvdata(dev);
	unsigned int value;

	if (kstrtouint(buf, 10, &value))
		return -EINVAL;

	data->interval = value;

	if (data->interval == 0) {
		cancel_delayed_work_sync(&data->blink_work);
		gpiod_set_value(data->led_gpio, data->state);
	} else if (data->state) {
		cancel_delayed_work_sync(&data->blink_work);
		schedule_delayed_work(&data->blink_work, 0);
	}

	return count;
}

static DEVICE_ATTR_RW(interval);

static ssize_t label_show(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	struct my_led_data *data = dev_get_drvdata(dev);

	return sysfs_emit(buf, "%s\n", data->label);
}

static DEVICE_ATTR_RO(label);

static int my_led_probe(struct platform_device *pdev)
{
	struct my_led_data *data;

	dev_info(&pdev->dev, "my_led: probe\n");

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->led_gpio = devm_gpiod_get(&pdev->dev, NULL, GPIOD_OUT_LOW);
	if (IS_ERR(data->led_gpio))
	{
		dev_err(&pdev->dev, "Failed to get GPIO\n");
		return PTR_ERR(data->led_gpio);
	}

	platform_set_drvdata(pdev, data);

	if (of_property_read_string(pdev->dev.of_node, "label",
				&data->label))
	{
		dev_err(&pdev->dev, "Failed to read LED label from Device Tree\n");
		return -EINVAL;
	}

	dev_info(&pdev->dev, "my_led: LED label from DT: %s\n", data->label);

	data->state = false;
	data->interval = 0;
	INIT_DELAYED_WORK(&data->blink_work, my_led_blink_work);

	data->class = class_create("my_led");
	if (IS_ERR(data->class))
		return PTR_ERR(data->class);

	data->device = device_create(data->class, NULL, 0, data, "my_led");
	if (IS_ERR(data->device))
	{
		class_destroy(data->class);
		return PTR_ERR(data->device);
	}

	if (device_create_file(data->device, &dev_attr_state))
	{
		device_destroy(data->class, 0);
		class_destroy(data->class);
		return -EINVAL;
	}

	if (device_create_file(data->device, &dev_attr_interval))
	{
		device_remove_file(data->device, &dev_attr_state);
		device_destroy(data->class, 0);
		class_destroy(data->class);
		return -EINVAL;
	}

	if (device_create_file(data->device, &dev_attr_label))
	{
		device_remove_file(data->device, &dev_attr_interval);
		device_remove_file(data->device, &dev_attr_state);
		device_destroy(data->class, 0);
		class_destroy(data->class);
		return -EINVAL;
	}

	dev_info(&pdev->dev, "my_led: GPIO initialized\n");
	dev_info(&pdev->dev, "my_led: sysfs interface created\n");

	return 0;
}

static int my_led_remove(struct platform_device *pdev)
{
	struct my_led_data *data = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&data->blink_work);

	device_remove_file(data->device, &dev_attr_interval);
	device_remove_file(data->device, &dev_attr_label);
	device_remove_file(data->device, &dev_attr_state);

	gpiod_set_value(data->led_gpio, 0);

	device_destroy(data->class, 0);
	class_destroy(data->class);

	dev_info(&pdev->dev, "my_led: remove\n");

	return 0;
}

static const struct of_device_id my_led_of_match[] = {
	{ .compatible = "custom,my-led" },
	{ }
};
MODULE_DEVICE_TABLE(of, my_led_of_match);

static struct platform_driver my_led_driver = {
	.probe = my_led_probe,
	.remove = my_led_remove,
	.driver = {
		.name = "my_led",
		.of_match_table = my_led_of_match,
	},
};

module_platform_driver(my_led_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vakare");
MODULE_DESCRIPTION("Task 6 LED driver");