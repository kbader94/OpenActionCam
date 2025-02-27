#ifndef ERROR_H
#define ERROR_H

#include "led.h"
#include "blink_led_animation.h"

class ErrorHandler {
private:
    Led* led;
    BlinkLedAnimation* blinkAnim;

public:
    ErrorHandler(Led* led_ref);
    void throwError(int code);
};

#endif /* ERROR_H */
