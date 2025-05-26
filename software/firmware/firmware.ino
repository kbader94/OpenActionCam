#include <Adafruit_NeoPixel.h>

#include <avr/wdt.h> 
#include <avr/power.h>  
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "led.h"
#include "error.h"
#include "rainbow_led_animation.h"
#include "system_state.h"

#define BUTTON_PIN 2   /* Button on PD2 (INT0), atmega328 physical pin 4, AKA arduino digital 2 */
#define STAT_LED_PIN 3 /* WS2812 LED strip on PD3, atmega328 physical pin 5, AKA arduino digial 3 */
#define POWER_PIN 4    /* MOSFET control on PD4, atmega328 physical pin 6, AKA arduino digital 4 */
#define CHRG_STAT 5    /* Charger status on PD5, atmega physical pin 9, AKA arduino digital 5*/
#define PI_INT 7       /* RPI Interrupt on PD7, atmega328 physical pin 13, AKA arduino digital 7 */
#define LED_EN 9       /* Enable LED on PB1, atmega physical pin 15, AKA arduino digital 9 */

#define DEBOUNCE_DELAY 50       /* Button debounce delay (ms) */
#define LONG_PRESS_TIME 1000    /* Long press threshold (ms) */
#define STATUS_INTERVAL_MS 500  /* Interval between status messages */ 

#define BATTERY_SAMPLES 8                   /* ADC re-samples */
#define BATTERY_MIN_UV  6000000
#define BATTERY_MAX_UV  8450000

#define CHARGER_MAX_FAULT_EDGES 4
#define CHARGER_FAULT_INTERVAL_MIN_MS 400
#define CHARGER_FAULT_INTERVAL_MAX_MS 600

volatile bool charging = false;
volatile bool charger_blink_edge_detected = false;
volatile unsigned long charger_blink_timestamp = 0;
unsigned long charger_fault_edge_times[CHARGER_MAX_FAULT_EDGES];
uint8_t charger_fault_edge_index = 0;
bool charger_fault = false;
volatile bool charger_stat_last_state = HIGH;

volatile unsigned long button_press_start = 0;    /* Stores when the button was pressed */ 
volatile unsigned long button_press_duration = 0; /* Stores how long it was held */
volatile bool button_pressed = false;

Adafruit_NeoPixel led_strip(1, STAT_LED_PIN, NEO_GRB + NEO_KHZ800);
Led led(&led_strip, 0);
SystemStateManager system_state(&led, POWER_PIN);

/* 
 * Interrupt handler for charger status
 * 
 */
ISR(PCINT2_vect)
{
    bool current_state = digitalRead(CHRG_STAT);
    charging = (current_state == LOW);
    
    if (current_state != charger_stat_last_state) {
        charger_blink_edge_detected = true;
        charger_blink_timestamp = millis();
    }
    
    charger_stat_last_state = current_state;
    
}

void process_charger_status(void)
{
    if (!charger_blink_edge_detected)
        return;

    charger_blink_edge_detected = false;

    charger_fault_edge_times[charger_fault_edge_index % CHARGER_MAX_FAULT_EDGES] = charger_blink_timestamp;
    charger_fault_edge_index++;

    if (charger_fault_edge_index >= CHARGER_MAX_FAULT_EDGES) {
        bool pattern_valid = true;

        for (uint8_t i = 1; i < CHARGER_MAX_FAULT_EDGES; i++) {
            unsigned long dt = charger_fault_edge_times[i] - charger_fault_edge_times[i - 1];
            if (dt < CHARGER_FAULT_INTERVAL_MIN_MS || dt > CHARGER_FAULT_INTERVAL_MAX_MS) {
                pattern_valid = false;
                break;
            }
        }

        if (pattern_valid) {
            charger_fault = true;
        }
    }
}

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
 * read_battery_voltage - Output battery voltage in microvolts
 * @note: to read battery voltage, we perform a series of ADC samples, average the samples
 * and adjust for the voltage divider.
 * @return battery voltage in microvolts 
 */
