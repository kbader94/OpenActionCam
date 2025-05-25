#include "comms.h"
#include "error.h"


static uint8_t lastErrorCode = 0;
static Led* led = nullptr;
static SystemStateManager* system_state_manager = nullptr;
static BlinkLedAnimation blinkAnim(nullptr, 0);

void init_error_system(Led* l, SystemStateManager* ssm) {
    led = l;
    system_state_manager = ssm;

    Serial.println("Error handler initialized.");
    blinkAnim = BlinkLedAnimation(led, 0); 
}

void throw_error(uint8_t code, const char* message) {
    if (!led) {
        Serial.println("ERROR: LED not initialized!");
        return;
    }

    if (!system_state_manager) {
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

    system_state_manager->transitionTo(ERROR_STATE);
}

void reset_error() {
    if (!led) {
        Serial.println("ERROR: LED not initialized!");
        return;
    }

    if (!system_state_manager) {
        Serial.println("ERROR: Power management not initialized!");
        return;  
    }

    if (lastErrorCode == 0) return;

    lastErrorCode = 0;
    blinkAnim.setBlinkCount(0);

    led->off();
    
    /* TODO: determine if the rpi is  */
    system_state_manager->transitionTo(LOW_POWER_STATE);

    Serial.println("Error Reset");
}

uint8_t get_current_error() {
    return lastErrorCode;
}

