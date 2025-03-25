#include "blink_led_animation.h"

BlinkLedAnimation::BlinkLedAnimation(Led* led_ref, int initial_blink_count)
    : led(led_ref), blink_count(initial_blink_count), remaining_blinks(initial_blink_count),
      last_update(0), is_on(false), in_pause(false), cycle_end_time(0) {

}

void BlinkLedAnimation::setBlinkCount(int count) {
    if (count <= 0) return; // Prevent invalid values
    blink_count = count;
    remaining_blinks = count;
    in_pause = false;         
}

int BlinkLedAnimation::getBlinkCount() const {
    return blink_count;
}

void BlinkLedAnimation::update() {
    unsigned long now = millis();

    if (in_pause) {
        // Wait for the pause duration before restarting the blink cycle
        if (now - cycle_end_time >= PAUSE_DURATION) {
            in_pause = false;
            remaining_blinks = blink_count;  // Reset blink counter
        }
        return; // Exit early to avoid executing the blinking logic
    }

    if (now - last_update >= BLINK_INTERVAL) {
        last_update = now;
        is_on = !is_on;
        led->setVal(is_on ? 255 : 0);

        if (remaining_blinks == 0) {
            led->setVal(0);
            in_pause = true;
            cycle_end_time = millis(); // Mark pause start time
        }

        if (is_on) remaining_blinks--; // Decrease count only on LED ON state
    }

}
