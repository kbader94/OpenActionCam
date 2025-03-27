#include <Adafruit_NeoPixel.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include "led.h"
#include "error.h"
#include "rainbow_led_animation.h"
#include "power_management.h"

#define BUTTON_PIN 2   /* Button on PD2 (INT0), atmega328 physical pin 4, AKA arduino digital 2 */
#define STAT_LED_PIN 3 /* WS2812 LED strip on PD3, atmega328 physical pin 5, AKA arduino digial 3 */
#define POWER_PIN 4    /* MOSFET control on PD4, atmega328 physical pin 6, AKA arduino digital 4 */
#define PI_INT 7       /* RPI Interrupt on PD7, atmega328 physical pin 13, AKA arduino digital 7 */
#define LED_EN 9       /* Enable LED on PB1, atmega physical pin 15, AKA arduino digital 9 */

#define DEBOUNCE_DELAY 50     /* Button debounce delay (ms) */
#define LONG_PRESS_TIME 1000  /* Long press threshold (ms) */
#define STARTUP_TIMEOUT 30000 /* 30 second startup timeout */

Adafruit_NeoPixel led_strip(1, STAT_LED_PIN, NEO_GRB + NEO_KHZ800);
Led led(&led_strip, 0);
PowerManagement power_management(&led, POWER_PIN);

volatile unsigned long button_press_start = 0;    // Stores when the button was pressed
volatile unsigned long button_press_duration = 0; // Stores how long it was held
volatile bool button_pressed = false;

/*
 * Interrupt handler for button press.
 */
void button_isr(void)
{
    if (digitalRead(BUTTON_PIN) == LOW)
    {
        // Button just pressed (falling edge)
        button_press_start = millis();
        button_pressed = false; // Reset state
    }
    else
    {
        // Button just released (rising edge)
        button_press_duration = millis() - button_press_start;
        button_pressed = true; // Mark press as handled
    }
}

/*
 * Handle button press - output press duration.
 */
unsigned long handle_button_press(void)
{
    if (button_pressed)
    {
                             
        button_pressed = false;       // Reset flag
        return button_press_duration; // Return duration
    }
    return 0; // No new button press detected
}

/*
 * Setup sleep as power-down
 */
void setup_sleep_mode()
{   
    SMCR &= ~((1 << SM2) | (1 << SM1) | (1 << SM0));  // Clear mode bits
    SMCR |= (1 << SM1);  // Set Power-down mode (010)

    SMCR |= (1 << SE);   // Enable sleep
}

/*
 * Enter sleep, wait for interrupt, then wake up
 */
void enter_sleep()
{
    led.setVal(0);  /*  TODO: led.off() */
    led.update();

    sleep_cpu();         // Enter sleep (until interrupt)

    led.setVal(255);  /*  TODO: led.on() */
    led.update();
}

void setup(void)
{
    /* init serial comms */
    comms_init();

    /* init firmware error handler */
    init_error_system(&led, &power_management);

    /* init pins */
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(POWER_PIN, OUTPUT);

    /* button interrupt */
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button_isr, CHANGE);

    /* init led strip */
    led_strip.begin();
    led_strip.setBrightness(8);
    led.setVal(0); // turn off LED
    led_strip.show();

    /* init sleep mode as power-down and wake on interrupt INT0 */
    setup_sleep_mode();

    /* enable interrupts*/
    sei();
}

void loop(void)
{
    /* Receive serial messages */
    struct Message msg = {0};
    comms_receive_message(&msg);

    /* Process error messages */
    if (msg.header.message_type == MESSAGE_TYPE_ERROR)
    {
        throw_error(msg.body.payload_error.error_code, msg.body.payload_error.error_message);
    }
    
    unsigned long press_duration = handle_button_press();

    /* Handle events which trigger power state transitions */
    switch (power_management.currentState())
    {
    case LOW_POWER_STATE:
        if (press_duration > 0)
        {
            power_management.transitionTo(STARTUP_STATE);
        }
        break;

    case STARTUP_STATE:
        if (millis() - power_management.startTime() > STARTUP_TIMEOUT)
        {
            ERROR(ERR_NO_COMM_RPI);
        }

        if (msg.header.message_type == MESSAGE_TYPE_COMMAND &&
            msg.body.payload_command.command == COMMAND_BOOT)
        {
            power_management.transitionTo(READY_STATE);
        }
        break;

    case READY_STATE:
        if (press_duration > 0 && press_duration < LONG_PRESS_TIME)
        {
            power_management.transitionTo(RECORDING_STATE);
        }
        else if (press_duration >= LONG_PRESS_TIME)
        {
            power_management.transitionTo(SHUTDOWN_REQUEST_STATE);
        }
        break;

    case RECORDING_STATE:
        if (press_duration > 0 && press_duration < LONG_PRESS_TIME)
        {
            power_management.transitionTo(READY_STATE);
        }
        else if (press_duration >= LONG_PRESS_TIME)
        {
            power_management.transitionTo(SHUTDOWN_REQUEST_STATE);
        }
        break;

    case SHUTDOWN_REQUEST_STATE:
        if (millis() - power_management.shutdownRequestTime() > STARTUP_TIMEOUT)
        {
            ERROR(ERR_RPI_SHUTDOWN_REQ_TIMEOUT);
        }

        if (msg.header.message_type == MESSAGE_TYPE_COMMAND &&
            msg.body.payload_command.command == COMMAND_SHUTDOWN_REQ)
        {
            power_management.transitionTo(SHUTDOWN_STATE);
        }
        break;

    case SHUTDOWN_STATE:
        if (power_management.waitForShutdown())
        {
            power_management.transitionTo(LOW_POWER_STATE);
        }
        else
        {
            ERROR(ERR_RPI_SHUTDOWN_TIMEOUT);
        }
        break;

    case ERROR_STATE:
        if (press_duration > 0)
        {
            reset_error();
        }
        break;
    }

    led.update();

     if (power_management.currentState() == LOW_POWER_STATE)
         enter_sleep();
}
