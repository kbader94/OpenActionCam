#include "error.h"
#include <Arduino.h>

static uint8_t lastErrorCode = 0;
static Led* led = nullptr;
static PowerManagement* power_management = nullptr;
static BlinkLedAnimation blinkAnim(nullptr, 0);

void initErrorSystem(Led* l, PowerManagement* pm) {
    led = l;
    power_management = pm;

    Serial.println("Error handler initialized.");
    blinkAnim = BlinkLedAnimation(led, 0); 
}

void throwError(uint8_t code, const char* message) {
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

void resetError() {
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

uint8_t getCurrentError() {
    return lastErrorCode;
}
