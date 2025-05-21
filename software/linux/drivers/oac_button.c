// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/of_device.h>
#include "oac_dev.h"
#include "oac_comms.h"

struct oac_button {
	struct input_dev *input;
	struct oac_dev *core;
};

static void oac_button_on_message(struct oac_dev *core, const struct Message *msg)
{
	struct oac_button *btn = dev_get_drvdata(&core->serdev->dev);

	if (!btn || !btn->input || msg->header.message_type != OAC_MESSAGE_TYPE_COMMAND)
		return;

	switch (msg->body.payload_command.command) {
	case OAC_COMMAND_BTN_SHORT:
		input_report_key(btn->input, KEY_PROG1, 1);
		input_sync(btn->input);
		input_report_key(btn->input, KEY_PROG1, 0);
		input_sync(btn->input);
		break;
	case OAC_COMMAND_BTN_LONG:
		input_report_key(btn->input, KEY_POWER, 1);
		input_sync(btn->input);
		input_report_key(btn->input, KEY_POWER, 0);
		input_sync(btn->input);
		break;
	}
}

static int oac_button_probe(struct platform_device *pdev)
{
	struct oac_dev *core = dev_get_drvdata(pdev->dev.parent);
	struct oac_button *btn;
	struct input_dev *input;
	int err;

	btn = devm_kzalloc(&pdev->dev, sizeof(*btn), GFP_KERNEL);
	if (!btn)
		return -ENOMEM;

	input = devm_input_allocate_device(&pdev->dev);
	if (!input)
		return -ENOMEM;

	btn->core = core;
	btn->input = input;

	input->name = "Open Action Cam Button";
	input->phys = "oac/button0";
	input->id.bustype = BUS_RS232;
	input->dev.parent = &pdev->dev;

	input_set_capability(input, EV_KEY, KEY_PROG1);
	input_set_capability(input, EV_KEY, KEY_POWER);

	err = input_register_device(input);
	if (err)
		return dev_err_probe(&pdev->dev, err, "Failed to register input device\n");

	dev_set_drvdata(&pdev->dev, btn);

	/* Register with oac_dev core */ 
	err = oac_dev_register_callback(core, oac_button_on_message);
	if (err)
		return dev_err_probe(&pdev->dev, err, "Failed to register callback\n");

	dev_info(&pdev->dev, "OAC button driver initialized\n");
	return 0;
}

static int oac_button_remove(struct platform_device *pdev)
{
	struct oac_button *btn = dev_get_drvdata(&pdev->dev);

	if (btn && btn->core)
		oac_dev_unregister_callback(btn->core, oac_button_on_message);

	dev_info(&pdev->dev, "OAC button driver removed\n");
	return 0;
}

static const struct of_device_id oac_button_of_match[] = {
	{ .compatible = "oac,button" },
	{}
};
MODULE_DEVICE_TABLE(of, oac_button_of_match);

static struct platform_driver oac_button_driver = {
	.probe = oac_button_probe,
	.remove = oac_button_remove,
	.driver = {
		.name = "oac_button",
		.of_match_table = oac_button_of_match,
	},
};
module_platform_driver(oac_button_driver);

MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Open Action Cam Button Driver");
MODULE_LICENSE("GPL");
