#include "rainbow_led_animation.h"

RainbowLedAnimation::RainbowLedAnimation(Led* led_ref) 
    : led(led_ref){
      hue = 0;
      last_update = millis();
    }

void RainbowLedAnimation::update() {
    if (millis() - last_update >= 50) {
        last_update = millis();

        hue += 300;
        if (hue >= 65535) {
            hue = 0;
        }

        led->setHue(hue);
    }
}
