#include "power_management.h"

PowerManagement::PowerManagement(Led* led_ref, uint8_t pinToMosfet) 
    : led(led_ref), pin(pinToMosfet), startup_start_time(0), state(LOW_POWER_STATE), rainbow(led) {}

void PowerManagement::transitionTo(DeviceState new_state) {
   switch (new_state) {
        case LOW_POWER_STATE:
            resetError();
            led->clearAnimation();
            digitalWrite(pin, LOW);
            state = LOW_POWER_STATE;
            DEBUG("[PM] Device state transitioned to LOW_POWER_STATE");
            break;

        case STARTUP_STATE:
            digitalWrite(pin, HIGH); /* Turn on power to rpi */
            startup_start_time = millis();
            led->setAnimation(&rainbow);
            state = STARTUP_STATE;
            DEBUG("[PM] Device state transitioned to STARTUP_STATE");
            break;

        case READY_STATE:
            led->clearAnimation(); 
            led->setHue(LED_HUE_GREEN);  /* Green */
            state = READY_STATE;
            DEBUG("[PM] Device state transitioned to READY_STATE");
            break;

        case RECORDING_STATE:
            Serial.println("START_RECORD");
            led->setHue(LED_HUE_WHITE);  /* White */
            led->setSat(0);
            state = RECORDING_STATE;
            DEBUG("[PM] Device state transitioned to RECORDING_STATE");
            break;

        case SHUTDOWN_REQUEST_STATE:
            Serial.println("SHUTDOWN_REQUEST");
            led->setAnimation(&rainbow);
            state = SHUTDOWN_STATE;
            DEBUG("[PM] Device state transitioned to SHUTDOWN_REQUEST_STATE");
            break;

        case SHUTDOWN_REQ_ACK_STATE:
            DEBUG("[PM] RPI shutting down. Waiting for UART to go LOW");
            state = SHUTDOWN_REQ_ACK_STATE;
            break;

        case ERROR_STATE:
            // call throwError first: errorHandler.throwError(4);
            DEBUG("[PM] Device state transitioned to ERROR_STATE");
            state = ERROR_STATE;
            break;
    }
}

DeviceState PowerManagement::currentState(){
    return state;
}

unsigned long PowerManagement::startTime() {
  return startup_start_time;
}