#ifndef COMMS_H
#define COMMS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* Define IS_MCU Macro to determine whether code is firware on MCU or linux application */
#if defined(ARDUINO) || defined(__AVR__) || defined(TEENSYDUINO)
#define IS_MCU 1
#define IS_LINUX 0
#else
#define IS_MCU 0
#define IS_LINUX 1
#endif

/* Platform Specific Definitions */
#if IS_MCU
    #include "serial_wrapper.h" 
    #define ARDUINO_BAUDRATE 9600
    #define GET_TIME_MS() millis()

#else /* IS_LINUX */
    #include <time.h>
    #include <stdio.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <termios.h>
    #include <sys/ioctl.h>

    #define LINUX_BAUDRATE B9600
    static inline uint32_t GET_TIME_MS(void) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
    }
#endif

/* Message Framing */
#define MESSAGE_START 0xAA
#define MESSAGE_END   0x55

/* Payload Constraints */
#define MAX_PAYLOAD_SIZE 128  /* Adjust based on RAM availability */
/* Timeout for message reception (milliseconds) */
#define MAX_MESSAGE_TIMEOUT_MS 100

/* Command Definitions */
#define COMMAND_RECORD_REQ_START  0xF000
#define COMMAND_RECORD_STARTED    0xF001
#define COMMAND_RECORD_REQ_END    0xE000
#define COMMAND_RECORD_ENDED      0xE001
#define COMMAND_SHUTDOWN_REQ      0xD000
#define COMMAND_SHUTDOWN_STARTED  0xD001
#define COMMAND_BOOT              0xC000

/* Message Recipient Definitions */
#define MESSAGE_RECIPIENT_LINUX 0x01
#define MESSAGE_RECIPIENT_FIRMWARE 0x02

/* Message Type Identifiers */
enum MessageType {
    MESSAGE_TYPE_COMMAND = 0x01,
    MESSAGE_TYPE_STATUS  = 0x02,
    MESSAGE_TYPE_ERROR   = 0x03,
    MESSAGE_TYPE_DATA    = 0x04
};

/* Message Header */
struct MessageHeader {
    uint8_t recipient;
    uint8_t message_type;
    uint8_t payload_length;
};

/* Command Payload */
struct CommandBody {
    uint16_t command;
};

/* Status Payload */
struct StatusBody {
    uint16_t battery_level;
    uint8_t state;
    bool charging;
    bool recording;
    bool error;
};

/* Error Payload */
struct ErrorBody {
    uint8_t error_code;
    char error_message[MAX_PAYLOAD_SIZE - 1];
};

/* Full Message (Tagged Union) */
struct Message {
    struct MessageHeader header;
    union {
        struct CommandBody payload_command;
        struct StatusBody payload_status;
        struct ErrorBody payload_error;
        uint8_t payload_raw[MAX_PAYLOAD_SIZE]; /* Raw access (used for MESSAGE_TYPE_DATA) */
    } body;

};

/* Public API */

int comms_init(void);

int comms_receive_message(struct Message *msg);

int comms_send_command(uint16_t command);

int comms_send_error(uint8_t code, const char *message);

int comms_send_status(const struct StatusBody *status);

void comms_close(void);

#ifdef __cplusplus
}
#endif

#endif /* COMMS_H */
