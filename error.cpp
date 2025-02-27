#include "error.h"

ErrorHandler::ErrorHandler(Led* led_ref) : led(led_ref), blinkAnim(nullptr) {}

void ErrorHandler::throwError(int code) {

    if (!blinkAnim) { // Create only if not already allocated
        blinkAnim = new BlinkLedAnimation(led, code);
    }

    led->setAnimation(blinkAnim);
}
