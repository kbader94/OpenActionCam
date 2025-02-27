#include <Adafruit_NeoPixel.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include "led.h"
#include "error.h"
#include "rainbow_led_animation.h"

#define BUTTON_PIN        2  /* Button on PD2 (INT0), physical pin 4, arduino pin 2 */
#define STAT_LED_PIN      3  /* WS2812 LED strip on PD3, physical pin 5, arduino pin 3*/
#define POWER_PIN         4  /* MOSFET control (PD4), physical pin 6, arduino pin 4 */

#define DEBOUNCE_DELAY    50   /* Button debounce delay (ms) */
#define LONG_PRESS_TIME   1000  /* Long press threshold (ms) */
#define STARTUP_TIMEOUT   3000  /* 5 minutes startup timeout */

Adafruit_NeoPixel led_strip(1, STAT_LED_PIN, NEO_GRB + NEO_KHZ800);
Led led(&led_strip, 0);
ErrorHandler errorHandler(&led);
RainbowLedAnimation rainbow(&led);

volatile unsigned long button_press_start = 0;  // Stores when the button was pressed
volatile unsigned long button_press_duration = 0;  // Stores how long it was held
volatile bool button_pressed = false;
unsigned long startup_start_time;

enum DeviceState {
    LOW_POWER,
    STARTUP,
    READY,
    RECORDING,
    SHUTDOWN,
    ERROR
};
DeviceState state = LOW_POWER;

/*
 * Interrupt handler for button press.
 */
void button_isr(void) {
    if (digitalRead(BUTTON_PIN) == LOW) {
        // Button just pressed (falling edge)
        button_press_start = millis();
        button_pressed = false; // Reset state
    } else {
        // Button just released (rising edge)
        button_press_duration = millis() - button_press_start;
        button_pressed = true; // Mark press as handled
    }
}

/*
 * Handles button press duration.
 */
unsigned long handle_button_press(void) {
    if (button_pressed) {
        button_pressed = false; // Reset flag
        Serial.print("Button held for: ");
        Serial.print(button_press_duration);
        Serial.println(" ms");

        if (button_press_duration < LONG_PRESS_TIME) {
            Serial.println("Short Press Detected!");
            // Handle short press
        } else {
            Serial.println("Long Press Detected!");
            // Handle long press
        }

        return button_press_duration; // Return duration
    }
    return 0;  // No new button press detected
}


/*
 * Enters low power mode.
 */
void enter_sleep_mode(void) {
    // led.setColor(0x00000000); /*  */ 
    // led.update();

    // attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button_isr, FALLING);

    // set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    // sleep_enable();
    // sleep_cpu();  /* MCU sleeps here */

    // sleep_disable();
}

/*
 * Handles Serial Input.
 */
const char* check_serial(void) {
    static char serial_buffer[32];
    static uint8_t serial_index = 0;

    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n') {
            serial_buffer[serial_index] = '\0';
            serial_index = 0;
            return serial_buffer;
        }
        if (serial_index < sizeof(serial_buffer) - 1)
            serial_buffer[serial_index++] = c;
    }
    return "";
}



void setup(void) {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(POWER_PIN, OUTPUT);

    Serial.begin(9600);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button_isr, CHANGE);
    
    led_strip.begin();
    led_strip.setBrightness(8);
    led.setVal(0); // turn off LED
    led_strip.show();
    

    sei();
}

void loop(void) {

    const char* serial_message = check_serial();
    unsigned long press_duration = handle_button_press();

    switch (state) {
        case LOW_POWER:
            if (press_duration > 0) {
                digitalWrite(POWER_PIN, HIGH);
                startup_start_time = millis();
                state = STARTUP;
            }
            break;

        case STARTUP:
            led.setAnimation(&rainbow); 
            if (millis() - startup_start_time > STARTUP_TIMEOUT)
                errorHandler.throwError(4);  /* No communication from RPi */

            if (strcmp(serial_message, "BOOT") == 0) {
                state = READY;
                led.clearAnimation(); 
                led.setHue(LED_HUE_GREEN);  /* Green */
            }
            break;

        case READY:
            if (press_duration > 0 && press_duration < LONG_PRESS_TIME) {
                Serial.println("START_RECORD");
                state = RECORDING;
                led.setHue(LED_HUE_WHITE);  /* White */
                led.setSat(0);
            } else if (press_duration >= LONG_PRESS_TIME) {
                Serial.println("SHUTDOWN_REQUEST");
                Serial.print("Press: ");
                Serial.println(press_duration);
                led.setAnimation(&rainbow); 
                state = SHUTDOWN;
            }
            break;

        case RECORDING:
            if (press_duration > 0 && press_duration < LONG_PRESS_TIME) {
                Serial.println("STOP_RECORD");
                state = READY;
                led.setHue(LED_HUE_GREEN);  /* Green */
            } else if (press_duration >= LONG_PRESS_TIME) {
                Serial.println("SHUTDOWN_REQUEST");
                led.setAnimation(&rainbow);
                state = SHUTDOWN;
            }
            break;

        case SHUTDOWN:
            if (strcmp(serial_message, "SHUTDOWN_ACK") == 0) {
                delay(5000);
                digitalWrite(POWER_PIN, LOW);
                state = LOW_POWER;
            }
            break;

        case ERROR:
            if (press_duration > 0) {
                state = LOW_POWER;
            }
            break;
    }

    led.update();

   // if (state == LOW_POWER)
  //      enter_sleep_mode();
}
