#ifndef ERROR_H
#define ERROR_H

#include "comms.h"
#include "Arduino.h"
#include "led.h"
#include "blink_led_animation.h"
#include "system_state.h"

/* Logging Levels */
#define LOG_LEVEL_SILENT   0
#define LOG_LEVEL_ERRORS   1
#define LOG_LEVEL_WARNINGS 2
#define LOG_LEVEL_DEBUG    3

/* Current Logging Level */
#ifndef LOG_VERBOSITY
#define LOG_VERBOSITY LOG_LEVEL_DEBUG
#endif

/* Error Definitions
 * TODO: this should be further simplified! It's a legacy of the firmware attempting to 
 * handle all errors, which has been made obsolete after introduction of linux drivers 
 */
#define ORIGIN_MCU   1
#define ORIGIN_LINUX 2
typedef struct {
    uint8_t code;
    const char* message;
    uint8_t origin;
} error_def_t;
#define ERR_LOW_BATTERY               ((error_def_t){  2, "Low Battery",                            ORIGIN_MCU })  
#define ERR_INSUFFICIENT_SPACE        ((error_def_t){  3, "Insufficient storage space!",            ORIGIN_LINUX })
#define ERR_CHARGER_FAULT             ((error_def_t){  4, "Charger error",                          ORIGIN_MCU })  
#define ERR_BATTERY_OV                ((error_def_t){  6, "Battery over-voltage detected",          ORIGIN_MCU })  
#define ERR_NO_COMM_RPI               ((error_def_t){  7, "No contact with RPI!",                   ORIGIN_MCU })
#define ERR_RPI_SHUTDOWN_REQ_TIMEOUT  ((error_def_t){  8, "RPI did not acknowledge shutdown!",      ORIGIN_MCU })
#define ERR_RPI_SHUTDOWN_TIMEOUT      ((error_def_t){  9, "Could not kill RPI - no serial hangup!", ORIGIN_MCU })
#define ERR_STORAGE_CHECK_FAILED      ((error_def_t){ 10, "Failed to check available storage.",     ORIGIN_LINUX })
#define ERR_INVALID_RESOLUTION        ((error_def_t){ 12, "Invalid resolution format.",             ORIGIN_LINUX })
#define ERR_PIPE_CREATION_FAILED      ((error_def_t){ 13, "Failed to create pipe.",                 ORIGIN_LINUX })
#define ERR_MONITOR_THREAD_FAILED     ((error_def_t){ 14, "Failed to create monitor thread.",       ORIGIN_LINUX })
#define ERR_CAMERA_NOT_FOUND          ((error_def_t){ 15, "No camera detected.",                    ORIGIN_LINUX })
#define ERR_RECORD_START_FAILED       ((error_def_t){ 16, "Recording failed to start.",             ORIGIN_LINUX })
#define ERR_TRANSCODE_FAILED          ((error_def_t){ 17, "Transcoding process failed.",            ORIGIN_LINUX })

class SystemStateManager;

void init_error_system(Led* led, SystemStateManager* ssm);
void throw_error(uint8_t code, const char* message);
void reset_error();
uint8_t get_current_error();

#define ERROR(e) do { \
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

#endif /* ERROR_H */
