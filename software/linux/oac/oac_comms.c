// SPDX-License-Identifier: GPL-2.0
/*
 * oac/oac_comms.c - Open Action Cam serial communication protocol
 */
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>
#include "oac_comms.h"

static u8 comms_calculate_checksum(const u8 *data, u8 len)
{
	u8 csum = 0;
	int i;
	for (i = 0; i < len; i++)
		csum ^= data[i];
	return csum;
}

int oac_serialize_message(const struct Message *msg, u8 *out_buf, size_t out_len)
{
	size_t payload_size = 0;
	const u8 *payload_ptr = NULL;

	if (out_len < 6) // framing + header + checksum + end
		return -EINVAL;

	out_buf[0] = OAC_MESSAGE_START;
	out_buf[1] = msg->header.recipient;
	out_buf[2] = msg->header.message_type;
	out_buf[3] = msg->header.payload_length;

	switch (msg->header.message_type) {
	case OAC_MESSAGE_TYPE_COMMAND:
		payload_ptr = (const u8 *)&msg->body.payload_command;
		payload_size = sizeof(struct CommandBody);
		break;
	case OAC_MESSAGE_TYPE_GET:
		payload_ptr = (const u8 *)&msg->body.payload_get_param;
		payload_size = sizeof(struct GetParamBody);
		break;
	case OAC_MESSAGE_TYPE_SET:
		payload_ptr = (const u8 *)&msg->body.payload_set_param;
		payload_size = sizeof(struct SetParamBody);
		break;
	case OAC_MESSAGE_TYPE_STATUS:
		payload_ptr = (const u8 *)&msg->body.payload_status;
		payload_size = sizeof(struct StatusBody);
		break;
	case OAC_MESSAGE_TYPE_ERROR:
		payload_ptr = (const u8 *)&msg->body.payload_error;
		payload_size = sizeof(struct ErrorBody);
		break;
	default:
		return -EINVAL;
	}

	if (4 + payload_size + 2 > out_len)
		return -EMSGSIZE;

	memcpy(&out_buf[4], payload_ptr, payload_size);

	u8 checksum = 0;
	for (size_t i = 1; i < 4 + payload_size; ++i)
		checksum += out_buf[i];

	out_buf[4 + payload_size] = checksum;
	out_buf[5 + payload_size] = OAC_MESSAGE_END;

	return 6 + payload_size - 1;
}
int oac_deserialize_message(const u8 *buf, size_t len, struct Message *msg)
{
	if (len < 6)
		return -EINVAL;

	if (buf[0] != OAC_MESSAGE_START || buf[len - 1] != OAC_MESSAGE_END)
		return -EINVAL;

	msg->header.recipient = buf[1];
	msg->header.message_type = buf[2];
	msg->header.payload_length = buf[3];

	u8 calc_checksum = 0;
	for (size_t i = 1; i < len - 2; i++)
		calc_checksum += buf[i];

	if (calc_checksum != buf[len - 2])
		return -EINVAL;

	switch (msg->header.message_type) {
	case OAC_MESSAGE_TYPE_COMMAND:
		memcpy(&msg->body.payload_command, &buf[4], sizeof(struct CommandBody));
		break;
	case OAC_MESSAGE_TYPE_GET:
		memcpy(&msg->body.payload_get_param, &buf[4], sizeof(struct GetParamBody));
		break;
	case OAC_MESSAGE_TYPE_SET:
		memcpy(&msg->body.payload_set_param, &buf[4], sizeof(struct SetParamBody));
		break;
	case OAC_MESSAGE_TYPE_STATUS:
		memcpy(&msg->body.payload_status, &buf[4], sizeof(struct StatusBody));
		break;
	case OAC_MESSAGE_TYPE_ERROR:
		memcpy(&msg->body.payload_error, &buf[4], sizeof(struct ErrorBody));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

