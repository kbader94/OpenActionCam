#include "system_state.h"
#include <Arduino.h>
#include "comms.h"

SystemStateManager::SystemStateManager(Led* led_ref, uint8_t pinToMosfet)
    : led(led_ref),
      power_control_pin(pinToMosfet),
      startup_start_time(0),
      shutdown_start_time(0),
      state(LOW_POWER_STATE),
      rainbow(led) {}

void SystemStateManager::processStateTransition(unsigned long button_press_duration, const struct Message *msg)
{
    switch (state)
    {
    case LOW_POWER_STATE:
        if (button_press_duration > 0)
            transitionTo(STARTUP_STATE);
        break;

    case STARTUP_STATE:
        if (millis() - startup_start_time > STARTUP_TIMEOUT)
            ERROR(ERR_NO_COMM_RPI);

        if (msg && msg->header.message_type == MESSAGE_TYPE_COMMAND &&
            msg->body.payload_command.command == OAC_COMMAND_WD_KICK)
            transitionTo(READY_STATE);
        break;

    case READY_STATE:
        if (msg->header.message_type == MESSAGE_TYPE_COMMAND &&
            msg->body.payload_command.command == COMMAND_SHUTDOWN_STARTED)
        {
            transitionTo(SHUTDOWN_STATE);
        }   
        break;

    case SHUTDOWN_STATE:
        if (waitForShutdown())
            transitionTo(LOW_POWER_STATE);
        else
            ERROR(ERR_RPI_SHUTDOWN_TIMEOUT);
        break;

    case ERROR_STATE:
        if (button_press_duration > 0)
            reset_error();
        break;
    }
}

void SystemStateManager::transitionTo(SystemState new_state) {
    switch (new_state) {
        case LOW_POWER_STATE:
            Serial.begin(9600); /* DEBUG output only. TODO: implement dev mode */ 
            reset_error();
            led->clearAnimation();
            led->setVal(0);
            digitalWrite(power_control_pin, LOW);
            state = LOW_POWER_STATE;
            DEBUG_MESSAGE("[SYS] Transition to LOW_POWER_STATE");
            break;

        case STARTUP_STATE:
            Serial.begin(9600);
            digitalWrite(power_control_pin, HIGH);
            startup_start_time = millis();
            led->setAnimation(&rainbow);
            state = STARTUP_STATE;
            DEBUG_MESSAGE("[SYS] Transition to STARTUP_STATE");
            break;

        case READY_STATE:
            led->clearAnimation();
            led->setHue(LED_HUE_GREEN);
            state = READY_STATE;
            DEBUG_MESSAGE("[SYS] Transition to READY_STATE");
            break;

        case SHUTDOWN_STATE:
            DEBUG_MESSAGE("[SYS] Transition to SHUTDOWN_STATE, waiting for UART idle...");
            shutdown_start_time = millis();
            Serial.end(); /* Stop serial to listen on TX line */ 
            pinMode(PIN_RX, INPUT_PULLUP);
            led->setAnimation(&rainbow);
            state = SHUTDOWN_STATE;
            /* TODO: waitForShutdown() but async*/
            break;

        case ERROR_STATE:
            state = ERROR_STATE;
            DEBUG_MESSAGE("[SYS] Transition to ERROR_STATE");
            break;
    }
}

bool SystemStateManager::waitForShutdown() {
    unsigned long start = millis();
    unsigned long rxLowStart = 0;
    bool rxLowSeen = false;

    while (millis() - start < SHUTDOWN_TIMEOUT) {
        if (digitalRead(PIN_RX) == HIGH) {
            rxLowStart = 0;
            rxLowSeen = false;
        } else {
            if (!rxLowSeen) {
                rxLowSeen = true;
                rxLowStart = millis();
            }

            if (millis() - rxLowStart >= CONFIRM_SHUTDOWN_TIME) {
                return true;
            }
        }
    }

    WARN("[SYS] Shutdown timeout exceeded.");
    return false;
}

unsigned long SystemStateManager::startTime() {
    return startup_start_time;
}

SystemState SystemStateManager::currentState() {
    return state;
}