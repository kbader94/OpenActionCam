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

#define DEBOUNCE_DELAY 50       /* Button debounce delay (ms) */
#define LONG_PRESS_TIME 1000    /* Long press threshold (ms) */
#define STARTUP_TIMEOUT 30000   /* 30 second startup timeout */
#define STATUS_INTERVAL_MS 500  /* Interval between status messages */ 

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
 * Handle button press
 * @return Button press duration in milliseconds
 */
unsigned long handle_button_press(void)
{
    if (button_pressed)
    {               
        button_pressed = false;       
        return button_press_duration; /* return duration in millis */
    }
    return 0;
}

/*
 * Enter sleep, wait for interrupt, then wake up
 */
void sleep_until_button_press()
{
    led.setVal(0);  /* TODO: led.off() */
    led.update();

    DEBUG_MESSAGE("Sleeping");

    cli();

    /* Disable watchdog, ADC, I2C, SPI, and UART */
    wdt_disable();                   
    ADCSRA &= ~(1 << ADEN);          
    power_twi_disable();            
    power_spi_disable();        
    power_usart0_disable();        

    /* Disable BOD during sleep */
    MCUCR |= (1 << BODS) | (1 << BODSE);
    MCUCR = (MCUCR & ~(1 << BODSE)) | (1 << BODS);

    sleep_enable();
    sei();
    sleep_cpu(); /* Sleep until interrupt (button press) */
    sleep_disable();

    /* Re-enable UART, SPI, I2C,and ADC */
    power_usart0_enable();  
    power_spi_enable();         
    power_twi_enable();            
    ADCSRA |= (1 << ADEN);     

    DEBUG_MESSAGE("Waking");

    led.setVal(255);  /* TODO: led.on() */
    led.update();
}

/*
 * read_battery_voltage - Reads battery voltage via analog input (PC3 / A3).
 * @return Actual battery voltage in volts (e.g., 7.42V)
 */
float read_battery_voltage(void)
{
    const float VREF = 5.0;                /* Reference voltage for ADC */ 
    const float ADC_RESOLUTION = 1023.0;   /* 10-bit ADC resolution */ 
    const float VOLTAGE_DIVIDER_RATIO = (10.0 + 15.0) / 15.0;  /* (R1 + R2) / R2 = 25k / 15k */ 

    /* Request ADC value */ 
    ADMUX = (1 << REFS0) | (1 << MUX1) | (1 << MUX0);
    ADCSRA |= (1 << ADSC);  

    while (ADCSRA & (1 << ADSC));  /* Wait for ADC value */
    uint16_t raw = ADC;  /* Get ADC value */ 

    /* Convert to battery voltage */
    float vout = (raw * VREF) / ADC_RESOLUTION;
    float vin = vout * VOLTAGE_DIVIDER_RATIO;
    return vin;
}

/* 
 * transmit_status_message - Periodically send a status to linux system via comms.
 */
void transmit_status_message(void)
{
    static unsigned long last_status_time = 0;
    unsigned long now = millis();

    if (now - last_status_time >= STATUS_INTERVAL_MS) {
        last_status_time = now;

        struct StatusBody status = {
            .battery_voltage = read_battery_voltage(),
            .state = power_management.currentState(),
            .charging = false,  /* TODO: implement charging detection */ 
            .error_code = get_current_error()
        };

        comms_send_status(&status);
    }
}

void setup(void)
{
    /* init serial messaging protocol */
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

    /* init sleep mode as power-down */
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);

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
    
    /* Get button press duration */
    unsigned long button_press_duration = handle_button_press();

    /* Handle events for current power state */
    switch (power_management.currentState())
    {
    case LOW_POWER_STATE:
        if (button_press_duration > 0)
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
        if (button_press_duration > 0 && button_press_duration < LONG_PRESS_TIME)
        {
            power_management.transitionTo(RECORDING_STATE);
        }
        else if (button_press_duration >= LONG_PRESS_TIME)
        {
            power_management.transitionTo(SHUTDOWN_REQUEST_STATE);
        }
        break;

    case RECORDING_STATE:
        if (button_press_duration > 0 && button_press_duration < LONG_PRESS_TIME)
        {
            power_management.transitionTo(READY_STATE);
        }
        else if (button_press_duration >= LONG_PRESS_TIME)
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
        if (button_press_duration > 0)
        {
            reset_error();
        }
        break;
    }

    /* Send current status */
    transmit_status_message();
    
    /* Update led */
    led.update();

    /* Sleep - Disable peripherals and power down */
    if (power_management.currentState() == LOW_POWER_STATE)
        sleep_until_button_press();
    
}
