#include "power_management.h"

PowerManagement::PowerManagement(Led* led_ref, uint8_t pinToMosfet) 
    : led(led_ref), power_control_pin(pinToMosfet), startup_start_time(0), state(LOW_POWER_STATE), rainbow(led) {}

    void PowerManagement::transitionTo(DeviceState new_state) {
        switch (new_state) {
             case LOW_POWER_STATE:
                 Serial.begin(9600); // TODO: remove for true low power, this is kept for dev purposes(mayhaps we need a #define)
                 reset_error();
                 led->clearAnimation();
                 led->setVal(0);
                 digitalWrite(power_control_pin, LOW);
                 state = LOW_POWER_STATE;
                 DEBUG_MESSAGE("[PM] Device state transitioned to LOW_POWER_STATE");
                 break;
     
             case STARTUP_STATE:
                 Serial.begin(9600);
                 digitalWrite(power_control_pin, HIGH); /* Turn on power to rpi */
                 startup_start_time = millis();
                 led->setAnimation(&rainbow);
                 state = STARTUP_STATE;
                 DEBUG_MESSAGE("[PM] Device state transitioned to STARTUP_STATE");
                 break;
     
             case READY_STATE:
                 led->clearAnimation(); 
                 led->setHue(LED_HUE_GREEN);  /* Green */
                 if (state == RECORDING_STATE) {
                     comms_send_command(COMMAND_RECORD_REQ_END);
                 }
                 state = READY_STATE;
                 DEBUG_MESSAGE("[PM] Device state transitioned to READY_STATE");
                 break;
     
             case RECORDING_STATE:
                 comms_send_command(COMMAND_RECORD_REQ_START);
                 led->setHue(LED_HUE_WHITE);  /* White */
                 led->setSat(0);
                 state = RECORDING_STATE;
                 DEBUG_MESSAGE("[PM] Device state transitioned to RECORDING_STATE");
                 break;
     
             case SHUTDOWN_REQUEST_STATE:
                 comms_send_command(COMMAND_SHUTDOWN_REQ);
                 led->setAnimation(&rainbow);
                 shutdown_request_sent_time = millis();
                 state = SHUTDOWN_REQUEST_STATE;
                 DEBUG_MESSAGE("[PM] Device state transitioned to SHUTDOWN_REQUEST_STATE");
                 break;
     
             case SHUTDOWN_STATE:
                 DEBUG_MESSAGE("[PM] RPi is shutting down. Waiting for UART idle");
                 pi_shutting_down = true;
                 shutdown_req_ack_rec_time = millis();
                 DEBUG_MESSAGE("[PM] Disabling Serial to listen for UART idle.");
                 Serial.end();
                 /* TODO: huh? we aren't currently waiting for shutdown(detect via tx lvl) 
                  * We should call waitForShutdown() here
                  */
                 pinMode(PIN_RX, INPUT_PULLUP);
                 state = SHUTDOWN_STATE;
                 break;
     
             case ERROR_STATE:
                 DEBUG_MESSAGE("[PM] Device state transitioned to ERROR_STATE");
                 state = ERROR_STATE;
                 break;
         }
     }

/*
 * PowerManagement::waitForShutdown()
 * NOTE: This is a blocking function.
 * returns: True if the RPI successfully shut down (UART RX remains LOW for > 5 seconds). 
 *          False otherwise.
 */
bool PowerManagement::waitForShutdown() {
    unsigned long shutdownStartTime = millis();
    unsigned long rxLowStartTime = 0;
    bool rxHasBeenLow = false;

    while (millis() - shutdownStartTime < SHUTDOWN_TIMEOUT) {
        if (digitalRead(PIN_RX) == HIGH) {
            // Reset if TX goes HIGH again
            rxLowStartTime = 0;
            rxHasBeenLow = false;
        } else {
            // If TX has gone LOW, start the confirmation timer
            if (!rxHasBeenLow) {
                rxHasBeenLow = true;
                rxLowStartTime = millis();
            }

            // If TX has been LOW for 5 seconds, confirm shutdown
            if (millis() - rxLowStartTime >= CONFIRM_SHUTDOWN_TIME) {

                return true;
            }
        }
    }

    // If we reach here, shutdown never happened TODO: reactivate serial, output error, wait, and THEN kill power?
    WARN("[PM] Shutdown timeout reached!");
    return false;
}

DeviceState PowerManagement::currentState(){
    return state;
}

unsigned long PowerManagement::startTime() {
  return startup_start_time;
}

unsigned long PowerManagement::shutdownRequestTime() {
  return shutdown_request_sent_time;
}