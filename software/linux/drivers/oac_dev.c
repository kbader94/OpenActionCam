// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/serdev.h>
#include <linux/of_device.h>
#include <linux/mfd/core.h>
#include "oac_comms.h"
#include "oac_dev.h"

static oac_dev_message_cb_t oac_dev_registered_cbs[OAC_DEV_MAX_CB]; 
static DEFINE_MUTEX(cb_lock); /* callback registration lock */

static void oac_dev_message_registered_callbacks(struct oac_dev *dev, struct Message *msg)
{
	mutex_lock(&cb_lock);
	for (int i = 0; i < ARRAY_SIZE(oac_dev_registered_cbs); i++) {
		if (oac_dev_registered_cbs[i]){
			oac_dev_registered_cbs[i](dev, msg);
		}
			
	}
	mutex_unlock(&cb_lock);
}

int oac_dev_register_callback(struct oac_dev *core, oac_dev_message_cb_t cb)
{

	mutex_lock(&cb_lock);
	for (int i = 0; i < ARRAY_SIZE(oac_dev_registered_cbs); i++) {
		if (!oac_dev_registered_cbs[i]) {
			oac_dev_registered_cbs[i] = cb;
			mutex_unlock(&cb_lock);
			return 0;
		}
	}
	mutex_unlock(&cb_lock);
	return -ENOMEM;
}
EXPORT_SYMBOL_GPL(oac_dev_register_callback);

void oac_dev_unregister_callback(struct oac_dev *core, oac_dev_message_cb_t cb)
{

	mutex_lock(&cb_lock);
	for (int i = 0; i < ARRAY_SIZE(oac_dev_registered_cbs); i++) {
		if (oac_dev_registered_cbs[i] == cb) {
			oac_dev_registered_cbs[i] = NULL;
			break;
		}
	}
	mutex_unlock(&cb_lock);

}
EXPORT_SYMBOL_GPL(oac_dev_unregister_callback);

int oac_dev_send_message(struct oac_dev *dev, struct Message *msg)
{
	u8 buf[OAC_MAX_PAYLOAD_SIZE + 6];
	int len;

	len = oac_serialize_message(msg, buf, sizeof(buf));
	if (len < 0)
		return -EINVAL;

	pr_info("OAC: sending message\n");

	return serdev_device_write_buf(dev->serdev, buf, len);
}
EXPORT_SYMBOL_GPL(oac_dev_send_message);

static int oac_dev_receive(struct serdev_device *serdev, const u8 *data, size_t count)
{
	struct oac_dev *odev = serdev_device_get_drvdata(serdev);
	size_t i;

	for (i = 0; i < count; ++i) {
		u8 byte = data[i];

		/* Validate start byte */
		if (!odev->receiving) {
			if (byte == OAC_MESSAGE_START) {
				odev->receiving = true;
				odev->rx_pos = 0;
				odev->expected_len = 0;
				odev->rx_buf[odev->rx_pos++] = byte;
			}
			continue;
		}

		/* Append to buffer */
		if (odev->rx_pos < OAC_RX_BUF_SIZE) {
			odev->rx_buf[odev->rx_pos++] = byte;
		} else {
			dev_warn(&serdev->dev, "RX buffer overflow\n");
			odev->receiving = false;
			odev->rx_pos = 0;
			continue;
		}

		/*
		 * Once enough data is received to read the payload length,
		 * calculate expected total length:
		 * header (4) + payload + checksum (1) + end byte (1)
		 */
		if (odev->rx_pos == 3) {
			odev->expected_len = 6 + odev->rx_buf[3];
			if (odev->expected_len > OAC_RX_BUF_SIZE) {
				dev_warn(&serdev->dev, "Invalid expected message length\n");
				odev->receiving = false;
				odev->rx_pos = 0;
				continue;
			}
		}

		/* If full message received, validate and parse */
		if (odev->expected_len > 0 && odev->rx_pos >= odev->expected_len) {
			u8 *rx_buf = odev->rx_buf;
			size_t len = odev->expected_len;

			if (rx_buf[len - 1] != OAC_MESSAGE_END) {
				dev_warn(&serdev->dev, "Invalid end byte: 0x%02X\n",
					 rx_buf[len - 1]);
				odev->receiving = false;
				odev->rx_pos = 0;
				continue;
			}

			/* Deserialize and handle the message */
			{
				struct Message msg;
				if (oac_deserialize_message(rx_buf, len, &msg) == 0) {
					dev_info(&serdev->dev, "Received message type %u\n",
							 msg.header.message_type);
				
					switch (msg.header.message_type) {
					case OAC_MESSAGE_TYPE_STATUS:
					case OAC_MESSAGE_TYPE_COMMAND:
					case OAC_MESSAGE_TYPE_RESPONSE:
					case OAC_MESSAGE_TYPE_ERROR:
					case OAC_MESSAGE_TYPE_DATA:
						/* Broadcast message to all registered callbacks */
						oac_dev_message_registered_callbacks(odev, &msg);
						break;
				
					default:
						dev_warn(&serdev->dev, "Unknown message type: %u\n",
								 msg.header.message_type);
						break;
					}
				} else {
					dev_warn(&serdev->dev, "Failed to deserialize message\n");
				}
				
			}

			/* Reset state for next message */
			odev->receiving = false;
			odev->rx_pos = 0;
		}
	}

	return count;
}

static const struct serdev_device_ops oac_serdev_ops = {
	.receive_buf = oac_dev_receive,
};

static int oac_dev_probe(struct serdev_device *serdev)
{
	struct oac_dev *dev;

	dev_info(&serdev->dev, "Probing oac_dev driver \n");
	
	static struct mfd_cell cells[] = {
		{
			.name = "oac_battery",
			.of_compatible = "oac,battery",
		},	
		{
			.name = "oac_button",
			.of_compatible = "oac,button",
		},				
		{
			.name = "oac_watchdog",
			.of_compatible = "oac,watchdog",
		},
	};

	dev = devm_kzalloc(&serdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	spin_lock_init(&dev->status_lock);

	serdev_device_set_drvdata(serdev, dev);
	dev->serdev = serdev;

	serdev_device_set_client_ops(serdev, &oac_serdev_ops);

	if (serdev_device_open(serdev) < 0)
		return dev_err_probe(&serdev->dev, -ENODEV, "Failed to open serdev");

	serdev_device_set_baudrate(serdev, OAC_DEV_BR);
	serdev_device_set_flow_control(serdev, false);
	serdev_device_set_parity(serdev, SERDEV_PARITY_NONE);

	dev_info(&serdev->dev, "Probe complete \n");

	return devm_mfd_add_devices(&serdev->dev, PLATFORM_DEVID_AUTO,
				 cells, ARRAY_SIZE(cells), NULL, 0, NULL);
}

static void oac_dev_remove(struct serdev_device *serdev)
{
	serdev_device_close(serdev);
}

static const struct of_device_id oac_dev_of_match[] = {
	{ .compatible = "oac,dev" },
	{}
};
MODULE_DEVICE_TABLE(of, oac_dev_of_match);

static struct serdev_device_driver oac_dev_driver = {
	.driver = {
		.name = "oac_dev",
		.of_match_table = oac_dev_of_match,
	},
	.probe = oac_dev_probe,
	.remove = oac_dev_remove,
};
module_serdev_device_driver(oac_dev_driver);

MODULE_AUTHOR("Kyle Bader");
MODULE_DESCRIPTION("Open Action Cam - Device Driver");
MODULE_LICENSE("GPL");
