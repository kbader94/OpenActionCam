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
 * ERROR Macro - Logs error messages and sends them to the ATmega.
 * Supports formatted messages using __VA_ARGS__.
 */
#define ERROR(code, message, ...) do { \
    char error_msg[128]; \
    snprintf(error_msg, sizeof(error_msg), "[ERROR %d] " message, code, ##__VA_ARGS__); \
    fprintf(stderr, "%s\n", error_msg); \
    syslog(LOG_ERR, "%s", error_msg); \
    comms_send_error(code, error_msg); /* Send error to ATmega */ \
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
