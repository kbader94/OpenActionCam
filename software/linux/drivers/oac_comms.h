#ifndef _OAC_COMMS_H
#define _OAC_COMMS_H

#include <linux/types.h>

/* Message Framing */
#define OAC_MESSAGE_START 			  0xAA
#define OAC_MESSAGE_END               0x55

/* Payload Constraints */
#define OAC_MAX_PAYLOAD_SIZE 		  128

/* Message Recipient Definitions */
#define OAC_COMMS_RECIPIENT_LINUX     0x01
#define OAC_COMMS_RECIPIENT_FIRMWARE  0x02

/* Command Definitions */
#define OAC_COMMAND_RECORD_REQ_START  0xF000
#define OAC_COMMAND_RECORD_STARTED    0xF001
#define OAC_COMMAND_RECORD_REQ_END    0xE000
#define OAC_COMMAND_RECORD_ENDED      0xE001
#define OAC_COMMAND_SHUTDOWN_REQ      0xD000
#define OAC_COMMAND_SHUTDOWN_STARTED  0xD001
#define OAC_COMMAND_HB                0xC000 	/* Redundant, use WD_KICK*/

/* Button Definitions */
#define OAC_COMMAND_BTN_SHORT  		  0xA001
#define OAC_COMMAND_BTN_LONG   	      0xA002

/* Watchdog Definitions */
#define OAC_COMMAND_WD_START          0xB000
#define OAC_COMMAND_WD_STOP           0xB001
#define OAC_COMMAND_WD_KICK           0xB002
#define OAC_COMMAND_WD_SET_TO         0xB003

/* Message Type Identifiers */
enum MessageType {
	OAC_MESSAGE_TYPE_COMMAND 	= 0x01,
	OAC_MESSAGE_TYPE_STATUS  	= 0x02,
	OAC_MESSAGE_TYPE_ERROR   	= 0x03,
	OAC_MESSAGE_TYPE_DATA    	= 0x04,
	OAC_MESSAGE_TYPE_RESPONSE	= 0x06,
};

struct __packed MessageHeader {
    uint8_t recipient;
    uint8_t message_type;
    uint8_t payload_length;
    uint8_t checksum;
};

/* Command Payload */
struct CommandBody {
	u16 command;
};

struct ResponseBody {
	u16 param;
	u64 val;
};

/* Status Payload */
struct StatusBody {
	u32 bat_volt_uv;
	u8 state;
	bool charging;
	u8 error_code;
};

/* Error Payload */
struct ErrorBody {
	u8 error_code;
	char error_message[OAC_MAX_PAYLOAD_SIZE - 1];
};

/* Tagged Union Message */
struct Message {
	struct MessageHeader header;
	union {
		struct CommandBody payload_command;
		struct ResponseBody payload_response;
		struct StatusBody payload_status;
		struct ErrorBody payload_error;
		u8 payload_raw[OAC_MAX_PAYLOAD_SIZE];
	} body;
};

/* Serialization and Deserialization API */
int oac_serialize_message(const struct Message *msg, u8 *out_buf, size_t out_len);
int oac_deserialize_message(const u8 *buf, size_t len, struct Message *msg);

#endif /* _OAC_COMMS_H */
