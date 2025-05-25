#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include "led.h"
#include "led_animation.h"
#include "rainbow_led_animation.h"
#include "error.h"

#define STARTUP_TIMEOUT 30000   /* 30 second startup timeout */
#define SHUTDOWN_TIMEOUT 30000         /* 30 seconds max wait time */ 
#define CONFIRM_SHUTDOWN_TIME 5000     /* 5 seconds for TX to stay LOW */
#define PIN_RX 0                       /* ATmega physical pin 2, Arduino pin 0 */

enum SystemState {
    LOW_POWER_STATE,
    STARTUP_STATE,
    READY_STATE,
    SHUTDOWN_STATE,
    ERROR_STATE
};

class SystemStateManager {
private:
    SystemState state;
    Led* led;
    uint8_t power_control_pin;
    RainbowLedAnimation rainbow;
    unsigned long startup_start_time;
    unsigned long shutdown_start_time;


public:
    SystemStateManager(Led* led_ref, uint8_t pinToMosfet);
    void transitionTo(SystemState new_state);
    void processStateTransition(unsigned long button_press_duration, const struct Message *msg);
    bool waitForShutdown();
    unsigned long startTime();
    SystemState currentState();
};

#endif /* SYSTEM_STATE_H */