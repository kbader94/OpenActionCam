// SPDX-License-Identifier: GPL-2.0
/*
 * oac/oac_comms.c - Open Action Cam serial communication protocol
 *
 * Message Framing Format - Open Action Cam Protocol
 *
 * Each message transmitted over the serial interface follows a strict frame
 * format for reliable communication between the Linux system and the firmware.
 *
 * Frame Layout (all fields are 8-bit unless otherwise specified):
 *
 *   +--------+------------+-------------+--------+----------+-----------+--------+
 *   | Byte 0 | Byte 1     | Byte 2      | Byte 3 | Byte 4   | Bytes 5.. | Final  |
 *   |        |            |             |        |          | N+4       | Byte   |
 *   +--------+------------+-------------+--------+----------+-----------+--------+
 *   | START  | RECIPIENT  | TYPE        | LEN    | CHECKSUM | PAYLOAD   | END    |
 *   +--------+------------+-------------+--------+----------+-----------+--------+
 *
 * Fields:
 *   START      : Start-of-frame byte (constant, e.g. 0xAA)
 *   RECIPIENT  : Target of the message (e.g. Linux or firmware)
 *   TYPE       : Message type ID (set_param, get_param, status, error, etc.)
 *   LEN        : Payload length in bytes (0..128)
 *   CHECKSUM   : XOR of RECIPIENT, TYPE, LEN, and PAYLOAD bytes
 *   PAYLOAD    : Message-specific structure, size determined by LEN
 *   END        : End-of-frame byte (constant, e.g. 0x55)
 *
 * Total size of a frame is: 6 + LEN bytes
 *
 * CHECKSUM Calculation:
 *   The checksum is computed as:
 *
 *     XOR(buf[1] to buf[3 + LEN])
 *
 *   This includes the recipient, type, length, and each byte of the payload.
 *   It does NOT include the START or END bytes.
 *
 * Example (command 0x1234 to firmware):
 *
 *   [0xAA] [0x01] [0x01] [0x02] [0x37] [0x34] [0x12] [0x55]
 *    START  TO     CMD    LEN   CHKS   LSB    MSB    END
 *
 *   - TO = 0x01 (firmware)
 *   - TYPE = 0x01 (command)
 *   - LEN = 2 (payload size = 2 bytes for 16-bit command)
 *   - CHKSUM = 0x01 ^ 0x01 ^ 0x02 ^ 0x34 ^ 0x12 = 0x37
 *
 * This framing format is used in both directions (Linux <-> MCU).
 *
 * Notes:
 *   - All multi-byte payload fields (e.g. 16-bit values) are transmitted
 *     in little-endian order (LSB first).
 *   - Deserialization functions must validate START, END, and CHECKSUM
 *     before processing the payload.
 *   - Any frame with invalid structure must be discarded without action.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>
#include "oac_comms.h"

static u8 __maybe_unused oac_calculate_checksum(const u8 *data, u8 len)
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

	if (!msg || !out_buf)
		return -EINVAL;

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

	/* Validate buffer size: start(1) + header(4) + payload + end(1) */
	if (out_len < 6 + payload_size)
		return -EMSGSIZE;

	out_buf[0] = OAC_MESSAGE_START;
	out_buf[1] = msg->header.recipient;
	out_buf[2] = msg->header.message_type;
	out_buf[3] = payload_size;

	memcpy(&out_buf[5], payload_ptr, payload_size);

	/* Compute checksum over recipient, type, length, and payload */
	out_buf[4] = oac_calculate_checksum(&out_buf[1], 3 + payload_size);

	out_buf[5 + payload_size] = OAC_MESSAGE_END;

	return 6 + payload_size;
}

int oac_deserialize_message(const u8 *buf, size_t len, struct Message *msg)
{
	u8 payload_len;

	if (!buf || !msg || len < 6)
		return -EINVAL;

	if (buf[0] != OAC_MESSAGE_START || buf[len - 1] != OAC_MESSAGE_END)
		return -EINVAL;

	msg->header.recipient = buf[1];
	msg->header.message_type = buf[2];
	payload_len = buf[3];
	msg->header.payload_length = payload_len;
	msg->header.checksum = buf[4];

	/* Check length matches expectation */
	if (len != 6 + payload_len)
		return -EMSGSIZE;

	/* Verify checksum */
	u8 computed = oac_calculate_checksum(&buf[1], 3 + payload_len);
	if (computed != msg->header.checksum)
		return -EINVAL;

	switch (msg->header.message_type) {
	case OAC_MESSAGE_TYPE_COMMAND:
		memcpy(&msg->body.payload_command, &buf[5], sizeof(struct CommandBody));
		break;
	case OAC_MESSAGE_TYPE_GET:
		memcpy(&msg->body.payload_get_param, &buf[5], sizeof(struct GetParamBody));
		break;
	case OAC_MESSAGE_TYPE_SET:
		memcpy(&msg->body.payload_set_param, &buf[5], sizeof(struct SetParamBody));
		break;
	case OAC_MESSAGE_TYPE_STATUS:
		memcpy(&msg->body.payload_status, &buf[5], sizeof(struct StatusBody));
		break;
	case OAC_MESSAGE_TYPE_ERROR:
		memcpy(&msg->body.payload_error, &buf[5], sizeof(struct ErrorBody));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}


