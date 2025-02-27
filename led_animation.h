#ifndef LED_ANIMATION_H
#define LED_ANIMATION_H

#include <Arduino.h>

class Led;

class LedAnimation {
public:
    virtual void update() = 0;
    virtual ~LedAnimation() {}
};

#endif /* LED_ANIMATION_H */
