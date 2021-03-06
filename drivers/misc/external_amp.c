/*
 *	external_amp.c
 *
 * Copyright (c) 2010 Motorola
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/gpio_mapping.h>
#include <linux/external_amp.h>
#include <linux/mutex.h>

#define NAME "external_amp"
#define ENABLE   1
#define DISABLE  0
#define MC_ACTIVE  0x01         /*  flag for Media call */
#define VC_ACTIVE  0x02         /*  flag for VOIP call */

static int external_amp_gpio = -1;
static unsigned char gpio_usage_flag;
static DEFINE_MUTEX(amplock);

static void external_amp_ctl(int onoff)
{
	if (onoff)
		gpio_set_value(external_amp_gpio, 1);
	else
		gpio_set_value(external_amp_gpio, 0);
}

static int external_amp_misc_open(struct inode *inode, struct file *file)
{
	return 0;
}

static long external_amp_misc_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	mutex_lock(&amplock);

	switch (cmd) {
	case EXTERNAL_AMP_IOCTL_DISABLE:
		gpio_usage_flag &= ~MC_ACTIVE;
		if (gpio_usage_flag == 0) {
			external_amp_ctl(DISABLE);
			pr_debug("Disabled an external amp\n");
		}
		break;
	case EXTERNAL_AMP_IOCTL_ENABLE:
		gpio_usage_flag |= MC_ACTIVE;
		external_amp_ctl(ENABLE);
		pr_debug("Enabled an external amp\n");
		break;

	case VC_EXTERNAL_AMP_IOCTL_DISABLE:
		gpio_usage_flag &= ~VC_ACTIVE;
		if (gpio_usage_flag == 0) {
			external_amp_ctl(DISABLE);
			pr_debug("VC:Disabled an external amp\n");
		}
		break;

	case VC_EXTERNAL_AMP_IOCTL_ENABLE:
		gpio_usage_flag |= VC_ACTIVE;
		external_amp_ctl(ENABLE);
		pr_debug("VC:Enabled an external amp\n");
		break;

	default:
		mutex_unlock(&amplock);
		return -EINVAL;
	}

	mutex_unlock(&amplock);
	return 0;
}

static const struct file_operations external_amp_misc_fops = {
	.owner = THIS_MODULE,
	.open = external_amp_misc_open,
	.unlocked_ioctl = external_amp_misc_ioctl,
};

static struct miscdevice external_amp_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = NAME,
	.fops = &external_amp_misc_fops,
};

static int mapphone_external_amp_probe(struct platform_device *pdev)
{
	int error;

	external_amp_gpio = *(int *)(pdev->dev.platform_data);

	pr_debug("external_amp_gpio = %d\n", external_amp_gpio);

	gpio_usage_flag = 0;

	error = misc_register(&external_amp_misc_device);
	if (error < 0)
		pr_err("%s:misc register failed!\n", __func__);

	return 0;
}

static int mapphone_external_amp_remove(struct platform_device *pdev)
{
	misc_deregister(&external_amp_misc_device);
	return 0;
}

static struct platform_driver mapphone_external_amp_driver = {
	.probe		= mapphone_external_amp_probe,
	.remove		= mapphone_external_amp_remove,
	.driver		= {
		.name		= "mapphone_external_amp",
		.owner		= THIS_MODULE,
	},
};

static int __init mapphone_external_amp_init(void)
{
	pr_debug("mapphone_external_amp_init\n");
	return platform_driver_register(&mapphone_external_amp_driver);
}

static void __exit mapphone_external_amp_exit(void)
{
	pr_debug("mapphone_external_amp_exit\n");
	platform_driver_unregister(&mapphone_external_amp_driver);
}

module_init(mapphone_external_amp_init);
module_exit(mapphone_external_amp_exit);

MODULE_DESCRIPTION("Mapphone External Amp Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Motorola Mobility");
