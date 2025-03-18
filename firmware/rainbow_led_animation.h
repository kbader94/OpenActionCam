#ifndef RAINBOW_LED_ANIMATION_H
#define RAINBOW_LED_ANIMATION_H

#include "led_animation.h"
#include "led.h"

class RainbowLedAnimation : public LedAnimation {
private:
    Led* led;
    unsigned long last_update;
    uint16_t hue;

public:
    RainbowLedAnimation(Led* led_ref);
    void update() override;
};

#endif /* RAINBOW_LED_ANIMATION_H */
