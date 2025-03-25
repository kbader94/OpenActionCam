#include "comms.h"
#include "error.h"

/* Firmware error handling */
#if IS_MCU

#include <Arduino.h>

static uint8_t lastErrorCode = 0;
static Led* led = nullptr;
static PowerManagement* power_management = nullptr;
static BlinkLedAnimation blinkAnim(nullptr, 0);

void init_error_system(Led* l, PowerManagement* pm) {
    led = l;
    power_management = pm;

    Serial.println("Error handler initialized.");
    blinkAnim = BlinkLedAnimation(led, 0); 
}

void throw_error(uint8_t code, const char* message) {
    if (!led) {
        Serial.println("ERROR: LED not initialized!");
        return;
    }

    if (!power_management) {
        Serial.println("ERROR: Power management not initialized!");
        return;
    }

    if (lastErrorCode == code) return;  /* Prevent redundant updates */

    Serial.print("[ERROR] ");
    Serial.print(code);
    Serial.print(": ");
    Serial.println(message);
    
    lastErrorCode = code;

    blinkAnim.setBlinkCount(code);
    led->setHue(LED_HUE_RED);
    led->setAnimation(&blinkAnim);

    power_management->transitionTo(ERROR_STATE);
}

void reset_error() {
    if (!led) {
        Serial.println("ERROR: LED not initialized!");
        return;
    }

    if (!power_management) {
        Serial.println("ERROR: Power management not initialized!");
        return;  
    }

    if (lastErrorCode == 0) return;

    lastErrorCode = 0;
    blinkAnim.setBlinkCount(0);

    led->off();
    
    /* TODO: determine if the rpi is  */
    power_management->transitionTo(LOW_POWER_STATE);

    Serial.println("Error Reset");
}

uint8_t get_current_error() {
    return lastErrorCode;
}

/* Linux error handling */
#elif IS_LINUX

#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Global Error State */
static uint8_t current_error = 0;

/* Initialize Error System */
void init_error_system() {
    /* Open syslog for system-wide logging */
    openlog("linux_camera", LOG_PID | LOG_CONS, LOG_USER);
    syslog(LOG_INFO, "Error system initialized.");
}

/* Reset error */
void reset_error() {
    current_error = 0;
}

void throw_error(uint8_t code, const char* message) {
    fprintf(stderr, "[ERROR %d]%s\n", code, message); 
    syslog(LOG_WARNING, "%s", message); \
}

/* Get the current error */
uint8_t get_current_error() {
    return current_error;
}

#ifdef __cplusplus
}
#endif

#else

#error "IS_LINUX or IS_MCU must be defined when compiling error.cpp"

#endif /* platform check */
