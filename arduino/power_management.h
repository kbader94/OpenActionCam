#ifndef POWER_MANAGEMENT_H
#define POWER_MANAGEMENT_H

#define SHUTDOWN_TIMEOUT 30000  // 30 seconds max wait time
#define CONFIRM_SHUTDOWN_TIME 5000  // 5 seconds for TX to stay LOW
#define PIN_RX 0 // ATmega physical pin 2, arduino pin 0

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
    uint8_t power_control_pin; /* Pin to gate on Power Mosfet */
    RainbowLedAnimation rainbow;
    unsigned long startup_start_time;
    bool pi_shutting_down;
    unsigned long shutdown_req_ack_rec_time = 0; // when the shutdown request acknowledgment was received
    unsigned long shutdown_request_sent_time = 0; // When the shutdown request was sent

public:
    PowerManagement(Led* led_ref, uint8_t pinToMosfet);
    void transitionTo(DeviceState state);
    bool waitForShutdown(); 
    unsigned long startTime();
    unsigned long shutdownRequestTime();
    DeviceState currentState();

};

#endif /* POWER_MANAGEMENT_H */