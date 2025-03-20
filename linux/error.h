#ifndef ERROR_H
#define ERROR_H

#include <stdint.h>
#include <stdio.h>
#include <syslog.h>

#define LOG_LEVEL_SILENT   0  /* No logging */
#define LOG_LEVEL_ERRORS   1  /* Errors only */
#define LOG_LEVEL_WARNINGS 2  /* Errors + Warnings */
#define LOG_LEVEL_DEBUG    3  /* Debug + Errors + Warnings */

/* Default verbosity */
#ifndef LOG_VERBOSITY
#define LOG_VERBOSITY LOG_LEVEL_DEBUG
#endif

/*
 * ERROR Macro - Logs error messages.
 * Supports formatted messages using __VA_ARGS__.
 */
#define ERROR(code, message, ...) do { \
    fprintf(stderr, "[ERROR %d] " message "\n", code, ##__VA_ARGS__); \
    syslog(LOG_ERR, "[ERROR %d] " message, code, ##__VA_ARGS__); \
} while (0)

/*
 * WARN Macro - Logs warnings depending on verbosity.
 */
#define WARN(message, ...) do { \
    if (LOG_VERBOSITY >= LOG_LEVEL_WARNINGS) { \
        fprintf(stderr, "[WARN] " message "\n", ##__VA_ARGS__); \
        syslog(LOG_WARNING, "[WARN] " message, ##__VA_ARGS__); \
    } \
} while (0)

/*
 * DEBUG Macro - Logs debug messages depending on verbosity.
 */
#define DEBUG(message, ...) do { \
    if (LOG_VERBOSITY >= LOG_LEVEL_DEBUG) { \
        printf("[DEBUG] " message "\n", ##__VA_ARGS__); \
        syslog(LOG_DEBUG, "[DEBUG] " message, ##__VA_ARGS__); \
    } \
} while (0)

#endif /* ERROR_H */
