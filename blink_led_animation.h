#ifndef BLINK_LED_ANIMATION_H
#define BLINK_LED_ANIMATION_H

#include "led_animation.h"
#include "led.h"

const unsigned long BLINK_INTERVAL = 200;  // Time between blinks (ms)
const unsigned long PAUSE_DURATION = 3000; // Pause duration at end of cycle (ms)

class BlinkLedAnimation : public LedAnimation {
private:
    Led* led;
    unsigned long last_update;
    int blink_count;
    int remaining_blinks;
    bool is_on;
    bool in_pause;
    unsigned long cycle_end_time;

public:
    BlinkLedAnimation(Led* led_ref, int initial_blink_count = 0);

    void setBlinkCount(int count); // New setter function
    int getBlinkCount() const;     // Optional getter function

    void update() override;
};

#endif /* BLINK_LED_ANIMATION_H */
