#ifndef POWER_MANAGEMENT_H
#define POWER_MANAGEMENT_H

#include "led.h"
#include "led_animation.h"
#include "rainbow_led_animation.h"
#include "error.h"

enum DeviceState {
    LOW_POWER_STATE,
    STARTUP_STATE,
    READY_STATE,
    RECORDING_STATE,
    SHUTDOWN_REQUEST_STATE,
    SHUTDOWN_REQ_ACK_STATE,
    ERROR_STATE
};

class PowerManagement {
private:
    DeviceState state;
    Led* led;
    uint8_t pin; /* Pin to gate on Power Mosfet */
    RainbowLedAnimation rainbow;
    unsigned long startup_start_time;
public:
    PowerManagement(Led* led_ref, uint8_t pinToMosfet);
    void transitionTo(DeviceState state);
    unsigned long startTime();
    DeviceState currentState();

};

#endif /* POWER_MANAGEMENT_H */