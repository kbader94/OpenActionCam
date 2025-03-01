#ifndef ERROR_H
#define ERROR_H

#include "led.h"
#include "blink_led_animation.h"

class ErrorHandler {
private:
    Led* led;
    BlinkLedAnimation blinkAnim;
    uint8_t currentCode;



public:
    ErrorHandler(Led* led);
    void throwError(uint8_t code);
    uint8_t currentError();
    void reset();
};

#endif /* ERROR_H */
