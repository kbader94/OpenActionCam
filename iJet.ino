#include <Adafruit_NeoPixel.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include "led.h"
#include "error.h"
#include "rainbow_led_animation.h"
#include "power_management.h"

#define BUTTON_PIN        2  /* Button on PD2 (INT0), atmega328 physical pin 4, AKA arduino pin 2 */
#define STAT_LED_PIN      3  /* WS2812 LED strip on PD3, atmega328 physical pin 5, AKA arduino pin 3 */
#define POWER_PIN         4  /* MOSFET control (PD4), atmega328 physical pin 6, AKA arduino pin 4 */

#define DEBOUNCE_DELAY    50   /* Button debounce delay (ms) */
#define LONG_PRESS_TIME   1000  /* Long press threshold (ms) */
#define STARTUP_TIMEOUT   30000  /* 30 second startup timeout */

Adafruit_NeoPixel led_strip(1, STAT_LED_PIN, NEO_GRB + NEO_KHZ800);
Led led(&led_strip, 0);
PowerManagement power_management(&led, POWER_PIN); 

volatile unsigned long button_press_start = 0;  // Stores when the button was pressed
volatile unsigned long button_press_duration = 0;  // Stores how long it was held
volatile bool button_pressed = false;

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
 * Handle button press - output press duration.
 */
unsigned long handle_button_press(void) {
    if (button_pressed) {

        button_pressed = false; // Reset flag
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
    initErrorSystem(&led, &power_management);

    /* init pins */
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(POWER_PIN, OUTPUT);

    Serial.begin(9600);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button_isr, CHANGE);
    
    /* init led strip */
    led_strip.begin();
    led_strip.setBrightness(8);
    led.setVal(0); // turn off LED
    led_strip.show();
    
    sei();

}

void loop(void) {

    const char* serial_message = check_serial();
    unsigned long press_duration = handle_button_press();

    switch (power_management.currentState()) {
        case LOW_POWER_STATE:
            if (press_duration > 0) { /* Transition to Startup if button is pressed */
                power_management.transitionTo(STARTUP_STATE);
            }
            break;

        case STARTUP_STATE:
            if (millis() - power_management.startTime() > STARTUP_TIMEOUT){
                ERROR(ERR_NO_COMM_RPI);  /* No communication from RPi */
            }
            if (serial_message && serial_message[0] != '\0') {
                Serial.println((uint8_t)serial_message[0]);
            } else {
               // Serial.println("String is empty.");
            }

            if (strcmp(serial_message, "BOOT") == 0) {
                power_management.transitionTo(READY_STATE);
            }
            break;

        case READY_STATE:
            if (press_duration > 0 && press_duration < LONG_PRESS_TIME) {
                power_management.transitionTo(RECORDING_STATE);
            } else if (press_duration >= LONG_PRESS_TIME) {
                power_management.transitionTo(SHUTDOWN_STATE);
            }
            break;

        case RECORDING_STATE:
            if (press_duration > 0 && press_duration < LONG_PRESS_TIME) {
                power_management.transitionTo(READY_STATE);
            } else if (press_duration >= LONG_PRESS_TIME) {
                power_management.transitionTo(SHUTDOWN_STATE);
            }
            break;

        case SHUTDOWN_STATE:
            if (strcmp(serial_message, "SHUTDOWN_ACK") == 0) {
                power_management.transitionTo(SHUTDOWN_SUCCESS_STATE);
            }
            break;

        case SHUTDOWN_SUCCESS_STATE:
            /* TODO: implement async timer and transition to LOW_POWER */
            break;

        case ERROR_STATE:
            if (press_duration > 0) {
                resetError();
            }
            break;
    }

    led.update();

   // if (state == LOW_POWER)
  //      enter_sleep_mode();
}
