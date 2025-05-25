// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/of_device.h>
#include "oac_dev.h"
#include "oac_comms.h"

struct oac_battery {
	struct power_supply *psy;
	struct power_supply_desc desc;
	struct oac_dev *core;

	bool charging;
	int voltage_uv;
	int error_code;
};

/* === Static global reference (only supports one battery) === */
static struct oac_battery *oac_battery_device;

/* === Power Supply Property Getter === */
static int oac_battery_get_property(struct power_supply *psy,
				    enum power_supply_property psp,
				    union power_supply_propval *val)
{
	struct oac_battery *bat = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = 1;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = bat->charging ? POWER_SUPPLY_STATUS_CHARGING :
					      POWER_SUPPLY_STATUS_DISCHARGING;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = bat->voltage_uv;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static enum power_supply_property oac_battery_props[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_HEALTH,
};

/* === Message Handler === */
static void oac_battery_message_cb(const struct Message *msg)
{
	if (!msg || msg->header.message_type != OAC_MESSAGE_TYPE_STATUS || !oac_battery_device)
		return;

	const struct StatusBody *status = &msg->body.payload_status;

	oac_battery_device->voltage_uv = status->bat_volt_uv;
	oac_battery_device->charging = status->charging;
	oac_battery_device->error_code = status->error_code;

	power_supply_changed(oac_battery_device->psy);
}

/* === Probe / Remove === */
static int oac_battery_probe(struct platform_device *pdev)
{
	struct oac_dev *core = dev_get_drvdata(pdev->dev.parent);
	struct oac_battery *bat;

	bat = devm_kzalloc(&pdev->dev, sizeof(*bat), GFP_KERNEL);
	if (!bat)
		return -ENOMEM;

	bat->core = core;

	bat->desc.name = "oac-battery";
	bat->desc.type = POWER_SUPPLY_TYPE_BATTERY;
	bat->desc.properties = oac_battery_props;
	bat->desc.num_properties = ARRAY_SIZE(oac_battery_props);
	bat->desc.get_property = oac_battery_get_property;

	bat->psy = devm_power_supply_register(&pdev->dev, &bat->desc, bat);
	if (IS_ERR(bat->psy))
		return dev_err_probe(&pdev->dev, PTR_ERR(bat->psy), "Failed to register power supply\n");

	platform_set_drvdata(pdev, bat);
	oac_battery_device = bat;

	return oac_dev_register_callback(core, oac_battery_message_cb);
}

static int oac_battery_remove(struct platform_device *pdev)
{
	struct oac_battery *bat = platform_get_drvdata(pdev);
	oac_dev_unregister_callback(bat->core, oac_battery_message_cb);
	oac_battery_device = NULL;
	return 0;
}

static const struct of_device_id oac_battery_of_match[] = {
	{ .compatible = "oac,battery" },
	{ },
};
MODULE_DEVICE_TABLE(of, oac_battery_of_match);

static struct platform_driver oac_battery_driver = {
	.probe  = oac_battery_probe,
	.remove = oac_battery_remove,
	.driver = {
		.name = "oac_battery",
		.of_match_table = oac_battery_of_match,
	},
};
module_platform_driver(oac_battery_driver);

MODULE_AUTHOR("Kyle Bader");
MODULE_DESCRIPTION("OAC Battery/Charger driver");
MODULE_LICENSE("GPL");
