#ifndef ERROR_H
#define ERROR_H

#include <stdint.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include "comms.h" /* Include comms system to send errors to ATmega */

/* Logging Levels */
#define LOG_LEVEL_SILENT   0  /* No logging */
#define LOG_LEVEL_ERRORS   1  /* Errors only */
#define LOG_LEVEL_WARNINGS 2  /* Errors + Warnings */
#define LOG_LEVEL_DEBUG    3  /* Debug + Errors + Warnings */

/* Default verbosity */
#ifndef LOG_VERBOSITY
#define LOG_VERBOSITY LOG_LEVEL_DEBUG
#endif

/*
 * comms_throw_error - Sends an error message to the ATmega.
 * @recipient: The intended recipient (e.g., 0x01 for the ATmega).
 * @error_code: The error code (matches predefined error codes).
 * @error_message: Human-readable error description.
 */
static inline void comms_throw_error(uint8_t error_code, const char *error_message)
{
    struct Error err_msg;

    /* Initialize error message structure */
    err_msg.base.recipient = MESSAGE_RECIPIENT_FIRMWARE;
    err_msg.base.message_type = MESSAGE_TYPE_ERROR;
    err_msg.base.payload_length = sizeof(uint8_t) + strlen(error_message);

    err_msg.error_code = error_code;
    strncpy(err_msg.error_message, error_message, sizeof(err_msg.error_message) - 1);
    err_msg.error_message[sizeof(err_msg.error_message) - 1] = '\0'; /* Ensure null termination */

    /* Send error message */
    comms_send_message(err_msg.base.recipient, err_msg.base.message_type, 
                       (uint8_t *)&err_msg.error_code, err_msg.base.payload_length);
}

/*
 * ERROR Macro - Logs error messages and sends them to the ATmega.
 * Supports formatted messages using __VA_ARGS__.
 */
#define ERROR(code, message, ...) do { \
    char error_msg[128]; \
    snprintf(error_msg, sizeof(error_msg), "[ERROR %d] " message, code, ##__VA_ARGS__); \
    fprintf(stderr, "%s\n", error_msg); \
    syslog(LOG_ERR, "%s", error_msg); \
    comms_throw_error(0x01, code, error_msg); /* Send error to ATmega */ \
} while (0)

/*
 * WARN Macro - Logs warnings depending on verbosity.
 */
#define WARN(message, ...) do { \
    if (LOG_VERBOSITY >= LOG_LEVEL_WARNINGS) { \
        char warn_msg[128]; \
        snprintf(warn_msg, sizeof(warn_msg), "[WARN] " message, ##__VA_ARGS__); \
        fprintf(stderr, "%s\n", warn_msg); \
        syslog(LOG_WARNING, "%s", warn_msg); \
    } \
} while (0)

/*
 * DEBUG Macro - Logs debug messages depending on verbosity.
 */
#define DEBUG(message, ...) do { \
    if (LOG_VERBOSITY >= LOG_LEVEL_DEBUG) { \
        char debug_msg[128]; \
        snprintf(debug_msg, sizeof(debug_msg), "[DEBUG] " message, ##__VA_ARGS__); \
        printf("%s\n", debug_msg); \
        syslog(LOG_DEBUG, "%s", debug_msg); \
    } \
} while (0)

#endif /* ERROR_H */
