#ifndef BLINK_LED_ANIMATION_H
#define BLINK_LED_ANIMATION_H

#include "led_animation.h"
#include "led.h"

const unsigned long blink_interval = 200;  // Time between blinks (ms)
const unsigned long pause_duration = 3000; // Pause duration at end of cycle (ms)

class BlinkLedAnimation : public LedAnimation {
private:
    Led* led;
    unsigned long last_update;
    int blink_count;
    int remaining_blinks;
    bool is_on;
    bool in_pause = false;
    unsigned long cycle_end_time = 0;

public:
    BlinkLedAnimation(Led* led_ref, int blinks);
    void update() override;
};

#endif /* BLINK_LED_ANIMATION_H */
