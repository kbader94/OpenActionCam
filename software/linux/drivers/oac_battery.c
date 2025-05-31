// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/of_device.h>
#include "oac_dev.h"
#include "oac_comms.h"

#define ENERGY_FULL_DESIGN_UWH 48840000  /* 7.4V * 6.6aH = 48.84Wh */

struct oac_battery {
	struct power_supply *psy;
	struct power_supply_desc desc;
	struct oac_dev *core;

	bool charging;
	int voltage_uv;
	int bat_lvl;
	int error_code;
};

static struct oac_battery *bat;

static int oac_battery_get_property(struct power_supply *psy,
	enum power_supply_property psp,
	union power_supply_propval *val)
{
	struct oac_battery *bat = power_supply_get_drvdata(psy);

	switch (psp)
	{
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = 1;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = bat->charging ? POWER_SUPPLY_STATUS_CHARGING : POWER_SUPPLY_STATUS_DISCHARGING;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = bat->voltage_uv;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = bat->bat_lvl;
		break;
	case POWER_SUPPLY_PROP_ENERGY_NOW:
		val->intval = (bat->bat_lvl * ENERGY_FULL_DESIGN_UWH) / 100;
		break;
	case POWER_SUPPLY_PROP_ENERGY_FULL:
	case POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN:
		val->intval = ENERGY_FULL_DESIGN_UWH;
		break;
	case POWER_SUPPLY_PROP_ENERGY_EMPTY:
		val->intval = 0;
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
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_ENERGY_NOW,
	POWER_SUPPLY_PROP_ENERGY_FULL,
	POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN,
	POWER_SUPPLY_PROP_ENERGY_EMPTY,
	POWER_SUPPLY_PROP_HEALTH,
};

static void oac_battery_message_cb(struct oac_dev *dev, const struct Message *msg)
{

	if (!msg || msg->header.message_type != OAC_MESSAGE_TYPE_STATUS || !bat)
		return;

	const struct StatusBody *status = &msg->body.payload_status;
	bat->voltage_uv = status->bat_volt_uv;
	bat->bat_lvl = status->bat_lvl;
	bat->charging = status->charging;
	bat->error_code = status->error_code;

	power_supply_changed(bat->psy);
}

static int oac_battery_probe(struct platform_device *pdev)
{
	struct oac_dev *core = dev_get_drvdata(pdev->dev.parent);
	struct power_supply_config psy_cfg = {};

	bat = devm_kzalloc(&pdev->dev, sizeof(*bat), GFP_KERNEL);
	if (!bat)
		return -ENOMEM;

	psy_cfg.drv_data = bat;
	psy_cfg.of_node = pdev->dev.of_node;

	bat->core = core;
	bat->desc.name = "oac-battery";
	bat->desc.type = POWER_SUPPLY_TYPE_BATTERY;
	bat->desc.properties = oac_battery_props;
	bat->desc.num_properties = ARRAY_SIZE(oac_battery_props);
	bat->desc.get_property = oac_battery_get_property;

	bat->psy = devm_power_supply_register(&pdev->dev, &bat->desc, &psy_cfg);
	if (IS_ERR(bat->psy))
		return dev_err_probe(&pdev->dev, PTR_ERR(bat->psy), "Failed to register power supply\n");

	platform_set_drvdata(pdev, bat);

	if (oac_dev_register_callback(core, oac_battery_message_cb) < 0)
		return dev_err_probe(&pdev->dev, -ENODEV, "Failed to register message callback\n");

	dev_info(&pdev->dev, "Open Action Cam - battery driver initialized\n");
	return 0;
}

static int oac_battery_remove(struct platform_device *pdev)
{
	struct oac_battery *bat = platform_get_drvdata(pdev);
	oac_dev_unregister_callback(bat->core, oac_battery_message_cb);
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
