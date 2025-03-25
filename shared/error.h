#ifndef ERROR_H
#define ERROR_H

#include "comms.h"

/* Logging Levels */
#define LOG_LEVEL_SILENT   0
#define LOG_LEVEL_ERRORS   1
#define LOG_LEVEL_WARNINGS 2
#define LOG_LEVEL_DEBUG    3

/* Current Logging Level */
#ifndef LOG_VERBOSITY
#define LOG_VERBOSITY LOG_LEVEL_DEBUG
#endif

/* Error Origins */
#define ORIGIN_MCU   1
#define ORIGIN_LINUX 2

/* Error definition struct */
typedef struct {
    uint8_t code;
    const char* message;
    uint8_t origin;
} error_def_t;

/* === Error Definitions === */
#define ERR_NO_COMM_RPI               ((error_def_t){  4, "No contact with RPI!",                   ORIGIN_MCU })
#define ERR_RPI_SHUTDOWN_REQ_TIMEOUT  ((error_def_t){  5, "RPI did not acknowledge shutdown!",      ORIGIN_MCU })
#define ERR_RPI_SHUTDOWN_TIMEOUT      ((error_def_t){  6, "Could not kill RPI - no serial hangup!", ORIGIN_MCU })
#define ERR_STORAGE_CHECK_FAILED      ((error_def_t){ 10, "Failed to check available storage.",     ORIGIN_LINUX })
#define ERR_INSUFFICIENT_SPACE        ((error_def_t){ 11, "Insufficient storage space!",            ORIGIN_LINUX })
#define ERR_INVALID_RESOLUTION        ((error_def_t){ 12, "Invalid resolution format.",             ORIGIN_LINUX })
#define ERR_PIPE_CREATION_FAILED      ((error_def_t){ 13, "Failed to create pipe.",                 ORIGIN_LINUX })
#define ERR_MONITOR_THREAD_FAILED     ((error_def_t){ 14, "Failed to create monitor thread.",       ORIGIN_LINUX })
#define ERR_CAMERA_NOT_FOUND          ((error_def_t){ 15, "No camera detected.",                    ORIGIN_LINUX })
#define ERR_RECORD_START_FAILED       ((error_def_t){ 16, "Recording failed to start.",             ORIGIN_LINUX })
#define ERR_TRANSCODE_FAILED          ((error_def_t){ 17, "Transcoding process failed.",            ORIGIN_LINUX })

#if IS_MCU

#define CURRENT_PLATFORM ORIGIN_MCU

#include "Arduino.h"
#include "led.h"
#include "blink_led_animation.h"
#include "power_management.h"

class PowerManagement;
class Led;

void init_error_system(Led* led, PowerManagement* pm);
void throw_error(uint8_t code, const char* message);
void reset_error();
uint8_t get_current_error();

#define ERROR(e) do { \
    if ((e).origin == CURRENT_PLATFORM) { \
        comms_send_error((e).code, (e).message); \
    } \
    throw_error((e).code, (e).message); \
} while (0)

#define WARN(message, ...) do { \
    if (LOG_VERBOSITY >= LOG_LEVEL_WARNINGS) { \
        Serial.print("[WARN] "); \
        Serial.print((message), ##__VA_ARGS__); \
        Serial.println(); \
    } \
} while (0)

#define DEBUG_MESSAGE(message, ...) do { \
    if (LOG_VERBOSITY >= LOG_LEVEL_DEBUG) { \
        Serial.print("[DEBUG] "); \
        Serial.print((message), ##__VA_ARGS__); \
        Serial.println(); \
    } \
} while (0)

#elif IS_LINUX

#define CURRENT_PLATFORM ORIGIN_LINUX

#include <stdint.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

void init_error_system(void);
void throw_error(uint8_t code, const char* message);
void reset_error(void);
uint8_t get_current_error(void);

#ifdef __cplusplus
}
#endif

#define ERROR(e) do { \
    if ((e).origin == CURRENT_PLATFORM) { \
        comms_send_error((e).code, (e).message); \
    } \
    throw_error((e).code, (e).message); \
} while (0)

#define WARN(message, ...) do { \
    if (LOG_VERBOSITY >= LOG_LEVEL_WARNINGS) { \
        char warn_msg[128]; \
        snprintf(warn_msg, sizeof(warn_msg), "[WARN] " message, ##__VA_ARGS__); \
        fprintf(stderr, "%s\n", warn_msg); \
        syslog(LOG_WARNING, "%s", warn_msg); \
    } \
} while (0)

#define DEBUG_MESSAGE(message, ...) do { \
    if (LOG_VERBOSITY >= LOG_LEVEL_DEBUG) { \
        char debug_msg[128]; \
        snprintf(debug_msg, sizeof(debug_msg), "[DEBUG] " message, ##__VA_ARGS__); \
        printf("%s\n", debug_msg); \
        syslog(LOG_DEBUG, "%s", debug_msg); \
    } \
} while (0)

#else

#error "Platform not defined: you must define either IS_MCU or IS_LINUX."

#endif /* platform check */

#endif /* ERROR_H */
