/* SPDX-License-Identifier: GPL-2.0 */
#ifndef OAC_DEV_H
#define OAC_DEV_H

#define OAC_RX_BUF_SIZE 128
#define OAC_DEV_BR		9600
#define OAC_DEV_MAX_CB	12

#include <linux/types.h>
#include <linux/serdev.h>
#include "oac_comms.h"

/* Forward declaration */
struct oac_dev;

/* Watchdog operation interface (used by subdrivers) */
struct oac_watchdog_ops {
	int (*kick)(struct oac_dev *dev);
	int (*start)(struct oac_dev *dev);
	int (*stop)(struct oac_dev *dev);
	int (*set_timeout)(struct oac_dev *dev);
};

/* Top-level device structure for the OAC Device */
struct oac_dev {
	struct serdev_device *serdev;

	u8 rx_buf[OAC_RX_BUF_SIZE];
	int rx_pos;
	bool receiving;
	size_t expected_len;

	/* Last received status from MCU */
	struct StatusBody latest_status;
    spinlock_t status_lock;

	/* Interfaces for subdevices */
	struct oac_watchdog_ops wd_ops;

};

typedef void (*oac_dev_message_cb_t)(struct oac_dev *core, const struct Message *msg);

int oac_dev_register_callback(struct oac_dev *core, oac_dev_message_cb_t cb);
void oac_dev_unregister_callback(struct oac_dev *core, oac_dev_message_cb_t cb);

int oac_dev_send_message(struct oac_dev *dev, struct Message *msg);
struct list_head message_callbacks;


#endif /* OAC_DEV_H */
