#include "blink_led_animation.h"

BlinkLedAnimation::BlinkLedAnimation(Led* led_ref, int blinks)
    : led(led_ref), blink_count(blinks), remaining_blinks(blinks), 
      last_update(0), is_on(false) {

        led->setVal(0);
        led->setHue(0); // red

      }

void BlinkLedAnimation::update() {
    unsigned long now = millis();

    if (in_pause) {
        // Wait for the pause duration before restarting the blink cycle
        if (now - cycle_end_time >= pause_duration) {
            in_pause = false;
            remaining_blinks = blink_count;  // Reset blink counter
        }
        return; // Exit early to avoid executing the blinking logic
    }

    if (now - last_update >= blink_interval) {
        last_update = now;
        is_on = !is_on;
        led->setVal(is_on ? 255 : 0);

        if (remaining_blinks == 0) {
            led->setVal(0);
            in_pause = true;
            cycle_end_time = millis(); // Mark pause start time
        }

        if (is_on) remaining_blinks--;
    }
}
