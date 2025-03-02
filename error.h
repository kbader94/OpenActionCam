#ifndef ERROR_H
#define ERROR_H

#include <stdint.h>
#include "led.h"
#include "blink_led_animation.h"
#include "power_management.h"

class PowerManagement;

void initErrorSystem(Led* led, PowerManagement* pm);
void throwError(uint8_t code, const char* message);
void resetError();
uint8_t getCurrentError();

#define WARN_VERBOSITY 3  /* 0 = Silent, 1 = Errors only, 2 = Errors and Warnings only, 3 = Debug, Errors, and Warnings */

/* Predefined Error Codes and messages */
#define ERR_NO_COMM_RPI        4, "No communication with RPI"
#define ERR_SENSOR_FAILURE     3, "Sensor failure detected"
#define ERR_OVERTEMP           5, "Overheating warning"
#define ERR_LOW_BATTERY        6, "Low battery detected"

/* 
 * ERROR Macro - Logs error and flashes error code on LED
 */
#define ERROR(...) do { \
    Serial.print("[ERROR] "); Serial.print(__VA_ARGS__); Serial.println(); \
    throwError(__VA_ARGS__); \
} while(0)

/* 
 * WARN Macro - Logs warnings depending on verbosity
 */
#define WARN(message) do { \
    if (WARN_VERBOSITY > 1) { \
        Serial.print("[WARN] "); Serial.println(message); \
    } \
} while(0)

/* 
 * DEBUG Macro - Logs debug messages depending on verbosity
 */
#define DEBUG(message) do { \
    if (WARN_VERBOSITY > 2) { \
        Serial.print("[DEBUG] "); Serial.println(message); \
    } \
} while(0)

#endif // ERROR_H
