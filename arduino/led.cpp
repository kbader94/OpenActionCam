#include "led.h"

Led::Led(Adafruit_NeoPixel* strip, uint8_t led_index)
    : led_strip(strip), index(led_index), hue(0x0000), sat(0xFF), val(0xFF), animation(nullptr) {}

void Led::setHue(uint16_t h) {
    hue = h;
    sat = 255;
    val = 255;
}

void Led::setSat(uint8_t s) {
    sat = s;
}

void Led::setVal(uint8_t v) {
    val = v;
}

void Led::off() {
  val = 0;
  animation = nullptr;
}

void Led::fullWhite() {
  sat = 255;
  val = 255;
}

void Led::setAnimation(LedAnimation* anim) {
    animation = anim;
}

void Led::clearAnimation() {  
    animation = nullptr;
}

void Led::update() {
    if (animation) animation->update();
    led_strip->setPixelColor(index, led_strip->ColorHSV(hue, sat, val));
    led_strip->show();
}
