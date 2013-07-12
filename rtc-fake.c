/*
 * Fake RTC whose time is settable using the command line
 *
 * Copyright (C) 2013 Joseph C. Lehner <joseph.c.lehner@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * This driver provides a very simple RTC device, which requires the
 * user to specify an initial time via a command-line option. Reading
 * from the device will return that initial time plus the time elapsed
 * since the device was initialized. By default, setting the time is not
 * supported, as it makes little sense given the fact that the value is
 * lost upon reboot/module unloading.
 *
 * In conjunction with CONFIG_RTC_HCTOSYS the driver may be used to specify
 * the machine's boot time (may be of interest on systems lacking a builtin
 * RTC). For this to work, the driver must be built as a module!
 *
 */

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/time.h>
#include <linux/err.h>
#include <linux/rtc.h>

#define DRVNAME "rtc-fake"

#define MOD_WARNING KERN_WARNING DRVNAME ": "
#define MOD_NOTICE KERN_NOTICE DRVNAME ": "
#define MOD_INFO KERN_INFO DRVNAME ": "

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Joseph C. Lehner");
MODULE_DESCRIPTION("Fake RTC that returns a time specified by the module/kernel command line");

static struct platform_device *pdev = NULL;
static struct timespec begtime;

static unsigned long time = 0;
module_param(time, ulong, S_IRUGO);
MODULE_PARM_DESC(time, "Initial time in seconds since the epoch; use 0 to disable");

static bool can_set_time = 0;
module_param(can_set_time, bool, S_IRUGO);
MODULE_PARM_DESC(can_set_time, "Allow setting the time; this usually makes no sense");

static inline unsigned long timespec_to_ulong(struct timespec *ts)
{
	return ts->tv_nsec < NSEC_PER_SEC/2 ? ts->tv_sec : ts->tv_sec + 1;
}

static inline void get_uptime(struct timespec* ts)
{
	getrawmonotonic(ts);
	monotonic_to_bootbased(ts);
}

static int fake_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct timespec now, diff;

	get_uptime(&now);
	diff = timespec_sub(now, begtime);

	rtc_time_to_tm(time + timespec_to_ulong(&diff), tm);
	return rtc_valid_tm(tm);
}

static int fake_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	if (!can_set_time)
		return -ENOSYS;

	get_uptime(&begtime);
	rtc_tm_to_time(tm, &time);

	return 0;
}

static const struct rtc_class_ops fake_rtc_ops = {
	.read_time = fake_rtc_read_time,
	.set_time = fake_rtc_set_time
};

static int fake_rtc_probe(struct platform_device *pdev)
{
	struct rtc_device *rtc;

	if (!time) {
#ifdef MODULE
		printk(MOD_NOTICE "missing 'time' parameter\n");
#endif
		return -ENODEV;
	}

	get_uptime(&begtime);

	rtc = rtc_device_register(pdev->name, &pdev->dev, &fake_rtc_ops,
			THIS_MODULE);

	if (IS_ERR(rtc))
		return PTR_ERR(rtc);

	platform_set_drvdata(pdev, rtc);
	dev_info(&pdev->dev, "loaded; begtime is %lu, time is %lu\n",
			timespec_to_ulong(&begtime), time);

	return 0;
}

static int fake_rtc_remove(struct platform_device *pdev)
{
	rtc_device_unregister(platform_get_drvdata(pdev));
	return 0;
}

static struct platform_driver fake_rtc_drv = {
	.probe = fake_rtc_probe,
	.remove = fake_rtc_remove,
	.driver = {
		.name = DRVNAME,
		.owner = THIS_MODULE
	},
};

static int __init fake_rtc_init(void)
{
	int err;

	if ((err = platform_driver_register(&fake_rtc_drv))) {
		printk(MOD_INFO "platform_register_driver failed; errno=%d\n", err);
		goto out;
	}

	pdev = platform_device_register_simple(DRVNAME, -1, NULL, 0);
	if (IS_ERR(pdev)) {
		err = PTR_ERR(pdev);
		printk(MOD_INFO "platform_device_register_simple failed; errno=%d\n", err);
		platform_driver_unregister(&fake_rtc_drv);
		goto out;
	}

out:
	return err;
}

static void __exit fake_rtc_exit(void)
{
	platform_device_unregister(pdev);
	platform_driver_unregister(&fake_rtc_drv);
}

module_init(fake_rtc_init);
module_exit(fake_rtc_exit);

