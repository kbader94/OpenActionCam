#ifndef COMMS_H
#define COMMS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Define IS_MCU Macro to determine whether code is firware on MCU or linux application */
#if defined(ARDUINO) || defined(__AVR__) || defined(TEENSYDUINO)
#define IS_MCU 1
#define IS_LINUX 0
#else
#define IS_MCU 0
#define IS_LINUX 1
#endif

/* Message Framing */
#define MESSAGE_START 0xAA
#define MESSAGE_END   0x55

/* Message Type Identifiers */
#define MESSAGE_TYPE_COMMAND  0x01
#define MESSAGE_TYPE_DATA     0x02
#define MESSAGE_TYPE_ERROR    0x03

/* Payload Constraints */
#define MAX_PAYLOAD_SIZE 256  /* Adjust based on RAM availability */
/* Timeout for message reception (milliseconds) */
#define MAX_MESSAGE_TIMEOUT_MS 100

/* Command Definitions */
#define COMMAND_RECORD_REQ_START  0xF000
#define COMMAND_RECORD_STARTED    0xF001
#define COMMAND_RECORD_REQ_END    0xE000
#define COMMAND_RECORD_ENDED      0xE001
#define COMMAND_SHUTDOWN_REQ      0xD000
#define COMMAND_SHUTDOWN_STARTED  0xD001

/* Message Recipient Definitions */
#define MESSAGE_RECIPIENT_LINUX 0x01
#define MESSAGE_RECIPIENT_FIRMWARE 0x02

/* Platform Specific Definitions */
#if IS_MCU
    #include "serial_wrapper.h" 
    #define ARDUINO_BAUDRATE 9600
    #define GET_TIME_MS() millis()
#else /* IS_LINUX */
    #include <time.h>
    #define LINUX_BAUDRATE B9600
    static inline uint32_t GET_TIME_MS(void) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
    }
#endif

struct Message {
    uint8_t recipient;
    uint8_t message_type; /* COMMAND, DATA, or ERROR */
    uint8_t payload_length;
    uint8_t payload[MAX_PAYLOAD_SIZE];
};

/* Command Structure (inherits from Message) */
struct Command {
    struct Message message;
    uint16_t command;
};

/* Status Structure (inherits from Message) */
struct Status {
    struct Message message;
    uint16_t battery_level;
    uint8_t state;
    bool charging;
    bool recording;
    bool error;
    
};

/* Error Structure (inherits from Message) */
struct Error {
    struct Message base;
    uint8_t error_code;
    char error_message[MAX_PAYLOAD_SIZE - 1]; /* Reserve space for message */
};

/* Initialize communication */
void comms_init(void);

/* Send a message */
void comms_send_message(uint8_t recipient, uint8_t type, uint8_t *payload, uint8_t length);

/* Receive messages (non-blocking on Arduino) */
bool comms_receive_message(struct Message *msg);

/* Utility function to calculate checksum */
uint8_t comms_calculate_checksum(uint8_t *data, uint8_t length);

/* Close communication (for Linux) */
void comms_close(void);

#ifdef __cplusplus
}
#endif

#endif /* COMMS_H */
