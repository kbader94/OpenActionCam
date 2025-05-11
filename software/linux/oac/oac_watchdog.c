// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/watchdog.h>
#include <linux/of_device.h>
#include "oac_comms.h"
#include "oac_dev.h"

struct oac_watchdog {
	struct watchdog_device wdd;
	struct oac_dev *core;
};

static int oac_wd_ping(struct watchdog_device *wdd)
{
	struct oac_watchdog *owd = watchdog_get_drvdata(wdd);
	if (!owd || !owd->core)
		return -EINVAL;

	struct Message msg = {
		.header.message_type = OAC_MESSAGE_TYPE_COMMAND,
		.body.payload_command.command = OAC_COMMAND_WD_KICK,
	};

	return oac_dev_send_message(owd->core, &msg);
}

static int oac_wd_start(struct watchdog_device *wdd)
{	
	struct oac_watchdog *owd = watchdog_get_drvdata(wdd);
	if (!owd || !owd->core)
		return -EINVAL;

	struct Message msg = {
		.header.message_type = OAC_MESSAGE_TYPE_COMMAND,
		.body.payload_command.command = OAC_COMMAND_WD_START,
	};

	return oac_dev_send_message(owd->core, &msg);
}

static int oac_wd_stop(struct watchdog_device *wdd)
{
	struct oac_watchdog *owd = watchdog_get_drvdata(wdd);
	if (!owd || !owd->core)
		return -EINVAL;

	struct Message msg = {
		.header.message_type = OAC_MESSAGE_TYPE_COMMAND,
		.body.payload_command.command = OAC_COMMAND_WD_STOP,
	};

	return oac_dev_send_message(owd->core, &msg);
}

static int oac_wd_set_timeout(struct watchdog_device *wdd, unsigned int timeout)
{
	struct oac_watchdog *owd = watchdog_get_drvdata(wdd);
	if (!owd || !owd->core)
		return -ENODEV;

	struct Message msg = {
		.header = {
			.message_type = OAC_MESSAGE_TYPE_SET,
			.recipient = OAC_COMMS_RECIPIENT_FIRMWARE,
			.payload_length = sizeof(struct SetParamBody),
		},
		.body.payload_set_param.param = OAC_COMMAND_WD_SET_TO,
		.body.payload_set_param.val = timeout,
	};

	return oac_dev_send_message(owd->core, &msg);
}


static const struct watchdog_ops oac_wd_ops = {
	.owner = THIS_MODULE,
	.ping = oac_wd_ping,
	.start = oac_wd_start,
	.stop = oac_wd_stop,
	.set_timeout = oac_wd_set_timeout,
};

static const struct watchdog_info oac_wd_info = {
	.options = WDIOF_KEEPALIVEPING | WDIOF_SETTIMEOUT,
	.identity = "Open Action Cam Watchdog",
	.firmware_version = 1,
};

static int oac_watchdog_probe(struct platform_device *pdev)
{
	struct oac_dev *core = dev_get_drvdata(pdev->dev.parent);
	struct oac_watchdog *owd;
	int ret;

	dev_info(&pdev->dev, "watchdog probe started\n");

	owd = devm_kzalloc(&pdev->dev, sizeof(*owd), GFP_KERNEL);
	if (!owd) {
		dev_warn(&pdev->dev, "failed to allocate memory\n");
		return -ENOMEM;
	}

	owd->core = core;
	owd->wdd.info = &oac_wd_info;
	owd->wdd.ops = &oac_wd_ops;
	owd->wdd.min_timeout = 1;
	owd->wdd.
	owd->wdd.max_timeout = 60;
	owd->wdd.parent = &pdev->dev;

	watchdog_set_drvdata(&owd->wdd, owd);
	watchdog_set_nowayout(&owd->wdd, 0);

	ret = devm_watchdog_register_device(&pdev->dev, &owd->wdd);
	if (ret)
		return dev_err_probe(&pdev->dev, ret, "failed to register watchdog\n");

	dev_info(&pdev->dev, "Open Action Cam - Watchdog registered\n");
	return 0;
}

static int oac_watchdog_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "Watchdog removed\n");
	return 0;
}

static const struct of_device_id oac_watchdog_of_match[] = {
	{ .compatible = "oac,watchdog" },
	{}
};
MODULE_DEVICE_TABLE(of, oac_watchdog_of_match);

static struct platform_driver oac_watchdog_driver = {
	.probe = oac_watchdog_probe,
	.remove = oac_watchdog_remove,
	.driver = {
		.name = "oac_watchdog",
		.of_match_table = oac_watchdog_of_match,
	},
};
module_platform_driver(oac_watchdog_driver);

MODULE_AUTHOR("Kyle Bader");
MODULE_DESCRIPTION("Open Action Camera - Watchdog driver");
MODULE_LICENSE("GPL");
