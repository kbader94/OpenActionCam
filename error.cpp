#include "error.h"

ErrorHandler::ErrorHandler(Led* led) 
    : led(led), blinkAnim(led, 0), currentCode(0) {}

void ErrorHandler::throwError(uint8_t code) {
    if (currentCode == code) return;  // Prevent redundant updates from restarting the sequence

    Serial.print("Error ");
    Serial.println(code);
    currentCode = code;  // Update the stored error code

    blinkAnim.setBlinkCount(code); // Update blink count
    led->setHue(LED_HUE_RED);
    led->setAnimation(&blinkAnim);
    
}

void ErrorHandler::reset() {
    currentCode = 0;
}

uint8_t ErrorHandler::currentError() {
    return currentCode;
}