uint32_t read_battery_voltage(void)
{
    const uint32_t VREF_UV = 5000000;            /* 5.000 V reference */ 
    const uint16_t ADC_MAX = 1023;               /* 10-bit ADC */ 
    const uint16_t DIVIDER_NUM = 25;             /* Voltage divider: R1 + R2 */
    const uint16_t DIVIDER_DEN = 15;             /* Voltage divider: R2 */
    const uint32_t OFFSET_UV = 150000;           /* Offset = 0.15 V = 150000 µV Note: This should be further verified in HW on future revisions */

    uint16_t sum = 0;

    /* Get ADC samples */
    for (uint8_t i = 0; i < BATTERY_SAMPLES; i++) {
        ADMUX = (1 << REFS0) | (1 << MUX1) | (1 << MUX0); /* PC3 / A3 */ 
        ADCSRA |= (1 << ADSC);
        while (ADCSRA & (1 << ADSC));
        sum += ADC;
    }

    /* Average samples */
    uint16_t avg_raw = sum / BATTERY_SAMPLES;

    /* Convert to microvolts */
    uint32_t vout_uv = ((uint32_t)avg_raw * VREF_UV) / ADC_MAX;

    /* Adjust for voltage divider */
    uint32_t vin_uv = ((uint64_t)vout_uv * DIVIDER_NUM) / DIVIDER_DEN;
    vin_uv += OFFSET_UV;

    return vin_uv; /* return voltage in microvolts */
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
            .bat_volt_uv = read_battery_voltage(),
            .state = system_state.currentState(),
            .charging = charging, 
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
    init_error_system(&led, &system_state);

    /* init pins */
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(POWER_PIN, OUTPUT);
    pinMode(CHRG_STAT, INPUT_PULLUP);

    /* button interrupt */
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button_isr, CHANGE);

    /* charger status interrupt*/
    PCICR |= (1 << PCIE2);      /* Enable PCINT[23:16] */
    PCMSK2 |= (1 << PCINT21);   /* Enable interrupt on PD5 (digital 5 / PCINT21) */
    charging = digitalRead(CHRG_STAT) == LOW;


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

    /* Detect charger status*/
    process_charger_status();
    if (charger_fault) {
        ERROR(ERR_CHARGER_FAULT);
    }

    /* Receive serial messages */
    struct Message msg = {0};
    int err = comms_receive_message(&msg);
    if(err < 0) WARN("Error receiving message: ");
    
    /* Get button press duration */
    unsigned long button_press_duration = handle_button_press();

    /* Check system conditions and transition states if necessary */
    system_state.processStateTransition(button_press_duration, &msg);

    /* Handle Ready State */
    if(system_state.currentState() == READY_STATE) {

        /* Check battery level 
        * NOTE: Our board uses mm3220H15NRH -  4.280V OVL, 2.800V UVL
        * So 8.56V is the Battery absolute OV level, and 5.6V is the absolute UV level.
        * In reality, the cells themselves are only rated for 8.4V max, with an assumed minimum of 6V.
        * TODO: async battery level reading: only read every few hundred ms. Note: battery level is 
        * subsequently read in transmit_status_message(). This can be optimized with one read on a timer.  
        */
        uint32_t batt_lvl = read_battery_voltage();
        if (batt_lvl < BATTERY_MAX_UV) ERROR(ERR_LOW_BATTERY);
        if (batt_lvl > BATTERY_MAX_UV) ERROR(ERR_BATTERY_OV);
            
        /* TODO: transmit button press duration */

        transmit_status_message();
    }

    
    /* Update led */
    led.update();

    /* Sleep - Disable peripherals and power down */
   // if (system_state.currentState() == LOW_POWER_STATE)
    //    sleep_until_button_press();
    
}
