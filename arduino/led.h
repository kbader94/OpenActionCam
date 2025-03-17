#ifndef LED_H
#define LED_H

#define LED_HUE_RED       (0)                     // 0°   → 0
#define LED_HUE_ORANGE    ((65535 * 32) / 360)    // 32°  → ~5825
#define LED_HUE_YELLOW    ((65535 * 60) / 360)    // 60°  → ~10922
#define LED_HUE_GREEN     ((65535 * 120) / 360)   // 120° → ~21845
#define LED_HUE_CYAN      ((65535 * 180) / 360)   // 180° → ~32767
#define LED_HUE_BLUE      ((65535 * 240) / 360)   // 240° → ~43690
#define LED_HUE_MAGENTA   ((65535 * 300) / 360)   // 300° → ~54612
#define LED_HUE_PURPLE    ((65535 * 270) / 360)   // 270° → ~49151
#define LED_HUE_PINK      ((65535 * 350) / 360)   // 350° → ~63657
#define LED_HUE_WHITE     (0)                     /* White is full sat+val */

#include <Adafruit_NeoPixel.h>
#include "led_animation.h"

class Led {
private:
    Adafruit_NeoPixel* led_strip;
    uint8_t index;
    uint16_t hue;
    uint8_t sat;
    uint8_t val;
    LedAnimation* animation;

public:
    Led(Adafruit_NeoPixel* strip, uint8_t led_index);
    void setHue(uint16_t hue);
    void setSat(uint8_t sat);
    void setVal(uint8_t val);
    void setAnimation(LedAnimation* anim);
    void off();
    void fullWhite();
    void clearAnimation();
    void update();
};

#endif /* LED_H */
